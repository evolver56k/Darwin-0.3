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
	File:		KeyboardModuleHeader.c

	Contains:	Keyboard Module Header file

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB12>	 6/17/98	CJK		add pipe abort in finalize
	 <USB11>	 6/11/98	CJK		change display of status to display of interfaceRef (in finalize
									routine)
	 <USB10>	 6/11/98	CJK		investigate interface driver load time call of routines
	  <USB9>	  6/8/98	CJK		deallocation of full configuration structure
	  <USB8>	 5/20/98	CJK		change target name from USBKeyboardModule to
									USBHIDKeyboardModule
	  <USB7>	 4/27/98	TC		Implement a USBDriverLoadingOption to enforce protocol matching.
	  <USB6>	  4/9/98	CJK		remove include of USBDeviceDefines.h
		 <5>	 3/17/98	CJK		fix pragma unuseds (MW needs parens around the variable name).
		 <4>	  3/5/98	CJK		use version info defined in KeyboardModuleVersion.h
		 <3>	  3/2/98	CJK		remove include of KBDHIDEmulation.h
		 <2>	 2/10/98	CJK		Correct change history (to reflect the keyboard module changes)
		 <1>	 2/10/98	CJK		First time check in.  Cloned from Mouse HID Module.
*/

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
#include "../USB.h"

#include "KeyboardModule.h"
#include "KeyboardModuleVersion.h"

static 	OSStatus 	KeyboardModuleInitialize (USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
static 	OSStatus 	KeyboardModuleFinalize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);
static 	OSStatus 	KeyboardInterfaceInitialize (UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device);

extern	usbKeyboardPBStruct myKeyboardPB;

//------------------------------------------------------
//
//	This is the driver description structure that the expert looks for first.
//  If it's here, the information within is used to match the driver
//  to the device whose descriptor was passed to the expert.
//	Information in this block is also used by the expert when an
//  entry is created in the Name Registry.
//
//------------------------------------------------------
USBDriverDescription	TheUSBDriverDescription = 
{
	// Signature info
	kTheUSBDriverDescriptionSignature,
	kInitialUSBDriverDescriptor,
	
	// Device Info
	0,										// vendor = not device specific
	0,										// product = not device specific
	0,										// version of product = not device specific
	kUSBKeyboardInterfaceProtocol,				// protocol = not device specific
	
	// Interface Info	(* I don't think this would always be required...*)				
	0,										// Configuration Value
	0,										// Interface Number
	kUSBHIDInterfaceClass,					// Interface Class
	kUSBBootInterfaceSubClass, 				// Interface SubClass
	kUSBKeyboardInterfaceProtocol,				// Interface Protocol
		
	
	// Driver Info
	"USBHIDKeyboardModule",				// Driver name for Name Registry
	kUSBHIDInterfaceClass,					// Device Class  (from USBDeviceDefines.h)
	kUSBBootInterfaceSubClass,				// Device Subclass 
	kKBDHexMajorVers, 
	kKBDHexMinorVers, 
	kKBDCurrentRelease, 
	kKBDReleaseStage,						// version of driver
	
	// Driver Loading Info
	kUSBProtocolMustMatch					// Flags 
};
/*naga
USBClassDriverPluginDispatchTable TheClassDriverPluginDispatchTable =
{
	kClassDriverPluginVersion,				// Version of this structure
	0,										// Hardware Validation Procedure
	KeyboardModuleInitialize,					// Initialization Procedure
	KeyboardInterfaceInitialize,				// Interface Initialization Procedure
	KeyboardModuleFinalize,					// Finalization Procedure
	0,										// Driver Notification Procedure
};
	
*/
	
// Initialization function
// Called upon load by Expert
static 	OSStatus 	KeyboardModuleInitialize (USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable)
{
#pragma unused (busPowerAvailable)
#pragma unused (pDesc)
	USBExpertStatus(device, "USBHIDKeyboardModule: Entered via KeyboardModuleInitialize", device);
	return (OSStatus)noErr;
}

// Interface Initialization Initialization function
// Called upon load by Expert
static 	OSStatus 	KeyboardInterfaceInitialize (UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device)
{
	InterfaceEntry(interfacenum, pInterface, pDesc, device);
	return (OSStatus)noErr;
}



// Termination function
// Called by Expert when driver is being shut down
static OSStatus KeyboardModuleFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc)
{
#pragma unused (pDesc)

OSStatus myErr;

	USBExpertStatus(theDeviceRef, "USBHIDKeyboardModule: Finalize", theDeviceRef);
	if (myKeyboardPB.pFullConfigDescriptor != nil)
	{
		if (myKeyboardPB.pipeRef)
		{
			USBExpertStatus(theDeviceRef, "USBHIDKeyboardModule: Aborting interrupt pipe", theDeviceRef);
			USBAbortPipeByReference(myKeyboardPB.pipeRef);
		}
			
		myKeyboardPB.pb.usbReference = theDeviceRef;
		myKeyboardPB.pb.pbVersion = kUSBCurrentPBVersion;
		myKeyboardPB.pb.usbFlags = 0;
		myKeyboardPB.pb.usbRefcon = 0; 			
		myKeyboardPB.pb.usbBuffer = myKeyboardPB.pFullConfigDescriptor;		
		myKeyboardPB.pb.usbCompletion = (USBCompletion)-1;
		
		myErr = USBDeallocMem(&myKeyboardPB.pb);
	};
	return (OSStatus)noErr;
}

