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
 * Ethernet Device class private declarations.
 *
 * HISTORY
 *
 * 10 Mar 1993 David E. Bohman at NeXT
 *	Created.
 */

#ifdef	KERNEL

#import <driverkit/IOEthernet.h>

#ifndef sparc
void reserveDebuggerLock(id debuggerDeviceObject);
void releaseDebuggerLock(id debuggerDeviceObject);
#endif 

#if sparc
void
    en_reset(BOOL enable);
#endif sparc
/*
 * A linked list of these structs is kept in _multicastQueue. Each 
 * represents one "legal" multicast address to which we will respond. 
 */
typedef struct {
	enet_addr_t	address;
	queue_chain_t	link;
	int		refCount;
} enetMulti_t;
		
@interface IOEthernet(EthernetDebugger)

- (void)registerAsDebuggerDevice;
#ifndef sparc
- (void)reserveDebuggerLock;
- (void)releaseDebuggerLock;
#endif

@end

@interface IOEthernet(PrivateMethods)

- (queue_head_t *)multicastQueue;
- (void)enableMulticast:(enet_addr_t *)addrs;
- (void)disableMulticast:(enet_addr_t *)addrs;
- (void)enqueueMulticast:(enet_addr_t *)addrs;
- (void)dequeueMulticast:(enet_addr_t *)addrs;
- (enetMulti_t *)searchMulti:(enet_addr_t *)addrs;

@end

#endif
