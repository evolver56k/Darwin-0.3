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
/*	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * userConfigServer.h - public API for Config server.
 *
 * HISTORY
 * 11-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <bsd/sys/types.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <driverkit/driverServer.h>

/*
 * The IODeleteDevice() RPC is not currently supported.
 */
#define SUPPORT_DEVICE_DELETE	0

/*
 * Config-specific return types.
 */
typedef int IOConfigReturn;

#define IO_CNF_SUCCESS		0	// OK
#define IO_CNF_NOT_REGISTERED	201	// dev_port or driver_sig_port not 
					//    registered
#define IO_CNF_NOT_FOUND	202	// slot_id/dev_type not found
#define IO_CNF_BUSY		203	// operation can't be performed while
					//   autoconfig is progress
#define IO_CNF_REGISTERED	204     // driver_port already registered
#define IO_CNF_RESOURCE		205 	// kernel resource shortage	
#define IO_CNF_ACCESS		206	// device access denied
#define IO_CNF_BOOTSTRAP	207	// bootstrap error

/*
 * Name of public server advertised with nmserver.
 */
#define CONFIG_SERVER_NAME	"devConfigPort"

/*
 * Name of signature port given to each driver.
 */
#define SIG_PORT_NAME		"driverSigPort"

/*
 * Path where executable drivers reside.
 */
#define DRIVER_PATH	"/usr/Devices"

/*
 * RPC prototypes.
 */
IOConfigReturn IORegisterDriver( 
	port_t configPort,
	port_t driverSigPort,		// created by driver
	port_t driverPort);		// created by Config
			
IOConfigReturn IODeleteDriver(
	port_t configPort,
	port_t driverSigPort);	
	
#if	SUPPORT_DEVICE_DELETE
IOConfigReturn IODeleteDevice(
	port_t configPort,
	port_t driverSigPort,
	port_t devicePort);	
#endif	SUPPORT_DEVICE_DELETE

IOConfigReturn IORescanDriver(
	port_t configPort);
	
IOConfigReturn IOConfigDevice(
	port_t configPort,
	IOSlotId slotId,
	IODeviceType deviceType);

#ifdef	DEBUG
IOConfigReturn IODeleteDeviceByType(
	port_t configPort,
	IOSlotId slotId,
	IODeviceType deviceType);
#endif	DEBUG

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
	port_t *driverSigPort);		// returned

