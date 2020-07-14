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
/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   libc/i386/ansi/memcpy.c
 *      Author: Bruce Martin, NeXT Computer, Inc.
 *
 *      This file contains machine dependent code for block copies
 *      on NeXT i386-based products.  Currently tuned for the i486.
 *
 * HISTORY
 * 12-Aug-92  Bruce Martin (Bruce_Martin@NeXT.COM)
 *	Created.
 */


/*
 * 486 Factoids:
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
#define BYTES_PAST_INT_ALIGNMENT(voidp)	(UNS(voidp) & (sizeof(int) - 1))

#define CHAR(voidp, offset)	(*(char *)(UNS(voidp) + (offset)))

static inline void
simple_fwd_char_copy(void *dst, const void *src, int len)
{
    asm("rep; movsb"
	: /* no outputs */
#if NX_CURRENT_COMPILER_RELEASE==NX_COMPILER_RELEASE_3_1
	: "&c" (len), "D" (dst), "e" (src)
#else
	: "&c" (len), "D" (dst), "S" (src)
#endif
	: "ecx", "esi", "edi");
}

static inline void
simple_fwd_int_copy(void *dst, const void *src, int len)
{
    asm("rep; movsl"
	: /* no outputs */
#if NX_CURRENT_COMPILER_RELEASE==NX_COMPILER_RELEASE_3_1
	: "&c" (len>>2), "D" (dst), "e" (src)
#else
	: "&c" (len>>2), "D" (dst), "S" (src)
#endif
	: "ecx", "esi", "edi");
}


void *memcpy(void *dst, const void *src, unsigned long ulen)
{
    int len = ulen;
    int need_to_move;
    void *orig_dst = dst;

    /* Always do a forward copy */

    if (len < MIN_TO_ALIGN) {
	simple_fwd_char_copy(dst, src, len);
	return orig_dst;
    }

    if (need_to_move = BYTES_PAST_INT_ALIGNMENT(src)) {
	need_to_move = sizeof(int) - need_to_move;
	simple_fwd_char_copy(dst, src, need_to_move);
	len -= need_to_move;
	UNS(dst) += need_to_move;
	UNS(src) += need_to_move;
    }

    simple_fwd_int_copy(dst, src, len);
	
    if (need_to_move = (len & 3)) {
	UNS(src) += (len & ~3);
	UNS(dst) += (len & ~3);
	switch(need_to_move) {
	    case 3: CHAR(dst, 2) = CHAR(src, 2);
	    case 2: CHAR(dst, 1) = CHAR(src, 1);
	    case 1: CHAR(dst, 0) = CHAR(src, 0);
	}
    }
    return orig_dst;
}
