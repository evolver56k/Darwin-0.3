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
 * userDriver.m.
 * sample user driver. Exec'd by Config.
 */
 
#import "userDriver.h"
#import <driverkit/IODeviceDescription.h>
#import <libc.h>
#import <mach/mach_error.h>
#import <driverkit/driverServer.h>
#import <driverkit/userConfigServer.h>
#import <driverkit/generalFuncs.h>
#import <servers/netname.h>

int main(int argc, char **argv)
{
	IOConfigReturn  crtn;
	kern_return_t 	krtn;
	port_t 		driverSigPort;
	port_t 		devPorts[NUM_DEVICES];
	port_t 		driverPort;
	int 		i;
	id		deviceDescription;
	int		numPorts;
	port_t 		configPort;
	
	IOLog("driver %s: starting\n", argv[0], 2,3,4,5);
	
	port_allocate(task_self(), &driverPort);
	krtn = netname_look_up(name_server_port,
		"",					// hostname
		CONFIG_SERVER_NAME,
		&configPort);
	if(krtn) {
		IOLog("%s: can't find %s: %s\n",
			argv[0], CONFIG_SERVER_NAME, mach_error_string(krtn),
			4,5);
		exit(1);
	}
	if(crtn = IOGetDevicePorts(NUM_DEVICES,
			"userDriver",
			devPorts, 
			&numPorts,
			&driverSigPort) != IO_CNF_SUCCESS) {
		IOLog("IOGetDevicePorts() returned %d\n", crtn);
		exit(1);
	}
	
	/*
	 * Register with Config.
	 */
	crtn = IORegisterDriver(configPort,
		driverSigPort,
		driverPort);	
	if(crtn) {
		IOLog("%s: IORegisterDriver: crtn = %d\n",
			argv[0], crtn, 3,4,5);
		exit(1);
	}
	
	/*
	 * Start up a driver instance for each dev_port.
	 */
	for(i=0; i<NUM_DEVICES; i++) {
		deviceDescription = [IODeviceDescription new];
		[deviceDescription setDevicePort:devPorts[i]];
		[MyDevice probe:deviceDescription];
	}
	
	/*
	 * Sleep until killed.
	 */
	while(1)
		sleep(1);
	exit(0);
}

#if	0
/*
 * Look up all of our devPorts.  Returns # of ports found.
 */
static int getDevrPort(port_t *devPorts, 
	int maxDevices,
	const char *driverName)
{
	int i;
	name_array_t serviceNames;
	unsigned int serviceCount;
	name_array_t serverNames;
	unsigned int serverCount;
	bool_array_t serviceActive;
	unsigned int serviceActiveCount;
	kern_return_t krtn;
	int portIndex = 0;
	
	krtn = bootstrap_info(bootstrap_port, 
		&serviceNames, 
		&serviceCount,
		&serverNames, 
		&serverCount, 
		&serviceActive, 
		&serviceActiveCount);
	if (krtn != BOOTSTRAP_SUCCESS) {
		IOLog("%s: bootstrap_info: %s", 
			driverName, mach_error_string(krtn), 3,4,5);
		return(PORT_NULL);
	}

	/*
	 * Search for devr_XXXX_XXXX. Later - versions and dev_index via
	 * dev_port_to_type().
	 */
	IOLog("%s: serviceCount %d\n", driverName, serviceCount, 3,4,5);
	for (i = 0; i < serviceCount; i++) {
#ifdef	notdef
		IOLog("%s: serviceName %s\n", driverName, serviceNames[i],
			4,5);
#endif	notdef
		if(strncmp(serviceNames[i], 
		    "dev_port_", strlen("dev_port_")) == 0) {
			IOLog("%s: port %s found\n", 
				driverName, serviceNames[i], 3,4,5);
			
			/*
			 * Get the devPort. 
			 */
			krtn = bootstrap_look_up(bootstrap_port,
				serviceNames[i],
				&devPorts[portIndex]);
			if(krtn) {
				IOLog("%s: bootstrap_look_up: %s",
					driverName, mach_error_string(krtn),
					3,4,5);
				return(0);
			}
			else {
				if(++portIndex > maxDevices) {
					IOLog("%s: maxDevices exceeded\n",
						driverName, 2,3,4,5);
					return(portIndex);
				}
			}
		}
	}
	IOLog("%s: %d devPorts found\n", driverName, portIndex, 3,4,5);
	return(portIndex);
}

#endif	0

/*
 * Simple driver class.
 */
@implementation MyDevice

static int unitNum;

+ (BOOL)probe : deviceDescription
{
	MyDevice *myId;
	IOString devName;
	
	myId = [self alloc];
	[myId setDeviceDescription:deviceDescription];
	[myId setUnit:unitNum];
	sprintf(devName, "MyDevice%d", unitNum++);
	[myId setName:devName];
	[myId setDeviceKind:"Example Driver"];
	[myId setLocation:NULL];
	[myId MyDeviceInit];
	return YES;
}
/*
 * Device-specific initialization.
 */
- MyDeviceInit
{
	IOLog("MyDeviceInit: unit %d\n", [self unit], 2,3,4,5);
	
	/*
	 * ...initialization code here.
	 */
	return self;
}


@end
