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
 * myDriver.c - simple driver example. This driver is started up by
 *	Config, from which a devicePort is obtained. We then load 
 *	a loadable kernel server into the kernel and pass the device
 * 	port to the LKS.
 *
 * HISTORY
 * 18-Dec-92    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/generalFuncs.h>
#import <driverkit/userConfigServer.h>
#import <mach/mach.h>
#import <servers/netname.h>
#import "kl_com.h"
#import <myHandler.h>		// MIG generated, in the build directory

#define MAX_DEVICE_PORTS	2

#define RELOCATABLE_FILE \
	"/Net/grrr/mknrw/dmitch/DRIVERKIT/driverkit/Examples/loadable/Kernel/m88krelocdebug/myHandler_reloc"

#define SERVER_NAME		"myHandler"
#define SERVER_PORT_NAME	"myHandler"


static port_t driverSigPort;	// created for us by IOGetDevicePorts()
static port_t serverPort;	// communication with LKS via this

static int loadServer();

/* 
 * When true, main() wait for debugger connection before proceeding.
 */
#define WAIT_FOR_DEBUG	0
 
int main(int arcg, char **argv)
{
	port_t devicePorts[MAX_DEVICE_PORTS];
	int numDevPorts;
	IOConfigReturn crtn;
	
	/*
	 * Initialize libraries.
	 */
	IOInitGeneralFuncs();

#if	WAIT_FOR_DEBUG
	{
		volatile int proceed = 0;
	
		IOLog("myDriver: waiting for debugger connection\n");
		while(!proceed) {
			IOSleep(100);
		}
	}
#endif	WAIT_FOR_DEBUG

	/* 
	 * Get device ports for each instance of the printer hardware
	 * from Config.
	 */
	crtn = IOGetDevicePorts(MAX_DEVICE_PORTS,
		"myDriver",
		devicePorts,
		&numDevPorts,
		&driverSigPort);
	if(crtn) {
		exit(1);
	}
	if(numDevPorts == 0) {
		IOLog("myDevice: No Device Ports available; aborting\n");
		exit(1);
	}
		
	/*
	 * Load in the LKS.
	 */
	if(loadServer()) {
		exit(1);
	}
	IOLog("myDriver: server %s loaded\n", SERVER_NAME);
	
	/*
	 * Pass down the device port and initialize device.
	 */
	setDevicePort(serverPort, devicePorts[0]);
	initDevice(serverPort);
	
	/*
	 * Shut down driver.
	 */
	shutDown(serverPort);
	IOLog("myDriver: Done; exiting\n");
	return 0;	
}

/*
 * Load the server into kernel memory and initiate 
 * communication with it. Returns 0 if successful.
 */
static int 
loadServer()
{
	int rtn;
	kern_return_t krtn;
	
	/*
	 * First load the parasite server into kernel memory.
	 */
	rtn = kl_com_add(RELOCATABLE_FILE, SERVER_NAME);
	if(rtn) {
		return(1);
	}
	rtn = kl_com_load(SERVER_NAME);
	if(rtn) {
		return(1);
	}
	
	/*
	 * Sync up with kern_loader.
	 */
	kl_com_wait();
	
	/*
	 * Establish contact with the server.
	 */
	krtn = netname_look_up(name_server_port, 
		"",			// must be local host
		SERVER_PORT_NAME, 
		&serverPort);
	if(krtn) {
	    	IOLog("myDriver: Can\'t find server port (error %d)", krtn);
		return 1;
	}
	
	return 0;
}

