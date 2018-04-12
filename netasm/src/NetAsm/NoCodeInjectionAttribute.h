// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once

using namespace System;

namespace NetAsm {

	/// <summary>
	/// Set this attribute on a method that should not use any code injection defined at the class or application level. 
	/// </summary>
	[AttributeUsageAttribute(AttributeTargets::Method, Inherited = false, AllowMultiple = false)]
	public ref class NoCodeInjectionAttribute :
		public System::Attribute
		{
		public:
			NoCodeInjectionAttribute() {}
		};
}