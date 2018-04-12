// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Simple NopCodeInjector used by <see cref="TestDynamicCodeInjection"/>. 
    /// This CodeInjector replace generate a Nop method for any methods (with eventual parameters).
    /// </summary>
    public class NopCodeInjector : ICodeInjector
    {
        // .X86 instruction : RET + stacksize
        private static byte[] ReturnWithStackSize = new byte[] {0xC2, 0x00, 0x00};
        private static byte[] ReturnWithNoStack = new byte[] { 0xC3};

        public void CompileMethod(MethodContext context)
        {
            // Print information about the method being compiled using reflection classes.
            MethodInfo methodInfo = context.MethodInfo;
            PerfManager.Instance.WriteLine("> JIT NopCodeInjector is called on method [{0}] with a StackSize {1}",
                                  methodInfo.Name, context.StackSize);

            // Generate a native code with a Ret + StackSize of the method
            CodeRef nativeCodeRef;
            if (context.StackSize > 0)
            {
                // If there is a stack, then, generate the RET + Stacksize opcode
                nativeCodeRef = context.AllocExecutionBlock(ReturnWithStackSize);
                Marshal.WriteInt16(nativeCodeRef.Pointer, 1, (short) context.StackSize);
            } else
            {
                // If there is no stack used, then RET
                nativeCodeRef = context.AllocExecutionBlock(ReturnWithNoStack);
            }

            // Inform NetAsm the native code to use for the method
            context.SetOutputNativeCode(nativeCodeRef);
        }
    }

    /// <summary>
    /// Simple CodeInjector used by <see cref="TestDynamicCodeInjection"/>.
    /// </summary>
    public class SimpleCodeInjector : ICodeInjector
    {
        static byte[] nativeIndirectCall = new byte[] 
            { 
                0xE9, 0x00, 0x00, 0x00, 0x00,         // .X86 instruction : jmp RelativeMethodAddress
            };

        public void CompileMethod(MethodContext context)
        {
            MethodInfo methodInfo = context.MethodInfo;
            PerfManager.Instance.WriteLine("> JIT CodeInjector is called on method [{0}]", methodInfo.Name);
            if (methodInfo.Name == "NetAsmAdd")
            {
                // Output the native code with the C code (see TestStaticCodeInjection class)
                context.SetOutputNativeCode(new byte[] {0x8b, 0x44, 0x24, 0x04, 0x03, 0xc2, 0xc2, 0x04, 0x00});
            } else if ( methodInfo.Name == "NetAsmAddFromDll")
            {
                CodeRef referenceToDllFunction = CodeRef.BindToDllFunction("NetAsmDemoCLib.dll", "NetAsmAddInC");
                // Set the native code to use the DLL function
                context.SetOutputNativeCode(referenceToDllFunction);
            } else if ( methodInfo.Name == "NetAsmIndirectCaller")
            {
                // Demonstrate how to call a clr method from a native method.
                MethodInfo info = typeof(TestDynamicCodeInjection).GetMethod("ManagedMethodCalledByNetAsm");
                // Get the pointer to the method ManagedMethodCalledByNetAsm
                IntPtr pointerToFunction = info.MethodHandle.GetFunctionPointer();
                // Generate an execution block with the template code nativeIndirectCall
                CodeRef codeRef = context.AllocExecutionBlock(nativeIndirectCall);
                // Calculate relative JMP address
                IntPtr jmpRelativeAddress = new IntPtr(pointerToFunction.ToInt64() - (codeRef.Pointer.ToInt64() + nativeIndirectCall.Length ));
                // Write the relative JMP address just after the JMP opcode (0xE9)
                Marshal.WriteIntPtr(codeRef.Pointer, 1, jmpRelativeAddress);
                // Set output native code
                context.SetOutputNativeCode(codeRef);
            }
        }
    }

    /// <summary>
    /// Demonstrate simple Add call with NetAsm using Dynamic Code Injection. The <see cref="SimpleCodeInjector"/> is 
    /// used at the class level and at the method level. 
    /// <list type="bullet">
    /// <item><description>The method <see cref="NetAsmAdd"/> is compiled with a buffer of native code</description></item>
    /// <item><description>The method <see cref="NetAsmAddFromDll"/> is compiled using the function NetAsmAddInC in NetAsmDemoCLib.dll</description></item>
    /// <item><description>The method <see cref="ManagedAdd"/> is compiled with the JIT compiler. The CodeInjector is called </description></item>
    /// <item><description>The method <see cref="ManagedAddInlined"/> is compiled with the JIT compiler. Because
    /// the method is inlined by default, the CodeInjector is not called (in release mode execution).</description></item>
    /// <item><description>The static method <see cref="NetAsmIndirectCaller"/> is compiled with the JIT compiler and is redirecting the call 
    /// to another static method <see cref="ManagedMethodCalledByNetAsm"/>.</description></item>
    /// </list>    
    /// </summary>
    [AllowCodeInjection, CodeInjection(typeof(SimpleCodeInjector))]
    class TestDynamicCodeInjection
    {
        /// <summary>
        /// NetAsm dynamic code injection using a simple addition method. Use <see cref="CodeInjectionAttribute"/> with a subclass of 
        /// <see cref="ICodeInjector"/>. For this method, the CodeInjector returns a buffer of native code.
        [CodeInjection(typeof(SimpleCodeInjector)), MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAdd(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// NetAsm dynamic code injection using a simple addition method. Use <see cref="CodeInjectionAttribute"/> with a subclass of 
        /// <see cref="ICodeInjector"/>. For this method, the CodeInjector bind the implementation to a dll function.
        [CodeInjection(typeof(SimpleCodeInjector)), MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAddFromDll(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// Managed Add method but with inlining off.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public int ManagedAdd(int x, int y)
        {
            return x + y + 1;
        }

        /// <summary>
        /// Managed Add method. This method is inlined while executing in Release mode (not Debug).
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <returns></returns>
        public int ManagedAddInlined(int x, int y)
        {
            return x + y + 1;
        }


        /// <summary>
        /// This method is generated by the CodeInjector and is going to call the <see cref="ManagedMethodCalledByNetAsm"/> function.
        /// </summary>
        [CodeInjection(typeof(SimpleCodeInjector)), MethodImpl(MethodImplOptions.NoInlining)]
        public static int NetAsmIndirectCaller()
        {
            // This method is not going to return -1 but 2 (from ManagedMethodCalledByNetAsm).
            return -1;
        }

        /// <summary>
        /// Method called by <see cref="NetAsmIndirectCaller"/>. The calling code is generated by the CodeInjector.
        /// </summary>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static int ManagedMethodCalledByNetAsm()
        {
            PerfManager.Instance.WriteLine("NetAsmIndirectCalled is called");
            return 2;
        }

        /// <summary>
        /// NopMethod
        /// </summary>
        [CodeInjection(typeof(NopCodeInjector)), MethodImpl(MethodImplOptions.NoInlining)]
        public static void NetAsmNopMethod()
        {
            throw new Exception("This should not append");
        }

        /// <summary>
        /// NopMethod with parameters
        /// </summary>
        [CodeInjection(typeof(NopCodeInjector)), MethodImpl(MethodImplOptions.NoInlining)]
        public static void NetAsmNopMethodWithArgs(int x, int y, int z, int w)
        {
            throw new Exception("This should not append");
        }

        /// <summary>
        /// Runs the dynamic code injection demo.
        /// </summary>
        [Description("Dynamic Code Injection Demo")]
        public static void Run()
        {
            TestDynamicCodeInjection demo = new TestDynamicCodeInjection();
            int result = demo.NetAsmAdd(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAdd(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAdd is not set");

            result = demo.NetAsmAddFromDll(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAddFromDll(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAddFromDll is not set");

            result = demo.ManagedAdd(1, 2);
            PerfManager.Instance.WriteLine("Result ManagedAdd(1,2) + 1 = {0}", result);
            PerfManager.Assert(result == 4, "Error, NetAsm hook on method ManagedAdd is not set");

            result = demo.ManagedAddInlined(1, 2);
            PerfManager.Instance.WriteLine("Result ManagedAddInlined(1,2) + 1 = {0}", result);
            PerfManager.Assert(result == 4, "Error, NetAsm hook on method ManagedAddInlined is not set");

            result = NetAsmIndirectCaller();
            PerfManager.Instance.WriteLine("Result NetAsmIndirectCaller() = {0}", result);
            PerfManager.Assert(result == 2, "Error, NetAsm hook on method NetAsmIndirectCaller is not set");

            // Test Nop Methods
            NetAsmNopMethod();
            NetAsmNopMethodWithArgs(1,2,3,4);
        }
    }
}
