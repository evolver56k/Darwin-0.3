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
	File:		HIDEmulation.c

	Contains:	HID Emulation glue code between the ADB SHIM and the USB Mouse HID Module

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BWS)	Brent Schorsch
		(DKF)	David Ferguson
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB30>	  7/6/98	CJK		make certain cursor device is removed when interrupt is
									installed
	 <USB29>	 6/22/98	CJK		move cursordevice setup to InterfaceEntry, which is guarenteed
									to be called at task time
	 <USB28>	 6/18/98	BWS		fixed interrupt refcon support
	 <USB27>	 6/18/98	CJK		add interrupt refcon support
	 <USB26>	 5/28/98	CJK		remove pragma unused from NotifyRegisteredHIDUser
	 <USB25>	 5/20/98	CJK		change driver name from USBMouseModule to USBHIDMouseModule
	 <USB24>	 5/13/98	CJK		move DPI setting to interface entry code
	 <USB23>	 4/24/98	CJK		add "get device units per inch" functionality
	 <USB22>	 4/23/98	CJK		add status message when interrupt is installed.
	 <USB21>	 4/23/98	CJK		Reworked mouse functionality to always pass mouse data on to the
									register HID user (even if the data is the same as the last
									read).
	 <USB20>	 4/20/98	CJK		add in mouse demo mode control selector
	 <USB19>	  4/9/98	CJK		remove include of USBClassDrivers.h, USBHIDModules.h, and
									USBDeviceDefines.h
	 <USB18>	  4/9/98	CJK		remove demo mode support
		<17>	  4/8/98	CJK		rework USBMouseIn to work the same way as the standard interrupt
									handler
		<16>	 3/27/98	CJK		only set units per inch to 100 when using a BTC keyboard.
		<15>	 3/25/98	CJK		Move units per inch initialization to here, as we're now using
									interrupt transactions. Remove clip to -90. Change name of
									paramblock pointer to better indicate that it's a pointer to a
									cursor device manager struct.
		<14>	 3/18/98	CJK		Add back in the clip to -90. Seems that this is the cause of the
									mouse skip.
		<13>	 3/18/98	CJK		Remove checks for X & Y deltas of less than -90
		<12>	 3/17/98	CJK		Replace some "};" with just "}" (MetroWerks has a problem with
									it). Add parens around the pragma unused variables. Move
									function prototypes to MouseModule.h
		<11>	  3/9/98	CJK		Add check for BTC mouse port. If that's the mouse we're using,
									then set the DPI to 100.
		<10>	  3/3/98	CJK		change cursor device manager device creation to used 'Fixed'
									data type properly.
		 <9>	  3/2/98	CJK		add check for illegal mouse movement values.  Don't allow
									movement if it's less than negative 90.
		 <8>	  3/2/98	CJK		Add include of USBHIDModules.h. Remove include of HIDEmulation.h
		 <7>	 2/18/98	CJK		change call to mouse buttons to only pass a char.
	  <USB6>	 2/17/98	DKF		Add mouse movement if there is no shim installed
		 <5>	 2/10/98	CJK		change hid notification code to munge X & Y delta values into
									signed 16 bit values.
		 <4>	 2/10/98	CJK		remove debugstr (that is hit when both mouse buttons are
									pressed).
		 <3>	 2/10/98	CJK		Remove keyboard related code
		 <2>	  2/9/98	CJK		remove get descriptor from HID Emulation code
		 <1>	  2/9/98	CJK		First time check in.  Cloned from CompoundDriver.
*/

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
//#include <CursorDevices.h>
#include "../USB.h"
#include "MouseModule.h"

extern	usbMousePBStruct myMousePB;
extern void mouse_adbhandler(int number, unsigned char *buffer, int count, void * ssp);


void USBMouseIn(UInt32 refcon, void * theData)
{
#pragma unused (refcon)

USBHIDDataPtr	pMouseData;
static UInt16 	oldbuttons = 0;
UInt16 	changedbuttons = 0;

//	DebugStr("In USBMouseIn");
	pMouseData = (USBHIDDataPtr)theData;

	// Tell the Cursor Device Manager that we moved
	if ((pMouseData->mouse.XDelta != 0) || (pMouseData->mouse.YDelta !=0))
	{
		CursorDeviceMove(myMousePB.pCursorDeviceInfo, pMouseData->mouse.XDelta, pMouseData->mouse.YDelta);
	}
	
	// Update with the state of the buttons.
	pMouseData->mouse.buttons &= 0x07;
	changedbuttons = oldbuttons ^ pMouseData->mouse.buttons;
	if (changedbuttons)
	{
		CursorDeviceButtons(myMousePB.pCursorDeviceInfo, (short)pMouseData->mouse.buttons);
	}
	oldbuttons = pMouseData->mouse.buttons;
}

OSStatus USBHIDInstallInterrupt(HIDInterruptProcPtr HIDInterruptFunction, UInt32 refcon)
{
	USBExpertStatus(myMousePB.deviceRef, "USBHIDMouseModule: Demo Mode Disabled (interrupt service routine installed)", myMousePB.deviceRef);
	myMousePB.interruptRefcon = refcon;
	myMousePB.pSHIMInterruptRoutine = HIDInterruptFunction;
	if (myMousePB.pCursorDeviceInfo != 0)
	{
		CursorDeviceDisposeDevice(myMousePB.pCursorDeviceInfo);
		myMousePB.pCursorDeviceInfo = 0;
	}
	return noErr;
}

OSStatus USBHIDPollDevice(void)
{
	return kUSBInternalErr;
}

OSStatus USBHIDControlDevice(UInt32 theControlSelector, void * theControlData)
{
#pragma unused (theControlData)

	switch (theControlSelector)
	{
		case kHIDRemoveInterruptHandler:
			myMousePB.interruptRefcon = nil;
			myMousePB.pSavedInterruptRoutine = nil;
			myMousePB.pSHIMInterruptRoutine = nil;
			break;
			
		case kHIDEnableDemoMode:
			USBExpertStatus(myMousePB.deviceRef, "USBHIDMouseModule: Demo Mode Enabled", myMousePB.deviceRef);
			
			if (myMousePB.pCursorDeviceInfo == 0)
			{
				myMousePB.pCursorDeviceInfo = &myMousePB.cursorDeviceInfo;
				CursorDeviceNewDevice(&myMousePB.pCursorDeviceInfo);
				
				CursorDeviceSetAcceleration(myMousePB.pCursorDeviceInfo, (Fixed)(1<<16));
				
				CursorDeviceSetButtons(myMousePB.pCursorDeviceInfo, 3);			// should actually be set by reading
																				// the HID descriptor, but lacking
																				// a parser, we'll just force it
																				// this way.
				CursorDeviceButtonOp(myMousePB.pCursorDeviceInfo, 0, kButtonSingleClick, 0L);
				CursorDeviceButtonOp(myMousePB.pCursorDeviceInfo, 1, kButtonSingleClick, 0L);
				CursorDeviceButtonOp(myMousePB.pCursorDeviceInfo, 2, kButtonSingleClick, 0L);
				CursorDeviceUnitsPerInch(myMousePB.pCursorDeviceInfo, (Fixed)(myMousePB.unitsPerInch));
			}
		
			myMousePB.pSavedInterruptRoutine = myMousePB.pSHIMInterruptRoutine;
			myMousePB.pSHIMInterruptRoutine = USBMouseIn;
			break;

		case kHIDDisableDemoMode:
			USBExpertStatus(myMousePB.deviceRef, "USBHIDMouseModule: Demo Mode Disabled", myMousePB.deviceRef);
			if (myMousePB.pCursorDeviceInfo != 0)
			{
				CursorDeviceDisposeDevice(myMousePB.pCursorDeviceInfo);
				myMousePB.pCursorDeviceInfo = 0;
			}
			myMousePB.pSHIMInterruptRoutine = myMousePB.pSavedInterruptRoutine;
			break;

		default:
			return paramErr;
	}
	return noErr;
}



OSStatus USBHIDGetDeviceInfo(UInt32 theInfoSelector, void * theInfo)
{
HIDInterruptProcPtr * pHIDIntProcPtr;
UInt32 * pUnits;
UInt32 * pInterruptRefcon;

	switch (theInfoSelector)
	{
		case kHIDGetDeviceUnitsPerInch:
			pUnits = (UInt32*)theInfo;
			*pUnits = (UInt32)(myMousePB.unitsPerInch);
			break;
			
			
		case kHIDGetInterruptHandler:
			pHIDIntProcPtr = (HIDInterruptProcPtr *)theInfo;     
			*pHIDIntProcPtr = myMousePB.pSHIMInterruptRoutine;
			break;

		case kHIDGetInterruptRefcon:
			pInterruptRefcon = (UInt32 *)theInfo;
			*pInterruptRefcon = myMousePB.interruptRefcon;
			break;
		
		default:
			return paramErr;
	}
	return noErr;
}

OSStatus USBHIDEnterPolledMode(void)
{
	return unimpErr;
}

OSStatus USBHIDExitPolledMode(void)
{
	return unimpErr;
}

void NotifyRegisteredHIDUser(UInt32 devicetype, UInt8 hidReport[])
{
#pragma unused (devicetype)

USBHIDData		theMouseData;
SInt8			myXDelta, myYDelta;
UInt8			adb_data[4];

	theMouseData.mouse.buttons = (UInt16)hidReport[0];
	
	myXDelta = (SInt8)hidReport[1];
	myYDelta = (SInt8)hidReport[2];  
//kprintf("***Mouse data: buttons=0x%x, Xdelta=0x%x, YDelta=0x%x***\n",hidReport[0],hidReport[1],hidReport[2]);
	theMouseData.mouse.XDelta = (SInt16)myXDelta;
	theMouseData.mouse.YDelta = (SInt16)myYDelta;
	
	//A.W. call the mouse driver in ADB-land
	adb_data[0] = (SInt8)hidReport[2];
	adb_data[1] = (SInt8)hidReport[1];
	if (hidReport[0] == 1)   /* Clicked */
	{
		adb_data[0] = ((adb_data[0] >>1) & 0x7f);    
	}
	else adb_data[0] = 0x80 | (adb_data[0] >>1);  //button 1 clickif (hidReport[0] == 2)

	if (hidReport[0] == 2)   /* Clicked */
	{
		adb_data[1] = ((adb_data[1] >>1) & 0x7f);    
	}
	else adb_data[1] = 0x80 | (adb_data[1] >>1);  //button 1 clickif (hidReport[0] == 2)

	mouse_adbhandler(3, adb_data, 2, (void *) 0);

	if (myMousePB.pSHIMInterruptRoutine)
	{
		(*myMousePB.pSHIMInterruptRoutine)(myMousePB.interruptRefcon, (void *)&theMouseData);
	}
}
USBHIDModuleDispatchTable TheHIDModuleDispatchTable =
{
	(UInt32)0,
	(USBHIDInstallInterruptProcPtr)USBHIDInstallInterrupt,
	(USBHIDPollDeviceProcPtr)USBHIDPollDevice,
	(USBHIDControlDeviceProcPtr)USBHIDControlDevice,
	(USBHIDGetDeviceInfoProcPtr)USBHIDGetDeviceInfo,
	(USBHIDEnterPolledModeProcPtr)USBHIDEnterPolledMode,
	(USBHIDExitPolledModeProcPtr)USBHIDExitPolledMode
};
CursorDevicePtr gUSBMouse;
CursorDevice	ourDevice;

