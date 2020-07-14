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
 	File:		PseudoKernel.c

 	Contains:	BlueBox PseudoKernel calls

 	Copyright:	1997 by Apple Computer, Inc., all rights reserved

	File Ownership:

		DRI:               Thomas Mason

		Other Contact:     Brett Halle

		Technology:        Kernel/IO

	Writers:

		(tjm)	Thomas Mason

	Change History (most recent first):
*/

#include <mach/mach_types.h>

#include <machdep/ppc/PseudoKernel.h>

#define printf kprintf

int	debugNotify = 0;

/*
** Function:	NotifyInterruption
**
** Inputs:	port			- mach_port for Blue thread
**		ppcInterrupHandler	- interrupt handler to execute
**		interruptStatePtr	- current interrupt state
**		emulatorDescriptor	- where in emulator to notify
**		originalPC		- where the emulator was executing
**		originalR2		- new R2
**
** Outputs:
**
** Notes:
**
*/
void NotifyInterruption (	void *			port_thread,
				UInt32			ppcInterruptHandler,
				UInt32 *		interruptStatePtr,
				EmulatorDescriptor *	emulatorDescriptor,
				void **			originalPC,
				void **			originalR2,
				thread_t		*othread )
{
    pcb_t		bluePCB;
    UInt32		interruptState; 
    thread_t		thread, cur_thread;
    task_t		task;
    ipc_port_t		sright;
    ipc_space_t		space;
    mach_port_t		port, name;
    queue_head_t	*list;
    spl_t s;


    if (debugNotify > 5) {
	printf("\nNotifyInterruption: %X, %X, %X, %X, %X, %X\n",
		port_thread, ppcInterruptHandler, interruptStatePtr,
		emulatorDescriptor, originalPC, originalR2 );
    }

    if (othread != 0) {
	port = (mach_port_t)port_thread;
	cur_thread = current_thread();
	task = cur_thread->task;
	space = task->itk_space;
	list = &task->thread_list;

	/* find the thread */
	queue_iterate(list, thread, thread_t, thread_list) {

	sright = retrieve_thread_self(thread);
	name = ipc_port_copyout_send_compat(sright, space);

	if (debugNotify > 5) {
	    printf("NotifyInterruption: port = %x\n", port);
	    printf("                    th->ith_sself = %x\n",thread->ith_sself);
	    printf("                    th->ith_self = %x\n",thread->ith_self);
	    printf("                    task = %x\n",task);
	    printf("                    sright = %x\n",sright);
	    printf("                    space = %x\n",space);
	    printf("                    name = %x\n",name);
	}
	    if (name == port) {
		if (debugNotify > 1)
		    printf("                    thread = %x\n",thread);
	    break;
	}
	    else {
		if (debugNotify > 5) {
		    printf("NotifyInterruption: looking for port = %x\n", port);
		    if (name == MACH_PORT_NULL)
			printf("Error from ipc_port_copyout_send_compat\n");
		    printf("                    got name = %x\n",name);
		    printf("                    task = %x\n",task);
		    printf("                    thread = %x\n",thread);
		}
	    }
	}
	if (queue_end(list, (queue_entry_t)(thread))) {
	    if (name != port) {
		printf("COULD NOT FIND the thread!!!\n");
	while (1)
	    ;
	    }
	}
	*othread = thread;
    }
    else {
	if (debugNotify > 1)
	    printf("                    thread = %x\n",thread);
	thread = (thread_t)port_thread;
    }

	s=splsched();
    thread_lock(thread);
    interruptState = (*interruptStatePtr & kInterruptStateMask) >> kInterruptStateShift; 
    bluePCB = thread->pcb;

    switch (interruptState)
    {
    case kInUninitialized:
	if (debugNotify > 2)
		printf("NotifyInterrupt: kInUninitialized\n");
	break;
		
    case kInPseudoKernel:
    case kOutsideBlue:
	if (debugNotify > 2)
		printf("NotifyInterrupt: kInPseudoKernel/kOutsideBlue\n");
	*interruptStatePtr = *interruptStatePtr
		| ((emulatorDescriptor->postIntMask >> kCR2ToBackupShift)
		& kBackupCR2Mask);
	break;
		
    case kInSystemContext:
	if (debugNotify > 2)
		printf("NotifyInterrupt: kInSystemContext\n");
	bluePCB->ss.cr |= emulatorDescriptor->postIntMask;
	break;
		
    case kInAlternateContext:
	if (debugNotify > 2)
		printf("NotifyInterrupt: kInAlternateContext\n");
	*interruptStatePtr = *interruptStatePtr
		| ((emulatorDescriptor->postIntMask >> kCR2ToBackupShift)
			& kBackupCR2Mask);
	*interruptStatePtr = (*interruptStatePtr & ~kInterruptStateMask) | (kInPseudoKernel << kInterruptStateShift);
		
	*originalPC = (void *)bluePCB->ss.srr0;
	bluePCB->ss.srr0 = ppcInterruptHandler;
	*originalR2 = (void *)bluePCB->ss.r2;
	break;
		
    case kInExceptionHandler:
	if (debugNotify > 2)
		printf("NotifyInterrupt: kInExceptionHandler\n");
	*interruptStatePtr = *interruptStatePtr
		| ((emulatorDescriptor->postIntMask >> kCR2ToBackupShift)
		& kBackupCR2Mask);
	break;
		
    default:
	if (debugNotify)
		printf("NotifyInterrupt: default ");
		printf("Interruption while running in unknown state\n");
		printf("interruptState = 0x%X\n",interruptState);
	break;
    }
    thread_unlock(thread);
    (void) splx(s);
}

#import <driverkit/return.h>

#ifndef	NULL
#define NULL ((void *)0)
#endif

static void		*Blueowner = NULL;
static vm_offset_t	Blueowner_addr;
static port_t		Blueowner_task = PORT_NULL;
static vm_size_t	Blueshmem_size;
static vm_offset_t	*Blueshmem_addr;

/*
 * Set up the shared memory area between the BlueBox and the kernel.
 *
 *	Obtain page aligned wired kernel memory for 'size' bytes using
 *	kmem_alloc().
 *	Find a similar sized region in the BlueBox task VM map using
 *	vm_map_find().  This function will find an appropriately sized region,
 *	create a memory object, and insert it in the VM map.
 *	For each physical page in the kernel's wired memory we got from
 *	kmem_alloc(), enter that page at the appropriate location in the page
 *	map for the BlueBox, in the address range we allocated using
 *	vm_map_find().  
 */
int
mapBlueBoxShmem (	port_t		task,	// in
		vm_size_t	size,	// in
		vm_offset_t *	addr)	// out
{
	vm_offset_t	off;
	vm_offset_t	phys_addr;
	IOReturn	krtn;
	void *		task_map;
	vm_offset_t	task_addr;
	
	if ( task == PORT_NULL || size == 0 )	// malformed request
	    return IO_R_INVALID_ARG;
	if ( Blueowner_task != PORT_NULL || Blueowner != NULL )
	    return IO_R_INVALID_ARG;	// Mapping set up already

	krtn = createEventShmem(task,size,&task_map,&task_addr,&Blueshmem_addr);
	if ( krtn != KERN_SUCCESS )
	{
	    printf("mapEventShmem: createEventShmem fails (%d).\n",krtn);
	    return krtn;
	}

	Blueshmem_size = size;
	Blueowner_task = task;
	Blueowner_addr = task_addr;
	*addr = task_addr;
	Blueowner = task_map;

	return IO_R_SUCCESS;
}

/* 
 * Unmap the shared memory area and release the wired memory.
 */
int
unmapBlueBoxShmem (void)
{
	vm_offset_t off;
	IOReturn r;

	r=destroyEventShmem(Blueowner_task,Blueowner,Blueshmem_size,Blueowner_addr,Blueshmem_addr);
	if ( r != KERN_SUCCESS )
	{
	    printf("unmapBlueBoxShmem: destroyEventShmem fails (%d).\n",  r);
	}
	Blueshmem_addr = Blueowner_addr = (vm_offset_t)0;
	Blueshmem_size = 0;
	Blueowner = NULL;
	Blueowner_task = PORT_NULL;
	return r;
}
