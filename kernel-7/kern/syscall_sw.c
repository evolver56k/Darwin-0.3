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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
 * HISTORY
 */

#include <mach_ipc_compat.h>

#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/syscall_sw.h>

/* Include declarations of the trap functions. */
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <kern/syscall_subr.h>
#include <kern/time_stamp.h>


/*
 *	To add a new entry:
 *		Add an "MACH_TRAP(routine, arg count)" to the table below.
 *
 *		Add trap definition to mach/syscall_sw.h and
 *		recompile user library.
 *
 * WARNING:	If you add a trap which requires more than 7
 *		parameters, mach/ca/syscall_sw.h and ca/trap.c both need
 *		to be modified for it to work successfully on an
 *		RT.  Similarly, mach/mips/syscall_sw.h and mips/locore.s
 *		need to be modified before it will work on Pmaxen.
 *
 * WARNING:	Don't use numbers 0 through -9.  They (along with
 *		the positive numbers) are reserved for Unix.
 */

mach_port_t	null_port()
{
	return(MACH_PORT_NULL);
}

kern_return_t	kern_invalid()
{
	return(KERN_INVALID_ARGUMENT);
}



#import <driverkit.h>

#if	!DRIVERKIT
#define	device_master_self	kern_invalid
#endif

mach_trap_t	mach_trap_table[] = {
	MACH_TRAP(kern_invalid, 0),		/* 0 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 1 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 2 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 3 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 4 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 5 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 6 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 7 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 8 */		/* Unix */
	MACH_TRAP(kern_invalid, 0),		/* 9 */		/* Unix */

#if	MACH_IPC_COMPAT
	MACH_TRAP(task_self, 0),		/* 10 */	/* obsolete */
	MACH_TRAP(thread_reply, 0),		/* 11 */	/* obsolete */
	MACH_TRAP(task_notify, 0),		/* 12 */	/* obsolete */
	MACH_TRAP(thread_self, 0),		/* 13 */	/* obsolete */
#else	/* MACH_IPC_COMPAT */
	MACH_TRAP(null_port, 0),		/* 10 */
	MACH_TRAP(null_port, 0),		/* 11 */
	MACH_TRAP(null_port, 0),		/* 12 */
	MACH_TRAP(null_port, 0),		/* 13 */
#endif	/* MACH_IPC_COMPAT */
	MACH_TRAP(kern_invalid, 0),		/* 14 */
	MACH_TRAP(kern_invalid, 0),		/* 15 */
	MACH_TRAP(kern_invalid, 0),		/* 16 */
	MACH_TRAP(kern_invalid, 0),		/* 17 */
	MACH_TRAP(kern_invalid, 0),		/* 18 */
	MACH_TRAP(kern_invalid, 0),		/* 19 */

#if	MACH_IPC_COMPAT
	MACH_TRAP_STACK(msg_send_trap, 4),	/* 20 */	/* obsolete */
	MACH_TRAP_STACK(msg_receive_trap, 5),	/* 21 */	/* obsolete */
	MACH_TRAP_STACK(msg_rpc_trap, 6),	/* 22 */	/* obsolete */
#else	/* MACH_IPC_COMPAT */
	MACH_TRAP(kern_invalid, 0),		/* 20 */
	MACH_TRAP(kern_invalid, 0),		/* 21 */
	MACH_TRAP(kern_invalid, 0),		/* 22 */
#endif	/* MACH_IPC_COMPAT */
	MACH_TRAP(kern_invalid, 0),		/* 23 */
	MACH_TRAP_STACK(mach_msg_simple_trap, 5),
						/* 24 */
	MACH_TRAP_STACK(mach_msg_trap, 7),	/* 25 */
	MACH_TRAP(mach_reply_port, 0),		/* 26 */
	MACH_TRAP(mach_thread_self, 0),		/* 27 */
	MACH_TRAP(mach_task_self, 0),		/* 28 */
	MACH_TRAP(mach_host_self, 0),		/* 29 */

	MACH_TRAP(kern_invalid, 0),		/* 30 */
	MACH_TRAP(kern_invalid, 0),		/* 31 */
	MACH_TRAP_STACK(mach_msg_overwrite_trap, 9), /* 32 */
	MACH_TRAP(task_by_pid, 1),		/* 33 */
	MACH_TRAP(kern_invalid, 0),		/* 34 */
	MACH_TRAP(_lookupd_port, 1),		/* 35 */
	MACH_TRAP(_lookupd_port1, 1),		/* 36 */
	MACH_TRAP(kern_invalid, 0),		/* 37 */
	MACH_TRAP(kern_invalid, 0),		/* 38 */

	MACH_TRAP(kern_invalid, 0),		/* 39 */
	MACH_TRAP(mach_swapon, 4),		/* 40 */

	MACH_TRAP(init_process, 0),		/* 41 */
	MACH_TRAP(kern_invalid, 0),		/* 42 */
	MACH_TRAP(map_fd, 5),			/* 43 */
	MACH_TRAP(kern_invalid, 0),		/* 44 */
	MACH_TRAP(mach_swapon, 4),		/* 45 */
	MACH_TRAP(kern_invalid, 0),		/* 46 */
	MACH_TRAP(kern_invalid, 0),		/* 47 */
	MACH_TRAP(kern_invalid, 0),		/* 48 */
	MACH_TRAP(kern_invalid, 0),		/* 49 */

	MACH_TRAP(kern_invalid, 0),		/* 50 */
	MACH_TRAP(kern_timestamp, 1),		/* 51 */
	MACH_TRAP(kern_invalid, 0),		/* 52 */
	MACH_TRAP(kern_invalid, 0),		/* 53 */
	MACH_TRAP(kern_invalid, 0),		/* 54 */
#if	MACH_IPC_COMPAT
	MACH_TRAP(host_self, 0),		/* 55 */
	MACH_TRAP(host_priv_self, 0),		/* 56 */
#else	/* MACH_IPC_COMPAT */
	MACH_TRAP(null_port, 0),		/* 55 */
	MACH_TRAP(null_port, 0),		/* 56 */
#endif	/* MACH_IPC_COMPAT */
	MACH_TRAP(kern_invalid, 0),		/* 57 */
	MACH_TRAP(kern_invalid, 0),		/* 58 */
 	MACH_TRAP_STACK(swtch_pri, 1),		/* 59 */

	MACH_TRAP_STACK(swtch, 0),		/* 60 */
	MACH_TRAP_STACK(thread_switch, 3),	/* 61 */
	MACH_TRAP(kern_invalid, 0),		/* 62 */
	MACH_TRAP(kern_invalid, 0),		/* 63 */
	MACH_TRAP(kern_invalid, 0),		/* 64 */
	MACH_TRAP(kern_invalid, 0),		/* 65 */
	MACH_TRAP(kern_invalid, 0),		/* 66 */
	MACH_TRAP(kern_invalid, 0),		/* 67 */
	MACH_TRAP(_event_port_by_tag, 1),	/* 68 */
	MACH_TRAP(device_master_self, 0),	/* 69 */
};

int	mach_trap_count = (sizeof(mach_trap_table) / sizeof(mach_trap_table[0]));
