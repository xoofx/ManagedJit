// (c) Copyright Microsoft Corporation.
// This source is subject to the Microsoft Permissive License.
// See http://www.microsoft.com/resources/sharedsource/licensingbasics/sharedsourcelicenses.mspx.
// All other rights reserved.
// NetAsmDemoMixedLib.h
#pragma once
#include <intrin.h>
using namespace System;

#pragma unmanaged
#include "..\NetAsmDemoCLib\NetAsmDemoCLibStdCall.cpp"

#pragma managed
namespace NetAsmDemoMixedLib {

	public ref class MixedClass
	{
	public:
		static int  SimpleAddMixed(int x, int y) {
			return SimpleAddInterop_static_stdcall(x,y);
		}

		static void  matrix_mul(float const* m1, float const* m2, float* out) {
			matrix_mul_sse2_stdcall(m1, m2, out);
		}
	};
}
