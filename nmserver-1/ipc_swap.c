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
/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#include	<stdio.h>

#include	"ipc_swap.h"
#include	"netmsg.h"
#include	"nm_extra.h"
#include	"sbuf.h"


/*
 * swap_long_sbuf -- 
 *
 * Copy and byte-swap several long words from an sbuf into a continuous buffer. 
 *
 * Parameters: 
 *
 * sb_ptr: pointer to the source sbuf. 
 *
 * pos_ptr: pointer to the sbuf_pos structure specifying the starting point. 
 *
 * to_ptr: pointer to the pointer specifying the destination. 
 *
 * count: number of words to copy. 
 *
 * Results: 
 *
 * pos_ptr ans to_ptr updated. 
 *
 * Side effects: 
 *
 * Performs the specified copy and byte-swap. May cause a panic if the sbuf does
 * not contain enough data. 
 *
 * Note: 
 *
 */
PUBLIC void
swap_long_sbuf(IN sb_ptr, INOUT pos_ptr, INOUT to_ptr, IN count)
	sbuf_ptr_t	sb_ptr;
	sbuf_pos_t	*pos_ptr;
	unsigned long	**to_ptr;
	int		count;
{
	register int		cur_left;
	int			end_bytes;
	register unsigned long	*f;
	register unsigned long	*t;
SWAP_DECLS;

t = *to_ptr;

/*
 * Iterate over several sbuf segments. 
 */
cur_left = pos_ptr->data_left >> 2;
f = (unsigned long *) pos_ptr->data_ptr;
while (count > cur_left) {
	/*
	 * Copy as many full words as possible from the current segment. 
	 */
	count -= cur_left;
	end_bytes = pos_ptr->data_left - (cur_left << 2);
	for (; cur_left; cur_left--, f++, t++) {
		(void) SWAP_LONG(*f, *t);
	}

	/*
	 * Cross over to the next segment. 
	 */
	pos_ptr->seg_ptr++;
	if (count && (pos_ptr->seg_ptr == sb_ptr->end))
		panic("swap_long_sbuf");
	if (end_bytes) {
		register char  *fc = (char *) f;
		register char  *tc = (char *) t;
		register int    cnt = end_bytes;

		for (; cnt; cnt--, fc++, tc++)
			*tc = *fc;
		fc = (char *) pos_ptr->seg_ptr->p;
		cnt = 4 - end_bytes;
		pos_ptr->data_left = pos_ptr->seg_ptr->s - cnt;
		for (; cnt; cnt--, fc++, tc++)
			*tc = *fc;
		(void) SWAP_LONG(*t, *t);
		t++;
		count--;
		f = (unsigned long *) fc;
	} else {
		pos_ptr->data_left = pos_ptr->seg_ptr->s;
		f = (unsigned long *) pos_ptr->seg_ptr->p;
	}

	cur_left = pos_ptr->data_left >> 2;
}

/*
 * Get the remaining words from the beginning of the current segment. 
 */
if (count) {
	pos_ptr->data_left -= count << 2;
	for (; count; count--, f++, t++) {
		(void) SWAP_LONG(*f, *t);
	}
}
pos_ptr->data_ptr = (pointer_t) f;
*to_ptr = t;

RET;
}

PUBLIC void
swap_long_long_sbuf(IN sb_ptr, INOUT pos_ptr, INOUT to_ptr, IN count)
	sbuf_ptr_t	sb_ptr;
	sbuf_pos_t	*pos_ptr;
	unsigned long long
			**to_ptr;
	int		count;
{
	register int		cur_left;
	int			end_bytes;
	register unsigned long long	*f;
	register unsigned long long	*t;
SWAP_DECLS;

t = *to_ptr;

/*
 * Iterate over several sbuf segments. 
 */
cur_left = pos_ptr->data_left >> 3;
f = (unsigned long long *) pos_ptr->data_ptr;
while (count > cur_left) {
	/*
	 * Copy as many full words as possible from the current segment. 
	 */
	count -= cur_left;
	end_bytes = pos_ptr->data_left - (cur_left << 3);
	for (; cur_left; cur_left--, f++, t++) {
		 SWAP_LONG_LONG_PTR(f, t);
	}

	/*
	 * Cross over to the next segment. 
	 */
	pos_ptr->seg_ptr++;
	if (count && (pos_ptr->seg_ptr == sb_ptr->end))
		panic("swap_long_sbuf");
	if (end_bytes) {
		register char  *fc = (char *) f;
		register char  *tc = (char *) t;
		register int    cnt = end_bytes;

		for (; cnt; cnt--, fc++, tc++)
			*tc = *fc;
		fc = (char *) pos_ptr->seg_ptr->p;
		cnt = 8 - end_bytes;
		pos_ptr->data_left = pos_ptr->seg_ptr->s - cnt;
		for (; cnt; cnt--, fc++, tc++)
			*tc = *fc;
		SWAP_LONG_LONG_PTR(t, t);
		t++;
		count--;
		f = (unsigned long long *) fc;
	} else {
		pos_ptr->data_left = pos_ptr->seg_ptr->s;
		f = (unsigned long long *) pos_ptr->seg_ptr->p;
	}

	cur_left = pos_ptr->data_left >> 3;
}

/*
 * Get the remaining words from the beginning of the current segment. 
 */
if (count) {
	pos_ptr->data_left -= count << 3;
	for (; count; count--, f++, t++) {
		SWAP_LONG_LONG_PTR(f, t);
	}
}
pos_ptr->data_ptr = (pointer_t) f;
*to_ptr = t;

RET;
}



/*
 * swap_short_sbuf -- 
 *
 * Copy and byte-swap several short words from an sbuf into a continuous buffer. 
 *
 * Parameters: 
 *
 * sb_ptr: pointer to the source sbuf. 
 *
 * pos_ptr: pointer to the sbuf_pos structure specifying the starting point. 
 *
 * to_ptr: pointer to the pointer specifying the destination. 
 *
 * count: number of words to copy. 
 *
 * Results: 
 *
 * pos_ptr ans to_ptr updated. 
 *
 * Side effects: 
 *
 * Performs the specified copy and byte-swap. May cause a panic if the sbuf does
 * not contain enough data. 
 *
 * Note: 
 *
 */
PUBLIC void
swap_short_sbuf(IN sb_ptr, INOUT pos_ptr, INOUT to_ptr, IN count)
	sbuf_ptr_t	sb_ptr;
	sbuf_pos_t	*pos_ptr;
	unsigned short	**to_ptr;
	int		count;
{
	register int		cur_left;
	int			end_bytes;
	register unsigned short	*f;
	register unsigned short	*t;
SWAP_DECLS;

t = *to_ptr;

/*
 * Iterate over several sbuf segments. 
 */
cur_left = pos_ptr->data_left >> 1;
f = (unsigned short *) pos_ptr->data_ptr;
while (count > cur_left) {
	/*
	 * Copy as many short words as possible from the current segment. 
	 */
	count -= cur_left;
	end_bytes = pos_ptr->data_left - (cur_left << 1);
	for (; cur_left; cur_left--, f++, t++) {
		(void) SWAP_SHORT(*f, *t);
	}

	/*
	 * Cross over to the next segment. 
	 */
	pos_ptr->seg_ptr++;
	if (count && (pos_ptr->seg_ptr == sb_ptr->end))
		panic("swap_short_sbuf");
	if (end_bytes) {
		register char  *fc = (char *) f;
		register char  *tc = (char *) t;
		*tc = *fc;
		tc++;
		fc = (char *) pos_ptr->seg_ptr->p;
		*tc = *fc;
		fc++;
		(void) SWAP_SHORT(*t, *t);
		t++;
		count--;
		pos_ptr->data_left = pos_ptr->seg_ptr->s - 1;
		f = (unsigned short *) fc;
	} else {
		pos_ptr->data_left = pos_ptr->seg_ptr->s;
		f = (unsigned short *) pos_ptr->seg_ptr->p;
	}

	cur_left = pos_ptr->data_left >> 1;
}

/*
 * Get the remaining words from the beginning of the current segment. 
 */
if (count) {
	pos_ptr->data_left -= count << 1;
	for (; count; count--, f++, t++) {
		(void) SWAP_SHORT(*f, *t);
	}
}
pos_ptr->data_ptr = (pointer_t) f;
*to_ptr = t;

RET;
}
