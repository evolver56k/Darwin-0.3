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
 	File:		Types.h
 
 	Contains:	Basic Macintosh data types.
 
 	Version:	System 7.5
 
 	DRI:		Nick Kledzik
 
 	Copyright:	© 1984-1996 by Apple Computer, Inc.
 				All rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Alan Mimms
 				With Interfacer:	2.0d13   (PowerPC native)
 				From:				Types.i
 					Revision:		83
 					Dated:			1/30/96
 					Last change by:	ngk
 					Last comment:	Fix extended80 for Metrowerks 68K
 
 	Bugs:		Report bugs to Radar component “System Interfaces”, “Latest”
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __TYPES__
#define __TYPES__

#ifndef __CONDITIONALMACROS__
#include <ConditionalMacros.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif


enum {
	noErr						= 0
};

#ifndef NULL
/* Symantec C compilers (but not C++) want NULL and nil to be (void*)0  */
#if !defined(__cplusplus) && (defined(__SC__) || defined(THINK_C))
#define NULL ((void *) 0)
#else
#define	NULL 0
#endif
#endif

#ifndef nil
#define nil NULL
#endif
typedef unsigned char Byte;
typedef signed char SignedByte;
typedef Byte UInt8;
typedef SignedByte SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned long UInt32;
typedef signed long SInt32;
typedef UInt16 UniChar;
typedef char *Ptr;
/*  pointer to a master pointer */
typedef Ptr *Handle;
/* fixed point arithmatic type */
typedef long Fixed;
typedef Fixed *FixedPtr;
typedef long Fract;
typedef Fract *FractPtr;
/*
Note: on PowerPC extended is undefined.
      on 68K when mc68881 is on, extended is 96 bits.  
             when mc68881 is off, extended is 80 bits.  
      Some old toolbox routines require an 80 bit extended so we define extended80
*/
struct _extended80 { short exp; short man[4]; };
struct _extended96 { short exp[2]; short man[4]; };

#if GENERATING68K
#if defined(__MWERKS__)
/* Note: Metrowerks on 68K doesn't declare 'extended' or 'comp' implicitly. */
typedef long double extended;
typedef struct comp { long hi,lo; } comp;
#elif defined(THINK_C)
/* Note: THINK C doesn't declare 'comp' implicitly and needs magic for 'extended' */
typedef struct { short man[4]; } comp;
typedef struct _extended80 __extended;	/*  <-- this line is magic */
typedef __extended extended;
#endif
#endif

#if GENERATING68K && GENERATING68881
typedef extended 			extended96;
#else
typedef struct _extended96 	extended96;
#endif

#if GENERATING68K && !GENERATING68881
typedef extended 			extended80;
#else
typedef struct _extended80 	extended80;
#endif

/*
Note: float_t and double_t are "natural" computational types
      (i.e.the compiler/processor can most easily do floating point
	  operations with that type.) 
*/
#if GENERATINGPOWERPC
/* on PowerPC, double = 64-bit which is fastest.  float = 32-bit */
typedef float float_t;
typedef double double_t;
#else
/* on 68K, long double (a.k.a. extended) is always the fastest.  It is 80 or 96-bits */
typedef long double float_t;
typedef long double double_t;
#endif
struct SInt64 {
	SInt32 							hi;
	UInt32 							lo;
};
typedef struct SInt64 SInt64;

struct UInt64 {
	UInt32 							hi;
	UInt32 							lo;
};
typedef struct UInt64 UInt64;

/*
	wide and UnsignedWide are being replace by SInt64 and UInt64 in the Math64 library and interfaces
*/
typedef SInt64 wide;
typedef SInt64 *WidePtr;
typedef UInt64 UnsignedWide;
typedef UInt64 *UnsignedWidePtr;
#if defined(__SC__) && !defined(__STDC__) && defined(__cplusplus)
	class __machdl HandleObject {};
#if !GENERATINGPOWERPC
	class __pasobj PascalObject {};
#endif
#endif

enum {
	false						= 0,
	true						= 1
};

typedef unsigned char Boolean;

enum {
	v							= 0,
	h							= 1
};

typedef SInt8 VHSelect;
typedef long (*ProcPtr)();
typedef pascal void (*Register68kProcPtr)();
typedef ProcPtr *ProcHandle;
#if GENERATINGCFM
/*
	Note that the RoutineDescriptor structure is defined in the 
		MixedMode.h header 
*/
typedef struct RoutineDescriptor *UniversalProcPtr;
#else
typedef ProcPtr UniversalProcPtr;
#endif
typedef UniversalProcPtr *UniversalProcHandle;
typedef unsigned char Str255[256];
typedef unsigned char Str63[64];
typedef unsigned char Str32[33];
typedef unsigned char Str31[32];
typedef unsigned char Str27[28];
typedef unsigned char Str15[16];
/*
	The type Str32 is used in many AppleTalk based data structures.
	It holds up to 32 one byte chars.  The problem is that with the
	length byte it is 33 bytes long.  This can cause weird alignment
	problems in structures.  To fix this the type "Str32Field" has
	been created.  It should only be used to hold 32 chars, but
	it is 34 bytes long so that there are no alignment problems.
*/
typedef unsigned char Str32Field[34];
typedef unsigned char *StringPtr;
typedef StringPtr *StringHandle;
typedef const unsigned char *ConstStr255Param;
typedef const unsigned char *ConstStr63Param;
typedef const unsigned char *ConstStr32Param;
typedef const unsigned char *ConstStr31Param;
typedef const unsigned char *ConstStr27Param;
typedef const unsigned char *ConstStr15Param;
#ifdef __cplusplus
inline unsigned char StrLength(ConstStr255Param string) { return (*string); }
#else
#define StrLength(string) (*(unsigned char *)(string))
#endif
#if OLDROUTINENAMES
#define Length(string) StrLength(string)
#endif
/* error code */
typedef short OSErr;
typedef short ScriptCode;
typedef short LangCode;
typedef short RegionCode;
typedef unsigned long FourCharCode;
typedef short CharParameter;

enum {
	normal						= 0,
	bold						= 1,
	italic						= 2,
	underline					= 4,
	outline						= 8,
	shadow						= 0x10,
	condense					= 0x20,
	extend						= 0x40
};

typedef unsigned char Style;
typedef short StyleParameter;
typedef Style StyleField;
/*
	CharParameter is only used in function parameter lists.  
	StyleParameter is only used in function parameter lists.
	StyleField is only used in struct fields.  
	The original Macintosh toolbox in 68K pascal defined Style as a SET.  
	Both Style and CHAR can occupy 8-bits in packed records or 16-bits when used as fields
	in non-packed records or as parameters. 
	These intermediate types hide that difference.
*/
typedef FourCharCode OSType;
typedef FourCharCode ResType;
typedef OSType *OSTypePtr;
typedef ResType *ResTypePtr;
struct Point {
	short 							v;
	short 							h;
};
typedef struct Point Point;

typedef Point *PointPtr;
struct Rect {
	short 							top;
	short 							left;
	short 							bottom;
	short 							right;
};
typedef struct Rect Rect;

typedef Rect *RectPtr;
/*
	kVariableLengthArray is used in array bounds to specify a variable length array.
	It is ususally used in variable length structs when the last field is an array
	of any size.  Before ANSI C, we used zero as the bounds of variable length 
	array, but that is illegal in ANSI C.  Example:
	
		struct FooList 
		{
			short 	listLength;
			Foo		elements[kVariableLengthArray];
		};
*/

enum {
	kVariableLengthArray		= 1
};

/* Numeric version part of 'vers' resource */
struct NumVersion {
	UInt8 							majorRev;					/*1st part of version number in BCD*/
	UInt8 							minorAndBugRev;				/*2nd & 3rd part of version number share a byte*/
	UInt8 							stage;						/*stage code: dev, alpha, beta, final*/
	UInt8 							nonRelRev;					/*revision level of non-released version*/
};
typedef struct NumVersion NumVersion;

/* Wrapper so NumVersion can be accessed as a 32-bit value */
union NumVersionVariant {
	NumVersion 						parts;
	unsigned long 					whole;
};
typedef union NumVersionVariant NumVersionVariant;

/* 'vers' resource format */
struct VersRec {
	NumVersion 						numericVersion;				/*encoded version number*/
	short 							countryCode;				/*country code from intl utilities*/
	Str255 							shortVersion;				/*version number string - worst case*/
	Str255 							reserved;					/*longMessage string packed after shortVersion*/
};
typedef struct VersRec VersRec;

typedef VersRec *VersRecPtr;
typedef VersRecPtr *VersRecHndl;
typedef SInt32 OSStatus;
typedef void *LogicalAddress;
typedef const void *ConstLogicalAddress;
typedef UInt8 *BytePtr;
typedef UInt32 ByteCount;
typedef UInt32 ByteOffset;
typedef UInt32 ItemCount;
typedef void *PhysicalAddress;
typedef UInt32 OptionBits;
typedef UInt32 PBVersion;
typedef SInt32 Duration;
typedef UInt64 AbsoluteTime;

enum {
	kSizeOfTimeObject			= 16
};

/* TimeObject type definitions */
struct TimeObject {
	UInt8 							hidden[16];
};
typedef struct TimeObject TimeObject;

typedef TimeObject *TimeObjectPtr;
typedef const TimeObject *ConstTimeObjectPtr;
struct TimeObjectInterval {
	UInt8 							hidden[16];
};
typedef struct TimeObjectInterval TimeObjectInterval;

typedef TimeObjectInterval *TimeObjectIntervalPtr;
typedef const TimeObjectInterval *ConstTimeObjectIntervalPtr;
#if FOR_SYSTEM8_COOPERATIVE
/*
	RefLabels: these are non-localizable identifiers for system objects. These should
	be passed by reference.  
*/
struct RefLabel {
	OSType 							creator;					/* creator of the object.  should be the same as the application signature.*/
	OSType 							id;							/* uniquely identifies the object within a scope defined by semantics of the object*/
};
typedef struct RefLabel RefLabel;

#endif
#define kInvalidID	 0

enum {
	kNilOptions					= 0
};

extern pascal void Debugger(void )
 ONEWORDINLINE(0xA9FF);

extern pascal void DebugStr(ConstStr255Param debuggerMsg)
 ONEWORDINLINE(0xABFF);

extern pascal void Debugger68k(void )
 ONEWORDINLINE(0xA9FF);

extern pascal void DebugStr68k(ConstStr255Param debuggerMsg)
 ONEWORDINLINE(0xABFF);

#if CGLUESUPPORTED
extern void debugstr(const char *debuggerMsg);

#endif
extern pascal void SysBreak(void )
 THREEWORDINLINE(0x303C, 0xFE16, 0xA9C9);

extern pascal void SysBreakStr(ConstStr255Param debuggerMsg)
 THREEWORDINLINE(0x303C, 0xFE15, 0xA9C9);

extern pascal void SysBreakFunc(ConstStr255Param debuggerMsg)
 THREEWORDINLINE(0x303C, 0xFE14, 0xA9C9);

/*
	Who implements what debugger functions:
	
	Name			MacsBug			Macintosh Debugger					Copland Debugger
	----------		-----------		-----------------------------		-------------------
	Debugger		yes				InterfaceLib maps to DebugStr		yes
	DebugStr		yes				yes									yes
	Debugger68k		yes				InterfaceLib maps to DebugStr		?
	DebugStr68k		yes				InterfaceLib maps to DebugStr		?
	debugstr		yes				InterfaceLib maps to DebugStr		yes
	SysBreak		no				InterfaceLib maps to SysError		obsolete?
	SysBreakStr		no				InterfaceLib maps to SysError		obsolete?
	SysBreakFunc	no				InterfaceLib maps to SysError		obsolete?
	LLDebugger		no				yes									Low Level Nub
	LLDebugStr		no				yes									Low Level Nub
*/
extern pascal void LLDebugger(void );

extern pascal void LLDebugStr(ConstStr255Param debuggerMsg);


#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TYPES__ */

