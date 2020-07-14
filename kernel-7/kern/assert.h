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
 * HISTORY
 * 15-Feb-90  Gregg Kellogg (gk) at NeXT
 *	Updated to machine independent based on mach sources.
 *
 * 21-Nov-88  Avadis Tevanian (avie) at NeXT.
 *	Created based on machine dependent version.
 */ 

#ifndef	_KERN_ASSERT_H_
#define	_KERN_ASSERT_H_

#if	_KERNEL
#ifdef	KERNEL_BUILD
#import <mach_assert.h>
#else	/* KERNEL_BUILD */
#ifndef	MACH_ASSERT
#define	MACH_ASSERT DEBUG
#endif	/* MACH_ASSERT */
#endif	/* KERNEL_BUILD */

#if	MACH_ASSERT
#import <kernserv/printf.h>

#ifdef __GNUC__
#define	ASSERT(e) \
	if ((e) == 0) { \
		printf ("ASSERTION " #e " failed at line %d in %s\n", \
		    __LINE__, __FILE__); \
		panic ("assertion failed"); \
	}
#else /* !__GNUC__ */
#define	ASSERT(e) \
	if ((e) == 0) { \
		printf ("ASSERTION e failed at line %d in %s\n", \
		    __LINE__, __FILE__); \
		panic ("assertion failed"); \
	}
#endif /* !__GNUC__ */
#else	/* MACH_ASSERT */
#define	ASSERT(e)
#endif	/* MACH_ASSERT */

/*
 *	For compatibility with the code they are writing at CMU.
 */
#define assert(e)	ASSERT(e)
#ifdef	lint
#define assert_static(x)
#else	/* lint */
#define assert_static(x)	assert(x)
#endif	/* lint */

#endif	/* _KERNEL */
#endif	/* _KERN_ASSERT_H_ */
