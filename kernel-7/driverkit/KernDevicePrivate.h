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
 * Private declarations for Kernel Device Object.
 *
 * HISTORY
 *
 * 27 Feb 1994 ? at NeXT
 *	Major rewrite.
 * 3 Oct 1993 ? at NeXT
 *	Created.
 */

#ifdef	KERNEL_PRIVATE
 
#import <mach/mach_types.h>
#import <ipc/ipc_kmsg.h>
#import <kern/thread_call_private.h>
 
#import <objc/objc.h>
				
@interface KernDevice(Private)

- (void)_detachInterruptSources;

@end

typedef struct KernDeviceInterrupt_ {
    @defs(KernDeviceInterrupt)
} KernDeviceInterrupt_;

/*
 * Information used to send an ipc
 * message from interrupt context.
 * Besides one static reference, we
 * hold an extra reference to the
 * interrupt port.  This obviates
 * the need to aquire a reference
 * at interrupt time for the in
 * transit message.
 */

typedef struct _KernDeviceInterruptMsg {
    struct ipc_kmsg		kmsg;
    ipc_port_t			iport;
    id				lock;
    struct _thread_call	callout;
    BOOL			queued	:1,	/* on message queue */
				pending	:1,	/* callout pending */
				destroy	:1,	/* destroy on release */
					:0;
} KernDeviceInterruptMsg;

void
KernDeviceInterruptMsgRelease(
	void				*msg
);

static
KernDeviceInterruptMsg *
_KernDeviceInterruptMsgCreate(
	ipc_port_t			interruptPort
);
static
void
_KernDeviceInterruptMsgDestroy(
	KernDeviceInterruptMsg		*interruptMsg
);

static
void
_KernDeviceInterruptCallout(
    	KernDeviceInterruptMsg		*interruptMsg
);

#endif
