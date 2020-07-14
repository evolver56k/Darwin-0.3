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
 * 8237A DMA Controller definitions.
 *
 * HISTORY
 *
 * 30 May 1992 ? at NeXT
 *	Created.
 */

/*
 * Command register.
 */
typedef struct {
    unsigned char
					:2,
			chip_disbl	:1,
					:1,
			priority	:1,
#define DMA_PRIO_FIXED		0
#define DMA_PRIO_ROTATING	1
					:1,
			dreq_sense	:1,
#define DMA_DREQ_ACT_HIGH	0
#define DMA_DREQ_ACT_LOW	1
			dack_sense	:1;
#define DMA_DACK_ACT_LOW	0
#define DMA_DACK_ACT_HIGH	1
} dma_cmd_reg_t;

/*
 * Mode register.
 */
typedef struct {
    unsigned char
			chan_sel	:2,
#define DMA_CHAN0_SEL		0
#define DMA_CHAN1_SEL		1
#define DMA_CHAN2_SEL		2
#define DMA_CHAN3_SEL		3
			xfer_dir	:2,
#define DMA_XFER_VERIFY		0
#define DMA_XFER_WRITE		1
#define DMA_XFER_READ		2
			autoinit	:1,
			adrs_dir	:1,
#define DMA_ADRS_INCR		0
#define DMA_ADRS_DECR		1
			xfer_mode	:2;
#define DMA_MODE_DEMAND		0
#define DMA_MODE_SINGLE		1
#define DMA_MODE_BLOCK		2
#define DMA_MODE_CASCADE	3
} dma_mode_reg_t;

/*
 * Request register.
 */
typedef struct {
    unsigned char
			chan_sel	:2,
			req		:1,
#define DMA_REQ_RESET		0
#define DMA_REQ_SET		1
					:5;
} dma_req_reg_t;

/*
 * Mask register.
 */
typedef struct {
    unsigned char
			chan_sel	:2,
			mask		:1,
#define DMA_MASK_CLEAR		0
#define DMA_MASK_SET		1
					:5;
} dma_mask_reg_t;

/*
 * Status register.
 */
typedef struct {
    unsigned char
			chan0_done	:1,
			chan1_done	:1,
			chan2_done	:1,
			chan3_done	:1,
			chan0_req	:1,
			chan1_req	:1,
			chan2_req	:1,
			chan3_req	:1;
} dma_status_reg_t;

/*
 * Extended mode register (EISA only).
 */
typedef struct {
    unsigned char
				chan_sel	:2,
				xfer_width	:2,
#define DMA_XFR_8_BIT		0
#define DMA_XFR_16_BIT_WORD	1
#define DMA_XFR_32_BIT		2
#define DMA_XFR_16_BIT_BYTE	3
	
				dma_timing	:2,
#define DMA_TIMING_COMPAT	0
#define DMA_TIMING_A		1
#define DMA_TIMING_B		2
#define DMA_TIMING_BURST	3

				eop_io		:1,
#define	DMA_EOP_OUT		0
#define	DMA_EOP_IN		1
				stop_enable	:1;
#define DMA_STOP_DISABLE	0
#define DMA_STOP_ENABLE		1

} dma_extend_mode_reg_t;

/*
 * DMA I/O Ports.
 */

/*
 * Master device.
 */
#define DMA_CHAN0_ADDR_PORT	0x0000
#define DMA_CHAN0_COUNT_PORT	0x0001
#define DMA_CHAN1_ADDR_PORT	0x0002
#define DMA_CHAN1_COUNT_PORT	0x0003
#define DMA_CHAN2_ADDR_PORT	0x0004
#define DMA_CHAN2_COUNT_PORT	0x0005
#define DMA_CHAN3_ADDR_PORT	0x0006
#define DMA_CHAN3_COUNT_PORT	0x0007

#define DMA_CHAN0_PAGE_PORT	0x0087
#define DMA_CHAN1_PAGE_PORT	0x0083
#define DMA_CHAN2_PAGE_PORT	0x0081
#define DMA_CHAN3_PAGE_PORT	0x0082

// HICOUNT (bits 23:16): EISA Only
#define DMA_CHAN0_HICOUNT_PORT	0x0401
#define DMA_CHAN1_HICOUNT_PORT	0x0403
#define DMA_CHAN2_HICOUNT_PORT	0x0405
#define DMA_CHAN3_HICOUNT_PORT	0x0407

// HIPAGE (bits 31:24): EISA Only
#define DMA_CHAN0_HIPAGE_PORT	0x0487
#define DMA_CHAN1_HIPAGE_PORT	0x0483
#define DMA_CHAN2_HIPAGE_PORT	0x0481
#define DMA_CHAN3_HIPAGE_PORT	0x0482

#define DMA_CMD_STATUS_PORT	0x0008
#define DMA_REQ_PORT		0x0009
#define DMA_MASK_PORT		0x000A
#define DMA_MODE_PORT		0x000B
#define DMA_CLEAR_FF_PORT	0x000C
#define DMA_MASTER_RESET_PORT	0x000D
#define DMA_EXTEND_MODE_PORT	0x040B

/*
 * Slave device.
 */
#define DMA2_CHAN0_ADDR_PORT	0x00C0
#define DMA2_CHAN0_COUNT_PORT	0x00C2
#define DMA2_CHAN1_ADDR_PORT	0x00C4
#define DMA2_CHAN1_COUNT_PORT	0x00C6
#define DMA2_CHAN2_ADDR_PORT	0x00C8
#define DMA2_CHAN2_COUNT_PORT	0x00CA
#define DMA2_CHAN3_ADDR_PORT	0x00CC
#define DMA2_CHAN3_COUNT_PORT	0x00CE

#define DMA2_CHAN1_PAGE_PORT	0x008B
#define DMA2_CHAN2_PAGE_PORT	0x0089
#define DMA2_CHAN3_PAGE_PORT	0x008A

// HICOUNT (bits 23:16): EISA Only
#define DMA2_CHAN1_HICOUNT_PORT	0x04C6
#define DMA2_CHAN2_HICOUNT_PORT	0x04CA
#define DMA2_CHAN3_HICOUNT_PORT	0x04CE

// HIPAGE (bits 31:24): EISA Only
#define DMA2_CHAN1_HIPAGE_PORT	0x048B
#define DMA2_CHAN2_HIPAGE_PORT	0x0489
#define DMA2_CHAN3_HIPAGE_PORT	0x048A

#define DMA2_CMD_STATUS_PORT	0x00D0
#define DMA2_REQ_PORT		0x00D2
#define DMA2_MASK_PORT		0x00D4
#define DMA2_MODE_PORT		0x00D6
#define DMA2_CLEAR_FF_PORT	0x00D8
#define DMA2_MASTER_RESET_PORT	0x00DA
#define DMA2_EXTEND_MODE_PORT	0x04D6
