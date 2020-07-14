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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * IODeviceMaster class description.
 *
 * HISTORY
 * 13-Jan-98	Martin Minow at Apple
 *	Added createMachPort
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#ifndef	KERNEL

#import <objc/Object.h>
#import <driverkit/driverTypes.h>
#import <mach/port.h>

/*
 * IODeviceMaster class. Used in user space only, to do Get/Set parameter
 * type operations. 
 *
 * General return values for all methods in this class which return an 
 * IOReturn:
 *
 * IO_R_NO_DEVICE - IOObjectNumber out of range of known device objects.
 * IO_R_OFFLINE   - invalid IOObjectNumber, but not out of range. There
 *		    can be 'holes' in the IOObjectNumber space; IO_R_OFFLINE
 *		    indicates an IOObjectNumber assicated with such a 
 *		    non-existent device object.
 * IO_R_UNSUPPORTED - The device named by the IOObjectNumber does not
 *		    implement the getIntValues (etc.) selector or does
 *		    not implement a createMachPort service.
 *
 * All other return codes are device-specific.
 */

@interface IODeviceMaster : Object
{
@private
	port_t	_deviceMasterPort;
	int	_IODeviceMaster_reserved[4];
}

/*
 * Obtain an instance of IODeviceMaster.
 */
+ new;

/*
 * Free an instance of IODeviceMaster.
 */
- free;

/*
 * Get device type and device name for specified IOObjectNumber.
 */
- (IOReturn)lookUpByObjectNumber	: (IOObjectNumber)objectNumber
			     deviceKind : (IOString *)deviceKind
			     deviceName : (IOString *)deviceName;
				   

/*
 * Get IOObjectNumber and device type for specified deviceName.
 */
- (IOReturn)lookUpByDeviceName		: (IOString)deviceName
			   objectNumber : (IOObjectNumber *)objectNumber
			     deviceKind : (IOString *)deviceKind;
			     
/*
 * Get/set parameter methods used at user level to communicate with
 * kernel-level drivers.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count;	// in/out

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count;	// in/out
	
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;
/*
 * - serverConnect:taskPort: notifies an object that is a sub class of
 * IODevice that a client task is interested in communicating with it.
 * The IODevice superclass implements this method returning IO_R_UNSUPPORTED.
 * If a subclass, such as the IOSCSIController class, provices direct RPC
 * service, it will override serverConnect:taskPort: and, when called, create a
 * port, returning IO_R_SUCCESS to IODeviceMaster, which will return send right
 * to the non-privileged client. The client and IODevice are subsequently
 * responsible for all port management.
 */
- (IOReturn) serverConnect: (port_t *)       machPort
	      objectNumber: (IOObjectNumber) objectNumber;

- (IOReturn) lookUpByStringPropertyList:(const char *)values
            results:(char *)results
            maxLength:(unsigned int)length;

- (IOReturn) getStringPropertyList:(IOObjectNumber) objectNumber
            names:(const char *)names
            results:(char *) results
            maxLength:(unsigned int)length;

- (IOReturn) getByteProperty:(IOObjectNumber) objectNumber
            name:(const char *)name
            results:(char *) results
            maxLength:(unsigned int *)length;


@end	/* IODeviceMaster */

#endif	/* !KERNEL */

