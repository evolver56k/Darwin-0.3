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
 */

/*
 */
#import <cputypes.h>
#import <mach_nbc.h>
#import <mach/features.h>

#import <kern/task.h>
#import <kern/thread.h>
#import <mach/time_value.h>
#import <mach/vm_param.h>
#import <vm/vm_map.h>
#import <vm/vm_page.h>
#import <kern/parallel.h>

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/dir.h>
#import <sys/proc.h>
#import <sys/vm.h>
#import <sys/file.h>
#import <sys/vnode.h>
#import <sys/buf.h>
#import <sys/mount.h>
#import <sys/trace.h>
#import <sys/kernel.h>

#import <kern/kalloc.h>
#import <mach/port.h>

#import <machine/spl.h>

useracc(addr, len, prot)
	caddr_t	addr;
	u_int	len;
	int	prot;
{
	return (vm_map_check_protection(
			current_task()->map,
			trunc_page(addr), round_page(addr+len),
			prot == B_READ ? VM_PROT_READ : VM_PROT_WRITE));
}

vslock(addr, len)
	caddr_t	addr;
	int	len;
{
	vm_map_pageable(current_task()->map, trunc_page(addr),
				round_page(addr+len), FALSE);
}

vsunlock(addr, len, dirtied)
	caddr_t	addr;
	int	len;
	int dirtied;
{
#if	NeXT
	pmap_t		pmap;
	vm_page_t	pg;
	vm_offset_t	vaddr, paddr;

	if (dirtied) {
		pmap = current_task()->map->pmap;
		for (vaddr = trunc_page(addr); vaddr < round_page(addr+len);
				vaddr += PAGE_SIZE) {
			paddr = pmap_extract(pmap, vaddr);
			pg = PHYS_TO_VM_PAGE(paddr);
			vm_page_set_modified(pg);
		}
	}
#endif	/* NeXT */
#ifdef	lint
	dirtied++;
#endif	/* lint */
	vm_map_pageable(current_task()->map, trunc_page(addr),
				round_page(addr+len), TRUE);
}

#if	defined(sun) || BALANCE || defined(m88k) || defined(i386)
#else	defined(sun) || BALANCE || defined(m88k) || defined(i386)
subyte(addr, byte)
	void * addr;
	int byte;
{
	char character;
	
	character = (char)byte;
	return (copyout((void *)&(character), addr, sizeof(char)) == 0 ? 0 : -1);
}

suibyte(addr, byte)
	void * addr;
	int byte;
{
	char character;
	
	character = (char)byte;
	return (copyout((void *) &(character), addr, sizeof(char)) == 0 ? 0 : -1);
}

int fubyte(addr)
	void * addr;
{
	unsigned char byte;

	if (copyin(addr, (void *) &byte, sizeof(char)))
		return(-1);
	return(byte);
}

int fuibyte(addr)
	void * addr;
{
	unsigned char byte;

	if (copyin(addr, (void *) &(byte), sizeof(char)))
		return(-1);
	return(byte);
}

suword(addr, word)
	void * addr;
	long word;
{
	return (copyout((void *) &word, addr, sizeof(int)) == 0 ? 0 : -1);
}

long fuword(addr)
	void * addr;
{
	long word;

	if (copyin(addr, (void *) &word, sizeof(int)))
		return(-1);
	return(word);
}

/* suiword and fuiword are the same as suword and fuword, respectively */

suiword(addr, word)
	void * addr;
	long word;
{
	return (copyout((void *) &word, addr, sizeof(int)) == 0 ? 0 : -1);
}

long fuiword(addr)
	void * addr;
{
	long word;

	if (copyin(addr, (void *) &word, sizeof(int)))
		return(-1);
	return(word);
}
#endif	/* defined(sun) || BALANCE || defined(m88k) || defined(i386) */

swapon()
{
	/* Not yet implemented */
	printf("swapon(): not supported!\n");
	return(EPERM);
}

thread_t procdup(child, parent)
	struct proc *child, *parent;
{
	thread_t	thread;
	task_t		task;
 	kern_return_t	result;
	boolean_t	inherit_memory = TRUE;

	if (parent->task == kernel_task)
		inherit_memory = FALSE;

	result = task_create(parent->task, inherit_memory, &task);
	if(result != KERN_SUCCESS)
	    printf("fork/procdup: task_create failed. result: %d\n", result);
	child->task = task;	/* retained reference */
	task->proc = child;

	result = thread_create(task, &thread);
	if(result != KERN_SUCCESS)
	    printf("fork/procdup: thread_create failed. Code: 0x%x\n", result);
	thread_deallocate(thread);	/* release reference */

	/*
	 *	Don't need to lock thread here because it can't
	 *	possibly execute and no one else knows about it.
	 */
	compute_priority(thread, FALSE);

	return(thread);
}

chgprot(_addr, prot)
	caddr_t		_addr;
	vm_prot_t	prot;
{
	vm_offset_t	addr = (vm_offset_t) _addr;

	return(vm_map_protect(current_task()->map,
				trunc_page(addr),
				round_page(addr + 1),
				prot, FALSE) == KERN_SUCCESS);
}

kern_return_t	unix_pid(t, x)
	task_t	t;
	int	*x;
{
	if (t == TASK_NULL) {
		*x = -1;
		return(KERN_FAILURE);
	} else {
		if (t->proc)
			*x = t->proc->p_pid;
		else {
			*x = -1;
			return(KERN_FAILURE);
		}
		return(KERN_SUCCESS);
	}
}

/*
 *	Routine:	task_by_unix_pid
 *	Purpose:
 *		Get the task port for another "process", named by its
 *		process ID on the same host as "target_task".
 *
 *		Only permitted to privileged processes, or processes
 *		with the same user ID.
 */
kern_return_t	task_by_unix_pid(target_task, pid, t)
	task_t		target_task;
	int		pid;
	task_t		*t;
{
	struct proc	*p;

	unix_master();

	if (
		((p = pfind(pid)) != (struct proc *) 0)
		&& (target_task->proc != (struct proc *) 0)
		&& ((p->p_ucred->cr_uid == target_task->proc->p_ucred->cr_uid)
		|| !(suser(target_task->proc->p_ucred, &target_task->proc->p_acflag)))
		&& (p->p_stat != SZOMB)
		) {
			if (p->task != TASK_NULL)
				task_reference(p->task);
			*t = p->task;
			/* compatibility with NeXT 2.0 debuggers, launchers */
			if (suser(p->p_ucred, &p->p_acflag)) {
				/* don't set it for anyone; could exec a careless nameserver */
				struct proc *p = current_task()->proc;

				if (p)
					p->p_debugger = 1;
			}
			unix_release();
			return(KERN_SUCCESS);
	}
	*t = TASK_NULL;
	unix_release();
	return(KERN_FAILURE);
}

/*
 *	Routine:	task_by_pid
 *	Purpose:
 *		Trap form of "task_by_unix_pid"; soon to be eliminated.
 */
port_t		task_by_pid(pid)
	int		pid;
{
	task_t		self = current_task();
	port_t		t = PORT_NULL;
	task_t		result_task;

	if (task_by_unix_pid(self, pid, &result_task) == KERN_SUCCESS) {
		t = convert_task_to_port(result_task);
		if (t != PORT_NULL)
			object_copyout(self, t, MSG_TYPE_PORT, &t);
	}

	return(t);
}
