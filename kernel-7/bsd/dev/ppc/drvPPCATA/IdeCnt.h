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
 * IdeCnt.h - Interface for Ide Controller class. 
 *
 * HISTORY 
 *
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_I386_IDECNT_H_
#define _BSD_DEV_I386_IDECNT_H_

#define __APPLE_TYPES_DEFINED__ 1

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/IOTreeDevice.h>
#import <driverkit/ppc/IODBDMA.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/generalFuncs.h>
#import <sys/types.h>
#import "IdeCntPublic.h"
#import "AtapiCntPublic.h"
#import <driverkit/IOPower.h>


typedef enum	{
    IDE_PM_SLEEP = 0, IDE_PM_STANDBY, IDE_PM_IDLE, IDE_PM_ACTIVE
} idePowerState_t;

typedef enum	{
    IDE_TRANSFER_PIO = 0, IDE_TRANSFER_SW_DMA, IDE_TRANSFER_MW_DMA, IDE_TRANSFER_ULTRA_DMA,
} ideTransferType_t;

typedef enum	{
    IDE_TRANSFER_16_BIT = 0, IDE_TRANSFER_32_BIT
} ideTransferWidth_t;

typedef enum	{
    IDE_PIO_ACCESS_BASE		= 0,
    IDE_PIO_ACCESS_MIN 		= 4,
    IDE_PIO_RECOVERY_BASE 	= 4,
    IDE_PIO_RECOVERY_MIN	= 1,

    IDE_DMA_ACCESS_BASE 	= 0,
    IDE_DMA_ACCESS_MIN  	= 1,
    IDE_DMA_RECOVERY_BASE	= 1,
    IDE_DMA_RECOVERY_MIN	= 1,
} ideDMAConfigLimits_t;

typedef enum 	{
    IDE_DMA_NONE, IDE_DMA_SINGLEWORD, IDE_DMA_MULTIWORD, IDE_DMA_ULTRA
} ideDMATypes_t;

typedef struct _Cmd646xRegs
{
            u_int8_t    cntrlReg;
            u_int8_t	arttimReg;
            u_int8_t	cmdtimReg;
            u_int8_t	drwtimRegPIO;
            u_int8_t	drwtimRegDMA;
            u_int8_t	udidetcrReg;
} Cmd646xRegs_t;       


typedef struct
{
    BOOL		fChanged;
    BOOL		useDMA;
    int         	pioMode;
    int			pioAccessTime;
    int        		pioCycleTime;
    ideDMATypes_t	dmaType;
    int         	dmaMode;
    int         	dmaAccessTime;
    int         	dmaCycleTime;
    union 
    {
        u_int32_t	dbdmaConfig;
        Cmd646xRegs_t	cmd646XConfig;
    } ideConfig;
} ideCycleTimes_t;


@interface IdeController:IODirectDevice<IdeControllerPublic, 
				AtapiControllerPublic, IOPower>
{
@private
    id					_ideCmdLock;			/* NXLock */

    ideRegsAddrs_t			_ideRegsAddrs;

    volatile IODBDMAChannelRegisters	*_ideDMARegs;
    volatile IODBDMADescriptor		*_ideDMACommands;
    uint				_ideDMACommandsPhys;

    port_t 		_ideDevicePort;
    port_t 		_ideInterruptPort;
    unsigned int	_interruptTimeOut;

    unsigned int	_busNum;
    /*
     * From BIOS/CMOS. We use this only if the user asks for it. The ideInfo
     * structure is like the INT41 table. We fill this in even if we do not
     * use BIOS and get all our info from the drive using IDE_IDENTIFY_DRIVE. 
     */
    ideDriveInfo_t	_ideInfo[MAX_IDE_DRIVES];

    /*
     * This is used for communication between ideSendCommand and methods for 
     * individual commands. 
     */
    unsigned char	_driveNum;			/* target drive */
    
    unsigned char	_controllerNum;			/* our number */
    
    unsigned char	_controllerType;
   /*
    * Information from IDE_IDENTIFY_DRIVE commnad. 
    */
    BOOL		_ideIdentifyInfoSupported[MAX_IDE_DRIVES];
    ideIdentifyInfo_t   _ideIdentifyInfo[MAX_IDE_DRIVES];
    BOOL		_dmaSupported[MAX_IDE_DRIVES];
    unsigned char	_driveReset[MAX_IDE_DRIVES];

    BOOL		_multiSectorRequested;		/* user option */
    unsigned short	_multiSector[MAX_IDE_DRIVES];
    unsigned char	_addressMode[MAX_IDE_DRIVES];	/* LBA or CHS */
    ideCycleTimes_t     _cycleTimes[MAX_IDE_DRIVES];

    BOOL		_IOCHRDYSupport;		/* Host side */
    
    /*
     * ATAPI related ivars. 
     */
    BOOL		_atapiDevice[MAX_IDE_DRIVES];
    BOOL		_atapiCommandActive[MAX_IDE_DRIVES];
    unsigned char	_atapiCmdLen[MAX_IDE_DRIVES];
    unsigned char	_atapiCmdDrqType[MAX_IDE_DRIVES];

    /*
     * Power management related ivars. 
     */
    idePowerState_t	_drivePowerState; 	/* PM state for both drives */
    BOOL		_driveSleepRequest;
}

/*
 * Exported methods.
 */
+ (BOOL)probe:(IOTreeDevice *)devDesc;

- (void)enableInterrupts;
- (void)disableInterrupts;

- (void)setInterruptTimeOut:(unsigned int)timeOut;
- (unsigned int)interruptTimeOut;

- (ide_return_t)ideWaitForInterrupt:(unsigned int)command
			   ideStatus:(unsigned char *)status;
- (void)clearInterrupts;

/*
 * Controller status checks. 
 */
- (ide_return_t)waitForNotBusy;
- (ide_return_t)waitForDeviceReady;
- (ide_return_t)waitForDataReady;

- (ide_return_t)ataIdeReadGetInfoCommonWaitForDataReady;

/*
 * Miscellaneous methods.
 */

- (void) configWriteByte: (u_int32_t)reg value: (u_int8_t)cfgByte;
- (void) configReadByte: (u_int32_t)reg value: (u_int8_t *)cfgByte;


- (IOReturn) ideExecuteCmd:(ideIoReq_t *)ideIoReq ToDrive:(unsigned char)drive;

- (void)xferData:(caddr_t)addr read:(BOOL)read client:(struct vm_map *)client 
		length:(unsigned)length;

- (void)ideCntrlrLock;
- (void)ideCntrlrUnLock;

- (u_int) numberOfDrives;

- (void)atapiCntrlrLock;
- (void)atapiCntrlrUnLock;

- (BOOL)isMultiSectorAllowed:(unsigned int)unit;
- (unsigned int)getMultiSectorValue:(unsigned int)unit;
- (ideIdentifyInfo_t *) getIdeIdentifyInfo:(unsigned int)unit;
- (ideDriveInfo_t *) getIdeDriveInfo:(unsigned int)unit;
- (void)getIdeRegisters:(ideRegsVal_t *)rvp Print:(char *)printString;
- (BOOL) isDmaSupported:(unsigned int)unit;

-(void) setTransferRate: (int) unit UseDMA:(BOOL)fUseDMA;
-(void) setTransferMode:(int) unit;

/*
 * Number of sectors that can be transferred via READ_MULTIPLE or
 * WRITE_MULTIPLE command. 
 */
- (unsigned int)getMultiSectorValue:(unsigned int)unit;

/*
 * ATAPI related methods. 
 */
- (unsigned int)numDevices;			/* max devices on ATA bus */
- (BOOL)isAtapiDevice:(unsigned char)unit;
- (BOOL)isAtapiCommandActive:(unsigned char)unit;
- (void)setAtapiCommandActive:(BOOL)state forUnit:(unsigned char)unit;

/*
 * Power management methods. 
 */
- (idePowerState_t)drivePowerState;
- (void)setDrivePowerState:(idePowerState_t)state;
- (void)putDriveToSleep;
- (void)wakeUpDrive;
- (BOOL)spinUpDrive:(unsigned int)unit;
- (BOOL)startUpAttachedDevices;

/*
 * The PM state is per controller, not per device. 
 */
- (IOReturn) getPowerState:(PMPowerState *)state_p;
- (IOReturn) setPowerState:(PMPowerState)state;
- (IOReturn) getPowerManagement:(PMPowerManagementState *)state_p;
- (IOReturn) setPowerManagement:(PMPowerManagementState)state;

@end

#define MAX_BUSY_DELAY			(10 * 1000 * 1000)	// 10 seconds
#define MAX_DATA_READY_DELAY		(10 * 1000 * 1000)
 
#define MEDIA_BAY_ID_MASK		0x0000ff00
#define MEDIA_BAY_ID_CDROM		0x00003000

/*
 * This is actually quite a long time but it is mandated by the spec. 
 */
#define IDE_INTR_TIMEOUT		(30*1000)	// thirty seconds

#endif	_BSD_DEV_I386_IDECNT_H_

#endif	DRIVER_PRIVATE

