/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdeCntInit.m - ATA controller initialization module. 
 *
 * 1-Feb-1998	Joe Liu at Apple
 *	Added "new style" device overrides, and matching inspector.
 *  Report the transfer mode set for both drives.
 *	Set PIO transfer mode for ATAPI drives as well.
 *	getTransferModeFromCycleTime method now deals with Multiword DMA cases.
 *
 * 04-Mar-1997	 Scott Vail at NeXT
 *	Fixes made for modifications in ideDriveInfo_t struct.
 *
 * 04-Sep-1996	 Becky Divinski at NeXT
 *	Added code to ideControllerInit method handle Dual EIDE personality case.
 *
 * 3-Sept-1996	Becky Divinski at NeXT
 * 		Changed name of controllerPrersent method to 
 *		controllerPresent.
 *
 * 30-Jan-1996 	Rakesh Dubey at NeXT
 *      Added forced device detection to fix PIONEER bug. 
 *
 * 13-Jul-1995 	Rakesh Dubey at NeXT
 *      Improved device detection. 
 *
 * 11-Aug-1994 	Rakesh Dubey at NeXT
 *      Created. 
 */

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdePIIX.h"
#import "AtapiCntCmds.h"
#import "IdeShared.h"
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#if (IO_DRIVERKIT_VERSION != 330)
#import <machdep/machine/pmap.h>        // XXX get rid of this
#endif
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "IdeDDM.h"
#import <machdep/i386/io_inline.h>
#import "DualEide.h"
#import <driverkit/return.h>
#import <mach/port.h>
#import <stdlib.h>


//#define DEBUG

/*
 * The controller at base address 0x1f0-0x1f7 is always the first controller
 * no matter when it gets probed, so we start the counter at 1. 
 */
static unsigned char controllerNum = 1;

/*
 * There is no standard for channels 3 and 4 (channel 2 is almost there). We
 * should stick to the following base address/IRQ pairings: 1F0/14, 170/15,
 * 1E8/11, 168/10. 
 */
static __inline__
unsigned int
assignRegisterAddresses(ideRegsAddrs_t *ideRegsAddrs,
            unsigned int baseAddress1)
{
    unsigned int baseAddress2;
    unsigned char cntNum;
    
    switch (baseAddress1) {
      case 0x1f0:
	cntNum = 0;			/* primary, always controller 0 */
	break;
      default:
	cntNum = controllerNum++;
	break;
    }
    	
    baseAddress2 = baseAddress1 +  0x206;
    
    ideRegsAddrs->data = baseAddress1;
    ideRegsAddrs->error = baseAddress1 + 1;
    ideRegsAddrs->features = baseAddress1 + 1;
    ideRegsAddrs->sectCnt = baseAddress1 + 2;
    ideRegsAddrs->sectNum = baseAddress1 + 3;
    ideRegsAddrs->cylLow = baseAddress1 + 4;
    ideRegsAddrs->cylHigh = baseAddress1 + 5;
    ideRegsAddrs->drHead = baseAddress1 + 6;
    ideRegsAddrs->status = baseAddress1 + 7;
    ideRegsAddrs->command = baseAddress1 + 7;
    
    ideRegsAddrs->deviceControl = baseAddress2;
    ideRegsAddrs->altStatus = baseAddress2;

    return cntNum;
}

/*
 * Function: ata_mode_to_num
 *
 * Convert ata_mode_t into a value. i.e.
 *	0x01 ==> 0
 *	0x02 ==> 1
 */	 
extern unsigned char
ata_mode_to_num(ata_mode_t mode)
{
	unsigned char tmp = 0;
	if (mode == ATA_MODE_NONE) {
//		IOLog("bad argument to ata_mode_to_num()\n");
		return (0);
	}
	while (mode != 0x01) {
		mode >>= 1;
		tmp++;
	}
	return (tmp);
}

/*
 * Function: ata_mask_to_mode
 *
 * Return the most significant bit in a ata_mask_t mask. i.e.
 *	0x03 ==> 0x02
 *	0x07 ==> 0x04
 */
extern ata_mode_t
ata_mask_to_mode(ata_mask_t mask)
{
	int i;
	if (mask == 0)
		return ATA_MODE_NONE;
	for (i = 7; i >= 0; i--) {
		if ((1 << i) & mask)
			return (1 << i);
	}
	return ATA_MODE_NONE;
}

/*
 * Function: ata_mode_to_mask
 *
 * Convert from a mode to a mask such that all less significant bits,
 * including the mode bit is set.
 */
extern ata_mask_t
ata_mode_to_mask(ata_mode_t mode)
{
	return ((mode == ATA_MODE_NONE) ? mode : (mode | (mode - 1)));
}

@implementation IdeController(Initialize)


#define ATAPI_SIGNATURE_LOW		0x14
#define ATAPI_SIGNATURE_HIGH		0xeb

#define ATA_SIGNATURE_LOW		0x00
#define ATA_SIGNATURE_HIGH		0x00

/*
 * This is called just after resetting the drives connected to this
 * controller. We read the drive registers to figure out if it is an ATAPI
 * drive. If it is then we make a note of it and return YES. 
 */
- (BOOL)controllerPresent
{
    unsigned char dh;
    unsigned char unit;
    
    unit = 0;
    
    dh = _drives[unit].addressMode;
    
    dh |= (unit ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);

    /*
     * Test with some data. 
     */
    
    if (unit == 0)	{
	outb(_ideRegsAddrs.cylLow, 0xaa);
	outb(_ideRegsAddrs.cylHigh, 0xbb);
    } else {
	outb(_ideRegsAddrs.cylLow, 0xba);
	outb(_ideRegsAddrs.cylHigh, 0xbe);
    }
    
    if (unit == 0) {
	if ((inb(_ideRegsAddrs.cylLow) != 0xaa) ||
	    (inb(_ideRegsAddrs.cylHigh) != 0xbb)) {
	    return NO;
	}
    } else {
	if ((inb(_ideRegsAddrs.cylLow) != 0xba) ||
	    (inb(_ideRegsAddrs.cylHigh) != 0xbe)) {
	    return NO;
	}
    }
    
    return YES;
}

/*
 * One-time only init. Returns NO on failure. 
 */
- (BOOL)ideControllerInit:(IODeviceDescription *)deviceDescription
{
    int     	unit;
    int     	drivesPresent = 0;
    const char  *params;
    const char  *addressingMode, *multisectorMode;
    IOEISADeviceDescription *eisaDeviceDescription =
		(IOEISADeviceDescription *)deviceDescription;
    IOConfigTable *configTable = [deviceDescription configTable];
    int		override[MAX_IDE_DRIVES];

#if 0
    _printWaitForNotBusy = NO;
#endif 0

    if (dualEide) {
    	if (instanceNum)	
            [super setDeviceDescription:deviceDescription];
    } 

    if ([self enableAllInterrupts]) {
		IOLog("ideControllerInit: failed enableAllInterrupts\n");
		return NO;
    }
    
    if ((_ideCmdLock = [NXLock new]) == NULL) {
		IOLog("ideControllerInit: failed _ideCmdLock\n");
		return NO;
    }
    
    /*
     * We don't want to execute any IDE commands from Disk until
     * initialization is complete. 
     */
    [self ideCntrlrLock];
    
    /*
     * Initialize the register address values.
     */
    _controllerNum = assignRegisterAddresses(&_ideRegsAddrs,
                            [eisaDeviceDescription portRangeList][0].start);

    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
		_drives[unit].atapiDevice = NO;
    }
    
    /*
     * We have to test here if the controller is present or else we will
     * block looking for a drive (in ideReset). 
     */
    if ([self controllerPresent] == NO)	{
		IOLog("%s: no devices detected at port 0x%0x\n", [self name],
			_ideRegsAddrs.data);
		[self ideCntrlrUnLock];
		return NO;
    } else {
		IOLog("%s: device detected at port 0x%x irq %d\n", [self name], 
			_ideRegsAddrs.data, [deviceDescription interrupt]);
    }
    
    _ideInterruptPort = [self interruptPort];
    _ideDevicePort = [deviceDescription devicePort];

    [self setInterruptTimeOut:IDE_INTR_TIMEOUT];

	_multiSectorRequested = NO;
    multisectorMode = [configTable valueForStringKey:MULTIPLE_SECTORS_ENABLE];
	if (multisectorMode) {
		if ((*multisectorMode == 'Y') || (*multisectorMode == 'y'))
			_multiSectorRequested = YES;
		[configTable freeString:multisectorMode];
	}

    /*
     * Select mode for addressing drives. The driver will fall back to CHS
     * mode if LBA is unsupported by the drive.
     */
    addressingMode = [[deviceDescription configTable] 
    				valueForStringKey: ADDRESS_MODE];
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
        if ((addressingMode == NULL) || (strcmp(addressingMode, "LBA") != 0)){
			_drives[unit].addressMode = ADDRESS_MODE_CHS;
		} else {
			_drives[unit].addressMode = ADDRESS_MODE_LBA;
		}
	}
	if (addressingMode) [configTable freeString:addressingMode];

#ifdef DEBUG    
    if (_addressMode[0] == ADDRESS_MODE_LBA)	{
	IOLog("%s: Using Logical address mode..\n", [self name]);
    }
#endif DEBUG
    
    /*
     * Find out from config table if we are not using BIOS parameters.
     * Default is to use stored values in CMOS/BIOS.
     */
    params = [[deviceDescription configTable] 
    			valueForStringKey: DRIVE_PARAMETERS];
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
	if ((params == NULL) || ((*params != 'Y') && (*params != 'y')))
	    _drives[unit].biosGeometry = YES;
	else
	    _drives[unit].biosGeometry = NO;
    }
	if (params) [configTable freeString:params];

    /*
     * Same as DRIVE_PARAMETERS. See comment above.
     */
    params = [[deviceDescription configTable] 
    			valueForStringKey: USE_DISK_GEOMETRY];
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
		if (params && ((*params == 'Y') || (*params == 'y')))
	    	_drives[unit].biosGeometry = NO;
    }
	if (params) [configTable freeString:params];

    /*
     * Are we supporting ATAPI? 
     */
	_EIDESupport = YES;
    params = [[deviceDescription configTable] 
    			valueForStringKey: EIDE_SUPPORT];
	if (params) {
		if ((*params == 'N') || (*params == 'n'))
			_EIDESupport = NO;			
		[configTable freeString:params];
	}

	/*
	 * Obtain the transfer type/mode mask. A user may have disabled
	 * certain type of transfer modes.
	 */
	for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
		char *key;
		_drives[unit].driveMasks.modes = 0x000000ff;	// enable all PIO modes
		
		if (dualEide && instanceNum) {
			if (unit)
				key = MODES_MASK_SLAVE_SEC;
			else
				key = MODES_MASK_MASTER_SEC;
		}
		else {
			if (unit)
				key = MODES_MASK_SLAVE;
			else
				key = MODES_MASK_MASTER;
		}
		
    	params = [[deviceDescription configTable] 
    			valueForStringKey:key];		
		if (params) {
			// Read the mask set by the user, however we assume that
			// PIO modes 0 is always available.
			unsigned int mask = strtoul(params, NULL, 16);
			_drives[unit].driveMasks.modes = mask | 
				ata_mode_to_mask(ATA_MODE_0);
			[configTable freeString:params];
		}
	}

#if 0
    /*
     * Does the host support IOCHRDY? There is no way for the driver to find
     * this out. The user has to set this flag.
     */
    params = [[deviceDescription configTable] 
    			valueForStringKey: HOST_IORDY_SUPPORT];

    if ((params == NULL) || (strcmp(params, "Yes") != 0))
	_IOCHRDYSupport = NO;
    else
	_IOCHRDYSupport = YES;  
#endif /* 0 */

    _transferWidth = IDE_TRANSFER_16_BIT;

    /*
     * We acquire the disk geometry from either the BIOS or the disk itself.
     * If we are using bios we get the values from it. If we are not using
     * BIOS then we just determine presence of disk here. The actual geometry
     * will be read later. However if the user has chosen "disk geometry"
     * option and disk 0 is not detected then we fall back to BIOS for
     * geometry. This is intended to save the user from getting hosed in case
     * the disk does not support Identify drive command. 
     */

    /*
     * FIXME: This recovery strategy needs to thought over. 
     */
     
    _driveNum = unit;

    for (unit = 0; unit < MAX_IDE_DRIVES; unit++)
		override[unit] = DEVICE_AUTO;

    // Old-style overrides remain here for compatibility
    {
	const char *tmp;
	tmp = [configTable valueForStringKey:ATA_LOCATION];
        if (tmp) {
            if (strcmp(tmp, "Master")==0)
		override[0]=DEVICE_ATA;
            else if (strcmp(tmp, "Slave")==0)
		override[1]=DEVICE_ATA;
	    [configTable freeString:tmp];
        }
	tmp = [configTable valueForStringKey:ATAPI_LOCATION];
        if (tmp) {
            if (strcmp(tmp, "Master")==0)
		override[0]=DEVICE_ATAPI;
            else if (strcmp(tmp, "Slave")==0)
		override[1]=DEVICE_ATAPI;
	    [configTable freeString:tmp];
        }
    }
	
    // New-style overrides take precidence over Old-style overrides
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
        const char *tmp;
		
		if (dualEide && instanceNum)
			tmp = [configTable valueForStringKey:
				(unit ? IDE_SLAVE_KEY_SEC : IDE_MASTER_KEY_SEC)];
		else
        	tmp = [configTable valueForStringKey:
				(unit ? IDE_SLAVE_KEY : IDE_MASTER_KEY)];
		
		if (tmp == NULL)
			continue;

		if ( (strcmp(tmp, "")==0) ||
			(strcmp(tmp, overrideTable[DEVICE_AUTO])==0) )
			override[unit] = DEVICE_AUTO;	
		else if (strcmp(tmp, overrideTable[DEVICE_NONE])==0)
			override[unit] = DEVICE_NONE;
		else if ( (strcmp(tmp, overrideTable[DEVICE_ATA])==0) ||
			(strcmp(tmp, "IDE")==0) || (strcmp(tmp, "EIDE")==0) )
			override[unit] = DEVICE_ATA;
		else if ( (strcmp(tmp, overrideTable[DEVICE_ATAPI])==0) ||
			(strcmp(tmp, "CD")==0) || (strcmp(tmp, "CDROM")==0) )
			override[unit] = DEVICE_ATAPI;
		[configTable freeString:tmp];
    }

    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
        switch (override[unit]) {
            case DEVICE_NONE:
				IOLog("%s: Drive %d scan will be skipped due to override.\n",
					[self name], unit);
				break;
            case DEVICE_ATA:
				IOLog("%s: Drive %d forced to ATA by override.\n",
					[self name], unit);
				break;
            case DEVICE_ATAPI:
				IOLog("%s: Drive %d forced to ATAPI by override.\n",
					[self name], unit);
				break;
            default :
				break;
	}
	bzero(&(_drives[unit].ideInfo), sizeof(ideDriveInfo_t));
	
	[self ideReset];
	
	/*
	 * We need to use the disk geometry (and override user selection) if
	 * the BIOS reports more than 16 heads. 
	 */
	if (_drives[unit].biosGeometry == YES)	{
	    _drives[unit].ideInfo = [self getIdeInfoFromBIOS:unit];
	    if ((_drives[unit].ideInfo.type != 0) &&
			(_drives[unit].ideInfo.heads > 16)) {
			_drives[unit].biosGeometry = NO;
			_drives[unit].ideInfo.type = 0;
			IOLog("%s: WARNING: using disk geometry for drive %d.\n", 
			    [self name], unit);
	    }
	}

	/*
	 * If IDE_IDENTIFY_DRIVE command fails then we will override user
	 * selection and use BIOS geometry if this is the boot disk (first
	 * disk on first controller). 
	 */
	if ((override[unit] == DEVICE_AUTO) || (override[unit] == DEVICE_ATA)) {
	    if (_drives[unit].biosGeometry == NO)	{
	        if ([self ideDetectDrive:unit override:override[unit]] == YES)	{
		    _drives[unit].ideInfo.type = 255;
	        } else if ((_controllerNum == 0) && (unit == 0))	{
		    _drives[unit].ideInfo = [self getIdeInfoFromBIOS:unit];
		    if (_drives[unit].ideInfo.type != 0)	{
		        _drives[unit].biosGeometry = YES;
		        IOLog("%s: WARNING: using BIOS geometry for drive %d.\n", 
				    [self name], unit);
		    }
	        }
	    }
	}

	/*
	 * If we didn't find ATA drive then look for ATAPI device. 
	 */
	if ((override[unit] == DEVICE_AUTO) || (override[unit] == DEVICE_ATAPI)) {
	    if ((_drives[unit].ideInfo.type == 0) && (_EIDESupport == YES) &&
	        ([self ideDetectATAPIDevice:unit override:override[unit]] == YES)){
	        _drives[unit].ideInfo.type = 127;
	    }
	}

	if (_drives[unit].ideInfo.type != 0)
	    drivesPresent += 1 ;
    }
    
    if (drivesPresent == 0)	{
		[self ideCntrlrUnLock];
		IOLog("ideControllerInit: failed drivesPresent\n");
		return NO;
    }

    /*
     * Initialization was successful. We should release the lock so that the
     * controller can now execute commands from disk objects. 
     */

    [self resetAndInit];

#ifdef DDM_DEBUG
    IOInitDDM(IDE_NUM_DDM_BUFS);
#endif DDM_DEBUG
    
    /*
     * Set drive power state to active.
     */
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
		if (_drives[unit].ideInfo.type != 0)
	    	[self setDrivePowerState:IDE_PM_ACTIVE];
    }
    
    _driveSleepRequest = NO;
    [self enableInterrupts];
    [self ideCntrlrUnLock];
    return YES;
}

/*
 * Method: getControllerCapability
 *
 * Query the controller's transfer mode capability and record them
 * to _controllerModes.
 *
 * Note that _controllerModes fields contain the mask of all modes that it
 * supports. Not just the highest possible mode.
 */
- (void) getControllerCapability
{
	/*
	 * Set default values.
	 */
	_controllerModes.mode.pio   = ATA_MODE_0;
	_controllerModes.mode.swdma = ATA_MODE_NONE;
	_controllerModes.mode.mwdma = ATA_MODE_NONE;
	_controllerModes.mode.udma  = ATA_MODE_NONE;
	
	if (_controllerID != PCI_ID_NONE)
		[self getPCIControllerCapabilities:(txferModes_t *)&_controllerModes];
	else {
		// Assume all IDE controllers are capable of PIO Mode 4
		//
		_controllerModes.mode.pio   = ata_mode_to_mask(ATA_MODE_4);
	}
}

#define ATA_RESET_DELAY		1500		/* milliseconds */

/*
 * Determine the presence and type of drive (IDE/ATAPI) attached to this
 * controller. We use this only if we are not using the info from CMOS/BIOS. 
 *
 * Even though this is an optional command it is safe to use since all ATA 
 * disks support it. Vey old disks may not support it and they can be used 
 * by the "Use BIOS Geometry" option. 
 *
 * Note Well: Some drives bail out immediately and return with ERR bit set in
 * the status register. Some bail out after a minute and set ERR bit. However
 * some simply return zero in the status register. I am sure there are other
 * variants. Hence we check for DRQ (success) bit. Note that we would not get
 * an interrupt since -registerDevice has not been called yet. This routine
 * probably needs some more thinking. It is very important that this routine
 * never detect ATAPI drives as ATA. 
 */

/*
 * In Micron machine, if there is one ATAPI CD-ROM, it returns 78 or 7a in the
 * status register. Completely bogus. 
 */
-(BOOL)ideDetectDrive:(unsigned int)unit override:(int)override
{
    unsigned char dh = _drives[unit].addressMode;
    int i;
    unsigned char status;
    BOOL found = NO;
    
    if ((override != DEVICE_AUTO) && (override != DEVICE_ATA))
	return NO;

    IOLog("%s: Checking for ATA drive %d...  ", [self name], unit);
#ifdef DEBUG
    IOLog("\n");
#endif DEBUG
    
    [self disableInterrupts];
    
    dh |= (unit ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    
    outb(_ideRegsAddrs.command, IDE_IDENTIFY_DRIVE);

    for (i = 0; i < (ATA_RESET_DELAY * 100); i++) {
		IODelay(10);
		status = inb(_ideRegsAddrs.status);
		
		if (status & BUSY)
			continue;
		
		if (status & ERROR) {
			break;				/* certain failure */
		}
		if (unit && (status & WRITE_FAULT)) {
			break;				/* failure as well */
		}
		if (status & DREQUEST) {
			found = YES;		/* success */
			break;
		}    
    }
#ifdef DEBUG
    IOLog("%s: ideDetectDrive: status %x\n", [self name], status);
#endif DEBUG
    
    if (found)	{
        IOLog("Detected\n");
    	return YES;
    }
    
    /*
     * If we got here this means that the ATA device is not present or was
     * not detected. Check if the user has told us that the drive is present. 
     */
    
    if (override == DEVICE_ATA) {
	IOLog("Not detected but proceeding\n");
	return YES;
    }

    IOLog("\n");
    return NO;
}

/*
 * We send the device ATAPI soft reset command and check for signature. This
 * is the first command that ATAPI devices need. 
 */
-(BOOL)ideDetectATAPIDevice:(unsigned int)unit override:(int)override
{
    unsigned char low, high;
//  const char  *location;
    BOOL found = NO;
    
    if ((override != DEVICE_AUTO) && (override != DEVICE_ATAPI))
	return NO;

    IOLog("%s: Checking for ATAPI drive %d... ", [self name], unit);
#ifdef DEBUG
    IOLog("\n");
#endif DEBUG
	
    [self atapiSoftReset:unit];
	
    low = inb(_ideRegsAddrs.cylLow);
    high = inb(_ideRegsAddrs.cylHigh);
    
    if ((low == ATAPI_SIGNATURE_LOW) && (high == ATAPI_SIGNATURE_HIGH)) {
		_drives[unit].atapiDevice = YES;
#ifdef undef
	IOLog("%s: ATAPI drive %d detected.\n", [self name], unit);
#endif undef
	found = YES;
    } else if (_ide_debug) {
	IOLog("%s: No ATAPI drive %d, low = %x high = %x\n", 
		[self name], unit, low, high);
    }
    
    if (found)	{
        IOLog("Detected\n");
    	return YES;
    }
    
    /*
     * If we got here this means that the ATAPI device is not present or was
     * not detected. Check if the user has told us that the device is
     * present. This will probably fail later but succeeds in case of some
     * drives (like PIONEER DR-124X 1.02). It is definitely lame to work
     * around firmware bugs in the driver but there seems to be no
     * alternative.. 
     */
    if (override == DEVICE_ATAPI) {
	_drives[unit].atapiDevice = YES;
	IOLog("Not detected but proceeding\n");
	return YES;
    }
    
    IOLog("\n");
    return NO;
}


-(ideDriveInfo_t)getIdeDriveInfo:(unsigned int)unit
{
    return _drives[unit].ideInfo;
}

#define MAX_RESET_ATTEMPTS 		2

- (void)ideReset
{
    int count;
    int delay;
    unsigned char status;

#ifdef DEBUG_TRACE
    IOLog("%s: ideReset\n", [self name]);
#endif DEBUG_TRACE

    for (count = 0; count < MAX_RESET_ATTEMPTS; count++) {
    
	outb(_ideRegsAddrs.deviceControl, DISK_RESET_ENABLE);
	IODelay(100);	   		/* spec >= 25 us */
	outb(_ideRegsAddrs.deviceControl, 0x0);
	
	[self enableInterrupts];

    	delay = 31 * 1000 * 1000;	/* thirty one seconds */
	
	IOSleep(1000);			/* Enough time to assert busy */
	
	while (delay > 0) {
	
	    status = inb(_ideRegsAddrs.status);
	    if (!(status & BUSY)) {
		return;
	    }
	    
	    IOSleep(1);
	    delay -= 1000;
	}
	
	IOLog("%s: Reset failed, retrying..\n", [self name]);
	IOSleep(2000);
    }
    
    /* FIXME: I don't think we should panic */
    if (count == MAX_RESET_ATTEMPTS) {
	IOLog("%s: can not reset.\n", [self name]);
	IOPanic("IDE Reset");
	/* NOT REACHED */
    }
}

/*
 * Initialize the IDE interface. Find about drive capabilities (by
 * IDE_IDENTIFY_DRIVE command) and configure drive. We either use CMOS/BIOS
 * settings to program the drive or bypass CMOS/BIOS and use the values
 * returned by the drive depending upon biosGeometry flag. By the time we
 * enter this routine we know about all devices connected to this controller. 
 */
- (void)resetAndInit
{
    int i;
    unsigned char unit;
    ide_return_t rtn;
    BOOL ataDevicePresent;
	BOOL retry;
	
	/*
	 * Determine what transfer modes the controller is capable of.
	 */
	[self getControllerCapability];

	/* Qualify the default mask for each drive with the
	 * controller's mask.
	 */
	for (i = 0; i < MAX_IDE_DRIVES; i++) {
		_drives[i].driveMasks.modes &= _controllerModes.modes;
	}
	
	/*
	 * Loops through one cycle of the configuration process:
	 *
	 * 0. Get controller capability.
	 * 1. Revert controller back to compatibility timing.
	 * 2. Reset ATA/ATAPI.
	 * 3. Determine drive(s) capabilities.
	 * 4. Set drive(s) feature according to controller & drive capability.
	 * 5. Set controller timing and mode. (UDMA, PIO, etc...)
	 * 6. If DMA or Multisector mode, perform test.
	 * 7. If test failed, mask out the current mode, goto step 1.
	 */

	do {
	
	/*
	 * Reset the IDE controller to stop any transfer in progress, and
	 * revert back to the slowest timing settings.
	 */
	[self resetController];
	
	retry = NO;
    IOLog("%s: Resetting drives...\n", [self name]);

    /*
     * If there is at least one ATA drive connected to this controller then
     * is is necessary to (ATA style) reset them. Otherwise, We use the ATAPI 
     * reset instead (in setATAPIDriveCapabilities: method below).
     */
    ataDevicePresent = NO;
    for (i = 0; i < MAX_IDE_DRIVES; i++) {
		if ((_drives[i].ideInfo.type != 0) && ([self isAtapiDevice:i] == NO)){
			ataDevicePresent = YES;
			break;
		}
    }
    if (ataDevicePresent == YES) {
#ifdef DEBUG
        IOLog("%s: ATA device present on this controller\n", [self name]);
#endif DEBUG
		[self ideReset];
    }

    /*
     * Send the controller necessary commands to set capabilities and then
     * set drive parameters. It is necessary to set drive parameters before
     * any data I/O can be done from the disk. 
     */
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {

		if (_drives[unit].ideInfo.type == 0)	{
			//IOLog("%s: Drive %d not present.\n", [self name], unit);
			continue;
		}

		/* Set default values.
		 */
		_drives[unit].multiSector  = 0;
		_drives[unit].transferType = IDE_TRANSFER_PIO;
		_drives[unit].transferMode = ATA_MODE_0;

		/*
		 * ATAPI devices.
		 */
		if ([self isAtapiDevice:unit] == YES) {
			rtn = [self setATAPIDriveCapabilities:unit];
			if (rtn != IDER_SUCCESS) {
				IOLog("%s: ATAPI drive %d is not present.\n",
					[self name], unit);
				_drives[unit].ideInfo.type = 0;
				_drives[unit].atapiDevice = NO;
			}
			continue;
		}
		
		/*
		 * ATA devices.
		 */
		if (_drives[unit].biosGeometry == YES)	{
			rtn = [self setATADriveCapabilities:unit withBIOSInfo:YES];
		} else {
			rtn = [self setATADriveCapabilities:unit withBIOSInfo:NO];
			if (rtn != IDER_SUCCESS) {
				IOLog("%s: ATA drive %d is not present.\n", [self name], unit);
				_drives[unit].ideInfo.type = 0;
				continue;
			}
		}
    }

    /*
     * Set IDE controller capabilities. This must be done after the disks are
     * configured. 
     */
	[self setControllerCapabilities];

	/*
	 * Report the transfer mode set for both drives.
	 */
	if (_controllerID != PCI_ID_NONE) {
		const char *mode_string[] =
			{"PIO", "Single-word DMA", "Multiword DMA", "Ultra DMA"};
		for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
			if (_drives[unit].ideInfo.type == 0)
				continue;
			if (_drives[unit].multiSector)
				IOLog("%s: Drive %d: %s Mode %d (%d sectors)\n",
					[self name], unit, mode_string[_drives[unit].transferType], 
					ata_mode_to_num(_drives[unit].transferMode),
					_drives[unit].multiSector);
			else				
				IOLog("%s: Drive %d: %s Mode %d\n",
					[self name], unit, mode_string[_drives[unit].transferType], 
					ata_mode_to_num(_drives[unit].transferMode));
		}
	}
	
	/*
	 * After the drives and the controller are setup, we perform a read
	 * and make sure everything is OK. We do not perform this test for
	 * ATAPI or removable ATA devices (if they become supported).
	 *
	 * FIXME: perhaps instead of reading a block from the disk, we should
	 * consider using DMA on non-media commands. This will be needed when
	 * DMA is supported on removable media drives. Do those commands exist?
	 * It seems that ATA-3 defined a IDENTIFY DEVICE DMA command, but they
	 * no longer exist in ATA-4. Did they get dropped?
	 */
	for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {

		_driveNum = unit;
		
		// Don't test ATAPI devices.
		//
		if ([self isAtapiDevice:unit])
			continue;
		
		// Test DMA reads.
		//
		if ((_drives[unit].ideInfo.type != 0) &&	
			(_drives[unit].transferType != IDE_TRANSFER_PIO) &&
			([self performDMATest] != IDER_SUCCESS)) {
			IOLog("%s: Drive %d: DMA read test FAILED\n", [self name], unit);
			/* Failure, mask out the current mode */
			_drives[unit].driveMasks.array.mode[_drives[unit].transferType] &= 
				~_drives[unit].transferMode;
			retry = YES;
		}

		// Test PIO multisector reads.
		//
		// FIXME - Test Fast PIO modes as well?
		//
		if ((_drives[unit].ideInfo.type != 0) &&
			(_drives[unit].transferType == IDE_TRANSFER_PIO) &&
			(_drives[unit].multiSector)) {

			ideRegsVal_t ideRegs;
			char *buf;
			
			ideRegs = [self logToPhys:0 numOfBlocks:_drives[unit].multiSector];
			buf = IOMalloc(_drives[unit].multiSector * IDE_SECTOR_SIZE);
			if ([self ideReadMultiple:&ideRegs 
	    		client:(struct vm_map *)IOVmTaskSelf()
	    		addr:buf] != IDER_SUCCESS) {
	    		IOLog("%s: Drive %d: Read Multiple test FAILED\n",
					[self name], unit);
				_multiSectorRequested = NO;
				retry = YES;
	    	}
	    	IOFree(buf, _drives[unit].multiSector * IDE_SECTOR_SIZE);
		}
	}

	} while (retry);
}

/*
 * Method: resetController
 *
 * Reset the controller on the HOST side.
 */
- (void)resetController
{
	/*
	 * Return the controller to the compatible timing mode.
	 */
	[self resetPCIController];
}

/*
 * Method: getBestTransferMode
 *
 * Find the "best" mode given the txferModes_t structure for both the drive
 * and the controller. Recall that the txferModes_t type encodes all the
 * transfer modes that a device can attain. Certain modes may also be masked
 * so that they will never be used. This mask is stored in the driveMasks.
 */
-(void)getBestTransferMode:(ata_mode_t *)mode type:(ideTransferType_t *)type 
	forUnit:(unsigned int)unit
{
	txferModes_t m;		// store all the possible modes.

	m.modes = _drives[unit].driveModes.modes & _drives[unit].driveMasks.modes &
		_controllerModes.modes;

	/*
	 * Test in the order of "best" to "worst" modes.
	 */	
	if (m.mode.udma) {
		*type = IDE_TRANSFER_ULTRA_DMA;
		*mode = ata_mask_to_mode(m.mode.udma);
	}
	else if (m.mode.mwdma) {
		*type = IDE_TRANSFER_MW_DMA;
		*mode = ata_mask_to_mode(m.mode.mwdma);
	}
	else if (m.mode.swdma) {
		*type = IDE_TRANSFER_SW_DMA;
		*mode = ata_mask_to_mode(m.mode.swdma);
	}
	else if (m.mode.pio) {
		*type = IDE_TRANSFER_PIO;
		*mode = ata_mask_to_mode(m.mode.pio);
	}
	else {
		*type = IDE_TRANSFER_PIO;
		*mode = ATA_MODE_0;
	}
}

/*
 * Method: getTransferModes:fromInfo:
 *
 * Purpose:
 * Given an infoPtr which points to information from the IDE Identify Drive
 * command, determine the transfer modes that the drive is capable of.
 *
 */
- (void)getTransferModes:(txferModes_t *)modes
	fromInfo:(ideIdentifyInfo_t *)infoPtr 
{
	unsigned char n;
    ata_mode_t m = ATA_MODE_0;
	int i;

    /*
     * For PIO, check if we support the ATA-2 additions. 
     */
	if (infoPtr->fieldValidity & IDE_WORDS64_TO_70_SUPPORTED) {
		if (infoPtr->fcPioDataTransferCyleTimingMode &
			IDE_FC_PIO_MODE_5_SUPPORTED)
			m = ATA_MODE_5;
		else if (infoPtr->fcPioDataTransferCyleTimingMode &
			IDE_FC_PIO_MODE_4_SUPPORTED)
			m = ATA_MODE_4;
		else if (infoPtr->fcPioDataTransferCyleTimingMode &
			IDE_FC_PIO_MODE_3_SUPPORTED)
			m = ATA_MODE_3;
	}
	else {
		n = (infoPtr->pioDataTransferCyleTimingMode &
			IDE_PIO_TIMING_MODE_MASK) >> 8;
		if (n >= 2) n = 2;
		m = (1 << n);	// convert number to mode
	}
		
	/* For mode 2 and above, IORDY must be supported. */
	if ((m > ATA_MODE_2) &&
		!(infoPtr->capabilities & IDE_CAP_IORDY_SUPPORTED))
		m = ATA_MODE_2;
	
	/*
	 * A drive supporting a certain mode must also support slower modes
	 * of the same transfer type.
	 */
	modes->mode.pio = ata_mode_to_mask(m);
	
	/*
	 * Drive must indicate that it supports DMA in Word 49 before
	 * continuing.
	 */
	if (!(infoPtr->capabilities & IDE_CAP_DMA_SUPPORTED)) {
		modes->mode.swdma = ATA_MODE_NONE;
		modes->mode.mwdma = ATA_MODE_NONE;
		modes->mode.udma  = ATA_MODE_NONE;
		return;
	}
	
	/*
	 * Single word DMA. Read from Word 52.
	 */
	n = (infoPtr->dmaDataTransferCyleTimingMode &
		IDE_DMA_TIMING_MODE_MASK) >> 8;
	if (n >= 2)
		n = 2;
	m = (1 << n);
	modes->mode.swdma = ata_mode_to_mask(m);	
	
	/*
	 * Multiword DMA. Read Word 63.
	 */
	m = ATA_MODE_NONE;
	for (i = 2; i >= 0; i--) {
		if (infoPtr->mwDma & (1 << i)) {
			m = (1 << i);
			break;
		}
	}
	/* Can't be Multiword DMA mode 1 and above and NOT support
	 * Words 64 through 70.
	 */
	if ((m >= ATA_MODE_1) &&
		!(infoPtr->fieldValidity & IDE_WORDS64_TO_70_SUPPORTED)) {
		m = ATA_MODE_0;
    }
	modes->mode.mwdma = ata_mode_to_mask(m);	
    
	/*
	 * Ultra DMA. Read capability from Word 88.
	 */
	m = ATA_MODE_NONE;
	if (infoPtr->fieldValidity & IDE_WORD88_SUPPORTED) {
		for (i = 2; i >= 0; i--) {
			if (infoPtr->UDma & (1 << i)) {
				m = (1 << i);
				break;
			}
		}
	}
	modes->mode.udma = ata_mode_to_mask(m);

    return;
}

/*
 * We do host dependent hardware initialization here. 
 */
- (BOOL) setControllerCapabilities
{
	if (_controllerID != PCI_ID_NONE) {
		if ([self setPCIControllerCapabilitiesForDrives:_drives] == NO)
			return NO;
		_transferWidth = [self getPIOTransferWidth];
		return YES;
	}
	return YES;
}

/*
 * Method: setATADriveCapabilities
 *
 * Send the Set Parameters command to the drive and set multi-sector mode
 * etc. if the drive supports them. If biosInfo is YES, then we are using
 * drive geometry information from the BIOS else we use Identify command to
 * get this. 
 */
- (ide_return_t)setATADriveCapabilities:(unsigned int)unit	
		withBIOSInfo:(BOOL)biosInfo
{
    ideRegsVal_t ideRegs;
    unsigned char nSectors;
	ide_return_t rtn;
    ideIdentifyInfo_t *infoPtr = _drives[unit].ideIdentifyInfo;

    _drives[unit].ideIdentifyInfoSupported = YES;
    bzero(infoPtr, sizeof(ideIdentifyInfo_t));

    _driveNum = unit;

    /*
     * Remember this command is optional. Failure means that either this
     * command is not supported or the drive is not present.
     */
    if ([self ideReadGetInfoCommon:&ideRegs 
    		client:(struct vm_map *)IOVmTaskSelf() 
    		addr:(unsigned char *)infoPtr
			command:IDE_IDENTIFY_DRIVE] != IDER_SUCCESS) {
		_drives[unit].ideIdentifyInfoSupported = NO;
		if (_ide_debug)
	    	IOLog("ATA: ideReadGetInfoCommon failed.\n");
		/*
		 * FIXME: If this is a slave device and we perform a reset. The
		 * master device will lose its configuration state.
		 */
		[self ideReset];	/* necessary */
        return IDER_CMD_ERROR;
    }

    /*
     * Fill in this struct similar to which we would have gotten from CMOS. 
     */
    if (biosInfo == NO)	{
		ideDriveInfo_t *ip = &(_drives[unit].ideInfo);
	
		ip->cylinders = _drives[unit].ideIdentifyInfo->cylinders;
		ip->heads = _drives[unit].ideIdentifyInfo->heads;
		ip->control_byte = 0;
		ip->landing_zone = 0;
		ip->sectors_per_trk = _drives[unit].ideIdentifyInfo->sectorsPerTrack;
		ip->bytes_per_sector = IDE_SECTOR_SIZE;
		if ([IODevice driverKitVersion] > 410) { 
			ip->total_sectors = ip->sectors_per_trk *
								ip->heads *
								ip->cylinders;
			
			/* If disk supports LBA, and Words (61:60) indicates that the
			 * user-addressable logical sectors is larger than the number
			 * of sectors computed through CHS translation, then use the
			 * larger value. This will be needed for disks with more than
			 * 16,514,064 sectors. (16383 C x 16 H x 63 S)
			 */
			if ((infoPtr->capabilities & IDE_CAP_LBA_SUPPORTED) &&
				(infoPtr->userAddressableSectors > ip->total_sectors))
				ip->total_sectors = infoPtr->userAddressableSectors;
		}
		else {
    		// fake capacity for backward compatibility.
			ip->total_sectors = ip->sectors_per_trk * ip->heads *
		    	ip->cylinders * ip->bytes_per_sector;
		}
    }

    /*
     * Set address mode. The only case in which we need to override user
     * selection if the user chooses LBA and the drive supports only CHS. 
     */
    if (((infoPtr->capabilities & IDE_CAP_LBA_SUPPORTED) == 0x0) &&
		(_drives[unit].addressMode == ADDRESS_MODE_LBA)) {
#ifdef DEBUG
		IOLog("%s: WARNING: LBA mode is not supported by drive %d.\n",
		    [self name], unit);
#endif DEBUG
		_drives[unit].addressMode = ADDRESS_MODE_CHS;
	}

    /*
     * Set disk parameters (Initialize Device Parameters command). 
     */
    [self ideSetParams:_drives[unit].ideInfo.sectors_per_trk
		numHeads:_drives[unit].ideInfo.heads ForDrive:unit];
	
	/*
	 * Determine the best transfer mode.
	 */
	[self getTransferModes:&_drives[unit].driveModes fromInfo:infoPtr];
	[self getBestTransferMode:&_drives[unit].transferMode
		type:&_drives[unit].transferType
		forUnit:unit];

	/*
	 * Determine the number of sectors for each Multiple Read/Write.
	 */
	nSectors = infoPtr->multipleSectors & IDE_MULTI_SECTOR_MASK;
	if ((_drives[unit].transferType == IDE_TRANSFER_PIO) &&
		(nSectors) && (_multiSectorRequested == YES)) {
		if ([self ideSetMultiSectorMode:&ideRegs numSectors:nSectors] ==
			IDER_SUCCESS) {
			_drives[unit].multiSector = nSectors;
		}
	}

#if 0
	IOLog("Drive      modes: %08x\n", _driveModes[unit].modes);
	IOLog("Controller modes: %08x\n", _controllerModes.modes);
	IOLog("drive %d: MODE:0x%02x TYPE:%d\n", unit, _transferMode[unit],
		_transferType[unit]);
#endif

	/*
	 * Set drive feature.
	 */
	rtn = [self ideSetDriveFeature:FEATURE_SET_TRANSFER_MODE
			value:_drives[unit].transferMode
			transferType:_drives[unit].transferType];

	if (rtn != IDER_SUCCESS) {
		IOLog("%s: Drive %d: set transfer mode failed\n", [self name], unit);
		_drives[unit].transferType = IDE_TRANSFER_PIO;
		_drives[unit].transferMode = ATA_MODE_0;
	}

    return IDER_SUCCESS;
}

/*
 * Method: setATAPIDriveCapabilities
 *
 * Identify the ATAPI device and set its transfer type/mode.
 */
- (ide_return_t)setATAPIDriveCapabilities:(unsigned int)unit	
{
    atapi_return_t 	rtn;

	_driveNum = unit;
		
	[self atapiSoftReset:unit];

	/*
	 * We have to do this test because of "task file shadow" bug
	 * present in many ATAPI CD-ROMs. 
	 */
	rtn = [self atapiIdentifyDevice:(struct vm_map *)IOVmTaskSelf()
			addr:(unsigned char *)_drives[unit].ideIdentifyInfo unit:unit];
		
	/* Too bad.. */
	if (rtn != IDER_SUCCESS)
		return rtn;

	[self printAtapiInfo:_drives[unit].ideIdentifyInfo Device:unit];
	[self atapiInitParameters:_drives[unit].ideIdentifyInfo Device:unit];

	_drives[unit].addressMode = ADDRESS_MODE_LBA;
	
	[self getTransferModes:&_drives[unit].driveModes
		fromInfo:_drives[unit].ideIdentifyInfo];
	[self getBestTransferMode:&_drives[unit].transferMode
		type:&_drives[unit].transferType forUnit:unit];

	rtn = [self ideSetDriveFeature:FEATURE_SET_TRANSFER_MODE
			value:_drives[unit].transferMode
			transferType:_drives[unit].transferType];
	if (rtn != IDER_SUCCESS) {
		IOLog("%s: Drive %d: set transfer mode failed\n",
			[self name], unit);
		_drives[unit].transferMode = ATA_MODE_0;
		_drives[unit].transferType = IDE_TRANSFER_PIO;
	}

	return IDER_SUCCESS;
}

-(ideIdentifyInfo_t *)getIdeIdentifyInfo:(unsigned int)unit
{
    return _drives[unit].ideIdentifyInfoSupported ?
		_drives[unit].ideIdentifyInfo : NULL;
}

-(BOOL)isDiskGeometry:(unsigned int)unit
{
    return (_drives[unit].biosGeometry == YES) ? NO : YES;
}


- (ideDriveInfo_t)getIdeInfoFromBIOS:(unsigned)unit 
{
    caddr_t hdtbl;
    ideDriveInfo_t hdInfo;

    hdInfo.type = [self getIdeTypeFromCMOS:unit];
    if ((hdInfo.type == 0) || (hdInfo.type < MIN_CMOS_IDETYPE) ||
	(hdInfo.type > MAX_CMOS_IDETYPE)) {
	hdInfo.type = 0;
	return (hdInfo);
    }

    /*
     * Drive info (the drive characteristic table) for drive 0 is located at
     * INT41h and for drive 1 it is located at INT46h. 
     */
    if (unit == 0) {
	hdtbl = (caddr_t) (*(unsigned short *)(0x41 << 2) +
			(*(unsigned short *)((0x41 << 2) + 2) << 4));
    } else if (unit == 1) {
	hdtbl = (caddr_t) (*(unsigned short *)(0x46 << 2) +
			(*(unsigned short *)((0x46 << 2) + 2) << 4));
    } else {
	hdInfo.type = 0;
	return (hdInfo);
    }

    if (hdtbl == 0) {
	IOLog("%s: drive %d, type %d, using geometry from CMOS.\n",
		[self name], unit, hdInfo.type);
    } else {
	IOLog("%s: drive %d, type %d, using geometry from INT table.\n",
		[self name], unit, hdInfo.type);
    }
    
    if (hdtbl == 0) {
	hdtbl = (caddr_t) pmap_phys_to_kern(CMOS_IDE_TABLE);
	hdtbl += ((hdInfo.type - 1) * SIZE_OF_IDE_TABLE);
    }
    
    hdInfo.cylinders = *(unsigned short *)hdtbl;
    hdInfo.heads = *(hdtbl + 2);
    hdInfo.precomp = *(unsigned short *)(hdtbl + 5);
    hdInfo.control_byte = *(hdtbl + 8);
    hdInfo.landing_zone = *(unsigned short *)(hdtbl + 12);
    hdInfo.sectors_per_trk = *(hdtbl + 14);
    hdInfo.bytes_per_sector = IDE_SECTOR_SIZE;
    if([IODevice driverKitVersion] > 410) { 
	hdInfo.total_sectors = hdInfo.sectors_per_trk *
	    hdInfo.heads * hdInfo.cylinders;
    }
    else {
    	// fake capacity for backward compatibility.
	hdInfo.total_sectors = hdInfo.sectors_per_trk *
	    hdInfo.heads * hdInfo.cylinders * hdInfo.bytes_per_sector;
    }

#ifdef DEBUG
    /*
     * FIXME: This is for testing only. Remove later. 
     */
    IOLog("%s: drive %d, %d cylinders, %d heads, %d sectors (BIOS).\n", 
	[self name], unit, hdInfo.cylinders, hdInfo.heads, 
	hdInfo.sectors_per_trk);
#endif DEBUG

    return hdInfo;
}

- (unsigned)getIdeTypeFromCMOS:(int)unit
{
    unsigned int hdtype;

    outb(CMOSADDR, HDTYPE);
    IODelay(10);
    hdtype = inb(CMOSDATA);

    if (unit == 0) {
	hdtype = hdtype >> 4;
	if (hdtype != 0xf) {
	    return hdtype;
	} else {
	    outb(CMOSADDR, HD0_EXTTYPE);
	    IODelay(10);
	    hdtype = inb(CMOSDATA);
	}
    } else {
	hdtype = hdtype & 0x0f;
	if (hdtype != 0xf) {
	    return hdtype;
	} else {
	    outb(CMOSADDR, HD1_EXTTYPE);
	    IODelay(10);
	    hdtype = inb(CMOSDATA);
	}
    }


    return hdtype;
}

@end

