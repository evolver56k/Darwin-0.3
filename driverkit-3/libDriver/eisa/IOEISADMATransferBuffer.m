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
 * Copyright 1994 NeXT Computer, Inc.
 * All rights reserved.
 *
 */

#if NOTYET

#import <driverkit/i386/IOEISADMATransferBuffer.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <machdep/i386/dma.h>
#import <machdep/i386/dma_inline.h>
#import <machdep/i386/dma_exported.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

@implementation IOEISADMATransferBuffer


-setChannel:(unsigned int)newChannel
 forDevice:device
{
    unsigned int *list;

    localChannel = newChannel;
    directDevice = device;
    deviceDescription = [device deviceDescription];
    if (deviceDescription == nil) {
	return nil;
    }
    if ((list = [deviceDescription channelList])) {
	realChannel = list[newChannel];
    } else {
	return nil;
    }
    return self;
}

-(IODMADirection)transferDirection
{
    return transferDirection;
}

-(void)setTransferDirection:(IODMADirection)dir
{
    transferDirection = dir;
}

-(IOReturn)startTransfer
{
    IORange range;
    
    range.start = 0;
    range.size = [self size];
    return [self startTransferAt:range];
}

-(IOReturn)startTransferAt:(IORange)range
{
    int Size;
    IOPhysicalRange prange;
    
    Size = [self size];
    if (range.start > Size || (range.start + range.size) > Size)
	return IO_R_INVALID;

    if ([self getPhysicalAddress:&prange at:range.start]
	    == NO) {
	return IO_R_INVALID;
    }
    prange.size = MIN(range.size, prange.size);
    
    (void) dma_mask_chan(realChannel);
    if (transferDirection == IO_DMARead) {
	/* DMA write to memory */
	dma_chan_xfer_dir(realChannel, DMA_XFER_WRITE);
    } else {
	/* DMA read from memory */
	dma_chan_xfer_dir(realChannel, DMA_XFER_READ);
    }

    dma_set_chan_addr(realChannel, prange.start);

    dma_set_chan_count(realChannel, prange.size);

    currentTransfer = prange;
    
    (void) dma_unmask_chan(realChannel);

    return (IO_R_SUCCESS);
}

-(IOReturn)abortTransfer
{
    // XXX
    return (IO_R_SUCCESS);
}

-(BOOL)transferIsComplete
{
    return is_dma_done(realChannel);
}

-(unsigned)currentTransferLocation
{
    return get_dma_addr(realChannel) - currentTransfer.start;
}

-(unsigned)currentTransferCount
{
    return get_dma_count(realChannel);
}


-(IOReturn)enableChannel
{
    dma_unmask_chan(realChannel);
    return IO_R_SUCCESS;
}
-(void)disableChannel
{
    dma_mask_chan(realChannel);
}


/*
 * Reserve and release exclusive DMA lock. Use is optional; provides
 * exclusion between mutually incompatible DMA devices.
 */
- (void)reserveDMALock
{
    [directDevice reserveDMALock];
}
- (void)releaseDMALock
{
    [directDevice releaseDMALock];
}

/*
 * FIXME -- the kernel dma code should provide a way
 * to get the current transfer mode.
 */

static IODMATransferMode currentMode;

-(IODMATransferMode)transferMode
{
    return currentMode;
}

-(IOReturn)setTransferMode:(IODMATransferMode)mode
{
    unsigned real_mode;
    
    switch(mode) {
    case IO_Demand:
	real_mode = DMA_MODE_DEMAND;
	break;
    case IO_Single:
    default:
	real_mode = DMA_MODE_SINGLE;
	break;
    case IO_Block:
	real_mode = DMA_MODE_BLOCK;
	break;
    case IO_Cascade:
	real_mode = DMA_MODE_CASCADE;
	break;
    }
    dma_chan_xfer_mode(realChannel, real_mode);
    currentMode = mode;
    return IO_R_SUCCESS;
}

-(IOEISADMATransferWidth)transferWidth
{
    int width;
    IOEISADMATransferWidth ret;
    
    width = get_dma_xfer_width(realChannel);
    switch(width) {
    case DMA_XFR_8_BIT:
    default:	/* should never be reached */
	ret = IO_8Bit;
	break;
    case DMA_XFR_16_BIT_WORD:
	ret = IO_16BitWordCount;
	break;
    case DMA_XFR_16_BIT_BYTE:
	ret = IO_16BitByteCount;
	break;
    case DMA_XFR_32_BIT:
	ret = IO_32Bit;
	break;
    }
    return ret;
}

-(IOReturn)setTransferWidth:(IOEISADMATransferWidth)width
{
    int dmaWidth;
    
    switch(width) {
    case IO_8Bit:
	dmaWidth = DMA_XFR_8_BIT;
	break;
    case IO_16BitWordCount:
	dmaWidth = DMA_XFR_16_BIT_WORD;
	break;
    case IO_16BitByteCount:
	dmaWidth = DMA_XFR_16_BIT_BYTE;
	break;
    case IO_32Bit:
	dmaWidth = DMA_XFR_32_BIT;
	break;
    default:
	return IO_R_INVALID_ARG;
    }
    dma_xfer_width(realChannel, dmaWidth);
    return IO_R_SUCCESS;
}

-(IOEISADMATiming)transferTiming
{
    return 0; //XXX
}

-(IOReturn)setTransferTiming:(IOEISADMATiming)timing
{
    int dmaTiming;
    
    switch(timing) {
    case IO_Compatible:
	dmaTiming = DMA_TIMING_COMPAT;
	break;
    case IO_TypeA:
	dmaTiming = DMA_TIMING_A;
	break;
    case IO_TypeB:
	dmaTiming = DMA_TIMING_B;
	break;
    case IO_Burst:
	dmaTiming = DMA_TIMING_BURST;
	break;
    }
    dma_timing(realChannel, dmaTiming);
    return IO_R_SUCCESS;
}

-(IOEISAStopRegisterMode)stopRegisterMode
{
    return 0; //XXX
}

-(IOReturn)setStopRegisterMode:(IOEISAStopRegisterMode)mode
{
    unsigned dmaFlag;
    
    if(mode == IO_StopRegisterEnable) {
	    dmaFlag = DMA_STOP_ENABLE;
    }
    else {
	    dmaFlag = DMA_STOP_DISABLE;
    }
    dma_stop_enable(realChannel, dmaFlag);
    return IO_R_SUCCESS;
}

-(IOIncrementMode)transferIncrementMode
{
    return 0; //XXX
}
-(IOReturn)setTransferIncrementMode:(IOIncrementMode)mode
{
    int dir = (mode == IO_Decrement ? DMA_ADRS_DECR : DMA_ADRS_INCR);

    dma_chan_adrs_dir(realChannel, dir);
    return IO_R_SUCCESS;
}

-(BOOL)autoinitializeMode
{
    return 0; //XXX
}

-(IOReturn)setAutoinitializeMode:(BOOL)mode
{
    dma_chan_autoinit(realChannel, (mode ? 1 : 0));
    return IO_R_SUCCESS;
}

-(BOOL)isValidForDMA
{
    int offset, total;
    IOPhysicalRange prange;
    
    if (eisa_present())
	return YES;

    for (total = [self size], offset = 0; total > 0; ) {
	if ([self getPhysicalAddress:&prange at:offset] == NO) {
	    return NO;
	}
	if (prange.start >= 16 * 1024 * 1024)	// XXX use symbolic constant
	    return NO;
	total -= prange.size;
	offset += prange.size;
    }
    return YES;
}


/*
 * Returns the maximum number of bytes that can be transferred in one
 * operation from the specified offset in the buffer.
 */
-(unsigned int)maximumTransferLengthAtOffset:(unsigned int)offset
{
    int len = 0;
    IOPhysicalRange prange;
    
    if ([self getPhysicalAddress:&prange at:offset] == NO)
	return 0;
    if (eisa_present())
	return prange.size;
    else
	return MIN(prange.size, 64 * 1024);
}

+(BOOL)allocMemoryWithSize:(int)memSize
       attributes:(IOBufferAttributes)attr
       alignment:(IOAlignment)align
       allocedAddress:(IOVirtualAddress *)allocPtr
       dataAddress:(IOVirtualAddress *)dataPtr
       allocedSize:(int *)sizePtr
       newAttributes:(IOBufferAttributes *)newAttrPtr
       newMap:(IOAddressMap *)mapPtr
{
    IOVirtualAddress vaddr;
    int alignSize, newSize;
    
    alignSize = IOAlignmentToSize(align);
    newSize = memSize + 2 * alignSize;
    
    /* an optimization that relies on IOMalloc returning aligned pages. */
    if (newSize > PAGE_SIZE && memSize <= PAGE_SIZE)
	newSize = PAGE_SIZE;
    
    if (eisa_present())
	vaddr = (IOVirtualAddress)IOMalloc(newSize);
    else
	vaddr = (IOVirtualAddress)IOMallocLow(newSize);
    if (vaddr == 0)
	return FALSE;

    *allocPtr = vaddr;
    if (newSize != memSize)
	*dataPtr = (vaddr + alignSize - 1) & ~(alignSize - 1);
    else
	*dataPtr = vaddr;
    *sizePtr = newSize;
    *newAttrPtr = IOBuf_Wired | IOBuf_WiredStatic | IOBuf_Contiguous;
    *mapPtr = (IOAddressMap)IOVmTaskSelf();
    return TRUE;
}


+(BOOL)freeMemory:(IOVirtualAddress)addr
       size:(int)memsize
       map:(IOAddressMap)memmap
       attributes:(IOBufferAttributes)attr
{
    if (eisa_present())
	IOFree((void *)addr, memsize);
    else
	IOFreeLow((void *)addr, memsize);
    return TRUE;
}

@end

#endif NOTYET

