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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * ISA/EISA direct device implementation.
 *
 * HISTORY
 *
 * 10Jan93 Brian Pinkerton at NeXT
 *	Created.
 *
 *
 *  TODO:
 *	Implement IOPort reservation
 *	Verify that host-master DMA is working with DavidS
 */

#define KERNEL_PRIVATE	1

#import <objc/List.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/IODirectDevicePrivate.h>
#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/driverTypes.h>
#import <driverkit/i386/driverTypesPrivate.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/i386/driverTypes.h>
#import <driverkit/i386/EISAKernBus.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <machdep/i386/dma_exported.h>
#import <machdep/i386/dma.h>
#import <machdep/i386/xpr.h>
#import <machkit/NXLock.h>
#import <kernserv/lock.h>
#import <kernserv/prototypes.h>

#import <objc/HashTable.h>

extern NXLock 	*dmaLock;	// in autoconf_i386.m

@interface IODirectDevice(EISAPrivate)

- (unsigned int) _localToChannel:(unsigned int) localChannel;
- initEISA;
- freeEISA;

@end

struct _eisa_private {
    Arch	type;
};

@implementation IODirectDevice(IOEISADirectDevice)

- initEISA
{
    	struct _eisa_private	*private;

	private = _busPrivate = (void *)IOMalloc(sizeof (*private));
	private->type = EISA;
	
	return self;
}

- freeEISA
{
    	struct _eisa_private	*private = _busPrivate;
	
	IOFree((void *)private, sizeof (*private));
	
	return self;
}

/*
 *  A bunch of convenient private API:
 *
 *  Map from local channel numbers to real ones, and from local IRQ numbers
 *  to real ones.
 */
- (unsigned int) _localToChannel:(unsigned int) localChannel
{
	return [(IOEISADeviceDescription *)_deviceDescription channelList][localChannel];
}


/*
 *  Routines for dealing with interrupts start here.
 */

- (IOReturn) reserveInterrupt	: (unsigned int)localInterrupt
{
	return IO_R_SUCCESS;
}

- (void) releaseInterrupt	: (unsigned int) localInterrupt
{
}




/*
 *  The subclass overrides this method.  In the superclass, we just
 *  return NO.
 */
- (BOOL) getHandler:(IOEISAInterruptHandler *)handler
              level:(unsigned int *)ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
	return NO;
}


- (IOReturn) reserveChannel	: (unsigned int) localChannel
{
	return IO_R_SUCCESS;
}

- (void) releaseChannel		: (unsigned int) localChannel
{
}

- (IOReturn) enableChannel:(unsigned int) localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return IO_R_INVALID_ARG;

	realChannel = [self _localToChannel:localChannel];

	dma_unmask_chan(realChannel);
	return IO_R_SUCCESS;
}


- (void) disableChannel:(unsigned int) localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return;

	realChannel = [self _localToChannel:localChannel];

	dma_mask_chan(realChannel);
}


- (IOReturn) setTransferMode	: (IODMATransferMode) mode 
		     forChannel : (unsigned int) localChannel;
{
	int real_mode;
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return IO_R_INVALID_ARG;

	realChannel = [self _localToChannel:localChannel];

	switch(mode) {
	    case IO_Demand:
	    	real_mode = DMA_MODE_DEMAND;
		break;
	    case IO_Single:
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
	return IO_R_SUCCESS;
}

/*
 * Enable/disable autoinitialize DMA mode. Default is 
 * disabled.
 */
- (IOReturn)setAutoinitialize  	: (BOOL)flag
                    forChannel 	: (unsigned)localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return IO_R_INVALID_ARG;

	realChannel = [self _localToChannel:localChannel];

	dma_chan_autoinit(realChannel, (flag ? 1 : 0));
	return IO_R_SUCCESS;
}

/*
 * Set DMA address increment/decrement mode.
 * Default is IO_Increment.
 */
- (IOReturn)setIncrementMode   	: (IOIncrementMode)mode
                    forChannel 	: (unsigned)localChannel
{
	unsigned int realChannel;
	int dir;
	
	if(mode == IO_Decrement) {
		return IO_R_UNSUPPORTED;		// nope!
	}
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return IO_R_INVALID_ARG;

	realChannel = [self _localToChannel:localChannel];

	dir = (mode == IO_Decrement ? DMA_ADRS_DECR : DMA_ADRS_INCR);
	dma_chan_adrs_dir(realChannel, dir);
	return IO_R_SUCCESS;
}


/*
 *  Returns a DMA buffer for the contents of physical memory starting at
 *  addr and continuing for length bytes.  If the physical address changed
 *  to accommodate the ISA bus, the new physical address is returned in
 *  place.  The DMABuffer is an opaque type.
 *
 *  Warning: this method allocates memory, so it can block!
 */
- (IOEISADMABuffer) createDMABufferFor:(unsigned int *) physAddr
			 length:(unsigned int) length
			 read:(BOOL) isRead
			 needsLowMemory:(BOOL) lowerMem
			 limitSize:(BOOL) limitSize
{
	dma_xfer_t *xfer = IOMalloc(sizeof(dma_xfer_t));

	if (xfer == NULL)
		return NULL;

	xfer->phys = *physAddr;
	xfer->len = length;
	xfer->read = isRead;
	xfer->lower16 = lowerMem;
	xfer->bound64 = limitSize;
	xfer->buffered = FALSE;

	if (!dma_xfer(xfer, physAddr)) {
		xpr_dmabuf("createDMABufferFor: dma_xfer() failed; physAddr"
			"0x%x\n", physAddr, 2,3,4,5);
		IOFree(xfer, sizeof(dma_xfer_t));
		return NULL;
	}
	
	xpr_dmabuf("createDMABufferFor: xfer 0x%x\n", xfer, 2,3,4,5);
	return (IOEISADMABuffer) xfer;
}


- (void) freeDMABuffer:(IOEISADMABuffer) dmaBuffer
{
	dma_xfer_t *xfer = (dma_xfer_t *) dmaBuffer;
	
	xpr_dmabuf("freeDMABuffer: dmaBuffer active %d 0x%x\n", 
		xfer, xfer->active,3,4,5);
	if (xfer->active)
		dma_xfer_done(xfer);
	IOFree(xfer, sizeof(dma_xfer_t));
}


- (void) abortDMABuffer:(IOEISADMABuffer) dmaBuffer
{
	dma_xfer_t *xfer = (dma_xfer_t *) dmaBuffer;
	
	xpr_dmabuf("abortDMABuffer: dmaBuffer active %d 0x%x\n", 
		xfer, xfer->active,3,4,5);
	if (xfer->active)
		dma_xfer_abort(xfer);
	IOFree(xfer, sizeof(dma_xfer_t));
}

- (IOReturn) startDMAForBuffer:(IOEISADMABuffer) buffer channel:(unsigned int) localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return IO_R_INVALID_ARG;

	realChannel = [self _localToChannel:localChannel];

	if (dma_xfer_chan(realChannel, (dma_xfer_t *) buffer))
		return IO_R_SUCCESS;
	else
		return IO_R_NO_FRAMES;			/* XXX */
}

/*
 * Return localChannel's current address and count.
 */
- (unsigned)currentAddressForChannel	: (unsigned)localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return 0;
	realChannel = [self _localToChannel:localChannel];
	return(get_dma_addr(realChannel));
}

- (unsigned)currentCountForChannel	: (unsigned)localChannel
{
	unsigned int realChannel;
	
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return 0;
	realChannel = [self _localToChannel:localChannel];
	return(get_dma_count(realChannel));
}


- (BOOL) isDMADone:(unsigned int) localChannel
{
	if (localChannel >= [(IOEISADeviceDescription *)_deviceDescription numChannels])
		return NO;

	return (is_dma_done([self _localToChannel:localChannel]) ?
		YES : NO);
}




/*
 * Determine whether or not the aassociated device is connected to an EISA 
 * bus. Returns YES if so, else returns NO.
 */
extern BOOL is_ISA;		// in driverkit/i386/autoconf_i386.m

- (BOOL)isEISAPresent
{
	return (is_ISA ? NO : YES);
}

- (BOOL)getEISAId: (unsigned int *)_id  forSlot: (int) slot
{
	return eisa_id(slot, _id);
}

/*
 * DMA lock package. 
 */
 
typedef struct _dmaQueueEntry dmaQueueEntry;
struct _dmaQueueEntry {
	id		device;
	int		count;
	dmaQueueEntry	*next;
};

static dmaQueueEntry *dmaQueueHead;
static dmaQueueEntry *dmaQueueTail;
static int sleepChannel;			// for sleep/wakeup
static simple_lock_t dmaSpinLock;		// ditto

int dmaLockDisable = 0;

extern void thread_wakeup(int x);

/*
 * Init routine, called out from dev_server_init().
 */
void initDmaLock()
{
	dmaQueueHead = NULL;
	dmaQueueTail = NULL;
	dmaSpinLock  = simple_lock_alloc();
	simple_lock_init(dmaSpinLock);
}

/*
 * Sleep until specified dmaQueueEntry is at the head of dmaQueue.
 * Note we have to wait for a specific dmaQueueEntry to bubble to the head,
 * not just any dmaQueueEntry for a specific device, since one device can have 
 * one entry at the head and another somewhere back in line. 
 *
 * dmaSpinLock held on entry and exit.
 */
static void waitTilHead(dmaQueueEntry *queueEntry)
{
	while(queueEntry != dmaQueueHead) {
		thread_sleep((int)&sleepChannel, dmaSpinLock, FALSE); 
		simple_lock(dmaSpinLock);
	}
}

/*
 * Cons up a new dmaQueueEntry, count of 1, for specified device.
 */
static inline dmaQueueEntry *queueEntryAlloc(id device)
{
	dmaQueueEntry *queueEntry = IOMalloc(sizeof(dmaQueueEntry));
	
	queueEntry->count  = 1;
	queueEntry->device = device;
	queueEntry->next   = NULL;
	return queueEntry;
}

/*
 * Free a dmaQueueEntry.
 */
static inline void queueEntryFree(dmaQueueEntry *queueEntry)
{
	ASSERT(queueEntry->count == 0);
	IOFree(queueEntry, sizeof(dmaQueueEntry));
}

/*
 * Acquire dma lock. Allows multiple threads associated with one device
 * instance to hold the DMA lock siimultaneously. If calling device
 * owns the lock but other devices are blocked waiting for it, calling 
 * thread will block, and will not acquire the lock until all other pending
 * requests are honored.
 */
- (void)reserveDMALock
{
	dmaQueueEntry *queueEntry;
	dmaQueueEntry *newQueueEntry;
	
	if([self isEISAPresent] || dmaLockDisable) {
		return;
	}
	newQueueEntry = queueEntryAlloc(self);
	simple_lock(dmaSpinLock);
	if(dmaQueueHead == NULL) {
		/*
		 * Trivial quiescent case.
		 */
		dmaQueueHead = dmaQueueTail = newQueueEntry;
		simple_unlock(dmaSpinLock);
		return;
	}
	queueEntry = dmaQueueHead;
	if((queueEntry->device == self) && (queueEntry->next == NULL)) {
	   	/*
		 * This device owns the lock, and no other requests pending.
		 */
		queueEntry->count++;
		simple_unlock(dmaSpinLock);
		queueEntryFree(newQueueEntry);
		return;
	}
	
	/*
	 * See if any subsequent entries (after the first one) match this
	 * device.
	 */
	queueEntry = queueEntry->next;
	while(queueEntry) {
		if(queueEntry->device == self) {
			/*
			 * Calling device has a request pending, not at head 
			 * of queue. Lump these together.
			 */
			queueEntry->count++;
			waitTilHead(queueEntry);
			simple_unlock(dmaSpinLock);
			queueEntryFree(newQueueEntry);
			return;
		}
		queueEntry = queueEntry->next;
	}
	
	/*
	 * New request for this device.
	 */
	dmaQueueTail->next = newQueueEntry;
	dmaQueueTail = newQueueEntry;
	waitTilHead(newQueueEntry);
	simple_unlock(dmaSpinLock);
	return;
}

- (void)releaseDMALock
{
	dmaQueueEntry *queueEntry = dmaQueueHead;
	dmaQueueEntry *freeQueueEntry = NULL;
	boolean_t needWakeup = FALSE;
	
	if([self isEISAPresent] || dmaLockDisable) {
		return;
	}
	simple_lock(dmaSpinLock);
	if(queueEntry->device != self) {
		simple_unlock(dmaSpinLock);
		IOLog("%s: releaseDmaLock when not holding lock\n", 
			[self name]);
		panic("releaseDMALock");
	}
	if(--queueEntry->count == 0) {
		/*
		 * This device is done. Wake up others, if any.
		 */
		needWakeup = (queueEntry->next != NULL);
		freeQueueEntry = queueEntry;
		dmaQueueHead = queueEntry->next;
	}
	simple_unlock(dmaSpinLock);
	if(needWakeup) {
		thread_wakeup((int)&sleepChannel);
	}
	if(freeQueueEntry != NULL) {
		queueEntryFree(freeQueueEntry);
	}
}

/*
 * Support for the extended mode register (EISA only).
 */

- (IOReturn)setDMATransferWidth	: (IOEISADMATransferWidth)width
                     forChannel : (unsigned)localChannel
{
	unsigned int realChannel;
	int dmaWidth;
	
	if((width == IO_16BitWordCount) || is_ISA) {
		return IO_R_UNSUPPORTED;
	}
	if (localChannel >= [[self deviceDescription] numChannels]) {
		return IO_R_INVALID_ARG;
	}
	realChannel = [self _localToChannel:localChannel];

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

- (IOReturn)getDMATransferWidth	: (IOEISADMATransferWidth *)width_p
                     forChannel : (unsigned)localChannel
{
	unsigned int realChannel;
	int width;
	
	if (localChannel >= [[self deviceDescription] numChannels]) {
		return IO_R_INVALID_ARG;
	}
	realChannel = [self _localToChannel:localChannel];

	width = get_dma_xfer_width(realChannel);
	switch(width) {
	    case DMA_XFR_8_BIT:
	    	*width_p = IO_8Bit;
		break;
	    case DMA_XFR_16_BIT_WORD:
	    	*width_p = IO_16BitWordCount;
		break;
	    case DMA_XFR_16_BIT_BYTE:
	    	*width_p = IO_16BitByteCount;
		break;
	    case DMA_XFR_32_BIT:
	    	*width_p = IO_32Bit;
		break;
	    default:	/* should never be reached */
		return IO_R_NOT_ATTACHED;
	}
	return IO_R_SUCCESS;
}

/*
 * Select DMA Timing.
 */
- (IOReturn)setDMATiming	: (IOEISADMATiming)timing
                     forChannel : (unsigned)localChannel
{
	unsigned int realChannel;
	int dmaTiming;
	
	if(is_ISA) {
		return IO_R_UNSUPPORTED;
	}
	if (localChannel >= [[self deviceDescription] numChannels]) {
		return IO_R_INVALID_ARG;
	}
	realChannel = [self _localToChannel:localChannel];

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

/*
 * Select whether EOP pin is output (default) or input.
 */
- (IOReturn)setEOPAsOutput	: (BOOL)flag
                     forChannel	: (unsigned)localChannel
{
	unsigned realChannel;
	unsigned dmaFlag;
	
	if(is_ISA) {
		return IO_R_UNSUPPORTED;
	}
	if (localChannel >= [[self deviceDescription] numChannels]) {
		return IO_R_INVALID_ARG;
	}
	realChannel = [self _localToChannel:localChannel];

	dmaFlag = (flag ? DMA_EOP_OUT : DMA_EOP_IN);
	dma_eop_in(realChannel, dmaFlag);
	return IO_R_SUCCESS;
}

/*
 * Enable Stop register. Default is disabled; enabling this feature
 * is not supported in [PR2]the current release.
 */
- (IOReturn)setStopRegisterMode : (IOEISAStopRegisterMode)mode
                    forChannel 	: (unsigned)localChannel
{
	unsigned realChannel;
	unsigned dmaFlag;
	
	if((mode == IO_StopRegisterEnable) || is_ISA) {
		return IO_R_UNSUPPORTED;
	}
	if (localChannel >= [[self deviceDescription] numChannels]) {
		return IO_R_INVALID_ARG;
	}
	realChannel = [self _localToChannel:localChannel];

	if(mode == IO_StopRegisterEnable) {
		dmaFlag = DMA_STOP_ENABLE;
	}
	else {
		dmaFlag = DMA_STOP_DISABLE;
	}
	dma_stop_enable(realChannel, dmaFlag);
	return IO_R_SUCCESS;
}

- (IOReturn) reservePortRange	: (unsigned int) localPortRange
{
	return IO_R_SUCCESS;
}

- (void) releasePortRange	: (unsigned int) localPortRange
{
}

@end
