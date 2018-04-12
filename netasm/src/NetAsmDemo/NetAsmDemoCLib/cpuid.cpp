// NetAsmDemoCLibClrCall.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#pragma unmanaged
#include "NetAsmDemoCLib.h"

extern "C" bool __fastcall IsCpuidSupported() {
	__asm {
		 pushfd;
		 pop     eax;			//; Get EFLAGS to EAX
		 mov     ecx, eax;		//; Preserve it in ECX
		 
		 xor     eax, 200000h;  //; Check if CPUID bit can toggle
		 push    eax;
		 popfd;//                  ; Restore the modified EAX to EFLAGS

		 pushfd  ;//               ; Get the EFLAGS again
		 pop     ebx;//            ; to EBX
		 xor     eax, ebx;//       ; Has it toggled?
		 and     eax, 200000h;//
		 jnz     __not__;//        ; No? CPUID is not supported
		 
		 mov     eax, 1;//
		 jmp     _ciis_ret_;//     ; Yes? CPUID is supported
		 
__not__:
		 xor  eax, eax;//

_ciis_ret_:
		 push     ecx;//           ; Restore the original EFLAGS
		 popfd;//
//		 ret;//
	};
}

extern "C" bool __fastcall Cpuid(int funcNumber, int* outEax, int* outEdx, int* outEcx, int* outEbx) { 
	//if ( ! IsCpuidSupported() ) {
	//	return 0;
	//}
	__asm {
	 // ; Call CPUID on the given level in EAX
	 mov     eax, funcNumber;
	 cpuid;
	 
	 // ; Transfer the results back to the out parameters
	 mov     edi, outEax;
	 mov     dword ptr [edi], eax;//    ; __eax = eax
	 mov     edi, outEcx;
	 mov     dword ptr [edi], ecx;//    ; __ecx = ecx
	 mov     edi, outEdx;
	 mov     dword ptr [edi], edx;//    ; __edx = edx
	 mov     edi, outEbx;
	 mov     dword ptr [edi], ebx;//    ; __ebx = ebx
	 
	 mov     eax, 1;
	};
}