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
 * Implementation for hardware dependent (relatively) code 
 * for the DECchip 21040 and 21041. 
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
 * 02-Nov-95	Rakesh Dubey (rdubey) at NeXT 
 *	Added support for 21041 based adapters.
 *	   -- serial ROM for station address
 *	   -- new network interface selection scheme
 * 11-Dec-95	Dieter Siegmund (dieter) at NeXT
 *	Split out 21040 and 21041 connector auto-detect logic.
 * 07-Feb-96	Dieter Siegmund (dieter) at NeXT
 *	Fixed multicast.
 * 25-Jun-96	Dieter Siegmund (dieter) at NeXT
 *	Added kernel debugger support.
 * 05-Aug-97	Joe Liu at Apple
 *	Made kernel debugger support more robust.
 */

#import "DECchip2104x.h"
#import "DECchip2104xPrivate.h"

#import <machdep/ppc/proc_reg.h>
#import <mach/vm_param.h>			// PAGE_SIZE

extern void                     *kernel_map;
extern kern_return_t            kmem_alloc_wired();

//#define DEBUG

#import "DECchip2104xInline.h"

#ifdef DEBUG
static void IOBreak()
{
    //IOBreakToDebugger();
    return;
}

static void printDesc(void *descriptorStruct)
{
    txDescriptorStruct *desc = (txDescriptorStruct *)descriptorStruct;
    
    IOLog("DESC: ");
    IOLog("%x (%x) ", (unsigned int)&desc->status.data, desc->status.data);
    IOLog("%x (%x: %x %x) ", (unsigned int)&desc->control.data, desc->control.data, desc->control.reg.byteCountBuffer1, desc->control.reg.byteCountBuffer2);
    IOLog("%x (%x) ", (unsigned int)&desc->bufferAddress1, desc->bufferAddress1);
    IOLog("%x (%x) ", (unsigned int)&desc->bufferAddress2, desc->bufferAddress2);
    IOLog("\n");
    
}
#endif DEBUG

/*
 * Breaks up an ethernet data buffer into two physical chunks. We know that
 * the buffer can't straddle more than two pages. If the content of paddr2 is
 * zero this means that all of the buffer lies in one physical page. Note
 * that we use the fact that tx and rx descriptors have the same size and
 * same layout of relevent fields (data address and count). 
 */
static BOOL 
IOUpdateDescriptorFromNetBuf(netbuf_t nb, void *desc, BOOL isReceive)
{
    IOReturn 		result;
    vm_address_t 	pageBreak;
    unsigned int 	len;
    vm_address_t	vaddr;
    unsigned int 	*paddr1, *paddr2;
    
    struct _descriptorStruct {
    	unsigned int status;
	union	{
	    struct {
	        unsigned int
		    rsvd		:10,
		    byteCountBuffer2	:11,
		    byteCountBuffer1	:11;
	    } reg;
	    unsigned int data;
	} control;
	unsigned int bufferAddress1;
	unsigned int bufferAddress2;
    } *descriptorStruct;
    
    
    descriptorStruct = (struct _descriptorStruct *)desc;
    len = isReceive ? NETWORK_BUFSIZE : nb_size(nb);
    
    vaddr = (vm_address_t)nb_map(nb);
    paddr1 = &descriptorStruct->bufferAddress1;
    paddr2 = &descriptorStruct->bufferAddress2;
    
    /* Assume contiguity */
    *paddr2 = 0;
    descriptorStruct->control.reg.byteCountBuffer2 = 0;
    descriptorStruct->control.reg.byteCountBuffer1 = len;
    
    result = IOPhysicalFromVirtual(IOVmTaskSelf(), vaddr, paddr1);
    if (result != IO_R_SUCCESS) {
	return NO;
    }
    
    /*
     * Now check if this memory block crosses a page boundary. 
     */
    if (trunc_page(vaddr) == trunc_page(vaddr+len))	{
    	return YES;
    }
    
    /* Nice try... */
    pageBreak = round_page(vaddr);
    descriptorStruct->control.reg.byteCountBuffer1 = pageBreak - vaddr;
    descriptorStruct->control.reg.byteCountBuffer2 = len - (pageBreak - vaddr);
    
    result = IOPhysicalFromVirtual(IOVmTaskSelf(), pageBreak, paddr2);
    if (result != IO_R_SUCCESS) {
	return NO;
    }
    
    return YES;
}


@implementation DECchip2104x (Private)

/*
 * Private functions
 */
- (BOOL)_allocateMemory
{
    int			i;
    IOReturn		result;

//kprintf("[DECchip2104x _allocateMemory]\n");

    memorySize = sizeof(rxDescriptorStruct) * DEC_21X40_RX_RING_LENGTH
	+ sizeof(txDescriptorStruct) * DEC_21X40_TX_RING_LENGTH
	+ DEC_21X40_DESCRIPTOR_ALIGNMENT * 2
	+ sizeof(setupBuffer_t) + DEC_21X40_DESCRIPTOR_ALIGNMENT;
    if (memorySize > PAGE_SIZE) {
    	IOLog("%s: 1 page limit exceeded for descriptor memory\n",[self name]);
	return NO;
    }

    /*
     * We need one page of contiguous memory.
     */
    result = kmem_alloc_wired(kernel_map,(vm_offset_t *)&memoryPtr,memorySize);
    if (result != KERN_SUCCESS) {
        IOLog ("%s: can't allocate 0x%x bytes of memory\n",
	       [self name], memorySize);
	return NO;
    }

    rxRing = (rxDescriptorStruct *)memoryPtr;
    if (!IOIsAligned(rxRing, DEC_21X40_DESCRIPTOR_ALIGNMENT))
    	rxRing = IOAlign(rxDescriptorStruct *, rxRing, 
	    DEC_21X40_DESCRIPTOR_ALIGNMENT);
    for (i = 0 ; i < DEC_21X40_RX_RING_LENGTH ; i++) {
    	bzero((void *)&rxRing[i], sizeof(rxDescriptorStruct));
	rxNetbuf[i] = NULL;
    }
    
    txRing = (txDescriptorStruct *)(rxRing + DEC_21X40_RX_RING_LENGTH);
    if (!IOIsAligned(txRing, DEC_21X40_DESCRIPTOR_ALIGNMENT))
    	txRing = IOAlign(txDescriptorStruct *, txRing, 
	    DEC_21X40_DESCRIPTOR_ALIGNMENT);
    for (i = 0 ; i < DEC_21X40_TX_RING_LENGTH ; i++) {
    	bzero((void *)&txRing[i], sizeof(txDescriptorStruct));
	txNetbuf[i] = NULL;
    }
    
    setupBuffer = (setupBuffer_t *)(txRing + DEC_21X40_TX_RING_LENGTH);
    if (!IOIsAligned(setupBuffer, DEC_21X40_DESCRIPTOR_ALIGNMENT))
    	setupBuffer = IOAlign(setupBuffer_t *, 
		setupBuffer, DEC_21X40_DESCRIPTOR_ALIGNMENT);
    result = IOPhysicalFromVirtual(IOVmTaskSelf(),
	(vm_address_t)setupBuffer, (unsigned *)&setupBufferPhysical);
    if (result != IO_R_SUCCESS) {
        IOLog("%s: Invalid shared memory address\n", [self name]);
        return NO;
    }
    
    return YES;
}

#ifdef DEBUG
- (void)_dumpRegisters
{
    int i;
    
    return;
    IOLog("%s: Regs: ", [self name]);
    for (i = 0; i <= 15; i++) {
    	IOLog("%02d: %x ", i, readCsr(ioBase, 8*i));
    }
    IOLog("\n");
}
#endif DEBUG

#ifdef DEBUG
- (void)_dumpDescriptor:(void *)ds
{
    txDescriptorStruct *desc = (txDescriptorStruct *)ds;
    
    IOLog("%s: Desc: ", [self name]);
    IOLog("%x  ", *(unsigned int *)&desc->status);
    IOLog("%x  ", *(unsigned int *)&desc->control);
    IOLog("%x  ", *(unsigned int *)&desc->bufferAddress1);
    IOLog("%x  ", *(unsigned int *)&desc->bufferAddress2);
    IOLog("\n");
}
#endif DEBUG

- (BOOL)_initTxRing
{
    unsigned int i;

//kprintf("[DECchip2104x _initTxRing]\n");
    
    for (i = 0; i < DEC_21X40_TX_RING_LENGTH; i++) {
	bzero((void *)&txRing[i], sizeof(txDescriptorStruct));
	txRing[i].status.reg.own = DEC_21X40_DESC_OWNED_BY_HOST;
	if (txNetbuf[i]) {
	    nb_free(txNetbuf[i]);
	    txNetbuf[i] = NULL;
	}
    }
    txRing[DEC_21X40_TX_RING_LENGTH-1].control.reg.ter = YES;
     
    txPutIndex = 0;
    txDoneIndex = 0;
    txNumFree = DEC_21X40_TX_RING_LENGTH;
    txIntCount = 0;

    /* Init the transmit queue. */
    if (transmitQueue)	{
	[transmitQueue free];
    }
    transmitQueue = [[IONetbufQueue alloc] 
    			initWithMaxCount:TRANSMIT_QUEUE_SIZE];
    if (!transmitQueue)	{
        IOPanic("_initTxRing");
    }
    
    return YES;
}

- (BOOL)_initRxRing
{
    int 	i;
    BOOL	status;

//kprintf("[DECchip2104x _initRxRing]\n");

    for (i = 0; i < DEC_21X40_RX_RING_LENGTH; i++) {
	bzero((void *)&rxRing[i], sizeof(rxDescriptorStruct));
	rxRing[i].status.reg.own = DEC_21X40_DESC_OWNED_BY_HOST;
	if (rxNetbuf[i] == NULL)	{
	    rxNetbuf[i] = [self allocateNetbuf];
	    if (rxNetbuf[i] == NULL)	{
	    	IOPanic("allocateNetbuf returned NULL in _initRxRing");
	    }
	}
	status = IOUpdateDescriptorFromNetBuf(rxNetbuf[i], 
			(void *)&rxRing[i], YES);
	if (status == NO)
	    IOPanic("_initRxRing");
	rxRing[i].status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
    }
    
    rxRing[DEC_21X40_RX_RING_LENGTH-1].control.reg.rer = YES;

    rxDoneIndex = 0;
    
    return YES;
}


- (BOOL) _initChip
{

//kprintf("[DECchip2104x _initChip]\n");

    [self _initRegisters];
    [self _startTransmit];

    /*
     * Set up station, broadcast, and multicast addresses
     */
    if (![self _setAddressFiltering:YES])
       return NO;

    [self selectInterface];

#ifdef DEBUG
    [self _dumpRegisters];
#endif DEBUG
    
    return YES;
}

- (void)_resetChip
{
    csrRegUnion reg;

//kprintf("[DECchip2104x _resetChip]\n");
    
    reg.data = 0;
    reg.csr0.swr = 1;			/* reset */
    writeCsr(ioBase, DEC_21X40_CSR0, reg.data);
    IODelay(100);
    reg.csr0.swr = 0;			/* deassert */
    writeCsr(ioBase, DEC_21X40_CSR0, reg.data);
    
    IOSleep(1);				/* >= 100 PCI bus cycles */
}

- (void)_startTransmit
{

//kprintf("[DECchip2104x _startTransmit]\n");

    operationMode.csr6.st = YES;
    writeCsr(ioBase, DEC_21X40_CSR6, operationMode.data);
}

- (void)_startReceive
{

//kprintf("[DECchip2104x _startReceive]\n");

    operationMode.csr6.sr = YES;
    writeCsr(ioBase, DEC_21X40_CSR6, operationMode.data);
}

- (void)_initRegisters
{
    IOReturn 	result;
    csrRegUnion reg;
   
    unsigned int paddr;

//kprintf("[DECchip2104x _initRegisters]\n");
    
    [self _resetChip];
    
    reg.data = 0;
    reg.csr0.dbo = 1;           /* Big Endian Descriptors */
//    reg.csr0.ble = 1;           /* Big Endian Buffers */   
    reg.csr0.pbl = 16;		/* errata: don't set to 0 or 32 */
    reg.csr0.cal = 1;		/* depends upon NETWORK_BUFALIGNMENT */
    writeCsr(ioBase, DEC_21X40_CSR0, reg.data);		/* bus mode */
    
    result = IOPhysicalFromVirtual(IOVmTaskSelf(),
	(vm_address_t)rxRing, (unsigned int *)&paddr);
    if (result != IO_R_SUCCESS) {
        IOLog("%s: Invalid shared memory address\n", [self name]);
        return;
    }
    reg.data = 0;
    reg.csr3.rdlba = (unsigned int)paddr;
    writeCsr(ioBase, DEC_21X40_CSR3, reg.data);	/* receive list base */
    
    result = IOPhysicalFromVirtual(IOVmTaskSelf(),
	(vm_address_t)txRing, (unsigned int *)&paddr);
    if (result != IO_R_SUCCESS) {
        IOLog("%s: Invalid shared memory address\n", [self name]);
        return;
    }
    reg.data = 0;
    reg.csr4.tdlba = (unsigned int)paddr;
    writeCsr(ioBase, DEC_21X40_CSR4, reg.data);	/* transmit list base */

    reg.data = 0;	/* FIXME -- choose appropriate values */
    writeCsr(ioBase, DEC_21X40_CSR6, reg.data);	/* operations mode */
    operationMode.data = reg.data;
    
    reg.data = 0;
    reg.csr7.tim = reg.csr7.rim = reg.csr7.tjm = 1;
    reg.csr7.nim = 1;
#ifdef DEBUG    
    reg.csr7.tim = reg.csr7.tsm = reg.csr7.tum = reg.csr7.tjm = 1;
    reg.csr7.unm = reg.csr7.rim = reg.csr7.rum = 1;
    reg.csr7.atm = 1;
    reg.csr7.lfm = reg.csr7.sem = reg.csr7.aim = 1;
#endif DEBUG
    interruptMask.data = reg.data;
    
    writeCsr(ioBase, DEC_21X40_CSR11, 0);	/* no full duplex */
}

- (void)_transmitPacket:(netbuf_t)packet
{
    csrRegUnion		reg;
    txDescriptorStruct	*desc;
    BOOL		status;

//kprintf("[DECchip2104x _transmitPacket]\n");
    
    [self performLoopback:packet];

    [self reserveDebuggerLock];

    if (txNumFree == 0) {
	[self releaseDebuggerLock];
	nb_free(packet);
	return;
    }
    
    desc = &txRing[txPutIndex];
    txNetbuf[txPutIndex] = packet;
    
    if (desc->control.reg.ter == 1) {
    	desc->control.data = 0;
	desc->control.reg.ter = 1;
    } 
    else
    	desc->control.data = 0;

    status = IOUpdateDescriptorFromNetBuf(packet, (void *)desc, NO);
    if (status == NO)	{
	[self releaseDebuggerLock];
    	IOLog("%s: _transmitPacket: IOUpdateDescriptorFromNetBuf failed\n",
		[self name]);
	nb_free(packet);		/* this is the best we can do here */
	return;
    }
		
    desc->control.reg.fs = YES;
    desc->control.reg.ls = YES;
    if (++txIntCount == (DEC_21X40_TX_RING_LENGTH / 2)) {
	desc->control.reg.ic = YES;
	txIntCount = 0;
    }
    else
	desc->control.reg.ic = NO;

    desc->status.data = 0;
    desc->status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
    
    // txPutIndex = (txPutIndex + 1) % DEC_21X40_TX_RING_LENGTH;
    if (++txPutIndex == DEC_21X40_TX_RING_LENGTH)
	txPutIndex = 0;
    --txNumFree;

    /*  
     * Make the chip poll immediately.  
     */
    reg.data = 0;
    reg.csr1.tpd = YES;
    writeCsr(ioBase, DEC_21X40_CSR1, reg.data);

    [self releaseDebuggerLock];

    return;
}

/*
 * Method: receivePacket
 *
 * Purpose:
 *   Part of kerneldebugger protocol.
 */
- (void)receivePacket:(void *)pkt length:(unsigned int *)pkt_len
    timeout:(unsigned int)timeout
{
    *pkt_len = 0;
    timeout *= 1000;

    if (resetAndEnabled == NO)
	return;

    while (1) {
	if (rxRing[rxDoneIndex].status.reg.own  
	    == DEC_21X40_DESC_OWNED_BY_HOST) {
	    rxDescriptorStruct *	rds;

	    rds = &rxRing[rxDoneIndex];
	    if (rds->status.reg.es 
		|| (!rds->status.reg.fs || !rds->status.reg.ls) 
		|| (rds->status.reg.fl < ETHERMINPACKET)) {
		rxRing[rxDoneIndex].status.reg.own 
		    = DEC_21X40_DESC_OWNED_BY_CTRL;
		// rxDoneIndex = (rxDoneIndex + 1) % DEC_21X40_RX_RING_LENGTH;
		if (++rxDoneIndex == DEC_21X40_RX_RING_LENGTH)
		    rxDoneIndex = 0;
	    }
	    else
		break;
	}
	else {
	    if ((int)timeout <= 0)
		return;
	    IODelay(50);
	    timeout -= 50;
	}
    }
    {
	rxDescriptorStruct *	rds;

	rds = &rxRing[rxDoneIndex];
	*pkt_len = rds->status.reg.fl - ETHERCRC;
	bcopy(nb_map(rxNetbuf[rxDoneIndex]), pkt, *pkt_len);

	rds->status.data = 0;
	rds->status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
	// rxDoneIndex = (rxDoneIndex + 1) % DEC_21X40_RX_RING_LENGTH;
	if (++rxDoneIndex == DEC_21X40_RX_RING_LENGTH)
	    rxDoneIndex = 0;
    }
    return;
}
/*
 * Method: sendPacket
 *
 * Purpose:
 *   Part of kerneldebugger protocol.
 */
- (void)sendPacket:(void *)pkt length:(unsigned int)pkt_len
{
    int					size;
    volatile txDescriptorStruct	*	desc;
    csrRegUnion				reg;
    BOOL				status;

	if (resetAndEnabled == NO)
		return; /* can't transmit right now */
	
	[self _transmitInterruptOccurred];
	
    if (txNumFree == 0) {
		IOLog("%s: _sendPacket: No free tx descriptors\n", [self name]);
		return; /* can't transmit right now */
	}
	
    desc = &txRing[txPutIndex];
    txNetbuf[txPutIndex] = NULL;
    
    bcopy(pkt, nb_map(KDB_txBuf), pkt_len);
    size = nb_size(KDB_txBuf);
    nb_shrink_bot(KDB_txBuf, size - pkt_len);

    if (desc->control.reg.ter == 1) {
    	desc->control.data = 0;
	desc->control.reg.ter = 1;
    } 
    else
    	desc->control.data = 0;

    status = IOUpdateDescriptorFromNetBuf(KDB_txBuf, (void *)desc, NO);
    if (status == NO)	{
    	IOLog("%s: _sendPacket: IOUpdateDescriptorFromNetBuf failed\n",
	      [self name]);
	return;
    }
		
    desc->control.reg.fs = YES;
    desc->control.reg.ls = YES;
    desc->control.reg.ic = NO;
    desc->status.data = 0;
    desc->status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
    
    // txPutIndex = (txPutIndex + 1) % DEC_21X40_TX_RING_LENGTH;
    if (++txPutIndex == DEC_21X40_TX_RING_LENGTH)
	txPutIndex = 0;
    --txNumFree;

    /*  
     * Make the chip poll immediately.  
     */
    reg.data = 0;
    reg.csr1.tpd = YES;
    writeCsr(ioBase, DEC_21X40_CSR1, reg.data);
    
    { /* poll for completion */
	int i;
	for (i = 0; 
	     i < 10000 && desc->status.reg.own == DEC_21X40_DESC_OWNED_BY_CTRL;
	     i++) {
	    IODelay(500);
	}
    }
    if (desc->status.reg.own == DEC_21X40_DESC_OWNED_BY_CTRL)
	IOLog("%s: _sendPacket: polling timed out\n", [self name]);

    /* restore the actual size of the buffer */
    nb_grow_bot(KDB_txBuf, size - pkt_len);

    return;
}

- (BOOL)_receiveInterruptOccurred
{
    netbuf_t			packet;
    BOOL			passPacketUp;
    rxDescriptorStruct *	rds;
    unsigned int		receivedFrameSize;
    BOOL			status;

//kprintf("[DECchip2104x _receiveInterruptOccurred]\n");
    
    [self reserveDebuggerLock];
    
    while (rxRing[rxDoneIndex].status.reg.own
	   == DEC_21X40_DESC_OWNED_BY_HOST) {
	passPacketUp = NO;
    	rds = &rxRing[rxDoneIndex];
	receivedFrameSize = rds->status.reg.fl - ETHERCRC;
	
	/*
	 * Bad packet. Just leave the netbuf in place.  
	 */
	if (rds->status.reg.es
	    || (!rds->status.reg.fs || !rds->status.reg.ls) 
	    || (receivedFrameSize < (ETHERMINPACKET - ETHERCRC))) {
	    [networkInterface incrementInputErrors];
	    rxRing[rxDoneIndex].status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
	    // rxDoneIndex = (rxDoneIndex + 1) % DEC_21X40_RX_RING_LENGTH;
	    if (++rxDoneIndex == DEC_21X40_RX_RING_LENGTH)
		rxDoneIndex = 0;
	    continue;
	}
	
	/*  Toss packets not in our multicast address-space.  */
	packet = rxNetbuf[rxDoneIndex];
	if (isPromiscuous 
	    || rds->status.reg.mf == NO 
	    || [super isUnwantedMulticastPacket:
		(ether_header_t *)nb_map(packet)] == NO) {
	    netbuf_t newPacket;

	    newPacket = [self allocateNetbuf];
	    if (newPacket != NULL)	{
		rxNetbuf[rxDoneIndex] = newPacket;
		passPacketUp = YES;
		status = 
		    IOUpdateDescriptorFromNetBuf(rxNetbuf[rxDoneIndex], 
						 (void *)&rxRing[rxDoneIndex], 
						 YES);
		if (status == NO) {
		    IOPanic("DECchip21040: IOUpdateDescriptorFromNetBuf\n"); 
		}
		/*  Adjust to size received */
		nb_shrink_bot(packet, nb_size(packet) - receivedFrameSize);
	    } 
	}
	rds->status.data = 0;
	rds->status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;
	
	// rxDoneIndex = (rxDoneIndex + 1) % DEC_21X40_RX_RING_LENGTH;
	if (++rxDoneIndex == DEC_21X40_RX_RING_LENGTH)
	    rxDoneIndex = 0;
	if (passPacketUp) {
	    passPacketUp = NO;
	    [self releaseDebuggerLock];
	    [networkInterface handleInputPacket:packet extra:0];
	    [self reserveDebuggerLock];
	}
    }

    [self releaseDebuggerLock];
    
    return YES;
}

- (BOOL)_transmitInterruptOccurred
{
    txDescriptorStruct		*tds;

//kprintf("[DECchip2104x _transmitInterruptOccurred]\n");
    
    while (txNumFree < DEC_21X40_TX_RING_LENGTH) { 
	tds = &txRing[txDoneIndex];
    	if (tds->status.reg.own == DEC_21X40_DESC_OWNED_BY_CTRL)
	    break;
	
    	if (tds->control.reg.set == 1)	{
	    //IOLog("TX interrupt: Ignoring set up packet\n");
	    // txDoneIndex = (txDoneIndex + 1) % DEC_21X40_TX_RING_LENGTH;
	    if (++txDoneIndex == DEC_21X40_TX_RING_LENGTH)
		txDoneIndex = 0;
	    ++txNumFree;
	    continue;
	}

	if (tds->status.reg.uf || tds->status.reg.ec
		|| tds->status.reg.lc || tds->status.reg.nc 
		|| tds->status.reg.to)
	    [networkInterface incrementOutputErrors];
	else
	    [networkInterface incrementOutputPackets];
	
	if (tds->status.reg.ec)
	    [networkInterface incrementCollisionsBy:16];
	else if (tds->status.reg.cc)
	    [networkInterface incrementCollisionsBy:tds->status.reg.cc];
	if (tds->status.reg.lc && !tds->status.reg.uf)
	    [networkInterface incrementCollisions];
	    
        if (txNetbuf[txDoneIndex] != NULL) {
	    nb_free(txNetbuf[txDoneIndex]);
	    txNetbuf[txDoneIndex] = NULL;
	}
	// txDoneIndex = (txDoneIndex + 1) % DEC_21X40_TX_RING_LENGTH;
	if (++txDoneIndex == DEC_21X40_TX_RING_LENGTH)
	    txDoneIndex = 0;
	++txNumFree;
    }
    
    return YES;
}

#define DEC_21X40_SETUP_TIMEOUT 	10000

- (BOOL)_loadSetupFilter:(BOOL)pollingMode
{
    txDescriptorStruct 	*setupDescriptor;
    unsigned int	time = DEC_21X40_SETUP_TIMEOUT;
    csrRegUnion		reg;

//kprintf("[DECchip2104x _loadSetupFilter]\n");
    
    /*
     * Make sure that we have a descriptor for setup frame
     */
    if (txNumFree == 0)
	return NO;

    setupDescriptor = &txRing[txPutIndex];
    // txPutIndex = (txPutIndex + 1) % DEC_21X40_TX_RING_LENGTH;
    if (++txPutIndex == DEC_21X40_TX_RING_LENGTH)
	txPutIndex = 0;
    --txNumFree;

    /*
     * Initialize the Setup descriptor
     */
    if (setupDescriptor->control.reg.ter == YES)	{
	setupDescriptor->control.data = 0;
	setupDescriptor->control.reg.ter = YES;
    } else {
	setupDescriptor->control.data = 0;
    }
    
    setupDescriptor->control.reg.set = YES;
    setupDescriptor->control.reg.ic = YES;
    setupDescriptor->control.reg.byteCountBuffer1 = 
    		sizeof(setupBuffer_t);
    setupDescriptor->control.reg.byteCountBuffer2 = 0;

    setupDescriptor->bufferAddress1 = setupBufferPhysical;
    setupDescriptor->bufferAddress2 = 0;

    setupDescriptor->status.data = 0;
    setupDescriptor->status.reg.own = DEC_21X40_DESC_OWNED_BY_CTRL;

    /* tell the chip about this setup command */
    reg.data = 0;
    reg.csr1.tpd = YES;
    writeCsr(ioBase, DEC_21X40_CSR1, reg.data);
    
    if (pollingMode) {
	while (time--) {
	    IODelay(5);
	    reg.data = readCsr(ioBase, DEC_21X40_CSR5);
	    if (reg.csr5.tu) {
		writeCsr(ioBase, DEC_21X40_CSR5, reg.data);
		break;
	    }
	}

	// txDoneIndex = (txDoneIndex + 1) % DEC_21X40_TX_RING_LENGTH;
	if (++txDoneIndex == DEC_21X40_TX_RING_LENGTH)
	    txDoneIndex = 0;
	++txNumFree;
    }
    
    return YES;
}

- (BOOL)_setAddressFiltering:(BOOL)pollingMode 
{
    int			i;
    BOOL 		status = YES;				
    unsigned		q_count;	
    unsigned short	*address;

//kprintf("[DECchip2104x _setAddressFiltering]\n");

    /*
     * Copy station address into the first entry.  Note that the
     * upper 2 bytes of all longwords in the setup frame are don't
     * cares for perfect filtering.
     */
    address = (unsigned short *)&myAddress;
    for (i = 0 ; i < 3; i++) {
      unsigned short addr = (unsigned short)*address++;
      setupBuffer->physicalAddress[0][i] = (addr << 16);
    }

    /*
     * Load the broadcast address in the second entry
     */
    for (i = 0 ; i < 3; i++) {
         setupBuffer->physicalAddress[1][i] = 0xffffffff;
    }

    /*
     * Skip over the first 2 entries
     */
    q_count = 2;
        
    if (multicastEnabled) {
        enetMulti_t             *multiAddr;
        queue_head_t            *multiQueue = [super multicastQueue];

        if (!queue_empty(multiQueue)) {
            /*
             * Fill in the addresses
             */
            queue_iterate(multiQueue, multiAddr, enetMulti_t *, link) {
		union {
		      unsigned char	bytes[4];
		      unsigned long	word;
		} typeConvert; 

		typeConvert.word = 0;

	        for (i = 0; i < 3; i++) {
		    typeConvert.bytes[0] = multiAddr->address.ea_byte[i*2];
		    typeConvert.bytes[1] = multiAddr->address.ea_byte[i*2+1];
		    setupBuffer->physicalAddress[q_count][i] = 
			typeConvert.word;
	    	}
		if (++q_count >= DEC_21X40_SETUP_PERFECT_ENTRIES) {
		    IOLog("%s: %d multicast address limit exceeded\n",
		    	[self name], DEC_21X40_SETUP_PERFECT_MCAST_ENTRIES);
		    break;
		}
	    }
	}
    }
    
    /* 
     * Duplicate station address for remaining entries
     */
    for (; q_count < DEC_21X40_SETUP_PERFECT_ENTRIES; ++q_count) {
	bcopy(setupBuffer->physicalAddress[0], 
	    setupBuffer->physicalAddress[q_count], 
	    3 * sizeof (long));
    }

    if (![self _loadSetupFilter:pollingMode])
	status = NO;

    return status;
}


@end

