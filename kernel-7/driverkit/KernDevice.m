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
 * Copyright (c) 1993, 1994 NeXT Computer, Inc.
 *
 * Kernel Device Object.
 *
 * HISTORY
 *
 * 1 Jul 1994 ? at NeXT
 *	Modifications for shared interrupts.
 * 27 Feb 1994 ? at NeXT
 *	Major rewrite.
 * 3 Oct 1993 ? at NeXT
 *	Created.
 */
 
#import <mach/mach_types.h>

#import <objc/List.h>

#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDevicePrivate.h>
#import <driverkit/KernBusInterrupt.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/KernLock.h>

#import <kernserv/machine/spl.h>

@implementation KernDevice

- initWithDeviceDescription:	deviceDescription
{    
    if (deviceDescription == nil)
    	return [self free];
	
    [super init];
	
    _deviceDescription = deviceDescription;
    
    return self;
}

- init
{
    return [self free];
}

- free
{
    [self detachInterruptPort];
    
    return [super free];
}

- deviceDescription
{
    return _deviceDescription;
}

- attachInterruptPort:	(port_t)interruptPort
{
    ipc_port_t		port;
    id			interrupts;
    int			i, numInterrupts;

    if (_interruptPort != IP_NULL)
    	return nil;

    if (ipc_object_copyin(current_task()->itk_space,
			    interruptPort, MACH_MSG_TYPE_MAKE_SEND,
			    &port))
	return nil;

    interrupts = [_deviceDescription interrupts];
    if (interrupts == nil) {
	ipc_port_release_send(port);
	return self;
    }

    numInterrupts = [interrupts count];
    if (numInterrupts == 0) {
	ipc_port_release_send(port);
    	return self;
    }

    _interruptPort = port;

    _interrupts = [[List alloc] initCount:numInterrupts];	

    for (i = 0; i < numInterrupts; i++) {
	if (![_interrupts addObject:
		    [[KernDeviceInterrupt alloc]
			    initWithInterruptPort:_interruptPort]]) {
	    [self _detachInterruptSources];
	    ipc_port_release_send(_interruptPort);
	    _interruptPort = IP_NULL;
	    return nil;
	}
    }
    
    return self;
}

- detachInterruptPort
{
    if (_interruptPort == IP_NULL)
    	return self;
        
    [self _detachInterruptSources];
    
    ipc_port_release_send(_interruptPort);
    _interruptPort = IP_NULL;
    
    return self;
}

- interrupt:	(int)index
{
    return [_interrupts objectAt:index];
}

- interrupts
{
    return _interrupts;
}

@end

@implementation KernDevice(Private)

- (void)_detachInterruptSources
{
    if (_interrupts != nil) {
    	[[_interrupts freeObjects] free];

	_interrupts = nil;
    }
}

@end

@implementation KernDeviceInterrupt

- initWithInterruptPort:	(void *)port
{
    [super init];

    _lock = [[KernLock alloc] initWithLevel:IPLHIGH];

    _ipcMessage = _KernDeviceInterruptMsgCreate(port);
    
    return self;
}

- free
{
    [self detach];
    
    [_lock free];

    _KernDeviceInterruptMsgDestroy(_ipcMessage);

    return [super free];
}

- attachToBusInterrupt:		busInterrupt
	    withArgument:	(void *)argument
{
    [_lock acquire];

    if (_busInterrupt) {
    	[_lock release];
	return nil;
    }
    
    _busInterrupt = busInterrupt;
    
    [_lock release];
    
    [busInterrupt suspend];
    
    if (![busInterrupt attachDeviceInterrupt:self]) {
    	[busInterrupt resume];
	
	[_lock acquire];
    	_busInterrupt = nil;
	[_lock release];

	return nil;
    }

    [_lock acquire];

    _handler = IOSendInterrupt;
    _handlerArgument = argument;
    
    [_lock release];

    [busInterrupt resume];
	
    return self;
}

- attachToBusInterrupt:		busInterrupt
	withSpecialHandler:	(void *)handler
		argument:	(void *)argument
		atLevel:	(int)level
{
    [_lock acquire];
    
    if (_busInterrupt) {
    	[_lock release];
    	return nil;
    }
    
    _busInterrupt = busInterrupt;
    
    [_lock release];
    
    [busInterrupt suspend];
	
    if (![_busInterrupt attachDeviceInterrupt:self atLevel:level]) {
    	[busInterrupt resume];
	
	[_lock acquire];
	_busInterrupt = nil;
	[_lock release];

    	return nil;
    }
    
    [_lock acquire];

    _handler = handler;
    _handlerArgument = argument;

    [_lock release];
    
    [busInterrupt resume];
    
    return self;
}

- detach
{
    id		busInterrupt;
    BOOL	isSuspended;

    [_lock acquire];
    
    if (!_busInterrupt) {
    	[_lock release];
	return nil;
    }
    
    busInterrupt = _busInterrupt; _busInterrupt = nil;
    isSuspended = _isSuspended; _isSuspended = NO;
    
    [_lock release];

    if (!isSuspended)
	[busInterrupt suspend];
    
    [busInterrupt detachDeviceInterrupt:self];
    
    [busInterrupt resume];

    return self;
}

- suspend
{
    id		busInterrupt;
    BOOL	isSuspended;

    [_lock acquire];
    
    if (!_busInterrupt) {
    	[_lock release];
	return nil;
    }
    
    busInterrupt = _busInterrupt;
    isSuspended = _isSuspended; _isSuspended = YES;
    
    [_lock release];

    if (!isSuspended)
    	[busInterrupt suspend];
    
    return self;
}

- resume
{
    id		busInterrupt;
    BOOL	isSuspended;
    
    [_lock acquire];
    
    if (!_busInterrupt) {
    	[_lock release];
	return nil;
    }
    
    busInterrupt = _busInterrupt;
    isSuspended = _isSuspended; _isSuspended = NO;
    
    [_lock release];
	
    if (isSuspended)
    	[busInterrupt resume];
    
    return self;
}

@end

static
KernDeviceInterruptMsg *
_KernDeviceInterruptMsgCreate(
    ipc_port_t		interruptPort
)
{
    KernDeviceInterruptMsg	*msg;
    
    msg = (void *)kalloc(sizeof (*msg));
    *msg = (KernDeviceInterruptMsg) { 0 };
    
    msg->lock = [[KernLock alloc] initWithLevel:IPLSCHED];

    ipc_port_reference(interruptPort);
    msg->iport = interruptPort;

    msg->callout.func = (thread_call_func_t)_KernDeviceInterruptCallout;
    msg->callout.spec_proto = msg;
    msg->callout.status = IDLE;

    ikm_init_special(&msg->kmsg, IKM_SIZE_DEVICE);

    ipc_port_reference(msg->iport);

    return msg;
}

static
void
_KernDeviceInterruptMsgDestroy(
    KernDeviceInterruptMsg	*msg
)
{
    KernLockAcquire(msg->lock);
    if (msg->queued || msg->pending) {
	msg->destroy = YES;
	KernLockRelease(msg->lock);
	return;
    }
    KernLockRelease(msg->lock);

    ipc_port_release(msg->iport);
    ipc_port_release(msg->iport);
    
    [msg->lock free];
    
    kfree(msg, sizeof (*msg));
}

void
KernDeviceInterruptMsgRelease(
    void			*m
)
{
    KernDeviceInterruptMsg	*msg = m;
    
    ipc_port_reference(msg->iport);

    KernLockAcquire(msg->lock);
    msg->queued = FALSE;
    if (msg->destroy) {
	KernLockRelease(msg->lock);
	_KernDeviceInterruptMsgDestroy(msg);
    }
    else
	KernLockRelease(msg->lock);
}
#if sparc
int
#else
void
#endif
KernDeviceInterruptDispatch(
    KernDeviceInterrupt		*_interrupt,
    void			*state
)
{
    KernDeviceInterrupt_	*interrupt =
				    (KernDeviceInterrupt_ *)_interrupt;
#if sparc
   	int			(*handler)() = interrupt->_handler;
    return((*handler)(interrupt, state, interrupt->_handlerArgument));
#else
   	void		(*handler)() = interrupt->_handler;
    (*handler)(interrupt, state, interrupt->_handlerArgument);

#endif

}

void
KernDeviceInterruptDispatchShared(
    KernDeviceInterrupt		*_interrupt,
    void			*state
)
{
    KernDeviceInterrupt_	*interrupt =
				    (KernDeviceInterrupt_ *)_interrupt;
    void			(*handler)() = interrupt->_handler;
    
    IODisableInterrupt(interrupt);

    (*handler)(interrupt, state, interrupt->_handlerArgument);
}

void
IOEnableInterrupt(
    void		*_interrupt
)
{
    KernDeviceInterrupt_	*interrupt =
				    (KernDeviceInterrupt_ *)_interrupt;
    KernBusInterrupt		*busInterrupt;
    BOOL			isSuspended;
    
    KernLockAcquire(interrupt->_lock);
    
    busInterrupt = interrupt->_busInterrupt;
    isSuspended = interrupt->_isSuspended; interrupt->_isSuspended = NO;
    
    KernLockRelease(interrupt->_lock);
    
    if (isSuspended)
    	KernBusInterruptResume(busInterrupt);
}

void
IODisableInterrupt(
    void		*_interrupt
)
{
    KernDeviceInterrupt_	*interrupt =
				    (KernDeviceInterrupt_ *)_interrupt;
    KernBusInterrupt		*busInterrupt;
    BOOL			isSuspended;

    KernLockAcquire(interrupt->_lock);
    
    busInterrupt = interrupt->_busInterrupt;
    isSuspended = interrupt->_isSuspended; interrupt->_isSuspended = YES;

    KernLockRelease(interrupt->_lock);
    
    if (!isSuspended)
    	KernBusInterruptSuspend(busInterrupt);
}

/*
 * Template for creating interrupt messages.
 */

typedef struct _InterruptMsg {
    mach_msg_header_t		header;
} InterruptMsg;

static InterruptMsg	interruptMsgTemplate = {
    {
	MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0),
	sizeof (InterruptMsg),
	MACH_PORT_NULL,
	MACH_PORT_NULL,
	0,
	IO_DEVICE_INTERRUPT_MSG
    }
};

void
IOSendInterrupt(
    void			*_interrupt,
    void			*state,
    unsigned int		msgId
)
{
    KernDeviceInterrupt_	*interrupt = _interrupt;
    KernDeviceInterruptMsg	*msg = interrupt->_ipcMessage;
    InterruptMsg		*imsg;
    mach_msg_return_t		result;

    if (curipl() > IPLSCHED)
    	return;

    KernLockAcquire(msg->lock);
    if (msg->queued || msg->pending) {
    	KernLockRelease(msg->lock);
	return;
    }
    msg->queued = YES;
    KernLockRelease(msg->lock);

    imsg = (InterruptMsg *)&msg->kmsg.ikm_header;
    *imsg = interruptMsgTemplate;
    imsg->header.msgh_id = msgId;
    imsg->header.msgh_remote_port = (mach_port_t)msg->iport;

    result = ipc_mqueue_send_interrupt(&msg->kmsg);
    if (result != MACH_MSG_SUCCESS) {
	KernLockAcquire(msg->lock);
	msg->queued = NO;
	msg->pending = YES;
	KernLockRelease(msg->lock);

	thread_call_enter(&msg->callout);
    }
}

static
void
_KernDeviceInterruptCallout(
    KernDeviceInterruptMsg	*msg
)
{
    mach_msg_return_t		result;
	
    KernLockAcquire(msg->lock);
    msg->pending = NO;
    msg->queued = YES;
    KernLockRelease(msg->lock);
    
    result = ipc_mqueue_send(
    			&msg->kmsg,
			MACH_SEND_ALWAYS | MACH_SEND_TIMEOUT,
			0, IMQ_NULL_CONTINUE);
    if (result != MACH_MSG_SUCCESS) {
	ipc_port_release(msg->iport);
	KernDeviceInterruptMsgRelease(msg);
    }
}
