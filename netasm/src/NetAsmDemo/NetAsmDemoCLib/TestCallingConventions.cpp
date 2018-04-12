
#include "stdafx.h"
#pragma unmanaged
#include "NetAsmDemoCLib.h"
#include <intrin.h>
#include <stdio.h>

// ----------------------------------------------------------------------------------------------------
// Test ClrCall Convention parameters : (Arg0: ecx, Arg1: edx, other parameters left to right on stack)
// ----------------------------------------------------------------------------------------------------
// From 0 to 3 parameters, the ClrCall is equivalent to a FastCall
extern "C" int __fastcall  Test0ArgClrCall() { return 1;}
extern "C" int __fastcall Test1ArgClrCall(int x) {return x;}
extern "C" int __fastcall Test2ArgClrCall(int x, int y) { return x + y * 3; }
extern "C" int __fastcall Test3ArgClrCall(int x, int y, int z) {return x + y * 3 + z * 5;}
// Beyond 3 parameters (or more than one double in the parameters), arguments are passed from left to right first.
// Look at this example: initial call is x, y, z, w
// public static int Test4ArgClrCall(int x, int y, int z, int w) {return 0;}
// Translated in c : clrcall is a fastcall, but parameter order is x (ECX), y (EDX), w, z
// w and z are switch, because they are passe from left to right : so to make c fastcall compatible to clrcall, we reverse the parameter here
extern "C" int __fastcall Test4ArgClrCall(int x, int y, int w, int z) {return x + y * 3 + z * 5 + w * 7;}
extern "C" double __fastcall Test4ArgWith1DoubleClrCall(int x, int z, int w, double y) { return x + y * 3 + z * 5 + w * 7;}
extern "C" double __fastcall Test4ArgWith2DoubleClrCall(int x, int w, double z, double y) { return x + y * 3 + z * 5 + w * 7;}
extern "C" int __fastcall Test4ArgWithRefAndOutClrCall(int x, int& y, int w, int* z) { *z =  x + y * 3 + w * 5; return *z; }
extern "C" int __fastcall TestArgSizeClrCall(char x, short y, __int64 ww, double w, int z) { return x + y * 3 + z * 5 + w * 7 + ww * 9; }

// -----------------------------------------------------------------------------------------------
// Test StdCall Convention parameters : (parameters right to left on stack)
// -----------------------------------------------------------------------------------------------
extern "C" int __stdcall  Test0ArgStdCall() { return 1;}
extern "C" int __stdcall Test1ArgStdCall(int x) {return x;}
extern "C" int __stdcall Test2ArgStdCall(int x, int y) { return x + y * 3; }
extern "C" int __stdcall Test3ArgStdCall(int x, int y, int z) {return x + y * 3 + z * 5;}
extern "C" int __stdcall Test4ArgStdCall(int x, int y, int z, int w) {return x + y * 3 + z * 5 + w * 7;}
extern "C" double __stdcall Test4ArgWith1DoubleStdCall(int x, double y, int z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" double __stdcall Test4ArgWith2DoubleStdCall(int x, double y, double z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" int __stdcall Test4ArgWithRefAndOutStdCall(int x, int& y, int* z, int w) {*z =  x + y * 3 + w * 5; return *z; }
extern "C" int __stdcall  TestArgSizeStdCall(char x, short y, int z, double w, __int64 ww) { 
	return x + y * 3 + z * 5 + w * 7 + ww * 9; 
}

// -----------------------------------------------------------------------------------------------
// Test FastCall Convention parameters : (Arg0: ecx, Arg1: edx, parameters right to left on stack)
// -----------------------------------------------------------------------------------------------
extern "C" int __fastcall  Test0ArgFastCall() { return 1;}
extern "C" int __fastcall Test1ArgFastCall(int x) {return x;}
extern "C" int __fastcall Test2ArgFastCall(int x, int y) { return x + y * 3; }
extern "C" int __fastcall Test3ArgFastCall(int x, int y, int z) {return x + y * 3 + z * 5;}
extern "C" int __fastcall Test4ArgFastCall(int x, int y, int z, int w) {return x + y * 3 + z * 5 + w * 7;}
extern "C" double __fastcall Test4ArgWith1DoubleFastCall(int x, double y, int z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" double __fastcall Test4ArgWith2DoubleFastCall(int x, double y, double z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" int __fastcall Test4ArgWithRefAndOutFastCall(int x, int& y, int* z, int w) { *z =  x + y * 3 + w * 5; return *z; }
extern "C" int __fastcall  TestArgSizeFastCall(char x, short y, int z, double w, __int64 ww) { return x + y * 3 + z * 5 + w * 7 + ww * 9; }

// -----------------------------------------------------------------------------------------------
// Test Cdecl Convention parameters : (parameters right to left on stack, caller clean the stack)
// -----------------------------------------------------------------------------------------------
extern "C" int __cdecl  Test0ArgCdecl() { return 1;}
extern "C" int __cdecl Test1ArgCdecl(int x) {return x;}
extern "C" int __cdecl Test2ArgCdecl(int x, int y) { return x + y * 3; }
extern "C" int __cdecl Test3ArgCdecl(int x, int y, int z) {return x + y * 3 + z * 5;}
extern "C" int __cdecl Test4ArgCdecl(int x, int y, int z, int w) {return x + y * 3 + z * 5 + w * 7;}
extern "C" double __cdecl Test4ArgWith1DoubleCdecl(int x, double y, int z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" double __cdecl Test4ArgWith2DoubleCdecl(int x, double y, double z, int w) { return x + y * 3 + z * 5 + w * 7;}
extern "C" int __cdecl Test4ArgWithRefAndOutCdecl(int x, int& y, int* z, int w) { *z =  x + y * 3 + w * 5; return *z; }
extern "C" int __cdecl  TestArgSizeCdecl(char x, short y, int z, double w, __int64 ww) { return x + y * 3 + z * 5 + w * 7 + ww * 9; }

