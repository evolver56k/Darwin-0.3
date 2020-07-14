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
 	File:		CursorDevices.h
 
 	Contains:	Cursor Devices (mouse/trackball/etc) Interfaces.
 
 	Version:	System 7.5
 
 	DRI:		Rich Kubota
 
 	Copyright:	© 1993-1998 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				CursorDevices.i
 					Revision:		25
 					Dated:			1/22/98
 					Last change by:	ngk
 					Last comment:	Change Types.i to MacTypes.i
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __CURSORDEVICES__
#define __CURSORDEVICES__

#ifndef __MACTYPES__
//#include <MacTypes.h>
#endif
#ifndef __MIXEDMODE__
//#include <MixedMode.h>
#endif



#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

/*
	                    * * *  I M P O R T A N T  * * * 

	        You will need CursorDevicesGlue.o to use CDM from PowerPC


	In order to use the Cursor Devices Manager (CDM) on PowerPC systems, you must 
	link with the file CursorDevicesGlue.o and InterfaceLib 1.1.3.  This is necessary
	because the original MixedMode transition code for CDM in InterfaceLib in ROM
	was wrong.  The code in CursorDevicesGlue.o will check to see if the ROM has
    been fixed and calls through to it if so.  If it detects that the ROM has not
	been fixed, it uses its own implementation of the CDM MixedMode transition 
	routines. 
	
*/

typedef short 							ButtonOpcode;
/* ButtonOpcodes */

enum {
	kButtonNoOp					= 0,							/* No action for this button */
	kButtonSingleClick			= 1,							/* Normal mouse button */
	kButtonDoubleClick			= 2,							/* Click-release-click when pressed */
	kButtonClickLock			= 3								/* Click on press, release on next press */
};


enum {
	kButtonCharStroke			= 4,							/* A keystroke, data=(modifiers, char code) */
	kButtonAppleScript			= 5								/* An AppleScript */
};


enum {
	kButtonCustom				= 6								/* Custom behavior, data = CursorDeviceCustomButtonUPP */
};

/* Device Classes */

enum {
	kDeviceClassAbsolute		= 0,							/* a flat-response device */
	kDeviceClassMouse			= 1,							/* mechanical or optical mouse */
	kDeviceClassTrackball		= 2,							/* trackball */
	kDeviceClassTrackPad		= 3
};


enum {
	kDeviceClassScratchPad		= 3,							/* (midas) */
	kDeviceClassJoystick		= 4,							/* (homeRow) */
	kDeviceClassRelTablet		= 5								/* relative tablet */
};


enum {
	kDeviceClass3D				= 6								/* a 3D pointing device */
};

/* Structures used in Cursor Device Manager calls */

struct CursorData {
	struct CursorData *				nextCursorData;				/* next in global list */
	Ptr 							displayInfo;				/* unused (reserved for future) */
	Fixed 							whereX;						/* horizontal position */
	Fixed 							whereY;						/* vertical position */
	Point 							where;						/* the pixel position */
	Boolean 						isAbs;						/* has been stuffed with absolute coords */
	UInt8 							buttonCount;				/* number of buttons currently pressed */
	long 							screenRes;					/* pixels per inch on the current display */
	short 							privateFields[22];			/* fields use internally by CDM */
};
typedef struct CursorData				CursorData;
typedef CursorData *					CursorDataPtr;

struct CursorDevice {
	struct CursorDevice *			nextCursorDevice;			/* pointer to next record in linked list */
	CursorData *					whichCursor;				/* pointer to data for target cursor */
	long 							refCon;						/* application-defined */
	long 							unused;						/* reserved for future */
	OSType 							devID;						/* device identifier (from ADB reg 1) */
	Fixed 							resolution;					/* units/inch (orig. from ADB reg 1) */
	UInt8 							devClass;					/* device class (from ADB reg 1) */
	UInt8 							cntButtons;					/* number of buttons (from ADB reg 1) */
	UInt8 							filler1;					/* reserved for future */
	UInt8 							buttons;					/* state of all buttons */
	UInt8 							buttonOp[8];				/* action performed per button */
	unsigned long 					buttonTicks[8];				/* ticks when button last went up (for debounce) */
	long 							buttonData[8];				/* data for the button operation */
	unsigned long 					doubleClickTime;			/* device-specific double click speed */
	Fixed 							acceleration;				/* current acceleration */
	short 							privateFields[15];			/* fields used internally to CDM */
};
typedef struct CursorDevice				CursorDevice;
typedef CursorDevice *					CursorDevicePtr;
/* for use with CursorDeviceButtonOp when opcode = kButtonCustom */
typedef CALLBACK_API( void , CursorDeviceCustomButtonProcPtr )(CursorDevicePtr ourDevice, short button);
/*
	WARNING: CursorDeviceCustomButtonProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
//naga typedef REGISTER_UPP_TYPE(CursorDeviceCustomButtonProcPtr) 		CursorDeviceCustomButtonUPP;
enum { uppCursorDeviceCustomButtonProcInfo = 0x000ED802 }; 		/* register no_return_value Func(4_bytes:A2, 2_bytes:D3) */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(CursorDeviceCustomButtonUPP)
NewCursorDeviceCustomButtonProc    (CursorDeviceCustomButtonProcPtr userRoutine);
EXTERN_API(void)
CallCursorDeviceCustomButtonProc    (CursorDeviceCustomButtonUPP userRoutine,
								CursorDevicePtr			ourDevice,
								short					button);
#else
#define NewCursorDeviceCustomButtonProc(userRoutine) 			(CursorDeviceCustomButtonUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppCursorDeviceCustomButtonProcInfo, GetCurrentArchitecture())
#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	/* CallCursorDeviceCustomButtonProc can't be called from classic 68k without glue code */
#else
	#define CallCursorDeviceCustomButtonProc(userRoutine, ourDevice, button)  CALL_TWO_PARAMETER_UPP((userRoutine), uppCursorDeviceCustomButtonProcInfo, (ourDevice), (button))
#endif
#endif
EXTERN_API( OSErr )
CursorDeviceMove				(CursorDevicePtr 		ourDevice,
								 long 					deltaX,
								 long 					deltaY)								TWOWORDINLINE(0x7000, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceMoveTo				(CursorDevicePtr 		ourDevice,
								 long 					absX,
								 long 					absY)								TWOWORDINLINE(0x7001, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceFlush				(CursorDevicePtr 		ourDevice)							TWOWORDINLINE(0x7002, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceButtons				(CursorDevicePtr 		ourDevice,
								 short 					buttons)							TWOWORDINLINE(0x7003, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceButtonDown			(CursorDevicePtr 		ourDevice)							TWOWORDINLINE(0x7004, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceButtonUp			(CursorDevicePtr 		ourDevice)							TWOWORDINLINE(0x7005, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceButtonOp			(CursorDevicePtr 		ourDevice,
								 short 					buttonNumber,
								 ButtonOpcode 			opcode,
								 long 					data)								TWOWORDINLINE(0x7006, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceSetButtons			(CursorDevicePtr 		ourDevice,
								 short 					numberOfButtons)					TWOWORDINLINE(0x7007, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceSetAcceleration		(CursorDevicePtr 		ourDevice,
								 Fixed 					acceleration)						TWOWORDINLINE(0x7008, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceDoubleTime			(CursorDevicePtr 		ourDevice,
								 long 					durationTicks)						TWOWORDINLINE(0x7009, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceUnitsPerInch		(CursorDevicePtr 		ourDevice,
								 Fixed 					resolution)							TWOWORDINLINE(0x700A, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceNextDevice			(CursorDevicePtr *		ourDevice)							TWOWORDINLINE(0x700B, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceNewDevice			(CursorDevicePtr *		ourDevice)							TWOWORDINLINE(0x700C, 0xAADB);

EXTERN_API( OSErr )
CursorDeviceDisposeDevice		(CursorDevicePtr 		ourDevice)							TWOWORDINLINE(0x700D, 0xAADB);




/*
	                   * * *  W A R N I N G  * * * 
				
	The routines CrsrDevMoveTo and CrsrDevNextDevice are no longer needed.
	They were added as a work around until the glue code CursorDevicesGlue.o
	was created.  Please use the functions CursorDeviceMoveTo and
	CursorDeviceNextDevice instead.

*/
#if OLDROUTINENAMES
EXTERN_API( OSErr )
CrsrDevMoveTo					(CursorDevicePtr 		ourDevice,
								 long 					absX,
								 long 					absY)								TWOWORDINLINE(0x7001, 0xAADB);

EXTERN_API( OSErr )
CrsrDevNextDevice				(CursorDevicePtr *		ourDevice)							TWOWORDINLINE(0x700B, 0xAADB);

#endif  /* OLDROUTINENAMES */




#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CURSORDEVICES__ */

