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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Network interface class.
 *
 * HISTORY
 *
 * 10 December 1992 David E. Bohman at NeXT
 *	Created.
 */

#ifdef	KERNEL

#import <objc/Object.h>
#import <driverkit/IODevice.h>
#import <net/netbuf.h>

@interface IONetwork:Object
{
@private
    struct ifnet * _ifp;
    id		_device;
    int		_IONetwork_reserved[4];
}

- initForNetworkDevice:device
		name:(const char *)name
		unit:(unsigned int)unit
		type:(const char *)type
		maxTransferUnit:(unsigned int)mtu
		flags:(unsigned int)flags;

- (int)handleInputPacket:(netbuf_t)pkt
		extra:(void *)extra;

- (unsigned)inputPackets;
- (void)incrementInputPackets;
- (void)incrementInputPacketsBy:(unsigned)increment;

- (unsigned)inputErrors;
- (void)incrementInputErrors;
- (void)incrementInputErrorsBy:(unsigned)increment;

- (unsigned)outputPackets;
- (void)incrementOutputPackets;
- (void)incrementOutputPacketsBy:(unsigned)increment;

- (unsigned)outputErrors;
- (void)incrementOutputErrors;
- (void)incrementOutputErrorsBy:(unsigned)increment;

- (unsigned)collisions;
- (void)incrementCollisions;
- (void)incrementCollisionsBy:(unsigned)increment;
- (struct ifnet *)getIONetworkIfnet;
@end

@protocol IONetworkDeviceMethods

- (int)finishInitialization;

- (int)outputPacket:(netbuf_t)pkt
	    address:(void *)addrs;

- (netbuf_t)allocateNetbuf;

- (int)performCommand:(const char *)command
	    data:(void *)data;

@end

#endif	KERNEL
