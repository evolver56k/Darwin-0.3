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
/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * devsw.h - Kernel-level functions to allow adding entries
 *		     to cdevsw, bdevsw, and vfssw.
 *
 * HISTORY
 * 20-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <sys/types.h>
#import <sys/cdefs.h>
#ifdef KERNEL
#import <bsd/sys/conf.h>
#endif KERNEL
#if 0
#import <bsd/sys/vfs.h>
#endif 0
#import <objc/objc.h>		/* for BOOL */

typedef int (*IOSwitchFunc)();

/*
 * Add an entry to bdevsw. Returns the index into bdevsw at which the entry
 * was created, or -1 of no space can be found.
 */
extern int IOAddToBdevswAt(
	int major,
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc strategyFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc dumpFunc,
	IOSwitchFunc psizeFunc,
	BOOL isTape);		// TRUE if device is a tape device
extern int IOAddToBdevsw(
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc strategyFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc dumpFunc,
	IOSwitchFunc psizeFunc,
	BOOL isTape);		// TRUE if device is a tape device
	
/*
 * Remove an entry from bdevsw, replace it with a null entry.
 */
extern void IORemoveFromBdevsw(int bdevswNumber);

/*
 * Add an entry to cdevsw. Returns the index into cdevsw at which the entry
 * was created, or -1 of no space can be found.
 */
extern int IOAddToCdevswAt(
	int major,
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc readFunc,
	IOSwitchFunc writeFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc stopFunc,
	IOSwitchFunc resetFunc,
	IOSwitchFunc selectFunc,
	IOSwitchFunc mmapFunc,
	IOSwitchFunc getcFunc,
	IOSwitchFunc putcFunc);
extern int IOAddToCdevsw(
	IOSwitchFunc openFunc,
	IOSwitchFunc closeFunc,
	IOSwitchFunc readFunc,
	IOSwitchFunc writeFunc,
	IOSwitchFunc ioctlFunc,
	IOSwitchFunc stopFunc,
	IOSwitchFunc resetFunc,
	IOSwitchFunc selectFunc,
	IOSwitchFunc mmapFunc,
	IOSwitchFunc getcFunc,
	IOSwitchFunc putcFunc);
	
/*
 * Remove an entry from cdevsw, replace it with a null entry.
 */
extern void IORemoveFromCdevsw(int cdevswNumber);
#if 0
#warning removed loadable file systems


/*
 * Add an entry to vfssw. Returns the index into vfssw at which the entry
 * was created, or -1 of no space can be found.
 */
extern int IOAddToVfsswAt(
	int index,
	const char *vfsswName,
	const struct vfsops *vfsswOps);
extern int IOAddToVfssw(
	const char *vfsswName,
	const struct vfsops *vfsswOps);
	
/*
 * Remove an entry from vfssw, replace it with a null entry.
 */
extern void IORemoveFromVfssw(int vfsswNumber);
#endif 0
	
