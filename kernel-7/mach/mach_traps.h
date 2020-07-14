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
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 */
/*
 *	Definitions of general Mach system traps.
 *
 *	IPC traps are defined in <mach/message.h>.
 *	Kernel RPC functions are defined in <mach/mach_interface.h>.
 */

#ifndef	_MACH_MACH_TRAPS_H_
#define _MACH_MACH_TRAPS_H_

#import <mach/mach_types.h>
#import <mach/message.h>

mach_port_t		mach_reply_port(void);
mach_port_t		mach_thread_self(void);
mach_port_t		mach_task_self(void);
mach_port_t		mach_host_self(void);
port_t			(task_self)(void);
port_t			task_notify(void);
port_t			thread_self(void);
port_t			thread_reply(void);
host_t			host_self(void);
boolean_t		swtch(void);
boolean_t		swtch_pri(int pri);
kern_return_t		thread_switch(mach_port_t thread_name, int option,
				      mach_msg_timeout_t option_time);
kern_return_t		map_fd(int fd, vm_offset_t offset, vm_offset_t *va,
			       boolean_t findspace, vm_size_t size);

#if defined(KERNEL_PRIVATE)
host_priv_t		host_priv_self(void);
port_t			device_master_self(void);
port_t			_event_port_by_tag(int num);
port_t			_lookupd_port(port_name_t name),
			_lookupd_port1(port_name_t name);
port_t			task_by_pid(int pid);
kern_return_t		init_process(void);
int		mach_swapon(char *filename, int flags, long lowat, long hiwat);
#import <kern/time_stamp.h>
kern_return_t	kern_timestamp(struct	tsval	*tsp);
#endif /* KERNEL_PRIVATE */

#endif	/* _MACH_MACH_TRAPS_H_ */
