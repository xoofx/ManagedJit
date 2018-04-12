// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace NetAsmDemoSafeGarden
{

    /// <summary>
    /// Helper class to measure performance and display methods call.
    /// </summary>
    public class PerfManager
    {
        List<MapMethodToPerf> perfList = new List<MapMethodToPerf>();

        internal class CountPerMethod
        {
            public String name;
            public long totalElapsedTime;
            public int countCall;

        }

        internal class MapMethodToPerf : Dictionary<MethodInfo, CountPerMethod>
        {
            Stopwatch watchTime = new Stopwatch();

            public void Start()
            {
                watchTime.Reset();
                watchTime.Start();
            }

            public void Stop()
            {
                watchTime.Stop();
                
            }

            public long ElapsedMilliseconds
            {
                get
                {
                    return watchTime.ElapsedMilliseconds;
                }
            }
        }

        public delegate void MethodToRun();

        public PerfManager()
        {
            perfList.Add(new MapMethodToPerf());
        }

        private static PerfManager instance = null;
        public static PerfManager Instance
        {
            get {
                if (instance == null)
                {
                    instance = new PerfManager();
                }
                return instance;
            }
        }

        public static void Assert(bool condition)
        {
            Assert(condition, String.Format("on method {0}", new StackFrame().GetMethod()));
        }

        public static void Assert(bool condition, String message)
        {
            if ( ! condition )
            {
                Instance.WriteLine("Check failed {0}", message);
                StackTrace st = new StackTrace(1, true);
                Instance.WriteLine(st.ToString());
                MessageBox.Show(st.ToString(), String.Format("Check Failed {0}", message), MessageBoxButtons.OK, MessageBoxIcon.Error );
            }

        }

        private static String getMethodName(MethodToRun run)
        {
            object[] attributes = run.Method.GetCustomAttributes(typeof (DescriptionAttribute), false);
            if ( attributes.Length > 0 )
            {
                return ((DescriptionAttribute) attributes[0]).Description;
            }

            if ( run.Method.Name.StartsWith("Run") )
            {
                return run.Method.Name.Substring("Run".Length);
            }
            return run.Method.Name;
        }

        private MapMethodToPerf getCurrentPerf()
        {
            return perfList[perfList.Count - 1];
        }

        public void Run(MethodToRun run)
        {
            Run(run, false);
        }

        public void Run(MethodToRun run, bool displayBeginEnd)
        {
            String methodName = getMethodName(run);
            if (displayBeginEnd)
            {
                WriteLine("Begin [{0}]", methodName);
            }
            MapMethodToPerf perfMethod = getCurrentPerf();
            perfMethod.Start();
            perfList.Add(new MapMethodToPerf());
            run();
            perfList.RemoveAt(perfList.Count - 1);
            perfMethod.Stop();

            CountPerMethod countPerMethod;
            perfMethod.TryGetValue(run.Method, out countPerMethod);
            if ( countPerMethod == null )
            {
                countPerMethod = new CountPerMethod();
                countPerMethod.name = methodName;
                countPerMethod.totalElapsedTime = 0;
                countPerMethod.countCall = 0;
                perfMethod.Add(run.Method, countPerMethod);
            }

            long elapsedTime = perfMethod.ElapsedMilliseconds;
            countPerMethod.totalElapsedTime += elapsedTime;
            countPerMethod.countCall++;
            if (displayBeginEnd)
            {
                WriteLine("End [{0}] computed in {1}ms", methodName, elapsedTime);
                WriteLine();
            }
            else
            {
                WriteLine("{0}\t computed in {1}ms", methodName, elapsedTime);
            }
        }

        public void Reset()
        {
            perfList[perfList.Count - 1].Clear();
        }

        public void WriteLine()
        {
            Console.Out.WriteLine();
        }

        public void WriteLine(String format, params Object[] args)
        {
            StringBuilder tabBuilder = new StringBuilder();
            for(int i = 0; i < perfList.Count; i++)
            {
                tabBuilder.Append("    ");
            }
            tabBuilder.Append(format);
            Console.Out.WriteLine(tabBuilder.ToString(), args);
        }

        public void Write(String format, params Object[] args)
        {
            StringBuilder tabBuilder = new StringBuilder();
            for (int i = 0; i < perfList.Count; i++)
            {
                tabBuilder.Append("    ");
            }
            tabBuilder.Append(format);
            Console.Out.Write(tabBuilder.ToString(), args);
        }

        public void DisplayAverage()
        {
            MapMethodToPerf elapsedTimePerMethod = perfList[perfList.Count - 1];
            WriteLine("Average:");
            foreach(CountPerMethod countPerMethod in elapsedTimePerMethod.Values)
            {
                WriteLine("{0}\t{1}ms", countPerMethod.name, (countPerMethod.totalElapsedTime / countPerMethod.countCall));
            }
        }

    }
}
