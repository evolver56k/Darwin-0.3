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
#ifndef _POWERMAC_INTERRUPTS_H_
#define _POWERMAC_INTERRUPTS_H_

#include <mach/ppc/boolean.h>
#include <mach/ppc/thread_status.h> /* for struct ppc_saved_state */
#include <machdep/ppc/machspl.h>

/*
 * Generic Power Macintosh Interrupts
 * (Things common across all systems)
 *
 */

/* DMA Interrupts */
#define	PMAC_DMA_SCSI0		0
#define	PMAC_DMA_SCSI1		1
#define	PMAC_DMA_AUDIO_OUT	2
#define	PMAC_DMA_AUDIO_IN	3
#define	PMAC_DMA_FLOPPY		4
#define	PMAC_DMA_ETHERNET_TX	5
#define	PMAC_DMA_ETHERNET_RX	6
#define	PMAC_DMA_SCC_A_TX	7
#define	PMAC_DMA_SCC_A_RX	8
#define	PMAC_DMA_SCC_B_TX	9
#define	PMAC_DMA_SCC_B_RX	10
#define PMAC_DMA_IDE0           11
#define PMAC_DMA_IDE1           12

#define	PMAC_DMA_START		0
#define	PMAC_DMA_END		125

/* Device Interrupts */

#define	PMAC_DEV_SCSI0		128
#define	PMAC_DEV_SCSI1		129
#define	PMAC_DEV_ETHERNET	130
#define	PMAC_DEV_SCC_A		131
#define	PMAC_DEV_SCC_B		132
#define	PMAC_DEV_AUDIO		134
#define	PMAC_DEV_VIA1		135
#define	PMAC_DEV_FLOPPY		136
#define	PMAC_DEV_NMI		137
#define PMAC_DEV_IDE0           138
#define PMAC_DEV_IDE1           139
#define PMAC_DEV_IN             140
#define PMAC_DEV_ADB            141
#define	PMAC_DEV_VIA2		142
#define	PMAC_DEV_VIA3		143

#define	PMAC_DEV_SCC		PMAC_DEV_SCC_A	/* Older SCC chip. */

/* Add-on cards */

#define	PMAC_DEV_CARD0		256
#define	PMAC_DEV_CARD1		257
#define	PMAC_DEV_CARD2		258
#define	PMAC_DEV_CARD3		259
#define	PMAC_DEV_CARD4		260
#define	PMAC_DEV_CARD5		261
#define	PMAC_DEV_CARD6		262
#define	PMAC_DEV_CARD7		263
#define	PMAC_DEV_CARD8		264
#define	PMAC_DEV_CARD9		265
#define	PMAC_DEV_CARD10		266
#define PMAC_DEV_CARD11         267

#define	PMAC_DEV_PDS		270	/* Processor Direct Slot - Nubus only */

/* Some NuBus aliases.. */
#define	PMAC_DEV_NUBUS0		PMAC_DEV_CARD0
#define	PMAC_DEV_NUBUS1		PMAC_DEV_CARD1
#define	PMAC_DEV_NUBUS2		PMAC_DEV_CARD2
#define	PMAC_DEV_NUBUS3		PMAC_DEV_CARD3

#define	PMAC_DEV_HZTICK		300
#define	PMAC_DEV_TIMER1		301
#define	PMAC_DEV_TIMER2		302
#define	PMAC_DEV_VBL		303	/* VBL Interrupt */

#define	PMAC_DEV_START		128
#define	PMAC_DEV_END		511

/* Macro for accessing VIA registers */
#define via_reg(X) reg8(X)

struct powermac_interrupt {
	void		(*i_handler)(int, void *, void *);
	spl_t		i_level;
	void *		i_arg;
	int		i_device;
};

extern unsigned int (*pmac_int_to_number)(int index);

extern void	(*pmac_register_int)(int interrupt, spl_t level,
				     void (*handler)(int, void *, void *), 
				     void *);

extern void	(*pmac_interrupt)(int type, struct ppc_saved_state *ssp,
					unsigned int dsisr, unsigned int dar);

extern boolean_t (*pmac_enable_irq)(int irq);
extern boolean_t (*pmac_disable_irq)(int irq);

#endif /* !POWERMAC_INTERRUPTS_H_ */
