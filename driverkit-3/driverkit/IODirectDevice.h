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
 * IODirectDevice class description.
 *
 * HISTORY
 * 08-Jan-93    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/IODevice.h>

@interface IODirectDevice : IODevice
{
@private
	IODeviceDescription	*_deviceDescription;
	port_t			_interruptPort;
	void *			_ioThread;
	id			_deviceDescriptionDelegate;
	void			*_busPrivate;
	void			*_private;
	int			_IODirectDevice_reserved[2];
}

/*
 * By defintion, all subclasses of this class are of style IO_DirectDevice.
 * Subclasses need not implement this method.
 */
+ (IODeviceStyle)deviceStyle;

- initFromDeviceDescription: deviceDescription;
- free;

- (port_t)interruptPort;
- (IOReturn)attachInterruptPort;
- (IOReturn)startIOThread;
- (IOReturn)startIOThreadWithPriority:(int)priority;
- (IOReturn)startIOThreadWithFixedPriority:(int)priority;
- (IOReturn)waitForInterrupt:(int *)id;
- (void)receiveMsg;
- (void)timeoutOccurred;
- (void)commandRequestOccurred;
- (void)interruptOccurred;
- (void)interruptOccurredAt:(int)localNum;
- (void)otherOccurred:(int)id;

@end	/* IODirectDevice */

@interface IODirectDevice(IOInterrupts)
/*
 *  Dealing with interrupts.
 */
- (IOReturn) enableAllInterrupts;
- (void) disableAllInterrupts;

- (IOReturn) enableInterrupt	: (unsigned int) localInterrupt;
- (void) disableInterrupt	: (unsigned int) localInterrupt;

/*
 *  Implement this method to provide your own function as the handler for a
 *  particular local interrupt, and to specify a level for that handler to run
 *  at.  You can also specify an argument that gets passed to your handler.
 *  This method will be called once before an interrupt is enabled.
 *
 *  This method returns NO by default.
 */
- (BOOL) getHandler		: (IOInterruptHandler *)handler
              		  level : (unsigned int *)ipl
	  	       argument : (unsigned int *) arg
       		   forInterrupt : (unsigned int) localInterrupt;

@end

@interface IODirectDevice(IOMemory)
/*
 *  Methods to map device memory into the calling task's address space.
 */
- (IOReturn) mapMemoryRange	: (unsigned int) localRange
			     to : (vm_address_t *) destAddr
		      findSpace : (BOOL) findSpace
			  cache : (IOCache) cache;

- (void) unmapMemoryRange 	: (unsigned int) localRange
			   from : (vm_address_t) virtAddr;

@end

void
IOSendInterrupt(
	void			*interrupt,
	void			*state,
	unsigned int	msgId
);

void
IOEnableInterrupt(
	void			*interrupt
);

void
IODisableInterrupt(
	void			*interrupt
);

