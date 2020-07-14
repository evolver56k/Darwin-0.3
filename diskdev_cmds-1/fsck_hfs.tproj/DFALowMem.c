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
	File:		DFALowMem.c

	Contains:	Functions to either access "Real" LowMem values, or access substitute
				values located in an array referenced off an ApplScratch location.

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(DSH)	Deric Horn

	Change History (most recent first):

	  <HFS5>	10/30/97	DSH		UseDFALowMems is now a bit in ApplScratch
	  <HFS4>	  9/5/97	DSH		Added LMGetHFSFlags
	  <HFS3>	 8/18/97	DSH		LGGetSetDrvQHdr always access real LowMem
		 <HFS2>	 3/27/97	DSH		Cleaning out DebugStrs
		 <HFS1>	 3/19/97	DSH		Initial Check-In
*/

#include	"ScavDefs.h"
#include	"Prototypes.h"

Boolean	AccessDFALowMem( void )
{
	UInt32	flags	= GetApplScratchValue( kDFAFlagsIndex );
	return( flags & kUseDFALowMemsMask );
}

void	UseDFALowMems( Boolean useDFALowMems )
{
	UInt32	flags;

	flags = (UInt32) GetApplScratchValue( kDFAFlagsIndex );
	
	if ( useDFALowMems == true )
		flags	|= kUseDFALowMemsMask;
	else
		flags	&= ~kUseDFALowMemsMask;
	
	SetApplScratchValue( kDFAFlagsIndex, (UInt32) flags );
}





QHdrPtr	LMGetVCBQHdr( void )
{
	if ( AccessDFALowMem() == true )
		return( (QHdrPtr) GetDFALowMem( kLMVCBQHdr ) );
	else
		return( (QHdrPtr) 0x0356 );	
}

void	LMSetVCBQHdr( QHdrPtr value )
{
	if ( AccessDFALowMem() == true )
		SetDFALowMem( kLMVCBQHdr, (UInt32) value );
	else
		(* (QHdrPtr) 0x0356) = *(QHdrPtr)(value);	
}



QHdrPtr	LMGetDrvQHdr( void )
{
//	if ( AccessDFALowMem() == true )
//		return( (QHdrPtr) GetDFALowMem( kLMDrvQHdr ) );
//	else
		return( (QHdrPtr) 0x0308 );	
}

void	LMSetDrvQHdr( QHdrPtr value )
{
//	if ( AccessDFALowMem() == true )
//		SetDFALowMem( kLMDrvQHdr, (UInt32) value );
//	else
		(* (QHdrPtr) 0x0308) = *(QHdrPtr)(value);	
}



Ptr		LMGetFSMVars( void )
{
	if ( AccessDFALowMem() == true )
		return( (Ptr) GetDFALowMem( kLMFSMVars ) );
	else
		return( (Ptr) *( (UInt32 *) 0x0BB8 ) );	
}

void	LMSetFSMVars( Ptr value )
{
	if ( AccessDFALowMem() == true )
		SetDFALowMem( kLMFSMVars, (UInt32) value );
	else
	{
		(* (UInt32 *) 0x0BB8) = (UInt32)(value);
	}
}


Ptr		LMGetFCBSPtr( void )
{
	if ( AccessDFALowMem() == true )
		return( (Ptr) GetDFALowMem( kLMFCBSPtr ) );
	else
		return( (Ptr) *( (UInt32 *) 0x034E ) );	
}

void	LMSetFCBSPtr( Ptr value )
{
	if ( AccessDFALowMem() == true )
		SetDFALowMem( kLMFCBSPtr, (UInt32) value );
	else
	{
		(* (UInt32 *) 0x034E) = (UInt32)(value);
	}
}


UInt8	LMGetHFSFlags( void )
{
	if ( AccessDFALowMem() == true )
		return( (UInt8) GetDFALowMem( kLMHFSFlags ) );
	else
		return( (UInt8) *( (UInt8 *) 0x0376 ) );	
}

void	LMSetHFSFlags( UInt8 value )
{
	if ( AccessDFALowMem() == true )
		SetDFALowMem( kLMHFSFlags, (UInt32) value );
	else
	{
		(* (UInt8 *) 0x0376) = (UInt8)(value);
	}
}


