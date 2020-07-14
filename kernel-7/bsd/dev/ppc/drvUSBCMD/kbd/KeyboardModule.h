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
	File:		KeyboardModule.h

	Contains:	Header file for Keyboard Module

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

	 <USB19>	 6/18/98	BWS		fix interrupt refcon support
	 <USB18>	 6/18/98	CJK		add interrupt refcon
	 <USB17>	 6/17/98	CJK		change retry count to 3
	 <USB16>	 6/16/98	CJK		change FindHIDInterfaceByProtocol to FindHIDInterfaceByNumber
	 <USB15>	 6/11/98	CJK		Remove driverentry routine (not needed or used).
	 <USB14>	 4/30/98	CJK		init rawreportmode and initialized flags appropriately.
	 <USB13>	 4/26/98	CJK		add code/states to handle getting the full configuration and
									parsing it.
	 <USB12>	  4/9/98	CJK		remove include of USBDeviceDefines.h
		<11>	  4/8/98	CJK		rework PostUSBKeyToMac to work the same way as the standard interrupt
									handler
		<10>	 3/25/98	CJK		Remove unneeded readHID & delay enums
		 <9>	 3/24/98	CJK		Change 'kTransactionPending' to 'kCompletionPending'
		 <8>	 3/17/98	CJK		Beging adding support for interrupt transactions.  Add function
									prototypes for the various keyboard HID module files.
		 <7>	  3/2/98	CJK		remove include of KBDHIDEmulation.h
		 <6>	 2/27/98	CJK		add include of USBHIDModules.h
		 <5>	 2/26/98	CJK		add set LED state
		 <4>	 2/16/98	CJK		add extra poll state (not currently used)
		 <3>	 2/11/98	CJK		remove unneeded state values.
		 <2>	 2/10/98	CJK		Correct change history (to reflect the keyboard module changes)
		 <1>	 2/10/98	CJK		First time check in.  Cloned from Mouse HID Module
*/
#ifndef __KeyboardModuleH__
#define __KeyboardModuleH__

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
//#include <Processes.h>
#include "../USB.h"

void 	PostUSBKeyToMac(UInt16 rawUSBkey);
void	PostADBKeyToMac(UInt16 virtualKeycode, UInt8 state);

void	USBDemoKeyIn(UInt32 refcon, void * theData);
void	InitUSBKeyboard(void);
void 	KBDHIDNotification(UInt32 devicetype, UInt8 NewHIDData[], UInt8 OldHIDData[]);

Boolean	SetBit(UInt8 *bitmapArray, UInt16 index, Boolean value);

 	OSStatus	KeyboardModuleInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
 	OSStatus	KeyboardInterfaceInitialize(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device);
 	OSStatus	KeyboardModuleFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc);

void	KeyboardModuleInitiateTransaction(USBPB *pb);
void	KeyboardModuleDelay1CompletionProc(USBPB *pb);
void	KeyboardCompletionProc(USBPB *pb);
void	kbd_InterfaceEntry(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterfaceDescriptor, USBDeviceDescriptorPtr pDeviceDescriptor, USBDeviceRef device);

OSErr 	FindHIDInterfaceByNumber(LogicalAddress pConfigDesc, UInt32 ReqInterface, USBInterfaceDescriptorPtr * hInterfaceDesc);
void 	kbd_NotifyRegisteredHIDUser(UInt32 devicetype, UInt8 hidReport[]);

#define kKeyboardRetryCount		3
#define kKeyboardModifierBits	8
#define kKeyboardReportKeys		6
#define	kKeyboardOffsetToKeys	2
#define kKeyboardReportSize		8

enum driverstages
{
	kUndefined = 0,
	kSetKeyboardLEDs,
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
	UInt8							hidReport[8];
	UInt8							oldHIDReport[8];
	UInt8							padding[8];
	
	Boolean							sendRawReportFlag;
	Boolean							hidEmulationInit;
	
	HIDInterruptProcPtr 			pSHIMInterruptRoutine;
	HIDInterruptProcPtr 			pSavedInterruptRoutine;
	
	UInt32							interruptRefcon;
	
	SInt32 							retryCount;
	SInt32							delayLevel;
	SInt32							transDepth;
} usbKeyboardPBStruct;

#endif //__KeyboardModuleH__
