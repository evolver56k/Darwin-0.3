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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992,1991,1990,1989,1988 Carnegie Mellon University
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
 *	File:	mach/mach_types.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1986
 *
 *	Mach external interface definitions.
 *
 */

#ifndef	_MACH_MACH_TYPES_H_
#define _MACH_MACH_TYPES_H_

#import <mach/host_info.h>
#import <mach/machine.h>
#import <mach/machine/vm_types.h>
#import <mach/memory_object.h>
#import <mach/port.h>
#import <mach/processor_info.h>
#import <mach/task_info.h>
#import <mach/task_special_ports.h>
#import <mach/thread_info.h>
#import <mach/thread_special_ports.h>
#import <mach/thread_status.h>
#import <mach/time_value.h>
#import <mach/vm_attributes.h>
#import <mach/vm_inherit.h>
#import <mach/vm_prot.h>
#import <mach/vm_statistics.h>

#if	_KERNEL && !MACH_USER_API	/* KERNEL_PRIVATE */
#import <kern/task.h>		/* for task_array_t */
#import <kern/thread.h>		/* for thread_array_t */
#import <kern/processor.h>	/* for processor_array_t,
				       processor_set_array_t,
				       processor_set_name_array_t */
#import <vm/vm_user.h>
#import <vm/vm_object.h>
typedef vm_map_t	vm_task_t;
#else	/* _KERNEL */			/* KERNEL_PRIVATE */
typedef	mach_port_t	task_t;
typedef task_t		*task_array_t;
typedef	task_t		vm_task_t;
typedef task_t		ipc_space_t;
typedef	mach_port_t	thread_t;
typedef	thread_t	*thread_array_t;
typedef mach_port_t	host_t;
typedef mach_port_t	host_priv_t;
typedef mach_port_t	processor_t;
typedef mach_port_t	*processor_array_t;
typedef mach_port_t	processor_set_t;
typedef mach_port_t	processor_set_name_t;
typedef mach_port_t	*processor_set_array_t;
typedef mach_port_t	*processor_set_name_array_t;

#ifndef TASK_NULL
#define	TASK_NULL		((task_t) 0)
#endif

#ifndef THREAD_NULL
#define	THREAD_NULL		((thread_t) 0)
#endif

#ifndef PROCESSOR_NULL
#define	PROCESSOR_NULL		((processor_t) 0)
#endif

#ifndef PROCESSOR_SET_NULL
#define	PROCESSOR_SET_NULL	((processor_set_t) 0)
#endif

#endif	/* _KERNEL */

/*
 *	Backwards compatibility, for those programs written
 *	before mach/{std,mach}_types.{defs,h} were set up.
 */
#import <mach/std_types.h>

#endif	/* _MACH_MACH_TYPES_H_ */
