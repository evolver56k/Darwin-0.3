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
 * IdeCnt.m - IDE Controller class. 
 *
 * HISTORY 
 *
 * 1-Feb-1998	Joe Liu at Apple
 *	Created an initFromDeviceDescription method to do things that used to be
 *	done in the probe method. This allows us to call [super init...] after
 *	we have finished munging with our resources.
 *
 * 04-Sep-1996	 Becky Divinski at NeXT
 *	Added code in probe method to handle Dual EIDE personality case.
 * 
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */

#if 0
#define KERNEL		1
#define KERNEL_PRIVATE	1
#define ARCH_PRIVATE	1 
#undef	MACH_USER_API
#endif

#import "IdeCnt.h"
#import "IdeCntInit.h"
#import "IdeCntCmds.h"
#import "IdePIIX.h"
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/i386/IOPCIDirectDevice.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#if (IO_DRIVERKIT_VERSION != 330)
#import <machdep/machine/pmap.h>        // XXX get rid of this
#endif
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <driverkit/align.h>
#import <machkit/NXLock.h>
#import <machdep/i386/io_inline.h>
#import <kernserv/i386/spl.h>
#ifdef NO_IRQ_MSG
#import <kern/sched_prim.h>
#endif NO_IRQ_MSG
#import <kern/lock.h>
#import "IdeDDM.h"
#import <bsd/stdio.h>

// XXX get rid of this
#if (IO_DRIVERKIT_VERSION != 330)
#import <machdep/machine/pmap.h>
#endif
#import "IdeCntInline.h"
#import "DualEide.h"

/*
 * If set to non-zero some debugging messages will be prinetd in case of
 * failure. 
 */
__private_extern__ unsigned int _ide_debug = 0;

/*
 * Config table keys that determine our configuration. 
 */
#define CONTROLLER_INSTANCE 	"Instance"
#define ATA_DEBUG 				"Debug"

#ifdef NO_IRQ_MSG
#warning Interrupt messaging OFF
typedef struct _ideStruct {
    @defs(IdeController)
} ideStruct_t;
#endif NO_IRQ_MSG

@implementation IdeController

/*
 * Create and initialize one instance of IDE Controller. 
 */

+ (BOOL)probe:(IODeviceDescription *)devDesc
{
    IdeController 	*idec;

	idec = [self alloc];
    if (idec == nil) {
		IOLog("%s: Failed to alloc instance\n", [self name]);
    	return NO;
    }
	
	return ([idec initFromDeviceDescription:devDesc] != nil);
}

- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    char			devName[20];
    const char  	*ctlType, *param;
    int     		n, hcUnitNum;
	
    IOEISADeviceDescription *deviceDescription = 
    			(IOEISADeviceDescription *)devDesc;

    if ((n = [deviceDescription numPortRanges]) != 1) {
		IOLog("ATA: Invalid port ranges: %d.\n", n);
		[self free];
		return nil;
	}
    
    if ((n = [deviceDescription numInterrupts]) != 1) {
		IOLog("ATA: Invalid number of interrupts: %d.\n", n);
		[self free];
		return nil;
	}

    ctlType = [[deviceDescription configTable] 
    			valueForStringKey: CONTROLLER_INSTANCE];

    hcUnitNum = (unsigned char) ctlType[0] - '0';

	// Hack for Dual EIDE case

	if (dualEide)
	{
		if (instanceNum > 0)
			hcUnitNum = instanceNum;
	}
    
    if (hcUnitNum > MAX_IDE_CONTROLLERS-1) {
		IOLog("ATA: Controller %d not supported.\n", hcUnitNum);
		[self free];
		return nil;
    }
    
    param = [[deviceDescription configTable] 
    			valueForStringKey: ATA_DEBUG];
    if ((param != NULL) && (strcmp(param, "Yes") == 0))	{
    	_ide_debug = 1;
		IOLog("ATA/ATAPI: Debug mode enabled.\n");
    } else {
    	_ide_debug = 0;
    }

    /*
     * Proceed with initialization. 
     */
    [self setUnit:hcUnitNum];
    sprintf(devName, "hc%d", hcUnitNum);
    [self setName:devName];
    [self setDeviceKind:"IdeController"];

	/*
	 * Allocate storage for the ideIdentifyInfo_t structures.
	 */
	for (n = 0; n < MAX_IDE_DRIVES; n++) {
		_drives[n].ideIdentifyInfo =
			(ideIdentifyInfo_t *)IOMalloc(sizeof(ideIdentifyInfo_t));
		if (_drives[n].ideIdentifyInfo == NULL) {
			[self free];
			return nil;
		}
	}

	/*
	 * This must be called before [super init...] so that we can append
	 * to the portRangeList before it is registered.
	 */
	if ([self probePCIController:(IOPCIDeviceDescription *)devDesc] != YES) {
		[self free];
		return nil;
    }
	
    if ([super initFromDeviceDescription:deviceDescription] == nil) {
		IOLog("%s: initFromDeviceDescription failed.\n", [self name]);
		[self free];
		return nil;
    }

    if ([self ideControllerInit:devDesc] != YES) {
		[self free];
		return nil;
    }

    if ([self registerDevice] == nil)	{
		IOLog("%s: Failed to register device.\n", [self name]);
		[self free];
		return nil;
    }
	
    return self;
}

static int logInterrupts = 0;
static unsigned short lastCommand = 0;

/*
 * Wait for interrupt or timeout. Returns IDER_SUCCESS or IDER_TIMEOUT. 
 */
- (ide_return_t)ideWaitForInterrupt:(unsigned int)command
			  ideStatus:(unsigned char *)status
{
#ifdef NO_IRQ_MSG
	ide_return_t	ret;
	u_int 			s;
	BOOL			_interruptOccurred;

	s = spldevice();

	assert_wait((void *)&waitQueue, TRUE);
	thread_set_timeout(HZ * _interruptTimeOut / 1000);	// value in ticks
	thread_block_with_continuation((void (*)()) 0);

	_interruptOccurred = interruptOccurred;

	splx(s);
	
	if (_interruptOccurred == YES)  {
		if (status != NULL)
			*status = inb(_ideRegsAddrs.status);	/* acknowledge */
		else
			inb(_ideRegsAddrs.status);

        logInterrupts -= 1;
        lastCommand = command;

		ret = IDER_SUCCESS;
    }
    else {
		IOLog("%s: interrupt timeout, cmd: 0x%0x\n", [self name], command);
		ret = IDER_CMD_ERROR;
	}

    return (ret);

#else  NO_IRQ_MSG

    msg_return_t result;
    msg_header_t msg;
    
	msg.msg_local_port = _ideInterruptPort;
    msg.msg_size = sizeof(msg);

#ifdef undef
        if (logInterrupts > 0)
	    IOLog("%s: waiting for interrupt for command %x\n", 
		    [self name], command);
#endif undef

    result = msg_receive(&msg, RCV_TIMEOUT, _interruptTimeOut);
    
    if (result == RCV_SUCCESS || result == RCV_TOO_LARGE) {
		if (status != NULL)
			*status = inb(_ideRegsAddrs.status);	/* acknowledge */
		else
			inb(_ideRegsAddrs.status);
	    
#ifdef undef
        if (logInterrupts > 0)
	    IOLog("%s: interrupt received for cmd: 0x%0x, status: 0x%0x\n", 
		    [self name], command, (status == NULL) ? 0 : *status);
#endif undef
        logInterrupts -= 1;
        lastCommand = command;

		return IDER_SUCCESS;
    }
    
    if (result == RCV_TIMED_OUT)
		IOLog("%s: interrupt timeout, cmd: 0x%0x\n", [self name], command);
    else
		IOLog("%s: Error %d in receiving interrupt, cmd: 0x%0x\n", 
			[self name], result, command);

    return IDER_CMD_ERROR;
#endif NO_IRQ_MSG
}

/*
 * Remove any interrupts messages that have queued up. This will get rid of
 * any spurious or duplicate interrupts. 
 */
- (void)clearInterrupts
{
#ifdef NO_IRQ_MSG
	return;
#else  NO_IRQ_MSG
    msg_return_t result;
    msg_header_t msg;
    int count;

    count = 0;

    while (1)	{

	msg.msg_local_port = _ideInterruptPort;
	msg.msg_size = sizeof(msg);
	
	result = msg_receive(&msg, RCV_TIMEOUT, 0);
	
	if (result != RCV_SUCCESS) {
	    if (count != 0)	{
#ifdef DEBUG
		IOLog("%s: %d spurious interrupts after command 0x%0x\n", 
			[self name], count, lastCommand);
#endif DEBUG
	    }
	    return;
	}
	count += 1;
    }
#endif NO_IRQ_MSG
}

- free
{
	int n;

	[self resetController];
			
	if ((_controllerID == PCI_ID_PIIX) ||
		(_controllerID == PCI_ID_PIIX3) ||
		(_controllerID == PCI_ID_PIIX4)) {
		if (_prdTable.ptr)
			IOFree(_prdTable.ptrReal, _prdTable.sizeReal);
	}
	
	for (n = 0; n < MAX_IDE_DRIVES; n++) {
		if (_drives[n].ideIdentifyInfo)
			IOFree(_drives[n].ideIdentifyInfo, sizeof(ideIdentifyInfo_t));
	}

	return [super free];
}

- (ide_return_t)waitForNotBusy
{
    int     delay = MAX_BUSY_DELAY;	/* microseconds */
    unsigned char status;

#ifdef undef
    IOLog("%s: waitForNotBusy\n", [self name]);
#endif undef

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
	
#ifdef undef
	if ((_printWaitForNotBusy) && (delay % 20000 == 0))	{
	    IOLog("%s: waitForNotBusy, status = %x\n", 
	    	[self name], status);
	}
#endif undef
    }

    return IDER_TIMEOUT;
}

- (ide_return_t)waitForDeviceReady
{
    int     delay = MAX_BUSY_DELAY;
    unsigned char status;

#ifdef undef
    if (_printWaitForNotBusy)
	IOLog("%s: waitForDeviceReady\n", [self name]);
#endif undef
	
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
	
#ifdef undef
	if ((_printWaitForNotBusy) && (delay % 20000 == 0))	{
	    IOLog("%s: waitForDeviceReady, status = %x\n", 
	    	[self name], status);
	}
#endif undef

    }
    return IDER_TIMEOUT;
}

#define ATA_IDLE_BIT_MASK	(BUSY | DREQUEST)
- (ide_return_t)waitForDeviceIdle
{
    int     delay = MAX_BUSY_DELAY;
    unsigned char status;

#ifdef undef
	IOLog("%s: waitForDeviceIdle\n", [self name]);
#endif undef
	
    delay -= 2;
    while (delay > 0) {
		status = inb(_ideRegsAddrs.status);
		if ((status & ATA_IDLE_BIT_MASK) == 0)
			return IDER_SUCCESS;
		if (delay % 1000) {
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

#ifdef undef
    IOLog("waitForDataReady\n");
#endif undef

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

#ifdef undef
    IOLog("waitForDataReady\n");
#endif undef

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
}

- (void)disableInterrupts
{
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
	IOLog("%s: %s: error=0x%x secCnt=0x%x "
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
    ideXferData(addr, read, client, length, _ideRegsAddrs, _transferWidth);
}

- (BOOL)isMultiSectorAllowed:(unsigned int)unit
{
    return (_drives[unit].multiSector != 0);
}

-(unsigned int)getMultiSectorValue:(unsigned int)unit
{
    return _drives[unit].ideIdentifyInfoSupported ?
		_drives[unit].multiSector : 0;
}

- (void)ideCntrlrLock
{
    [_ideCmdLock lock];
}

- (void)ideCntrlrUnLock
{
    [_ideCmdLock unlock];
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
	
    return _drives[unit].atapiDevice;
}

- (BOOL)isAtapiCommandActive:(unsigned char)unit
{
    return _drives[unit].atapiCommandActive;
}

- (void)setAtapiCommandActive:(BOOL)state forUnit:(unsigned char)unit
{
    _drives[unit].atapiCommandActive = state; 
}

- (BOOL) isDmaSupported:(unsigned int)unit
{
	return (_drives[unit].transferType != IDE_TRANSFER_PIO);
}

/*
 * Simply forward the interrupt. 
 */
static void ideInterruptHandler(void *identity, void *state, unsigned int arg)
{
#ifdef NO_IRQ_MSG
	ideStruct_t	*obj = (ideStruct_t *)arg;
	obj->interruptOccurred = YES;
	thread_wakeup_one(&obj->waitQueue);
#else  NO_IRQ_MSG
    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
#endif NO_IRQ_MSG
}

- (BOOL)getHandler:(IOEISAInterruptHandler *)handler
		level:(unsigned int *)ipl
		argument:(unsigned int *)arg
		forInterrupt:(unsigned int)localInterrupt
{
    *handler = ideInterruptHandler;
    *ipl = IPLDEVICE;
    *arg = (unsigned int)self;    
    return YES;
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
    IOLog("%s: Entering suspend mode.\n", [self name]);
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

    dh = _drives[unit].addressMode | (unit ? SEL_DRIVE1 : SEL_DRIVE0);

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
		IOLog("%s: Recovered from suspend mode.\n", [self name]);
		return YES;
	    }
	}
	IOLog("%s: Retrying to recover from suspend mode.\n", [self name]);
    }

    IOLog("%s: Failed to recover from suspend mode.\n", [self name]);
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
	if ((_drives[i].ideInfo.type != 0) && ([self isAtapiDevice:i] == NO)) {
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
        //IOLog("%s: unknown APM event %x\n", [self name], (unsigned int)state);
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

@end
