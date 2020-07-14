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
 * simple dev_server test
 */

#import <driverkit/driverServer.h>
#import <architecture/nrw/adb_regs.h>
#import <driverkit/generalFuncs.h>
#import <libc.h>
#import <mach/mach.h>
#import <driverkit/IODevice.h>

extern port_t device_master_self();

static void usage(char **argv);

int main(int argc, char **argv) 
{
	kern_return_t 	krtn;
	IOReturn 	drtn;
	port_t	 	devPort;
	char 		*regAddrs;
	IODeviceType 	devType;
	IOSlotId 	slotId;
	port_t 		deviceMaster;
	int 		rtn = 0;
	port_t 		interruptPort;
	IODeviceNumber 	devNum = 0;
	BOOL 		inUse;
	int 		arg;
	int 		ourDev=-1; 
	
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'd':
		    	ourDev = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
			break;
		}
	}
	deviceMaster = device_master_self();
	printf("deviceMaster = %d\n", deviceMaster);
	
	/*
	 * List all of the devices the kernel knows about.
	 */
	while(1) {
		drtn = _IOLookupByDeviceNumber(deviceMaster,
			devNum,
			&slotId,
			&devType,
			&inUse);	
		if(drtn == IO_R_NO_DEVICE) {
			break;
		}	
		printf("devNum %d: \n", devNum);
		printf("   slotId 0x%x   devType 0x%x  inUse %s\n",
			slotId, devType, (inUse ? "TRUE" : "FALSE"));
		devNum++;
	}
	if(ourDev == -1) {
		exit(0);
	}
	drtn = _IOCreateDevicePort(deviceMaster,
		task_self(),
		ourDev,
		&devPort);
	if(drtn) {
		printf("***Can't create devicePort (%s)\n",
			[IODevice stringFromReturn:drtn]);
		exit(1);
	}
	else {
		printf("devPort %d\n", devPort);
	}

#if	!i386	
	drtn = _IOLookupByDevicePort(devPort,
		&slotId,
		&devType);
	if(drtn) {
		printf("***Can't get devType (%s)\n",
			[IODevice stringFromReturn:drtn]);
		rtn = 1;
		goto done;
	}
	else {
		printf("...devType 0x%x slotId 0x%x\n", devType, slotId);
	}
		
	drtn = _IOMapDevicePage(devPort,
		task_self(),
		(vm_offset_t *)&regAddrs,
		YES,				// anywhere
		IO_CacheOff);			// cache inhibit
	if(drtn) {
		printf("***Can't map regs (%s)\n",
			[IODevice stringFromReturn:drtn]);
		rtn = 1;
		goto done;
	}
	printf("regAddrs 0x%x\n", regAddrs);
	
	printf("*reg  = 0x%02x 0x%02x 0x%02x 0x%02x\n", 
		regAddrs[0], regAddrs[1], regAddrs[2], regAddrs[3]);
#endif
		
	krtn = port_allocate(task_self(), &interruptPort);
	if(krtn) {
		printf("***port_allocate returned %d\n", krtn);
		rtn = 1;
		goto done;
	}
	drtn = _IOAttachInterrupt(devPort, interruptPort);
	if(drtn) {
		printf("***Can't attach interrupts (%s)\n",
			[IODevice stringFromReturn:drtn]);
		rtn = 1;
		goto done;
	}
	printf("*reg  = 0x%02x 0x%02x 0x%02x 0x%02x\n", 
		regAddrs[0], regAddrs[1], regAddrs[2], regAddrs[3]);
	printf("...Success\n");
done:
	drtn = _IODestroyDevicePort(deviceMaster, devPort);
	if(drtn) {
		printf("***Can't destroy devPort (%s)\n",
			[IODevice stringFromReturn:drtn]);
	}
	return rtn;
}	
static void usage(char **argv) 
{
	printf("Usage: %s [d=deviceNumber]\n", argv[0]);
	exit(1);
}
