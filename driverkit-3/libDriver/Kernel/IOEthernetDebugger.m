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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Kernel Debugger Category for Ethernet Class.
 *
 * HISTORY
 *
 * 7 December 1992 David E. Bohman at NeXT
 *	Created.
 *
 * 29 May 1997 Dieter Siegmund at NeXT
 * - Added debugger registration call, and made
 *   en_send_pkt() and en_recv_pkt static
 */

#import <mach/boolean.h>
#import <mach/machine/simple_lock.h>
#import	<sys/types.h>
#import	<sys/socket.h>
#import <driverkit/IOEthernet.h>
#import <driverkit/IOEthernetPrivate.h>
#import <kern/kdp_en_debugger.h>

#ifndef ppc
extern int spl3();
#else
extern int splbio();
#endif

static id		debuggerDevice;
#ifndef sparc /* [ */
static int		savedIpl;
#ifndef ppc
int			(*debuggerIplRoutine)() = spl3;
#else
int			(*debuggerIplRoutine)() = splbio;
#endif
static BOOL		_kernDebuggerLocked = NO;
extern simple_lock_t	_kernDebuggerLock;
#endif sparc /* ] */

static void
    en_recv_pkt(
		void		*pkt,
		unsigned int	*pkt_len,
		unsigned int	timeout);

static void
    en_send_pkt(
		void		*pkt,
		unsigned int	pkt_len);

static void
    (*recvPkt_methd)(
		id, SEL,
		void *			pkt,
		unsigned int *		pkt_len,
		unsigned int		timeout);
#define RECV_PKT_SEL		@selector(receivePacket:length:timeout:)

static void
    (*sendPkt_methd)(
		id, SEL,
		void *			pkt,
		unsigned int		pkt_len);
#define SEND_PKT_SEL		@selector(sendPacket:length:)

#ifdef sparc
static BOOL
    (*reset_method)(
		id, SEL, 
		BOOL 		enable);
#define RESET_SEL	@selector(resetAndEnable:)
#endif sparc

@implementation IOEthernet(EthernetDebugger)

- (void)registerAsDebuggerDevice
{
    if (debuggerDevice == nil) {
	/* register our debugger routines */
	kdp_register_send_receive(&en_send_pkt, &en_recv_pkt);
    	(IMP)recvPkt_methd = [self methodFor:RECV_PKT_SEL];
	(IMP)sendPkt_methd = [self methodFor:SEND_PKT_SEL];
#ifndef  sparc
	if ((IMP)recvPkt_methd && (IMP)sendPkt_methd)
	    debuggerDevice = self;
#else
	(IMP)reset_method = [self methodFor:RESET_SEL];
	if ((IMP)recvPkt_methd && (IMP)sendPkt_methd && (IMP)reset_method)
	    debuggerDevice = self;
#endif sparc
    }
}

#ifndef sparc /* [ */
- (void)reserveDebuggerLock
{
    if (self == debuggerDevice) {
	if (_kernDebuggerLocked) {
	    IOLog("reserveDebuggerLock: already locked\n");
	    return;
	}
    	savedIpl = (*debuggerIplRoutine)();
	simple_lock(_kernDebuggerLock);
	_kernDebuggerLocked = YES;
    }
}

- (void)releaseDebuggerLock
{
    if (self == debuggerDevice) {
	if (!_kernDebuggerLocked) {
	    return;
	}
	simple_unlock(_kernDebuggerLock);
	_kernDebuggerLocked = NO;
    	splx(savedIpl);
    }
}

#endif sparc /* ] */
@end

#ifndef sparc /* [ */
void reserveDebuggerLock(id debuggerDeviceObject)
{
    if (debuggerDeviceObject == debuggerDevice) {
	if (_kernDebuggerLocked) {
	    IOLog("reserveDebuggerLock: already locked\n");
	    return;
	}
	simple_lock(_kernDebuggerLock);
	_kernDebuggerLocked = YES;
    }
}

void releaseDebuggerLock(id debuggerDeviceObject)
{
    if (debuggerDeviceObject == debuggerDevice) {
	if (!_kernDebuggerLocked) {
	    return;
	}
	simple_unlock(_kernDebuggerLock);
	_kernDebuggerLocked = NO;
    }
}

#endif sparc /* ] */

static void
en_recv_pkt(
    void		*pkt,
    unsigned int	*pkt_len,
    unsigned int	timeout
)
{
    *pkt_len = 0;
    
    if (debuggerDevice)
    	(*recvPkt_methd)(debuggerDevice, RECV_PKT_SEL, pkt, pkt_len, timeout);    
}

static void
en_send_pkt(
    void		*pkt,
    unsigned int	pkt_len
)
{
    if (debuggerDevice)
    	(*sendPkt_methd)(debuggerDevice, SEND_PKT_SEL, pkt, pkt_len);
}

#ifdef sparc 
void
en_reset(
    BOOL	enable
)
{
    if (debuggerDevice)
    	(*reset_method)(debuggerDevice, RESET_SEL, enable);    
}
#endif sparc