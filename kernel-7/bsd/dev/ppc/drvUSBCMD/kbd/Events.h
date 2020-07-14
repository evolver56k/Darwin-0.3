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
 	File:		Events.h
 
 	Contains:	Event Manager Interfaces.
 
 	Version:	Mac OS 8
 
 	DRI:		Rich Wilkes
 
 	Copyright:	© 1985-1998 by Apple Computer, Inc., all rights reserved
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				Events.i
 					Revision:		69
 					Dated:			9/23/98
 					Last change by:	NG
 					Last comment:	More Syncing with Carbon; add GetGlobalMouse
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#include "../OSUtils.h"
#ifndef __EVENTS__
#define __EVENTS__

/*naga
#ifndef __OSUTILS__
#include "../OSUtils.h"
#endif
#ifndef __QUICKDRAW__
#include <Quickdraw.h>
#endif
#ifndef __MACTYPES__
#include <MacTypes.h>
#endif

#if !TARGET_OS_MAC
#ifndef __ENDIAN__
#include <Endian.h>
#endif
#endif*/ /* !TARGET_OS_MAC */



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

typedef UInt16 							EventKind;
typedef UInt16 							EventMask;

enum {
	nullEvent					= 0,
	mouseDown					= 1,
	mouseUp						= 2,
	keyDown						= 3,
	keyUp						= 4,
	autoKey						= 5,
	updateEvt					= 6,
	diskEvt						= 7,
	activateEvt					= 8,
	osEvt						= 15,
	kHighLevelEvent				= 23
};


enum {
	mDownMask					= 1 << mouseDown,				/* mouse button pressed*/
	mUpMask						= 1 << mouseUp,					/* mouse button released*/
	keyDownMask					= 1 << keyDown,					/* key pressed*/
	keyUpMask					= 1 << keyUp,					/* key released*/
	autoKeyMask					= 1 << autoKey,					/* key repeatedly held down*/
	updateMask					= 1 << updateEvt,				/* window needs updating*/
	diskMask					= 1 << diskEvt,					/* disk inserted*/
	activMask					= 1 << activateEvt,				/* activate/deactivate window*/
	highLevelEventMask			= 0x0400,						/* high-level events (includes AppleEvents)*/
	osMask						= 1 << osEvt,					/* operating system events (suspend, resume)*/
	everyEvent					= 0xFFFF						/* all of the above*/
};


enum {
	charCodeMask				= 0x000000FF,
	keyCodeMask					= 0x0000FF00,
	adbAddrMask					= 0x00FF0000,
	osEvtMessageMask			= (long)0xFF000000
};


enum {
																/* OS event messages.  Event (sub)code is in the high byte of the message field.*/
	mouseMovedMessage			= 0x00FA,
	suspendResumeMessage		= 0x0001
};


enum {
	resumeFlag					= 1,							/* Bit 0 of message indicates resume vs suspend*/
	convertClipboardFlag		= 2								/* Bit 1 in resume message indicates clipboard change*/
};


typedef UInt16 							EventModifiers;

enum {
																/* modifiers */
	activeFlagBit				= 0,							/* activate? (activateEvt and mouseDown)*/
	btnStateBit					= 7,							/* state of button?*/
	cmdKeyBit					= 8,							/* command key down?*/
	shiftKeyBit					= 9,							/* shift key down?*/
	alphaLockBit				= 10,							/* alpha lock down?*/
	optionKeyBit				= 11,							/* option key down?*/
	controlKeyBit				= 12,							/* control key down?*/
	rightShiftKeyBit			= 13,							/* right shift key down?*/
	rightOptionKeyBit			= 14,							/* right Option key down?*/
	rightControlKeyBit			= 15							/* right Control key down?*/
};


enum {
	activeFlag					= 1 << activeFlagBit,
	btnState					= 1 << btnStateBit,
	cmdKey						= 1 << cmdKeyBit,
	shiftKey					= 1 << shiftKeyBit,
	alphaLock					= 1 << alphaLockBit,
	optionKey					= 1 << optionKeyBit,
	controlKey					= 1 << controlKeyBit,
	rightShiftKey				= 1 << rightShiftKeyBit,
	rightOptionKey				= 1 << rightOptionKeyBit,
	rightControlKey				= 1 << rightControlKeyBit
};


enum {
	kNullCharCode				= 0,
	kHomeCharCode				= 1,
	kEnterCharCode				= 3,
	kEndCharCode				= 4,
	kHelpCharCode				= 5,
	kBellCharCode				= 7,
	kBackspaceCharCode			= 8,
	kTabCharCode				= 9,
	kLineFeedCharCode			= 10,
	kVerticalTabCharCode		= 11,
	kPageUpCharCode				= 11,
	kFormFeedCharCode			= 12,
	kPageDownCharCode			= 12,
	kReturnCharCode				= 13,
	kFunctionKeyCharCode		= 16,
	kEscapeCharCode				= 27,
	kClearCharCode				= 27,
	kLeftArrowCharCode			= 28,
	kRightArrowCharCode			= 29,
	kUpArrowCharCode			= 30,
	kDownArrowCharCode			= 31,
	kDeleteCharCode				= 127,
	kNonBreakingSpaceCharCode	= 202
};


struct EventRecord {
	EventKind 						what;
	UInt32 							message;
	UInt32 							when;
	Point 							where;
	EventModifiers 					modifiers;
};
typedef struct EventRecord				EventRecord;
#if 0     //naga
typedef CALLBACK_API( void , FKEYProcPtr )(void );
typedef STACK_UPP_TYPE(FKEYProcPtr) 							FKEYUPP;
enum { uppFKEYProcInfo = 0x00000000 }; 							/* pascal no_return_value Func() */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(FKEYUPP)
NewFKEYProc					   (FKEYProcPtr				userRoutine);
EXTERN_API(void)
CallFKEYProc				   (FKEYUPP					userRoutine);
#else
#define NewFKEYProc(userRoutine) 								(FKEYUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppFKEYProcInfo, GetCurrentArchitecture())
#define CallFKEYProc(userRoutine) 								CALL_ZERO_PARAMETER_UPP((userRoutine), uppFKEYProcInfo)
#endif
EXTERN_API( void )
GetMouse						(Point *				mouseLoc)							ONEWORDINLINE(0xA972);

EXTERN_API( Boolean )
Button							(void)														ONEWORDINLINE(0xA974);

EXTERN_API( Boolean )
StillDown						(void)														ONEWORDINLINE(0xA973);

EXTERN_API( Boolean )
WaitMouseUp						(void)														ONEWORDINLINE(0xA977);

EXTERN_API( UInt32 )
TickCount						(void)														ONEWORDINLINE(0xA975);

EXTERN_API( UInt32 )
KeyTranslate					(const void *			transData,
								 UInt16 				keycode,
								 UInt32 *				state)								ONEWORDINLINE(0xA9C3);

EXTERN_API( UInt32 )
GetCaretTime					(void)														TWOWORDINLINE(0x2EB8, 0x02F4);


/* 
	QuickTime 3.0 supports GetKeys() on unix and win32
	But, on little endian machines you will have to be
	careful about bit numberings and/or use a KeyMapByteArray
	instead.
*/
#endif 0  //naga
//naga#if TARGET_OS_MAC

typedef UInt32 							KeyMap[4];
/*naga
#else
typedef BigEndianLong 					KeyMap[4];
#endif*/ /* TARGET_OS_MAC */

typedef UInt8 							KeyMapByteArray[16];
EXTERN_API( void )
GetKeys							(KeyMap 				theKeys)							ONEWORDINLINE(0xA976);


/* Obsolete event types & masks */

enum {
	networkEvt					= 10,
	driverEvt					= 11,
	app1Evt						= 12,
	app2Evt						= 13,
	app3Evt						= 14,
	app4Evt						= 15,
	networkMask					= 0x0400,
	driverMask					= 0x0800,
	app1Mask					= 0x1000,
	app2Mask					= 0x2000,
	app3Mask					= 0x4000,
	app4Mask					= 0x8000
};


struct EvQEl {
	QElemPtr 						qLink;
	SInt16 							qType;
	EventKind 						evtQWhat;					/* this part is identical to the EventRecord as defined above */
	UInt32 							evtQMessage;
	UInt32 							evtQWhen;
	Point 							evtQWhere;
	EventModifiers 					evtQModifiers;
};
typedef struct EvQEl					EvQEl;
typedef EvQEl *							EvQElPtr;
typedef CALLBACK_API( void , GetNextEventFilterProcPtr )(EventRecord *theEvent, Boolean *result);
/*
	WARNING: GetNextEventFilterProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
//naga
#if 0
typedef REGISTER_UPP_TYPE(GetNextEventFilterProcPtr) 			GetNextEventFilterUPP;
enum { uppGetNextEventFilterProcInfo = 0x000000BF }; 			/* SPECIAL_CASE_PROCINFO(11) */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(GetNextEventFilterUPP)
NewGetNextEventFilterProc	   (GetNextEventFilterProcPtr userRoutine);
EXTERN_API(void)
CallGetNextEventFilterProc	   (GetNextEventFilterUPP	userRoutine,
								EventRecord *			theEvent,
								Boolean *				result);
#else
#define NewGetNextEventFilterProc(userRoutine) 					(GetNextEventFilterUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppGetNextEventFilterProcInfo, GetCurrentArchitecture())
#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	/* CallGetNextEventFilterProc can't be called from classic 68k without glue code */
#else
	#define CallGetNextEventFilterProc(userRoutine, theEvent, result)  CALL_TWO_PARAMETER_UPP((userRoutine), uppGetNextEventFilterProcInfo, (theEvent), (result))
#endif
#endif
typedef GetNextEventFilterUPP 			GNEFilterUPP;
EXTERN_API( QHdrPtr )
GetEvQHdr						(void)														THREEWORDINLINE(0x2EBC, 0x0000, 0x014A);

EXTERN_API( UInt32 )
GetDblTime						(void)														TWOWORDINLINE(0x2EB8, 0x02F0);

EXTERN_API( void )
SetEventMask					(EventMask 				value)								TWOWORDINLINE(0x31DF, 0x0144);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PPostEvent(__A0, __D0, __A1)
																							#endif
EXTERN_API( OSErr )
PPostEvent						(EventKind 				eventCode,
								 UInt32 				eventMsg,
								 EvQElPtr *				qEl)								TWOWORDINLINE(0xA12F, 0x2288);

#define KeyTrans(transData, keycode, state) KeyTranslate(transData, keycode, state)
EXTERN_API( Boolean )
GetNextEvent					(EventMask 				eventMask,
								 EventRecord *			theEvent)							ONEWORDINLINE(0xA970);

EXTERN_API( Boolean )
WaitNextEvent					(EventMask 				eventMask,
								 EventRecord *			theEvent,
								 UInt32 				sleep,
								 RgnHandle 				mouseRgn) /* can be NULL */			ONEWORDINLINE(0xA860);

EXTERN_API( Boolean )
EventAvail						(EventMask 				eventMask,
								 EventRecord *			theEvent)							ONEWORDINLINE(0xA971);


																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 PostEvent(__A0, __D0)
																							#endif
EXTERN_API( OSErr )
PostEvent						(EventKind 				eventNum,
								 UInt32 				eventMsg)							ONEWORDINLINE(0xA02F);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 OSEventAvail(__D0, __A0)
																							#endif
EXTERN_API( Boolean )
OSEventAvail					(EventMask 				mask,
								 EventRecord *			theEvent)							TWOWORDINLINE(0xA030, 0x5240);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 GetOSEvent(__D0, __A0)
																							#endif
EXTERN_API( Boolean )
GetOSEvent						(EventMask 				mask,
								 EventRecord *			theEvent)							TWOWORDINLINE(0xA031, 0x5240);

EXTERN_API( void )
FlushEvents						(EventMask 				whichMask,
								 EventMask 				stopMask)							TWOWORDINLINE(0x201F, 0xA032);

EXTERN_API( void )
SystemClick						(const EventRecord *	theEvent,
								 WindowPtr 				theWindow)							ONEWORDINLINE(0xA9B3);

EXTERN_API( void )
SystemTask						(void)														ONEWORDINLINE(0xA9B4);

EXTERN_API( Boolean )
SystemEvent						(const EventRecord *	theEvent)							ONEWORDINLINE(0xA9B2);

/* This call is only available as part of the Carbon API*/
EXTERN_API( void )
GetGlobalMouse					(Point *				globalMouse);



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
#endif 0   //naga
#endif /* __EVENTS__ */

