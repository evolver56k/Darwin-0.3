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
 * for the DECchip 21X40 
 *
 * HISTORY
 *
 * 26-Apr-95	Rakesh Dubey (rdubey) at NeXT 
 *	Created.
 * 11-Dec-95	Dieter Siegmund (dieter) at NeXT
 *	Split out 21040 and 21041 connector auto-detect logic.
 */


#import "DECchip2104x.h"

@interface DECchip2104x (Private)

- (BOOL)_allocateMemory;
- (BOOL)_initTxRing;
- (BOOL)_initRxRing;
- (BOOL)_initChip;
- (void)_resetChip;
- (void)_startTransmit;
- (void)_startReceive;
- (void)_initRegisters;
- (void)_transmitPacket:(netbuf_t)packet;
- (BOOL)_receiveInterruptOccurred;
- (BOOL)_transmitInterruptOccurred;

- (BOOL)_loadSetupFilter:(BOOL) pollingMode;
- (BOOL)_setAddressFiltering:(BOOL)pollingMode;

@end

