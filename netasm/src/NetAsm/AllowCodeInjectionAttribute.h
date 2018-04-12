// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

using namespace System;

namespace NetAsm {

	ref class CodeInjectionAttribute;

	/// <summary>
	/// Set this attribute on class that should be analysed from code injection. 
	/// Use <see cref="CodeInjectionAttribute"/> to specify the code injection mode at the class level
	/// or the method level.
	/// </summary>
	[AttributeUsageAttribute(AttributeTargets::Class, Inherited = false, AllowMultiple = false)]
	public ref class AllowCodeInjectionAttribute :
		public System::Attribute
		{
		public:
			AllowCodeInjectionAttribute();
		};
}
