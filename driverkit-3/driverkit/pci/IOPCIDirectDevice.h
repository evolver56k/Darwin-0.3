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
 * PCI direct device interface.
 *
 * HISTORY
 *
 * 19 Aug 1994  Dean Reece at NeXT
 *	Added class methods.
 *
 * 13 May 1994	Dean Reece at NeXT
 *	Created.
 */

#import <driverkit/IODirectDevice.h>
#import <driverkit/i386/PCI.h>		/* IOPCIConfigSpace defined */

@interface IODirectDevice (IOPCIDirectDevice)

/*
 * Returns YES if PCI Bus support is enabled.  Returns NO otherwise.
 */
+ (BOOL)isPCIPresent;
- (BOOL)isPCIPresent;

/*
 * Reads the device's entire configuration space.  Returns IO_R_SUCCESS if
 * successful.  If this method fails, the driver should make no assumptions
 * about the state of the data returned in the IOPCIConfigSpace struct.
 */
+ (IOReturn)getPCIConfigSpace: (IOPCIConfigSpace *) configSpace
	withDeviceDescription: descr;

- (IOReturn)getPCIConfigSpace: (IOPCIConfigSpace *) configSpace;

/*
 * Writes the device's entire configuration space.  Returns IO_R_SUCCESS if
 * successful.  If this method fails, the driver should make no assumptions
 * about the state of the device's configuration space.
 */
+ (IOReturn)setPCIConfigSpace: (IOPCIConfigSpace *) configSpace
	withDeviceDescription: descr;

- (IOReturn)setPCIConfigSpace: (IOPCIConfigSpace *) configSpace;

/*
 * Reads from the device's configuration space.  All access are 32 bits wide
 * and the address must be aligned as such.
 */
+ (IOReturn)getPCIConfigData: (unsigned long *) data
		  atRegister: (unsigned char) address
       withDeviceDescription: descr;

- (IOReturn)getPCIConfigData: (unsigned long *) data
		  atRegister: (unsigned char) address;

/*
 * Writes to the device's configuration space.  All access are 32 bits wide
 * and the address must be aligned as such.
 */
+ (IOReturn)setPCIConfigData: (unsigned long) data
		  atRegister: (unsigned char) address
       withDeviceDescription: descr;

- (IOReturn)setPCIConfigData: (unsigned long) data
		  atRegister: (unsigned char) address;
@end
