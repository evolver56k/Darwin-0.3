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
 * PCMCIA direct device implementation.
 *
 * HISTORY
 *
 * 17 May 1994 Curtis Galloway at NeXT
 *	Created.
 *
 *
 */

#define KERNEL_PRIVATE	1

#import <driverkit/i386/IOPCMCIADirectDevice.h>
#import <driverkit/IODirectDevicePrivate.h>

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/i386/PCMCIAKernBus.h>
#import <driverkit/KernBusMemory.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/i386/PCMCIAPool.h>

#define ATTRIBUTE_MAPPING_KEY	"PCMCIA_DEVICE_ATTR_MAPPING"

@implementation IODirectDevice(IOPCMCIADirectDevice)

- (IOReturn) mapAttributeMemoryTo:(vm_address_t *) destAddr
			findSpace:(BOOL) findSpace
{
	struct _eisa_private		*private = _busPrivate;
	id				mapping, mappingList;
	id				resource;
	id				socketElement;
	id				memWindowElement, memWindow;
	id				windowList;
	Range				memRange;
	id				thePCMCIABus;
					
	thePCMCIABus = [KernBus lookupBusInstanceWithName:"PCMCIA" busId:0];
	
	if ([_deviceDescriptionDelegate
	    resourcesForKey:ATTRIBUTE_MAPPING_KEY]) {
		return IO_R_BUSY;
	}
	/* Get memory window */
	socketElement = [[_deviceDescriptionDelegate
	    resourcesForKey:PCMCIA_SOCKET_LIST] objectAt:0];
	if (socketElement == nil)
		return IO_R_RESOURCE;
	memWindowElement = [thePCMCIABus allocMemoryWindowForSocket:
	    [socketElement object]];
	if (memWindowElement == nil)
		return IO_R_RESOURCE;
	windowList = [_deviceDescriptionDelegate
	    resourcesForKey:PCMCIA_WINDOW_LIST];
	
	resource = [thePCMCIABus memoryRangeResource];
	
	if (findSpace)
		mapping =
		    [resource mapInTarget:current_task_EXTERNAL()
			      cache:NO];
	else
		mapping =
		    [resource mapToAddress:*destAddr
			      inTarget:current_task_EXTERNAL()
			      cache:NO];

	if (mapping == nil) {
		[memWindowElement free];
		return IO_R_NO_MEMORY;
	}
		
	*destAddr = [mapping address];

	mappingList = [[List alloc] initCount:1];
	[mappingList addObject:mapping];
	[_deviceDescriptionDelegate
	    setResources:mappingList forKey:ATTRIBUTE_MAPPING_KEY];

	memWindow = [memWindowElement object];
	memRange = [resource range];
	[memWindow setEnabled:NO];
	[memWindow setMemoryInterface:YES];
	[memWindow setAttributeMemory:YES];
	[memWindow setMapWithSize:memRange.length
		systemAddress:memRange.base cardAddress:0];
	[memWindow setEnabled:YES];
	[[memWindow socket] setMemoryInterface:YES];

	[windowList addObject:memWindowElement];

	return IO_R_SUCCESS;
}

- (void) unmapAttributeMemory
{
	struct _eisa_private		*private = _busPrivate;
	id				windowList;
	int				i;

	if ([_deviceDescriptionDelegate
	    resourcesForKey:ATTRIBUTE_MAPPING_KEY] == nil)
		return;

	/* Find the memory window */
	windowList = [_deviceDescriptionDelegate
	    resourcesForKey:PCMCIA_WINDOW_LIST];
	
	for (i=0; i < [windowList count]; i++) {
	    id window, windowElement;
	    
	    windowElement = [windowList objectAt:i];
	    window = [windowElement object];
	    if ([window memoryInterface] && [window attributeMemory]) {
		/* Right now there is only one attribute memory window allowed,
		 * so assume this is the right one.
		 */
		[window setAttributeMemory:NO];
		[window setEnabled:NO];
		[[window socket] setMemoryInterface:NO];
		[windowList removeObject:windowElement];
		[windowElement free];
		break;
	    }
	}
	
	/* This will free the mapping */
	[_deviceDescriptionDelegate
	    removeResourcesForKey:ATTRIBUTE_MAPPING_KEY];
}

@end

