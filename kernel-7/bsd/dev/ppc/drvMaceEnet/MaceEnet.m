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
 * Hardware independent (relatively) code for the Mace Ethernet Controller 
 *
 * HISTORY
 *
 * dd-mmm-yy	 
 *	Created.
 *
 */

#import "MaceEnetPrivate.h"

@implementation MaceEnet

/*
 * Public Factory Methods
 */
 
+ (BOOL)probe:(IODeviceDescription *)devDesc
{
    MaceEnet		*MaceEnetInstance;
    extern int		kdp_flag;

    /*
     * If bootargs: kdp bit 0 using in-kernel mace driver for early debugging,
     *              Don't probe this driver.
     */
    if( kdp_flag & 1)
    {
        return nil;
    }	

    MaceEnetInstance = [self alloc];
    return [MaceEnetInstance initFromDeviceDescription:devDesc] != nil;
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
    IORange		*ioRangeMace;

    if ([super initFromDeviceDescription:devDesc] == nil)     
    {
      IOLog( "Ethernet(Mace): [super initFromDeviceDescription] failed\n");
      return nil;
    }

    if ( [devDesc numMemoryRanges] < 3 )
    {
      IOLog( "Ethernet(Mace): Incorrect deviceDescription - 1\n\r");
      return nil;
    }

    ioRangeMace = [devDesc memoryRangeList];

    ioBaseEnet      = (IOPPCAddress) ioRangeMace[0].start;
    ioBaseEnetTxDMA = (IOPPCAddress) ioRangeMace[1].start;
    ioBaseEnetRxDMA = (IOPPCAddress) ioRangeMace[2].start;

    ioBaseEnetROM   = (IOPPCAddress) (((unsigned int) ioBaseEnet & ~0xffff) | kControllerROMOffset);

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
- (void) interruptOccurredAt:(int)irqNum
{

    [self reserveDebuggerLock];
	 
//    kprintf("IRQ = %d\n",irqNum );

    switch ( irqNum )
    {
      case kIRQEnetTxDMA:
        txWDInterrupts++;
        KERNEL_DEBUG(DBG_MACE_TXIRQ | DBG_FUNC_START, 0, 0, 0, 0, 0 );
        [self _transmitInterruptOccurred];
        KERNEL_DEBUG(DBG_MACE_TXIRQ | DBG_FUNC_END,   0, 0, 0, 0, 0 );
        [self enableInterrupt:kIRQEnetTxDMA];
        [self serviceTransmitQueue];
        break;
      case kIRQEnetRxDMA:
        KERNEL_DEBUG(DBG_MACE_RXIRQ | DBG_FUNC_START, 0, 0, 0, 0, 0 );
        [self _receiveInterruptOccurred];
        KERNEL_DEBUG(DBG_MACE_RXIRQ | DBG_FUNC_END,   0, 0, 0, 0, 0 );
        [self enableInterrupt:kIRQEnetRxDMA];
        break;
    }

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
    u_int8_t		regValue;

    KERNEL_DEBUG(DBG_MACE_TXQUEUE | DBG_FUNC_NONE, (int) pkt, (int) nb_size(pkt), 0, 0, 0 );

    /*
     * Hold the debugger lock so the debugger can't interrupt us
     */
    [self reserveDebuggerLock];

    do
    {
      /*
       * Someone is turning off the receiver before the first transmit.
       * Dont know who yet!
       */
      regValue = ReadMaceRegister( ioBaseEnet, kMacCC );
      regValue |= kMacCCEnRcv;
      WriteMaceRegister( ioBaseEnet, kMacCC, regValue );

      /* 
       * Preliminary sanity checks
       */
      if (!pkt) 
      {
        IOLog("EtherNet(Mace): transmit received NULL netbuf\n");
        continue;  
      }
      if ( ![self isRunning] ) 
      {
        nb_free(pkt);
        continue;
      }
    
      /*
       * Remove any completed packets from the Tx ring 
       */
      [self _transmitInterruptOccurred]; 
	
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
          IOLog("Ethernet(Mace) Transmit queue overflow\n");
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
      if ( ![self _initRxRing] || ![self _initTxRing] || ![self _initChip] ) 
      {
	[self setRunning:NO];
	return NO;
      }

      [self _startChip];

      [self setRelativeTimeout: WATCHDOG_TIMER_MS];

      [self enableInterrupt:kIRQEnetRxDMA];
      [self enableInterrupt:kIRQEnetTxDMA];
      [self _enableAdapterInterrupts];

      resetAndEnabled = YES;
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
      IOLog( "Ethernet(Mace): Checking for timeout - RxHead = %d RxTail = %d\n\r", rxCommandHead, rxCommandTail);
#if 0
      IOLog( "Ethernet(Mace): Rx Commands = %08x(p) Rx DMA Ptr = %08x(p)\n\r", rxDMACommandsPhys, IOGetDBDMACommandPtr(ioBaseEnetRxDMA) ); 
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
       * then force servicing of the Tx ring.
       * If we have more than one timeout interval without any transmit interrupts,
       * then force the transmitter to reset.
       */
      if ( txWDInterrupts == 0 )
      { 
        if ( ++txWDTimeouts > 1 ) txWDForceReset = YES;  
       
        IOLog( "Ethernet(Mace): Checking for timeout - TxHead = %d TxTail = %d\n\r", txCommandHead, txCommandTail); 
        [self _transmitInterruptOccurred];
      }
      else
      {
        txWDTimeouts     = 0;
        txWDInterrupts   = 0;
      }
    }
    else
    {
      txWDTimeouts     = 0;
      txWDInterrupts   = 0;
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

- (netbuf_t)allocateNetbuf
{
    netbuf_t		packet;
	
    packet = nb_alloc(NETWORK_BUFSIZE);
    if (!packet) 
    {
      IOLog("nb_alloc() returned NULL\n");
    }
    
    return packet;
}

/*-------------------------------------------------------------------------
 *
 *
 *
 *-------------------------------------------------------------------------*/

- (BOOL)enablePromiscuousMode
{
    u_int8_t		regVal;
    
    isPromiscuous = YES;
    
    [self reserveDebuggerLock];

    regVal = ReadMaceRegister( ioBaseEnet, kMacCC );
    WriteMaceRegister( ioBaseEnet, kMacCC, regVal & ~kMacCCEnRcv );
    regVal |= kMacCCProm;	
    WriteMaceRegister( ioBaseEnet, kMacCC, regVal );	
	
    [self releaseDebuggerLock];

    return YES;
}

- (void)disablePromiscuousMode
{
    u_int8_t		regVal;
    
    isPromiscuous = NO;
    
    [self reserveDebuggerLock];

    regVal = ReadMaceRegister(ioBaseEnet, kMacCC);
    WriteMaceRegister( ioBaseEnet, kMacCC, regVal & ~kMacCCEnRcv );
    regVal &= ~kMacCCProm;	
    WriteMaceRegister( ioBaseEnet, kMacCC, regVal );	
	
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
    [self _updateHashTableMask];
    [self releaseDebuggerLock];
}

- (void)removeMulticastAddress:(enet_addr_t *)addr
{
    [self reserveDebuggerLock];
    [self _removeFromHashTableMask:addr->ea_byte];
    [self _updateHashTableMask];
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

@end















