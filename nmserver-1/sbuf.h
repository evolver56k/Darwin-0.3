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
#ifndef	_SBUF_
#define	_SBUF_

#include <mach/mach.h>
#include "mem.h"

extern void sbuf_grow();
/*
sbuf_ptr_t		sbuf_ptr;
int			inc;
*/

extern void sbuf_printf();
/*
FILE			where
sbuf_ptr_t		sbuf_ptr;
*/


typedef struct sbuf_seg {	/* one segment of an sbuf */
	pointer_t       p;	/* pointer to start of data */
	unsigned long   s;	/* data size in bytes */
} sbuf_seg_t;

typedef sbuf_seg_t *sbuf_seg_ptr_t;

typedef struct sbuf {
	sbuf_seg_ptr_t  end;	/* first unused segment at the end */
	sbuf_seg_ptr_t  segs;	/* pointer to the list of segments */
	int             free;	/* number of free segments at the end of the list */
	int             size;	/* number of segments allocated in the list (for GC) */
} sbuf_t;

typedef sbuf_t *sbuf_ptr_t;


/*
 * Initialize an empty sbuf of a given size (# of segments).
 */
#define SBUF_INIT(sb,sz) {						\
	sbuf_seg_ptr_t	_s;						\
									\
	MEM_ALLOC(_s,sbuf_seg_ptr_t,(int)(sz) * sizeof(sbuf_seg_t),FALSE);\
	(sb).end = (sb).segs = _s;					\
	(sb).free = (sz);						\
	(sb).size = (sz);						\
}

/*
 * Initialise an empty sbuf with one segment (given).
 */
#define SBUF_SEG_INIT(sb,sp) {						\
	(sb).end = (sb).segs = (sp);					\
	(sb).free = (sb).size = 1;					\
}

/*
 * Reinitialize an sbuf.
 */
#define SBUF_REINIT(sb) {						\
	(sb).end = (sb).segs;						\
	(sb).free = (sb).size;						\
}

/*
 * Free an sbuf -- data associated with the sbuf is not freed.
 */
#define SBUF_FREE(sb) MEM_DEALLOC((pointer_t)(sb).segs, ((sb).size * sizeof(sbuf_seg_t)))

/*
 * Add a segment at the end of an sbuf. Allocate some space if necessary. 
 */
#define SBUF_APPEND(sb,ptr,seg_size) {					\
	if ((sb).free == 0) sbuf_grow(&(sb),(sb).size);			\
	(sb).end->p = (pointer_t)(ptr);					\
	(sb).end->s = (seg_size);					\
	(sb).end++;							\
	(sb).free--;							\
}

/*
 * Get the first segment from an sbuf.
 */
#define SBUF_GET_SEG(sb,sp,type) (sp) = (type)((sb).segs->p);

/*
 * Get the size of the data in an sbuf.
*/
#define SBUF_GET_SIZE(sb, size) {					\
    register sbuf_seg_ptr_t ssp = (sb).segs;				\
    (size) = 0;								\
    for (; ssp != (sb).end; ssp++) (size) += (int)ssp->s;		\
}


/*
 * Add the contents of an sbuf at the end of another sbuf. Allocate
 * some space if necessary.
 */
#define	SBUF_SB_APPEND(to,from) {				\
	register sbuf_seg_ptr_t		from_ptr;		\
	register sbuf_seg_ptr_t		to_ptr;			\
	int				count;			\
								\
	count = (from).size - (from).free;			\
	if ((to).free < count) sbuf_grow(&(to),(count + 10));	\
	from_ptr = (from).segs;					\
	to_ptr = (to).end;					\
	for (; count; count--) *to_ptr++ = *from_ptr++;		\
	(to).end = to_ptr;					\
	(to).free -= ((from).size - (from).free);		\
}


/*
 * Copy an sbuf in to a contiguous area of storage.
 *	source is a pointer to an sbuf
 *	destination is any old pointer
 *	numbytes is set to the number of bytes copied
 */
#define SBUF_FLATTEN(source, destination, numbytes) {			\
    register sbuf_seg_ptr_t end = (source)->end;			\
    register sbuf_seg_ptr_t ssp = (source)->segs;			\
    register char *dest = (char *)(destination);			\
    register int seg_size;						\
    for (; ssp != end; ssp++) {						\
	seg_size = (int)ssp->s;						\
	if (seg_size) {							\
            memmove((char *)dest, (char *)ssp->p, seg_size);            \
	    dest += seg_size;						\
	}								\
    }									\
    (numbytes) = (long)dest - (long)(destination);			\
}

/*
 * Structure used to store a position within an sbuf.
 */
typedef	struct {
	sbuf_seg_ptr_t		seg_ptr;
	pointer_t		data_ptr;
	int			data_left;
} sbuf_pos_t;

/*
 * Position pos at offset off from the start of sbuf sb.
 */
#define	SBUF_SEEK(sb,pos,off) {						\
	register int		loc = (off);				\
	register sbuf_seg_ptr_t	seg_ptr = (sb).segs;			\
									\
	while (loc > seg_ptr->s) {					\
		loc -= seg_ptr->s;					\
		seg_ptr++;						\
		if (seg_ptr == (sb).end) panic("SBUF_SEEK");		\
	}								\
	(pos).seg_ptr = seg_ptr;					\
	(pos).data_ptr = (pointer_t)((char *)(seg_ptr->p) + loc);	\
	(pos).data_left = seg_ptr->s - loc;				\
}


/*
 * Copy count bytes from position pos in sbuf sb to address addr.
 * Update pos and addr after the operation.
 */
#define	SBUF_EXTRACT(sb,pos,addr,count) {					\
	register int		total_left = (count);				\
										\
	while (total_left > (pos).data_left) {					\
            memmove((char *)(addr), (char *)(pos).data_ptr, (pos).data_left);	\
            total_left -= (pos).data_left;					\
            (pointer_t)(addr) = (pointer_t)(((char *)addr) + (pos).data_left);	\
            (pos).seg_ptr++;							\
            if ((pos).seg_ptr == (sb).end) {					\
                if (total_left) panic("SBUF_EXTRACT");				\
            }									\
            else {								\
                (pos).data_ptr = (pointer_t) (pos).seg_ptr->p;			\
                (pos).data_left = (int) (pos).seg_ptr->s;			\
            }									\
	}									\
	if (total_left) {							\
            memmove((char *)(addr), (char *)(pos).data_ptr, total_left);	\
            (pointer_t)(addr) = (pointer_t)(((char *)addr) + total_left);	\
            (pos).data_ptr += total_left;					\
            (pos).data_left -= total_left;					\
	}									\
}


/*
 * Copy count bytes from position pos in sbuf sb to address addr.
 * Update pos, addr and count after the operation.
 * Count should say how many bytes were actually copied.
 */
#define	SBUF_SAFE_EXTRACT(sb,pos,addr,count) {					\
	register int		total_left = (count);				\
										\
	(count) = 0;								\
	while (total_left > (pos).data_left) {					\
            	memmove((char *)(addr), (char *)(pos).data_ptr, (pos).data_left);  \
		total_left -= (pos).data_left;					\
		(count) += (pos).data_left;					\
		(addr) = (pointer_t)(((char *)addr) + (pos).data_left);		\
		(pos).seg_ptr++;						\
		if ((pos).seg_ptr != (sb).end) {				\
			(pos).data_ptr = (pos).seg_ptr->p;			\
			(pos).data_left = (pos).seg_ptr->s;			\
		}								\
		else {								\
			total_left = 0;						\
			break;							\
		}								\
	}									\
	if (total_left) {							\
        	memmove((char *)(addr), (char *)(pos).data_ptr, total_left);	\
		(addr) = (pointer_t)(((char *)addr) + total_left);		\
		(pos).data_ptr += total_left;					\
		(pos).data_left -= total_left;					\
		(count) += total_left;						\
	}									\
}

/*
 * Is the position pos at the end of the sbuf sb?
 */
#define SBUF_POS_AT_END(sb,pos) ((pos).seg_ptr == (sb).end)


#endif	_SBUF_
