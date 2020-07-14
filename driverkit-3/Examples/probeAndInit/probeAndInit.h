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
#define MACH_USER_API		1
#define KERNEL			1
#define KERNEL_PRIVATE		0

#import <driverkit/nrw/directDevice.h>
#import <mach/mach_interface.h>
#import <kernserv/prototypes.h>
#import <driverkit/generalFuncs.h>
#import <architecture/nrw/dma_macros.h>

#define MY_DEVICE_CHANNEL		0		// local channel #
#define MY_DEVICE_BUFSIZE		32		// TE buffferSize
#define MY_DEVICE_CHAN_INTR_MASK	CI_INTR0	// accepted channel
							//    interrupts
#define MY_DEVICE_DEV_INTR_MASK		0x80		// accepred device
							//    interrupts

@interface MyDevice:IODirectDevice
{
	volatile IODevicePage	*devicePage;	// m88k register page
}

+ (BOOL)probe:deviceDescription;
- myDeviceInit;

@end
