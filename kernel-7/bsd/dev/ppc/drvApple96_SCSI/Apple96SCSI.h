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
 * Copyright (c) 1994-1996 NeXT Software, Inc.	All rights reserved. 
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author  Martin Minow    mailto:minow@apple.com
 * @revision	1997.02.13  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Apple96SCSI.h - top-level API for Apple 53C96 (Curio/DBDMA) SCSI adaptor.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM  Initial conversion from AMDPCSCSIDriver sources.
 * 1997.03.24	MM  Normalized against AppleMeshSCSI.h
 * 1997.11.12	MM  Added support for scatter-gather lists.
 */
#define APPLE96_ALWAYS_ASSERT		0
#define APPLE96_ENABLE_GET_SET		1
#define VERY_SERIOUS_DEBUGGING		0	/* Log each method entry and exit */
#define SERIOUS_DEBUGGING		0	/* Log command, buffer, etc operation */
#define LOG_COMMAND_AND_RESULT		0	/* Log command and results */
#define LOG_ERROR_RECOVERY		0	/* Log during error recovery */
#define LOG_RESULT_ON_ERROR		0	/* Log all errors, even expected ones */
#define DUMP_USER_BUFFER		0	/* Set to buffer length to dump */
#define TIMESTAMP_AT_IOCOMPLETE		0
/*
 * Radar 2005639. Tell SCSIDisk how many threads to create.
 */
#define MAX_CLIENT_THREADS		1	/* Ask SCSIDisk to create threads */
#define ENABLE_DISCONNECT		FALSE	/* TEMP TEMP TEMP */
#define ENABLE_TAGGED_QUEUEING		FALSE	/* TEMP TEMP TEMP */
// #define USE_CURIO_METHODS		1	/* TEMP TEMP TEMP */
#define USE_CURIO_METHODS		0	/* Default to inline */
#define WRITE_CHIP_OPERATION_INTO_TIMESTAMP_LOG 1
#define APPLE96_ENABLE_GET_SET		1	/* Enable management ioctl's */
#ifndef VERY_SERIOUS_DEBUGGING
#define VERY_SERIOUS_DEBUGGING		0
#endif
#ifndef USE_CURIO_METHODS
#define USE_CURIO_METHODS		0
#endif
#ifndef WRITE_CHIP_OPERATION_INTO_TIMESTAMP_LOG
#define WRITE_CHIP_OPERATION_INTO_TIMESTAMP_LOG 0
#endif
#ifndef APPLE96_ALWAYS_ASSERT
#define APPLE96_ALWAYS_ASSERT		0
#endif
#if VERY_SERIOUS_DEBUGGING
#undef SERIOUS_DEBUGGING
#define SERIOUS_DEBUGGING		1
#endif
#ifndef SERIOUS_DEBUGGING
#define SERIOUS_DEBUGGING		0
#endif
#ifndef LOG_COMMAND_AND_RESULT
#define LOG_COMMAND_AND_RESULT		0
#endif
#ifndef LOG_ERROR_RECOVERY
#define LOG_ERROR_RECOVERY		0
#endif
#ifndef TIMESTAMP_AT_IOCOMPLETE
#define TIMESTAMP_AT_IOCOMPLETE		0
#endif
#ifndef MAX_CLIENT_THREADS
#define MAX_CLIENT_THREADS		6	/* Radar 2005639 */
#endif

#undef NDEBUG	/* Enable ASSERT macro */

#import <driverkit/IODirectDevice.h>
#import <driverkit/IOSCSIController.h>
#import <driverkit/scsiTypes.h>
#import <driverkit/IOPower.h>
#import "Apple96Types.h"
#import "Apple96Curio.h"
#import "Apple96DBDMADefs.h"
#import "Timestamp.h"
#import "Apple96DDM.h"

@interface Apple96_SCSI : IOSCSIController <IOPower>
{
    /*
     * These globals locate the hardware interface registers in logical and physical
     * address spaces.
     */
    PhysicalAddress	gSCSIPhysicalAddress;	/* -> NCR registers (physical)	*/
    volatile UInt8	*gSCSILogicalAddress;	/* -> NCR registers (logical)	*/
    UInt32		gSCSIRegisterLength;    /* == Size of NCR registers	*/
    
    PhysicalAddress	gDBDMAPhysicalAddress;	/* -> DBDMA registers (physical) */
    DBDMAChannelRegisters   *gDBDMALogicalAddress;  /* -> DBDMA registers (logical) */
    UInt32		gDBDMARegisterLength;   /* == Size of DBDMA registers   */

    PhysicalAddress	gDBDMAChannelAddress;	/* -> DBDMA channel area (physical) */
    DBDMADescriptor	*gChannelCommandArea;	/* -> DBDMA channel area (logical)  */
    UInt32		gChannelCommandAreaSize; /* == DBDMA channel allocated size */
    UInt32		gDBDMADescriptorMax;    /* Number of DATA descriptors   */
    
    UInt32		gPageSize;		/* Logical page size	    	*/
    UInt32		gPageMask;		/* Logical page size - 1	*/
    
    /*
     * Autosense I/O requests write into this permanently-allocated area. Then, we copy
     * the sense data to the caller's SCSI command parameter block. The autosense area
     * is at the end of the descriptor area.
     */
    esense_reply_t	*gAutosenseArea;	/* -> Autosense area (logical)	*/
    PhysicalAddress	gAutosenseAddress;	/* Used to autosense DMA	*/
    
    /*
     * This is the message port that the kernel uses to signal hardware interrupts.
     */
    port_t		gKernelInterruptPort;

    /*
     * Commands are passed from exported methods to the I/O thread
     * via gIncomingCommandQueue, which is protected by gIncomingCommandLock.
     * 
     * Commands which are disconnected but not complete are kept
     * in gDisconnectedCommandQueue.
     *
     * Commands which have been dequeued from gIncomingCommandQueue by the 
     * I/O thread but which have not been started because a command is currently
     * active on the bus are kept in gPendingCommandQueue. This queue also
     * holds commands pushed back when we lose arbitration.
     *
     * The currently active command, if any, is kept in gActiveCommand.
     * Only commandBufs with op == kCommandExecute are ever placed in
     * gActiveCommand.
     */
    queue_head_t	gIncomingCommandQueue;
    id			gIncomingCommandLock;	/* NXLock for gIncomingCommandQueue */
    queue_head_t	gPendingCommandQueue;
    queue_head_t	gDisconnectedCommandQueue;
    /*
     * This is the command we're currently execution. If NULL, the Macintosh is idle
     * (or all commands are disconnected). Normally, gCurrentTarget and gCurrentLUN
     * track the values in the active command's associated SCSI request. They are set
     * to kInvalidTarget and kInvalidLUN at initialization, command deactivation,
     * command complete, and command disconnect. They are set to valid values (with no
     * active command) during reselection. This is tricky, so look carefully at the code.
     */ 
    CommandBuffer	*gActiveCommand;	/* -> The currently executing command */
    UInt32		gCurrentTarget;		/* == The current target bus ID	*/
    UInt32		gCurrentLUN;		/* == The current target LUN	*/
	
    /*
     * Global option flags, accessible via instance table or setIntValues. Note: some of
     * these are intended for debugging. However, users may need to disable command
     * queuing, synchronous, or fast to handle device or bus limitations. The
     * architecture-specific initialization "looks" at the device to determine whether
     * specific features (such as synchronous) are supported.
     *	gOptionAutoSenseEnable	    Debug only, normally set
     *	gOptionCmdQueueEnable	    Enable tagged queuing.
     *	gOptionSyncModeEnable	    Enable synchronous transfers (clear if problems)
     *	gOptionFastModeEnable	    Enable fast transfers (clear if problems)
     *	gOptionDisconnectEnable	    Debug only, normally set.
     *	gOptionExtendTiming	Extended selection timing (debug, unused)
     *	gOptionSyncModeSupportedByHardware  Set if this NCR chip supports synchronous.
     *	gOptionFastModeSupportedByHardware  Set if this NCR chip supports fast mode.
     *	gFlagIOThreadRunning	    Set when I/O thread is initialized. Needed
     *			for shutdown.
     *	gFlagBusBusy		Set in the state machine when we are apparently
     *			connected to a target (even for target selection).
     *			Cleared in the interrupt service routine on
     *			a disconnected or bus reset interrupt state.
     *	gFlagCheckForAnotherInterrupt	Set in the interrupt service routine on
     *			entrance to the bus finite-state automaton.
     *			Automaton functions clear this to force an
     *			exit from the ISR when it expects a long bus
     *			operation.
     */
    unsigned	gOptionAutoSenseEnable		: 1,
		gOptionCmdQueueEnable		: 1,
		gOptionSyncModeEnable		: 1,
		gOptionFastModeEnable		: 1,
		gOptionExtendTiming		: 1,
		gOptionDisconnectEnable		: 1,
		gOptionSyncModeSupportedByHardware : 1,
		gOptionFastModeSupportedByHardware : 1,
		gFlagIOThreadRunning		: 1,	/* Set at init		*/
		gFlagBusBusy			: 1,
		gFlagCheckForAnotherInterrupt	: 1,	/* Loop through ISR	*/
		gFlagNeedAnotherInterrupt	: 1,	/* mikej		*/
		pad				: 20;
    UInt32	scsiClockRate;		    /* in MHz	    */

    /*
     * Array of active I/O counters, one counter per lun per target. If command
     * queueing is disabled, the max value of each counter is 1. gActiveCount is
     * the sum of all elements in activeArray.
     */
    UInt8	gActiveArray[SCSI_NTARGETS][SCSI_NLUNS];
    UInt32	gActiveCount;	

    /*
     * These variables change during SCSI I/O operation.
     */
    BusState		gBusState;	    /* The overall state automaton  */
    /*
     * These variables manage Message In bus phase. Because the message in handler
     * uses programmed I/O, gMsgInCount, gMsgInIndex, and gMsgInState are actually
     * local variables to the message reader, and are here for debugging convenience.
     */
    signed int		gMsgInCount;	/* Message bytes still to read	*/
    MsgInState		gMsgInState;	/* How are we handling messages */
    UInt32		gMsgInIndex;	/* -> free spot in gMsgInBuffer */
    UInt8		gMsgInBuffer[kMessageInBufferLength];
    /*
     * These variables manage Message Out bus phase. To send a message,
     * write it into the buffer, starting at gMsgOutSendIndex and set ATN.
     * When the target enters Message Out phase, write the bytes into the
     * NCR fifo starting at gMsgOutStoreIndex. When the transfer completes
     * with MSG parity error, use the residual count to "back up" gMsgOutSendIndex
     * to resend the message.
     */
    UInt8	*gMsgOutPtr;	    /* -> free spot in gMsgOutBuffer	*/
    UInt8	*gMsgPutPtr;	    /* -> next byte to send	*/
    UInt8	gMsgOutBuffer[kMessageOutBufferLength];
    UInt8	gLastMsgOut[2];	    /* Has last message out specifier	*/
    /*
     * Hardware related variables
     */
    UInt8	gInitiatorID;	    /* Our SCSI ID	    */
    UInt8	gInitiatorIDMask;   /* BusID bitmask for selection  */
//    UInt8	gSelectionTimeout;  /* In MESH 10 msec units	*/
    UInt32	gClockFrequency;    /* For selection timeout	*/
    
    /*
     * commandBuf->queueTag for next I/O. This is never zero; for all requests
     * involving a T/L/Q nexus, a queue tag of zero indicates a nontagged command.
     */
    UInt8	gNextQueueTag;
    
    /*
     * Shadow current chip values.
     *	gSaveStatus	rSTA register at last interrupt.
     *	gSaveSeqStep	    rSQS register at last interrupt
     *	gSaveInterrupt	    rINT register at last interrupt.
     *	gCurrentBusPhase    The last bus phase (for error recovery)
     *	gLastSelectionTimeout	Last selection timeout we stored in Curio
     *	gLastSynchronousOffset	Last synchronous offset we stored in Curio.
     *	gLastSynchronousPeriod	Last synchronous perioud we stored in Curio.
     */
    UInt8	gSaveStatus;
    UInt8	gSaveSeqStep;
    UInt8	gSaveInterrupt;
    UInt8	gCurrentBusPhase;   /* Last known SCSI phase	*/
    UInt8	gLastSelectionTimeout;	/* Chip selection timeout   */
    UInt8	gLastSynchronousOffset; /* Synchronous transfer offset	*/
    UInt8	gLastSynchronousPeriod; /* Synchronous period shadow	*/
    /*
     * Per-target information.
     */
    PerTargetData   gPerTargetData[SCSI_NTARGETS];
	
    /*
     * Statistics support.
     */
    UInt32	gMaxQueueLen;
    UInt32	gQueueLenTotal;
    UInt32	gTotalCommands;
}

/**
 * Public methods
 */
/**
 * Initialize the SCSI driver.
 */
+ (Boolean) probe   : deviceDescription;
/**
 * Shutdown the driver.
 */
- free;

/*
 * Execute a SCSI request using a single logical address range.
 * This is compatible with earlier DriverKit implementations.
 * @param scsiReq   The SCSI request command record, including the
 *	target device and LUN, the command to execute,
 *	and various control flags.
 * @param buffer    The data buffer, if any. This may be NULL if
 *	no data phase is expected.
 * @param client    The client task that "owns" the memory buffer.
 * @return  Return a bus adaptor specific error status.
 */
- (sc_status_t) executeRequest	: (IOSCSIRequest *) scsiReq 
	 buffer			: (void *) buffer 
	 client			: (vm_task_t) client;

/*
 * Execute a SCSI request using an IOMemoryDescriptor. This allows callers to
 * provide (kernel-resident) logical scatter-gather lists. For compatibility
 * with existing implementations, the low-level SCSI device driver must first
 * ensure that executeRequest:ioMemoryDescriptor is supported by executing:
 *  [controller respondsToSelector : executeRequest:ioMemoryDescriptor]
 * @param scsiReq   The SCSI request command record, including the
 *	target device and LUN, the command to execute,
 *	and various control flags.
 * @param ioMemoryDescriptor The data buffer(s), if any. This may be NULL if
 *	no data phase is expected.
 * @param client    The client task that "owns" the memory buffer.
 * @return  Return a bus adaptor specific error status.
 */
- (sc_status_t) executeRequest
				: (IOSCSIRequest *) scsiReq 
    ioMemoryDescriptor		: (IOMemoryDescriptor *) ioMemoryDescriptor;

- (sc_status_t) resetSCSIBus;

/**
 * Reset the SCSI bus.
 */
- (sc_status_t) resetSCSIBus;
/**
 * Reset statistics buffers.
 */
- (void)    resetStats;

- (unsigned)	numQueueSamples;

- (unsigned)	sumQueueLengths;

- (unsigned)	maxQueueLength;

/**
 * interruptOccurred is a public method called by the I/O
 * thread in IODirectDevice when an interrupt occurs.
 */
- (void)    interruptOccurred;

/**
 * timeoutOccurred is a public method called by the I/O
 * thread in IODirectDevice when it receives a timeout
 * message.
 */
- (void)    timeoutOccurred;

#if APPLE96_ENABLE_GET_SET

- (IOReturn) setIntValues	: (unsigned *)parameterArray
	   forParameter		: (IOParameterName)parameterName
	      count		: (unsigned)count;
- (IOReturn) getIntValues	: (unsigned *)parameterArray
	   forParameter		: (IOParameterName)parameterName
	      count		: (unsigned *)count;    // in/out
/*
 * get/setIntValues parameters.
 */
#define APPLE96_AUTOSENSE		"AutoSense"
#define APPLE96_CMD_QUEUE		"CmdQueue"
#define APPLE96_SYNC			"Synchronous"
#define APPLE96_FAST_SCSI		"FastSCSI"
#define APPLE96_RESET_TARGETS		"ResetTargets"
#define APPLE96_RESET_TIMESTAMP		"ResetTimestamp"
#define APPLE96_ENABLE_TIMESTAMP	"EnableTimestamp"
#define APPLE96_DISABLE_TIMESTAMP	"DisableTimestamp"
#define APPLE96_PRESERVE_FIRST_TIMESTAMP "PreserveFirstTimestamp"
#define APPLE96_PRESERVE_LAST_TIMESTAMP	"PreserveLastTimestamp"
#define APPLE96_READ_TIMESTAMP		"ReadTimestamp"
#define APPLE96_STORE_TIMESTAMP		"StoreTimestamp"
/*
 * Radar 2005639. Tell SCSIDisk how many threads to create.
 * Allow access to the enable disconnect option.
 */
#define APPLE96_ENABLE_DISCONNECT	"EnableDisconnect"
#define APPLE_MAX_THREADS		"ClientThreads"

/*
 * Recording and setting timestamps may be done using getIntValues (this permits
 * access from non-privileged tasks.
 *  ResetTimestamp	Clear the timestamp vector - do this before starting
 *			a sequence (no parameters)
 *  EnableTimestamp	Start recording (no parameters) (default)
 *  DisableTimestamp	Stop recording (no parameters)
 *  PreserveFirstTimestamp  Stop recording when the buffer fills (until it is emptied)
 *  PreserveLastTimestamp   Discard old values when new arrive (default)
 *  ReadTimestamp	Read a vector of timestamps (see sample below)
 *  StoreTimestamp	Store a timestamp (from user mode) (see sample below)
 * ReadTimestamp copies timestamps from the internal database to user-specified vector.
 * Because getIntValues parameters are defined in int units, the code is slighthly
 * non-obvious:
 *  TimestampDataRecord myTimestamps[123];
 *  unsigned	    count;
 *  count = sizeof (myTimestamps) / sizeof (unsigned);
 *  [scsiDevice getIntValues
 *			: (unsigned int *) myTimestamps
 *	forParameter	: APPLE96_READ_TIMESTAMP
 *	count		: &count
 *  ];
 *  count = (count * sizeof (unsigned)) / sizeof (TimestampDataRecord);
 *  for (i = 0; i < count; i++) {
 *			Process(myTimestamps[i]);
 *  }
 * Applications can store timestamps using one of three parameter formats:
 *  unsigned	    paramVector[4];
 * Tag only -- the library will supply the event time
 *  paramVector[0] = kMyTagValue;
 *  [scsiDevice getIntValues
 *			: paramVector
 *	forParameter	: "StoreTimestamp"
 *	count		: 1
 *  ];
 * Tag plus value:
 *  paramVector[0] = kMyTagValue;
 *  paramVector[1] = 123456;
 *  [scsiDevice getIntValues
 *			: paramVector
 *	forParameter	: "StoreTimestamp"
 *	count		: 2
 *  ];
 * Tag plus value + time:
 *  paramVector[0] = kMyTagValue;
 *  paramVector[1] = 123456;
 *  IOGetTimestamp((ns_time_t *) &paramVector[2]);
 *  [scsiDevice getIntValues
 *			: paramVector
 *	forParameter	: "StoreTimestamp"
 *	count		: 4
 *  ];
 * Note that you can combine tag only with tag plus value plus time to measure
 * user->device latency.
 */

#endif	/* APPLE96_ENABLE_GET_SET */

@end


