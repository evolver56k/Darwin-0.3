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
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  It
 * is padded so that new data can take the place of storage occupied by part
 * of it.
 */
#define	MACH_INIT_SLOTS		1
#include <mach/mach_init.h>
#include <mach/mach.h>

extern int mach_init();

port_t		task_self_ = 1;
port_t		name_server_port = PORT_NULL;
port_t		xxx_environment_port = PORT_NULL;	/* Obsolete */
/*
 * Bootstrap port.
 */
port_t		bootstrap_port = PORT_NULL;	/* Was service_port */
vm_size_t	vm_page_size = 0;
port_set_name_t	xxx_PORT_ENABLED = PORT_NULL;	/* Obsolete */
port_array_t	_old_mach_init_ports = { 0 };	/* Obsolete */
unsigned int	_old_mach_init_ports_count = 0;	/* Obsolete */
int		(*mach_init_routine)() = mach_init;

asm(".data");
asm("_old_next_minbrk:");
#ifdef SHLIB
asm("old_next_minbrk:	.long	0");
#else
asm("old_next_minbrk:	.long	0");
#endif
asm("_old_next_curbrk:");
#ifdef SHLIB
asm("old_next_curbrk:	.long	0");
#else
asm("old_next_curbrk:	.long	0");
#endif

/* global data padding, must NOT be static */
char _mach_data_padding[212] = { 0 };

