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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * PCI direct device implementation.
 *
 * HISTORY
 *
 * 19 Aug 1994  Dean Reece at NeXT
 *	Added class methods.
 *
 * 13 May 1994	Dean Reece at NeXT
 *	Created.
 *
 */

#import <driverkit/i386/PCI.h>
#import <driverkit/i386/PCIKernBus.h>
#import <driverkit/i386/IOPCIDirectDevice.h>
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/IODeviceDescription.h>

static inline id
getThePCIBus(void)
{
    return [KernBus lookupBusInstanceWithName:"PCI" busId:0];
}

@implementation IODirectDevice(IOPCIDirectDevice)

/*
 * Determine whether or not the associated device is connected to a PCI 
 * bus. Returns YES if so, else returns NO.
 */
+ (BOOL)isPCIPresent
{
	id	thePCIBus = getThePCIBus();
	
	if (thePCIBus == nil) return NO;
	return [thePCIBus isPCIPresent];
}

- (BOOL)isPCIPresent
{
	id	thePCIBus = getThePCIBus();

	if (thePCIBus == nil) return NO;
	return [thePCIBus isPCIPresent];
}


/*
 * Reads the device's entire configuration space.  Returns IO_R_SUCCESS if
 * successful.  If this method fails, the driver should make no assumptions
 * about the state of the data returned in the IOPCIConfigSpace struct.
 */
+ (IOReturn)getPCIConfigSpace: (IOPCIConfigSpace *) configSpace
	withDeviceDescription: descr
{
	unsigned char	devNum, funNum, busNum;
	unsigned long	*ptr;
	int		address;
	IOReturn	ret;
	id	thePCIBus = getThePCIBus();

	if (![self isPCIPresent]) return IO_R_NO_DEVICE;

	ret = [descr getPCIdevice: &devNum function: &funNum bus: &busNum];
	if (ret != IO_R_SUCCESS) return ret;

	ptr = (unsigned long *)configSpace;
	for (address=0x00; address<0x100; address+=0x04) {
		ret = [thePCIBus getRegister: address
				      device: devNum
				    function: funNum
					 bus: busNum
					data: ptr++];
		if (ret != IO_R_SUCCESS) return ret;
	}
	return IO_R_SUCCESS;	
}

- (IOReturn)getPCIConfigSpace: (IOPCIConfigSpace *) configSpace
{
	return [[self class] getPCIConfigSpace: configSpace
			withDeviceDescription:[self deviceDescription]];
}


/*
 * Writes the device's entire configuration space.  Returns IO_R_SUCCESS if
 * successful.  If this method fails, the driver should make no assumptions
 * about the state of the device's configuration space.
 */
+ (IOReturn)setPCIConfigSpace: (IOPCIConfigSpace *) configSpace
	withDeviceDescription: descr
{
	unsigned char	devNum, funNum, busNum;
	unsigned long	*ptr;
	int		address;
	IOReturn	ret;
	id	thePCIBus = getThePCIBus();

	if (![self isPCIPresent]) return IO_R_NO_DEVICE;

	ret = [descr getPCIdevice: &devNum function: &funNum bus: &busNum];
	if (ret != IO_R_SUCCESS) return ret;

	ptr = (unsigned long *)configSpace;
	for (address=0x00; address<0x100; address+=0x04) {
		ret = [thePCIBus setRegister: address
				      device: devNum
				    function: funNum
					 bus: busNum
					data: *ptr++];
		if (ret != IO_R_SUCCESS) return ret;
	}
	return IO_R_SUCCESS;	
}

- (IOReturn)setPCIConfigSpace: (IOPCIConfigSpace *) configSpace
{
	return [[self class] setPCIConfigSpace: configSpace
			withDeviceDescription:[self deviceDescription]];
}


/*
 * Reads from the device's configuration space.  All access are 32 bits wide
 * and the address must be aligned as such.
 */
+ (IOReturn)getPCIConfigData: (unsigned long *) data
		  atRegister: (unsigned char) address
       withDeviceDescription: descr
{
	unsigned char devNum, funNum, busNum;
	IOReturn      ret;
	id	thePCIBus = getThePCIBus();

	if (![self isPCIPresent]) return IO_R_NO_DEVICE;

	ret = [descr getPCIdevice: &devNum function: &funNum bus: &busNum];
	if (ret != IO_R_SUCCESS) return ret;

	return [thePCIBus getRegister: address
			       device: devNum
			     function: funNum
				  bus: busNum
				 data: data];
}

- (IOReturn)getPCIConfigData: (unsigned long *) data
		  atRegister: (unsigned char) address
{
	return [[self class] getPCIConfigData: data atRegister: address
			withDeviceDescription:[self deviceDescription]];
}


/*
 * Writes to the device's configuration space.  All access are 32 bits wide
 * and the address must be aligned as such.  This method reads the register
 * back after setting it, and returns that value.  If this method fails, it
 * will return PCI_DEFAULT_DATA
 */
+ (IOReturn)setPCIConfigData: (unsigned long) data
		  atRegister: (unsigned char) address
       withDeviceDescription: descr
{
	unsigned char devNum, funNum, busNum;
	IOReturn      ret;
	id	thePCIBus = getThePCIBus();

	if (![self isPCIPresent]) return IO_R_NO_DEVICE;

	ret = [descr getPCIdevice: &devNum function: &funNum bus: &busNum];
	if (ret != IO_R_SUCCESS) return ret;

	return [thePCIBus setRegister: address
			       device: devNum
			     function: funNum
				  bus: busNum
				 data: data];
}

- (IOReturn)setPCIConfigData: (unsigned long) data
		  atRegister: (unsigned char) address
{
	return [[self class] setPCIConfigData: data atRegister: address
			withDeviceDescription:[self deviceDescription]];
}

@end

