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
	File:		MouseModuleHeader.c

	Contains:	Mouse Module Header file

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

	 <USB15>	 6/23/98	CJK		comment out the 'theMouseData' variable; it's not used (yet).
	 <USB14>	 6/22/98	CJK		make certain that the cursor device is disposed in the finalize
									routine
	 <USB13>	 6/18/98	CJK		add pipe abort to finalize routine
	 <USB12>	 6/11/98	CJK		change display of status to display of interfaceRef (in finalize
									routine)
	 <USB11>	 6/11/98	CJK		remove call to driverentry in MouseModuleInitialize. Add some
									USBExpertStatus messages to the initialize and finalize
									routines.
	 <USB10>	  6/8/98	CJK		add dealloc of full configuration to finalize routine
	  <USB9>	 5/20/98	CJK		change driver name from USBMouseModule to USBHIDMouseModule
	  <USB8>	 4/27/98	TC		Implement a USBDriverLoadingOption to enforce protocol matching.
	  <USB7>	  4/9/98	CJK		remove include of USBClassDrivers.h, USBHIDModules.h, and
									USBDeviceDefines.h
		 <6>	 3/17/98	CJK		Add parens around pragma unused variables.
		 <5>	  3/5/98	CJK		use version defines from MouseModuleVersion.h
		 <4>	  3/2/98	CJK		Add include of USBHIDModules.h. Remove include of HIDEmulation.h
		 <3>	  2/9/98	CJK		remove deallocate of full configuration memory (the full
									configuration data is not needed for HID devices)
		 <2>	  2/9/98	CJK		Add interface initialization routine.
		 <1>	  2/9/97	CJK		First time checkin
*/

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
#include "../USB.h"


#include "MouseModule.h"
#include "MouseModuleVersion.h"

static 	OSStatus 	MouseModuleInitialize (USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable);
static 	OSStatus 	MouseModuleFinalize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);
static 	OSStatus 	MouseInterfaceInitialize (UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device);

extern	usbMousePBStruct myMousePB;

//------------------------------------------------------
//
//	This is the driver description structure that the expert looks for first.
//  If it's here, the information within is used to match the driver
//  to the device whose descriptor was passed to the expert.
//	Information in this block is also used by the expert when an
//  entry is created in the Name Registry.
//
//------------------------------------------------------
/* naga define in kbd 
USBDriverDescription	TheUSBDriverDescription = 
{
	// Signature info
	kTheUSBDriverDescriptionSignature,
	kInitialUSBDriverDescriptor,
	
	// Device Info
	0,										// vendor = not device specific
	0,										// product = not device specific
	0,										// version of product = not device specific
	kUSBMouseInterfaceProtocol,				// protocol = not device specific
	
	// Interface Info	(* I don't think this would always be required...*)				
	0,										// Configuration Value
	0,										// Interface Number
	kUSBHIDInterfaceClass,					// Interface Class
	kUSBBootInterfaceSubClass, 				// Interface SubClass
	kUSBMouseInterfaceProtocol,				// Interface Protocol
		
	
	// Driver Info
	"USBHIDMouseModule",						// Driver name for Name Registry
	kUSBHIDInterfaceClass,					// Device Class  (from USBDeviceDefines.h)
	kUSBBootInterfaceSubClass,				// Device Subclass 
	kMouseHexMajorVers, 
	kMouseHexMinorVers, 
	kMouseCurrentRelease, 
	kMouseReleaseStage,						// version of driver
	
	// Driver Loading Info
	kUSBProtocolMustMatch					// Flags (currently undefined)
};
*/
/* naga defined in CompositeDriverDescription.c 
USBClassDriverPluginDispatchTable TheClassDriverPluginDispatchTable =
{
	kClassDriverPluginVersion,				// Version of this structure
	0,										// Hardware Validation Procedure
	MouseModuleInitialize,					// Initialization Procedure
	MouseInterfaceInitialize,				// Interface Initialization Procedure
	MouseModuleFinalize,					// Finalization Procedure
	0,										// Driver Notification Procedure
};
	
*/
// Initialization function
// Called upon load by Expert
static 	OSStatus 	MouseModuleInitialize (USBDeviceRef device, USBDeviceDescriptorPtr pDesc, UInt32 busPowerAvailable)
{
#pragma unused (busPowerAvailable)
#pragma unused (pDesc)
	USBExpertStatus(device, "USBHIDMouseModule: Entered via KeyboardModuleInitialize", device);
	return (OSStatus)noErr;
}

// Interface Initialization Initialization function
// Called upon load by Expert
static 	OSStatus 	MouseInterfaceInitialize (UInt32 interfacenum, USBInterfaceDescriptorPtr pInterface, USBDeviceDescriptorPtr pDesc, USBDeviceRef device)
{
	InterfaceEntry(interfacenum, pInterface, pDesc, device);
	return (OSStatus)noErr;
}



// Termination function
// Called by Expert when driver is being shut down
static OSStatus MouseModuleFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc)
{
#pragma unused (pDesc)
OSStatus myErr;
//USBHIDData		theMouseData;

	USBExpertStatus(theDeviceRef, "USBHIDMouseModule: Finalize", theDeviceRef);
	
	/*
	if (myMousePB.pSHIMInterruptRoutine)			
	{												
		USBExpertStatus(theDeviceRef, "USBHIDMouseModule: Release mouse buttons", theDeviceRef);
		theMouseData.mouse.buttons = 0;
		theMouseData.mouse.XDelta = 1;
		theMouseData.mouse.YDelta = 1;
		(*myMousePB.pSHIMInterruptRoutine)(myMousePB.interruptRefcon, (void *)&theMouseData);
	}
	*/
	
	if (myMousePB.pCursorDeviceInfo != 0)				
	{
		USBExpertStatus(theDeviceRef, "USBHIDMouseModule: Remove CDM device", theDeviceRef);
		CursorDeviceDisposeDevice(myMousePB.pCursorDeviceInfo);
		myMousePB.pCursorDeviceInfo = 0;
	}
	
	if (myMousePB.pFullConfigDescriptor != nil)
	{
		if (myMousePB.pipeRef)
		{
			USBExpertStatus(theDeviceRef, "USBHIDMouseModule: Aborting interrupt pipe", theDeviceRef);
			USBAbortPipeByReference(myMousePB.pipeRef);
		}
		myMousePB.pb.usbReference = theDeviceRef;
		myMousePB.pb.pbVersion = kUSBCurrentPBVersion;
		myMousePB.pb.usbFlags = 0;
		myMousePB.pb.usbRefcon = 0; 			
		myMousePB.pb.usbBuffer = myMousePB.pFullConfigDescriptor;		
		myMousePB.pb.usbCompletion = (USBCompletion)-1;
		
		myErr = USBDeallocMem(&myMousePB.pb);
	}
	
	return (OSStatus)noErr;
}

