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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PC support initialization.
 *
 * HISTORY
 *
 * 9 Mar 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <kern/kern_port.h>

#import <vm/vm_kern.h>

#import "PCprivate.h"

kern_return_t
PCcreate(
    port_t		th_port,
    vm_offset_t		paddress,
    boolean_t		anywhere
)
{
    kern_return_t	result;
    task_t		target_task = current_task();
    vm_offset_t		address;
    thread_t		thread;
    kern_port_t		th_kport;
    PCprivate_t		priv;
    
    if (!suser())
    	return (KERN_FAILURE);
    
    if (!anywhere) {
	if (copyin(paddress, &address, sizeof (address)))
	    return (KERN_INVALID_ARGUMENT);
	    
	address = trunc_page(address);
    }
    else
    	address = vm_map_min(target_task->map);

    if (!object_copyin(
    			target_task,
			th_port, MSG_TYPE_PORT,
			FALSE, (void *)&th_kport))
	return (KERN_INVALID_ARGUMENT);
	
    thread = convert_port_to_thread(th_kport);

    port_release(th_kport);
    
    if (thread == THREAD_NULL)
    	return (KERN_INVALID_ARGUMENT);
	
    if (thread->pcb->PCpriv) {
	if (anywhere) {
	    address = thread->pcb->PCpriv->taskShared;

	    if (copyout(&address, paddress, sizeof (address)))
		result = KERN_INVALID_ARGUMENT;
	    else
	    	result = KERN_SUCCESS;
	}
	else {
	    if (address == thread->pcb->PCpriv->taskShared)
	    	result = KERN_SUCCESS;
	    else
	    	result = KERN_INVALID_ADDRESS;
	}
	
	thread_deallocate(thread);
	
	return (result);
    }
    
    result = vm_map_find(
			    target_task->map,
			    VM_OBJECT_NULL, 0,
			    &address, round_page(sizeof (struct PCshared)),
			    anywhere);
	
    if (result != KERN_SUCCESS) {
	thread_deallocate(thread);

    	return (result);
    }
    
    if (anywhere) {
	if (copyout(&address, paddress, sizeof (address))) {
	    thread_deallocate(thread);
	    
	    return (KERN_INVALID_ARGUMENT);
	}
    }
    
    vm_map_reference(target_task->map);

    priv = (PCprivate_t)kalloc(sizeof (struct PCprivate));
    *priv = (struct PCprivate) { 0 };

    (void) kmem_alloc_wired(kernel_map,
				(PCshared_t *)&priv->shared,
				sizeof (struct PCshared));
    priv->task = target_task->map;
    priv->taskShared = address;
    
    pmap_enter_shared_range(
			vm_map_pmap(priv->task),
			priv->taskShared,
			sizeof (struct PCshared),
			(vm_offset_t)priv->shared);
    
    priv->shared->versionNumber = PCVERSIONNUMBER;
    priv->shared->releaseNumber = PCRELEASENUMBER;
		
    thread->pcb->PCpriv = priv;

    thread_deallocate(thread);
    
    return (KERN_SUCCESS);
}

void
PCdestroy(
    thread_t		thread
)
{
    PCprivate_t		priv = thread->pcb->PCpriv;
    
    pmap_remove(
		    vm_map_pmap(priv->task),
		    priv->taskShared,
		    round_page(priv->taskShared + sizeof (struct PCshared)));
		
    (void) vm_map_remove(
		    priv->task,
		    priv->taskShared,
		    round_page(priv->taskShared + sizeof (struct PCshared)));
			    
    vm_map_deallocate(priv->task);
    
    PCcancelAllTimers(thread);
			    
    kmem_free(
    		kernel_map,
		(vm_offset_t)priv->shared,
		round_page(sizeof (struct PCshared)));
    
    kfree(priv, sizeof (struct PCprivate));
}

kern_return_t
PCldt(
    port_t		th_port,
    vm_offset_t		address,
    vm_size_t		size
)
{
    kern_return_t		result;
    thread_t			thread;
    kern_port_t			th_kport;
    thread_saved_state_t	*saved_state;
    
    if (!suser())
    	return (KERN_FAILURE);

    if (!object_copyin(
    			current_task(),
			th_port, MSG_TYPE_PORT,
			FALSE, (void *)&th_kport))
	return (KERN_INVALID_ARGUMENT);
	
    thread = convert_port_to_thread(th_kport);

    port_release(th_kport);
    
    if (thread == THREAD_NULL)
    	return (KERN_INVALID_ARGUMENT);
	
    saved_state = USER_REGS(thread);
    
    if (address == (vm_offset_t)-1 && size == (vm_offset_t)-1)
	result = task_default_ldt(thread->task);
    else
	result = task_locate_ldt(thread->task, address, size);

    if (result == KERN_SUCCESS) {
	saved_state->frame.cs	= UCODE_SEL;
	saved_state->frame.ss	= UDATA_SEL;
	saved_state->regs.ds	= UDATA_SEL;
	saved_state->regs.es	= UDATA_SEL;
	saved_state->regs.fs	= NULL_SEL;
	saved_state->regs.gs	= NULL_SEL;
    }
	
    thread_deallocate(thread);
    
    return (result);
}

kern_return_t
PCcopyBIOSData(
    vm_address_t	dest
)
{
    if (dest == (vm_address_t)0)
	return (KERN_INVALID_ARGUMENT);

    if (copyout((vm_address_t)BIOS_DATA_ADDR, dest, BIOS_DATA_SIZE))
	return (KERN_INVALID_ARGUMENT);

    return (KERN_SUCCESS);
}

vm_size_t
PCsizeBIOSExtData(void)
{
    return (CNV_MEM_END_ADDR - bios_extdata_addr());
}

kern_return_t
PCcopyBIOSExtData(
    vm_address_t	dest
)
{
    vm_address_t	data = bios_extdata_addr();

    if (dest == (vm_address_t)0)
    	return (KERN_INVALID_ARGUMENT);
	
    if (copyout(data, dest, CNV_MEM_END_ADDR - data))
    	return (KERN_INVALID_ARGUMENT);
	
    return (KERN_SUCCESS);
}

kern_return_t
PCmapBIOSRom(
    port_t		task_port,
    vm_offset_t		paddress,
    boolean_t		anywhere
)
{
    kern_return_t	result;
    vm_offset_t		address;
    task_t		target_task;
    kern_port_t		task_kport;

    if (!anywhere) {
	if (copyin(paddress, &address, sizeof (address)))
	    return (KERN_INVALID_ARGUMENT);
	    
	address = trunc_page(address);
    }
    else
    	address = vm_map_min(target_task->map);

    if (!object_copyin(
    			current_task(),
			task_port, MSG_TYPE_PORT,
			FALSE, (void *)&task_kport))
	return (KERN_INVALID_ARGUMENT);
	
    target_task = convert_port_to_task(task_kport);

    port_release(task_kport);
    
    if (target_task == TASK_NULL)
    	return (KERN_INVALID_ARGUMENT);
    
    result = vm_map_find(
			    target_task->map,
			    VM_OBJECT_NULL, 0,
			    &address, round_page(BIOS_ROM_SIZE),
			    anywhere);
	
    if (result != KERN_SUCCESS) {
	task_deallocate(target_task);

    	return (result);
    }
    
    if (anywhere) {
	if (copyout(&address, paddress, sizeof (address))) {
	    task_deallocate(target_task);
	    
	    return (KERN_INVALID_ARGUMENT);
	}
    }
    
    {
	vm_offset_t	end = round_page(address + BIOS_ROM_SIZE);
	vm_offset_t	phys = trunc_page(BIOS_ROM_ADDR);
	
	while (address < end) {
	    pmap_enter(
	    		vm_map_pmap(target_task->map),
			address,
			phys,
			VM_PROT_READ,
			TRUE);
			
	    address += PAGE_SIZE; phys += PAGE_SIZE;
	}
    }
    
    task_deallocate(target_task);
    
    return (KERN_SUCCESS);
}
