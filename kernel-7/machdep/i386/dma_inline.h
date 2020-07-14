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
 * dma_inline.h -- Inlines for DMA controller access.
 *
 * HISTORY
 * 08 Jan  1993 David Somayajulu at NeXT
 *	fixed dma_set_chan_addr, and dma_set_chan_count 
 *  		added code for keeping track of status register state
 * 
 * 13 July 1992 ? at NeXT
 *	Created.
 */
 
#import <mach/mach_types.h>

#import <machdep/i386/io_inline.h>

#import <machdep/i386/dma.h>
#import <machdep/i386/dma_internal.h>
#import <bsd/i386/param.h>

#define DMA_DEBUG 0

#if	DMA_DEBUG
#define	dprintf(x) printf x;
#else	/* DMA_DEBUG */
#define	dprintf(x)
#endif	/* DMA_DEBUG */

/*
 * Required delay before inb/out to/from 8237.
 */
#define DMA_DELAY	DELAY(1)

#if !DMA_DEBUG
static inline
#endif
void
dma_master_reset(
    void
)
{ 
	DMA_DELAY;
	outb(DMA_CHIP_PORT(0, master_reset), 0);
	DMA_DELAY;
	outb(DMA_CHIP_PORT(4, master_reset), 0);
}


#if !DMA_DEBUG
static inline
#endif
void
dma_set_cmd(
    int			chip_num,
    dma_cmd_reg_t	reg
)
{
    union {
	dma_cmd_reg_t		reg;
	unsigned char		data;
    } tconv;
    
    tconv.reg = reg;
    DMA_DELAY;
    if(chip_num) {
        outb(DMA_CHIP_PORT(4, cmd_status), tconv.data);
    }
    else {
	outb(DMA_CHIP_PORT(0, cmd_status), tconv.data);
    }
}

#define ENABLE_DISABLE	1

extern dma_cmd_reg_t dma_cmd_regs[DMA_NCHIPS];

static inline 
void 
dma_chip_enable(
	int 	chan
)
{
	int chip = (chan > 3 ? 1 : 0);
	
	if(!ENABLE_DISABLE)
		return;
	dma_cmd_regs[chip].chip_disbl = 0;
   	dma_set_cmd(chip, dma_cmd_regs[chip]);
}

static inline
void 
dma_chip_disable(
	int 	chan
)
{
	int chip = (chan > 3 ? 1 : 0);
	
	if(!ENABLE_DISABLE)
		return;
	dma_cmd_regs[chip].chip_disbl = 1;
   	dma_set_cmd(chip, dma_cmd_regs[chip]);
}


#if !DMA_DEBUG
static inline
#endif
void
dma_set_chan_mask(
    int			chan,
    dma_mask_reg_t	reg
)
{
    union {
	dma_mask_reg_t		reg;
	unsigned char		data;
    } tconv;

    tconv.reg = reg;
    dma_chip_disable(chan);
    DMA_DELAY;
    outb(DMA_CHIP_PORT(chan, mask), tconv.data);
    dma_chip_enable(chan);
}

#if !DMA_DEBUG
static inline
#endif
void
dma_set_chan_mode(
    int			chan,
    dma_mode_reg_t	reg
)
{
    union {
	dma_mode_reg_t		reg;
	unsigned char		data;
    } tconv;

    tconv.reg = reg;

    dma_chip_disable(chan);
    DMA_DELAY;
    outb(DMA_CHIP_PORT(chan, mode), tconv.data);
    dma_chip_enable(chan);
}

#if !DMA_DEBUG
static inline
#endif
void
dma_set_chan_extend_mode(
    int				chan,
    dma_extend_mode_reg_t	reg
)
{
    union {
	dma_extend_mode_reg_t	reg;
	unsigned char		data;
    } tconv;

    tconv.reg = reg;

    dma_chip_disable(chan);
    DMA_DELAY;
    outb(DMA_CHIP_PORT(chan, extend_mode), tconv.data);
    dma_chip_enable(chan);
}

#if !DMA_DEBUG
static inline
#endif
void
dma_set_chan_addr(
    int			chan,
    vm_offset_t		addr
)
{
	union {
		vm_offset_t	addr;
		struct {
		vm_offset_t	addr_byte0	:8,
				addr_byte1	:8,
				page		:8,
				hipage		:8;
		} data;
		struct {
		vm_offset_t	addr		:16,
				page		:8,
				hipage		:8;
		} isa_data;
				
	} tconv;
	
	tconv.addr = addr;
	if (dma_write_regs[chan].extend_mode.xfer_width ==
		DMA_XFR_16_BIT_WORD) {
	    tconv.isa_data.addr >>= 1;
	}
	dma_chip_disable(chan);

	DMA_DELAY;
	outb(DMA_CHIP_PORT(chan, clear_ff), 0xFF);
				/* write some crap into this */
	dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte0));
	DMA_DELAY;
   	outb(DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte0);

	dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte1));
	DMA_DELAY;
	outb(DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte1);

	dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, page), tconv.data.page));

	DMA_DELAY;
    	outb(DMA_CHAN_PORT(chan, page), tconv.data.page);
	if (eisa_present()){
		dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, hipage), tconv.data.hipage));
	
		// write the hipage address register last so that if we 
		// have an 82357 or its compatible on an ISA machine 
		// nothing bad happens

		DMA_DELAY;
    		outb(DMA_CHAN_PORT(chan, hipage), tconv.data.hipage);
	}
   	 dma_chip_enable(chan);
}

#if !DMA_DEBUG
static inline
#endif
void
dma_set_chan_count(
    int			chan,
    vm_size_t		count
)
{
	union {
		vm_size_t		count;
		struct {
			vm_size_t	count_byte0	:8,
				   	count_byte1	:8,
				   	hicount		:8,
						    	:8;
		} data;
	} tconv;
	
	if (dma_write_regs[chan].extend_mode.xfer_width ==
		DMA_XFR_16_BIT_WORD) {
	    tconv.count = (count >> 1) - 1;
	} else {
	    tconv.count = count - 1;
	}

	dma_chip_disable(chan);
	DMA_DELAY;
	outb(DMA_CHIP_PORT(chan, clear_ff), 0xFF);
				/* write some crap into this */

	dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, count), tconv.data.count_byte0));

	DMA_DELAY;
   	outb(DMA_CHAN_PORT(chan, count), tconv.data.count_byte0);

	dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, count), tconv.data.count_byte1));

	DMA_DELAY;
	outb(DMA_CHAN_PORT(chan, count), tconv.data.count_byte1);
	if (eisa_present()){
		// write the hicount register first so that if we have an
		// 82357 or its compatible on an ISA machine nothing bad 
		// happens
		dprintf(("writing to port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, hicount), tconv.data.hicount));

		DMA_DELAY;
    		outb(DMA_CHAN_PORT(chan, hicount), tconv.data.hicount);
	}

    dma_chip_enable(chan);

}

