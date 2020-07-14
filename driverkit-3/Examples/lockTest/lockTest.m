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
 * Test of NXLock, NXConditionLock, and NXSpinLock (user version).
 */

#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>
#import <bsd/sys/printf.h>

static int ioThread();

id lock;
id condLock;
id spinLock;

/*
 * flags meean "I'm waiting for you" when YES.
 */
BOOL threadFlag = NO;
BOOL mainFlag = NO;

/*
 *  part   	main			thread
 *  ----        -------------------     -----------------------
 *   1	   	mainFlag NO		threadFlag NO 
 *		grab spin lock		wait for mainFlag
 *		mainFlag YES		
 *		wait for threadFlag
 *					threadFlag YES
 *					try to acquire spinLock (held by main)
 *		mainFlag NO
 *		sleep
 *		unlock spin lock	
 *		wait for threadFlag NO
 *					unlock spinLock
 *					threadFlag NO
 *
 *  2		grab lock		wait for mainFlag
 *		mainFlag YES		
 *		wait for threadFlag
 *					threadFlag YES
 *					try to acquire lock (held by main)
 *		mainFlag NO
 *		sleep
 *		unlock lock	
 *		wait for threadFlag NO
 *					unlock spinLock
 *					threadFlag NO
 *
 *  3		grab condLock		wait for mainFlag
 *		mainFlag YES
 *		wait for threadFlag
 *					threadFlag YES
 *					[condLock lockWhen 1]
 *		mainFlag NO
 *		sleep
 *		[condLock unlockWith 1]
 *		[condLock lockWhen 2]
 *					sleep
 *					[condLock unlockWith 2]
 *		done			done
 */
 
int main(int argc, char **argv)
{
	/*
	 * Init the three locks.
	 */
	lock     = [NXLock new];
	condLock = [NXConditionLock alloc];
	[condLock initWith:0];
	spinLock = [NXSpinLock new];
	
	/*
	 * Start up the I/O thread.
	 */
	IOForkThread((IOThreadFunc)ioThread, (void *)ioThread);
	
	/*
	 * Part 1.
	 */
	[spinLock lock];
	mainFlag = YES;
	while(!threadFlag)
		;
	mainFlag = NO;
	printf("1 (main) - threadFlag YES, main() holds spin lock\n");
	IOSleep(1000);
	[spinLock unlock];
	
	/*
	 * Wait for I/O Thread to finish part 1.
	 */
	while(threadFlag)
		;
		
	/*
	 * Part 2.
	 */
	[lock lock];
	mainFlag = YES;
	while(!threadFlag)
		;
	mainFlag = NO;
	printf("2 (main) - threadFlag YES, main() holds lock\n");
	IOSleep(1000);
	[lock unlock];
	
	/*
	 * Wait for I/O Thread to finish part 2.
	 */
	while(threadFlag)
		;

	/*
	 * Part 3.
	 */		
	[condLock lock];
	mainFlag = YES;
	while(!threadFlag)
		;
	printf("3 (main) - threadFlag YES, main() holds condLock\n");
	IOSleep(1000);
	[condLock unlockWith:1];
	[condLock lockWhen:2];
	printf("3 (main) - got condLock:2; done\n");
	return(0);
}

static int ioThread()
{
	/*
	 * part 1.
	 */
	while(!mainFlag)
		;
	threadFlag = YES;
	[spinLock lock];
	printf("1 (thread) - thread got spinLock\n");
	[spinLock unlock];
	threadFlag = NO;
	
	/*
	 * part 2.
	 */
	while(!mainFlag)
		;
	threadFlag = YES;
	[lock lock];
	printf("2 (thread) - thread got lock\n");
	[lock unlock];
	threadFlag = NO;
	
	/*
	 * Part 3.
	 */
	while(!mainFlag)
		;
	threadFlag = YES;
	[condLock lockWhen:1];
	printf("3 (thread) - got condLock:1\n");
	IOSleep(1000);
	[condLock unlockWith:2];
	IOExitThread();
}
