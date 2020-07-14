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

/*	@(#)ldd.h	2.0	03/20/90	(c) 1990 NeXT	
 *
 * ldd.h - kernel prototypes used by loadable device drivers
 *
 * HISTORY
 * 22-May-91	Gregg Kellogg (gk) at NeXT
 *	Split out public interface.
 *
 * 16-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Removed a lot of stuff that's defined in other header files. 
 *	Eventually this file should either go away or contain only imports of
 *	other files.
 *
 * 20-Mar-90	Doug Mitchell at NeXT
 *	Created.
 *
 */

#ifndef	_DEV_LDD_PRIV_
#define _DEV_LDD_PRIV_

#import <kernserv/prototypes.h>
#import	<sys/cdefs.h>
#import <bsd/dev/disk.h>
#ifdef	KERNEL_PRIVATE
#if	!MACH_USER_API
#import <kern/task.h>
#import <kern/thread.h>
#endif	/* MACH_USER_API */
#endif	/* KERNEL_PRIVATE */

typedef int (*PFI)();

extern int physio(int (*strat)(), struct buf *bp, dev_t dev, int rw, 
	unsigned (*mincnt)(), struct uio *uio, int blocksize);

extern u_short checksum_16 (u_short *wp, int shorts);
extern int sdchecklabel(struct disk_label *dlp, int blkno);

int	sleep __P((void *chan, int pri));
void	wakeup __P((void *chan));
extern void psignal(struct proc *p, int sig);

void	timeout __P((void (*)(void *), void *arg, int ticks));
int		untimeout __P((void (*)(void *), void *arg));

#if 	KERNEL_PRIVATE && !MACH_USER_API

extern kern_return_t vm_map_delete(vm_map_t map, 
	vm_offset_t start, 
	vm_offset_t end);
extern kern_return_t vm_map_pageable(vm_map_t map, 
	vm_offset_t start, 
	vm_offset_t end,
	boolean_t new_pageable);
	
#endif 	/* KERNEL_PRIVATE && !MACH_USER_API */

#endif	/* _DEV_LDD_PRIV_ */

