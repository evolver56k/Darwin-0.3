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
	File:		OHCIRootHub.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	  <USB8>	 8/12/98	BT		Move root hub into UIM again
	  <USB7>	 7/10/98	TC		Back out previous rev.
	  <USB6>	 6/30/98	BT		Move Root hub sim into UIM
	  <USB5>	 6/11/98	BT		Fix hub descriptor so it knows about overcurrent and power mode
	  <USB4>	 5/28/98	CJK		change file creater to 'MPS '
	  <USB3>	 4/23/98	BT		Add reset portsuspend change
	  <USB2>	 4/15/98	BT		Add over current change reset
		 <1>	 3/19/98	BT		first checked in
*/

#ifndef __OHCIROOTHUB__
#define __OHCIROOTHUB__

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=power
#endif



// Root hub sttaus reg
enum
{
	kOHCIHcRhStatus_LPS			= kOHCIBit0,
	kOHCIHcRhStatus_OCI			= kOHCIBit1,
	kOHCIHcRhStatus_DRWE		= kOHCIBit15,
	kOHCIHcRhStatus_LPSC		= kOHCIBit16,
	kOHCIHcRhStatus_OCIC		= kOHCIBit17,
	kOHCIHcRhStatus_CRWE		= kOHCIBit31,
	kOHCIHcRhStatus_Change		= kOHCIHcRhStatus_LPSC|kOHCIHcRhStatus_OCIC
};

// Port status reg 
enum
{
	kOHCIHcRhPortStatus_CCS			= kOHCIBit0,
	kOHCIHcRhPortStatus_PES			= kOHCIBit1,
	kOHCIHcRhPortStatus_PSS			= kOHCIBit2,
	kOHCIHcRhPortStatus_POCI		= kOHCIBit3,
	kOHCIHcRhPortStatus_PPS			= kOHCIBit8,
	kOHCIHcRhPortStatus_LSDA		= kOHCIBit9,
	kOHCIHcRhPortStatus_CSC			= kOHCIBit16,
	kOHCIHcRhPortStatus_PESC		= kOHCIBit17,
	kOHCIHcRhPortStatus_PSSC		= kOHCIBit18,
	kOHCIHcRhPortStatus_OCIC		= kOHCIBit19,
	kOHCIHcRhPortStatus_PRSC		= kOHCIBit20,
	kOHCIHcRhPortStatus_Change		= kOHCIHcRhPortStatus_CSC|kOHCIHcRhPortStatus_PESC|
				kOHCIHcRhPortStatus_PSSC|kOHCIHcRhPortStatus_OCIC|kOHCIHcRhPortStatus_PRSC
};


enum
{
	kOHCINumPortsMask				= OHCIBitRange (0, 7),
	kOHCIPowerSwitchingMask			= OHCIBitRange (9, 9),
	kOHCIGangedSwitchingMask		= OHCIBitRange (8, 8),
	kOHCICompoundDeviceMask			= OHCIBitRange (10, 10),
	kOHCIOverCurrentMask			= OHCIBitRange (12, 12),
	kOHCIGlobalOverCurrentMask		= OHCIBitRange (11, 11),
	kOHCIDeviceRemovableMask		= OHCIBitRange (0, 15),
	kOHCIGangedPowerMask			= OHCIBitRange (16, 31),

	kOHCIPowerSwitchingOffset		= 9,
	kOHCIGangedSwitchingOffset		= 8,
	kOHCICompoundDeviceOffset		= 10,
	kOHCIOverCurrentOffset			= 12,
	kOHCIGlobalOverCurrentOffset	= 11,
	kOHCIGangedPowerOffset			= 0,
	kOHCIDeviceRemovableOffset		= 16,

	kOHCIPortFlagsMask				= OHCIBitRange (0, 16),

	kOHCIportChangeFlagsOffset		= 16
};




OSStatus OHCIGetRootDescriptor(
	uslBusRef 					bus,
	uslRootHubDescriptorPtr 	descr);

OSStatus OHCIRootHubResetChangeConnection(
	uslBusRef 					bus,
	short						port);

OSStatus OHCIRootHubResetResetChange(
	uslBusRef 					bus,
	short						port);

OSStatus OHCIRootHubResetSuspendChange(
	uslBusRef 					bus,
	short						port);

OSStatus OHCIRootHubResetEnableChange(
	uslBusRef 					bus,
	short						port);

OSStatus OHCIRootHubResetOverCurrentChange(
	uslBusRef 					bus,
	short						port);

OSStatus OHCIGetInterruptStatus(
	uslBusRef 					bus,
	UInt32 						*status);


OSStatus OHCIClearInterruptStatus(
	uslBusRef 					bus,
	UInt32 						status);

OSStatus OHCIGetRhStatus(
	uslBusRef 					bus,
	UInt32 						*status);

OSStatus OHCIRootHubGetPortStatus(
	uslBusRef 					bus,
	short 						port,
	uslRootHubPortStatusPtr 	portStatus);

void OHCIPollRootHubSim (
	uslBusRef					bus);

OSStatus OHCIResetRootHub (
	uslBusRef					bus);

OSStatus OHCIRootHubPortSuspend(
	uslBusRef 					bus,
	short						port,
	Boolean						on);

OSStatus OHCIRootHubPower(
	uslBusRef 					bus,
	Boolean						on);

OSStatus OHCIRootHubResetPort (
	uslBusRef 					bus,
	short						port);

OSStatus OHCIRootHubPortEnable(
	uslBusRef 					bus,
	short						port,
	Boolean						on);

OSStatus OHCIRootHubPortPower(
	uslBusRef 					bus,
	short						port,
	Boolean						on);


#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OHCIROOTHUB__ */
