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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Interrupt handling internal definitions.
 *
 * HISTORY
 *
 * 10 July 1993 ? at NeXT
 *	Removed 'arg' from interrupt support.
 * 5 July 1993 ? at NeXT
 *	Removed software interrupt support.
 * 22 August 1992 ? at NeXT
 *	Added software interrupts.
 * 1 June 1992 ? at NeXT
 *	Created.
 */
 
typedef struct {
    unsigned short	
			mask		:16;
} intr_irq_mask_t;

#define INTR_VECT_OFF		0x40	// vector that IRQ0 corresponds to
#define INTR_NIRQ		16	// number of IRQs available
#define INTR_SLAVE_IRQ		2	// IRQ of slave
#define INTR_MASK_IRQ(x)	((unsigned short) (1 << (x)))
#define INTR_MASK_SLAVE		INTR_MASK_IRQ(INTR_SLAVE_IRQ)
#define INTR_MASK_ALL		((unsigned short) (-1 & ~INTR_MASK_SLAVE))
#define INTR_MASK_NONE		((unsigned short) (0  & ~INTR_MASK_SLAVE))

typedef struct {
    unsigned int	which;
    void		(*routine)();
    int			ipl;
} intr_dispatch_t;
