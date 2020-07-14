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

/**
 * Copyright (c) 1994-1996 NeXT Software, Inc.  All rights reserved. 
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * AppleMeshHardware.m - Architecture-specific methods for Apple96 SCSI driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */
#import "Apple96SCSI.h"
#import "Apple96SCSIPrivate.h"
#import "Apple96Hardware.h"
#import "Apple96HWPrivate.h"
#import "Apple96ConfigKeys.h"
#import "Apple96Curio.h"
#import "Apple96CurioPublic.h"
#import "MacSCSICommand.h"
#import "bringup.h"
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import	<driverkit/align.h>
#import <bsd/dev/scsireg.h>
// #import <mach/mach.h>
// #import <mach/mach_error.h>
#import <mach/kern_return.h>
#import <kernserv/prototypes.h>

static int		getConfigParam(
	id		    configTable,
	const char	    *paramName
    );
static unsigned int	GetSCSICommandLength(
	const cdb_t	    *cdbPtr,
	unsigned int	    defaultLength
    );


/**
 * These are the hardware-specific methods that are not explicitly
 * tied to Mesh and DBDMA.
 */

@implementation Apple96_SCSI(Hardware)

/**
 * Perform architecture-specific initialization: fetch the device's
 * bus address and interrupt port number. Also, allocate one page
 * of memory for the channel command. (Strictly speaking, we don't
 * need an entire page, but we can use the rest of the page for
 * a permanent status log).
 *
 * @param	deviceDescription	Specify the device to initialize.
 * @return	self			[self free] if failure.
 */
- hardwareInitialization
		: deviceDescription
{
	IOReturn	ioReturn	= IO_R_SUCCESS;
	id		result		= self;
	kern_return_t	kernelReturn;
	UInt8		lun;
	id		configTable;
	const char	*configValue;
	UInt8		deviceNumber;
	UInt8		functionNumber;
	UInt8		busNumber;
		
	ENTRY("Hhi hardwareInitialization");
	ddmInit("%s: hardwareInitialization\n",
		"Apple96SCSI",
		2,3,4,5
	);
	configTable = [deviceDescription configTable];
	ASSERT(configTable != NULL);
	configValue = [configTable valueForStringKey: "Bus Type"];
	if (configValue == NULL) {
	    IOLog("%s: No bus type in configuration table\n",
			"Apple96SCSI"
		);
	    ioReturn = IO_R_NO_DEVICE;
	}
	ddmInit("Bus type: \"%s\"\n", configValue, 2, 3, 4, 5);
	if (configValue == NULL || strcmp(configValue, "PPC") != 0) {
	    IOLog("%s: Unsupported bus type \"%s\" in configuration table\n",
			"Apple96SCSI",
			configValue ? configValue :"null"
		);
	    ioReturn = IO_R_NO_DEVICE;
	}
	if (ioReturn == IO_R_SUCCESS) {
#if 0 // ** ** ** Need correct definition ** ** **
	    ioReturn = [deviceDescription getPCIDevice
				: &deviceNumber
		function	: &functionNumber
		bus		: &busNumber];
#else
	    deviceNumber = 00;
	    functionNumber = 00;
	    busNumber = 00;
	    kernelReturn = 0;
#endif
	    if (ioReturn != IO_R_SUCCESS) {
		IOLog("%s: Can't get PCI device information: %s\n",
			    "Apple96SCSI",
			    [self stringFromReturn : ioReturn]
		    );
	    }
	}
	if (ioReturn == IO_R_SUCCESS) {
#if 0
	    IOLog("%s: Hardware device found at"
			" bus %u, device %u, function %u, interrupt %u\n",
			"Apple96SCSI",
			busNumber,
			deviceNumber,
			functionNumber,
			0
//			[deviceDescription interrupt]
	    );
#endif
	}
	if (configValue != NULL) {
	    [configTable freeString : configValue];
	    configValue = NULL;
	}
	if (ioReturn == IO_R_SUCCESS) {
	    ioReturn = [self hardwareAllocateHardwareAndChannelMemory : deviceDescription];
	}
	if (ioReturn == IO_R_SUCCESS) {
	    /*
	     * Initialize gPerTargetData[] and the dbdma channel area.
	     */
	    [self initializePerTargetData];
	    /*
	     * All of the addresses are established. Check that the hardware
	     * is present and working.
	     */
	    ioReturn = [self hardwareChipSelfTest];
	}
	if (ioReturn == IO_R_SUCCESS) {
	    /*
	     * Tell the superclass to initialize our I/O thread. After this,
	     * we should be able to execute SCSI requests.
	     */
	    if ([super initFromDeviceDescription:deviceDescription] == NULL) {
		IOLog("%s: Host Adaptor was not initialized. Fatal\n",
			    "Apple96SCSI"
		    );
		ioReturn = IO_R_NO_DEVICE;
	    }
	}
	if (ioReturn == IO_R_SUCCESS) {
	    ddmInit("Init: I/O thread running\n", 1, 2, 3, 4, 5);
	    gFlagIOThreadRunning = 1;
	    /*
	     * Initialize local variables. Note that activeArray and 
	     * perTarget arrays are zeroed by objc runtime.
	     */
	    queue_init(&gDisconnectedCommandQueue);
	    queue_init(&gIncomingCommandQueue);
	    queue_init(&gPendingCommandQueue);
	    gIncomingCommandLock = [[NXLock alloc] init];
	    gActiveCommand = NULL;
	    [self resetStats];
	    gOptionDisconnectEnable	= ENABLE_DISCONNECT;
	    gOptionCmdQueueEnable	= ENABLE_TAGGED_QUEUEING;
	    gNextQueueTag		= QUEUE_TAG_NONTAGGED + 1;
	    gInitiatorID		= kInitiatorIDDefault;
	    /*
	     * Initiator bus ID as a bitmask for selection.
	     */
	    gInitiatorIDMask		= (1 << gInitiatorID);
	    /*
	     * Reserve the initiator ID for all LUNs.
	     */
	    for (lun = 0; lun < SCSI_NLUNS; lun++) {
		[self reserveTarget
					: gInitiatorID 
			lun		: lun
			forOwner	: self
		    ];
	    }
	    /*
	     * get tagged command queueing, sync mode, fast mode enables from
	     * configTable.
	     */
#ifdef CRAP	/* this requires "inspector" support:	*/
	    gOptionCmdQueueEnable = getConfigParam(configTable, CMD_QUEUE_ENABLE);
#endif /* CRAP */
	    /*
	     * Note: the external bus on PCI Mac's do not support synchronous
	     * and/or fast modes. This code should be modified to read the
	     * chip version id and configure sync/fast accordingly.
	     */
#if 0	/* Enable for 8100 internal bus */
	    gOptionSyncModeEnable = getConfigParam(configTable, SYNC_ENABLE);
	    gOptionFastModeEnable = getConfigParam(configTable, FAST_ENABLE);
#else
	    gOptionSyncModeEnable = 0;
	    gOptionFastModeEnable = 0;
#endif
	    gOptionExtendTiming   = getConfigParam(configTable, EXTENDED_TIMING);
	    gOptionAutoSenseEnable = AUTO_SENSE_ENABLE;	// from bringup.h
	    /*
	     * Get internal version of interruptPort; set the port queue 
	     * length to the maximum size. It is not clear if we want
	     * to do this.
	     */
	    gKernelInterruptPort = IOConvertPort(
		[self interruptPort],
		IO_KernelIOTask,
		IO_Kernel
	    );
#if 0 // ** ** ** Need correct header file ** ** **
	    kernelReturn = port_set_backlog(
		task_self(),
		[self interruptPort],
		PORT_BACKLOG_MAX
	    );
	    if (kernelReturn != KERN_SUCCESS) {
		IOLog("%s: warning, port_set_backlog error %d (%s)\n",
			[self name],
			kernelReturn,
			mach_error_string(kernelReturn)
		    );
	    }
#endif
	    /*
	     * Initialize the chip and reset the bus.
	     */
	    ioReturn = [self hardwareReset : TRUE	reason : NULL];

	}
	if (ioReturn == IO_R_SUCCESS) {
	    /*
	     * OK, we're ready to roll.
	     */
	    [self enableInterrupt:0];
//	    [self enableAllInterrupts ];
	    [self registerDevice];
	}
	else {
	    /*
	     * Do we need to free the locks and similar?
	     */
	    [self free];
	    result = NULL;
	}
	RESULT(result);
	return (result);
}

/**
 * Initialize sync mode to async for all targets at start and
 * after bus reset.
 */
- (void)    initializePerTargetData
{
	UInt8		    target;

	ENTRY("His initializeTargetSyncMode");
	for (target = 0; target < SCSI_NTARGETS; target++) {
	    [self renegotiateSyncMode : target];
	    gPerTargetData[ target ].inquiry_7 = 0;
	}
	EXIT();
}

/**
 * Initialize the synchronous data transfer state for a target.
 */
- (void)    renegotiateSyncMode
			: (UInt8) target
{
	ENTRY("Hrs renegotiateSyncMode");
	EXIT();
}

/**
 * Reusable CURIO init function. This includes a SCSI reset.
 * Handling of ioComplete of active and disconnected commands must be done
 * elsewhere. Returns IO_R_SUCCESS if successful. This is called
 * from a Task thread. It will disable and re-enable interrupts.
 * Reason is for error logging.
 */
- (IOReturn) hardwareReset
		    : (Boolean) resetSCSIBus
	    reason  : (const char *) reason
{
	ENTRY("HHr hardwareReset");
	if (reason != NULL) {
	    IOLog("%s: Bus Reset (%s)\n",
		[self name],
		reason
	    );
	}
	/*
	 * First of all, reset the hardware.
	 */
	[self curioHardwareReset : resetSCSIBus	    reason : reason];
	/*
	 * Kill all disconnected, pending, and incoming I/O requests.
	 */
	[self abortAllCommands : SR_IOST_RESET];
	RESULT(IO_R_SUCCESS);
	return (IO_R_SUCCESS);	
}


/*
 * Start a SCSI transaction for the specified command. ActiveCmd must be 
 * NULL. A return of kHardwareStartRejected indicates that caller may try
 * again with another command; kHardwareStartBusy indicates a condition
 * other than (activeCmd != NULL) which prevents the processing of the command.
 */
- (HardwareStartResult) hardwareStart
			: (CommandBuffer *) cmdBuf
{   
	IOSCSIRequest	    *scsiReq;
	HardwareStartResult result			= kHardwareStartOK;
	cdb_t		    	*cdbp;
	Boolean		    	okToDisconnect	= gOptionDisconnectEnable;
//	Boolean		    	okToDisconnect	= TRUE;
	Boolean		    	okToQueue		= gOptionCmdQueueEnable;
	UInt8		    	msgByte;
	
	ENTRY("Hst hardwareStart");
	/*
	 * We do not have a current command here.
	 */
	ASSERT(cmdBuf != NULL && cmdBuf->scsiReq != NULL);
	scsiReq = cmdBuf->scsiReq;
	cdbp = &scsiReq->cdb;
	ddmChip("hardwareStart[%d.%d] cmdBuf = 0x%x opcode %s\n",
	    scsiReq->target,
	    scsiReq->lun,
	    cmdBuf, 
	    IOFindNameForValue(cdbp->cdb_opcode, IOSCSIOpcodeStrings),
	    5);
	cmdBuf->cdbLength = GetSCSICommandLength(cdbp, scsiReq->cdbLength);
	if (cmdBuf->cdbLength == 0) {
	    /*
	     * Failure: we can't determine the length of this command.
	     */
	    [self ioComplete
				: cmdBuf
		finalStatus	: SR_IOST_CMDREJ
	    ];
	    ddmChip("hardwareStart rejected: invalid cdb length\n", 1, 2, 3, 4, 5);
		    result = kHardwareStartRejected;
	}
	if (result == kHardwareStartOK) {
	    /*
	     * Peek at the control byte (the last byte in the command).
	     */
	    msgByte = ((UInt8 *) cdbp)[cmdBuf->cdbLength - 1];
	    if ((msgByte & CTRL_LINKFLAG) != CTRL_NOLINK) {
		/*
		 * Failure: we don't support linked commands.
		 */
		[self ioComplete
					: cmdBuf
			finalStatus	: SR_IOST_CMDREJ
		];
		ddmChip("hardwareStart rejected: linked command\n", 1, 2, 3, 4, 5);
		result = kHardwareStartRejected;
	    }
	}
	if (result == kHardwareStartOK) {
	    /*
	     * Autosense always renegotiates synchronous transfer mode.
	     * This is necessary as the target might have been reset
	     * or hit with a power-cycle.
	     */
	    if (cmdBuf->flagIsAutosense) {
			[self renegotiateSyncMode : gCurrentTarget];
			okToDisconnect = FALSE;
	    }
	    else {
		/*
		 * This is a real command. Setup the user data
		 * pointers and counters and build a SCSI request
		 * channel command list.
		 *
		 * First, peek at the command for some special cases.
		 */
		cmdBuf->queueTag = QUEUE_TAG_NONTAGGED; /* No tag just yet  */
		switch (cdbp->cdb_opcode) {
		case kScsiCmdInquiry:
		    /*
		     * The first command SCSIDisk sends us is an Inquiry
		     * command. This never gets retried, so avoid a possible 
		     * reject of a command queue tag. Avoid this hack if
		     * there are any other commands outstanding for this
		     * target/lun.
		     */
		    if (gActiveArray[scsiReq->target][scsiReq->lun] == 0) {
			scsiReq->cmdQueueDisable = TRUE;
		    }
		    okToDisconnect = FALSE;
		    break;
		case kScsiCmdRequestSense:
		    /*
		     * Always force sync renegotiation on any Request Sense to 
		     * catch independent target power cycles.
		     * (Synch renegotiation needed should be set after all
		     * target-detected errors -- fix needed in MessageIn).
		     * Sense is always issued with disconnect disabled to
		     * maintain T/L/Q nexus.
		     *
		     * Watch it: request sense from a client is incompatible
		     * with tagged queuing.
		     */
		    [self renegotiateSyncMode : gCurrentTarget];
		    okToDisconnect	= FALSE;
		    break;
    
		case kScsiCmdTestUnitReady:
		case kScsiCmdReadCapacity:
		    okToDisconnect = FALSE;
		    break;
		}
	    }

	    if ( scsiReq->disconnect == FALSE )		/* If this req doesn't want */
			okToDisconnect = FALSE;				/* it, clear disconnect.    */

		okToQueue	&= okToDisconnect
					&& (scsiReq->cmdQueueDisable == FALSE)
					&& (gPerTargetData[ scsiReq->target ].inquiry_7 & 0x02);

	    cmdBuf->flagActive		    = 0;	/* Init flags for this command	*/
	    /*
	     * Make sure that the chip is stable before we try to start
	     * a request.
	     */
	    /*
	     * Currently, the only expected reason we return kHardwareStartBusy
	     * is if we have a reselect pending.
	     * ** ** ** I don't think this can happen ** ** **
	     */
	    if (gFlagBusBusy) {
		queue_enter(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
		ddmChip("hardwareStart deferred: bus busy\n", 1, 2, 3, 4, 5);
		result = kHardwareStartBusy;
	    }
	}
	if (result == kHardwareStartOK) {
	    if (gActiveCommand != NULL) {
		/*
		 * This should never happen. It ensures that there are
		 * no race conditions that reselect us between the time
		 * threadExecuteRequest looked at gActiveCommand and the
		 * time we disabled interrupts.
		 */
		queue_enter(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
		ddmChip("hardwareStart deferred: there is an active command\n", 1, 2, 3, 4, 5);
		result = kHardwareStartBusy;
	    }
	}
	if (result == kHardwareStartOK) {
	    /*
	     * Activate this command - if we fail later, we'll de-activate it.
	     */
#if SERIOUS_DEBUGGING
	    /*
	     * If we're reading, preset the buffer to a known data.
	     */
	    if (cmdBuf->scsiReq != NULL
	     && cmdBuf->scsiReq->read
	     && cmdBuf->buffer != NULL
	     && (((UInt32) cmdBuf->buffer) & 0x03) == 0) {
		int		i;
		UInt32		*ptr = (UInt32 *) cmdBuf->buffer;
		for (i = 0; i < cmdBuf->scsiReq->maxTransfer; i += sizeof (UInt32)) {
		    *ptr++ = 0xDEADBEEF;
		}
	    }
#endif /* SERIOUS_DEBUGGING */
#if SERIOUS_DEBUGGING || LOG_COMMAND_AND_RESULT
	    [self logCommand
			: cmdBuf
		logMemory   : FALSE
		reason	: "Command at commandStart"
	    ];
#endif /* SERIOUS_DEBUGGING || LOG_COMMAND_AND_RESULT */
	    ASSERT(gActiveCommand == NULL);
	    [self activateCommand : cmdBuf];
	    ASSERT(scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN);
	    /* MESH only: [self clearChannelCommandResults];	*/
	    /*
	     * Reset the message out buffer pointers.
	     */
	    gMsgOutPtr = gMsgOutBuffer;
	    gMsgPutPtr = gMsgOutBuffer;
	    msgByte = kScsiMsgIdentify | scsiReq->lun;
	    if (okToDisconnect)
		msgByte |= kScsiMsgEnableDisconnectMask;
	    *gMsgOutPtr++ = msgByte;
	    /*
	     * According to the SCSI Spec, the tag command immediately
	     * follows the selection.
	     */
	    /*
	     * Note that autosense is issued on the current tag.
	     * The command was initialized with QUEUE_TAG_NONTAGGED.
	     */
		/***** Driver Kit supports only simple queue tags.	*****/
	    if ( okToQueue ) {
				/* Avoid using tag QUEUE_TAG_NONTAGGED (zero).	*/
			cmdBuf->queueTag = gNextQueueTag;
			if (++gNextQueueTag == QUEUE_TAG_NONTAGGED) {
				gNextQueueTag++;
			}
			*gMsgOutPtr++	= kScsiMsgSimpleQueueTag;
			*gMsgOutPtr++	= cmdBuf->queueTag;
	    }
	    if (cmdBuf->flagIsAutosense == 0) {
		cmdBuf->currentDataIndex    = 0;
		cmdBuf->savedDataIndex	    = 0;
		if (gActiveCommand->mem != NULL) {
		    [gActiveCommand->mem setPosition : 0];  /* Needed?	*/
		}
		scsiReq->driverStatus	    = SR_IOST_INVALID;
		scsiReq->totalTime	    = 0ULL;
		scsiReq->latentTime	    = 0ULL;
	    }
	    [self hardwareStartSCSIRequest];
	    IOGetTimestamp(&cmdBuf->startTime);
	}
	RESULT(result);
	return (result);
}


@end /* Apple96_SCSI(Hardware) */



/*
 * Obtain a YES/NO type parameter from the config table.
 * @param   configTable The table to examine.
 * @param   paramName	The parameter to look for.
 * @result  Zero if missing from the table or the table value is
 *	not YES. One if present in the table and the table
 *	value is YES.
 */
static int		     getConfigParam(
	id			configTable,
	const char		*paramName
    )
{
	const char		*value;
	int			result = 0; // default if not present in table
	
	ENTRY("Hgc getConfigParam");
	value = [configTable valueForStringKey:paramName];
	if (value != NULL) {
	    if (strcmp(value, "YES") == 0) {
		result = 1;
	    }
	    [configTable freeString:value];
	}
	RESULT(result);
	return (result);
}

static unsigned int
GetSCSICommandLength(
	const cdb_t		*cdbPtr,
	unsigned int		defaultLength
    )
{
	unsigned int		result;
	
	/*
	 * Warning: don't use sizeof here - the compiler rounds the
	 * value up to the next word boundary.
	 */
	switch (((UInt8 *) cdbPtr)[0] & 0xE0) {
	case (0 << 5):
	    result = 6;
	    break;
	case (1 << 5):
	case (2 << 5):
	    result = 10;
	    break;
	case (5 << 5):
	    result = 12;
	    break;
	case (6 << 5):
	    result = (defaultLength != 0) ? defaultLength : 6;
	    break;
	case (7 << 5):
	    result = (defaultLength != 0) ? defaultLength : 10;
	    break;
	default:
	    result = 0;
	    break;
	}
	return (result);
}
