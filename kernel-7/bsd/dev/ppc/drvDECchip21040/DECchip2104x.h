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
 * Interface definition for the DECchip 2104x
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
 * 11-Dec-95	Dieter Siegmund (dieter) at NeXT
 *	Re-named 2104x.h, split out 21040 and 21041 personalities.
 * 07-Feb-96	Dieter Siegmund (dieter) at NeXT
 *	Fixed multicast.
 */

//#define PERF
#ifdef PERF
#import "perfDevice.h"
#import "netEvents.h"
#define OUR_PRODUCER_NAME	"DECchip21040"
#endif PERF

#import <driverkit/kernelDriver.h>
#import <driverkit/align.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/IOEthernet.h>
#import <driverkit/IONetbufQueue.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/ppc/IOPCIDevice.h>
#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/IOPower.h>
#import <net/etherdefs.h>
#import <string.h>
#import <driverkit/IOEthernetPrivate.h>		/* Needed for MC-setup */

#undef DEBUG

#import "DECchip2104xShared.h"
#import "DECchip2104xRegisters.h"

typedef unsigned long         IOPPCAddress;

@interface DECchip2104x:IOEthernet <IOPower>
{
    IOPPCAddress	ioBase;
    unsigned short	irq;
    enet_addr_t		myAddress;
    IONetwork		*networkInterface;
    IONetbufQueue	*transmitQueue;
    BOOL		isPromiscuous;
    BOOL		multicastEnabled;
    BOOL		isInterruptMasked;

    BOOL		resetAndEnabled;
    
    netbuf_t		txNetbuf[DEC_21X40_TX_RING_LENGTH];
    netbuf_t		rxNetbuf[DEC_21X40_RX_RING_LENGTH];
    
    rxDescriptorStruct	*rxRing;	/* RX descriptor ring ptr */
    txDescriptorStruct	*txRing; 	/* TX descriptor ring ptr */

    unsigned int	txPutIndex;	/* Transmit ring descriptor index */
    unsigned int	txDoneIndex;
    unsigned int	txNumFree;
    unsigned int	txIntCount;	/* used to decide when to do tx int */
    unsigned int	rxDoneIndex;    /* Receive ring descriptor index */

    netbuf_t		KDB_txBuf;
    
    void		*memoryPtr;
    unsigned int	memorySize;
    
    setupBuffer_t 	*setupBuffer;
    unsigned long	setupBufferPhysical;
    
    connector_t		connector;
    
    csrRegUnion		interruptMask;
    csrRegUnion		operationMode;
#ifdef PERF
    perfQueue_t *		perfQ;
    perfProducerId_t		ourProducerId;
    perfEventHdr_t 		ev_p;
#endif PERF
}

+ (BOOL)probe:(IOPCIDevice *)devDesc;
- initFromDeviceDescription:(IODeviceDescription *)devDesc;

- free;
- (void)serviceTransmitQueue;
- (void)transmit:(netbuf_t)pkt;
- (BOOL)resetAndEnable:(BOOL)enable;

- (void)interruptOccurred;
- (void)timeoutOccurred;

- (BOOL)enableMulticastMode;
- (void)disableMulticastMode;
- (void)addMulticastAddress:(enet_addr_t *)addr;
- (void)removeMulticastAddress:(enet_addr_t *)addr;

- (BOOL)enablePromiscuousMode;
- (void)disablePromiscuousMode;

- (void)selectInterface;
- (void)getStationAddress:(enet_addr_t *)ea;

/*
 * Queue interface
 */
- (int) transmitQueueSize;
- (int) transmitQueueCount;
- (int) pendingTransmitCount;

/*
 * Power management methods. 
 */
- (IOReturn)getPowerState:(PMPowerState *)state_p;
- (IOReturn)setPowerState:(PMPowerState)state;
- (IOReturn)getPowerManagement:(PMPowerManagementState *)state_p;
- (IOReturn)setPowerManagement:(PMPowerManagementState)state;

@end

