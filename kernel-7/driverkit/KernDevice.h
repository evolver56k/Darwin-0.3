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
 * Exported interface for Kernel Device Object.
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

#ifdef	DRIVER_PRIVATE

#import <mach/mach_types.h>

#import <objc/Object.h>

/*
 * This object supports the
 * delivery of hardware device
 * interrupts via an IPC message
 * sent to a port.
 */

@interface KernDevice : Object
{
@private
    void	*_interruptPort;
    id		_interrupts;
    id		_deviceDescription;
}

- initWithDeviceDescription: deviceDescription;

- deviceDescription;

- attachInterruptPort: (port_t)interruptPort;
- detachInterruptPort;
- interrupt: (int)index;
- interrupts;

@end

@interface KernDeviceInterrupt : Object
{
@private
    id			_busInterrupt;
    id			_lock;
    void		*_handler;
    void		*_handlerArgument;
    void		*_ipcMessage;
    BOOL		_isSuspended;
}

- initWithInterruptPort: (void *)port;

- attachToBusInterrupt: busInterrupt
	    withArgument: (void *)argument;
- attachToBusInterrupt: busInterrupt
	withSpecialHandler: (void *)handler
		argument: (void *)argument
		atLevel: (int)level;
- detach;

- suspend;
- resume;

@end

/*
 * This primative is used
 * to deliver an interrupt
 * message to an IPC port
 * from an interrupt handler.
 * This routine is public API.
 */

void
IOSendInterrupt(
	void			*interrupt,
	void			*state,
	unsigned int		msgId
);

/*
 * These primatives provide the
 * ability to manipulate a device
 * interrupt from an interrupt
 * handler.  These only work
 * correctly from inside a bone-
 * fide interrupt handler.
 */

void
IOEnableInterrupt(
	void			*interrupt
);

void
IODisableInterrupt(
	void			*interrupt
);

/*
 * One of these primatives should
 * be invoked to indicate the
 * assertion of a hardware bus
 * interrupt for an attached device
 * interrupt.
 */
#if sparc
int
#else
void
#endif
KernDeviceInterruptDispatch(
	KernDeviceInterrupt	*interrupt,
	void			*state
);

void
KernDeviceInterruptDispatchShared(
	KernDeviceInterrupt	*interrupt,
	void			*state
);

#endif
