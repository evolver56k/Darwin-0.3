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

/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   libc/i386/ansi/memcmp.c
 *      Author: Bruce Martin, NeXT Computer, Inc.
 *
 *      This file contains machine dependent code for block compares
 *      on NeXT i386-based products.  Currently tuned for the i486.
 *
 * HISTORY
 * 12-Aug-92  Bruce Martin (Bruce_Martin@NeXT.COM)
 *	Created.
 */

/*
 * 486 factoids:
 *	- Using 'repe cmp' is a very marginal win.  You can easily beat it
 *	with a aggressively unrolled loop, but that typically has much more
 *	setup time.  So, we go with less setup, assuming shorter comparisons
 *	will be the rule.
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

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif


static inline char
simple_fwd_char_cmp(const char *s1, const char *s2, int len)
{
    int diff;
    asm("	repe; cmpsb
		jz 1f
		movb -1(%2), %1
		movb -1(%3), %2
		subb %2, %1
	1:	movl %1, %0
	"
	: "=r" (diff)
	: "c" (len), "D" (s1), SIREG (s2)
	: "ecx", "edi", "esi");
    return diff;
}

static inline int
simple_fwd_int_cmp(const int *s1, const int *s2, int len)
{
    int diff;
    len >>= 2;
    asm("	repe; cmpsl
		jz 1f
		movl -4(%2), %1
		movl -4(%3), %2
		subl %2, %1
	1:	movl %1, %0
	"
	: "=r" (diff)
	: "c" (len), "D" (s1), SIREG (s2)
	: "ecx", "edi", "esi");
    return diff;
}


int memcmp(const void *, const void *, unsigned long);

int bcmp(const void *src1, const void *src2, unsigned long ulen)
{
    return memcmp(src1, src2, ulen);
}


int memcmp(const void *s1, const void *s2, unsigned long ulen)
{
    int len = ulen;
    int need_to_cmp;
    int res;
    
    if (len == 0)
    	return 0;

    /* Always do a forward compare */

    if (len < MIN_TO_ALIGN) {
	return simple_fwd_char_cmp(s1, s2, len);
    }

    if (need_to_cmp = BYTES_PAST_INT_ALIGNMENT(s2)) {
	need_to_cmp = sizeof(int) - need_to_cmp;
	if (res = simple_fwd_char_cmp(s1, s2, need_to_cmp))
	    return res;
	len -= need_to_cmp;
	UNS(s1) += need_to_cmp;
	UNS(s2) += need_to_cmp;
    }

    res = simple_fwd_int_cmp(s1, s2, len);
	
    if (!res && (need_to_cmp = (len & 3))) {
	UNS(s2) += (len & ~3);
	UNS(s1) += (len & ~3);
	res = simple_fwd_char_cmp(s1, s2, need_to_cmp);
    }
    return res;
}
