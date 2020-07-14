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
 * Test of NXLock, NXConditionLock, and NXSpinLock (kernel version).
 */

#define KERNEL_PRIVATE	1		// pick up kernel private files

#import <kernserv/kern_server_types.h>
#import <mach/message.h>
#import <driverkit/generalFuncs.h>

extern int doNXLockTest(int opcode);

kern_server_t klibtest_ks_var;

/*
 * kernserv callout functions.
 */
void locktest_announce(int unit);
void locktest_port_gone(port_name_t port);
void locktest_terminate(int unit);
void locktest_server(msg_header_t *in_p, int unit);

/* 
 * we're just placed into memory here.
 */
void locktest_announce(int unit) 
{
	IOLog("klibtest Loaded\n", unit);
} 

void locktest_port_gone(port_name_t port) 
{
	IOLog("klibtest port death\n");
	
}

void locktest_terminate(int unit) 
{
	IOLog("klibtest Unloaded\n");
}

/*
 * This is invoked upon receipt of any message. (Use the smsg utility to
 * send an empty message here.)
 */
void locktest_server(msg_header_t *in_p, int unit)
{
	int opcode = in_p->msg_id;
	
	IOLog("klibtest: Starting\n");
	doNXLockTest(opcode);
	msg_send(in_p, 
		MSG_OPTION_NONE,
		0);	
} /* locktest_server() */


