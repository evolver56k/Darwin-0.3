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

/*
	File:		OHCIUIM.h

	Contains:	Header file for OHCIUIM.c

	Version:	xxx put version here xxx

	Written by:	Guillermo Gallegos

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(DRF)	Dave Falkenburg
		(BG)	Bill Galcher
		(DF)	David Ferguson
		(DKF)	David Ferguson
		(GG)	Guillermo Gallegos
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB41>	  9/9/98	BT		Fix size of refcon in Isoc transfers
	 <USB40>	  9/3/98	GG		Add Isochronous Suppurt for Create, Abort, Delete endpoint and
									create transfers.
	 <USB39>	 8/12/98	BT		Move root hub back into UIM again.
	 <USB38>	  8/7/98	GG		Fixed Opti data corruption problem.
	 <USB37>	 7/10/98	TC		Back out <USB35>
	 <USB36>	 6/30/98	BT		Tidy up
	 <USB35>	 6/30/98	BT		Move Root hub sim into UIM
	 <USB34>	 6/14/98	DF		Add more Volatile keywords to the globals
	 <USB33>	  6/4/98	DRF		In UIMInitialize & UIMFinalize, add a Ptr to facilitate
									handing-off global state when performing replacement.
	 <USB32>	  6/3/98	GG		Added kOHCIFrameOverflowBit for frame overflows.
	 <USB31>	  6/2/98	GG		Add interfaces for UIMFrameCount, IsochEndpointCreate, and
									IsochTransfer.
	 <USB30>	  6/2/98	DRF		Added params to UIMInitialize & UIMFinalize for replacement.
	 <USB29>	 5/18/98	BG		Remove comma from final entry in enums so that they compile with
									no complaints from MrC.
	 <USB28>	 5/15/98	DF		keep last pointer to the various descriptor free lists
	 <USB27>	 5/14/98	GG		Added some constants to support Buffer Underrun Errata fix.
	 <USB26>	 5/14/98	DF		Add retryBufferUnderrun errata
	 <USB25>	 5/12/98	BT		fix spare comma
	 <USB24>	  5/5/98	GG		Change Buffersize from short to unsigned long in
									uimBulkTransfer.
	 <USB23>	  5/2/98	DF		Add volatile keyboard for variables updated by the interrupt
									handler, also added stuff for erratas
	 <USB22>	 4/29/98	BT		Move common errors to USBpriv
	 <USB21>	 4/24/98	GG		Added support for bulk transfers greater than 4k.  Added fix to
									support to clear an endpoint stall.
	 <USB20>	  4/9/98	BT		Use USB.h
		<19>	  4/8/98	GG		Added Abort and Delete APIs.
		<18>	  4/7/98	GG		Added Abort and delete apis.
		<17>	 3/19/98	BT		Split UIM into UIM and root hub parts
		<16>	 3/18/98	BT		Add reset enable change to root hub.
		<15>	 3/18/98	GG		Added some constants
		<14>	 3/11/98	BT		More int transaction simualtion
		<13>	 2/25/98	GG		Added kOHCIUIMUiniqueNoDir.
		<12>	 2/23/98	GG		Moved interrupt structure, changed OHCIData struct to hold array
									of InterruptHeads.
		<11>	 2/20/98	GG		Added Interrupt Transfer Support.
		<10>	 2/19/98	GG		Added Endpoint Creation Functionality.
	  <USB9>	 2/17/98	DKF		Add HW Interrupt related definitions and globals
		 <8>	 2/12/98	GG		Added Rom in Ram Memory management support.
		 <7>	  2/2/98	BT		Add bulk stuff
		 <6>	 1/26/98	BT		Hack in clear enpoint stall
		 <5>	12/22/97	CJK		Add include of UIMSupport.h
		 <4>	12/19/97	BT		Temp hack, Make it a shared lib
		 <4>	30/11/97	BT		Various mods to get USL preveiw running
		<2*>	11/20/97	GG		Add callback parameter to bulk and control creators.
		 <2>	11/20/97	GG		Filled in contains field. Initial check in has minimal support
									for Root Hub, Control and Bulk transfers.
		 <1>	11/20/97	GG		first checked in

*/

/* IMPORTANT NOTES:
   The generic UIM structures and typedefs needed by the Family Expert
   have been moved to the UIMSupport.h file in the Neptune:Interfaces
   folder.  Any UIM typedefs or structs that are not OHCI specific
   should be kept there, so that the family expert, etc., can have
   access to them without needing to be hardware saavy.  
   
   While that's the goal, the current reality is that some roothub
   support is performed in the USL, with special roothub dispatch
   entries needed for root hub functions.  So until the roothub
   simulation is implement in the UIM, you will find a small amount
   of USL & OHCI related items in UIMSupport.  These non-generic
   UIM thangs will need to be removed when roothub simulation
   moves from the USL to the UIM.
*/

#ifndef __OHCIUIM__
#define __OHCIUIM__

#ifndef __TYPES__
//#include "types.h"
#endif
#ifndef __INTERRUPTS__
//#include "interrupts.h"
#endif

#include "USBpriv.h"

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=power
#endif

/*zzz*/
/* Isn't this PCI standard stuff?  Shouldn't it be in some regular include */
/* file like PCI.h? */
//#include <PCI.h>

#define bit0			0x00000001
#define bit1			0x00000002
#define bit2			0x00000004
#define bit3			0x00000008
#define bit4			0x00000010
#define bit5			0x00000020
#define bit6			0x00000040
#define bit7			0x00000080
#define bit8			0x00000100
#define bit9			0x00000200
#define bit10			0x00000400
#define bit11			0x00000800
#define bit12			0x00001000
#define bit13			0x00002000
#define bit14			0x00004000
#define bit15			0x00008000
#define bit16			0x00010000
#define bit17			0x00020000
#define bit18			0x00040000
#define bit19			0x00080000
#define bit20			0x00100000
#define bit21			0x00200000
#define bit22			0x00400000
#define bit23			0x00800000
#define bit24			0x01000000
#define bit25			0x02000000
#define bit26			0x04000000
#define bit27			0x08000000
#define bit28			0x10000000
#define bit29			0x20000000
#define bit30			0x40000000
#define bit31			0x80000000

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * *
 * Configuration Registers
 *
 */
enum {
	kConfigStart		= 0x00,
	cwVendorID			= 0x00,			/* 0x1000									*/
	cwDeviceID			= 0x02,			/* 0x0003									*/
	cwCommand			= 0x04,
	cwStatus			= 0x06,
	clClassCodeAndRevID	= 0x08,
	clHeaderAndLatency	= 0x0C,
	clBaseAddressZero	= 0x10,			/* I/O Base address							*/
	clBaseAddressOne	= 0x14,			/* Memory Base address						*/
	clExpansionRomAddr	= 0x30,
	clLatGntIntPinLine	= 0x3C,			/* Max_Lat, Max_Gnt, Int. Pin, Int. Line	*/
	kConfigEnd			= 0x40
};

/*
 * 0x04 cwCommand					Command Register (read/write)
 */
enum {
	cwCommandSERREnable			= bit8,
	cwCommandEnableParityError	= bit6,
	cwCommandEnableBusMaster	= bit2,		/* Set this on initialization			*/
	cwCommandEnableMemorySpace	= bit1,		/* Respond at Base Address One if set	*/
	cwCommandEnableIOSpace		= bit0		/* Respond at Base Address Zero if set	*/
};
/*
 * 0x06 cwStatus					Status Register (read/write)
 */
enum {
	cwStatusDetectedParityError	= bit15,	/* Detected from slave					*/
	cwStatusSignaledSystemError = bit14,	/* Device asserts SERR/ signal			*/
	cwStatusMasterAbort		 	= bit13,	/* Master sets when transaction aborts	*/
	cwStatusReceivedTargetAbort	= bit12,	/* Master sets when target-abort		*/
	cwStatusDEVSELTimingMask	= (bit10 | bit9),	/* DEVSEL timing encoding R/O	*/
	 cwStatusDEVSELFastTiming	= 0,
	 cwStatusDEVSELMediumTiming	= bit9,
	 cwStatusDEVSELSlowTiming	= bit10,
	cwStatusDataParityReported	= bit8
};

/*zzz*/


////////////////////////////////////////////////////////////////////////////////
//
// Useful macro defs.
//

#define EndianSwapImm32Bit(data32)					\
(													\
	(((UInt32) data32) >> 24)				|		\
	((((UInt32) data32) >> 8) & 0xFF00)		|		\
	((((UInt32) data32) << 8) & 0xFF0000)	|		\
	(((UInt32) data32) << 24)						\
)

////////////////////////////////////////////////////////////////////////////////
//
// OHCI type defs.
//

typedef volatile struct OHCIRegistersStruct
								OHCIRegisters,
								*OHCIRegistersPtr;

typedef struct OHCIIntHeadStruct
								OHCIIntHead,
								*OHCIIntHeadPtr;

typedef struct OHCIEndpointDescriptorStruct
								OHCIEndpointDescriptor,
								*OHCIEndpointDescriptorPtr;

typedef struct OHCIGeneralTransferDescriptorStruct
								OHCIGeneralTransferDescriptor,
								*OHCIGeneralTransferDescriptorPtr;

typedef struct OHCIIsochTransferDescriptorStruct
								OHCIIsochTransferDescriptor,
								*OHCIIsochTransferDescriptorPtr;

typedef struct OHCIUIMDataStruct
								OHCIUIMData, *OHCIUIMDataPtr;
								
typedef struct OHCIPhysicalLogicalStruct
								OHCIPhysicalLogical,
								*OHCIPhysicalLogicalPtr;

typedef struct ErrataListEntryStruct 
								ErrataListEntry, 
								*ErrataListEntryPtr;
								
////////////////////////////////////////////////////////////////////////////////
//
// OHCI type defs.
// (moved to UIMSupport.h so that the Expert can use them as well.)

////////////////////////////////////////////////////////////////////////////////
//
// OHCI register file.
//

enum
{
	kOHCIBit0					= (1 << 0),
	kOHCIBit1					= (1 << 1),
	kOHCIBit2					= (1 << 2),
	kOHCIBit3					= (1 << 3),
	kOHCIBit4					= (1 << 4),
	kOHCIBit5					= (1 << 5),
	kOHCIBit6					= (1 << 6),
	kOHCIBit7					= (1 << 7),
	kOHCIBit8					= (1 << 8),
	kOHCIBit9					= (1 << 9),
	kOHCIBit10					= (1 << 10),
	kOHCIBit11					= (1 << 11),
	kOHCIBit12					= (1 << 12),
	kOHCIBit13					= (1 << 13),
	kOHCIBit14					= (1 << 14),
	kOHCIBit15					= (1 << 15),
	kOHCIBit16					= (1 << 16),
	kOHCIBit17					= (1 << 17),
	kOHCIBit18					= (1 << 18),
	kOHCIBit19					= (1 << 19),
	kOHCIBit20					= (1 << 20),
	kOHCIBit21					= (1 << 21),
	kOHCIBit22					= (1 << 22),
	kOHCIBit23					= (1 << 23),
	kOHCIBit24					= (1 << 24),
	kOHCIBit25					= (1 << 25),
	kOHCIBit26					= (1 << 26),
	kOHCIBit27					= (1 << 27),
	kOHCIBit28					= (1 << 28),
	kOHCIBit29					= (1 << 29),
	kOHCIBit30					= (1 << 30),
	kOHCIBit31					= (1 << 31)
};

#define OHCIBitRange(start, end)					\
(													\
	((((UInt32) 0xFFFFFFFF) << (31 - (end))) >>		\
	 ((31 - (end)) + (start))) <<					\
	(start)											\
)

#define OHCIBitRangePhase(start, end)				\
	(start)


// OHCI register file.

struct OHCIRegistersStruct
{
	// Control and status group.
	volatile UInt32				hcRevision;
	volatile UInt32				hcControl;
	volatile UInt32				hcCommandStatus;
	volatile UInt32				hcInterruptStatus;
	volatile UInt32				hcInterruptEnable;
	volatile UInt32				hcInterruptDisable;

	// Memory pointer group.
	volatile UInt32				hcHCCA;
	volatile UInt32				hcPeriodCurrentED;
	volatile UInt32				hcControlHeadED;
	volatile UInt32				hcControlCurrentED;
	volatile UInt32				hcBulkHeadED;
	volatile UInt32				hcBulkCurrentED;
	volatile UInt32				hcDoneHead;

	// Frame counter group.
	volatile UInt32				hcFmInterval;
	volatile UInt32				hcFmRemaining;
	volatile UInt32				hcFmNumber;
	volatile UInt32				hcPeriodicStart;
	volatile UInt32				hcLSThreshold;

	// Root hub group.
	volatile UInt32				hcRhDescriptorA;
	volatile UInt32				hcRhDescriptorB;
	volatile UInt32				hcRhStatus;
	volatile UInt32				hcRhPortStatus[1];
};

// hcControl register defs.
enum
{
	kOHCIHcControl_CBSR			= OHCIBitRange (0, 1),
	kOHCIHcControl_CBSRPhase	= OHCIBitRangePhase (0, 1),
	kOHCIHcControl_PLE			= kOHCIBit2,
	kOHCIHcControl_IE			= kOHCIBit3,
	kOHCIHcControl_CLE			= kOHCIBit4,
	kOHCIHcControl_BLE			= kOHCIBit5,
	kOHCIHcControl_HCFS			= OHCIBitRange (6, 7),
	kOHCIHcControl_HCFSPhase	= OHCIBitRangePhase (6, 7),
	kOHCIHcControl_IR			= kOHCIBit8,
	kOHCIHcControl_RWC			= kOHCIBit9,
	kOHCIHcControl_RWE			= kOHCIBit10,

	kOHCIHcControl_Reserved		= OHCIBitRange (11, 31),

	kOHCIFunctionalState_Reset			= 0,
	kOHCIFunctionalState_Resume			= 1,
	kOHCIFunctionalState_Operational	= 2,
	kOHCIFunctionalState_Suspend		= 3
};

// hcCommandStatus register defs.
enum
{
	kOHCIHcCommandStatus_HCR		= kOHCIBit0,
	kOHCIHcCommandStatus_CLF		= kOHCIBit1,
	kOHCIHcCommandStatus_BLF		= kOHCIBit2,
	kOHCIHcCommandStatus_OCR		= kOHCIBit3,
	kOHCIHcCommandStatus_SOC		= OHCIBitRange (16, 17),
	kOHCIHcCommandStatus_SOCPhase	= OHCIBitRangePhase (16, 17),

	kOHCIHcCommandStatus_Reserved	= OHCIBitRange (4, 15) | OHCIBitRange (18, 31)
};

// hcInterrupt register defs.
enum
{
	kOHCIHcInterrupt_SO				= kOHCIBit0,
	kOHCIHcInterrupt_WDH			= kOHCIBit1,
	kOHCIHcInterrupt_SF				= kOHCIBit2,
	kOHCIHcInterrupt_RD				= kOHCIBit3,
	kOHCIHcInterrupt_UE				= kOHCIBit4,
	kOHCIHcInterrupt_FNO			= kOHCIBit5,
	kOHCIHcInterrupt_RHSC			= kOHCIBit6,
	kOHCIHcInterrupt_OC				= kOHCIBit30,
	kOHCIHcInterrupt_MIE			= kOHCIBit31
};

// this is what I would like it to be
//#define kOHCIDefaultInterrupts		(kOHCIHcInterrupt_SO | kOHCIHcInterrupt_WDH | kOHCIHcInterrupt_UE | kOHCIHcInterrupt_FNO | kOHCIHcInterrupt_RHSC)
#define kOHCIDefaultInterrupts		(kOHCIHcInterrupt_WDH | kOHCIHcInterrupt_UE | kOHCIHcInterrupt_FNO)
	

// hcFmInterval register defs.
enum
{
	kOHCIHcFmInterval_FI			= OHCIBitRange (0, 13),
	kOHCIHcFmInterval_FIPhase		= OHCIBitRangePhase (0, 13),
	kOHCIHcFmInterval_FSMPS			= OHCIBitRange (16, 30),
	kOHCIHcFmInterval_FSMPSPhase	= OHCIBitRangePhase (16, 30),
	kOHCIHcFmInterval_FIT			= kOHCIBit31,

	kOHCIHcFmInterval_Reserved		= OHCIBitRange (14, 15)
};

// hcRhDescriptorA register defs.
enum
{
	kOHCIHcRhDescriptorA_NDP			= OHCIBitRange (0, 7),
	kOHCIHcRhDescriptorA_NDPPhase		= OHCIBitRangePhase (0, 7),
	kOHCIHcRhDescriptorA_PSM			= kOHCIBit8,
	kOHCIHcRhDescriptorA_NPS			= kOHCIBit9,
	kOHCIHcRhDescriptorA_DT				= kOHCIBit10,
	kOHCIHcRhDescriptorA_OCPM			= kOHCIBit11,
	kOHCIHcRhDescriptorA_NOCP			= kOHCIBit12,
	kOHCIHcRhDescriptorA_POTPGT			= OHCIBitRange (24, 31),
	kOHCIHcRhDescriptorA_POTPGTPhase	= OHCIBitRangePhase (24, 31),

	kOHCIHcRhDescriptorA_Reserved		= OHCIBitRange (13,23)
};

// Root hub sttaus reg

// Barry, not moved to OHCIRootHub.h


// Port status reg 

// Barry, not moved to OHCIRootHub.h



// Config space defs.
enum
{
	kOHCIConfigRegBaseAddressRegisterNumber	= 0x10
};


enum
{
	kOHCIEDControl_FA			= OHCIBitRange (0,  6),
	kOHCIEDControl_FAPhase		= OHCIBitRangePhase (0, 6),
	kOHCIEDControl_EN			= OHCIBitRange (7, 10),
	kOHCIEDControl_ENPhase		= OHCIBitRangePhase (7, 10),
	kOHCIEDControl_D			= OHCIBitRange (11, 12),
	kOHCIEDControl_DPhase		= OHCIBitRangePhase (11, 12),
	kOHCIEDControl_S			= OHCIBitRange (13, 13),
	kOHCIEDControl_SPhase		= OHCIBitRangePhase (13, 13),
	kOHCIEDControl_K			= kOHCIBit14,
	kOHCIEDControl_F			= OHCIBitRange (15, 15),
	kOHCIEDControl_FPhase		= OHCIBitRangePhase (15, 15),
	kOHCIEDControl_MPS			= OHCIBitRange (16, 26),
	kOHCIEDControl_MPSPhase		= OHCIBitRangePhase (16, 26),

	kOHCITailPointer_tailP		= OHCIBitRange (4, 31),
	kOHCITailPointer_tailPPhase	= OHCIBitRangePhase (4, 31),

	kOHCIHeadPointer_H			= kOHCIBit0,
	kOHCIHeadPointer_C			= kOHCIBit1,
	kOHCIHeadPointer_headP		= OHCIBitRange (4, 31),
	kOHCIHeadPointer_headPPhase	= OHCIBitRangePhase (4, 31),

	kOHCINextEndpointDescriptor_nextED		= OHCIBitRange (4, 31),
	kOHCINextEndpointDescriptor_nextEDPhase	= OHCIBitRangePhase (4, 31),

	kOHCIEDDirectionTD			= 0,
	kOHCIEDDirectionOut			= 1,
	kOHCIEDDirectionIn			= 2,

	kOHCIEDSpeedFull			= 0,
	kOHCIEDSpeedLow				= 1,

	kOHCIEDFormatGeneralTD		= 0,
	kOHCIEDFormatIsochronousTD	= 1
};

// General Transfer Descriptor
enum
{
	kOHCIGTDControl_R			= kOHCIBit18,
	kOHCIGTDControl_DP			= OHCIBitRange (19, 20),
	kOHCIGTDControl_DPPhase		= OHCIBitRangePhase (19, 20),
	kOHCIGTDControl_DI			= OHCIBitRange (21, 23),
	kOHCIGTDControl_DIPhase		= OHCIBitRangePhase (21, 23),
	kOHCIGTDControl_T			= OHCIBitRange (24, 25),
	kOHCIGTDControl_TPhase		= OHCIBitRangePhase (24, 25),
	kOHCIGTDControl_EC			= OHCIBitRange (26, 27),
	kOHCIGTDControl_ECPhase		= OHCIBitRangePhase (26, 27),
	kOHCIGTDControl_CC			= OHCIBitRange (28, 31),
	kOHCIGTDControl_CCPhase		= OHCIBitRangePhase (28, 31),

	kOHCIGTDPIDSetup			= 0,
	kOHCIGTDPIDOut				= 1,
	kOHCIGTDPIDIn				= 2,

	kOHCIGTDNoInterrupt			= 7,

	kOHCIGTDDataToggleCarry		= 0,
	kOHCIGTDDataToggle0			= 2,
	kOHCIGTDDataToggle1			= 3,

	kOHCIGTDConditionNoError				= 0,
	kOHCIGTDConditionCRC					= 1,
	kOHCIGTDConditionBitStuffing			= 2,
	kOHCIGTDConditionDataToggleMismatch		= 3,
	kOHCIGTDConditionStall					= 4,
	kOHCIGTDConditionDeviceNotResponding	= 5,
	kOHCIGTDConditionPIDCheckFailure		= 6,
	kOHCIGTDConditionUnexpectedPID			= 7,
	kOHCIGTDConditionDataOverrun			= 8,
	kOHCIGTDConditionDataUnderrun			= 9,
	kOHCIGTDConditionBufferOverrun			= 12,
	kOHCIGTDConditionBufferUnderrun			= 13,
	kOHCIGTDConditionNotAccessed			= 15
};

// Isochronous Transfer Descriptor
enum
{
	kOHCIITDControl_SF						= OHCIBitRange (0,15),
	kOHCIITDControl_SFPhase					= OHCIBitRangePhase(0,15),				
	kOHCIITDControl_DI						= OHCIBitRange (21,23),
	kOHCIITDControl_DIPhase					= OHCIBitRangePhase (21,23),
	kOHCIITDControl_FC						= OHCIBitRange (24,26),
	kOHCIITDControl_FCPhase					= OHCIBitRangePhase (24,26),
	kOHCIITDControl_CC						= OHCIBitRange (28,31),
	kOHCIITDControl_CCPhase					= OHCIBitRangePhase (28,31),
	kOHCIITDPSW_Size						= OHCIBitRange(0,10),
	kOHCIITDPSW_SizePhase					= OHCIBitRangePhase(0,10),
	kOHCIITDPSW_CC							= OHCIBitRange(12,15),
	kOHCIITDPSW_CCPhase 					= OHCIBitRangePhase(12,15),
	kOHCIITDPSW_CCNA						= OHCIBitRange(13,15),
	kOHCIITDPSW_CCNAPhase 					= OHCIBitRangePhase(13,15),
	kOHCIITDOffset_Size						= OHCIBitRange(0,10),
	kOHCIITDOffset_SizePhase				= OHCIBitRangePhase(0,10),
	kOHCIITDOffset_PC						= OHCIBitRange(12,12),
	kOHCIITDOffset_PCPhase					= OHCIBitRangePhase(12,12),
	kOHCIITDOffset_CC						= OHCIBitRange(13,15),
	kOHCIITDOffset_CCPhase 					= OHCIBitRangePhase(13,15),
	kOHCIITDConditionNoError				= 0,
	kOHCIITDConditionCRC					= 1,
	kOHCIITDConditionBitStuffing			= 2,
	kOHCIITDConditionDataToggleMismatch		= 3,
	kOHCIITDConditionStall					= 4,
	kOHCIITDConditionDeviceNotResponding	= 5,
	kOHCIITDConditionPIDCheckFailure		= 6,
	kOHCIITDConditionUnExpectedPID			= 7,
	kOHCIITDConditionDataOverrun			= 8,
	kOHCIITDConditionDataUnderrun			= 9,
	kOHCIITDConditionBufferOverrun			= 12,
	kOHCIITDConditionBufferUnderrun			= 13,
	kOHCIITDConditionNotAccessed			= 7,
	kOHCIITDConditionNotAccessedReturn		= 15,
	kOHCIITDConditionNotCrossPage			= 0,
	kOHCIITDConditionCrossPage				= 1
};



// misc definitions -- most of these need to be cleaned up/replaced with better terms defined previously

enum
{
// Barry, note - Root hub defines moved to OHCIRootHub.h

	kOHCIEndpointNumberOffset		= 7,
	kOHCIEndpointDirectionOffset	= 11,
	kOHCIMaxPacketSizeOffset		= 16,
	kOHCISpeedOffset				= 13,
	kOHCIBufferRoundingOffset		= 18,
	kOHCIDirectionOffset			= 19,
	kENOffset						= 7,

	kUniqueNumMask					= OHCIBitRange (0, 12),
	kUniqueNumNoDirMask				= OHCIBitRange (0, 10),
	kOHCIHeadPMask					= OHCIBitRange (4, 31),
	kOHCIInterruptSOFMask			= kOHCIHcInterrupt_SF,
	kOHCISkipped					= kOHCIEDControl_K,
	kOHCIDelayIntOffset				= 21,
	kOHCIMaxPages					= 20,			//arbitrary value for #of pages to use in prepare memory for io
	kOHCIPageSize					= 4096,
	kOHCIEndpointDirectionMask		= OHCIBitRange (11, 12),
	kOHCIEDToggleBitMask 			= OHCIBitRange (1, 1),
	kOHCIGTDClearErrorMask			= OHCIBitRange (0, 25)
};


// errataBits contents
enum {
	kErrataCMDDisableTestMode		= kOHCIBit0,		// turn off UHCI test mode
	kErrataOnlySinglePageTransfers	= kOHCIBit1,		// Don't cross page boundaries in a single transfer
	kErrataRetryBufferUnderruns		= kOHCIBit2,		// UIM will retry out transfers with buffer underrun errors
	kErrataLSHSOpti					= kOHCIBit3			// UIM will insert delay buffer between HS and LS transfers
};

enum {
	kOHCIBulkTransferOutType		= 1,
	kOHCIBulkTransferInType			= 2,
	kOHCIControlSetupType			= 3,
	kOHCIControlDataType			= 4,
	kOHCIControlStatusType 			= 5,
	kOHCIInterruptInType			= 6,
	kOHCIInterruptOutType			= 7,
	kOHCIOptiLSBug					= 8,
	kOHCIIsochronousType			= 9
};

enum {
	kOHCIFrameOffset				= 16,
	kOHCIFmNumberMask				= OHCIBitRange (0, 15),
	kOHCIFrameOverflowBit			= kOHCIBit16,
	kOHCIMaxRetrys					= 20

};

////////////////////////////////////////////////////////////////////////////////
//
// OHCI UIM data records.
//

typedef short RootHubID;

// Interrupt head struct
struct OHCIIntHeadStruct
{
	OHCIEndpointDescriptorPtr				pHead;
	OHCIEndpointDescriptorPtr				pTail;
	UInt32									pHeadPhysical;
	int										nodeBandwidth;
};
//naga
typedef int ohciRegEntryID;
struct OHCIUIMDataStruct
{
	RegEntryID									ohciRegEntryID;			// Name Registry entry of OHCI.
//	UIMID										uimID;					// ID for OHCI UIM.
	RootHubID									rootHubID;				// Status of root hub, if 0, not initialized otherwise has virtual ID number
	UInt32										errataBits;				// various bits for chip erratas

	OHCIRegistersPtr							pOHCIRegisters;			// Pointer to base address of OHCI registers.
	Ptr											pHCCA,					// Pointer to HCCA.
												pHCCAAllocation;		// Pointer to memory allocated for HCCA.
	OHCIIntHead									pInterruptHead[63];		// ptr to private list of all interrupts heads 			
	volatile UInt32								pIsochHead;				// ptr to Isochtonous list
	volatile UInt32								pIsochTail;				// ptr to Isochtonous list
	volatile UInt32								pBulkHead;				// ptr to Bulk list
	volatile UInt32								pControlHead;			// ptr to Control list
	volatile UInt32								pBulkTail;				// ptr to Bulk list
	volatile UInt32								pControlTail;			// ptr to Control list
	volatile OHCIPhysicalLogicalPtr				pPhysicalLogical;		// ptr to list of memory maps
	volatile OHCIGeneralTransferDescriptorPtr	pFreeTD;				// list of availabble Trasfer Descriptors
	volatile OHCIIsochTransferDescriptorPtr		pFreeITD;				// list of availabble Trasfer Descriptors
	volatile OHCIEndpointDescriptorPtr			pFreeED;				// list of available Endpoint Descriptors
	volatile OHCIGeneralTransferDescriptorPtr	pLastFreeTD;			// last of availabble Trasfer Descriptors
	volatile OHCIIsochTransferDescriptorPtr		pLastFreeITD;			// last of availabble Trasfer Descriptors
	volatile OHCIEndpointDescriptorPtr			pLastFreeED;			// last of available Endpoint Descriptors
	volatile OHCIGeneralTransferDescriptorPtr	pPendingTD;				// list of non processed Trasfer Descriptors

	UInt32										pageSize;				// OS Logical page size
	
	InterruptSetMember							interruptSetMember;
	void										*oldInterruptRefCon;
	InterruptHandler							oldInterruptHandler;
	InterruptEnabler							interruptEnabler;
	InterruptDisabler							interruptDisabler;
	struct  {
		volatile UInt32							scheduleOverrun;		// updated by the interrupt handler
		volatile UInt32							unrecoverableError;		// updated by the interrupt handler
		volatile UInt32							frameNumberOverflow;	// updated by the interrupt handler
		volatile UInt32							ownershipChange;		// updated by the interrupt handler
	} errors;
	volatile UInt64								frameNumber;
	UInt16										rootHubFuncAddress;		// Funciotn Address for the root hub
	int											OptiOn;

};

struct OHCIEndpointDescriptorStruct
{
	UInt32						dWord0;				// control
	UInt32						dWord1;				// pointer to last TD
	UInt32						dWord2;				// pointer to first TD
	UInt32						dWord3;				// Pointer to next ED
	UInt32						pVirtualNext;
	UInt32						pPhysical;
	UInt32						pVirtualTailP;
	UInt32						pVirtualHeadP;
};
struct OHCIGeneralTransferDescriptorStruct
{
	volatile UInt32				dWord0;				// Data controlling transfer.
	volatile UInt32				dWord1;				// Current buffer pointer.
	volatile UInt32				dWord2;				// Pointer to next transfer descriptor.
	UInt32						dWord3;				// Pointer to end of buffer.
	CallBackFuncPtr 			CallBack;			// only used if last TD, other wise its  nil
	long						refcon;				// 
	UInt32						pPhysical;
	UInt32						pVirtualNext;
	UInt32						pType;
	UInt32						pEndpoint;			// pointer to TD's Endpoint
	UInt32						bufferSize;			// used only by control transfers to keep track of data buffers size leftover
	IOPreparationID				preparationID;		// used for CheckpointIO
};

struct OHCIIsochTransferDescriptorStruct
{
	UInt32						dWord0;				// Condition code/FrameCount/DelayInterrrupt/StartingFrame.
	UInt32						dWord1;				// Buffer Page 0.
	UInt32						dWord2;				// Pointer to next transfer descriptor.
	UInt32						dWord3;				// Pointer to end of buffer.
	UInt16						offset[8];
/*
	UInt32						dWord4;				// offset1/PSW1 - offset0/PSW0
	UInt32						dWord5;				// offset3/PSW3 - offset2/PSW2
	UInt32						dWord6;				// offset5/PSW5 - offset4/PSW4
	UInt32						dWord7;				// offset7/PSW7 - offset6/PSW6
*/
	UInt32						pType;				// type of descriptor, must always be in this position
	IsocCallBackFuncPtr			handler;			// callback for Isoch transactions
	UInt32						pPhysical;			// physical address of ITD
	UInt32						pVirtualNext;		// virtual ptr to next ITD
	UInt32						pIsocFrame;			// ptr to USLs status and length array
	UInt32						refcon;				// callback reference
	UInt32						frameNum;			//	index to pIsocFrame array
	UInt32						reserve2;			// 
};

struct OHCIPhysicalLogicalStruct
{
	UInt32						LogicalStart;
	UInt32						LogicalEnd;
	UInt32						PhysicalStart;
	UInt32						PhysicalEnd;
	UInt32						type;
	UInt32						pNext;
	
};

struct ErrataListEntryStruct {
	UInt16 						vendID;
	UInt16 						deviceID;
	UInt16 						revisionLo;
	UInt16 						revisionHi;
	UInt32 						errata;
};


//Globals -yuck  (Barry, made more global to share with root hub)





int OHCIUIMInitialize(unsigned long);

OSStatus OHCIUIMFinalize(
	Boolean						beingReplaced,
	Ptr	*						savedStatePtr);

OSStatus OHCIUIMControlEDCreate(
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed);

OSStatus OHCIUIMControlTransfer(
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	short						bufferSize,
	short						direction);

OSStatus OHCIUIMControlEDDelete(
	short 						functionNumber,
	short						endpointNumber);

OSStatus OHCIUIMClearEndPointStall(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

OSStatus OHCIUIMBulkEDCreate(
	UInt8 						functionAddress,
	UInt8						endpointNumber,
	UInt8						maxPacketSize,
	UInt8						speed);

OSStatus OHCIUIMBulkTransfer(
	UInt32						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	UInt32						bufferSize,
	short						direction);

OSStatus OHCIUIMBulkEDDelete(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

OSStatus OHCIUIMInterruptEDCreate(
	short						functionNumber,
	short						endPointNumber,
	short						speed,
	UInt16 						maxPacketSize,
	short						pollingRate,
	UInt32						reserveBandwidth);

OSStatus OHCIUIMInterruptTransfer(
	short						functionNumber,
	short						endpointNumber,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						bufferSize);

OSStatus OHCIProcessDoneQueue(void);

OSStatus OHCIUIMAbortEndpoint(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

OSStatus	OHCIUIMEndpointDelete(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

UInt64 OHCIUIMGetCurrentFrameNumber(void);

OSStatus OHCIUIMIsochEDCreate(
	short						functionAddress,
	short						endpointNumber,
	UInt32						maxPacketSize,
	UInt8						direction);
	
OSStatus OHCIUIMIsochTransfer(
	short						functionAddress,						
	short						endpointNumber,	
	UInt32						refcon,
	UInt8						direction,
	IsocCallBackFuncPtr			pIsochHandler,							
	UInt64						frameNumberStart,								
	UInt32						pBufferStart,							
	UInt32						frameCount,									
	USBIsocFrame				*pFrames);								

// BT Root hub internal functions

OSStatus RootHubFrame(void *p1, void *p2);
OSStatus RootHubStatusChange(void *p1, void *p2);

Boolean OHCISetOurAddress(uslBusRef bus, UInt16 addr);

void OHCISetFrameInterrupt(void);
OSStatus UIMSimulateRootHubStages(
	UInt32						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						endpointNumber,
	short						bufferSize,
	short						direction);

void SimulateRootHubInt(UInt8 endpoint, UInt8 *buf, UInt32 bufLen,
						CallBackFuncPtr handler, UInt32 refCon);

void OHCIPollRootHubSim(uslBusRef bus);

void ResetRootHubSimulation(void);

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OHCIUIM__ */
