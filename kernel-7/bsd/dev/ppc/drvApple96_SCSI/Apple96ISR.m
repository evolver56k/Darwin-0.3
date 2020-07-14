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
 * Apple96Chip.m - Chip-specific methods for Apple96 SCSI driver.
 *
 * Edit History
 * 1997.02.18	MM		Initial conversion from AMDPCSCSIDriver sources.
 * 1997.04.17	MM		Removed SCS_PHASECHANGE (not needed)
 */
#import "Apple96SCSI.h"
#import "Apple96ISR.h"
#import "Apple96BusState.h"
#import "Apple96Curio.h"
#import "Apple96CurioPublic.h"
#import "Apple96CurioPrivate.h"
#import "Apple96SCSIPrivate.h"
#import "MacSCSICommand.h"
#import "bringup.h"
#import <driverkit/generalFuncs.h>
#import <kernserv/prototypes.h>

IONamedValue scsiMsgValues[] = {
	{ kScsiMsgCmdComplete,			"Command Complete"		},
	{ kScsiMsgExtended,				"Extended Message"		},
	{ kScsiMsgSaveDataPointers,		"Save Pointers"			},
	{ kScsiMsgRestorePointers,		"Restore Pointers"		},
	{ kScsiMsgDisconnect,			"Disconnect"			},
	{ kScsiMsgInitiatorDetectedErr,	"Initiator Det Error"	},
	{ kScsiMsgAbort,				"Abort"					},
	{ kScsiMsgRejectMsg,			"Message Reject"		},
	{ kScsiMsgNop,					"Nop"					},
	{ kScsiMsgParityErr,			"Message parity Error"	},
	{ 0,							NULL					}
};

IONamedValue scsiPhaseValues[] = {
	{ kBusPhaseDATO,		"data out"				},
	{ kBusPhaseDATI,		"data in"				},
	{ kBusPhaseCMD,			"command"				},
	{ kBusPhaseSTS,			"status"				},
	{ kBusPhaseMSGO,		"message out"			},
	{ kBusPhaseMSGI,		"message in"			},
	{ kBusPhaseBusFree,		"bus free"				},
	{ 0,					NULL					}
};

/*
 * For IOFindNameForValue() and ddm's.
 */
IONamedValue gAutomatonStateValues[] = { 
	{SCS_UNINITIALIZED,		"uninitialized"					},
	{SCS_DISCONNECTED,		"disconnected"					},
	{SCS_SELECTING,			"selecting"						},
	{SCS_RESELECTING,		"reselection in progress"		},
	{SCS_INITIATOR,			"in progress, initiator" 		},
	{SCS_COMPLETING,		"completing command"			},
	{SCS_WAIT_FOR_BUS_FREE, "waiting for bus free"			},
	{SCS_DMACOMPLETE,		"waiting for dma completion"	},
	{SCS_SENDINGMSG,		"sending message bytes"			},
	{SCS_GETTINGMSG,		"receiving message bytes"		},
	{SCS_SENDINGCMD,		"sending command bytes"			},
	{SCS_DEATH_MARCH,		"error recovery"				},
	{0, 				NULL					},
};

@implementation Apple96_SCSI(InterruptService)

/*
 * SCSI device interrupt handler. We are called with interrupts disabled.
 * ** ** ** Check whether interrupts are actually disabled!
 */
- (void) hardwareInterrupt
{
		ENTRY("Hin hardwareInterrupt");
	if (CURIOinterruptPending()) {
			ddmChip("hardwareInterrupt entry: cmd %08x [%02x],"
			" status %02x, intr %02x, bus state: %s\n",
			gActiveCommand,
			(gCurrentTarget << 4) | gCurrentLUN,
			gSaveStatus,
			gSaveInterrupt,
			IOFindNameForValue(gBusState, gAutomatonStateValues)
		);
			/*
			 * There is an interrupt pending for this device. Note:
			 * because we can cycle through this sequence, it must
			 * be executed from a (high-priority) I/O thread, and not
			 * from a primary interrupt service routine.
			 */
			gFlagCheckForAnotherInterrupt = TRUE;
			do {
				gFlagNeedAnotherInterrupt = FALSE;
				ddmChip("hardwareInterrupt loop: cmd %08x [%02x],"
					" status %02x, intr %02x, bus state: %s\n",
					gActiveCommand,
					(gCurrentTarget << 4) | gCurrentLUN,
					gSaveStatus,
					gSaveInterrupt,
					IOFindNameForValue(gBusState, gAutomatonStateValues)
				);
				if ((gSaveInterrupt & sGrossErr) != 0) {
			/*
					 * Gross error is set incorrectly (according to
					 * Clinton Bauder).
			 */
		    ddmChip("Gross error: ignored, sts %02x, int %02x\n",
				gSaveStatus, gSaveInterrupt, 3, 4, 5);
					gSaveInterrupt &= ~sGrossErr;
				}
				if ((gSaveInterrupt & iIlegalCmd) != 0) {
			 	/*
				 * Software screwup (gross error). Start over from scratch.
				 * We can get this if we write too many commands into the register.
				 */
					[self logRegisters : FALSE reason : "Illegal command interrupt"];
					if (gBusState != SCS_DEATH_MARCH) {
						[self fsmStartErrorRecovery
								: SR_IOST_INT
									reason	: "Illegal command interrupt"
				];
			}
					gFlagCheckForAnotherInterrupt = FALSE;
				} /* If chip error */
				if ((gSaveInterrupt & iDisconnect) != 0) {
					ddmChip("hardwareInterrupt: enabling reselection\n", 1,2,3,4,5);
		    CURIOenableSelectionOrReselection();
					/*
					 * Radar 1678545: this is the only (normal) place
					 * that gFlagBusBusy is cleared. (It's also cleared by
					 * bus reset and driver initialization.)
					 */
					gFlagBusBusy = FALSE;
				}
				if ((gSaveInterrupt & iResetDetect) != 0) {
				IOLog("%s: SCSI Bus Reset\n", [self name]);
				/*
				 * While we would like to abort all pending and active commands,
				 * we can't do this without taking the pending command lock,
				 * which can, conceivably, lead to a race condition. If this
				 * (quick and dirty) analysis is incorrect, enable the following:
				 *		[self abortAllCommands : SR_IOST_RESET];
				 */
				[self curioHardwareReset
								: FALSE
						reason	: "SCSI Bus Reset"
				];
					gFlagCheckForAnotherInterrupt = FALSE;
				} /* If bus reset */
				else if ((gSaveStatus & sParityErr) != 0 && gFlagBusBusy) {
					[self fsmStartErrorRecovery
								: SR_IOST_PARITY
						reason	: "SCSI bus parity error"
				];
					gFlagCheckForAnotherInterrupt = FALSE;
				} /* If device parity error and not disconnected */
			else {
				/*
					 * Only certain states are legal if the bus is busy.
					 */
					if (gFlagBusBusy) {
						/*
				 * This is a legitimate interrupt (parity error and bus reset
				 * have already been handled). The entity that started the
				 * chip action that caused the interrupt (deep breath)
				 * set gBusState to indicate why the interrupt happened.
				 * Call the proper finite-state machine function. On return,
				 * gBusState will be set to one of the following values:
				 *	SCS_INITIATOR		Start an action on the current phase.
					 *	SCS_DISCONNECT			The bus is free.
					 *	SCS_WAIT_FOR_BUS_FREE	The bus should go free shortly.
					 * Note that SCS_WAIT_FOR_BUS_FREE and fsmWaitForBusFree
					 * short-circuit some of the automaton to avoid unnecessary
					 * interrupt events.
				 */
				switch (gBusState) {
				case SCS_DISCONNECTED:	[self fsmDisconnected];		break;
				case SCS_SELECTING:		[self fsmSelecting];		break;
						case SCS_RESELECTING:		[self fsmReselecting];				break;
				case SCS_INITIATOR:		[self fsmInitiator];		break;
				case SCS_COMPLETING:	[self fsmCompleting];		break;
				case SCS_DMACOMPLETE:	[self fsmDMAComplete];		break;
				case SCS_SENDINGMSG:	[self fsmSendingMsg];		break;
				case SCS_GETTINGMSG:	[self fsmGettingMsg];		break;
				case SCS_SENDINGCMD:	[self fsmSendingCmd];		break;
					case SCS_WAIT_FOR_BUS_FREE:	[self fsmWaitForBusFree];			break;
					case SCS_DEATH_MARCH:		[self fsmErrorRecoveryInterruptService];	break;
				case SCS_UNINITIALIZED:	/* Illegal here */
				default:
					IOLog("%s: Bug: illegal interrupt state: %s\n",
						[self name],
						IOFindNameForValue(gSaveStatus & mPhase, scsiPhaseValues)
					);
					IOPanic("Apple96 SCSI: Illegal bus automaton state.");
				} /* switch gBusState */
					}
					else {
					/*
						 * The bus is (or just went) free. We only allow selection, reselection,
						 * wait for disconnect, or death march interrupts.
					 */
						switch (gBusState) {
						case SCS_DISCONNECTED:		[self fsmDisconnected];				break;
						case SCS_SELECTING:			[self fsmSelecting];				break;
						case SCS_WAIT_FOR_BUS_FREE:	[self fsmWaitForBusFree];			break;
						case SCS_DEATH_MARCH:		[self fsmErrorRecoveryInterruptService];	break;
						default:
							[self fsmStartErrorRecovery
												: SR_IOST_HW
										reason	: "Strange SCSI state when bus free"
							];
							gFlagCheckForAnotherInterrupt = FALSE;
						} /* switch gBusState */
				}
			}
			/*
			 * We have (presumably) completely handled the previous interrupt.
				 * At this point, there are five legitimate gBusState values:
				 *	SCS_RESELECTING			Just got a reselection interrupt
			 *	SCS_INITIATOR		Continue operation for this target.
				 *	SCS_DISCONNECTED		Enable selection/reselection
				 *	SCS_WAIT_FOR_BUS_FREE	Command completion transition
				 *	SCS_DEATH_MARCH			Error recovery
			 * Handle a SCSI Phase change if necessary. This will leave
			 * the bus state in the "expected" state for the next operation.
			 */
				switch (gBusState) {
				case SCS_DEATH_MARCH:
					gFlagCheckForAnotherInterrupt = FALSE;
					break;
				case SCS_RESELECTING:
					[self fsmReselectionAction];
					break;
				case SCS_INITIATOR:
					if ( gFlagNeedAnotherInterrupt )	// from tagged reselect
					{
						gFlagNeedAnotherInterrupt = FALSE;
						if ( [ self curioQuickCheckForChipInterrupt ] )
							 [ self fsmPhaseChange ];
					}
					else
					{
						[ self fsmPhaseChange ];
					}
					break;
				case SCS_WAIT_FOR_BUS_FREE:
					break;
				default:
					break;
			}
			/*
				 * This is the final check: if we determine that the previous action will
				 * complete quickly (for example, it's a message in byte), we'll spin for
				 * up to ten microseconds to see if the chip is ready for another operation.
			 */
	    } while (gFlagCheckForAnotherInterrupt && CURIOquickCheckForChipInterrupt());
			if (gBusState == SCS_DEATH_MARCH) {
				[self fsmContinueErrorRecovery];
			}
			if (gBusState == SCS_DISCONNECTED) {
				[self busFree];
			}
			ddmChip("hardwareInterrupt: DONE; state: %s, phase %s\n", 
				IOFindNameForValue(gBusState, gAutomatonStateValues),
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues),
				3,4,5
			);
		}
		EXIT();
}

@end	/* Apple96_SCSI(InterruptService) */

/* end of Apple96_Chip.m */

