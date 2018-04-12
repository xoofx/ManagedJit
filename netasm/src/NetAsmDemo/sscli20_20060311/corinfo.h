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
* CorInfo.h -    EE / Code generator interface                                *
*                                                                             *
*               Version 1.0                                                   *
*******************************************************************************
*                                                                             *
*                                                                              
                                                                              *
*                                                                             *
*******************************************************************************
*
* This file exposes CLR runtime functionality. It can be used by compilers,
* both Just-in-time and ahead-of-time, to generate native code which
* executes in the runtime environment.
*******************************************************************************


*******************************************************************************
The semantic contract between the EE and the JIT should be documented here
It is incomplete, but as time goes on, that hopefully will change

-------------------------------------------------------------------------------
Class Construction

First of all class contruction comes in two flavors precise and 'beforeFieldInit'.
In C# you get the former if you declare an explicit class constructor method and
the later if you declaratively initialize static fields.  Precise class construction
guarentees that the .cctor is run precisely before the first access to any method
or field of the class.  'beforeFieldInit' semantics guarentees only that the 
.cctor will be run some time before the first static field access (note that
calling methods (static or insance) or accessing instance fields does not cause
.cctors to be run).  

Next you need to know that there are two kinds of code generation that can happen
in the JIT: appdomain neutral and appdomain specialized.  The difference 
between these two kinds of code is how statics are handled.  For appdomain
specific code, the address of a particular static variable is embeded in the
code.  This makes it usable only for one appdomain (since every appdomain gets
a own copy of its statics).  Appdomain neutral code calls a helper that looks
up static variables off of a thread local variable.  Thus the same code can be
used by mulitple appdomains in the same process.  

Generics also introduce a similar issue.  Code for generic classes might be
specialised for a particular set of type arguments, or it could use helpers
to access data that depends on type parameters and thus be shared across several
instantiations of the generic type.

Thus there four cases

    BeforeFieldInitCCtor - Unshared code.
        Cctors are only called when static fields are fetched.  At the time
        the method that touches the static field is JITed (or fixed up in the
        case of NGENed code), the .cctor is called.  

    BeforeFieldInitCCtor - Shared code.
        Since the same code is used for multiple classes, the act of JITing
        the code can not be used as a hook.  However, it is also the case that
        since the code is shared, it can not wire in a particular address for
        the static and thus needs to use a helper that looks up the correct 
        address based on the thread ID.   This helper does the .cctor check, 
        and thus no additional cctor logic is needed.

    PreciseCCtor - Unshared code.
        Any time a method is JITTed (or fixed up in the case of NGEN), a 
        cctor check for the class of the method being JITTed is done.  In 
        addition the JIT inserts explicit checks before any static field 
        accesses.  Instance methods and fields do NOT have hooks because
        a .ctor method must be called before the instance can be created.

    PreciseCctor - Shared code
        .cctor checks are placed in the prolog of every .ctor and static 
        method.  All methods that access static fields have an explicit
        .cctor check before use.   Again instance methods don't have hooks
        because a .ctor would have to be called first. 

Technically speaking, however the optimization of avoiding checks
on instance methods is flawed.  It requires that a .ctor alwasy
preceed a call to an instance methods.   This break down when

    1) A NULL is passed to an instance method.
    2) A .ctor does not call its superlcasses .ctor.  THis allows
       an instance to be created without necessarily calling all
       the .cctors of all the superclasses.  A virtual call can
       then be made to a instance of a superclass without necessarily
       calling the superclass's .cctor.
    3) The class is a value class (which exists without a .ctor 
       being called. 

Nevertheless, the cost of plugging these holes is considered to
high and the benefit is low.  

----------------------------------------------------------------------

Thus the JIT's cctor responsibilities require it to check with the EE on every
static field access using 'initClass', and also to check CORINFO_FLG_RUN_CCTOR     
before jitting any method to see if a .cctor check must be placed in the prolog.

CORINFO_FLG_NEEDS_INIT  Only classes with NEEDS_INIT need to check possibly
                    place .cctor checks before static field accesses.

CORINFO_FLG_INITIALIZED Even if the class needs initing, it might be true
                    that the class has already been initialized.  If 
                    this bit is true, again, nothing needs to be done.
                    If false, initClass needs to be called

initClass           For classes with CORINFO_FLG_NEEDS_INIT and not
                    CORINFO_FLG_INITIALIZED, initClass needs to be 
                    called to determine if a CCTOR check is needed 
                    (For class with beforeFieldInit semantics initclass 
                    may run the cctor eagerly and returns that not 
                    .cctor check is needed).  If a .cctor check
                    is required the CORINFO_HELP_INITCLASS method has
                    to be called with a class handle parameter 
                    (see embedGenericHandle)

CORINFO_FLG_RUN_CCTOR The jit is required to put CCTOR checks in the 
                    prolog of methods with this attribute.  Unfortunately
                    exactly what helper what helper is complicated

                        TODO describe

CORINFO_FLG_BEFOREFIELDINIT indicate the class has beforeFieldInit semantics.
                    The jit does not strictly need this information
                    however, it is valuable in optimizing static field
                    fetch helper calls.  Helper call for classes with
                    BeforeFieldInit semantics can be hoisted before other
                    side effects where classes with precise .cctor
                    semantics do not allow this optimization.


Inlining also complicates things.  Because the class could have precise semantics
it is also required that the inlining of any constructor or static method must
also do the CORINFO_FLG_NEEDS_INIT, CORINFO_FLG_INITIALIZED, initClass check.  
(Instance methods don't have to because constructors always come first).  In 
addition inlined functions must also check the CORINFO_FLG_RUN_CCTOR bit.  In
either case, the inliner has the option of inserting any required runtime check
or simply not inlining the function.  
                    
Finally, the JIT does has the option of skipping the CORINFO_FLG_NEEDS_INIT, 
CORINFO_FLG_INITIALIZED, initClass check in the following cases

    1) A static field access from an instance method of the same class
    2) When inlining a method from and instance method of the same class.

The rationale here is that it can be assumed that if you are in an instance 
method, the .cctor has already been called, and thus the check is not needed.
Historyically the getClassAttribs function did not have the method being 
compiled passed to it so the EE could not fold this into its .cctor logic and
thus the JIT needed to do this.  TODO: Now that this has changed, we should
have the EE do this. 

-------------------------------------------------------------------------------

Static fields

The first 4 options are mutially exclusive 

CORINFO_FLG_HELPER  If the field has this set, then the JIT must call
                    getFieldHelper and call the returned helper with
                    the object ref (for an instance field) and a fieldDesc.  
                    Note that this should be able to handle ANY field
                    so to get a JIT up quickly, it has the option of
                    using helper calls for all field access (and skip
                    the complexity below).  Note that for statics it
                    is assumed that you will alwasy ask for the 
                    ADDRESSS helper and to the fetch in the JIT. 

CORINFO_FLG_SHARED_HELPER This is currently only used for static fields.
                    If this bit is set it means that the field is
                    feched by a helper call that takes a module
                    identifier (see getModuleDomainID) and a class
                    identifier (see getClassDomainID) as arguments.
                    The exact helper to call is determined by 
                    getSharedStaticBaseHelper.  The return value is
                    of this function is the base of all statics in
                    the module. The offset from getFieldOffset must
                    be added to this value to get the address of the
                    field itself. (see also CORINFO_FLG_STATIC_IN_HEAP).


CORINFO_FLG_GENERICS_STATIC This is currently only used for static fields
                    (of generic type).  This function is intended to
                    be called with a Generic handle as a argument
                    (from embedGenericHandle).  The exact helper to
                    call is determined by getSharedStaticBaseHelper.  
                    The returned value is the base of all statics
                    in the class.  The offset from getFieldOffset must
                    be added to this value to get the address of the
                    (see also CORINFO_FLG_STATIC_IN_HEAP).

CORINFO_FLG_TLS     This indicate that the static field is a Windows
                    style Thread Local Static.  (We also have managed
                    thread local statics, which work through the HELPER.
                    Support for this is considered legacy, and going
                    forward, the EE should 

<NONE>                    This is a normal static field.   Its address in 
                    in memory is determined by getFieldAddress.
                    (see also CORINFO_FLG_STATIC_IN_HEAP).


This last field can modify any of the cases above except CORINFO_FLG_HELPER

CORINFO_FLG_STATIC_IN_HEAP This is currently only used for static fields
                    of value classes.  If the field has this set then 
                    after computing what would normally be the field, 
                    what you actually get is a object poitner (that 
                    must be reported to the GC) to a boxed version 
                    of the value.  Thus the actual field address is
                    computed by addr = (*addr+sizeof(OBJECTREF))

Instance fields

CORINFO_FLG_HELPER  This is used if the class is MarshalByRef, which
                    means that the object might be a proxyt to the
                    real object in some other appdomain or process.
                    If the field has this set, then the JIT must call
                    getFieldHelper and call the returned helper with
                    the object ref.  If the helper returned is helpers
                    that are for structures the args are as follows

CORINFO_HELP_GETFIELDSTRUCT - args are: retBuff, object, fieldDesc
CORINFO_HELP_SETFIELDSTRUCT - args are: object fieldDesc value

The other GET helpers take an object fieldDesc and return the value
        The other SET helpers take an object fieldDesc and value

    Note that unlike static fields there is no helper to take the address
    of a field because in general there is no address for proxies (LDFLDA
    is illegal on proxies). 

    CORINFO_FLG_EnC         This is to support adding new field for edit and
                            continue.  This field also indicates that a helper 
                            is needed to access this field.  However this helper
                            is always CORINFO_HELP_GETFIELDADDR, and this
                            helper always takes the object and field handle
                            and returns the address of the field.  It is the
                            JIT's responcibility to do the fetch or set. 

-------------------------------------------------------------------------------

TODO: Talk about initializing strutures before use 


*******************************************************************************
*/

#ifndef _COR_INFO_H_
#define _COR_INFO_H_

#include <corhdr.h>
#include <specstrings.h>




// CorInfoHelpFunc defines the set of helpers (accessed via the ICorDynamicInfo::getHelperFtn())
// These helpers can be called by native code which executes in the runtime.
// Compilers can emit calls to these helpers.
//
// The signatures of the helpers are below (see RuntimeHelperArgumentCheck)

enum CorInfoHelpFunc
{
    CORINFO_HELP_UNDEF,         // invalid value. This should never be used

    /* Arithmetic helpers */

    CORINFO_HELP_LLSH,
    CORINFO_HELP_LRSH,
    CORINFO_HELP_LRSZ,
    CORINFO_HELP_LMUL,
    CORINFO_HELP_LMUL_OVF,
    CORINFO_HELP_ULMUL_OVF,
    CORINFO_HELP_LDIV,
    CORINFO_HELP_LMOD,
    CORINFO_HELP_ULDIV,
    CORINFO_HELP_ULMOD,
    CORINFO_HELP_ULNG2DBL,              // Convert a unsigned in to a double
    CORINFO_HELP_DBL2INT,
    CORINFO_HELP_DBL2INT_OVF,
    CORINFO_HELP_DBL2LNG,
    CORINFO_HELP_DBL2LNG_OVF,
    CORINFO_HELP_DBL2UINT,
    CORINFO_HELP_DBL2UINT_OVF,
    CORINFO_HELP_DBL2ULNG,
    CORINFO_HELP_DBL2ULNG_OVF,
    CORINFO_HELP_FLTREM,
    CORINFO_HELP_DBLREM,

    /* Allocating a new object. Always use ICorClassInfo::getNewHelper() to decide 
       which is the right helper to use to allocate an object of a given type. */

    CORINFO_HELP_NEW_DIRECT,        // new object
    CORINFO_HELP_NEW_CROSSCONTEXT,  // cross context new object
    CORINFO_HELP_NEWFAST,
    CORINFO_HELP_NEWSFAST,          // allocator for small, non-finalizer, non-array object
    CORINFO_HELP_NEWSFAST_ALIGN8,   // allocator for small, non-finalizer, non-array object, 8 byte aligned
    CORINFO_HELP_NEWSFAST_CHKRESTORE, // allocator like CORINFO_HELP_NEWSFAST, bails if type not restored.
    CORINFO_HELP_NEW_SPECIALDIRECT, // direct but only if no context needed
    CORINFO_HELP_NEW_MDARR,         // multi-dim array helper (with or without lower bounds)
    CORINFO_HELP_NEW_MDARR_NO_LBOUNDS, // multi-dim helper without lower bounds
    CORINFO_HELP_NEWARR_1_DIRECT,   // helper for any one dimensional array creation
    CORINFO_HELP_NEWARR_1_OBJ,      // optimized 1-D object arrays
    CORINFO_HELP_NEWARR_1_VC,       // optimized 1-D value class arrays
    CORINFO_HELP_NEWARR_1_ALIGN8,   // like VC, but aligns the array start
    CORINFO_HELP_STRCNS,            // create a new string literal

    /* Object model */

    CORINFO_HELP_INITCLASS,         // Initialize class if not already initialized
    CORINFO_HELP_INITINSTCLASS,     // Initialize class for instantiated type

    // Use ICorClassInfo::getIsInstanceOfHelper/getChkCastHelper to determine
    // the right helper to use

    CORINFO_HELP_ISINSTANCEOFINTERFACE, // Optimized helper for interfaces
    CORINFO_HELP_ISINSTANCEOFARRAY,  // Optimized helper for arrays
    CORINFO_HELP_ISINSTANCEOFCLASS, // Optimized helper for classes
    CORINFO_HELP_ISINSTANCEOFANY,   // Slow helper for any type
    CORINFO_HELP_CHKCASTINTERFACE,
    CORINFO_HELP_CHKCASTARRAY,
    CORINFO_HELP_CHKCASTCLASS,
    CORINFO_HELP_CHKCASTANY,
    CORINFO_HELP_CHKCASTCLASS_SPECIAL, // Optimized helper for classes. Assumes that the trivial cases 
                                    // has been taken care of by the inlined check

    CORINFO_HELP_BOX,
    CORINFO_HELP_BOX_NULLABLE,      // special form of boxing for Nullable<T>
    CORINFO_HELP_UNBOX,
    CORINFO_HELP_UNBOX_NULLABLE,    // special form of unboxing for Nullable<T>
    CORINFO_HELP_GETREFANY,         // Extract the byref from a TypedReference, checking that it is the expected type

    CORINFO_HELP_ARRADDR_ST,        // assign to element of object array with type-checking
    CORINFO_HELP_LDELEMA_REF,       // does a precise type comparision and returns address

    /* Exceptions */

    CORINFO_HELP_THROW,             // Throw an exception object
    CORINFO_HELP_RETHROW,           // Rethrow the currently active exception
    CORINFO_HELP_USER_BREAKPOINT,   // For a user program to break to the debugger
    CORINFO_HELP_RNGCHKFAIL,        // array bounds check failed
    CORINFO_HELP_OVERFLOW,          // throw an overflow exception

    CORINFO_HELP_INTERNALTHROW,     // Support for really fast jit
    CORINFO_HELP_INTERNALTHROW_FROM_HELPER,
    CORINFO_HELP_VERIFICATION,      // Throw a VerificationException
    CORINFO_HELP_SEC_UNMGDCODE_EXCPT, // throw a security unmanaged code exception
    CORINFO_HELP_FAIL_FAST,         // Kill the process avoiding any exceptions or stack and data dependencies (use for GuardStack unsafe buffer checks)

    CORINFO_HELP_ENDCATCH,          // call back into the EE at the end of a catch block

    /* Synchronization */

    CORINFO_HELP_MON_ENTER,
    CORINFO_HELP_MON_EXIT,
    CORINFO_HELP_MON_ENTER_STATIC,
    CORINFO_HELP_MON_EXIT_STATIC,

    CORINFO_HELP_GETCLASSFROMMETHODPARAM, // Given a generics method handle, returns a class handle
    CORINFO_HELP_GETSYNCFROMCLASSHANDLE,  // Given a generics class handle, returns the sync monitor 
                                          // in its ManagedClassObject

    /* Security callout support */
    
    CORINFO_HELP_SECURITY_PROLOG,   // Required if CORINFO_FLG_SECURITYCHECK is set, or CORINFO_FLG_NOSECURITYWRAP is not set
    CORINFO_HELP_SECURITY_PROLOG_FRAMED, // Slow version of CORINFO_HELP_SECURITY_PROLOG. Used for instrumentation.
    CORINFO_HELP_SECURITY_EPILOG,   // Required if CORINFO_FLG_SECURITYCHECK is set, or CORINFO_FLG_NOSECURITYWRAP is not set

    CORINFO_HELP_CALL_ALLOWED_BYSECURITY, // Callout to security - used if CORINFO_CALL_ALLOWED_BYSECURITY

     /* Verification runtime callout support */

    CORINFO_HELP_VERIFICATION_RUNTIME_CHECK, // Do a Demand for UnmanagedCode permission at runtime

    /* GC support */

    CORINFO_HELP_STOP_FOR_GC,       // Call GC (force a GC)
    CORINFO_HELP_POLL_GC,           // Ask GC if it wants to collect

    CORINFO_HELP_STRESS_GC,         // Force a GC, but then update the JITTED code to be a noop call
    CORINFO_HELP_CHECK_OBJ,         // confirm that ECX is a valid object pointer (debugging only)

    /* GC Write barrier support */

    CORINFO_HELP_ASSIGN_REF,        // universal helpers with F_CALL_CONV calling convention
    CORINFO_HELP_CHECKED_ASSIGN_REF,

    CORINFO_HELP_ASSIGN_BYREF,      // EDI is byref, will do a MOVSD, incl. inc of ESI and EDI
                                    // will trash ECX

    CORINFO_HELP_ASSIGN_STRUCT,

    CORINFO_HELP_SAFE_RETURNABLE_BYREF, // Check that a byref is safe to be returned from a call by ensuring that it points into the GC heap

#ifdef _X86_
    CORINFO_HELP_ASSIGN_REF_EAX,    // EAX hold GC ptr, want do a 'mov [EDX], EAX' and inform GC
    CORINFO_HELP_ASSIGN_REF_EBX,    // EBX hold GC ptr, want do a 'mov [EDX], EBX' and inform GC
    CORINFO_HELP_ASSIGN_REF_ECX,    // ECX hold GC ptr, want do a 'mov [EDX], ECX' and inform GC
    CORINFO_HELP_ASSIGN_REF_ESI,    // ESI hold GC ptr, want do a 'mov [EDX], ESI' and inform GC
    CORINFO_HELP_ASSIGN_REF_EDI,    // EDI hold GC ptr, want do a 'mov [EDX], EDI' and inform GC
    CORINFO_HELP_ASSIGN_REF_EBP,    // EBP hold GC ptr, want do a 'mov [EDX], EBP' and inform GC

    CORINFO_HELP_CHECKED_ASSIGN_REF_EAX,  // These are the same as ASSIGN_REF above ...
    CORINFO_HELP_CHECKED_ASSIGN_REF_EBX,  // ... but checks if EDX points to heap.
    CORINFO_HELP_CHECKED_ASSIGN_REF_ECX,
    CORINFO_HELP_CHECKED_ASSIGN_REF_ESI,
    CORINFO_HELP_CHECKED_ASSIGN_REF_EDI,
    CORINFO_HELP_CHECKED_ASSIGN_REF_EBP,
#endif

    /* Accessing fields */

    // For COM object support (using COM get/set routines to update object)
    // and EnC and cross-context support
    CORINFO_HELP_GETFIELD32,
    CORINFO_HELP_SETFIELD32,
    CORINFO_HELP_GETFIELD64,
    CORINFO_HELP_SETFIELD64,
    CORINFO_HELP_GETFIELDOBJ,
    CORINFO_HELP_SETFIELDOBJ,
    CORINFO_HELP_GETFIELDSTRUCT,
    CORINFO_HELP_SETFIELDSTRUCT,
    CORINFO_HELP_GETFIELDFLOAT,
    CORINFO_HELP_SETFIELDFLOAT,
    CORINFO_HELP_GETFIELDDOUBLE,
    CORINFO_HELP_SETFIELDDOUBLE,

    CORINFO_HELP_GETFIELDADDR,

    CORINFO_HELP_GETSTATICFIELDADDR,
    CORINFO_HELP_GETGENERICS_GCSTATIC_BASE,
    CORINFO_HELP_GETGENERICS_NONGCSTATIC_BASE,

    // Use ICorClassInfo::getSharedStaticBaseHelper/getSharedCCtorHelper
    // to determine the right helper to use
    CORINFO_HELP_GETSHARED_GCSTATIC_BASE,
    CORINFO_HELP_GETSHARED_NONGCSTATIC_BASE,
    CORINFO_HELP_GETSHARED_GCSTATIC_BASE_NOCTOR,
    CORINFO_HELP_GETSHARED_NONGCSTATIC_BASE_NOCTOR,
    CORINFO_HELP_GETSHARED_GCSTATIC_BASE_DYNAMICCLASS,
    CORINFO_HELP_GETSHARED_NONGCSTATIC_BASE_DYNAMICCLASS,

    /* Debugger */

    CORINFO_HELP_DBG_IS_JUST_MY_CODE,    // Check if this is "JustMyCode" and needs to be stepped through.

    /* Profiling enter/leave probe addresses */
    CORINFO_HELP_PROF_FCN_ENTER,        // record the entry to a method (caller)
    CORINFO_HELP_PROF_FCN_LEAVE,        // record the completion of current method (caller)
    CORINFO_HELP_PROF_FCN_TAILCALL,     // record the completionof current method through tailcall (caller)

    /* Miscellaneous */

    CORINFO_HELP_BBT_FCN_ENTER,         // record the entry to a method for collecting Tuning data

    CORINFO_HELP_PINVOKE_CALLI,         // Indirect pinvoke call
    CORINFO_HELP_TAILCALL,              // Perform a tail call
    

    CORINFO_HELP_GET_THREAD_FIELD_ADDR_PRIMITIVE,
    CORINFO_HELP_GET_THREAD_FIELD_ADDR_OBJREF,


    CORINFO_HELP_GET_THREAD,
    CORINFO_HELP_INIT_PINVOKE_FRAME,   // initialize an inlined PInvoke Frame for the JIT-compiler
    CORINFO_HELP_FAST_PINVOKE,          // fast PInvoke helper
    CORINFO_HELP_CHECK_PINVOKE_DOMAIN,  // check which domain the pinvoke call is in

    CORINFO_HELP_MEMSET,                // Init block of memory
    CORINFO_HELP_MEMCPY,                // Copy block of memory

    CORINFO_HELP_RUNTIMEHANDLE,         // determine a type/field/method handle at run-time
    CORINFO_HELP_VIRTUAL_FUNC_PTR,      // look up a virtual method at run-time

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
     *
     * Start of Machine-specific helpers. All new common JIT helpers
     * should be added before here
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#if !CPU_HAS_FP_SUPPORT
    //
    // Some architectures need helpers for FP support
    //
    // Keep these at the end of the enum
    //

    CORINFO_HELP_R4_NEG,
    CORINFO_HELP_R8_NEG,

    CORINFO_HELP_R4_ADD,
    CORINFO_HELP_R8_ADD,
    CORINFO_HELP_R4_SUB,
    CORINFO_HELP_R8_SUB,
    CORINFO_HELP_R4_MUL,
    CORINFO_HELP_R8_MUL,
    CORINFO_HELP_R4_DIV,
    CORINFO_HELP_R8_DIV,

    CORINFO_HELP_R4_EQ,
    CORINFO_HELP_R8_EQ,
    CORINFO_HELP_R4_NE,
    CORINFO_HELP_R8_NE,
    CORINFO_HELP_R4_LT,
    CORINFO_HELP_R8_LT,
    CORINFO_HELP_R4_LE,
    CORINFO_HELP_R8_LE,
    CORINFO_HELP_R4_GE,
    CORINFO_HELP_R8_GE,
    CORINFO_HELP_R4_GT,
    CORINFO_HELP_R8_GT,

    CORINFO_HELP_R8_TO_I4,
    CORINFO_HELP_R8_TO_I8,
    CORINFO_HELP_R8_TO_R4,

    CORINFO_HELP_R4_TO_I4,
    CORINFO_HELP_R4_TO_I8,
    CORINFO_HELP_R4_TO_R8,

    CORINFO_HELP_I4_TO_R4,
    CORINFO_HELP_I4_TO_R8,
    CORINFO_HELP_I8_TO_R4,
    CORINFO_HELP_I8_TO_R8,
    CORINFO_HELP_U4_TO_R4,
    CORINFO_HELP_U4_TO_R8,
    CORINFO_HELP_U8_TO_R4,
    CORINFO_HELP_U8_TO_R8,
#endif

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
     *
     * Don't add new JIT helpers here! Add them before the machine-specific
     * helpers.
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

    CORINFO_HELP_COUNT
};

// The enumeration is returned in 'getSig','getType', getArgType methods
enum CorInfoType
{
    CORINFO_TYPE_UNDEF           = 0x0,
    CORINFO_TYPE_VOID            = 0x1,
    CORINFO_TYPE_BOOL            = 0x2,
    CORINFO_TYPE_CHAR            = 0x3,
    CORINFO_TYPE_BYTE            = 0x4,
    CORINFO_TYPE_UBYTE           = 0x5,
    CORINFO_TYPE_SHORT           = 0x6,
    CORINFO_TYPE_USHORT          = 0x7,
    CORINFO_TYPE_INT             = 0x8,
    CORINFO_TYPE_UINT            = 0x9,
    CORINFO_TYPE_LONG            = 0xa,
    CORINFO_TYPE_ULONG           = 0xb,
    CORINFO_TYPE_NATIVEINT       = 0xc,
    CORINFO_TYPE_NATIVEUINT      = 0xd,
    CORINFO_TYPE_FLOAT           = 0xe,
    CORINFO_TYPE_DOUBLE          = 0xf,
    CORINFO_TYPE_STRING          = 0x10,         // Not used, should remove
    CORINFO_TYPE_PTR             = 0x11,
    CORINFO_TYPE_BYREF           = 0x12,
    CORINFO_TYPE_VALUECLASS      = 0x13,
    CORINFO_TYPE_CLASS           = 0x14,
    CORINFO_TYPE_REFANY          = 0x15,

    // CORINFO_TYPE_VAR is for a generic type variable.
    // Generic type variables only appear when the JIT is doing
    // verification (not NOT compilation) of generic code
    // for the EE, in which case we're running
    // the JIT in "import only" mode.

    CORINFO_TYPE_VAR             = 0x16,
    CORINFO_TYPE_COUNT,                         // number of jit types
};

enum CorInfoTypeWithMod
{
    CORINFO_TYPE_MASK            = 0x3F,        // lower 6 bits are type mask
    CORINFO_TYPE_MOD_PINNED      = 0x40,        // can be applied to CLASS, or BYREF to indiate pinned
};

inline CorInfoType strip(CorInfoTypeWithMod val) {
    return CorInfoType(val & CORINFO_TYPE_MASK);
}

// The enumeration is returned in 'getSig'

enum CorInfoCallConv
{
    // These correspond to CorCallingConvention

    CORINFO_CALLCONV_DEFAULT    = 0x0,
    CORINFO_CALLCONV_C          = 0x1,
    CORINFO_CALLCONV_STDCALL    = 0x2,
    CORINFO_CALLCONV_THISCALL   = 0x3,
    CORINFO_CALLCONV_FASTCALL   = 0x4,
    CORINFO_CALLCONV_VARARG     = 0x5,
    CORINFO_CALLCONV_FIELD      = 0x6,
    CORINFO_CALLCONV_LOCAL_SIG  = 0x7,
    CORINFO_CALLCONV_PROPERTY   = 0x8,
    CORINFO_CALLCONV_NATIVEVARARG = 0xb,    // used ONLY for 64bit PInvoke vararg calls

    CORINFO_CALLCONV_MASK       = 0x0f,     // Calling convention is bottom 4 bits
    CORINFO_CALLCONV_GENERIC    = 0x10,
    CORINFO_CALLCONV_HASTHIS    = 0x20,
    CORINFO_CALLCONV_EXPLICITTHIS=0x40,
    CORINFO_CALLCONV_PARAMTYPE  = 0x80,     // Passed last. Same as CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG
};

enum CorInfoUnmanagedCallConv
{
    // These correspond to CorUnmanagedCallingConvention

    CORINFO_UNMANAGED_CALLCONV_UNKNOWN,
    CORINFO_UNMANAGED_CALLCONV_C,
    CORINFO_UNMANAGED_CALLCONV_STDCALL,
    CORINFO_UNMANAGED_CALLCONV_THISCALL,
    CORINFO_UNMANAGED_CALLCONV_FASTCALL
};

// These are returned from getMethodOptions
enum CorInfoOptions
{
    CORINFO_OPT_INIT_LOCALS                 = 0x00000010, // zero initialize all variables

    CORINFO_GENERICS_CTXT_FROM_THIS         = 0x00000020, // is this shared generic code that access the generic context from the this pointer?  If so, then if the method has SEH then the 'this' pointer must always be reported and kept alive.
    CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG = 0x00000040, // is this shared generic code that access the generic context from the ParamTypeArg?  If so, then if the method has SEH then the 'ParamTypeArg' must always be reported and kept alive. Same as CORINFO_CALLCONV_PARAMTYPE
    CORINFO_GENERICS_CTXT_MASK              = (CORINFO_GENERICS_CTXT_FROM_THIS |
                                               CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG),
    CORINFO_GENERICS_CTXT_KEEP_ALIVE        = 0x00000080, // Keep the generics context alive throughout the method even if there is no explicit use, and report its location to the CLR

};

// These are potential return values for ICorFieldInfo::getFieldCategory
// If the JIT receives a category that it doesn't know about or doesn't
// care to optimize specially for, it should treat it as CORINFO_FIELDCATEGORY_UNKNOWN.
enum CorInfoFieldCategory
{
    CORINFO_FIELDCATEGORY_NORMAL,   // normal GC object
    CORINFO_FIELDCATEGORY_UNKNOWN,  // always call field get/set helpers
    CORINFO_FIELDCATEGORY_I1_I1,    // indirect access: fetch 1 byte
    CORINFO_FIELDCATEGORY_I2_I2,    // indirect access: fetch 2 bytes
    CORINFO_FIELDCATEGORY_I4_I4,    // indirect access: fetch 4 bytes
    CORINFO_FIELDCATEGORY_I8_I8,    // indirect access: fetch 8 bytes
    CORINFO_FIELDCATEGORY_BOOLEAN_BOOL, // boolean to 4-byte BOOL
    CORINFO_FIELDCATEGORY_CHAR_CHAR,// (Unicode) "char" to (ansi) CHAR
    CORINFO_FIELDCATEGORY_UI1_UI1,  // indirect access: fetch 1 byte
    CORINFO_FIELDCATEGORY_UI2_UI2,  // indirect access: fetch 2 bytes
    CORINFO_FIELDCATEGORY_UI4_UI4,  // indirect access: fetch 4 bytes
    CORINFO_FIELDCATEGORY_UI8_UI8,  // indirect access: fetch 8 bytes
};

// these are the attribute flags for fields and methods (getMethodAttribs)
enum CorInfoFlag
{
    // These values are an identical mapping from the resp.
    // access_flag bits
    CORINFO_FLG_PUBLIC                = 0x00000001,
    CORINFO_FLG_PRIVATE               = 0x00000002,
    CORINFO_FLG_PROTECTED             = 0x00000004,
    CORINFO_FLG_STATIC                = 0x00000008,
    CORINFO_FLG_FINAL                 = 0x00000010,
    CORINFO_FLG_SYNCH                 = 0x00000020,
    CORINFO_FLG_VIRTUAL               = 0x00000040,
//  CORINFO_FLG_AGILE                 = 0x00000080,
    CORINFO_FLG_NATIVE                = 0x00000100,
    CORINFO_FLG_NOTREMOTABLE          = 0x00000200,
    CORINFO_FLG_ABSTRACT              = 0x00000400,

    CORINFO_FLG_EnC                   = 0x00000800, // member was added by Edit'n'Continue

    // These are internal flags that can only be on methods
    CORINFO_FLG_IMPORT                = 0x00020000, // method is an imported symbol
    CORINFO_FLG_DELEGATE_INVOKE       = 0x00040000, // "Delegate
//  CORINFO_FLG_UNCHECKEDPINVOKE      = 0x00080000, // Is a P/Invoke call that requires no security checks
    CORINFO_FLG_PINVOKE               = 0x00080000, // Is a P/Invoke call
    CORINFO_FLG_SECURITYCHECK         = 0x00100000, // Is one of the security routines that does a stackwalk (e.g. Assert, Demand)
    CORINFO_FLG_NOGCCHECK             = 0x00200000, // This method is FCALL that has no GC check.  Don't put alone in loops
    CORINFO_FLG_INTRINSIC             = 0x00400000, // This method MAY have an intrinsic ID
    CORINFO_FLG_CONSTRUCTOR           = 0x00800000, // method is an instance or type initializer
    CORINFO_FLG_RUN_CCTOR             = 0x01000000, // This method must run the class cctor as it has PreciseInit semantics
    CORINFO_FLG_SHAREDINST            = 0x02000000, // the code for this method is shared between different generic instantiations
    CORINFO_FLG_NOSECURITYWRAP        = 0x04000000, // method requires no security checks

    // These are the valid bits that a jitcompiler can pass to setJitterMethodFlags for
    // non-native methods only; a jitcompiler can access these flags using getMethodFlags.
    CORINFO_FLG_JITTERFLAGSMASK       = 0xF0000000,

    CORINFO_FLG_DONT_INLINE           = 0x10000000, // The method should not be inlined
//  CORINFO_FLG_INLINED               = 0x20000000,
//  CORINFO_FLG_NOSIDEEFFECTS         = 0x40000000,

    // These are internal flags that can only be on Fields
    CORINFO_FLG_HELPER                = 0x00010000, // field accessed via ordinary helper calls
    CORINFO_FLG_TLS                   = 0x00020000, // This variable accesses thread local store.
    CORINFO_FLG_SHARED_HELPER         = 0x00040000, // field is access via optimized shared helper
    CORINFO_FLG_STATIC_IN_HEAP        = 0x00080000, // This static field is in the GC heap as a boxed object
    CORINFO_FLG_GENERICS_STATIC       = 0x00100000, // This static field is being accessed from shared code. Use ICorClassInfo::getSharedStaticBaseHelper() to access it
    CORINFO_FLG_UNMANAGED             = 0x00200000, // is this an unmanaged value class?
    CORINFO_FLG_SAFESTATIC_BYREF_RETURN = 0x00800000, // Field can be returned safely (has GC heap lifetime)
    CORINFO_FLG_NONVERIFIABLYOVERLAPS   = 0x01000000, // Field overlaps a different field

    // These are internal flags that can only be on Classes
    CORINFO_FLG_VALUECLASS            = 0x00010000, // is the class a value class
    CORINFO_FLG_INITIALIZED           = 0x00020000, // The class has been cctor'ed
    CORINFO_FLG_VAROBJSIZE            = 0x00040000, // the object size varies depending of constructor args
    CORINFO_FLG_ARRAY                 = 0x00080000, // class is an array class (initialized differently)
    CORINFO_FLG_ALIGN8_HINT           = 0x00100000, // align to 8 bytes if posible
    CORINFO_FLG_INTERFACE             = 0x00200000, // it is an interface
    CORINFO_FLG_CONTEXTFUL            = 0x00400000, // is this a contextful class?
    CORINFO_FLG_CONTAINS_GC_PTR       = 0x01000000, // does the class contain a gc ptr ?
    CORINFO_FLG_DELEGATE              = 0x02000000, // is this a subclass of delegate or multicast delegate ?
    CORINFO_FLG_MARSHAL_BYREF         = 0x04000000, // is this a subclass of MarshalByRef ?
    CORINFO_FLG_CONTAINS_STACK_PTR    = 0x08000000, // This class has a stack pointer inside it
    CORINFO_FLG_NEEDS_INIT            = 0x10000000, // Access to this type should check ICorClassInfo::initClass() and then add JIT hooks for the cctor.
    CORINFO_FLG_BEFOREFIELDINIT       = 0x20000000, // Additional flexibility for when to run .cctor
    CORINFO_FLG_GENERIC_TYPE_VARIABLE = 0x40000000, // This is really a handle for a variable type
    CORINFO_FLG_UNSAFE_VALUECLASS     = 0x80000000, // Unsafe (C++'s /GS) value type
};

// Flags computed by a runtime compiler
enum CorInfoMethodRuntimeFlags
{
    CORINFO_FLG_BAD_INLINEE         = 0x00000001, // The method is not suitable for inlining
    CORINFO_FLG_VERIFIABLE          = 0x00000002, // The method has verifiable code
    CORINFO_FLG_UNVERIFIABLE        = 0x00000004, // The method has unverifiable code
};


enum CORINFO_ACCESS_FLAGS
{
    CORINFO_ACCESS_ANY        = 0x0000, // Normal access
    CORINFO_ACCESS_THIS       = 0x0001, // Accessed via the this reference
    CORINFO_ACCESS_UNWRAP     = 0x0002, // Accessed via an unwrap reference

    CORINFO_ACCESS_NONNULL    = 0x0004, // Instance is guaranteed non-null

    CORINFO_ACCESS_DIRECT     = 0x0008, // Set by ngen if it already checked that the method can be called directly
                                        // using ICorMethodInfo::canSkipMethodPreparation

    CORINFO_ACCESS_STABLE     = 0x0010, // Set by ngen to request call via stable entrypoint
};

// These are the flags set on an CORINFO_EH_CLAUSE
enum CORINFO_EH_CLAUSE_FLAGS
{
    CORINFO_EH_CLAUSE_NONE    = 0,
    CORINFO_EH_CLAUSE_FILTER  = 0x0001, // If this bit is on, then this EH entry is for a filter
    CORINFO_EH_CLAUSE_FINALLY = 0x0002, // This clause is a finally clause
    CORINFO_EH_CLAUSE_FAULT   = 0x0004, // This clause is a fault   clause
};

// This enumeration is passed to InternalThrow
enum CorInfoException
{
    CORINFO_NullReferenceException,
    CORINFO_DivideByZeroException,
    CORINFO_InvalidCastException,
    CORINFO_IndexOutOfRangeException,
    CORINFO_OverflowException,
    CORINFO_SynchronizationLockException,
    CORINFO_ArrayTypeMismatchException,
    CORINFO_RankException,
    CORINFO_ArgumentNullException,
    CORINFO_ArgumentException,
    CORINFO_Exception_Count,
};


// This enumeration is returned by getIntrinsicID. Methods corresponding to
// these values will have "well-known" specified behavior. Calls to these
// methods could be replaced with inlined code corresponding to the
// specified behavior (without having to examine the IL beforehand).

enum CorInfoIntrinsics
{
    CORINFO_INTRINSIC_Sin,
    CORINFO_INTRINSIC_Cos,
    CORINFO_INTRINSIC_Sqrt,
    CORINFO_INTRINSIC_Abs,
    CORINFO_INTRINSIC_Round,
    CORINFO_INTRINSIC_GetChar,              // fetch character out of string
    CORINFO_INTRINSIC_Array_GetDimLength,   // Get number of elements in a given dimension of an array
    CORINFO_INTRINSIC_Array_GetLengthTotal, // Get total number of elements in an array
    CORINFO_INTRINSIC_Array_Get,            // Get the value of an element in an array
    CORINFO_INTRINSIC_Array_Address,        // Get the address of an element in an array
    CORINFO_INTRINSIC_Array_Set,            // Set the value of an element in an array
    CORINFO_INTRINSIC_StringGetChar,        // fetch character out of string
    CORINFO_INTRINSIC_StringLength,         // get the length
    CORINFO_INTRINSIC_InitializeArray,      // initialize an array from static data
    CORINFO_INTRINSIC_Type_TypeHandle_get,
    CORINFO_INTRINSIC_GetClassFromHandle,
    CORINFO_INTRINSIC_Object_GetType,

    CORINFO_INTRINSIC_Count,
    CORINFO_INTRINSIC_Illegal = -1,         // Not a true intrinsic,
};

// Can a value be accessed directly from JITed code.
enum InfoAccessType
{
    IAT_VALUE,      // The info value is directly available
    IAT_PVALUE,     // The value needs to be accessed via an       indirection
    IAT_PPVALUE     // The value needs to be accessed via a double indirection
};

// Can a value be accessed directly from JITed code.
enum InfoAccessModule
{
    IAM_UNKNOWN_MODULE,  // The target module for the info value is not known
    IAM_CURRENT_MODULE,  // The target module for the info value is the current module
    IAM_EXTERNAL_MODULE, // The target module for the info value is an external module
};

enum CorInfoGCType
{
    TYPE_GC_NONE,   // no embedded objectrefs
    TYPE_GC_REF,    // Is an object ref
    TYPE_GC_BYREF,  // Is an interior pointer - promote it but don't scan it
    TYPE_GC_OTHER   // requires type-specific treatment
};

enum CorInfoClassId
{
    CLASSID_SYSTEM_OBJECT,
    CLASSID_TYPED_BYREF,
    CLASSID_TYPE_HANDLE,
    CLASSID_FIELD_HANDLE,
    CLASSID_METHOD_HANDLE,
    CLASSID_STRING,
    CLASSID_ARGUMENT_HANDLE,
};

enum CorInfoFieldAccess
{
    CORINFO_GET,
    CORINFO_SET,
    CORINFO_ADDRESS,
};

enum CorInfoInline
{
    INLINE_PASS                 = 0,    // Inlining OK

    // failures are negative
    INLINE_FAIL                 = -1,   // Inlining not OK for this case only
    INLINE_NEVER                = -2,   // This method should never be inlined, regardless of context
};

enum CorInfoInlineRestrictions
{
    INLINE_RESPECT_BOUNDARY = 0x00000001, // You can inline if there are no calls from the method being inlined
    INLINE_NO_CALLEE_LDSTR  = 0x00000002, // You can inline only if you guarantee that if inlinee does an ldstr
                                          // inlinee's module will never see that string (by any means).
                                          // This is due to how we implement the NoStringInterningAttribute
                                          // (by reusing the fixup table).
    INLINE_SAME_THIS        = 0x00000004, // You can inline only if the callee is on the same this reference as caller
};

enum CorInfoIsCallAllowedResult
{
    CORINFO_CALL_ALLOWED = 0,           // Call allowed
    CORINFO_CALL_ILLEGAL = 1,           // Call not allowed
    CORINFO_CALL_RUNTIME_CHECK = 2,     // Ask at runtime whether to allow the call or not
};

enum CORINFO_CALL_ALLOWED_FLAGS
{
    CORINFO_CALL_ALLOWED_NONE       = 0x0000, // Invalid value
    CORINFO_CALL_ALLOWED_BYSECURITY = 0x0001, // Callout to security permission  
};

struct CORINFO_CALL_ALLOWED_INFO
{
    CORINFO_CALL_ALLOWED_FLAGS mask;        // Which callout(s) are needed?
    void *                     context;     // The context for the callout?
};

enum CorInfoCanSkipVerificationResult
{
    CORINFO_VERIFICATION_CANNOT_SKIP    = 0,    // Cannot skip verification during jit time.
    CORINFO_VERIFICATION_CAN_SKIP       = 1,    // Can skip verification during jit time.
    CORINFO_VERIFICATION_RUNTIME_CHECK  = 2,    // Skip verification during jit time,
                                                //     but need to insert a callout to the VM to ask during runtime 
                                                //     whether to skip verification or not.
};

// Reason codes for making indirect calls
#define INDIRECT_CALL_REASONS() \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_UNKNOWN) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_EXOTIC) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_PINVOKE) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_GENERIC) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_NO_CODE) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_FIXUPS) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_STUB) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_REMOTING) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_CER) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_CCTOR_TRIGGER) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_RESTORE_METHOD) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_RESTORE_FIRST_CALL) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_RESTORE_VALUE_TYPE) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_RESTORE) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_CANT_PATCH) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_PROFILING) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_DISABLED) \
    INDIRECT_CALL_REASON_FUNC(CORINFO_INDIRECT_CALL_OTHER_LOADER_MODULE) \

enum CorInfoIndirectCallReason
{
    #undef INDIRECT_CALL_REASON_FUNC
    #define INDIRECT_CALL_REASON_FUNC(x) x,
    INDIRECT_CALL_REASONS()

    #undef INDIRECT_CALL_REASON_FUNC

    CORINFO_INDIRECT_CALL_COUNT
};

// This is for use when the JIT is compiling an instantiation
// of generic code.  The JIT needs to know if the generic code itself
// (which can be verified once and for all independently of the
// instantiations) passed verification.
enum CorInfoInstantiationVerification
{
    // The method is NOT a concrete instantiation (eg. List<int>.Add()) of a method 
    // in a generic class or a generic method. It is either the typical instantiation 
    // (eg. List<T>.Add()) or entirely non-generic.
    INSTVER_NOT_INSTANTIATION           = 0,

    // The method is an instantiation of a method in a generic class or a generic method, 
    // and the generic class was successfully verified
    INSTVER_GENERIC_PASSED_VERIFICATION = 1,

    // The method is an instantiation of a method in a generic class or a generic method, 
    // and the generic class failed verification
    INSTVER_GENERIC_FAILED_VERIFICATION = 2,
};


inline bool dontInline(CorInfoInline val) {
    return(val < 0);
}

// Cookie types consumed by the code generator (these are opaque values
// not inspected by the code generator):

typedef struct CORINFO_ASSEMBLY_STRUCT_*    CORINFO_ASSEMBLY_HANDLE;
typedef struct CORINFO_MODULE_STRUCT_*      CORINFO_MODULE_HANDLE;
typedef struct CORINFO_DEPENDENCY_STRUCT_*  CORINFO_DEPENDENCY_HANDLE;
typedef struct CORINFO_CLASS_STRUCT_*       CORINFO_CLASS_HANDLE;
typedef struct CORINFO_METHOD_STRUCT_*      CORINFO_METHOD_HANDLE;
typedef struct CORINFO_FIELD_STRUCT_*       CORINFO_FIELD_HANDLE;
typedef struct CORINFO_ARG_LIST_STRUCT_*    CORINFO_ARG_LIST_HANDLE;    // represents a list of argument types
typedef struct CORINFO_SIG_STRUCT_*         CORINFO_SIG_HANDLE;         // represents the whole list
typedef struct CORINFO_JUST_MY_CODE_HANDLE_*CORINFO_JUST_MY_CODE_HANDLE;
typedef struct CORINFO_PROFILING_STRUCT_*   CORINFO_PROFILING_HANDLE;   // a handle guaranteed to be unique per process
typedef DWORD*                              CORINFO_SHAREDMODULEID_HANDLE;
typedef struct CORINFO_GENERIC_STRUCT_*     CORINFO_GENERIC_HANDLE;     // a generic handle (could be any of the above)

// what is actually passed on the varargs call
typedef struct CORINFO_VarArgInfo *         CORINFO_VARARGS_HANDLE;

// Generic tokens are resolved with respect to a context, which is usually the method
// being compiled. The CORINFO_CONTEXT_HANDLE indicates which exact instantiation
// (or the open instantiation) is being referred to.
// CORINFO_CONTEXT_HANDLE is more tightly scoped than CORINFO_MODULE_HANDLE. For cases 
// where the exact instantiation does not matter, CORINFO_MODULE_HANDLE is used.
typedef CORINFO_METHOD_HANDLE               CORINFO_CONTEXT_HANDLE;

// Bit-twiddling of contexts assumes word-alignment of method handles and type handles
// If this ever changes, some other encoding will be needed
enum CorInfoContextFlags
{
    CORINFO_CONTEXTFLAGS_METHOD = 0x00, // CORINFO_CONTEXT_HANDLE is really a CORINFO_METHOD_HANDLE
    CORINFO_CONTEXTFLAGS_CLASS  = 0x01, // CORINFO_CONTEXT_HANDLE is really a CORINFO_CLASS_HANDLE
    CORINFO_CONTEXTFLAGS_MASK   = 0x01
};

#define MAKE_CLASSCONTEXT(c)  (CORINFO_CONTEXT_HANDLE((size_t) (c) | CORINFO_CONTEXTFLAGS_CLASS))
#define MAKE_METHODCONTEXT(m) (CORINFO_CONTEXT_HANDLE((size_t) (m) | CORINFO_CONTEXTFLAGS_METHOD))

enum CorInfoSigInfoFlags
{
    CORINFO_SIGFLAG_IS_LOCAL_SIG = 0x01,
    CORINFO_SIGFLAG_IL_STUB      = 0x02,
};

struct CORINFO_SIG_INST
{
    unsigned                classInstCount;
    CORINFO_CLASS_HANDLE *  classInst; // (representative, not exact) instantiation for class type variables in signature
    unsigned                methInstCount;
    CORINFO_CLASS_HANDLE *  methInst; // (representative, not exact) instantiation for method type variables in signature
};

struct CORINFO_SIG_INFO
{
    CorInfoCallConv         callConv;
    CORINFO_CLASS_HANDLE    retTypeClass;   // if the return type is a value class, this is its handle (enums are normalized)
    CORINFO_CLASS_HANDLE    retTypeSigClass;// returns the value class as it is in the sig (enums are not converted to primitives)
    CorInfoType             retType : 8;
    unsigned                flags   : 8;    // used by IL stubs code
    unsigned                numArgs : 16;
    struct CORINFO_SIG_INST sigInst;  // information about how type variables are being instantiated in generic code
    CORINFO_ARG_LIST_HANDLE args;
    CORINFO_SIG_HANDLE      sig;
    CORINFO_MODULE_HANDLE   scope;          // passed to getArgClass
    mdToken                 token;

    bool                hasRetBuffArg()     { return retType == CORINFO_TYPE_VALUECLASS || retType == CORINFO_TYPE_REFANY; }
    CorInfoCallConv     getCallConv()       { return CorInfoCallConv((callConv & CORINFO_CALLCONV_MASK)); }
    bool                hasThis()           { return ((callConv & CORINFO_CALLCONV_HASTHIS) != 0); }
    unsigned            totalILArgs()       { return (numArgs + hasThis()); }
    unsigned            totalNativeArgs()   { return (totalILArgs() + hasRetBuffArg()); }
    bool                isVarArg()          { return ((getCallConv() == CORINFO_CALLCONV_VARARG) || (getCallConv() == CORINFO_CALLCONV_NATIVEVARARG)); }
    bool                hasTypeArg()        { return ((callConv & CORINFO_CALLCONV_PARAMTYPE) != 0); }
};

struct CORINFO_METHOD_INFO
{
    CORINFO_METHOD_HANDLE       ftn;
    CORINFO_MODULE_HANDLE       scope;
    BYTE *                      ILCode;
    unsigned                    ILCodeSize;
    unsigned short              maxStack;
    unsigned short              EHcount;
    CorInfoOptions              options;
    CORINFO_SIG_INFO            args;
    CORINFO_SIG_INFO            locals;
};

//----------------------------------------------------------------------------
// Looking up handles and addresses.
//
// When the JIT requests a handle, the EE may direct the JIT that it must
// access the handle in a variety of ways.  These are packed as
//    CORINFO_CONST_LOOKUP
// or CORINFO_LOOKUP (contains either a CORINFO_CONST_LOOKUP or a CORINFO_RUNTIME_LOOKUP)
//
// Constant Lookups v. Runtime Lookups (i.e. when will Runtime Lookups be generated?)
// -----------------------------------------------------------------------------------
//
// CORINFO_LOOKUP_KIND is part of the result type of embedGenericHandle,
// getVirtualCallInfo and any other functions that may require a
// runtime lookup when compiling shared generic code.
//
// CORINFO_LOOKUP_KIND indicates whether a particular token in the instruction stream can be:
// (a) Mapped to a handle (type, field or method) at compile-time (!needsRuntimeLookup)
// (b) Must be looked up at run-time, and if so which runtime lookup technique should be used (see below)
//
// If the JIT or EE does not support code sharing for generic code, then
// all CORINFO_LOOKUP results will be "constant lookups", i.e.
// the needsRuntimeLookup of CORINFO_LOOKUP.lookupKind.needsRuntimeLookup
// will be false.
//
// Constant Lookups
// ----------------
//
// Constant Lookups are either:
//     IAT_VALUE: immediate (relocatable) values,
//     IAT_PVALUE: immediate values access via an indirection through an immediate (relocatable) address
//     IAT_PPVALUE: immediate values access via a double indirection through an immediate (relocatable) address
//
// Runtime Lookups
// ---------------
//
// CORINFO_LOOKUP_KIND is part of the result type of embedGenericHandle,
// getVirtualCallInfo and any other functions that may require a
// runtime lookup when compiling shared generic code.
//
// CORINFO_LOOKUP_KIND indicates whether a particular token in the instruction stream can be:
// (a) Mapped to a handle (type, field or method) at compile-time (!needsRuntimeLookup)
// (b) Must be looked up at run-time using the class dictionary
//     stored in the vtable of the this pointer (needsRuntimeLookup && THISOBJ)
// (c) Must be looked up at run-time using the method dictionary
//     stored in the method descriptor parameter passed to a generic
//     method (needsRuntimeLookup && METHODPARAM)
// (d) Must be looked up at run-time using the class dictionary stored
//     in the vtable parameter passed to a method in a generic
//     struct (needsRuntimeLookup && CLASSPARAM)

struct CORINFO_CONST_LOOKUP
{
    union
    {
        CORINFO_GENERIC_HANDLE  handle;
        void *                  addr;
    };
    InfoAccessType              accessType;
    InfoAccessModule            accessModule;
};

enum CORINFO_RUNTIME_LOOKUP_KIND
{
    CORINFO_LOOKUP_THISOBJ,
    CORINFO_LOOKUP_METHODPARAM,
    CORINFO_LOOKUP_CLASSPARAM,
};

struct CORINFO_LOOKUP_KIND
{
    bool                        needsRuntimeLookup;
    CORINFO_RUNTIME_LOOKUP_KIND runtimeLookupKind;
} ;

// CORINFO_RUNTIME_LOOKUP indicates the details of the runtime lookup
// operation to be performed.
//
// CORINFO_MAXINDIRECTIONS is the maximum number of
// indirections used by runtime lookups.
// This accounts for up to 2 indirections to get at a dictionary followed by a possible spill slot
#define CORINFO_MAXINDIRECTIONS 4
#define CORINFO_USEHELPER ((WORD) 0xffff)

struct CORINFO_RUNTIME_LOOKUP
{
    // This are the tokens you must pass back to the runtime lookup helper
    DWORD                   token1;
    DWORD                   token2;

    // Here is the helper you must call.  At the moment it is always
    // CORINFO_HELP_RUNTIMEHANDLE.
    CorInfoHelpFunc         helper;

    // Number of indirections to get there
    // CORINFO_USEHELPER = don't know how to get it, so use helper function at run-time instead
    // 0 = use the this pointer itself (e.g. token is C<!0> inside code in sealed class C)
    //     or method desc itself (e.g. token is method void M::mymeth<!!0>() inside code in M::mymeth)
    // Otherwise, follow each byte-offset stored in the "offsets[]" array (may be negative)
    WORD                    indirections;

    // If set, test for null and branch to helper if null
    WORD                    testForNull;

    WORD                    offsets[CORINFO_MAXINDIRECTIONS];
} ;

// Result of calling embedGenericHandle
struct CORINFO_LOOKUP
{
    CORINFO_LOOKUP_KIND     lookupKind;

    // If kind.needsRuntimeLookup then this indicates how to do the lookup
    CORINFO_RUNTIME_LOOKUP  runtimeLookup;

    // If the handle is obtained at compile-time, then this handle is the "exact" handle (class, method, or field)
    // Otherwise, it's a representative...  If accessType is
    //     IAT_VALUE --> "handle" stores the real handle or "addr " stores the computed address
    //     IAT_PVALUE --> "addr" stores a pointer to a location which will hold the real handle
    //     IAT_PPVALUE --> "addr" stores a double indirection to a location which will hold the real handle
    CORINFO_CONST_LOOKUP    constLookup;
};

// Flags that are combined with a type token in calls to embedGenericHandle, findMethod, findClass and CORINFO_HELP_RUNTIMEHANDLE
//
// ARRAY is used in conjunction with mdtTypeSpec tokens to construct SZARRAY types for newarr.
//
// ALLOWINSTPARAM is used in conjunction with mdtMemberRef, mdtMethodSpec
// and mdtMethodDef tokens to indicate that the caller can deal with methods
// that require instantiation parameters (at present this is shared-code
// generic methods and shared-code instance methods in generic structs).
//
// If not set (the default), then you'll get a specialized stub instead,
// which is required for LDFTN and also by ngen as it can't assume
// anything about the implementation of generic code.
//
// ENTRYPOINT is used in conjunction with mdtMemberRef, mdtMethodSpec
// and mdtMethodDef tokens to return the entry point instead of the
// method descriptor.
//
enum CORINFO_ANNOTATION
{
    CORINFO_ANNOT_NONE                  = 0x00000000,

    // These flags are used only on TypeDef/Ref/Spec tokens
    CORINFO_ANNOT_ARRAY                 = 0x40000000,
    CORINFO_ANNOT_PERMITUNINSTDEFORREF  = 0x80000000,

    // These flags are used only on MethodDef/MemberRef/MethodSpec tokens
    CORINFO_ANNOT_ENTRYPOINT            = 0x40000000,
    CORINFO_ANNOT_ALLOWINSTPARAM        = 0x80000000,
    CORINFO_ANNOT_DISPATCH_STUBADDR     = 0xC0000000,

    CORINFO_ANNOT_MASK                  = 0xC0000000,
};


//----------------------------------------------------------------------------
// Embedding type, method and field handles (for "ldtoken" or to pass back to helpers)

// Result of calling embedGenericHandle
struct CORINFO_GENERICHANDLE_RESULT
{
    CORINFO_LOOKUP          lookup;

    // compileTimeHandle is guaranteed to be either NULL or a handle that is usable during compile time.
    // It must not be embedded in the code because it might not be valid at run-time.
    CORINFO_GENERIC_HANDLE  compileTimeHandle;

};

//----------------------------------------------------------------------------
// getCallInfo and CORINFO_CALL_INFO: The EE instructs the JIT about how to make a call
//
// callKind
// --------
//
// CORINFO_CALL :
//   Indicates that the JIT can use getFunctionEntryPoint to make a call,
//   i.e. there is nothing abnormal about the call.  The JITs know what to do if they get this.
//   Except in the case of constraint calls (see below), [targetMethodHandle] will hold
//   the CORINFO_METHOD_HANDLE that a call to findMethod would
//   have returned.
//   This flag may be combined with nullInstanceCheck=TRUE for uses of callvirt on methods that can
//   be resolved at compile-time (non-virtual, final or sealed).
//
// CORINFO_CALL_CODE_POINTER (shared generic code only) :
//   Indicates that the JIT should do an indirect call to the entrypoint given by address, which may be specified
//   as a runtime lookup by CORINFO_CALL_INFO::codePointerLookup.
//   [targetMethodHandle] will not hold a valid value.
//   This flag may be combined with nullInstanceCheck=TRUE for uses of callvirt on methods whose target method can
//   be resolved at compile-time but whose instantiation can be resolved only through runtime lookup.
//
// CORINFO_VIRTUALCALL_STUB (interface calls) :
//   Indicates that the EE supports "stub dispatch" and request the JIT to make a
//   "stub dispatch" call (an indirect call through CORINFO_CALL_INFO::stubLookup,
//   similar to CORINFO_CALL_CODE_POINTER).
//   "Stub dispatch" is a specialized calling sequence (that may require use of NOPs)
//   which allow the runtime to determine the call-site after the call has been dispatched.
//   If the call is too complex for the JIT (e.g. because
//   fetching the dispatch stub requires a runtime lookup, i.e. lookupKind.requiresRuntimeLookup
//   is set) then the JIT is allowed to implement the call as if it were CORINFO_VIRTUALCALL_LDVIRTFTN
//   [targetMethodHandle] will hold the CORINFO_METHOD_HANDLE that a call to findMethod would
//   have returned.
//   This flag is always accompanied by nullInstanceCheck=TRUE.
//
// CORINFO_VIRTUALCALL_LDVIRTFTN (virtual generic methods) :
//   Indicates that the EE provides no way to implement the call directly and
//   that the JIT should use a LDVIRTFTN sequence (as implemented by CORINFO_HELP_VIRTUAL_FUNC_PTR)
//   followed by an indirect call.
//   [targetMethodHandle] will hold the CORINFO_METHOD_HANDLE that a call to findMethod would
//   have returned.
//   This flag is always accompanied by nullInstanceCheck=TRUE though typically the null check will
//   be implicit in the access through the instance pointer.
//
//  CORINFO_VIRTUALCALL_VTABLE (regular virtual methods) :
//   Indicates that the EE supports vtable dispatch and that the JIT should use getVTableOffset etc.
//   to implement the call.
//   [targetMethodHandle] will hold the CORINFO_METHOD_HANDLE that a call to findMethod would
//   have returned.
//   This flag is always accompanied by nullInstanceCheck=TRUE though typically the null check will
//   be implicit in the access through the instance pointer.
//   On x64 platforms the thisInSecretRegister flag might be set. If so it indicates the callsite
//   should load the instance into r11 (in addition to rcx or rdx) before making the call.
//
// thisTransform and constraint calls
// ----------------------------------
//
// For evertyhing besides "constrained." calls "thisTransform" is set to
// CORINFO_NO_THIS_TRANSFORM.
//
// For "constrained." calls the EE attempts to resolve the call at compile
// time to a more specific method, or (shared generic code only) to a runtime lookup
// for a code pointer for the more specific method.
//
// In order to permit this, the "this" pointer supplied for a "constrained." call
// is a byref to an arbitrary type (see the IL spec). The "thisTransform" field
// will indicate how the JIT must transform the "this" pointer in order
// to be able to call the resolved method:
//
//  CORINFO_NO_THIS_TRANSFORM --> Leave it as a byref to an unboxed value type
//  CORINFO_BOX_THIS          --> Box it to produce an object
//  CORINFO_DEREF_THIS        --> Deref the byref to get an object reference
//
// In addition, the "kind" field will be set as follows for constraint calls:

//    CORINFO_CALL              --> the call was resolved at compile time, and
//                                  can be compiled like a normal call.
//    CORINFO_CALL_CODE_POINTER --> the call was resolved, but the target address will be
//                                  computed at runtime.  Only returned for shared generic code.
//    CORINFO_VIRTUALCALL_STUB,
//    CORINFO_VIRTUALCALL_LDVIRTFTN,
//    CORINFO_VIRTUALCALL_VTABLE   --> usual values indicating that a virtual call must be made

enum CORINFO_CALL_KIND
{
    CORINFO_CALL,
    CORINFO_CALL_CODE_POINTER,
    CORINFO_VIRTUALCALL_RESOLVED,
    CORINFO_VIRTUALCALL_STUB,
    CORINFO_VIRTUALCALL_LDVIRTFTN,
    CORINFO_VIRTUALCALL_VTABLE
};



enum CORINFO_THIS_TRANSFORM
{
    CORINFO_NO_THIS_TRANSFORM,
    CORINFO_BOX_THIS,
    CORINFO_DEREF_THIS
};

enum CORINFO_CALLINFO_FLAGS
{
    CORINFO_CALLINFO_ALLOWINSTPARAM = 0x0001,   // Can the compiler generate code to pass an instantiation parameters? Simple compilers should not use this flag
    CORINFO_CALLINFO_CALLVIRT       = 0x0002,   // Is it a virtual call?
    CORINFO_CALLINFO_KINDONLY       = 0x0004,   // This is set to only query the kind of call to perform, without getting any other information
};

struct CORINFO_CALL_INFO
{
    CORINFO_CALL_KIND       kind;
    BOOL                    nullInstanceCheck;

    // See above section on constraintCalls to understand when these are set to unusual values.
    CORINFO_THIS_TRANSFORM  thisTransform;
    CORINFO_METHOD_HANDLE   targetMethodHandle;

    // If kind.CORINFO_VIRTUALCALL_STUB then stubLookup will be set.
    // If kind.CORINFO_CALL_CODE_POINTER then entryPointLookup will be set.
    // If kind.CORINFO_VIRTUALCALL_VTABLE then thisInSecretRegister will be set. (x64 only)
    union
    {
        CORINFO_LOOKUP      stubLookup;

        CORINFO_LOOKUP      codePointerLookup;

    };
};

// Depending on different opcodes, we might allow/disallow certain types of tokens.
// This enum is used for JIT to tell EE where this token comes from.
// EE implements its policy in CheckTokenKind() method.
enum CorInfoTokenKind
{
    CORINFO_TOKENKIND_Default,  // token comes from other opcodes
    CORINFO_TOKENKIND_Ldtoken,  // token comes from CEE_LDTOKEN
    CORINFO_TOKENKIND_Casting   // token comes from CEE_CASTCLASS or CEE_ISINST
};

//----------------------------------------------------------------------------
// Exception handling

struct CORINFO_EH_CLAUSE
{
    CORINFO_EH_CLAUSE_FLAGS     Flags;
    DWORD                       TryOffset;
    DWORD                       TryLength;
    DWORD                       HandlerOffset;
    DWORD                       HandlerLength;
    union
    {
        DWORD                   ClassToken;       // use for type-based exception handlers
        DWORD                   FilterOffset;     // use for filter-based exception handlers (COR_ILEXCEPTION_FILTER is set)
    };
};

enum CORINFO_OS
{
    CORINFO_WIN9x,
    CORINFO_WINNT,
    CORINFO_WINCE,
    CORINFO_PAL,
};

struct CORINFO_CPU
{
    DWORD           dwCPUType;
    DWORD           dwFeatures;
    DWORD           dwExtendedFeatures;
};

// For some highly optimized paths, the JIT must generate code that directly
// manipulates internal EE data structures. The getEEInfo() helper returns
// this structure containing the needed offsets and values.
struct CORINFO_EE_INFO
{
    // Information about the InlinedCallFrame structure layout
    struct InlinedCallFrameInfo
    {
        // Size of the Frame structure
        unsigned    size;

        unsigned    offsetOfGSCookie;
        unsigned    offsetOfFrameVptr;
        unsigned    offsetOfFrameLink;
        unsigned    offsetOfCallSiteSP;
        unsigned    offsetOfCalleeSavedRegisters;
        unsigned    offsetOfCalleeSavedEbp;
        unsigned    offsetOfCallTarget;
        unsigned    offsetOfReturnAddress;
    }
    inlinedCallFrameInfo;
   
    // Details about NDirectMethodFrameGeneric
    unsigned    sizeOfNDirectFrame;
    unsigned    sizeOfNDirectNegSpace;
    unsigned    offsetOfTransitionFrameDatum;

    // Offsets into the Thread structure
    unsigned    offsetOfThreadFrame;            // offset of the current Frame
    unsigned    offsetOfGCState;                // offset of the preemptive/cooperative state of the Thread

    // Offsets into the methodtable
    unsigned    offsetOfEEClass;

    // Offsets into the EEClass
    unsigned    offsetOfInterfaceTable;

    // Delegate offsets
    unsigned    offsetOfDelegateInstance;
    unsigned    offsetOfDelegateFirstTarget;

    // Remoting offsets
    unsigned    offsetOfTransparentProxyRP;
    unsigned    offsetOfRealProxyServer;

    CORINFO_OS  osType;
    unsigned    osMajor;
    unsigned    osMinor;
    unsigned    osBuild;
};

// This is used to indicate that a finally has been called 
// "locally" by the try block
enum { LCL_FINALLY_MARK = 0xFC }; // FC = "Finally Call"

/**********************************************************************************
 * The following is the internal structure of an object that the compiler knows about
 * when it generates code
 **********************************************************************************/
#include <pshpack4.h>

#define CORINFO_PAGE_SIZE   0x1000                           // the page size on the machine

#define MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT ((32*1024)-1)   // when generating JIT code

typedef void* CORINFO_MethodPtr;            // a generic method pointer

struct CORINFO_Object
{
    CORINFO_MethodPtr      *methTable;      // the vtable for the object
};

struct CORINFO_String : public CORINFO_Object
{
    unsigned                buffLen;
    unsigned                stringLen;
    const wchar_t           chars[1];       // actually of variable size
};

struct CORINFO_Array : public CORINFO_Object
{
    unsigned                length;


    union
    {
        __int8              i1Elems[1];    // actually of variable size
        unsigned __int8     u1Elems[1];
        __int16             i2Elems[1];
        unsigned __int16    u2Elems[1];
        __int32             i4Elems[1];
        unsigned __int32    u4Elems[1];
        float               r4Elems[1];
    };
};

#include <pshpack4.h>
struct CORINFO_Array8 : public CORINFO_Object
{
    unsigned                length;

    union
    {
        double              r8Elems[1];
        __int64             i8Elems[1];
        unsigned __int64    u8Elems[1];
    };
};

#include <poppack.h>

struct CORINFO_RefArray : public CORINFO_Object
{
    unsigned                length;
    CORINFO_CLASS_HANDLE    cls;


    CORINFO_Object*         refElems[1];    // actually of variable size;
};

struct CORINFO_RefAny
{
    void                      * dataPtr;
    CORINFO_CLASS_HANDLE        type;
};

// The jit assumes the CORINFO_VARARGS_HANDLE is a pointer to a subclass of this
struct CORINFO_VarArgInfo
{
    unsigned                argBytes;       // number of bytes the arguments take up.
                                            // (The CORINFO_VARARGS_HANDLE counts as an arg)
};

#include <poppack.h>

/* data to optimize delegate construction */
struct DelegateCtorArgs
{
    void * pMethod;
    void * pArg3;
    void * pArg4;
    void * pArg5;
};

// use offsetof to get the offset of the fields above
#include <stddef.h> // offsetof
#ifndef offsetof
#define offsetof(s,m)   ((size_t)&(((s *)0)->m))
#endif

// Guard-stack cookie for preventing against stack buffer overruns
typedef SIZE_T GSCookie;


/**********************************************************************************/
// Some compilers cannot arbitrarily allow the handler nesting level to grow
// arbitrarily during Edit'n'Continue.
// This is the maximum nesting level that a compiler needs to support for EnC

const int MAX_EnC_HANDLER_NESTING_LEVEL = 6;

/************************************************************************
 * CORINFO_METHOD_HANDLE can actually refer to either a Function or a method, the
 * following callbacks are legal for either functions are methods
 ************************************************************************/

class ICorMethodInfo
{
public:
    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getMethodName (
            CORINFO_METHOD_HANDLE       ftn,        /* IN */
            const char                **moduleName  /* OUT */
            ) = 0;

    // this function is for debugging only.  It returns a value that
    // is will always be the same for a given method.  It is used
    // to implement the 'jitRange' functionality
    virtual unsigned __stdcall getMethodHash (
            CORINFO_METHOD_HANDLE       ftn         /* IN */
            ) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    // The callerHnd can be either the methodBeingCompiled or the immediate
    // caller of an inlined function.
    virtual DWORD __stdcall getMethodAttribs (
            CORINFO_METHOD_HANDLE       calleeHnd,        /* IN */
            CORINFO_METHOD_HANDLE       callerHnd     /* IN */
            ) = 0;

    // sets private JIT flags, which can be, retrieved using getAttrib.
    virtual void __stdcall setMethodAttribs (
            CORINFO_METHOD_HANDLE       ftn,        /* IN */
            CorInfoMethodRuntimeFlags   attribs     /* IN */
            ) = 0;

    // Given a method descriptor ftnHnd, extract signature information into sigInfo
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    virtual void __stdcall getMethodSig (
             CORINFO_METHOD_HANDLE      ftn,        /* IN  */
             CORINFO_SIG_INFO          *sig,        /* OUT */
             CORINFO_CLASS_HANDLE      memberParent = NULL /* IN */
             ) = 0;

    /*********************************************************************
     * Note the following methods can only be used on functions known
     * to be IL.  This includes the method being compiled and any method
     * that 'getMethodInfo' returns true for
     *********************************************************************/

    // return information about a method private to the implementation
    //      returns false if method is not IL, or is otherwise unavailable.
    //      This method is used to fetch data needed to inline functions
    virtual bool __stdcall getMethodInfo (
            CORINFO_METHOD_HANDLE   ftn,            /* IN  */
            CORINFO_METHOD_INFO*    info            /* OUT */
            ) = 0;

    // Decides if you have any limitations for inlining. If everything's OK, it will return
    // INLINE_PASS and will fill out pRestrictions with a mask of restrictions the caller of this
    // function must respect. If caller passes pRestrictions = NULL, if there are any restrictions
    // INLINE_FAIL will be returned
    //
    //
    // The inlined method need not be verified

    virtual CorInfoInline __stdcall canInline (
            CORINFO_METHOD_HANDLE       callerHnd,                  /* IN  */
            CORINFO_METHOD_HANDLE       calleeHnd,                  /* IN  */
            DWORD*                      pRestrictions              /* OUT */
            ) = 0;


    //  Returns false if the call is across assemblies thus we cannot tailcall

    virtual bool __stdcall canTailCall (
            CORINFO_METHOD_HANDLE   callerHnd,      /* IN  */
            CORINFO_METHOD_HANDLE   calleeHnd,      /* IN  */
            bool fIsTailPrefix                      /* IN */
            ) = 0;

    // Returns false if precompiled code must ensure that
    // the EE's DoPrestub function gets run before the
    // code for the method is used, i.e. if it returns false
    // then an indirect call must be made.
    //
    // Returning true does not guaratee that a direct call can be made:
    // there can be other reasons why the entry point cannot be embedded.
    //

    virtual bool __stdcall canSkipMethodPreparation (
            CORINFO_METHOD_HANDLE   callerHnd,      /* IN  */
            CORINFO_METHOD_HANDLE   calleeHnd,      /* IN  */
            bool                    fCheckCode,     /* IN */
            CorInfoIndirectCallReason *pReason = NULL,
            CORINFO_ACCESS_FLAGS    accessFlags = CORINFO_ACCESS_ANY) = 0;

    //  Returns true if a direct call can be made via the method entry point
    //
    virtual bool __stdcall canCallDirectViaEntryPointThunk (
            CORINFO_METHOD_HANDLE   calleeHnd,      /* IN  */
            void **                 pEntryPoint     /* OUT */
            ) = 0;

    // get individual exception handler
    virtual void __stdcall getEHinfo(
            CORINFO_METHOD_HANDLE ftn,              /* IN  */
            unsigned          EHnumber,             /* IN */
            CORINFO_EH_CLAUSE* clause               /* OUT */
            ) = 0;

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE __stdcall getMethodClass (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // return module it belongs to
    virtual CORINFO_MODULE_HANDLE __stdcall getMethodModule (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // This function returns the offset of the specified method in the
    // vtable of it's owning class or interface.
    virtual unsigned __stdcall getMethodVTableOffset (
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // If a method's attributes have (getMethodAttribs) CORINFO_FLG_INTRINSIC set,
    // getIntrinsicID() returns the intrinsic ID.
    virtual CorInfoIntrinsics __stdcall getIntrinsicID(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // return the unmanaged calling convention for a PInvoke
    virtual CorInfoUnmanagedCallConv __stdcall getUnmanagedCallConv(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // return if any marshaling is required for PInvoke methods.  Note that
    // method == 0 => calli.  The call site sig is only needed for the varargs or calli case
    virtual BOOL __stdcall pInvokeMarshalingRequired(
            CORINFO_METHOD_HANDLE       method,
            CORINFO_SIG_INFO*           callSiteSig
            ) = 0;

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    virtual BOOL __stdcall canAccessMethod(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_CLASS_HANDLE        parent,
            CORINFO_METHOD_HANDLE       target,
            CORINFO_CLASS_HANDLE        instance
            ) = 0;

    // Check constraints on method type arguments (only).
    // The parent class should be checked separately using satisfiesClassConstraints(parent).
    virtual BOOL __stdcall satisfiesMethodConstraints(
            CORINFO_CLASS_HANDLE        parent, // the exact parent of the method
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // Given a delegate target class, a target method parent class,  a  target method,
    // a delegate class, a scope, the target method ref, and the delegate constructor member ref
    // check if the method signature is compatible with the Invoke method of the delegate
    // (under the typical instantiation of any free type variables in the memberref signatures).
    // NB: arguments 2-4 could be inferred from 5-7, but are assumed to be available, and thus passed in for efficiency.
    virtual BOOL __stdcall isCompatibleDelegate(
            CORINFO_CLASS_HANDLE        objCls,           /* type of the delegate target, if any */
            CORINFO_CLASS_HANDLE        methodParentCls,  /* exact parent of the target method, if any */
            CORINFO_METHOD_HANDLE       method,           /* (representative) target method, if any */
            CORINFO_CLASS_HANDLE        delegateCls,      /* exact type of the delegate */
            CORINFO_MODULE_HANDLE       moduleHnd,        /* scope of the following refs */
            unsigned        methodMemberRef,              /* memberref of the target method */
            unsigned        delegateConstructorMemberRef  /* memberref of the delegate constructor */
            ) = 0;


    // Indicates if the method is an instance of the generic
    // method that passes (or has passed) verification
    virtual CorInfoInstantiationVerification __stdcall isInstantiationOfVerifiedGeneric (
            CORINFO_METHOD_HANDLE   method /* IN  */
            ) = 0;

    // Loads the constraints on a typical method definition, detecting cycles;
    // for use in verification.
    virtual void __stdcall initConstraintsForVerification(
            CORINFO_METHOD_HANDLE   method, /* IN */
            BOOL *pfHasCircularClassConstraints, /* OUT */
            BOOL *pfHasCircularMethodConstraint /* OUT */
            ) = 0;

    // Returns enum whether the method does not require verification
    // Also see ICorModuleInfo::canSkipVerification
    virtual CorInfoCanSkipVerificationResult __stdcall canSkipMethodVerification (
            CORINFO_METHOD_HANDLE       ftnHandle,     /* IN  */
            BOOL                        fQuickCheckOnly
            ) = 0;

    //  Determines whether a callout is allowed.
    virtual CorInfoIsCallAllowedResult __stdcall isCallAllowed (
            CORINFO_METHOD_HANDLE       callerHnd,                  // IN
            CORINFO_METHOD_HANDLE       calleeHnd,                  // IN
            CORINFO_CALL_ALLOWED_INFO * CallAllowedInfo             // OUT
            ) = 0;

    // load and restore the method
    virtual void __stdcall methodMustBeLoadedBeforeCodeIsRun(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    virtual CORINFO_METHOD_HANDLE __stdcall mapMethodDeclToMethodImpl(
            CORINFO_METHOD_HANDLE       method
            ) = 0;

    // Returns the global cookie for the /GS unsafe buffer checks
    // The cookie might be a constant value (JIT), or a handle to memory location (Ngen)
    virtual void __stdcall getGSCookie(
            GSCookie * pCookieVal,                     // OUT
            GSCookie ** ppCookieVal                    // OUT
            ) = 0;
};


/**********************************************************************************/

class ICorModuleInfo
{
public:
    // Given a type token metaTOK, use context to instantiate any type variables and return a type handle
    // The context parameter is also used to do access checks.
    virtual CORINFO_CLASS_HANDLE __stdcall findClass (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN  */
            CORINFO_CONTEXT_HANDLE      context,    /* IN  */
            CorInfoTokenKind            tokenKind = CORINFO_TOKENKIND_Default /* IN  */
            ) = 0;

    // Given a field token metaTOK, use context to instantiate any type variables in its *parent* type and return a field handle
    // The context parameter is also used to do access checks.
    virtual CORINFO_FIELD_HANDLE __stdcall findField (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN  */
            CORINFO_CONTEXT_HANDLE      context     /* IN  */
            ) = 0;

    // Given a method token metaTOK, use context to instantiate any type variables in its *parent* type and return a method handle
    // This looks up a function by token (what the IL CALLVIRT, CALLSTATIC instructions use)
    // The context parameter is also used to do access checks.
    // If metaTOK is combined with the flag CORINFO_ANNOT_ALLOWINSTPARAM and it refers to a generic method or an instance method in a generic struct,
    // then the method handle returned might be for shared code which expects an extra parameter providing extra instantiation information.
    // Otherwise (the default), a specialized stub method is returned that doesn't take this parameter
    virtual CORINFO_METHOD_HANDLE __stdcall findMethod (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK,    /* IN */
            CORINFO_CONTEXT_HANDLE      context     /* IN  */
            ) = 0;

    // Given a field or method token metaTOK return its parent token
    virtual unsigned __stdcall getMemberParent(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK) = 0;

    // Signature information about the call sig
    virtual void __stdcall findSig (
            CORINFO_MODULE_HANDLE       module,     /* IN */
            unsigned                    sigTOK,    /* IN */
            CORINFO_METHOD_HANDLE       context,     /* IN */
            CORINFO_SIG_INFO           *sig         /* OUT */
            ) = 0;

    // for Varargs, the signature at the call site may differ from
    // the signature at the definition.  Thus we need a way of
    // fetching the call site information
    virtual void __stdcall findCallSiteSig (
            CORINFO_MODULE_HANDLE       module,     /* IN */
            unsigned                    methTOK,    /* IN */
            CORINFO_METHOD_HANDLE       context,     /* IN */
            CORINFO_SIG_INFO           *sig         /* OUT */
            ) = 0;

    virtual CORINFO_CLASS_HANDLE __stdcall getTokenTypeAsHandle (
            CORINFO_MODULE_HANDLE  scopeHnd,
            unsigned metaTOK) = 0;


    virtual size_t __stdcall findNameOfToken (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            mdToken                     metaTOK,     /* IN  */
            __out_ecount (FQNameCapacity) char * szFQName, /* OUT */
            size_t FQNameCapacity  /* IN */
            ) = 0;

    // Returns true if the module does not require verification
    //
    // If fQuickCheckOnlyWithoutCommit=TRUE, the function only checks that the
    // module does not currently require verification in the current AppDomain.
    // This decision could change in the future, and so should not be cached.
    // If it is cached, it should only be used as a hint.
    // This is only used by ngen for calculating certain hints.
    //
   
    // Returns enum whether the module does not require verification
    // Also see ICorMethodInfo::canSkipMethodVerification();
    virtual CorInfoCanSkipVerificationResult __stdcall canSkipVerification (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            BOOL                        fQuickCheckOnlyWithoutCommit = FALSE /* IN */
            ) = 0;

    // Checks if the given metadata token is valid
    virtual BOOL __stdcall isValidToken (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK     /* IN  */
            ) = 0;

    // Checks if the given metadata token is valid StringRef
    virtual BOOL __stdcall isValidStringRef (
            CORINFO_MODULE_HANDLE       module,     /* IN  */
            unsigned                    metaTOK     /* IN  */
            ) = 0;

};

/**********************************************************************************/

class ICorClassInfo
{
public:
    // If the value class 'cls' is isomorphic to a primitive type it will
    // return that type, otherwise it will return CORINFO_TYPE_VALUECLASS
    virtual CorInfoType __stdcall asCorInfoType (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    // for completeness
    virtual const char* __stdcall getClassName (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;


    // Append a (possibly truncated) representation of the type cls to the preallocated buffer ppBuf of length pnBufLen
    // If fNamespace=TRUE, include the namespace/enclosing classes
    // If fFullInst=TRUE (regardless of fNamespace and fAssembly), include namespace and assembly for any type parameters
    // If fAssembly=TRUE, suffix with a comma and the full assembly qualification
    // return size of representation
    virtual int __stdcall appendClassName(
            WCHAR** ppBuf, int* pnBufLen,
            CORINFO_CLASS_HANDLE    cls,
            BOOL fNamespace,
            BOOL fFullInst,
            BOOL fAssembly
            ) = 0;

    // If this method returns true, JIT will do optimization to inline the check for
    //     GetClassFromHandle(handle) == obj.GetType()
    virtual BOOL __stdcall canInlineTypeCheckWithObjectVTable(CORINFO_CLASS_HANDLE cls) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    //
    // The "methodBeingCompiled" parameter is used to determine
    // domain-neutrality of the code being generated, for the CORINFO_FLG_NEEDS_INIT
    // and CORINFO_FLG_INITIALIZED flags.  It thus is really used
    // to represent a compilation scope.  If you are inling
    // methods, then methodBeingCompiled should be the originating
    // method being compiled, not the nested method that refers to "cls".
    virtual DWORD __stdcall getClassAttribs (
            CORINFO_CLASS_HANDLE    cls,
            CORINFO_METHOD_HANDLE   methodBeingCompiledHnd
            ) = 0;

    virtual CORINFO_MODULE_HANDLE __stdcall getClassModule (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    virtual CORINFO_MODULE_HANDLE __stdcall getClassModuleForStatics (
            CORINFO_CLASS_HANDLE    cls
            ) = 0;

    // get the class representing the single dimensional array for the
    // element type represented by clsHnd
    virtual CORINFO_CLASS_HANDLE __stdcall getSDArrayForClass (
            CORINFO_CLASS_HANDLE    clsHnd
            ) = 0 ;

    // return the number of bytes needed by an instance of the class
    virtual unsigned __stdcall getClassSize (
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    virtual unsigned __stdcall getClassAlignmentRequirement (
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // This is only called for Value classes.  It returns a boolean array
    // in representing of 'cls' from a GC perspective.  The class is
    // assumed to be an array of machine words
    // (of length // getClassSize(cls) / sizeof(void*)),
    // 'gcPtrs' is a poitner to an array of BYTEs of this length.
    // getClassGClayout fills in this array so that gcPtrs[i] is set
    // to one of the CorInfoGCType values which is the GC type of
    // the i-th machine word of an object of type 'cls'
    // returns the number of GC pointers in the array
    virtual unsigned __stdcall getClassGClayout (
            CORINFO_CLASS_HANDLE        cls,        /* IN */
            BYTE                       *gcPtrs      /* OUT */
            ) = 0;

    // returns the number of instance fields in a class
    virtual unsigned __stdcall getClassNumInstanceFields (
            CORINFO_CLASS_HANDLE        cls        /* IN */
            ) = 0;

    virtual CORINFO_FIELD_HANDLE __stdcall getFieldInClass(
            CORINFO_CLASS_HANDLE clsHnd,
            INT num
            ) = 0;

    virtual INT __stdcall getClassCustomAttribute(
            CORINFO_CLASS_HANDLE clsHnd,
            LPCSTR attrib,
            const BYTE** ppVal
            ) = 0;

    virtual mdMethodDef __stdcall getMethodDefFromMethod(
            CORINFO_METHOD_HANDLE hMethod
            ) = 0;

    virtual BOOL __stdcall checkMethodModifier(
            CORINFO_METHOD_HANDLE hMethod,
            LPCSTR modifier,
            BOOL fOptional
            ) = 0;

    // returns the "NEW" helper optimized for "newCls."
    virtual CorInfoHelpFunc __stdcall getNewHelper(
            CORINFO_CLASS_HANDLE        newCls,
            CORINFO_METHOD_HANDLE       context,
            unsigned classToken,
            CORINFO_MODULE_HANDLE tokenContext
            ) = 0;

    // returns the newArr (1-Dim array) helper optimized for "arrayCls."
    virtual CorInfoHelpFunc __stdcall getNewArrHelper(
            CORINFO_CLASS_HANDLE        arrayCls,
            CORINFO_METHOD_HANDLE       context
            ) = 0;

    // returns the newMdArr (multi-Dim array) helper optimized for "arrayCtorMethod"
    virtual CorInfoHelpFunc __stdcall getNewMDArrHelper(
            CORINFO_CLASS_HANDLE        arrayCls,
            CORINFO_METHOD_HANDLE       arrayCtorMethod,
            CORINFO_METHOD_HANDLE       context
            ) = 0;

    // returns the "IsInstanceOf" helper optimized for "IsInstCls."
    virtual CorInfoHelpFunc __stdcall getIsInstanceOfHelper(
            CORINFO_MODULE_HANDLE       scopeHnd,
            unsigned                    metaTOK,
            CORINFO_CONTEXT_HANDLE      context) = 0;

    // returns the "ChkCast" helper optimized for "IsInstCls."
    virtual CorInfoHelpFunc __stdcall getChkCastHelper(
            CORINFO_MODULE_HANDLE       scopeHnd,
            unsigned                    metaTOK,
            CORINFO_CONTEXT_HANDLE      context) = 0;

    virtual CorInfoHelpFunc __stdcall getSharedStaticBaseHelper(
            CORINFO_FIELD_HANDLE    fldHnd,
            BOOL                    runtimeLookup = FALSE
            ) = 0;

    virtual CorInfoHelpFunc __stdcall getSharedCCtorHelper(
            CORINFO_CLASS_HANDLE clsHnd
            ) = 0;

    virtual CorInfoHelpFunc __stdcall getSecurityHelper(
            CORINFO_METHOD_HANDLE   ftn,
            BOOL                    fEnter // TRUE = prolog, FALSE = epilog
            ) = 0;

    // This is not pretty.  Boxing nullable<T> actually returns
    // a boxed<T> not a boxed Nullable<T>.  This call allows the verifier
    // to call back to the EE on the 'box' instruction and get the transformed
    // type to use for verification.
    virtual CORINFO_CLASS_HANDLE  getTypeForBox(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // returns the correct box helper for a particular class.  Note
    // that if this returns CORINFO_HELP_BOX, the JIT can assume 
    // 'standard' boxing (allocate object and copy), and optimize
    virtual CorInfoHelpFunc __stdcall getBoxHelper(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // returns the unbox helper.  If 'helperCopies' points to a true 
    // value it means the JIT is requesting a helper that unboxes the
    // value into a particular location and thus has the signature
    //     void unboxHelper(void* dest, CORINFO_CLASS_HANDLE cls, Object* obj)
    // Otherwise (it is null or points at a FALSE value) it is requesting 
    // a helper that returns a poitner to the unboxed data 
    //     void* unboxHelper(CORINFO_CLASS_HANDLE cls, Object* obj)
    // The EE has the option of NOT returning the copy style helper
    // (But must be able to always honor the non-copy style helper)
    // The EE set 'helperCopies' on return to indicate what kind of
    // helper has been created.  

    virtual CorInfoHelpFunc __stdcall getUnBoxHelper(
            CORINFO_CLASS_HANDLE        cls,
            BOOL*                       helperCopies = NULL
            ) = 0;

    virtual const char* __stdcall getHelperName(
            CorInfoHelpFunc
            ) = 0;

    // This function tries to initialize the class (run the class constructor).
    // this function can return false, which means that the JIT must
    // insert helper calls before accessing class members.
    //
    // This should probably be renamed to something like "ensureClassIsInitedBeforeCodeIsRun"
    // but that doesn't capture the fact that the function returns FALSE if a slow path
    // is needed to ensure the initialization semantics for the class.                      
    //
    // The methodBeingCompiledHnd is used to determine if
    virtual BOOL __stdcall initClass(
            CORINFO_CLASS_HANDLE        cls,
            CORINFO_METHOD_HANDLE       methodBeingCompiledHnd,
            BOOL                        speculative = FALSE,    // TRUE means don't actually run it
            BOOL                       *pfNeedsInitFixup = NULL // Set to TRUE if it is necessary to init the class by fixup in prejit code
            ) = 0;



    // This used to be called "loadClass".  This records the fact
    // that the class must be loaded (including restored if necessary) before we execute the
    // code that we are currently generating.  When jitting code
    // the function loads the class immediately.  When zapping code
    // the zapper will if necessary use the call to record the fact that we have
    // to do a fixup/restore before running the method currently being generated.
    //
    // This is typically used to ensure value types are loaded before zapped
    // code that manipulates them is executed, so that the GC can access information
    // about those value types.
    virtual void __stdcall classMustBeLoadedBeforeCodeIsRun(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // returns the class handle for the special builtin classes
    virtual CORINFO_CLASS_HANDLE __stdcall getBuiltinClass (
            CorInfoClassId              classId
            ) = 0;

    // "System.Int32" ==> CORINFO_TYPE_INT..
    virtual CorInfoType __stdcall getTypeForPrimitiveValueClass(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // TRUE if child is a subtype of parent
    // if parent is an interface, then does child implement / extend parent
    virtual BOOL __stdcall canCast(
            CORINFO_CLASS_HANDLE        child,  // subtype (extends parent)
            CORINFO_CLASS_HANDLE        parent  // base type
            ) = 0;

    // returns is the intersection of cls1 and cls2.
    virtual CORINFO_CLASS_HANDLE __stdcall mergeClasses(
            CORINFO_CLASS_HANDLE        cls1,
            CORINFO_CLASS_HANDLE        cls2
            ) = 0;

    // Given a class handle, returns the Parent type.
    // For COMObjectType, it returns Class Handle of System.Object.
    // Returns 0 if System.Object is passed in.
    virtual CORINFO_CLASS_HANDLE __stdcall getParentType (
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // Returns the CorInfoType of the "child type". If the child type is
    // not a primitive type, *clsRet will be set.
    // Given an Array of Type Foo, returns Foo.
    // Given BYREF Foo, returns Foo
    virtual CorInfoType __stdcall getChildType (
            CORINFO_CLASS_HANDLE       clsHnd,
            CORINFO_CLASS_HANDLE       *clsRet
            ) = 0;

    // Check Visibility rules.
    virtual BOOL __stdcall canAccessType(
            CORINFO_METHOD_HANDLE       context,
            CORINFO_CLASS_HANDLE        target
            ) = 0;

    // Check constraints on type arguments of this class and parent classes
    virtual BOOL __stdcall satisfiesClassConstraints(
            CORINFO_CLASS_HANDLE cls
            ) = 0;

    // Check if this is a single dimensional array type
    virtual BOOL __stdcall isSDArray(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // Get the numbmer of dimensions in an array 
    virtual unsigned __stdcall getArrayRank(
            CORINFO_CLASS_HANDLE        cls
            ) = 0;

    // Get static field data for an array
    virtual void * __stdcall getArrayInitializationData(
            CORINFO_FIELD_HANDLE        field,
            DWORD                       size
            ) = 0;
};


/**********************************************************************************/

class ICorFieldInfo
{
public:
    // this function is for debugging only.  It returns the field name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* __stdcall getFieldName (
                        CORINFO_FIELD_HANDLE        ftn,        /* IN */
                        const char                **moduleName  /* OUT */
                        ) = 0;

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD __stdcall getFieldAttribs (
                        CORINFO_FIELD_HANDLE    field,
                        CORINFO_METHOD_HANDLE   context,
                        CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY
                        ) = 0;

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE __stdcall getFieldClass (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // Return the field's type, if it is CORINFO_TYPE_VALUECLASS 'structType' is set
    // the field's value class (if 'structType' == 0, then don't bother
    // the structure info).
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    virtual CorInfoType __stdcall getFieldType(
                        CORINFO_FIELD_HANDLE    field,
                        CORINFO_CLASS_HANDLE   *structType,
                        CORINFO_CLASS_HANDLE    memberParent = NULL /* IN */
                        ) = 0;

    // returns the field's compilation category
    virtual CorInfoFieldCategory __stdcall getFieldCategory (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // returns the field's compilation category
    virtual CorInfoHelpFunc __stdcall getFieldHelper(
                        CORINFO_FIELD_HANDLE    field,
                        enum CorInfoFieldAccess kind    // Get, Set, Address
                        ) = 0;

    // return the data member's instance offset
    virtual unsigned __stdcall getFieldOffset (
                        CORINFO_FIELD_HANDLE    field
                        ) = 0;

    // Check Visibility rules.
    // For Protected (family access) members, type of the instance is also
    // considered when checking visibility rules.
    virtual BOOL __stdcall canAccessField(
                        CORINFO_METHOD_HANDLE   context,
                        CORINFO_CLASS_HANDLE    parent,
                        CORINFO_FIELD_HANDLE    target,
                        CORINFO_CLASS_HANDLE    instance
                        ) = 0;
};

/*********************************************************************************/

class ICorDebugInfo
{
public:

    /*----------------------------- Boundary-info ---------------------------*/

    enum MappingTypes
    {
        NO_MAPPING  = -1,
        PROLOG      = -2,
        EPILOG      = -3,
        MAX_MAPPING_VALUE = -3 // Sentinal value. This should be set to the largest magnitude value in the enum
                               // so that the compression routines know the enum's range.
    };

    enum BoundaryTypes
    {
        NO_BOUNDARIES           = 0x00,     // No implicit boundaries
        STACK_EMPTY_BOUNDARIES  = 0x01,     // Boundary whenever the IL evaluation stack is empty
        NOP_BOUNDARIES          = 0x02,     // Before every CEE_NOP instruction
        CALL_SITE_BOUNDARIES    = 0x04,     // Before every CEE_CALL, CEE_CALLVIRT, etc instruction

        // Set of boundaries that debugger should always reasonably ask the JIT for.
        DEFAULT_BOUNDARIES      = STACK_EMPTY_BOUNDARIES | NOP_BOUNDARIES | CALL_SITE_BOUNDARIES
    };

    // Note that SourceTypes can be OR'd together - it's possible that
    // a sequence point will also be a stack_empty point, and/or a call site.
    // The debugger will check to see if a boundary offset's source field &
    // SEQUENCE_POINT is true to determine if the boundary is a sequence point.

    enum SourceTypes
    {
        SOURCE_TYPE_INVALID        = 0x00, // To indicate that nothing else applies
        SEQUENCE_POINT             = 0x01, // The debugger asked for it.
        STACK_EMPTY                = 0x02, // The stack is empty here
        CALL_SITE                  = 0x04, // This is a call site.
        NATIVE_END_OFFSET_UNKNOWN  = 0x08  // Indicates a epilog endpoint
    };

    struct OffsetMapping
    {
        DWORD           nativeOffset;
        DWORD           ilOffset;
        SourceTypes     source; // The debugger needs this so that
                                // we don't put Edit and Continue breakpoints where
                                // the stack isn't empty.  We can put regular breakpoints
                                // there, though, so we need a way to discriminate
                                // between offsets.
    };

    // Query the EE to find out where interesting break points
    // in the code are.  The native compiler will ensure that these places
    // have a corresponding break point in native code.
    //
    // Note that unless CORJIT_FLG_DEBUG_CODE is specified, this function will
    // be used only as a hint and the native compiler should not change its
    // code generation.
    virtual void __stdcall getBoundaries(
                CORINFO_METHOD_HANDLE   ftn,                // [IN] method of interest
                unsigned int           *cILOffsets,         // [OUT] size of pILOffsets
                DWORD                 **pILOffsets,         // [OUT] IL offsets of interest
                                                            //       jit MUST free with freeArray!
                BoundaryTypes          *implictBoundaries   // [OUT] tell jit, all boundries of this type
                ) = 0;

    // Report back the mapping from IL to native code,
    // this map should include all boundaries that 'getBoundaries'
    // reported as interesting to the debugger.

    // Note that debugger (and profiler) is assuming that all of the
    // offsets form a contiguous block of memory, and that the
    // OffsetMapping is sorted in order of increasing native offset.
    virtual void __stdcall setBoundaries(
                CORINFO_METHOD_HANDLE   ftn,            // [IN] method of interest
                ULONG32                 cMap,           // [IN] size of pMap
                OffsetMapping          *pMap            // [IN] map including all points of interest.
                                                        //      jit allocated with allocateArray, EE frees
                ) = 0;

    /*------------------------------ Var-info -------------------------------*/

    enum RegNum
    {
#ifdef _X86_
        REGNUM_EAX,
        REGNUM_ECX,
        REGNUM_EDX,
        REGNUM_EBX,
        REGNUM_ESP,
        REGNUM_EBP,
        REGNUM_ESI,
        REGNUM_EDI,
#elif _PPC_
        REGNUM_R1,
        REGNUM_R30,
        REGNUM_R3,
        REGNUM_R4,
        REGNUM_R5,
        REGNUM_R6,
        REGNUM_R7,
        REGNUM_R8,
        REGNUM_R9,
        REGNUM_R10,
#else
        PORTABILITY_WARNING("Register numbers not defined on this platform")
#endif
        REGNUM_COUNT,
        REGNUM_AMBIENT_SP, // ambient SP support. Ambient SP is the original SP in the non-BP based frame.
                           // Ambient SP should not change even if there are push/pop operations in the method.

#ifdef _X86_
        REGNUM_FP = REGNUM_EBP,
        REGNUM_SP = REGNUM_ESP,
#elif _PPC_
        REGNUM_FP = REGNUM_R30,
        REGNUM_SP = REGNUM_R1,
#else
        // RegNum values should be properly defined for this platform
        REGNUM_FP = 0,
        REGNUM_SP = 1,
#endif

    };

    // VarLoc describes the location of a native variable.  Note that currently, VLT_REG_BYREF and VLT_STK_BYREF 
    // are only used for value types on X64.

    enum VarLocType
    {
        VLT_REG,        // variable is in a register
        VLT_REG_BYREF,  // address of the variable is in a register
        VLT_REG_FP,     // variable is in an fp register
        VLT_STK,        // variable is on the stack (memory addressed relative to the frame-pointer)
        VLT_STK_BYREF,  // address of the variable is on the stack (memory addressed relative to the frame-pointer)
        VLT_REG_REG,    // variable lives in two registers
        VLT_REG_STK,    // variable lives partly in a register and partly on the stack
        VLT_STK_REG,    // reverse of VLT_REG_STK
        VLT_STK2,       // variable lives in two slots on the stack
        VLT_FPSTK,      // variable lives on the floating-point stack
        VLT_FIXED_VA,   // variable is a fixed argument in a varargs function (relative to VARARGS_HANDLE)

        VLT_COUNT,
        VLT_INVALID
    };

    struct VarLoc
    {
        VarLocType      vlType;

        union
        {
            // VLT_REG/VLT_REG_FP -- Any pointer-sized enregistered value (TYP_INT, TYP_REF, etc)
            // eg. EAX
            // VLT_REG_BYREF -- the specified register contains the address of the variable
            // eg. [EAX]

            struct
            {
                RegNum      vlrReg;
            } vlReg;

            // VLT_STK -- Any 32 bit value which is on the stack
            // eg. [ESP+0x20], or [EBP-0x28]
            // VLT_STK_BYREF -- the specified stack location contains the address of the variable
            // eg. mov EAX, [ESP+0x20]; [EAX]

            struct
            {
                RegNum      vlsBaseReg;
                signed      vlsOffset;
            } vlStk;

            // VLT_REG_REG -- TYP_LONG with both DWords enregistred
            // eg. RBM_EAXEDX

            struct
            {
                RegNum      vlrrReg1;
                RegNum      vlrrReg2;
            } vlRegReg;

            // VLT_REG_STK -- Partly enregistered TYP_LONG
            // eg { LowerDWord=EAX UpperDWord=[ESP+0x8] }

            struct
            {
                RegNum      vlrsReg;
                struct
                {
                    RegNum      vlrssBaseReg;
                    signed      vlrssOffset;
                }           vlrsStk;
            } vlRegStk;

            // VLT_STK_REG -- Partly enregistered TYP_LONG
            // eg { LowerDWord=[ESP+0x8] UpperDWord=EAX }

            struct
            {
                struct
                {
                    RegNum      vlsrsBaseReg;
                    signed      vlsrsOffset;
                }           vlsrStk;
                RegNum      vlsrReg;
            } vlStkReg;

            // VLT_STK2 -- Any 64 bit value which is on the stack,
            // in 2 successsive DWords.
            // eg 2 DWords at [ESP+0x10]

            struct
            {
                RegNum      vls2BaseReg;
                signed      vls2Offset;
            } vlStk2;

            // VLT_FPSTK -- enregisterd TYP_DOUBLE (on the FP stack)
            // eg. ST(3). Actually it is ST("FPstkHeigth - vpFpStk")

            struct
            {
                unsigned        vlfReg;
            } vlFPstk;

            // VLT_FIXED_VA -- fixed argument of a varargs function.
            // The argument location depends on the size of the variable
            // arguments (...). Inspecting the VARARGS_HANDLE indicates the
            // location of the first arg. This argument can then be accessed
            // relative to the position of the first arg

            struct
            {
                unsigned        vlfvOffset;
            } vlFixedVarArg;

            // VLT_MEMORY

            struct
            {
                void        *rpValue; // pointer to the in-process
                // location of the value.
            } vlMemory;
        };
    };

    // This is used to report implicit/hidden arguments

    enum
    {
        VARARGS_HND_ILNUM   = -1, // Value for the CORINFO_VARARGS_HANDLE varNumber
        RETBUF_ILNUM        = -2, // Pointer to the return-buffer
        TYPECTXT_ILNUM      = -3, // ParamTypeArg for CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG

        UNKNOWN_ILNUM       = -4, // Unknown variable

        MAX_ILNUM           = -4  // Sentinal value. This should be set to the largest magnitude value in th enum
                                  // so that the compression routines know the enum's range.
    };

    struct ILVarInfo
    {
        DWORD           startOffset;
        DWORD           endOffset;
        DWORD           varNumber;
    };

    struct NativeVarInfo
    {
        DWORD           startOffset;
        DWORD           endOffset;
        DWORD           varNumber;
        VarLoc          loc;
    };

    // Query the EE to find out the scope of local varables.
    // normally the JIT would trash variables after last use, but
    // under debugging, the JIT needs to keep them live over their
    // entire scope so that they can be inspected.
    //
    // Note that unless CORJIT_FLG_DEBUG_CODE is specified, this function will
    // be used only as a hint and the native compiler should not change its
    // code generation.
    virtual void __stdcall getVars(
            CORINFO_METHOD_HANDLE           ftn,            // [IN]  method of interest
            ULONG32                        *cVars,          // [OUT] size of 'vars'
            ILVarInfo                     **vars,           // [OUT] scopes of variables of interest
                                                            //       jit MUST free with freeArray!
            bool                           *extendOthers    // [OUT] it TRUE, then assume the scope
                                                            //       of unmentioned vars is entire method
            ) = 0;

    // Report back to the EE the location of every variable.
    // note that the JIT might split lifetimes into different
    // locations etc.

    virtual void __stdcall setVars(
            CORINFO_METHOD_HANDLE           ftn,            // [IN] method of interest
            ULONG32                         cVars,          // [IN] size of 'vars'
            NativeVarInfo                  *vars            // [IN] map telling where local vars are stored at what points
                                                            //      jit allocated with allocateArray, EE frees
            ) = 0;

    /*-------------------------- Misc ---------------------------------------*/

    // Used to allocate memory that needs to handed to the EE.
    // For eg, use this to allocated memory for reporting debug info,
    // which will be handed to the EE by setVars() and setBoundaries()
    virtual void * __stdcall allocateArray(
                        ULONG              cBytes
                        ) = 0;

    // JitCompiler will free arrays passed by the EE using this
    // For eg, The EE returns memory in getVars() and getBoundaries()
    // to the JitCompiler, which the JitCompiler should release using
    // freeArray()
    virtual void __stdcall freeArray(
            void               *array
            ) = 0;
};

/*****************************************************************************/

class ICorArgInfo
{
public:
    // advance the pointer to the argument list.
    // a ptr of 0, is special and always means the first argument
    virtual CORINFO_ARG_LIST_HANDLE __stdcall getArgNext (
            CORINFO_ARG_LIST_HANDLE     args            /* IN */
            ) = 0;

    // Get the type of a particular argument
    // CORINFO_TYPE_UNDEF is returned when there are no more arguments
    // If the type returned is a primitive type (or an enum) *vcTypeRet set to NULL
    // otherwise it is set to the TypeHandle associted with the type
    // Enumerations will always look their underlying type (probably should fix this)
    // Otherwise vcTypeRet is the type as would be seen by the IL,
    // The return value is the type that is used for calling convention purposes
    // (Thus if the EE wants a value class to be passed like an int, then it will
    // return CORINFO_TYPE_INT
    virtual CorInfoTypeWithMod __stdcall getArgType (
            CORINFO_SIG_INFO*           sig,            /* IN */
            CORINFO_ARG_LIST_HANDLE     args,           /* IN */
            CORINFO_CLASS_HANDLE       *vcTypeRet       /* OUT */
            ) = 0;

    // If the Arg is a CORINFO_TYPE_CLASS fetch the class handle associated with it
    virtual CORINFO_CLASS_HANDLE __stdcall getArgClass (
            CORINFO_SIG_INFO*           sig,            /* IN */
            CORINFO_ARG_LIST_HANDLE     args            /* IN */
            ) = 0;
};

class ICorLinkInfo
{
public:
    // Called when an absolute or relative pointer is embedded in the code
    // stream. A relocation is recorded if we are pre-jitting.

    virtual void __stdcall recordRelocation(
            void                       *location,   /* IN  */
            WORD                        fRelocType  /* IN  */
            ) = 0;
};

/*****************************************************************************
 * ICorErrorInfo contains methods to deal with SEH exceptions being thrown
 * from the corinfo interface.  These methods may be called when an exception
 * with code EXCEPTION_COMPLUS is caught.
 *
 *                                                                                     
 *****************************************************************************/

class ICorErrorInfo
{
public:
    // Returns the HRESULT of the current exception
    virtual HRESULT __stdcall GetErrorHRESULT(
            struct _EXCEPTION_POINTERS *pExceptionPointers
            ) = 0;

    // Returns the class of the current exception
    virtual CORINFO_CLASS_HANDLE __stdcall GetErrorClass() = 0;

    // Fetches the message of the current exception
    // Returns the size of the message (including terminating null). This can be
    // greater than bufferLength if the buffer is insufficient.
    virtual ULONG __stdcall GetErrorMessage(
            LPWSTR buffer,
            ULONG bufferLength
            ) = 0;

    // returns EXCEPTION_EXECUTE_HANDLER if it is OK for the compile to handle the
    //                        exception, abort some work (like the inlining) and continue compilation
    // returns EXCEPTION_CONTINUE_SEARCH if exception must always be handled by the EE
    //                    things like ThreadStoppedException ...
    // returns EXCEPTION_CONTINUE_EXECUTION if exception is fixed up by the EE

    virtual int __stdcall FilterException(
            struct _EXCEPTION_POINTERS *pExceptionPointers
            ) = 0;
};


/*****************************************************************************
 * ICorStaticInfo contains EE interface methods which return values that are
 * constant from invocation to invocation.  Thus they may be embedded in
 * persisted information like statically generated code. (This is of course
 * assuming that all code versions are identical each time.)
 *****************************************************************************/

class ICorStaticInfo : public virtual ICorMethodInfo, public virtual ICorModuleInfo,
                       public virtual ICorClassInfo,  public virtual ICorFieldInfo,
                       public virtual ICorDebugInfo,  public virtual ICorArgInfo,
                       public virtual ICorLinkInfo,   public virtual ICorErrorInfo
{
public:
    // Return details about EE internal data structures
    virtual void __stdcall getEEInfo(
                CORINFO_EE_INFO            *pEEInfoOut
                ) = 0;
};

/*****************************************************************************
 * ICorDynamicInfo contains EE interface methods which return values that may
 * change from invocation to invocation.  They cannot be embedded in persisted
 * data; they must be requeried each time the EE is run.
 *****************************************************************************/

class ICorDynamicInfo : public virtual ICorStaticInfo
{
public:

    //
    // These methods return values to the JIT which are not constant
    // from session to session.
    //
    // These methods take an extra parameter : void **ppIndirection.
    // If a JIT supports generation of prejit code (install-o-jit), it
    // must pass a non-null value for this parameter, and check the
    // resulting value.  If *ppIndirection is NULL, code should be
    // generated normally.  If non-null, then the value of
    // *ppIndirection is an address in the cookie table, and the code
    // generator needs to generate an indirection through the table to
    // get the resulting value.  In this case, the return result of the
    // function must NOT be directly embedded in the generated code.
    //
    // Note that if a JIT does not support prejit code generation, it
    // may ignore the extra parameter & pass the default of NULL - the
    // prejit ICorDynamicInfo implementation will see this & generate
    // an error if the jitter is used in a prejit scenario.
    //

    // Return details about EE internal data structures

    virtual DWORD __stdcall getThreadTLSIndex(
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual const void * __stdcall getInlinedCallFrameVptr(
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual LONG * __stdcall getAddrOfCaptureThreadGlobal(
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual SIZE_T* __stdcall       getAddrModuleDomainID(CORINFO_MODULE_HANDLE   module) = 0;

    // return the native entry point to an EE helper (see CorInfoHelpFunc)
    virtual void* __stdcall getHelperFtn (
                    CorInfoHelpFunc         ftnNum,
                    void                  **ppIndirection = NULL,
                    InfoAccessModule       *pAccessModule = NULL
                    ) = 0;

    // return a callable address of the function (native code). This function
    // may return a different value (depending on whether the method has
    // been JITed or not.  pAccessType is an in-out parameter.  The JIT
    // specifies what level of indirection it desires, and the EE sets it
    // to what it can provide (which may not be the same).
    virtual void __stdcall getFunctionEntryPoint(
                              CORINFO_METHOD_HANDLE   ftn,                 /* IN  */
                              InfoAccessType          requestedAccessType, /* IN */
                              CORINFO_CONST_LOOKUP *  pResult,             /* OUT */
                              CORINFO_ACCESS_FLAGS    accessFlags = CORINFO_ACCESS_ANY) = 0;

    // return a directly callable address. This can be used similarly to the
    // value returned by getFunctionEntryPoint() except that it is
    // guaranteed to be the same for a given function.
    // pAccessType is an in-out parameter.  The JIT
    // specifies what level of indirection it desires, and the EE sets it
    // to what it can provide (which may not be the same).
    virtual void __stdcall getFunctionFixedEntryPointInfo(
                              CORINFO_MODULE_HANDLE  scopeHnd,
                              unsigned               metaTOK,
                              CORINFO_CONTEXT_HANDLE context,
                              CORINFO_LOOKUP *       pResult) = 0;

    // get the syncronization handle that is passed to monXstatic function
    virtual void* __stdcall getMethodSync(
                    CORINFO_METHOD_HANDLE               ftn,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // These entry points must be called if a handle is being embedded in
    // the code to be passed to a JIT helper function. (as opposed to just
    // being passed back into the ICorInfo interface.)

    // a module handle may not always be available. A call to embedModuleHandle should always
    // be preceeded by a call to canEmbedModuleHandleForHelper. A dynamicMethod does not have a module
    virtual bool __stdcall canEmbedModuleHandleForHelper(
                    CORINFO_MODULE_HANDLE   handle
                    ) = 0;

    virtual CORINFO_MODULE_HANDLE __stdcall embedModuleHandle(
                    CORINFO_MODULE_HANDLE   handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_CLASS_HANDLE __stdcall embedClassHandle(
                    CORINFO_CLASS_HANDLE    handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_METHOD_HANDLE __stdcall embedMethodHandle(
                    CORINFO_METHOD_HANDLE   handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    virtual CORINFO_FIELD_HANDLE __stdcall embedFieldHandle(
                    CORINFO_FIELD_HANDLE    handle,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Given a module scope (module), a method handle (context) and
    // a metadata token (metaTOK), fetch the handle
    // (type, field or method) associated with the token.
    // If this is not possible at compile-time (because the current method's
    // code is shared and the token contains generic parameters)
    // then indicate how the handle should be looked up at run-time.
    //
    // Type tokens can be combined with CORINFO_ANNOT_MASK flags
    // to obtain array type handles. These are typically required by the 'newarr'
    // instruction which takes a token for the *element* type of the array.
    //
    // Similarly method tokens can be combined with CORINFO_ANNOT_MASK flags
    // method entry points. These are typically required by the 'call' and 'ldftn'
    // instructions.
    //
    // Byrefs or System.Void should only occur in method and local signatures, which 
    // are accessed using ICorClassInfo and ICorClassInfo.getChildType. ldtoken is one 
    // exception from this rule. allowAllTypes should be set to true only for ldtoken only!
    //
    virtual void __stdcall embedGenericHandle(
                        CORINFO_MODULE_HANDLE   module,
                        unsigned                metaTOK,
                        CORINFO_CONTEXT_HANDLE  context,
                        CorInfoTokenKind        tokenKind,
                        CORINFO_GENERICHANDLE_RESULT *pResult) = 0;

    // Return information used to locate the exact enclosing type of the current method.
    // Used only to invoke .cctor method from code shared across generic instantiations
    //   !needsRuntimeLookup       statically known (enclosing type of method itself)
    //   needsRuntimeLookup:
    //      CORINFO_LOOKUP_THISOBJ     use vtable pointer of 'this' param
    //      CORINFO_LOOKUP_CLASSPARAM  use vtable hidden param
    //      CORINFO_LOOKUP_METHODPARAM use enclosing type of method-desc hidden param
    virtual CORINFO_LOOKUP_KIND __stdcall getLocationOfThisType(
                    CORINFO_METHOD_HANDLE context
                    ) = 0;

    // return the unmanaged target *if method has already been prelinked.*
    virtual void* __stdcall getPInvokeUnmanagedTarget(
                    CORINFO_METHOD_HANDLE   method,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return address of fixup area for late-bound PInvoke calls.
    virtual void* __stdcall getAddressOfPInvokeFixup(
                    CORINFO_METHOD_HANDLE   method,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Generate a cookie based on the signature that would needs to be passed
    // to CORINFO_HELP_PINVOKE_CALLI
    virtual LPVOID GetCookieForPInvokeCalliSig(
            CORINFO_SIG_INFO* szMetaSig,
            void           ** ppIndirection = NULL
            ) = 0;

    // Gets a handle that is checked to see if the current method is
    // included in "JustMyCode"
    virtual CORINFO_JUST_MY_CODE_HANDLE __stdcall getJustMyCodeHandle(
                    CORINFO_METHOD_HANDLE       method,
                    CORINFO_JUST_MY_CODE_HANDLE**ppIndirection = NULL
                    ) = 0;

    // Gets a method handle that can be used to correlate profiling data.
    // This is the IP of a native method, or the address of the descriptor struct
    // for IL.  Always guaranteed to be unique per process, and not to move. */
    virtual void __stdcall GetProfilingHandle(
                    CORINFO_METHOD_HANDLE      method,
                    BOOL                      *pbHookFunction,
                    void                     **pEEHandle,
                    void                     **pProfilerHandle,
                    BOOL                      *pbIndirectedHandles
                    ) = 0;

    // returns the offset into the interface table
    virtual unsigned __stdcall getInterfaceTableOffset (
                    CORINFO_CLASS_HANDLE    cls,
                    void                  **ppIndirection = NULL
                    ) = 0;

    //return the address of a pointer to a callable stub that will do the virtual or interface call
    //
    // When inlining methodBeingCompiledHnd should be the originating caller in a sequence of nested
    // inlines, e.g. it is used to determine if the code being generated is domain
    // neutral or not.

    virtual void __stdcall getCallInfo(
                        CORINFO_METHOD_HANDLE   methodBeingCompiledHnd,
                        CORINFO_MODULE_HANDLE   tokenScope,
                        unsigned                methodToken,
                        unsigned                constraintToken, // the type token from a preceding constraint. prefix instruction (if any)
                        CORINFO_CONTEXT_HANDLE  tokenContext,
                        CORINFO_CALLINFO_FLAGS  flags,
                        CORINFO_CALL_INFO *pResult) = 0;

    // Returns TRUE if the Class Domain ID is the RID of the class (currently true for every class
    // except reflection emitted classes and generics)
    virtual BOOL __stdcall isRIDClassDomainID(CORINFO_CLASS_HANDLE cls) = 0;

    // returns the class's domain ID for accessing shared statics
    virtual unsigned __stdcall getClassDomainID (
                    CORINFO_CLASS_HANDLE    cls,
                    void                  **ppIndirection = NULL
                    ) = 0;


    virtual size_t __stdcall getModuleDomainID  (
                    CORINFO_MODULE_HANDLE    module,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // return the data's address (for static fields only)
    virtual void* __stdcall getFieldAddress(
                    CORINFO_FIELD_HANDLE    field,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // registers a vararg sig & returns a VM cookie for it (which can contain other stuff)
    virtual CORINFO_VARARGS_HANDLE __stdcall getVarArgsHandle(
                    CORINFO_SIG_INFO       *pSig,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // Allocate a string literal on the heap and return a handle to it
    virtual InfoAccessType __stdcall constructStringLiteral(
                    CORINFO_MODULE_HANDLE   module,
                    mdToken                 metaTok,
                    void                  **ppInfo
                    ) = 0;

    // (static fields only) given that 'field' refers to thread local store,
    // return the ID (TLS index), which is used to find the begining of the
    // TLS data area for the particular DLL 'field' is associated with.
    virtual DWORD __stdcall getFieldThreadLocalStoreID (
                    CORINFO_FIELD_HANDLE    field,
                    void                  **ppIndirection = NULL
                    ) = 0;

    // returns the class typedesc given a methodTok (needed for arrays since
    // they share a common method table, so we can't use getMethodClass)
    virtual CORINFO_CLASS_HANDLE __stdcall findMethodClass(
                    CORINFO_MODULE_HANDLE   module,
                    mdToken                 methodTok,
                    CORINFO_METHOD_HANDLE   context
                    ) = 0;

    // Sets another object to intercept calls to "self"
    virtual void __stdcall setOverride(
                ICorDynamicInfo             *pOverride
                ) = 0;

    // Adds an active dependency from the context method's module to the given module
    virtual void __stdcall addActiveDependency(
               CORINFO_MODULE_HANDLE       moduleFrom,
               CORINFO_MODULE_HANDLE       moduleTo
                ) = 0;

    virtual CORINFO_METHOD_HANDLE __stdcall GetDelegateCtor(
            CORINFO_METHOD_HANDLE  methHnd,
            CORINFO_CLASS_HANDLE   clsHnd,
            CORINFO_METHOD_HANDLE  targetMethodHnd,
            DelegateCtorArgs *     pCtorData
            ) = 0;

    virtual void __stdcall MethodCompileComplete(
                CORINFO_METHOD_HANDLE methHnd
                ) = 0;
};

/**********************************************************************************/

#define IA64_BUNDLE_SIZE            16

#if defined(_X86_)
#define HARDBOUND_DYNAMIC_CALLS
#endif

#if defined(_X86_) || defined(_PPC_)

#define HELPER_TABLE_ENTRY_LEN      8
#define HELPER_TABLE_ALIGN          8

#else
#error "Unknown target"
#endif  // _IA64_

#endif // _COR_INFO_H_
