// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
#include "StdAfx.h"
#include "NativeHook.h"
#include "CodeInjectionAttribute.h"
#include "NoCodeInjectionAttribute.h"
#include "ICodeInjector.h"
#include "CallingConventionHelper.h"

#include "MethodContext.h"

//$(ProjectDir)\publicKey.snk

// -----------------------------------------------------------------------------------
// The code in this file was inspired from the fantastic work of Daniel Pistelli 
// - http://ntcore.com/Files/netint_native.htm 
// -----------------------------------------------------------------------------------

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;
using namespace NetAsm;

// Maximum classname size, should it be extended?
#define MAX_CLASSNAME 512

// Internal struct to map the vtable of the JIT class
struct JITVtable
{
	jitCompileMethodDef compileMethod;
};


struct Fjit_GCInfo
{
    size_t              methodSize;
    unsigned short      methodFrame;      /* includes all save regs and security obj, units are sizeof(void*) */
    unsigned short      methodArgsSize;   /* amount to pop off in epilog */
    unsigned short      methodJitGeneratedLocalsSize; /* number of jit generated locals in method */
    unsigned char       prologSize;
    unsigned char       epilogSize;
    bool                hasThis : 1;
    bool                hasRetBuff : 1;
    bool                isValueClass : 1;
    bool                savedIP : 1;
    bool		EnCMode : 1;	  /* has been compiled in EnC mode */
};

// Variable from NativeHook class
bool NativeHook::isHookInstalled;
bool NativeHook::isGlobalInjectorUsed;
ULONG_PTR *(__stdcall *NativeHook::p_getJit)();
jitCompileMethodDef NativeHook::compileMethod;
// NativeHookInterop* NativeHook::nativeHookInterop;


#pragma unmanaged

// DebugIndirectCall unmanaged function called by delegate DelegateDebugCall when
// Debug is allowed. Use debug with NetAsm is very slow.
// -------------------------------------------------------------------------------------------
int DebugIndirectCall(JITMethodToDebug methodToDebug, int* stackPointer, int stackSize) {
	stackSize = stackSize>>2;
	stackPointer = stackPointer  + stackSize - 1 + 3;

	// TODO add documentation here!!!

	// This part copy the stack from the original CLR call to the real method. This function is just an indirection
	// to go back to unmanaged code.
#ifdef _X86_
	_asm {
		push ebx;
		mov ebx, stackPointer;
	}
	while (stackSize > 0 ) {
		_asm {
			sub esp, 4;
			mov eax, [ebx];
			mov [esp],eax;
			sub ebx, 4
		};
		stackSize--;
	}
	_asm {
		sub ebx, 4;
		mov edx, [ebx];
		sub ebx, 4;
		mov ecx, [ebx];
		call methodToDebug;
		pop ebx;
	}
#endif
	return 0;
}

#pragma managed
// ReflectionPermission is not necessary in JIT CompileMethod
// [assembly:ReflectionPermissionAttribute(SecurityAction::RequestMinimum, ReflectionEmit=true)];
// -----------------------------------------------------------------------------------
// NativeHookInterop class. Provide the CLR Code to bind to the CodeInjection attribute.
// These methods are called from the CompileMethodHook, at runtime, when the JIT is
// compiling a method.
// -----------------------------------------------------------------------------------
NativeHook* NativeHookInterop::nativeHook;
gcroot<System::Text::RegularExpressions::Regex^> NativeHookInterop::regex;
gcroot<NetAsm::ICodeInjector^> NativeHookInterop::globalCodeInjector;
gcroot<NetAsm::DelegateDebugCall^> NativeHookInterop::delegateDebugCall;
bool NativeHookInterop::isDebugAllowed;

NativeHookInterop::NativeHookInterop() {
	nativeHook = new NativeHook();

	// Default is no debug
	isDebugAllowed = false;

	// Create the delegate for debug call
	delegateDebugCall = (DelegateDebugCall^)Marshal::GetDelegateForFunctionPointer(IntPtr(DebugIndirectCall), DelegateDebugCall::typeid );

}

void NativeHookInterop::InitRegex(System::Text::RegularExpressions::Regex^ regExpMatch, NetAsm::ICodeInjector^ globalCodeInjectorArg) {
	regex = regExpMatch;
	globalCodeInjector = globalCodeInjectorArg;
	NativeHook::isGlobalInjectorUsed = true;
}

void NativeHookInterop::RemoveRegex() {
	regex = nullptr;
	globalCodeInjector = nullptr;
	NativeHook::isGlobalInjectorUsed = false;
}

bool NativeHookInterop::IsMatch(wchar_t* stringToMatch) {
	if ( ((System::Text::RegularExpressions::Regex^)regex) != nullptr ) {
		return regex->IsMatch(gcnew String(stringToMatch));
	}
	return false;
}

// InstantiateJitCodeInjector. Given a Type, try to instantiate a subclass of ICodeInjector
// Throws an ArgumentException if the Type is not a subclass of ICodeInjector
// -------------------------------------------------------------------------------------------
NetAsm::ICodeInjector^ NativeHookInterop::InstantiateJitCodeInjector(System::Type^ jitCodeIntjectorClass) {
		NetAsm::ICodeInjector^ jitCodeInjector = nullptr;
		if ( jitCodeIntjectorClass != nullptr ) {
			if ( ! NetAsm::ICodeInjector::typeid->IsAssignableFrom( jitCodeIntjectorClass) ) {
				throw gcnew System::ArgumentException(System::String::Format("The class [{0}] is not implementing interface [NetAsm.ICodeInjector]", jitCodeIntjectorClass->Name));
			}
			jitCodeInjector = (NetAsm::ICodeInjector^)System::Activator::CreateInstance(jitCodeIntjectorClass);
		}
		return jitCodeInjector;
	}

// 58               pop         eax  
// 52               push        edx  
// 51               push        ecx  
// 50               push        eax  
// E9 D7 FF FF FF   jmp         functionCalled (12B1000h) 
byte stdCallBuffer_2[9] = {0x58, 0x52, 0x51, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00};
byte stdCallBuffer_1[8] = {0x58, 0x51, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00};
byte stdCallBuffer_0[5] = {0xE9, 0x00, 0x00, 0x00, 0x00};

/*
[StructLayout(LayoutKind::Sequential)]
public value struct MyStruct
{
	unsigned char x;	
//	int y;
};
*/

// GetCodeInjection. Return the ASM code from CodeInjection attribute.
// If null is return or nbOutputBytes is 0, then no ASM code is available.
// -------------------------------------------------------------------------------------------
byte* NativeHookInterop::GetCodeInjection(CONST WCHAR* classTypeName, int* nbOutputBytes, NativeMethodContext* context) {
		*nbOutputBytes = 0;
		// Retreive the Calling Assembly : Warning, the CallingAssembly is the Assembly of the 
		// method currently being jitted.
		System::Reflection::Assembly^ assem = System::Reflection::Assembly::GetCallingAssembly();

		String^ classTypeNameStr = gcnew String(classTypeName);

	
		// Retrieve the Metatoken of the Method
		mdMethodDef methodToken = context->comp->getMethodDefFromMethod(context->info->ftn);

		// Resolve the method token for the class
		System::Reflection::MethodBase^ methodBase = assem->GetType(classTypeNameStr)->Module->ResolveMethod(methodToken);
		if ( ! System::Reflection::MethodInfo::typeid->IsAssignableFrom( methodBase->GetType()) ) {
			return 0;
		}


		// Retreive MethodInfo of the method being jitted
		System::Reflection::MethodInfo^ methodInfo = (System::Reflection::MethodInfo^)methodBase;
		String^ methodNameStr = methodInfo->Name;
			
#if defined(DEBUG)
		System::Diagnostics::Debug::Assert( (methodInfo == nullptr ), String.Format("Unable to retrieve MethodInfo for Method {0}", methodNameStr) );
#endif
		if ( methodInfo == nullptr ) {
			return 0;
		}

		// If NoCodeInjection is found, than force the method to use default JIT Compiler
		cli::array<System::Object^>^ attributeOnMethod = methodInfo->GetCustomAttributes(NetAsm::NoCodeInjectionAttribute::typeid,false);
		if ( attributeOnMethod->Length == 1 ) {
			return 0;
		}

		// Instantiate a MethodContext
		MethodContextImpl^ methodContext = gcnew MethodContextImpl(methodInfo, context);

		// Get CodeInjectionAttribute
		attributeOnMethod = methodInfo->GetCustomAttributes(NetAsm::CodeInjectionAttribute::typeid,false);
		NetAsm::CodeInjectionAttribute^ codeInjectionAttribute = nullptr;
		CallingConvention callingConvention;

		CodeRef nativeCodeRef = CodeRef::Zero;

		// Output pointer to executable native code
		void* codeBytesPtr = 0;

		// Try to get CodeInjection Attribute from the method
		if ( attributeOnMethod->Length == 1 ) {
			codeInjectionAttribute = (NetAsm::CodeInjectionAttribute^)attributeOnMethod[0];
			callingConvention = codeInjectionAttribute->CallingConvention;
			// First, use static code
			cli::array<Byte>^ asmCodeArray = codeInjectionAttribute->Code;
			// If not null, use static code
			if ( asmCodeArray != nullptr ) {
				nativeCodeRef = methodContext->AllocExecutionBlock(asmCodeArray);
			} else {
				// If null, use dynamic code
				NetAsm::ICodeInjector^ jitCodeInjector = InstantiateJitCodeInjector(codeInjectionAttribute->JITCodeInjectorClass);
				if ( jitCodeInjector != nullptr ) {
					// Fill StackInfo for context
					CallingConventionHelper::FillStackInfoForContext(methodContext);
					// Use dynamic JIT Injetor
					jitCodeInjector->CompileMethod(methodContext);
					nativeCodeRef = methodContext->GetOutputNativeCode();
				} else if ( codeInjectionAttribute->DllName != nullptr) {
					// Use Dll injection
					System::String^ dllMethodName = codeInjectionAttribute->DllMethodName;
					if ( dllMethodName == nullptr ) {
						dllMethodName = methodNameStr;
					}
					nativeCodeRef = CodeRef::BindToDllFunction(codeInjectionAttribute->DllName, dllMethodName);
				}
			}
		}

		// If nothing was generated for this method, try to look CodeInjection at the class level.
		if ( nativeCodeRef.IsNull() ) {
			cli::array<System::Object^>^ attributeOnClass = methodInfo->DeclaringType->GetCustomAttributes(NetAsm::CodeInjectionAttribute::typeid,true);
			// Use only dynamic mode  and dll mode at the class level 
			if ( attributeOnClass->Length == 1 ) {
				codeInjectionAttribute = (NetAsm::CodeInjectionAttribute^)attributeOnClass[0];
				callingConvention = codeInjectionAttribute->CallingConvention;
				NetAsm::ICodeInjector^ jitCodeInjector = InstantiateJitCodeInjector(codeInjectionAttribute->JITCodeInjectorClass);
				if ( jitCodeInjector != nullptr ) {
					// Fill StackInfo for context
					CallingConventionHelper::FillStackInfoForContext(methodContext);
					// Use dynamic JIT Injector
					jitCodeInjector->CompileMethod(methodContext);
					nativeCodeRef = methodContext->GetOutputNativeCode();
				} else if ( codeInjectionAttribute->DllName != nullptr) {
					// Use linkable dll
					System::String^ dllMethodName = codeInjectionAttribute->DllMethodName;
					if ( dllMethodName == nullptr ) {
						dllMethodName = methodNameStr;
					}
					nativeCodeRef = CodeRef::BindToDllFunction(codeInjectionAttribute->DllName, dllMethodName);
				}
			}
		}

		// Last, check if a Global Code Injector is set
		if ( nativeCodeRef.IsNull() ) {
			if ( ((NetAsm::ICodeInjector^)globalCodeInjector) != nullptr ) {
				// Fill StackInfo for context
				CallingConventionHelper::FillStackInfoForContext(methodContext);
				// Use dynamic JIT Injector
				globalCodeInjector->CompileMethod(methodContext);
				nativeCodeRef = methodContext->GetOutputNativeCode();
			}
		}

		// If the native code is non null, set it back to the jit.
		if ( ! nativeCodeRef.IsNull() ) {
			// Debug must be allowed otherwise it could be very slow
			bool isDebug = context->isInDebug && isDebugAllowed;
			// Handle Calling Conventions
			nativeCodeRef = CallingConventionHelper::ConvertClrCallTo(methodContext, nativeCodeRef, callingConvention);

			// Don't try to debug a native code wich was not allocated from the context. It won't be debuggable
			// It means that any DLL Code Injection is not debuggable
			if ( isDebug && methodContext->IsCodeAllocatedFromContext) {

				// Allocate GCInfo for debug
				Fjit_GCInfo* gcInfo = (Fjit_GCInfo*)context->comp->allocGCInfo(sizeof(Fjit_GCInfo));
				gcInfo->methodSize = nativeCodeRef.Size;
				gcInfo->methodFrame = 8 * methodInfo->GetParameters()->Length;
				gcInfo->methodArgsSize = methodContext->StackSize;
				gcInfo->methodJitGeneratedLocalsSize = sizeof(void*);
				gcInfo->prologSize = 0;
				gcInfo->epilogSize = 0;
				gcInfo->hasThis = !methodInfo->IsStatic;
				gcInfo->hasRetBuff = false;
				gcInfo->isValueClass = methodInfo->DeclaringType->IsValueType;
				gcInfo->savedIP = false;
				gcInfo->EnCMode = context->flags & CORJIT_FLG_DEBUG_EnC;


				// Build a fake mapping from IL Code to Native Code
				int NB_MAPPING = 3;
				ICorDebugInfo::OffsetMapping *mapping = (ICorDebugInfo::OffsetMapping*) context->comp->allocateArray(NB_MAPPING * sizeof(ICorDebugInfo::OffsetMapping));
				mapping[0].ilOffset = (unsigned)ICorDebugInfo::PROLOG;
				mapping[0].nativeOffset = 0;
				mapping[0].source = ICorDebugInfo::SOURCE_TYPE_INVALID;

				mapping[1].ilOffset = 0;
				mapping[1].nativeOffset = 0;
				
				mapping[2].ilOffset = (unsigned)ICorDebugInfo::EPILOG;
		        mapping[2].source = ICorDebugInfo::SOURCE_TYPE_INVALID;

				// Set the mapping for the debugger
     			context->comp->setBoundaries(context->info->ftn, NB_MAPPING, mapping);
				
/*
			OLD DEBUG MODE USING INTEROP. 
			Now, we use the JIT debug facilities, even if it is not possible to debug external dll.

			// If debug, we need to go back to interop in order to leave the ClrRuntime and to enable breakpoints
			// on native code. It should have been mush better if we could have called the HostTaskManager.EnterRuntime / LeaveRuntime
			// methods, but there are no ways to get this HostTaskManager (or to get the HostControl).
			// Using debug is very slow and the behaviour of the native code executed is different.
			// Need more documentation here to explain how we did the hack on the debug (Use of an unamanaged delegate to make
			// an indirect call to the native code).

   			DelegateDebugCall^ myDelegate = delegateDebugCall;
				void* myDelegatePtr = &myDelegate;

				cli::array<byte>^ callDebug = gcnew cli::array<byte> {
					0x52,						//		push edx;			   // push Current EDX
					0x51,						//		push ecx;			   // push Current ECX
					0x54,						//		push esp;			   // push Stack pointer
					0x6A,0x00,					//		push 00;			   // push stacksize
					0xB9,0x00,0x00,0x00,0x00,	//		mov ecx, 0x00000000;   // Address of Delegate
					0xBA,0x00,0x00,0x00,0x00,	//		mov edx, 0x00000000;   // Address of methodToDebug
					0x8B, 0x41, 0x0C,			//		mov eax, [ecx + 12];   // Get address of Invoke method Delegate
					0x8B, 0x49, 0x04,			//		mov ecx, [ecx + 4];    // Get VTable Address
					0xFF, 0xD0,					//		call eax;			   // Call the Delegate
					0x59,						//		pop ecx;			   // pop ECX
					0x5A,						//		pop edx;			   // pop EDX;
					0xC2,0x00, 0x00				//		ret 0x0000;			   // Cleanup stack (stack size)
				};

				CodeRef debugCodeRef = CodeRef::AllocExecutionBlock(callDebug);
				IntPtr codeRefPtr = debugCodeRef.Pointer;
				// Write stacksizes
				Marshal::WriteByte(codeRefPtr, 4, methodContext->StackSize);
				Marshal::WriteInt16(codeRefPtr, callDebug->Length - 2, (short)methodContext->StackSize);
				// Write address of the MulticastDelegate instance
				Marshal::WriteIntPtr(codeRefPtr, 6, IntPtr(*((int*)myDelegatePtr)));
				// Write address of the native code to call
				Marshal::WriteIntPtr(codeRefPtr, 11, nativeCodeRef.Pointer);

				// Set the debugCodeRef as the native code of the methods
				nativeCodeRef = debugCodeRef;
*/
			}

			/*
			  Tried DebugJustMyCode, but not sure what is it for...
			if ( context->debugJustMyCode != 0 ) {
				// E8 00 00 00 00   call justmycode
				// E9 00 00 00 00   jmp  functionCalled (0000000h) 
				cli::array<byte>^ stdCallBuffer = gcnew cli::array<byte> {0xE8, 0x00, 0x00, 0x00, 0x00, 0xE9, 0x00, 0x00, 0x00, 0x00};

				CodeRef stdCallCodeRef = CodeRef::AllocExecutionBlock(stdCallBuffer);
				IntPtr jumpPointer = IntPtr((byte*)((void*)nativeCodeRef.Pointer) - (((byte*)((void*)stdCallCodeRef.Pointer)) + stdCallBuffer->Length ));
				System::Runtime::InteropServices::Marshal::WriteIntPtr(stdCallCodeRef.Pointer, stdCallBuffer->Length - IntPtr::Size, jumpPointer);


				byte* stdCallCodeRefPtr = (byte*)((void*)stdCallCodeRef.Pointer);
				jumpPointer = IntPtr((byte*)((byte*)context->debugJustMyCode - (stdCallCodeRefPtr + 5)));
				System::Runtime::InteropServices::Marshal::WriteIntPtr(stdCallCodeRef.Pointer, 1, jumpPointer);

				nativeCodeRef = stdCallCodeRef;
			}*/

			*nbOutputBytes = nativeCodeRef.Size;
			codeBytesPtr = (void*)nativeCodeRef.Pointer;
		}

		methodContext = nullptr;
		return (byte*)codeBytesPtr;
	}

// NativeHook constructor
// ----------------------------------------------------------------
NativeHook::NativeHook(void)
{
	isHookInstalled = FALSE;
	isGlobalInjectorUsed = false;
}

// NativeHook destructor
// ----------------------------------------------------------------
NativeHook::~NativeHook(void)
{
}

// -----------------------------------------------------------------------------------
// Hook on CompileMethod on Jit class from mscorjit.dll.
//
// Work inspired from Daniel Pistelli - http://ntcore.com/Files/netint_native.htm 
// -----------------------------------------------------------------------------------
#pragma unmanaged
int __stdcall compileMethodHook(ULONG_PTR classthis, ICorJitInfo *comp, CORINFO_METHOD_INFO *info,
								unsigned flags, BYTE **nativeEntry, ULONG  *nativeSizeOfCode)
{
	CONST BYTE* pointerToAttribute = 0;
	WCHAR fullClassName[MAX_CLASSNAME];
	WCHAR* pBufferFullClassName = fullClassName;
	int length = MAX_CLASSNAME;
	const char *methodName = NULL;
	const char *className = NULL;
	fullClassName[0] = 0;

	// Get Method class Handle
	CORINFO_CLASS_HANDLE classHandle = comp->getMethodClass(info->ftn);
	// Get namespace+classname of the class of the method being jitted.s
	int ret = comp->appendClassName(&pBufferFullClassName,&length, classHandle,1,0,0);
	
	// Check if class is allowing CodeInjection
	if ( comp->getClassCustomAttribute(classHandle,"NetAsm.AllowCodeInjectionAttribute", &pointerToAttribute) >= 0
			// If we are in NetAsm assembly, don't try to compile methods with a custom injector
			|| ( NativeHook::isGlobalInjectorUsed &&
		     wcsncmp(fullClassName, _T("NetAsm."), 7 ) != 0 &&
			// WARNING, the method NativeHookInterop::IsMatch is compiled in JITHook::Install to avoid any recursive call to compileMethodHook.
			NativeHookInterop::IsMatch(fullClassName)) 
		) {

		// Get methodName
		methodName = comp->getMethodName(info->ftn, &className);

		// Don't do anything on constructors
		if ( strncmp(methodName,".c", 2) != 0) {
			int nbBytes = 0;
			// Call NativeHookInterop to get the Code Injection. Use mixed clr to get attributes from the method.
			// ICorJitInfo doesn't provide any way to directly get attributes from method or properties but only
			// for class : the only way is to go back to the CLR.
			NativeMethodContext context;
			context.classthis = classthis;
			context.comp = comp;
			context.flags = flags;
			context.info = info;
			context.compileMethod = NativeHook::compileMethod;
			if (flags & CORJIT_FLG_DEBUG_CODE) {
				//CORINFO_JUST_MY_CODE_HANDLE *pDbgHandle;
				//CORINFO_JUST_MY_CODE_HANDLE dbgHandle = comp->getJustMyCodeHandle(info->ftn, &pDbgHandle);
				//context.debugJustMyCode = comp->getHelperFtn(CORINFO_HELP_DBG_IS_JUST_MY_CODE);
				context.isInDebug = true;
			} else {
				context.isInDebug = false;
				//context.debugJustMyCode = 0;
			}

			// Get the CodeInjection
			byte* codeByte = NativeHookInterop::GetCodeInjection(fullClassName, &nbBytes, &context);

			// Only plug the assembler code if it is non null and length > 0
			// otherwise, use initial JIT compileMethod (see later).
			if ( codeByte!= 0 && nbBytes > 0 ) {
				*nativeEntry = codeByte;
				*nativeSizeOfCode = nbBytes;
				return nbBytes;
			}
		}
	}
	// Call the initial compileMethod from the JIT.
	return NativeHook::compileMethod(classthis, comp, info, flags, nativeEntry, nativeSizeOfCode);
}

// -----------------------------------------------------------------------------------
// Install the hook.
//
// Work inspired from Daniel Pistelli - http://ntcore.com/Files/netint_native.htm 
// -----------------------------------------------------------------------------------
#pragma unmanaged
bool NativeHook::Install()
{
	if (isHookInstalled) return true;

	// WARNING !!!!
	// Force this method to be compiled by the JIT BEFORE we install the hook because this method is used by the hook itself.
	// If this method was not called from here, the compiler hook would loop recursively.
	NativeHookInterop::IsMatch(_T("XXXXXXXX"));

	LoadLibrary(_T("mscoree.dll"));

	HMODULE hJitMod = LoadLibrary(_T("mscorjit.dll"));

	if (!hJitMod)
		return false;

	// Get "getJit" function entry point from from mscorjit.dll
	p_getJit = (ULONG_PTR *(__stdcall *)()) GetProcAddress(hJitMod, "getJit");

	if (p_getJit)
	{
		// Get the JIT and bind to the virtual table
		JITVtable *pJit = (JITVtable *) *((ULONG_PTR *) p_getJit());

		// If successfull, hook the JIT compileMethod with our compileMethodHook (see corjit.h)
		if (pJit)
		{
			DWORD OldProtect;
			VirtualProtect(pJit, sizeof (ULONG_PTR), PAGE_READWRITE, &OldProtect);
			compileMethod =  pJit->compileMethod;
			pJit->compileMethod = &compileMethodHook;
			VirtualProtect(pJit, sizeof (ULONG_PTR), OldProtect, &OldProtect);
			isHookInstalled = TRUE;
		}
	}
	return isHookInstalled;
}

// -----------------------------------------------------------------------------------
// Remove the hook.
// -----------------------------------------------------------------------------------
#pragma unmanaged
void NativeHook::Remove()
{
	if (!isHookInstalled) return;

	if (p_getJit)
	{
		JITVtable *pJit = (JITVtable *) *((ULONG_PTR *) p_getJit());

		if (pJit)
		{
			DWORD OldProtect;
			VirtualProtect(pJit, sizeof (ULONG_PTR), PAGE_READWRITE, &OldProtect);
			pJit->compileMethod = compileMethod;
			VirtualProtect(pJit, sizeof (ULONG_PTR), OldProtect, &OldProtect);
			isHookInstalled = false;
		}
	}
}