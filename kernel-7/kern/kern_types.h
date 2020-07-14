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
 * Copyright (c) 1992 Carnegie Mellon University
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

#ifndef	_KERN_KERN_TYPES_H_
#define	_KERN_KERN_TYPES_H_

#if	_KERNEL			/* KERNEL_PRIVATE */

#include <mach/port.h>		/* for mach_port_t */

/*
 *	Common kernel type declarations.
 *	These are handles to opaque data structures defined elsewhere.
 *
 *	These types are recursively included in each other`s definitions.
 *	This file exists to export the common declarations to each
 *	of the definitions, and to other files that need only the
 *	type declarations.
 */

/*
 * Task structure, from kern/task.h
 */
typedef struct task *		task_t;
#define	TASK_NULL		((task_t) 0)

typedef	mach_port_t *		task_array_t;	/* should be task_t * */

/*
 * Thread structure, from kern/thread.h
 */
typedef	struct thread *		thread_t;
#define	THREAD_NULL		((thread_t) 0)

typedef	mach_port_t *		thread_array_t;	/* should be thread_t * */

/*
 * Processor structure, from kern/processor.h
 */
typedef	struct processor *	processor_t;
#define	PROCESSOR_NULL		((processor_t) 0)

/*
 * Processor set structure, from kern/processor.h
 */
typedef	struct processor_set *	processor_set_t;
#define	PROCESSOR_SET_NULL	((processor_set_t) 0)
#endif /* _KERNEL && !MACH_USER_API */	/* KERNEL_PRIVATE */

#endif	/* _KERN_KERN_TYPES_H_ */
