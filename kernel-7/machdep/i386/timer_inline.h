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
 * Timer (PIT) access routines.
 *
 * HISTORY
 *
 * 24 March 1992 ? at NeXT
 *	Created.
 */
 
#import <machdep/i386/timer.h>
#import <machdep/i386/io_inline.h>

#define timer_get_ctl	_timer_get_ctl_
static inline timer_ctl_reg_t
_timer_get_ctl_()
{
    union {
	timer_ctl_reg_t	reg;
	unsigned char	iodata;
    } temp;
    
    temp.iodata = inb(TIMER_CTL_PORT);
    
    return (temp.reg);
}

#define timer_set_ctl	_timer_set_ctl_
static inline void
_timer_set_ctl_(reg)
timer_ctl_reg_t	reg;
{
    union {
	timer_ctl_reg_t	reg;
	unsigned char	iodata;
    } temp;

    temp.reg = reg;
    
    outb(TIMER_CTL_PORT, temp.iodata);
}

#define timer_latch	_timer_latch_
static inline void
_timer_latch_(sel)
int		sel;
{
    timer_ctl_reg_t	reg;
    
    reg = (timer_ctl_reg_t){ 0 };
    reg.sel = sel;
    
    timer_set_ctl(reg);
}

#define TIMER_CNT_PORT(n)	\
    _timer_cnt_port_[(n)]
static const int	_timer_cnt_port_[] =
	{ TIMER_CNT0_PORT, TIMER_CNT1_PORT, TIMER_CNT2_PORT };

#define timer_read	_timer_read_
static inline timer_cnt_val_t
_timer_read_(sel)
int		sel;
{
    union {
	timer_cnt_val_t	val;
	unsigned char	iodata[2];
    } temp;

    temp.iodata[0] = inb(TIMER_CNT_PORT(sel));
    temp.iodata[1] = inb(TIMER_CNT_PORT(sel));
    
    return (temp.val);
}

#define timer_write	_timer_write_
static inline void
_timer_write_(sel, val)
int		sel;
timer_cnt_val_t	val;
{
    union {
	timer_cnt_val_t	val;
	unsigned char	iodata[2];
    } temp;
    
    temp.val = val;
    
    outb(TIMER_CNT_PORT(sel), temp.iodata[0]);
    outb(TIMER_CNT_PORT(sel), temp.iodata[1]);
}
