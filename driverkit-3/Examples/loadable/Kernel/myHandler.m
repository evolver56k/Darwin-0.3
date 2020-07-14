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
 * Simple loadable kernel Handler example.
 *
 * This Handler accepts one message, setDevicePort(). This is how the
 * user-level module, XXX, passes the device port obtained from Config
 * down to a kernel-level IODevice.
 */
#import <kernserv/kern_server_types.h>
#import <kernserv/printf.h>
#import <mach/mig_errors.h>
#import "myDevice.h"
#import "myHandlerHandler.h"
#import <driverkit/kernelDriver.h>
#import <driverkit/generalFuncs.h>

kern_return_t setDevicePort(void *arg, port_t devicePort);
kern_return_t initDevice(void *arg);
kern_return_t shutDown(void *arg);

static id myDeviceId;

/*
 * Create the struct used by mig-generated code to convert from 
 * message to setDevicePort() callout.
 */
myHandler_t myHandlerStruct = {
	0,
	100,
	setDevicePort,
	initDevice,
	shutDown
};

/*
 * Allocate an instance variable to be used by the kernel server interface
 * routines for initializing and accessing this service.
 */
kern_server_t myHandlerInstance;

/*
 * Stamp our arival.
 */
void myHandlerInit(void)
{
	printf("myHandler initialized\n");
}

/*
 * Notify the world that we're going away.
 */
void myHandlerSignoff(void)
{
	printf("myHandler unloaded\n");
}

kern_return_t setDevicePort(
	void *arg,		// what's this?
	port_t devicePort)
{
#if	0
	port_t devicePortIOTask = convertPortToIOTask(devicePort);
#else	0
	port_t devicePortIOTask = IOConvertPort(devicePort,
		IO_CurrentTask, IO_KernelIOTask);
#endif	0
	IOLog("myHandler setDevicePort: devicePort %d; %d in IOTask space\n", 
		devicePort, devicePortIOTask);
	myDeviceId = [myDevice alloc];
	[myDeviceId setDevicePort:devicePortIOTask];
	return KERN_SUCCESS;
}

kern_return_t initDevice(void *arg)
{
	IOLog("myHandler initDevice\n");
	[myDeviceId init];
	return KERN_SUCCESS;
}

kern_return_t shutDown(void *arg)
{
	IOLog("myHandler shutDown\n");
	[myDeviceId shutDown];
	[myDevice free];
	return KERN_SUCCESS;
}
