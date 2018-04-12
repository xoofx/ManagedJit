// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System.ComponentModel;
using System.Runtime.CompilerServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Simple structure used. See file NetAsmDemoCLibClrCall.cpp to see the C++ equivalent.
    /// </summary>
    struct SimpleStructure
    {
        public byte x;
        public int y;
        public double z;
    }

    /// <summary>
    /// This simple example shows how to use a structure as a parameter.
    /// The only supported calling conventions for structure is ClrCall. Structure are not - yet - working with other calling conventions.
    /// </summary>
    [AllowCodeInjection]
    class TestStructure
    {
        /// <summary>
        /// Use of a structure as a parameter.
        /// </summary>
        [CodeInjection("NetAsmDemoCLib.dll"), MethodImpl(MethodImplOptions.NoInlining)]
        static public int CalculateStructure(SimpleStructure structOnStack, ref SimpleStructure structByRef)
        {
            return 0;
        }

        [Description("Test Structure parameters")]
        static public void Run()
        {
            SimpleStructure simpleStructure;
            simpleStructure.x = 1;
            simpleStructure.y = 2;
            simpleStructure.z = 3;
            SimpleStructure outputSimpleStructure = new SimpleStructure();

            int resultInt = CalculateStructure(simpleStructure, ref outputSimpleStructure);
            PerfManager.Assert(resultInt == (simpleStructure.x + simpleStructure.y * 3 + simpleStructure.z * 5), "Invalid CalculateStructure ByValue");
            PerfManager.Assert((outputSimpleStructure.x == simpleStructure.x 
                                && outputSimpleStructure.y == simpleStructure.y
                                && outputSimpleStructure.z == simpleStructure.z ), "Invalid CalculateStructure ByRef");

        }
    }

}
