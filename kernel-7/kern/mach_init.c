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
 *
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 */

#import <mach/features.h>

#import <mach/machine.h>
#import <sys/version.h>
#import <kern/thread.h>
#import <kern/task.h>
#import <sys/param.h>
#import <mach/vm_param.h>
#import <vm/vm_kern.h>
#import <machine/spl.h>
#import <kern/miniMon.h>

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 *	     - process 2 to page out
 */
thread_t	first_thread;

thread_t setup_main()
/*
 *	first_addr contains the first available physical address
 *	running in virtual memory on the interrupt stack
 *
 *	returns initial thread to run
 */
{
	extern vm_offset_t	virtual_avail;
	vm_offset_t		end_stack, cur_stack;
	int			i;
	extern void	main();
	extern void	vm_mem_init();
#if	MACH_NET
	extern void	mach_net_init();
#endif	MACH_NET
	unsigned s;
	
	machine_clock_init();
	sched_init();

	vm_mem_init();

	init_timers();
	init_timeout();

	startup(virtual_avail);
	printf("minimum quantum is %d ms\n", (1000 / hz) * min_quantum);

	machine_info.max_cpus = NCPUS;
	machine_info.memory_size = roundup(mem_size, 1024*1024);
	machine_info.avail_cpus = 0;
	machine_info.major_version = KERNEL_MAJOR_VERSION;
	machine_info.minor_version = KERNEL_MINOR_VERSION;

	/*
	 *	Initialize the task and thread subsystems.
	 */

	uzone_init();

	ipc_bootstrap();

	cpu_up(master_cpu);	/* signify we are up */
#if	MACH_NET
	mach_net_init();
#endif	MACH_NET
	task_init();
	thread_init();
	swapper_init();
	ipc_init();
	vnode_pager_init();
#if	MACH_HOST
	pset_sys_init();
#endif	MACH_HOST

	(void) thread_create(kernel_task, &first_thread);
	thread_deallocate(first_thread);
	thread_start(first_thread, main);
	thread_doswapin(first_thread);
	first_thread->state |= TH_RUN;
	(void) thread_resume(first_thread);	

	/*
	 *	Tell the pmap system that our cpu is using the kernel pmap
	 */
	s = splvm();
	PMAP_ACTIVATE(kernel_pmap, first_thread, cpu_number());
	splx(s);
	
#if 	DRIVERKIT
	dev_server_init();
#endif	DRIVERKIT

	miniMonInit();

	/*
	 *	Return to assembly code to start the first process.
	 */

	return(first_thread);
}
