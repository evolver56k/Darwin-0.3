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
 * dma_internal.h  -- DMA controller module internal definitions.
 *
 * HISTORY
 *
 * 13 July 1992 ? at NeXT
 *	Created.
 */
 
#import <architecture/i386/io.h>

#import <machdep/i386/dma.h>

#define DMA_NCHAN		8	// A fundamental constant
#define DMA_NCHIPS		2
#define DMA_CASCADE_CHAN	4	// Channel is not usable for transfers

struct _dma_chan_port {
    io_addr_t		addr;
    io_addr_t		page;
    io_addr_t		hipage;
    io_addr_t		count;
    io_addr_t		hicount;
};

#define DMA_CHAN_PORT(chan, which) \
    ( _dma_chan_port[(chan)].which )

struct _dma_chip_port {
    io_addr_t		cmd_status;
    io_addr_t		req;
    io_addr_t		mask;
    io_addr_t		mode;
    io_addr_t		clear_ff;
    io_addr_t		master_reset;
    io_addr_t		extend_mode;
};

#define DMA_CHIP_PORT(chan, which) \
    ( ((chan) <= 3)? _dma_chip_port[0].which : _dma_chip_port[1].which )


#define DMA_CHAN_IS_ASSIGNED(chan) \
    ( (boolean_t) ((dma_assigned_bits & (1 << (chan))) != 0) )

struct dma_write_regs {
    /* dma_cmd_reg_t	cmd; */
    dma_mode_reg_t		mode;
    dma_extend_mode_reg_t	extend_mode;
};

void	dma_buf_initialize(
				void);

extern struct _dma_chan_port	_dma_chan_port[];
extern struct _dma_chip_port	_dma_chip_port[];
extern unsigned char		dma_assigned_bits;
extern struct dma_write_regs	dma_write_regs[];

