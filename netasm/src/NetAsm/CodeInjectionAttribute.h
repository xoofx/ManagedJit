// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

#pragma managed
using namespace System;

namespace NetAsm {


	interface class ICodeInjector;

	/// <summary>
	/// Code Injection Attribute. Use this attribute at the class or method level to specify how to inject the native code. 
	/// NetAsm provides 3 modes to inject native code :
	/// <list type="bullet">
	/// <item><description>Static mode, specify the native code at compile time. The attribute contains a byte buffer of native code (assembler code)</description></item>
	/// <item><description>Dll link mode, specify the native code at runtime. Link the method at runtime to a function in an external dll</description></item>
	/// <item><description>Dynamic mode, with a subclass of the ICodeInjector interface executed at runtime</description></item>
	/// </list>
	/// Be aware that the calling convetion used is FASTCALL. 
	/// NetAsm first look at the CodeInjectionAttribute at the method level, and then at the class level.
	/// At class level, only the Dll link and dynamic mode can be used.
	/// </summary>
	[AttributeUsageAttribute(AttributeTargets::Method|AttributeTargets::Property|AttributeTargets::Class, Inherited = false, AllowMultiple = false)]
	public ref class CodeInjectionAttribute : public System::Attribute 
	{
		public:
			System::Runtime::InteropServices::CallingConvention CallingConvention;

			/// <summary>
			/// Initializes a new instance of the <see cref="CodeInjectionAttribute"/> class with a native static code.
			/// </summary>
			/// <param name="nativeCode">The native code (assembler) in a byte buffer</param>
			CodeInjectionAttribute(cli::array<Byte>^ nativeCode);

			/// <summary>
			/// Initializes a new instance of the <see cref="CodeInjectionAttribute"/> class with a dynamic JIT Code Injector. The class must
			/// inherit from <see cref="NetAsm::ICodeInjector"/>.
			/// </summary>
			/// <param name="jitCodeInjectorClass">The JIT code injector class.</param>
			CodeInjectionAttribute(Type^ jitCodeInjectorClass);

			/// <summary>
			/// Initializes a new instance of the <see cref="CodeInjectionAttribute"/> class with a DLL name. The name of the method
			/// currently jitted will be used to search for exported function in the dll.
			/// </summary>
			/// <param name="dllName">Name or path to the dll to bind to.</param>
			CodeInjectionAttribute(System::String^ dllName);

			/// <summary>
			/// Initializes a new instance of the <see cref="CodeInjectionAttribute"/> class with a DLL and the 
			/// exported function to bind to.
			/// </summary>
			/// <param name="dllName">Name or path to the dll to bind to.</param>
			/// <param name="functionNameExportedInDllWithFascallConventionArg">Name of the function to bind to</param>
			CodeInjectionAttribute(System::String^ dllName, System::String^ functionNameExportedInDllWithFascallConventionArg);

			property cli::array<Byte>^ Code { 
				cli::array<Byte>^  get() { return code; }
			}

			property Type^ JITCodeInjectorClass { 
				Type^ get() { return jitCodeInjectorClass; }
			}

			property System::String^ DllName {
				System::String^ get() { return dllName; }
			}

			property System::String^ DllMethodName {
				System::String^ get() { return dllMethodName; }
			}
	private:
			cli::array<Byte>^ code;
			Type^ jitCodeInjectorClass;
			System::String^ dllName;
			System::String^ dllMethodName;
	};

}