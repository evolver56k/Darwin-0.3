/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		DFALowMem.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn

	Change History (most recent first):

	  <HFS5>	 10/6/97	DSH		Add more LowMem definitions
	  <HFS4>	  9/5/97	DSH		Added LMGetHFSFlags
	  <HFS3>	  9/4/97	msd		Add prototype for AccessDFALowMem.
	  <HFS2>	 4/11/97	DSH		Added LMGetPMSPPtr
*/





#ifndef __DFALOWMEM__
#define __DFALOWMEM__

#if TARGET_OS_MAC
#ifndef __TYPES__
#include <Types.h>
#endif
#ifndef __CONTROLS__
#include <Controls.h>
#endif
#ifndef __EVENTS__
#include <Events.h>
#endif
#ifndef __FILES__
#include <Files.h>
#endif
#ifndef __FONTS__
#include <Fonts.h>
#endif
#ifndef __MEMORY__
#include <Memory.h>
#endif
#ifndef __MENUS__
#include <Menus.h>
#endif
#ifndef __OSUTILS__
#include <OSUtils.h>
#endif
#ifndef __QUICKDRAW__
#include <Quickdraw.h>
#endif
#ifndef __RESOURCES__
#include <Resources.h>
#endif
#ifndef __WINDOWS__
#include <Windows.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */
#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED



UInt32	GetDFALowMem( SInt16 whichLowMem );
void	SetDFALowMem( SInt16 whichLowMem, UInt32 value );
void	UseDFALowMems( Boolean useDFALowMems );
Boolean	AccessDFALowMem( void );


//	These are OK to leave
#define LMGetFSQHdr()	( (QHdrPtr) 0x0360)
#define LMGetApplScratch()	((Ptr) 0x0A78)
#define LMSetApplScratch(applScratchValue)	(BlockMoveData((Ptr) (applScratchValue), (Ptr) 0x0A78, 12))

#if TARGET_OS_MAC
extern pascal THz LMGetSysZone(void)
 TWOWORDINLINE(0x2EB8, 0x02A6);

extern pascal void LMSetTheZone(THz value)
 TWOWORDINLINE(0x21DF, 0x0118);

extern pascal THz LMGetApplZone(void)
 TWOWORDINLINE(0x2EB8, 0x02AA);

extern pascal Ptr LMGetPMSPPtr(void)
 TWOWORDINLINE(0x2EB8, 0x0386);

extern pascal Ptr LMGetROMBase(void)
 TWOWORDINLINE(0x2EB8, 0x02AE);

extern pascal SInt16 LMGetBootDrive(void)
 TWOWORDINLINE(0x3EB8, 0x0210);

//	DFA Replacement Get-Seters
typedef unsigned long *QHdrPtr ;
#endif

QHdrPtr	LMGetVCBQHdr( void );
void	LMSetVCBQHdr( QHdrPtr value );

Ptr		LMGetFSMVars( void );
void	LMSetFSMVars( Ptr value );

Ptr		LMGetFCBSPtr( void );
void	LMSetFCBSPtr( Ptr value );

Ptr		LMGetFCBSPtr( void );
void	LMSetFCBSPtr( Ptr value );

UInt8	LMGetHFSFlags( void );
void	LMSetHFSFlags( UInt8 value );

QHdrPtr	LMGetDrvQHdr( void );
void	LMSetDrvQHdr( QHdrPtr value );




SInt16	LMGetFSFCBLen( void );
void	LMSetFSFCBLen( SInt16 value );
#define LMGetFSFCBLen( )		( (SInt16) 	GetDFALowMem( kLMFSFCBLen ) )
#define LMSetFSFCBLen( value )	( (void) 	SetDFALowMem( kLMFSFCBLen, (UInt32) value ) )

Ptr		LMGetDefVCBPtr( void );
void	LMSetDefVCBPtr( Ptr value );
#define LMGetDefVCBPtr( )		( (Ptr)		GetDFALowMem( kLMDefVCBPtr ) )
#define LMSetDefVCBPtr( value )	( (void)	SetDFALowMem( kLMDefVCBPtr, (UInt32) value ) )

SInt16	LMGetNewMount( void );
void	LMSetNewMount( SInt16 value );
#define LMGetNewMount( )		( (SInt16)	GetDFALowMem( kLMNewMount ) )
#define LMSetNewMount( value )	( (void)	SetDFALowMem( kLMNewMount, (UInt32) value ) )

UInt8	LMGetFlushOnly( void );
void	LMSetFlushOnly( UInt8 value );
#define LMGetFlushOnly( )		( (UInt8)	GetDFALowMem( kLMFlushOnly ) )
#define LMSetFlushOnly( value )	( (void)	SetDFALowMem( kLMFlushOnly, (UInt32) value ) )

Ptr		LMGetWDCBsPtr( void );
void	LMSetWDCBsPtr( Ptr value );
#define LMGetWDCBsPtr( )		( (Ptr)		GetDFALowMem( kLMWDCBsPtr ) )
#define LMSetWDCBsPtr( value )	( (void)	SetDFALowMem( kLMWDCBsPtr, (UInt32) value ) )

Ptr		LMGetReqstVol( void );
void	LMSetReqstVol( Ptr value );
#define LMGetReqstVol( )		( (Ptr)		GetDFALowMem( kLMSReqstVol ) )
#define LMSetReqstVol( value )	( (void)	SetDFALowMem( kLMSReqstVol, (UInt32) value ) )

Ptr		LMGetSysVolCPtr( void );
void	LMSetSysVolCPtr( Ptr value );
#define LMGetSysVolCPtr( )		( (Ptr)		GetDFALowMem( kLMSysVolCPtr ) )
#define LMSetSysVolCPtr( value )( (void)	SetDFALowMem( kLMSysVolCPtr, (UInt32) value ) )

SInt16	LMGetDefVRefNum( void );
void	LMSetDefVRefNum( SInt16 value );
#define LMGetDefVRefNum( )		( (SInt16)	GetDFALowMem( kLMDefVRefNum ) )
#define LMSetDefVRefNum( value )( (void)	SetDFALowMem( kLMDefVRefNum, (UInt32) value ) )


	
	
//	Private

//	These are OK to leave
#if TARGET_OS_MAC
extern pascal Ptr LMGetHFSStkTop(void)
 TWOWORDINLINE(0x2EB8, 0x036A);
#endif



#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DFALOWMEM__ */

