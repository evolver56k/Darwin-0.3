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
	File:		HFSUtilities.c

	Contains:	xxx put contents here xxx

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contacts:		Mark Day, Deric Horn

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	  <Rhap>	 3/31/98	djb		Sync up with final HFSVolumes.h header file.
	  <CS13>	  9/9/97	msd		Make PascalBinaryCompare and UnicodeBinaryCompare faster.
	  <CS12>	  9/7/97	djb		Conditionalize DebugStrs to DEBUG_BUILD.
	  <CS11>	  9/4/97	msd		Add U64SetU routine.
	  <CS10>	 8/18/97	DSH		Conditionalizedout Math64 routines already compiled for DFA.
	   <CS9>	 7/22/97	msd		LocalToUTC and UTCToLocal are now functions (not macros) so that
									they can pin their outputs.
	   <CS8>	 7/21/97	djb		Add U64Add and U64Subtract (used by instrumentation).
	   <CS7>	 7/18/97	msd		Include LowMemPriv.h.
	   <CS6>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS5>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS4>	  6/9/97	msd		Add GetTimeUTC and GetTimeLocal.
	   <CS3>	 5/23/97	djb		Fixing ClearMemory bug - it was clearing an extra byte!
	   <CS2>	 5/16/97	msd		Include FilesInternal.h
	   <CS1>	 4/24/97	djb		first checked in
	  <HFS1>	 3/31/97	djb		first checked in
*/

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
	#include	<Types.h>
	#include	<LowMemPriv.h>
        #else
                #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */


#include "../headers/HFSVolumes.h"
#include "../headers/FileMgrInternal.h"


/*-------------------------------------------------------------------------------
Routine:	ClearMemory	-	clear a block of memory

-------------------------------------------------------------------------------*/

void ClearMemory( void* start, UInt32 length )
{
	UInt32		zero = 0;
	UInt32*		dataPtr;
	UInt8*		bytePtr;
	UInt32		fragCount;		// serves as both a length and quadlong count
								// for the beginning and main fragment
	
	if ( length == 0 )
		return;

	// is request less than 4 bytes?
	if ( length < 4 )				// length = 1,2 or 3
	{
		bytePtr = (UInt8 *) start;
		
		do
		{
			*bytePtr++ = zero;		// clear one byte at a time
		}
		while ( --length );

		return;
	}

	// are we aligned on an odd boundry?
	fragCount = (UInt32) start & 3;

	if ( fragCount )				// fragCount = 1,2 or 3
	{
		bytePtr = (UInt8 *) start;
		
		do
		{
			*bytePtr++ = zero;		// clear one byte at a time
			++fragCount;
			--length;
		}
		while ( (fragCount < 4) && (length > 0) );

		if ( length == 0 )
			return;

		dataPtr = (UInt32*) (((UInt32) start & 0xFFFFFFFC) + 4);	// make it long word aligned
	}
	else
	{
		dataPtr = (UInt32*) ((UInt32) start & 0xFFFFFFFC);			// make it long word aligned
	}

	// At this point dataPtr is long aligned

	// are there odd bytes to copy?
	fragCount = length & 3;
	
	if ( fragCount )
	{
		bytePtr = (UInt8 *) ((UInt32) dataPtr + (UInt32) length - 1);	// point to last byte
		
		length -= fragCount;		// adjust remaining length
		
		do
		{
			*bytePtr-- = zero;		// clear one byte at a time
		}
		while ( --fragCount );

		if ( length == 0 )
			return;
	}

	// At this point length is a multiple of 4

	#if DEBUG_BUILD
	  if ( length < 4 )
		 DebugStr("\p ClearMemory: length < 4");
	#endif

	// fix up beginning to get us on a 64 byte boundary
	fragCount = length & (64-1);
	
	#if DEBUG_BUILD
	  if ( fragCount < 4 && fragCount > 0 )
		  DebugStr("\p ClearMemory: fragCount < 4");
	#endif
	
	if ( fragCount )
	{
		length -= fragCount; 		// subtract fragment from length now
		fragCount >>= 2; 			// divide by 4 to get a count, for DBRA loop
		do
		{
			// clear 4 bytes at a time...
			*dataPtr++ = zero;		
		}
		while (--fragCount);
	}

	// Are we finished yet?
	if ( length == 0 )
		return;
	
	// Time to turn on the fire hose
	length >>= 6;		// divide by 64 to get count
	do
	{
		// spray 64 bytes at a time...
		*dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero;
		*dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero;
		*dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero;
		*dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero; *dataPtr++ = zero;
	}
	while (--length);
}


//_______________________________________________________________________

Boolean UnicodeBinaryCompare( ConstHFSUniStr255Param ustr1, ConstHFSUniStr255Param ustr2 )
{
	UInt16			len;
	const UniChar	*u1;
	const UniChar	*u2;

	len = ustr1->length;

	if ( len != ustr2->length )
		return false;
		
	u1 = ustr1->unicode;
	u2 = ustr2->unicode;

	++len;							//	adjust for pre-decrement in loop
	
	//	Keep going until we run out of characters, or find one that differs
	while (--len && *(u1++) == *(u2++))
		;

	//	Return true iff we ran out of characters
	return len==0;
}

//_______________________________________________________________________

Boolean	PascalBinaryCompare( ConstStr31Param pstr1, ConstStr31Param pstr2 )
{
	unsigned	length;
	
	length = *pstr1;
	if (*(pstr1++) != *(pstr2++))		//	lengths must match
		return false;
	
	++length;							//	adjust for pre-decrement in loop
	
	//	Keep going until we run out of characters, or find one that differs
	while (--length && *(pstr1++) == *(pstr2++))
		;

	//	Return true iff we ran out of characters
	return length==0;
}

#if TARGET_OS_MAC

#if (FORDISKFIRSTAID)

UInt32 LocalToUTC(UInt32 localTime)
{
	return localTime;
}
UInt32 UTCToLocal(UInt32 utcTime)
{
	return utcTime;
}

#else	/* not for Disk First Aid */

UInt32 LocalToUTC(register UInt32 localTime)
{
	register UInt32	utc;
	
	//
	//	An input of zero means "never".  In that case, don't adjust it.
	//
	if (localTime == 0)
		return 0;

	//
	//	Compute Universal Time
	//
        utc = localTime - ((FSVarsRec*) LMGetFSMVars())->offsetToUTC;
	
	//
	//	If we wrapped around, then use the most extreme
	//	value closest to the localTime input.
	//
	if ((utc ^ localTime) & 0xFF000000 == 0xFF000000) {
		if (localTime & 0xFF000000)
			utc = 0xFFFFFFFF;		// pin to maximum value
		else
			utc = 0;				// pin to minimum value
	}
	
	return utc;
}


UInt32 UTCToLocal(register UInt32 utcTime)
{
	register UInt32	local;

	//
	//	An input of zero means "never".  In that case, don't adjust it.
	//
	if (utcTime == 0)
		return 0;

	//
	//	Compute Universal Time
	//
	local = utcTime + ((FSVarsRec *) LMGetFSMVars())->offsetToUTC;
	
	//
	//	If we wrapped around, then use the most extreme
	//	value closest to the utcTime input.
	//
	if ((local ^ utcTime) & 0xFF000000 == 0xFF000000) {
		if (utcTime & 0xFF000000)
			local = 0xFFFFFFFF;		// pin to maximum value
		else
			local = 0;				// pin to minimum value
	}
	
	return local;
}
#endif
#endif	/* TARGE_OS_MAC */


#if TARGET_OS_MAC
UInt32 GetTimeUTC(void)
{
	UInt32 localTime;
	
	GetDateTime(&localTime);
	return LocalToUTC(localTime);
}


UInt32 GetTimeLocal(void)
{
	UInt32 localTime;
	
	GetDateTime(&localTime);
	return localTime;
}
#endif	/* TARGET_OS_MAC */


#ifndef __MATH64__

UInt64 U64Add (UInt64 x, UInt64 y)
{
	UInt64	result;
	
	result.lo = x.lo + y.lo;
	result.hi = x.hi + y.hi;
	
	//	Now see if there was a carry out of the low half.  If there was a carry
	//	out, then the unsigned interpretation of the result will be less than
	//	the unsigned interpretation of both addends.
	
	if (((UInt32) result.lo) < ((UInt32) x.lo))
		++result.hi;							//	was carry; add it to upper half
	
	return(result);								//	pass the answer back
}


UInt64 U64Subtract (UInt64 left, UInt64 right)
{
	UInt64	result;
	
	result.lo = left.lo - right.lo;
	result.hi = left.hi - right.hi;
	
	//	Now see if there was a borrow from the low half.  There will be one if
	//	the unsigned interpretation of right.lo is greater than the unsigned
	//	interpretation of left.lo.
	
	if (((UInt32) right.lo) > ((UInt32) left.lo))
		--result.hi;							//	was borrow; subtract from upper half

	return(result);								//	pass the answer back
}

#endif
