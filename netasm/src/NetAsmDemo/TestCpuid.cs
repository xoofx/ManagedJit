// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Simple class that implements cpuid assembler function. 
    /// Use static code injection but the code is also written in C (see cpuid.cpp in NetAsmDemoCLib.dll)
    /// </summary>
    [AllowCodeInjection]
    public class Cpuid
    {
        /// <summary>
        /// Determines whether cpuid is supported.
        /// </summary>
        /// <returns>
        /// 	<c>true</c> if cpuid is supported; otherwise, <c>false</c>.
        /// </returns>
        [CodeInjection(new byte[] {0x53,0x9c,0x58,0x8b,0xc8,0x35,0x00,0x00,0x20,0x00,0x50,0x9d,0x9c,0x5b,0x33,0xc3,0x25,0x00,0x00,0x20,0x00
              ,0x75,0x07,0xb8,0x01,0x00,0x00,0x00,0xeb,0x02,0x33,0xc0,0x51,0x9d,0x5b,0xc3}), MethodImpl(MethodImplOptions.NoInlining)]
//        [CodeInjection("NetAsmDemoCLib.dll", "IsCpuidSupported"), MethodImpl(MethodImplOptions.NoInlining)]
        static public bool IsSupported()
        {
            return false;
        }

        /// <summary>
        /// Get the cpuid for a specific function number.
        /// </summary>
        /// <param name="funcNumber">The function number.</param>
        /// <param name="outEax">The out eax.</param>
        /// <param name="outEbx">The out ebx.</param>
        /// <param name="outEcx">The out ecx.</param>
        /// <param name="outEdx">The out edx.</param>
        [CodeInjection( new byte[] {0x83,0xec,0x08,0x53, 0x57,0x89,0x54,0x24,0x08,0x89,0x4c,0x24,0x0c,0x8b,0x44,0x24,0x0c,0x0f,0xa2,
            0x8b,0x7c,0x24,0x08,0x89,0x07,0x8b,0x7c,0x24,0x18,0x89,0x0f,0x8b,0x7c,0x24,0x14,0x89,0x17,
            0x8b,0x7c,0x24,0x1c,0x89,0x1f,0x5f,0x5b,0x83,0xc4,0x08,0xc2,0x0c,0x00}), MethodImpl(MethodImplOptions.NoInlining)]
//        [CodeInjection("NetAsmDemoCLib.dll", "Cpuid"), MethodImpl(MethodImplOptions.NoInlining)]
        static public void Get(uint funcNumber, out uint outEax, out uint outEbx, out uint outEcx, out uint outEdx)
        {
            throw new Exception("Not supported without NetAsm");
        }

        /// <summary>
        /// Convert a list of dwords to a concatenated string.
        /// </summary>
        /// <param name="dwords">The dwords.</param>
        /// <returns>a string</returns>
        public static string dword2str(params uint[] dwords)
        {
            StringBuilder builder = new StringBuilder();
            foreach (uint dword in dwords)
            {
                builder.Append(dword2str(dword));
            }
            return builder.ToString();
        }

        /// <summary>
        /// Convert a dword to a string
        /// </summary>
        /// <param name="dword">The dword.</param>
        /// <returns>the resulting string</returns>
        public static string dword2str(uint dword)
        {
            uint dxl = (dword & 0xff);
            uint dxh = (dword & 0xff00) >> 8;
            uint dlx = (dword & 0xff0000) >> 16;
            uint dhx = (dword & 0xff000000) >> 24;

            return
                ((char)dxl).ToString() +
                ((char)dxh).ToString() +
                ((char)dlx).ToString() +
                ((char)dhx).ToString();
        }
    }

    /// <summary>
    /// This simple example shows cpuid function. The code is a C# implementation of the __cpuid code in the 
    /// Visual C++ Language Reference.
    /// </summary>
    [AllowCodeInjection]
    class TestCpuid
    {
        static string[] szFeatures = new string[] 
        {
            "x87 FPU On Chip",
            "Virtual-8086 Mode Enhancement",
            "Debugging Extensions",
            "Page Size Extensions",
            "Time Stamp Counter",
            "RDMSR and WRMSR Support",
            "Physical Address Extensions",
            "Machine Check Exception",
            "CMPXCHG8B Instruction",
            "APIC On Chip",
            "Unknown1",
            "SYSENTER and SYSEXIT",
            "Memory Type Range Registers",
            "PTE Global Bit",
            "Machine Check Architecture",
            "Conditional Move/Compare Instruction",
            "Page Attribute Table",
            "36-bit Page Size Extension",
            "Processor Serial Number",
            "CFLUSH Extension",
            "Unknown2",
            "Debug Store",
            "Thermal Monitor and Clock Ctrl",
            "MMX Technology",
            "FXSAVE/FXRSTOR",
            "SSE Extensions",
            "SSE2 Extensions",
            "Self Snoop",
            "Multithreading Technology",
            "Thermal Monitor",
            "Unknown4",
            "Pend. Brk. EN."
        };

        private static void __cpuid(uint[] CPUInfo, uint infoType)
        {
            Cpuid.Get(infoType, out CPUInfo[0], out CPUInfo[1], out CPUInfo[2], out CPUInfo[3]);
        }

        [Description("Test CPUID")]
        static public void Run()
        {
            String CPUString = "";
            String CPUBrandString = "";
            uint[] CPUInfo = new uint[4];
            uint nSteppingID = 0;
            uint nModel = 0;
            uint nFamily = 0;
            uint nProcessorType = 0;
            uint nExtendedmodel = 0;
            uint nExtendedfamily = 0;
            uint nBrandIndex = 0;
            uint nCLFLUSHcachelinesize = 0;
            uint nLogicalProcessors = 0;
            uint nAPICPhysicalID = 0;
            uint nFeatureInfo = 0;
            uint nCacheLineSize = 0;
            uint nL2Associativity = 0;
            uint nCacheSizeK = 0;
            uint nPhysicalAddress = 0;
            uint nVirtualAddress = 0;
            uint nIds, nExIds, i;

            bool bSSE3Instructions = false;
            bool bMONITOR_MWAIT = false;
            bool bCPLQualifiedDebugStore = false;
            bool bVirtualMachineExtensions = false;
            bool bEnhancedIntelSpeedStepTechnology = false;
            bool bThermalMonitor2 = false;
            bool bSupplementalSSE3 = false;
            bool bL1ContextID = false;
            bool bCMPXCHG16B = false;
            bool bxTPRUpdateControl = false;
            bool bPerfDebugCapabilityMSR = false;
            bool bSSE41Extensions = false;
            bool bSSE42Extensions = false;
            bool bPOPCNT = false;

            bool bMultithreading = false;

            bool bLAHF_SAHFAvailable = false;
            bool bCmpLegacy = false;
            bool bSVM = false;
            bool bExtApicSpace = false;
            bool bAltMovCr8 = false;
            bool bLZCNT = false;
            bool bSSE4A = false;
            bool bMisalignedSSE = false;
            bool bPREFETCH = false;
            bool bSKINITandDEV = false;
            bool bSYSCALL_SYSRETAvailable = false;
            bool bExecuteDisableBitAvailable = false;
            bool bMMXExtensions = false;
            bool bFFXSR = false;
            bool b1GBSupport = false;
            bool bRDTSCP = false;
            bool b64Available = false;
            bool b3DNowExt = false;
            bool b3DNow = false;
            bool bNestedPaging = false;
            bool bLBRVisualization = false;
            bool bFP128 = false;
            bool bMOVOptimization = false;

            PerfManager perf = PerfManager.Instance;


            if (!Cpuid.IsSupported())
            {
                perf.WriteLine("Sorry, CPUID is not supported...");
                return;
            }

            // __cpuid with an InfoType argument of 0 returns the number of
            // valid Ids in CPUInfo[0] and the CPU identification string in
            // the other three array elements. The CPU identification string is
            // not in linear order. The code below arranges the information 
            // in a human readable form.
            __cpuid(CPUInfo, 0);
            nIds = CPUInfo[0];
            CPUString = Cpuid.dword2str(CPUInfo[1], CPUInfo[3], CPUInfo[2]);

            // Get the information associated with each valid Id
            for (i = 0; i <= nIds; ++i)
            {
                __cpuid(CPUInfo, i);
                //perf.WriteLine();
                //perf.WriteLine("For InfoType {0}", i);
                //perf.WriteLine("CPUInfo[0] = 0x{0:X}", CPUInfo[0]);
                //perf.WriteLine("CPUInfo[1] = 0x{0:X}", CPUInfo[1]);
                //perf.WriteLine("CPUInfo[2] = 0x{0:X}", CPUInfo[2]);
                //perf.WriteLine("CPUInfo[3] = 0x{0:X}", CPUInfo[3]);

                // Interpret CPU feature information.
                if (i == 1)
                {
                    nSteppingID = CPUInfo[0] & 0xf;
                    nModel = (CPUInfo[0] >> 4) & 0xf;
                    nFamily = (CPUInfo[0] >> 8) & 0xf;
                    nProcessorType = (CPUInfo[0] >> 12) & 0x3;
                    nExtendedmodel = (CPUInfo[0] >> 16) & 0xf;
                    nExtendedfamily = (CPUInfo[0] >> 20) & 0xff;
                    nBrandIndex = CPUInfo[1] & 0xff;
                    nCLFLUSHcachelinesize = ((CPUInfo[1] >> 8) & 0xff) * 8;
                    nLogicalProcessors = ((CPUInfo[1] >> 16) & 0xff);
                    nAPICPhysicalID = (CPUInfo[1] >> 24) & 0xff;
                    bSSE3Instructions = (CPUInfo[2] & 0x1) != 0;
                    bMONITOR_MWAIT = (CPUInfo[2] & 0x8) != 0;
                    bCPLQualifiedDebugStore = (CPUInfo[2] & 0x10) != 0;
                    bVirtualMachineExtensions = (CPUInfo[2] & 0x20) != 0;
                    bEnhancedIntelSpeedStepTechnology = (CPUInfo[2] & 0x80) != 0;
                    bThermalMonitor2 = (CPUInfo[2] & 0x100) != 0;
                    bSupplementalSSE3 = (CPUInfo[2] & 0x200) != 0;
                    bL1ContextID = (CPUInfo[2] & 0x300) != 0;
                    bCMPXCHG16B = (CPUInfo[2] & 0x2000) != 0;
                    bxTPRUpdateControl = (CPUInfo[2] & 0x4000) != 0;
                    bPerfDebugCapabilityMSR = (CPUInfo[2] & 0x8000) != 0;
                    bSSE41Extensions = (CPUInfo[2] & 0x80000) != 0;
                    bSSE42Extensions = (CPUInfo[2] & 0x100000) != 0;
                    bPOPCNT = (CPUInfo[2] & 0x800000) != 0;
                    nFeatureInfo = CPUInfo[3];
                    bMultithreading = (nFeatureInfo & (1 << 28)) != 0;
                }
            }

            // Calling __cpuid with 0x80000000 as the InfoType argument
            // gets the number of valid extended IDs.
            __cpuid(CPUInfo, 0x80000000);
            nExIds = CPUInfo[0];

            // Get the information associated with each extended ID.
            for (i = 0x80000000; i <= nExIds; ++i)
            {
                __cpuid(CPUInfo, i);
                //perf.WriteLine();
                //perf.WriteLine("For InfoType {0:X}", i);
                //perf.WriteLine("CPUInfo[0] = 0x{0:X}", CPUInfo[0]);
                //perf.WriteLine("CPUInfo[1] = 0x{0:X}", CPUInfo[1]);
                //perf.WriteLine("CPUInfo[2] = 0x{0:X}", CPUInfo[2]);
                //perf.WriteLine("CPUInfo[3] = 0x{0:X}", CPUInfo[3]);

                if (i == 0x80000001)
                {
                    bLAHF_SAHFAvailable = (CPUInfo[2] & 0x1) != 0;
                    bCmpLegacy = (CPUInfo[2] & 0x2) != 0;
                    bSVM = (CPUInfo[2] & 0x4) != 0;
                    bExtApicSpace = (CPUInfo[2] & 0x8) != 0;
                    bAltMovCr8 = (CPUInfo[2] & 0x10) != 0;
                    bLZCNT = (CPUInfo[2] & 0x20) != 0;
                    bSSE4A = (CPUInfo[2] & 0x40) != 0;
                    bMisalignedSSE = (CPUInfo[2] & 0x80) != 0;
                    bPREFETCH = (CPUInfo[2] & 0x100) != 0;
                    bSKINITandDEV = (CPUInfo[2] & 0x1000) != 0;
                    bSYSCALL_SYSRETAvailable = (CPUInfo[3] & 0x800) != 0;
                    bExecuteDisableBitAvailable = (CPUInfo[3] & 0x10000) != 0;
                    bMMXExtensions = (CPUInfo[3] & 0x40000) != 0;
                    bFFXSR = (CPUInfo[3] & 0x200000) != 0;
                    b1GBSupport = (CPUInfo[3] & 0x400000) != 0;
                    bRDTSCP = (CPUInfo[3] & 0x8000000) != 0;
                    b64Available = (CPUInfo[3] & 0x20000000) != 0;
                    b3DNowExt = (CPUInfo[3] & 0x40000000) != 0;
                    b3DNow = (CPUInfo[3] & 0x80000000) != 0;
                }

                // Interpret CPU brand string and cache information.
                if (i == 0x80000002)
                    CPUBrandString = CPUBrandString + Cpuid.dword2str(CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
                else if (i == 0x80000003)
                    CPUBrandString = CPUBrandString + Cpuid.dword2str(CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
                else if (i == 0x80000004)
                    CPUBrandString = CPUBrandString + Cpuid.dword2str(CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
                else if (i == 0x80000006)
                {
                    nCacheLineSize = CPUInfo[2] & 0xff;
                    nL2Associativity = (CPUInfo[2] >> 12) & 0xf;
                    nCacheSizeK = (CPUInfo[2] >> 16) & 0xffff;
                }
                else if (i == 0x80000008)
                {
                    nPhysicalAddress = CPUInfo[0] & 0xff;
                    nVirtualAddress = (CPUInfo[0] >> 8) & 0xff;
                }
                else if (i == 0x8000000A)
                {
                    bNestedPaging = (CPUInfo[3] & 0x1) != 0;
                    bLBRVisualization = (CPUInfo[3] & 0x2) != 0;
                }
                else if (i == 0x8000001A)
                {
                    bFP128 = (CPUInfo[0] & 0x1) != 0;
                    bMOVOptimization = (CPUInfo[0] & 0x2) != 0;
                }
            }

            // Display all the information in user-friendly format.

            perf.WriteLine();
            perf.WriteLine("CPU String: {0}", CPUString);

            if (nIds >= 1)
            {
                if (nSteppingID != 0)
                    perf.WriteLine("Stepping ID = {0}", nSteppingID);
                if (nModel != 0)
                    perf.WriteLine("Model = {0}", nModel);
                if (nFamily != 0)
                    perf.WriteLine("Family = {0}", nFamily);
                if (nProcessorType != 0)
                    perf.WriteLine("Processor Type = {0}", nProcessorType);
                if (nExtendedmodel != 0)
                    perf.WriteLine("Extended model = {0}", nExtendedmodel);
                if (nExtendedfamily != 0)
                    perf.WriteLine("Extended family = {0}", nExtendedfamily);
                if (nBrandIndex != 0)
                    perf.WriteLine("Brand Index = {0}", nBrandIndex);
                if (nCLFLUSHcachelinesize != 0)
                    perf.WriteLine("CLFLUSH cache line size = {0}", nCLFLUSHcachelinesize);
                if (bMultithreading && (nLogicalProcessors > 0))
                    perf.WriteLine("Logical Processor Count = {0}", nLogicalProcessors);
                if (nAPICPhysicalID != 0)
                    perf.WriteLine("APIC Physical ID = {0}", nAPICPhysicalID);

                if (nFeatureInfo != 0 || bSSE3Instructions ||
                     bMONITOR_MWAIT || bCPLQualifiedDebugStore ||
                     bVirtualMachineExtensions || bEnhancedIntelSpeedStepTechnology ||
                     bThermalMonitor2 || bSupplementalSSE3 || bL1ContextID ||
                     bCMPXCHG16B || bxTPRUpdateControl || bPerfDebugCapabilityMSR ||
                     bSSE41Extensions || bSSE42Extensions || bPOPCNT ||
                     bLAHF_SAHFAvailable || bCmpLegacy || bSVM ||
                     bExtApicSpace || bAltMovCr8 ||
                     bLZCNT || bSSE4A || bMisalignedSSE ||
                     bPREFETCH || bSKINITandDEV || bSYSCALL_SYSRETAvailable ||
                     bExecuteDisableBitAvailable || bMMXExtensions || bFFXSR || b1GBSupport ||
                     bRDTSCP || b64Available || b3DNowExt || b3DNow || bNestedPaging ||
                     bLBRVisualization || bFP128 || bMOVOptimization)
                {
                    perf.WriteLine();
                    perf.WriteLine("The following features are supported:");

                    if (bSSE3Instructions)
                        perf.WriteLine("    SSE3");
                    if (bMONITOR_MWAIT)
                        perf.WriteLine("    MONITOR/MWAIT");
                    if (bCPLQualifiedDebugStore)
                        perf.WriteLine("    CPL Qualified Debug Store");
                    if (bVirtualMachineExtensions)
                        perf.WriteLine("    Virtual Machine Extensions");
                    if (bEnhancedIntelSpeedStepTechnology)
                        perf.WriteLine("    Enhanced Intel SpeedStep Technology");
                    if (bThermalMonitor2)
                        perf.WriteLine("    Thermal Monitor 2");
                    if (bSupplementalSSE3)
                        perf.WriteLine("    Supplemental Streaming SIMD Extensions 3");
                    if (bL1ContextID)
                        perf.WriteLine("    L1 Context ID");
                    if (bCMPXCHG16B)
                        perf.WriteLine("    CMPXCHG16B Instruction");
                    if (bxTPRUpdateControl)
                        perf.WriteLine("    xTPR Update Control");
                    if (bPerfDebugCapabilityMSR)
                        perf.WriteLine("    Perf\\Debug Capability MSR");
                    if (bSSE41Extensions)
                        perf.WriteLine("    SSE4.1 Extensions");
                    if (bSSE42Extensions)
                        perf.WriteLine("    SSE4.2 Extensions");
                    if (bPOPCNT)
                        perf.WriteLine("    PPOPCNT Instruction");

                    for (nIds = 1, i = 0; i < szFeatures.Length; i++)
                    {
                        if ((nFeatureInfo & nIds) != 0)
                        {
                            perf.WriteLine("    " + szFeatures[i]);
                        }
                        nIds <<= 1;
                    }
                    if (bLAHF_SAHFAvailable)
                        perf.WriteLine("    LAHF/SAHF in 64-bit mode");
                    if (bCmpLegacy)
                        perf.WriteLine("    Core multi-processing legacy mode");
                    if (bSVM)
                        perf.WriteLine("    Secure Virtual Machine");
                    if (bExtApicSpace)
                        perf.WriteLine("    Extended APIC Register Space");
                    if (bAltMovCr8)
                        perf.WriteLine("    AltMovCr8");
                    if (bLZCNT)
                        perf.WriteLine("    LZCNT instruction");
                    if (bSSE4A)
                        perf.WriteLine("    SSE4A (EXTRQ, INSERTQ, MOVNTSD, MOVNTSS)");
                    if (bMisalignedSSE)
                        perf.WriteLine("    Misaligned SSE mode");
                    if (bPREFETCH)
                        perf.WriteLine("    PREFETCH and PREFETCHW Instructions");
                    if (bSKINITandDEV)
                        perf.WriteLine("    SKINIT and DEV support");
                    if (bSYSCALL_SYSRETAvailable)
                        perf.WriteLine("    SYSCALL/SYSRET in 64-bit mode");
                    if (bExecuteDisableBitAvailable)
                        perf.WriteLine("    Execute Disable Bit");
                    if (bMMXExtensions)
                        perf.WriteLine("    Extensions to MMX Instructions");
                    if (bFFXSR)
                        perf.WriteLine("    FFXSR");
                    if (b1GBSupport)
                        perf.WriteLine("    1GB page support");
                    if (bRDTSCP)
                        perf.WriteLine("    RDTSCP instruction");
                    if (b64Available)
                        perf.WriteLine("    64 bit Technology");
                    if (b3DNowExt)
                        perf.WriteLine("    3Dnow Ext");
                    if (b3DNow)
                        perf.WriteLine("    3Dnow! instructions");
                    if (bNestedPaging)
                        perf.WriteLine("    Nested Paging");
                    if (bLBRVisualization)
                        perf.WriteLine("    LBR Visualization");
                    if (bFP128)
                        perf.WriteLine("    FP128 optimization");
                    if (bMOVOptimization)
                        perf.WriteLine("    MOVU Optimization");
                }
            }

            perf.WriteLine();
            if (nExIds >= 0x80000004)
                perf.WriteLine("CPU Brand String: {0}", CPUBrandString);

            if (nExIds >= 0x80000006)
            {
                perf.WriteLine("Cache Line Size = {0}", nCacheLineSize);
                perf.WriteLine("L2 Associativity = {0}", nL2Associativity);
                perf.WriteLine("Cache Size = {0}K", nCacheSizeK);
            }

            if (nExIds >= 0x80000008)
            {
                perf.WriteLine("Physical Address = {0}", nPhysicalAddress);
                perf.WriteLine("Virtual Address = {0}", nVirtualAddress);
            }
        }
    }
}
