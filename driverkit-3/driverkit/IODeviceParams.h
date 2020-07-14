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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * IODeviceParams.h - Global Get/Set parameter interface to IODevice.
  *
 * HISTORY
 * 09-Sep-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/IODevice.h>
#import <driverkit/driverTypes.h>

/*
 * Get/set parameter RPC support. In the kernel, these methods are called
 * by the driverServer; they map a unit number to an id and invoke the 
 * appropriate methods. Usage in user space TBD.
 *
 * General return codes for all these methods:
 *
 * IO_R_NO_DEVICE - IOObjectNumber out of rangs of known device objects.
 * IO_R_OFFLINE   - invalid IOObjectNumber, but not out of range. There
 *		    can be 'holes' in the IOObjectNumber space; IO_R_OFFLINE
 *		    indicates an IOObjectNumber assicated with such a 
 *		    non-existent device object.
 * IO_R_UNSUPPORTED - createMachPort is not supported by this IODevice
 *		    subclass.
 *
 * All other return codes are device-specific.

 */
@interface IODevice(GlobalParameter)

+ (IOReturn)lookupByObjectNumber	 : (IOObjectNumber)objectNumber
			      deviceKind : (IOString *)deviceKind
			      deviceName : (IOString *)deviceName;

+ (IOReturn)lookupByObjectNumber	: (IOObjectNumber)objectNumber
			       instance : (id *)instance;

+ (IOReturn)lookupByDeviceName		: (IOString)deviceName
			   objectNumber : (IOObjectNumber *)objectNumber
			     deviceKind : (IOString *)deviceKind;
	
+ (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count;	// in/out

+ (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned *)count;	// in/out
	
+ (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;

+ (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			   objectNumber : (IOObjectNumber)objectNumber
			          count : (unsigned)count;

+ (IOReturn) lookUpByStringPropertyList:(char *)values
	results:(char *)results
	maxLength:(unsigned int)length;

+ (IOReturn) getStringPropertyList     	: (IOObjectNumber)objectNumber
			   names        : (const char *)names
			   results      : (char *)results
			   maxLength    : (unsigned int)length;

+ (IOReturn) getByteProperty     	: (IOObjectNumber)objectNumber
			   name         : (const char *)name
			   results      : (char *)results
			   maxLength    : (unsigned int *)length;

#if KERNEL
/*
 * In kernel mapping of the IODeviceMaster serverConnect:objectNumber call
 * Note the task is tagged on to the end of the method and filled in
 * by IODeviceMaster before crossing the boundary.
 */
+ (IOReturn) serverConnect: (port_t *)       machPort   // out - IOTask space
	      objectNumber: (IOObjectNumber) objectNumber
	          taskPort: (port_t)         taskPort;  // in  - ipc_port_t
#endif /* KERNEL */

+ (IOReturn)callDeviceMethod		: (IOString)methodName
		    inputParams		: (unsigned char *)inputParams
		    inputCount		: (unsigned)inputCount
		    outputParams	: (unsigned char *)outputParams
		    outputCount		: (unsigned *)outputCount	// in/out
		    privileged		: (host_priv_t *)privileged
		    objectNumber	: (IOObjectNumber)objectNumber;


@end
