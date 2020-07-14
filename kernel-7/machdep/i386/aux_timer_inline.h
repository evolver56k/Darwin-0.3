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
 * Auxillary event timing using TIMER 2.
 *
 * HISTORY
 *
 * 17 June 1992 ? at NeXT
 *	Created.
 */

#import <machdep/i386/timer_inline.h>

static inline
aux_timer_init(
    void
)
{
    timer_ctl_reg_t	reg;

    reg = (timer_ctl_reg_t) { 0 };

    reg.mode = TIMER_EVENTMODE;
    reg.rw = TIMER_CTL_RW_BOTH;
    reg.sel = TIMER_CNT2_SEL;
    timer_set_ctl(reg);

    outb(0x61, inb(0x61) | 0x3);	/* XXX enable the TIMER2 gate */
}

static inline
aux_timer_start(
    void
)
{
    timer_write(TIMER_CNT2_SEL, TIMER_COUNT_MAX);
}

static inline
aux_timer_stop(
    unsigned int *	cnt
)
{
    timer_cnt_val_t	leftover;

    timer_latch(TIMER_CNT2_SEL); leftover = timer_read(TIMER_CNT2_SEL);

    *cnt = TIMER_COUNT_MAX - leftover;
}
