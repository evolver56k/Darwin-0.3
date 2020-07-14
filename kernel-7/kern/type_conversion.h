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
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
#ifndef	_KERN_TYPE_CONVERSION_H_
#define	_KERN_TYPE_CONVERSION_H_

#import <mach/port.h>
#import <kern/task.h> 
#import <kern/thread.h>
#import <vm/vm_map.h>
#import <kern/host.h>
#import <kern/processor.h>

/*
 *	Conversion routines, to let Matchmaker do this for
 *	us automagically.
 */

extern task_t convert_port_to_task( /* port_t x */ );
extern thread_t convert_port_to_thread( /* port_t x */ );
extern vm_map_t convert_port_to_map( /* port_t x */ );
extern port_t convert_task_to_port( /* task_t x */ );
extern port_t convert_thread_to_port( /* thread_t x */ );

extern host_t convert_port_to_host( /* port_t x */ );
extern host_t convert_port_to_host_priv( /* port_t x */ );
extern processor_t convert_port_to_processor( /* port_t x */ );
extern processor_set_t convert_port_to_pset( /* port_t x */ );
extern processor_set_t convert_port_to_pset_name( /* port_t x */ );
extern port_t convert_host_to_port( /* host_t x */ );
extern port_t convert_processor_to_port( /* processor_t x */ );
extern port_t convert_pset_to_port( /* processor_set_t x */ );
extern port_t convert_pset_name_to_port( /* processor_set_t x */ );

#endif	/* _KERN_TYPE_CONVERSION_H_ */
