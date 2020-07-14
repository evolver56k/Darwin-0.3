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
 
/*
 * Operations (performed sequentially per msg_id passed to server)
 */
#define NXL_LIB_INIT	1		// call IOInitGeneralFuncs()
#define NXL_OBJ_ALLOC	2		// alloc an Object
#define NXL_OBJ_INIT	3		// init an Object
#define NXL_LOCK_INIT	5		// alloc and init all 3 locks
#define NXL_THREAD	6		// start up ioThread
#define NXL_PART1	7		// part 1 - NXSpinLock test
#define NXL_PART2	8		// part 2 - NXLock test
#define NXL_PART3	9		// part 2 - NXConditionLock test

/*
 * Kernel can't do spin lock tests on a single CPU.
 */
#define DO_SPINLOCK	0

#define KERNEL		1
#define KERNEL_PRIVATE	1		// pick up kernel private files

#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>
#import <kernserv/kern_server_types.h>
#import <mach/message.h>

static int do_NXLockTest(int opcode);
static int ioThread(void *param);

kern_server_t NXLock_ks_var;
id nxLock;
id nxCondLock;
id nxSpinLock;
int libInitFlag = 1;

/*
 * flags mean "I'm waiting for you" when true.
 */
boolean_t threadFlag;
boolean_t mainFlag;

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
	IOLog("NXLockTest Loaded\n", unit);
} 

void locktest_port_gone(port_name_t port) 
{
	IOLog("NXLockTest port death\n");
	
}

void locktest_terminate(int unit) 
{
	IOLog("NXLockTest Unloaded\n");
}

/*
 * This is invoked upon receipt of any message. (Use the smsg utility to
 * send an empty message here.)
 */
void locktest_server(msg_header_t *in_p, int unit)
{
	int opcode = in_p->msg_id;
	id foo;
	
	IOLog("NXLockTest: Starting\n");
	
#ifdef	TEST_IN_KERNEL
	doNXLockTest(opcode);
	goto done;
#endif	TEST_IN_KERNEL

	/*
	 * First initialize the libraries we'll use. There's no shutdown
	 * needed later (or is there...?).
	 */
	if(opcode < NXL_LIB_INIT) 
		goto done;
	if(!libInitFlag) {
		IOInitGeneralFuncs();	
		libInitFlag = 1;
	}
	IOLog("IOInitGeneralFunc called\n");

	/*
	 * Just to try out objc run time, alloc and init an Object.
	 */
	if(opcode < NXL_OBJ_ALLOC)
		goto done;
	foo = [Object alloc];
	if(opcode < NXL_OBJ_INIT)
		goto done;
	IOLog("Object alloc'd\n");
	[foo init];
	IOLog("Object init'd\n");

	do_NXLockTest(opcode);

	/*
	 * return message to loader.
	 */
done:
	msg_send(in_p, 
		MSG_OPTION_NONE,
		0);	
} /* locktest_server() */

/*
 * Call to avoid spinning.
 */
static void yield()
{
	IOSleep(10);
}

/*
 * Actual NXLock test. do_NXLockTest runs from our msg_receive thread in
 * kernserv. This thread forks off ioThread() and the two perform the
 * following:
 *
 *  part   	main			ioThread
 *   1	   	mainFlag FALSE		threadFlag FALSE 
 *		grab spin lock		wait for mainFlag
 *		mainFlag TRUE		
 *		wait for threadFlag
 *					threadFlag TRUE
 *					try to acquire spinLock (held by main)
 *		mainFlag FALSE
 *		sleep
 *		unlock spin lock	
 *		wait for threadFlag FALSE
 *					unlock spinLock
 *					threadFlag FALSE
 *
 *  2		grab nxLock		wait for mainFlag
 *		mainFlag TRUE		
 *		wait for threadFlag
 *					threadFlag TRUE
 *					try to acquire nxLock (held by main)
 *		mainFlag FALSE
 *		sleep
 *		unlock nxLock	
 *		wait for threadFlag FALSE
 *					unlock spinLock
 *					threadFlag FALSE
 *
 *  3		grab nxCondLock		wait for mainFlag
 *		mainFlag TRUE
 *		wait for threadFlag
 *					threadFlag TRUE
 *					[condLock lockWhen 1]
 *		mainFlag FALSE
 *		sleep
 *		[condLock unlockWith 1]
 *		[condLock lockWhen 2]
 *					sleep
 *					[condLock unlockWith 2]
 *		done			done
 */
 
static int do_NXLockTest(int opcode)
{
	if(opcode < NXL_LOCK_INIT)
		return 0;
		
	/*
	 * Init the three locks.
	 */
	nxLock     = [NXLock new];
	IOLog("NXLock new'd\n");
	nxCondLock = [NXConditionLock alloc];
	[nxCondLock initWith:0];
	IOLog("NXCondLock initWith:'d\n");
	nxSpinLock = [NXSpinLock new];
	IOLog("NXSpinLock new'd\n");
	
	if(opcode < NXL_THREAD)
		return 0;
		
	/*
	 * Start up the I/O thread.
	 */
	IOForkThread((IOThreadFunc)ioThread, (void *)opcode);
	IOLog("ioThread forked\n");
	if(opcode < NXL_PART1)
		return 0;
		
	/*
	 * Part 1.
	 */
#if	DO_SPINLOCK
	[nxSpinLock lock];
	mainFlag = TRUE;
	while(!threadFlag) {
		yield();
	}

	mainFlag = FALSE;
	IOLog("1 (main) - threadFlag true, main() holds spin lock\n");
	IOSleep(1);
	[nxSpinLock unlock];
	
	/*
	 * Wait for I/O Thread to finish part 1.
	 */
	while(threadFlag) {
		yield();
	}
#endif	DO_SPINLOCK

	if(opcode < NXL_PART2)
		return 0;
		
	/*
	 * Part 2.
	 */
	[nxLock lock];
	mainFlag = TRUE;
	while(!threadFlag) {
		yield();
	}
	mainFlag = FALSE;
	IOLog("2 (main) - threadFlag true, main() holds nxLock\n");
	IOSleep(1);
	[nxLock unlock];
	
	/*
	 * Wait for I/O Thread to finish part 2.
	 */
	while(threadFlag) {
		yield();
	}

	if(opcode < NXL_PART3)
		return 0;
		
	/*
	 * Part 3.
	 */		
	[nxCondLock lock];
	mainFlag = TRUE;
	while(!threadFlag) {
		yield();
	}
	IOLog("3 (main) - threadFlag true, main() holds condLock\n");
	IOSleep(1);
	[nxCondLock unlockWith:1];
	[nxCondLock lockWhen:2];
	IOLog("3 (main) - got condLock:2; done\n");
	
	/*
	 * Free the locks.
	 */
	[nxLock free];
	[nxCondLock free];
	[nxSpinLock free];
	return(0);
}

static int ioThread(void *param)
{
	int opcode = (int)param;
	
	if(opcode < NXL_PART1)
		goto done;
		
	/*
	 * part 1.
	 */
#if	DO_SPINLOCK
	while(!mainFlag)
		;
	threadFlag = TRUE;
	[nxSpinLock lock];
	IOLog("1 (thread) - thread got spinLock\n");
	[nxSpinLock unlock];
	threadFlag = FALSE;
#endif	DO_SPINLOCK
	
	if(opcode < NXL_PART2)
		goto done;
		
	/*
	 * part 2.
	 */
	while(!mainFlag) {
		yield();
	}
	threadFlag = TRUE;
	[nxLock lock];
	IOLog("2 (thread) - thread got nxLock\n");
	[nxLock unlock];
	threadFlag = FALSE;
	
	if(opcode < NXL_PART3)
		goto done;

	/*
	 * Part 3.
	 */
	while(!mainFlag) {
		yield();
	}
	threadFlag = TRUE;
	[nxCondLock lockWhen:1];
	IOLog("3 (thread) - got condLock:1\n");
	IOSleep(1);
	[nxCondLock unlockWith:2];
	
	/*
	 * Done, kill this thread.
	 */
done:
	IOExitThread();
}
