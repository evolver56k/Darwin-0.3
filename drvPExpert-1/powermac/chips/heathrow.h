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

#ifndef _INTERRUPT_HEATHROW_H_
#define _INTERRUPT_HEATHROW_H_

#include <ppc/spl.h>

#include <machdep/ppc/dbdma.h>


/* Physical address and size of IO control registers region */

/* Heathrow Items */

#define HEATHROW_SIZE	0x80000		/* 512k */

/* Interrupts */
#define HEATHROW_INT_EVENTS1    0x00020
#define HEATHROW_INT_EVENTS2    0x00010
#define HEATHROW_INT_MASK1      0x00024
#define HEATHROW_INT_MASK2      0x00014
#define HEATHROW_INT_CLEAR1     0x00028
#define HEATHROW_INT_CLEAR2     0x00018
#define HEATHROW_INTT_LEVELS1   0x0002C
#define HEATHROW_INTT_LEVELS2   0x0001C


#define	HEATHROW_VIA1_AUXCONTROL	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x01600))
#define	HEATHROW_VIA1_T1COUNTERLOW	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00800))
#define	HEATHROW_VIA1_T1COUNTERHIGH	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00A00))
#define	HEATHROW_VIA1_T1LATCHLOW	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00C00))
#define	HEATHROW_VIA1_T1LATCHHIGH	(POWERMAC_IO(powermac_io_info.via_base_phys + 0x00E00))

#define HEATHROW_VIA1_IER		(POWERMAC_IO(powermac_io_info.via_base_phys + 0x01c00))
#define HEATHROW_VIA1_IFR		(POWERMAC_IO(powermac_io_info.via_base_phys + 0x01a00))
#define HEATHROW_VIA1_PCR		(POWERMAC_IO(powermac_io_info.via_base_phys + 0x01800))

/* Interrupt controller */
#define HEATHROW_INTERRUPT_EVENTS1_REG      ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_EVENTS1))
#define HEATHROW_INTERRUPT_EVENTS2_REG      ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_EVENTS2))

#define HEATHROW_INTERRUPT_MASK1_REG        ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_MASK1))
#define HEATHROW_INTERRUPT_MASK2_REG        ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_MASK2))

#define HEATHROW_INTERRUPT_CLEAR1_REG       ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_CLEAR1))
#define HEATHROW_INTERRUPT_CLEAR2_REG       ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_CLEAR2))

#define HEATHROW_INTERRUPT_LEVELS1_REG      ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_LEVELS1))
#define HEATHROW_INTERRUPT_LEVELS2_REG      ((v_u_long *) POWERMAC_IO(PCI_IO_BASE_PHYS + HEATHROW_INT_LEVELS2))

/* DBDMA Channel Map */
extern powermac_dbdma_channels_t heathrow_dbdma_channels;

/* Storage for interrupt table pointers */
extern struct powermac_interrupt *heathrow_interrupts;
extern struct powermac_interrupt *heathrow_via1_interrupts;
extern int nheathrow_via_interrupts;
extern int nheathrow_interrupts;
  
/* prototypes */

#ifndef __ASSEMBLER__

void   heathrow_interrupt_initialize(void);
void heathrow_via1_interrupt(int device, void *ssp, void *arg);

#endif /* ndef __ASSEMBLER__ */

#endif /* _INTERRUPT_HEATHROW_H_ */
