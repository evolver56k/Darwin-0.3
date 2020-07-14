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
	File:		KBDHIDEmulation.c

	Contains:	Keyboard Emulation code

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(BWS)	Brent Schorsch
		(BG)	Bill Galcher
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB32>	 6/20/98	CJK		Expand on Brent's comments... It wasn't a "fix" of the refcon,
									it was the addition of a new getinfo selector so that an
									InputSprocket can ask the Keyboard HID Module to pass back it's
									refcon to the caller. There was nothing "broken" in the original
									code.
	 <USB31>	 6/18/98	BWS		fix interrupt refcon support
	 <USB30>	 6/18/98	CJK		update to use modified HID dispatch table (passing in refcon)
	 <USB29>	  6/5/98	CJK		remove pragma unused from GetKeysPressed
	 <USB28>	 5/28/98	CJK		add HID GetInfo Selector to get notification proc address
	 <USB27>	 5/20/98	CJK		change driver name from USBKeyboardModule to
									USBHIDKeyboardModule
	 <USB26>	 5/20/98	CJK		add check for 'DDKBuild' define. If it's not defined then it's
									okay to include USBPriv.h and use the USL's DoneQueue API.
	 <USB25>	 5/19/98	CJK		remove include of USBPriv.h. This file should not be included,
									as the DDK will not supply it. If there are prototypes needed by
									this file, they should be moved to USB.h (usb.i).
	 <USB24>	 5/19/98	BG		Add includes of <USBPriv.h> and "UIMPriv.h" to bring in needed
									prototypes.
	 <USB23>	 5/18/98	CJK		remove the breaks...
	 <USB22>	 5/18/98	CJK		Add support for 'GetCurrentKeyboardState' getinfo selector
	 <USB21>	 4/30/98	CJK		fix break statements.
	 <USB20>	 4/30/98	CJK		handle keyreleasedflag slightly differently
	 <USB19>	 4/30/98	CJK		add a "send raw report" mode.  rework so that the old hid report
									and initialized flags are kept in the keyboard data structure
									(rather than in static vars.)
	 <USB18>	 4/27/98	CJK		correct zero'ing of non-relevant hid report bytes.
	 <USB17>	 4/27/98	CJK		Add handling of key rollover errors.  No longer sends already
									down keys when the rollover error is removed.
	 <USB16>	 4/26/98	CJK		update to match reworked keyboard interface driver.
									Restructured user notification routine.
	 <USB15>	 4/14/98	CJK		change call to USBIdleTask to UIMProcessDoneQueue
	 <USB14>	  4/9/98	CJK		remove include of USBDeviceDefines.h
		<13>	  4/8/98	CJK		add enable/disable of emulation code
		<12>	 3/26/98	CJK		remove debugstr
		<11>	 3/17/98	CJK		change }; to just }. MetroWerks has a problem with };.  Also
									added parens around pragma unused's variable name.
		<10>	 3/12/98	CJK		Don't worry about initializing the shim keyboard paramblock.
									That's done in the keyboard initialize routine.
		 <9>	  3/2/98	CJK		remove include of KBDHIDEmulation.h
		 <8>	  3/2/98	CJK		change to use USBHIDModule hid data structure (rather than the
									keyboard data structure).
		 <7>	 2/27/98	CJK		add check for keycount growing too large.  Add include of
									USBHIDModules.h.  Change LED control selector to be "by bit" (as
									opposed to by ID, which is probably how things will ultimately
									work).
		 <6>	 2/26/98	CJK		Add USBHIDControlDevice functionality. All it does (right now)
									is set the LEDs.
		 <5>	 2/17/98	CJK		Integrate Ferg's changes.
		<4*>	 2/16/98	DKF		Add keyposting if there is no shim registered
		 <4>	 2/16/98	CJK		remove write of $00 keycodes into the keycode array. Streamline
									old vs. new keycode detection (hopefully faster than before).
		 <3>	 2/16/98	CJK		Improve check of modifier key changes (shouldn't need to scan
									through each bit to determine if any modifiers changed.  Just check
									the changed bits.  If any changed, then look through each bit individually.
		 <2>	 2/10/98	CJK		Correct change history (to reflect the keyboard module changes)
		 <1>	 2/10/98	CJK		First time checkin.  Cloned from Mouse HID Module.
*/

//#include <Types.h>
//#include <Devices.h>
#include "../driverservices.h"
#include "../USB.h"
#ifndef DDKBuild
#include "../USBpriv.h"
#endif

#include "KeyboardModule.h"
//prototype from PPCKeyboard.m
void keyboard_adbhandler(int number, unsigned char *buffer, int count, void * ssp);


extern	usbKeyboardPBStruct myKeyboardPB;
extern	usbKeyboardPBStruct shimKeyboardPB;
extern  UInt8 usb_2_adb_keymap[];

void GetKeysPressed(USBHIDData * pKeysPressed);

OSStatus kbd_USBHIDInstallInterrupt(HIDInterruptProcPtr HIDInterruptFunction, UInt32 refcon)
{
	myKeyboardPB.interruptRefcon = refcon;
	myKeyboardPB.pSHIMInterruptRoutine = HIDInterruptFunction;
	return 0;
}

OSStatus kbd_USBHIDControlDevice(UInt32 theControlSelector, void * theControlData)
{
	switch (theControlSelector)
	{
		case kHIDSetLEDStateByBits:
			shimKeyboardPB.hidReport[0] = *(UInt8*)theControlData;

			shimKeyboardPB.retryCount = kKeyboardRetryCount;
			shimKeyboardPB.delayLevel = 0;							
			shimKeyboardPB.transDepth = 0;	
			
			shimKeyboardPB.pb.usbRefcon = kSetKeyboardLEDs;					/* Start with setting the interface protocol */
			KeyboardModuleInitiateTransaction(&shimKeyboardPB.pb);
			break;
			
		case kHIDRemoveInterruptHandler:
			myKeyboardPB.interruptRefcon = nil;
			myKeyboardPB.pSavedInterruptRoutine = nil;
			myKeyboardPB.pSHIMInterruptRoutine = nil;
			break;
			
		case kHIDEnableDemoMode:
			USBExpertStatus(myKeyboardPB.deviceRef, "USBHIDKeyboardModule: Demo Mode Enabled", myKeyboardPB.deviceRef);
			myKeyboardPB.pSavedInterruptRoutine = myKeyboardPB.pSHIMInterruptRoutine;
			myKeyboardPB.pSHIMInterruptRoutine = USBDemoKeyIn;
			break;

		case kHIDDisableDemoMode:
			USBExpertStatus(myKeyboardPB.deviceRef, "USBHIDKeyboardModule: Demo Mode Disabled", myKeyboardPB.deviceRef);
			myKeyboardPB.pSHIMInterruptRoutine = myKeyboardPB.pSavedInterruptRoutine;
			break;

		default:
			return paramErr;
	}
	return 0;
}

void GetKeysPressed(USBHIDDataPtr pKeysPressed)
{
UInt8	i,keycount;
	
	keycount = 0;
	for (i = 0; i < kKeyboardModifierBits; i++)
	{
		if (myKeyboardPB.oldHIDReport[0] & (1 << i))
		{
			pKeysPressed->kbd.usbkeycode[keycount++] = (0xe0 + i);
		}
	}
			
	if ((myKeyboardPB.oldHIDReport[kKeyboardOffsetToKeys] == 0) || (myKeyboardPB.oldHIDReport[kKeyboardOffsetToKeys] > 0x03))
	{
		for (i = kKeyboardOffsetToKeys; i < (kKeyboardOffsetToKeys + kKeyboardReportKeys); i++)
		{
			if (myKeyboardPB.oldHIDReport[i] > 0x03)
			{
				pKeysPressed->kbd.usbkeycode[keycount++] = myKeyboardPB.oldHIDReport[i];
			}
		}														
	}
	pKeysPressed->kbd.keycount = keycount;
}


OSStatus kbd_USBHIDGetDeviceInfo(UInt32 theInfoSelector, void * theInfo)
{
HIDInterruptProcPtr * pHIDIntProcPtr;
UInt32 * pInterruptRefcon;

	switch (theInfoSelector)
	{
		case kHIDGetCurrentKeys:
			GetKeysPressed((USBHIDDataPtr)theInfo);
			break;

		case kHIDGetInterruptHandler:
			pHIDIntProcPtr = (HIDInterruptProcPtr *)theInfo;  
			*pHIDIntProcPtr = myKeyboardPB.pSHIMInterruptRoutine;
			break;

		case kHIDGetInterruptRefcon:
			pInterruptRefcon = (UInt32 *)theInfo;
			*pInterruptRefcon = myKeyboardPB.interruptRefcon;
			break;
		
		default:
			return paramErr;
	}
	return 0;
}

OSStatus kbd_USBHIDPollDevice(void)
{
#ifndef DDKBuild
	USLPolledProcessDoneQueue();
#endif
	return kUSBNoErr;
}

OSStatus kbd_USBHIDEnterPolledMode(void)
{    
	return unimpErr;
}

OSStatus kbd_USBHIDExitPolledMode(void)
{
	return unimpErr;
}

void kbd_NotifyRegisteredHIDUser(UInt32 devicetype, UInt8 hidReport[])
{
#pragma unused (devicetype)

UInt8	i, j, newkey, oldkey, deltas;

UInt8		changedmodifiers, keycount;
USBHIDData	theKeyboardData;
Boolean		keypressedflag, keyreleasedflag;
char		adb_data[8];
	
	deltas = 0;
	if (myKeyboardPB.hidEmulationInit == false)
	{
		myKeyboardPB.hidEmulationInit = true;
		for (i = 0; i < kKeyboardReportSize; i++)
			myKeyboardPB.oldHIDReport[i] = 0;
	};
	
	myKeyboardPB.oldHIDReport[1] = 0x0;
	hidReport[1] = 0x0;
	
	for (i = 0; i < kKeyboardReportSize; i++)
	{
		if (hidReport[i] != myKeyboardPB.oldHIDReport[i])
		{
			deltas++;
		}
	}
	
	if ((myKeyboardPB.sendRawReportFlag) && deltas)
	{
		(*myKeyboardPB.pSHIMInterruptRoutine)(0xff, (void *)&hidReport[0]);
	}
	else
	{
		if (deltas)
		{
			keycount = 0;
			changedmodifiers = hidReport[0] ^  myKeyboardPB.oldHIDReport[0];
			
			if (changedmodifiers)
			{
				for (i = 0; i < kKeyboardModifierBits; i++)
				{
					if (changedmodifiers & (1 << i))
					{
						if (hidReport[0] & (1 << i))
						{
							theKeyboardData.kbd.usbkeycode[keycount++] = (0xe0 + i);
						}
						else
						{
							theKeyboardData.kbd.usbkeycode[keycount++] = (0x80e0 + i);
						}
					}
				}
				myKeyboardPB.oldHIDReport[0] = hidReport[0];
			}
			
			if ((hidReport[kKeyboardOffsetToKeys] == 0) || (hidReport[kKeyboardOffsetToKeys] > 0x03))
			{
				// While this double loop may look strange, I refer you to appendix C, of the HID Devices specification.
				// pp. 73 & 74 clearly state that report order is abitrary and does not reflect the order of events.
				// to quote: "The order of keycodes in array fields has no significance.  Order determination is done
				// by the host software comparing the contents of the previous report to the current report.  If two or 
				// more keys are pressed in one report, their order is indeterminate.  Keyboards may buffer events that
				// would have otherwise resulted in multiple events in a single report".
				
				// Because this specification (or lack thereof) states that the order is indeterminant, we have to 
				// check all the old keys against all the new keys...
				
				for (i = kKeyboardOffsetToKeys; i < (kKeyboardOffsetToKeys + kKeyboardReportKeys); i++)
				{
					keypressedflag = true;
					newkey = hidReport[i];
					
					keyreleasedflag = true;
					oldkey = myKeyboardPB.oldHIDReport[i];							
					for (j = kKeyboardOffsetToKeys; j < (kKeyboardOffsetToKeys + kKeyboardReportKeys); j++)								// then look through all the keys that were previously and are currently reported as pressed
					{									
						if (newkey == myKeyboardPB.oldHIDReport[j])		// was this new key already pressed?
						{								
							keypressedflag = false;						// if it was, then don't report it as being pressed.
						}
						
						if (oldkey == hidReport[j])						// Is the old key still pressed?
						{								
							keyreleasedflag = false;					// If yes, then don't report it as being released
						}
					}
					if ((newkey > 0x03) && keypressedflag)
					{
						theKeyboardData.kbd.usbkeycode[keycount++] = newkey;
					}
					
					if ((oldkey > 0x03) && keyreleasedflag)
					{
						theKeyboardData.kbd.usbkeycode[keycount++] = (oldkey + 0x8000);
					}
					
					if (keycount > 20)									// a worse case scenario is:
					{													// 4 modifier keys up
						break;											// the other 4 modifier keys down
					}													// 6 previously reported keys up
				}														// 6 newly reported keys down
																		// 6 + 6 + 4 + 4 = 20
				for (i = 0; i<kKeyboardReportSize; i++)
				{
					myKeyboardPB.oldHIDReport[i] = hidReport[i];
				};
			}
			
			//Now we map to ADB from USB 
			//Adam Wang 12/2/98
			adb_data[1] = 0xff;
			for(i=0;i<keycount;++i)
			{
				static char caps_lock_state = 1;
                                unsigned char newLEDState;
				//Special handling of USB caps-lock-that-doesn't-lock 12/8/98
				if ( theKeyboardData.kbd.usbkeycode[i] == 0x0039)  //00 = going down, 39 = caps lock                  
				{
					newLEDState = 2;
					//This call to set LED will hang on Yosemite 
					kbd_USBHIDControlDevice(1, &newLEDState);
				}
				if ( theKeyboardData.kbd.usbkeycode[i] == 0x8039)  //80 = going up, 39 = caps lock                  
				{ 
					caps_lock_state ^= 1; //toggle bit
					if (caps_lock_state == 0)     
						continue;   //naga changed break to continue
					//The logic is, ignore first caps-up on iMac keyboards        
					newLEDState = 0;
					kbd_USBHIDControlDevice(1, &newLEDState);
				}                

				adb_data[0] = usb_2_adb_keymap[(theKeyboardData.kbd.usbkeycode[i] & 0x00ff)];

				if ((theKeyboardData.kbd.usbkeycode[i] & 0x8000) == 0x8000) // key is on its way up
					adb_data[0] |= 0x80;		//Set ADB most significant bit
				
				//Special handling needed for ADB POWER key
				if (adb_data[0] == 0x7f)
				{
					adb_data[1] = 0x7f;  //ADB power scan code is 7f 7f
				}

				keyboard_adbhandler(2, adb_data, 2, (void *) 0);
			}


/*
			if ((myKeyboardPB.pSHIMInterruptRoutine) && keycount)
			{
				theKeyboardData.kbd.keycount = keycount;
				(*myKeyboardPB.pSHIMInterruptRoutine)(myKeyboardPB.interruptRefcon, (void *)&theKeyboardData);
			}
*/
		}
	}
}


USBHIDModuleDispatchTable kbd_TheHIDModuleDispatchTable =
{
	(UInt32)0,
	(USBHIDInstallInterruptProcPtr)kbd_USBHIDInstallInterrupt,
	(USBHIDPollDeviceProcPtr)kbd_USBHIDPollDevice,
	(USBHIDControlDeviceProcPtr)kbd_USBHIDControlDevice,
	(USBHIDGetDeviceInfoProcPtr)kbd_USBHIDGetDeviceInfo,
	(USBHIDEnterPolledModeProcPtr)kbd_USBHIDEnterPolledMode,
	(USBHIDExitPolledModeProcPtr)kbd_USBHIDExitPolledMode,
};
