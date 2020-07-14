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
	NXCharacterSet.m
	Copyright 1991, NeXT, Inc.
	Responsibility:
*/
#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXCharacterSet.h"

// Although this implementation will work for unicodes, it should be more memory efficient
// while still keeping the O(1) nature. (For instance, a two-level tree; NULLs in the first
// level could indicate 256 zeros, -1 can indicate 256 1s; otherwise a pointer to a real
// block of 256 bits...)

@implementation NXCharacterSet

// ??? The next two numbers are CPU dependant.

#define BITSPERINT	32	/* (CHAR_BIT * sizeof(unsigned int)) */
#define LOGBITSPERINT	5

// ??? Depends on definition of unichar

#define NUMCHARACTERS	256

// Number of ints in the array keeping the bits.

#define NUMINTS		(NUMCHARACTERS / sizeof(unsigned int))

#define ADDCH(ch) _bits[(ch) >> LOGBITSPERINT] |= (((unsigned)1) << (ch & (BITSPERINT - 1)))
#define DELCH(ch) _bits[(ch) >> LOGBITSPERINT] &= ~(((unsigned)1) << (ch & (BITSPERINT - 1)))
#define MEMCH(ch) ((_bits[(ch) >> LOGBITSPERINT] & (((unsigned)1) << (ch & (BITSPERINT - 1)))) ? YES : NO)

- init
{
    unsigned int cnt;
    
    [super init];
    _bits = (unsigned int *)NXZoneMalloc([self zone], NUMINTS * sizeof(unsigned int));
    for (cnt = 0; cnt < NUMINTS; cnt++) _bits[cnt] = 0;   
    return self;
}

- (BOOL)characterIsMember:(unichar)ch
{
    return MEMCH(ch);
}

- addCharacters:(const unichar *)chars length:(unsigned)len
{
    unsigned int cnt;
    
    for (cnt = 0; cnt < len; cnt++) {
        ADDCH(chars[cnt]);
    }
    return self;
}

- removeCharacters:(const unichar *)chars length:(unsigned)len
{
    unsigned int cnt;
    
    for (cnt = 0; cnt < len; cnt++) {
        DELCH(chars[cnt]);
    }
    return self;
}    

- addRange:(unichar)from :(unichar)to
{
    unichar ch;
    for (ch = from; ch < to; ch++) {
        ADDCH(ch);
    }
    return self;
}

- removeRange:(unichar)from :(unichar)to;
{
    unichar ch;
    for (ch = from; ch < to; ch++) {
        DELCH(ch);
    }
    return self;
}

- invert
{
    int cnt;
    for (cnt = NUMINTS - 1; cnt >= 0; cnt--) {
	_bits[cnt] = ~_bits[cnt];
    }
    return self;
}

- unionWith:(const NXCharacterSet *)otherSet
{
    unsigned int *otherBits = ((struct {@defs(NXCharacterSet);} *)otherSet)->_bits;
    int cnt;
    for (cnt = NUMINTS - 1; cnt >= 0; cnt--) {
	_bits[cnt] |= otherBits[cnt];	
    }
    return self;
}

- intersectWith:(const NXCharacterSet *)otherSet
{
    unsigned int *otherBits = ((struct {@defs(NXCharacterSet);} *)otherSet)->_bits;
    int cnt;
    for (cnt = NUMINTS - 1; cnt >= 0; cnt--) {
	_bits[cnt] &= otherBits[cnt];	
    }
    return self;
}

- copyFromZone:(NXZone *)zone
{
    NXCharacterSet *newInstance = [super copyFromZone:zone];
    newInstance->_bits = (unsigned int *)NXZoneMalloc(zone, NUMINTS * sizeof(unsigned int));
    bcopy (_bits, newInstance->_bits, NUMINTS * sizeof(unsigned int));
    return newInstance;
}

- free
{
    free (_bits);
    return [super free];
}

// ??? Doesn't work properly on little-endian machines. Also very wasteful of space;
// Need to write these out compressed.

- write:(NXTypedStream *)s
{
    [super write:s];
    NXWriteArray (s, "i", NUMINTS, &_bits);
    return self;
}

- read:(NXTypedStream *)s
{
    [super read:s];
    NXReadArray (s, "i", NUMINTS, &_bits);
    return self;
}

@end

/*

Created: Jul 91 aozer (Seperated NXCharacterSet code from NXString.m)

Modifications:
--------------

*/


#endif