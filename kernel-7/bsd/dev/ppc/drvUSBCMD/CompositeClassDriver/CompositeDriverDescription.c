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
	File:		CompositeDriverDescription.c

	Contains:	Composite Class Driver Definition Header

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(DF)	David Ferguson
		(BT)	Barry Twycross
		(CJK)	Craig Keithley

	Change History (most recent first):

	  <USB7>	  9/4/98	TC		Update CompositeDriverNotifyProc to use version 1.1 of dispatch
									table.
	  <USB6>	 8/25/98	BT		Isoc name changes
	  <USB5>	  6/8/98	CJK		add bus power available to DeviceInitialize params
	  <USB4>	  6/5/98	CJK		clean up/straighten up Initialize routine
	  <USB3>	 4/27/98	DF		Don't load against a matching interface!
	  <USB2>	  4/9/98	CJK		change to use USB.h
		 <1>	  4/7/98	CJK		first checked in as USBCompositeClassDriver
		<27>	  4/2/98	CJK		Change kUSBCompound… to kUSBComposite…
		<26>	 3/17/98	CJK		Replace "};" with just "}". Moved function prototypes to header
									file. Changed pragma unuseds so that MW doesn't complain about
									them.
		<25>	  3/6/98	CJK		Add dispatch table for add/remove simulation
		<24>	  3/5/98	CJK		Add include of version.h file. Change descriptor header to use
									the defines from the version.h file.
		<23>	 2/26/98	CJK		remove driver services mem alloc/dealloc function calls
		<22>	  2/9/98	CJK		Add pragma unused for buspoweravailable
		<21>	  2/9/98	CJK		remove HIDEmulation.h include
		<20>	  2/9/98	BT		Stub in new functions
		<19>	  2/6/98	BT		Power allocation stuff
		<18>	  2/6/98	CJK		remove debugstr. Add pragma unused
		<17>	  2/4/98	CJK		Remove expert entry proc.
		<16>	  2/3/98	CJK		change pFullConfig field name to pFullConfigDescriptor.
		<15>	  2/2/98	CJK		change vendor/product ids back to 0
		<14>	 1/30/98	CJK		move HID Module dispatch table to HIDEmulation.c
		<13>	 1/27/98	CJK		Change dispatch table to have a separate HID API dispatch table.
		<12>	 1/26/98	CJK		Change to use USBDeviceDescriptor structure
		<11>	 1/26/98	BT		Mangle names after design review
		<10>	 1/26/98	CJK		Add other dispatch functions needed by the ADB/USB shim.
		 <9>	 1/23/98	CJK		Add pFullConfig MemDeAlloc call.
		 <8>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <7>	 1/14/98	BT		Getting rid of time entry, change to USBClassDriver.h
		 <6>	 1/14/98	BT		Removing outdated pre expert stuff
		 <5>	 1/14/98	CJK		Change name registry name.
		 <4>	  1/8/98	CJK		Moved the initialize & finalize routines into this file (allows
									them to be static and thus not have the symbols exported).
		 <3>	  1/6/98	CJK		Add dispatch table entry points needed by expert.
		 <2>	12/17/97	CJK		Add BBS file header
		 <1>	12/17/97	CJK		First time checkin
*/
/*naga
#include <Types.h>
#include <Devices.h>
#include <DriverServices.h>
#include <USB.h>
*/
#include "../driverservices.h"  
#include "../USB.h"

#include "CompositeClassDriver.h"
#include "CompositeClassVersion.h"

usbCompositePBStruct newInterfacesPB[2];


OSStatus CompositeDriverInitInterface(
			UInt32 						interfaceNum, 
			USBInterfaceDescriptor		*interfaceDesc, 
			USBDeviceDescriptor			*deviceDesc, 
			USBDeviceRef 				device);

//------------------------------------------------------
//
//	This is the driver description structure that the expert looks for first.
//  If it's here, the information within is used to match the driver
//  to the device whose descriptor was passed to the expert.
//	Information in this block is also used by the expert when an
//  entry is created in the Name Registry.
//
//------------------------------------------------------
/*naga
USBDriverDescription	TheUSBDriverDescription = 
{
	// Signature info
	kTheUSBDriverDescriptionSignature,
	kInitialUSBDriverDescriptor,
	
	// Device Info
	{0,										// vendor = not device specific
	0,										// product = not device specific
	0,										// version of product = not device specific
	0,},										// protocol = not device specific
	
	// Interface Info	(* I don't think this would always be required...*)				
	0,										// Configuration Value
	0,										// Interface Number
	0,										// Interface Class
	0, 										// Interface SubClass
	0,										// Interface Protocol
		
	
	// Driver Info
	"USBCompositeDevice",					// Driver name for Name Registry
	kUSBCompositeClass,						// Device Class  (from USBDeviceDefines.h)
	kUSBCompositeSubClass,					// Device Subclass 
	kCMPHexMajorVers, 
	kCMPHexMinorVers, 
	kCMPCurrentRelease, 
	kCMPReleaseStage,						// version of driver
	
	// Driver Loading Info
	kUSBDoNotMatchInterface					// Please don't load us as an interface driver.
};
naga */
USBClassDriverPluginDispatchTable TheClassDriverPluginDispatchTable =
{
	kClassDriverPluginVersion,				// Version of this structure
	CompositeDriverValidateHW,				// Hardware Validation Procedure
	CompositeDriverInitialize,				// Initialization Procedure
	CompositeDriverInitInterface,			// Interface Initialization Procedure
	CompositeDriverFinalize,					// Finalization Procedure
	CompositeDriverNotifyProc				// Driver Notification Procedure
};

// hubDriverInitInterface function
// Called to initialize driver for an individual interface - either by expert or
// internally by driver
OSStatus CompositeDriverInitInterface(
			UInt32 						interfaceNum, 
			USBInterfaceDescriptor		*interfaceDesc, 
			USBDeviceDescriptor			*deviceDesc, 
			USBDeviceRef 				device)
{
#pragma unused (interfaceNum)
#pragma unused (interfaceDesc)
#pragma unused (deviceDesc)
#pragma unused (device)

	return (OSStatus)kUSBNoErr;
}

OSStatus	CompositeDriverNotifyProc(UInt32 	notification, void *pointer, UInt32 refcon)
{
#pragma unused (pointer)
#pragma unused (notification)
#pragma unused (refcon)
	return(kUSBNoErr);
}

// Hardware Validation
// Called upon load by Expert
OSStatus CompositeDriverValidateHW(USBDeviceRef device, USBDeviceDescriptor *desc)
{
#pragma unused (device)
#pragma unused (desc)

	return (OSStatus)kUSBNoErr;
}

// Initialization function
// Called upon load by Expert
OSStatus 	CompositeDriverInitialize (USBDeviceRef device, USBDeviceDescriptorPtr pDesc,  UInt32 busPowerAvailable)
{
#pragma unused (busPowerAvailable)
	// until we get going, it's okay to accept a call to finalize
	//naga newInterfacesPB[index].okayToFinalizeFlag = true;
    if(pDesc->protocol == 0)
          newInterfacesPB[0].okayToFinalizeFlag = true;
    else newInterfacesPB[1].okayToFinalizeFlag = true;
	DeviceInitialize(device, pDesc, busPowerAvailable);
	return (OSStatus)kUSBNoErr;
}

// Termination function
// Called by Expert when driver is being shut down
OSStatus CompositeDriverFinalize(USBDeviceRef theDeviceRef, USBDeviceDescriptorPtr pDesc)
{
#pragma unused (pDesc)
UInt32 i;
int index;
OSStatus myErr;

	// If all the interfaces have been examined and had their interface
	// drivers loaded, then go ahead and removed them all
	// otherwise just return with a device busy error code.
        if(pDesc->protocol == 0)
            index = 0;
        else index = 1;
	USBExpertStatus(newInterfacesPB[index].pb.usbReference, "Composite Driver: Finalize", newInterfacesPB[index].pb.usbStatus);
	if (theDeviceRef == newInterfacesPB[index].deviceRef)
	{
		if (newInterfacesPB[index].okayToFinalizeFlag)
		{
			USBExpertStatus(newInterfacesPB[index].pb.usbReference, "Composite Driver: Removing Interface drivers & InterfaceRefs", newInterfacesPB[index].pb.usbStatus);
			for (i=0; i < newInterfacesPB[index].interfaceCount; i++)
			{
				USBExpertRemoveInterfaceDriver(newInterfacesPB[index].interfaceRefArray[i]);
				InitParamBlock(newInterfacesPB[index].deviceRef, &newInterfacesPB[index].pb);
				newInterfacesPB[index].pb.usbRefcon = 0; 			
				newInterfacesPB[index].pb.usb.cntl.WIndex = i;
				newInterfacesPB[index].pb.usbCompletion = (USBCompletion)-1;
				myErr = USBDisposeInterfaceRef(&newInterfacesPB[index].pb);
			}
			
			InitParamBlock(newInterfacesPB[index].deviceRef, &newInterfacesPB[index].pb);
			newInterfacesPB[index].pb.usbRefcon = 0; 			
			newInterfacesPB[index].pb.usbBuffer = newInterfacesPB[index].pFullConfigDescriptor;		
			newInterfacesPB[index].pb.usbCompletion = (USBCompletion)-1;
			myErr = USBDeallocMem(&newInterfacesPB[index].pb);
			return (OSStatus)kUSBNoErr;
		}
		else
		{
			return (OSStatus)kUSBDeviceBusy;
		}
	}
	else
	{
		USBExpertFatalError(newInterfacesPB[index].pb.usbReference, kUSBInternalErr, "Composite Driver - Finalize received unknown deviceRef", newInterfacesPB[index].pb.usbRefcon);
		return (OSStatus)kUSBUnknownDeviceErr;
	}
}

void DeviceInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDeviceDescriptor, UInt32 busPowerAvailable)
{
Boolean beenThereDoneThat = false;
int index;
    if(pDeviceDescriptor->protocol == 0)
         index = 0;
    else index = 1;
kprintf("USB: in DeviceInitialize Composite Driver\n");
	if(beenThereDoneThat)
	{
		USBExpertFatalError(device, kUSBInternalErr, "Composite driver is not reentrant!", 0);
		return;
	}
//naga	beenThereDoneThat = true;
	newInterfacesPB[index].okayToFinalizeFlag = false;
	
	newInterfacesPB[index].deviceRef = device;	
	newInterfacesPB[index].deviceDescriptor = *pDeviceDescriptor;	
	
	newInterfacesPB[index].busPowerAvailable = busPowerAvailable;							
	newInterfacesPB[index].delayLevel = 0;							
	newInterfacesPB[index].transDepth = 0;							
	newInterfacesPB[index].retryCount = kCompositeRetryCount;
	newInterfacesPB[index].pFullConfigDescriptor = nil;
	
	InitParamBlock(device, &newInterfacesPB[index].pb);
	newInterfacesPB[index].pb.usbRefcon = kGetFullConfiguration0;			/* Start out at first state */
	
//	DebugStr("In Composite Driver");
	CompositeDeviceInitiateTransaction(&newInterfacesPB[index].pb);
}


