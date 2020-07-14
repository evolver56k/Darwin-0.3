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
 * Copyright (c) 1998-1999 by Apple Computer, Inc., All rights reserved.
 *
 * Implementation for hardware dependent (relatively) code 
 * for the BMac Ethernet controller. 
 *
 * HISTORY
 *
 */
#import "BMacEnetPrivate.h"
#import <mach/vm_param.h>			// page alignment macros

extern void 			*kernel_map;
extern kern_return_t		kmem_alloc_wired();

static IODBDMADescriptor	dbdmaCmd_Nop;
static IODBDMADescriptor   	dbdmaCmd_NopWInt;
static IODBDMADescriptor        dbdmaCmd_LoadInt;			
static IODBDMADescriptor        dbdmaCmd_LoadIntWInt;			
static IODBDMADescriptor	dbdmaCmd_Stop;
static IODBDMADescriptor	dbdmaCmd_Branch;

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
    

@implementation BMacEnet (Private)


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
          IOLog( "Ethernet(BMac): Cant allocate channel DBDMA commands\n\r" );
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
         IOLog( "Ethernet(BMac): Cant allocate contiguous memory for DBDMA commands\n\r" );
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

    IOMakeDBDMADescriptor( (&dbdmaCmd_LoadInt),
                            kdbdmaLoadQuad,
                            kdbdmaKeySystem,
                            kdbdmaIntNever,
                            kdbdmaBranchNever,
                            kdbdmaWaitNever,
                            2,
                            ((int)ioBaseEnet +  kSTAT)   );

    IOMakeDBDMADescriptor( (&dbdmaCmd_LoadIntWInt),
                            kdbdmaLoadQuad,
                            kdbdmaKeySystem,
                            kdbdmaIntAlways,
                            kdbdmaBranchNever,
                            kdbdmaWaitNever,
                            2,
                            ((int)ioBaseEnet +  kSTAT)   );

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
 * which read transmit frame status and interrupt status from the Bmac chip. The last
 * DMA command in each transmit ring entry generates a host interrupt.
 * The last entry in the ring is followed by a DMA branch to the first
 * entry.
 *-------------------------------------------------------------------------*/

- (BOOL)_initTxRing
{
    BOOL			kr;
    u_int32_t			i;
    IODBDMADescriptor		dbdmaCmd, dbdmaCmdInt;

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
      IOLog("Ethernet(BMac): Cant allocate transmit queue\n\r");
      return NO;
    }

    /*
     * DMA Channel commands 2 are the same for all DBDMA entries on transmit.
     * Initialize them now.
     */
    
    dbdmaCmd     = ( chipId >= kCHIPID_PaddingtonXmitStreaming ) ? dbdmaCmd_Nop     : dbdmaCmd_LoadInt;
    dbdmaCmdInt  = ( chipId >= kCHIPID_PaddingtonXmitStreaming ) ? dbdmaCmd_NopWInt : dbdmaCmd_LoadIntWInt;

    for( i=0; i < txMaxCommand; i++ )
    {
      txDMACommands[i].desc_seg[2] = ( (i+1) % TX_PKTS_PER_INT ) ? dbdmaCmd : dbdmaCmdInt;
    }

    /* 
     * Put a DMA Branch command after the last entry in the transmit ring. Set the branch address
     * to the physical address of the start of the transmit ring.
     */
    txDMACommands[txMaxCommand].desc_seg[0] = dbdmaCmd_Branch; 

    kr = IOPhysicalFromVirtual( (vm_task_t) IOVmTaskSelf(), (vm_address_t) txDMACommands, (u_int32_t *)&txDMACommandsPhys );
    if ( kr != IO_R_SUCCESS )
    {
      IOLog( "Ethernet(BMac): Bad DBDMA command buf - %08x\n\r", (u_int32_t)txDMACommands );
    }
    IOSetCCCmdDep( &txDMACommands[txMaxCommand].desc_seg[0], txDMACommandsPhys );
 
    /* 
     * Set the Transmit DMA Channel pointer to the first entry in the transmit ring.
     */
    IOSetDBDMACommandPtr( ioBaseEnetTxDMA, txDMACommandsPhys );

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
    u_int32_t 		i;
    BOOL		status;
    IOReturn    	kr;
    
    /*
     * Clear the receive DMA command memory
     */
    bzero( (void *)rxDMACommands, sizeof(enet_dma_cmd_t) * rxMaxCommand);

    kr = IOPhysicalFromVirtual( (vm_task_t) IOVmTaskSelf(), (vm_address_t) rxDMACommands, (u_int32_t *)&rxDMACommandsPhys );
    if ( kr != IO_R_SUCCESS )
    {
      IOLog( "Ethernet(BMac): Bad DBDMA command buf - %08x\n\r",  (u_int32_t)rxDMACommands );
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
          IOLog("Ethernet(BMac): allocateNetbuf returned NULL in _initRxRing\n\r");
          return NO;
	}
      }
      /* 
       * Set the DMA commands for the ring entry to transfer data to the NetBuf.
       */
      status = [self _updateDescriptorFromNetBuf:rxNetbuf[i]  Desc:(void *)&rxDMACommands[i]  ReceiveFlag:YES ];
      if (status == NO)
      {    
        IOLog("Ethernet(BMac): cant map Netbuf to physical memory in _initRxRing\n\r");
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
    IOSetDBDMACommandPtr (ioBaseEnetRxDMA, rxDMACommandsPhys);

    return YES;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)_startChip
{
    u_int16_t	oldConfig;

    IODBDMAContinue( ioBaseEnetRxDMA );
  
    // turn on rx plus any other bits already on (promiscuous possibly)
    oldConfig = ReadBigMacRegister(ioBaseEnet, kRXCFG);		
    WriteBigMacRegister(ioBaseEnet, kRXCFG, oldConfig | kRxMACEnable ); 
 
    oldConfig = ReadBigMacRegister(ioBaseEnet, kTXCFG);		
    WriteBigMacRegister(ioBaseEnet, kTXCFG, oldConfig | kTxMACEnable );  
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)_resetChip
{
    volatile u_int32_t  *heathrowFCR;
    u_int32_t		fcrValue;
    u_int16_t		*pPhyType;
	 
    IODBDMAReset( ioBaseEnetRxDMA );  
    IODBDMAReset( ioBaseEnetTxDMA );  

    IOSetDBDMAWaitSelect(      ioBaseEnetTxDMA, IOSetDBDMAChannelControlBits( kdbdmaS5 ) );

    IOSetDBDMABranchSelect(    ioBaseEnetRxDMA, IOSetDBDMAChannelControlBits( kdbdmaS6 ) );
    IOSetDBDMAInterruptSelect( ioBaseEnetRxDMA, IOSetDBDMAChannelControlBits( kdbdmaS6 ) );

    heathrowFCR = (u_int32_t *)((u_int8_t *)ioBaseHeathrow + kHeathrowFCR);

    fcrValue = *heathrowFCR;
    eieio();

    fcrValue = ReadSwap32( &fcrValue, 0 );

    /*
     * Enable the ethernet transceiver/clocks
     */
    fcrValue |= kEnetEnabledBits;
    fcrValue &= ~kResetEnetCell;							

    *heathrowFCR = ReadSwap32( &fcrValue, 0 );
    eieio();
    IOSleep( 100 );

    /*
     * Determine if PHY chip is configured. Reset and enable it (if present).
     */
    if ( phyId == 0xff )
    {
        phyMIIDelay = 20;
        if ( [self miiFindPHY:&phyId] == YES )
        {
            [self miiResetPHY:phyId];

            pPhyType = (u_int16_t *)&phyType;
            [self miiReadWord: pPhyType   reg: MII_ID0 phy:phyId];
            [self miiReadWord: pPhyType+1 reg: MII_ID1 phy:phyId];

            if ( (phyType & MII_ST10040_MASK) == MII_ST10040_ID )
            {
                phyMIIDelay = MII_ST10040_DELAY;
            }
            else if ( (phyType & MII_DP83843_MASK) == MII_DP83843_ID )
            {
                phyMIIDelay = MII_DP83843_DELAY;
            }
        }
    }

    /*
     * Reset the reset the ethernet cell
     */
    fcrValue |= kResetEnetCell;						
    *heathrowFCR = ReadSwap32( &fcrValue, 0 );
    eieio();
    IOSleep( 10 );

    fcrValue &= ~kResetEnetCell;							
    *heathrowFCR = ReadSwap32( &fcrValue, 0 );
    eieio();
    IOSleep( 10 );

    chipId = ReadBigMacRegister(ioBaseEnet, kCHIPID) & 0xFF;
	
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL) _initChip
{
    volatile u_int16_t		regValue;
    ns_time_t   		timeStamp;	   
    u_int16_t			*pWord16;

    WriteBigMacRegister(ioBaseEnet, kTXRST, kTxResetBit);

    do	
    {
      regValue = ReadBigMacRegister(ioBaseEnet, kTXRST);		// wait for reset to clear..acknowledge
    } 
    while( regValue & kTxResetBit );

    WriteBigMacRegister(ioBaseEnet, kRXRST, kRxResetValue);

    if ( phyId == 0xff )
    {
        WriteBigMacRegister(ioBaseEnet, kXCVRIF, kClkBit | kSerialMode | kCOLActiveLow);
    }	

    IOGetTimestamp(&timeStamp);	
    WriteBigMacRegister(ioBaseEnet, kRSEED, (u_int16_t) timeStamp );		

    regValue = ReadBigMacRegister(ioBaseEnet, kXIFC);
    regValue |= kTxOutputEnable;
    WriteBigMacRegister(ioBaseEnet, kXIFC, regValue);

    ReadBigMacRegister(ioBaseEnet, kPAREG);

    // set collision counters to 0
    WriteBigMacRegister(ioBaseEnet, kNCCNT, 0);
    WriteBigMacRegister(ioBaseEnet, kNTCNT, 0);
    WriteBigMacRegister(ioBaseEnet, kEXCNT, 0);
    WriteBigMacRegister(ioBaseEnet, kLTCNT, 0);

    // set rx counters to 0
    WriteBigMacRegister(ioBaseEnet, kFRCNT, 0);
    WriteBigMacRegister(ioBaseEnet, kLECNT, 0);
    WriteBigMacRegister(ioBaseEnet, kAECNT, 0);
    WriteBigMacRegister(ioBaseEnet, kFECNT, 0);
    WriteBigMacRegister(ioBaseEnet, kRXCV, 0);

    // set tx fifo information
    WriteBigMacRegister(ioBaseEnet, kTXTH, 0xff);				// 255 octets before tx starts

    WriteBigMacRegister(ioBaseEnet, kTXFIFOCSR, 0);				// first disable txFIFO
    WriteBigMacRegister(ioBaseEnet, kTXFIFOCSR, kTxFIFOEnable );

    // set rx fifo information
    WriteBigMacRegister(ioBaseEnet, kRXFIFOCSR, 0);				// first disable rxFIFO
    WriteBigMacRegister(ioBaseEnet, kRXFIFOCSR, kRxFIFOEnable ); 

    //WriteBigMacRegister(ioBaseEnet, kTXCFG, kTxMACEnable);			// kTxNeverGiveUp maybe later
    ReadBigMacRegister(ioBaseEnet, kSTAT);					// read it just to clear it

    // zero out the chip Hash Filter registers
    WriteBigMacRegister(ioBaseEnet, kHASH3, hashTableMask[0]); 	// bits 15 - 0
    WriteBigMacRegister(ioBaseEnet, kHASH2, hashTableMask[1]); 	// bits 31 - 16
    WriteBigMacRegister(ioBaseEnet, kHASH1, hashTableMask[2]); 	// bits 47 - 32
    WriteBigMacRegister(ioBaseEnet, kHASH0, hashTableMask[3]); 	// bits 63 - 48
	
    pWord16 = (u_int16_t *)&myAddress.ea_byte[0];
    WriteBigMacRegister(ioBaseEnet, kMADD0, *pWord16++);
    WriteBigMacRegister(ioBaseEnet, kMADD1, *pWord16++);
    WriteBigMacRegister(ioBaseEnet, kMADD2, *pWord16);
    
    WriteBigMacRegister(ioBaseEnet, kRXCFG, kRxCRCEnable | kRxHashFilterEnable | kRxRejectOwnPackets);
    
    return YES;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _disableAdapterInterrupts
{
    WriteBigMacRegister( ioBaseEnet, kINTDISABLE, kNoEventsMask );
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _enableAdapterInterrupts
{
    WriteBigMacRegister( ioBaseEnet, 
                         kINTDISABLE, 
                         ( chipId >= kCHIPID_PaddingtonXmitStreaming ) ? kNoEventsMask: kNormalIntEvents );
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _setDuplexMode: (BOOL) duplexMode
{
    u_int16_t		txCFGVal;

    isFullDuplex = duplexMode;

    txCFGVal = ReadBigMacRegister( ioBaseEnet, kTXCFG);

    WriteBigMacRegister( ioBaseEnet, kTXCFG, txCFGVal & ~kTxMACEnable );
    while( ReadBigMacRegister(ioBaseEnet, kTXCFG) & kTxMACEnable )
      ;
 
    if ( isFullDuplex )
    {
        txCFGVal |= (kTxIgnoreCollision | kTxFullDuplex);
    }
    else
    {
        txCFGVal &= ~(kTxIgnoreCollision | kTxFullDuplex);
    }
    
    WriteBigMacRegister( ioBaseEnet, kTXCFG, txCFGVal );
}    

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _restartTransmitter
{
    u_int16_t		regValue;

    /*
     * Shutdown DMA channel
     */
    [self _stopTransmitDMA];

    /*
     * Get the silicon's attention
     */
    WriteBigMacRegister( ioBaseEnet, kTXFIFOCSR, 0 );
    WriteBigMacRegister( ioBaseEnet, kTXFIFOCSR, kTxFIFOEnable );

    ReadBigMacRegister( ioBaseEnet, kSTAT );

    regValue = ReadBigMacRegister(ioBaseEnet, kTXCFG);		
    WriteBigMacRegister(ioBaseEnet, kTXCFG, regValue | kTxMACEnable );  

    /*
     * Restart transmit DMA
     */
    IODBDMAContinue( ioBaseEnetTxDMA );  
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _restartReceiver
{
    u_int16_t		oldConfig;

    /*
     * Shutdown DMA channel
     */
    [self _stopReceiveDMA];

    /*
     * Get the silicon's attention
     */
    WriteBigMacRegister( ioBaseEnet, kRXFIFOCSR, 0 );
    WriteBigMacRegister( ioBaseEnet, kRXFIFOCSR, kRxFIFOEnable );

    oldConfig = ReadBigMacRegister(ioBaseEnet, kRXCFG);		
    WriteBigMacRegister(ioBaseEnet, kRXCFG, oldConfig | kRxMACEnable ); 
 
    /*
     * Restart receive DMA
     */
    IODBDMAContinue( ioBaseEnetRxDMA );  
}

/*-------------------------------------------------------------------------
 *
 * Orderly stop of receive DMA.
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _stopReceiveDMA
{
    u_int32_t		dmaCmdPtr;
    u_int8_t		rxCFGVal;

    /* 
     * Stop the receiver and allow any frame receive in progress to complete
     */
    rxCFGVal = ReadBigMacRegister(ioBaseEnet, kRXCFG);
    WriteBigMacRegister(ioBaseEnet, kRXCFG, rxCFGVal & ~kRxMACEnable );
    IODelay( RECEIVE_QUIESCE_uS * 1000 );

    IODBDMAReset( ioBaseEnetRxDMA );

    dmaCmdPtr = rxDMACommandsPhys + rxCommandHead * sizeof(enet_dma_cmd_t); 
    IOSetDBDMACommandPtr( ioBaseEnetRxDMA, dmaCmdPtr );
}    

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) _stopTransmitDMA
{
    u_int32_t		dmaCmdPtr;
    u_int8_t		txCFGVal;

    /* 
     * Stop the transmitter and allow any frame transmit in progress to abort
     */
    txCFGVal = ReadBigMacRegister(ioBaseEnet, kTXCFG);
    WriteBigMacRegister(ioBaseEnet, kTXCFG, txCFGVal & ~kTxMACEnable );

    IODelay( TRANSMIT_QUIESCE_uS * 1000 );

    IODBDMAReset( ioBaseEnetTxDMA );
    
    dmaCmdPtr = txDMACommandsPhys + txCommandHead * sizeof(enet_txdma_cmd_t); 
    IOSetDBDMACommandPtr( ioBaseEnetTxDMA, dmaCmdPtr );
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
  
    [self performLoopback:packet];

    /*
     * Check for room on the transmit ring. There should always be space since it is
     * the responsibility of the caller to verify this before calling _transmitPacket.
     */
    i = txCommandTail + 1;
    if ( i >= txMaxCommand ) i = 0;
    if ( i == txCommandHead )
    {
	IOLog("Ethernet(BMac): Freeing transmit packet eh?\n\r");
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

    txNetbuf[txCommandTail] = packet;
    txDMACommands[txCommandTail].desc_seg[0].operation = tmpCommand.desc_seg[0].operation;

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

    [self disableAllInterrupts];

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

    [self enableAllInterrupts];

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

    [self disableAllInterrupts];

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
      IOLog( "Ethernet(BMac): Polled tranmit timeout - 1\n\r");
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
      IOLog( "Ethernet(BMac): Polled tranmit timeout - 2\n\r");
    }

    [self enableAllInterrupts];

    return;
}

/*-------------------------------------------------------------------------
 * _sendDummyPacket
 * ----------------
 * The BMac receiver seems to be locked until we send our first packet.
 *
 *-------------------------------------------------------------------------*/
- (void) _sendDummyPacket
{
    union
    {
        u_int8_t	bytes[64];
        enet_addr_t     enet_addr[2];
    } dummyPacket;

    bzero( &dummyPacket, sizeof(dummyPacket) );
    dummyPacket.enet_addr[0] = myAddress;   
    dummyPacket.enet_addr[1] = myAddress;
    [self _sendPacket:(void *)&dummyPacket length:sizeof(dummyPacket)];
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
    int			receivedFrameSize = 0;
    u_int32_t           dmaCount[2], dmaResid[2], dmaStatus[2];
    u_int32_t		dmaChnlStatus;
    u_int16_t           rxPktStatus = 0;
    u_int32_t           badFrameCount;
    BOOL		passPacketUp;
    BOOL		reusePkt;
    BOOL		status;
    u_int32_t		nextDesc; 
       
    last      = -1;  
    i         = rxCommandHead;

    while ( 1 )
    {
      passPacketUp = NO;
      reusePkt     = NO;

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
      IOLog("Ethernet(BMac): Rx NetBuf[%2d] = %08x Resid[0] = %04x Status[0] = %04x Resid[1] = %04x Status[1] = %04x\n\r",
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
       * The BMac Ethernet controller appends two bytes to each receive buffer containing the buffer 
       * size and receive frame status.
       * We locate these bytes by using the DMA residual counts.
       */ 
      receivedFrameSize = dmaCount[0] - dmaResid[0] + dmaCount[1] - ((dmaStatus[0] & kdbdmaBt) ? dmaCount[1] : dmaResid[1]);

      if ( receivedFrameSize >= 2 )
      {
      /*
       * Get the receive frame size as reported by the BMac controller
       */
        rxPktStatus =  *(u_int16_t *)((u_int32_t)nb_map(rxNetbuf[i]) + receivedFrameSize - 2);
        receivedFrameSize = rxPktStatus & kRxLengthMask;
      }

      /*
       * Reject packets that are runts or that have other mutations.
       */
      if ( receivedFrameSize < (ETHERMINPACKET - ETHERCRC) || 
                   receivedFrameSize > (ETHERMAXPACKET + ETHERCRC) || 
                      rxPktStatus & kRxAbortBit )
      {
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
        status = [self _updateDescriptorFromNetBuf:rxNetbuf[i] Desc:(void *)&rxDMACommands[i] ReceiveFlag:YES];
        if (status == NO) 
        {
          IOLog("Ethernet(BMac): _updateDescriptorFromNetBuf failed for receive\n"); 
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
//        IOLog("Ethernet(Bmac): Packet from network - %08x %d\n", (int) packet, (int)nb_size(packet) );
          KERNEL_DEBUG(DBG_BMAC_RXCOMPLETE | DBG_FUNC_NONE, (int) packet, (int)nb_size(packet), 0, 0, 0 );
          [networkInterface handleInputPacket:packet extra:0];
        }
        else
        { 
//          IOLog("Ethernet(BMac): Packet to debugger - %08x %d\n", (int) packet, (int)nb_size(packet) );
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
    IOLog( "Ethernet(BMac): Prev - Rx Head = %2d Rx Tail = %2d Rx Last = %2d\n\r", rxCommandHead, rxCommandTail, last );
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

      rxNetbuf[rxCommandTail] = newPacket;

      rxDMACommands[rxCommandTail].desc_seg[0].operation = tmpCommand.desc_seg[0].operation;

      /*
       * Update rxCommmandTail to point to the new Stop command. Update rxCommandHead to point to 
       * the next slot in the ring past the Stop command 
       */
      rxCommandTail = last;
      rxCommandHead = i;
    }

    /*
     * Update receive error statistics
     */
    badFrameCount =  ReadBigMacRegister(ioBaseEnet, kFECNT) 
                       + ReadBigMacRegister(ioBaseEnet, kAECNT)
                           + ReadBigMacRegister(ioBaseEnet, kLECNT);
    /*
     * Clear Hardware counters
     */
    WriteBigMacRegister(ioBaseEnet, kFECNT, 0);
    WriteBigMacRegister(ioBaseEnet, kAECNT, 0);
    WriteBigMacRegister(ioBaseEnet, kLECNT, 0);

    [networkInterface incrementInputErrorsBy: badFrameCount];    

    /*
     * Check for error conditions that may cause the receiver to stall
     */
    dmaChnlStatus = IOGetDBDMAChannelStatus( ioBaseEnetRxDMA );
 
    if ( dmaChnlStatus & kdbdmaDead )
    {
      
      [networkInterface incrementInputErrors]; 
 
      IOLog( "Ethernet(Bmac): Rx DMA Error - Status = %04x\n\r", dmaChnlStatus );
      [self _restartReceiver];
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
    u_int32_t           	badFrameCount;
    BOOL			fServiced = NO;

    while ( 1 )
    {
      /*
       * Check the status of the last descriptor in this entry to see if the DMA engine completed
       * this entry.
       */
      dmaStatus = IOGetCCResult( &txDMACommands[txCommandHead].desc_seg[1] ) >> 16;
      if ( !(dmaStatus & kdbdmaActive) )
      {
        break;
      }

      [networkInterface incrementOutputPackets];

      fServiced = YES;

      /*
       * Free the Netbuf we just transmitted
       */
      KERNEL_DEBUG(DBG_BMAC_TXCOMPLETE | DBG_FUNC_NONE, (int) txNetbuf[txCommandHead], (int) nb_size(txNetbuf[txCommandHead]), 0, 0, 0 );
      nb_free( txNetbuf[txCommandHead] );
      txNetbuf[txCommandHead] = NULL;

      if ( ++txCommandHead >= txMaxCommand ) txCommandHead = 0;
    }

    /* 
     * Increment transmit error statistics
     */
    badFrameCount = ReadBigMacRegister(ioBaseEnet, kNCCNT ); 

    WriteBigMacRegister( ioBaseEnet, kNCCNT, 0 );

    [networkInterface incrementCollisionsBy: badFrameCount];
    
    badFrameCount = ReadBigMacRegister(ioBaseEnet, kEXCNT )
                      + ReadBigMacRegister(ioBaseEnet, kLTCNT );

    WriteBigMacRegister( ioBaseEnet, kEXCNT, 0 );
    WriteBigMacRegister( ioBaseEnet, kLTCNT, 0 );

    [networkInterface incrementOutputErrorsBy: badFrameCount];

    /*
     * Check for error conditions that may cause the transmitter to stall
     */
    dmaStatus = IOGetDBDMAChannelStatus( ioBaseEnetTxDMA );
 
    if ( dmaStatus & kdbdmaDead )
    {
      [networkInterface incrementOutputErrors]; 
 
      IOLog( "Ethernet(Bmac): Tx DMA Error - Status = %04x\n\r", dmaStatus );
      [self _restartTransmitter];

      fServiced = YES;
    }   

    return fServiced;
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
    u_int32_t		waitMask = 0;                

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
   
    if ( isReceive || chipId >= kCHIPID_PaddingtonXmitStreaming )
    {
        waitMask = kdbdmaWaitNever;
    }
    else
    {
        waitMask = kdbdmaWaitIfFalse;
    }

    if ( !len[1] )
    {
      IOMakeDBDMADescriptor( (&desc->desc_seg[0]),
                             ((isReceive) ? kdbdmaInputLast : kdbdmaOutputLast), 
                             (kdbdmaKeyStream0),
                             (kdbdmaIntNever),
                             (kdbdmaBranchNever),
                             (waitMask),
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
                                (waitMask),
                                (len[1]),
                                (paddr[1])  );
    }

    return YES;
}
    
#ifdef DEBUG
/*
 * Useful for testing. 
 */
- (void)_dump_srom
{
    unsigned short data;
    int i;
	
    for (i = 0; i < 128; i++)	
    {
      reset_and_select_srom(ioBaseEnet);
      data = read_srom(ioBaseEnet, i, sromAddressBits);
      IOLog("Ethernet(BMac): %x = %x ", i, data);
      if (i % 10 == 0) IOLog("\n");
    }
}

- (void)_dumpDesc:(void *)addr Size:(u_int32_t)size
{
    u_int32_t		i;
    unsigned long	*p;
    vm_offset_t         paddr;

    IOPhysicalFromVirtual( IOVmTaskSelf(), (vm_offset_t) addr, (vm_offset_t *)&paddr );

    p = (unsigned long *)addr;

    for ( i=0; i < size/sizeof(IODBDMADescriptor); i++, p+=4, paddr+=sizeof(IODBDMADescriptor) )
    {
        IOLog("Ethernet(BMac): %08x(v) %08x(p):  %08x %08x %08x %08x\n\r",
              (int)p,
              (int)paddr,
              (int)ReadSwap32(p, 0),   (int)ReadSwap32(p, 4),
              (int)ReadSwap32(p, 8),   (int)ReadSwap32(p, 12) );
    }
}

- (void)_dumpRegisters
{
    u_int16_t	dataValue;

    IOLog("\nEthernet(BMac): IO Address = %08x", (int)ioBaseEnet );

    dataValue = ReadBigMacRegister(ioBaseEnet, kXIFC);
    IOLog("\nEthernet(BMac): Read Register %04x Transceiver I/F = %04x", kXIFC, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kSTAT);
    IOLog("\nEthernet(BMac): Read Register %04x Int Events      = %04x", kSTAT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kINTDISABLE);
    IOLog("\nEthernet(BMac): Read Register %04x Int Disable     = %04x", kINTDISABLE, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXRST);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Reset        = %04x", kTXRST, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXCFG);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Config       = %04x", kTXCFG, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kIPG1);
    IOLog("\nEthernet(BMac): Read Register %04x IPG1            = %04x", kIPG1, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kIPG2);
    IOLog("\nEthernet(BMac): Read Register %04x IPG2            = %04x", kIPG2, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kALIMIT);
    IOLog("\nEthernet(BMac): Read Register %04x Attempt Limit   = %04x", kALIMIT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kSLOT);
    IOLog("\nEthernet(BMac): Read Register %04x Slot Time       = %04x", kSLOT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kPALEN);
    IOLog("\nEthernet(BMac): Read Register %04x Preamble Length = %04x", kPALEN, dataValue );

    IOLog("\nEthernet(BMac): -------------------------------------------------------" );
    dataValue = ReadBigMacRegister(ioBaseEnet, kPAPAT);
    IOLog("\nEthernet(BMac): Read Register %04x Preamble Pattern         = %04x", kPAPAT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXSFD);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Start Frame Delimeter = %04x", kTXSFD, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kJAM);
    IOLog("\nEthernet(BMac): Read Register %04x Jam Size                 = %04x", kJAM, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXMAX);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Max Size              = %04x", kTXMAX, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXMIN);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Min Size              = %04x", kTXMIN, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kPAREG);
    IOLog("\nEthernet(BMac): Read Register %04x Peak Attempts           = %04x", kPAREG, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kDCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Defer Timer             = %04x", kDCNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kNCCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Normal Collision Count  = %04x", kNCCNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kNTCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Network Collision Count = %04x", kNTCNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kEXCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Excessive Coll Count    = %04x", kEXCNT, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kLTCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Late Collision Count = %04x", kLTCNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRSEED);
    IOLog("\nEthernet(BMac): Read Register %04x Random Seed          = %04x", kRSEED, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXSM);
    IOLog("\nEthernet(BMac): Read Register %04x Tx State Machine     = %04x", kTXSM, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXRST);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Reset             = %04x", kRXRST, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXCFG);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Config            = %04x", kRXCFG, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXMAX);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Max Size         = %04x", kRXMAX, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXMIN);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Min Size         = %04x", kRXMIN, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMADD2);
    IOLog("\nEthernet(BMac): Read Register %04x Mac Address 2       = %04x", kMADD2, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMADD1);
    IOLog("\nEthernet(BMac): Read Register %04x Mac Address 1       = %04x", kMADD1, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMADD0);
    IOLog("\nEthernet(BMac): Read Register %04x Mac Address 0       = %04x", kMADD0, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kFRCNT);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Frame Counter    = %04x", kFRCNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kLECNT);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Length Error Cnt = %04x", kLECNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kAECNT);
    IOLog("\nEthernet(BMac): Read Register %04x Alignment Error Cnt = %04x", kAECNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kFECNT);
    IOLog("\nEthernet(BMac): Read Register %04x FCS Error Cnt       = %04x", kFECNT, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXSM);
    IOLog("\nEthernet(BMac): Read Register %04x Rx State Machine    = %04x", kRXSM, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXCV);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Code Violation = %04x", kRXCV, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kHASH3);
    IOLog("\nEthernet(BMac): Read Register %04x Hash 3            = %04x", kHASH3, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kHASH2);
    IOLog("\nEthernet(BMac): Read Register %04x Hash 2            = %04x", kHASH2, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kHASH1);
    IOLog("\nEthernet(BMac): Read Register %04x Hash 1            = %04x", kHASH1, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kHASH0);
    IOLog("\nEthernet(BMac): Read Register %04x Hash 0            = %04x", kHASH0, dataValue );
    IOLog("\n-------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kAFR2);
    IOLog("\nEthernet(BMac): Read Register %04x Address Filter 2   = %04x", kAFR2, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kAFR1);
    IOLog("\nEthernet(BMac): Read Register %04x Address Filter 1   = %04x", kAFR1, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kAFR0);
    IOLog("\nEthernet(BMac): Read Register %04x Address Filter 0   = %04x", kAFR0, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kAFCR);
    IOLog("\nEthernet(BMac): Read Register %04x Adress Filter Mask = %04x", kAFCR, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXFIFOCSR);
    IOLog("\nEthernet(BMac): Read Register %04x Tx FIFO CSR        = %04x", kTXFIFOCSR, dataValue );
    IOLog("\n-------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXTH);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Threshold  = %04x", kTXTH, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXFIFOCSR);
    IOLog("\nEthernet(BMac): Read Register %04x Rx FIFO CSR   = %04x", kRXFIFOCSR, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMEMADD);
    IOLog("\nEthernet(BMac): Read Register %04x Mem Addr      = %04x", kMEMADD, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMEMDATAHI);
    IOLog("\nEthernet(BMac): Read Register %04x Mem Data High = %04x", kMEMDATAHI, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMEMDATALO);
    IOLog("\nEthernet(BMac): Read Register %04x Mem Data Low  = %04x", kMEMDATALO, dataValue );
    IOLog("\n-------------------------------------------------------" );

    dataValue = ReadBigMacRegister(ioBaseEnet, kXCVRIF);
    IOLog("\nEthernet(BMac): Read Register %04x Transceiver IF Control = %04x", kXCVRIF, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kCHIPID);
    IOLog("\nEthernet(BMac): Read Register %04x Chip ID                = %04x", kCHIPID, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kMIFCSR);
    IOLog("\nEthernet(BMac): Read Register %04x MII CSR                = %04x", kMIFCSR, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kSROMCSR);
    IOLog("\nEthernet(BMac): Read Register %04x SROM CSR               = %04x", kSROMCSR, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kTXPNTR);
    IOLog("\nEthernet(BMac): Read Register %04x Tx Pointer             = %04x", kTXPNTR, dataValue );

    dataValue = ReadBigMacRegister(ioBaseEnet, kRXPNTR);
    IOLog("\nEthernet(BMac): Read Register %04x Rx Pointer             = %04x", kRXPNTR, dataValue );
    IOLog("\nEthernet(BMac): -------------------------------------------------------\n" );
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
    unsigned short data;

    for (i = 0; i < sizeof(*ea)/2; i++)	
    {
      reset_and_select_srom(ioBaseEnet);
      data = read_srom(ioBaseEnet, i + enetAddressOffset/2, sromAddressBits);
      ea->ea_byte[2*i]   = reverseBitOrder(data & 0x0ff);
      ea->ea_byte[2*i+1] = reverseBitOrder((data >> 8) & 0x0ff);
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
 * Add requested mcast addr to BMac's hash table filter.  
 *  
 */
-(void) _addToHashTableMask:(u_int8_t *)addr
{	
    u_int32_t	 crc;
    u_int16_t	 mask;

    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (hashTableUseCount[crc]++)	
      return;			/* This bit is already set */
    mask = crc % 16;
    mask = (unsigned short)1 << mask;
    hashTableMask[crc/16] |= mask;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

-(void) _removeFromHashTableMask:(u_int8_t *)addr
{	
    unsigned int crc;
    u_int16_t	 mask;

    /* Now, delete the address from the filter copy, as indicated */
    crc = mace_crc((unsigned short *)addr)&0x3f; /* Big-endian alert! */
    crc = reverse6[crc];	/* Hyperfast bit-reversing algorithm */
    if (hashTableUseCount[crc] == 0)
      return;			/* That bit wasn't in use! */

    if (--hashTableUseCount[crc])
      return;			/* That bit is still in use */

    mask = crc % 16;
    mask = (u_int16_t)1 << mask; /* To turn off bit */
    hashTableMask[crc/16] &= ~mask;
}

/*
 * Sync the adapter with the software copy of the multicast mask
 *  (logical address filter).
 */
-(void) _updateBMacHashTableMask
{
    u_int16_t 		rxCFGReg;

    rxCFGReg = ReadBigMacRegister(ioBaseEnet, kRXCFG);
    WriteBigMacRegister(ioBaseEnet, kRXCFG, rxCFGReg & ~(kRxMACEnable | kRxHashFilterEnable) );

    while ( ReadBigMacRegister(ioBaseEnet, kRXCFG) & (kRxMACEnable | kRxHashFilterEnable) )
      ;

    WriteBigMacRegister(ioBaseEnet, kHASH0, hashTableMask[0]); 	// bits 15 - 0
    WriteBigMacRegister(ioBaseEnet, kHASH1, hashTableMask[1]); 	// bits 31 - 16
    WriteBigMacRegister(ioBaseEnet, kHASH2, hashTableMask[2]); 	// bits 47 - 32
    WriteBigMacRegister(ioBaseEnet, kHASH3, hashTableMask[3]); 	// bits 63 - 48

    rxCFGReg |= kRxHashFilterEnable;
    WriteBigMacRegister(ioBaseEnet, kRXCFG, rxCFGReg );
}

@end

