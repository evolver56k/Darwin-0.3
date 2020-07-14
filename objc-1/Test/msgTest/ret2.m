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
#include <objc/objc.h>
#include "rettest.h"

#import "ret2.h"

@implementation SuperTest : MsgTest

	// scalar types
#define THIS_CLASS [SuperTest class]
makeSuperMethod(char)
makeSuperMethod(uchar_t)
makeSuperMethod(short)
makeSuperMethod(ushort_t)
makeSuperMethod(int)
makeSuperMethod(unsigned)
makeSuperMethod(long)
makeSuperMethod(ulong_t)
makeSuperMethod(float)
makeSuperMethod(double)
makeSuperMethod(id)
makeSuperMethod(STR)

	// structure types
makeSuperMethod(S_BITS_16_t)
makeSuperMethod(S_BITS_32_t)
makeSuperMethod(S_BITS_64_t)
makeSuperMethod(S_BITS_BIG_t)

	// union types
makeSuperMethod(U_BITS_16_t)
makeSuperMethod(U_BITS_32_t)
makeSuperMethod(U_BITS_64_t)
makeSuperMethod(U_BITS_BIG_t)

	// enum types
makeSuperMethod(E_BITS_8_t)
makeSuperMethod(E_BITS_16_t)
#ifdef INT_32
makeSuperMethod(E_BITS_32_t)
#endif

@end
