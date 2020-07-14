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
 * User-level libIO timer test.
 */
 
#import <objc/Object.h>
#import <driverkit/generalFuncs.h>
#import <libc.h>
#import <driverkit/Device_ddm.h>

#define DO_DDM_DEBUG 1

static void usage(char **argv);

ns_time_t last_time = 0;
int callout_interval = 10;

static void callFcn()
{
	printf("Callout\n");
	IOScheduleFunc((IOThreadFunc)callFcn, NULL, callout_interval);
}

int main(int argc, char **argv)
{
	int loop;
	ns_time_t current_time;
	unsigned delta;
	unsigned long long delta_l;
	int arg;
	struct tsval ts;
	
	if(argc > 2)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'c':
		    	callout_interval = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
		}
	}
	IOInitGeneralFuncs();
#ifdef	DO_DDM_DEBUG
	IOInitDDM(1000, "libIOxpr");
	IOSetDDMMask(XPR_IODEVICE_INDEX, XPR_LIBIO);
#endif	DO_DDM_DEBUG
	printf("Should see one loop every second, and a callout every %d "
		"loops\n\n", callout_interval);
	IOScheduleFunc((IOThreadFunc)callFcn, NULL, callout_interval);
	for(loop=0; ; loop++) {
		IOGetTimestamp(&current_time);
		delta_l = current_time - last_time;
		delta = delta_l;
		kern_timestamp(&ts);
		printf("...loop %d delta %u ns tsval %u:%u\n", 
			loop, (unsigned)delta_l, ts.high_val, ts.low_val);
		/*
		 * eliminate printf latency...
		 */
		IOGetTimestamp(&last_time);
		
		/*
		 * alternate between IODelay and IOSleep...
		 */
		if(loop & 1) {
			IOSleep(1000);
		}
		else {
			IODelay(1000000);
		}
	}	
}

static void usage(char **argv)
{
	printf("usage: %s [c=c(allout interval)]\n");
	exit(1);
}
