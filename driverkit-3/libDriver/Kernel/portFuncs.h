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
 * portFuncs.h
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver deamon main thread.
 *
 * HISTORY
 *      06/26/92/mtm    Original coding.
 */
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>

#import <kernserv/prototypes.h>		// task_self
#import <driverkit/generalFuncs.h>

/*
 * Allocate a port.
 */
static inline port_t allocatePort(void)
{
    kern_return_t krtn;
    port_t aPort;
    
    krtn = port_allocate(task_self(), &aPort);
    if (krtn) {
	IOLog("Audio: port_allocate");
	//IOPanic("allocatePort");
    }
    return aPort;
}

/*
 * Deallocate a port.
 */
static inline void deallocatePort(port_t aPort)
{
    kern_return_t krtn;

    krtn = port_deallocate(task_self(), aPort);
    if (krtn) {
	IOLog("Audio: port_deallocate\n" );
	//IOPanic("deallocatePort");
    }
}

/*
 * Add a port to a port set.
 */
static inline void addPort(port_t aPortSet, port_t aPort)
{
    kern_return_t krtn;

    krtn = port_set_add(task_self(), aPortSet, aPort);
    if (krtn) {
	IOLog("Audio: port_set_add\n" );
	//IOPanic("addPort");
    }
}

/*
 * Allocate a port set.
 */
static inline port_t allocatePortSet(void)
{
    kern_return_t krtn;
    port_set_name_t aPortSet;

    krtn = port_set_allocate(task_self(), &aPortSet);
    if (krtn) {
	IOLog("Audio: port_set_allocate: %d\n", krtn);
	//IOPanic("allocatePortSet");
    }
    return aPortSet;
}
