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
 * adbevent - FAKE_HARDWARE event dispatch test
 */
 
#define FAKE_HARDWARE	1

#import <adb/adbDriver.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/debugging.h>
#import <driverkit/KernDevUxpr.h>
#import <libc.h>
#import <driverkit/IOtsval.h>

static struct tsval tsvalLast = {0,0};

/*
 * prototypes for private functions.
 */
static void usage(char **argv);
static void resetThread(id driverid);
static void commandThread(id driverid);

/*
 * Dumb little object which will be a client of AdbDriver.
 */
@interface adbClient : Object <adbDriverClient>
{
	id 	_driver;
}

- setDriver : driver;
@end

@implementation adbClient

/*
 * These standard methods run from the driver's I/O thread.
 */
 
/*
 * Called when driver detects one new ADB event.
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
	[_driver freeAdbEvent:event];
}

/*
 * Called on ADB bus reset.
 */
- (void)adbBusWasReset
{
	printf("adbClient: reset detected\n");
	
	/*
	 * Re-register; the driver thinks the world has changed.
	 */
	[_driver attachAddress: FAKE_HARDWARE_START_ADDRESS client:self];
}

- setDriver : driver
{
	_driver = driver;
	return self;
}

@end

extern int verbose;
int reset_interval = 0;
int command_interval = 0;

int main(int argc, char **argv)
{
	int arg;
	id driverId;
	id clientId;
		
	if(argc < 2)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'v':
		    	verbose++;
			break;
		    case 'r':
		    	reset_interval = atoi(&argv[arg][2]);
			break;
		    case 'c':
		    	command_interval = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
		}
	}

        IOInitGeneralFuncs();
	IOInitDDM(1000, "adbXpr");
	IOSetDDMMask(XPR_IODEVICE_INDEX, XPR_LIBIO | XPR_ADB);

	/*
	 * Instantiate and init a client and the driver.
	 */
	clientId = [adbClient new];
	driverId = [adbDriver probe:0 deviceMaster:PORT_NULL];
	[clientId setDriver:driverId];
	[driverId attachAddress: FAKE_HARDWARE_START_ADDRESS client:clientId];
	if(reset_interval)
		IOForkThread((IOThreadFunc)resetThread, driverId);
	if(command_interval)
		IOForkThread((IOThreadFunc)commandThread, driverId);
	while(1) {
		IOSleep(10);
	}
}

static void resetThread(id driverId)
{
	while(1) {
		IOSleep(reset_interval * 1000);
		[driverId resetAdb];
	}
}

static void commandThread(id driverId)
{
	adbUserCommand cmd;
	IOReturn rtn;
	
	while(1) {
		IOSleep(command_interval * 1000);
		cmd.command = TALK_REGISTER(3, FAKE_HARDWARE_START_ADDRESS);
		cmd.reg.length = 0;
		rtn = [driverId sendUserCommand:&cmd];
		if(rtn) {
			printf("sendUserCommand returned %s\n",
				[driverId stringFromReturn:rtn]);
		}
		else {
			printf("Talk Register 3: data = 0x%x\n",
				cmd.reg.data.reg3.sh.data);
		}
	}
}

static void usage(char **argv)
{
	printf("usage: %s [options]\n", argv[0]);
	printf("Options (must specify at least one):\n");
	printf("\tv  verbose mode\n");
	printf("\tr=reset_interval (in seconds; default is no reset)\n");
	printf("\tc=command_interval (in seconds; default is no command\n");
	exit(1);
}

