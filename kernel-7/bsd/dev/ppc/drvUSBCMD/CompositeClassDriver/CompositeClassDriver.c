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
	File:		CompositeClassDriver.c

	Contains:	Core functionality to Composite Class Driver

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(DF)	David Ferguson
		(BT)	Barry Twycross
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB12>	 8/25/98	BT		Isoc name changes
	 <USB11>	 8/11/98	CJK		add error checking to GetInterfaceDescriptor.  If
									GetInterfaceDescriptor has a problem then the Composite class
									driver won't try to load an interface driver for the device.
	 <USB10>	 6/11/98	CJK		add reserved params to USBExpertSetDevicePowerStatus. Wanted to
									get it into the USB.i before freezing C1. pass 0 for right now.
	  <USB9>	  6/8/98	CJK		add check of power required against power available
	  <USB8>	  6/6/98	DF		Increment the InterfaceIndex before checking to see if it's
									valid, pass the deviceRef to InstallInterfaceDriver
	  <USB7>	  6/5/98	CJK		implement finalize routine (which disposes of the interface refs
									and removes the interface drivers)
	  <USB6>	 5/20/98	BT		Set pbVersion correctly
	  <USB5>	 5/12/98	BT		New name for add interface
	  <USB4>	  5/2/98	DF		use pascal strings for errors
	  <USB3>	  4/9/98	CJK		change to use USB.h
	  <USB2>	  4/9/98	DF		Massaged to work with MasterInterfaces
		 <1>	  4/7/98	CJK		first checked in
		<36>	 3/31/98	CJK		start out at config 0
		<35>	 3/27/98	CJK		set default config to be 1, then step back to 0, if that doesn't
									work.
		<34>	 3/26/98	CJK		check for config #0 & config #1 when setting up the Composite
									device.  Some devices respond to 1, others to 0.
		<33>	 3/17/98	CJK		Replace "};" with just "}". Used ExtractPrototypes to create
									function prototypes for this file (placed the in the header
									files).
		<32>	 2/26/98	CJK		change to use USL's GetFullConfiguration function.
		<31>	 2/17/98	CJK		replace debugstrs with notify expert calls.
		<30>	 2/17/98	CJK		Change get configuration to byte swap the length
		<29>	 2/10/98	CJK		Change call to USBExpertNewDevice to use the USL's add interface
									call.
		<28>	  2/9/98	CJK		add back in set configuration
		<27>	  2/9/98	CJK		remove HID Emulation related items (no more include of
									HIDEmulation.h, etc.)
		<26>	  2/9/98	CJK		remove unneeded HID support. Now in mouse & keyboard modules.
									The add new interface call now properly loads the HID Modules
									for mouse & keyboard.
		<25>	  2/8/98	BT		Power allocation stuff
		<24>	  2/6/98	CJK		add call to InitUSBKeyboard.  (Disabled for now)
		<23>	  2/6/98	CJK		Crank delay back down to 10ms.
		<22>	  2/4/98	BT		Add ref/port info to expert notify]
		<21>	  2/3/98	CJK		Change set configuration USB Direction to "USBOut". Changed
									sequence of states to be "get configuration, set configuration,
									set protocol, get HID descriptor"
		<20>	  2/2/98	CJK		change retry to simply abort if limit is reached.  Also changed
									sequence of getting the configuration data & then setting the
									selected configuration.  Used to do a set configuration before
									reading the first one.
		<19>	  2/2/98	CJK		change readhidreport state to only copy the new hid data to the
									old hid data *after* doing a hid notification.
		<18>	 1/30/98	CJK		rework test for mouse/keyboard device types
		<17>	 1/27/98	CJK		Fixed change history... Previously said something about
									commenting out code
		<16>	 1/27/98	CJK		Work on interface set protocol & get report handling.  Wasn't
									setting the direction or target properly.
		<15>	 1/26/98	CJK		Add call to USBExpertNewDevice routine
		<14>	 1/26/98	BT		Mangle names after design review
		<13>	 1/23/98	CJK		Change to use USBServicesLib USBDelay function.
		<12>	 1/23/98	CJK		Implement HID data change detection
		<11>	 1/23/98	CJK		Add call to configuration parser.
		<10>	 1/22/98	CJK		Work on get configuration code.
		 <9>	 1/22/98	CJK		Change state tag to async operation
		 <8>	 1/20/98	CJK		Add states to handle reading the configuration (both the base
									size and full size).
		 <7>	 1/15/98	CJK		Change ClassDrivers.h to USBClassDrivers.h
		 <6>	 1/14/98	BT		Remove superceded expert stuff
		 <5>	 1/14/98	CJK		Add setup of version field
		 <4>	  1/8/98	CJK		Moved initialize & finalize routines over to
									CompositeClassDescription.c
		 <3>	  1/6/98	CJK		Add tasktime and expert entry procs
		 <2>	12/17/97	CJK		Add basic functionality for mouse and keyboard.
		 <1>	12/17/97	CJK		First time checkin
*/

//naga#include "../types.h"
//naga#include "../devices.h"
//naga#include "../processes.h"
#include "../driverservices.h"
#include "../USB.h"

#include "CompositeClassDriver.h"
extern	usbCompositePBStruct myCompositePBRecord;


void InitParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock)
{
	paramblock->usbReference = theDeviceRef;
	paramblock->usbCompletion = (USBCompletion)CompositeDeviceCompletionProc;
	paramblock->pbLength = sizeof(usbCompositePBStruct);
	paramblock->pbVersion = kUSBCurrentPBVersion;
	paramblock->usb.cntl.WIndex = 0; 			
	paramblock->usbBuffer = nil;		
	paramblock->usbStatus = kUSBNoErr;
	paramblock->usbReqCount = 0;
	paramblock->usbActCount = 0;
	paramblock->usb.cntl.WValue = 0;
	paramblock->usbFlags = 0;
}


static Boolean immediateError(OSStatus err)
{
	return((err != kUSBPending) && (err != kUSBNoErr) );
}

void CompositeDeviceInitiateTransaction(USBPB *pb)
{
register usbCompositePBStruct *pCompositePB;
OSStatus myErr;

	pCompositePB = (usbCompositePBStruct *)(pb);
	pCompositePB->transDepth++;
	if ((pCompositePB->transDepth < 0) || (pCompositePB->transDepth > 1))
	{
	
		USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: Illegal Transaction Depth", pCompositePB->pb.usbRefcon);
	}
	do 
	{
		switch(pCompositePB->pb.usbRefcon & ~kRetryTransaction)
		{
			case kGetFullConfiguration0:
				InitParamBlock(pCompositePB->deviceRef, &pCompositePB->pb);
				pCompositePB->pb.usb.cntl.WIndex = 0; 			/* First try configuration 0, if it doesn't succeed, then try config 1*/
				
				pCompositePB->pb.usbRefcon |= kCompletionPending;
				pCompositePB->pb.usbCompletion = (USBCompletion)CompositeDeviceCompletionProc;
				myErr = USBGetFullConfigurationDescriptor(pb);
				if(immediateError(myErr))
				{
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: USBGetFullConfiguration (#0) - immediate error", myErr);
				}
				break;
			
			case kGetFullConfiguration1:
				InitParamBlock(pCompositePB->deviceRef, &pCompositePB->pb);
				pCompositePB->pb.usb.cntl.WIndex = 1; 			/* Try configuration 1 (some devices seem to expect config 0, others config 1 */
				pCompositePB->pb.usbRefcon |= kCompletionPending;
				
				pCompositePB->pb.usbCompletion = (USBCompletion)CompositeDeviceCompletionProc;
				myErr = USBGetFullConfigurationDescriptor(pb);
				if(immediateError(myErr))
				{
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: USBGetFullConfiguration (#1) - immediate error", myErr);
				}
				break;
			
			case kSetConfig:
				InitParamBlock(pCompositePB->deviceRef, &pCompositePB->pb);
				pCompositePB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBStandard, kUSBDevice);
				
				pCompositePB->pb.usb.cntl.BRequest = kUSBRqSetConfig;
				pCompositePB->pb.usb.cntl.WValue = pCompositePB->pFullConfigDescriptor->configValue; 		/* Use configuration ID value from descriptor */
				
				pCompositePB->pb.usbRefcon |= kCompletionPending;
				
				pCompositePB->pb.usbCompletion = (USBCompletion)CompositeDeviceCompletionProc;
				myErr = USBDeviceRequest(pb);
				if(immediateError(myErr))
				{
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: kSetConfig - immediate error", myErr);
				}
				break;
				
			case kNewInterfaceRef:
				InitParamBlock(pCompositePB->deviceRef, &pCompositePB->pb);
				// Note: pCompositePB->usb.cntl.WIndex will be set to zero by InitParamBlock
				// so set it again to pCompositePB->interfaceIndex before calling USBNewInterfaceRef
				pCompositePB->pb.usb.cntl.WIndex = pCompositePB->interfaceIndex;
				pCompositePB->pb.usbRefcon |= kCompletionPending;
				pCompositePB->pb.usbCompletion = (USBCompletion)CompositeDeviceCompletionProc;

				myErr = USBNewInterfaceRef(pb);
				if(immediateError(myErr))
				{
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: kNewInterfaceRef - immediate error", myErr);
				}
				break;
				
			default:
				USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver - Transaction initiated with bad refcon value", pCompositePB->pb.usbRefcon);
				pCompositePB->pb.usbRefcon = kUndefined + kExitDriver;
				break;
		}
	} while (false);
	
// At this point the control is returned to the system.  If a USB transaction
// has been initiated, then it will call the Complete procs
// (below) to handle the results of the transaction.
}

void CompositeDeviceCompletionProc(USBPB *pb)
{
OSStatus myErr;
register usbCompositePBStruct *pCompositePB;
USBInterfaceDescriptorPtr pInterfaceDescriptor;
UInt32 i;

	pCompositePB = (usbCompositePBStruct *)(pb);
	pCompositePB->transDepth--;
	if ((pCompositePB->transDepth < 0) || (pCompositePB->transDepth > 1))
	{
		USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver - Illegal Transaction Depth", pCompositePB->transDepth);
	}
	
	if((pCompositePB->pb.usbStatus != kUSBNoErr) && (pCompositePB->pb.usbStatus != kUSBPending))
	{
		USBExpertStatus(pCompositePB->pb.usbReference, "Composite Driver: Completion Error", pCompositePB->pb.usbStatus);
		pCompositePB->pb.usbRefcon &= ~(kCompletionPending + kExitDriver);
		pCompositePB->pb.usbRefcon |= kRetryTransaction;
		pCompositePB->retryCount--;
		if (!pCompositePB->retryCount)
		{
			if (pCompositePB->pb.usbRefcon == kGetFullConfiguration1)
			{
				pCompositePB->pb.usbRefcon = kGetFullConfiguration0;
			}
			else
			{
				USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: Too many retries", pCompositePB->pb.usbRefcon);
				pCompositePB->pb.usbRefcon = kExitDriver;
				return;
			}
		}
	}
	else
	{
		pCompositePB->pb.usbRefcon &= ~kRetryTransaction;
		pCompositePB->retryCount = kCompositeRetryCount;
	}

	if (pCompositePB->pb.usbRefcon & kCompletionPending)			 
	{												
		pCompositePB->pb.usbRefcon &= ~(kCompletionPending + kExitDriver);
		switch(pCompositePB->pb.usbRefcon)
		{
			case kGetFullConfiguration0:
			case kGetFullConfiguration1:
//				DebugStr("Check power");
				pCompositePB->pFullConfigDescriptor = pCompositePB->pb.usbBuffer;
				if (pCompositePB->pFullConfigDescriptor == nil)
				{
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver: USBGetFullConfiguration - pointer is nil", pCompositePB->pb.usbRefcon);
					pCompositePB->pb.usbRefcon = kExitDriver;
					break;
				}
				
				BlockCopy(   (void *)pCompositePB->pFullConfigDescriptor, (void *)(&(pCompositePB->partialConfigDescriptor)), (Size)(sizeof(USBConfigurationDescriptor) )  );
				if (pCompositePB->pFullConfigDescriptor->maxPower <= pCompositePB->busPowerAvailable)
				{
					pCompositePB->pb.usbRefcon = kSetConfig;
				}
				else
				{
					USBExpertSetDevicePowerStatus(pCompositePB->pb.usbReference, 0, 0, kUSBDevicePower_BusPowerInsufficient, pCompositePB->busPowerAvailable, pCompositePB->pFullConfigDescriptor->maxPower);
					USBExpertFatalError(pCompositePB->pb.usbReference, kUSBDevicePowerProblem, "Composite Driver - Insufficient power for device", pCompositePB->pb.usbRefcon);
					pCompositePB->pb.usbRefcon = kExitDriver;
				}
				break;
				
			case kSetConfig:			/* get all the interface descriptors and save them */
				for (i=0; i < pCompositePB->partialConfigDescriptor.numInterfaces; i++)
				{
					pCompositePB->interfaceRefArray[i] = 0;
					myErr = GetInterfaceDescriptor(pCompositePB->pFullConfigDescriptor, (UInt32)i, &pInterfaceDescriptor);
					if (kUSBNoErr == myErr)
					{
						BlockCopy((void *)pInterfaceDescriptor, (void *)(&(pCompositePB->interfaceDescriptors[i])), (Size)(pInterfaceDescriptor->length));
					}
					else				/* if GetInterfaceDescriptor returned an error, then set the length to zero (so we know later) */
					{
						pCompositePB->interfaceDescriptors[i].length = 0;
					}
				}
				pCompositePB->interfaceIndex = 0;
				pCompositePB->interfaceCount = pCompositePB->partialConfigDescriptor.numInterfaces;
				pCompositePB->pb.usbRefcon = kNewInterfaceRef;
				break;
				
			case kNewInterfaceRef:
				/* save the new interface ref for this interface */
				pCompositePB->interfaceRefArray[pCompositePB->interfaceIndex] = pCompositePB->pb.usbReference;
				
				/* only install the interface driver if we had a valid interface descriptor */
				if (pCompositePB->interfaceDescriptors[pCompositePB->interfaceIndex].length != 0)
				{
					USBExpertInstallInterfaceDriver(pCompositePB->interfaceRefArray[pCompositePB->interfaceIndex], &pCompositePB->deviceDescriptor, &pCompositePB->interfaceDescriptors[pCompositePB->interfaceIndex], pCompositePB->deviceRef, 0);
				}
				
				/* advance to the next interface */
				pCompositePB->interfaceIndex++;
				
				/* if there's more interfaces, then just keep cycling through kNewInterfaceRef */
				if (pCompositePB->interfaceIndex < pCompositePB->interfaceCount)
				{
					pCompositePB->pb.usbRefcon = kNewInterfaceRef;
				}
				else
				{
					pCompositePB->pb.usbRefcon = kExitDriver;
				}
				break;
			
			default:
				USBExpertFatalError(pCompositePB->pb.usbReference, kUSBInternalErr, "Composite Driver - Transaction completed with a bad refcon value", pCompositePB->pb.usbRefcon);
				pCompositePB->pb.usbRefcon = kExitDriver;
				break;
		}
	}
	if (!(pCompositePB->pb.usbRefcon & kExitDriver))
		CompositeDeviceInitiateTransaction(pb);

	pCompositePB->okayToFinalizeFlag = true;
	pCompositePB->disposeCompletedFlag = true;
}


