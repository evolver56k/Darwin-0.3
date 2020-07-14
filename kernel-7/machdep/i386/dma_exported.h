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
 * dma_exported.h -- DMA external interface.
 *
 * HISTORY
 *
 * 08 Jan 1993  David Somayajulu at NeXT
 *  added functions is_dma_done, get_dma_addr, get_dma_count
 * 
 * 14 July 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#define DMA_BUF_SM_LEN		PAGE_SIZE
#define DMA_BUF_LG_LEN		(64*1024)

typedef struct dma_buf {
    void		*_ptr;		/* opaque region ptr */
    void		*_type;		/* opaque buffer type */
} dma_buf_t;

typedef struct dma_xfer {
    vm_offset_t		phys;		/* phys address of data to transfer */
    vm_size_t		len;		/* length of transfer */
    int			chan;		/* core dma channel number */
    dma_buf_t		buf;		/* optional dma buffer */
    unsigned int	active	:1,	/* xfer is in progress */
    			buffered:1,	/* xfer is using dma buffer */
			use_chan:1,	/* xfer uses core dma channel */
    			read	:1,	/* read or write xfer */
    			lower16	:1,	/* dma restricted to lower 16 meg */
			bound64	:1,	/* dma cannot cross 64 k boundary */
    				:0;
} dma_xfer_t;

// Core initialization
void		dma_initialize(
				void);

// Channel manipulation
boolean_t	dma_assign_chan(
				int	chan);
void		dma_deassign_chan(
				int	chan);
void		dma_mask_chan(
				int	chan);
void		dma_unmask_chan(
				int	chan);
void		dma_chan_xfer_mode(
				int	chan,
				int	xfer_mode);
void		dma_chan_autoinit(
				int 		chan,
				boolean_t	autoinit);
void		dma_chan_adrs_dir(
				int 		chan,
				boolean_t	adrs_dir);

// Transfer operations
boolean_t	dma_xfer_chan(
				int		chan,
				dma_xfer_t	*xfer);
boolean_t	dma_xfer(
				dma_xfer_t	*xfer,
				vm_offset_t	*rphys);
void		dma_xfer_done(
				dma_xfer_t	*xfer);
void		dma_xfer_abort(
				dma_xfer_t	*xfer);
			
// Extended Mode register manipulation	
void		dma_xfer_width(
				int		chan,
				int		xfer_width);
void		dma_timing(
				int		chan,
				int		dma_timing);
void		dma_eop_in(
				int		chan,
				int		eop_io);
void		dma_stop_enable(
				int		chan,
				int		stop_enable);

// Buffer operations
boolean_t	dma_buf_alloc(
				dma_buf_t	*buf,
				vm_size_t	len);
void		dma_buf_free(
				dma_buf_t	*buf);

// Status checks

boolean_t is_dma_done(int chan );

vm_offset_t get_dma_addr( int  chan);
vm_size_t get_dma_count(int chan);

