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
 * IODeviceDescription.h.
 *
 * HISTORY
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <objc/Object.h>
#import <mach/port.h>
#import <driverkit/IOConfigTable.h>
#import <driverkit/driverTypes.h>

/*
 * IODeviceDescription object. This describes a
 * configured instance of a device.
 */
@interface IODeviceDescription : Object
{
@private
	port_t		_devicePort;
	id		_directDevice;
	id		_delegate;
	void		*_private;
	int		_IODeviceDescription_reserved[3];
}

- (port_t)devicePort;
- (void)setDevicePort 		: (port_t)devicePort;
- directDevice;
- (void)setDirectDevice 	: directDevice;
- (void)setConfigTable 		: (IOConfigTable *)configTable;
- (IOConfigTable *)configTable;
- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias;
- (char *) matchDevicePath:(char *)matchPath;

@end	/* IODeviceDescription */

@interface IODeviceDescription(IOInterrupt)
- (unsigned int) interrupt;
- (unsigned int *) interruptList;
- (unsigned int) numInterrupts;
- (IOReturn) setInterruptList	: (unsigned int *)list 
		    	    num : (unsigned int) numInterrupts;

@end

@interface IODeviceDescription(IOMemory)
- (IORange *) memoryRangeList;
- (unsigned int) numMemoryRanges;
- (IOReturn) setMemoryRangeList	: (IORange *)list
			    num : (unsigned int) numRanges;
@end
