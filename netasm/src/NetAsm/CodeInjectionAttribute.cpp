// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#include "StdAfx.h"
#include "CodeInjectionAttribute.h"

using namespace System;

namespace NetAsm {

CodeInjectionAttribute::CodeInjectionAttribute(cli::array<Byte>^ codeArg) {
	code = codeArg;
	dllName = nullptr;
}

CodeInjectionAttribute::CodeInjectionAttribute(Type^ jitCodeInjectorClassArg) {
	code = nullptr;
	jitCodeInjectorClass = jitCodeInjectorClassArg;
	dllName = nullptr;
}

CodeInjectionAttribute::CodeInjectionAttribute(System::String^ dllNameArg, System::String^ functionNameExportedInDllWithFascallConventionArg) {
	code = nullptr;
	jitCodeInjectorClass = nullptr;
	dllName = dllNameArg;
	dllMethodName = functionNameExportedInDllWithFascallConventionArg;
}

CodeInjectionAttribute::CodeInjectionAttribute(System::String^ dllNameArg) {
	code = nullptr;
	jitCodeInjectorClass = nullptr;
	dllName = dllNameArg;
	dllMethodName = nullptr;
}

}
