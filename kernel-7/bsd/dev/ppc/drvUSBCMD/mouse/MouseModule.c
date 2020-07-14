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
	File:		MouseModule.c

	Contains:	HID Module for USB Mouse

	Version:	xxx put version here xxx

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			USB on Mac

	Writers:

		(bwm)	Bruce Merritt
		(BT)	Barry Twycross
		(DF)	David Ferguson
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB53>	10/28/98	CJK		[2282187]  changed to use interface number from the interface
									descriptor supplied by the composite driver.  See the keyboard
									driver (there's a similar change in there as well).
	 <USB52>	10/21/98	DF		Change LMGetMBState to it's new name LMGetMouseButtonState, now
									we can include LowMem.h instead of LowMemPriv.h
	 <USB51>	10/21/98	bwm		[2229080]  NewWorld: whenever a mouse driver fails in an
									interrupt pipe read, force the mouse button up to prevent a
									possible hung system: we do this in case someone was holding the
									mouse button down while the mouse was unplugged, leaving MacOS
									is a somewhat constipated state.
	 <USB50>	 8/25/98	BT		Isoc name changes
	 <USB49>	 6/22/98	CJK		move cursordevice setup to InterfaceEntry, which is guarenteed
									to be at task time
	 <USB48>	 6/22/98	CJK		rename FindNextEndpointDescriptor to
									FindNextEndpointDescriptorImmediate
	 <USB47>	 6/18/98	CJK		update to use new style of error handling implemented in
									keyboard module.
	 <USB46>	 6/16/98	CJK		change FindHIDInterfaceByProtocol to FindHIDInterfaceByNumber
	 <USB45>	 6/15/98	CJK		change transaction depth error messages so that they output the
									refcon (current state machine stage)
	 <USB44>	 6/11/98	CJK		Remove driverentry routine (not needed or used).
	 <USB43>	  6/8/98	CJK		eliminate PBVERSION1 define
	 <USB42>	  6/5/98	CJK		remove ExpertStatus; too many clutter up expert log
	 <USB41>	 5/26/98	DF		Look for the incorrect vendorID (0x1452), when checking for
									version 0.04 Rudi
	 <USB40>	 5/26/98	CJK		add prototype for InitParamBlock
	 <USB39>	 5/22/98	CJK		add check for the 0.04 version of the Rudi mouse. Do a SetIdle
									of 50ms if found.
	 <USB38>	 5/20/98	BT		Set version 1 PB compatability
	 <USB37>	 5/20/98	CJK		change driver name from USBMouseModule to USBHIDMouseModule
	 <USB36>	 5/15/98	CJK		Things seem to have stabilized; no longer need to try various
									different int read report sizes until we find the one that
									works. Just use the maxpacketsize value.
	 <USB35>	 5/13/98	CJK		move DPI setting to interface entry code
	 <USB34>	 5/11/98	CJK		setidle to 0
	 <USB33>	  5/4/98	CJK		implement adaptive reqCount.  This way, when a mouse gives back
									an OverRun error, the mouse driver automatically increases the
									reqCount value (never exceeding the maxPacketSize).
	 <USB32>	  5/4/98	CJK		recover from repeated stalls when doing a setidle.  added detect
									for MS mouse, at which we ask for a 4 byte report (rather than a
									3 byte report)
	 <USB31>	 4/29/98	CJK		add break where needed
	 <USB30>	 4/26/98	CJK		Add back in SetIdle.  This seems to solve the stall problem and
									double click problem.
	 <USB29>	 4/26/98	CJK		change so that pipe stall clear is only called if happens as a
									result of a read or open pipe.  Also cleaned up transaction
									depth error messages so that the user can tell if the problem
									occurred in the completion or initiation routines.
	 <USB28>	 4/24/98	CJK		add call to clear pipestall by reference
	 <USB27>	 4/24/98	CJK		add call to turn on demo mode for shim-less environments.
	 <USB26>	 4/23/98	CJK		add status message to show who's driver this is...
	 <USB25>	 4/23/98	CJK		Reworked mouse module dramatically.  It now 1) gets the full
									configuration and looks for the mouse HID interface, 2) uses
									this mouse HID interface to find the interrupt endpoint, 3) uses
									the interrupt endpoint descriptor to open the the interrupt
									pipe.   Of note is that all the stuff to support the non-spec'd
									mice has been removed.  It will be added back in, either in a
									different mouse module, or within this mouse module, as further
									testing demands.
	 <USB24>	 4/14/98	CJK		Change to use driverservices strcpy
	 <USB23>	  4/9/98	CJK		change driver services to be a externally supplied header file
									(using < & >) rather than quotes.
	 <USB22>	  4/9/98	DF		Massaged to work with MasterInterfaces
		<21>	  4/8/98	CJK		correct mouse status & fatal error messages (should read
									"mouse", not Mouse)
		<20>	  4/8/98	CJK		add initialization of saved interrupt routine ptr.
		<19>	 3/31/98	CJK		change error messages
		<18>	 3/26/98	CJK		fix delay callback depth problem.  Clean up USBExpertFatal
									messages.
		<17>	 3/25/98	CJK		Add support for interrupt transactions
		<16>	 3/17/98	CJK		Replace "};" with just "}". Add parens to pragma unused
									variables.
		<15>	  3/5/98	CJK		I feel like a darn ping pong ball, changing the X-Delta &
									Y-Delta checks back and forth. But now that the mouse 'skip'
									problem has been resolved, let's see if I can't make both the
									BTC PS/2 mouse AND the Logitech mouse work properly. See Radar
									2216777.
		<14>	  3/3/98	CJK		change poll rate to 20ms
		<13>	  3/2/98	CJK		change back to check for non-zero
		<12>	  3/2/98	CJK		Add include of USBHIDModules.h. Remove include of HIDEmulation.h
		<11>	 2/19/98	CJK		change polling rate back to 10ms.
		<10>	 2/19/98	CJK		change back to notifying the MacOS when the deltas change from
									report to report, not just when they're non-zero.
		 <9>	 2/18/98	CJK		Implement Ferg's suggestion that we notify the HID Emulation
									routine if the X & Y deltas are non-zero.
		 <8>	 2/17/98	CJK		remove "" from expert fatal error calls.
		 <7>	 2/11/98	CJK		Change debugstr to expertfatal calls
		 <6>	 2/11/98	CJK		remove unneeded states
		 <5>	 2/10/98	CJK		change hid byte count from 8 to 3. No need to look at all eight
									bytes as mouse reports only consist of 3 bytes.
		 <4>	 2/10/98	CJK		Remove keyboard related code
		 <3>	  2/9/98	CJK		change first state to setprotocol (when entering driver via
									InterfaceEntry
		 <2>	  2/9/98	CJK		Add InterfaceEntry function
		 <1>	  2/9/98	CJK		First time check in.  Cloned from CompoundDriver.
*/

//#include <Types.h>
//#include <Devices.h>
//#include <processes.h>
#include "../driverservices.h"
#include "../USB.h"
//#include <LowMem.h>

#include "MouseModule.h"

usbMousePBStruct myMousePB;
static void InitParamBlock(USBDeviceRef theDeviceRef, USBPB * paramblock)
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

void MouseModuleInitiateTransaction(USBPB *pb)
{
register usbMousePBStruct *pMousePB;
OSStatus myErr;

	pMousePB = (usbMousePBStruct *)(pb);
	pMousePB->transDepth++;
	if (pMousePB->transDepth < 0)
	{
		USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: transDepth < 0 (initiation)", pMousePB->pb.usbRefcon );
	}
	
	if (pMousePB->transDepth > 1)
	{
		USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: transDepth > 1 (initiation)", pMousePB->pb.usbRefcon );
	}
	
	switch(pMousePB->pb.usbRefcon & ~kRetryTransaction)
	{
		case kGetFullConfiguration:
			InitParamBlock(pMousePB->deviceRef, &pMousePB->pb);
			
			pMousePB->pb.usbRefcon |= kCompletionPending;
			pMousePB->pb.usbCompletion = (USBCompletion)TransactionCompletionProc;
			myErr = USBGetFullConfigurationDescriptor(pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pMousePB->pb.usbReference, kUSBInternalErr, "USBHIDMouseModule: kGetFullConfiguration (ImmediateError)", myErr);
			}
			break;
			
		case kFindInterfaceAndSetProtocol:
			myErr = FindHIDInterfaceByNumber(pMousePB->pFullConfigDescriptor, pMousePB->interfaceDescriptor.interfaceNumber, &pMousePB->pInterfaceDescriptor );	// find the HID interface
			if ((pMousePB->pInterfaceDescriptor == NULL) || (myErr != noErr))
			{
				USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: Interface not found", myErr);
				pMousePB->pb.usbRefcon = kReturnFromDriver;
			}
			
			InitParamBlock(pMousePB->deviceRef, &pMousePB->pb);
			
			pMousePB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBInterface);			
			pMousePB->pb.usb.cntl.BRequest = kHIDRqSetProtocol;
			pMousePB->pb.usb.cntl.WValue = kHIDBootProtocolValue; 
			pMousePB->pb.usb.cntl.WIndex = pMousePB->interfaceDescriptor.interfaceNumber;
			pMousePB->pb.usbCompletion = (USBCompletion)TransactionCompletionProc;
			
			pMousePB->pb.usbRefcon |= kCompletionPending;
			myErr = USBDeviceRequest(&pMousePB->pb);
			if (immediateError(myErr))
			{
				USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: kSetProtocol (ImmediateError)", myErr);
			}
			break;
			
		case kSetIdleRequest:
			InitParamBlock(pMousePB->deviceRef, &pMousePB->pb);
			
			pMousePB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBInterface);			
			
			pMousePB->pb.usb.cntl.BRequest = kHIDRqSetIdle;
			if ((myMousePB.deviceDescriptor.vendor == USB_CONSTANT16(0x1452)) &&
				(myMousePB.deviceDescriptor.product == USB_CONSTANT16(0x0301)) &&
				(pMousePB->deviceDescriptor.devRel == USB_CONSTANT16(0x0004)))
			{
				pMousePB->pb.usb.cntl.WValue = ((50/4)<<8); 				// force a read completion if idle for more than 50ms
			}
			else
			{
				pMousePB->pb.usb.cntl.WValue = 0; 						// Turn off SetIdle time (set it to 0ms)
			}
			pMousePB->pb.usb.cntl.WIndex = pMousePB->interfaceDescriptor.interfaceNumber;
			pMousePB->pb.usbCompletion = (USBCompletion)TransactionCompletionProc;
			
			pMousePB->pb.usbRefcon |= kCompletionPending;

			myErr = USBDeviceRequest(&pMousePB->pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: kSetIdleRequest - immediate error", myErr);
			}
			break;

		case kFindAndOpenInterruptPipe:
			pMousePB->pb.usbClassType = kUSBInterrupt;
			pMousePB->pb.usbOther = 0;			
			pMousePB->pb.usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBInterface);
			pMousePB->pb.usbFlags = kUSBIn;
			pMousePB->pb.usbBuffer = pMousePB->pInterfaceDescriptor;
			pMousePB->pb.usbReqCount = (UInt8*)pMousePB->pInterfaceDescriptor - (UInt8*)pMousePB->pFullConfigDescriptor;
			myErr = USBFindNextEndpointDescriptorImmediate( &pMousePB->pb );
			if((immediateError(myErr)) || (pMousePB->pb.usbBuffer == nil))
			{
				USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: Endpoint not found", myErr);
				pMousePB->pb.usbRefcon = kReturnFromDriver;
			}
			else
			{
				pMousePB->pEndpointDescriptor = (USBEndPointDescriptorPtr) pMousePB->pb.usbBuffer;

				InitParamBlock(pMousePB->deviceRef, &pMousePB->pb);
				
				pMousePB->pb.usbFlags = kUSBIn;
				pMousePB->pb.usb.cntl.WValue = USBToHostWord(pMousePB->pEndpointDescriptor->maxPacketSize);
				pMousePB->pb.usbClassType = kUSBInterrupt;
				pMousePB->pb.usbOther = (pMousePB->pEndpointDescriptor->endpointAddress & 0x0f);
				pMousePB->pb.usbRefcon |= kCompletionPending;
				pMousePB->pb.usbCompletion = (USBCompletion)TransactionCompletionProc;
		
				myErr = USBOpenPipe( &pMousePB->pb );
				if(immediateError(myErr))
				{
					USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: USBOpenPipe Failed (ImmediateError)", myErr);
				}
			}
			break;
		
		case kReadInterruptPipe:
			InitParamBlock(pMousePB->pipeRef, &pMousePB->pb);

			pMousePB->pb.usbBuffer = (Ptr)pMousePB->hidReport;
			pMousePB->pb.usbReqCount = USBToHostWord(pMousePB->pEndpointDescriptor->maxPacketSize);
			pMousePB->pb.usb.cntl.WIndex = pMousePB->interfaceDescriptor.interfaceNumber;	
			pMousePB->pb.usbCompletion = (USBCompletion)TransactionCompletionProc;
			
			pMousePB->pb.usbRefcon |= kCompletionPending;
		
			myErr = USBIntRead(&pMousePB->pb);
			if(immediateError(myErr))
			{
				USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: Read Interrupt Pipe (ImmediateError)", myErr);
			}
			break;
			
		default:
			USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: Transaction initiated with bad refcon value", pMousePB->pb.usbRefcon);
			pMousePB->pb.usbRefcon = kUndefined + kReturnFromDriver;
			break;
	}
	
// At this point the control is returned to the system.  If a USB transaction
// has been initiated, then it will call the Complete procs
// (below) to handle the results of the transaction.
}


void TransactionCompletionProc(USBPB *pb)
{
register usbMousePBStruct *pMousePB;
unsigned char	* errstring;
USBPipeState 	pipeState;
kprintf("TransactionCompletionProc is called for Mouse read\n");
	pMousePB = (usbMousePBStruct *)(pb);
	pMousePB->transDepth--;
	if (pMousePB->transDepth < 0)
	{
		USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: transDepth < 0 (completion)", pMousePB->pb.usbRefcon );
	}
	
	if (pMousePB->transDepth > 1)
	{
		USBExpertFatalError(pMousePB->deviceRef, kUSBInternalErr, "USBHIDMouseModule: transDepth > 1 (completion)", pMousePB->pb.usbRefcon );
	}
	
	if(pMousePB->pb.usbStatus != noErr)														// was there an error?
	{
		switch(pMousePB->pb.usbRefcon & 0x0fff)												// yes, so show where the error occurred
		{
			case kGetFullConfiguration:  		errstring = "USBHIDMouseModule: Error during GetFullConfiguration"; break;
			case kFindInterfaceAndSetProtocol:	errstring = "USBHIDMouseModule: Error during FindInterfaceAndSetProtocol"; break;
			case kSetIdleRequest:				errstring = "USBHIDMouseModule: Error during kSetIdleRequest"; break;
			case kFindAndOpenInterruptPipe:  	errstring = "USBHIDMouseModule: Error during FindAndOpenInterruptPipe"; break;
			case kReadInterruptPipe:
				{
				errstring = "USBHIDMouseModule: Error during ReadInterruptPipe";
				//naga LMSetMouseButtonState(0x80);	// release any possibly held-down mouse button
				break;
				}
			default: 	 						errstring = "USBHIDMouseModule: Error occurred, but state is unknown"; break;
		};
		USBExpertFatalError(pMousePB->deviceRef, pMousePB->pb.usbStatus, errstring, (pMousePB->pb.usbRefcon & 0x0fff));
		
		pMousePB->pb.usbRefcon &= ~(kCompletionPending + kReturnFromDriver);				// set up to retry the transaction
		pMousePB->pb.usbRefcon |= kRetryTransaction;
		pMousePB->retryCount--;
		
		if ((pMousePB->retryCount == 1) && ((pMousePB->pb.usbRefcon&pMousePB->pb.usbRefcon & 0x0fff) == kSetIdleRequest))
		{
			USBExpertStatus(pMousePB->deviceRef, "USBHIDMouseModule: Device doesn't accept SetIdle", pMousePB->deviceRef);
			pMousePB->pb.usbRefcon = kFindAndOpenInterruptPipe;
			pMousePB->pb.usbStatus = noErr;
		}
		else
		{
			if ((!pMousePB->retryCount)	|| (pMousePB->pb.usbStatus == kUSBAbortedError))	// have we exhausted the retries?
			{																				// or received an abort?
				USBExpertStatus(pMousePB->deviceRef, "USBHIDMouseModule: Pipe abort or unable to recover from error", pMousePB->deviceRef);
				pMousePB->pb.usbRefcon = kReturnFromDriver;									// if so, just exit.
			}
			else																			// if it didn't abort and there's retries left, then...
			{
				if (pMousePB->pipeRef)														// check if the pipe is open.
				{
					USBGetPipeStatusByReference(pMousePB->pipeRef, &pipeState);				// yes, so what it's state?
					if (pipeState != kUSBActive)											// if it's not active, try to clear it.  It might be stalled...
					{
						USBExpertStatus(pMousePB->deviceRef, "USBHIDMouseModule: Pipe is open and stalled, clearing stall...", pMousePB->deviceRef);
						USBClearPipeStallByReference(pMousePB->pipeRef);
					}
				}
			}
		}
	}
	else
	{


kprintf(".");
//Adam added this 11/30

		pMousePB->pb.usbRefcon &= ~kRetryTransaction;
		pMousePB->retryCount = kMouseRetryCount;
	}

	if (pMousePB->pb.usbRefcon & kCompletionPending)			 
	{												
		pMousePB->pb.usbRefcon &= ~(kCompletionPending + kReturnFromDriver);
		switch(pMousePB->pb.usbRefcon)
		{
			case kGetFullConfiguration:
				pMousePB->pFullConfigDescriptor = pMousePB->pb.usbBuffer;
				if (pMousePB->pFullConfigDescriptor == nil)
				{
					USBExpertFatalError(pMousePB->pb.usbReference, kUSBInternalErr, "USBHIDMouseModule: USBGetFullConfiguration - pointer is nil", pMousePB->pb.usbRefcon);
					pMousePB->pb.usbRefcon = kReturnFromDriver;
				}
				else
				{
					pMousePB->pb.usbRefcon = kFindInterfaceAndSetProtocol;
				}
				break;
				
			case kFindInterfaceAndSetProtocol:
				pMousePB->pb.usbRefcon = kSetIdleRequest;
				break;
				
			case kSetIdleRequest:
				pMousePB->pb.usbRefcon = kFindAndOpenInterruptPipe;
				break;
				
			case kFindAndOpenInterruptPipe:
				pMousePB->pipeRef = pMousePB->pb.usbReference;
				pMousePB->pb.usbRefcon = kReadInterruptPipe;
				break;
				
			case kReadInterruptPipe:
				NotifyRegisteredHIDUser(pMousePB->hidDeviceType, pMousePB->hidReport);
				pMousePB->pb.usbRefcon = kReadInterruptPipe;
				break;

		}
	}
	if (!(pMousePB->pb.usbRefcon & kReturnFromDriver))
		MouseModuleInitiateTransaction(pb);
}


void InterfaceEntry(UInt32 interfacenum, USBInterfaceDescriptorPtr pInterfaceDescriptor, USBDeviceDescriptorPtr pDeviceDescriptor, USBDeviceRef device)
{
#pragma unused (interfacenum)

static Boolean beenThereDoneThat = false;

	if(beenThereDoneThat)
	{
		USBExpertFatalError(device, kUSBInternalErr, "USBHIDMouseModule is not reentrant", 0);
		return;
	}
//Hot Plug n Play
//naga	beenThereDoneThat = true;
	
//	DebugStr("In Mouse Interface Entry routine");
	kprintf("In Mouse Interface Entry routine");

	myMousePB.deviceDescriptor = *pDeviceDescriptor;				
	myMousePB.interfaceDescriptor = *pInterfaceDescriptor;				
	myMousePB.transDepth = 0;							
	myMousePB.retryCount = kMouseRetryCount;
	  
	myMousePB.pSHIMInterruptRoutine = nil;
	myMousePB.pSavedInterruptRoutine = nil;

	myMousePB.deviceRef = device;		
	myMousePB.interfaceRef = nil;		
	myMousePB.pipeRef = nil;		
	
	InitParamBlock(device, &myMousePB.pb);
	
	myMousePB.pb.usbReference = device;
	myMousePB.pb.pbLength = sizeof(usbMousePBStruct);
	myMousePB.pb.usbRefcon = kGetFullConfiguration;		
	
	if ((myMousePB.deviceDescriptor.vendor == USB_CONSTANT16(0x046e)) &&
		(myMousePB.deviceDescriptor.product == USB_CONSTANT16(0x6782)))
	{
		myMousePB.unitsPerInch = (Fixed)(100<<16);
	}
	else
	{
		myMousePB.unitsPerInch = (Fixed)(400<<16);
	}
	
	myMousePB.pCursorDeviceInfo = 0;				
	USBHIDControlDevice(kHIDEnableDemoMode,0);

	MouseModuleInitiateTransaction(&myMousePB.pb);
}


/*  naga */
OSErr CursorDeviceNewDevice(CursorDevicePtr *              ourDevice)
{
//                                           TWOWORDINLINE(0x700C, 0xAADB);
     return 0;
}

OSErr CursorDeviceDisposeDevice(CursorDevicePtr     ourDevice)
{
//                                 TWOWORDINLINE(0x700D, 0xAADB);
     return 0;
}
OSErr CursorDeviceMove(CursorDevicePtr ourDevice, long	deltaX, long	deltaY)		
{
     //TWOWORDINLINE(0x7000, 0xAADB);
     return 0;
}
OSErr CursorDeviceMoveTo(CursorDevicePtr ourDevice, long absX, long absY)
{
   //TWOWORDINLINE(0x7001, 0xAADB);
     return 0;
}
OSErr CursorDeviceFlush	(CursorDevicePtr	ourDevice)
{
      //TWOWORDINLINE(0x7002, 0xAADB);
     return 0;
}

OSErr CursorDeviceButtons(CursorDevicePtr ourDevice, short buttons)							
{
     //TWOWORDINLINE(0x7003, 0xAADB);
     return 0;
}
OSErr CursorDeviceButtonDown(CursorDevicePtr ourDevice)				
{
     return 0;
}
OSErr CursorDeviceButtonUp(CursorDevicePtr 		ourDevice)
{
     //TWOWORDINLINE(0x7005, 0xAADB);
     return 0;
}

OSErr CursorDeviceButtonOp(CursorDevicePtr ourDevice, short buttonNumber, ButtonOpcode opcode, long data)
{
   //TWOWORDINLINE(0x7006, 0xAADB);
    return 0;
}

OSErr CursorDeviceSetButtons(CursorDevicePtr ourDevice, short numberOfButtons)		
{
    //TWOWORDINLINE(0x7007, 0xAADB);
    return 0;
}

OSErr CursorDeviceSetAcceleration(CursorDevicePtr ourDevice,Fixed acceleration)		
{
  //TWOWORDINLINE(0x7008, 0xAADB);
    return 0;
}

OSErr CursorDeviceDoubleTime(CursorDevicePtr ourDevice, long durationTicks)		
{
    //TWOWORDINLINE(0x7009, 0xAADB);
    return 0;
}

OSErr CursorDeviceUnitsPerInch(CursorDevicePtr	ourDevice, Fixed resolution)	
{
     // TWOWORDINLINE(0x700A, 0xAADB);
    return 0;
}

OSErr CursorDeviceNextDevice(CursorDevicePtr *		ourDevice)		
{
      //TWOWORDINLINE(0x700B, 0xAADB);
      return 0;
}

