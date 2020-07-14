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
 * IODeviceDescription.m.
 *
 * HISTORY
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <objc/List.h>
#import <driverkit/IODeviceDescription.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/KernBus.h>
#import <driverkit/KernDeviceDescription.h>

struct _private {
    unsigned int	*interrupts;
    unsigned int	numInterrupts;
    IORange		*memoryRanges;
    unsigned int	numMemoryRanges;
};

@implementation IODeviceDescription

- init
{
    return [self _initWithDelegate:nil];
}

- free
{
    struct _private	*private = _private;
    
    if (private->numInterrupts > 0)
    	IOFree(private->interrupts,
		private->numInterrupts * sizeof (*private->interrupts));
    if (private->numMemoryRanges > 0)
    	IOFree(private->memoryRanges,
		private->numMemoryRanges * sizeof (*private->memoryRanges));
    IOFree(private, sizeof (*private));

    if (_devicePort != PORT_NULL)
    	destroy_dev_port(_devicePort);	// XXX
	
    return [super free];
}

- (port_t)devicePort
{
    return _devicePort;
}

- (void)setDevicePort:		(port_t)devicePort
{
    _devicePort = devicePort;
}

- directDevice
{
    return _directDevice;
}

- (void)setDirectDevice:	directDevice
{
    _directDevice = directDevice;
}

- (void)setConfigTable:		(IOConfigTable *)configTable
{
    /* nop */
}

- (IOConfigTable *)configTable
{
    return [_delegate configTable];
}

- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    return( NULL);
}

- (char *) matchDevicePath:(char *)matchPath
{
    return( NULL);
}

@end	

@implementation IODeviceDescription(IOInterrupt)
- (unsigned int) interrupt
{
    return [self interruptList][0];
}

- (unsigned int *) interruptList
{
    struct _private	*private = _private;

    if (private->numInterrupts == 0)
    	private->interrupts = [self
				_fetchItemList:[[self _delegate]
					resourcesForKey:IRQ_LEVELS_KEY]
				    returnedNum:&private->numInterrupts];
    return private->interrupts;
}

- (unsigned int) numInterrupts
{
    struct _private	*private = _private;

    if (private->numInterrupts == 0)
    	private->interrupts = [self
				_fetchItemList:[[self _delegate] 
					resourcesForKey:IRQ_LEVELS_KEY]
				    returnedNum:&private->numInterrupts];
    return private->numInterrupts;
}

- (IOReturn) setInterruptList:(unsigned int *)aList num:(unsigned int) length
{
    IOReturn ret = IO_R_RESOURCE;
    
    if ([[self _delegate]
	allocateItems:aList numItems:length
	forKey:IRQ_LEVELS_KEY] != nil) {
	
	struct _private *private = (struct _private *)_private;
	if (private->numInterrupts > 0)
	    IOFree(private->interrupts,
		private->numInterrupts * sizeof (*private->interrupts));
	private->numInterrupts = 0;
	ret = IO_R_SUCCESS;
    }
    return ret;
}
@end

@implementation IODeviceDescription(IOMemory)
- (IORange *) memoryRangeList
{
    struct _private	*private = _private;

    if (private->numMemoryRanges == 0)
    	private->memoryRanges =
			    [self
			    	_fetchRangeList:[[self _delegate] 
					resourcesForKey:MEM_MAPS_KEY]
				    returnedNum:&private->numMemoryRanges];
    return private->memoryRanges;
}

- (unsigned int) numMemoryRanges
{
    struct _private	*private = _private;

    if (private->numMemoryRanges == 0)
    	private->memoryRanges =
			    [self
			    	_fetchRangeList:[[self _delegate] 
					resourcesForKey:MEM_MAPS_KEY]
				    returnedNum:&private->numMemoryRanges];
    return private->numMemoryRanges;
}

- (IOReturn) setMemoryRangeList:(IORange *)aList num:(unsigned int) length
{
    IOReturn ret = IO_R_RESOURCE;
    Range *ranges;
    int i;
    
    ranges = (Range *)IOMalloc(sizeof(Range) * length);
    for (i=0; i<length; i++) {
	ranges[i].base = aList[i].start;
	ranges[i].length = aList[i].size;
    }
    if ([[self _delegate]
	allocateRanges:ranges numRanges:length forKey:MEM_MAPS_KEY]
	!= nil) {
	
	struct _private *private = (struct _private *)_private;
	if (private->numMemoryRanges > 0)
	    IOFree(private->memoryRanges,
		private->numMemoryRanges * sizeof (*private->memoryRanges));
	private->numMemoryRanges = 0;
	ret = IO_R_SUCCESS;
    }
    IOFree(ranges, sizeof(Range) * length);
    return ret;
}

@end

@implementation IODeviceDescription(Private)

- _initWithDelegate:delegate
{
    _private = (void *)IOMalloc(sizeof (struct _private));
    bzero(_private, sizeof (struct _private));

    _delegate = delegate;

    return self;
}

- _delegate
{
    return _delegate;
}

- _deviceClass
{
    return nil;
}

- (void *)_fetchItemList:	list
	    returnedNum:	(unsigned int *)returnedNum
{
    unsigned int	*array = 0;
    int			i, num;
    
    num = [list count];
    if (num > 0) {
    	array = (void *)IOMalloc(num * sizeof (*array));
	for (i = 0; i < num; i++)
	    array[i] = [[list objectAt:i] item];
    }
    
    *returnedNum = num;
    return array;
}

- (void *)_fetchRangeList:	list
	    returnedNum:	(unsigned int *)returnedNum
{
    IORange		*array = 0;
    Range		range;
    int			i, num;
    
    num = [list count];
    if (num > 0) {
    	array = (void *)IOMalloc(num * sizeof (*array));
	for (i = 0; i < num; i++) {
	    range = [[list objectAt:i] range];
	    array[i].start = range.base;
	    array[i].size = range.length;
	}
    }
    
    *returnedNum = num;
    return array;
}

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length
	selector:(SEL)selector
	isString:(BOOL)isString
{
    id		rtn = nil;
    char *	str;

    if( [self respondsTo:selector]) {
	// guarantee a minimal size so most methods don't have to check
	if( (isString == NO) || ((strlen( value) + 32) <= *length))
            rtn = objc_msgSend( self, selector, value, length );
    }

    if( nil == rtn) {
        if( (str = [[self configTable] valueForStringKey:propertyName])) {
	    if( (strlen( str) + 1) < *length) {
		strcpy( value, str);
                rtn = self;
	    }
	}
    }

    return( rtn);
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    return( nil);
}

- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen
{
    return( nil);
}

- (char *) nodeName
{
    return( NULL);
}

@end
