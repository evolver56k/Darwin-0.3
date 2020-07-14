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
	File:		UIMpriv.h

	Contains:	Barry Twycross

	Version:	Neptune 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(TC)	Tom Clark
		(DF)	David Ferguson
		(GG)	Guillermo Gallegos
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB25>	 8/31/98	BT		Add isoc pipes
	 <USB24>	 8/13/98	BT		Add multibus support
	 <USB23>	 8/12/98	BT		Move root hub into UIM again
	 <USB22>	 7/10/98	TC		Back out <USB21>
	 <USB21>	 6/30/98	BT		Move RH sim to UIM
	 <USB20>	 6/14/98	DF		Add ResetRootHubSimulation function prototype
	 <USB19>	  6/5/98	BT		Use UIM time
	 <USB18>	  5/5/98	GG		Change Buffersize from short to unsigned long in
									uimBulkTransfer.
	 <USB17>	 4/23/98	BT		Add reset portsuspend change
	 <USB16>	 4/20/98	BT		Add abort pipe
	 <USB15>	 4/16/98	BT		Add over current prototype
	 <USB14>	 4/14/98	BT		Add back ControlEDDelete, EDDelete doesn't work for control
									endpoints
	 <USB13>	  4/9/98	BT		Use USB.h
		<12>	 3/19/98	BT		Split UIM into UIM and root hub parts
		<11>	 3/18/98	BT		Add clear port enable change feature
		<10>	 3/11/98	BT		Int simulation for root hub.
		 <9>	  3/5/98	BT		Add int transactions
		 <8>	 2/23/98	BT		Simulate root hub to not corrupt reqCount
		 <7>	  2/2/98	BT		Add bulk stuff
		 <6>	 1/26/98	BT		Hack in clear enpoint stall
		 <5>	 1/19/98	BT		More root hub sim
		 <4>	 1/13/98	BT		Change uslPacket to uslControlPacket, get ready for int
									transactions
		 <3>	12/19/97	BT		UIM now a Shared lib
*/

#ifndef __UIMPRIVH__
#define __UIMPRIVH__

#include "Library/uslpriv.h"

#if 0
enum{
	uimKBadRef = kUSBNoTran

	};
#endif

OSStatus UIMControlEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed);

OSStatus UIMControlEDDelete(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber);
	
OSStatus UIMInterruptEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed);

OSStatus UIMIsocEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt8						direction,	
	UInt16						maxPacketSize);

OSStatus UIMBulkEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt8						direction,	
	UInt8						maxPacketSize);

OSStatus UIMControlTransfer(
	UInt32 bus,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	short						bufferSize,
	short						direction);

OSStatus UIMEDDelete(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

OSStatus UIMBulkTransfer(
	UInt32 bus,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	UInt32						bufferSize,
	short						direction);

OSStatus UIMIntTransfer(
	UInt32 bus,
	short						functionNumber,
	short						endpointNumber,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						bufferSize);

OSStatus UIMIsocTransfer(
	UInt32 bus,
	short						functionNumber,
	short						endpointNumber,
	short						direction,
	UInt32 				 		frameStart, 
	UInt32 						refcon,
	IsocCallBackFuncPtr			handler,
	void 						*buffer,
	UInt32 						numFrames,
	USBIsocFrame 				*frames);

OSStatus UIMClearEndPointStall(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

OSStatus UIMAbortEndpoint(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction);



OSStatus UIMResetRootHub (UInt32 bus);

UInt64 UIMGetCurrentFrame(UInt32 bus);

void UIMPollRootHubSim(void);


#endif /*__UIMPRIVH__ */
