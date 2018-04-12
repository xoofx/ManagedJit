// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Demonstrate multiple calling conventions supported by NetAsm : ClrCall (default calling convention), StdCall, FastCall, Cdecl.
    /// (ThisCall is supported but there is no test available at the moment).
    /// Test also parameters [out] and [ref].
    /// See documentation to understand calling conventions
    /// </summary>
    [AllowCodeInjection]
    class TestCallingConventions
    {
        // ----------------------------------------------------------------------------------------------------
        // Test ClrCall Convention parameters : (Arg0: ecx, Arg1: edx, other parameters left to right on stack)
        // ----------------------------------------------------------------------------------------------------
        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test0ArgClrCall() { return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test1ArgClrCall(int x) {return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test2ArgClrCall(int x, int y) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test3ArgClrCall(int x, int y, int z) {return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgClrCall(int x, int y, int z, int w) {return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith1DoubleClrCall(int x, double y, int z, int w) { return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith2DoubleClrCall(int x, double y, double z, int w) { return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgWithRefAndOutClrCall(int x, ref int y, out int z, int w) { z = 0; return 0;}

        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        public static int TestArgSizeClrCall(byte x, short y, int z, double w, long ww) { return 0; }

        [Description("Test ClrCall calling convention")]
        public static void RunClrCall()
        {
            int resultInt;
            double resultDouble;
            int x = 1;
            int y = 2;
            int z = 3;
            int w = 4;

            resultInt = Test0ArgClrCall();
            PerfManager.Assert(resultInt == 1, "Invalid result Test0ArgClrCall");

            resultInt = Test1ArgClrCall(x);
            PerfManager.Assert(resultInt == 1, "Invalid result Test01rgClrCall");

            resultInt = Test2ArgClrCall(x, y);
            PerfManager.Assert(resultInt == (x + y * 3), "Invalid result Test2ArgClrCall");

            resultInt = Test3ArgClrCall(x, y, z);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5), "Invalid result Test03rgClrCall");

            resultInt = Test4ArgClrCall(x, y, z, w);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgClrCall");

            resultDouble = Test4ArgWith1DoubleClrCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith1DoubleClrCall");

            resultDouble = Test4ArgWith2DoubleClrCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith2DoubleClrCall");

            resultInt = Test4ArgWithRefAndOutClrCall(x, ref y, out z, w);
            PerfManager.Assert((resultInt == z) && (resultInt == (x + y * 3 + w * 5)), "Invalid result Test4ArgWithRefAndOutClrCall");

            z = 3;
            resultInt = TestArgSizeClrCall((byte)x, (short)y, z, w, 5);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7 + 5 * 9), "Invalid result TestArgSizeClrCall");
        }

        // -----------------------------------------------------------------------------------------------
        // Test StdCall Convention parameters : (parameters right to left on stack)
        // -----------------------------------------------------------------------------------------------

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test0ArgStdCall() { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test1ArgStdCall(int x) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test2ArgStdCall(int x, int y) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test3ArgStdCall(int x, int y, int z) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgStdCall(int x, int y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith1DoubleStdCall(int x, double y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith2DoubleStdCall(int x, double y, double z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall ), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgWithRefAndOutStdCall(int x, ref int y, out int z, int w) { z = 0; return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int TestArgSizeStdCall(byte x, short y, int z, double w, long ww) { return 0; }

        [Description("Test StdCall calling convention")]
        public static void RunStdCall()
        {
            int resultInt;
            double resultDouble;
            int x = 1;
            int y = 2;
            int z = 3;
            int w = 4;

            resultInt = Test0ArgStdCall();
            PerfManager.Assert(resultInt == 1, "Invalid result Test0ArgStdCall");

            resultInt = Test1ArgStdCall(x);
            PerfManager.Assert(resultInt == 1, "Invalid result Test01rgStdCall");

            resultInt = Test2ArgStdCall(x, y);
            PerfManager.Assert(resultInt == (x + y * 3), "Invalid result Test2ArgStdCall");

            resultInt = Test3ArgStdCall(x, y, z);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5), "Invalid result Test03rgStdCall");

            resultInt = Test4ArgStdCall(x, y, z, w);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgStdCall");

            resultDouble = Test4ArgWith1DoubleStdCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith1DoubleStdCall");

            resultDouble = Test4ArgWith2DoubleStdCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith2DoubleStdCall");

            resultInt = Test4ArgWithRefAndOutStdCall(x, ref y, out z, w);
            PerfManager.Assert((resultInt == z) && (resultInt == (x + y * 3 + w * 5)), "Invalid result Test4ArgWithRefAndOutStdCall");

            z = 3;
            resultInt = TestArgSizeStdCall((byte)x, (short)y, z, w, 5);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7 + 5 * 9), "Invalid result TestArgSizeStdCall");
        }

        // -----------------------------------------------------------------------------------------------
        // Test FastCall Convention parameters : (Arg0: ecx, Arg1: edx, parameters right to left on stack)
        // -----------------------------------------------------------------------------------------------

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test0ArgFastCall() { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test1ArgFastCall(int x) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test2ArgFastCall(int x, int y) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test3ArgFastCall(int x, int y, int z) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgFastCall(int x, int y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith1DoubleFastCall(int x, double y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith2DoubleFastCall(int x, double y, double z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgWithRefAndOutFastCall(int x, ref int y, out int z, int w) { z = 0; return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.FastCall), MethodImpl(MethodImplOptions.NoInlining)]
        public static int TestArgSizeFastCall(byte x, short y, int z, double w, long ww) { return 0; }

        [Description("Test FastCall calling convention")]
        public static void RunFastCall()
        {
            int resultInt;
            double resultDouble;
            int x = 1;
            int y = 2;
            int z = 3;
            int w = 4;

            resultInt = Test0ArgFastCall();
            PerfManager.Assert(resultInt == 1, "Invalid result Test0ArgFastCall");

            resultInt = Test1ArgFastCall(x);
            PerfManager.Assert(resultInt == 1, "Invalid result Test01rgFastCall");

            resultInt = Test2ArgFastCall(x, y);
            PerfManager.Assert(resultInt == (x + y * 3), "Invalid result Test2ArgFastCall");

            resultInt = Test3ArgFastCall(x, y, z);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5), "Invalid result Test03rgFastCall");

            resultInt = Test4ArgFastCall(x, y, z, w);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgFastCall");

            resultDouble = Test4ArgWith1DoubleFastCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith1DoubleFastCall");

            resultDouble = Test4ArgWith2DoubleFastCall(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith2DoubleFastCall");

            resultInt = Test4ArgWithRefAndOutFastCall(x, ref y, out z, w);
            PerfManager.Assert((resultInt == z) && (resultInt == (x + y * 3 + w * 5)), "Invalid result Test4ArgWithRefAndOutFastCall");

            z = 3;
            resultInt = TestArgSizeFastCall((byte)x, (short)y, z, w, 5);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7 + 5 * 9), "Invalid result TestArgSizeFastCall");
        }

        // -----------------------------------------------------------------------------------------------
        // Test ThisCall Convention parameters : (Arg0: ecx, parameters right to left on stack)
        // -----------------------------------------------------------------------------------------------

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test0ArgThisCall() { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test1ArgThisCall(int x) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test2ArgThisCall(int x, int y) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test3ArgThisCall(int x, int y, int z) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test4ArgThisCall(int x, int y, int z, int w) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static double Test4ArgWith1DoubleThisCall(int x, double y, int z, int w) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static double Test4ArgWith2DoubleThisCall(int x, double y, double z, int w) { return 0; }

        //[CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.ThisCall), MethodImpl(MethodImplOptions.NoInlining)]
        //public static int Test4ArgWithRefAndOutThisCall(int x, ref int y, out int z, int w) { z = 0; return 0; }

        // -----------------------------------------------------------------------------------------------
        // Test Cdecl Convention parameters : (parameters right to left on stack, caller clean the stack)
        // -----------------------------------------------------------------------------------------------

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test0ArgCdecl() { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test1ArgCdecl(int x) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test2ArgCdecl(int x, int y) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test3ArgCdecl(int x, int y, int z) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgCdecl(int x, int y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith1DoubleCdecl(int x, double y, int z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static double Test4ArgWith2DoubleCdecl(int x, double y, double z, int w) { return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int Test4ArgWithRefAndOutCdecl(int x, ref int y, out int z, int w) { z = 0; return 0; }

        [CodeInjection("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.Cdecl), MethodImpl(MethodImplOptions.NoInlining)]
        public static int TestArgSizeCdecl(byte x, short y, int z, double w, long ww) { return 0; }

        [Description("Test Cdecl calling convention")]
        public static void RunCdecl()
        {
            int resultInt;
            double resultDouble;
            int x = 1;
            int y = 2;
            int z = 3;
            int w = 4;

            resultInt = Test0ArgCdecl();
            PerfManager.Assert(resultInt == 1, "Invalid result Test0ArgCdecl");

            resultInt = Test1ArgCdecl(x);
            PerfManager.Assert(resultInt == 1, "Invalid result Test01rgCdecl");

            resultInt = Test2ArgCdecl(x, y);
            PerfManager.Assert(resultInt == (x + y * 3), "Invalid result Test2ArgCdecl");

            resultInt = Test3ArgCdecl(x, y, z);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5), "Invalid result Test03rgCdecl");

            resultInt = Test4ArgCdecl(x, y, z, w);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgCdecl");

            resultDouble = Test4ArgWith1DoubleCdecl(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith1DoubleCdecl");

            resultDouble = Test4ArgWith2DoubleCdecl(x, y, z, w);
            PerfManager.Assert(resultDouble == (x + y * 3 + z * 5 + w * 7), "Invalid result Test4ArgWith2DoubleCdecl");

            resultInt = Test4ArgWithRefAndOutCdecl(x, ref y, out z, w);
            PerfManager.Assert((resultInt == z) && (resultInt == (x + y * 3 + w * 5)), "Invalid result Test4ArgWithRefAndOutCdecl");

            resultInt = TestArgSizeCdecl((byte)x, (short)y, z, w, 5);
            PerfManager.Assert(resultInt == (x + y * 3 + z * 5 + w * 7 + 5 * 9), "Invalid result TestArgSizeCdecl");
        }

        /// <summary>
        /// Runs the dynamic code injection demo. Because the CodeInjection at the class level will try to bind this method
        /// to the dll, we need to use a custom DefaultCodeInjector to compile this method with the default JIT compiler.
        /// </summary>
        [Description("Calling Conventions Demo")]
        public static void Run()
        {
            PerfManager.Instance.Run(RunClrCall);
            PerfManager.Instance.Run(RunStdCall);
            PerfManager.Instance.Run(RunFastCall);
            PerfManager.Instance.Run(RunCdecl);
        }
    }
}
