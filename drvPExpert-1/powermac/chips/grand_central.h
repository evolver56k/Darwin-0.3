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

#ifndef _INTERRUPT_GC_H_
#define _INTERRUPT_GC_H_

#include <ppc/spl.h>

/* Physical address and size of IO control registers region */

/* Grand Central Items */

#define GRAND_CENTRAL_SIZE	0x20000		/* 128k */

/* Interrupts */
#define	GC_INTERRUPT_EVENTS	0x00020
#define	GC_INTERRUPT_MASK	0x00024
#define	GC_INTERRUPT_CLEAR	0x00028
#define	GC_INTERRUPT_LEVELS	0x0002C

/* Interrupt controller */
#define	GC_INTERRUPT_EVENTS_REG	((v_u_long *) POWERMAC_IO(powermac_io_info.io_base_phys + GC_INTERRUPT_EVENTS))

#define	GC_INTERRUPT_MASK_REG	((v_u_long *) POWERMAC_IO(powermac_io_info.io_base_phys + GC_INTERRUPT_MASK))

#define	GC_INTERRUPT_CLEAR_REG	((v_u_long *) POWERMAC_IO(powermac_io_info.io_base_phys + GC_INTERRUPT_CLEAR))

#define GC_INTERRUPT_LEVELS_REG	((v_u_long *) POWERMAC_IO(powermac_io_info.io_base_phys + GC_INTERRUPT_LEVELS))

/* DBDMA Channel Map */
extern powermac_dbdma_channels_t gc_dbdma_channels;

extern struct powermac_interrupt *gc_interrupts;
extern struct powermac_interrupt *gc_via1_interrupts;
extern int ngc_via_interrupts;
extern int ngc_interrupts;

/* prototypes */

#ifndef __ASSEMBLER__

void	gc_interrupt_initialize(void);
void gc_via1_interrupt(int device, void *ssp, void *arg);

#endif /* ndef __ASSEMBLER__ */

#endif /* _INTERRUPT_GC_H_ */
