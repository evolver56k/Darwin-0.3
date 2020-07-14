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
 * Copyright (c) 1990 NeXT, Inc.
 *
 *  History:
 *
 *	18-Jul-90: Brian Pinkerton at NeXT
 *		created
 */
#import <mach/machine/vm_types.h>
#import	<mach/machine/vm_param.h>
#import <kern/queue.h>

/*
 *  Public procedures
 */
void initKernelStacks();		/* initialize the module */
vm_offset_t allocStack();		/* allocate a new kernel stack */
void freeStack(vm_offset_t stack);	/* free a kernel stack */
void swapOutStack(vm_offset_t stack);   /* try to swap out a stack */
void swapInStack(vm_offset_t stack);	/* swap in a stack */


/*
 *  The struct _kernelStack sits at the beginning of a kernel stack (under the
 *  pcb & u area).  We declare it here because the size of the kernel stack depends
 *  on it.
 */
enum stackStatus { STACK_FREE, STACK_SWAPPED, STACK_IN_USE };

typedef struct _kernelStack {
	queue_chain_t		freeList;
	enum stackStatus	status;
} *kernelStack;

/*
 * The size of the kernel stack returned from allocStack().
 * KERNSTACK_SIZE is defined in <mach/machine/vm_param.h>
 */
#define	KERNEL_STACK_SIZE	(KERNSTACK_SIZE - sizeof(struct _kernelStack))
