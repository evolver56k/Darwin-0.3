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
	File:		MouseModule.h

	Contains:	Header file for mouse module

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BWS)	Brent Schorsch
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB23>	 6/18/98	BWS		fixed interrupt refcon support
	 <USB22>	 6/18/98	CJK		change retry to 3.  add refcon to mouse data struct
	 <USB21>	 6/16/98	CJK		change FindHIDInterfaceByProtocol to FindHIDInterfaceByNumber
	 <USB20>	 6/11/98	CJK		Remove driverentry routine (not needed or used).
	 <USB19>	  6/8/98	CJK		change SetParamBlock function name to InitParamBlock. Remove
									unneeded xxget... functions
	 <USB18>	 5/26/98	CJK		add prototype for SetParamBlock
	 <USB17>	 5/15/98	CJK		Increase size of HID Report.
	 <USB16>	  5/4/98	CJK		add IntRead req count setup field
	 <USB15>	 4/26/98	CJK		add setidle back in.  This seems to solve periodic stall
									problems.
	 <USB14>	 4/26/98	CJK		increase retry count to 10
	 <USB13>	 4/24/98	CJK		Change cursor device field name in mouse data structure to be
									"CursorDeviceInfo"
	 <USB12>	 4/23/98	CJK		update to new style of setting bmRequest
	 <USB11>	  4/9/98	CJK		remove include of USBClassDrivers.h, USBHIDModules.h, and
									USBDeviceDefines.h
		<10>	  4/8/98	CJK		add saved interrupt routine proc pointer
		 <9>	 3/26/98	CJK		fix delay callback depth problem
		 <8>	 3/25/98	CJK		add interrupt states to enum
		 <7>	 3/17/98	CJK		Remove comma from last enum item.  Update function prototypes
									using extractprototypes tool.
		 <6>	  3/2/98	CJK		Add include of USBHIDModules.h. Remove include of HIDEmulation.h
		 <5>	 2/18/98	CJK		Add cursor device structure
		 <4>	 2/11/98	CJK		remove unneeded state values.
		 <3>	  2/9/98	CJK		Change Interface Descriptor field in mouse paramblock to be just
									a single interface.
		 <2>	  2/9/98	CJK		Add InterfaceEntry function prototype
		 <1>	12/17/97	CJK		First time checkin
*/
#ifndef __MouseModuleH__
#define __MouseModuleH__

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
//#include <Processes.h>
#include "CursorDevices.h"
#include "../USB.h"

Boolean	immediateError(OSStatus err);
void	MouseModuleInitiateTransaction(USBPB *pb);
void	MouseModuleDelayCompletionProc(USBPB *pb);
void 	TransactionCompletionProc(USBPB *pb);
void	InterfaceEntry(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterfaceDescriptor, USBDeviceDescriptorPtr pDeviceDescriptor, USBDeviceRef device);

/*	Prototypes from MouseModuleHeader.c	Tue, Mar 17, 1998 3:30:22 PM	*/
static 	OSStatus	MouseModuleInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
static 	OSStatus	MouseInterfaceInitialize(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device);
static 	OSStatus	MouseModuleFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc);


/*	Prototypes from MouseConfigParse.c	Tue, Mar 17, 1998 3:17:14 PM	*/
OSErr 	FindHIDInterfaceByNumber(LogicalAddress pConfigDesc, UInt32 ReqInterface, USBInterfaceDescriptorPtr * hInterfaceDesc);

OSStatus USBHIDControlDevice(UInt32 theControlSelector, void * theControlData);
void 	NotifyRegisteredHIDUser(UInt32 devicetype, UInt8 hidReport[]);

void 	USBMouseIn(UInt32 refcon, void * theData);
void 	InitParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock);

#define kMouseRetryCount	3

enum driverstages
{
	kUndefined = 0,
	kGetFullConfiguration,
	kFindInterfaceAndSetProtocol,
	kSetIdleRequest,
	kFindAndOpenInterruptPipe,
	kReadInterruptPipe,
	kReturnFromDriver = 	0x1000,
	kRetryTransaction = 	0x2000,
	kSyncTransaction = 		0x4000,
	kCompletionPending = 	0x8000
};

typedef struct
{
	USBPB 							pb;
	void (*handler)(USBPB 			*pb);

	USBDeviceRef					deviceRef;
	USBInterfaceRef					interfaceRef;
	USBPipeRef						pipeRef;
	
	USBDeviceDescriptor 			deviceDescriptor;
	USBInterfaceDescriptor			interfaceDescriptor;

	USBConfigurationDescriptorPtr 	pFullConfigDescriptor;
	USBInterfaceDescriptorPtr		pInterfaceDescriptor;
	USBEndPointDescriptorPtr		pEndpointDescriptor;

	UInt32							hidDeviceType;
	UInt8							hidReport[64];
	
	HIDInterruptProcPtr 			pSHIMInterruptRoutine;
	HIDInterruptProcPtr 			pSavedInterruptRoutine;
	
	UInt32							interruptRefcon;
	
	SInt32 							retryCount;
	SInt32							transDepth;
	
	CursorDevicePtr					pCursorDeviceInfo;
	CursorDevice					cursorDeviceInfo;
	Fixed							unitsPerInch;
} usbMousePBStruct;

#endif //__MouseModuleH__
