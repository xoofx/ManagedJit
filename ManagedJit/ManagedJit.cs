using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace HackCoreCLR
{
    public class ManagedJit : IDisposable
    {
        // Used vtable indices for ICorJitCompiler
        private const int ICorJitCompiler_compileMethod_index = 0;
        private const int ICorJitCompiler_getVersionIdentifier_index = 4;

        // Used vtable indices for ICorJitInfo
        private const int ICorJitInfo_getMethodDefFromMethod_index = 105;
        private const int ICorJitInfo_getModuleAssembly_index = 43;

        private static readonly byte[] DelegateTrampolineCode = {
            // mov rax, 0000000000000000h ;Pointer address to _overrideCompileMethodPtr
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // jmp rax
            0xFF, 0xE0
        };

        // The expected version matching what is actually declared in corjit.h
        // https://github.com/dotnet/coreclr/blob/bb01fb0d954c957a36f3f8c7aad19657afc2ceda/src/inc/corinfo.h#L191-L221
        private static readonly Guid ExpectedJitVersion = new Guid("f00b3f49-ddd2-49be-ba43-6e49ffa66959");

        // A lock to have a single instance of our JIT
        private static readonly object JitLock;

        // A pointer to the original ICorJitCompiler::compileMethod
        private static readonly IntPtr DefaultCompileMethodPtr;
        // The C# delegate of ICorJitCompiler::compileMethod to allow to call it from C#
        private static readonly CompileMethodDelegate DefaultCompileMethod;

        // A pointer to the vtable of ICorJitCompiler
        private static readonly IntPtr JitVtable;
        // A pointer to the ICorJitCompiler instance (returned by getJit())
        private static ManagedJit ManagedJitInstance;

        // A Map storing an assembly handle with a managed assembly
        private static readonly Dictionary<IntPtr, Assembly> MapHandleToAssembly;

        // Our C# JIT CompileMethod exposed as a delegate that will be called by the vtable of ICorJitCompiler
        private CompileMethodDelegate _overrideCompileMethod;
        // The actual pointer of our JIT Compile method that we will setup on the ICorJitCompiler vtable to replace the previous JIT
        private IntPtr _overrideCompileMethodPtr;

        private bool _isDisposed;
        private bool _isHookInstalled;

        [ThreadStatic]
        private static CompileTls _compileTls;

        public static ManagedJit GetOrCreate()
        {
            lock (JitLock)
            {
                return ManagedJitInstance ?? (ManagedJitInstance = new ManagedJit());
            }
        }

        static ManagedJit()
        {
            JitLock = new object();
            MapHandleToAssembly = new Dictionary<IntPtr, Assembly>(IntPtrEqualityComparer.Instance);
            var process = Process.GetCurrentProcess();
            foreach (ProcessModule module in process.Modules)
            {
                if (Path.GetFileName(module.FileName) == "clrjit.dll")
                {
                    // This is the address of ICorJitCompiler
                    // https://github.com/dotnet/coreclr/blob/bb01fb0d954c957a36f3f8c7aad19657afc2ceda/src/inc/corjit.h#L391-L445
                    var jitAddress = GetProcAddress(module.BaseAddress, "getJit");
                    if (jitAddress != IntPtr.Zero)
                    {
                        var getJit = (GetJitDelegate) Marshal.GetDelegateForFunctionPointer(jitAddress, typeof(GetJitDelegate));
                        var jit = getJit();
                        if (jit != IntPtr.Zero)
                        {
                            JitVtable = Marshal.ReadIntPtr(jit);

                            // Check JitVersion
                            var getVersionIdentifierPtr = Marshal.ReadIntPtr(JitVtable, IntPtr.Size * ICorJitCompiler_getVersionIdentifier_index);
                            var getVersionIdentifier = (GetVersionIdentifierDelegate)Marshal.GetDelegateForFunctionPointer(getVersionIdentifierPtr, typeof(GetVersionIdentifierDelegate));
                            getVersionIdentifier(jitAddress, out var version);
                            if (version != ExpectedJitVersion)
                            {
                                return;
                            }

                            // If version, ok, get CompileMethod
                            DefaultCompileMethodPtr = Marshal.ReadIntPtr(JitVtable, IntPtr.Size * ICorJitCompiler_compileMethod_index);
                            DefaultCompileMethod = (CompileMethodDelegate) Marshal.GetDelegateForFunctionPointer(DefaultCompileMethodPtr, typeof(CompileMethodDelegate));
                        }
                    }
                    break;
                }
            }
        }

        private ManagedJit()
        {
            if (DefaultCompileMethod != null)
            {
                // 1) Converts a reference to our compile method to a `CompileMethodDelegate`
                _overrideCompileMethod = CompileMethod;
                _overrideCompileMethodPtr = Marshal.GetFunctionPointerForDelegate(_overrideCompileMethod);

                // 2) Build a trampoline that will allow to simulate a call from native to our delegate
                var trampolinePtr = AllocateTrampoline(_overrideCompileMethodPtr);
                var trampoline = (CompileMethodDelegate)Marshal.GetDelegateForFunctionPointer(trampolinePtr, typeof(CompileMethodDelegate));

                // 3) Call our trampoline
                IntPtr value;
                int size;
                var emptyInfo = default(CORINFO_METHOD_INFO);
                trampoline(IntPtr.Zero, IntPtr.Zero, ref emptyInfo, 0, out value, out size);
                FreeTrampoline(trampolinePtr);

                // 4) Once our `CompileMethodDelegate` can be accessible from native code, we can install it
                InstallManagedJit(_overrideCompileMethodPtr);
                _isHookInstalled = true;
            }
        }

        public bool IsActive => _overrideCompileMethod != null;

        private static void InstallManagedJit(IntPtr compileMethodPtr)
        {
            // We need to unprotect the JitVtable as it is by default not read-write
            // It is usually a C++ VTable generated at compile time and placed into a read-only section in the shared library
            VirtualProtect(JitVtable + ICorJitCompiler_compileMethod_index, new IntPtr(IntPtr.Size), MemoryProtection.ReadWrite, out var oldFlags);
            Marshal.WriteIntPtr(JitVtable, ICorJitCompiler_compileMethod_index, compileMethodPtr);
            VirtualProtect(JitVtable + ICorJitCompiler_compileMethod_index, new IntPtr(IntPtr.Size), oldFlags, out oldFlags);
        }

        private static void UninstallManagedJit()
        {
            InstallManagedJit(DefaultCompileMethodPtr);
        }

        private static IntPtr AllocateTrampoline(IntPtr ptr)
        {
            // Create an executable region of code in memory
            var jmpNative = VirtualAlloc(IntPtr.Zero, DelegateTrampolineCode.Length, AllocationType.Commit, MemoryProtection.ExecuteReadWrite);
            // Copy our trampoline code there
            Marshal.Copy(DelegateTrampolineCode, 0, jmpNative, DelegateTrampolineCode.Length);
            // Setup the delegate we want to call as part of a reverse P/Invoke call
            Marshal.WriteIntPtr(jmpNative, 2, ptr);
            return jmpNative;
        }

        private static void FreeTrampoline(IntPtr ptr)
        {
            VirtualFree(ptr, new IntPtr(DelegateTrampolineCode.Length), FreeType.Release);
        }

        public void Dispose()
        {
            lock (JitLock)
            {
                if (_isDisposed) return;

                if (_overrideCompileMethodPtr == IntPtr.Zero)
                {
                    return;
                }

                UninstallManagedJit();
                _overrideCompileMethodPtr = IntPtr.Zero;
                _overrideCompileMethod = null;
                _isDisposed = true;
                ManagedJitInstance = null;
                _isHookInstalled = false;
            }
        }

        private void ReplaceCompile(MethodBase method, IntPtr ilCodePtr, int ilSize, IntPtr nativeCodePtr, int nativeCodeSize)
        {
            if (method.Name != nameof(Program.JitReplaceAdd))
            {
                return;
            }

            // Instead of: public static int JitReplaceAdd(int a, int b) => a + b;
            // This code generates: a + b + 1

            // 0:  01 d1 add    ecx,edx
            // 2:  89 c8 mov    eax,ecx
            // 4:  ff c0        inc eax
            // 6:  c3 ret

            var instructions = new byte[]
            {
                0x01, 0xD1, 0x89, 0xC8, 0xFF, 0xC0, 0xC3
            };
            
            Marshal.Copy(instructions, 0, nativeCodePtr, instructions.Length);
        }

        private int CompileMethod(
            IntPtr thisPtr,
            IntPtr comp, // ICorJitInfo* comp, /* IN */
            ref CORINFO_METHOD_INFO info, // struct CORINFO_METHOD_INFO  *info,               /* IN */
            uint flags, // unsigned /* code:CorJitFlag */   flags,          /* IN */
            out IntPtr nativeEntry, // BYTE                        **nativeEntry,       /* OUT */
            out int nativeSizeOfCode // ULONG* nativeSizeOfCode    /* OUT */
        )
        {
            // Early exit gracefully. We are entering this if when calling the trampoline
            // As our JIT is not yet compile, but we still want this method to be compiled by the 
            // original JIT!
            if (!_isHookInstalled)
            {
                nativeEntry = IntPtr.Zero;
                nativeSizeOfCode = 0;
                return 0;
            }

            var compileEntry = _compileTls ?? (_compileTls = new CompileTls());
            compileEntry.EnterCount++;
            try
            {
                // We always let the default JIT compile the method
                var result = DefaultCompileMethod(thisPtr, comp, ref info, flags, out nativeEntry, out nativeSizeOfCode);

                // If we are at a top level method to compile, we wil recompile it
                if (compileEntry.EnterCount == 1)
                {
                    var vtableCorJitInfo = Marshal.ReadIntPtr(comp);

                    var getMethodDefFromMethodPtr = Marshal.ReadIntPtr(vtableCorJitInfo, IntPtr.Size * ICorJitInfo_getMethodDefFromMethod_index);
                    var getMethodDefFromMethod = (GetMethodDefFromMethodDelegate)Marshal.GetDelegateForFunctionPointer(getMethodDefFromMethodPtr, typeof(GetMethodDefFromMethodDelegate));
                    var methodToken = getMethodDefFromMethod(comp, info.ftn);

                    var getModuleAssemblyDelegatePtr = Marshal.ReadIntPtr(vtableCorJitInfo, IntPtr.Size * ICorJitInfo_getModuleAssembly_index);
                    var getModuleAssemblyDelegate = (GetModuleAssemblyDelegate)Marshal.GetDelegateForFunctionPointer(getModuleAssemblyDelegatePtr, typeof(GetModuleAssemblyDelegate));
                    var assemblyHandle = getModuleAssemblyDelegate(comp, info.scope);

                    // Check if this assembly was not already found
                    Assembly assemblyFound;

                    // Map AssemblyHandle to the Managed Assembly instance
                    // (use JitLock and MapHandleToAssembly, as the CompileMethod can be called concurrently from different threads)
                    lock (JitLock)
                    {
                        if (!MapHandleToAssembly.TryGetValue(assemblyHandle, out assemblyFound))
                        {
                            var getAssemblyNamePtr = Marshal.ReadIntPtr(vtableCorJitInfo, IntPtr.Size * 44);
                            var getAssemblyName = (GetAssemblyNameDelegate)Marshal.GetDelegateForFunctionPointer(getAssemblyNamePtr, typeof(GetAssemblyNameDelegate));
                            var assemblyNamePtr = getAssemblyName(comp, assemblyHandle);

                            var assemblyName = Marshal.PtrToStringAnsi(assemblyNamePtr);

                            // TODO: Very inefficient way of finding the assembly
                            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
                            {
                                if (assembly.GetName().Name == assemblyName)
                                {
                                    assemblyFound = assembly;
                                    break;
                                }
                            }

                            // Register the assembly
                            MapHandleToAssembly.Add(assemblyHandle, assemblyFound);
                        }
                    }

                    // Find the method with the token
                    MethodBase method = null;
                    if (assemblyFound != null)
                    {
                        foreach (var module in assemblyFound.Modules)
                        {
                            try
                            {
                                method = module.ResolveMethod(methodToken);
                            }
                            catch (Exception)
                            {
                            }
                        }
                    }

                    if (method != null)
                    {
                        ReplaceCompile(method, info.ILCode, info.ILCodeSize, nativeEntry, nativeSizeOfCode);
                    }
                }

                return result;
            }
            finally
            {
                compileEntry.EnterCount--;
            }
        }

        enum CorInfoOptions
        {
            CORINFO_OPT_INIT_LOCALS = 0x00000010, // zero initialize all variables

            CORINFO_GENERICS_CTXT_FROM_THIS = 0x00000020, // is this shared generic code that access the generic context from the this pointer?  If so, then if the method has SEH then the 'this' pointer must always be reported and kept alive.
            CORINFO_GENERICS_CTXT_FROM_METHODDESC = 0x00000040, // is this shared generic code that access the generic context from the ParamTypeArg(that is a MethodDesc)?  If so, then if the method has SEH then the 'ParamTypeArg' must always be reported and kept alive. Same as CORINFO_CALLCONV_PARAMTYPE
            CORINFO_GENERICS_CTXT_FROM_METHODTABLE = 0x00000080, // is this shared generic code that access the generic context from the ParamTypeArg(that is a MethodTable)?  If so, then if the method has SEH then the 'ParamTypeArg' must always be reported and kept alive. Same as CORINFO_CALLCONV_PARAMTYPE
            CORINFO_GENERICS_CTXT_MASK = (CORINFO_GENERICS_CTXT_FROM_THIS |
                                          CORINFO_GENERICS_CTXT_FROM_METHODDESC |
                                          CORINFO_GENERICS_CTXT_FROM_METHODTABLE),
            CORINFO_GENERICS_CTXT_KEEP_ALIVE = 0x00000100, // Keep the generics context alive throughout the method even if there is no explicit use, and report its location to the CLR

        };

        enum CorInfoRegionKind
        {
            CORINFO_REGION_NONE,
            CORINFO_REGION_HOT,
            CORINFO_REGION_COLD,
            CORINFO_REGION_JIT,
        };

        struct CORINFO_SIG_INFO
        {
            public CorInfoCallConv callConv;
            public IntPtr retTypeClass;   // if the return type is a value class, this is its handle (enums are normalized)
            public IntPtr retTypeSigClass;// returns the value class as it is in the sig (enums are not converted to primitives)
            public CorInfoType retType;
            public byte flags;
            public ushort numArgs;
            public CORINFO_SIG_INST sigInst;  // information about how type variables are being instantiated in generic code
            public IntPtr args;
            public IntPtr pSig;
            public uint cbSig;
            public IntPtr scope;          // passed to getArgClass
            public uint token;
            public long garbage;
        };

        struct CORINFO_SIG_INST
        {
            public uint classInstCount;
            public IntPtr classInst; // (representative, not exact) instantiation for class type variables in signature
            public uint methInstCount;
            public IntPtr methInst; // (representative, not exact) instantiation for method type variables in signature
        };

        enum CorInfoType : byte
        {
            CORINFO_TYPE_UNDEF = 0x0,
            CORINFO_TYPE_VOID = 0x1,
            CORINFO_TYPE_BOOL = 0x2,
            CORINFO_TYPE_CHAR = 0x3,
            CORINFO_TYPE_BYTE = 0x4,
            CORINFO_TYPE_UBYTE = 0x5,
            CORINFO_TYPE_SHORT = 0x6,
            CORINFO_TYPE_USHORT = 0x7,
            CORINFO_TYPE_INT = 0x8,
            CORINFO_TYPE_UINT = 0x9,
            CORINFO_TYPE_LONG = 0xa,
            CORINFO_TYPE_ULONG = 0xb,
            CORINFO_TYPE_NATIVEINT = 0xc,
            CORINFO_TYPE_NATIVEUINT = 0xd,
            CORINFO_TYPE_FLOAT = 0xe,
            CORINFO_TYPE_DOUBLE = 0xf,
            CORINFO_TYPE_STRING = 0x10,         // Not used, should remove
            CORINFO_TYPE_PTR = 0x11,
            CORINFO_TYPE_BYREF = 0x12,
            CORINFO_TYPE_VALUECLASS = 0x13,
            CORINFO_TYPE_CLASS = 0x14,
            CORINFO_TYPE_REFANY = 0x15,

            // CORINFO_TYPE_VAR is for a generic type variable.
            // Generic type variables only appear when the JIT is doing
            // verification (not NOT compilation) of generic code
            // for the EE, in which case we're running
            // the JIT in "import only" mode.

            CORINFO_TYPE_VAR = 0x16,
            CORINFO_TYPE_COUNT,                         // number of jit types
        };

        enum CorInfoCallConv
        {
            // These correspond to CorCallingConvention

            CORINFO_CALLCONV_DEFAULT = 0x0,
            CORINFO_CALLCONV_C = 0x1,
            CORINFO_CALLCONV_STDCALL = 0x2,
            CORINFO_CALLCONV_THISCALL = 0x3,
            CORINFO_CALLCONV_FASTCALL = 0x4,
            CORINFO_CALLCONV_VARARG = 0x5,
            CORINFO_CALLCONV_FIELD = 0x6,
            CORINFO_CALLCONV_LOCAL_SIG = 0x7,
            CORINFO_CALLCONV_PROPERTY = 0x8,
            CORINFO_CALLCONV_NATIVEVARARG = 0xb,    // used ONLY for IL stub PInvoke vararg calls

            CORINFO_CALLCONV_MASK = 0x0f,     // Calling convention is bottom 4 bits
            CORINFO_CALLCONV_GENERIC = 0x10,
            CORINFO_CALLCONV_HASTHIS = 0x20,
            CORINFO_CALLCONV_EXPLICITTHIS = 0x40,
            CORINFO_CALLCONV_PARAMTYPE = 0x80,     // Passed last. Same as CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG
        };

        struct CORINFO_METHOD_INFO
        {
            public IntPtr ftn;
            public IntPtr scope;
            public IntPtr ILCode;
            public int ILCodeSize;
            public uint maxStack;
            public uint EHcount;
            public CorInfoOptions options;
            public CorInfoRegionKind regionKind;
            public CORINFO_SIG_INFO args;
            public CORINFO_SIG_INFO locals;
        };

        [DllImport("kernel32", EntryPoint = "GetProcAddress")]
        private static extern IntPtr GetProcAddress(IntPtr handle, string procedureName);

        [DllImport("kernel32", EntryPoint = "VirtualProtect")]
        private static extern int VirtualProtect(IntPtr lpAddress, IntPtr dwSize, MemoryProtection flNewProtect, out MemoryProtection lpflOldProtect);

        [DllImport("kernel32", EntryPoint = "VirtualAlloc")]
        private static extern IntPtr VirtualAlloc(IntPtr lpAddress, int dwSize, AllocationType flAllocationType, MemoryProtection flProtect);

        [DllImport("kernel32", EntryPoint = "VirtualFree")]
        private static extern int VirtualFree(IntPtr lpAddress, IntPtr dwSize, FreeType freeType);

        [Flags]
        private enum FreeType
        {
            Decommit = 0x4000,
            Release = 0x8000,
        }

        [Flags]
        private enum AllocationType
        {
            Commit = 0x1000,
            Reserve = 0x2000,
            Decommit = 0x4000,
            Release = 0x8000,
            Reset = 0x80000,
            Physical = 0x400000,
            TopDown = 0x100000,
            WriteWatch = 0x200000,
            LargePages = 0x20000000
        }

        [Flags]
        private enum MemoryProtection
        {
            Execute = 0x10,
            ExecuteRead = 0x20,
            ExecuteReadWrite = 0x40,
            ExecuteWriteCopy = 0x80,
            NoAccess = 0x01,
            ReadOnly = 0x02,
            ReadWrite = 0x04,
            WriteCopy = 0x08,
            GuardModifierflag = 0x100,
            NoCacheModifierflag = 0x200,
            WriteCombineModifierflag = 0x400
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate IntPtr GetJitDelegate();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate int CompileMethodDelegate(
            IntPtr thisPtr,
            IntPtr comp, // ICorJitInfo* comp, /* IN */
            ref CORINFO_METHOD_INFO info, // struct CORINFO_METHOD_INFO  *info,               /* IN */
            uint flags, // unsigned /* code:CorJitFlag */   flags,          /* IN */
            out IntPtr nativeEntry, // BYTE                        **nativeEntry,       /* OUT */
            out int nativeSizeOfCode // ULONG* nativeSizeOfCode    /* OUT */
        );

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate void GetVersionIdentifierDelegate(IntPtr thisPtr, out Guid versionIdentifier /* OUT */);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int GetMethodDefFromMethodDelegate(IntPtr thisPtr, IntPtr hMethodHandle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr GetMethodClassDelegate(IntPtr thisPtr, IntPtr methodHandle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr GetMethodNameDelegate(IntPtr thisPtr, IntPtr ftn, out IntPtr moduleName);


        // Returns the assembly that contains the module "mod".
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr GetModuleAssemblyDelegate(IntPtr thisPtr, IntPtr moduleHandle);

        // Returns the name of the assembly "assem".
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate IntPtr GetAssemblyNameDelegate(IntPtr thisPtr, IntPtr assemblyHandle);

        private class CompileTls
        {
            public int EnterCount;
        }

        private class IntPtrEqualityComparer : IEqualityComparer<IntPtr>
        {
            public static readonly IntPtrEqualityComparer Instance = new IntPtrEqualityComparer();

            public bool Equals(IntPtr x, IntPtr y)
            {
                return x == y;
            }

            public int GetHashCode(IntPtr obj)
            {
                return obj.GetHashCode();
            }
        }
    }
}