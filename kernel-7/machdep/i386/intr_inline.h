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
 * Inlines for interrupt chip access.
 *
 * HISTORY
 *
 * 25 June 1992 ? at NeXT
 *	Created.
 */
 
#import <machdep/i386/intr.h>
#import <machdep/i386/io_inline.h>

typedef union {
    intr_icw1_t		icw1;
    intr_icw2_t		icw2;
    intr_icw3m_t	icw3m;
    intr_icw3s_t	icw3s;
    intr_icw4_t		icw4;
    intr_ocw1_t		ocw1;
    intr_ocw2_t		ocw2;
    intr_ocw3_t		ocw3;
    unsigned char	iodata;
} cw_conv_t;

static
initialize_master(
    intr_icw1_t		icw1,
    intr_icw2_t		icw2,
    intr_icw3m_t	icw3,
    intr_icw4_t		icw4
)
{
    cw_conv_t		tconv;
    
    tconv.icw1 = icw1;
    outb(INTR_PRIMARY_PORT, tconv.iodata);
    
    tconv.icw2 = icw2;
    outb(INTR_SECONDARY_PORT, tconv.iodata);
    
    if (!icw1.no_slaves) {
	tconv.icw3m = icw3;
	outb(INTR_SECONDARY_PORT, tconv.iodata);
    }
    
    if (icw1.ic4) {
	tconv.icw4 = icw4;
	outb(INTR_SECONDARY_PORT, tconv.iodata);
    }

    // Set up status register to read ISR instead of IRR.
    tconv.iodata = 0;
    tconv.ocw3.read_reg = INTR_OCW3_READ_ISR;
    tconv.ocw3.set_to_one = 1;
    tconv.ocw3.smm = INTR_OCW3_RESET_SMM;
    outb(INTR_PRIMARY_PORT, tconv.iodata);
}

static
initialize_slave(
    intr_icw1_t		icw1,
    intr_icw2_t		icw2,
    intr_icw3s_t	icw3,
    intr_icw4_t		icw4
)
{
    cw_conv_t		tconv;
    
    tconv.icw1 = icw1;
    outb(INTR2_PRIMARY_PORT, tconv.iodata);
    
    tconv.icw2 = icw2;
    outb(INTR2_SECONDARY_PORT, tconv.iodata);
    
    if (!icw1.no_slaves) {
	tconv.icw3s = icw3;
	outb(INTR2_SECONDARY_PORT, tconv.iodata);
    }
    
    if (icw1.ic4) {
	tconv.icw4 = icw4;
	outb(INTR2_SECONDARY_PORT, tconv.iodata);
    }
    
    // Set up status register to read ISR instead of IRR.
    tconv.iodata = 0;
    tconv.ocw3.read_reg = INTR_OCW3_READ_ISR;
    tconv.ocw3.set_to_one = 1;
    tconv.ocw3.smm = INTR_OCW3_RESET_SMM;
    outb(INTR2_PRIMARY_PORT, tconv.iodata);
}

static inline
void
send_eoi_command(
    intr_ocw2_t		ocw2
)
{
    cw_conv_t		tconv;

    tconv.ocw2 = ocw2;

    outb(INTR_PRIMARY_PORT, tconv.iodata);
    outb(INTR2_PRIMARY_PORT, tconv.iodata);
}

static inline
void
set_master_mask(
    intr_ocw1_t		ocw1
)
{
    outb(INTR_SECONDARY_PORT, ocw1.mask);
}

static inline
void
set_slave_mask(
    intr_ocw1_t		ocw1
)
{
    outb(INTR2_SECONDARY_PORT, ocw1.mask);
}

static inline
void
set_master_elcr(
    intr_elcr_t		elcr
)
{
    outb(INTR_ELCR_PORT, elcr.mask);
}

static inline
void
set_slave_elcr(
    intr_elcr_t		elcr
)
{
    outb(INTR2_ELCR_PORT, elcr.mask);
}

static inline
unsigned char
get_master_isr(
    void
)
{
    return inb(INTR_PRIMARY_PORT);
}

static inline
unsigned char
get_slave_isr(
    void
)
{
    return inb(INTR2_PRIMARY_PORT);
}
