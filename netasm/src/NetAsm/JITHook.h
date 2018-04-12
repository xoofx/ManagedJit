// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

#include "NativeHook.h"
#include "ICodeInjector.h"

#pragma managed
using namespace System;

namespace NetAsm {

    /// <summary>
	/// Main hook class. Just add a call to <c>JITHook.Install()</c> at the beginning of your application.
    /// </summary>
	public ref class JITHook
	{
	private:
		static NativeHookInterop* nativeHookInterop = new NativeHookInterop();
		JITHook() {}
	public:
        /// <summary>
        /// Installs the JIT Code Injector Hook.
        /// </summary>
        /// <returns>true if installed successfully</returns>
		static bool Install() {			
			return nativeHookInterop->nativeHook->Install();
		}

        /// <summary>
        /// Installs the JIT Code Injector Hook with a global injector and a regex pattern matching on classnames.
		/// Use global injector with caution. The regexMatchClasses shouldn't match any class that are use directly or indirectly
		/// in the code executed by the global injector - or the global injector itself, as the JIT compiler will end up
		/// to an infinite recursive loop.
        /// </summary>
		/// <param name="regexMatchOnClassNames">A regular expression matching full class names (with namespaces).</param>
		/// <param name="globalInjector">An instance of a global injector</param>
		/// <returns>true if installed successfully</returns>
		static bool Install(String^ regexMatchOnClassNames, NetAsm::ICodeInjector^ globalInjector) {
			return Install(gcnew System::Text::RegularExpressions::Regex(regexMatchOnClassNames), globalInjector);
		}

        /// <summary>
        /// Installs the JIT Code Injector Hook with a global injector and a regex pattern matching on classnames.
		/// Use global injector with caution. The regexMatchClasses shouldn't match any class that are use directly or indirectly
		/// in the code executed by the global injector - or the global injector itself, as the JIT compiler will end up
		/// to an infinite recursive loop.
        /// </summary>
		/// <param name="regexMatchOnClassNames">A regular expression matching full class names (with namespaces).</param>
		/// <param name="globalInjector">An instance of a global injector</param>
		/// <returns>true if installed successfully</returns>
		static bool Install(System::Text::RegularExpressions::Regex^ regexMatchOnClassNames, NetAsm::ICodeInjector^ globalInjector) {
			if ( regexMatchOnClassNames == nullptr || globalInjector == nullptr ) {
				throw gcnew ArgumentException("Install(..) arguments cannot be null");
			}		
			nativeHookInterop->InitRegex(regexMatchOnClassNames, globalInjector);
			return nativeHookInterop->nativeHook->Install();
		}

        /// <summary>
        /// Uninstall the hook.
        /// </summary>
		static void Remove() {
			nativeHookInterop->nativeHook->Remove();
			NativeHookInterop::RemoveRegex();
		}
	};
}
