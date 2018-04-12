// NetAsmDemoSampleDll.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#pragma unmanaged
#include "NetAsmDemoCLib.h"
#include <intrin.h>
#include <stdio.h>

extern "C" int __stdcall SimpleAddInterop_stdcall(void* thisPtr, int x, int y) {
	return x + y;
}

extern "C" int __stdcall SimpleAddInterop_static_stdcall(int x, int y) {
	return x + y;
}

extern "C" void __declspec(dllexport) __stdcall matrix_mul_std_stdcall(float const* m1, float const* m2, float* o)
{
    o[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
    o[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
    o[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
    o[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];

    o[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
    o[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
    o[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
    o[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];

    o[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
    o[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
    o[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
    o[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];

    o[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
    o[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
    o[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
    o[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
}

extern "C" void __declspec(dllexport) __stdcall matrix_mul_sse2_stdcall(float const* m1, float const* m2, float* out)
{
	__m128 r;

	__m128 col1 = _mm_loadu_ps(m2);
	__m128 col2 = _mm_loadu_ps(m2 + 4);
	__m128 col3 = _mm_loadu_ps(m2 + 8);
	__m128 col4 = _mm_loadu_ps(m2 + 12);

	__m128 row1 = _mm_loadu_ps(m1);
	
	r = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row1, row1, _MM_SHUFFLE(0, 0, 0, 0)), col1),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row1, row1, _MM_SHUFFLE(1, 1, 1, 1)), col2),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row1, row1, _MM_SHUFFLE(2, 2, 2, 2)), col3),
		_mm_mul_ps(_mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 3, 3, 3)), col4))));

	_mm_storeu_ps(out, r);  
	__m128 row2 = _mm_loadu_ps(m1 + 4);

	r = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row2, row2, _MM_SHUFFLE(0, 0, 0, 0)), col1),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row2, row2, _MM_SHUFFLE(1, 1, 1, 1)), col2),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row2, row2, _MM_SHUFFLE(2, 2, 2, 2)), col3),
		_mm_mul_ps(_mm_shuffle_ps(row2, row2, _MM_SHUFFLE(3, 3, 3, 3)), col4))));

	_mm_storeu_ps(out + 4, r);  
	__m128 row3 = _mm_loadu_ps(m1 + 8);

	r = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row3, row3, _MM_SHUFFLE(0, 0, 0, 0)), col1),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row3, row3, _MM_SHUFFLE(1, 1, 1, 1)), col2),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row3, row3, _MM_SHUFFLE(2, 2, 2, 2)), col3),
		_mm_mul_ps(_mm_shuffle_ps(row3, row3, _MM_SHUFFLE(3, 3, 3, 3)), col4))));

	_mm_storeu_ps(out + 8, r);  
	__m128 row4 = _mm_loadu_ps(m1 + 12);

	r = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row4, row4, _MM_SHUFFLE(0, 0, 0, 0)), col1),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row4, row4, _MM_SHUFFLE(1, 1, 1, 1)), col2),
		_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(row4, row4, _MM_SHUFFLE(2, 2, 2, 2)), col3),
		_mm_mul_ps(_mm_shuffle_ps(row4, row4, _MM_SHUFFLE(3, 3, 3, 3)), col4))));

	_mm_storeu_ps(out + 12, r);
}


