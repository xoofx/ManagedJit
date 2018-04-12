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
    /// Demonstrate simple Add call with NetAsm using DLL Code Injection. The DLL Code injection is used at the class level and at 
    /// the method level.  
    /// <list type="bullet">
    /// <item><description>The method <see cref="NetAsmAddDll"/> is linked to the DLL specified with the attribute of the method.</description></item>
    /// <item><description>The method <see cref="NetAsmAddDll1"/> is linked to the function DLL specified with the attribute of the class.</description></item>
    /// <item><description>The method <see cref="NetAsmAddDll2"/> is linked to the function DLL specified with the attribute of the class.</description></item>
    /// </list>    
    /// </summary>
    [AllowCodeInjection, CodeInjection("NetAsmDemoCLib.dll")]
    class TestDllCodeInjection
    {
        /// <summary>
        /// NetAsm DLL Code Injection. 
        /// </summary>
        [CodeInjection("NetAsmDemoCLib.dll", "NetAsmAddInC"), MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAddDll(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// NetAsm DLL Code Injection. Name of the method is use to find the dll function.
        /// </summary>
        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAddInC(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// The code of this method is bind to the dll function NetAsmAddDll1 using the DLL Injection specified at the class level.
        /// </summary>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAddDll1(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// The code of this method is bind to the dll function NetAsmAddDll1 using the DLL Injection specified at the class level.
        /// </summary>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public int NetAsmAddDll2(int x, int y)
        {
            // This method is compiled using the native code from the CodeInjection Attribute
            // The IL code of this method is never compiled by the JIT (if MethodImplOptions.NoInlining is set for small methods)
            // ADD +1 to check that this method is not used.
            return x + y + 1;
        }

        /// <summary>
        /// Runs the dynamic code injection demo. Because the CodeInjection at the class level will try to bind this method
        /// to the dll, we force NoCodeInjection to let the default JIT compiler compile this method.
        /// </summary>
        [NoCodeInjection, Description("DLL Code Injection Demo")]
        public static void Run()
        {
            TestDllCodeInjection demo = new TestDllCodeInjection();
            int result = demo.NetAsmAddDll(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAddDll(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAddDll is not set");

            result = demo.NetAsmAddDll1(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAddDll1(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAddDll1 is not set");

            result = demo.NetAsmAddDll2(1, 2);
            PerfManager.Instance.WriteLine("Result NetAsmAddDll2(1,2) = {0}", result);
            PerfManager.Assert(result == 3, "Error, NetAsm hook on method NetAsmAddDll2 is not set");
        }
    }
}
