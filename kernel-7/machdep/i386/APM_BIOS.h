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

#ifdef KERNEL_PRIVATE

#import <mach/mach_types.h>

#import <machdep/i386/seg.h>
#import <machdep/i386/bios.h>

#import <kern/power.h>

#import <architecture/i386/frame.h>

/* APM 1.0 */
#define APM_BIOS_CODE		0x53
#define APM_BIOS_IDLE		0x05
#define APM_BIOS_BUSY		0x06
#define APM_BIOS_SETSTATE	0x07
#define APM_BIOS_DISABLE	0x08
#define APM_BIOS_DEFAULT	0x09
#define APM_BIOS_GETSTATUS	0x0A
#define APM_BIOS_GETEVENT	0x0B

/* APM 1.1 */
#define APM_BIOS_GETSTATE	0x0C
#define APM_BIOS_ENBL_DEVICE	0x0D
#define APM_BIOS_VERSION	0x0E
#define APM_BIOS_ENGAGE		0x0F

static unsigned long APM_BIOS_addr;

static inline void
initBIOSBuf(
    biosBuf_t *bb,
    unsigned char code
)
{
    union {
	unsigned short	data;
	sel_t		sel;
    } u;

    bb->eax.r.h = APM_BIOS_CODE;
    bb->eax.r.l = code;
    u.sel = APMCODE32_SEL; 
    bb->cs = u.data;
    u.sel = KDS_SEL;
    bb->ds = u.data;
    bb->addr = APM_BIOS_addr;
}

static inline PMReturn
BIOSToPM(
    int biosReturn
)
{
    if (biosReturn == 0x00)
	return PM_R_UNKNOWN;
    else
	return (PMReturn)pm_err(biosReturn);
}

/*
 * Inlines for APM BIOS calls.
 */

static inline PMReturn
APMBIOS_Idle()
{
    biosBuf_t bb;

    initBIOSBuf(&bb, APM_BIOS_IDLE);
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0)
	return PM_R_SUCCESS;
    else
	return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_Busy()
{
    biosBuf_t bb;
    
    initBIOSBuf(&bb, APM_BIOS_BUSY);
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0)
	return PM_R_SUCCESS;
    else
	return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_SetPowerState(
    unsigned short deviceID,
    unsigned short stateID
)
{
    biosBuf_t bb;

    initBIOSBuf(&bb, APM_BIOS_SETSTATE);
    bb.ebx.rx = deviceID;
    bb.ecx.rx = stateID;
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0)
	return PM_R_SUCCESS;
    else
	return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_GetPowerStatus(
    unsigned char *line_status,
    unsigned char *batt_status,
    unsigned char *batt_life
)
{
    biosBuf_t bb;

    initBIOSBuf(&bb, APM_BIOS_GETSTATUS);
    bb.ebx.rx = 0x0001;
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0) {
	*line_status = bb.ebx.r.h;
	*batt_status = bb.ebx.r.l;
	*batt_life = bb.ecx.r.l;
	return PM_R_SUCCESS;
    }
    return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_GetPowerEvent(
    unsigned short *event_code
)
{
    biosBuf_t bb;

    initBIOSBuf(&bb, APM_BIOS_GETEVENT);
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0) {
	*event_code = bb.ebx.rx;
	return PM_R_SUCCESS;
    }
    return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_SetSystemPowerManagement(
    int	enabled
)
{
    biosBuf_t bb;
    
    initBIOSBuf(&bb, APM_BIOS_DISABLE);
    bb.ecx.rx = enabled ? 1 : 0;
    bb.ebx.rx = 0xFFFF;	// entire system
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0)
	return PM_R_SUCCESS;
    return BIOSToPM(bb.eax.r.h);
}

static inline PMReturn
APMBIOS_RestoreDefaults(void)
{
    biosBuf_t bb;
    
    initBIOSBuf(&bb, APM_BIOS_DEFAULT);
    bb.ebx.rx = 0xFFFF;
    bios32(&bb);
    if ((bb.flags & EFL_CF) == 0)
	return PM_R_SUCCESS;
    return BIOSToPM(bb.eax.r.h);
}


#endif /* KERNEL_PRIVATE */

