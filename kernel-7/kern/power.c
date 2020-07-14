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
 * Copyright 1994 NeXT Computer, Inc.
 * All rights reserved.
 */

#import <mach/mach_types.h>
#import <kern/power.h>
#import <kern/thread_call.h>

/*
 * Power Management (PM) callout function.
 */

#define CYCLE_TIME	((tvalspec_t) { 1, 10000000 } )	/* 1.01 seconds */

static PMPowerState systemState;

static PMReturn
get_power_event(PMPowerEvent *ev_p)
{
    PMPowerEvent event;
    PMReturn ret;
    
    if ((ret = PMGetPowerEvent(&event)) == PM_R_SUCCESS) {
	switch (event) {
	case PM_STANDBY_REQUEST:
	case PM_USER_STANDBY_REQUEST:
	    systemState = PM_STANDBY;
	    PMSetPowerState(PM_SYSTEM_DEVICE, PM_STANDBY);
	    break;
	case PM_CRITICAL_SUSPEND:
	case PM_SUSPEND_REQUEST:
	case PM_USER_SUSPEND_REQUEST:
	    systemState = PM_SUSPENDED;
	    PMSetPowerState(PM_SYSTEM_DEVICE, PM_SUSPENDED);
	    systemState = PM_READY;
	    PMSetPowerState(PM_SYSTEM_DEVICE, PM_READY);
	    break;
	case PM_NORMAL_RESUME:
	case PM_CRITICAL_RESUME:
	case PM_STANDBY_RESUME:
	    if (systemState != PM_READY) {
		systemState = PM_READY;
		PMSetPowerState(PM_SYSTEM_DEVICE, PM_READY);
	    }
	    /* fall through to update time */
	case PM_UPDATE_TIME:
	    /* Must read time from RTC */
	    PMUpdateClock();
	    break;
	case PM_BATTERY_LOW:
	case PM_POWER_STATUS_CHANGE:
	    break;
	}
	if (ev_p)
	    *ev_p = event;
    }
    return ret;
}

void
power_callout(
    thread_call_spec_t	argument,
    thread_call_t	callout
)
{
    PMPowerEvent event;
    
    get_power_event(0);
    if (callout == 0)
    	callout = thread_call_allocate(power_callout, 0);
	
    thread_call_enter_delayed(callout, deadline_from_interval(CYCLE_TIME));
}

void
power_init()
{
    kern_return_t	result;
    static boolean_t	initialized = FALSE;
    
    if (initialized == FALSE) {
	if (PMConnect() == PM_R_SUCCESS)
	    (void) power_callout(0, 0);
	
	systemState = PM_READY;
	initialized = TRUE;
    }
}


/*
 * Kernel server side of power management MIG interface.
 */

kern_return_t
kern_PMSetPowerState(
    host_t		host,
    PMDeviceID 		device,
    PMPowerState 	state
)
{
    if (host != &realhost)
    	return KERN_INVALID_HOST;

    return PMSetPowerState(device, state);
}

kern_return_t
kern_PMGetPowerEvent(
    host_t		host,
    PMPowerEvent	*event
)
{
    if (host != &realhost)
    	return KERN_INVALID_HOST;

    return get_power_event(event);
}

kern_return_t
kern_PMGetPowerStatus(
    host_t		host,
    PMPowerStatus	*status
)
{
    if (host != &realhost)
    	return KERN_INVALID_HOST;

    return PMGetPowerStatus(status);
}

kern_return_t
kern_PMSetPowerManagement(
    host_t		host,
    PMDeviceID		device,
    PMPowerManagementState
    			state
)
{
    if (host != &realhost)
    	return KERN_INVALID_HOST;

    return PMSetPowerManagement(device, state);
}

kern_return_t
kern_PMRestoreDefaults(
    host_t		host
)
{
    if (host != &realhost)
    	return KERN_INVALID_HOST;

    return PMRestoreDefaults();
}
