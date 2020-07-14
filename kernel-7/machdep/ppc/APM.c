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

/*
 * Power Management (PM) kernel interface.
 */

#import <kern/power.h>

/*
 * Initialize PM support.  Returns TRUE if PM is available.
 */
PMReturn PMConnect(void)
{
    return PM_R_SUCCESS;
}

/*
 * Return PM control to previous user (probably the BIOS).
 */
PMReturn PMDisconnect(void)
{
    return PM_R_SUCCESS;
}

/*
 * Inform PM of the CPU state.  If the CPU is idle, PM may
 * put the CPU in a power-saving state until the next system event
 * (typically an interrupt).  Marking the CPU busy will ensure
 * that the CPU is running at full speed.
 */
PMReturn PMSetCpuState(PMCpuState state)
{
    return PM_R_SUCCESS;
}

/*
 * Set the power state of a device or the entire computer.
 */
PMReturn PMSetPowerState(PMDeviceID device, PMPowerState state)
{
    return PM_R_BAD_STATE;
}

/*
 * Get a power event from PM, if one has occurred.
 */
PMReturn PMGetPowerEvent(PMPowerEvent *event)
{
    return PM_R_NO_EVENT;
}

/*
 * Get the power state of the computer.
 */
PMReturn PMGetPowerStatus(PMPowerStatus *status)
{
    status->lineStatus = PM_LINE_UNKNOWN;
    status->batteryStatus = PM_BATT_UNKNOWN;
    status->batteryLife = 0;
    return PM_R_SUCCESS;
}

/*
 * Enable or disable power management by the computer.
 * If enable == PM_ENABLED, the computer can take power saving steps
 * when the CPU state is idle.  If enable == PM_DISABLED, no automatic
 * power saving steps will be taken and power management functions
 * will be disabled.
 */
PMReturn PMSetPowerManagement(
    PMDeviceID device,
    PMPowerManagementState state
)
{
    return PM_R_SUCCESS;
}

/*
 * Restores the default settings for all power management settings.
 */
PMReturn PMRestoreDefaults(void)
{
    return PM_R_SUCCESS;
}

/*
 * Update current kernel clock from real-time clock.
 */
void PMUpdateClock(void)
{
}


