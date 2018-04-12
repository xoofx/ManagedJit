// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using NetAsm;
using NetAsmDemoSafeGarden;

namespace NetAsmDemo
{
    /// <summary>
    /// Demonstration of NetAsm using a hack on a byte buffer. .NET doesn't allow to to cast (or map) a byte[] to a float[] (unless you use
    /// a union in an explicit StructLayout). Even if you know that the number of bytes is a multiple of a size of a float (4 bytes).
    /// This class use the structure of a managed array (see corinfo.h in SSCLI2.0) : 
    /// <code>
    /// struct ManagedArray {
    ///        void* pointerToVTable;
    ///        int length;
    ///        typeOfArray[] array;
    ///        ...
    /// }</code>
    /// The native code in the CastToFloatArray function copy the Vtable of a float buffer to the Vtable of the byte buffer, and it
    /// divide by 4 the length of the buffer and return the bytebuffer as a floatbuffer.
    /// 
    /// WARNING, when the byte[] buffer is converted to a float[], the original byte[] buffer has a length /4. You can't use the original
    /// length unless you transform it back the float buffer to the byte buffer.
    /// </summary>
    [AllowCodeInjection]
    class TestCast
    {
        private static float[] EMPTY_FLOAT_BUFFER = new float[0];

        /// <summary>
        /// Convert with a custom cast from a byte[] to a float[], keeping the same memory layout.
        /// </summary>
        /// <param name="buffer">The byte buffer to convert.</param>
        /// <returns>the float buffer that is mapped to the byte buffer</returns>
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static float[] CastConvert(byte[] buffer)
        {
            if ( (buffer.Length % 4) != 0 )
            {
                throw new ArgumentException("byte[] buffer must have a length of modulo 4 bytes");
            }
            return CastToFloatArray(buffer, EMPTY_FLOAT_BUFFER);
        }

        // [CodeInjection("NetAsmDemoCLib.dll", "CastByteToFloat"), MethodImpl(MethodImplOptions.NoInlining)]
        [CodeInjection( new byte[] {
            // Static code injection. Code extracted from function CastByteToFloat in NetAsmDemoCLib :
            //extern "C" CORINFO_Array*  __fastcall CastByteToFloat(CORINFO_Array* byteBuffer, CORINFO_Array* floatBuffer)
            //{
            //    // Use the VTable of the float buffer to switch the byteBuffer to a float Buffer
            //    byteBuffer->methTable = floatBuffer->methTable;
            //    // Divide by 4 the size
            //    byteBuffer->length = byteBuffer->length / 4;
            //    // Return the byteBuffer (now a floatbuffer)
            //    return byteBuffer;
            //}
            0x8b,0xc1 		 // mov	 eax, ecx
            ,0x8b,0x0a       // mov	 ecx, DWORD PTR [edx]
            ,0xc1,0x68,0x04,0x02 // shr	 DWORD PTR [eax+4], 2
            ,0x89,0x08		 // mov	 DWORD PTR [eax], ecx
            ,0xc3		     // ret	 0
        }), MethodImpl(MethodImplOptions.NoInlining)]
        private static float[] CastToFloatArray(byte[] buffer, float[] floatBuffer)
        {
            throw new ArgumentException("Function is not supported without NetAsm");
        }

        /// <summary>
        /// Runs this instance.
        /// </summary>
        [Description("Cast Hack Demo")]
        public static void Run()
        {
            byte[] byteBuffer = new byte[64];
            PerfManager.Instance.WriteLine("ByteBuffer Len before cast : {0}", byteBuffer.Length);

            float[] floatBuffer = CastConvert(byteBuffer);
            PerfManager.Instance.WriteLine("ByteBuffer Len after cast : {0}", byteBuffer.Length);
            PerfManager.Assert(floatBuffer.Length == 16, "Bad Length on float buffer. Expected 16");

            floatBuffer[0] = 1.0f;
            PerfManager.Assert(byteBuffer[3] == 0x3F, "Error, NetAsm hook on method NetAsmAdd is not set");

            PerfManager.Instance.WriteLine("Cast byte[] to float[] successfull");            
        }
    }
}
