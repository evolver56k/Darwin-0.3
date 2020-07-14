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
#import <mach/error.h>

/*
 * Power Management (PM) kernel interface.
 */

#define	pm_err(errno)		(err_kern|err_sub(4000)|errno)

typedef enum {
    PM_R_SUCCESS	= 0x00,
    PM_R_DISABLED	= pm_err(0x01),  // power management disabled
    PM_R_CONNECTED	= pm_err(0x02),  // interface already connected
    PM_R_NOT_CONNECTED 	= pm_err(0x03),	 // interface not connected
    PM_R_BAD_ID		= pm_err(0x09),  // unrecognized device ID
    PM_R_BAD_VALUE	= pm_err(0x0a),  // parameter value out of range
    PM_R_BAD_STATE	= pm_err(0x60),  // can't enter requested state
    PM_R_NO_EVENT	= pm_err(0x80),  // no power events pending
    PM_R_NO_PM		= pm_err(0x86),  // PM not present
    PM_R_UNSUPPORTED	= pm_err(0x100), // unsupported function call
    PM_R_UNKNOWN	= pm_err(0x101)  // unknown error from hardware
} PMReturn;

typedef enum {
    PM_CPU_IDLE,
    PM_CPU_BUSY
} PMCpuState;

typedef enum {
    PM_READY		= 0x00,		// PM enabled, full power
    PM_STANDBY		= 0x01,		// Standby mode
    PM_SUSPENDED	= 0x02,		// Suspend mode
    PM_OFF		= 0x03		// Off completely
} PMPowerState;

typedef enum {
    PM_SYSTEM		= 0x00,
    PM_DISPLAY		= 0x01,
    PM_STORAGE		= 0x02,
    PM_PARALLEL		= 0x03,
    PM_SERIAL		= 0x04,
    PM_NETWORK		= 0x05,
    PM_PCMCIA		= 0x06
} PMDeviceType;

typedef struct {
    unsigned int 	deviceNumber	:16;
    PMDeviceType 	deviceType	:16;
} PMDeviceID;

/*
 * Note: the entire computer is referred to as device type PM_SYSTEM,
 * device number 1.
 */
#define PM_SYSTEM_DEVICE ((PMDeviceID){1, PM_SYSTEM})

typedef enum {
    PM_STANDBY_REQUEST		= 0x01,
    PM_SUSPEND_REQUEST		= 0x02,
    PM_NORMAL_RESUME		= 0x03,
    PM_CRITICAL_RESUME		= 0x04,
    PM_BATTERY_LOW		= 0x05,
    PM_POWER_STATUS_CHANGE	= 0x06,
    PM_UPDATE_TIME		= 0x07,
    PM_CRITICAL_SUSPEND		= 0x08,
    PM_USER_STANDBY_REQUEST	= 0x09,
    PM_USER_SUSPEND_REQUEST	= 0x0a,
    PM_STANDBY_RESUME		= 0x0b
} PMPowerEvent;

typedef enum {
    PM_LINE_OFFLINE	= 0x00,
    PM_LINE_ONLINE	= 0x01,
    PM_LINE_BACKUP	= 0x02,
    PM_LINE_UNKNOWN	= 0xFF
} PMLineStatus;

typedef enum {
    PM_BATT_HIGH	= 0x00,
    PM_BATT_LOW		= 0x01,
    PM_BATT_CRITICAL	= 0x02,
    PM_BATT_CHARGING	= 0x03,
    PM_BATT_UNKNOWN	= 0xFF
} PMBatteryStatus;

typedef struct {
    PMLineStatus	lineStatus;
    PMBatteryStatus	batteryStatus;
    int			batteryLife;
} PMPowerStatus;

typedef enum {
    PM_DISABLED,
    PM_ENABLED
} PMPowerManagementState;


#ifdef	KERNEL

#define MARK_CPU_IDLE(mycpu)	PMSetCpuState(PM_CPU_IDLE)
#define MARK_CPU_ACTIVE(mycpu)	PMSetCpuState(PM_CPU_BUSY)

/*
 * Initialize PM support.  Returns TRUE if PM is available.
 */
PMReturn PMConnect(void);

/*
 * Return PM control to previous user (probably the BIOS).
 * Normally this will never be called.
 */
PMReturn PMDisconnect(void);

/*
 * Inform PM of the CPU state.  If the CPU is idle, PM may
 * put the CPU in a power-saving state until the next system event
 * (typically an interrupt).  Marking the CPU busy will ensure
 * that the CPU is running at full speed.
 */
PMReturn PMSetCpuState(PMCpuState state);

/*
 * Update current kernel clock from real-time clock.
 */
void PMUpdateClock(void);

/*
 * Set the power state of a device or the entire computer.
 */
PMReturn PMSetPowerState(PMDeviceID device, PMPowerState state);

/*
 * Get a power event from PM, if one has occurred.
 */
PMReturn PMGetPowerEvent(PMPowerEvent *event);

/*
 * Get the power state of the computer.
 */
PMReturn PMGetPowerStatus(PMPowerStatus *status);

/*
 * Enable or disable power management by the computer.
 * If enable == PM_ENABLED, the computer can take power saving steps
 * when the CPU state is idle.  If enable == PM_DISABLED, no automatic
 * power saving steps will be taken and power management functions
 * will be disabled.
 */
PMReturn PMSetPowerManagement(PMDeviceID device,
				PMPowerManagementState state);

/*
 * Restores the default settings for all power management settings.
 */
PMReturn PMRestoreDefaults(void);

#else	/* KERNEL */

/*
 * Set the power state of a device or the entire computer.
 */
kern_return_t _PMSetPowerState(
    host_t 		host,
    PMDeviceID 		device,
    PMPowerState 	state);

/*
 * Get a power event from PM, if one has occurred.
 */
kern_return_t _PMGetPowerEvent(
    host_t		host,
    PMPowerEvent 	*event);

/*
 * Get the power state of the computer.
 */
kern_return_t _PMGetPowerStatus(
    host_t		host,
    PMPowerStatus 	*status);

/*
 * Enable or disable power management by the computer.
 * If enable == PM_ENABLED, the computer can take power saving steps
 * when the CPU state is idle.  If enable == PM_DISABLED, no automatic
 * power saving steps will be taken and power management functions
 * will be disabled.
 */
kern_return_t _PMSetPowerManagement(
    host_t		host,
    PMDeviceID 		device,
    PMPowerManagementState state);

/*
 * Restores the default settings for all power management settings.
 */
kern_return_t _PMRestoreDefaults(
    host_t		host);


#endif	/* KERNEL */
