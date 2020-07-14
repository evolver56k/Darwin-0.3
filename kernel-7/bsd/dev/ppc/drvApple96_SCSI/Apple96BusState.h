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
 * @revision	1997.02.17	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96BusState.h - Curio-specific methods for Apple96 SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.18	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */
 
#import "Apple96SCSI.h"

@interface Apple96_SCSI(BusState)

/*
 * Methods invoked upon interrupt. One per legal gBusState.
 * These are actually private and are only called by hardwareInterrupt.
 */
- (void)			fsmDisconnected;
- (void)			fsmHandleReselectionInterrupt;	/* Subroutine to fsmDisconnected */
- (void)			fsmSelecting;
- (void)			fsmReselecting;
- (void)			fsmReselectionAction;
- (void)			fsmInitiator;
- (void)			fsmCompleting;
- (void)			fsmWaitForBusFree;
- (void)			fsmDMAComplete;
- (void)			fsmSendingMsg;
- (void)			fsmSelecting;
- (void)			fsmGettingMsg;
- (IOReturn)		fsmProcessMessage;
- (void)			fsmSelecting;
- (void)			fsmSendingCmd;
- (void)			fsmStartErrorRecovery
						: (sc_status_t) status
			reason		: (const char *) reason;
- (void)			fsmErrorRecoveryInterruptService;
- (void)			fsmContinueErrorRecovery;

- (void)			putMessageOutByte
						: (UInt8)	messageByte
			setATN		: (Boolean) setATN;

/*
 * This is called after an interrupt leaves us as SCS_RESELECTING
 */
- (void)			fsmReselectionAction;
/*
 * This is is called after an interrupt leaves us as SCS_INITIATOR. 
 */
- (void)			fsmPhaseChange;
/*
 * Common code (from fsmPhaseChange) to start DATI or DATO transfer.
 */
- (void)			fsmStartDataPhase;
/*
 * Disable specified mode for activeCmd's target.
 */
 
typedef enum {
	kTargetModeCmdQueue
} TargetMode;	

- (void)			disableMode
						: (TargetMode) mode;

/**
 * Validate the target's reselection byte (put on the bus before
 * reselecting us). Erase the initiator ID and convert the other
 * bit into an index. The algorithm should be faster than a
 * sequential search, but it probably doesn't matter much.
 * @return	IO_R_SUCCESS if successful (gCurrentTarget is now valid).
 *			SR_IOST_BV if the select byte is incorrect.
 * This function does not check whether there actually
 * is a command pending for this target.
 */
- (IOReturn) getReselectionTargetID
						: (UInt8) selectByte;

@end
