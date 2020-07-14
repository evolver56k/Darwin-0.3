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
 * for the Mace Ethernet controller. 
 *
 * HISTORY
 *
 * 10-Sept-97		 
 *	Created.
 *
 */
#import "MaceEnetPrivate.h"
#import <mach/vm_param.h>			// PAGE_SIZE

extern 	void 			*kernel_map;
extern 	kern_return_t		kmem_alloc_wired();

static IODBDMADescriptor       	dbdmaCmd_Nop;
static IODBDMADescriptor    	dbdmaCmd_NopWInt;
static IODBDMADescriptor    	dbdmaCmd_LoadXFS;
static IODBDMADescriptor        dbdmaCmd_LoadIntwInt;		
static IODBDMADescriptor       	dbdmaCmd_Stop;
static IODBDMADescriptor       	dbdmaCmd_Branch;


static u_int8_t reverseBitOrder(u_int8_t data )
{
    u_int8_t		val = 0;
    int			i;

    for ( i=0; i < 8; i++ )
    {
      val <<= 1;
      if (data & 1) val |= 1;
      data >>= 1;
    }
    return( val );
}      
    

@implementation MaceEnet(Private)


/*
 * Private functions
 */
- (BOOL)_allocateMemory
{
    u_int32_t			dbdmaSize;
    u_int32_t			i, n;
    unsigned char *		virtAddr;
    u_int32_t			physBase;
    u_int32_t	 		physAddr;
    IOReturn   			kr;
 
    /* 
     * Calculate total space for DMA channel commands
     */
    dbdmaSize = round_page( RX_RING_LENGTH * sizeof(enet_dma_cmd_t) + TX_RING_LENGTH * sizeof(enet_txdma_cmd_t) 
                                                                              + 2 * sizeof(IODBDMADescriptor) );
    /*
     * Allocate required memory
     */
    if ( !dmaCommands )
    {
      kr = kmem_alloc_wired(kernel_map, (vm_offset_t *) &dmaCommands, dbdmaSize );

      if ( kr != KERN_SUCCESS  )
      {
          IOLog( "Ethernet(Mace): Cant allocate channel DBDMA commands\n\r" );
          return NO;
      }
    }

    /*
     * If we needed more than one page, then make sure we received contiguous memory.
     */
    n = (dbdmaSize - PAGE_SIZE) / PAGE_SIZE;
    IOPhysicalFromVirtual( (vm_task_t) kernel_map, (vm_address_t) dmaCommands, &physBase );

    virtAddr = (unsigned char *) dmaCommands;
    for( i=0; i < n; i++, virtAddr += PAGE_SIZE )
    {
       IOPhysicalFromVirtual( (vm_task_t) kernel_map, (vm_address_t) virtAddr, &physAddr );
       if (physAddr != (physBase + i * PAGE_SIZE) )
       {
         IOLog( "Ethernet(Mace): Cant allocate contiguous memory for DBDMA commands\n\r" );
         return NO;
       }
    }           

    /* 
     * Setup the receive ring pointers
     */
    rxDMACommands = (enet_dma_cmd_t *)dmaCommands;
    rxMaxCommand  = RX_RING_LENGTH;

    /*
     * Setup the transmit ring pointers
     */
    txDMACommands = (enet_txdma_cmd_t *)(dmaCommands + RX_RING_LENGTH * sizeof(enet_dma_cmd_t) + sizeof(IODBDMADescriptor));
    txMaxCommand  = TX_RING_LENGTH;


    /*
     * Setup pre-initialized DBDMA commands 
     */
    IOMakeDBDMADescriptor( (&dbdmaCmd_Nop),
                            kdbdmaNop,
		 	    kdbdmaKeyStream0,
			    kdbdmaIntNever,
			    kdbdmaBranchNever,
			    kdbdmaWaitNever,
                            0,
                            0          );

    IOMakeDBDMADescriptor( (&dbdmaCmd_NopWInt),
                            kdbdmaNop,
		 	    kdbdmaKeyStream0,
			    kdbdmaIntAlways,
			    kdbdmaBranchNever,
			    kdbdmaWaitNever,
                            0,
                            0          );

    IOMakeDBDMADescriptor( (&dbdmaCmd_LoadXFS),
                            kdbdmaLoadQuad,
		 	    kdbdmaKeySystem,
			    kdbdmaIntNever,
			    kdbdmaBranchNever,
			    kdbdmaWaitNever,
                            1,
                            ((int)ioBaseEnet +  kXmtFS)   );

    IOMakeDBDMADescriptor( (&dbdmaCmd_LoadIntwInt),
                            kdbdmaLoadQuad,
		 	    kdbdmaKeySystem,
			    kdbdmaIntAlways,
			    kdbdmaBranchNever,
			    kdbdmaWaitNever,
                            1,
                            ((int)ioBaseEnet +  kIntReg)   );

    IOMakeDBDMADescriptor( (&dbdmaCmd_Stop),
                            kdbdmaStop,
		 	    kdbdmaKeyStream0,
			    kdbdmaIntNever,
			    kdbdmaBranchNever,
			    kdbdmaWaitNever,
                            0,
                            0          );

    IOMakeDBDMADescriptor( (&dbdmaCmd_Branch),
                            kdbdmaNop,
		 	    kdbdmaKeyStream0,
			    kdbdmaIntNever,
			    kdbdmaBranchAlways,
			    kdbdmaWaitNever,
                            0,
                            0          );

    return YES;
}

/*-------------------------------------------------------------------------
 *
 * Setup the Transmit Ring
 * -----------------------
 * Each transmit ring entry consists of two words to transmit data from buffer
 * segments (possibly) spanning a page boundary. This is followed by two DMA commands 
 * which read transmit frame status and interrupt status from the Mace chip. The last
 * DMA command in each transmit ring entry generates a host interrupt.
 * The last entry in the ring is followed by a DMA branch to the first
 * entry.
 *-------------------------------------------------------------------------*/

- (BOOL)_initTxRing
{
    BOOL			kr;
    u_int32_t			i;

    /*
     * Clear the transmit DMA command memory
     */
    bzero( (void *)txDMACommands, sizeof(enet_txdma_cmd_t) * txMaxCommand);
    txCommandHead = 0;
    txCommandTail = 0;

    /* 
     * Init the transmit queue. 
     */
    if (transmitQueue)	
    {
      [transmitQueue free];
    }

    transmitQueue = [[IONetbufQueue alloc] initWithMaxCount:TRANSMIT_QUEUE_SIZE];
    if (!transmitQueue)	
    {
      IOLog("Ethernet(Mace): Cant allocate transmit queue\n\r");
      return NO;
    }

    /*
     * DMA Channel commands 2,3 are the same for all DBDMA entries on transmit. 
     * Initialize them now.
     */
    for( i=0; i < txMaxCommand; i++ )
    {
      txDMACommands[i].desc_seg[2] = dbdmaCmd_LoadXFS;
      txDMACommands[i].desc_seg[3] = dbdmaCmd_LoadIntwInt;
    }

    /* 
     * Put a DMA Branch command after the last entry in the transmit ring. Set the branch address
     * to the physical address of the start of the transmit ring.
     */
    txDMACommands[txMaxCommand].desc_seg[0] = dbdmaCmd_Branch; 

    kr = IOPhysicalFromVirtual( (vm_task_t) IOVmTaskSelf(), (vm_address_t) txDMACommands, (u_int32_t *)&txDMACommandsPhys );
    if ( kr != IO_R_SUCCESS )
    {
      IOLog( "Ethernet(Mace): Bad Tx DBDMA command buf - %08x\n\r", (u_int32_t)txDMACommands );
    }
    IOSetCCCmdDep( &txDMACommands[txMaxCommand].desc_seg[0], txDMACommandsPhys );

    /* 
     * Set the Transmit DMA Channel pointer to the first entry in the transmit ring.
     */
    IOSetDBDMACommandPtr( ioBaseEnetTxDMA, txDMACommandsPhys );

    /*
     * Push the DMA channel words into physical memory.
     */
    flush_cache_v( (vm_offset_t)txDMACommands, txMaxCommand*sizeof(enet_txdma_cmd_t) + sizeof(IODBDMADescriptor));

    return YES;
}

/*-------------------------------------------------------------------------
 *
 * Setup the Receive ring
 * ----------------------
 * Each receive ring entry consists of two DMA commands to receive data
 * into a network buffer (possibly) spanning a page boundary. The second
 * DMA command in each entry generates a host interrupt.
 * The last entry in the ring is followed by a DMA branch to the first
 * entry. 
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_initRxRing
{
    u_int32_t	 		i;
    BOOL			status;
    IOReturn    		kr;
    
    /*
     * Clear the receive DMA command memory
     */
    bzero( (void *)rxDMACommands, sizeof(enet_dma_cmd_t) * rxMaxCommand);

    kr = IOPhysicalFromVirtual( (vm_task_t) IOVmTaskSelf(), (vm_address_t) rxDMACommands, (u_int32_t *)&rxDMACommandsPhys );
    if ( kr != IO_R_SUCCESS )
    {
      IOLog( "Ethernet(Mace): Bad Rx DBDMA command buf - %08x\n\r",  (u_int32_t)rxDMACommands );
      return NO;
    }

    /*
     * Allocate a receive buffer for each entry in the Receive ring
     */
    for (i = 0; i < rxMaxCommand-1; i++) 
    {
      if (rxNetbuf[i] == NULL)	
      {
        rxNetbuf[i] = [self allocateNetbuf];
        if (rxNetbuf[i] == NULL)	
	{
          IOLog("Ethernet(Mace): allocateNetbuf returned NULL in _initRxRing\n\r");
          return NO;
	}
      }
      /* 
       * Set the DMA commands for the ring entry to transfer data to the NetBuf.
       */
      status = [self _updateDescriptorFromNetBuf:rxNetbuf[i]  Desc:(void *)&rxDMACommands[i]  ReceiveFlag:YES ];
      if (status == NO)
      {    
        IOLog("Ethernet(Mace): Cant map Netbuf to physical memory in _initRxRing\n\r");
        return NO;
      }
    }
    
    /*
     * Set the receive queue head to point to the first entry in the ring. Set the
     * receive queue tail to point to a DMA Stop command after the last ring entry
     */
    rxCommandHead = 0;
    rxCommandTail = i;

    rxDMACommands[i].desc_seg[0] = dbdmaCmd_Stop; 
    rxDMACommands[i].desc_seg[1] = dbdmaCmd_Nop;

    /*
     * Setup a DMA branch command after the stop command
     */
    i++;
    rxDMACommands[i].desc_seg[0] = dbdmaCmd_Branch; 

    IOSetCCCmdDep( &rxDMACommands[i].desc_seg[0], rxDMACommandsPhys );

    /*
     * Set DMA command pointer to first receive entry
     */ 
    IOSetDBDMACommandPtr( ioBaseEnetRxDMA, rxDMACommandsPhys );

    /*
     * Push DMA commands to physical memory
     */
    flush_cache_v( (vm_offset_t)&rxDMACommands[rxCommandTail], 2 * sizeof(enet_dma_cmd_t) );

    return YES;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)_startChip
{
    WriteMaceRegister( ioBaseEnet, kMacCC, kMacCCEnXmt | kMacCCEnRcv );

    // enable rx dma channel
    IODBDMAContinue( ioBaseEnetRxDMA );

}



/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)_resetChip
{
    u_int8_t			regValue;

    /*
     * Mace errata - chip reset does not clear pending interrupts
     */
    ReadMaceRegister( ioBaseEnet, kIntReg );

    IODBDMAReset( ioBaseEnetRxDMA );  
    IODBDMAReset( ioBaseEnetTxDMA );  

    IOSetDBDMAWaitSelect(      ioBaseEnetTxDMA, IOSetDBDMAChannelControlBits( kdbdmaS5 ) );

    IOSetDBDMABranchSelect(    ioBaseEnetRxDMA, IOSetDBDMAChannelControlBits( kdbdmaS6 ) );
    IOSetDBDMAInterruptSelect( ioBaseEnetRxDMA, IOSetDBDMAChannelControlBits( kdbdmaS6 ) );

    WriteMaceRegister( ioBaseEnet, kBIUCC, kBIUCCSWRst );
    do
    {
      regValue = ReadMaceRegister( ioBaseEnet, kBIUCC );
    }
    while( regValue & kBIUCCSWRst );
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_initChip
{
    volatile u_int16_t		regValue;
    u_int32_t			i;

    [self _disableAdapterInterrupts];

    chipId  = ReadMaceRegister( ioBaseEnet, kMaceChipId0 );
    chipId |= ReadMaceRegister( ioBaseEnet, kMaceChipId1 ) << 8;

    /*
     * Turn off ethernet header stripping
     */
    regValue  = ReadMaceRegister( ioBaseEnet, kRcvFC );
    regValue &= ~kRcvFCAStrpRcv;
    WriteMaceRegister( ioBaseEnet, kRcvFC, regValue );

    /*
     * Set Mace destination address. 
     */
    if ( chipId != kMaceRevisionA2 )
    { 
      WriteMaceRegister( ioBaseEnet, kIAC, kIACAddrChg | kIACPhyAddr );
      do
      {
        regValue = ReadMaceRegister( ioBaseEnet, kIAC );
      }
      while( regValue & kIACAddrChg );
    }
    else
    {
      WriteMaceRegister( ioBaseEnet, kIAC, kIACPhyAddr );
    }

    for (i=0; i < sizeof(enet_addr_t); i++ )
    {
      WriteMaceRegister( ioBaseEnet, kPADR, reverseBitOrder(((unsigned char *)ioBaseEnetROM)[i<<4]) );
    }

    /*
     * Clear logical address (multicast) filter
     */
    if ( chipId != kMaceRevisionA2 )
    { 
      WriteMaceRegister( ioBaseEnet, kIAC, kIACAddrChg | kIACLogAddr );
      do
      {
        regValue = ReadMaceRegister( ioBaseEnet, kIAC );
      }
      while( regValue & kIACAddrChg );
    }
    else
    {
      WriteMaceRegister( ioBaseEnet, kIAC, kIACLogAddr );
    }

    for (i = 0; i < 8; i++ )
    {
      WriteMaceRegister( ioBaseEnet, kLADRF, 0 );
    }

    /* 
     * Enable ethernet transceiver 
     */
    WriteMaceRegister( ioBaseEnet, kPLSCC, kPLSCCPortSelGPSI | kPLSCCEnSts );

    return YES;
}


/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _restartChip
{
    /*
     * Shutdown DMA channels
     */
    [self _stopReceiveDMA];
    [self _stopTransmitDMA];

    /*
     * Get the silicon's attention
     */
    [self _resetChip];
    [self _initChip];

    /*
     * Restore multicast settings
     */
    [self _updateHashTableMask];

    if ( isPromiscuous )
    {
      [self enablePromiscuousMode];
    }

    /*
     * Enable receiver and transmitter
     */
    [self _startChip];
    [self _enableAdapterInterrupts]; 

    /*
     * Restart transmit DMA
     */
    IODBDMAContinue( ioBaseEnetTxDMA );  

}

/*-------------------------------------------------------------------------
 *
 * Orderly stop of receive DMA.
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _stopReceiveDMA
{
    u_int32_t		dmaStatus;
    u_int32_t		dmaCmdPtr;
    u_int32_t		dmaIndex;
    u_int8_t		tmpBuf[16];
    u_int8_t		*p = 0;
    u_int8_t		MacCCReg;

    /* 
     * Stop the receiver and allow any frame receive in progress to complete
     */
    MacCCReg = ReadMaceRegister( ioBaseEnet, kMacCC );
    WriteMaceRegister( ioBaseEnet, kMacCC, MacCCReg & ~kMacCCEnRcv );
    IODelay( RECEIVE_QUIESCE_uS * 1000 );

    /* 
     * Capture channel status and pause the dma channel.
     */
    dmaStatus = IOGetDBDMAChannelStatus( ioBaseEnetRxDMA );
    IODBDMAPause( ioBaseEnetRxDMA );

    /*
     * Read the command pointer and convert it to a byte offset into the DMA program.
     */
    dmaCmdPtr = IOGetDBDMACommandPtr( ioBaseEnetRxDMA );
    dmaIndex  = (dmaCmdPtr - rxDMACommandsPhys);

    /*
     * If the channel status is DEAD, the DMA pointer is pointing to the next command
     */
    if ( dmaStatus & kdbdmaDead )
    {
      dmaIndex -= sizeof(IODBDMADescriptor);
    }

    /*
     * Convert channel program offset to command index
     */
    dmaIndex = dmaIndex / sizeof(enet_dma_cmd_t);
    if ( dmaIndex >= rxMaxCommand ) dmaIndex = 0;
      
    /*
     * The DMA controller doesnt like being stopped before transferring any data. 
     *
     * When we do so it pollutes up to 16-bytes aligned to the nearest (lower) 16-byte
     * boundary. This corruption can be outside the data transfer area of the Netbuf, so we
     * capture and then restore these bytes after stopping the channel. 
     *
     */
    if ( rxNetbuf[dmaIndex] )
    {
      p = (u_int8_t *)nb_map( rxNetbuf[dmaIndex] );
    }

    (u_int32_t)p &= ~0x0f;

    if ( p )
    {
      bcopy( p, tmpBuf, 16 );
    }

    IODBDMAReset( ioBaseEnetRxDMA );

    if ( p )
    {
      bcopy( tmpBuf, p, 16 );
    }

    /*
     * Reset the dma channel pointer to the nearest command index
     */
    dmaCmdPtr = rxDMACommandsPhys + sizeof(enet_dma_cmd_t) * dmaIndex;
    IOSetDBDMACommandPtr( ioBaseEnetRxDMA, dmaCmdPtr);

}    

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _stopTransmitDMA
{
    u_int32_t		dmaStatus;
    u_int32_t		dmaCmdPtr;
    u_int32_t		dmaIndex;
    u_int8_t		MacCCReg;

    /* 
     * Stop the transmitter and allow any frame transmit in progress to abort
     */
    MacCCReg = ReadMaceRegister( ioBaseEnet, kMacCC );
    WriteMaceRegister( ioBaseEnet, kMacCC, MacCCReg & ~kMacCCEnXmt );
    IODelay( TRANSMIT_QUIESCE_uS * 1000 );

    /* 
     * Capture channel status and pause the dma channel.
     */
    dmaStatus = IOGetDBDMAChannelStatus( ioBaseEnetTxDMA );
    IODBDMAPause( ioBaseEnetTxDMA );

    /*
     * Read the command pointer and convert it to a byte offset into the DMA program.
     */
    dmaCmdPtr = IOGetDBDMACommandPtr( ioBaseEnetTxDMA );
    dmaIndex  = (dmaCmdPtr - txDMACommandsPhys);

    /*
     * If the channel status is DEAD, the DMA pointer is pointing to the next command
     */
    if ( dmaStatus & kdbdmaDead )
    {
      dmaIndex -= sizeof(IODBDMADescriptor);
    }
 
    /*
     * Convert channel program offset to command index
     */
    dmaIndex = dmaIndex / sizeof(enet_txdma_cmd_t);
    if ( dmaIndex >= txMaxCommand ) dmaIndex = 0;

    IODBDMAReset( ioBaseEnetTxDMA );

    /*
     * Reset the dma channel pointer to the nearest command index
     */
    dmaCmdPtr = txDMACommandsPhys + sizeof(enet_txdma_cmd_t) * dmaIndex;
    IOSetDBDMACommandPtr( ioBaseEnetTxDMA, dmaCmdPtr );
}

   

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _disableAdapterInterrupts
{
    WriteMaceRegister( ioBaseEnet, kIntMask, 0xFF );
}

/*-------------------------------------------------------------------------
 *
 * _enableAdapterInterrupts
 *
 * It appears to make the Mace chip work properly with the DBDMA channel
 * we need to leave the transmit interrupt unmasked at the chip. This
 * is weird, but that's what happens when you try to glue a chip that
 * wasn't intended to work with a DMA engine on to a DMA. 
 *
 *-------------------------------------------------------------------------*/

- (void) _enableAdapterInterrupts
{
    u_int8_t		regValue;

    regValue = ReadMaceRegister( ioBaseEnet, kIntMask );
    regValue &= ~kIntMaskXmtInt;
    WriteMaceRegister( ioBaseEnet, kIntMask, regValue );
    IODelay(500); 
    ReadMaceRegister( ioBaseEnet, kXmtFS );
    ReadMaceRegister( ioBaseEnet, kIntReg );
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_transmitPacket:(netbuf_t)packet
{
    enet_dma_cmd_t	tmpCommand;
    u_int32_t		i;
  

    /* 
     * Not sure we need this. Only required if Mace disables receiption of transmit packets 
     * addressed to itself.
     */
    [self performLoopback:packet];

    /*
     * Check for room on the transmit ring. There should always be space since it is
     * the responsibility of the caller to verify this before calling _transmitPacket.
     */
    i = txCommandTail + 1;
    if ( i >= txMaxCommand ) i = 0;
    if ( i == txCommandHead )
    {
	IOLog("Ethernet(Mace): Freeing transmit packet eh?\n\r");
	nb_free(packet);
	return NO;
    }

    /*
     * txCommandTail points to the current DMA Stop command for the channel. We are
     * now creating a new DMA Stop command in the next slot in the transmit ring. The 
     * previous DMA Stop command will be overwritten with the DMA commands to 
     * transfer the new NetBuf.
     */
    txDMACommands[i].desc_seg[0] = dbdmaCmd_Stop;
    txDMACommands[i].desc_seg[1] = dbdmaCmd_Nop;

    flush_cache_v( (vm_offset_t)&txDMACommands[i], sizeof(enet_dma_cmd_t) );

    /*
     * Get a copy of the DMA transfer commands in a temporary buffer. 
     * The new DMA command is written into the channel program so that the command
     * word for the old Stop command is overwritten last. This prevents the DMA
     * engine from executing a partially written channel command.
     */
    [self _updateDescriptorFromNetBuf:packet Desc:&tmpCommand ReceiveFlag:NO];

    bcopy( ((u_int32_t *)&tmpCommand)+1,
           ((u_int32_t *)&txDMACommands[txCommandTail])+1,
           sizeof(enet_dma_cmd_t)-sizeof(u_int32_t) );

    flush_cache_v( (vm_offset_t)&txDMACommands[txCommandTail], sizeof(enet_dma_cmd_t) );

    txNetbuf[txCommandTail] = packet;
    txDMACommands[txCommandTail].desc_seg[0].operation = tmpCommand.desc_seg[0].operation;

    flush_cache_v( (vm_offset_t)&txDMACommands[txCommandTail], sizeof(enet_dma_cmd_t) );

    /*
     * Set the transmit tail to the new stop command.
     */
    txCommandTail = i;

    /*
     * Tap the DMA channel to wake it up
     */
    IODBDMAContinue( ioBaseEnetTxDMA );

    return YES;
}	

/*-------------------------------------------------------------------------
 * _receivePacket
 * --------------
 * This routine runs the receiver in polled-mode (yuk!) for the kernel debugger.
 *
 * The _receivePackets allocate NetBufs and pass them up the stack. The kernel
 * debugger interface passes a buffer into us. To reconsile the two interfaces,
 * we allow the receive routine to continue to allocate its own buffers and
 * transfer any received data to the passed-in buffer. This is handled by 
 * _receivePacket calling _packetToDebugger.
 *-------------------------------------------------------------------------*/

- (void)_receivePacket:(void *)pkt length:(unsigned int *)pkt_len  timeout:(unsigned int)timeout
{
    ns_time_t		startTime;
    ns_time_t		currentTime;
    u_int32_t		elapsedTimeMS;

    *pkt_len = 0;

    if (resetAndEnabled == NO)
      return;

    debuggerPkt     = pkt;
    debuggerPktSize = 0;

    IOGetTimestamp(&startTime);
    do
    {
      [self _receivePackets:YES];
      IOGetTimestamp(&currentTime);
      elapsedTimeMS = (currentTime - startTime) / (1000*1000);
    } 
    while ( (debuggerPktSize == 0) && (elapsedTimeMS < timeout) );

    *pkt_len = debuggerPktSize;

    return;
}

/*-------------------------------------------------------------------------
 * _packetToDebugger
 * -----------------
 * This is called by _receivePackets when we are polling for kernel debugger
 * packets. It copies the NetBuf contents to the buffer passed by the debugger.
 * It also sets the var debuggerPktSize which will break the polling loop.
 *-------------------------------------------------------------------------*/

-(void) _packetToDebugger: (netbuf_t) packet
{
    debuggerPktSize = nb_size(packet);
    bcopy( nb_map(packet), debuggerPkt, debuggerPktSize );
  
    nb_free( packet );
}

/*-------------------------------------------------------------------------
 * _sendPacket
 * -----------
 *
 * This routine runs the transmitter in polled-mode (yuk!) for the kernel debugger.
 *
 *-------------------------------------------------------------------------*/

- (void)_sendPacket:(void *)pkt length:(unsigned int)pkt_len
{
    ns_time_t		startTime;
    ns_time_t		currentTime;
    u_int32_t		elapsedTimeMS;
    int			size;

    if (resetAndEnabled == NO)
      return; 

    /*
     * Wait for the transmit ring to empty
     */
    IOGetTimestamp(&startTime); 
    do
    {	
      [self _transmitInterruptOccurred];
      IOGetTimestamp(&currentTime);
      elapsedTimeMS = (currentTime - startTime) / (1000*1000);
    }
    while ( (txCommandHead != txCommandTail) && (elapsedTimeMS < TX_KDB_TIMEOUT) ); 
	
    if ( txCommandHead != txCommandTail )
    {
      IOLog( "Ethernet(Mace): Polled tranmit timeout - 1\n\r");
      return;
    }

    /*
     * Allocate a NetBuf and copy the debugger transmit data into it
     */
    debuggerPkt = [self allocateNetbuf];
    bcopy(pkt, nb_map(debuggerPkt), pkt_len);
    size = nb_size(debuggerPkt);
    nb_shrink_bot(debuggerPkt, size - pkt_len);

    /*
     * Send the debugger packet. _transmitPacket will free the Netbuf we allocated
     * above.
     */
    [self _transmitPacket: debuggerPkt];

    /*
     * Poll waiting for the transmit ring to empty again
     */ 
    do 
    {
      [self _transmitInterruptOccurred];
      IOGetTimestamp(&currentTime);
      elapsedTimeMS = (currentTime - startTime) / (1000*1000);
    }
    while ( (txCommandHead != txCommandTail) && (elapsedTimeMS < TX_KDB_TIMEOUT) ); 

    if ( txCommandHead != txCommandTail )
    {
      IOLog( "Ethernet(Mace): Polled tranmit timeout - 2\n\r");
      return;
    }

    return;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_receiveInterruptOccurred
{
  [self _receivePackets:NO];

  return YES;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_receivePackets:(BOOL)fDebugger
{
    enet_dma_cmd_t      tmpCommand;
    netbuf_t		packet, newPacket;
    u_int32_t           i,j,last;
    u_int32_t		dmaChnlStatus;
    int			receivedFrameSize = 0;
    u_int32_t           dmaCount[2], dmaResid[2], dmaStatus[2];
    BOOL		passPacketUp;
    BOOL		reusePkt;
    BOOL		status;
    u_int8_t		*rxFS = NULL;	   
    u_int32_t		nextDesc; 


    last      = -1;  
    i         = rxCommandHead;

    while ( 1 )
    {
      passPacketUp = NO;
      reusePkt     = NO;

      /* 
       * Purge cache references for the DBDMA entry we are about to look at.
       */
      invalidate_cache_v((vm_offset_t)&rxDMACommands[i], sizeof(enet_dma_cmd_t));

      /*
       * Collect the DMA residual counts/status for the two buffer segments.
       */ 
      for ( j = 0; j < 2; j++ )
      {
        dmaResid[j]   = IOGetCCResult( &rxDMACommands[i].desc_seg[j] );
        dmaStatus[j]  = dmaResid[j] >> 16;
        dmaResid[j]  &= 0x0000ffff;
        dmaCount[j]   = IOGetCCOperation( &rxDMACommands[i].desc_seg[j] ) & kdbdmaReqCountMask;
      }

#if 0
      IOLog("Ethernet(Mace): Rx NetBuf[%2d] = %08x Resid[0] = %04x Status[0] = %04x Resid[1] = %04x Status[1] = %04x\n\r",
                i, (int)nb_map(rxNetbuf[i]), dmaResid[0], dmaStatus[0], dmaResid[1], dmaStatus[1] );      
#endif 
      /* 
       * If the current entry has not been written, then stop at this entry
       */
      if (  !((dmaStatus[0] & kdbdmaBt) || (dmaStatus[1] & kdbdmaActive)) )
      {
        break;
      }

      /*
       * The Mace Ethernet controller appends four bytes to each receive buffer containing the buffer
       * size and receive frame status.
       * We locate these bytes by using the DMA residual counts.
       */ 
      receivedFrameSize = dmaCount[0] - dmaResid[0] + dmaCount[1] - ((dmaStatus[0] & kdbdmaBt) ? dmaCount[1] : dmaResid[1]);

      if ( receivedFrameSize >= 4 )
      {
        /*
         * Get the receive frame size as reported by the Mace controller
         */
        rxFS = (u_int8_t *)nb_map(rxNetbuf[i]) + receivedFrameSize - 4;
        receivedFrameSize =  (u_int16_t) rxFS[0] | (rxFS[1] & kRcvFS1RcvCnt) << 8;
      }

      /*
       * Reject packets that are runts or that have other mutations.
       */
      if ( receivedFrameSize < (ETHERMINPACKET - ETHERCRC) || 
                   receivedFrameSize > (ETHERMAXPACKET + ETHERCRC) || 
                       (rxFS[1] & (kRcvFS1OFlo | kRcvFS1Clsn | kRcvFS1Fram | kRcvFS1FCS)) )
      {
        [networkInterface incrementInputErrors];
        reusePkt = YES;
      }      
       
      packet = rxNetbuf[i];
   
      /*
       * If we are not accepting all packets, then check for multicast packets that got past the
       * logical address filter but were not wanted.
       */
      if ( !isPromiscuous && [super isUnwantedMulticastPacket: (ether_header_t *)nb_map(packet)] == YES )
      {
        reusePkt = YES;
      } 
 
      /*
       * Before we pass this packet up the networking stack. Make sure we can get a replacement. 
       * Otherwise, hold on to the current packet and increment the input error count.
       * Thanks Justin!
       */
      if ( reusePkt == NO )
      { 
        newPacket = [self allocateNetbuf];
        if ( newPacket == NULL )
        {
          reusePkt = YES;
          [networkInterface incrementInputErrors];
        }
      }           

      /*
       * Install the new Netbuf for the one we're about to pass to the network stack
       */
      if ( reusePkt == NO )	
      {    
        rxNetbuf[i] = newPacket;
        passPacketUp = YES;
        status = [self _updateDescriptorFromNetBuf:rxNetbuf[i] Desc:(void *)&rxDMACommands[i] ReceiveFlag:YES] ;
        if (status == NO) 
        {
          IOLog("Ethernet(Mace): _updateDescriptorFromNetBuf failed for receive\n"); 
        }
        /*  Adjust to size received */
        nb_shrink_bot(packet, nb_size(packet) - receivedFrameSize);
      }
      /*
       * If we are reusing the existing Netbuf, then refurbish the existing DMA command \
       * descriptors by clearing the status/residual count fields.
       */
      else
      {
        for ( j=0; j < sizeof(enet_dma_cmd_t)/sizeof(IODBDMADescriptor); j++ )
        {
          IOSetCCResult( &rxDMACommands[i].desc_seg[j], 0 );
        }
        flush_cache_v( (vm_offset_t)&rxDMACommands[i], sizeof(enet_dma_cmd_t) );
      }
    
      /*
       * Keep track of the last receive descriptor processed
       */
      last = i;

      /*
       * Implement ring wrap-around
       */
      if (++i >= rxMaxCommand) i = 0;
        
      /*
       * Transfer received packet to debugger or network
       */
      if (passPacketUp) 
      {
        if ( fDebugger == NO )
        {
//        IOLog("Ethernet(Mace): Packet from network - %08x %d\n", (int) packet, (int)nb_size(packet) );
          KERNEL_DEBUG(DBG_MACE_RXCOMPLETE | DBG_FUNC_NONE, (int) packet, (int)nb_size(packet), 0, 0, 0 );
          [networkInterface handleInputPacket:packet extra:0];
        }
        else
        { 
//        IOLog("Ethernet(Mace): Packet to debugger - %08x %d\n", (int) packet, (int)nb_size(packet) );
          [self _packetToDebugger: packet];
          break;
        }
      }
    }

    /*
     * OK...this is a little messy
     *
     * We just processed a bunch of DMA receive descriptors. We are going to exchange the
     * current DMA stop command (rxCommandTail) with the last receive descriptor we
     * processed (last). This will make these list of descriptors we just processed available. 
     * If we processed no receive descriptors on this call then skip this exchange.
     */
   
#if 0
    IOLog( "Ethernet(Mace): Prev - Rx Head = %2d Rx Tail = %2d Rx Last = %2d\n\r", rxCommandHead, rxCommandTail, last );
#endif

    if ( last != -1 )
    {
      /*
       * Save the contents of the last receive descriptor processed.
       */
      newPacket              		= rxNetbuf[last];
      tmpCommand          		= rxDMACommands[last];

      /*
       * Write a DMA stop command into this descriptor slot
       */
      rxDMACommands[last].desc_seg[0] 	= dbdmaCmd_Stop;
      rxDMACommands[last].desc_seg[1]   = dbdmaCmd_Nop;  
      rxNetbuf[last]      = 0;

      flush_cache_v( (vm_offset_t)&rxDMACommands[last], sizeof(enet_dma_cmd_t) );


      /*
       * Replace the previous DMA stop command with the last receive descriptor processed.
       * 
       * The new DMA command is written into the channel program so that the command
       * word for the old Stop command is overwritten last. This prevents the DMA
       * engine from executing a partially written channel command.
       * 
       * Note: When relocating the descriptor, we must update its branch field to reflect its
       *       new location.
       */
      nextDesc = rxDMACommandsPhys + (int)&rxDMACommands[rxCommandTail+1] - (int)rxDMACommands;
      IOSetCCCmdDep( &tmpCommand.desc_seg[0], nextDesc );

      bcopy( (u_int32_t *)&tmpCommand+1,
             (u_int32_t *)&rxDMACommands[rxCommandTail]+1,
             sizeof(enet_dma_cmd_t)-sizeof(u_int32_t) );

      flush_cache_v( (vm_offset_t)&rxDMACommands[rxCommandTail], sizeof(enet_dma_cmd_t) );


      rxNetbuf[rxCommandTail] = newPacket;

      rxDMACommands[rxCommandTail].desc_seg[0].operation = tmpCommand.desc_seg[0].operation;

      flush_cache_v( (vm_offset_t)&rxDMACommands[rxCommandTail], sizeof(IODBDMADescriptor) );

      /*
       * Update rxCommmandTail to point to the new Stop command. Update rxCommandHead to point to 
       * the next slot in the ring past the Stop command 
       */
      rxCommandTail = last;
      rxCommandHead = i;
    }

    /*
     * The DMA channel has a nasty habit of shutting down when there is a non-recoverable error
     * on receive. We get no interrupt for this since the channel shuts down before the 
     * descriptor that causes the host interrupt is executed.
     * 
     * We check if the channel is DEAD by checking the channel status reg. Also, the watchdog 
     * timer can force receiver interrupt servicing based on detecting that the receive DMA is DEAD.
     */
    dmaChnlStatus = IOGetDBDMAChannelStatus( ioBaseEnetRxDMA );
    if ( dmaChnlStatus & kdbdmaDead )
    {
      /*
       * Read log error
       */
      [networkInterface incrementInputErrors]; 
      IOLog( "Ethernet(Mace): Rx DMA Error - Status = %04x\n", dmaChnlStatus );
  
      /*
       * Reset and reinitialize chip
       */
      [self _restartChip];
    }   
    else
    {
      /*
       * Tap the DMA to wake it up
       */
      IODBDMAContinue( ioBaseEnetRxDMA );
    }

#if 0
    IOLog( "Ethernet(Mace): New  - Rx Head = %2d Rx Tail = %2d\n\r", rxCommandHead, rxCommandTail );
#endif

    return YES;
}
 
/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)_transmitInterruptOccurred
{
    u_int32_t			dmaStatus;
    u_int32_t           	xmtFS;

    while ( 1 )
    {
      /* 
       * Purge cache references for the DBDMA entry we are about to look at.
       */
      invalidate_cache_v((vm_offset_t)&txDMACommands[txCommandHead], sizeof(enet_txdma_cmd_t));

      /*
       * Check the status of the last descriptor in this entry to see if the DMA engine completed
       * this entry.
       */
      dmaStatus = IOGetCCResult( &txDMACommands[txCommandHead].desc_seg[3] ) >> 16;
      if ( !(dmaStatus & kdbdmaActive) )
      {
        break;
      }

      /* 
       * Reset the status word for the entry we are about to process
       */     
      IOSetCCResult( &txDMACommands[txCommandHead].desc_seg[3], 0 );
      flush_cache_v( (vm_offset_t) &txDMACommands[txCommandHead].desc_seg[3], sizeof(IODBDMADescriptor) );


      /*
       * This DMA descriptor read the transmit frame status. See what it has to tell us.
       */
      xmtFS = IOGetCCCmdDep( &txDMACommands[txCommandHead].desc_seg[2] );
      if ( xmtFS & kXmtFSXmtSV )
      {
        if (xmtFS & (kXmtFSUFlo | kXmtFSLCol | kXmtFSRtry | kXmtFSLCar) )
        {
          [networkInterface incrementOutputErrors];
        }
        else
        {
          [networkInterface incrementOutputPackets];
        }

        if (xmtFS & kXmtFSOne )
        {
           [networkInterface incrementCollisions];
        }
        if ( xmtFS & kXmtFSMore )
        {
          [networkInterface incrementCollisions];
        }
      }  

      /*
       * Free the Netbuf we just transmitted
       */
      KERNEL_DEBUG(DBG_MACE_TXCOMPLETE | DBG_FUNC_NONE, (int) txNetbuf[txCommandHead], (int) nb_size(txNetbuf[txCommandHead]), 0, 0, 0 );
      nb_free( txNetbuf[txCommandHead] );
      txNetbuf[txCommandHead] = NULL;

      if ( ++txCommandHead >= txMaxCommand ) txCommandHead = 0;

    }

    /*
     * The DMA channel has a nasty habit of shutting down when there is non-recoverable error
     * on transmit. We get no interrupt for this since the channel shuts down before the 
     * descriptor that causes the host interrupt is executed.
     * 
     * We check if the channel is DEAD by checking the channel status reg. Also, the watchdog 
     * timer can force a transmitter reset if it sees no interrupt activity for to consecutive
     * timeout intervals.
     */
 
    dmaStatus = IOGetDBDMAChannelStatus( ioBaseEnetTxDMA );
    if ( dmaStatus & kdbdmaDead || txWDForceReset == YES )
    {
      /*
       * Read the transmit frame status and log error
       */
      xmtFS = ReadMaceRegister( ioBaseEnet, kXmtFS );
      [networkInterface incrementOutputErrors]; 
      IOLog( "Ethernet(Mace): Tx DMA Error - Status = %04x FS = %02x\n\r", dmaStatus, xmtFS);
  
      /*
       * Reset and reinitialize chip
       */
      [self _restartChip];
  
      txWDForceReset = NO;
    }   
    
    return YES;
}
   

/*
 * Breaks up an ethernet data buffer into two physical chunks. We know that
 * the buffer can't straddle more than two pages. If the content of paddr2 is
 * zero this means that all of the buffer lies in one physical page. Note
 * that we use the fact that tx and rx descriptors have the same size and
 * same layout of relevent fields (data address and count). 
 */

-(BOOL) _updateDescriptorFromNetBuf:(netbuf_t)nb  Desc:(enet_dma_cmd_t *)desc  ReceiveFlag:(BOOL)isReceive
{
    IOReturn 		result;
    vm_address_t 	pageBreak;
    vm_address_t	vaddr;
    u_int32_t   	paddr[2];
    u_int32_t 		len[2];  
    u_int32_t 		size;   
    u_int32_t           nextDesc = 0;         
    
    size = isReceive ? NETWORK_BUFSIZE : nb_size(nb);
    vaddr = (vm_address_t)nb_map(nb);

    result = IOPhysicalFromVirtual(IOVmTaskSelf(), vaddr, &paddr[0]);
    if (result != IO_R_SUCCESS) 
    {
 	return NO;
    }

    /*
     * Now check if this memory block crosses a page boundary. 
     */
    if (trunc_page(vaddr) != trunc_page(vaddr+size-1))	
    {    
      /* Nice try... */
      pageBreak = round_page(vaddr);
      len[0] = pageBreak - vaddr;
      len[1] = size - (pageBreak - vaddr);
    
      result = IOPhysicalFromVirtual(IOVmTaskSelf(), pageBreak, &paddr[1]);
      if (result != IO_R_SUCCESS) 
      {
 	return NO;
      }
    }
    else
    {
      paddr[1] = 0;
      len[0]   = size;
      len[1]   = 0;
    }    
   
    if ( !len[1] )
    {
      IOMakeDBDMADescriptor( (&desc->desc_seg[0]),
                             ((isReceive) ? kdbdmaInputLast : kdbdmaOutputLast), 
                             (kdbdmaKeyStream0),
                             (kdbdmaIntNever),
                             (kdbdmaBranchNever),
                             ((isReceive) ? kdbdmaWaitNever : kdbdmaWaitIfFalse),
                             (len[0]),
                             (paddr[0])  );
  
      desc->desc_seg[1] = (isReceive) ? dbdmaCmd_NopWInt : dbdmaCmd_Nop;
    }
    else
    {
      if ( isReceive ) 
      {
        nextDesc = rxDMACommandsPhys + (int)desc - (int)rxDMACommands + sizeof(enet_dma_cmd_t);
      }

      IOMakeDBDMADescriptorDep( (&desc->desc_seg[0]),
                                ((isReceive) ? kdbdmaInputMore : kdbdmaOutputMore), 
                                (kdbdmaKeyStream0),
                                ((isReceive) ? kdbdmaIntIfTrue : kdbdmaIntNever),
                                ((isReceive) ? kdbdmaBranchIfTrue : kdbdmaBranchNever),
                                (kdbdmaWaitNever),
                                (len[0]),
                                (paddr[0]),  
                                nextDesc   ); 

      IOMakeDBDMADescriptor(    (&desc->desc_seg[1]),
                                ((isReceive) ? kdbdmaInputLast : kdbdmaOutputLast), 
                                (kdbdmaKeyStream0),
                                ((isReceive) ? kdbdmaIntAlways : kdbdmaIntNever),
                                (kdbdmaBranchNever),
                                ((isReceive) ? kdbdmaWaitNever : kdbdmaWaitIfFalse),
                                (len[1]),
                                (paddr[1])  );
    }

    flush_cache_v( (vm_offset_t)desc, sizeof(enet_dma_cmd_t) );

    return YES;
}

 
#ifdef DEBUG
/*
 * Useful for testing. 
 */
- (void)_dumpDesc:(void *)addr Size:(u_int32_t)size
{
    u_int32_t		i;
    unsigned long	*p;
    vm_offset_t         paddr;

    IOPhysicalFromVirtual( IOVmTaskSelf(), (vm_offset_t) addr, (vm_offset_t *)&paddr );

    p = (unsigned long *)addr;

    for ( i=0; i < size/sizeof(IODBDMADescriptor); i++, p+=4, paddr+=sizeof(IODBDMADescriptor) )
    {    
        IOLog("Ethernet(Mace): %08x(v) %0x08(p):  %08x %08x %08x %08x\n", 
              (int)p, 
              (int)paddr,
              (int)ReadSwap32(p, 0),   (int)ReadSwap32(p, 4),
              (int)ReadSwap32(p, 8),   (int)ReadSwap32(p, 12) );
    }
    IOLog("\n");
}

- (void)_dumpRegisters
{
    u_int8_t	dataValue;

    IOLog("\nEthernet(Mace): IO Address = %08x", (int)ioBaseEnet );
 
    dataValue = ReadMaceRegister(ioBaseEnet, kXmtFC);
    IOLog("\nEthernet(Mace): Read Register %04x Transmit Frame Control     = %02x", kXmtFC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kXmtFS);
    IOLog("\nEthernet(Mace): Read Register %04x Transmit Frame Status      = %02x", kXmtFS, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kXmtRC);
    IOLog("\nEthernet(Mace): Read Register %04x Transmit Retry Count       = %02x", kXmtRC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kRcvFC);
    IOLog("\nEthernet(Mace): Read Register %04x Receive Frame Control      = %02x", kRcvFC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kRcvFS0);
    IOLog("\nEthernet(Mace): Read Register %04x Receive Frame Status 0     = %02x", kRcvFS0, dataValue );
    dataValue = ReadMaceRegister(ioBaseEnet, kRcvFS1);
    IOLog("\nEthernet(Mace): Read Register %04x Receive Frame Status 1     = %02x", kRcvFS1, dataValue );
    dataValue = ReadMaceRegister(ioBaseEnet, kRcvFS2);
    IOLog("\nEthernet(Mace): Read Register %04x Receive Frame Status 2     = %02x", kRcvFS2, dataValue );
    dataValue = ReadMaceRegister(ioBaseEnet, kRcvFS3);
    IOLog("\nEthernet(Mace): Read Register %04x Receive Frame Status 3     = %02x", kRcvFS3, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kFifoFC);
    IOLog("\nEthernet(Mace): Read Register %04x FIFO Frame Count           = %02x", kFifoFC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kIntReg);
    IOLog("\nEthernet(Mace): Read Register %04x Interrupt Register         = %02x", kIntReg, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kIntMask);
    IOLog("\nEthernet(Mace): Read Register %04x Interrupt Mask Register    = %02x", kIntMask, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kPollReg);
    IOLog("\nEthernet(Mace): Read Register %04x Poll Register              = %02x", kPollReg, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kBIUCC);
    IOLog("\nEthernet(Mace): Read Register %04x BUI Configuration Control  = %02x", kBIUCC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kFifoCC);
    IOLog("\nEthernet(Mace): Read Register %04x FIFO Configuration Control = %02x", kFifoCC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kMacCC);
    IOLog("\nEthernet(Mace): Read Register %04x MAC Configuration Control  = %02x", kMacCC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kPLSCC);
    IOLog("\nEthernet(Mace): Read Register %04x PLS Configuration Contro   = %02x", kPLSCC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kPHYCC);
    IOLog("\nEthernet(Mace): Read Register %04x PHY Configuration Control  = %02x", kPHYCC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kMaceChipId0);
    IOLog("\nEthernet(Mace): Read Register %04x MACE ChipID Register 7:0   = %02x", kMaceChipId0, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kMaceChipId1);
    IOLog("\nEthernet(Mace): Read Register %04x MACE ChipID Register 15:8  = %02x", kMaceChipId1, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kMPC);
    IOLog("\nEthernet(Mace): Read Register %04x Missed Packet Count        = %02x", kMPC, dataValue );

    dataValue = ReadMaceRegister(ioBaseEnet, kUTR);
    IOLog("\nEthernet(Mace): Read Register %04x User Test Register         = %02x", kUTR, dataValue );
    IOLog("\nEthernet(Mace): -------------------------------------------------------\n" );
}
#endif DEBUG


/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)_getStationAddress:(enet_addr_t *)ea
{
    int i;
    unsigned char data;

    for (i = 0; i < sizeof(*ea); i++)	
    {
      data = ((unsigned char *)ioBaseEnetROM)[i << 4];
      ea->ea_byte[i]   = reverseBitOrder(data);
    }
}



/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

#define ENET_CRCPOLY 0x04c11db7

/* Real fast bit-reversal algorithm, 6-bit values */
static int reverse6[] = 
{	0x0,0x20,0x10,0x30,0x8,0x28,0x18,0x38,
	0x4,0x24,0x14,0x34,0xc,0x2c,0x1c,0x3c,
	0x2,0x22,0x12,0x32,0xa,0x2a,0x1a,0x3a,
	0x6,0x26,0x16,0x36,0xe,0x2e,0x1e,0x3e,
	0x1,0x21,0x11,0x31,0x9,0x29,0x19,0x39,
	0x5,0x25,0x15,0x35,0xd,0x2d,0x1d,0x3d,
	0x3,0x23,0x13,0x33,0xb,0x2b,0x1b,0x3b,
	0x7,0x27,0x17,0x37,0xf,0x2f,0x1f,0x3f
};

static u_int32_t crc416(unsigned int current, unsigned short nxtval )
{
    register unsigned int counter;
    register int highCRCBitSet, lowDataBitSet;

    /* Swap bytes */
    nxtval = ((nxtval & 0x00FF) << 8) | (nxtval >> 8);

    /* Compute bit-by-bit */
    for (counter = 0; counter != 16; ++counter)
    {	/* is high CRC bit set? */
      if ((current & 0x80000000) == 0)	
        highCRCBitSet = 0;
      else
        highCRCBitSet = 1;
		
      current = current << 1;
	
      if ((nxtval & 0x0001) == 0)
        lowDataBitSet = 0;
      else
	lowDataBitSet = 1;

      nxtval = nxtval >> 1;
	
      /* do the XOR */
      if (highCRCBitSet ^ lowDataBitSet)
        current = current ^ ENET_CRCPOLY;
    }
    return current;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

static u_int32_t mace_crc(unsigned short *address)
{	
    register u_int32_t newcrc;

    newcrc = crc416(0xffffffff, *address);	/* address bits 47 - 32 */
    newcrc = crc416(newcrc, address[1]);	/* address bits 31 - 16 */
    newcrc = crc416(newcrc, address[2]);	/* address bits 15 - 0  */

    return(newcrc);
}

/*
 * Add requested mcast addr to Mace's hash table filter.  
 *  
 */
-(void) _addToHashTableMask:(u_int8_t *)addr
{	
    u_int32_t	 crc;
    u_int8_t	 mask;

    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (hashTableUseCount[crc]++)	
      return;			/* This bit is already set */
    mask = crc % 8;
    mask = (unsigned char) 1 << mask;
    hashTableMask[crc/8] |= mask;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

-(void) _removeFromHashTableMask:(u_int8_t *)addr
{	
    unsigned int crc;
    unsigned char mask;

    /* Now, delete the address from the filter copy, as indicated */
    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (hashTableUseCount[crc] == 0)
      return;			/* That bit wasn't in use! */

    if (--hashTableUseCount[crc])
      return;			/* That bit is still in use */

    mask = crc % 8;
    mask = ((unsigned char)1 << mask) ^ 0xffff; /* To turn off bit */
    hashTableMask[crc/8] &= mask;
}

/*
 * Sync the adapter with the software copy of the multicast mask
 *  (logical address filter).
 */
-(void) _updateHashTableMask
{
    u_int8_t		status;
    u_int32_t		i;
    u_int8_t		*p;

    if ( chipId != kMaceRevisionA2 )
    { 
      WriteMaceRegister( ioBaseEnet, kIAC, kIACAddrChg | kIACLogAddr );
      do
      {
        status = ReadMaceRegister( ioBaseEnet, kIAC );
      }
      while( status & kIACAddrChg );
    }
    else
    {
      WriteMaceRegister( ioBaseEnet, kIAC, kIACLogAddr );
    }

    p = (u_int8_t *) hashTableMask;
    for (i = 0; i < 8; i++, p++ )
    {
      WriteMaceRegister( ioBaseEnet, kLADRF, *p );
    }
}

@end

