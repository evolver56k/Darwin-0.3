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
 * 8253 Timer (PIT) definitions.
 *
 * HISTORY
 *
 * 30 May 1992 ? at NeXT
 *	Formalized 8253 support.
 *
 * 24 March 1992 ? at NeXT
 *	Created.
 */

/*
 * Control register.
 */
typedef struct {
    unsigned char
    			bcd		:1,
			mode		:3,
#define TIMER_EVENTMODE		0		/* event timer */
#define TIMER_1SHOTMODE		1		/* one-shot */
#define TIMER_NDIVMODE		2		/* divide by n */
#define TIMER_SQWAVEMODE	3		/* square wave generator */
#define TIMER_SWTRIGMODE	4		/* software trigger */
#define TIMER_HWTRIGMODE	5		/* hardware trigger */
			rw		:2,
#define TIMER_CTL_LATCH		0
#define TIMER_CTL_RW_LSB	1
#define TIMER_CTL_RW_MSB	2
#define TIMER_CTL_RW_BOTH	3
			sel		:2;
#define TIMER_CNT0_SEL		0
#define TIMER_CNT1_SEL		1
#define TIMER_CNT2_SEL		2
} timer_ctl_reg_t;

/*
 * Counter value (16 bits)
 */
typedef unsigned short	timer_cnt_val_t;

/*
 * Maximum count value
 */
#define TIMER_COUNT_MAX	((timer_cnt_val_t) -1)

/*
 * Timer I/O ports
 */
#define TIMER_CNT0_PORT	0x40		/* counter 0 */	
#define TIMER_CNT1_PORT	0x41		/* counter 1 */	
#define TIMER_CNT2_PORT	0x42		/* counter 2 */	
#define TIMER_CTL_PORT	0x43		/* control port */

/*
 * Timer count rate (1/s)
 */
#define TIMER_CONSTANT	1193167
