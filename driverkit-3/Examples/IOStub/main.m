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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * main.m - top-level module for IOStub example, Distributed Objects
 *	    version.
 *
 * HISTORY
 * 09-Jun-92    Doug Mitchell at NeXT
 *      Created. 
 */

#import "IOStub.h"
#import <driverkit/generalFuncs.h>
#import <remote/NXConnection.h>
#import <objc/error.h>
#import <mach/cthreads.h>
#import <mach/mach.h>
#import <driverkit/debugging.h>

#define	NUM_STUB_DEVICES	2
#define	THREADS_PER_DEVICE	2

int main(int arcg, char **argv)
{
	id stub;
	id roserver;
	int stubNum;
	int threadNum;
	
	IOInitDDM(500, "IOStubXpr");
	IOInitGeneralFuncs();
	for(stubNum=0; stubNum<NUM_STUB_DEVICES; stubNum++) {
		stub = [[IOStub alloc] init];
		if(stub == nil) {
			IOLog("IOSTub: can\'t init\n");
			exit(1);
		}
		if([stub initStub:stubNum] == nil) {
			IOLog("IOStub: initStub failed\n");
			exit(1);
		}
		
		/*
		 * Set up an NXConnection, the means by which all clients
		 * communicate with us.
		 */
		NX_DURING {
			roserver = [NXConnection registerRoot:stub
				 withName:[stub name]];
		} NX_HANDLER {
			IOLog("registerRoot raised %d\n", 
				NXLocalHandler.code);
			exit(1);
		} NX_ENDHANDLER
		
		if(roserver == nil) {
			IOLog("registerRoot returned nil\n");
			exit(1);
		}
		
		/*
		 * Start up "API threads".
		 */
		for(threadNum=0; threadNum<THREADS_PER_DEVICE; threadNum++) {
			NX_DURING {
				[roserver runInNewThread];
			} NX_HANDLER {
				IOLog("runInNewThread raised %d\n", 
					NXLocalHandler.code);
				exit(1);
			} NX_ENDHANDLER;
		}
	} 
	
	/*
	 * Success. This thread is done; all subsequent work is done by 
	 * the IOStub objects' I/O threads (started in initStub:) and
	 * by the NXProxy threads.
	 */
	cthread_exit(0);
	return 0;	
}

