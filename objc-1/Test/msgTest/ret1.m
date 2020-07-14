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
// A class to test all possible return types

#include <objc/objc.h>
#include "rettest.h"

#import "ret1.h"

@implementation MsgTest : Object 

#include <stdio.h>

// Exit handler
+ retTest$exitCleanup:arg/*;*/, int estat {
	extern myCleanup();
	myCleanup(estat, arg);
}
// enable trap catching and install an exit handler
+ initialize {
	extern (*_onExit)();

	if (self == [MsgTest class]) {
		// enable trap catching
		//settrap();

		// arrange to get sent a message on exit.
		//(*_onExit)(self,@selector(retTest$exitCleanup:),(id)0);
	}
	return self;
}

	// scalar types
#define THIS_CLASS [MsgTest class]
makeMethod(char,99,;)
makeMethod(uchar_t, 199,;)
makeMethod(short, 1999,;)
makeMethod(ushort_t, 19999,;)
makeMethod(int, 2999,;)
makeMethod(unsigned, 29999,;)
makeMethod(long, 399999,;)
makeMethod(ulong_t, 3999999,;)
makeMethod(float, 3.1416,;)
makeMethod(double, 2.7123456789876,;)
makeMethod(id,self,;)
makeMethod(STR,junk,char* junk = "hello there")


	//
	// structure-returning methods
	//
static struct S_BITS_16 extStructBits16 = { 101, 102 };
static struct S_BITS_32 extStructBits32 = { 103, 104, 105, 106 };
static struct S_BITS_64 extStructBits64 =
	{ 110, 111, 112, 113, 114, 115, 116, 117 };
static struct S_BITS_BIG extStructBitsBig ={
	0,1,2,3,4,5,6,7,
	8,9,10,11,12,13,14,15, 16,17,18,19,20,21,22,23,
	24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,
	40,41,42,43,44,45,46,47,
	48,49,50,51,52,53,54,55,
	56,57,58,59,60,61,62,63,
};

makeMethod(S_BITS_16_t,junk,S_BITS_16_t junk; junk = extStructBits16)
makeMethod(S_BITS_32_t,junk,S_BITS_32_t junk; junk = extStructBits32)
makeMethod(S_BITS_64_t,junk,S_BITS_64_t junk; junk = extStructBits64)
makeMethod(S_BITS_BIG_t,junk,S_BITS_BIG_t junk; junk = extStructBitsBig)

	//
	// union-returning methods
	//

makeMethod(U_BITS_16_t,junk,U_BITS_16_t junk; junk.mem = extStructBits16)
makeMethod(U_BITS_32_t,junk,U_BITS_32_t junk; junk.mem = extStructBits32)

#if 1
makeMethod(U_BITS_64_t,junk,U_BITS_64_t junk; junk.mem = extStructBits64)
#else
// deliberately make bad method to see if diagnostics work
- (U_BITS_64_t) ret_U_BITS_64_t:(U_BITS_64_t*) retRef {
	U_BITS_64_t junk;
	junk.mem = extStructBits64;
	// force a fault
	*(int*)0 = *(int*)0;
	return junk;
}
#endif

makeMethod(U_BITS_BIG_t,junk,U_BITS_BIG_t junk; junk.mem = extStructBitsBig)

	// Enum types

makeMethod(E_BITS_8_t,junk,E_BITS_8_t junk; junk = e8_two)
makeMethod(E_BITS_16_t,junk,E_BITS_16_t junk; junk = e16_two)
#ifdef INT_32
makeMethod(E_BITS_32_t,junk,E_BITS_32_t junk; junk = e32_two)
#endif
