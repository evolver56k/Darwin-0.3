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
 *
 */
/*
 *  This file is used to pick up the various configuration options that affect
 *  the use of kernel include files by special user mode system applications.
 *
 *  The entire file (and hence all configuration symbols which it indirectly
 *  defines) is enclosed in the (KERNEL && !KERNEL_BUILD) conditional to
 *  prevent accidental interference with normal user applications.  Only
 *  special system applications need to know the precise layout of 
 *  internal kernel structures and they will explicitly set the appropriate 
 *  flags to obtain the proper environment.
 */

#ifndef	_MACH_FEATURES_H_
#define	_MACH_FEATURES_H_

#ifdef	KERNEL

#ifdef	KERNEL_BUILD
#import <meta_features.h>

#else	/* KERNEL_BUILD */
#import <machdep/machine/features.h>

#endif	/* KERNEL_BUILD */

#endif	/* KERNEL */

#endif	/* _MACH_FEATURES_H_ */
