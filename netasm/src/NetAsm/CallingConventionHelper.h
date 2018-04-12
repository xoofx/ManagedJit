#pragma once

#include "CodeRef.h"
#include "MethodContext.h"
#include "X86AssemblerStream.h"

#pragma managed

using namespace System;

namespace NetAsm {

	// This class needs a good refactoring to support multiple platform (x64, x86) and better handle the 
	// stack size of the parameters.
	ref class CallingConventionHelper
	{
	public:

		CallingConventionHelper(void)
		{
		}

		static public CodeRef ConvertClrCallTo(MethodContextImpl^ methodContext, CodeRef codeRef, 
											   Runtime::InteropServices::CallingConvention callingConvention) 
		{
			if ( IntPtr::Size == 4 ) {
				switch (callingConvention) {
					case Runtime::InteropServices::CallingConvention::FastCall:
						return ConvertClrCallToFastCall(methodContext,codeRef);
						break;
					case Runtime::InteropServices::CallingConvention::StdCall:
					case Runtime::InteropServices::CallingConvention::Winapi:
						return ConvertClrCallToStdCall(methodContext,codeRef);
						break;
					case Runtime::InteropServices::CallingConvention::ThisCall:
						return ConvertClrCallToThisCall(methodContext,codeRef);
						break;
					case Runtime::InteropServices::CallingConvention::Cdecl:
						return ConvertClrCallToCdecl(methodContext, codeRef);
						break;
					default:
						return ConvertClrCallToClrCall(methodContext,codeRef);
				}
			} else {
				switch (callingConvention) {
					case Runtime::InteropServices::CallingConvention::FastCall:
					case Runtime::InteropServices::CallingConvention::StdCall:
					case Runtime::InteropServices::CallingConvention::Winapi:
					case Runtime::InteropServices::CallingConvention::ThisCall:
					case Runtime::InteropServices::CallingConvention::Cdecl:
						throw gcnew ArgumentException("Invalid Calling Convention. CLRCall calling convention is supported only for x64");
						break;
					default:
						// WARNING FOR x64, THE STACKSIZE IS PROBABLY WRONG!!!
						// THIS IS JUST A TEMPORARY WORKAROUND TO COMPILE UNDER X64
						return ConvertClrCallToClrCall(methodContext,codeRef);
				}

			}
			return codeRef;
		}


		static const int PARAM_ECX = -2;
		static const int PARAM_EDX = -1;

		ref class ParameterStackInfo {
		public:
			cli::array<int>^ argStackOffset;
			cli::array<int>^ argStackSize;
			int paramRegister;
			int CurrentArg;
			int stackSize;

			ParameterStackInfo(int numberOfArgs) {
				argStackOffset = gcnew cli::array<int>(numberOfArgs);
				argStackSize = gcnew cli::array<int>(numberOfArgs);
				stackSize = 0;
				paramRegister = PARAM_ECX;
				CurrentArg = 0;
			}

			property int Length { 
				int get() {
					return argStackOffset->Length;
				}
			}

			void pushParamOnStack(int paramSize) {
				argStackOffset[CurrentArg] = stackSize;
				argStackSize[CurrentArg] = paramSize;
				stackSize += paramSize;
			} 

			bool pushParamOnRegister(int paramSize) {
				if ( paramSize <=4 && paramRegister <= PARAM_EDX ) {
					argStackOffset[CurrentArg] = paramRegister;
					argStackSize[CurrentArg] = 4;
					paramRegister++;
					return true;
				}
				return false;
			} 

			void pushParamOnStackOrRegister(int paramSize) {
				if ( ! pushParamOnRegister(paramSize) ) {
					pushParamOnStack(paramSize);
				}
			} 

			virtual System::String^ ToString() override {
				System::Text::StringBuilder^ strBuilder = gcnew System::Text::StringBuilder();
				for(int i = 0; i < argStackOffset->Length; i++ ) {
					if ( argStackOffset[i] < 0 ) {
						strBuilder->AppendFormat("{0,3}({1,2}) ", ((argStackOffset[i]==PARAM_ECX)?"ECX":"EDX"), argStackSize[i]);
					} else {
						strBuilder->AppendFormat("{0,3:d}({1,2}) ", argStackOffset[i], argStackSize[i]);
					}
				}
				strBuilder->AppendFormat(" StackSize: {0}", stackSize);
				return strBuilder->ToString();
			}

		};

		static int GetParamStackSize(MethodContextImpl^ methodContext, System::Reflection::ParameterInfo^ paramInfo) {
			Type^ paramType = paramInfo->ParameterType;
			// If Reference or Param Out, then the argument is of a size of a pointer
			if ( paramType->IsByRef || paramInfo->IsOut) {
				return IntPtr::Size;
			}

			// Use JIT GetClassSize to get the size of the parameter
			int size = methodContext->GetClassSize(paramType);

			// Aligned on 4 bytes for the stack
			// QUESTION: on x64, is this aligned on 8 bytes?
			// TODO : CHECK MORE THIS RULE. Not sure it's working for all the cases.
			int aligned = size % 4;
			if ( aligned != 0 ) {
				size = size + (4 - aligned);
			}
			// Else 32 bits
			return size;
		}

		static cli::array<int>^ GetStackSizeOfParameters(MethodContextImpl^ methodContext) {
			cli::array<int>^ sizeOfParameters = methodContext->sizeOfParameters;
			if ( sizeOfParameters == nullptr ) {
				System::Reflection::MethodInfo^ methodInfo = methodContext->MethodInfo;
				cli::array<System::Reflection::ParameterInfo^>^ parametersInfo = methodInfo->GetParameters();
				bool isStatic = methodInfo->IsStatic;
				int nbArgsWithThis = parametersInfo->Length + (isStatic? 0 : 1);

				sizeOfParameters = gcnew cli::array<int>(nbArgsWithThis);
				methodContext->sizeOfParameters = sizeOfParameters;
				int i = 0;
				if ( ! isStatic ) {
					sizeOfParameters[i++] = IntPtr::Size;
				}
				for(int j = 0; j < parametersInfo->Length; j++,i++) {
					System::Reflection::ParameterInfo^ paramInfo = parametersInfo[j];
					sizeOfParameters[i] = GetParamStackSize( methodContext, paramInfo );
				}
			}
			return sizeOfParameters;
		}

		static ParameterStackInfo^ GetParameterStackInfoForClrCall(cli::array<int>^ sizeOfParameters) {
			int nbParams = sizeOfParameters->Length;
			ParameterStackInfo^ fromStackInfo = gcnew ParameterStackInfo(nbParams);
			// Parameter on the register and stack, left to right
			for(int i = 0; i < nbParams; i++ ) {
				fromStackInfo->pushParamOnStackOrRegister(sizeOfParameters[i]);
				fromStackInfo->CurrentArg++;
			}
			// Reorder the stack from offset ESP
			for(int i = 0; i < nbParams; i++ ) {
				if ( fromStackInfo->argStackOffset[i] >= 0 ) {
					fromStackInfo->argStackOffset[i] = fromStackInfo->stackSize - fromStackInfo->argStackOffset[i] + 4 - fromStackInfo->argStackSize[i];
				}
			}
			return fromStackInfo;
		}


		static CodeRef GenerateNativeParameterStackConversion(ParameterStackInfo^ fromStackInfo, ParameterStackInfo^ toStackInfo, CodeRef codeRef) {
			return GenerateNativeParameterStackConversion(fromStackInfo, toStackInfo, codeRef, false);
		}

		static CodeRef GenerateNativeParameterStackConversion(ParameterStackInfo^ fromStackInfo, ParameterStackInfo^ toStackInfo, CodeRef codeRef, bool isCdecl) {
			int nbParams = fromStackInfo->Length;
			X86AssemblerStream^ x86 = gcnew X86AssemblerStream();

			//  0002d	55		            push	 ebp
			x86->PushEbp();
			//  0002e	8b ec		        mov	 ebp, esp
			x86->MovEbpEsp();
			for(int i = 0; i < nbParams; i++) {
				int fromArgOffset = fromStackInfo->argStackOffset[i]; // SHIFT +4 because of push EBP
				if ( fromArgOffset >= 0 ) {
					fromArgOffset += 4;
				}
				int toArgOffset = toStackInfo->argStackOffset[i];
				int fromArgSize = fromStackInfo->argStackSize[i];
				int toArgSize = toStackInfo->argStackSize[i];
#ifdef _DEBUG
				Console::Out->WriteLine("; Map param {0} Size={1}",i, fromArgSize);
#endif
				switch (fromArgOffset) {
					case PARAM_ECX:
						switch (toArgOffset) {
							case PARAM_ECX:
								break;
							case PARAM_EDX:
								break;
							default:
								x86->MovEspOffsetEcx(-toArgOffset);
						}
						break;
					case PARAM_EDX:
						switch (toArgOffset) {
							case PARAM_ECX:
								break;
							case PARAM_EDX:
								break;
							default:
								x86->MovEspOffsetEdx(-toArgOffset);
						}
						break;
					default:
						switch (toArgOffset) {
							case PARAM_ECX:
								x86->MovEcxEspOffset(fromArgOffset);
								break;
							case PARAM_EDX:
								x86->MovEdxEspOffset(fromArgOffset);
								break;
							default:
								if ( fromArgSize == 8 ) {
									x86->MovEaxEspOffset(fromArgOffset);
									x86->MovEspOffsetEax(-toArgOffset);
									x86->MovEaxEspOffset(fromArgOffset+4);
									x86->MovEspOffsetEax(-toArgOffset+4);
								} else {
									x86->MovEaxEspOffset(fromArgOffset);
									x86->MovEspOffsetEax(-toArgOffset);
								}
						}
						break;
				}
			}
			if ( toStackInfo->stackSize > 0 ) {
				x86->SubEspOffset(toStackInfo->stackSize);
			}
			int offsetOfCall = x86->Call();

			// If isCdecl, caller clean the stack
			if ( isCdecl ) {			
				if ( toStackInfo->stackSize > 0 ) {
					// Clean the stack
					x86->AddEspOffset(toStackInfo->stackSize);
				}
			}
			x86->PopEbp();

			x86->RetOffset(fromStackInfo->stackSize);

			cli::array<unsigned char>^ codeBuffer = x86->ToBuffer();
			
			CodeRef newCodeRef = CodeRef::AllocExecutionBlock(codeBuffer);
			unsigned char* pointerBuffer = (unsigned char*)((void*)newCodeRef.Pointer);
			unsigned char* pointerOldBuffer = (unsigned char*)((void*)codeRef.Pointer);
			IntPtr callPointer = IntPtr( pointerOldBuffer - (pointerBuffer + offsetOfCall));
			System::Runtime::InteropServices::Marshal::WriteIntPtr( newCodeRef.Pointer, offsetOfCall - 4, callPointer);

			return newCodeRef;
		}

		static private CodeRef ConvertClrCallToClrCall(MethodContextImpl^ methodContext, CodeRef codeRef) {
			FillStackInfoForContext(methodContext);
			return codeRef;
		}

		static public void FillStackInfoForContext(MethodContextImpl^ methodContext) {
			if ( methodContext->sizeOfParameters == nullptr ) {
				cli::array<int>^ sizeOfParameters = GetStackSizeOfParameters(methodContext);
				ParameterStackInfo^ fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
				methodContext->StackSize = fromStackInfo->stackSize;
			}
		}


		/// <summary>
		/// Convert a ClrCall convention to a FastCall convention.
		/// </summary>
		/// <param name="methodInfo">The method to convert param from</param>
		/// <param name="codeRef">A CodeRef to the original native code ref using FastCall convention</param>
		/// <return>A CodeRef using ClrCall convetion and redirecting call to orignal coderef</return>
		static private CodeRef ConvertClrCallToFastCall(MethodContextImpl^ methodContext, CodeRef codeRef) {
			cli::array<int>^ sizeOfParameters = GetStackSizeOfParameters(methodContext);
			ParameterStackInfo^ fromStackInfo = nullptr;

			fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			methodContext->StackSize = fromStackInfo->stackSize;

			int nbParams = sizeOfParameters->Length;
			if ( nbParams == 0 ) {
				return codeRef;
			}

			// Optimize calling convention for number of params <= 3
			// If there is no 64 bit params or the 64 bit params is the last of the parameters (so on the stack)
			// then the ClrCall is equivalent to FastCall
			if ( nbParams <= 3 ) {
				int first64Bit = -1;
				for(int i = 0; i < nbParams; i++ ) {
					if ( sizeOfParameters[i] == 8 ) {
						first64Bit = i;
						break;
					}
				}
				// If no 64Bit parameters or the first 64 bit parameters is the last parameter
				// then ClrCall is equivalent to FastCall without any parameter conversion
				if ( first64Bit < 0 || first64Bit == (nbParams-1) ) {
					// Return the original codeRef. There is no need to convert the parameters.
					return codeRef;
				}
			}

			// Get ParameterStackInfo for ClrCall : convention Left To Right
			// Stack offset are positive and relative to esp when the method is called
			// CLR CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,    An)
			//                ECX,  EDX,  ESP+(n-2+1)*4, ...,   ESP+8,   ESP + 4)
			// Argument on the stack : push A2, push A3, ... push An
			if ( fromStackInfo == nullptr) {
				fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			}

			// Get ParameterStackInfo for FastCall : convention Right To Left
			// Stack offset are negative and relative to esp 
			// We use the ClrCall StackInfo but then, we reverse the stack offset
			// FAST CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,           An)
			//                ECX,  EDX,  ESP+4,       ,    ,   ESP + (n-1)*4,  ESP + n * 4)
			// Argument on the stack : push An, push An-1, ... push A2
			ParameterStackInfo^ toStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			for(int i = 0; i < nbParams; i++) {
				if ( toStackInfo->argStackOffset[i] >= 0 ) {
					// If parameter is on the stack, reverse the stack offset, as it is right to left
					toStackInfo->argStackOffset[i] = toStackInfo->argStackOffset[i] - 4 + toStackInfo->argStackSize[i];
				}
			}

#ifdef _DEBUG
			System::Console::Out->WriteLine("Conversion method [{0}] from ClrCall to FastCall", methodContext->MethodInfo->ToString());
			System::Console::Out->WriteLine("ClrCall : {0}", fromStackInfo);
			System::Console::Out->WriteLine("FastCall: {0}", toStackInfo);
#endif

			CodeRef newCodeRef = GenerateNativeParameterStackConversion(fromStackInfo, toStackInfo, codeRef );

			return newCodeRef;
		}

		static private CodeRef ConvertClrCallToCdecl(MethodContextImpl^ methodContext, CodeRef codeRef) {
			return ConvertCommonClrCallToStdCall(methodContext, codeRef, true);
		}

		static private CodeRef ConvertClrCallToStdCall(MethodContextImpl^ methodContext, CodeRef codeRef) {
			return ConvertCommonClrCallToStdCall(methodContext, codeRef, false);
		}

		/// <summary>
		/// Convert a ClrCall convention to a StdCall convention.
		/// </summary>
		/// <param name="methodInfo">The method to convert param from</param>
		/// <param name="codeRef">A CodeRef to the original native code ref using StdCall convention</param>
		/// <return>A CodeRef using StdCall convetion and redirecting call to orignal coderef</return>
		static private CodeRef ConvertCommonClrCallToStdCall(MethodContextImpl^ methodContext, CodeRef codeRef, bool isCdecl) {
			cli::array<int>^ sizeOfParameters = GetStackSizeOfParameters(methodContext);
			ParameterStackInfo^ fromStackInfo = nullptr;

			fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			methodContext->StackSize = fromStackInfo->stackSize;

			int nbParams = sizeOfParameters->Length;
			if ( nbParams == 0 ) {
				return codeRef;
			}

			// Optimize calling convention for number of params <= 3
			// If there is no 64 bit params or the 64 bit params is the last of the parameters (so on the stack)
			// then we just need to push ecx (And edx if params == 2) on the stack
			if ( nbParams <= 3 && ! isCdecl ) {
				int first64Bit = -1;
				for(int i = 0; i < nbParams; i++ ) {
					if ( sizeOfParameters[i] == 8 ) {
						first64Bit = i;
						break;
					}
				}
				// If no 64Bit parameters or the first 64 bit parameters is the last parameter
				// then ClrCall is equivalent to FastCall without any parameter conversion
				if ( first64Bit < 0 || first64Bit == (nbParams-1) ) {
					cli::array<byte>^ stdCallBuffer = nullptr;
					if ( nbParams == 1 ) {
						// 58               pop         eax  
						// 51               push        ecx  
						// 50               push        eax  
						// E9 00 00 00 00   jmp         functionCalled (0000000h) 
						stdCallBuffer = gcnew cli::array<byte> {0x58, 0x51, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00};
					} else {
						// 58               pop         eax  
						// 52               push        edx  
						// 51               push        ecx  
						// 50               push        eax  
						// E9 00 00 00 00   jmp         functionCalled (0000000h) 
						stdCallBuffer = gcnew cli::array<byte> {0x58, 0x52, 0x51, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00};
					}

					CodeRef stdCallCodeRef = CodeRef::AllocExecutionBlock(stdCallBuffer);
					IntPtr jumpPointer = IntPtr((byte*)((void*)codeRef.Pointer) - (((byte*)((void*)stdCallCodeRef.Pointer)) + stdCallBuffer->Length ));
					System::Runtime::InteropServices::Marshal::WriteIntPtr(stdCallCodeRef.Pointer, stdCallBuffer->Length - IntPtr::Size, jumpPointer);
					return stdCallCodeRef;
				}
			}

			// Get ParameterStackInfo for ClrCall : convention Left To Right
			// Stack offset are positive and relative to esp when the method is called
			// CLR CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,    An)
			//                ECX,  EDX,  ESP+(n-2+1)*4, ...,   ESP+8,   ESP + 4)
			// Argument on the stack : push A2, push A3, ... push An
			if ( fromStackInfo == nullptr) {
				fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			}

			// Get ParameterStackInfo for FastCall : convention Right To Left
			// Stack offset are negative and relative to esp 
			// We use the ClrCall StackInfo but then, we reverse the stack offset
			// FAST CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,           An)
			//                ECX,  EDX,  ESP+4,       ,    ,   ESP + (n-1)*4,  ESP + n * 4)
			// Argument on the stack : push An, push An-1, ... push A2
			ParameterStackInfo^ toStackInfo = gcnew ParameterStackInfo(nbParams);
			// Parameter on the register and stack, left to right
			for(int i = 0; i < nbParams; i++ ) {
				toStackInfo->pushParamOnStack(sizeOfParameters[i]);
				toStackInfo->CurrentArg++;
			}
			for(int i = 0; i < nbParams; i++ ) {
				toStackInfo->argStackOffset[i] =  toStackInfo->stackSize - toStackInfo->argStackOffset[i];
			}

#ifdef _DEBUG
			System::Console::Out->WriteLine("Conversion method [{0}] from ClrCall to StdCall", methodContext->MethodInfo->ToString());
			System::Console::Out->WriteLine("ClrCall: {0}", fromStackInfo);
			System::Console::Out->WriteLine("StdCall: {0}", toStackInfo);
#endif

			CodeRef newCodeRef = GenerateNativeParameterStackConversion(fromStackInfo, toStackInfo, codeRef, isCdecl);

			return newCodeRef;
		}

		/// <summary>
		/// Convert a ClrCall convention to a ThisCall convention.
		/// </summary>
		/// <param name="methodInfo">The method to convert param from</param>
		/// <param name="codeRef">A CodeRef to the original native code ref using ThisCall convention</param>
		/// <return>A CodeRef using ThisCall convetion and redirecting call to orignal coderef</return>
		static private CodeRef ConvertClrCallToThisCall(MethodContextImpl^ methodContext, CodeRef codeRef) {
			cli::array<int>^ sizeOfParameters = GetStackSizeOfParameters(methodContext);
			ParameterStackInfo^ fromStackInfo = nullptr;

			fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			methodContext->StackSize = fromStackInfo->stackSize;

			int nbParams = sizeOfParameters->Length;

			if ( nbParams == 0 ) {
				return codeRef;
			}

			// Optimize calling convention for number of params <= 3
			// If there is no 64 bit params or the 64 bit params is the last of the parameters (so on the stack)
			// then we just need to push ecx (And edx if params == 2) on the stack
			if ( nbParams <= 2 ) {
				int first64Bit = -1;
				for(int i = 0; i < nbParams; i++ ) {
					if ( sizeOfParameters[i] == 8 ) {
						first64Bit = i;
						break;
					}
				}
				// If no 64Bit parameters or the first 64 bit parameters is the last parameter
				// then ClrCall is equivalent to ThisCall without any parameter conversion
				if ( first64Bit < 0 || first64Bit == (nbParams-1) ) {
					cli::array<byte>^ stdCallBuffer = nullptr;
					if ( nbParams == 1 ) {
						return codeRef;
					} else {
						// 58               pop         eax  
						// 52               push        edx
						// 50               push        eax  
						// E9 00 00 00 00   jmp         functionCalled (0000000h) 
						stdCallBuffer = gcnew cli::array<byte> {0x58, 0x52, 0x50, 0xE9, 0x00, 0x00, 0x00, 0x00};
					}

					CodeRef stdCallCodeRef = CodeRef::AllocExecutionBlock(stdCallBuffer);
					IntPtr jumpPointer = IntPtr((byte*)((void*)codeRef.Pointer) - (((byte*)((void*)stdCallCodeRef.Pointer)) + stdCallBuffer->Length ));
					System::Runtime::InteropServices::Marshal::WriteIntPtr(stdCallCodeRef.Pointer, stdCallBuffer->Length - IntPtr::Size, jumpPointer);
					return stdCallCodeRef;
				}
			}

			// Get ParameterStackInfo for ClrCall : convention Left To Right
			// Stack offset are positive and relative to esp when the method is called
			// CLR CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,    An)
			//                ECX,  EDX,  ESP+(n-2+1)*4, ...,   ESP+8,   ESP + 4)
			// Argument on the stack : push A2, push A3, ... push An
			if ( fromStackInfo == nullptr) {
				fromStackInfo = GetParameterStackInfoForClrCall(sizeOfParameters);
			}

			// Get ParameterStackInfo for FastCall : convention Right To Left
			// Stack offset are negative and relative to esp 
			// We use the ClrCall StackInfo but then, we reverse the stack offset
			// FAST CALL
			// void function( A0,   A1,   A2,          , ...,   An-1,           An)
			//                ECX,  EDX,  ESP+4,       ,    ,   ESP + (n-1)*4,  ESP + n * 4)
			// Argument on the stack : push An, push An-1, ... push A2
			ParameterStackInfo^ toStackInfo = gcnew ParameterStackInfo(nbParams);
			// Parameter on the register and stack, left to right
			
			if ( sizeOfParameters[0] == 4) {
				toStackInfo->pushParamOnRegister(sizeOfParameters[0]);
			} else {
				throw gcnew System::ArgumentException(String::Format("Unsupported ThisCall convention with first argument in method {0}. This argument cannot be 64 bits", methodContext->MethodInfo));
			}

			for(int i = 1; i < nbParams; i++ ) {
				toStackInfo->pushParamOnStack(sizeOfParameters[i]);
				toStackInfo->CurrentArg++;
			}
			for(int i = 1; i < nbParams; i++ ) {
				toStackInfo->argStackOffset[i] =  toStackInfo->stackSize - toStackInfo->argStackOffset[i];
			}

#ifdef _DEBUG
			System::Console::Out->WriteLine("Conversion method [{0}] from ClrCall to ThisCall", methodContext->MethodInfo->ToString());
			System::Console::Out->WriteLine("ClrCall : {0}", fromStackInfo);
			System::Console::Out->WriteLine("ThisCall: {0}", toStackInfo);
#endif

			CodeRef newCodeRef = GenerateNativeParameterStackConversion(fromStackInfo, toStackInfo, codeRef );

			return newCodeRef;
		}
	};
}
