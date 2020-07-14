/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/********************************************************************
 ********************************************************************
 **
 **  objc-msg-i386.s - i386 code to support objc messaging.
 **
 ********************************************************************
 ********************************************************************/

// The assembler syntax for an immediate value is the same as the
// syntax for a macro argument number (dollar sign followed by the
// digits).  Argument number wins in this ambiguity.  Until the
// assembler is fixed we have to find another way.
#define NO_MACRO_CONSTS
#ifdef NO_MACRO_CONSTS
	kTwo			= 2
	kEight			= 8
#endif

#if defined(OBJC_COLLECTING_CACHE)
// _objc_entryPoints and _objc_exitPoints are used by objc
// to get the critical regions for which method caches 
// cannot be garbage collected.

	.data
.globl		_objc_entryPoints
_objc_entryPoints:
	.long	_objc_msgSend
	.long	_objc_msgSendSuper
	.long	0

.globl		_objc_exitPoints
_objc_exitPoints:
	.long	LMsgSendExit
	.long	LMsgSendSuperExit
	.long	0
#endif

/********************************************************************
 *
 * Structure definitions.
 *
 ********************************************************************/

// objc_super parameter to sendSuper
	receiver		= 0
	class			= 4

// Selected field offsets in class structure
	isa				= 0
	cache			= 32

// Method descriptor
	method_name		= 0
	method_imp		= 8

// Cache header
	mask			= 0
	occupied		= 4
	buckets			= 8		// variable length array

#if defined(OBJC_INSTRUMENTED)
// Cache instrumentation data, follows buckets
	hitCount		= 0
	hitProbes		= hitCount + 4
	maxHitProbes	= hitProbes + 4
	missCount		= maxHitProbes + 4
	missProbes		= missCount + 4
	maxMissProbes	= missProbes + 4
	flushCount		= maxMissProbes + 4
	flushedEntries	= flushCount + 4

// Buckets in CacheHitHistogram and CacheMissHistogram
	CACHE_HISTOGRAM_SIZE = 512
#endif

//////////////////////////////////////////////////////////////////////
//
// LOAD_STATIC_WORD	targetReg, symbolName, LOCAL_SYMBOL | EXTERNAL_SYMBOL
//
// Load the value of the named static data word.
//
// Takes: targetReg			- the register, other than r0, to load
//	 	  symbolName		- the name of the symbol
//	 	  LOCAL_SYMBOL		- symbol name used as-is
//		  EXTERNAL_SYMBOL	- symbol name gets nonlazy treatment
//
// Eats: edx and targetReg
//////////////////////////////////////////////////////////////////////

// Values to specify whether the symbols is plain or nonlazy
LOCAL_SYMBOL	= 0
EXTERNAL_SYMBOL	= 1

.macro	LOAD_STATIC_WORD

#if defined(__DYNAMIC__)
	call	1f
1:	popl	%edx
.if $2 == EXTERNAL_SYMBOL
	movl	L$1-1b(%edx),$0
	movl	0($0),$0
.elseif $2 == LOCAL_SYMBOL
	movl	$1-1b(%edx),$0
.else
	!!! Unknown symbol type !!!
.endif
#else
	movl	$1,$0
#endif

.endmacro

//////////////////////////////////////////////////////////////////////
//
// LEA_STATIC_DATA	targetReg, symbolName, LOCAL_SYMBOL | EXTERNAL_SYMBOL
//
// Load the address of the named static data.
//
// Takes: targetReg	 - the register, other than edx, to load
//	 symbolName		 - the name of the symbol
//	 LOCAL_SYMBOL	 - symbol is local to this module
//	 EXTERNAL_SYMBOL - symbol is imported from another module
//
// Eats: edx and targetReg
//////////////////////////////////////////////////////////////////////

.macro	LEA_STATIC_DATA
#if defined(__DYNAMIC__)
	call	1f
1:	popl	%edx
.if $2 == EXTERNAL_SYMBOL
	movl	L$1-1b(%edx),$0
.elseif $2 == LOCAL_SYMBOL
	leal	$1-1b(%edx),$0
.else
	!!! Unknown symbol type !!!
.endif
#else
	leal	$1,$0
#endif

.endmacro

//////////////////////////////////////////////////////////////////////
//
// ENTRY		functionName
//
// Assembly directives to begin an exported function.
//
// Takes: functionName - name of the exported function
//////////////////////////////////////////////////////////////////////

.macro ENTRY
	.text
	.globl	$0
	.align	4, 0x90
$0:
.endmacro

//////////////////////////////////////////////////////////////////////
//
// END_ENTRY	functionName
//
// Assembly directives to end an exported function.  Just a placeholder,
// a close-parenthesis for ENTRY, until it is needed for something.
//
// Takes: functionName - name of the exported function
//////////////////////////////////////////////////////////////////////

.macro END_ENTRY
.endmacro

//////////////////////////////////////////////////////////////////////
//
// CALL_MCOUNTER	counterName
//
// Allocate and maintain a counter for the call site.
//
// Takes: counterName - name of counter.
//////////////////////////////////////////////////////////////////////
HAVE_CALL_EXTERN_mcount	= 0

.macro CALL_MCOUNTER
#ifdef PROFILE
	pushl	%ebp
	movl	%esp,%ebp
	LOAD_STATIC_WORD %eax, $0, LOCAL_SYMBOL
.if HAVE_CALL_EXTERN_mcount == 0
HAVE_CALL_EXTERN_mcount = 1
	CALL_EXTERN(mcount)
.else
	CALL_EXTERN_AGAIN(mcount)
.endif
	.data
	.align	2
$0:
	.long	0
	.text
	movl	%ebp,%esp
	popl	%ebp
#endif
.endmacro

/////////////////////////////////////////////////////////////////////
//
//
// CacheLookup	MSG_SEND | MSG_SENDSUPER, cacheMissLabel
//
// Locate the implementation for a selector in a class method cache.
//
// Takes: MSG_SEND	(first parameter is receiver)
//	  MSG_SENDSUPER	(first parameter is address of objc_super structure)
//
//	 cacheMissLabel = label to branch to iff method is not cached
//
// On exit:	(found) imp in eax register
//		(not found) jumps to cacheMissLabel
//	
/////////////////////////////////////////////////////////////////////

// Values to specify to method lookup macros whether the return type of
// the method is an integer or structure.
MSG_SEND		= 0
MSG_SENDSUPER	= 1

.macro	CacheLookup

// load variables and save caller registers.
// Overlapped to prevent AGI
.if $0 == MSG_SEND					// MSG_SEND
	movl	selector(%esp), %ecx	//   get selector
	movl	isa(%eax), %eax			//   class = self->isa
.else								// MSG_SENDSUPER
	movl	super(%esp), %eax		//   get objc_super address
	movl	selector(%esp), %ecx	//   get selector
	movl	class(%eax), %eax		//   class = caller->class
.endif
	pushl	%edi					// save scratch register
	movl	cache(%eax), %eax		// cache = class->cache
	pushl	%esi					// save scratch register
#if defined(OBJC_INSTRUMENTED)
	pushl	%ebx					// save non-volatile register
	pushl	%eax					// save cache pointer
	xorl	%ebx, %ebx				// probeCount = 0
#endif
	leal	buckets(%eax), %edi		// buckets = &cache->buckets
	movl	mask(%eax), %esi		// mask = cache->mask
	movl	%ecx, %edx				// index = selector

// search the receiver's cache
LMsgSendProbeCache_$0_$1:
#if defined(OBJC_INSTRUMENTED)
	inc		%ebx					// probeCount += 1
#endif
	andl	%esi, %edx				// index &= mask
	movl	(%edi, %edx, 4), %eax	// method = buckets[index]

	testl	%eax, %eax				// check for end of bucket
	je		LMsgSendCacheMiss_$0_$1	// go to cache miss code
	cmpl	method_name(%eax), %ecx	// check for method name match
	je		LMsgSendCacheHit_$0_$1	// go handle cache hit
	inc		%edx					// bump index ...
	jmp		LMsgSendProbeCache_$0_$1// ... and loop

// not found in cache: restore state and go to callers handler
LMsgSendCacheMiss_$0_$1:
#if defined(OBJC_INSTRUMENTED)
	popl	%edx					// retrieve cache pointer
	movl	mask(%edx), %esi		// mask = cache->mask
	testl	%esi, %esi				// a mask of zero is only for the...
	je		LMsgSendMissInstrumentDone_$0	// ... emptyCache, do not record anything

	// locate and update the CacheInstrumentation structure
	inc		%esi					// entryCount = mask + 1
#ifdef NO_MACRO_CONSTS
	shll	$kTwo, %esi				// tableSize = entryCount * sizeof(entry)
#else
	shll	$2, %esi				// tableSize = entryCount * sizeof(entry)
#endif
	addl	$buckets, %esi			// offset = buckets + tableSize
	addl	%edx, %esi				// cacheData = &cache->buckets[mask+1]

	movl	missCount(%esi), %edi	// 
	inc		%edi					// 
	movl	%edi, missCount(%esi)	// cacheData->missCount += 1
	movl	missProbes(%esi), %edi	// 
	addl	%ebx, %edi				// 
	movl	%edi, missProbes(%esi)	// cacheData->missProbes += probeCount
	movl	maxMissProbes(%esi), %edi// if (cacheData->maxMissProbes < probeCount)
	cmpl	%ebx, %edi				// 
	jge		LMsgSendMaxMissProbeOK_$0	// 
	movl	%ebx, maxMissProbes(%esi)//	cacheData->maxMissProbes = probeCount
LMsgSendMaxMissProbeOK_$0:

	// update cache miss probe histogram
	cmpl	$CACHE_HISTOGRAM_SIZE, %ebx	// pin probeCount to max index
	jl		LMsgSendMissHistoIndexSet_$0
	movl	$(CACHE_HISTOGRAM_SIZE-1), %ebx
LMsgSendMissHistoIndexSet_$0:
	LEA_STATIC_DATA	%esi, _CacheMissHistogram, EXTERNAL_SYMBOL
#ifdef NO_MACRO_CONSTS
	shll	$kTwo, %ebx				// convert probeCount to histogram index
#else
	shll	$2, %ebx				// convert probeCount to histogram index
#endif
	addl	%ebx, %esi				// calculate &CacheMissHistogram[probeCount<<2]
	movl	0(%esi), %edi			// get current tally
	inc		%edi					// 
	movl	%edi, 0(%esi)			// tally += 1
LMsgSendMissInstrumentDone_$0:
	popl	%ebx					// restore non-volatile register
#endif

.if $0 == MSG_SEND					// MSG_SEND
	popl	%esi					//  restore callers register
	popl	%edi					//  restore callers register
	movl	self(%esp), %eax		//  get messaged object
	movl	isa(%eax), %eax			//  get objects class
.else								// MSG_SENDSUPER
	// replace "super" arg with "receiver"
	movl	super+8(%esp), %edi		//  get super structure
	movl	receiver(%edi), %esi	//  get messaged object
	movl	%esi, super+8(%esp)		//  make it the first argument
	movl	class(%edi), %eax		//  get messaged class
	popl	%esi					//  restore callers register
	popl	%edi					//  restore callers register
.endif

	jmp		$1						// go to callers handler

// eax points to matching cache entry
	.align	4, 0x90
LMsgSendCacheHit_$0_$1:
#if defined(OBJC_INSTRUMENTED)
	popl	%edx					// retrieve cache pointer
	movl	mask(%edx), %esi		// mask = cache->mask
	testl	%esi, %esi				// a mask of zero is only for the...
	je		LMsgSendHitInstrumentDone_$0_$1	// ... emptyCache, do not record anything

	// locate and update the CacheInstrumentation structure
	inc		%esi					// entryCount = mask + 1
#ifdef NO_MACRO_CONSTS
	shll	$kTwo, %esi				// tableSize = entryCount * sizeof(entry)
#else
	shll	$2, %esi				// tableSize = entryCount * sizeof(entry)
#endif
	addl	$buckets, %esi			// offset = buckets + tableSize
	addl	%edx, %esi				// cacheData = &cache->buckets[mask+1]

	movl	hitCount(%esi), %edi
	inc		%edi
	movl	%edi, hitCount(%esi)	// cacheData->hitCount += 1
	movl	hitProbes(%esi), %edi
	addl	%ebx, %edi
	movl	%edi, hitProbes(%esi)	// cacheData->hitProbes += probeCount
	movl	maxHitProbes(%esi), %edi// if (cacheData->maxHitProbes < probeCount)
	cmpl	%ebx, %edi
	jge		LMsgSendMaxHitProbeOK_$0_$1
	movl	%ebx, maxHitProbes(%esi)//	cacheData->maxHitProbes = probeCount
LMsgSendMaxHitProbeOK_$0_$1:

	// update cache hit probe histogram
	cmpl	$CACHE_HISTOGRAM_SIZE, %ebx	// pin probeCount to max index
	jl	LMsgSendHitHistoIndexSet_$0_$1
	movl	$(CACHE_HISTOGRAM_SIZE-1), %ebx
LMsgSendHitHistoIndexSet_$0_$1:
	LEA_STATIC_DATA	%esi, _CacheHitHistogram, EXTERNAL_SYMBOL
#ifdef NO_MACRO_CONSTS
	shll	$kTwo, %ebx				// convert probeCount to histogram index
#else
	shll	$2, %ebx				// convert probeCount to histogram index
#endif
	addl	%ebx, %esi				// calculate &CacheHitHistogram[probeCount<<2]
	movl	0(%esi), %edi			// get current tally
	inc		%edi					// 
	movl	%edi, 0(%esi)			// tally += 1
LMsgSendHitInstrumentDone_$0_$1:
	popl	%ebx					// restore non-volatile register
#endif

// load implementation address, restore state, and we're done
	movl	method_imp(%eax), %eax	// imp = method->method_imp
.if $0 == MSG_SENDSUPER				// MSG_SENDSUPER
	// replace "super" arg with "self"
	movl	super+8(%esp), %edi
	movl	receiver(%edi), %esi
	movl	%esi, super+8(%esp)
.endif

	// restore caller registers
	popl	%esi
	popl	%edi
.endmacro

/////////////////////////////////////////////////////////////////////
//
// MethodTableLookup MSG_SEND | MSG_SENDSUPER
//
// Takes: MSG_SEND	(first parameter is receiver)
//	  MSG_SENDSUPER	(first parameter is address of objc_super structure)
//
// On exit: Register parameters restored from CacheLookup
//	   imp in eax
//
/////////////////////////////////////////////////////////////////////

HAVE_CALL_EXTERN_lookupMethodAndLoadCache	= 0

.macro MethodTableLookup

	// push args (class, selector)
	pushl	%ecx
	pushl	%eax
.if HAVE_CALL_EXTERN_lookupMethodAndLoadCache == 0
HAVE_CALL_EXTERN_lookupMethodAndLoadCache = 1
	CALL_EXTERN(__class_lookupMethodAndLoadCache)
.else
	CALL_EXTERN_AGAIN(__class_lookupMethodAndLoadCache)
.endif
#ifdef NO_MACRO_CONSTS
	addl	$kEight, %esp				// pop parameters
#else
	addl	$8, %esp					// pop parameters
#endif
.endmacro

/********************************************************************
 * id		objc_msgSend	   (id		self,
 *								SEL		op,
 *								...);
 ********************************************************************/

// objc_msgSend arguments
	self		= 4
	selector	= 8

	ENTRY	_objc_msgSend
	CALL_MCOUNTER	LP0

	movl	self(%esp), %eax

// check whether receiver is nil 
	testl	%eax, %eax
	je		LMsgSendNilSelf

#if !defined(OBJC_COLLECTING_CACHE)
// check whether context is multithreaded
	EXTERN_TO_REG(__objc_multithread_mask,%ecx)
	testl	%ecx, %ecx
	je		LMsgSendMT
#endif

// single threaded and receiver is non-nil: search the cache
	CacheLookup MSG_SEND, LMsgSendCacheMiss
	jmp		*%eax					// goto *imp

// cache miss: go search the method lists
LMsgSendCacheMiss:
	MethodTableLookup MSG_SEND
	jmp		*%eax					// goto *imp

#if !defined(OBJC_COLLECTING_CACHE)
// multithreaded: hold _messageLock while accessing cache
LMsgSendMT:
	movl	$1, %ecx				// acquire _messageLock
	LEA_STATIC_DATA	%eax, _messageLock, EXTERNAL_SYMBOL
LMsgSendLockSpin:
	xchgl	%ecx, (%eax)
	cmpl	$0, %ecx
	jne		LMsgSendLockSpin
	movl	self(%esp), %eax		// restore eax

	CacheLookup MSG_SEND, LMsgSendMTCacheMiss
	LEA_STATIC_DATA	%ecx, _messageLock, EXTERNAL_SYMBOL
	movl	$0, (%ecx)				// unlock
	jmp		*%eax					// goto *imp

// cache miss: go search the method lists
LMsgSendMTCacheMiss:
	MethodTableLookup MSG_SEND
	LEA_STATIC_DATA	%ecx, _messageLock, EXTERNAL_SYMBOL
	movl	$0, (%ecx)				// unlock
	jmp		*%eax					// goto *imp
#endif

// message sent to nil object: call optional handler and return nil
LMsgSendNilSelf:
	EXTERN_TO_REG(__objc_msgNil,%eax)
	movl	0(%eax), %eax			// load nil message handler
	testl	%eax, %eax
	je		LMsgSendDone			// if NULL just return and don't do anything
	call	*%eax					// call __objc_msgNil
	xorl	%eax, %eax				// Rezero $eax just in case
LMsgSendDone:
	ret

LMsgSendExit:
	END_ENTRY	_objc_msgSend

/********************************************************************
 * id	objc_msgSendSuper  (struct objc_super *	super,
 *							SEL					op,
 *							...);
 *
 * struct objc_super {
 *		id		receiver;
 *		Class	class;
 * };
 ********************************************************************/

// _objc_msgSendSuper arguments
	super		= 4
//	selector	= 8

	ENTRY	_objc_msgSendSuper

	CALL_MCOUNTER LP1

#if !defined(OBJC_COLLECTING_CACHE)
// check whether context is multithreaded
	EXTERN_TO_REG(__objc_multithread_mask,%eax)
	testl	%eax, %eax
	je		LMsgSendSuperMT
#endif

// single threaded and receiver is non-nil: search the cache
	CacheLookup MSG_SENDSUPER, LMsgSendSuperCacheMiss
	jmp		*%eax					// goto *imp

// cache miss: go search the method lists
LMsgSendSuperCacheMiss:
	MethodTableLookup MSG_SENDSUPER
	jmp		*%eax					// goto *imp

#if !defined(OBJC_COLLECTING_CACHE)
LMsgSendSuperMT:
// multithreaded: hold _messageLock while accessing cache
	movl	$1, %ecx				// acquire _messageLock
	LEA_STATIC_DATA	%eax, _messageLock, EXTERNAL_SYMBOL
LMsgSendSuperLockSpin:
	xchgl	%ecx, (%eax)
	cmpl	$0, %ecx
	jne		LMsgSendSuperLockSpin
	movl	self(%esp), %eax		// restore eax

	CacheLookup MSG_SENDSUPER, LMsgSendSuperMTCacheMiss
	LEA_STATIC_DATA	%ecx, _messageLock, EXTERNAL_SYMBOL
	movl	$0, (%ecx)				// unlock
	jmp		*%eax					// goto *imp

// cache miss: go search the method lists
LMsgSendSuperMTCacheMiss:
	MethodTableLookup MSG_SENDSUPER
	LEA_STATIC_DATA	%ecx, _messageLock, EXTERNAL_SYMBOL
	movl	$0, (%ecx)				// unlock
	jmp		*%eax					// goto *imp
#endif

LMsgSendSuperExit:
	END_ENTRY	_objc_msgSendSuper

/********************************************************************
 * id		_objc_msgForward	(id	self,
 *								SEL	sel,
 *								...);
 ********************************************************************/

// Location LFwdStr contains the string "forward::"
// Location LFwdSel contains a pointer to LFwdStr, that can be changed
// to point to another forward:: string for selector uniquing
// purposes.  ALWAYS dereference LFwdSel to get to "forward::" !!
	.objc_meth_var_names
	.align	2
LFwdStr:.ascii	"forward::\0"

	.objc_message_refs
	.align	2
LFwdSel:.long	LFwdStr

	.cstring
	.align	2
LUnkSelStr:    .ascii	"Does not recognize selector %s\0"

	ENTRY	__objc_msgForward
#if defined(KERNEL)
	trap							// _objc_msgForward is not for the kernel
#else
	pushl   %ebp
	movl    %esp,%ebp
	movl	(selector+4)(%esp), %eax
#if defined(__DYNAMIC__)
	call	L__objc_msgForward$pic_base
L__objc_msgForward$pic_base:
	popl	%edx
	leal	LFwdSel-L__objc_msgForward$pic_base(%edx),%ecx
	cmpl	%ecx, %eax
#else
	cmpl	LFwdSel, %eax
#endif
	je		LMsgForwardError

	leal	(self+4)(%esp), %ecx
	pushl	%ecx
	pushl	%eax
#if defined(__DYNAMIC__)
	movl	LFwdSel-L__objc_msgForward$pic_base(%edx),%ecx
#else
	movl	LFwdSel,%ecx
#endif
	pushl	%ecx
	pushl	(self + 16)(%esp)
	call	_objc_msgSend
	movl    %ebp,%esp
	popl    %ebp
	ret

// call error handler with unrecognized selector message
	.align	4, 0x90
LMsgForwardError:
#if defined(__DYNAMIC__)
	leal	LFwdSel-L__objc_msgForward$pic_base(%edx),%eax
	pushl 	%eax
	leal	LUnkSelStr-L__objc_msgForward$pic_base(%edx),%eax
	pushl 	%eax
#else
	pushl	$LFwdSel
	pushl	$LUnkSelStr
#endif
	pushl	(self + 12)(%esp)
	BRANCH_EXTERN(___objc_error)		// volatile, will not return
#endif

	END_ENTRY	__objc_msgForward

/********************************************************************
 * id		objc_msgSendv  (id			self,
 *							SEL			sel,
 *							unsigned	arg_size,
 *							marg_list	arg_frame);
 ********************************************************************/

// arguments
	self	= 4
	sel		= 8
	size	= 12
	args	= 16

	ENTRY	_objc_msgSendv

#if defined(KERNEL)
	trap							// _objc_msgSendv is not for the kernel
#else
	pushl	%ebp
	movl	%esp, %ebp
	movl	(args + 4)(%ebp), %edx
	addl	$8, %edx				// skip self & selector
	movl	(size + 4)(%ebp), %ecx
	subl	$5, %ecx				// skip self & selector
	shrl	$2, %ecx
	jle		LMsgSendvArgsOK
LMsgSendvArgLoop:
	decl	%ecx
	movl	0(%edx, %ecx, 4), %eax
	pushl	%eax
	jg		LMsgSendvArgLoop

LMsgSendvArgsOK:
	movl	(sel + 4)(%ebp), %ecx
	pushl	%ecx
	movl	(self + 4)(%ebp),%ecx
	pushl	%ecx
	call	_objc_msgSend
	movl	%ebp,%esp
	popl	%ebp

	ret
#endif

	END_ENTRY	_objc_msgSendv
