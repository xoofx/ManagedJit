// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

#include <windows.h>
#include "corinfo.h"
#include "corjit.h"

using namespace System;

class NativeCodeRef {
public:
	static void* AllocExecutionBlock(int size);
	static void* AllocExecutionBlock(ICorJitInfo* comp, int size);
	static void* AllocExecutionBlock(int size, HANDLE hProcess);
	static IntPtr BindToDllFunction(String^ dllName, String^ dllMethodName);
};

#pragma managed

namespace NetAsm {

	/// <summary>
	/// CodeRef contains a reference to an allocated execution memory block of native code.
	/// </summary>
	public value struct CodeRef
	{
		IntPtr Pointer;
		unsigned int Size;

		static CodeRef Zero = CodeRef(IntPtr::Zero,0);

        // <summary>
        // Initializes a new instance of the <see cref="NetAsm::CodeRef"/> struct.
        // </summary>
        // <param name="ptr">A pointer to a code execution block.param>
        // <param name="size">The size.</param>
		CodeRef(IntPtr ptr, unsigned int size) {
			Pointer = ptr;
			Size = size;
		}

        /// <summary>
        /// Performs an implicit conversion from <see cref="CodeRef"/> to System.IntPtr.
        /// </summary>
        /// <returns>The result of the conversion.</returns>
		operator IntPtr() {
			return Pointer;
		}

        /// <summary>
        /// Performs an implicit conversion from <see cref="CodeRef"/> to void*.
        /// </summary>
        /// <returns>The result of the conversion.</returns>
		operator void*() {
			return Pointer.ToPointer();
		}

        /// <summary>
        /// Test if this CodeRef has a null reference.
        /// </summary>
        /// <returns>True if the reference is null.</returns>
		bool IsNull()
		{
			return Pointer == IntPtr::Zero;
		}

        /// <summary>
        /// Retrieve the CodeRef for a function in a DLL.
        /// </summary>
        /// <param name="dllName">Name of the dll.</param>
        /// <param name="dllFunctionName">Name of the function in the dll.</param>
		/// <returns>A CodeRef referencing the dll function.</returns>
		static CodeRef BindToDllFunction(String^ dllName, String^ dllFunctionName) {
			IntPtr dllFunctionPointer = NativeCodeRef::BindToDllFunction(dllName, dllFunctionName);
			return CodeRef(dllFunctionPointer, 5);
		}

//	internal:
        /// <summary>
        /// Alloccate a memory execution block.
        /// </summary>
        /// <param name="size">The size of the memory to allocate.</param>
        /// <returns>A CodeRef referencing the execution memory.</returns>
		static CodeRef AllocExecutionBlock(unsigned int size)
		{
			CodeRef codeRef;
			codeRef.Pointer = IntPtr(NativeCodeRef::AllocExecutionBlock(size, GetCurrentProcess()));
			codeRef.Size = size;
			return codeRef;
		}

        /// <summary>
        /// Alloccate a memory execution block and fill it with a byte buffer of native code.
        /// </summary>
        /// <param name="nativeCodeBuffer">A buffer containinng native code to copy to the allocated execution block.</param>
        /// <returns>A CodeRef referencing the execution memory.</returns>
		static CodeRef AllocExecutionBlock(cli::array<Byte>^ nativeCodeBuffer)
		{
			CodeRef codeRef = AllocExecutionBlock(nativeCodeBuffer->Length);
			System::Runtime::InteropServices::Marshal::Copy(nativeCodeBuffer, 0, codeRef.Pointer, codeRef.Size);
			return codeRef;
		}

	};
}
