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
	File:		DummyUnicodeStubs.c

	Contains:	Stubs to satisfy linker in absence of static-linked Unicode support.

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Roger Pantos

		Other Contact:		Don Brady

		Technology:			Fakes'R'Us

	Writers:

		(RNP)	Roger Pantos

	Change History (most recent first):

	   <CS1>	 9/24/97	RNP		first checked in
*/



#if TARGET_OS_MAC
#include <Unicode.h>
#include <TextCommon.h>
#include <Errors.h>
#else
#include "../headers/system/MacOSStubs.h"
#endif 	/* TARGET_OS_MAC */

pascal OSStatus	InitializeUnicode(StringPtr TECFileName)
{
#pragma unused( TECFileName)

	return notInitErr;
}


pascal OSStatus CreateTextToUnicodeInfo( ConstUnicodeMappingPtr iUnicodeMapping, TextToUnicodeInfo *oTextToUnicodeInfo )
{
#pragma unused( iUnicodeMapping)
#pragma unused( oTextToUnicodeInfo)

	return notInitErr;
}


pascal OSStatus CreateUnicodeToTextInfo( ConstUnicodeMappingPtr iUnicodeMapping, UnicodeToTextInfo *oUnicodeToTextInfo )
{
#pragma unused( iUnicodeMapping)
#pragma unused( oUnicodeToTextInfo)

	return notInitErr;
}


pascal OSStatus ConvertFromTextToUnicode( TextToUnicodeInfo iTextToUnicodeInfo, ByteCount iSourceLen,
	ConstLogicalAddress iSourceStr, OptionBits iControlFlags, ItemCount iOffsetCount,  ByteOffset iOffsetArray[],
	ItemCount *oOffsetCount,  ByteOffset oOffsetArray[], ByteCount iOutputBufLen, ByteCount *oSourceRead,
	ByteCount *oUnicodeLen, UniCharArrayPtr oUnicodeStr )
{
#pragma unused( iTextToUnicodeInfo)
#pragma unused( iSourceLen)
#pragma unused( iSourceStr)
#pragma unused( iControlFlags)
#pragma unused( iOffsetCount)
#pragma unused( iOffsetArray)
#pragma unused( oOffsetCount)
#pragma unused( oOffsetArray)
#pragma unused( iOutputBufLen)
#pragma unused( oSourceRead)
#pragma unused( oUnicodeLen)
#pragma unused( oUnicodeStr)

	return notInitErr;
}


pascal OSStatus ConvertFromUnicodeToText( UnicodeToTextInfo iUnicodeToTextInfo, ByteCount iUnicodeLen,
	ConstUniCharArrayPtr iUnicodeStr, OptionBits iControlFlags, ItemCount iOffsetCount,  ByteOffset iOffsetArray[],
	ItemCount *oOffsetCount,  ByteOffset oOffsetArray[], ByteCount iOutputBufLen, ByteCount *oInputRead,
	ByteCount *oOutputLen, LogicalAddress oOutputStr )
{
#pragma unused( iUnicodeToTextInfo)
#pragma unused( iUnicodeLen)
#pragma unused( iUnicodeStr)
#pragma unused( iControlFlags)
#pragma unused( iOffsetCount)
#pragma unused( iOffsetArray)
#pragma unused( oOffsetCount)
#pragma unused( oOffsetArray)
#pragma unused( iOutputBufLen)
#pragma unused( oInputRead)
#pragma unused( oOutputLen)
#pragma unused( oOutputStr)

	return notInitErr;
}


pascal OSStatus UpgradeScriptInfoToTextEncoding( ScriptCode iTextScriptID, LangCode iTextLanguageID,
	RegionCode iRegionID, ConstStr255Param iTextFontname, TextEncoding *oEncoding)
{
#pragma unused( iTextScriptID)
#pragma unused( iTextLanguageID)
#pragma unused( iRegionID)
#pragma unused( iTextFontname)
#pragma unused( oEncoding)

	return notInitErr;
}


OSStatus		LockMappingTable( void)
{
	return notInitErr;
}

