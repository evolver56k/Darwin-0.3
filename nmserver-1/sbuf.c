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

#include <stdio.h>

#include "mem.h"
#include "netmsg.h"
#include "nm_extra.h"
#include "sbuf.h"


/*
 * sbuf_grow
 *	Increase the number of segments in an sbuf.
 *
 * Parameters:
 *	sbuf_ptr	: a pointer to the sbuf to grow
 *	inc		: the number of new segments required
 *
 * Design:
 *	Allocates space for the new segments.
 *	Copies the entries from the old segments into the new ones.
 *	Deallocates the old segments.
 *	Inserts the new segments into the sbuf.
 *
 */
PUBLIC void sbuf_grow(sbuf_ptr, inc)
register sbuf_ptr_t	sbuf_ptr;
int			inc;
{
    register sbuf_seg_ptr_t	new_segs, old_seg_ptr, new_seg_ptr;
    register int		segs_used;
    int				num_segs;

    /*
     * Allocate space for the new segments.
     */
    num_segs = sbuf_ptr->size + inc;
    MEM_ALLOC(new_segs,sbuf_seg_ptr_t,(num_segs * sizeof(struct sbuf_seg)), FALSE);

    /*
     * Copy the old segments into the new segments.
     * Also sets new_seg_ptr to point at the first unused segment in new_segs.
     */
    old_seg_ptr = sbuf_ptr->segs;
    new_seg_ptr = new_segs;
    segs_used = 0;
    for (; old_seg_ptr != sbuf_ptr->end; old_seg_ptr++, new_seg_ptr++) {
	*new_seg_ptr = *old_seg_ptr;
	segs_used ++;
    }

    /*
     * Dellocate the old segments.
     */
    MEM_DEALLOC((pointer_t)sbuf_ptr->segs, (sbuf_ptr->size * sizeof(struct sbuf_seg)));

    /*
     * Place the new segments into the sbuf.
     */
    sbuf_ptr->segs = new_segs;
    sbuf_ptr->end = new_seg_ptr;
    sbuf_ptr->free = num_segs - segs_used;
    sbuf_ptr->size = num_segs;

}



/*
 * sbuf_printf
 *	print out an sbuf
 *
 * Parameters:
 *	where	: output file
 *	sb_ptr	: a pointer to the sbuf to print
 *
 */
EXPORT void sbuf_printf(where, sb_ptr)
FILE		*where;
sbuf_ptr_t	sb_ptr;
{
    sbuf_seg_ptr_t	seg_ptr;

    fprintf(where, "sbuf at %d:\n", (int)sb_ptr);
    fprintf(where, "  end = %d, segs = %d, free = %d & size = %d\n",
		 (int)sb_ptr->end, (int)sb_ptr->segs, sb_ptr->free, sb_ptr->size);
    seg_ptr = sb_ptr->segs;
    for (; seg_ptr != sb_ptr->end; seg_ptr++) {
	fprintf(where, "    segment at %d: data = %d, size = %d.\n",
			(int)seg_ptr, (int)seg_ptr->p, (int)seg_ptr->s);
    }
    RET;

}
