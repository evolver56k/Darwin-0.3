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
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY 
 * $Log: mach_init.h,v $
 * Revision 1.1.1.1  1999/04/14 23:19:16  wsanchez
 * Import of Libc-78-8
 *
 * Revision 1.1.1.1.38.3  1999/03/16 15:47:17  wsanchez
 * Substitute License
 *
 * 08-May-90  Morris Meyer (mmeyer) at NeXT
 *	Added prototypes for thread_reply(), host_self(), 
 *	host_priv_self().task_notify(), thread_self(), init_process(),
 *	swtch_pri(), swtch(), thread_switch(), mach_swapon().
 *
 * 22-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Removed task_notify() macro (can't use it on NeXT, as it must
 *	be allocated by those who needed.  The trap is the best way to
 *	retrieve this.
 *
 *	Added bootstrap port.
 *
 * Revision 1.3  89/06/13  16:45:00  mrt
 * 	Defined macros for thread_reply and made task_data be another
 * 	name for thread_reply, as task_data() is no longer exported from
 * 	the kernel.
 * 	[89/05/28            mrt]
 * 
 * 	Moved definitions of round_page and trunc_page to
 * 	here from mach/vm_param.h
 * 	[89/05/18            mrt]
 * 
 * Revision 1.2  89/05/05  18:45:39  mrt
 * 	Cleanup and change includes for Mach 2.5
 * 	[89/04/28            mrt]
 * 
 */
/*
 *	Items provided by the Mach environment initialization.
 */

#ifndef	_MACH_INIT_H_
#define	_MACH_INIT_H_	1

#import <mach/mach_types.h>

/*
 *	Kernel-related ports; how a task/thread controls itself
 */

extern	port_t	task_self_;

#define	task_self()	task_self_
#define	current_task()	task_self()

#import <mach/mach_traps.h>

extern	kern_return_t	init_process(void);

extern int		mach_swapon(char *filename, int flags, 
					long lowat, long hiwat);

extern host_priv_t	host_priv_self(void);

#define	task_data()	thread_reply()

extern void slot_name(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype, 
	char **cpu_name, char **cpu_subname);
/*
 *	Other important ports in the Mach user environment
 */

#define	NameServerPort	name_server_port	/* compatibility */

extern	port_t	bootstrap_port;
extern	port_t	name_server_port;

/*
 *	Globally interesting numbers
 */

extern	vm_size_t	vm_page_size;

#define round_page(x)	((((vm_offset_t)(x) + (vm_page_size - 1)) / vm_page_size) * vm_page_size)
#define trunc_page(x)	((((vm_offset_t)(x)) / vm_page_size) * vm_page_size)

#endif	/* _MACH_INIT_H_ */
