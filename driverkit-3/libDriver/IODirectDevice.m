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
 * IODirectDevice class implementation.
 *
 * HISTORY
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#ifdef	KERNEL
#define MACH_USER_API	1
#endif	KERNEL

#import <objc/List.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/IODirectDevicePrivate.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernBusMemory.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_types.h>
#import <mach/mach_interface.h>

@interface IODirectDevice(EISAPrivate)
- initEISA;
- freeEISA;
@end
@interface IODirectDevice(HPPAPrivate)
- initHPPA;
- freeHPPA;
@end
@interface IODirectDevice(SPARCPrivate)
- initSPARC;
- freeSPARC;
@end
@interface IODirectDevice(PPCPrivate)
- initPPC;
- freePPC;
@end

@interface IODirectDevice(InterruptPrivate)
- (IOReturn) _changeInterrupt:(unsigned int) localInterrupt to:(BOOL) enable;
@end

#define NUM_LOCAL_INTS		4

typedef struct _IODirectDevicePrivate {
    id		memory_mappings;
    id		device_interrupts;
    id		local_device_interrupts[ NUM_LOCAL_INTS ];
} IODirectDevicePrivate;

#define IO_EXIT_THREAD_MSG	IO_FIRST_UNRESERVED_INTERRUPT_MSG

@implementation IODirectDevice

/*
 * By defintion, all subclasses of this class are of style IO_DirectDevice.
 * Subclasses need not implement this method.
 */
+ (IODeviceStyle)deviceStyle
{
	return IO_DirectDevice;
}

- initFromDeviceDescription: deviceDescription
{
    IODirectDevicePrivate	*private;
    [super init];

    [self setDeviceDescription:deviceDescription];
    
#if i386
    [self initEISA];
#elif ppc
	[self initPPC];
#elif hppa
    [self initHPPA];
#elif sparc
	[self initSPARC];
#endif
    private = _private = (void *)IOMalloc(sizeof (*private));
    bzero( private, sizeof (*private));

    return self;
}

- init
{
    return [self free];
}

- free
{
    IODirectDevicePrivate	*private = _private;
    struct _private		*busPrivate = _busPrivate;
    msg_header_t		msg = { 0 };
    
    if (_interruptPort) {
	if (_ioThread) {
	    msg.msg_size = sizeof (msg);
	    msg.msg_id =  IO_EXIT_THREAD_MSG;
#ifdef	KERNEL
	    msg.msg_remote_port = IOGetKernPort(_interruptPort);
	    msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
#else
	    msg.msg_remote_port = _interruptPort;
	    msg_send(&msg, MSG_OPTION_NONE, 0);
#endif	KERNEL
	}

	[[_deviceDescriptionDelegate device] detachInterruptPort];
	(void) port_deallocate(task_self(), _interruptPort);
	_interruptPort = PORT_NULL;
    }
    
    if (busPrivate) {
	if (busPrivate->type == EISA)
	    [self freeEISA];
	else if (busPrivate->type == ArchPPC)
	    [self freePPC];
	else if (busPrivate->type == HPPA)
	    [self freeHPPA];
	else if (busPrivate->type == SPARC)
	    [self freeSPARC];
    }

    if (private) {
	if (private->memory_mappings)
	    [private->memory_mappings free];
	if (private->device_interrupts)
	    [private->device_interrupts free];
	IOFree((void *)private, sizeof (*private));
    }

    return [super free];
}

- (void)setDeviceDescription : deviceDescription
{
    [super setDeviceDescription:deviceDescription];
    _deviceDescription = deviceDescription;
    _deviceDescriptionDelegate = [deviceDescription _delegate];
}

- (port_t)interruptPort
{
    return _interruptPort;
}

- (IOReturn) attachInterruptPort
{
    if (_interruptPort == PORT_NULL) {
	if (port_allocate(task_self(), &_interruptPort))
	    return IO_R_NOT_ATTACHED;
	    
	/*
	 * Don't attach unless interrupts have been configured.
	 */
	if ([[_deviceDescriptionDelegate device]
				attachInterruptPort:_interruptPort] == nil) {
	    (void) port_deallocate(task_self(), _interruptPort);
	    _interruptPort = PORT_NULL;

	    return IO_R_NOT_ATTACHED;
	}
    }
    
    return IO_R_SUCCESS;
}

static void
IODirectDeviceThread(id		device);

- (IOReturn)startIOThreadWithPriority:(int)priority
{
    IOReturn		result = IO_R_SUCCESS;
    
    if (_ioThread == NULL) {
	result = [self attachInterruptPort];
    
	if (result == IO_R_SUCCESS) {
	    _ioThread = IOForkThread((IOThreadFunc)IODirectDeviceThread, self);
	    if (priority >= 0)
		(void) IOSetThreadPriority(_ioThread, priority);
	}
    }
    
    return (result);
}

- (IOReturn)startIOThreadWithFixedPriority:(int)priority
{
    IOReturn		result = IO_R_SUCCESS;

    if (_ioThread == NULL) {
	result = [self startIOThreadWithPriority:priority];
	if (result == IO_R_SUCCESS)
	    (void) IOSetThreadPolicy(_ioThread, POLICY_FIXEDPRI);
    }
	
    return (result);
}

- (IOReturn)startIOThread
{
    return [self startIOThreadWithPriority:-1];
}

- (IOReturn)waitForInterrupt:(int *)id
{
    IOInterruptMsg		interruptMsg;
    kern_return_t		result;
    msg_option_t		option = RCV_TIMEOUT | RCV_LARGE;

    if (_interruptPort == PORT_NULL)
    	return (IO_R_NO_INTERRUPT);
	
    do {
	interruptMsg.header.msg_size = sizeof (interruptMsg);
	interruptMsg.header.msg_local_port = _interruptPort;
	
	result = msg_receive(&interruptMsg.header, option, 0);
	if (result == RCV_TOO_LARGE)
	    return (IO_R_MSG_TOO_LARGE);

	if (result != RCV_SUCCESS && result != RCV_TIMED_OUT) {
	    IOLog("%s: %s waitForInterrupt: msg_receive returns %d\n",
		[self name], [self deviceKind], result);
	    return (IO_R_IPC_FAILURE);
	}
	
	if (option & RCV_TIMEOUT) {
	    if (result == RCV_TIMED_OUT)
	    	option = RCV_LARGE;
	    else /* if (result == RCV_SUCCESS) */
	    	(void) thread_block();
	}
    } while (result != RCV_SUCCESS);
    
    *id = interruptMsg.header.msg_id;
    
    return (IO_R_SUCCESS);
}

- (void)receiveMsg
{
    IOInterruptMsg		interruptMsg;
    
    if (_interruptPort == PORT_NULL)
    	return;
	
    interruptMsg.header.msg_size = sizeof (interruptMsg);
    interruptMsg.header.msg_local_port = _interruptPort;
    
    (void) msg_receive(&interruptMsg.header, MSG_OPTION_NONE, 0);
}

- (void)timeoutOccurred
{
}

- (void)interruptOccurred
{
    [self interruptOccurredAt:0];
}

- (void)interruptOccurredAt:(int)localNum
{
}

- (void)commandRequestOccurred
{
}

- (void)otherOccurred:(int)id
{
}

@end	/* IODirectDevice */

@implementation IODirectDevice(IOInterrupts)

- (IOReturn) enableAllInterrupts
{
	unsigned int	i, max = [_deviceDescription numInterrupts];

	for (i=0;  i < max;  i++) {
		if ([self enableInterrupt:i] != IO_R_SUCCESS) {
			[self disableAllInterrupts];
			return IO_R_NOT_ATTACHED;
		}
	}
	return IO_R_SUCCESS;
}


- (void) disableAllInterrupts
{
	unsigned int	i, max = [_deviceDescription numInterrupts];

	for (i=0;  i < max;  i++)
		[self disableInterrupt:i];
}

- (IOReturn) enableInterrupt:(unsigned int) localInterrupt
{
    IODirectDevicePrivate	*private = _private;

        if( (localInterrupt < NUM_LOCAL_INTS) && private->local_device_interrupts[ localInterrupt ]) {
            [private->local_device_interrupts[ localInterrupt ] resume];
            return IO_R_SUCCESS;
        }

	if ([self attachInterruptPort] != IO_R_SUCCESS)
		return IO_R_NOT_ATTACHED;
	return [self _changeInterrupt:localInterrupt to:YES];
}


- (void) disableInterrupt:(unsigned int) localInterrupt
{
    IODirectDevicePrivate	*private = _private;

    if( (localInterrupt < NUM_LOCAL_INTS) && private->local_device_interrupts[ localInterrupt ]) {
        [private->local_device_interrupts[ localInterrupt ] suspend];
        return;
    }
    [self _changeInterrupt:localInterrupt to:NO];
}

/*
 *  The subclass overrides this method.  In the superclass, we just
 *  return NO.
 */
- (BOOL) getHandler:(IOInterruptHandler *)handler
              level:(unsigned int *)ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
	return NO;
}

@end

@implementation IODirectDevice(InterruptPrivate)

- (IOReturn) _changeInterrupt:(unsigned int) localInterrupt to:(BOOL) enable
{
	IODirectDevicePrivate	*private = _private;
	id			interrupt;

	if (localInterrupt >= [_deviceDescription numInterrupts])
		return IO_R_INVALID_ARG;
		
	if (private->device_interrupts == nil)
		private->device_interrupts =
				[[HashTable alloc] initKeyDesc:"i"];
	interrupt = [private->device_interrupts
				valueForKey:(void *)localInterrupt];
	if (interrupt == nil) {
		IOInterruptHandler	handler;
		unsigned int		ipl = 3;	/* XXX */
		unsigned int		arg = 0;
		id			busInterrupt;

		interrupt = [[_deviceDescriptionDelegate device]
				interrupt:localInterrupt];
		[private->device_interrupts
			insertKey:(void *)localInterrupt
			    value:interrupt];
		if( localInterrupt < NUM_LOCAL_INTS)
                    private->local_device_interrupts[ localInterrupt ] = interrupt;
		busInterrupt = [[_deviceDescriptionDelegate
					resourcesForKey:IRQ_LEVELS_KEY]
						objectAt:localInterrupt];
		if ([self getHandler:&handler
				level:&ipl
				argument:&arg
			    forInterrupt:localInterrupt])
			[interrupt attachToBusInterrupt:busInterrupt
					withSpecialHandler:handler
						argument:(void *)arg
						atLevel:ipl];
		else
			[interrupt attachToBusInterrupt:busInterrupt
				    withArgument:(void *)
					(IO_DEVICE_INTERRUPT_MSG +
						localInterrupt)];
	}

	if (enable)
		[interrupt resume];
	else
		[interrupt suspend];
	
	return IO_R_SUCCESS;
}


@end

@implementation IODirectDevice(IOMemory)
/*
 *  Methods to map device memory into the calling task's address space.
 */
- (IOReturn) mapMemoryRange:(unsigned int) localRange
			to:(vm_address_t *) destAddr
			findSpace:(BOOL) findSpace
			cache:(IOCache) cache
{
	IODirectDevicePrivate		*private = _private;
	id				mapping;
	id				resource;

	if (localRange >= [_deviceDescription numMemoryRanges])
		return IO_R_INVALID_ARG;

	resource = [[_deviceDescriptionDelegate resourcesForKey:MEM_MAPS_KEY]
			objectAt:localRange];
	if (findSpace)
		mapping =
		    [resource mapInTarget:current_task_EXTERNAL()
			      cache:cache];
	else
		mapping =
		    [resource mapToAddress:*destAddr
			      inTarget:current_task_EXTERNAL()
			      cache:cache];

	if (mapping == nil)
		return IO_R_NO_MEMORY;
		
	*destAddr = [mapping address];

	if (private->memory_mappings == nil)
		private->memory_mappings = [[HashTable alloc] initKeyDesc:"i"];

	[private->memory_mappings insertKey:(void *)*destAddr value:mapping];	
		
	return IO_R_SUCCESS;
}

- (void) unmapMemoryRange:(unsigned int) localRange
			from:(vm_address_t) virtAddr;
{
	IODirectDevicePrivate		*private = _private;
	id				mapping;

	if (localRange >= [_deviceDescription numMemoryRanges])
		return;
		
	mapping = [private->memory_mappings valueForKey:(void *)virtAddr];
	if (mapping == nil)
		return;
		
	[private->memory_mappings removeKey:(void *)virtAddr];
	
	[mapping free];
}


@end

static void
IODirectDeviceThread(
    id		device
)
{
    IOReturn		result;
    int			id;
    
    while (TRUE) {
	result = [device waitForInterrupt:&id];
	if (result == IO_R_MSG_TOO_LARGE)
	    [device receiveMsg];
	else if (result != IO_R_SUCCESS)
	    IOLog("%s: %s thread: waitForInterrupt: returns %d\n",
	    	[device name], [device deviceKind], result);
	else {
	    switch (id) {

	    case IO_TIMEOUT_MSG:
	    	[device timeoutOccurred];
		break;
		
	    case IO_COMMAND_MSG:
	    	[device commandRequestOccurred];
		break;
		
	    case IO_DEVICE_INTERRUPT_MSG:
	    	[device interruptOccurred];
		break;
	    
	    case IO_EXIT_THREAD_MSG:
	    	IOExitThread();
		break;
		
	    default:
	    	if (		id >= IO_DEVICE_INTERRUPT_MSG_FIRST
			&&	id <= IO_DEVICE_INTERRUPT_MSG_LAST) {
		    int		localNum = id - IO_DEVICE_INTERRUPT_MSG_FIRST;
		    
		    [device interruptOccurredAt:localNum];
		}
		else
		    [device otherOccurred:id];
	    }
	}
    }
}

@implementation IODirectDevice(Private)

- _deviceDescriptionDelegate
{
    return _deviceDescriptionDelegate;
}

@end
