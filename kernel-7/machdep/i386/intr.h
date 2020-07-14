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
 * 8259A (PIC) Interrupt Controller definitions.
 *
 * HISTORY
 *
 * 31 May 1992 ? at NeXT
 *	Created.
 */

/*
 * Operation Command Words.
 */

/*
 * OCW1 - interrupt mask.
 */
typedef struct {
    unsigned char
			mask		:8;
} intr_ocw1_t;

/*
 * OCW2 - EOI support.
 */
typedef struct {
    unsigned char
			level		:3,
			set_to_zero	:2,
			eoi		:1,
			specific	:1,
			rotation	:1;
} intr_ocw2_t;

/*
 * OCW3 - register access, poll, smm commands.
 */
typedef struct {
    unsigned char
			read_reg	:2,
#define INTR_OCW3_READ_IRR	2
#define INTR_OCW3_READ_ISR	3
			poll		:1,
			set_to_one	:2,
			smm		:2,
#define INTR_OCW3_RESET_SMM	2
#define INTR_OCW3_SET_SMM	3
					:1;
} intr_ocw3_t;

/*
 * Initialization Command Words.
 */

/*
 * ICW1 - mode initialization.
 */
typedef struct {
    unsigned char
			ic4		:1,
			no_slaves	:1,
			vectsz		:1,
#define INTR_ICW1_VECTSZ_8	0
#define INTR_ICW1_VECTSZ_4	1
			trig_mode	:1,
#define INTR_ICW1_EDGE_TRIG	0
#define INTR_ICW1_LEVEL_TRIG	1
			set_to_one	:1,
					:3;
} intr_icw1_t;

/*
 * ICW2 - interrupt vector.
 */
typedef struct {
    unsigned char
			vectoff		:8;
} intr_icw2_t;

/*
 * ICW3 for master - slave inputs.
 */
typedef struct {
    unsigned char
			slave_ir	:8;
} intr_icw3m_t;

/*
 * ICW3 for slave - slave priority.
 */
typedef struct {
    unsigned char
			slave_id	:3,
					:5;
} intr_icw3s_t;

/*
 * ICW4 - misc mode initialization.
 */
typedef struct {
    unsigned char
			pmode		:1,
#define INTR_ICW4_8080_MODE	0
#define INTR_ICW4_8086_MODE	1
			auto_eoi	:1,
			bufmode		:2,
#define INTR_ICW4_NONBUF	0
#define INTR_ICW4_BUF_SLAVE	2
#define INTR_ICW4_BUF_MASTER	3
			sfnmode		:1,
					:3;
} intr_icw4_t;

/*
 * ELCR - EISA Edge/Level Triggered
 * Control Register.
 */
typedef struct {
    unsigned char
    			mask		:8;
} intr_elcr_t;

/*
 * PIC I/O Ports.
 */

/*
 * Master device.
 */
#define INTR_PRIMARY_PORT	0x0020
#define INTR_SECONDARY_PORT	0x0021
#define INTR_ELCR_PORT		0x04D0

/*
 * Slave device.
 */
#define INTR2_PRIMARY_PORT	0x00A0
#define INTR2_SECONDARY_PORT	0x00A1
#define INTR2_ELCR_PORT		0x04D1

/*
 * IRQs returned for missed interrupts.
 */
#define INTR_MASTER_PHANTOM_IRQ	7
#define INTR_SLAVE_PHANTOM_IRQ	15
#define INTR_PHANTOM_IRQ_MASK	(1 << 7)
