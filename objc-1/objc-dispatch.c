/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *	objc-dispatch.c
 *	Copyright 1988, NeXT, Inc.
 *	Author: s.naroff & s.stone
 *
 * 	Purpose: Compute implementation for method, and dispatch there,
 *		 leaving all arguments undisturbed on the stack.
 *
 *	Implementation:
 *
 *	- caches are maintained on a `per-class' basis.
 *	- caches are allocated dynamically, sized proportional to all the
 *	methods that the class can respond to.
 *	- cache entries cost 4 bytes each. This saves memory and enables
 *	a cache load to be accomplished in an atomic assignment operation,
 *	removing the expensive `tas' operation. Because a cache load in the
 *	Stepstone messager consisted of 3 assignment operations <cls,sel,imp>	
 *	they needed this instruction to enforce concurrency lockouts on 
 *	cache slot access. This insured the `integrity' of the cache
 *	when sending messages from interrupt handlers. This makes the 
 *	dispatcher more portable, considering many machines do not have or
 *	support (i.e. apollo) this instruction.
 *	- this cache supports collision resolution. Because we can estimate 
 *	the number of elements in the cache (and are concerned about data 
 *	space overhead), we use a simple open-addressing method called 
 *	`linear probing'. If all probes are filled, we replace one of them.
 *	In this implementation, the length of a chain will be 8.
 *	- _msgSuper DOES NOT depend on retreiving `self' from the previous
 *	stack frame.
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include "objc.h"
#include "objc-private.h"
#include "objc-dispatch.h"
extern id (*_objc_msgPreop)(MSG, int, int);

#define JMP(imp) \
	/* load function address */ \
	asm ("movel %0,a0" : "=m" (imp)); \
	/* restore a1...for methods that return > 8 bytes quantities */ \
	asm ("movel %0,a1" : "=m" (save_a1)); \
	/* unlink stack frame */ \
        asm ("unlk a6"); \
	/* go to method */ \
	asm ("jmp a0@"); 		 

#if 0
/* 
 * Implement [anObject aSelector] 
 */
id objc_msgSend(id self, SEL sel, ...) 
{ 
	Class cls; 
	Method *buckets;
	int index, mask, i;
	IMP imp;
	void *save_a1;

	asm ("movel a1,%0" : "=m" (save_a1));

	/* This provides the innocuous semantics for nil messages.
	 * Changing this to an error would reliably trap messages
	 * to nil objects.
	 */
	if (self == nil)
		return nil;

	cls = self->isa;

	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = sel & mask;
	for (;;) {
	    if (buckets[index] && (buckets[index]->method_name == sel))
	      goto cacheHit;
	    if (! buckets[index]) goto cacheMiss;
	    index++;
	    index &= mask;
	}
cacheMiss:
	imp = _class_lookupMethodAndLoadCache(cls, sel);

	if (_objc_msgPreop)
	  (*_objc_msgPreop)(GETFRAME(self), NO, NO);

	JMP(imp);

cacheHit:
	imp = buckets[index]->method_imp;

	if (_objc_msgPreop)
	  (*_objc_msgPreop)(GETFRAME(self), NO, YES);

	JMP(imp);
}
#endif
/* 
 * Implement [super aSelector] 
 */
id objc_msgSendSuper(struct objc_super *caller, SEL sel, ...)
{
	id self; Class cls;
	Method *buckets;
	int index, mask, i;
	IMP imp;
	void *save_a1;

	asm ("movel a1,%0" : "=m" (save_a1));

	/* Fetch the caller's value for "self". */
	cls = caller->class;

	/* replace `caller' with the real receiver */
	caller = (struct objc_super *)self = caller->receiver;

	/* Unlikely, but not impossible */
	if (self == nil)
		return nil;

	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = sel & mask;
	for (;;) {
	    if (buckets[index] && (buckets[index]->method_name == sel))
	      goto cacheHit;
	    if (! buckets[index]) goto cacheMiss;
	    index++;
	    index &= mask;
	}

cacheMiss:
	imp = _class_lookupMethodAndLoadCache(cls, sel);

	if (_objc_msgPreop)
	  (*_objc_msgPreop)(GETFRAME(self), YES, NO);

	JMP(imp);

cacheHit:
	imp = buckets[index]->method_imp;

	if (_objc_msgPreop)
	  (*_objc_msgPreop)(GETFRAME(self), YES, YES);

	JMP(imp);
}

