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

#ifndef	_IPC_SWAP_
#define	_IPC_SWAP_

#include	"config.h"

#import <architecture/byte_order.h>

/*
 * TypeType seen in reverse for byte-swapping. 
 */
#if	VaxOrder
typedef struct {
	unsigned int
		msg_type_name:8,
		msg_type_size:8,
		msg_type_numhigh:8,
		msg_type_dummy:1,
		msg_type_deallocate:1,
		msg_type_longform:1,
		msg_type_inline:1,
		msg_type_numlow:4;
} swap_msg_type_t;
#else	VaxOrder
typedef struct {
	unsigned int
		msg_type_name:8,
		msg_type_size:8,
		msg_type_numlow:8,
		msg_type_dummy:1,
		msg_type_deallocate:1,
		msg_type_longform:1,
		msg_type_inline:1,
		msg_type_numhigh:4;
} swap_msg_type_t;
#endif	VaxOrder

/*
 * Declarations needed for the following macros to work. 
 */
#define	SWAP_DECLS

/*
 * Swap and copy a value, and return the swapped value. 
 */
#define SWAP_LONG_LONG(f,t) ( \
	(t) = NXSwapLongLong(f) \
)

#define SWAP_LONG(f,t) ( \
	(t) = NXSwapLong(f) \
)

#define SWAP_SHORT(f,t) ( \
	(t) = NXSwapShort(f) \
)

/*
 * Swap in place an array of long longs. 
 */
#ifdef NATURAL_ALIGNMENT
   /* Buffers are only guaranteed to be on long boundaries, so need to 
    * worry about alignment of longlongs.
    */
#define SWAP_LONG_LONG_ARRAY(s,n) {					\
	unsigned char *p = (unsigned char *)s;				\
	unsigned char c;						\
	while(n--) {							\
	  c = p[0]; p[0]=p[7]; p[7]=c;					\
	  c = p[1]; p[1]=p[6]; p[6]=c;					\
	  c = p[2]; p[2]=p[5]; p[5]=c;					\
	  c = p[3]; p[3]=p[4]; p[4]=c;					\
	  p+=8;								\
	  }								\
}
#define SWAP_LONG_LONG_PTR(f,t) do {					\
	unsigned long *fptr = (unsigned long *)f;			\
	unsigned long *tptr = (unsigned long *)t;			\
	unsigned long long datum;					\
	datum = NXSwapLongLong(((long long)fptr[0] << 32) | fptr[1]);	\
	tptr[0] = datum >> 32;						\
	tptr[1] = datum & 0xffffffff;					\
	} while(0)
#else

#define	SWAP_LONG_LONG_ARRAY(s,n) do {					\
	register long long	*p = (long long *)s;			\
									\
	for (; n; n--, p++) SWAP_LONG_LONG(*p,*p);			\
} while(0)

#define SWAP_LONG_LONG_PTR(f,t) SWAP_LONG_LONG(*f,*t)

#endif NATURAL_ALIGNMENT

/*
 * Swap in place an array of longs. 
 */
#define	SWAP_LONG_ARRAY(s,n) {						\
	register long	*p = (long *)s;					\
									\
	for (; n; n--, p++) SWAP_LONG(*p,*p);				\
}

/*
 * Swap in place an array of shorts. 
 */
#define	SWAP_SHORT_ARRAY(s,n) {						\
	register short	*p = (short *)s;				\
									\
	for (; n; n--, p++) SWAP_SHORT(*p,*p);				\
}

/*
 * External definitions for functions implemented by ipc_swap.c
 */

extern void swap_long_sbuf();
/*
sbuf_ptr_t	sb_ptr;
sbuf_pos_t	*pos_ptr;
long		**to_ptr;
int		count;
*/

extern void swap_long_long_sbuf();
/*
sbuf_ptr_t	sb_ptr;
sbuf_pos_t	*pos_ptr;
long long	**to_ptr;
int		count;
*/

extern void swap_short_sbuf();
/*
sbuf_ptr_t	sb_ptr;
sbuf_pos_t	*pos_ptr;
short		**to_ptr;
int		count;
*/

#endif	_IPC_SWAP_
