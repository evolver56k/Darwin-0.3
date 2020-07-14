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
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log: mach.h,v $
 * Revision 1.1.1.1  1999/04/14 23:19:16  wsanchez
 * Import of Libc-78-8
 *
 * Revision 1.1.1.1.38.3  1999/03/16 15:47:17  wsanchez
 * Substitute License
 *
 * 31-May-90  Gregg Kellogg (gk) at NeXT
 *	Added <sys/thread_switch.h> and <mach_host.h>.
 *
 * Revision 2.2  89/10/28  11:30:47  mrt
 * 	Changed location of mach_interface from mach/ to
 * 	top level include.
 * 	[89/10/27            mrt]
 * 
 * Revision 2.1  89/06/13  16:47:44  mrt
 * Created.
 * 
 */
/* 
 *  Includes all the types that a normal user
 *  of Mach programs should need
 */

#ifndef	_MACH_H_
#define	_MACH_H_

#import <mach/mach_types.h>
#import <mach/thread_switch.h>
#import <mach/mach_interface.h>
#import <mach/mach_host.h>
#import <mach/mach_init.h>

#endif	/* _MACH_H_ */
