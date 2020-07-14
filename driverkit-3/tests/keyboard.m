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
 * keyboard.m - adbKeyboard test. Requires FAKE_HARDWARE simulation in
 * 	        the adbDriver object.
 */
 
#define FAKE_HARDWARE	1

#import <adb/adbDriver.h>
#import <driverkit/adbKeyboard.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/debugging.h>
#import <driverkit/KernDevUxpr.h>
#import <libc.h>
#import <driverkit/IOtsval.h>
#import <driverkit/kernelDriver.h>

static struct tsval tsvalLast = {0,0};

/*
 * prototypes for private functions.
 */
static void usage(char **argv);

/*
 * Dumb little object which will be a client of AdbKeyboard.
 */
@interface adbClient : Object <adbDeviceClient>
{
	id 	_keyboard;
}

- setKeyboard : keyboard;
@end

@implementation adbClient

/*
 * Called when adbKeyboard detects one new ADB event.
 */
- (void)dispatchAdbEvent : (adbEvent *)event
{
	unsigned reg;
	int regnum;
	unsigned long long regval_l;
	unsigned regval;
	
	printf("%u:%u  delta = %u\n",
		event->timeStamp.high_val, event->timeStamp.low_val,
		ts_diff(&event->timeStamp, &tsvalLast));
	tsvalLast = event->timeStamp;
	printf("   address %d\n", 
		event->deviceAddress);
	for(regnum=0; regnum<=3; regnum++) {
		reg = *((unsigned *)(&event->regs[regnum].data.stand.byte[0]));
		regval_l = adbDeviceRegisterData(&event->regs[regnum]);
		if(regval_l > ULONG_MAX) {
			printf("***length exceeded; using ULONG_MAX\n");
			regval = ULONG_MAX;
		}
		else
			regval = regval_l;
		printf("   register %d: length %d data 0x%x\n",
			regnum, event->regs[regnum].length, regval);
	}
	[_keyboard freeAdbEvent:event];
}

- (IOReturn)relinquishOwnershipRequest	: device;
{
	printf("adbClient: reset relinquishOwnershipRequest\n");
	return IO_R_SUCCESS;
}

- setKeyboard : keyboard
{
	_keyboard = keyboard;
	return self;
}

- (void)canBecomeOwner : device
{
	printf("adbClient: canBecomeOwner\n");
}

@end

extern int verbose;

int main(int argc, char **argv)
{
	int arg;
	id driverId;
	id keyboardId;
	id clientId;
	int reset_interval = 0;
	IOReturn drtn;
	
	if(argc < 1)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'v':
		    	verbose++;
			break;
		    case 'r':
		    	reset_interval = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
		}
	}

        IOInitGeneralFuncs();
	IOInitDDM(200, "adbXpr");
	IOSetDDMMask(XPR_IODEVICE_INDEX, XPR_ADB);

	/*
	 * Instantiate and init a client, the keyboard device, and the driver.
	 */
	clientId = [adbClient new];
	[adbDriver probe:0 deviceMaster:PORT_NULL];
	drtn = IOGetObjectForDeviceName("adb0", &driverId);
	if(drtn) {
		printf("IOGetObjectForDeviceName(adb0) returned %d\n", drtn);
		exit(1);
	}
	if(driverId == nil) {
		printf("adbDriver probe failed; exiting\n");
		exit(1);
	}
	[adbKeyboard probe:driverId];
	drtn = IOGetObjectForDeviceName("adbKeyboard0", &keyboardId);
	if(drtn) {
		printf("IOGetObjectForDeviceName(adbKeyboard0) returned %d\n", drtn);
		exit(1);
	}
	if(keyboardId == nil) {
		printf("adbKeyboard probe failed; exiting\n");
		exit(1);
	}
	[clientId setKeyboard:keyboardId];
	[keyboardId becomeOwner:clientId];

	while(1) {
		if(reset_interval) {
			IOSleep(reset_interval * 1000);
			printf("...resetting bus\n");
			[driverId resetAdb];
		}
		else {
			IOSleep(10);
		}
	}
}

static void usage(char **argv)
{
	printf("usage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\tv  verbose mode\n");
	printf("\tr=reset_interval (in seconds; default is no reset)\n");
	exit(1);
}

