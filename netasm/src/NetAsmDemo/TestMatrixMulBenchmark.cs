// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security;
using NetAsm;
using NetAsmDemoMixedLib;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// This class demonstrates the performance of Matrix Multiplication using an optimized version implemented through NetAsm 
    /// and comparing this version to others :
    /// <list type="bullet">
    /// <item><description>NetAsm SSE2 : C implementation with SSE2 instructions</description></item>
    /// <item><description>NetAsm Std  : C implementation without SSE2 instructions (standard floating point instructions)</description></item>
    /// <item><description>Managed Std : The same code implemented in C#.</description></item>
    /// <item><description>Managed Unsafe : Same than Managed Std, but using unsafe pointer to matrices. </description></item>
    /// <item><description>Interop Std : Same implementation as NetAsm Std but using standard interop to call the C function. </description></item>
    /// <item><description>Interop SSE2 : Same implementation as NetAsm SSE2 but using standard interop to call the C function. </description></item>
    /// <item><description>Mixed C++/Cli SSE2: Use a managed C++/CLI interface to perform an indirect call to the SSE2</description></item>
    /// </list>
    /// </summary>
    [AllowCodeInjection]
    unsafe class TestMatrixMulBenchmark
    {
        private static int MAX_CALL = 1000000;
        private static int MAXCOUNT = 5;
        static float[] m1 = new float[16];
        static float[] m2 = new float[16];
        static float[] m3 = new float[16];
        static float[] om1 = new float[16];
        static float* pm1;
        static float* pm2;
        static float* pm3;
        static PerfManager perf;

        /// <summary>
        /// Matrixes the mul managed standard.
        /// </summary>
        public static void MatrixMulManagedStd(float[] m1, float[] m2, float[] o)
        {
            o[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
            o[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
            o[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
            o[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

            o[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
            o[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
            o[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
            o[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

            o[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
            o[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
            o[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
            o[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

            o[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
            o[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
            o[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
            o[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
        }

        /// <summary>
        /// Matrixes the mul managed unsafe.
        /// </summary>
        public static void MatrixMulManagedUnsafe(float* m1, float* m2, float* o)
        {
            o[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
            o[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
            o[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
            o[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

            o[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
            o[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
            o[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
            o[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

            o[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
            o[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
            o[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
            o[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

            o[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
            o[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
            o[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
            o[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
        }

        /// <summary>
        /// Matrixes Mul using interop SSE2.
        /// </summary>
        [DllImport("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall,
            EntryPoint = "matrix_mul_sse2_stdcall"), SuppressUnmanagedCodeSecurity]
        public static extern void MatrixMulInteropNoSecuritySSE2(float* m1, float* m2, float* o);

        /// <summary>
        /// Matrixes Mul using interop SSE2.
        /// </summary>
        [DllImport("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall,
            EntryPoint = "matrix_mul_sse2_stdcall")]
        public static extern  void MatrixMulInteropSSE2(float* m1, float* m2, float* o);

        /// <summary>
        /// Matrixes Mul using interop Std.
        /// </summary>
        [DllImport("NetAsmDemoCLib.dll", CallingConvention = CallingConvention.StdCall,
        EntryPoint = "matrix_mul_std_stdcall")]
        public static extern void MatrixMulInteropStd(float* m1, float* m2, float* o);

        /// <summary>
        /// Matrixes Mul using NetAsm SSE2.
        /// </summary>
        [CodeInjection("NetAsmDemoCLib.dll", "matrix_mul_sse2_clrcall")]
        public static  void MatrixMulNetAsmSSE2(float* m1, float* m2, float* o)
        {
            // The IL code of this method is not used if the NetAsm Hook is installed
            // The Dll C function matrix_mul_sse2_fastcall is used instead.
            o[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
            o[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
            o[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
            o[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

            o[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
            o[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
            o[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
            o[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

            o[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
            o[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
            o[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
            o[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

            o[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
            o[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
            o[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
            o[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
        }

        /// <summary>
        /// Matrixes Mul using NetAsm Std.
        /// </summary>
        [CodeInjection("NetAsmDemoCLib.dll", "matrix_mul_std_clrcall")]
        public static void MatrixMulNetAsmStd(float* m1, float* m2, float* o)
        {
            // The IL code of this method is not used if the NetAsm Hook is installed
            // The Dll C function matrix_mul_std_fastcall is used instead.
            o[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
            o[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
            o[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
            o[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

            o[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
            o[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
            o[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
            o[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

            o[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
            o[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
            o[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
            o[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

            o[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
            o[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
            o[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
            o[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
        }


        public static void RunInteropSSE2()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulInteropSSE2(pm1, pm2, pm3);
                MatrixMulInteropSSE2(pm3, pm2, pm1);
                MatrixMulInteropSSE2(pm1, pm2, pm3);
                MatrixMulInteropSSE2(pm3, pm2, pm1);
                MatrixMulInteropSSE2(pm1, pm2, pm3);
                MatrixMulInteropSSE2(pm3, pm2, pm1);
                MatrixMulInteropSSE2(pm1, pm2, pm3);
                MatrixMulInteropSSE2(pm3, pm2, pm1);
                MatrixMulInteropSSE2(pm1, pm2, pm3);
                MatrixMulInteropSSE2(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }            
        }

        public static void RunInteropNoSecuritySSE2()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulInteropNoSecuritySSE2(pm1, pm2, pm3);
                MatrixMulInteropNoSecuritySSE2(pm3, pm2, pm1);
                MatrixMulInteropNoSecuritySSE2(pm1, pm2, pm3);
                MatrixMulInteropNoSecuritySSE2(pm3, pm2, pm1);
                MatrixMulInteropNoSecuritySSE2(pm1, pm2, pm3);
                MatrixMulInteropNoSecuritySSE2(pm3, pm2, pm1);
                MatrixMulInteropNoSecuritySSE2(pm1, pm2, pm3);
                MatrixMulInteropNoSecuritySSE2(pm3, pm2, pm1);
                MatrixMulInteropNoSecuritySSE2(pm1, pm2, pm3);
                MatrixMulInteropNoSecuritySSE2(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }
        }

        public static void RunInteropStd()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulInteropStd(pm1, pm2, pm3);
                MatrixMulInteropStd(pm3, pm2, pm1);
                MatrixMulInteropStd(pm1, pm2, pm3);
                MatrixMulInteropStd(pm3, pm2, pm1);
                MatrixMulInteropStd(pm1, pm2, pm3);
                MatrixMulInteropStd(pm3, pm2, pm1);
                MatrixMulInteropStd(pm1, pm2, pm3);
                MatrixMulInteropStd(pm3, pm2, pm1);
                MatrixMulInteropStd(pm1, pm2, pm3);
                MatrixMulInteropStd(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }
        }

        public static void RunManagedStd()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulManagedStd(m1, m2, m3);
                MatrixMulManagedStd(m3, m2, m1);
                MatrixMulManagedStd(m1, m2, m3);
                MatrixMulManagedStd(m3, m2, m1);
                MatrixMulManagedStd(m1, m2, m3);
                MatrixMulManagedStd(m3, m2, m1);
                MatrixMulManagedStd(m1, m2, m3);
                MatrixMulManagedStd(m3, m2, m1);
                MatrixMulManagedStd(m1, m2, m3);
                MatrixMulManagedStd(m3, m2, m1);
                Array.Copy(om1, m1, 16);
            }            
        }

        public static void RunManagedUnsafe()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulManagedUnsafe(pm1, pm2, pm3);
                MatrixMulManagedUnsafe(pm3, pm2, pm1);
                MatrixMulManagedUnsafe(pm1, pm2, pm3);
                MatrixMulManagedUnsafe(pm3, pm2, pm1);
                MatrixMulManagedUnsafe(pm1, pm2, pm3);
                MatrixMulManagedUnsafe(pm3, pm2, pm1);
                MatrixMulManagedUnsafe(pm1, pm2, pm3);
                MatrixMulManagedUnsafe(pm3, pm2, pm1);
                MatrixMulManagedUnsafe(pm1, pm2, pm3);
                MatrixMulManagedUnsafe(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }
        }

        public static void RunMixedCppCli()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MixedClass.matrix_mul(pm1, pm2, pm3);
                MixedClass.matrix_mul(pm3, pm2, pm1);
                MixedClass.matrix_mul(pm1, pm2, pm3);
                MixedClass.matrix_mul(pm3, pm2, pm1);
                MixedClass.matrix_mul(pm1, pm2, pm3);
                MixedClass.matrix_mul(pm3, pm2, pm1);
                MixedClass.matrix_mul(pm1, pm2, pm3);
                MixedClass.matrix_mul(pm3, pm2, pm1);
                MixedClass.matrix_mul(pm1, pm2, pm3);
                MixedClass.matrix_mul(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }            
        }

        public static void RunNetAsmSSE2()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulNetAsmSSE2(pm1, pm2, pm3);
                MatrixMulNetAsmSSE2(pm3, pm2, pm1);
                MatrixMulNetAsmSSE2(pm1, pm2, pm3);
                MatrixMulNetAsmSSE2(pm3, pm2, pm1);
                MatrixMulNetAsmSSE2(pm1, pm2, pm3);
                MatrixMulNetAsmSSE2(pm3, pm2, pm1);
                MatrixMulNetAsmSSE2(pm1, pm2, pm3);
                MatrixMulNetAsmSSE2(pm3, pm2, pm1);
                MatrixMulNetAsmSSE2(pm1, pm2, pm3);
                MatrixMulNetAsmSSE2(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }            
        }

        public static void RunNetAsmStd()
        {
            Array.Copy(om1, m1, 16);
            for (int i = 0; i < MAX_CALL; i++)
            {
                MatrixMulNetAsmStd(pm1, pm2, pm3);
                MatrixMulNetAsmStd(pm3, pm2, pm1);
                MatrixMulNetAsmStd(pm1, pm2, pm3);
                MatrixMulNetAsmStd(pm3, pm2, pm1);
                MatrixMulNetAsmStd(pm1, pm2, pm3);
                MatrixMulNetAsmStd(pm3, pm2, pm1);
                MatrixMulNetAsmStd(pm1, pm2, pm3);
                MatrixMulNetAsmStd(pm3, pm2, pm1);
                MatrixMulNetAsmStd(pm1, pm2, pm3);
                MatrixMulNetAsmStd(pm3, pm2, pm1);
                Array.Copy(om1, m1, 16);
            }
        }

        [Description("Matrix Mul Benchmark")]
        public static void Run()
        {
           perf = PerfManager.Instance;
           Random random = new Random();
            for (int i = 0; i < 16; i++)
            {
                m1[i] = (float)random.NextDouble();
                om1[i] = m1[i];
                m2[i] = (float)random.NextDouble();
                m3[i] = 0.0f;
            }

            fixed (float* pm1F = &m1[0], pm2F = &m2[0], pm3F = &m3[0])
            {
                pm1 = pm1F;
                pm2 = pm2F;
                pm3 = pm3F;

                perf.WriteLine("Start Benchmark of MatrixMul : Interop, Managed, NetAsm");


                for (int count = 0; count < MAXCOUNT; count++)
                {
                    if (count == 0)
                    {
                        perf.WriteLine("Warm-UP");
                    } else
                    {
                        perf.WriteLine("Count({0}/{1})", count, MAXCOUNT - 1);
                    }

                    perf.Run(RunNetAsmStd);
                    perf.Run(RunNetAsmSSE2);
                    perf.Run(RunInteropStd);
                    perf.Run(RunInteropSSE2);
                    perf.Run(RunInteropNoSecuritySSE2);
                    perf.Run(RunManagedStd);
                    perf.Run(RunManagedUnsafe);
                    perf.Run(RunMixedCppCli);

                    if ( count == 0 )
                    {
                        // First count is not taken into account
                        perf.Reset();
                    }

                    perf.WriteLine();
                }
                perf.DisplayAverage();
                perf.WriteLine("End Benchmark");
                perf.WriteLine();
            }
        }
    }
}
