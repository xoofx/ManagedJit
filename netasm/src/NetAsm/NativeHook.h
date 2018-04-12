// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#pragma once


#include <windows.h>
#include <tchar.h>
#include <vcclr.h>
#pragma unmanaged
#include "corHdr.h"
#include "corinfo.h"
#include "corjit.h"

typedef int (__stdcall *jitCompileMethodDef)(ULONG_PTR classthis, ICorJitInfo *comp,
											 CORINFO_METHOD_INFO *info, unsigned flags,        
											 BYTE **nativeEntry, ULONG  *nativeSizeOfCode);




typedef struct NativeMethodContext_ {
	ULONG_PTR classthis;
	ICorJitInfo *comp;
	CORINFO_METHOD_INFO *info;
	unsigned flags;
	BYTE *nativeEntry;
	ULONG  nativeSizeOfCode;
	bool isInDebug;
	jitCompileMethodDef compileMethod;

	void* GetFunctionEntry(int methodToken) {
		CORINFO_METHOD_HANDLE targetMethod = comp->findMethod(info->scope, methodToken, info->ftn);
		CORINFO_CONST_LOOKUP addrInfo;
		comp->getFunctionEntryPoint(targetMethod, IAT_VALUE, &addrInfo);
		return addrInfo.addr;
	}

	void Compile() {
		compileMethod(classthis, comp, info, flags, &nativeEntry, &nativeSizeOfCode);
	}
} NativeMethodContext;

// #include "ICodeInjector.h"

#pragma unmanaged
class NativeHook
{
public:
	NativeHook(void);
	virtual ~NativeHook(void);

	static bool Install();
	static void Remove();
	static jitCompileMethodDef compileMethod;
	static bool isGlobalInjectorUsed;
private:
	static bool isHookInstalled;
	static ULONG_PTR *(__stdcall *p_getJit)();
};


#pragma managed

typedef int (__stdcall *JITMethodToDebug)();

using namespace System;

namespace NetAsm {

	interface class ICodeInjector;

	[System::Runtime::InteropServices::UnmanagedFunctionPointer(System::Runtime::InteropServices::CallingConvention::StdCall)]
	delegate int DelegateDebugCall(IntPtr methodToDebug, IntPtr stackPointer, int stackSize);
};

class NativeHookInterop {
public:
	NativeHookInterop();
	static NativeHook* nativeHook;
	static gcroot<System::Text::RegularExpressions::Regex^> regex;
	static gcroot<NetAsm::ICodeInjector^> globalCodeInjector;
	static gcroot<NetAsm::DelegateDebugCall^> delegateDebugCall;
	static bool isDebugAllowed;

	static void InitRegex(System::Text::RegularExpressions::Regex^ regExpMatch, NetAsm::ICodeInjector^ globalCodeInjectorArg);
	static void RemoveRegex();
	static bool IsMatch(wchar_t* stringToMatch);
	static NetAsm::ICodeInjector^ InstantiateJitCodeInjector(System::Type^ jitCodeIntjectorClass);
	static byte* GetCodeInjection(CONST WCHAR* classTypeName, int* nbOutputBytes, NativeMethodContext* context);
};
