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
 * Interface for hardware dependent (relatively) code 
 * for the Mace Ethernet chip 
 *
 * HISTORY
 *
 * 11/22/97	R. Berkoff 
 *	Created.
 */


#import "MaceEnet.h"

void WriteMaceRegister( IOPPCAddress ioEnetBase, u_int32_t reg_offset, u_int8_t data);
volatile u_int8_t ReadMaceRegister( IOPPCAddress ioEnetBase, u_int32_t reg_offset);


@interface MaceEnet(Private)
- (BOOL)_allocateMemory;
- (BOOL)_initTxRing;
- (BOOL)_initRxRing;

- (BOOL)_initChip;
- (void)_resetChip;
- (void)_disableAdapterInterrupts;
- (void)_enableAdapterInterrupts;

- (void)_startChip;
- (void)_restartChip;
- (void)_stopReceiveDMA;
- (void)_stopTransmitDMA;
- (BOOL)_transmitPacket:(netbuf_t)packet;

- (BOOL)_receiveInterruptOccurred;
- (BOOL)_receivePackets:(BOOL)fDebugger;
- (BOOL)_transmitInterruptOccurred;
- (BOOL) _updateDescriptorFromNetBuf:(netbuf_t) nb Desc:(enet_dma_cmd_t *)desc ReceiveFlag:(BOOL) isReceive;

/*
 * Kernel Debugger
 */
- (void)_sendPacket:(void *)pkt length:(unsigned int)pkt_len;
- (void)_receivePacket:(void *)pkt length:(unsigned int *)pkt_len timeout:(unsigned int)timeout;
-(void) _packetToDebugger: (netbuf_t) packet;

- (void)_getStationAddress:(enet_addr_t *)ea;
- (void)_addToHashTableMask:(u_int8_t *)addr;
- (void)_removeFromHashTableMask:(u_int8_t *)addr;
- (void)_updateHashTableMask;
- (void)_dumpRegisters;
- (void)_dumpDesc:(void *)addr Size:(u_int32_t)size;


@end

