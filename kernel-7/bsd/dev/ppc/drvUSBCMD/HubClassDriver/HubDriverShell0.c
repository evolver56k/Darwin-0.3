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
	File:		HubDriverShell.c

	Contains:	Toms wrapper for BT's hub driver. Just the header block etc

	Version:	Neptune 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(TC)	Tom Clark
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	  <USB6>	 9/10/98	BT		Add are we finished
	  <USB5>	  9/4/98	TC		Hack to test DriverNotify.
	  <USB4>	 4/14/98	BT		Do removed devices when killed
	  <USB3>	  4/9/98	BT		Use USB.h
		 <2>	  3/6/98	CJK		change to get driver version number from version.h
		 <1>	 2/24/98	BT		first checked in
	   <15*>	 2/19/98	BT		Add debbugger
		<15>	  2/8/98	BT		Power allocation stuff
		<14>	  2/6/98	BT		Power allocation stuff
		<13>	  2/4/98	BT		Clean up after TOms changes
		<12>	  2/3/98	TC		Update to use latest DispatchTable definition.
		<11>	  2/2/98	TC		Make vendor,product & version match root hub simulator
		<10>	 1/26/98	CJK		Change to use USBDeviceDescriptor (instead of just
									devicedescriptor)
		 <9>	 1/26/98	BT		Fix name changes
		 <8>	 1/26/98	BT		Mangle names after design review
		 <7>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <6>	 1/14/98	BT		Change to USBClassDriver.h
		 <5>	 1/13/98	BT		Making timers internal
		 <4>	12/18/97	BT		Changes to dispatch table. Add notify and tickle
		 <3>	12/17/97	CJK		Change to use USBDeviceDefines for Class & Subclass values.
		 <2>	12/17/97	BT		Add file header
		 <1>	12/17/97	BT		First time checkin
*/

#include <Types.h>
#include <Devices.h>
#include <DriverServices.h>

#include <USB.h>
#include "hub.h"
#include "HubClassVersion.h"

//------------------------------------------------------
//
// Protos
//
//------------------------------------------------------
static OSStatus hubDriverValidateHW(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);
static OSStatus hubDriverInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc,
									UInt32 busPowerAvailable);
static OSStatus hubDriverInitInterface(
			UInt32 						interfaceNum, 
			USBInterfaceDescriptorPtr 	pInterface, 
			USBDeviceDescriptorPtr 		pDevice, 
			USBDeviceRef 				device);
static OSStatus hubDriverFinalize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc);
static OSStatus	hubDriverNotifyProc(UInt32 	notification, void *pointer, UInt32 refcon);

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
	0x03e8,									// vendor = Atmel
	0x3312,									// product, hub in Yosemite
	0x0001,									// version of product
	0,										// protocol = not device specific
	
	// Interface Info	(* I don't think this would always be required...*)				
	0,										// Configuration Value
	0,										// Interface Number
	0,										// Interface Class
	0, 										// Interface SubClass
	0,										// Interface Protocol
		
	
	// Driver Info
	"\pUSBHub0Apple",						// Driver name for Name Registry
	kUSBHubClass,							// Device Class  (from USBDeviceDefines.h)
	0,										// Device Subclass (for hubs that use zero)
	kHUBHexMajorVers, 
	kHUBHexMinorVers, 
	kHUBCurrentRelease, 
	kHUBReleaseStage,						// version of driver
	
	// Driver Loading Info
	0 //kUSBDoNotMatchGenericDevice			// Flags (currently undefined)
};

	
USBClassDriverPluginDispatchTable TheClassDriverPluginDispatchTable =
{
	kClassDriverPluginVersion,			// Version of this structure
	hubDriverValidateHW,				// Hardware Validation Procedure
	hubDriverInitialize,				// Initialization Procedure
	hubDriverInitInterface,				// Interface Initialization Procedure
	hubDriverFinalize,					// Finalization Procedure
	hubDriverNotifyProc,				// Driver Notification Procedure
};

// Hardware Validation
// Called upon load by Expert
OSStatus hubDriverValidateHW(USBDeviceRef device, USBDeviceDescriptor *desc)
{
	device = 0;
	desc = 0;
	return (OSStatus)noErr;
}

// Initialization function
// Called upon load by Expert
static OSStatus hubDriverInitialize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc,
									UInt32 busPowerAvailable)
{
	HubDriverEntry(device, pDesc, busPowerAvailable);
	return (OSStatus)noErr;
}


// hubDriverInitInterface function
// Called to initialize driver for an individual interface - either by expert or
// internally by driver
OSStatus hubDriverInitInterface(
			UInt32 						interfaceNum, 
			USBInterfaceDescriptor		*interfaceDesc, 
			USBDeviceDescriptor			*deviceDesc, 
			USBDeviceRef 				device)
{
	interfaceNum = 0;
	interfaceDesc = 0;
	deviceDesc = 0;
	device = 0;
	
	return (OSStatus)noErr;
}


// Termination function
// Called by Expert when driver is being shut down
OSStatus hubDriverFinalize(USBDeviceRef device, USBDeviceDescriptorPtr pDesc)
{
	pDesc = 0;	// Not used, saves message
	return(killHub(device));
}


static OSStatus	hubDriverNotifyProc(UInt32 	notification, void *pointer, UInt32 refcon)
{
OSStatus	status = noErr;

	switch (notification)
	{
		case kNotifyDriverBeingRemoved:
			break;
		case kNotifyHubEnumQuery:
				return(HubAreWeFinishedYet());
			break;
		default:
			break;
	
	
	} // switch
	
	pointer = 0;
	refcon = 0;
	return(status);
}
