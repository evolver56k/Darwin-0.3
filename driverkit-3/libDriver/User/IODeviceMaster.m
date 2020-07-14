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
 * IODeviceMaster.m
 *
 * HISTORY
 * 13-Jan-98	Martin Minow at Apple
 *	Added createMachPort
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/IODeviceMaster.h>
#import <driverkit/driverServer.h>
#import <mach/port.h>

/*
 * IODeviceMaster class. Used in user space only, to do Get/Set parameter
 * type operations. 
 */

static IODeviceMaster *thisTasksId = nil;

@implementation IODeviceMaster

/*
 * Alloc and init (if necessary) an instance of IODeviceMaster.
 */
+ new
{	
	if(thisTasksId == nil) {
		thisTasksId = [self alloc];
		thisTasksId->_deviceMasterPort = device_master_self();
	}
	return thisTasksId;
}

/*
 * Free an instance of IODeviceMaster. Nop; we keep the instance around.
 */
- free
{
	return self;
}

/*
 * Get device type and device name for specified IOObjectNumber.
 */
- (IOReturn)lookUpByObjectNumber	: (IOObjectNumber)objectNumber
			     deviceKind : (IOString *)deviceKind
			     deviceName : (IOString *)deviceName
{
	return _IOLookupByObjectNumber(_deviceMasterPort, 
		objectNumber,
		deviceKind,
		deviceName);
}
				   

/*
 * Get IOObjectNumber and device type for specified deviceName.
 */
- (IOReturn)lookUpByDeviceName		: (IOString)deviceName
			   objectNumber : (IOObjectNumber *)objectNumber
			     deviceKind : (IOString *)deviceKind
{
	return _IOLookupByDeviceName(_deviceMasterPort,
		deviceName,
		objectNumber,
		deviceKind);
}
			     
/*
 * Get/set parameter methods used at user level to communicate with
 * kernel-level drivers.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count	// in/out
{
	return _IOGetIntValues(_deviceMasterPort,
		objectNumber,
		parameterName,
		*count,
		parameterArray,
		count);
}
	
- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count	// in/out
{
	return _IOGetCharValues(_deviceMasterPort,
		objectNumber,
		parameterName,
		*count,
		parameterArray,
		count);

}
	
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;
{
	return _IOSetIntValues(_deviceMasterPort,
		objectNumber,
		parameterName,
		parameterArray,
		count);
}

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;
{
	return _IOSetCharValues(_deviceMasterPort,
		objectNumber,
		parameterName,
		parameterArray,
		count);

}

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
	      objectNumber: (IOObjectNumber) objectNumber
{
    return _IOServerConnect(_deviceMasterPort, objectNumber,
			   task_self(), machPort);
}

- (IOReturn) lookUpByStringPropertyList:(const char *)values
            results:(char *)results
            maxLength:(unsigned int)length
{
    int	unused = length;

    return( _IOLookUpByStringPropertyList( _deviceMasterPort,
            (unsigned char *) values, strlen( values) + 1,
	    (unsigned char *) results, &unused,
	    length));
}

- (IOReturn) getStringPropertyList:(IOObjectNumber) objectNumber
            names:(const char *)names
            results:(char *) results
            maxLength:(unsigned int)length
{
    int	unused = length;

    return( _IOGetStringPropertyList( _deviceMasterPort, objectNumber,
            (unsigned char *) names, strlen( names) + 1,
	    (unsigned char *) results, &unused,
	    length));
}

- (IOReturn) getByteProperty:(IOObjectNumber) objectNumber
            name:(const char *)name
            results:(char *) results
            maxLength:(unsigned int *)length
{

    return( _IOGetByteProperty( _deviceMasterPort, objectNumber,
            (unsigned char *) name, strlen( name) + 1,
	    (unsigned char *) results, length,
	    *length));
}


@end
