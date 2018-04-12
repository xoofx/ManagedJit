// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#include "stdafx.h"
#include "CodeRef.h"

#include <windows.h>
using namespace System::Runtime::InteropServices;

void* NativeCodeRef::AllocExecutionBlock(int size)
{
	return AllocExecutionBlock(size, GetCurrentProcess());
}

void* NativeCodeRef::AllocExecutionBlock(ICorJitInfo* comp, int size)
{
	void* hotCodeBlock;
	void* coldCodeBlock;
	void* roDataBlock;
	void* rwDataBlock;

	comp->allocMem(size, 0, 0, 0, 0, (CorJitAllocMemFlag)0,
		&hotCodeBlock, &coldCodeBlock, &roDataBlock, &rwDataBlock);
	return hotCodeBlock;
}


void* NativeCodeRef::AllocExecutionBlock(int size, HANDLE hProcess)
{
	void* codeBytesPtr = VirtualAllocEx(
		hProcess, 0, size,
		MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (codeBytesPtr == 0)
	{
		return 0;
	}

	DWORD lpflOldProtect;
	if (!VirtualProtectEx(
		hProcess, codeBytesPtr,
		size,
		PAGE_EXECUTE_READWRITE, &lpflOldProtect))
	{
		return 0;
	}

	return codeBytesPtr;
}

#pragma unmanaged
void* NativeLoadDllFunction(wchar_t* dllName, char* dllFunctionName) {
	HMODULE dllModule = LoadLibrary(dllName);
	return GetProcAddress(dllModule,dllFunctionName); 
}

#pragma managed
IntPtr NativeCodeRef::BindToDllFunction(String^ dllName, String^ dllMethodName) {
	void* dllNamePtr = (void*)Marshal::StringToHGlobalUni(dllName);
	void* dllMethodNamePtr = (void*)Marshal::StringToHGlobalAnsi(dllMethodName);
	void* functionPointer = NativeLoadDllFunction( (wchar_t*)((void*)dllNamePtr),(char *)((void*)dllMethodNamePtr)); 
	Marshal::FreeHGlobal(IntPtr(dllNamePtr));
	Marshal::FreeHGlobal(IntPtr(dllMethodNamePtr));
	if ( functionPointer == 0 ) {
		throw gcnew System::ArgumentException(System::String::Format("Unable to load Dll [{0}] or find method [{1}] in this dll",dllName, dllMethodName));
	}
	// Set size of dll to 1 (not used by the jit compiler)
	return IntPtr(functionPointer);
}
