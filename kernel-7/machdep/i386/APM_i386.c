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
 * This implementation uses the Microsoft/Intel APM 1.1 interface.
 */

#import <bsd/sys/time.h>

#import <kern/power.h>

#import <mach/mach_types.h>

#import <machdep/i386/pmap.h>
#import <machdep/i386/seg.h>
#import <machdep/i386/table_inline.h>
#import <machdep/i386/desc_inline.h>
#import <machdep/i386/kernBootStruct.h>
#import <machdep/i386/bios.h>
#import <machdep/i386/APM_BIOS.h>

static boolean_t APM_BIOS_connected;
static struct {
    unsigned major;
    unsigned minor;
} APM_BIOS_version;

#define VERSION_EQ(maj,min) ((APM_BIOS_version.major == (maj)) && \
			  (APM_BIOS_version.minor == (min)))
#define VERSION_GE(maj,min) ((APM_BIOS_version.major > (maj)) || \
			    ((APM_BIOS_version.major == (maj)) && \
			     (APM_BIOS_version.minor >= (min))))
/*
 * Note: we assume that the power management version is 1.0,
 * because some machine report a bogus version.
 * If we do the standard 1.0 "APM Connect" BIOS call in the booter,
 * then the BIOS should give us APM 1.0 behavior.
 */

static void
setUpGdtEntries()
{
    KERNBOOTSTRUCT *kbp = KERNSTRUCT_ADDR;
    
    map_code((data_desc_t *) sel_to_gdt_entry(APMCODE32_SEL),
	     (vm_offset_t) KERNEL_LINEAR_BASE + kbp->apm_config.cs32_base,
	     (vm_size_t) kbp->apm_config.cs_length,
	     KERN_PRIV,
	     FALSE
	    );

    map_code_16((data_desc_t *) sel_to_gdt_entry(APMCODE16_SEL),
	     (vm_offset_t) KERNEL_LINEAR_BASE + kbp->apm_config.cs16_base,
	     (vm_size_t) kbp->apm_config.cs_length,
	     KERN_PRIV,
	     FALSE
	    );

    map_data((data_desc_t *) sel_to_gdt_entry(APMDATA_SEL),
	     (vm_offset_t) KERNEL_LINEAR_BASE + kbp->apm_config.ds_base,
	     (vm_size_t) kbp->apm_config.ds_length,
	     KERN_PRIV,
	     FALSE
	    );
	    
    APM_BIOS_addr = kbp->apm_config.entry_offset;
}

/*
 * Initialize PM support.  Returns TRUE if PM is available.
 */
PMReturn PMConnect(void)
{
    KERNBOOTSTRUCT *kbp = KERNSTRUCT_ADDR;

    APM_BIOS_version.major = kbp->apm_config.major_vers;
    APM_BIOS_version.minor = kbp->apm_config.minor_vers;
    
    if (kbp->apm_config.connected) {
	APM_BIOS_connected = TRUE;
	setUpGdtEntries();
	printf("Power management is enabled.\n");
	return PM_R_SUCCESS;
    }
    return PM_R_NO_PM;
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
PMReturn PMSetCpuState(
    PMCpuState state
)
{
    boolean_t ret;
    
    if (APM_BIOS_connected) {
	switch (state) {
	case PM_CPU_IDLE:
	    ret = APMBIOS_Idle();
	    break;
	case PM_CPU_BUSY:
	    ret = APMBIOS_Busy();
	    break;
	default:
	    ret = FALSE;
	}
	if (ret == FALSE)
	    return PM_R_BAD_STATE;
    } else {
	if (state == PM_CPU_IDLE) {
	    asm volatile("hlt");
	}
    }
    return PM_R_SUCCESS;
}

/*
 * Set the power state of a device or the entire computer.
 */
PMReturn PMSetPowerState(
    PMDeviceID device,
    PMPowerState state
)
{
    union {
	struct {
	    unsigned int 	deviceNumber	:8;
	    unsigned int 	deviceType	:8;
	} device;
	unsigned short data;
    } du;
    union {
	PMPowerState state;
	unsigned short data;
    } su;
    PMReturn ret;
    
    /* Eventually this should differentiate between devices. */
    if (device.deviceType == PM_SYSTEM && device.deviceNumber == 1) {
	    _io_setDriverPowerState(state);
    }

    if (APM_BIOS_connected) {
	if (device.deviceType == PM_SYSTEM && device.deviceNumber == 1) {
	    /*
	     * In PM 1.1 and 1.0,
	     * you can't set the system state to "ready".
	     */
	    if (state == PM_READY)
		return PM_R_BAD_STATE;
	    /*
	     * In PM 1.0, you can't set the system state to "off".
	     */
	    if (state == PM_OFF)
		return PM_R_BAD_STATE;
	}
	du.device.deviceNumber = device.deviceNumber;
	du.device.deviceType = device.deviceType;
	su.state = state;
	return APMBIOS_SetPowerState(du.data, su.data);
    }
    return PM_R_NOT_CONNECTED;
}

/*
 * Get a power event from PM, if one has occurred.
 */
PMReturn PMGetPowerEvent(
    PMPowerEvent *event
)
{
    unsigned short event_code;
    PMReturn ret;

    if (APM_BIOS_connected) {
	if ((ret = APMBIOS_GetPowerEvent(&event_code)) == PM_R_SUCCESS) {
	    *event = (PMPowerEvent)event_code;
	    return PM_R_SUCCESS;
	}
	return ret;
    }
    return PM_R_NOT_CONNECTED;
}

/*
 * Get the power state of the computer.
 */
PMReturn PMGetPowerStatus(
    PMPowerStatus *status
)
{
    unsigned char line_status, batt_status, batt_life;
    PMReturn ret;
    
    if (APM_BIOS_connected) {
	if ((ret = APMBIOS_GetPowerStatus(&line_status, &batt_status, 
					  &batt_life)) == PM_R_SUCCESS) {
	    status->lineStatus = (PMLineStatus)line_status;
	    status->batteryStatus = (PMBatteryStatus)batt_status;
	    status->batteryLife = (batt_life == 0xFF) ? -1 : (int)batt_life;
	    return PM_R_SUCCESS;
	} else {
	    return ret;
	}
    }
    return PM_R_NOT_CONNECTED;
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
    if (APM_BIOS_connected) {
	if (device.deviceType == PM_SYSTEM && device.deviceNumber == 1)
	    return APMBIOS_SetSystemPowerManagement(state);
	else
	    return PM_R_BAD_ID;
    }

    return PM_R_NOT_CONNECTED;
}

/*
 * Restores the default settings for all power management settings.
 */
PMReturn PMRestoreDefaults(void)
{
    if (APM_BIOS_connected)
	return APMBIOS_RestoreDefaults();
    else
	return PM_R_NOT_CONNECTED;
}

/*
 * Update current kernel clock from real-time clock.
 */
void PMUpdateClock(void)
{
/* THIS IS WRONG!!
    extern struct timeval	time;
    readtodc(&time.tv_sec);
*/
}
