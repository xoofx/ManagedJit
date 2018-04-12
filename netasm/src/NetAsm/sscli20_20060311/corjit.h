// ==++==
//
//   
//    Copyright (c) 2006 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==
/*****************************************************************************\
*                                                                             *
* CorJit.h -    EE / JIT interface                                            *
*                                                                             *
*               Version 1.0                                                   *
*******************************************************************************
*                                                                             *
*                                                                              
                                                                              *
*                                                                             *
\*****************************************************************************/

#ifndef _COR_JIT_H_
#define _COR_JIT_H_

#include <corinfo.h>

/* The default max method size that the inliner considers for inlining */
#define DEFAULT_INLINE_SIZE             32

#define CORINFO_STACKPROBE_DEPTH        256*sizeof(UINT_PTR)          // Guaranteed stack until an fcall/unmanaged
                                                    // code can set up a frame. Please make sure
                                                    // this is less than a page. This is due to
                                                    // 2 reasons:
                                                    //
                                                    // If we need to probe more than a page
                                                    // size, we need one instruction per page
                                                    // (7 bytes per instruction)
                                                    //
                                                    // The JIT wants some safe space so it doesn't
                                                    // have to put a probe on every call site. It achieves
                                                    // this by probing n bytes more than CORINFO_STACKPROBE_DEPTH
                                                    // If it hasn't used more than n for its own stuff, it
                                                    // can do a call without doing any other probe
                                                    //
                                                    // In any case, we do really expect this define to be
                                                    // small, as setting up a frame should be only pushing
                                                    // a couple of bytes on the stack
                                                    //
                                                    // There is a compile time assert
                                                    // in the x86 jit to protect you from this
                                                    //




/*****************************************************************************/
    // These are error codes returned by CompileMethod
enum CorJitResult
{
    // Note that I dont use FACILITY_NULL for the facility number,
    // we may want to get a 'real' facility number
    CORJIT_OK            =     NO_ERROR,
    CORJIT_BADCODE       =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 1),
    CORJIT_OUTOFMEM      =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 2),
    CORJIT_INTERNALERROR =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 3),
    CORJIT_SKIPPED       =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 4),
};

/* values for flags in compileMethod */

enum CorJitFlag
{
    CORJIT_FLG_SPEED_OPT           = 0x00000001,
    CORJIT_FLG_SIZE_OPT            = 0x00000002,
    CORJIT_FLG_DEBUG_CODE          = 0x00000004, // generate "debuggable" code (no code-mangling optimizations)
    CORJIT_FLG_DEBUG_EnC           = 0x00000008, // We are in Edit-n-Continue mode
    CORJIT_FLG_DEBUG_INFO          = 0x00000010, // generate line and local-var info
    CORJIT_FLG_LOOSE_EXCEPT_ORDER  = 0x00000020, // loose exception order
#if defined(_X86_)
    CORJIT_FLG_TARGET_PENTIUM      = 0x00000100,
    CORJIT_FLG_TARGET_PPRO         = 0x00000200,
    CORJIT_FLG_TARGET_P4           = 0x00000400,
    CORJIT_FLG_TARGET_BANIAS       = 0x00000800,
    CORJIT_FLG_USE_FCOMI           = 0x00001000, // Generated code may use fcomi(p) instruction
    CORJIT_FLG_USE_CMOV            = 0x00002000, // Generated code may use cmov instruction
    CORJIT_FLG_USE_SSE2            = 0x00004000, // Generated code may use SSE-2 instructions
#endif
    CORJIT_FLG_PROF_CALLRET        = 0x00010000, // Wrap method calls with probes
    CORJIT_FLG_PROF_ENTERLEAVE     = 0x00020000, // Instrument prologues/epilogues
    CORJIT_FLG_PROF_INPROC_ACTIVE_DEPRECATED    = 0x00040000, // Inprocess debugging active requires different instrumentation
    CORJIT_FLG_PROF_NO_PINVOKE_INLINE           = 0x00080000, // Disables PInvoke inlining
    CORJIT_FLG_SKIP_VERIFICATION   = 0x00100000, // (lazy) skip verification - determined without doing a full resolve. See comment below
    CORJIT_FLG_PREJIT              = 0x00200000, // jit or prejit is the execution engine.
    CORJIT_FLG_RELOC               = 0x00400000, // Generate relocatable code
    CORJIT_FLG_IMPORT_ONLY         = 0x00800000, // Only import the function
    CORJIT_FLG_IL_STUB             = 0x01000000, // method is an IL stub
    CORJIT_FLG_PROCSPLIT           = 0x02000000, // JIT should separate code into hot and cold sections
    CORJIT_FLG_BBINSTR             = 0x04000000, // Collect basic block profile information
    CORJIT_FLG_BBOPT               = 0x08000000, // Optimize method based on profile information
    CORJIT_FLG_FRAMED              = 0x10000000, // All methods have an EBP frame
    CORJIT_FLG_ALIGN_LOOPS         = 0x20000000, // add NOPs before loops to align them at 16 byte boundaries
    CORJIT_FLG_PUBLISH_SECRET_PARAM= 0x40000000, // JIT must place stub secret param into local 0.  (used by IL stubs)
};

/*****************************************************************************
Here is how CORJIT_FLG_SKIP_VERIFICATION should be interepreted.
Note that even if any method is inlined, it need not be verified.

if (CORJIT_FLG_SKIP_VERIFICATION is passed in to ICorJitCompiler::compileMethod())
{
    No verification needs to be done.
    Just compile the method, generating unverifiable code if necessary
}
else
{
    switch(ICorMethodInfo::isInstantiationOfVerifiedGeneric())
    {
    case INSTVER_NOT_INSTANTIATION:

        //
        // Non-generic case, or open generic instantiation
        //

        switch(canSkipMethodVerification())
        {
        case CORINFO_VERIFICATION_CANNOT_SKIP:
            {
                ICorMethodInfo::initConstraintsForVerification(&circularConstraints)
                if (circularConstraints)
                {
                    Just emit code to call CORINFO_HELP_VERIFICATION
                    The IL will not be compiled
                }
                else
                {
                    Verify the method.
                    if (unverifiable code is detected)
                    {
                        In place of branches with unverifiable code, emit code to call CORINFO_HELP_VERIFICATION
                        Mark the method (and any of its instantiations) as unverifiable
                    }
                    Compile the rest of the verifiable code
                }
            }

        case CORINFO_VERIFICATION_CAN_SKIP:
            {
                No verification needs to be done.
                Just compile the method, generating unverifiable code if necessary
            }

        case CORINFO_VERIFICATION_RUNTIME_CHECK:
            {
                ICorMethodInfo::initConstraintsForVerification(&circularConstraints)
                if (circularConstraints)
                {
                    Just emit code to call CORINFO_HELP_VERIFICATION
                    The IL will not be compiled

                    TODO: This could be changed to call CORINFO_HELP_VERIFICATION_RUNTIME_CHECK
                }
                else
                {
                    Verify the method.
                    if (unverifiable code is detected)
                    {
                        In the prolog, emit code to call CORINFO_HELP_VERIFICATION_RUNTIME_CHECK
                        Mark the method (and any of its instantiations) as unverifiable
                    }
                    Compile the method, generating unverifiable code if necessary
                }
            }
        }

    case INSTVER_GENERIC_PASSED_VERIFICATION:
        {
            This cannot ever happen because the VM would pass in CORJIT_FLG_SKIP_VERIFICATION.
        }

    case INSTVER_GENERIC_FAILED_VERIFICATION:

        switch(canSkipMethodVerification())
        {
            case CORINFO_VERIFICATION_CANNOT_SKIP:
                {
                    This cannot be supported because the compiler does not know which branches should call CORINFO_HELP_VERIFICATION.
                    The CLR will throw a VerificationException instead of trying to compile this method
                }

            case CORINFO_VERIFICATION_CAN_SKIP:
                {
                    This cannot ever happen because the CLR would pass in CORJIT_FLG_SKIP_VERIFICATION.
                }

            case CORINFO_VERIFICATION_RUNTIME_CHECK:
                {
                    No verification needs to be done.
                    In the prolog, emit code to call CORINFO_HELP_VERIFICATION_RUNTIME_CHECK
                    Compile the method, generating unverifiable code if necessary
                }
        }
    }
}

*/

/*****************************************************************************/
// These are flags passed to ICorJitInfo::allocMem
// to guide the memory allocation for the code, readonly data, and read-write data
enum CorJitAllocMemFlag
{
    CORJIT_ALLOCMEM_FLG_16BYTE_ALIGN  = 0x00000001, // Force the code to be 16-byte aligned
};

class ICorJitCompiler;
class ICorJitInfo;

struct IEEMemoryManager;

extern "C" ICorJitCompiler* __stdcall getJit();

/*******************************************************************************
 * ICorJitCompiler is the interface that the EE uses to get IL byteocode converted
 * to native code.  Note that to accomplish this the JIT has to call back to the
 * EE to get symbolic information.  The IJitInfo passed to the compileMethod
 * routine is the handle the JIT uses to call back to the EE
 *******************************************************************************/

class ICorJitCompiler
{
public:
    virtual CorJitResult __stdcall compileMethod (
            ICorJitInfo                 *comp,               /* IN */
            struct CORINFO_METHOD_INFO  *info,               /* IN */
            unsigned /* CorJitFlag */   flags,               /* IN */
            BYTE                        **nativeEntry,       /* OUT */
            ULONG                       *nativeSizeOfCode    /* OUT */
            ) = 0;

    virtual void __stdcall clearCache() = 0;
    virtual BOOL __stdcall isCacheCleanupRequired() = 0;
};

/*********************************************************************************
 * a ICorJitInfo is the main interface that the JIT uses to call back to the EE and
 *   get information
 *********************************************************************************/

class ICorJitInfo : public virtual ICorDynamicInfo
{
public:
    // return memory manager that the JIT can use to allocate a regular memory
    virtual IEEMemoryManager* __stdcall getMemoryManager() = 0;

    // get a block of memory for the code, readonly data, and read-write data
    virtual void __stdcall allocMem (
            ULONG               hotCodeSize,    /* IN */
            ULONG               coldCodeSize,   /* IN */
            ULONG               roDataSize,     /* IN */
            ULONG               rwDataSize,     /* IN */
            ULONG               xcptnsCount,    /* IN */
            CorJitAllocMemFlag  flag,           /* IN */
            void **             hotCodeBlock,   /* OUT */
            void **             coldCodeBlock,  /* OUT */
            void **             roDataBlock,    /* OUT */
            void **             rwDataBlock     /* OUT */
            ) = 0;


        // Get a block of memory needed for the code manager information,
        // (the info for enumerating the GC pointers while crawling the
        // stack frame).
        // Note that allocMem must be called first
    virtual void * __stdcall allocGCInfo (
            ULONG                    size        /* IN */
            ) = 0;

    virtual void * __stdcall getEHInfo(
            ) = 0;

    virtual void __stdcall yieldExecution() = 0;

	// indicate how many exception handlers blocks are to be returned
	// this is guarenteed to be called before any 'setEHinfo' call.
	// Note that allocMem must be called before this method can be called
    virtual void __stdcall setEHcount (
            unsigned		     cEH    /* IN */
            ) = 0;

    // set the values for one particular exception handler block
    //
    // Handler regions should be lexically contiguous.
    // This is because FinallyIsUnwinding() uses lexicality to
    // determine if a "finally" clause is executing
    virtual void __stdcall setEHinfo (
            unsigned		     EHnumber,   /* IN  */
            const CORINFO_EH_CLAUSE *clause      /* IN */
            ) = 0;

    // Level -> fatalError, Level 2 -> Error, Level 3 -> Warning
    // Level 4 means happens 10 times in a run, level 5 means 100, level 6 means 1000 ...
    // returns non-zero if the logging succeeded
    virtual BOOL __cdecl logMsg(unsigned level, const char* fmt, va_list args) = 0;

    // do an assert.  will return true if the code should retry (DebugBreak)
    // returns false, if the assert should be igored.
    virtual int __stdcall doAssert(const char* szFile, int iLine, const char* szExpr) = 0;

    struct ProfileBuffer
    {
        ULONG bbOffset;
        ULONG bbCount;
    };

    // allocate a basic block profile buffer where execution counts will be stored
    // for jitted basic blocks.
    virtual HRESULT __stdcall allocBBProfileBuffer (
            ULONG                 size,
            ProfileBuffer **      profileBuffer
            ) = 0;

    // get profile information to be used for optimizing the current method.  The format
    // of the buffer is the same as the format the JIT passes to allocBBProfileBuffer.
    virtual HRESULT __stdcall getBBProfileData(
            CORINFO_METHOD_HANDLE ftnHnd,
            ULONG *               size,
            ProfileBuffer **      profileBuffer,
            ULONG *               numRuns
            ) = 0;


};

/**********************************************************************************/
#endif // _COR_CORJIT_H_
