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
 * Interrupt handling exported definitions.
 *
 * HISTORY
 *
 * 10 July 1993 ? at NeXT
 *	Removed 'arg' from interrupt support.
 *	Removed old obsolete API.
 *	Cleaned up somewhat.
 * 5 July 1993 ? at NeXT
 *	Removed software interrupt support.
 * 26 August 1992 ? at NeXT
 *	Major rev for driverkit support.
 * 22 August 1992 ? at NeXT
 *	Added software interrupts.
 * 20 Aug 1992	Joe Pasqua
 *	Added protoypes for intr_enable_irq/disable_irq.
 * 1 June 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

/*
 * A total of eight interrupt
 * levels are provided.
 */
#define INTR_NIPL	8

#define INTR_IPL0	0
#define INTR_IPL1	1
#define INTR_IPL2	2
#define INTR_IPL3	3
#define INTR_IPL4	4
#define INTR_IPL5	5
#define INTR_IPL6	6
#define INTR_IPL7	7

#define INTR_IPLHI	INTR_IPL7

typedef void
(*intr_handler_t)(
	unsigned int		which,
	void			*state,
	int			old_ipl
);

/*
 * Exported routines.
 */

/*
 * Core routines.
 */
void
intr_initialize(void);

void
intr_handler(
	void		*state
);
				
/*
 * Hardware IRQ support.
 */
boolean_t
intr_register_irq(
	int		irq,
	intr_handler_t	routine,
	unsigned int	which,
	int		ipl
);

boolean_t
intr_unregister_irq(
	int		irq
);

boolean_t
intr_enable_irq(
	int		irq
);

boolean_t
intr_disable_irq(
	int		irq
);

boolean_t
intr_change_ipl(
	int		irq,
	int		ipl
);

boolean_t
intr_change_mode(
	int		irq,
	boolean_t	level_trig
);
				
/*
 * Miscellaneous routines.
 */
boolean_t
intr_disbl(void);

boolean_t
intr_enbl(
	boolean_t	enable
);
