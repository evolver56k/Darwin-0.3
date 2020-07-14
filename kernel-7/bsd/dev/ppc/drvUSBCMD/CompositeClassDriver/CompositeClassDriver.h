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
	File:		CompositeClassDriver.h

	Contains:	Header file for Composite Class Driver 

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(BG)	Bill Galcher
		(BT)	Barry Twycross
		(CJK)	Craig Keithley

	Change History (most recent first):

	  <USB6>	  9/4/98	TC		Update CompositeDriverNotifyProc to use version 1.1 of dispatch
									table.
	  <USB5>	  6/8/98	CJK		remove driverentry; not needed
	  <USB4>	  6/5/98	BG		Add missing prototype - DriverEntry().
	  <USB3>	  6/5/98	CJK		add state to handle USBNewInterfaceRef.  add some new fields to
									the global structs to keep track of the interfacerefs, as well
									as flag when it's okay to tear things down.
	  <USB2>	  4/9/98	CJK		change to use USB.h
		 <1>	  4/7/98	CJK		first checked in as CompositeClassDriver
		<25>	 3/26/98	CJK		look at problem with Microsoft keyboard
		<24>	 3/17/98	CJK		Replace "};" with just "}" (where appropriate).  This eliminates
									some error messages from MetroWerks.  Added function prototypes.
		<23>	 2/26/98	CJK		change pFullConfigurationDescriptor type to
									USBConfigurationDescriptorPtr
		<22>	  2/9/98	CJK		remove HID Emulation related items (no more HID fields in
									Composite struct, no more include of HIDEmulation.h, etc.)
		<21>	  2/6/98	CJK		Crank 50ms delay back down to 10ms. Changed name of state to not
									say how many ms.
		<20>	  2/3/98	CJK		move device descriptor 'next to' the other USB descriptors.
									Removed unused fields from the CompositePBStruct.
		<19>	  2/2/98	CJK		Add XXGetHIDDescriptor prototype
		<18>	 1/30/98	CJK		Revise HIDModuleDispatchTable to have better names for the
									dispatch table entries.
		<17>	 1/27/98	CJK		Add HIDModuleDispatch Table definition
		<16>	 1/26/98	CJK		change to match codebert
		<15>	 1/26/98	BT		Mangle names after design review
		<14>	 1/26/98	CJK		Add other USB/ADB shim dispatch functions
		<13>	 1/23/98	CJK		remove hub related timer fields from Composite struct.
		<12>	 1/23/98	CJK		Implement HID data change detection
		<11>	 1/23/98	CJK		Work on configuration parsing
		<10>	 1/22/98	CJK		Add configuration pointer and configuration descriptors to
									Composite struct.
		 <9>	 1/22/98	CJK		Add RetryTransaction flag
		 <8>	 1/20/98	CJK		Add configuration parser function prototypes.
		 <7>	 1/20/98	CJK		Add other state values
		 <6>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <5>	 1/14/98	CJK		Add enum for 'kChangeState'
		 <4>	  1/8/98	CJK		Removed initialize & finalize function prototypes (they're not
									needed now that the functions are in the description.c file).
		 <3>	  1/6/98	CJK		Add prototypes for TaskTimeProc, ExpertEntryProc, and related
									functions.
		 <2>	12/17/97	CJK		Add BBS file header
		 <1>	12/17/97	CJK		First time checkin
*/
#ifndef __CompositeCLASSDRIVERH__
#define __CompositeCLASSDRIVERH__

//naga#include "../types.h"
//naga#include <Devices.h>
//naga#include "../driverservices.h"
//naga#include <Processes.h>
//naga#include "../USB.h"

Boolean		immediateError(OSStatus err);
void		CompositeDeviceInitiateTransaction(USBPB *pb);
void		CompositeDeviceCompletionProc(USBPB *pb);
void 		InitParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock);

OSStatus	CompositeDriverInitInterface(UInt32 interfaceNum, USBInterfaceDescriptor *interfaceDesc, USBDeviceDescriptor *deviceDesc, USBDeviceRef device);
OSStatus	CompositeDriverNotifyProc(UInt32 notification, void *pointer, UInt32 refcon);
OSStatus	CompositeDriverValidateHW(USBDeviceRef device, USBDeviceDescriptor *desc);
OSStatus	CompositeDriverInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
OSStatus	CompositeDriverFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc);

void		DeviceInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDeviceDescriptor, UInt32 busPowerAvailable);

OSErr		GetInterfaceDescriptor(LogicalAddress pConfigDesc, UInt32 ReqInterface, USBInterfaceDescriptorPtr *hInterfaceDesc);

#define 	kCompositeRetryCount	5

enum driverstates
{
	kUndefined = 0,
	kSetConfig,
	kGetFullConfiguration0,
	kGetFullConfiguration1,
	kNewInterfaceRef,
	kExitDriver = 			0x1000,
	kRetryTransaction = 	0x2000,
	kSyncTransaction = 		0x4000,
	kCompletionPending = 	0x8000
};

typedef struct
{
	USBPB 							pb;
	void (*handler)(USBPB 			*pb);

	USBDeviceRef					deviceRef;
	
	Boolean							disposeCompletedFlag;
	Boolean							okayToFinalizeFlag;
	
	USBDeviceDescriptor 			deviceDescriptor;
	USBConfigurationDescriptor		partialConfigDescriptor;
	USBConfigurationDescriptorPtr 	pFullConfigDescriptor;
	
	USBInterfaceDescriptor			interfaceDescriptors[32];
	USBInterfaceRef					interfaceRefArray[32];
	USBRqIndex						interfaceCount;
	USBRqIndex						interfaceIndex;
	
	SInt32 							retryCount;
	SInt32							delayLevel;
	SInt32							transDepth;
	UInt32							busPowerAvailable;
} usbCompositePBStruct;

#endif //__CompositeCLASSDRIVERH__
