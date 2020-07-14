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
	File:		KeyIn.c

	Contains:	ADB keyboard simulation (non-shim based).

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				David Ferguson

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TEB)	Ted Burge
		(BWS)	Brent Schorsch
		(BG)	Bill Galcher
		(DKF)	David Ferguson
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB24>	 9/10/98	TEB		[2267641]  Changed the right shift, right option and right
									control keys to use 0x38, 0x3A and 0x3B and not the type 3
									keyboard virtual key codes: 0x3C, 0x3D, and 0x3E listed here
									respectively.
	 <USB23>	 6/29/98	CJK		correct numeric keypad = key so that it reports as keypad =, not
									as keypad enter
	 <USB22>	 6/22/98	CJK		change left & right modifier keys to be unique.  This change
									only affects the DDK, not C1.
	 <USB21>	 6/18/98	BWS		fix interrupt refcon support
	 <USB20>	 6/10/98	CJK		make certain power key is sent twice (before keyup).
	 <USB19>	 6/10/98	CJK		[2238229]  change applicatinon key to $6E
	 <USB18>	 5/28/98	CJK		change to MPW type file
	 <USB17>	 5/22/98	CJK		Fix for CodeWarrior
	 <USB16>	 5/19/98	BG		Add include of LowMem.h to bring in needed prototype.
	 <USB15>	 4/26/98	CJK		update to match reworked keyboard interface driver
	 <USB14>	  4/9/98	CJK		replace include of USBHIDModules.h with include of USB.h
		<13>	  4/8/98	CJK		update selector name
		<12>	  4/8/98	CJK		rework PostUSBKeyToMac to work the same way as the standard interrupt
									handler
		<11>	 3/26/98	CJK		remove debugstr
		<10>	 3/19/98	CJK		Remove numlock & capslock toggle/led handling
		 <9>	 3/17/98	CJK		change } to just }. MetroWerks has a problem with }.
		 <8>	 3/12/98	CJK		Re-work modifier key handling so that it only propagates key
									state changes when the key is pressed, ignores modifier key up
									key codes, and "simulates" modifier key up routines to the Mac
									OS (when the keystate is toggled back to 'not pressed').
		 <7>	  3/2/98	CJK		remove include of KBDHIDEmulation.h
		 <6>	 2/27/98	CJK		disable call to HID Module control routine.
		 <5>	 2/27/98	CJK		Add include of USBHIDModules.h. Change keycode constants to be
									kUSB...
		 <4>	 2/26/98	CJK		add capslock, scroll lock, and num lock LED support.
	  <USB3>	 2/19/98	DKF		Fix keymapping for \, swap alt & command, update lowmem
									accessors
		 <2>	 2/17/98	CJK		Add change history.
		 <1>	 2/17/98	CJK		First time check in.
*/

/*
 USB Keyboard Translation to Macintosh 
 */
#include "../driverservices.h"
//#include <Types.h>
#include "Events.h"
//#include <Resources.h>
//#include <LowMem.h>
#include "../USB.h"
#include "KeyboardModule.h"


#define DOWN		0
#define UP			1

#define TRUE		1
#define FALSE		0

#define FakeADBAddr		16
#define FakeKBDType		2		// this should be the same as the Apple extended keyboard for now

static UInt32	myKeyMAP[4];
static UInt32	keyTransState;
static Handle	handleKCHR;
static UInt8*	KCHRptr;

typedef KeyMap * KeyMapPtr;

/* prototypes */
Boolean KeyInArray(UInt8 key, UInt8 *array, UInt16 len);
Boolean SetBit(UInt8 *bitmapArray, UInt16 index, Boolean value); 
void  PostADBKeyToMac(UInt16 virtualKeycode, UInt8 state);

/* when we move to master interfaces we can get this stuff from LowMemPriv.h */
/* be sure to turn on DIRECT_LOWMEM_ACCESSORS */
#define LMSetKbdVars(value) ((*(short *)0x0216) = (value))
#define LMSetKeyLast(value) ((*(short *)0x0184) = (value))
#define LMSetKeyTime(value) ((*(long *)0x0186) = (value))
#define LMSetKeyRepTime(value) ((*(long *)0x018A) = (value))
#define LMSetKeyMap(KeyMapValue)	BlockMove((Ptr)(KeyMapValue), (Ptr)0x0174, sizeof(KeyMap))


// index represents USB keyboard usage value, content is Mac virtual keycode
static UInt8	USBKMAP[256] = {  
	0xFF, 	/* 00 no event */		
	0xFF,	/* 01 ErrorRollOver */	
	0xFF,	/* 02 POSTFail */	
	0xFF,	/* 03 ErrorUndefined */	
	0x00,	/* 04 A */
	0x0B,	/* 05 B */
	0x08,	/* 06 C */
	0x02,	/* 07 D */
	0x0E,	/* 08 E */
	0x03,	/* 09 F */
	0x05,	/* 0A G */
	0x04,	/* 0B H */
	0x22,	/* 0C I */
	0x26,	/* 0D J */
	0x28,	/* 0E K */
	0x25,	/* 0F L */

	0x2E, 	/* 10 M */		
	0x2D,	/* 11 N */	
	0x1F,	/* 12 O */	
	0x23,	/* 13 P */	
	0x0C,	/* 14 Q */
	0x0F,	/* 15 R */
	0x01,	/* 16 S */
	0x11,	/* 17 T */
	0x20,	/* 18 U */
	0x09,	/* 19 V */
	0x0D,	/* 1A W */
	0x07,	/* 1B X */
	0x10,	/* 1C Y */
	0x06,	/* 1D Z */
	0x12,	/* 1E 1/! */
	0x13,	/* 1F 2/@ */

	0x14, 	/* 20 3 # */		
	0x15,	/* 21 4 $ */	
	0x17,	/* 22 5 % */	
	0x16,	/* 23 6 ^ */	
	0x1A,	/* 24 7 & */
	0x1C,	/* 25 8 * */
	0x19,	/* 26 9 ( */
	0x1D,	/* 27 0 ) */
	0x24,	/* 28 Return (Enter) */
	0x35,	/* 29 ESC */
	0x33,	/* 2A Delete (Backspace) */
	0x30,	/* 2B Tab */
	0x31,	/* 2C Spacebar */
	0x1B,	/* 2D - _ */
	0x18,	/* 2E = + */
	0x21,	/* 2F [ { */

	0x1E, 	/* 30 ] } */		
	0x2A,	/* 31 \ | */	
	0xFF,	/* 32 Non-US # and ~ (what?!!!) */	
	0x29,	/* 33 ; : */	
	0x27,	/* 34 ' " */
	0x32,	/* 35 ` ~ */
	0x2B,	/* 36 , < */
	0x2F,	/* 37 . > */
	0x2C,	/* 38 / ? */
	0x39,	/* 39 Caps Lock */
	0x7A,	/* 3A F1 */
	0x78,	/* 3B F2 */
	0x63,	/* 3C F3 */
	0x76,	/* 3D F4 */
	0x60,	/* 3E F5 */
	0x61,	/* 3F F6 */

	0x62, 	/* 40 F7 */		
	0x64,	/* 41 F8 */	
	0x65,	/* 42 F9 */	
	0x6D,	/* 43 F10 */	
	0x67,	/* 44 F11 */
	0x6F,	/* 45 F12 */
	0x69,	/* 46 F13/PrintScreen */
	0x6B,	/* 47 F14/ScrollLock */
	0x71,	/* 48 F15/Pause */				
	0x72,	/* 49 Insert */
	0x73,	/* 4A Home */
	0x74,	/* 4B PageUp */
	0x75,	/* 4C Delete Forward */
	0x77,	/* 4D End */
	0x79,	/* 4E PageDown */
	0x7C,	/* 4F RightArrow */

	0x7B, 	/* 50 LeftArrow */		
	0x7D,	/* 51 DownArrow */	
	0x7E,	/* 52 UpArrow */	
	0x47,	/* 53 NumLock/Clear */	
	0x4B,	/* 54 Keypad / */
	0x43,	/* 55 Keypad * */
	0x4E,	/* 56 Keypad - */
	0x45,	/* 57 Keypad + */
	0x4C,	/* 58 Keypad Enter */
	0x53,	/* 59 Keypad 1 */
	0x54,	/* 5A Keypad 2 */
	0x55,	/* 5B Keypad 3 */
	0x56,	/* 5C Keypad 4 */
	0x57,	/* 5D Keypad 5 */
	0x58,	/* 5E Keypad 6 */
	0x59,	/* 5F Keypad 7 */

	0x5B, 	/* 60 Keypad 8 */		
	0x5C,	/* 61 Keypad 9 */	
	0x52,	/* 62 Keypad 0 */	
	0x41,	/* 63 Keypad . */	
	0xFF,	/* 64 Non-US \ and  | (what ??!!) */
	0x6E,	/* 65 ApplicationKey (not on a mac!)*/
	0x7F,	/* 66 PowerKey  */
	0x51,	/* 67 Keypad = */
	0x69,	/* 68 F13 */
	0x6B,	/* 69 F14 */
	0x71,	/* 6A F15 */
	0xFF,	/* 6B F16 */
	0xFF,	/* 6C F17 */
	0xFF,	/* 6D F18 */
	0xFF,	/* 6E F19 */
	0xFF,	/* 6F F20 */

	0x5B, 	/* 70 F21 */		
	0x5C,	/* 71 F22 */	
	0x52,	/* 72 F23 */	
	0x41,	/* 73 F24 */	
	0xFF,	/* 74 Execute */
	0xFF,	/* 75 Help */
	0x7F,	/* 76 Menu */
	0x4C,	/* 77 Select */
	0x69,	/* 78 Stop */
	0x6B,	/* 79 Again */
	0x71,	/* 7A Undo */
	0xFF,	/* 7B Cut */
	0xFF,	/* 7C Copy */
	0xFF,	/* 7D Paste */
	0xFF,	/* 7E Find */
	0xFF,	/* 7F Mute */
	
	0xFF, 	/* 80 no event */		
	0xFF,	/* 81 no event */	
	0xFF,	/* 82 no event */	
	0xFF,	/* 83 no event */	
	0xFF,	/* 84 no event */
	0xFF,	/* 85 no event */
	0xFF,	/* 86 no event */
	0xFF,	/* 87 no event */
	0xFF,	/* 88 no event */
	0xFF,	/* 89 no event */
	0xFF,	/* 8A no event */
	0xFF,	/* 8B no event */
	0xFF,	/* 8C no event */
	0xFF,	/* 8D no event */
	0xFF,	/* 8E no event */
	0xFF,	/* 8F no event */

	0xFF, 	/* 90 no event */		
	0xFF,	/* 91 no event */	
	0xFF,	/* 92 no event */	
	0xFF,	/* 93 no event */	
	0xFF,	/* 94 no event */
	0xFF,	/* 95 no event */
	0xFF,	/* 96 no event */
	0xFF,	/* 97 no event */
	0xFF,	/* 98 no event */
	0xFF,	/* 99 no event */
	0xFF,	/* 9A no event */
	0xFF,	/* 9B no event */
	0xFF,	/* 9C no event */
	0xFF,	/* 9D no event */
	0xFF,	/* 9E no event */
	0xFF,	/* 9F no event */

	0xFF, 	/* A0 no event */		
	0xFF,	/* A1 no event */	
	0xFF,	/* A2 no event */	
	0xFF,	/* A3 no event */	
	0xFF,	/* A4 no event */
	0xFF,	/* A5 no event */
	0xFF,	/* A6 no event */
	0xFF,	/* A7 no event */
	0xFF,	/* A8 no event */
	0xFF,	/* A9 no event */
	0xFF,	/* AA no event */
	0xFF,	/* AB no event */
	0xFF,	/* AC no event */
	0xFF,	/* AD no event */
	0xFF,	/* AE no event */
	0xFF,	/* AF no event */

	0xFF, 	/* B0 no event */		
	0xFF,	/* B1 no event */	
	0xFF,	/* B2 no event */	
	0xFF,	/* B3 no event */	
	0xFF,	/* B4 no event */
	0xFF,	/* B5 no event */
	0xFF,	/* B6 no event */
	0xFF,	/* B7 no event */
	0xFF,	/* B8 no event */
	0xFF,	/* B9 no event */
	0xFF,	/* BA no event */
	0xFF,	/* BB no event */
	0xFF,	/* BC no event */
	0xFF,	/* BD no event */
	0xFF,	/* BE no event */
	0xFF,	/* BF no event */

	0xFF, 	/* C0 no event */		
	0xFF,	/* C1 no event */	
	0xFF,	/* C2 no event */	
	0xFF,	/* C3 no event */	
	0xFF,	/* C4 no event */
	0xFF,	/* C5 no event */
	0xFF,	/* C6 no event */
	0xFF,	/* C7 no event */
	0xFF,	/* C8 no event */
	0xFF,	/* C9 no event */
	0xFF,	/* CA no event */
	0xFF,	/* CB no event */
	0xFF,	/* CC no event */
	0xFF,	/* CD no event */
	0xFF,	/* CE no event */
	0xFF,	/* CF no event */

	0xFF, 	/* D0 no event */		
	0xFF,	/* D1 no event */	
	0xFF,	/* D2 no event */	
	0xFF,	/* D3 no event */	
	0xFF,	/* D4 no event */
	0xFF,	/* D5 no event */
	0xFF,	/* D6 no event */
	0xFF,	/* D7 no event */
	0xFF,	/* D8 no event */
	0xFF,	/* D9 no event */
	0xFF,	/* DA no event */
	0xFF,	/* DB no event */
	0xFF,	/* DC no event */
	0xFF,	/* DD no event */
	0xFF,	/* DE no event */
	0xFF,	/* DF no event */

	0x3B, 	/* E0 left control key */		
	0x38,	/* E1 left shift key key */	
	0x3A,	/* E2 left alt/option key */	
	0x37,	/* E3 left GUI (windows/cmd) key */	
	
	0x3B,	/* E4 right control key */ 
	0x38,	/* E5 right shift key key */ 
	0x3A,	/* E6 right alt/option key */ 
	0x37,	/* E7 right GUI (windows/cmd) key */
	0xFF,	/* E8 no event */
	0xFF,	/* E9 no event */
	0xFF,	/* EA no event */
	0xFF,	/* EB no event */
	0xFF,	/* EC no event */
	0xFF,	/* ED no event */
	0xFF,	/* EE no event */
	0xFF,	/* EF no event */
	
	0xFF, 	/* F0 no event */		
	0xFF,	/* F1 no event */	
	0xFF,	/* F2 no event */	
	0xFF,	/* F3 no event */	
	0xFF,	/* F4 no event */
	0xFF,	/* F5 no event */
	0xFF,	/* F6 no event */
	0xFF,	/* F7 no event */
	0xFF,	/* F8 no event */
	0xFF,	/* F9 no event */
	0xFF,	/* FA no event */
	0xFF,	/* FB no event */
	0xFF,	/* FC no event */
	0xFF,	/* FD no event */
	0xFF,	/* FE no event */
	0xFF,	/* FF no event */
};
		
void
InitUSBKeyboard()
{
	handleKCHR = GetResource('KCHR',0);	// US keyboard mapping (handled differently by ADB Mgr)
	HLock(handleKCHR);
	KCHRptr = (UInt8 *)*handleKCHR;
}



void PostUSBKeyToMac(UInt16 rawUSBkey)
{
static	UInt8	oldLEDState = 0x00;
static	UInt8	newLEDState = 0x00;

static	UInt8	capsLockState = 0x00;
static	UInt8	numLockState = 0x00;
static	UInt8	scrollLockState = 0x00;

register UInt8	virtualKeycode, keystate;

	
	if (KCHRptr == 0)
	{
		InitUSBKeyboard();
	}
	
	keystate = (rawUSBkey & 0x8000) ? UP : DOWN;
	rawUSBkey &= 0x0FF;
	
	if (keystate == DOWN)
	{
		newLEDState = oldLEDState;
		
		switch (rawUSBkey)
		{
// Note:  This switch statement is being left it to make it easy to add "toggling" keys in the future.
//        it used to support toggled numlock & scroll lock...  It doesn't anymore.
			case kUSBCapsLockKey:
				newLEDState ^= (1 << kCapsLockLED);
				keystate = (newLEDState & (1 << kCapsLockLED)) ? DOWN : UP;
				break;
		}
		
		if (newLEDState != oldLEDState)
		{
			oldLEDState = newLEDState;
			kbd_USBHIDControlDevice(kHIDSetLEDStateByBits, &newLEDState);
		}
	}
	else
	{
		switch (rawUSBkey)
		{
// Note:  This switch statement is being left it to make it easy to add "toggling" keys in the future.
			case kUSBCapsLockKey:
				return;
				break;
		}
	}
	
	// look up rawUSBkey in KMAP resource to get virtual keycode
	if (rawUSBkey < sizeof(USBKMAP))
	{
		virtualKeycode = USBKMAP[rawUSBkey];
	} 
	else 
	{
//		DebugStr("\pPostUSBKeyToMac: Need bigger KMAP table");
		virtualKeycode = 0xFF;
	}

	PostADBKeyToMac(virtualKeycode, keystate);
	if (virtualKeycode == 0x7F)
		PostADBKeyToMac(virtualKeycode, keystate);
}

void 
PostADBKeyToMac(UInt16 virtualKeycode, UInt8 state)
{
	UInt32	keyEventMsg;
	
	if (virtualKeycode > 127) return;  // not handled by MacOS!
	
	// stop repeating
	LMSetKeyLast(0);
	LMSetKbdVars(0);
	
	// update our keymap
	SetBit((UInt8 *)myKeyMAP, virtualKeycode, state == DOWN ? 1 : 0);
	LMSetKeyMap(&myKeyMAP);

	// set this keyboard as the last keyboard
	LMSetKbdLast(FakeADBAddr);
	LMSetKbdType(FakeKBDType);
	
	// call KeyTrans to get character code
	virtualKeycode |= ((myKeyMAP[1]<<9) & 0x00FE00) | ((myKeyMAP[1]>>7) & 0x0100) | ((state==UP) ? 0x080 : 0);
	keyEventMsg = KeyTranslate(KCHRptr, virtualKeycode , &keyTransState);
	virtualKeycode &= 0x7F;
	
	if (keyEventMsg & 0xFFFF0000) {
		// post event
		UInt32 event =(keyEventMsg & 0xFF000000) | ((FakeADBAddr << 16) & 0x0FF0000) | ((virtualKeycode << 8) & 0x0FF00)  | ((keyEventMsg>>16) & 0x0FF);
		if (state == DOWN){
			UInt32 ticks = TickCount();
			LMSetKeyTime(ticks);
			LMSetKeyRepTime(ticks);
			LMSetKeyLast(event & 0x0FFFF);
			LMSetKbdVars((event>>16) & 0x0FFFF);
		}		
		PostEvent((state == DOWN ? keyDown : keyUp), event);
	}
	if (keyEventMsg & 0x0000FFFF) {
		// post event
		UInt32 event =((keyEventMsg<<16) & 0xFF000000) | ((FakeADBAddr << 16) & 0x0FF0000) | ((virtualKeycode << 8) & 0x0FF00)  | (keyEventMsg & 0x0FF);
		if (state == DOWN){
			UInt32 ticks = TickCount();
			LMSetKeyTime(ticks);
			LMSetKeyRepTime(ticks);
			LMSetKeyLast(event & 0x0FFFF);
			LMSetKbdVars((event>>16) & 0x0FFFF);
		}		
		PostEvent((state == DOWN ? keyDown : keyUp), event);
	}
}

// Sets the bitmapArray[index] to value
// returns old value;
Boolean
SetBit(UInt8 *bitmapArray, UInt16 index, Boolean value)
{	
	UInt32	mask = 0x1 << (index % 8 );
	Boolean	oldVal;
	
	oldVal = (bitmapArray[index/8] & mask) ? TRUE : FALSE;
	
	if (value){
		bitmapArray[index/8] |= mask;
	}else{
		bitmapArray[index/8] &= ~mask;
	}
	
	return (oldVal);
}

void USBDemoKeyIn(UInt32 refcon, void * theData)
{
#pragma unused (refcon)

USBHIDDataPtr	pTheKeyboardData;
register		UInt8	i;

	pTheKeyboardData = (USBHIDDataPtr)theData;
	for (i=0; i<pTheKeyboardData->kbd.keycount; i++)
	{
		// no shim installed, let's just post some Macintosh keyevents
		PostUSBKeyToMac(pTheKeyboardData->kbd.usbkeycode[i]);
	}

}

