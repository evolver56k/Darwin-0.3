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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * ddmPrivate.h - private #defines for User-level XPR (aka ddm) module.
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#ifndef	_UXPR_PRIVATE_
#define _UXPR_PRIVATE_

#ifdef	KERNEL
#import <mach/machine/simple_lock.h>
#else	KERNEL
#import <objc/objc.h>
#endif	KERNEL

/*
 * An array of these is kept in xprArray[].
 */
typedef struct {
	char 			*msg;
	int			arg1,arg2,arg3,arg4,arg5;
	unsigned long long	timestamp;
	int			cpu_num;
} xprbuf_t;

/*
 * global uxpr data, shared by kernel and debug monitor.
 */
typedef struct {
	unsigned 	numXprBufs;		// size of xprArray
	xprbuf_t	*xprArray;		// Actual xpr buffer storage
	xprbuf_t 	*xprLast;		// ptr to last valid entry
	unsigned	numValidEntries;	// # of valid entries in
						//    xprArray
} uxprGlobal_t;

/*
 * Globals (in uxpr.m).
 */
extern int xprLocked;
#ifdef	KERNEL
extern	uxprGlobal_t		uxprGlobal;
extern simple_lock_data_t	xpr_lock;
#else	KERNEL
extern id xprLock;
#endif	KERNEL

/* 
 * in uxprServer.m.
 */
#ifndef	KERNEL
extern volatile void xprServerThread(char *portName);	
#endif	KERNEL


#endif _UXPR_PRIVATE_
