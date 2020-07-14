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
 * Copyright (c) 1992 NeXT, Inc.  All rights reserved.
 *
 * c_utils.h -- Helpful C macros
 */

#define	ROUND_UP(addr, align)	\
	( ( (unsigned)(addr) + (align) - 1) & ~((align) - 1) )

#define	TRUNC_DOWN(addr, align)	\
	( (unsigned)(addr) & ~((align) - 1) )

/*
 * FIXME: For some reason the compiler miscalculates the alignment of
 * struct uthread as 0, so this forces all structs to have an alignof of
 * 16.
 */
#define	COMPILER_BUG	1

#ifdef	COMPILER_BUG
#define	ROUND_PTR(type, addr)	\
	(type *)( ( (unsigned)(addr) + 16 - 1) \
		  & ~(16 - 1) )

#define	TRUNC_PTR(type, addr)	\
	(type *)( (unsigned)(addr) & ~(16 - 1) )
#else	/* COMPILER_BUG */
#define	ROUND_PTR(type, addr)	\
	(type *)( ( (unsigned)(addr) + __alignof__(type) - 1) \
		  & ~(__alignof__(type) - 1) )

#define	TRUNC_PTR(type, addr)	\
	(type *)( (unsigned)(addr) & ~(__alignof__(type) - 1) )
#endif	/* COMPILER_BUG */

#define	ARRAY_ELEM(x)		(sizeof(x)/sizeof(x[0]))
