#pragma once

using namespace System;

namespace NetAsm {

	ref class X86AssemblerStream {
		System::IO::BinaryWriter^ writer;
		System::IO::MemoryStream^ stream;

	public:
		X86AssemblerStream() {
			stream = gcnew System::IO::MemoryStream();
			writer = gcnew System::IO::BinaryWriter(stream);

		}

		cli::array<unsigned char>^ ToBuffer() {
			return stream->ToArray();
		}

		int Call() {
			//  00048	e8 00 00 00 00	 call	 ?functionCalled@@YGXHHH@Z ; functionCalled
			stream->WriteByte(0xE8);
			stream->WriteByte(0x00);
			stream->WriteByte(0x00);
			stream->WriteByte(0x00);
			stream->WriteByte(0x00);
#ifdef _DEBUG
			Console::Out->WriteLine("call function");
#endif
			return stream->Position;
		}

		void PushEbp() {
			//  0002d	55		            push	 ebp
			stream->WriteByte(0x55);
#ifdef _DEBUG
			Console::Out->WriteLine("push ebp");
#endif
		}

		void PopEbp() {
			//  00048	5d			        pop	 ebp
			stream->WriteByte(0x5D);
#ifdef _DEBUG
			Console::Out->WriteLine("pop ebp");
#endif
		}

		void RetOffset(int offset) {
			if ( offset == 0 ) {
				stream->WriteByte(0xC3);
#ifdef _DEBUG
				Console::Out->WriteLine("ret");
#endif
			} else {
				//  0004c	c2 0c 00	        ret	 12	
				stream->WriteByte(0xC2);
				writer->Write((short)offset);
#ifdef _DEBUG
				Console::Out->WriteLine("ret {0}",offset);
#endif
			}
		}

		void MovEbpEsp() {
			//  0002e	8b ec		        mov	 ebp, esp
			stream->WriteByte(0x8B);
			stream->WriteByte(0xEC);
#ifdef _DEBUG
			Console::Out->WriteLine("mov ebp, esp");
#endif
		}

		void SubEspOffset(int offset) {
			//  00048	83 ec 10	        sub	 esp, 16
			stream->WriteByte(0x83);
			stream->WriteByte(0xEC);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("sub esp, {0}",offset);
#endif			
		}

		void AddEspOffset(int offset) {
			//  00048	83 c4 05		add	 esp, 5
			stream->WriteByte(0x83);
			stream->WriteByte(0xC4);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("add esp, {0}",offset);
#endif
		}


		void MovEcxEspOffset(int offset) {
			//  00030	8b 4d 10	        mov	 ecx, DWORD PTR [ebp+16]
			stream->WriteByte(0x8B);
			stream->WriteByte(0x4D);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov ecx, DWORD PTR [ebp+{0}]",offset);
#endif	
		}
		void MovEdxEspOffset(int offset) {
			// 00033	8b 55 10	        mov	 edx, DWORD PTR [ebp+16]
			stream->WriteByte(0x8B);
			stream->WriteByte(0x55);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov edx, DWORD PTR [ebp+{0}]",offset);
#endif	
		}
		void MovEaxEspOffset(int offset) {
			//  00036	8b 45 10	        mov	 eax, DWORD PTR [ebp+16]
			stream->WriteByte(0x8B);
			stream->WriteByte(0x45);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov eax, DWORD PTR [ebp+{0}]",offset);
#endif	
		}

		void MovEspOffsetEcx(int offset) {
			//  00039	89 4d 10	        mov	 DWORD PTR [ebp+16], ecx
			stream->WriteByte(0x89);
			stream->WriteByte(0x4D);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov DWORD PTR [ebp{0}], ecx",offset);
#endif	
		}

		void MovEspOffsetEdx(int offset) {
			//  0003c	89 55 10	        mov	 DWORD PTR [ebp+16], edx
			stream->WriteByte(0x89);
			stream->WriteByte(0x55);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov DWORD PTR [ebp{0}], edx",offset);
#endif	
		}

		void MovEspOffsetEax(int offset) {
			//  0003f	89 45 10	        mov	 DWORD PTR [ebp+16], eax
			stream->WriteByte(0x89);
			stream->WriteByte(0x45);
			stream->WriteByte((unsigned char)offset);
#ifdef _DEBUG
			Console::Out->WriteLine("mov DWORD PTR [ebp{0}], eax",offset);
#endif	
		}
	};
}
