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
 *
 * 6 January 1994 ? at NeXT
 *	Made cpu_number() public.
 * 9 May 1992 ? at NeXT
 *	Cleaned up.
 * 20 April 1992 ? at NeXT
 *	Created from 68k version.
 * 09-Nov-86  John Seamons (jks) at NeXT
 *	Ported to NeXT.
 *
 */

#ifndef	_BSD_I386_CPU_H_
#define	_BSD_I386_CPU_H_
 
#ifdef	KERNEL_PRIVATE
 
#ifdef KERNEL
extern int	master_cpu;
#endif	/* KERNEL */

#endif	/* KERNEL_PRIVATE */

#define	cpu_number()	(0)

#endif	/* _BSD_I386_CPU_H_ */
