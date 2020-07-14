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
 * Copyright (c) 1991-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCnt.m - IDE Controller class. 
 *
 */

#import <sys/systm.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import "io_inline.h" 
#import "IdeDDM.h"

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdeCntDma.h"

#import "IdeCntInline.h"

#import "Cmd646xRegs.h"

static  int	hcUnitNum = 0;


@implementation IdeController

/*
 * For PCI ATA the driver is running attached to a different node
 * than OpenFirmware - patch up the path to match.
 */
#define PCIATASUBNODE	"ata-4@0"

- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    if( [super getDevicePath:path maxLength:maxLen  useAlias:doAlias]) {

	if( _controllerType == kControllerTypeCmd646X) {

            int	len = maxLen - strlen( path );

	    len -= strlen( PCIATASUBNODE );
            if( len < 0)
                return( nil);
            strcat( path, "/" PCIATASUBNODE );

            if( doAlias)
                IOAliasPath( path);
	}
        return( self);
    }
    return( nil);
}

- (char *) matchDevicePath:(char *)matchPath
{
    char    *	tail;
    char    *	skip;

    tail = [super matchDevicePath:matchPath];
    if( tail && (_controllerType == kControllerTypeCmd646X)) {

	/* skip the extra path component */
	if( (0 == strncmp( tail + 1, PCIATASUBNODE, strlen( PCIATASUBNODE )))
	&&  (skip = strchr( tail + 1, '/')) )
	    tail = skip;
    }
    return( tail);
}

/*
 * Create and initialize one instance of IDE Controller. 
 */

void call_kdp(void);

+ (BOOL)probe:(IOTreeDevice *)deviceDescription
{
    IdeController 	*idec;
    char		devName[20];
    int			numInts, numRange;
    int			maxInts, maxRange;

//    call_kdp();

    idec = [[self alloc] initFromDeviceDescription:deviceDescription];

    if ( strcmp([deviceDescription nodeName], "pci-ata") == 0 )
    {
        idec->_controllerType = kControllerTypeCmd646X;
    }

    numRange = [deviceDescription numMemoryRanges];
    numInts  = [deviceDescription numInterrupts];

    if ( idec->_controllerType == kControllerTypeCmd646X )
    {
        maxRange  = 5;
        maxInts   = 1;
    }
    else
    {
        maxRange = 2;
        maxInts  = 2;
    }

    if ( numRange != maxRange ) 
    {
    	IOLog("Disk(ata): Invalid port ranges: %d.\n", numRange);
	[idec free];
	return NO;
    }
    
    if (numInts != maxInts) 
    {
	IOLog("Disk(ata): Invalid number of interrupts: %d.\n", numInts);
	[idec free];
	return NO;
    }
     
    if ( [idec checkMediaBayOption: deviceDescription] == NO )
    {
        IOLog("Disk(ata): Non-ATA/ATAPI Media Bay option detected\n\r");
        [idec free];
	return NO;
    }

    /*
     * Proceed with initialization. 
     */
    [idec setUnit:hcUnitNum];
    sprintf(devName, "hc%d", hcUnitNum);
    [idec setName:devName];
    [idec setDeviceKind:"IdeController"];
    
    if ([idec ideControllerInit:deviceDescription] != YES) 
    {
	[idec free];
	return NO;
    }

    if ([idec registerDevice] == nil)	
    {
	IOLog("Disk(ata): %s: Failed to register device.\n", [self name]);
	[idec free];
	return NO;
    }

    hcUnitNum++;

    return YES;
}

/*
 * Wait for interrupt or timeout. Returns IDER_SUCCESS or IDER_TIMEOUT. 
 */
- (ide_return_t)ideWaitForInterrupt:(unsigned int)command
			  ideStatus:(unsigned char *)status
{
    msg_return_t 	result;
    msg_header_t 	msg;
    u_int8_t 		cfgByte;
    
    msg.msg_local_port = _ideInterruptPort;
    msg.msg_size = sizeof(msg);

    result = msg_receive(&msg, RCV_TIMEOUT, _interruptTimeOut);
    
    if (result == RCV_SUCCESS || result == RCV_TOO_LARGE) 
    {
	if (status != NULL)
	    *status = inb(_ideRegsAddrs.status);	/* acknowledge */
	else
	    inb(_ideRegsAddrs.status);
        
        if ( _controllerType == kControllerTypeCmd646X )
        {
            u_int32_t		intReg;

            intReg =  (_busNum == 0) ? kCmd646xCFR : kCmd646xARTTIM23;
            [self configReadByte:  intReg value: &cfgByte];
            [self configWriteByte: intReg value: cfgByte];
            [self enableInterrupt:0];

#if 0
            [self configReadByte:  intReg value: &cfgByte];
            IOLog( "Int Mask = %08x Int Levels = %08x Int Events = %08x CmdInt = %02x\n\r",
                   *(volatile u_int32_t *)0x80800024, *(volatile u_int32_t *)0x8080002c,
		   *(volatile u_int32_t *)0x80800020, cfgByte );
#endif
        }

	return IDER_SUCCESS;
    }
    
    if (result == RCV_TIMED_OUT)
    {
	IOLog("Disk(ata): %s: interrupt timeout, cmd: 0x%0x\n", [self name], command);
    }
    else
	IOLog("Disk(ata): %s: Error %d in receiving interrupt, cmd: 0x%0x\n", 
	    [self name], result, command);

    return IDER_CMD_ERROR;
}

/*
 * Remove any interrupts messages that have queued up. This will get rid of
 * any spurious or duplicate interrupts. 
 */
- (void)clearInterrupts
{
    msg_return_t result;
    msg_header_t msg;
    int count;

    count = 0;

    while (1)	{

	msg.msg_local_port = _ideInterruptPort;
	msg.msg_size = sizeof(msg);
	
	result = msg_receive(&msg, RCV_TIMEOUT, 0);
	
	if (result != RCV_SUCCESS) 
        {
	    return;
	}
	count += 1;
    }
}

- (ide_return_t)waitForNotBusy
{
    int     delay = MAX_BUSY_DELAY;	/* microseconds */
    unsigned char status;

    delay -= 2;			/* Or else will block on second try */
    while (delay > 0) {
	status = inb(_ideRegsAddrs.status);
	if (!(status & BUSY)) {
	    return IDER_SUCCESS;
	}
	if (delay % 1000)	{
	    IODelay(2);
            delay -= 2;
	} else {
	    IOSleep(1);
	    delay -= 1000;
	}	
    }

    return IDER_TIMEOUT;
}

- (ide_return_t)waitForDeviceReady
{
    int     delay = MAX_BUSY_DELAY;
    unsigned char status;

    delay -= 2;
    while (delay > 0) {
	status = inb(_ideRegsAddrs.status);
	if ((!(status & BUSY)) && (status & READY)) {
	    return IDER_SUCCESS;
	}
	if (delay % 1000)	{
	    IODelay(2);
            delay -= 2;
	} else {
	    IOSleep(1);
	    delay -= 1000;
	}	
    }
    return IDER_TIMEOUT;
}

- (ide_return_t)waitForDataReady
{
    int     delay = MAX_DATA_READY_DELAY;
    unsigned char status;

    delay -= 2;
    while (delay > 0) {
	status = inb(_ideRegsAddrs.altStatus);
	if ((!(status & BUSY)) && (status & DREQUEST))
	    return (IDER_SUCCESS);
	if (delay % 1000) 	{
	    IODelay(2);
            delay -= 2;
	} else {
	    IOSleep(1);
	    delay -= 1000;
	}
    }
    
    return IDER_TIMEOUT;
}


/*
 * This is a private version for ideReadGetInfoCommon. It has a short delay
 * time. This is the same situation as atapiIdentifyDeviceWaitForDataReady in
 * AtapiCntCmds.m. The problem in both cases is that some devices pass
 * identification checks even when they should not. Now the first command
 * issued after can really confirm that but we don't want to wait too long. 
 */
- (ide_return_t)ataIdeReadGetInfoCommonWaitForDataReady
{
    int     delay = MAX_DATA_READY_DELAY/25;
    unsigned char status;

    delay -= 2;
    while (delay > 0) {
	status = inb(_ideRegsAddrs.altStatus);
	if ((!(status & BUSY)) && (status & DREQUEST))
	    return (IDER_SUCCESS);
	if (delay % 1000) 	{
	    IODelay(2);
            delay -= 2;
	} else {
	    IOSleep(1);
	    delay -= 1000;
	}
    }
    
    return IDER_TIMEOUT;
}


- (void)enableInterrupts
{
    outb(_ideRegsAddrs.deviceControl, DISK_INTERRUPT_ENABLE);
    [self enableInterrupt:0];
}

- (void)disableInterrupts
{
    [self disableInterrupt: 0];
    outb(_ideRegsAddrs.deviceControl, DISK_INTERRUPT_DISABLE);
}

- (void)setInterruptTimeOut:(unsigned int)timeOut
{
    _interruptTimeOut = timeOut;
}

- (unsigned int)interruptTimeOut
{
    return _interruptTimeOut;
}

- (IOReturn) ideExecuteCmd:(ideIoReq_t *)ideIoReq ToDrive:(unsigned char)drive
{
    return [self _ideExecuteCmd:ideIoReq ToDrive:drive];
}


/*
 * Return task file values, optionally print them if given a non-null string. 
 */
- (void)getIdeRegisters:(ideRegsVal_t *)rvp Print:(char *)printString
{
    ideRegsVal_t	ideRegs;
    
    ideRegs.error = inb(_ideRegsAddrs.error);
    ideRegs.sectCnt = inb(_ideRegsAddrs.sectCnt);
    ideRegs.sectNum = inb(_ideRegsAddrs.sectNum);
    ideRegs.cylLow = inb(_ideRegsAddrs.cylLow);
    ideRegs.cylHigh = inb(_ideRegsAddrs.cylHigh);
    ideRegs.drHead = inb(_ideRegsAddrs.drHead);
    ideRegs.status = inb(_ideRegsAddrs.altStatus);	/* don't ack */
    
    if (rvp != NULL)
	*rvp = ideRegs;
    
    if (printString != NULL)	{
	IOLog("Disk(ata): %s: %s: error=0x%x secCnt=0x%x "
		"secNum=0x%x cyl=0x%x drhd=0x%x status=0x%x\n", 
		[self name], printString,
		ideRegs.error, ideRegs.sectCnt, ideRegs.sectNum,
		((ideRegs.cylHigh << 8) | ideRegs.cylLow),
		ideRegs.drHead, ideRegs.status);
    }
}

/*
 * Read maximum of one page from the sector buffer to "addr", write maximum
 * of one page from "addr" into the sector buffer. Note that (length <=
 * PAGE_SIZE). 
 */
- (void)xferData:(caddr_t)addr read:(BOOL)read client:(struct vm_map *)client
		length:(unsigned)length
{
    ideXferData(addr, read, client, length, _ideRegsAddrs );
}

- (BOOL)isMultiSectorAllowed:(unsigned int)unit
{
    return (_multiSector[unit] != 0);
}

-(unsigned int)getMultiSectorValue:(unsigned int)unit
{
    return _ideIdentifyInfoSupported[unit] ? _multiSector[unit] : 0;
}

- (ideIdentifyInfo_t *) getIdeIdentifyInfo:(unsigned int)unit
{
    return _ideIdentifyInfoSupported[unit] ?
		&_ideIdentifyInfo[unit] : NULL;
}

- (ideDriveInfo_t *) getIdeDriveInfo:(unsigned int)unit
{
    return (&_ideInfo[unit]);
}

- (BOOL) isDmaSupported:(unsigned int)unit
{
    return (_dmaSupported[unit] == YES);
}

- (u_int) getControllerType
{
    return _controllerType;
}

/*
 * Set the IDE Channel transfer rate to indicated drive
 */
-(void) setTransferRate: (int) unit UseDMA: (BOOL) fUseDMA
{
    Cmd646xRegs_t       	*cfgRegs;
    u_int8_t			cfgByte;
    u_int8_t                    drwtimReg;
    ideDMATypes_t		dmaType;	

    if ( _cycleTimes[unit].fChanged == NO && _cycleTimes[unit].useDMA == fUseDMA )
    {
        return;
    }
    _cycleTimes[unit].fChanged = NO;
    _cycleTimes[unit].useDMA   = fUseDMA;

    if ( _controllerType == kControllerTypeCmd646X )
    {
        /*
         *
         */
        cfgRegs = &_cycleTimes[unit].ideConfig.cmd646XConfig;

        if ( _busNum == 0 )
        {
            if ( unit == 0 )
            {
                [self configReadByte: kCmd646xCNTRL value:&cfgByte];
                cfgByte &= ~kCmd646xCNTRL_Drive0ReadAhead;
                cfgByte |= cfgRegs->cntrlReg;
                [self configWriteByte: kCmd646xCNTRL value:cfgByte];

                [self configWriteByte: kCmd646xCMDTIM  value:cfgRegs->cmdtimReg];

                [self configWriteByte: kCmd646xARTTIM0 value:cfgRegs->arttimReg];

                dmaType = _cycleTimes[unit].dmaType;
                if ( _cycleTimes[unit].useDMA && ((dmaType != IDE_DMA_NONE) || (dmaType != IDE_DMA_ULTRA)) )
                {
                    drwtimReg = cfgRegs->drwtimRegDMA;
                }
                else
                {
                    drwtimReg = cfgRegs->drwtimRegPIO;
                }
                [self configWriteByte: kCmd646xDRWTIM0 value:drwtimReg];             

                [self configReadByte: kCmd646xUDIDETCR0 value:&cfgByte];
                cfgByte &= ~(kCmd646xUDIDETCR0_Drive0UDMACycleTime | kCmd646xUDIDETCR0_Drive0UDMAEnable);
                cfgByte |= cfgRegs->udidetcrReg;
                [self configWriteByte: kCmd646xUDIDETCR0 value:cfgByte];
            }        
            else
            {
                [self configReadByte: kCmd646xCNTRL value:&cfgByte];
                cfgByte &= ~kCmd646xCNTRL_Drive1ReadAhead;
                cfgByte |= cfgRegs->cntrlReg;
                [self configWriteByte: kCmd646xCNTRL value:cfgByte];

                [self configWriteByte: kCmd646xARTTIM1 value:cfgRegs->arttimReg];

                dmaType = _cycleTimes[unit].dmaType;
                if ( _cycleTimes[unit].useDMA && ((dmaType != IDE_DMA_NONE) || (dmaType != IDE_DMA_ULTRA)) )
                {
                    drwtimReg = cfgRegs->drwtimRegDMA;
                }
                else
                {
                    drwtimReg = cfgRegs->drwtimRegPIO;
                }
                [self configWriteByte: kCmd646xDRWTIM1 value:drwtimReg];

                [self configReadByte: kCmd646xUDIDETCR0 value:&cfgByte];
                cfgByte &= ~(kCmd646xUDIDETCR0_Drive1UDMACycleTime | kCmd646xUDIDETCR0_Drive1UDMAEnable);
                cfgByte |= cfgRegs->udidetcrReg;
                [self configWriteByte: kCmd646xUDIDETCR0 value:cfgByte];
            }
        }
        else
        {
            if ( unit == 0 )
            {
                [self configReadByte: kCmd646xARTTIM23 value:&cfgByte];
                cfgByte &= ~(kCmd646xARTTIM23_Drive2ReadAhead | kCmd646xARTTIM23_AddrSetup);
                cfgByte |= (cfgRegs->cntrlReg >> 4) | cfgRegs->arttimReg;
                [self configWriteByte: kCmd646xARTTIM23 value:cfgByte];

                [self configWriteByte: kCmd646xCMDTIM  value:cfgRegs->cmdtimReg];

                dmaType = _cycleTimes[unit].dmaType;
                if ( _cycleTimes[unit].useDMA && ((dmaType != IDE_DMA_NONE) || (dmaType != IDE_DMA_ULTRA)) )
                {
                    drwtimReg = cfgRegs->drwtimRegDMA;
                }
                else
                {
                    drwtimReg = cfgRegs->drwtimRegPIO;
                }
                [self configWriteByte: kCmd646xDRWTIM2 value:drwtimReg];

                [self configReadByte: kCmd646xUDIDETCR1 value:&cfgByte];
                cfgByte &= ~(kCmd646xUDIDETCR1_Drive2UDMACycleTime | kCmd646xUDIDETCR1_Drive2UDMAEnable);
                cfgByte |= cfgRegs->udidetcrReg;
                [self configWriteByte: kCmd646xUDIDETCR1 value:cfgByte];
            }        
            else
            {
                [self configReadByte: kCmd646xARTTIM23 value:&cfgByte];
                cfgByte &= ~(kCmd646xARTTIM23_Drive3ReadAhead | kCmd646xARTTIM23_AddrSetup);
                cfgByte |= (cfgRegs->cntrlReg >> 4) | cfgRegs->arttimReg;
                [self configWriteByte: kCmd646xARTTIM23 value:cfgByte];

                dmaType = _cycleTimes[unit].dmaType;
                if ( _cycleTimes[unit].useDMA && ((dmaType != IDE_DMA_NONE) || (dmaType != IDE_DMA_ULTRA)) )
                {
                    drwtimReg = cfgRegs->drwtimRegDMA;
                }
                else
                {
                    drwtimReg = cfgRegs->drwtimRegPIO;
                }
                [self configWriteByte: kCmd646xDRWTIM3 value:drwtimReg];

                [self configReadByte: kCmd646xUDIDETCR1 value:&cfgByte];
                cfgByte &= ~(kCmd646xUDIDETCR1_Drive3UDMACycleTime | kCmd646xUDIDETCR1_Drive3UDMAEnable);
                cfgByte |= cfgRegs->udidetcrReg;
                [self configWriteByte: kCmd646xUDIDETCR1 value:cfgByte];
            }

        }
        return;
    }
    else
    {
        *(volatile uint *)_ideRegsAddrs.channelConfig = _cycleTimes[unit].ideConfig.dbdmaConfig;
        eieio();
        _cycleTimes[unit ^ 1].fChanged = YES;
    }
}

/*
 * Set the data transfer rate for the specified drive.
 */
-(void) setTransferMode:(int) unit
{
    int		value;

    value = IDE_FEATURE_MODE_PIO | _cycleTimes[unit].pioMode;
    [self ideSetDriveFeature:FEATURE_SET_TRANSFER_MODE value: value];

    if ( _cycleTimes[unit].dmaType != IDE_DMA_NONE )
    {
        value  = _cycleTimes[unit].dmaMode;
        switch ( _cycleTimes[unit].dmaType )
        {
            case IDE_DMA_NONE:
                return;
            case IDE_DMA_SINGLEWORD:
                value |= IDE_FEATURE_MODE_SWDMA;
                break;
            case IDE_DMA_MULTIWORD:
                value |= IDE_FEATURE_MODE_MWDMA;
                break;
            case IDE_DMA_ULTRA:
                value |= IDE_FEATURE_MODE_ULTRADMA;
                break;
        }
        [self ideSetDriveFeature:FEATURE_SET_TRANSFER_MODE value: value];
    }
}

- (void)ideCntrlrLock
{
    [_ideCmdLock lock];
}

- (void)ideCntrlrUnLock
{
    [_ideCmdLock unlock];
}

- (u_int) numberOfDrives
{
    return (MAX_IDE_DRIVES);
}


/*
 * There is no need for separate ATAPI locks but we keep them for testing
 * purposes. 
 */
- (void)atapiCntrlrLock
{
    [_ideCmdLock lock];
}

- (void)atapiCntrlrUnLock
{
    [_ideCmdLock unlock];
}

/*
 * The ATAPI driver (CD-ROM or Tape) will scan thorugh this list of devices
 * looking for ATAPI hardware. 
 */
- (unsigned int)numDevices
{
    return MAX_IDE_DRIVES;
}

- (BOOL)isAtapiDevice:(unsigned char)unit
{
    if (unit >= MAX_IDE_DRIVES)
    	return NO;
	
    return _atapiDevice[unit];
}

- (BOOL)isAtapiCommandActive:(unsigned char)unit
{
    return _atapiCommandActive[unit];
}

- (void)setAtapiCommandActive:(BOOL)state forUnit:(unsigned char)unit
{
    _atapiCommandActive[unit] = state; 
}

/*
 * Power management. 
 */
- (idePowerState_t)drivePowerState
{
    return _drivePowerState;
}

- (void)setDrivePowerState:(idePowerState_t)state
{
    _drivePowerState = state;
}

- (void)putDriveToSleep
{
    if ([self drivePowerState] == IDE_PM_SLEEP)
    	return;
	
    _driveSleepRequest = YES;
    ddm_ide_lock("putDriveToSleep: acquiring lock, suspend\n",1,2,3,4,5);
    [self ideCntrlrLock];
    ddm_ide_lock("putDriveToSleep: acquired lock, suspend\n",1,2,3,4,5);
#ifdef DEBUG
    IOLog("Disk(ata): %s: Entering suspend mode.\n", [self name]);
#endif DEBUG
    [self setDrivePowerState:IDE_PM_SLEEP];
}

/*
 * The method wakeUpDrive simply sets a flag. The drive is actually woken up
 * the next time we need it. This is done by a call from
 * ideExecuteCmd:ToDrive: method to the spinUpDrive:unit: method below. 
 */
- (void)wakeUpDrive
{
    if ([self drivePowerState] == IDE_PM_ACTIVE)
    	return;
	
    _driveSleepRequest = NO;
    ddm_ide_lock("wakeUpDrive: Exiting suspend mode.\n", 1,2,3,4,5);
    [self setDrivePowerState:IDE_PM_STANDBY];
    ddm_ide_lock("wakeUpDrive: releasing lock, recovered\n",1,2,3,4,5);    
    [self ideCntrlrUnLock];
}

/*
 * This will get the drive to spin up. We simply try to read a single sector
 * and are prepared to camp out for a very long time (>>30 secs). 
 */
#define MAX_MEDIA_ACCESS_TRIES		30

- (BOOL)spinUpDrive:(unsigned int)unit
{
    unsigned char status;
    int i, j;
    unsigned char dh;

    dh = _addressMode[unit] | (unit ? SEL_DRIVE1 : SEL_DRIVE0);

    for (i = 0; i < MAX_MEDIA_ACCESS_TRIES; i++)	{
    
	[self ideReset];
	[self disableInterrupts];
    
       /*
	* Read one sector at (0,0,1). (LBA == 1).
	*/
	outb(_ideRegsAddrs.drHead, dh);
	outb(_ideRegsAddrs.sectNum, 0x1);
	outb(_ideRegsAddrs.sectCnt, 0x1);
	outb(_ideRegsAddrs.cylLow, 0x0);
	outb(_ideRegsAddrs.cylHigh, 0x0);
	outb(_ideRegsAddrs.command, IDE_READ);   

	/*
	 * Wait for about ten seconds for response from drive. 
	 */
	for (j = 0; j < 5; j++)	{
            IOSleep(2000);
	    status = inb(_ideRegsAddrs.status);
	    if ((!(status & BUSY)) && (status & DREQUEST))	{
		[self ideReset];	/* FIXME: may not be needed */
		IOLog("Disk(ata): %s: Recovered from suspend mode.\n", [self name]);
		return YES;
	    }
	}
	IOLog("Disk(ata): %s: Retrying to recover from suspend mode.\n", [self name]);
    }

    IOLog("Disk(ata): %s: Failed to recover from suspend mode.\n", [self name]);
    return NO;
}

/*
 * We need to get the ATA drives spinning before we can try to reset them.
 * The situation gets complicated if the ATA and ATAPI device share the same
 * cable. We basically start up all devices on this controller. 
 */
- (BOOL)startUpAttachedDevices
{
    int i;
    BOOL status = YES;
    
    /* Spin up only ATA drives */
    for (i = 0; i < MAX_IDE_DRIVES; i++)	{
	if ((_ideInfo[i].type != 0) && ([self isAtapiDevice:i] == NO)){
	    status = [self spinUpDrive:i];
	}
    }
    
    [self resetAndInit];
    [self setDrivePowerState:IDE_PM_ACTIVE];
    
    return status;
}

- (IOReturn) getPowerState:(PMPowerState *)state_p
{
    return IO_R_UNSUPPORTED;
}

- (IOReturn) setPowerState:(PMPowerState)state
{
    if (state == PM_READY) {
	[self wakeUpDrive];
    } else if (state == PM_SUSPENDED)	{
	[self putDriveToSleep];
    } else {
        //IOLog("Disk(ata): %s: unknown APM event %x\n", [self name], (unsigned int)state);
    }
    
    return IO_R_SUCCESS;
}

- (IOReturn) getPowerManagement:(PMPowerManagementState *)state_p
{
    return IO_R_UNSUPPORTED;
}

- (IOReturn) setPowerManagement:(PMPowerManagementState)state
{
    return IO_R_UNSUPPORTED;
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassIDEController);
    return( self);
}

- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    strcat( types, " "IOTypeIDE);
    return( self);
}


- (void) configWriteByte: (u_int32_t)reg value: (u_int8_t)cfgByte
{
    union
    {
        unsigned long	word;
        unsigned char   byte[4];
    } cfgReg;

    u_int32_t		regWord;

    regWord = reg & ~0x03;

    [[self deviceDescription] configReadLong: regWord value: &cfgReg.word];
    cfgReg.word = EndianSwap32(cfgReg.word);
    switch (regWord)
    {
        case kCmd646xCFR:
            cfgReg.byte[kCmd646xCFR & 0x03] &= ~kCmd646xCFR_IDEIntPRI;
            break;
        case kCmd646xDRWTIM0:
            cfgReg.byte[kCmd646xARTTIM23 & 0x03] &= ~kCmd646xARTTIM23_IDEIntSDY;
            break;
        case kCmd646xBMIDECR0:
            cfgReg.byte[kCmd646xMRDMODE & 0x03 ] &= ~(kCmd646xMRDMODE_IDEIntPRI  | kCmd646xMRDMODE_IDEIntSDY);
            cfgReg.byte[kCmd646xBMIDESR0 & 0x03] &= ~(kCmd646xBMIDESR0_DMAIntPRI | kCmd646xBMIDESR0_DMAErrorPRI);
            break;
        case kCmd646xBMIDECR1:
            cfgReg.byte[kCmd646xBMIDESR1 & 0x03] &= ~(kCmd646xBMIDESR1_DMAIntSDY | kCmd646xBMIDESR1_DMAErrorSDY);
            break;
    }        
    cfgReg.byte[reg & 0x03] = cfgByte;

    cfgReg.word = EndianSwap32(cfgReg.word); 
    [[self deviceDescription] configWriteLong: regWord value: cfgReg.word];
}      
  

- (void) configReadByte: (u_int32_t)reg value: (u_int8_t *)cfgByte
{
    union
    {
        unsigned long	word;
        unsigned char   byte[4];
    } cfgReg;

    [[self deviceDescription] configReadLong: reg value:&cfgReg.word];
    *cfgByte = cfgReg.byte[3 - (reg & 0x03)];
    return;
}

@end
