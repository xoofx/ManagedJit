using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security;
using NetAsm;
using NetAsmDemoMixedLib;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    [AllowCodeInjection]
    class TestSimpleAddBenchmark : TestStaticCodeInjection
    {
        private static int MAX_CALL = 20000000;
        private static int MAXCOUNT = 5;
        private static int result = 0;

        [DllImport("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall,
            EntryPoint = "SimpleAddInterop_stdcall")]
        public static extern int InteropAdd(int thisPtr, int x, int y);

        [DllImport("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall,
            EntryPoint = "SimpleAddInterop_stdcall"), SuppressUnmanagedCodeSecurity]
        public static extern int InteropNoSecurityAdd(int thisPtr, int x, int y);

        public static void RunNetAsmAdd()
        {
            result = 0;
            TestSimpleAddBenchmark test = new TestSimpleAddBenchmark();
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += test.NetAsmAdd(i, 1);
                result += test.NetAsmAdd(i, i);
                result += test.NetAsmAdd(i, 1);
                result += test.NetAsmAdd(i, i);
                result += test.NetAsmAdd(i, 1);
                result += test.NetAsmAdd(i, i);
                result += test.NetAsmAdd(i, 1);
                result += test.NetAsmAdd(i, i);
                result += test.NetAsmAdd(i, 1);
                result += test.NetAsmAdd(i, i);
            }
        }

        public static void RunManagedAdd()
        {
            result = 0;
            TestSimpleAddBenchmark test = new TestSimpleAddBenchmark();
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += test.ManagedAdd(i, 1);
                result += test.ManagedAdd(i, i);
                result += test.ManagedAdd(i, 1);
                result += test.ManagedAdd(i, i);
                result += test.ManagedAdd(i, 1);
                result += test.ManagedAdd(i, i);
                result += test.ManagedAdd(i, 1);
                result += test.ManagedAdd(i, i);
                result += test.ManagedAdd(i, 1);
                result += test.ManagedAdd(i, i);
            }
        }

        public static void RunManagedAddInlined()
        {
            TestSimpleAddBenchmark test = new TestSimpleAddBenchmark();
            result = 0;
            int temp = 0;
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += test.ManagedAddInlined(i, 1);
                result += test.ManagedAddInlined(i, i);
                result += test.ManagedAddInlined(i, 1);
                result += test.ManagedAddInlined(i, i);
                result += test.ManagedAddInlined(i, 1);
                result += test.ManagedAddInlined(i, i);
                result += test.ManagedAddInlined(i, 1);
                result += test.ManagedAddInlined(i, i);
                result += test.ManagedAddInlined(i, 1);
                result += test.ManagedAddInlined(i, i);
            }
        }

        public static void RunInteropAdd()
        {
            result = 0;
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += InteropAdd(i, i, 1);
                result += InteropAdd(i, i, i);
                result += InteropAdd(i, i, 1);
                result += InteropAdd(i, i, i);
                result += InteropAdd(i, i, 1);
                result += InteropAdd(i, i, i);
                result += InteropAdd(i, i, 1);
                result += InteropAdd(i, i, i);
                result += InteropAdd(i, i, 1);
                result += InteropAdd(i, i, i);
            }
        }

        public static void RunInteropNoSecurityAdd()
        {
            result = 0;
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += InteropNoSecurityAdd(i, i, 1);
                result += InteropNoSecurityAdd(i, i, i);
                result += InteropNoSecurityAdd(i, i, 1);
                result += InteropNoSecurityAdd(i, i, i);
                result += InteropNoSecurityAdd(i, i, 1);
                result += InteropNoSecurityAdd(i, i, i);
                result += InteropNoSecurityAdd(i, i, 1);
                result += InteropNoSecurityAdd(i, i, i);
                result += InteropNoSecurityAdd(i, i, 1);
                result += InteropNoSecurityAdd(i, i, i);
            }
        }

        public static void RunMixedAdd()
        {
            result = 0;
            for (int i = 0; i < MAX_CALL; i++)
            {
                result += MixedClass.SimpleAddMixed(i, 1);
                result += MixedClass.SimpleAddMixed(i, i);
                result += MixedClass.SimpleAddMixed(i, 1);
                result += MixedClass.SimpleAddMixed(i, i);
                result += MixedClass.SimpleAddMixed(i, 1);
                result += MixedClass.SimpleAddMixed(i, i);
                result += MixedClass.SimpleAddMixed(i, 1);
                result += MixedClass.SimpleAddMixed(i, i);
                result += MixedClass.SimpleAddMixed(i, 1);
                result += MixedClass.SimpleAddMixed(i, i);
            }
        }

        /// <summary>
        /// Runs the static code injection demo.
        /// </summary>
        [Description("Simple Add Benchmark")]
        public static new void Run()
        {
            PerfManager perf = PerfManager.Instance;
            for (int count = 0; count < MAXCOUNT; count++)
            {
                if (count == 0)
                {
                    perf.WriteLine("Warm-UP");
                }
                else
                {
                    perf.WriteLine("Count({0}/{1})", count, MAXCOUNT - 1);
                }

                perf.Run(RunManagedAdd);
                perf.Run(RunManagedAddInlined);
                perf.Run(RunInteropAdd);
                perf.Run(RunInteropNoSecurityAdd);
                perf.Run(RunMixedAdd);
                perf.Run(RunNetAsmAdd);

                if (count == 0)
                {
                    // First count is not taken into account
                    perf.Reset();
                }

                perf.WriteLine();
            }
            perf.DisplayAverage();
            perf.WriteLine("End SimpleAdd Benchmark");
            perf.WriteLine();
        }
    }
}
