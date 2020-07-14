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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/*
 * Copyright (c) 1994-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCntInit.m - ATA controller initialization module. 
 *
 */

#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/ppc/IODBDMA.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "IdeDDM.h"
#import "io_inline.h"
#import <driverkit/return.h>
#import <mach/port.h>
#import <machdep/ppc/powermac.h>
#import <bsd/sys/systm.h>

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdeCntDma.h"
#import "AtapiCntCmds.h"


#define IDE_SYSCLK_NS	30

typedef struct
{
    int		accessTime;
    int		cycleTime;
} ideModes_t;

static ideModes_t pioModes[] =
{
    { 165,    600 },	/* Mode 0 */
    { 125,    383 },	/*      1 */ 
    { 100,    240 },	/*      2 */
    {  80,    180 },	/*      3 */
    {  70,    120 }	/*      4 */
};
#define MAX_PIO_MODES  (sizeof(pioModes) / sizeof(ideModes_t))

static ideModes_t singleWordModes[] =
{
    { 480,    960 },	/* Mode 0 */
    { 100,    480 },	/*      1 */
    { 120,    240 }	/*      2 */
};
#define MAX_SINGLEWORD_MODES  (sizeof(singleWordModes) / sizeof(ideModes_t))

static ideModes_t multiWordModes[] =
{
    { 215,    480 },	/* Mode 0 */
    {  80,    150 },	/*      1 */
    {  70,    120 }	/*      2 */
};
#define MAX_MULTIWORD_MODES  (sizeof(multiWordModes) / sizeof(ideModes_t))


static ideModes_t ultraModes[] =
{
    {   0,    114 },	/* Mode 0 */
    {   0,     75 },	/*      1 */
    {   0,     55 }	/*      2 */
};

#define MAX_ULTRA_MODES  (sizeof(ultraModes) / sizeof(ideModes_t))


static void endianSwap16Bit( volatile u_int16_t *p16 )
{
    u_int16_t     tmp16;
    
    tmp16 = *p16;
    tmp16 = (tmp16 >> 8) | (tmp16 << 8);
    *p16  = tmp16;
}

static void endianSwap32Bit( volatile u_int32_t *p32 )
{
    u_int32_t     tmp32;
    
    tmp32 = *p32;

    tmp32 = ( ((tmp32 & 0x000000ff) << 24)
            | ((tmp32 & 0x0000ff00) <<  8)
            | ((tmp32 & 0x00ff0000) >>  8)
            | ((tmp32 & 0xff000000) >> 24) );

    *p32  = tmp32;
}

static int rnddiv( int x, int y )
{
    if ( x < 0 )
      return 0;
    else
      return ( (x / y) + (( x % y ) ? 1 : 0) );
}

@implementation IdeController(Initialize)


/*
 * Check for MediaBay 
 *
 * Returns: NO  - MediaBay implementation with non-ATA/ATAPI option installed
 *          YES - Non-MediaBay implementation or MediaBay with ATA/ATAPI option
 */
- (BOOL) checkMediaBayOption: (IOTreeDevice *)deviceDescription
{
    int			n;
    u_int32_t		*heathrowIdReg;
    u_int32_t		heathrowIds;
    IOTreeDevice	*devDescParent;
    char 		*nodeNameParent;

    /*
     * Check for MediaBay implementation 
     */
    devDescParent  = [[deviceDescription parent] deviceDescription];
    nodeNameParent = [devDescParent nodeName];

    if ( strcmp( nodeNameParent, "media-bay" ) )
    {
        return YES;	/* Non-MediaBay implementation */
    }
    if ( (n = [devDescParent numMemoryRanges]) < 1 )
    {
        return YES;	/* Misc OF Device Tree problem */
    }
    	
    /* 
     * Check for ATA/ATAPI MediaBay option installed
     */
    heathrowIdReg = (u_int32_t *)[devDescParent memoryRangeList][0].start; 
    heathrowIds   = *heathrowIdReg;
    endianSwap32Bit( &heathrowIds );

    if ( (heathrowIds & MEDIA_BAY_ID_MASK) == MEDIA_BAY_ID_CDROM )
    {
        return YES; 	/* Media Bay with ATA/ATAPI option */
    }

    return NO; 
}

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
    
    dh = _addressMode[unit];
    
    dh |= (unit ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);

    /*
     * Test with some data. 
     */
    
    if (unit == 0)	
    {
	outb(_ideRegsAddrs.cylLow, 0xaa);
	outb(_ideRegsAddrs.cylHigh, 0xbb);
    } 
    else 
    {
	outb(_ideRegsAddrs.cylLow, 0xba);
	outb(_ideRegsAddrs.cylHigh, 0xbe);
    }
    
    if (unit == 0) 
    {
	if ((inb(_ideRegsAddrs.cylLow) != 0xaa) ||
	    (inb(_ideRegsAddrs.cylHigh) != 0xbb)) 
        {
	    return NO;
	}
    } 
    else 
    {
	if ((inb(_ideRegsAddrs.cylLow) != 0xba) ||
	    (inb(_ideRegsAddrs.cylHigh) != 0xbe)) 
        {
	    return NO;
	}
    }
    
    return YES;
}


/*
 * One-time only init. Returns NO on failure. 
 */
- (BOOL)ideControllerInit:(IOTreeDevice *)deviceDescription
{
    int     		unit;
    int     		drivesPresent = 0;
    ns_time_t		time, startTime;

    if ([self enableInterrupt:0]) {
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
    [self assignRegisterAddresses: deviceDescription];
        
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) {
	_atapiDevice[unit] = NO;
    }
    
    /*
     * We have to test here if the controller is present or else we will
     * block looking for a drive (in ideReset). 
     */
    
    if ([self controllerPresent] == NO)	
    {
	IOLog("%s: no devices detected at port 0x%0x\n", [self name],
		_ideRegsAddrs.data);
	[self ideCntrlrUnLock];
	return NO;
    } 
    else 
    {
    	IOLog("%s: device detected at port 0x%x irq %d\n", [self name], 
			_ideRegsAddrs.data, [deviceDescription interrupt]);
    }
    
    _ideInterruptPort = [self interruptPort];
    _ideDevicePort = [deviceDescription devicePort];

    [self setInterruptTimeOut:IDE_INTR_TIMEOUT];

    _multiSectorRequested = YES;

    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) 
    {
    	_addressMode[unit] = ADDRESS_MODE_LBA;
    }

    _driveNum = unit;
    
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) 
    {
    
	bzero(&_ideInfo[unit], sizeof(ideDriveInfo_t));
	
	[self ideReset];
	
        IOGetTimestamp(&startTime);
	if ([self ideDetectDrive:unit] == YES)	
        {
	    _ideInfo[unit].type = 255;
	}
        IOGetTimestamp(&time);
        time = (time - startTime) / (1000*1000);
        kprintf( "Disk(ata) - ideDetectDrive:%d - %d ms\n\r", unit, (u_int32_t)time ); 
 
	/*
	 * If we didn't find ATA drive then look for ATAPI device. 
	 */
	if ((_ideInfo[unit].type == 0)  && ([self ideDetectATAPIDevice:unit] == YES)) 
        {
	    _ideInfo[unit].type = 127;
	}

	if (_ideInfo[unit].type != 0)
	    drivesPresent += 1 ;
    }
    
    if (drivesPresent == 0)	
    {
	[self ideCntrlrUnLock];
	IOLog("ideControllerInit: failed drivesPresent\n");
	return NO;
    }

    /*
     * Initialization was successful. We should release the lock so that the
     * controller can now execute commands from disk objects. 
     */
    
    [self resetAndInit];

    /*
     * Set drive power state to active.
     */
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) 
    {
	if (_ideInfo[unit].type != 0)
	    [self setDrivePowerState:IDE_PM_ACTIVE];
    }
    
    _driveSleepRequest = NO;
    
    [self enableInterrupts];

    [self ideCntrlrUnLock];
    
    return YES;
}    

#define ATA_RESET_DELAY		(30 * 1000)		/* milliseconds */

/*
 * Determine the presence and type of drive (IDE/ATAPI) attached to this
 * controller. 
 *
 */

-(BOOL)ideDetectDrive:(unsigned int)unit
{
    unsigned char 	dh = _addressMode[unit];
    int 		i;
    unsigned char 	status;
    BOOL 		found = NO;

    [self disableInterrupts];
    
    dh |= (unit ? SEL_DRIVE1 : SEL_DRIVE0);
    outb(_ideRegsAddrs.drHead, dh);
    IODelay(10);

    status = inb(_ideRegsAddrs.status);
    if ( !(status & READY) )
    {
        return found;
    }
  
    outb(_ideRegsAddrs.command, IDE_IDENTIFY_DRIVE);

    for (i = 0; i < (ATA_RESET_DELAY * 5); i++) 
    {
	IODelay(200);
        status = inb(_ideRegsAddrs.status);

	if ((!(status & BUSY)) && (status & (ERROR | WRITE_FAULT | ERROR_CORRECTED)) )
        {
	    break;				/* certain failure */
	}
	
	if ((!(status & BUSY)) && (status & DREQUEST))	
        {
	    found = YES;			/* success */
	    break;
	}    
    }

    return found;    
}

/*
 * We send the device ATAPI soft reset command and check for signature. This
 * is the first command that ATAPI devices need. 
 */
-(BOOL)ideDetectATAPIDevice:(unsigned int)unit
{
    unsigned char low, high;
    BOOL found = NO;
	
    [self atapiSoftReset:unit];

    low = inb(_ideRegsAddrs.cylLow);
    high = inb(_ideRegsAddrs.cylHigh);
    
    if ((low == ATAPI_SIGNATURE_LOW) && (high == ATAPI_SIGNATURE_HIGH))
    {
	_atapiDevice[unit] = YES;
	IOLog("%s: ATAPI device %d detected.\n", [self name], unit);
	found = YES;
    }
    /*
     * Clear ATAPI signature
     */
    outb(_ideRegsAddrs.cylLow, 0 );
    outb(_ideRegsAddrs.cylHigh, 0 );
    
    return found;    
}


-(ideDriveInfo_t *)getIdeDriveInfo:(unsigned int)unit
{
    return &_ideInfo[unit];
}

#define MAX_RESET_ATTEMPTS 		2

- (void)ideReset
{
    int 		count;
    int 		delay;
    unsigned char 	status;

    for ( count = 0; count < MAX_IDE_DRIVES; count++ )
    {
        _driveReset[count] = YES;
    }

    for (count = 0; count < MAX_RESET_ATTEMPTS; count++) 
    {
	outb(_ideRegsAddrs.deviceControl, DISK_RESET_ENABLE);
	IODelay(100);	   		/* spec >= 25 us */
	outb(_ideRegsAddrs.deviceControl, 0x0);
	
	[self enableInterrupts];

    	delay = 31 * 1000 * 1000;	/* thirty one seconds */
	
	IOSleep(1000);			/* Enough time to assert busy */
	
	while (delay > 0) 
        {
	    status = inb(_ideRegsAddrs.status);
	    if (!(status & BUSY)) 
            {
		return;
	    }
	    
	    IOSleep(1);
	    delay -= 1000;
	}
	
	IOLog("%s: Reset failed, retrying..\n", [self name]);
	IOSleep(2000);
    }    
}


/*
 * Initialize the IDE interface. Find about drive capabilities (by
 * IDE_IDENTIFY_DRIVE command) and configure drive.
 * 
 * By the time we enter this routine we know about all devices connected to this controller. 
 */
- (void)resetAndInit
{
    int 			i;
    unsigned char 		unit;
    ide_return_t 		rtn;
    atapi_return_t 		atapi_rtn;
    BOOL 			ataDevicePresent;

    /*
     * If there is at least one ATA drive connected to this controller then
     * is is not necessary to (ATA style) reset them. We use the ATAPI reset
     * instead. 
     */
    ataDevicePresent = NO;
    for (i = 0; i < MAX_IDE_DRIVES; i++)	
    {
	if ((_ideInfo[i].type != 0) && ([self isAtapiDevice:i] == NO))
        {
	    ataDevicePresent = YES;
	    break;
	}
    }

    if (ataDevicePresent == YES)	
    {
	[self ideReset];
    }
    
    /*
     * Send the controller necessary commands to set capabilities and then
     * set drive parameters. It is necessary to set drive parameters before
     * any data I/O can be done from the disk. 
     */
    for (unit = 0; unit < MAX_IDE_DRIVES; unit++) 
    {
	if (_ideInfo[unit].type == 0)	
        {
	    continue;
	}
	
	_driveNum = unit;

        _cycleTimes[unit].pioMode = 0;
        _cycleTimes[unit].pioAccessTime  = pioModes[0].accessTime;
        _cycleTimes[unit].pioCycleTime   = pioModes[0].cycleTime;

        [self calcIdeConfig: unit];          
	
	if ( [self isAtapiDevice:unit] == YES )	
        {
	    [self atapiSoftReset:unit];

	    /*
	     * We have to do this test because of "task file shadow" bug
	     * present in many ATAPI CD-ROMs. 
	     */
	    atapi_rtn = 
	    	[self atapiIdentifyDevice:(struct vm_map *)IOVmTaskSelf() 
		    addr:(unsigned char *)&_ideIdentifyInfo[unit] unit:unit];
		
	    /* Too bad.. */
	    if (atapi_rtn != IDER_SUCCESS)	
            {
		_atapiDevice[unit] = NO;
		_ideInfo[unit].type = 0;
		continue;
	    }
		
            [self endianSwapIdentifyData: &_ideIdentifyInfo[unit]];

	    [self printAtapiInfo:&_ideIdentifyInfo[unit] Device:unit];
	    [self atapiInitParameters:&_ideIdentifyInfo[unit] Device:unit];
	    continue;
	}

	rtn = [self setATADriveCapabilities:unit];
	if (rtn != IDER_SUCCESS)	
        {
		_ideInfo[unit].type = 0;
		continue;
	}
    }
}

/*
 * FIXME: Need to figure out multi-word DMA.
 */
- (ide_return_t)getTransferModes:(ideIdentifyInfo_t *)infoPtr Unit: (int)unit
{
    int		i, n;
     
    int         pioMode		= 0;
    int		pioCycleTime	= 0;
    int         dmaMode		= 0;
    int         dmaType		= IDE_DMA_NONE;
    int         dmaCycleTime	= 0;
    
    /*
     *  PIO Cycle timing......  
     *
     *  1. Try to match Word 51 (pioCycleTime) with cycle timings
     *     in our pioModes table to get mode/CycleTime. (Valid for Modes 0-2)
     *  2. If Words 64-68 are supported and Mode 3 or 4 supported check, 
     *     update CycleTime with Word 68 (CycleTimeWithIORDY).
     */

    pioCycleTime = infoPtr->pioDataTransferCyleTimingMode;

    if ( pioCycleTime > 2 )
    {
        for ( i=MAX_PIO_MODES-1; i != -1; i-- )
        {
            if ( pioCycleTime <= pioModes[i].cycleTime )
            {
                pioMode = i;
                break;
            }
         }

         if ( i == -1 )
         {
             pioCycleTime = pioModes[pioMode].cycleTime;
         }
    }
    else
    {
        pioMode      = pioCycleTime;
        pioCycleTime = pioModes[pioMode].cycleTime;
    }

    if (infoPtr->fieldValidity & IDE_WORDS64_TO_68_SUPPORTED) 
    {
	if (infoPtr->fcPioDataTransferCyleTimingMode & IDE_FC_PIO_MODE_4_SUPPORTED)
            pioMode = 4;
	else if (infoPtr->fcPioDataTransferCyleTimingMode & IDE_FC_PIO_MODE_3_SUPPORTED)
            pioMode = 3;

        if ( (pioMode >= 3) && infoPtr->MinPIOTransferCycleTimeWithIORDY )
        {
            pioCycleTime = infoPtr->MinPIOTransferCycleTimeWithIORDY;
        }
    }
    
    /* 
     *  Ultra DMA timing.....
     *
     */                                                                
    if ( (_controllerType == kControllerTypeCmd646X) && (infoPtr->fieldValidity & IDE_WORD_88_SUPPORTED) ) 
    {
        n = infoPtr->ultraDma & IDE_ULTRA_DMA_SUPPORTED;
        if ( n )
        {
            dmaType = IDE_DMA_ULTRA;
            for ( i=0; n; i++, n>>=1 )
              ;

            dmaMode = i - 1;
            if ( dmaMode > MAX_ULTRA_MODES-1 )
            {
                dmaMode = MAX_ULTRA_MODES-1;
            }
            dmaCycleTime = ultraModes[dmaMode].cycleTime;

            goto getCycleTimes_exit;
        }
    }
            
    /* 
     *  Multiword DMA timing.....
     *
     *  1. Check Word 63(7:0) (Multiword DMA Modes Supported). Lookup
     *     CycleTime for highest mode we support.
     *  2. If Words 64-68 supported, update CycleTime from Word 66
     *     (RecommendedMultiWordCycleTime) if specified.
     */                                                                

    n = infoPtr->mwDma & IDE_MW_DMA_SUPPORTED;
    if ( n )
    {
        dmaType = IDE_DMA_MULTIWORD;
        for ( i=0; n; i++, n>>=1 )
          ;

        dmaMode = i - 1;
        if ( dmaMode > MAX_MULTIWORD_MODES-1 )
        {
            dmaMode = MAX_MULTIWORD_MODES-1;
        }
        dmaCycleTime = multiWordModes[dmaMode].cycleTime;

        if (infoPtr->fieldValidity & IDE_WORDS64_TO_68_SUPPORTED) 
        {
            if ( infoPtr->RecommendedMwDMACycleTime )
            {
                dmaCycleTime = infoPtr->RecommendedMwDMACycleTime;
            }
        }
        goto getCycleTimes_exit;
    }

    /*
     *  Single Word DMA timing.....
     *
     *  1. If Word 64(7:0) (SingleWord DMA Modes supported) is non-zero,
     *     find highest supported mode and get CycleTime from table.
     *  2. If Word 64(7:0) (SingleWord DMA Modes supported) is zero,
     *     then match Word 52 (Single Word DMA Cycle Time) against
     *     cycle times in singleWordModes table.
     */          
       
    n = infoPtr->swDma & IDE_SW_DMA_SUPPORTED;
    if ( n )
    {
        dmaType = IDE_DMA_SINGLEWORD;
        for ( i=0; n; i++, n>>=1 )
          ;

        dmaMode = i - 1;
        if ( dmaMode > MAX_SINGLEWORD_MODES-1 )
        {
            dmaMode = MAX_SINGLEWORD_MODES-1;
        }
        dmaCycleTime = singleWordModes[dmaMode].cycleTime;
    }
    else
    {
        dmaCycleTime = infoPtr->dmaDataTransferCyleTimingMode;
        for ( i=MAX_SINGLEWORD_MODES-1; i != -1; i-- )
        {
            if ( dmaCycleTime <= singleWordModes[i].cycleTime )
            {
                dmaType = IDE_DMA_SINGLEWORD;
                dmaMode = i;
                dmaCycleTime = singleWordModes[i].cycleTime;
                break;
            }
        }
    }


getCycleTimes_exit: ;

    _cycleTimes[unit].pioCycleTime   = pioCycleTime;
    _cycleTimes[unit].pioAccessTime  = pioModes[pioMode].accessTime;
    _cycleTimes[unit].pioMode        = pioMode;

    if ( dmaType != IDE_DMA_NONE )
    {
        _cycleTimes[unit].dmaType        = dmaType;
        _cycleTimes[unit].dmaMode        = dmaMode;
        _cycleTimes[unit].dmaCycleTime   = dmaCycleTime;
        switch ( dmaMode )
        {
            case IDE_DMA_SINGLEWORD:
                _cycleTimes[unit].dmaAccessTime = singleWordModes[dmaMode].accessTime;
                break;
            case IDE_DMA_MULTIWORD:
                _cycleTimes[unit].dmaAccessTime = multiWordModes[dmaMode].accessTime;
                break;
            case IDE_DMA_ULTRA:
                _cycleTimes[unit].dmaAccessTime = ultraModes[dmaMode].accessTime;
                break;
        }    
    }

    [self calcIdeConfig: (int)unit];

    
    return IDER_SUCCESS;
}

-(void) calcIdeConfig:(int)unit
{
    _cycleTimes[unit].fChanged = YES;

    if ( _controllerType == kControllerTypeCmd646X )
    {
        [self calcIdeTimingsCmd646X:unit];
    }
    else
    {
        [self calcIdeTimingsDBDMA:unit];
    }
}

-(void) calcIdeTimingsCmd646X:(int) unit
{
    u_int32_t		cycleClks;
    u_int32_t		cycleTime,  accessTime;
    u_int32_t		drwActClks, drwRecClks;
    u_int32_t		drwActTime, drwRecTime;
   
    _cycleTimes[unit].ideConfig.cmd646XConfig.arttimReg  = 0x40;
    _cycleTimes[unit].ideConfig.cmd646XConfig.cmdtimReg  = 0xA9;

    accessTime = pioModes[_cycleTimes[unit].pioMode].accessTime;

    drwActClks    =  accessTime / IDE_SYSCLK_NS;
    drwActClks   += (accessTime % IDE_SYSCLK_NS) ? 1 : 0;
    drwActTime    = drwActClks * IDE_SYSCLK_NS;

    drwRecTime    = pioModes[_cycleTimes[unit].pioMode].cycleTime - drwActTime;
    drwRecClks    = drwRecTime / IDE_SYSCLK_NS;
    drwRecClks   += (drwRecTime % IDE_SYSCLK_NS) ? 1 : 0;

    if ( drwRecClks >= 16 ) 
        drwRecClks = 1;
    else if ( drwRecClks <= 1 )
        drwRecClks = 16;

    _cycleTimes[unit].ideConfig.cmd646XConfig.drwtimRegPIO = ((drwActClks & 0x0f) << 4) | ((drwRecClks-1)  & 0x0f);


    _cycleTimes[unit].ideConfig.cmd646XConfig.udidetcrReg = 0;      

    if ( _cycleTimes[unit].dmaType == IDE_DMA_ULTRA )
    {
        cycleTime = _cycleTimes[unit].dmaCycleTime;
        
        cycleClks  = cycleTime / IDE_SYSCLK_NS;
        cycleClks += (cycleTime % IDE_SYSCLK_NS) ? 1 : 0;

        _cycleTimes[unit].ideConfig.cmd646XConfig.udidetcrReg = (0x01 << unit) | ((cycleClks-1) << ((!unit) ? 4 : 6)) ;  
    }
    else if ( _cycleTimes[unit].dmaType != IDE_DMA_NONE )
    {
        accessTime    = _cycleTimes[unit].dmaAccessTime;

        drwActClks    =  accessTime / IDE_SYSCLK_NS;
        drwActClks   += (accessTime % IDE_SYSCLK_NS) ? 1 : 0;
        drwActTime    = drwActClks * IDE_SYSCLK_NS;

        drwRecTime    = _cycleTimes[unit].dmaCycleTime - drwActTime;
        drwRecClks    = drwRecTime / IDE_SYSCLK_NS;
        drwRecClks   += (drwRecTime % IDE_SYSCLK_NS) ? 1 : 0;

        if ( drwRecClks >= 16 ) 
            drwRecClks = 1;
        else if ( drwRecClks <= 1 )
            drwRecClks = 16;

        _cycleTimes[0].ideConfig.cmd646XConfig.drwtimRegDMA = ((drwActClks & 0x0f) << 4) | ((drwRecClks-1)  & 0x0f);        
    }
}
        
 

-(void) calcIdeTimingsDBDMA:(int) unit
{
    int			accessTime;
    int			accessTicks;
    int         	recTime;
    int			recTicks;
    int        		cycleTime;
    int                 cycleTimeOrig;
    int         	halfTick = 0;
    uint                ideConfigWord = 0;

    /*
     * Calc PIO access time >= pioAccessTime in SYSCLK increments
     */
    accessTicks = rnddiv(_cycleTimes[unit].pioAccessTime, IDE_SYSCLK_NS);
    /*
     * Hardware limits access times to >= 120 ns 
     */
    accessTicks -= IDE_PIO_ACCESS_BASE;
    if (accessTicks < IDE_PIO_ACCESS_MIN )
    {
        accessTicks = IDE_PIO_ACCESS_MIN;
    }
    accessTime = (accessTicks + IDE_PIO_ACCESS_BASE) * IDE_SYSCLK_NS;

    /*
     * Calc recovery time in SYSCLK increments based on time remaining in cycle
     */
    recTime = _cycleTimes[unit].pioCycleTime - accessTime;
    recTicks = rnddiv( recTime, IDE_SYSCLK_NS );
    /*
     * Hardware limits recovery time to >= 150ns 
     */
    recTicks -= IDE_PIO_RECOVERY_BASE;
    if ( recTicks < IDE_PIO_RECOVERY_MIN )
    {
      recTicks = IDE_PIO_RECOVERY_MIN;
    }

    cycleTime = (recTicks + IDE_PIO_RECOVERY_BASE + accessTicks + IDE_PIO_ACCESS_BASE) * IDE_SYSCLK_NS;

    ideConfigWord = accessTicks | (recTicks << 5);

    kprintf("Disk(ata): Controller Base = %08x\n\r", _ideRegsAddrs.data );
    kprintf("Disk(ata): Unit %1d: PIO Requested Timings: Access: %3dns Cycle: %3dns \n\r", 
             unit, _cycleTimes[unit].pioAccessTime, _cycleTimes[unit].pioCycleTime);
    kprintf("Disk(ata):         PIO Actual    Timings: Access: %3dns Cycle: %3dns\n\r",
             accessTime, cycleTime );

    if ( _cycleTimes[unit].dmaType == IDE_DMA_NONE )
    {
      goto calcConfigWordDone;
    }

    /*
     * Calc DMA access time >= dmaAccessTime in SYSCLK increments
     */

    /*
     * OHare II erata - Cant handle write cycle times below 150ns
     */
    cycleTimeOrig = _cycleTimes[unit].dmaCycleTime;
    if ( IsPowerStar() )
    {
        if ( cycleTimeOrig < 150 ) _cycleTimes[unit].dmaCycleTime = 150;
    }

    accessTicks = rnddiv(_cycleTimes[unit].dmaAccessTime, IDE_SYSCLK_NS);

    accessTicks -= IDE_DMA_ACCESS_BASE;
    if ( accessTicks < IDE_DMA_ACCESS_MIN )
    {
        accessTicks = IDE_DMA_ACCESS_MIN;
    }
    accessTime = (accessTicks + IDE_DMA_ACCESS_BASE) * IDE_SYSCLK_NS;

    /*
     * Calc recovery time in SYSCLK increments based on time remaining in cycle
     */
    recTime = _cycleTimes[unit].dmaCycleTime - accessTime;    
    recTicks = rnddiv( recTime, IDE_SYSCLK_NS );

    recTicks -= IDE_DMA_RECOVERY_BASE;
    if ( recTicks < IDE_DMA_RECOVERY_MIN )
    {
        recTicks = IDE_DMA_RECOVERY_MIN;
    }
    cycleTime = (recTicks + IDE_DMA_RECOVERY_BASE + accessTicks + IDE_DMA_ACCESS_BASE) * IDE_SYSCLK_NS;
 
    /*
     * If our calculated access time is at least SYSCLK/2 > than what the disk requires, 
     * see if selecting the 1/2 Clock option will help. This adds SYSCLK/2 to 
     * the access time and subtracts SYSCLK/2 from the recovery time.
     * 
     * By setting the H-bit and subtracting one from the current access tick count,
     * we are reducing the current access time by SYSCLK/2 and the current recovery
     * time by SYSCLK/2. Now, check if the new cycle time still meets the disk's requirements.
     */  
    if ( (accessTicks > IDE_DMA_ACCESS_MIN) &&  ((accessTime - IDE_SYSCLK_NS/2) >= _cycleTimes[unit].dmaAccessTime) )
    {
        if ( (cycleTime - IDE_SYSCLK_NS) >= _cycleTimes[unit].dmaCycleTime )
        {
            halfTick    = 1;
            accessTicks--;
            accessTime -= IDE_SYSCLK_NS/2;
            cycleTime  -= IDE_SYSCLK_NS;
        }
    }

    ideConfigWord |= (accessTicks | (recTicks << 5) | (halfTick << 10)) << 11;

    kprintf("Disk(ata):         DMA Requested Timings: Access: %3dns Cycle: %3dns \n\r",  
             _cycleTimes[unit].dmaAccessTime, cycleTimeOrig);
    kprintf("Disk(ata):         DMA Actual    Timings: Access: %3dns Cycle: %3dns\n\r",   
             accessTime, cycleTime );

calcConfigWordDone: ;
    endianSwap32Bit( &ideConfigWord );
    _cycleTimes[unit].ideConfig.dbdmaConfig = ideConfigWord;

}


/*
 * Send the Set Parameters command to the drive and set multi-sector mode
 * etc. if the drive supports them. If biosInfo is YES, then we are using
 * drive geometry information from the BIOS else we use Identify command to
 * get this. 
 */
- (ide_return_t)setATADriveCapabilities:(unsigned int)unit	
{
    ideRegsVal_t 		ideRegs;
    unsigned char 		nSectors;
    char 			*buf;
    
    ideIdentifyInfo_t 		*infoPtr = &_ideIdentifyInfo[unit];


    _multiSector[unit] = 0;
    _dmaSupported[unit] = NO;

    _ideIdentifyInfoSupported[unit] = YES;
    bzero(infoPtr, sizeof(ideIdentifyInfo_t));

    /*
     * Remember this command is optional. Failure means that either this
     * command is not supported or the drive is not present.
     */
    _driveNum = unit;

    [self setTransferRate: _driveNum UseDMA:NO];

    if ([self ideReadGetInfoCommon:&ideRegs 
    		client:(struct vm_map *)IOVmTaskSelf() 
    		addr:(unsigned char *)infoPtr
		command:IDE_IDENTIFY_DRIVE] != IDER_SUCCESS) 
    {
	_ideIdentifyInfoSupported[unit] = NO;
	[self ideReset];	/* necessary */
        return IDER_CMD_ERROR;
    }

    [self endianSwapIdentifyData: infoPtr];

    /*
     * Fill in this struct similar to which we would have gotten from CMOS. 
     */
    _ideInfo[unit].cylinders 		= _ideIdentifyInfo[unit].cylinders;
    _ideInfo[unit].heads 		= _ideIdentifyInfo[unit].heads;
    _ideInfo[unit].control_byte 	= 0;
    _ideInfo[unit].landing_zone 	= 0;
    _ideInfo[unit].sectors_per_trk 	= _ideIdentifyInfo[unit].sectorsPerTrack;
    _ideInfo[unit].bytes_per_sector 	= IDE_SECTOR_SIZE;

    if ( infoPtr->capabilities & IDE_CAP_LBA_SUPPORTED)
    {
    _ideInfo[unit].total_sectors = infoPtr->userAddressableSectors;
    }
    else
    {
        _ideInfo[unit].total_sectors = _ideInfo[unit].sectors_per_trk  
	    	    			* _ideInfo[unit].heads 
					* _ideInfo[unit].cylinders;
    }

    /*
     * Set address mode. The only case in which we need to override user
     * selection if the user chooses LBA and the drive supports only CHS. 
     */
    if ((infoPtr->capabilities & IDE_CAP_LBA_SUPPORTED) == 0x0)	
    {
	_addressMode[unit] = ADDRESS_MODE_CHS;
    }

    /*
     * Set disk parameters. 
     */
    [self ideSetParams:_ideInfo[unit].sectors_per_trk
	    numHeads:_ideInfo[unit].heads ForDrive:unit];

    /*
     * Decide how are we going to do data transfers. Is DMA supported?
     * Multisector support? Verify that the data transfer mode really works
     * by doing a read operation in that mode. 
     */
    [self getTransferModes:infoPtr Unit:unit];

    /*
     * Set the Disk/CDROM transfer mode (Set Features SC=3) 
     */
    [self setTransferMode: unit]; 

    if (infoPtr->capabilities & IDE_CAP_DMA_SUPPORTED) 
    {
        if ( [self allocDmaMemory] == IDER_SUCCESS )
        { 
            _dmaSupported[unit] = YES;
            [self initIdeDma: unit];
        }
    }

    /*
     * We don't expect errors here since the drive claims that it can do
     * multiple sector transfers.
     */
    nSectors = infoPtr->multipleSectors & IDE_MULTI_SECTOR_MASK;

    if ( nSectors )	
    {	
	if ([self ideSetMultiSectorMode:&ideRegs
			 numSectors:nSectors] == IDER_SUCCESS) 
        {

            [self setTransferRate: unit UseDMA: NO];

	    _multiSector[unit] = nSectors;
	    ideRegs = [self logToPhys:0 numOfBlocks:nSectors];

	    buf = IOMalloc(nSectors * IDE_SECTOR_SIZE);
	    if ([self ideReadMultiple:&ideRegs 
	    		client:(struct vm_map *)IOVmTaskSelf()
	    		addr:buf] != IDER_SUCCESS)	
            {
	    	IOLog("%s: Read Multiple does not work!\n", [self name]);
		_multiSector[unit] = 0;
	    }
	    IOFree(buf, nSectors * IDE_SECTOR_SIZE);
	}
    }
    /*
     * A drive may not support set features command. Do we need to do
     * anything special if this command fails?
     */

    return IDER_SUCCESS;
}

-(ideIdentifyInfo_t *)getIdeIdentifyInfo:(unsigned int)unit
{
    return _ideIdentifyInfoSupported[unit] ? &_ideIdentifyInfo[unit] : NULL;
}

-(void)endianSwapIdentifyData:(ideIdentifyInfo_t *)pID
{
    endianSwap16Bit(&pID->genConfig);
    endianSwap16Bit(&pID->cylinders);
    endianSwap16Bit(&pID->heads);
    endianSwap16Bit(&pID->unformattedBytesPerTrack);
    endianSwap16Bit(&pID->unformattedBytesPerSector);
    endianSwap16Bit(&pID->sectorsPerTrack);
    endianSwap16Bit(&pID->bufferType);
    endianSwap16Bit(&pID->bufferSize); 
    endianSwap16Bit(&pID->eccBytes); 
    endianSwap16Bit(&pID->multipleSectors);
    endianSwap16Bit(&pID->doubleWordIO);			
    endianSwap16Bit(&pID->capabilities);
    endianSwap16Bit(&pID->pioDataTransferCyleTimingMode);
    endianSwap16Bit(&pID->dmaDataTransferCyleTimingMode);
    endianSwap16Bit(&pID->fieldValidity);	
    endianSwap16Bit(&pID->currentCylinders);
    endianSwap16Bit(&pID->currentHeads);
    endianSwap16Bit(&pID->currentSectorsPerTrack);
    endianSwap16Bit(&pID->capacity[0]);
    endianSwap16Bit(&pID->capacity[1]); 	
    endianSwap16Bit(&pID->multiSectorInfo);
    endianSwap32Bit(&pID->userAddressableSectors);	 	
    endianSwap16Bit(&pID->swDma);	
    endianSwap16Bit(&pID->mwDma);	
    endianSwap16Bit(&pID->ultraDma);	
    endianSwap16Bit(&pID->fcPioDataTransferCyleTimingMode); 
    endianSwap16Bit(&pID->minMwDMATransferCycleTimePerWord);
    endianSwap16Bit(&pID->RecommendedMwDMACycleTime);
    endianSwap16Bit(&pID->MinPIOTransferCycleTimeWithoutFlowControl);
    endianSwap16Bit(&pID->MinPIOTransferCycleTimeWithIORDY);
} 

-(void) assignRegisterAddresses: (IOTreeDevice *) deviceDescription
{
  
    if ( _controllerType == kControllerTypeCmd646X )
    {
        u_int32_t		pciRange1, pciRange2;

        [self mapMemoryRange: 0 to:(vm_address_t *)&pciRange1 findSpace:YES cache:IO_CacheOff];
        [self mapMemoryRange: 1 to:(vm_address_t *)&pciRange2 findSpace:YES cache:IO_CacheOff];

        _ideRegsAddrs.data      = pciRange1;    
        _ideRegsAddrs.error 	= pciRange1 + 0x1;
        _ideRegsAddrs.features 	= pciRange1 + 0x1;
        _ideRegsAddrs.sectCnt 	= pciRange1 + 0x2;
        _ideRegsAddrs.sectNum 	= pciRange1 + 0x3;
        _ideRegsAddrs.cylLow 	= pciRange1 + 0x4;
        _ideRegsAddrs.cylHigh 	= pciRange1 + 0x5;
        _ideRegsAddrs.drHead 	= pciRange1 + 0x6;
        _ideRegsAddrs.status 	= pciRange1 + 0x7;
        _ideRegsAddrs.command 	= pciRange1 + 0x7;

        _ideRegsAddrs.deviceControl = pciRange2 + 0x2;
        _ideRegsAddrs.altStatus     = pciRange2 + 0x2;

        [(IOPCIDevice *)deviceDescription configWriteLong:0x04 value:0x05];

    }
    else
    {
        IORange		*IORanges;

        IORanges = [deviceDescription memoryRangeList];

        _ideRegsAddrs.data 	= IORanges[0].start;
        _ideRegsAddrs.error 	= IORanges[0].start + 0x10;
        _ideRegsAddrs.features 	= IORanges[0].start + 0x10;
        _ideRegsAddrs.sectCnt 	= IORanges[0].start + 0x20;
        _ideRegsAddrs.sectNum 	= IORanges[0].start + 0x30;
        _ideRegsAddrs.cylLow 	= IORanges[0].start + 0x40;
        _ideRegsAddrs.cylHigh 	= IORanges[0].start + 0x50;
        _ideRegsAddrs.drHead 	= IORanges[0].start + 0x60;
        _ideRegsAddrs.status 	= IORanges[0].start + 0x70;
        _ideRegsAddrs.command 	= IORanges[0].start + 0x70;
    
        _ideRegsAddrs.deviceControl = IORanges[0].start + 0x160;
        _ideRegsAddrs.altStatus     = IORanges[0].start + 0x160;
 
        _ideRegsAddrs.channelConfig = IORanges[0].start + 0x200;

        _ideDMARegs		    = (IODBDMAChannelRegisters *)IORanges[1].start;
    }
}


@end

