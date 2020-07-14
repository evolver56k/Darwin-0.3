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
 * HISTORY
 *
 * 11-Dec-95	Dieter Siegmund (dieter) at NeXT
 *	Split out 21040 and 21041 personalities.
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
#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/IOPower.h>
#import <net/etherdefs.h>
#import <string.h>
#import <driverkit/IOEthernetPrivate.h>		/* Needed for MC-setup */

#import "DECchip2104x.h"
#import "DECchip2104xRegisters.h"
#import "DECchip2104xPrivate.h"


@interface DECchip21040:DECchip2104x
{
}

- initFromDeviceDescription:(IODeviceDescription *)devDesc;
- (void)selectInterface;
- (void)getStationAddress:(enet_addr_t *)ea;
@end

