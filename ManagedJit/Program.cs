using System;
using System.Runtime.CompilerServices;

namespace HackCoreCLR
{
    class Program
    {
        static void Main(string[] args)
        {
            using (var clrJit = ManagedJit.GetOrCreate())
            {
                // It will print `1 + 2 = 4!` instead of the expected 3
                var result = JitReplaceAdd(1, 2);
                Console.WriteLine($"{nameof(JitReplaceAdd)}: 1 + 2 = {result}!");
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static int JitReplaceAdd(int a, int b) => a + b;
    }
}
