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
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log: mach_init.c,v $
 * Revision 1.1.1.1  1999/04/14 23:19:15  wsanchez
 * Import of Libc-78-8
 *
 * Revision 1.1.1.1.38.3  1999/03/16 15:47:13  wsanchez
 * Substitute License
 *
 * Revision 1.3  89/06/13  16:07:25  mrt
 * 	Changed references to task_data to thread_reply as task_data() is
 * 	no longer exported from the kernel. Removed setting of
 * 	NameServerPort, the correct name is name_server_port.
 * 	[89/05/28            mrt]
 * 
 * Revision 1.2  89/05/05  18:45:30  mrt
 * 	Cleanup for Mach 2.5
 * 
 * 23-Nov-87  Mary Thompson (mrt) at Carnegie Mellon
 *	removed includes of <servers/msgn.h> and <servers/netname.h>
 *	as they are no longer used.
 *
 * 5-Oct-87   Mary Thompson (mrt) at Carnegie Mellon
 *	Added an extern void definition of mig_init to keep
 *	lint happy
 *
 * 30-Jul-87  Mary Thompson (mrt) at Carnegie Mellon
 *	Changed the intialization of the mig_reply_port to be
 *	a call to mig_init instead init_mach.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Changed initialization of mach interface, because the
 *	new mig doesn't export an alloc_port_mach function.
 *
 * 15-May-87  Mary Thompson
 *	Removed include of sys/features.h and conditional
 *	compliations
 *
 *  4-May-87  Mary Thompson
 *	vm_deallocted the init_ports array so that brk might 
 *	have a chance to be correct.
 *
 * 24-Apr-87  Mary Thompson
 *	changed type of mach_init_ports_count to unsigned int
 *
 * 12-Nov-86	Mary Thompson
 *	Added initialization call to init_netname the new
 *	interface for the net name server.
 *
 *  7-Nov-86  Michael Young
 * 	Add "service_port" 
 *
 *  7-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added initialization for MACH_IPC, using mach_ports_lookup.
 *
 * 29-Aug-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added other interesting global ports.
 */

#include <mach/mach_init.h>
#include <mach/mach.h>
#include <mach/machine/ndr.h>
#include <bootstrap.h>

#if	NeXT
static int	mach_init_doit(int isforkedchild);
#endif	NeXT

extern void mig_init();

extern port_t		task_self_;

extern port_t		name_server_port;
extern port_t		bootstrap_port;

extern vm_size_t	vm_page_size;

#ifndef NELEM
#define	NELEM(x)		(sizeof(x)/sizeof((x)[0]))
#endif	NELEM

#ifndef MIN
#define	MIN(a,b)		((a) < (b) ? (a) : (b))
#endif MIN

#if	NeXT
int	fork_mach_init()	// called in child
{
	mach_init_doit(TRUE);
}
int	mach_init()	// called in newly exec'ed process
{
	mach_init_doit(FALSE);
}
#endif	NeXT

#if	NeXT
static int	mach_init_doit(int isforkedchild)
#else
int		mach_init()
#endif	NeXT
{
	int i;
	vm_statistics_data_t vm_stat;

#if	NeXT
	/*
	 * The first cproc_self() that an execed program does (usually in 
	 * mig_get_reply_port()) can possibly return garbage, especially
	 * if its parent was multithreaded.  If we're forking a child, we
	 * need to preserve cproc_self so that both parent and child will
	 * see the same stack.
	 */
	if (isforkedchild == FALSE)
		cthread_set_self(0);
#endif	NeXT
	/*
	 * undefine the macros defined in mach_init.h so that we
	 * can make the real kernel calls
	 */

#undef task_self()

	/*
	 *	Get the important ports into the cached values,
	 *	as required by "mach_init.h".
	 */
	 
	task_self_ = task_self();

	/*
	 *	Initialize the single mig reply port
	 */

	mig_init(0);

	/*
	 *	Cache some other valuable system constants
	 */

	vm_statistics(task_self_, &vm_stat);
	vm_page_size = vm_stat.pagesize;
	
	/*
	 *	Find those ports important to every task.
	 */
	if (      task_get_bootstrap_port(task_self_, &bootstrap_port)
	       == KERN_SUCCESS
	    && bootstrap_port != PORT_NULL)
	{
		(void) bootstrap_look_up(bootstrap_port,
	   				 "NetMessage",
					 &name_server_port);
	} 	
	
	/* Compatability... */	
	NameServerPort = name_server_port;

	return(0);
}

extern int		(*mach_init_routine)();
