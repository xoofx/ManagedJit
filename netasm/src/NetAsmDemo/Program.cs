// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.Runtime.InteropServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Main Program run the following test :
    /// - A Global Injection Code based on regexp.
    /// 
    /// - HelloWorld
    /// - A Global Code Injection demo
    /// - A Static Code Injection demo
    /// - A Dll Code Injection demo
    /// - A Dynamic Code Injection demo
    /// - A Calling convention test (ClrCall, StdCall, FastCall, Cdecl)
    /// - A simple test with structure as a parameter.
    /// - A hack that can enable a cast from a byte[] to a float[] using the same memory
    /// - A demo with cpuid assembler function
    /// - A benchmark on simple addition function Add(x,y).
    /// - A benchmark on matrix multiplication using SSE2 for NetAsm.
    /// </summary>
    class Program
    {
        static void Main(string[] args)
        {
            PerfManager perfManager = PerfManager.Instance;
            Console.Out.WriteLine("NetAsmDemo. Framework .NET Version {0}", RuntimeEnvironment.GetSystemVersion());

            // -----------------------------------------------------------------------------------
            // Install the JITHook.
            // Test a Global Code Injector with pattern regex on classes names. Perform a Global
            // Code Injection only on NetAsmDemo..* namespace
            JITHook.Install("NetAsmDemo\\..*",new GlobalCodeInjector());
            perfManager.Run(TestGlobalInjection.RunPublicMethod, true);
            // Remove it
            JITHook.Remove();
            // The JITHook with the Global Injector is removed here
            // -----------------------------------------------------------------------------------

            // -----------------------------------------------------------------------------------
            // Reinstall a standard JITHook with no Global Code Injector
            JITHook.Install();
            
            // Run All tests
            perfManager.Run(TestHelloWorld.Run, true);
            perfManager.Run(TestStaticCodeInjection.Run, true);
            perfManager.Run(TestDllCodeInjection.Run, true);
            perfManager.Run(TestDynamicCodeInjection.Run, true);
            perfManager.Run(TestCallingConventions.Run, true);
            perfManager.Run(TestStructure.Run, true);
            perfManager.Run(TestCast.Run, true);
            perfManager.Run(TestCpuid.Run,true);
            perfManager.Run(TestSimpleAddBenchmark.Run, true);
            perfManager.Run(TestMatrixMulBenchmark.Run, true);

            JITHook.Remove();
            // -----------------------------------------------------------------------------------

            Console.Out.WriteLine("Press Enter to close the program");
            Console.In.ReadLine();

        }
    }
}
