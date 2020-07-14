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
 * Copyright 1993-1995 by Apple Computer, Inc., all rights reserved.
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96Curio.h - Hardware (chip) definitions for Apple 53c96 SCSI interface.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver
 * using design concepts from Copland DR2 Curio and MESH SCSI plugins.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"
/*
 * Define offsets into the SCSI 53c96 chip. Use the READ_REG and WRITE_REG
 * macros to access the chip.
 */
typedef enum {
	rXCL	= 0x00,		/* Transfer counter LSB							(r/w)	*/
	rXCM	= 0x10,		/* Transfer counter MSB							(r/w)	*/
	rFFO	= 0x20,		/* Fifo											(r/w)	*/
	rCMD	= 0x30,		/* Command										(r/w)	*/
	rSTA	= 0x40,		/* Status (r) or Destination bus ID				(w)		*/
	rINT	= 0x50,		/* Interrupt (r) or Select/reselect timeout		(w)		*/
	rSQS	= 0x60,		/* Sequence step (r) or Synch Period			(w)		*/
	rFOS	= 0x70,		/* FIFO Flags/Sequence Step (r) or Sync Offset	(w)		*/
	rCF1	= 0x80,		/* Configuration 1								(r/w)	*/
	rCKF	= 0x90,		/* Clock Conversion Factor						(w)		*/
	rTST	= 0xA0,		/* Test											(w)		*/
	rCF2	= 0xB0,		/* Configuration 2								(r/w)	*/
	rCF3	= 0xC0,		/* Configuration 3								(r/w)	*/
	rCF4	= 0xD0,		/* Configuration 4								(r/w)	*/
	rTCH	= 0xE0,		/* Transfer counter high/ID						(r/w)	*/
	rDMA	= 0xF0		/* Pseudo-DMA									(r/w)	*/
} CurioChipRegisters;

/*
 * SCSI 53C96 (ASC) Command Set
 */
enum {
	cNOP			= 0x00,		/* NOP command						*/
	cFlshFFO		= 0x01,		/* flush FIFO command				*/
	cRstSChp		= 0x02,		/* reset SCSI chip					*/
	cRstSBus		= 0x03,		/* reset SCSI bus					*/

	cIOXfer			= 0x10,		/* non-DMA Transfer command			*/
	cCmdComp		= 0x11,		/* Initiator Command Complete Seq	*/
	cMsgAcep		= 0x12,		/* Message Accepted					*/
	cXferPad		= 0x18,		/* Transfer pad						*/
	cDisconnect		= 0x23,		/* Disconnect from the SCSI bus		*/
	cSlctNoAtn		= 0x41,		/* Select Without ATN Sequence		*/
	cSlctAtn		= 0x42,		/* Select With ATN Sequence			*/
	cSlctAtnStp		= 0x43,		/* Select With ATN and Stop Seq		*/
	cEnSelResel		= 0x44,		/* Enable Selection/Reselection		*/
	cDsSelResel		= 0x45,		/* Disable Selection/Reselection	*/
	cSlctAtn3		= 0x46,		/* Select with ATN, send 3 byte msg	*/

	cSetAtn			= 0x1A,		/* Set ATN command					*/
	cRstAtn			= 0x1B,		/* Reset ATN command				*/

	bDMDEnBit		= 0x80,		/* Enable bit for DMA commands		*/
	bDscCmdState = 0x40,		/* Disconnected State Group Cmd's	*/

	cDMAXfer		= ( bDMDEnBit | cIOXfer ), /* DMA Transfer Cmd	*/
	cDMAXferPad		= ( bDMDEnBit | cXferPad ), /* DMA Transfer Pad	*/
	cDMASelWOAtn	= ( bDMDEnBit | cSlctNoAtn ), /* Sel w/o ATN + DMA */
	cDMASelWAtn		= ( bDMDEnBit | cSlctAtn ) /* Sel With ATN use DMA */
};

/*
 * SCSI 53C95 (ASC) Bit Definitions.
 * Note: these are duplicated so I don't have to retype the entire
 * state machine.
 */

/*
 * Bits in the Interrupt (rINT) register.
 */
enum {
	iSelected		= 0x01,		/* selected bit						*/
	iSelectWAtn		= 0x02,		/* selected w/ ATN bit				*/
	iReselected		= 0x04,		/* reselected bit					*/
	iFuncComp		= 0x08,		/* function complete bit			*/
	iBusService		= 0x10,		/* bus service bit					*/
	iDisconnect		= 0x20,		/* disconnected bit					*/
	iIlegalCmd		= 0x40,		/* illegal command bit				*/
	iResetDetect	= 0x80		/* SCSI reset detected bit			*/
};
//enum {	/* As used in AppleChip.m */
//	IS_SCSIRESET		= iResetDetect,
//	IS_ILLEGALCMD		= iIlegalCmd,
//	IS_DISCONNECT		= iDisconnect,
//	IS_SERVICE_REQ		= iBusService,
//	IS_SUCCESSFUL_OP	= iFuncComp,
//	IS_RESELECTED		= iReselected,
//	IS_SELECT_ATN		= iSelectWAtn,
//	IS_SELECTED			= iSelected
//};
	
/*
 * Bits in the Status (rSTA) register.
 */
enum {
	sIO				= 0x01,		/* I/O bit							*/
	sCD				= 0x02,		/* C/D bit							*/
	sMsg			= 0x04,		/* MSG bit							*/
	sCmdComp		= 0x08,		/* function complete bit			*/
	sTermCount		= 0x10,		/* bus service bit					*/
	sParityErr		= 0x20,		/* disconnected bit					*/
	sGrossErr		= 0x40,		/* illegal command bit				*/
	sIntPend		= 0x80,		/* SCSI interrupt pending			*/
	mPhase			= (sIO | sCD | sMsg),	/* the phase bitmask	*/
	sTCIntPend		= (sTermCount | sIntPend) /* TC int pending		*/
};
enum {	/* As used in AppleChip.m */
	SS_INTERRUPT		= sIntPend,
//	SS_ILLEGALOP		= sGrossErr,
	SS_PARITYERROR		= sParityErr,
	SS_COUNTZERO		= sTermCount,
//	SS_PHASEMASK		= mPhase
};

/*
 * Bits in the FIFO Count/Sequence Step (rFOS) register.
 */
enum {
	kFIFOCountMask	= 0x1F,		/* mask to get FIFO count			*/
	mSeqStep		= 0xE0		/* mask to get sequence step		*/
};
/*
 * internal state register (internState)
 * Hmm: I'm not sure if these exist on the Curio.
 */
#define INS_SYNC_FULL		0x10	// sync offset buffer full
#define INS_STATE_MASK		0x07

/*
 * Clock Conversion Values (based on SCSI chip clock - not CPU clock)
 */
enum {
	ccf10MHz		= 0x02,		/* CLK conv factor 10.0Mhz							*/
	ccf11to15MHz	= 0x03,		/* CLK conv factor 10.01 to 15.0Mhz					*/
	ccf16to20MHz	= 0x04,		/* CLK conv factor 15.01 to 20.0Mhz					*/
	ccf21to25MHz	= 0x05,		/* CLK conv factor 20.01 to 25.0Mhz					*/
	ccf26to30MHz	= 0x06,		/* CLK conv factor 25.01 to 30.0Mhz					*/
	ccf31to35MHz	= 0x07,		/* CLK conv factor 30.01 to 35.0Mhz					*/
	ccf36to40MHz	= 0x00		/* CLK conv factor 35.01 to 40.0Mhz (0 <- 8)		*/
};

/*
 * Select timeout values (these are the values stored in the chip register).
 * The "Mhz" value is the SCSI bus speed, returned by the Registry.
 */
enum {
	SelTO16Mhz		= 126,		/* (0x7e)  using the formula: RV (regr value)		*/
								/*   126 =  (16MHz * 250mS)/ (7682 * 4)				*/
								/*	250mS is ANSI standard.						*/
	SelTO25Mhz		= 167,		/* (0xa7)  using the formula: RV (regr value)		*/
								/*   163 =  (25MHz * 250mS)/ (7682 * 5)				*/
								/*	250mS is ANSI standard.						*/
	SelTO33Mhz		= 167,		/* (0xa7)  using the formula: RV (regr value)		*/
								/*   153 =  (33MHz * 250mS)/ (7682 * 7)				*/
								/*	250mS is ANSI standard.						*/
	SelTO40Mhz		= 167		/* (0xa7)  using the formula: RV (regr value)		*/
								/*   163 =  (40MHz * 250mS)/ (7682 * 8)				*/
								/*	250mS is ANSI standard.						*/
};

/*
 * Configuration Register 1 bit definition
 */
enum {
	CF1_SLOW		= 0x80,		/* Slow Cable Mode enabled bit						*/
	CF1_SRD			= 0x40,		/* SCSI Reset Reporting Intrp Disabled bit			*/
	CF1_PTEST		= 0x20,		/* Parity Test Mode bit								*/
	CF1_ENABPAR		= 0x10,		/* Enable Parity Checking bit						*/
	CF1_CHIPTEST	= 0x08,		/* Enable Chip Test Mode bit						*/
	CF1_DEFAULT_ID_MASK	= 0x07	/* The default host SCSI ID mask.					*/
};

/*
 * Configuration Register 2 bit definition
 */
enum {
	CF2_RFB			= 0x80,		/* Reserve FIFO Byte								*/
	CF2_ENFEATURES	= 0x40,		/* Enable features									*/
	CF2_EBC			= 0x20,		/* Enable Byte Control								*/
	CF2_DREQHIZ		= 0x10,		/* Force DREQ to HI-Z state							*/
	CF2_SCSI2		= 0x08,		/* SCSI-2 features									*/
	CF2_BPA			= 0x04,		/* Target:bad parity abort							*/
	CF2_RPE			= 0x02,		/* Rregister parity enable							*/
	CF2_DPE			= 0x01		/* DMA parity enable								*/
};

/*
 * Configuration Register 3 bit definition
 */
enum {
	CF3_MSGID		= 0x80,		/* Check for valid ID message						*/
	CF3_QTAG		= 0x40,		/* Enable 3 byte QTAG messages						*/
	CF3_CDB10		= 0x20,		/* Recognize 10 bytes CDBs							*/
	CF3_FASTCLOCK	= 0x10,		/* Enable fast clock for fast SCSI					*/
	CF3_FASTSCSI	= 0x08,		/* Enable fast SCSI									*/
	CF3_SRB			= 0x04,		/* Save residual byte								*/
	CF3_ALTDMA		= 0x02,		/* Alternate DMA (for threshold 8 only)				*/
	CF3_T8			= 0x01,		/* Force 8-byte DMA caching							*/
	CONFIG_FOR_DMA		= (CF3_SRB | CF3_ALTDMA | CF3_T8),
	CONFIG_FOR_NON_DMA	= (CF3_SRB)
};

/*
 * Configuration Register 4 bit definitions.
 */
enum {
	CF4_ACTIVENEGATION		= 0x04,
	CF4_TRANSFERCOUNTTEST	= 0x02,
	CF4_BACKTOBACK			= 0x01
};

/*
 * Miscellaneous Defines.
 */
enum {
	kDefaultInitiatorID			= 7,			/* SCSI Bus Initiator ID			*/
	/*
	 * The maximum transfer size is set *below* the absolute max of 65536 to avoid
	 * the problem of understanding the value zero, which means either 65536 bytes
	 * or zero bytes remaining to transfer.
	 */
	kMaxC96TransferSize			= (65536 - 4096),
// ** ** ** NEED VALUE
	kChipFastBusClockMHz		= 00,			/* Fast bus clock rate				*/
	kChipDefaultBusClockMHz		= 25			/* Default 53c96 clock rate in MHz	*/
};

enum {	/* Values for SyncParms MESH register:		*/
		/* 1st nibble is offset, 2nd is period.		*/
		/* Zero offset means async.					*/
		/* Note: the external bus does not support	*/
		/* either fast or synchronous modes, so the	*/
		/* following definitions are irrelevant.	*/
		/* However, they will be necessary for the	*/
		/* internal bus on the PowerMac 8100		*/
	kSyncParmsNone	= 0x00,	/* No synchronous negotiation needed	*/
	kSyncParmsAsync	= 0x02,	/* Async with min period = 2			*/
	kSyncParmsFast	= 0xF0	/* offset = 15, period = Fast (10 MB/s)	*/
};

/*
 * DMA alignment requirements.
 */
enum {
	AMIC_ReadStartAlignment		= 8,
	AMIC_WriteStartAlighment	= 8,
	AMIC_ReadLengthAlignment	= 0,
	AMIC_WriteLengthAlignment	= 0,
	DBDMA_ReadStartAlignment	= 0,
	DBDMA_WriteStartAlignment	= 0,
	DBDMA_ReadLengthAlignment	= 0,
	DBDMA_WriteLengthAlignment	= 0
};
