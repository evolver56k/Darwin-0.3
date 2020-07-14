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

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/machine/directDevice.h>
#import <driverkit/generalFuncs.h>
#import <sys/types.h>
#import "IdeCntPublic.h"
#import "AtapiCntPublic.h"
#import <driverkit/IOPower.h>
#import <string.h>	// bzero
// #import <stdlib.h>	// strtol

/*
 * Disable mach messaging, but instead use thead sleep/wakeup.
 */
// #define NO_IRQ_MSG 1

typedef enum {
    IDE_PM_SLEEP = 0, IDE_PM_STANDBY, IDE_PM_IDLE, IDE_PM_ACTIVE
} idePowerState_t;

typedef enum {
    IDE_TRANSFER_PIO = 0,
	IDE_TRANSFER_SW_DMA,
	IDE_TRANSFER_MW_DMA,
	IDE_TRANSFER_ULTRA_DMA,
} ideTransferType_t;

typedef enum	{
    IDE_TRANSFER_16_BIT = 0, IDE_TRANSFER_32_BIT
} ideTransferWidth_t;

__private_extern__ unsigned int _ide_debug;

/*
 * Structure used to keep track of PRD table memory within the driver.
 */
typedef struct {
    void			*ptr;
    unsigned int	size;
	void			*ptrReal;
	unsigned int	sizeReal;
} memStruct_t;

/*
 * ATA transfer Modes.
 */
#define ATA_MODE_NONE	0x0
#define ATA_MODE_0		0x01
#define ATA_MODE_1		0x02
#define ATA_MODE_2		0x04
#define ATA_MODE_3		0x08
#define ATA_MODE_4		0x10
#define ATA_MODE_5		0x20

/*
 * Define type to encode a single mode, or a mask to represent a group
 * of allowable modes.
 */
typedef unsigned char 	ata_mode_t;
typedef ata_mode_t		ata_mask_t;

/*
 * ATA Transfer mode capability struct. This encodes all the transfer
 * modes that a controller or a drive can support.
 */
typedef union {
	struct {
		ata_mask_t pio;
		ata_mask_t swdma;
		ata_mask_t mwdma;
		ata_mask_t udma;
	} mode;
	struct {
		ata_mask_t mode[4];	// indexed by ideTransferType_t
	} array;
	unsigned int modes;
} txferModes_t;

/*
 * Information stored for a given drive.
 */
typedef struct {
	// For All IDE devices	
	BOOL				biosGeometry;	/* using BIOS? */
	ideDriveInfo_t		ideInfo;
	BOOL				ideIdentifyInfoSupported;
	ideIdentifyInfo_t	*ideIdentifyInfo;
	u_short				dmaChannel;
	u_short				multiSector;
	u_char				addressMode;	/* LBA or CHS */
	txferModes_t		driveModes;		/* supported modes */
	txferModes_t		driveMasks;		/* masks */
	ideTransferType_t	transferType;	/* selected type */
	ata_mode_t			transferMode;	/* selected mode */
	
	// ATAPI only
	BOOL				atapiDevice;
	BOOL				atapiCommandActive;
	u_char				atapiCmdLen;
	u_char				atapiCmdDrqType;
} driveInfo_t;

__private_extern__ unsigned char ata_mode_to_num(unsigned char mode);
__private_extern__ ata_mode_t ata_mask_to_mode(ata_mask_t mask);
__private_extern__ ata_mask_t ata_mode_to_mask(ata_mode_t mode);


@interface IdeController:IODirectDevice<IdeControllerPublic, 
				AtapiControllerPublic, IOPower>
{
@private
    id				_ideCmdLock;			/* NXLock */
    ideRegsAddrs_t	_ideRegsAddrs;
    port_t 			_ideDevicePort;
    port_t 			_ideInterruptPort;
    unsigned int	_interruptTimeOut;

    /*
     * This is used for communication between ideSendCommand and methods for 
     * individual commands. 
     */
    unsigned char	_driveNum;				/* target drive */   
    unsigned char	_controllerNum;			/* our number */
    
   	/*
	 * Information for the drives attached to this controller. 
	 */
	driveInfo_t		_drives[MAX_IDE_DRIVES];

	/*
	 * User settings.
	 */
	BOOL				_multiSectorRequested;			/* user option */

	/*
	 * IDE Controller information
	 */
    BOOL			_EIDESupport;		/* EIDE enabled? */
    BOOL			_IOCHRDYSupport;	/* Host side */
	txferModes_t 	_controllerModes;	/* Masks of transfer modes supported */

    /*
     * PCI related ivars. 
     */
    ideTransferWidth_t 	_transferWidth;		// 16 or 32 bits
	unsigned long		_controllerID;		// PCI IDE controller's ID
	unsigned short		_bmRegs;			// I/O mapped bus master registers
	memStruct_t			_prdTable;			// descriptor table memory
	unsigned int		_tablePhyAddr;		// table's physical address
	BOOL				_busMaster;			// YES for bus master controllers
	enum {
	 	PCI_CHANNEL_PRIMARY,				// 0x1f0, 14
		PCI_CHANNEL_SECONDARY,				// 0x170, 15
		PCI_CHANNEL_OTHER,
	} _ideChannel;

    /*
     * Power management related ivars. 
     */
    idePowerState_t	_drivePowerState; 	/* PM state for both drives */
    BOOL		_driveSleepRequest;

#ifdef NO_IRQ_MSG
	BOOL		interruptOccurred;
	int			waitQueue;
#endif NO_IRQ_MSG

#ifdef DEBUG    
    /* For testing */
    BOOL		_printWaitForNotBusy;
#endif DEBUG
}

/*
 * Exported methods.
 */
+ (BOOL)probe:(IODeviceDescription *)devDesc;

- (void)enableInterrupts;
- (void)disableInterrupts;

- (void)setInterruptTimeOut:(unsigned int)timeOut;
- (unsigned int)interruptTimeOut;

- (ide_return_t)ideWaitForInterrupt:(unsigned int)command
			   ideStatus:(unsigned char *)status;
- (void)clearInterrupts;

- free;

/*
 * Controller status checks. 
 */
- (ide_return_t)waitForNotBusy;
- (ide_return_t)waitForDeviceReady;
- (ide_return_t)waitForDataReady;
- (ide_return_t)waitForDeviceIdle;

- (ide_return_t)ataIdeReadGetInfoCommonWaitForDataReady;

/*
 * Miscellaneous methods.
 */
- (void)xferData:(caddr_t)addr read:(BOOL)read client:(struct vm_map *)client 
		length:(unsigned)length;

- (void)ideCntrlrLock;
- (void)ideCntrlrUnLock;

- (void)atapiCntrlrLock;
- (void)atapiCntrlrUnLock;

- (BOOL)isMultiSectorAllowed:(unsigned int)unit;
- (void)getIdeRegisters:(ideRegsVal_t *)rvp Print:(char *)printString;

- (BOOL) isDmaSupported:(unsigned int)unit;

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
#define MAX_DATA_READY_DELAY	(10 * 1000 * 1000)
 
#define CMOSADDR				0x70
#define CMOSDATA				0x71
#define HDTYPE					0x12

#define HD0_EXTTYPE				0x19
#define HD1_EXTTYPE				0x1a

#define CMOS_IDE_TABLE			0xfe401
#define SIZE_OF_IDE_TABLE		16

/*
 * This is actually quite a long time but it is mandated by the spec. 
 */
#define IDE_INTR_TIMEOUT		(30*1000)	// thirty seconds

#endif	_BSD_DEV_I386_IDECNT_H_

#endif	DRIVER_PRIVATE
