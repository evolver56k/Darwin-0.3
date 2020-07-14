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
 * getDevPort.c - Get device ports from Config.
 *
 * HISTORY
 * 25-Jun-92    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/driverServer.h>
#import <driverkit/userConfigServer.h>
#import <driverkit/generalFuncs.h>
#import <mach/mach_error.h>
#import <bsd/sys/types.h>
#import <mach/mach.h>
#import <servers/bootstrap.h>
#import <servers/netname.h>
#import <bsd/libc.h>

#define	DPRINT_DEBUG	0
#if	DPRINT_DEBUG
#define	dprintf(x,a,b,c,d,e)  IOLog(x,a,b,c,d,e)
#else	DPRINT_DEBUG
#define	dprintf(x,a,b,c,d,e)
#endif	DPRINT_DEBUG

#define	LOG_SERVICE_NAMES	0

/*
 * look up all of a driver's devicePorts. returns # of ports found in
 * *numDevicePorts.
 * Returns IO_CNF_SUCCESS normally (even if no ports found); 
 * IO_CNF_BOOTSTRAP on other error.
 */
IOConfigReturn IOGetDevicePorts(int maxDevices,
	const char *driverName,		// for error logging only 
	port_t *devicePorts, 		// returned
	int *numDevicePorts,		// returned
	port_t *driverSigPort)		// returned
{
	int 		i;
	name_array_t 	service_names;
	unsigned int 	service_cnt;
	name_array_t 	server_names;
	unsigned int 	server_cnt;
	bool_array_t 	service_active;
	unsigned int 	service_active_cnt;
	kern_return_t 	krtn;
	int 		port_index = 0;
	
	*numDevicePorts = 0;
	
	/*
	 * Get some basic communication ports.
	 */
	krtn = bootstrap_look_up(bootstrap_port, 
		SIG_PORT_NAME,
		driverSigPort);
	if(krtn) {
		IOLog("%s: can't find %s: %s\n", driverName,
			SIG_PORT_NAME, mach_error_string(krtn));
		return IO_CNF_BOOTSTRAP;
	}
	 
	/*
	 * Look for our device ports, provided to us by Config.
	 */
	krtn = bootstrap_info(bootstrap_port, 
		&service_names, 
		&service_cnt,
		&server_names, 
		&server_cnt, 
		&service_active, 
		&service_active_cnt);
	if (krtn != BOOTSTRAP_SUCCESS) {
		IOLog("%s: bootstrap_info: %s", driverName,
			mach_error_string(krtn));
		return IO_CNF_BOOTSTRAP;
	}

	/*
	 * Search for dev_port_XXX. Later - versions and dev_index via
	 * dev_port_to_type().
	 */
	dprintf("%s: service_cnt %d\n", driverName, service_cnt, 3,4,5);
	for (i = 0; i < service_cnt; i++) {
#if	LOG_SERVICE_NAMES
		printf("%s: service_name %s\n", driverName, service_names[i],
			4,5);
#endif	LOG_SERVICE_NAMES
		if(strncmp(service_names[i], 
		    "dev_port_", strlen("dev_port_")) == 0) {
			dprintf("%s: port %s found\n", 
				driverName, service_names[i], 3,4,5);
			
			/*
			 * Get the dev_port. 
			 */
			krtn = bootstrap_look_up(bootstrap_port,
				service_names[i],
				&devicePorts[port_index]);
			if(krtn) {
				IOLog("%s: bootstrap_look_up: %s",
					driverName, mach_error_string(krtn));
				return IO_CNF_BOOTSTRAP;
			}
			else {
				if(++port_index > maxDevices) {
					IOLog("%s: num_devices exceeded\n",
						driverName);
					*numDevicePorts = port_index;
					return IO_CNF_SUCCESS;
				}
			}
		} 	/* device port found */
	} 		/* for each port in bootstrap server */
	
	dprintf("%s: %d devPorts found\n", driverName, port_index, 3,4,5);
	*numDevicePorts = port_index;
	return IO_CNF_SUCCESS;
}

