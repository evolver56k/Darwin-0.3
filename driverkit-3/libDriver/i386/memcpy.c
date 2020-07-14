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
/* Copyright (c) 1992 by NeXT Computer, Inc.
 *
 *      File:  memcpy.c
 *
 * HISTORY
 * 16-Aug-93  John Immordino at NeXT
 *      Created (stolen from mk/machdep/i386/libc/memcpy.c)
 */

#import <driverkit/driverTypes.h>

/*
 * 486 Facts:
 *  - if (move <= 16 bytes) use unrolled mem-mem mov instructions
 *  - if (move > 16 bytes) use "rep movs" [this avoids prefetch stalls]
 *  - Aligned read is better than aligned write. Both aligned is even better.
 */

#define MIN_TO_ALIGN	16

/*
 * Convert a void * into a unsigned int so arithmetic can be done
 * (Technically, the gnu C compiler is tolerant of arithmetic
 * on void *, but ansi isn't; so we do this.)
 */
#define UNS(voidp)      ((unsigned int)(voidp))

/*
 * Number of bytes addr is past an integer alignment
 */
#define BYTES_PAST_ALIGNMENT(voidp, boundary) (UNS(voidp) & (boundary - 1))

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif

#define CHAR(voidp, offset)    (*(char *)(UNS(voidp) + (offset)))

static __inline__ void
simple_fwd_char_copy(void *dst, const void *src, int len)
{
    asm("rep; movsb"
	: /* no outputs */
	: "&c" (len), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

static __inline__ void
simple_fwd_short_copy(void *dst, const void *src, int len)
{
    asm("rep; movsw"
	: /* no outputs */
	: "&c" (len>>1), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}

static __inline__ void
simple_fwd_int_copy(void *dst, const void *src, int len)
{
    asm("rep; movsl"
	: /* no outputs */
	: "&c" (len>>2), "D" (dst), SIREG (src)
	: "ecx", "esi", "edi");
}


void _IOCopyMemory(char *src, char *dst, unsigned copyLen, 
	unsigned copyUnitSize)
{
    int need_to_move;

    if (src == NULL || dst == NULL || src == dst || copyLen == 0)
    	return;
    
    /* 
     * Sanity check copyUnitSize
     */
    if (copyUnitSize == 0)
    	copyUnitSize = 1;
    if (copyUnitSize > 2)
    	copyUnitSize = 4;
    
    /* 
     * Always do a forward copy.  See if we need 
     * a byte-size copy. 
     */
    if (copyUnitSize == 1) {
	simple_fwd_char_copy(dst, src, (int)copyLen);
	return;
    }
    
    /* 
     * Copy bytes up to a copyUnitSize boundary. 
     */
    if (need_to_move = BYTES_PAST_ALIGNMENT(src, copyUnitSize)) {
	need_to_move = copyUnitSize - need_to_move;
	simple_fwd_char_copy(dst, src, need_to_move);
	copyLen -= need_to_move;
	UNS(dst) += need_to_move;
	UNS(src) += need_to_move;
    }

    /* 
     * Copy what we can at maximum specified size. 
     */
    if (copyUnitSize == 2)
    	simple_fwd_short_copy(dst, src, (int)copyLen);
    else
    	simple_fwd_int_copy(dst, src, (int)copyLen);

    /* 
     * Get the leftovers. 
     */
    if (need_to_move = (copyLen & (copyUnitSize - 1))) {
	UNS(src) += (copyLen & ~(copyUnitSize - 1));
	UNS(dst) += (copyLen & ~(copyUnitSize - 1));
	switch(need_to_move) {
	    case 3: CHAR(dst, 2) = CHAR(src, 2);
	    case 2: CHAR(dst, 1) = CHAR(src, 1);
	    case 1: CHAR(dst, 0) = CHAR(src, 0);
	}
    }
}
