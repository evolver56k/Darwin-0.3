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
 * Exported interface for Kernel Device Description Object.
 *
 * HISTORY
 *
 * 24 Feb 1994 ? at NeXT
 *	Major rewrite.
 * 17 Jan 1994 ? at NeXT
 *	Created.
 */
 
#ifdef	DRIVER_PRIVATE

#import <objc/Object.h>
#import <objc/HashTable.h>
#import <driverkit/KernBus.h>

@interface KernDeviceDescription : Object
{
@private
    id		_configTable;
    id		_device;
    id		_resourceTable;
    id		_stringTable;
    id		_interruptList;
    id		_busClass;
    id		_bus;
    int		_busId;
}

- initFromConfigTable: configTable;
- configTable;

- setDevice: device;
- device;

- busClass;

- setBus: bus;
- bus;

- interrupts;

- allocateResourcesForKey:(const char *)aKey;

- allocateItems:(unsigned int *)aList numItems:(unsigned int)num
    forKey:(const char *)aKey;
- allocateRanges:(Range *)aList numRanges:(unsigned int)num
    forKey:(const char *)aKey;

- (const char *)stringForKey:(const char *)aKey;
- resourcesForKey:(const char *)aKey;

- setString:(const char *)aString forKey:(const char *)aKey;
- setResources:resources forKey:(const char *)aKey;

- (BOOL)removeStringForKey:(const char *)aKey;
- (BOOL)removeResourcesForKey:(const char *)aKey;

- (NXHashState)initStringState;
- (BOOL)nextStringState:(NXHashState *)aState key:(const char **)aKey 
	value:(char **)stringPtr;

- (NXHashState)initResourcesState;
- (BOOL)nextResourcesState:(NXHashState *)aState key:(const char **)aKey 
	value:(id *)resourcesPtr;

@end

#endif
