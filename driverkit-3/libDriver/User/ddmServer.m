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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * ddmServer.m - user-level ddm server module.
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h>
#import <driverkit/debugging.h>
#import <driverkit/ddmPrivate.h>
#import <driverkit/debuggingMsg.h>
#import <driverkit/generalFuncs.h>
#ifdef	KERNEL
#import <m68k/dev/ldd.h>
#import <mach/mach_interface.h>
#else	KERNEL
#import <mach/mach.h>
#import <bsd/libc.h>
#endif	KERNEL
#import <servers/netname.h>
#import <machkit/NXLock.h>

static void ddmGetString(IODDMMsg *msgp);

/*
 * xpr server thread. This advertises specified port name with the nmserver,
 * the looops waiting for incoming requests from an XPR Viewer app. These
 * requests are generally either IODDMMasks set operations or requests for
 * strings cooked up from xprArray. 
 */
volatile void xprServerThread(char *portName)
{
	port_name_t xprServerPort, sigPort;
	kern_return_t krtn;
	netname_name_t regname;
	IODDMMsg *msgp;
	

	/*
	 * Set up port which external utility uses to communicate with us.
	 */
	krtn = port_allocate(task_self(), &xprServerPort);
	if(krtn) {
		IOLog("xprServerThread: port_allocate returned %d\n", krtn);
		IOExitThread();
	}
	krtn = port_allocate(task_self(), &sigPort);
	if(krtn) {
		IOLog("xprServerThread: port_allocate returned %d\n", krtn);
		IOExitThread();
	}
	strcpy(regname, portName);
	krtn = netname_check_in(name_server_port,
		regname,
		sigPort,
		xprServerPort);
	if(krtn) {
		IOLog("xprServerThread: netname_check_in returned %d\n", krtn);
		IOExitThread();
	}

	msgp = IOMalloc(sizeof(*msgp));
	
	/*
	 * Main loop. 
	 */
	while(1) {
		msgp->header.msg_local_port = xprServerPort;
		msgp->header.msg_size = sizeof(*msgp);
		krtn = msg_receive(&msgp->header, MSG_OPTION_NONE, 0);
		if(krtn) {
			IOLog("xprServerThread: msg_receive returned %d\n",
				krtn);
			continue;
		}
		msgp->status = IO_DDM_SUCCESS;
		switch(msgp->header.msg_id) {
		    case IO_LOCK_DDM_MSG:
		    	[xprLock lock];
		    	xprLocked = 1;
			[xprLock unlock];
			break;
		    case IO_UNLOCK_DDM_MSG:
		    	[xprLock lock];
		    	xprLocked = 0;
			[xprLock unlock];
			break;
		    case IO_GET_DDM_ENTRY_MSG:
		    	ddmGetString(msgp);
			break;
		    case IO_SET_DDM_MASK_MSG:
		    	IOSetDDMMask(msgp->index, msgp->maskValue);
			break;
		    case IO_CLEAR_DDM_MSG:
			IOClearDDM();
			break;
		    default:
		    	IOLog("xprServerThread: bogus msg_id (0x%x)\n",
				(u_int)msgp->header.msg_id);
			break;
		}
		
		/*
		 * Return message.
		 */
		krtn = msg_send(&msgp->header, MSG_OPTION_NONE, 0);
		if(krtn) {
			IOLog("xprServerThread: msg_send returned %d\n", krtn);
			continue;
		}
		
	}
	/* NOT REACHED */	
}

/*
 * Cook up one xpr string from the entry in xprArray[msgp->index] prior to 
 * xprLast, stash it in msgp->string. xprLocked really should be true here, but
 * if it isn't, that's the XPRViewer utility's problem.
 */
static void ddmGetString(IODDMMsg *msgp)
{
	unsigned long long timestamp;
	
	if(IOGetDDMEntry(msgp->index,
		IO_DDM_STRING_LENGTH,
		msgp->string,
		&timestamp,
		&msgp->cpuNumber)) {
		
		msgp->status = IO_NO_DDM_BUFFER;
	}
	else {
		msgp->timestampLowInt = (unsigned)timestamp; 
		msgp->timestampHighInt  = (unsigned)(timestamp >> 32);
	}
	return;
}
