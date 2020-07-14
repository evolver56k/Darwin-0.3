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
 * Copyright (c) 1995-1996 NeXT Software, Inc.
 *
 * Hardware independent (relatively) code for the DECchip 2104x
 *
 * HISTORY
 *
 * 26-Apr-95	Rakesh Dubey (rdubey) at NeXT 
 *	Created.
 * 11-Oct-95	Dieter Siegmund (dieter) at NeXT
 *	Added performance hooks; changed transmit interrupt
 *	to only interrupt every N / 2 times, where N is the
 *	number of transmit descriptors.  This reduces the
 *      number of interrupts taken, and the amount of CPU
 *	used by the driver.
 *	Removed timeout mechanism.
 * 02-Nov-95	Rakesh Dubey (rdubey) at NeXT 
 *	Added support for 21041 based adapters.
 *	   -- serial ROM for station address
 *	   -- new network interface selection scheme
 * 11-Dec-95	Dieter Siegmund (dieter) at NeXT
 *	Re-named 2104x.m, split out 21040 and 21041 personalities.
 * 07-Feb-96	Dieter Siegmund (dieter) at NeXT
 *	Fixed multicast.
 * 25-Jun-96	Dieter Siegmund (dieter) at NeXT
 *	Added kernel debugger support.
 * 05-Aug-97	Joe Liu at Apple
 *	Made kernel debugger support more robust.
 */

#import "DECchip2104x.h"
#import "DECchip2104xPrivate.h"


//#define DEBUG

#import "DECchip2104xInline.h"

@implementation DECchip2104x

/*
 * Public Factory Methods
 */
 
+ (BOOL)probe:(IOPCIDevice *)devDesc
{
    DECchip2104x		*dev;

    IOLog("%s: PCI Dev: %d Func: %d Bus: %d\n", [self name],
	  devDesc->deviceNum, devDesc->functionNum, devDesc->busNum);

    // Veryfy the corect number of memory ranges
    // Range 0 is I/O Space access to CSR's
    // Range 1 is Memory Space access to Expansion ROM
    // Range 2 is Memory Space access to CSR's
    if ([devDesc numMemoryRanges] != 3) {
      IOLog("%s: Incorrect number of memory ranges -> %d\n",
	    [self name], [devDesc numMemoryRanges]);
      return NO;
    }

    dev = [self alloc];
    if (dev == nil) {
	IOLog("%s: Failed to alloc instance\n", [self name]);
    	return NO;
    }

    return [dev initFromDeviceDescription:devDesc] != nil;
}

/*
 * Public Instance Methods
 */
- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    if ([super initFromDeviceDescription:devDesc] == nil) {
	[self free];
	return nil;
    }
    KDB_txBuf = [self allocateNetbuf];
    if (KDB_txBuf == NULL) {
	IOLog("%s: couldn't allocate KDB netbuf\n", [self name]);
	[self free];
	return nil;
    }
    resetAndEnabled = NO;
    return self;
}

- free
{
    int		i;
    
    [self clearTimeout];
    
    [self _resetChip];
    
    if (networkInterface)
	[networkInterface free];

    for (i = 0; i < DEC_21X40_RX_RING_LENGTH; i++)
    	if (rxNetbuf[i])
	    nb_free(rxNetbuf[i]);
    for (i = 0; i < DEC_21X40_TX_RING_LENGTH; i++)
    	if (txNetbuf[i])
	    nb_free(txNetbuf[i]);

    
//    if (memoryPtr) {
//	IOFree(memoryPtr, memorySize);
//    }
//    [self enableAllInterrupts];
    
    return [super free];
}

- (void) enableAdapterInterrupts
{

//kprintf("[DECchip2104x enableAdapterInterrupts]\n");

    writeCsr(ioBase, DEC_21X40_CSR7, interruptMask.data);
}

- (void) disableAdapterInterrupts
{

//kprintf("[DECchip2104x disableAdapterInterrupts]\n");

    writeCsr(ioBase, DEC_21X40_CSR7, 0);
}

- (BOOL)resetAndEnable:(BOOL)enable
{

//kprintf("[DECchip2104x resetAndEnable]\n");

    resetAndEnabled = NO;
    [self clearTimeout];
    [self disableAdapterInterrupts];
    
    [self _resetChip];
    
    if (enable)	{
    
	if (![self _initRxRing] || ![self _initTxRing])	{
	    return NO;
	}

	if ([self _initChip] == NO) {
	    [self setRunning:NO];
	    return NO;
	}

	[self _startTransmit];
	[self _startReceive];
	
	if ([self enableAllInterrupts] != IO_R_SUCCESS) {
	    [self setRunning:NO];
	    return NO;
	}
	[self enableAdapterInterrupts];
    } 
    
    [self setRunning:enable];
    
    resetAndEnabled = YES;
    return YES;
}


- (void)interruptOccurred
{    
    csrRegUnion	interruptReg;

//kprintf("[DECchip2104x interruptOccurred]\n");

#ifdef PERF    
    if (perfQ) {
	ev_p.eventId = PERF_INTR_ENTER_ID;
	perfEvent(perfQ, &ev_p);
    }
#endif PERF
    
    while (1) {
    
	[self reserveDebuggerLock];
	interruptReg.data = readCsr(ioBase, DEC_21X40_CSR5);
	writeCsr(ioBase, DEC_21X40_CSR5, interruptReg.data);
	[self releaseDebuggerLock];
#ifdef DEBUG
    	IOLog("%s: interruptOccurred: status %x\n", [self name],
		interruptReg.data);
#endif DEBUG
	
	if (!interruptReg.csr5.ti && !interruptReg.csr5.ri &&
			!interruptReg.csr5.tjt)	{
#ifdef DEBUG
	    IOLog("%s: BOGUS interrupt: status %x\n", [self name],
		    interruptReg.data);
#endif DEBUG
	    break;
	}
		
	if (interruptReg.csr5.ri) {
	    [self _receiveInterruptOccurred];
	}   
	if (interruptReg.csr5.ti) {
		[self reserveDebuggerLock];
	    [self _transmitInterruptOccurred];
		[self releaseDebuggerLock];
	    [self serviceTransmitQueue];
	}   
	if (interruptReg.csr5.tjt) {
	    //[self _jabberInterruptOccurred];
	}   
    }
    [self enableAllInterrupts];
#ifdef PERF    
    if (perfQ) {
	ev_p.eventId = PERF_INTR_EXIT_ID;
	perfEvent(perfQ, &ev_p);
    }
#endif PERF
}
    
- (void) serviceTransmitQueue
{
    netbuf_t			packet;

//kprintf("[DECchip2104x serviceTransmitQueue]\n");


    while (txNumFree && [transmitQueue count] > 0 
	   && (packet = [transmitQueue dequeue]))
        [self _transmitPacket:packet];
}

- (void)transmit:(netbuf_t)pkt
{

//kprintf("[DECchip2104x transmit]\n");

    if (!pkt) {
    	IOLog("%s: transmit: received NULL netbuf\n", [self name]);
	return;
    }
    
    if (![self isRunning]) {
	nb_free(pkt);
	return;
    }
    
	[self reserveDebuggerLock];
    [self _transmitInterruptOccurred]; /* check for tx completions */
	[self releaseDebuggerLock];
	
    /*
     * Queue the packet if we're out of transmit descriptors
     */
    [self serviceTransmitQueue];
    if (txNumFree == 0 || [transmitQueue count] > 0)
	[transmitQueue enqueue:pkt];
    else
	[self _transmitPacket:pkt];

}

- (int) transmitQueueSize
{
//kprintf("[DECchip2104x transmitQueueSize]\n");


    return (TRANSMIT_QUEUE_SIZE);
}

- (int) transmitQueueCount
{
//kprintf("[DECchip2104x transmitQueueCount]\n");


    return ([transmitQueue count]);
}

- (int) pendingTransmitCount
{

//kprintf("[DECchip2104x pendingTransmitCount]\n");

    return ([transmitQueue count] + (DEC_21X40_TX_RING_LENGTH - txNumFree));
}

- (void)timeoutOccurred
{
//kprintf("[DECchip2104x timeoutOccurred]\n");

#ifdef PERF    
    if (perfQ) {
	ev_p.eventId = PERF_INTR_ENTER_ID;
	perfEvent(perfQ, &ev_p);
    }
#endif PERF

#ifdef DEBUG
    IOLog("tx timeout: txPutIndex=%d, txDoneIndex=%d, txNumFree=%d\n",
    	txPutIndex, txDoneIndex, txNumFree);
#endif DEBUG

    if ([self isRunning]) {
	[self reserveDebuggerLock];
	[self _transmitInterruptOccurred];
	[self releaseDebuggerLock];
	[self serviceTransmitQueue];
    }
#ifdef PERF    
    if (perfQ) {
	ev_p.eventId = PERF_INTR_EXIT_ID;
	perfEvent(perfQ, &ev_p);
    }
#endif PERF
}

- (netbuf_t)allocateNetbuf
{
    netbuf_t		packet;
    unsigned int 	packetAlignment;

//kprintf("[DECchip2104x allocateNetbuf]\n");
	
    packet = nb_alloc(NETWORK_BUFSIZE + NETWORK_BUFALIGNMENT);
    if (packet) {
	packetAlignment = 
		(unsigned)nb_map(packet) & (NETWORK_BUFALIGNMENT - 1);
	if (packetAlignment)
	    nb_shrink_top(packet, NETWORK_BUFALIGNMENT - packetAlignment);
	nb_shrink_bot(packet, nb_size(packet) - ETHERMAXPACKET);
    }
#ifdef DEBUG
    else {
    	IOLog("%s: nb_alloc() returned NULL\n", [self name]);
//	IOBreakToDebugger();
    }
#endif DEBUG
    
    return packet;
}

- (BOOL)enablePromiscuousMode
{
    csrRegUnion	reg;

//kprintf("[DECchip2104x enablePromiscuousMode]\n");
    
    isPromiscuous = YES;

    [self reserveDebuggerLock];
    reg.data = readCsr(ioBase, DEC_21X40_CSR6);
    reg.csr6.pr = 1;
    writeCsr(ioBase, DEC_21X40_CSR6, reg.data);
    [self releaseDebuggerLock];

    return YES;
}

- (void)disablePromiscuousMode
{
    csrRegUnion	reg;

//kprintf("[DECchip2104x disablePromiscuousMode]\n");
    
    isPromiscuous = NO;
    
    [self reserveDebuggerLock];
    reg.data = readCsr(ioBase, DEC_21X40_CSR6);
    reg.csr6.pr = 0;
    writeCsr(ioBase, DEC_21X40_CSR6, reg.data);
    [self releaseDebuggerLock];

    return;
}

- (BOOL)enableMulticastMode
{

//kprintf("[DECchip2104x enableMulticastMode]\n");

    multicastEnabled = YES;
    return YES;
}
	 
- (void)disableMulticastMode
{

//kprintf("[DECchip2104x disableMulticastMode]\n");

    /* 
     * Clear out any current addresses 
     */
    if (multicastEnabled) {
	[self reserveDebuggerLock];
	if (![self _setAddressFiltering:NO])
	    IOLog("%s: disable multicast mode failed\n",[self name]);
	[self releaseDebuggerLock];
    }
    multicastEnabled = NO;
}

- (void)addMulticastAddress:(enet_addr_t *)addr
{

//kprintf("[DECchip2104x addMulticastAddress: %x:%x:%x:%x:%x:%x]\n",
//	addr->ea_byte[0],addr->ea_byte[1],addr->ea_byte[2],
//	addr->ea_byte[3],addr->ea_byte[4],addr->ea_byte[5]);

    multicastEnabled = YES;
    [self reserveDebuggerLock];
    if (![self _setAddressFiltering:NO])
    	IOLog("%s: add multicast address failed\n",[self name]);
    [self releaseDebuggerLock];
}

- (void)removeMulticastAddress:(enet_addr_t *)addr
{

//kprintf("[DECchip2104x removeMulticastAddress]\n");

    [self reserveDebuggerLock];
    if (![self _setAddressFiltering:NO])
    	IOLog("%s: remove multicast address failed\n",[self name]);
    [self releaseDebuggerLock];
}

/*
 * Power management methods. 
 */
- (IOReturn)getPowerState:(PMPowerState *)state_p
{
    return IO_R_UNSUPPORTED;
}

- (IOReturn)setPowerState:(PMPowerState)state
{
    if (state == PM_OFF) {
	resetAndEnabled = NO;
        [self _resetChip];
	return IO_R_SUCCESS;
    }
    return IO_R_UNSUPPORTED;
}

- (IOReturn)getPowerManagement:(PMPowerManagementState *)state_p
{
    return IO_R_UNSUPPORTED;
}

- (IOReturn)setPowerManagement:(PMPowerManagementState)state
{
    return IO_R_UNSUPPORTED;
}

- (void)getStationAddress:(enet_addr_t *)ea
{

//kprintf("[DECchip2104x getStationAddress]\n");

    return;
}

- (void)selectInterface
{
//kprintf("[DECchip2104x selectInterface]\n");

    return;
}


@end

