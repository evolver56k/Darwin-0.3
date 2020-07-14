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
 * Copyright (c) 1993-1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	task.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for tasks.
 *
 */

#ifndef	_KERN_TASK_H_
#define _KERN_TASK_H_

#import <mach/features.h>

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/time_value.h>
#include <mach/mach_param.h>
#include <mach/task_info.h>
#include <kern/kern_types.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/processor.h>
#include <vm/vm_map.h>

struct task {
	/* Synchronization/destruction information */
	decl_simple_lock_data(,lock)	/* Task's lock */
	int		ref_count;	/* Number of references to me */
	boolean_t	active;		/* Task has not been terminated */

	/* Miscellaneous */
	vm_map_t	map;		/* Address space description */
	queue_chain_t	pset_tasks;	/* list of tasks assigned to pset */
	int		suspend_count;	/* Internal scheduling only */

	/* Thread information */
	queue_head_t	thread_list;	/* list of threads */
	int		thread_count;	/* number of threads */
	decl_simple_lock_data(,thread_list_lock) /* XXX thread_list lock */
	processor_set_t	processor_set;	/* processor set for new threads */
	boolean_t	may_assign;	/* can assigned pset be changed? */
	boolean_t	assign_active;	/* waiting for may_assign */

	/* Garbage */
	struct proc	*proc;		/* corresponding process */

	/* Task-wide thread context information */
	struct
	    pcb_common	*pcb_common;

	/* User-visible scheduling information */
	int		user_stop_count;	/* outstanding stops */
	int		priority;		/* for new threads */

	/* Information for kernel-internal tasks */
	boolean_t	kernel_privilege; /* Is a kernel task */
	boolean_t	kernel_vm_space; /* Uses kernel's pmap? */

	/* Statistics */
	time_value_t	total_user_time;
				/* total user time for dead threads */
	time_value_t	total_system_time;
				/* total system time for dead threads */

	/* IPC structures */
	decl_simple_lock_data(, itk_lock_data)
	struct ipc_port *itk_self;	/* not a right, doesn't hold ref */
	struct ipc_port *itk_sself;	/* a send right */
	struct ipc_port *itk_exception;	/* a send right */
	struct ipc_port *itk_bootstrap;	/* a send right */
	struct ipc_port *itk_registered[TASK_PORT_REGISTER_MAX];
					/* all send rights */

	struct ipc_space *itk_space;

#if	NORMA_TASK
	long		child_node;	/* if != -1, node for new children */
#endif	/* NORMA_TASK */
};

#define task_lock(task)		simple_lock(&(task)->lock)
#define task_unlock(task)	simple_unlock(&(task)->lock)

#define	itk_lock_init(task)	simple_lock_init(&(task)->itk_lock_data)
#define	itk_lock(task)		simple_lock(&(task)->itk_lock_data)
#define	itk_unlock(task)	simple_unlock(&(task)->itk_lock_data)

#define ipc_task_lock(t)	simple_lock(&(t)->ipc_translation_lock)
#define ipc_task_unlock(t)	simple_unlock(&(t)->ipc_translation_lock)

/*
 *	Exported routines/macros
 */

extern kern_return_t	task_create();
extern kern_return_t	task_terminate();
extern kern_return_t	task_suspend();
extern kern_return_t	task_resume();
extern kern_return_t	task_threads();
extern kern_return_t	task_ports();
extern kern_return_t	task_info();
extern kern_return_t	task_get_special_port();
extern kern_return_t	task_set_special_port();
extern kern_return_t	task_assign();
extern kern_return_t	task_assign_default();

/*
 *	Internal only routines
 */

extern void		task_init();
extern void		task_reference();
extern void		task_deallocate();
extern kern_return_t	task_hold();
extern kern_return_t	task_dowait();
extern kern_return_t	task_release();
extern kern_return_t	task_halt();

extern kern_return_t	task_suspend_nowait();
extern task_t		kernel_task_create();

extern task_t	kernel_task;

#endif	/* _KERN_TASK_H_ */
