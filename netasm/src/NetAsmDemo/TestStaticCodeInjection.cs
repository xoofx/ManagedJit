// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Demonstrate simple Add call with NetAsm.
    /// </summary>
    [AllowCodeInjection]
    class TestStaticCodeInjection
    {
        /// <summary>
        /// NetAsm static code injection using a simple addition method. Use <see cref="CodeInjectionAttribute"/> with a static buffer of 
        /// native code. Note that this is an instance method, so the first parameter
        /// (ecx) is the This pointer to the instance. For demonstration purpose, we force this method not to be inlined.
        /// But, in real world, this kind of method should not be implemented with NetAsm, the JIT Compiler is able to inline
        /// the original method (NetAsm's methods cannot be inlined). So use NetAsm code injection with caution...
        /// Code generated from the C function :
        /// <code>int __fastcall NetAsmAdd(void* pThis, int x, int y) 
        /// { 
        ///     return x + y;
        /// }
        /// 
        /// Assembler code extracted from .cod file from VisualC++ :
        /// 00000	8b 44 24 04	 mov	 eax, DWORD PTR _y$[esp-4]
        /// 00004	03 c2		 add	 eax, edx
        /// 00006	c2 04 00	 ret	 4
        /// </code>
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <returns>the sum</returns>
[CodeInjection(new byte[] { 0x8b, 0x44, 0x24, 0x04, 0x03, 0xc2, 0xc2, 0x04, 0x00 }), MethodImpl(MethodImplOptions.NoInlining)]
public int NetAsmAdd(int x, int y)
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
            return x + y;
        }

        /// <summary>
        /// Managed Add method. This method is inlined while executing in Release mode (not Debug).
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <returns></returns>
        public int ManagedAddInlined(int x, int y)
        {
            return x + y;
        }

        /// <summary>
        /// Runs the static code injection demo.
        /// </summary>
        [Description("Static Code Injection Demo")]
        public static void Run()
        {
            TestStaticCodeInjection demo = new TestStaticCodeInjection();
            int result = demo.NetAsmAdd(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAdd(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAdd is not set");
        }

    }
}
