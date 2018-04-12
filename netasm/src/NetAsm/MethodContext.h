// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

#include "NativeHook.h"
#include "CodeRef.h"

#pragma managed
using namespace System;

namespace NetAsm {

    /// <summary>
    /// MethodContext contains the context of the method being compiled. It is used in dynamic code injection, as a parameter for 
	/// a subclass of <see cref="NetAsm::ICodeInjector"/>.
    /// </summary>
	public ref class MethodContext
	{
	internal:
		property bool IsCodeAllocatedFromContext { bool get() { return isCodeAllocatedFromContext; } }


	protected:
		NativeMethodContext* context;
		System::Reflection::MethodInfo^ methodInfo;
		bool isNativeCompiled;
		CodeRef nativeCodeRef;
		int stackSize;
		cli::array<Byte>^ nativeCode;
		bool isCodeAllocatedFromContext;

		MethodContext(System::Reflection::MethodInfo^ methodInfoArg, NativeMethodContext* contextArg) {
			methodInfo = methodInfoArg;
			context = contextArg;
			isNativeCompiled = false;
			isCodeAllocatedFromContext = false;
			nativeCodeRef.Pointer = IntPtr::Zero;
			nativeCodeRef.Size = 0;
			nativeCode = nullptr;
		}
	public:
		/// <summary>
        /// Gets the reflection MethodInfo associated with the method being compiled.
        /// </summary>
        /// <value>The reflection MethodInfo.</value>
		property System::Reflection::MethodInfo^ MethodInfo { System::Reflection::MethodInfo^ get() { return methodInfo; } }

		/// <summary>
        /// Compiles the IL code of this method using the native JIT compiler. This method call the original native compiler in order to compile
        /// the ILCode to native code. Use this method if you want to use the native compiled method while still being able to wrap
        /// the compiled code with your own code. 
		/// Use this method with caution, as you loose the debug when compiling a method with it.
        /// </summary>
        /// <returns>a pointer to the compiled code</returns>
	    CodeRef CompileILMethodToNative() {
			if ( ! isNativeCompiled ) {
				context->Compile();
				isNativeCompiled = true;
			}
			return CodeRef(IntPtr(context->nativeEntry), context->nativeSizeOfCode);
		}


		/// <summary>
        /// Gets the stackSize of the current method.
        /// </summary>
        /// <value>The stack size.</value>
		property int StackSize { int get() { return stackSize; } }

		/// <summary>
        /// Gets a pointer to the IL code of this method.
        /// </summary>
        /// <value>The IL code pointer.</value>
		property CodeRef ILCode { CodeRef get() { return CodeRef(IntPtr(context->info->ILCode), context->info->ILCodeSize); } }

		/// <summary>
        /// Set the native code to be used by this method implementation.
        /// </summary>
		/// <param name="nativeCodeRefArg">A reference to a memory containing the native code of the method.
		/// The memory allocated should be virtual and protected for execution. The Size of the CodeRef is not used by the CodeInjector but
		/// could be used for debugging purposes.</param>
	    void SetOutputNativeCode(CodeRef nativeCodeRefArg) {
			nativeCodeRef = nativeCodeRefArg;
		}

		/// <summary>
        /// Set the native code to be used by this method implementation.
        /// </summary>
		/// <param name="nativeCodeArg">A byte buffer containing the native code of the method.</param>
		void SetOutputNativeCode(cli::array<Byte>^ nativeCodeArg) {
			nativeCode = nativeCodeArg;
		}

        /// <summary>
        /// Alloccate a memory execution block.
        /// </summary>
        /// <param name="size">The size of the memory to allocate.</param>
        /// <returns>A CodeRef referencing the execution memory.</returns>
		CodeRef AllocExecutionBlock(unsigned int size)
		{
			if (isCodeAllocatedFromContext) {
				throw gcnew ArgumentException("Cannot allocate more than once execution block for a MethodContext. For additionnal execution block, use CodeRef.AllocateExecutionBlock instead.");
			}
			CodeRef codeRef;
			codeRef.Pointer = IntPtr(NativeCodeRef::AllocExecutionBlock(context->comp,size));
			isCodeAllocatedFromContext = true;
			codeRef.Size = size;
			return codeRef;
		}

        /// <summary>
        /// Alloccate a memory execution block and fill it with a byte buffer of native code.
        /// </summary>
        /// <param name="nativeCodeBuffer">A buffer containinng native code to copy to the allocated execution block.</param>
        /// <returns>A CodeRef referencing the execution memory.</returns>
		CodeRef AllocExecutionBlock(cli::array<Byte>^ nativeCodeBuffer)
		{
			CodeRef codeRef = AllocExecutionBlock(nativeCodeBuffer->Length);
			System::Runtime::InteropServices::Marshal::Copy(nativeCodeBuffer, 0, codeRef.Pointer, codeRef.Size);
			return codeRef;
		}

	};

	/// <summary>
	/// Internal MethodContext Implem
	/// </summary>
	ref class MethodContextImpl :  public MethodContext
	{
	public:
		// Stack info : TODO provide a better Parameter Stack Info
		cli::array<int>^ sizeOfParameters;

		MethodContextImpl(System::Reflection::MethodInfo^ methodInfoArg, NativeMethodContext* contextArg) :  MethodContext(methodInfoArg, contextArg) {
			sizeOfParameters = nullptr;
		}

		CodeRef GetOutputNativeCode() {
			if ( nativeCode != nullptr ) {
				return AllocExecutionBlock(nativeCode);
			} else if ( nativeCodeRef.Pointer != IntPtr::Zero ) {
				return nativeCodeRef;
			}
			return CodeRef::Zero;
		}

		property int StackSize { void set(int value) { stackSize = value; } }

		int GetClassSize(System::Type^ type) {
			return context->comp->getClassSize( (CORINFO_CLASS_HANDLE) (void*)type->TypeHandle.Value );
		}
	};
}
