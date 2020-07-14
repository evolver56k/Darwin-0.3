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
 * Generic Kernel Bus Interrupt Object.
 *
 * HISTORY
 *
 * 30 Jun 1994 ? at NeXT
 *	Created.
 */

#import <driverkit/KernBusInterrupt.h>
#import <driverkit/KernBusInterruptPrivate.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernLock.h>

#import <objc/List.h>

@implementation KernBusInterrupt

- initForResource:	resource
	    item:	(unsigned int)item
	withHandler:	(void *)handler
	shareable:	(BOOL)shareable
{
    [super initForResource:resource item:item shareable:shareable];
    
    _attachedInterrupts = [[List alloc] init];
    _interruptLock = [[KernLock alloc] init];
    _suspendLock = [[KernLock alloc] init];
#if !sparc
    _deviceHandler = (handler != nil)? handler:
    			(shareable? KernDeviceInterruptDispatchShared:
						KernDeviceInterruptDispatch);
#else
    _deviceHandler = (handler != nil)? handler:
    			(shareable? KernDeviceInterruptDispatch:
						KernDeviceInterruptDispatch);

#endif

    return self;
}

- initForResource:	resource
	    item:	(unsigned int)item
	shareable:	(BOOL)shareable
{
    return [self initForResource:resource
			    item:item
			withHandler:nil
			shareable:shareable];
}

- free
{
    [_interruptLock acquire];
    
    if (_attachedInterruptCount > 0) {
    	[_interruptLock release];
	return self;
    }
    
    [_interruptLock release];
    
    return [super free];
}

- dealloc
{
    [_interruptLock acquire];

    if (_attachedInterruptCount > 0) {
	[_interruptLock release];
    	return self;
    }

    [_suspendLock acquire];

    if (++_suspendCount < 0)
    	_suspendCount--;

    [_suspendLock release];

    [_interruptLock release];

    [_attachedInterrupts free];

    [_suspendLock free];

    [_interruptLock free];

    return [super dealloc];
}

- attachDeviceInterrupt:	interrupt
{
    id		result;

    [_interruptLock acquire];

    if ([_attachedInterrupts indexOf:interrupt] == NX_NOT_IN_LIST &&
    		[_attachedInterrupts addObject:interrupt])
    	_attachedInterruptCount++;
	
    [_suspendLock acquire];
	
    result = (_attachedInterruptCount > 0) &&
    			(_suspendCount == 0)? self: nil;

    [_suspendLock release];

    [_interruptLock release];

    return result;
}

- attachDeviceInterrupt:	interrupt
		atLevel:	(int)level
{
    return [self attachDeviceInterrupt:interrupt];
}

- detachDeviceInterrupt:	interrupt
{
    id		result;

    [_interruptLock acquire];

    if ([_attachedInterrupts removeObject:interrupt])
    	_attachedInterruptCount--;

    [_suspendLock acquire];

    result = (_attachedInterruptCount > 0) &&
			(_suspendCount == 0)? self: nil;

    [_suspendLock release];

    [_interruptLock release];

    return result;
}

- suspend
{
    [_suspendLock acquire];

    if (++_suspendCount < 0)
    	_suspendCount--;
	
    [_suspendLock release];
    
    return self;
}

- resume
{
    id		result;

    [_suspendLock acquire];

    if (_suspendCount > 0)
    	_suspendCount--;
	
    result = (_suspendCount == 0)? self: nil;

    [_suspendLock release];
	
    return result;
}

@end

BOOL
KernBusInterruptDispatch(
    KernBusInterrupt		*_interrupt,
    void			*state
)
{
    KernBusInterrupt_		*interrupt = (KernBusInterrupt_ *)_interrupt;
    KernDeviceInterrupt		**deviceInterrupt; 
    int				deviceInterruptCount;
    void			(*handler)() = interrupt->_deviceHandler;
    BOOL			result;
    
    KernLockAcquire(interrupt->_interruptLock);
    
    deviceInterrupt = ((List *)interrupt->_attachedInterrupts)->dataPtr;
    deviceInterruptCount = interrupt->_attachedInterruptCount;
    
    while (deviceInterruptCount-- > 0)
	(*handler)(*deviceInterrupt++, state);
	
    KernLockAcquire(interrupt->_suspendLock);
    
    result = (interrupt->_attachedInterruptCount > 0) &&
    					(interrupt->_suspendCount == 0);
					
    KernLockRelease(interrupt->_suspendLock);

    KernLockRelease(interrupt->_interruptLock);
    
    return (result);
}

void
KernBusInterruptSuspend(
	KernBusInterrupt	*_interrupt
)
{
    KernBusInterrupt_		*interrupt = (KernBusInterrupt_ *)_interrupt;
    
    if (_interrupt == nil)
    	return;

    KernLockAcquire(interrupt->_suspendLock);

    if (++interrupt->_suspendCount < 0)
    	interrupt->_suspendCount--;

    KernLockRelease(interrupt->_suspendLock);
}

void
KernBusInterruptResume(
	KernBusInterrupt	*_interrupt
)
{
    KernBusInterrupt_		*interrupt = (KernBusInterrupt_ *)_interrupt;

    if (_interrupt == nil)
    	return;

    KernLockAcquire(interrupt->_suspendLock);

    if (interrupt->_suspendCount > 0)
    	interrupt->_suspendCount--;

    KernLockRelease(interrupt->_suspendLock);
}
