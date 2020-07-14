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
 * dma.c -- DMA Routines.
 *
 * HISTORY
 *
 * 08 Jan 1993  David Somayajulu at NeXT
 *  	added functions is_dma_done, get_dma_addr, get_dma_count
 * 
 * 13 July 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <machdep/i386/dma.h>
#import <machdep/i386/dma_internal.h>
#import <machdep/i386/dma_inline.h>
#import <machdep/i386/dma_exported.h>

struct _dma_chan_port _dma_chan_port[DMA_NCHAN] = {
    {
	DMA_CHAN0_ADDR_PORT,
	DMA_CHAN0_PAGE_PORT, DMA_CHAN0_HIPAGE_PORT,
	DMA_CHAN0_COUNT_PORT, DMA_CHAN0_HICOUNT_PORT },
    {
	DMA_CHAN1_ADDR_PORT,
	DMA_CHAN1_PAGE_PORT, DMA_CHAN1_HIPAGE_PORT,
	DMA_CHAN1_COUNT_PORT, DMA_CHAN1_HICOUNT_PORT },
    {
	DMA_CHAN2_ADDR_PORT,
	DMA_CHAN2_PAGE_PORT, DMA_CHAN2_HIPAGE_PORT,
	DMA_CHAN2_COUNT_PORT, DMA_CHAN2_HICOUNT_PORT },
    {
	DMA_CHAN3_ADDR_PORT,
	DMA_CHAN3_PAGE_PORT, DMA_CHAN3_HIPAGE_PORT,
	DMA_CHAN3_COUNT_PORT, DMA_CHAN3_HICOUNT_PORT },
    {
    	/* CASCADE channel */
    },
    {
	DMA2_CHAN1_ADDR_PORT,
	DMA2_CHAN1_PAGE_PORT, DMA2_CHAN1_HIPAGE_PORT,
	DMA2_CHAN1_COUNT_PORT, DMA2_CHAN1_HICOUNT_PORT },
    {
	DMA2_CHAN2_ADDR_PORT,
	DMA2_CHAN2_PAGE_PORT, DMA2_CHAN2_HIPAGE_PORT,
	DMA2_CHAN2_COUNT_PORT, DMA2_CHAN2_HICOUNT_PORT },
    {
	DMA2_CHAN3_ADDR_PORT,
	DMA2_CHAN3_PAGE_PORT, DMA2_CHAN3_HIPAGE_PORT,
	DMA2_CHAN3_COUNT_PORT, DMA2_CHAN3_HICOUNT_PORT },
};

struct _dma_chip_port _dma_chip_port[] = {
    {
	DMA_CMD_STATUS_PORT, DMA_REQ_PORT, DMA_MASK_PORT, DMA_MODE_PORT,
	DMA_CLEAR_FF_PORT, DMA_MASTER_RESET_PORT, DMA_EXTEND_MODE_PORT },
    {
	DMA2_CMD_STATUS_PORT, DMA2_REQ_PORT, DMA2_MASK_PORT, DMA2_MODE_PORT,
	DMA2_CLEAR_FF_PORT, DMA2_MASTER_RESET_PORT, DMA2_EXTEND_MODE_PORT }
};

unsigned char		dma_assigned_bits;
struct dma_write_regs	dma_write_regs[DMA_NCHAN];
dma_cmd_reg_t		dma_cmd_regs[DMA_NCHIPS];

unsigned prev_tcstatus0, /* previous transfer count status of 
			DMA Chip Port 0,for channels 0-3*/
	 prev_tcstatus1; /* previous transfer count status of 
			DMA Chip Port 1,for channels 5-7*/	
		 	 /* The TC bits in the status register get cleared
			when it is read. Hence we need to keep track of it
				seperately */
		
static void dma_init_extend_mode(int chan);
					
#define PRIORITY_ROTATING	1

void
dma_initialize(
    void
)
{
	dma_cmd_reg_t	cmd = { 0 };
	int		chan;

	dma_master_reset();
	prev_tcstatus0 = 0;
	prev_tcstatus1 = 0;
    
#if 	PRIORITY_ROTATING
    	cmd.priority = DMA_PRIO_ROTATING;
#else 	PRIORITY_ROTATING
    	cmd.priority = DMA_PRIO_FIXED;
#endif	PRIORITY_ROTATING
	dma_set_cmd(0, cmd);
	dma_set_cmd(1, cmd);
	dma_cmd_regs[0] = cmd;
	dma_cmd_regs[1] = cmd;
    
	for (chan = 0; chan < DMA_NCHAN; chan++) {
		(void) dma_assign_chan(chan);
	
		if (chan != DMA_CASCADE_CHAN) {
		   	dma_deassign_chan(chan);	// does an unmask
			dma_init_extend_mode(chan);
		}
		else {
		    	dma_chan_xfer_mode(chan, DMA_MODE_CASCADE);
		    	dma_unmask_chan(chan);		// unmask and leave 
							//    assigned
		}
	}
   	dma_buf_initialize();
}

/*
 * Initialize extended mode register to defaults. 
 */
static void
dma_init_extend_mode(
    int		chan
)
{
	dma_write_regs[chan].extend_mode.dma_timing = 
		DMA_TIMING_COMPAT;
	dma_write_regs[chan].extend_mode.eop_io = 
		DMA_EOP_OUT;
	dma_write_regs[chan].extend_mode.stop_enable = 
		DMA_STOP_DISABLE;
	if (!eisa_present()) {
	    /* on ISA, use this to indicate transfer mode,
	     * but don't actually try to set the mode in hardware
	     */
	    if (chan < 4)
		dma_write_regs[chan].extend_mode.xfer_width = 
			DMA_XFR_8_BIT;
	    else
		dma_write_regs[chan].extend_mode.xfer_width = 
			DMA_XFR_16_BIT_WORD;
	} else {
	    dma_write_regs[chan].extend_mode.xfer_width = 
		    DMA_XFR_8_BIT;
	    dma_set_chan_extend_mode(chan, dma_write_regs[chan].extend_mode);
	}
}
boolean_t
dma_assign_chan(
    int		chan
)
{
    if (DMA_CHAN_IS_ASSIGNED(chan))
	return (FALSE);

    dma_assigned_bits |= (1 << chan);

    return (TRUE);
}

void
dma_deassign_chan(
    int		chan
)
{
    dma_mask_chan(chan);

    dma_assigned_bits &= ~(1 << chan);
}

void
dma_mask_chan(
    int		chan
)
{
    dma_mask_reg_t	mask = { 0 };

    if (!DMA_CHAN_IS_ASSIGNED(chan))
	return;

    mask.mask = DMA_MASK_SET;
    mask.chan_sel = chan;
  
    dma_set_chan_mask(chan, mask);
}

void
dma_unmask_chan(
    int		chan
)
{
    dma_mask_reg_t	mask = { 0 };

    if (!DMA_CHAN_IS_ASSIGNED(chan))
	return;

    mask.mask = DMA_MASK_CLEAR;
    mask.chan_sel = chan;

    dma_set_chan_mask(chan, mask);
}

void
dma_chan_xfer_mode(
    int		chan,
    int		xfer_mode
)
{
    dma_mode_reg_t	mode;

    if (!DMA_CHAN_IS_ASSIGNED(chan))
	return;

    dma_write_regs[chan].mode.xfer_mode = xfer_mode;

    mode = dma_write_regs[chan].mode;
    mode.chan_sel = chan;

    dma_set_chan_mode(chan, mode);
}

void	
dma_chan_autoinit(
	int		chan,
	boolean_t	autoinit)
{
    	dma_mode_reg_t	mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].mode.autoinit = autoinit;
	mode = dma_write_regs[chan].mode;
	mode.chan_sel = chan;
	dma_set_chan_mode(chan, mode);
}

void	
dma_chan_adrs_dir(
	int 		chan,
	boolean_t	adrs_dir)
{
    	dma_mode_reg_t	mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].mode.adrs_dir = adrs_dir ? 1 : 0;
	mode = dma_write_regs[chan].mode;
	mode.chan_sel = chan;
	dma_set_chan_mode(chan, mode);
}	

void
dma_chan_xfer_dir(
    int		chan,
    int		xfer_dir
)
{
    dma_mode_reg_t	mode;
    
    if (!DMA_CHAN_IS_ASSIGNED(chan))
    	return;
	
    dma_write_regs[chan].mode.xfer_dir = xfer_dir;
    
    mode = dma_write_regs[chan].mode;
    mode.chan_sel = chan;
    
    dma_set_chan_mode(chan, mode);
}

boolean_t
dma_xfer_chan(
    int			chan,
    dma_xfer_t		*xfer
)
{
	vm_offset_t		phys;
	extern boolean_t	isEISA;

//	REMOVE THIS
	caddr_t dma_addr;
	unsigned dma_count;

    
	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    	return (FALSE);
	    
	if (!eisa_present())
	    	xfer->lower16 = xfer->bound64 = TRUE;
	    
	if (!dma_xfer(xfer, &phys))
	    	return (FALSE);
	
    	xfer->chan = chan; xfer->use_chan = TRUE;
    
    	(void) dma_mask_chan(chan);
	
	if (xfer->read)
	    dma_chan_xfer_dir(chan, DMA_XFER_WRITE);/* dma write to memory */
	else
	    dma_chan_xfer_dir(chan, DMA_XFER_READ);/* dma read from  memory */
	    
	dma_set_chan_addr(chan, phys);

#if 	DMA_DEBUG
	dma_addr = 	get_dma_addr(2);
	dma_count = 	get_dma_count(2);
#endif    
    	dma_set_chan_count(chan, xfer->len);

#if 	DMA_DEBUG
	dma_count = 	get_dma_count(2);
#endif    
	/*
	 * Clear the shadow "channel done" bit.
	 */
	if(chan <= 3) {
		prev_tcstatus0 &= ~(1 << chan);
	}
	else {
		prev_tcstatus1 &= ~(1 << (chan - 4));
	}
    	(void) dma_unmask_chan(chan);

#if DMA_DEBUG
	dma_count = 	get_dma_count(2);
#endif    
   
    return (TRUE);
}

boolean_t
dma_xfer(
    dma_xfer_t		*xfer,
    vm_offset_t		*rphys
)
{
    dma_buf_t		*buf = &xfer->buf;

    /* Allocate DMA-able memory if we need to,
     * and if a buffer has not already been allocated for this xfer.
     */
    if (xfer->lower16 && ((xfer->phys )>= (16*1024*1024))
					    && !xfer->buffered) {
	vm_offset_t	phys;

	if (!dma_buf_alloc(buf, xfer->len))
	    return (FALSE);
	    
	xfer->buffered = TRUE;

	phys = (vm_offset_t)buf->_ptr;
	    
	if (!xfer->read)
	    bcopy(
		pmap_phys_to_kern(xfer->phys),
		pmap_phys_to_kern(phys),
		xfer->len);
	    
	*rphys = phys;
    }
    else
    	*rphys = xfer->phys;
	
    xfer->active = TRUE;
	
    return (TRUE);
}

void
dma_xfer_done(
    dma_xfer_t		*xfer
)
{
    if (xfer->active) {
	if (xfer->buffered) {
	    dma_buf_t		*buf = &xfer->buf;
	    vm_offset_t		phys;

	    phys = (vm_offset_t)buf->_ptr;
    
	    if (xfer->read)
		bcopy(
		    pmap_phys_to_kern(phys),
		    pmap_phys_to_kern(xfer->phys),
		    xfer->len);

	    dma_buf_free(buf);
	}
	
	xfer->active = xfer->buffered = FALSE;
    }
}

void
dma_xfer_abort(
    dma_xfer_t		*xfer
)
{
    if (xfer->active) {
	if (xfer->buffered)
	    dma_buf_free(&xfer->buf);
	
	xfer->active = xfer->buffered = FALSE;
    }
}

/*
 * Extended Mode register manipulation	
 */
void	
dma_xfer_width(
	int	chan,
	int	xfer_width
)
{
    	dma_extend_mode_reg_t	extend_mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].extend_mode.xfer_width = xfer_width;
	extend_mode = dma_write_regs[chan].extend_mode;
	extend_mode.chan_sel = chan;
	dma_set_chan_extend_mode(chan, extend_mode);
}

/*
 * Get the current transfer width for a channel.
 */
int
get_dma_xfer_width(
	int	chan
)
{
	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return -1;
	return dma_write_regs[chan].extend_mode.xfer_width;
}


void
dma_timing(
	int	chan,
	int	dma_timing
)
{
    	dma_extend_mode_reg_t	extend_mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].extend_mode.dma_timing = dma_timing;
	extend_mode = dma_write_regs[chan].extend_mode;
	extend_mode.chan_sel = chan;
	dma_set_chan_extend_mode(chan, extend_mode);
}

void
dma_eop_in(
	int	chan,
	int	eop_io
)
{
    	dma_extend_mode_reg_t	extend_mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].extend_mode.eop_io = eop_io;
	extend_mode = dma_write_regs[chan].extend_mode;
	extend_mode.chan_sel = chan;
	dma_set_chan_extend_mode(chan, extend_mode);
}

void
dma_stop_enable(
	int	chan,
	int	stop_enable
)
{
    	dma_extend_mode_reg_t	extend_mode;

	if (!DMA_CHAN_IS_ASSIGNED(chan))
	    return;
    
	dma_write_regs[chan].extend_mode.stop_enable = stop_enable;
	extend_mode = dma_write_regs[chan].extend_mode;
	extend_mode.chan_sel = chan;
	dma_set_chan_extend_mode(chan, extend_mode);
}

/* returns the status register contents for the corresponding channel */
boolean_t is_dma_done(int chan)
{
	unsigned char status;
	
	DMA_DELAY;
	status = (unsigned char)(inb(DMA_CHIP_PORT(chan, cmd_status)));

	/*
	 * Save the volatile bits we just read and OR in shadow
	 * bits for current test.
	 */
	if (chan <= 3) {
		prev_tcstatus0 |= (status & 0xf);
		return (prev_tcstatus0 & (1 << chan)) ? TRUE : FALSE;
		
	}else {
		chan = chan - 4;
		prev_tcstatus1 |= (status & 0xf);
		return (prev_tcstatus1 & (1 << chan)) ? TRUE : FALSE;
	}
}

vm_offset_t get_dma_addr(int chan)
{
	union {
		vm_offset_t addr;
		struct {
			vm_offset_t	addr_byte0 :8,
					addr_byte1 :8,
					page	   :8,
					hipage	   :8;
		} data;
		struct {
			vm_offset_t	addr	   :16,
					page	   :8,
					hipage	   :8;
		} isa_data;
	} tconv;
	tconv.addr = 0;
	dma_chip_disable(chan);
	
	DMA_DELAY;
	outb(DMA_CHIP_PORT(chan, clear_ff), 0xff);
	DMA_DELAY;
	tconv.data.addr_byte0 = inb(DMA_CHAN_PORT(chan, addr));
	dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte0));
	DMA_DELAY;
	tconv.data.addr_byte1 = inb(DMA_CHAN_PORT(chan, addr));
	dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, addr), tconv.data.addr_byte1));
	DMA_DELAY;
	tconv.data.page = inb(DMA_CHAN_PORT(chan, page));
	dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, page), tconv.data.page));
  	if(eisa_present()){
		DMA_DELAY;
		tconv.data.hipage = inb(DMA_CHAN_PORT(chan, hipage));
		dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, hipage), tconv.data.hipage));
	}
	dma_chip_enable(chan);
	if (dma_write_regs[chan].extend_mode.xfer_width ==
		DMA_XFR_16_BIT_WORD) {
	    tconv.isa_data.addr <<= 1;
	}
	return(tconv.addr);
}

vm_size_t get_dma_count(int chan)
{
	union {
		vm_size_t count;
		struct {
			vm_size_t	count_byte0 :8,
					count_byte1 :8,
					hicount	    :8,
						    :8;
		} data;
	} tconv;
	
	dma_chip_disable(chan);
	tconv.count = 0x00;
	DMA_DELAY;
	outb(DMA_CHIP_PORT(chan, clear_ff), 0xff);
	
	DMA_DELAY;
	tconv.data.count_byte0 = inb(DMA_CHAN_PORT(chan, count));
	dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, count), tconv.data.count_byte0));

	DMA_DELAY;
	tconv.data.count_byte1 = inb(DMA_CHAN_PORT(chan, count));
	dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan,  count), tconv.data.count_byte1));
	if (eisa_present()){
		tconv.data.hicount = inb(DMA_CHAN_PORT(chan, hicount));
		dprintf(("reading from port =0x%x value = 0x%x\n", 
			DMA_CHAN_PORT(chan, hicount), tconv.data.hicount));
		dma_chip_enable(chan);
	
		if (tconv.count == 0xFFFFFF) {
			tconv.count = 0;
		}
		else {
			++tconv.count;
		}
	} else {
		dma_chip_enable(chan);
		if (tconv.count == 0xFFFF) {
			tconv.count = 0;
		}
		else {
			++tconv.count;
		}
	}
	if (dma_write_regs[chan].extend_mode.xfer_width ==
		DMA_XFR_16_BIT_WORD) {
	    tconv.count <<= 1;
	}

}
