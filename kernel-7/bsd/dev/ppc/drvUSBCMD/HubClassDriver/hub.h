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
	File:		Hub.h

	Contains:	Definitions for the USB hub driver

	Version:	Neptune 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BT)	Barry Twycross
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB16>	 9/10/98	BT		Add are we finished
	 <USB15>	 6/30/98	BT		Some defs moved to usb.i
	 <USB14>	 5/28/98	CJK		change file creater to 'MPS '
	 <USB13>	 4/14/98	BT		Do removed devices when killed
		<12>	  4/2/98	BT		Fix hub features to set
		<11>	  2/8/98	BT		Power allocation stuff
		<10>	 1/26/98	CJK		Change to use USBDeviceDescriptor (instead of just
									devicedescriptor)
		 <9>	 1/26/98	BT		Mangle names after design review
		 <8>	 1/19/98	BT		More root hub sim
		 <7>	12/22/97	BT		StartRootHub moving to USL
		 <6>	12/18/97	BT		CHanges to dispatch table.
		 <5>	12/18/97	BT		Add expert notify to entry
		 <4>	12/18/97	BT		Move device descriptor to USL.h
		 <3>	12/17/97	BT		Bring my hub.h in from the USL.
		 <2>	 12/4/97	CJK		Add BBS file header
		 <1>	 12/4/97	CJK		First checked in
*/

#ifndef __HUBH__
#define __HUBH__



#define OFFSET(ty, fl)  ((UInt32)  &(((ty *)0)->fl))

long x(long y);
Boolean bit(long value, long mask);
Boolean immediateError(OSStatus err);


// A.W made it static, so no need for it here 		OSStatus HubAreWeFinishedYet(void);
// A.W. this seems unused anywhere in source code void hubExpertNotify(void *exNote);


void HubDriverEntry(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
void Hub2DriverEntry(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
// A.W made it static, so no need for it here 		OSStatus killHub(USBDeviceRef device);

#endif

