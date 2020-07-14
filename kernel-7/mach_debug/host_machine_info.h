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
 * Copyright 1998 Apple Computer, Inc. All rights reserved.
 *
 * History :
 * 15-Jan-1998  Umesh Vaishampayan (umeshv@apple.com)
 *	Created.
 *
 */

#ifndef	_MACH_DEBUG_HOST_MACHINE_INFO_H_
#define _MACH_DEBUG_HOST_MACHINE_INFO_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

/*
 *	Remember to update the mig type definitions
 *	in mach_debug_types.defs when adding/removing fields.
 */

typedef struct host_machine_info {
	natural_t	page_size;			/* number of bytes per page */
	natural_t	dcache_block_size;	/* number of bytes */
	natural_t	dcache_size;		/* number of bytes */
	natural_t	icache_size;		/* number of bytes */
	integer_t	caches_unified;		/* boolean_t */
	natural_t	processor_version;	/* contents of PVR on ppc */
	natural_t	cpu_clock_rate_hz;
	natural_t	bus_clock_rate_hz;
	natural_t	dec_clock_rate_hz;  /* only on ppc */
} host_machine_info_t;

#endif	/* _MACH_DEBUG_HOST_MACHINE_INFO_H_ */
