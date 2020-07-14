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
	File:		KeyboardModule.c

	Contains:	HID Module for USB Keyboard

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BT)	Barry Twycross
		(DF)	David Ferguson
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB35>	10/28/98	CJK		[2281764]  use the supplied interface descriptor (for the ,
									rather than searching the configuration descriptor and using the
									one found.
	 <USB34>	 8/25/98	BT		Isoc name changes
	 <USB33>	 6/22/98	CJK		rename FindNextEndpointDescriptor to
									FindNextEndpointDescriptorImmediate (no functional change, just
									a name change)
	 <USB32>	 6/17/98	CJK		leave retries in place for non-pipe stall situations. That's the
									most compatible with the earlier versions of the driver... It
									feels pretty risky disabling that functionality this close to
									intro.
	 <USB31>	 6/17/98	CJK		change to check pipe status, if stalled, then try to clear it.
	 <USB30>	 6/17/98	CJK		change error handling to not retry on some types of errors
	 <USB29>	 6/16/98	CJK		change FindHIDInterfaceByProtocol to FindHIDInterfaceByNumber
	 <USB28>	 6/15/98	CJK		Change "USL Reported an error" message so that it shows the
									refcon (and not the deviceRef)
	 <USB27>	 6/11/98	CJK		Remove driverentry routine (not needed or used).
	 <USB26>	  6/8/98	CJK		eliminate PBVERSION1 define
	 <USB25>	  6/5/98	CJK		remove ExpertStatus calls; cluttering up expert log
	 <USB24>	 5/20/98	BT		Add V2 param block stuff
	 <USB23>	 5/20/98	CJK		change driver name from USBKeyboardModule to
									USBHIDKeyboardModule
	 <USB22>	 5/20/98	CJK		add prototype for SetParamBlock
	 <USB21>	 4/30/98	CJK		add a "send raw report" mode.  initialize rawdatamode and
									initialized flags appropriately.
	 <USB20>	 4/29/98	CJK		Back out, just for the Columbus demo, the changes since the
									1.0a9 release.
	 <USB19>	 4/29/98	CJK		add break where needed
	 <USB18>	 4/27/98	CJK		don't fatalerror if setidle never succeeds.  just give up after
									5 tries and move on to opening the pipe.
	 <USB17>	 4/26/98	CJK		Add clear param block routine.  Dramatically reorganized to
									match the "new" style used in the mouse module.  The keyboard
									module now searchs for an interface, and when it finds a
									keyboard interface, it locates the interrupt pipe.  No more hard
									coded assumptions!  Okay, so we're still assuming that keyboard
									reports (in boot protocol mode) are 8 bytes.
	 <USB16>	  4/9/98	CJK		change driver services to be a externally supplied header file
									(using < & >) rather than quotes.
	 <USB15>	  4/9/98	DF		Massaged to work with MasterInterfaces
		<14>	  4/8/98	CJK		add initialization of saved interrupt routine ptr.
		<13>	 3/26/98	CJK		remove init of delay field inside of my keyboard structure.
									Clean up USBExpertFatalError messages (to show USB error code,
									etc.)
		<12>	 3/25/98	CJK		Remove unneeded poll & delay code
		<11>	 3/24/98	CJK		turn on interrupt pipe usage.  This required some changes in the
									USL, as well as doing a "set_idle" in order to get the reports
									when keys were released.
		<10>	 3/17/98	CJK		Begin adding support for interrupt transactions. Change the
									occasion "};" to just "}", as MetroWerks has a problem with some
									"};".
		 <9>	 3/12/98	CJK		add initialization of the shim keyboard param block.
		 <8>	  3/2/98	CJK		remove include of KBDHIDEmulation.h
		 <7>	 2/26/98	CJK		change poll rate to 10ms.  Add support for LEDs
		 <6>	 2/17/98	CJK		remove "" from each notify expert call.
		 <5>	 2/16/98	CJK		change poll time to 8ms
		 <4>	 2/11/98	CJK		change debugstrs to USBExpertFatalError calls.
		 <3>	 2/11/98	CJK		remove unneeded states
		 <2>	 2/10/98	CJK		Correct change history (to reflect the keyboard module changes)
		 <1>	 2/10/98	CJK		First time checkin.  Cloned from Mouse HID Module.
*/
/*
#include <Types.h>
#include <Devices.h>
#include <processes.h>
*/
#include "../driverservices.h"
#include "../USB.h"

#include "KeyboardModule.h"

usbKeyboardPBStruct myKeyboardPB;
usbKeyboardPBStruct shimKeyboardPB;


static void KeyboardModuleDelay1CompletionProc(USBPB *pb);
void SetParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock);

void SetParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock)
{
	paramblock->usbReference = theDeviceRef;
	paramblock->pbVersion = kUSBCurrentPBVersion;
	paramblock->usb.cntl.WIndex = 0; 			
	paramblock->usbBuffer = nil;		
	paramblock->usbStatus = noErr;
	paramblock->usbReqCount = 0;
	paramblock->usb.cntl.WValue = 0;
	paramblock->usbFlags = 0;
}
static Boolean immediateError(OSStatus err)
{
	return((err != kUSBPending) && (err != noErr) );
}
void KeyboardModuleInitiateTransaction(USBPB *pb)
{
register usbKeyboardPBStruct *pKeyboardPB;
OSStatus myErr;

//A.W. 12/9/98 It seems this code is never called 
	pKeyboardPB = (usbKeyboardPBStruct *)(pb);
	pKeyboardPB->transDepth++;
	if (pKeyboardPB->transDepth < 0)
	{
printf("transdepth<0\n");
		USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: transDepth < 0 (initiation)", pKeyboardPB->pb.usbRefcon );
	}
	
//A.W. 12/9/98 It seems this code is never called 
	if (pKeyboardPB->transDepth > 1)
	{
printf("transdepth>1\n");
		USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: transDepth > 1 (initiation)", pKeyboardPB->pb.usbRefcon );
	}
	
	switch(pKeyboardPB->pb.usbRefcon & ~kRetryTransaction)
	{
		case kSetKeyboardLEDs:
			SetParamBlock(pKeyboardPB->deviceRef, &pKeyboardPB->pb);
			
			pKeyboardPB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBInterface);			
			
			pKeyboardPB->pb.usb.cntl.BRequest = kHIDRqSetReport;
			pKeyboardPB->pb.usb.cntl.WValue = (kHIDRtOutputReport << 8); 
			pKeyboardPB->pb.usb.cntl.WIndex = pKeyboardPB->interfaceDescriptor.interfaceNumber;
			
			pKeyboardPB->pb.usbBuffer = (Ptr)&pKeyboardPB->hidReport[0];
			pKeyboardPB->pb.usbReqCount = 1;
				
			pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
			pKeyboardPB->pb.usbRefcon |= kCompletionPending;
			
			myErr = USBDeviceRequest(&pKeyboardPB->pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: kSetKeyboardLEDs - immediate error", myErr);
			}
//A.W. 12/9/98 This code seems to be going through OK

			break;

		case kGetFullConfiguration:
			SetParamBlock(pKeyboardPB->deviceRef, &pKeyboardPB->pb);
			
			pKeyboardPB->pb.usbRefcon |= kCompletionPending;
			pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
			myErr = USBGetFullConfigurationDescriptor(pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pKeyboardPB->pb.usbReference, kUSBInternalErr, "USBHIDKeyboardModule: kGetFullConfiguration (ImmediateError)", myErr);
			}
			break;
			
		case kFindInterfaceAndSetProtocol:
			myErr = kbd_FindHIDInterfaceByNumber(pKeyboardPB->pFullConfigDescriptor, pKeyboardPB->interfaceDescriptor.interfaceNumber, &pKeyboardPB->pInterfaceDescriptor );	// find the HID interface
			if ((pKeyboardPB->pInterfaceDescriptor == NULL) || 
			    (myErr != noErr))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: Interface not found", myErr);
				pKeyboardPB->pb.usbRefcon = kReturnFromDriver;
			}
			
			SetParamBlock(pKeyboardPB->deviceRef, &pKeyboardPB->pb);
			
			pKeyboardPB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBInterface);			
			pKeyboardPB->pb.usb.cntl.BRequest = kHIDRqSetProtocol;
			pKeyboardPB->pb.usb.cntl.WValue = kHIDBootProtocolValue; 
			pKeyboardPB->pb.usb.cntl.WIndex = pKeyboardPB->interfaceDescriptor.interfaceNumber;
			pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
			
			pKeyboardPB->pb.usbRefcon |= kCompletionPending;
			myErr = USBDeviceRequest(&pKeyboardPB->pb);
			if (immediateError(myErr))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: kSetProtocol (ImmediateError)", myErr);
			}
			break;
			
		case kSetIdleRequest:
			SetParamBlock(pKeyboardPB->deviceRef, &pKeyboardPB->pb);
			
			pKeyboardPB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBInterface);			
			
			pKeyboardPB->pb.usb.cntl.BRequest = kHIDRqSetIdle;
			pKeyboardPB->pb.usb.cntl.WValue = ((24/4)<<8); 				// force a read completion if idle for more than 24ms
			pKeyboardPB->pb.usb.cntl.WIndex = pKeyboardPB->interfaceDescriptor.interfaceNumber;
			pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
			
			pKeyboardPB->pb.usbRefcon |= kCompletionPending;

			myErr = USBDeviceRequest(&pKeyboardPB->pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: kSetIdleRequest - immediate error", myErr);
			}
			break;

		case kFindAndOpenInterruptPipe:
			pKeyboardPB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBInterface);
			pKeyboardPB->pb.usbFlags = kUSBIn;
			pKeyboardPB->pb.usbClassType = kUSBInterrupt;
			pKeyboardPB->pb.usbOther = 0;			
			pKeyboardPB->pb.usbBuffer = pKeyboardPB->pInterfaceDescriptor;
			pKeyboardPB->pb.usbReqCount = (UInt8*)pKeyboardPB->pInterfaceDescriptor - (UInt8*)pKeyboardPB->pFullConfigDescriptor;
			myErr = USBFindNextEndpointDescriptorImmediate( &pKeyboardPB->pb );
			if((immediateError(myErr)) || (pKeyboardPB->pb.usbBuffer == nil))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: Endpoint not found", myErr);
				pKeyboardPB->pb.usbRefcon = kReturnFromDriver;
			}
			else
			{
				pKeyboardPB->pEndpointDescriptor = (USBEndPointDescriptorPtr) pKeyboardPB->pb.usbBuffer;

				SetParamBlock(pKeyboardPB->deviceRef, &pKeyboardPB->pb);
				
				pKeyboardPB->pb.usbFlags = kUSBIn;
				pKeyboardPB->pb.usb.cntl.WValue = 0x08;
				pKeyboardPB->pb.usbClassType = kUSBInterrupt;
				pKeyboardPB->pb.usbOther = (pKeyboardPB->pEndpointDescriptor->endpointAddress & 0x0f);
				pKeyboardPB->pb.usbRefcon |= kCompletionPending;
				pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
		
				myErr = USBOpenPipe( &pKeyboardPB->pb );
				if(immediateError(myErr))
				{
					USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: USBOpenPipe Failed (ImmediateError)", myErr);
				}
			}
			break;
		
		case kReadInterruptPipe:
			SetParamBlock(pKeyboardPB->pipeRef, &pKeyboardPB->pb);

			pKeyboardPB->pb.usbBuffer = (Ptr)pKeyboardPB->hidReport;
			pKeyboardPB->pb.usbReqCount = 0x08;
			pKeyboardPB->pb.usb.cntl.WIndex = pKeyboardPB->interfaceDescriptor.interfaceNumber;	
			pKeyboardPB->pb.usbCompletion = (USBCompletion)KeyboardCompletionProc;
			
			pKeyboardPB->pb.usbRefcon |= kCompletionPending;
	kprintf("***Issuing keyboard USBIntRead\n");
			myErr = USBIntRead(&pKeyboardPB->pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: Read Interrupt Pipe (ImmediateError)", myErr);
			}
			break;
			
		default:
			USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule - Transaction completed with bad refcon value", pKeyboardPB->pb.usbRefcon );
			pKeyboardPB->pb.usbRefcon = kUndefined + kReturnFromDriver;
			break;
	}
	
// At this point the control is returned to the system.  If a USB transaction
// has been initiated, then it will call the Complete procs
// (below) to handle the results of the transaction.
}

void KeyboardCompletionProc(USBPB *pb)
{
unsigned char	* errstring;
register usbKeyboardPBStruct *pKeyboardPB;
USBPipeState pipeState;

	pKeyboardPB = (usbKeyboardPBStruct *)(pb);
	pKeyboardPB->transDepth--;
	if (pKeyboardPB->transDepth < 0)
	{
printf("transDepth<0\n");
		USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: transDepth < 0 (completion)", pKeyboardPB->pb.usbRefcon );
	}
	
	if (pKeyboardPB->transDepth > 1)
	{
printf("transDepth>1\n");
		USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule: transDepth > 1 (completion)", pKeyboardPB->pb.usbRefcon );
	}
	
	if(pKeyboardPB->pb.usbStatus != noErr)													// was there an error?
	{
		switch(pKeyboardPB->pb.usbRefcon & 0x0fff)											// yes, so show where the error occurred
		{
			case kSetKeyboardLEDs:  			errstring = "USBHIDKeyboardModule: Error during SetKeyboardLEDs"; break;
			case kGetFullConfiguration:  		errstring = "USBHIDKeyboardModule: Error during GetFullConfiguration"; break;
			case kFindInterfaceAndSetProtocol:	errstring = "USBHIDKeyboardModule: Error during FindInterfaceAndSetProtocol"; break;
			case kFindAndOpenInterruptPipe:  	errstring = "USBHIDKeyboardModule: Error during FindAndOpenInterruptPipe"; break;
			case kReadInterruptPipe: 	 		errstring = "USBHIDKeyboardModule: Error during ReadInterruptPipe"; break;
			default: 	 						errstring = "USBHIDKeyboardModule: Error occurred, but state is unknown"; break;
		};
		USBExpertFatalError(pKeyboardPB->deviceRef, pKeyboardPB->pb.usbStatus, errstring, (pKeyboardPB->pb.usbRefcon & 0x0fff));
		
		pKeyboardPB->pb.usbRefcon &= ~(kCompletionPending + kReturnFromDriver);				// set up to retry the transaction
		pKeyboardPB->pb.usbRefcon |= kRetryTransaction;
		pKeyboardPB->retryCount--;
		
		if ((!pKeyboardPB->retryCount)	|| (pKeyboardPB->pb.usbStatus == kUSBAbortedError))	// have we exhausted the retries?
		{																					// or received an abort?
			USBExpertStatus(pKeyboardPB->deviceRef, "USBHIDKeyboardModule: Pipe abort or unable to recover from error", pKeyboardPB->deviceRef);
			pKeyboardPB->pb.usbRefcon = kReturnFromDriver;									// if so, just exit.
		}
		else																				// if it didn't abort and there's retries left, then...
		{
			if (pKeyboardPB->pipeRef)														// check if the pipe is open.
			{
				USBGetPipeStatusByReference(pKeyboardPB->pipeRef, &pipeState);				// yes, so what it's state?
				if (pipeState != kUSBActive)												// if it's not active, try to clear it.  It might be stalled...
				{
					USBExpertStatus(pKeyboardPB->deviceRef, "USBHIDKeyboardModule: Pipe is open and stalled, clearing stall...", pKeyboardPB->deviceRef);
					USBClearPipeStallByReference(pKeyboardPB->pipeRef);
				}
			}
		}
	}
	else
	{
		pKeyboardPB->pb.usbRefcon &= ~kRetryTransaction;
		pKeyboardPB->retryCount = kKeyboardRetryCount;
	}

	if (pKeyboardPB->pb.usbRefcon & kCompletionPending)			 
	{												
		pKeyboardPB->pb.usbRefcon &= ~(kCompletionPending + kReturnFromDriver);
		switch(pKeyboardPB->pb.usbRefcon)
		{
			case kSetKeyboardLEDs:
				pKeyboardPB->pb.usbRefcon = kReturnFromDriver;
				break;
				
			case kGetFullConfiguration:
				pKeyboardPB->pFullConfigDescriptor = pKeyboardPB->pb.usbBuffer;
				if (pKeyboardPB->pFullConfigDescriptor == nil)
				{
					USBExpertFatalError(pKeyboardPB->pb.usbReference, kUSBInternalErr, "USBHIDKeyboardModule: USBGetFullConfiguration - pointer is nil", pKeyboardPB->pb.usbRefcon);
					pKeyboardPB->pb.usbRefcon = kReturnFromDriver;
				}
				else
				{
					pKeyboardPB->pb.usbRefcon = kFindInterfaceAndSetProtocol;
				}
				break;
				
			case kFindInterfaceAndSetProtocol:
				pKeyboardPB->pb.usbRefcon = kSetIdleRequest;
				break;
				
			case kSetIdleRequest:
				pKeyboardPB->pb.usbRefcon = kFindAndOpenInterruptPipe;
				break;
				
			case kFindAndOpenInterruptPipe:
				pKeyboardPB->pipeRef = pKeyboardPB->pb.usbReference;
				pKeyboardPB->pb.usbRefcon = kReadInterruptPipe;
				break;
				
			case kReadInterruptPipe:
				kbd_NotifyRegisteredHIDUser(pKeyboardPB->hidDeviceType, pKeyboardPB->hidReport);
				pKeyboardPB->pb.usbRefcon = kReadInterruptPipe;
				break;
				
			default:
				USBExpertFatalError(pKeyboardPB->deviceRef, kUSBInternalErr, "USBHIDKeyboardModule - Transaction completed with bad refcon value", pKeyboardPB->pb.usbRefcon );
				pKeyboardPB->pb.usbRefcon = kUndefined + kReturnFromDriver;
				break;
		}
	}
	if (!(pKeyboardPB->pb.usbRefcon & kReturnFromDriver))
		KeyboardModuleInitiateTransaction(pb);
}


void kbd_InterfaceEntry(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterfaceDescriptor, USBDeviceDescriptorPtr pDeviceDescriptor, USBDeviceRef device)
{
#pragma unused (interfacenum)

static Boolean beenThereDoneThat = false;

	if(beenThereDoneThat)
	{
		USBExpertFatalError(device, kUSBInternalErr, "USBHIDKeyboardModule is not reentrant", 12);
		return;
	}
//Hot Plug n Play
//naga	beenThereDoneThat = true;
	
//	DebugStr("In Keyboard Module Interface entry routine");
	shimKeyboardPB.deviceDescriptor = *pDeviceDescriptor;				
	shimKeyboardPB.interfaceDescriptor = *pInterfaceDescriptor;				
	shimKeyboardPB.transDepth = 0;							
	shimKeyboardPB.retryCount = kKeyboardRetryCount;
	shimKeyboardPB.pSHIMInterruptRoutine = nil;
	shimKeyboardPB.pSavedInterruptRoutine = nil;
	shimKeyboardPB.deviceRef = device;
	shimKeyboardPB.interfaceRef = 0;
	shimKeyboardPB.pipeRef = 0;
	
	shimKeyboardPB.deviceRef = device;		
	shimKeyboardPB.interfaceRef = nil;		
	shimKeyboardPB.pipeRef = nil;		
	
	SetParamBlock(device, &shimKeyboardPB.pb);
	shimKeyboardPB.pb.pbLength = sizeof(usbKeyboardPBStruct);
	shimKeyboardPB.pb.usbRefcon = 0;				

	myKeyboardPB.deviceDescriptor = *pDeviceDescriptor;				
	myKeyboardPB.interfaceDescriptor = *pInterfaceDescriptor;				
	myKeyboardPB.transDepth = 0;							
	myKeyboardPB.retryCount = kKeyboardRetryCount;
	myKeyboardPB.pSHIMInterruptRoutine = nil;
	myKeyboardPB.pSavedInterruptRoutine = nil;
	myKeyboardPB.deviceRef = device;
	myKeyboardPB.interfaceRef = 0;
	myKeyboardPB.pipeRef = 0;

	myKeyboardPB.deviceRef = device;		
	myKeyboardPB.interfaceRef = nil;		
	myKeyboardPB.pipeRef = nil;	
	
	myKeyboardPB.sendRawReportFlag = false;
	myKeyboardPB.hidEmulationInit = false;
	
	SetParamBlock(device, &myKeyboardPB.pb);
	myKeyboardPB.pb.pbLength = sizeof(usbKeyboardPBStruct);
	myKeyboardPB.pb.usbRefcon = kGetFullConfiguration;		/* Start with setting the interface protocol */
	
	
	kbd_USBHIDControlDevice(kHIDEnableDemoMode,0);
	KeyboardModuleInitiateTransaction(&myKeyboardPB.pb);
}



