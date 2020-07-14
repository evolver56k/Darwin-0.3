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
 * EventIO.m - Event System MiG interface for driver control and status.
 *
 * HISTORY
 * 2-April-92    Mike Paquette at NeXT 
 *      Created. 
 */

#import <objc/objc.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <bsd/dev/evio.h>

#import <kernserv/queue.h>
#import <bsd/dev/machine/ev_private.h>	/* Per-machine configuration info */
#import <driverkit/EventDriver.h>

/*
 * Event Status services.
 *
 *	These functions invoke the normal Get and Set Parameter methods
 *	from an interface that does not require root access to use.  The
 *	Event Status subsystem is used by all apps that wish to modify
 *	or query system state maintained by the Event Status driver.
 *
 *	Items handled by this driver include key maps, key repeat rate,
 *	wait cursor behavior, mouse handedness and acceleration, screen
 *	brightness and autodim behavior, and much more.  We don't want apps
 *	which manipulate these things to run setuid as root.  (Think of the
 *	havoc that a setuid root version of BackSpace could cause...)
 */
IOReturn EvGetParameterInt(
	port_t dev_port,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned int *parameterArray,		// data returned here
	unsigned int *returnedCount)		// size returned here
{
	id instance;
	IOReturn rtn;
	unsigned count = maxCount;
	
	*returnedCount = maxCount;
	instance = [EventDriver instance];
	if(instance == nil)
		return IO_R_NOT_ATTACHED;
	rtn = [instance    getIntValues : parameterArray
			   forParameter:parameterName
			   count : &count];
	if(rtn == IO_R_SUCCESS) {
		*returnedCount = count;
	}
	return rtn;

}

IOReturn EvGetParameterChar(
	port_t dev_port,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned char *parameterArray,		// data returned here
	unsigned int *returnedCount)		// size returned here
{
	id instance;
	IOReturn rtn;
	unsigned count = maxCount;
	
	*returnedCount = maxCount;
	instance = [EventDriver instance];
	if(instance == nil)
		return IO_R_NOT_ATTACHED;
	rtn = [instance    getCharValues : parameterArray
			   forParameter:parameterName
			   count : &count];
	if(rtn == IO_R_SUCCESS) {
		*returnedCount = count;
	}
	return rtn;

}

IOReturn EvSetParameterInt(
	port_t dev_port,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int *parameterArray,
	unsigned int count)			// size of parameterArray
{
	id instance;

	instance = [EventDriver instance];
	if(instance == nil)
		return IO_R_NOT_ATTACHED;
	if (   strncmp( parameterName, EV_PREFIX, (sizeof EV_PREFIX) - 1 ) == 0
	    && [instance ev_port] != dev_port )
		return IO_R_PRIVILEGE;
	return [instance    setIntValues : parameterArray
			   forParameter:parameterName
			   count : count];
}


IOReturn EvSetParameterChar(
	port_t dev_port,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned char *parameterArray,
	unsigned int count)			// size of parameterArray
{
	id instance;

	instance = [EventDriver instance];
	if(instance == nil)
		return IO_R_NOT_ATTACHED;
	if (   strncmp( parameterName, EV_PREFIX, (sizeof EV_PREFIX) - 1 ) == 0
	    && [instance ev_port] != dev_port )
		return IO_R_PRIVILEGE;
	return [instance    setCharValues : parameterArray
			   forParameter:parameterName
			   count : count];
}

/*
 * Event services.
 *
 *	These functions are invoked by a privileged port which can be
 *	obtained only by root.  The functions exist primarily to pass
 *	send rights on frame buffer ports out to the WindowServer, and
 *	to control specialized operations of the Event Driver including
 *	setting up the shared memory area.
 */
IOReturn EvOpen(
	port_t	dev_port,
	port_t	event_port )
{
	return [[EventDriver instance]	evOpen:dev_port token:event_port];
}

IOReturn EvClose(
	port_t	dev_port,
	port_t	event_port )
{
	return [[EventDriver instance]	evClose:dev_port token:event_port];
}

IOReturn EvSetSpecialKeyPort(
	port_t	dev_port,
	int	special_key,
	port_t	key_port )
{
	return [[EventDriver instance]	setSpecialKeyPort:dev_port
					keyFlavor: special_key
					keyPort:key_port];
}

/*
 *	FIXME: These routines only work from within the kernel.
 */
#ifdef KERNEL
IOReturn EvMapEventShmem(
	port_t dev_port,	// Only ev_port allowed
	port_t event_port,
	port_t task,		// Task to map shared memory into
	vm_size_t size,		// Size of shared memory area in bytes
	vm_offset_t *addr )	// Returned address of shared memory
{
	return [[EventDriver instance]	mapEventShmem:event_port
					task:task
					size:size
					at:addr];
}

IOReturn EvFrameBufferDevicePort(
	port_t dev_port,
	port_t event_port,
	IOString name,
	IOString class,
	port_t *nameDevicePort )
{
    return [[EventDriver instance]  evFrameBufferDevicePort:event_port
				    unitName:name
				    unitClass:class
				    unitPort:nameDevicePort];
}

/*
 * Additional kernel API to drivers using the Event Driver
 */
 int
EventCoalesceDisplayCmd( int cmd, int oldcmd )
{
	static const char coalesce[4][4] = {
	    /* nop */  {EVNOP,  EVHIDE, EVSHOW, EVMOVE},
	    /* hide */ {EVHIDE, EVHIDE, EVNOP,  EVSHOW},
	    /* show */ {EVSHOW, EVNOP,  EVSHOW, EVSHOW},
	    /* move */ {EVMOVE, EVHIDE, EVSHOW, EVMOVE}
	};
	if ( cmd < EVLEVEL )	// coalesce EVNOP thru EVMOVE only
	    cmd = coalesce[oldcmd & 3][cmd & 3];
	return cmd;
}

#endif /* KERNEL */

