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


#ifndef	_CPU_DATA_H_
#define	_CPU_DATA_H_
#if defined(KERNEL_BUILD)
#include <cpus.h>
#endif /* KERNEL_BUILD */
#include <kern/kern_types.h>

typedef struct
{
	thread_t	active_thread;
	int		preemption_level;
	int		simple_lock_count;
	int		interrupt_level;
        unsigned        flags;
} cpu_data_t;

extern cpu_data_t	cpu_data[NCPUS];

/*
 * NOTE: For non-realtime configurations, the accessor functions for
 * cpu_data fields are here, coded in C. For MACH_RT configurations,
 * the accessor functions must be defined in the machine-specific
 * cpu_data.h header file.
 */

#if	MACH_RT

#include <machine/cpu_data.h>

#else	/* MACH_RT */

#include <kern/cpu_number.h>

#if	defined(__GNUC__)

#ifndef __OPTIMIZE__
#define extern static
#endif

extern thread_t	__inline__	current_thread(void);
extern int __inline__		get_preemption_level(void);
extern int __inline__		get_simple_lock_count(void);
extern int __inline__		get_interrupt_level(void);
extern void __inline__		disable_preemption(void);
extern void __inline__		enable_preemption(void);

extern thread_t __inline__ current_thread(void)
{
	return (cpu_data[cpu_number()].active_thread);
}

extern int __inline__	get_preemption_level(void)
{
	return (0);
}

extern int __inline__	get_simple_lock_count(void)
{
	return (cpu_data[cpu_number()].simple_lock_count);
}

extern int __inline__	get_interrupt_level(void)
{
	return (cpu_data[cpu_number()].interrupt_level);
}

extern void __inline__	disable_preemption(void)
{
}

extern void __inline__	enable_preemption(void)
{
}

#ifndef	__OPTIMIZE__
#undef 	extern
#endif

#else	/* !defined(__GNUC__) */

#define current_thread()	(cpu_data[cpu_number()].active_thread)
#define get_preemption_level()	(0)
#define get_simple_lock_count()	(cpu_data[cpu_number()].simple_lock_count)
#define get_interrupt_level()	(cpu_data[cpu_number()].interrupt_level)
#define disable_preemption()	
#define enable_preemption()

#endif	/* defined(__GNUC__) */

#endif	/* MACH_RT */

#endif	/* _CPU_DATA_H_ */
