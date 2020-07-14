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
 * Hardware independent (relatively) code for the BMac Ethernet Controller 
 *
 * HISTORY
 *
 * dd-mmm-yy	 
 *	Created.
 *
 */

#import "BMacEnetPrivate.h"

@implementation BMacEnet

/*
 * Public Factory Methods
 */
 
+ (BOOL)probe:(IOTreeDevice *)devDesc
{
    BMacEnet		*BMacEnetInstance;

    BMacEnetInstance = [self alloc];
    return [BMacEnetInstance initFromDeviceDescription:devDesc] != nil;
}

/*
 * Public Instance Methods
 */

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- initFromDeviceDescription:(IOTreeDevice *)devDesc
{
    IOTreeDevice	*devDescHeathrow;
    IORange		*ioRangeBMac;
    IORange		*ioRangeHeathrow;
    
    if ([super initFromDeviceDescription:devDesc] == nil)     
    {
      IOLog( "Ethernet(BMac): [super initFromDeviceDescription] failed\n");
      return nil;
    }

    if ( [devDesc numMemoryRanges] < 3 )
    {
      IOLog( "Ethernet(BMac): Incorrect deviceDescription - 1\n\r");
      return nil;
    }

    ioRangeBMac = [devDesc memoryRangeList];

    ioBaseEnet      = (IOPPCAddress) ioRangeBMac[0].start;
    ioBaseEnetTxDMA = (IOPPCAddress) ioRangeBMac[1].start;
    ioBaseEnetRxDMA = (IOPPCAddress) ioRangeBMac[2].start;

    devDescHeathrow = [[devDesc parent] deviceDescription];
  
    if ( [devDescHeathrow numMemoryRanges] < 1 )
    {  
      IOLog( "Ethernet(BMac): Incorrect deviceDescription - 2\n\r");
      return nil;
    } 

    ioRangeHeathrow = [devDescHeathrow memoryRangeList];

    ioBaseHeathrow  = (IOPPCAddress) ioRangeHeathrow[0].start;

    sromAddressBits   = 6;
    enetAddressOffset = 20;
    phyId             = -1;
    phyMIIDelay	      = MII_DEFAULT_DELAY;

    if ( ![self resetAndEnable:NO] ) 
    {
      [self free];
      return nil;
    }

    [self _getStationAddress:&myAddress]; 

    if ([self _allocateMemory] == NO) 
    {
      [self free];
      return nil;
    }

    if ( ![self resetAndEnable:YES] ) 
    {
      [self free];
      return nil;
    }

    isPromiscuous = NO;
    multicastEnabled = NO;

    networkInterface = [super attachToNetworkWithAddress:myAddress];

    [networkInterface getIONetworkIfnet]->if_eflags |= IFEF_DVR_REENTRY_OK;

    return self;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- free
{
    int		i;
    
    [self clearTimeout];
    
    [self _resetChip];
    
    if (networkInterface)
	[networkInterface free];

    for (i = 0; i < rxMaxCommand; i++)
    	if (rxNetbuf[i])  nb_free(rxNetbuf[i]);

    for (i = 0; i < txMaxCommand; i++)
    	if (txNetbuf[i]) nb_free(txNetbuf[i]);
    
    return [super free];
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)interruptOccurredAt:(int)irqNum
{    

    [self reserveDebuggerLock];

    switch ( irqNum )
    {
      /*
       * On the transmit side, we use the chipset interrupt. Using the transmit
       * DMA interrupt (or having multiple transmit DMA entries) would allows us to 
       * send the next frame to the chipset prior the transmit fifo going empty. 
       * However, this aggrevates a BMac chipset bug where the next frame going out
       * gets corrupted (first two bytes lost) if the chipset had to retry the previous
       * frame.
       */ 
      case kIRQEnetDev:
      case kIRQEnetTxDMA:
        txWDInterrupts++;
        KERNEL_DEBUG(DBG_BMAC_TXIRQ | DBG_FUNC_START, 0, 0, 0, 0, 0 );
        [self _transmitInterruptOccurred];
        [self serviceTransmitQueue];
        KERNEL_DEBUG(DBG_BMAC_TXIRQ | DBG_FUNC_END,   0, 0, 0, 0, 0 );
        break;
      case kIRQEnetRxDMA:
        KERNEL_DEBUG(DBG_BMAC_RXIRQ | DBG_FUNC_START, 0, 0, 0, 0, 0 );
        [self _receiveInterruptOccurred];
        KERNEL_DEBUG(DBG_BMAC_RXIRQ | DBG_FUNC_END,   0, 0, 0, 0, 0 );
        break;
    }

    [self enableAllInterrupts];

    [self releaseDebuggerLock];
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void) serviceTransmitQueue
{
    netbuf_t			packet;
    u_int32_t			i;

    while ( 1 )
    {
      if ( ![transmitQueue count] )
      {
        break;
      }

      i = txCommandTail + 1;
      if ( i >= txMaxCommand ) i = 0;
      if ( i == txCommandHead )
      {
        break;
      }
      packet = [transmitQueue dequeue];
      [self _transmitPacket:packet];
    }

}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)transmit:(netbuf_t)pkt
{
    u_int32_t 		i;
    u_int32_t           n;

    KERNEL_DEBUG(DBG_BMAC_TXQUEUE | DBG_FUNC_NONE, (int) pkt, (int) nb_size(pkt), 0, 0, 0 );

    /*
     * Hold the debugger lock so the debugger can't interrupt us
     */
    [self reserveDebuggerLock];
 
    do
    {
      /* 
       * Preliminary sanity checks
       */
      if (!pkt) 
      {
        IOLog("EtherNet(BMac): transmit received NULL netbuf\n");
        continue;  
      }
      if ( ![self isRunning] ) 
      {
        nb_free(pkt);
        continue;
      }

#if 0
      /*
       * Remove any completed packets from the Tx ring 
       */
      if ( chipId >= kCHIPID_PaddingtonXmitStreaming )
      {
        [self _transmitInterruptOccurred]; 
      }
#endif
	
      /*
       * Refill the Tx ring from the Transmit waiting list
       */
      [self serviceTransmitQueue];

      /*
       * If the Transmit waiting list is not empty or the Tx ring is
       * full, add the new packet to the waiting list.
       */
      n = [transmitQueue count];
      if ( n > 0 )
      {
        if ( n >= [transmitQueue maxCount] )
        {
          IOLog("Ethernet(BMac): Transmit queue overflow\n");
        }

        [transmitQueue enqueue:pkt];
        continue;
      }
      i = txCommandTail + 1;
      if ( i >= txMaxCommand ) i = 0;
      if ( i == txCommandHead )
      {
        [transmitQueue enqueue:pkt];
        continue;
      }

      /*
       * If there is space on the Tx ring, add the packet directly to the
       * ring
       */
      [self _transmitPacket:pkt];
    }
    while( 0 );

    [self releaseDebuggerLock];
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (int) transmitQueueSize
{
    return (TRANSMIT_QUEUE_SIZE);
}

- (int) transmitQueueCount
{
    return ([transmitQueue count]);
}
/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)resetAndEnable:(BOOL)enable
{
    resetAndEnabled = NO;
    [self clearTimeout];
    [self disableAllInterrupts];
    [self _resetChip];
    
    if (enable) 
    {
      if (![self _initRxRing] || ![self _initTxRing])	
      {
        return NO;
      }

      if ( phyId != 0xff )
      {
          [self miiInitializePHY:phyId];
      }

      if ([self _initChip] == NO) 
      {
	[self setRunning:NO];
	return NO;
      }

      [self _startChip];
	
      [self setRelativeTimeout: WATCHDOG_TIMER_MS];

      [self enableAllInterrupts]; 
      [self _enableAdapterInterrupts];

      resetAndEnabled = YES;

      [self _sendDummyPacket];      
    }

    [self setRunning:enable];

    return YES;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (void)timeoutOccurred
{
    u_int32_t		dmaStatus;
    u_int16_t		phyStatus;
    u_int16_t		linkStatus;
    u_int16_t		phyStatusChange;
    BOOL		fullDuplex = NO;

    if ( ![self isRunning] ) 
    {    
      return;
    }

    [self reserveDebuggerLock];

    /*
     * Check for DMA shutdown on receive channel
     */
    dmaStatus = IOGetDBDMAChannelStatus( ioBaseEnetRxDMA );
    if ( !(dmaStatus & kdbdmaActive) )
    {
      IOLog( "Ethernet(BMac): Checking for timeout - RxHead = %d RxTail = %d\n\r", rxCommandHead, rxCommandTail); 
#if 0
      IOLog( "Ethernet(BMac): Rx Commands = %08x(p) Rx DMA Ptr = %08x(p)\n\r", rxDMACommandsPhys, IOGetDBDMACommandPtr(ioBaseEnetRxDMA) ); 
      [self _dumpDesc:(void *)rxDMACommands Size:rxMaxCommand * sizeof(enet_dma_cmd_t)];
#endif 
      [self _receiveInterruptOccurred];
    } 

    /*
     * If there are pending entries on the Tx ring
     */
    if ( txCommandHead != txCommandTail )
    {
      /* 
       * If we did not service the Tx ring during the last timeout interval,
       * then force servicing of the Tx ring
       */
      if ( txWDInterrupts == 0 )
      {      
        if ( txWDCount++ > 0 )
        {
          if ( [self _transmitInterruptOccurred] == NO )
          {
              IOLog( "Ethernet(BMac): Checking for timeout - TxHead = %d TxTail = %d\n\r", txCommandHead, txCommandTail); 
              [self _restartTransmitter];
          }
        }
      }
      else
      {
        txWDInterrupts   = 0;
        txWDCount        = 0;
      }
    }
    else
    {      
      txWDInterrupts   = 0;
      txWDCount        = 0;
    }

    if ( phyId != 0xff )
    {
        if ( [self miiReadWord:&phyStatus reg:MII_STATUS phy:phyId] == YES )
        {
            phyStatusChange = (phyStatusPrev ^ phyStatus) & (MII_STATUS_LINK_STATUS | MII_STATUS_NEGOTIATION_COMPLETE);
            if ( phyStatusChange )
            {
                if ( (phyStatus & MII_STATUS_LINK_STATUS) && (phyStatus & MII_STATUS_NEGOTIATION_COMPLETE ) )
                {
                    if ( (phyType & MII_ST10040_MASK) == MII_ST10040_ID )
                    {
                        [self miiReadWord:&linkStatus reg:MII_ST10040_CHIPST phy:phyId];

                        fullDuplex =  (linkStatus & MII_ST10040_CHIPST_DUPLEX) ? YES : NO;

                        IOLog( "Ethernet(BMac): Link is up at %sMb - %s Duplex\n\r",
                              (linkStatus & MII_ST10040_CHIPST_SPEED)  ? "100" : "10",
                              (fullDuplex)                             ? "Full" : "Half" );                        
                    }
                    else if ( (phyType & MII_DP83843_MASK) == MII_DP83843_ID )
                    {
                        [self miiReadWord:&linkStatus reg:MII_DP83843_PHYSTS phy:phyId];

                        fullDuplex =  (linkStatus & MII_DP83843_PHYSTS_DUPLEX) ? YES : NO;

                        IOLog( "Ethernet(BMac): Link is up at %sMb - %s Duplex\n\r",
                              (linkStatus & MII_DP83843_PHYSTS_SPEED10) ? "10" : "100",
                              (fullDuplex)                              ? "Full" : "Half" );           
                     }  
                     else
                     {
                        fullDuplex = NO ;
                        IOLog( "Ethernet(BMac): Link is up\n\r" );
                     }

                     if ( fullDuplex != isFullDuplex )
                     {
                         [self _setDuplexMode: fullDuplex];    
                     }
                }
                else
                {
                    IOLog( "Ethernet(BMac): Link is down.\n\r" );
                }
                phyStatusPrev = phyStatus;
            }
        }
    }          

    /*
     * Restart the watchdog timer
     */
    [self setRelativeTimeout: WATCHDOG_TIMER_MS];

    [self releaseDebuggerLock];

}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)enablePromiscuousMode
{
    u_int16_t		rxCFGVal;
    
    isPromiscuous = YES;
    
    [self reserveDebuggerLock];

    /*
     * Turn off the receiver and wait for the chipset to acknowledge
     */
    rxCFGVal = ReadBigMacRegister(ioBaseEnet, kRXCFG);
    WriteBigMacRegister(ioBaseEnet, kRXCFG, rxCFGVal & ~kRxMACEnable );
    while( ReadBigMacRegister(ioBaseEnet, kRXCFG) & kRxMACEnable )
      ;

    /*
     * Set promiscuous mode and restore receiver state
     */
    rxCFGVal |= kRxPromiscEnable;	
    WriteBigMacRegister( ioBaseEnet, kRXCFG, rxCFGVal );		

    [self releaseDebuggerLock];
    
    return YES;
}

- (void)disablePromiscuousMode
{
    u_int16_t		rxCFGVal;
    
    isPromiscuous = NO;
    
    [self reserveDebuggerLock];

    /*
     * Turn off the receiver and wait for the chipset to acknowledge
     */
    rxCFGVal = ReadBigMacRegister(ioBaseEnet, kRXCFG);
    WriteBigMacRegister(ioBaseEnet, kRXCFG, rxCFGVal & ~kRxMACEnable );
    while( ReadBigMacRegister(ioBaseEnet, kRXCFG) & kRxMACEnable )
      ;

    /*
     * Reset promiscuous mode and restore receiver state
     */
    rxCFGVal &= ~kRxPromiscEnable;	
    WriteBigMacRegister( ioBaseEnet, kRXCFG, rxCFGVal );		

    [self releaseDebuggerLock];
}

- (BOOL)enableMulticastMode
{
    multicastEnabled = YES;
    return YES;
}
	 
- (void)disableMulticastMode
{
    multicastEnabled = NO;
}

- (void)addMulticastAddress:(enet_addr_t *)addr
{
    multicastEnabled = YES;
    [self reserveDebuggerLock];
    [self _addToHashTableMask:addr->ea_byte];
    [self _updateBMacHashTableMask];
    [self releaseDebuggerLock];
}

- (void)removeMulticastAddress:(enet_addr_t *)addr
{
    [self reserveDebuggerLock];
    [self _removeFromHashTableMask:addr->ea_byte];
    [self _updateBMacHashTableMask];
    [self releaseDebuggerLock];
}

/*
 * Kernel Debugger Support 
 */
- (void)sendPacket:(void *)pkt length:(unsigned int)pkt_len
{
    [self _sendPacket:(void *)pkt length:(unsigned int)pkt_len];
}

- (void)receivePacket:(void *)pkt length:(unsigned int *)pkt_len  timeout:(unsigned int)timeout
{
    [self _receivePacket:(void *)pkt length:(unsigned int *)pkt_len  timeout:(unsigned int)timeout];
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

- (void)reserveDebuggerLock
{
    if ( debuggerLockCount++ == 0 )
    {
        [super reserveDebuggerLock];
    }
}   

- (void)releaseDebuggerLock
{
    if ( --debuggerLockCount == 0 )
    {
        [super releaseDebuggerLock];
    }
}   


@end















