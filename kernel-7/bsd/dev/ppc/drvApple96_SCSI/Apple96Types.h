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
 * @author  Martin Minow    mailto:minow@apple.com
 * @revision	1997.02.13  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Private structures and definitions for Apple 53C96 SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources.
 * 1997.03.24	MM		Partially normalized against AppleMeshTypes.h
 * 1997.04.17	MM		Removed SCS_PHASECHANGE (not needed)
 */
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/powermac.h>
#include <machdep/ppc/interrupts.h>
#ifndef TIMESTAMP
#define TIMESTAMP   1
#endif
#import <driverkit/scsiTypes.h>
#import <driverkit/IOMemoryDescriptor.h>
#import <machkit/NXLock.h>
#import <kernserv/queue.h>
#import <driverkit/debugging.h>
#import "bringup.h"

/*
 * These types will ultimately be moved to an implementation-wide header file.
 */
#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1
typedef unsigned char	UInt8;		/* An unsigned 8-bit value	*/
typedef unsigned int	UInt32;		/* An unsigned integer		*/
typedef signed int  SInt32;		/* An explicitly signed int	*/
typedef void	    *LogicalAddress;	/* A virtual address		*/
typedef UInt32	    PhysicalAddress;	/* A hardware address		*/
typedef UInt32	    ByteCount;		/* A transfer length count	*/
typedef UInt32	    ItemCount;		/* An index or counter		*/
typedef UInt32	    Boolean;		/* A true/false value		*/
#ifndef TRUE
#define TRUE	    1
#define FALSE	    0
#endif
#endif /* __APPLE_TYPES_DEFINED__ */

/**
 * Macros to access DBDMA Registers
 */
#ifndef SynchronizeIO
#define SynchronizeIO()	    eieio()	/* TEMP */
#endif /* SynchronizeIO */

/*
 * Operation flags and options.
 */
typedef enum BusPhase	/* These are the real SCSI bus phases			    :	*/
{
    kBusPhaseDATO	    = 0,
    kBusPhaseDATI,
    kBusPhaseCMD,
    kBusPhaseSTS,
    kBusPhaseReserved1,
    kBusPhaseReserved2,
    kBusPhaseMSGO,
    kBusPhaseMSGI,
    kBusPhaseBusFree
} BusPhase;

/*
 * Command to be executed by I/O thread. These are ultimately derived from ioctl
 * control values.
 */
typedef enum  { 
    kCommandExecute,		/* Execute IOSCSIRequest    */
    kCommandResetBus,		/* Reset bus		    */
    kCommandAbortRequest	/* Abort I/O thread	    */
} CommandOperation;

/*
 * We read target messages using a simple state machine. On entrance
 * to MSGI phase, gMsgInState = kMsgInIdle. Continue reading messages
 * until either gMsgInState == kMsgInReady or the target changes
 * phase (which is an error).
 */
typedef enum MsgInState {
    kMsgInIdle = 0,	/*  0 Not reading a message (must be zero)	*/
    kMsgInReading,	/*  1 MSG input state: reading counted data	*/
    kMsgInCounting,	/*  2 MSG input state: reading count byte	*/
    kMsgInReady		/*  3 MSG input state: a msg is now available	*/
} MsgInState;

/*
 * This is the maximum number of bytes to be transferred in an autosense
 * request. It should be equal to 255. 
 */
enum {
	kMaxAutosenseByteCount  = (sizeof (esense_reply_t) > 255)
		? sizeof (esense_reply_t) : 255
};

/*
 * gMaxDMATransfer is set so that we don't have to worry about the ambiguous
 * "zero" value in the MESH and DBDMA transfer registers that can mean either
 * 65536 bytes or zero bytes.
 */
enum {
	kMaxDMATransfer = (65536 - 4096)
};

/*
 * These values are stored in gCurrentTarget and gCurrentLUN when
 * there is no active request.
 */
enum {
    kInvalidTarget	    = 0xFFFF,
    kInvalidLUN		    = 0xFFFF,
/*
 * The default initiator bus ID (needs to be fetched from NVRAM).
 */
    kInitiatorIDDefault	    = 7
};
#define APPLE_SCSI_RESET_DELAY	(250)	/* Msec */

/*
 * Command struct passed to I/O thread.
 */
typedef struct CommandBuffer {

    /*
     * Fields valid when commandBuf is passed to I/O thread.
     */
    CommandOperation	op;	/* kCommandExecute, etc.		*/
    
    /*
     * The following 3 fields are only valid if op == CO_Execute.
     */
    IOSCSIRequest   *scsiReq;	/* -> The SCSI command parameter block	*/
    IOMemoryDescriptor *mem;	/* -> Memory to transfer, if any    */
    
    /*
     * These fields are used by the I/O thread to manage the I/O request.
     *	cmdLock		Wait for the command to complete
     *	link		Queue link for the command, disconnect, and
     *			pending queues.
     *	timeoutPort	Port for timeout messages
     *	queueTag	SCSI tagged request if not QUEUE_TAG_NONTAGGED
     */
    NXConditionLock *cmdLock;	    /* client waits on this		*/
    queue_chain_t   link;	    /* for enqueueing on commandQ	*/
    port_t	    timeoutPort;    /* for timeout messages		*/
    UInt8	    queueTag;	    /* QUEUE_TAG_NONTAGGED or queue tag */ 
    UInt8	    cdbLength;	    /* Actual length of this command	*/
    
    /*
     * SCSI bus state variables. Note that currentDataIndex can exceed
     * scsiReq->maxTransfer if the device sends (receives) more data than
     * we can receive (send). This values are NOT used for autosense. The
     * position in the transfer itself is in the IOMemoryDescriptor.
     */
    UInt32	    currentDataIndex;	/* Where we are in the DATA transfer	*/
    UInt32	    savedDataIndex;	/* Where we were at bus disconnect  */
    IOMemoryDescriptorState savedDataState; /* saved index for IOMemoryDescriptor	*/
    UInt32	    thisTransferLength; /* Current Data Phase transfer length	*/
    /*
     * Request management flags
     *	flagActive	    Set if we're in the active array and active count
     *			    reflects our existance. Managed by [self activateCmd]
     *			    and [self deactivateCmd : cmdBuf].
     *			    and that IOScheduleFunc() has been called.
     *	flagIsAutosense	    Set if we are executing an internally-generated
     *			    Request Sense command. If this is an autosense,
     *			    the operation is modified as follows:
     *			Arb/Select:	Disable disconnects. Re-establish
     *					synchronous and fast for this target,
     *					use the current tag, if any.
     *			Command:	Use an internally-generated Request Sense.
     *			Data:		Read into our wired-down sense buffer.
     *					Do not touch the data index and transfer
     *					count variables. On completion, copy
     *					from our wired-down buffer to the caller's
     *					sense array.
     *			Completion:	Good status, return SR_IOST_CHKSV to client.
     *					Bad status: never set isAutosense. Driver
     *					return SR_IOST_CHKSNV.
     */
    unsigned	    flagActive:1,	    /* We're in activeArray and activeCount */
		    flagIsAutosense:1,	    /* Set if THIS is an autosense command  */
					pad:30;
    /*
     * This is set by autosense Status phase.
     */
    UInt8	    autosenseStatus;	/* Did autosense complete ok?		*/ 
    /*
     * Statistics support.
     */
    ns_time_t	    startTime;		/* time cmd started			*/
    ns_time_t	    disconnectTime;	/* time of last disconnect		*/
    
} CommandBuffer;

/*
 * Condition variable states for commandBuf.cmdLock.
 */
#define CMD_PENDING		0
#define CMD_COMPLETE	1

/*
 * Dimension the message in/out buffers.
 */
enum {
    kMessageInBufferLength  = 16,
    kMessageOutBufferLength = 16
};

/*
 * Value of queueTag for nontagged commands. This value is never used for 
 * the tag for tagged commands.
 */
enum {
    QUEUE_TAG_NONTAGGED = 0
};

/*
 * Per-target info.
 * 
 * maxQueue is set to a non-zero value when we reach a target's queue size
 * limit, detected by a STAT_QUEUE_FULL status. A value of zero means we
 * have not reached the target's limit and we are free to queue additional
 * commands (if allowed by the overall cmdQueueEnable flag).
 *
 * syncXferPeriod and syncXferOffset are set to non-zero during sync  
 * transfer negotiation. Units of syncXferPeriod is NANOSECONDS, which
 * differs from both the chip's register format (dependent on clock 
 * frequency and fast SCSI/fast clock enables) and the SCSI bus's format
 * (which is 4 ns per unit).
 *
 * cmdQueueDisable has a default (initial) value of zero regardless of the
 * driver's overall cmdQueueEnable flag. It is set to one when a target
 * explicitly tells us that the indicated feature is unsupported. It is
 * not set to zero after bus reset.
 *
 * selectATNDisable has a default (initial) value of zero. It is set to
 * one when a target does not go to MSGO phase after a select with ATN.
 * It is not set to zero after bus reset.
 */
typedef struct PerTargetData {
	UInt8		maxQueue;	/* Max queue depth for this target	*/
	unsigned	cmdQueueDisable:1, /* No command queue for this target	*/
			selectATNDisable:1, /* No select with ATN for this target */
		pad:6;
	UInt8		inquiry_7;	/* 7th byte peeked fm Inquiry data	*/
} PerTargetData;

/*
 * Values for the finite-state automaton, stored in the gBusState instance
 * variable. See Statemachines.m for documentation. This state machine
 * operates when the bus is busy.
 *
 *	SCS_UNINITIALIZED	Initial power-up state. Illegal in the
 *				finite-state automaton (an interrupt arrived
 *				before we were completely initialized).
 *	SCS_DISCONNECTED	Normal "bus free" state (we're not processing
 *				any commands).
 *	SCS_SELECTING		Just tried to select a remote target.
 *	SCS_RESELECTING		Reselection transistion state
 *	SCS_INITIATOR		Normal "processing bus phases" state. Set
 *				after correctly responding to an interrupt.
 *				Changed to an in-progress state.
 *	SCS_COMPLETING		In command-complete sequence
 *	SCS_WAIT_FOR_BUS_FREE	After disconnect or command complete, before
 *				seeing disconnect interrupt or reselection
 *	SCS_DMACOMPLETE		After starting DMA, waiting for completion
 *	SCS_SENDINGMSG		After sending an MSGO byte
 *	SCS_GETTINGMSG		While getting MSGI bytes
 *	SCS_SENDINGCMD		While sending CMDO bytes
 *	SCS_DEATH_MARCH		The target got lost. Follow phases until
 *				it disconnects.
 */
typedef enum {
	SCS_UNINITIALIZED,	/* initial state			*/
	SCS_DISCONNECTED,	/* disconnected				*/
	SCS_SELECTING,		/* SELECT command issued 		*/
	SCS_RESELECTING,	/* Handle reselection after interrrupt	*/
	SCS_INITIATOR,		/* following target SCSI phase		*/
	SCS_COMPLETING,		/* initiator command complete in progress */
	SCS_WAIT_FOR_BUS_FREE,	/* transition after disconnect or complete */
	SCS_DMACOMPLETE,	/* dma (in or out) is in progress	*/
	SCS_SENDINGMSG,		/* MSG_OUT phase in progress		*/
	SCS_GETTINGMSG,		/* transfer msg in progress		*/
	SCS_SENDINGCMD,		/* command out in progress		*/
	SCS_DEATH_MARCH		/* recovery from target confusion	*/
} BusState;


/* 
 * The message out state machine works as follows:
 * 1. When the driver wishes to send a message out, it:
 *	-- places the message in currMsgOut[]
 *	-- places the number of message bytes in currMsgOutCnt
 *	-- asserts ATN 
 *	-- sets msgOutState to kMsgOutWaiting
 *	All of the above are done by -messageOut for single-byte messages.
 * 2. When bus phase = PHASE_MSGOUT, the message in currMsgOut[] is 
 *	sent to the target in -fsmPhaseChange. msgOutState is then
 *	set to kMsgOutSawMsgOut.
 * 3. On the next phase change to other than PHASE_MSGOUT or PHASE_MSGIN,
 *	msgOutState is set to kMsgOutNone and currMsgOutCnt is set to 0.
 */
typedef enum MsgOutState {
	kMsgOutNone	= 0,	/* no message to send			*/
	kMsgOutWaiting,		/* have msg, awaiting MSG OUT phase	*/
	kMsgOutSawMsgOut	/* sent msg, check for retry		*/
} MsgOutState;
