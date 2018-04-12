// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once
#include "MethodContext.h"

namespace NetAsm {

	value struct CodeRef;

    /// <summary>
    /// This interface is used when the dynamic code injection is used on a method. To implement a dynamic code injection,
	/// you need to provide a subclass of this interface and implement the CompileMethod.
	/// Call <see cref="NetAsm::MethodContext::SetOutputNativeCode"/> or to associate the native code to the method being compiled. If nothing
	/// is done in the CompileMethod, the default JIT compiler is used.
	/// It is possible, through the <see cref="NetAsm::MethodContext"/>, to look at the associated method's IL Code to make your own compiler.
    /// </summary>
	public interface class ICodeInjector
	{
        /// <summary>
        /// Called by the JIT to compile a method. This method should call SetOutputNativeCode on the context to return the
		/// native code to use for this method. The native code should follow the fastcall convention.
		/// If this method doesn't call SetOuputNativeCode, the default JIT compiler is used.
        /// </summary>
		/// <param name="context">Context of the method to compile : it contains the MethodInfo reflection param,
		/// pointer to the IL Code and a helper function to call the native compiler for this method</param>
		void CompileMethod(NetAsm::MethodContext^ context);
	};

}
