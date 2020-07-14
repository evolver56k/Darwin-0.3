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
 * probeAndInit.m.
 * +probe: and init example, nrw kernel driver.
 */

#import "probeAndInit.h"

/*
 * Probe and initialization of direct device, kernel mode.
 */

static int myDeviceNum = 0;

@implementation MyDevice

/*
 * Probe:deviceMaster: is called out during early system autoconfig when
 * the autoconfig module finds a hardware device with a valid mapping
 * in the internalDevMap[] table.
 */
+ (BOOL)probe:deviceDescription
{
	MyDevice *myDev;
	char devName[20];
	
	/*
	 * Instantiate and set common IODevice instance variables.
	 */
	myDev = [self alloc];
	[myDev setDeviceDescription:deviceDescription];
	[myDev setUnit:myDeviceNum];
	sprintf(devName, "myDevice%d", myDeviceNum++);
	[myDev setName:devName];
	[myDev setDeviceKind:"Example Driver"];
	
	/*
	 * Proceed with device-specific initialization.
	 */
	if([myDev myDeviceInit]) {
		return YES;
	}
	else {
		return NO;
	}
}

/*
 * Device-specific initialization. Returns nil on error. Assumes valid 
 * deviceDescription on entry.
 */
- myDeviceInit
{
	IOReturn rtn;
	unsigned intr;
	char location[15];
	
	/*
	 * ...Initialize instance variables here...
	 */
	 
	/*
	 * Perform one-time only hardware initialization. First set up 
	 * register pointer.
	 */
	rtn = [self mapDevicePage:(vm_offset_t *)&devicePage
		anywhere:YES
		cache:IO_CacheOff];
	if(rtn) {
		IOLog("MyDevice: Error on mapDevicePage (%s)\n", 
			[self stringFromReturn:rtn]);
		return nil;
	}
	
	sprintf(location, "0x%x", (unsigned)devicePage);
	[self setLocation:location];
	
	/*
	 * Clear and disable interrupts.
	 */
	intr = chan_intr_cause(devicePage, MY_DEVICE_CHANNEL);
	set_chan_intr_cause(devicePage, MY_DEVICE_CHANNEL, intr);
	set_chan_intr_mask(devicePage, MY_DEVICE_CHANNEL, 0);
	set_dev_intr_mask(devicePage, 0);
	
	/*
	 * Prepare for interrupt notification.
	 */
	rtn = [self attachInterrupt];
	if(rtn) {
		IOLog("%s attachInterrupt returned %s\n", 
			[self stringFromReturn:rtn]);
		return nil;
	}
		
	/*
	 * Get a DMA channel.
	 */
	rtn = [self attachChannel:MY_DEVICE_CHANNEL
		streamMode:NO
		bufferSize:MY_DEVICE_BUFSIZE];
	if(rtn) {
		IOLog("MyDevice: Error on attachChannel (%s)\n", 
			[self stringFromReturn:rtn]);
		return nil;
	}

	/*
	 * ...configure device-specific hardware...
	 */
	 
	/*
	 * Enable interrupts at the channel, device, and kernel level.
	 */
	set_chan_intr_mask(devicePage, MY_DEVICE_CHANNEL,
		MY_DEVICE_CHAN_INTR_MASK);
	set_dev_intr_mask(devicePage, MY_DEVICE_DEV_INTR_MASK);
	[self enableInterruptsForChannel:MY_DEVICE_CHANNEL];

	/*
	 * Register with IODevice-level code.
	 */
	[super init];
	[self registerDevice];
	return self;
}

@end
