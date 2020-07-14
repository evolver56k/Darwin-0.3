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
	File:		UnicodeWrappers.c

	Contains:	Wrapper routines for Unicode conversion and comparison.

	Version:	HFS Plus 1.0

	Written by:	Mark Day

	Copyright:	© 1996-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		Don Brady

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	  <CS38>	12/10/97	djb		Radar #2005461, don't use fallback chars when converting to
									Unicode, instead let the client (Catalog) retry with MacRoman.
	  <CS37>	 12/2/97	DSH		Conditionalize out some unicode related routines for DFA
	  <CS36>	11/26/97	djb		Radar #2005461,2005688 don't swallow kTECPartialCharErr errors!
	  <CS35>	11/17/97	djb		Name mangling was broken with decomposed Unicode.
	  <CS34>	11/16/97	djb		Radar #2001928 - use kUnicodeCanonicalDecompVariant variant.
	  <CS33>	11/11/97	DSH		Use Get_gLowerCaseTable for DiskFirstAid builds to avoid loading
									in a branch to the table.
	  <CS32>	 11/7/97	msd		Replace FastSimpleCompareStrings with FastUnicodeCompare (which
									handles ignorable Unicode characters). Remove the wrapper
									routine, CompareUnicodeNames, and have its callers call
									FastUnicodeCompare directly.
	  <CS31>	10/17/97	djb		Change kUnicodeUseHFSPlusMapping to kUnicodeUseLatestMapping.
	  <CS30>	10/17/97	msd		Fix some type casts for char pointers.
	  <CS29>	10/13/97	djb		Add new SPIs for Finder View font (radar #1679073).
	  <CS28>	 10/1/97	djb		Preserve current heap zone in InitializeEncodingContext routine
									(radar #1682686).
	  <CS27>	 9/17/97	djb		Handle kTECPartialCharErr errors in ConvertHFSNameToUnicode.
	  <CS26>	 9/16/97	msd		In MockConvertFromPStringToUnicode, use pragma unused instead of
									commenting out unused parameter (so SC will compile it).
	  <CS25>	 9/15/97	djb		Fix MockConverters to do either 7-bit ascii or else mangle the
									name (radar #1672388). Use 'p2u#' resource for bootstrapping
									Unicode. Make sure InitializeEncodingContext uses System heap.
	  <CS24>	 9/10/97	msd		Make InitializeEncodingContext public.
	  <CS23>	  9/7/97	djb		Handle 'Å' char in BasicLatinUnicode converter.
	  <CS22>	  9/4/97	djb		Add logging to BasicLatinUnicodeToPascal.
	  <CS21>	 8/26/97	djb		Make FastSimpleCompareStrings faster. Add
									BasicLatinUnicodeToPascal to make 7-bit ascii conversions
									faster.
	  <CS20>	 8/14/97	djb		Add FastRelString here (to be next to the data tables).
	  <CS19>	 7/21/97	djb		LogEndTime now takes an error code.
	  <CS18>	 7/18/97	msd		Include LowMemPriv.h, Gestalt.h, TextUtils.h.
	  <CS17>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	  <CS16>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	  <CS15>	  7/8/97	DSH		InitializeUnicode changed its API
	  <CS14>	  7/1/97	DSH		SC, DFA complier, requires parameters in functions. #pragma'd
									them out to eliminate C warnings.
	  <CS13>	 6/30/97	msd		Remove unused parameter warnings in FallbackProc by commenting
									out unused parameter names.
	  <CS12>	 6/26/97	DSH		FallbackProc declare variables before useage for SC,
									MockConverters no longer static for DFA.
	  <CS11>	 6/25/97	msd		In function InitStaticUnicodeConverter, the variable fsVars was
									being used before being initialized.
	  <CS10>	 6/24/97	DSH		Runtime checks to call through CFM or static linked routines.
	   <CS9>	 6/20/97	msd		Re-introduce fix from <CS7>. Fix another missing cast. Remove a
									spurious semicolon.
	   <CS8>	 6/18/97	djb		Add more ConversionContexts routines. Improved file mangling.
	   <CS7>	 6/16/97	msd		Add a missing cast in GetFileIDString.
	   <CS6>	 6/13/97	djb		Added support for long filenames. Switched to
									ConvertUnicodeToHFSName, ConvertHFSNameToUnicode, and
									CompareUnicodeNames.
	   <CS5>	  6/4/97	djb		Use system script instead of macRoman.
	   <CS4>	 5/19/97	djb		Add call to LockMappingTable so tables won't move!
	   <CS3>	  5/9/97	djb		Include HFSInstrumentation.h
	   <CS2>	  5/7/97	djb		Add summary traces. Add FastSimpleCompareStrings routine.
	   <CS1>	 4/24/97	djb		first checked in
	  <HFS5>	 3/27/97	djb		Add calls to real Unicode conversion routines.
	  <HFS4>	  2/6/97	msd		Add conditional code to use real Unicode comparison routines
									(default to off).
	  <HFS3>	  1/6/97	djb		Fix HFSUnicodeCompare - the final comparison of length1 and
									length2 was backwards.
	  <HFS2>	12/12/96	msd		Use precompiled headers.
	  <HFS1>	12/12/96	msd		first checked in

*/


#if TARGET_OS_MAC
#if ( PRAGMA_LOAD_SUPPORTED )
	#pragma	load	PrecompiledHeaders
#else
#include <Memory.h>
#include <CodeFragments.h>
#include <Errors.h>
#include <MixedModePriv.h>
#include <Script.h>
#include <UnicodeConverter.h>
#include <LowMemPriv.h>
#include <Gestalt.h>
#include <TextUtils.h>
#include <IntlResources.h>
#include <TextCommonPriv.h>
#include <UnicodeConverterPriv.h>
#include <FileMgrResources.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

#include "FileMgrInternal.h"
#include "HFSUnicodeWrappers.h"
#include "HFSInstrumentation.h"

#ifdef INVESTIGATE

enum {
	uupCreateTextToUnicodeInfoProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ConstUnicodeMappingPtr)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(TextToUnicodeInfo*))),


	uppCreateUnicodeToTextInfoProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ConstUnicodeMappingPtr)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(UnicodeToTextInfo*))),


	uppConvertFromTextToUnicodeProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1,  SIZE_CODE(sizeof(TextToUnicodeInfo)))
		| STACK_ROUTINE_PARAMETER(2,  SIZE_CODE(sizeof(ByteCount)))
		| STACK_ROUTINE_PARAMETER(3,  SIZE_CODE(sizeof(ConstLogicalAddress)))
		| STACK_ROUTINE_PARAMETER(4,  SIZE_CODE(sizeof(OptionBits)))
		| STACK_ROUTINE_PARAMETER(5,  SIZE_CODE(sizeof(ItemCount)))
		| STACK_ROUTINE_PARAMETER(6,  SIZE_CODE(sizeof(ByteOffset*)))
		| STACK_ROUTINE_PARAMETER(7,  SIZE_CODE(sizeof(ItemCount*)))
		| STACK_ROUTINE_PARAMETER(8,  SIZE_CODE(sizeof(ByteOffset*)))
		| STACK_ROUTINE_PARAMETER(9,  SIZE_CODE(sizeof(ByteCount)))
		| STACK_ROUTINE_PARAMETER(10, SIZE_CODE(sizeof(ByteCount*)))
		| STACK_ROUTINE_PARAMETER(11, SIZE_CODE(sizeof(ByteCount*)))
		| STACK_ROUTINE_PARAMETER(12, SIZE_CODE(sizeof(UniCharArrayPtr))),

	 
	uppConvertFromUnicodeToTextProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(UnicodeToTextInfo)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(ByteCount)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(ConstUniCharArrayPtr)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(OptionBits)))
		| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(ItemCount)))
		| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(ByteOffset*)))
		| STACK_ROUTINE_PARAMETER(7, SIZE_CODE(sizeof(ItemCount*)))
		| STACK_ROUTINE_PARAMETER(8, SIZE_CODE(sizeof(ByteOffset*)))
		| STACK_ROUTINE_PARAMETER(9, SIZE_CODE(sizeof(ByteCount)))
		| STACK_ROUTINE_PARAMETER(10, SIZE_CODE(sizeof(ByteCount*)))
		| STACK_ROUTINE_PARAMETER(11, SIZE_CODE(sizeof(ByteCount*)))
		| STACK_ROUTINE_PARAMETER(12, SIZE_CODE(sizeof(LogicalAddress))),


	uppUpgradeScriptInfoToTextEncodingProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(ScriptCode)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(LangCode)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(RegionCode)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(ConstStr255Param)))
		| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(TextEncoding*))),


	uppRevertTextEncodingToScriptInfoProcInfo =
		kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(TextEncoding)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(ScriptCode*)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(LangCode*)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(StringPtr))),


// NOTE: this one uses "C" calling conventions...
	uppLockMappingTableProcInfo =
		kCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(UnicodeMapping*)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Boolean)))
};


enum {
	smLargestScript = 32,
	
	kMinFileExtensionChars = 1,		// does not include dot
	kMaxFileExtensionChars = 5		// does not include dot
};


#define	kUnicodeTradeMarkChar		(UniChar) 0x2122

#define	kASCIITradeMarkChar			0xAA

#define Is7BitASCII(c)				( (c) >= 0x20 && (c) <= 0x7F )

#define	IsSpecialASCIIChar(c)		( (c) == (UInt8) 'µ' || (c) == (UInt8) 'º' || (c) == (UInt8) 'ê' )

// Note:	'µ' has two Unicode representations 0x00B5 (micro sign) and 0x03BC (greek)
//			'ê' has two Unicode representations 0x2206 (increment) and 0x0394 (greek)
#define	IsSpecialUnicodeChar(c)		( (c) == 0x00B5 || (c) == 0x03BC || (c) == 0x03C0 || (c) == 0x2206 || (c) == 0x0394 )

#define IsHexDigit(c)				( ((c) >= (UInt8) '0' && (c) <= (UInt8) '9') || ((c) >= (UInt8) 'A' && (c) <= (UInt8) 'F') )

//
// PToUTable and PToUEntry describe the 'p2u#' resource
// This resource is used to map pascal to Unicode before
// the real Unicode converter is initialize.
//
struct PToUEntry {
	Str31		pascalString;			// pascal representation
	UInt16		unicodeChars;			// unicode char count
	UniChar		unicodeString[63];		// unicode representation
};
typedef struct PToUEntry PToUEntry;


struct PToUTable {
	UInt16		entries;		// number of entries
	PToUEntry	entry[1];
};
typedef struct PToUTable PToUTable;


extern UniChar *	Get_gLowerCaseTable(void);

//	Unicode Glue routines
// WARNING:  These Glue APIs must match the APIs in UnicodeConverter.h (the glue assumes they are the same)
// 			 for example CreateTextToUnicodeInfo_Glue and CreateTextToUnicodeInfo must have identical parameters and calling conventions

extern pascal OSStatus CreateTextToUnicodeInfo_Glue(ConstUnicodeMappingPtr iUnicodeMapping, TextToUnicodeInfo *oTextToUnicodeInfo);
extern pascal OSStatus CreateUnicodeToTextInfo_Glue(ConstUnicodeMappingPtr iUnicodeMapping, UnicodeToTextInfo *oUnicodeToTextInfo);
extern pascal OSStatus ConvertFromTextToUnicode_Glue(TextToUnicodeInfo iTextToUnicodeInfo, ByteCount iSourceLen, ConstLogicalAddress iSourceStr, OptionBits iControlFlags, ItemCount iOffsetCount, ByteOffset iOffsetArray[], ItemCount *oOffsetCount, ByteOffset oOffsetArray[], ByteCount iBufLen, ByteCount *oSourceRead, ByteCount *oUnicodeLen, UniCharArrayPtr oUnicodeStr);
extern pascal OSStatus ConvertFromUnicodeToText_Glue(UnicodeToTextInfo iUnicodeToTextInfo, ByteCount iUnicodeLen, ConstUniCharArrayPtr iUnicodeStr, OptionBits iControlFlags, ItemCount iOffsetCount, ByteOffset iOffsetArray[], ItemCount *oOffsetCount, ByteOffset oOffsetArray[], ByteCount iBufLen, ByteCount *oInputRead, ByteCount *oOutputLen, LogicalAddress oOutputStr);
extern pascal OSStatus UpgradeScriptInfoToTextEncoding_Glue(ScriptCode textScriptID, LangCode textLanguageID, RegionCode regionID, ConstStr255Param textFontname, TextEncoding *encoding);
extern pascal OSStatus RevertTextEncodingToScriptInfo_Glue(TextEncoding encoding, ScriptCode *textScriptID, LangCode *textLanguageID, Str255 textFontname);

// EXCEPTION:  this one uses "C" calling conventions (why?)
extern OSStatus LockMappingTable_Glue(UnicodeMapping *unicodeMappingPtr, Boolean lockIt);


static OSErr InitStaticUnicodeConverter(void);
static OSErr InitDynamicUnicodeConverter( Boolean forBootVolume );

SInt32 FastRelString( ConstStr255Param str1, ConstStr255Param str2 );

static OSErr InstallConversionContexts( FSVarsRec *fsVars );


static OSErr	InstallLibraryVector( CFragConnectionID connectionID, ConstStr255Param symbolName,
									  ProcInfoType procInfo, UniversalProcPtr *vector );


static OSErr	InstallSystemConversionContext( FSVarsRec *fsVars, Boolean forBootVolume );

static void	GetFilenameExtension( ItemCount length, ConstUniCharArrayPtr unicodeStr, Str15 extStr );

static void	GetFileIDString( CatalogNodeID fileID, Str15 fileIDStr );

static void AppendPascalString( ConstStr15Param src, Str31 dst );

static UInt32 HexStringToInteger( UInt32 length, const UInt8 *hexStr );


pascal OSStatus FallbackProc( UniChar *srcUniStr, ByteCount srcUniStrLen, ByteCount *srcConvLen,
							  TextPtr destStr, ByteCount destStrLen, ByteCount *destConvLen,
							  LogicalAddress contextPtr, ConstUnicodeMappingPtr unicodeMappingPtr );


static Boolean BasicLatinUnicodeToPascal( ItemCount unicodeChars, ConstUniCharArrayPtr unicodeString, Str31 pascalString );
static Boolean PascalToBasicLatinUnicode( ConstStr31Param pascalString, ItemCount *unicodeChars, UniCharArrayPtr unicodeString );



/*
	Get the base encoding used by the File System
	
	If no HFS Plus volumes have been mounted yet then
	the default encoding could be kTextEncodingUndefined
*/

TextEncoding
GetDefaultTextEncoding(void)
{
	FSVarsRec	*fsVars;	

	fsVars = (FSVarsRec*) LMGetFSMVars();

	return fsVars->gDefaultBaseEncoding;
}



/*
	Set the base encoding used by the File System
*/

OSErr
SetDefaultTextEncoding(TextEncoding encoding)
{
	FSVarsRec	*fsVars;
	OSErr		result;


	fsVars = (FSVarsRec*) LMGetFSMVars();


	// undefined only makes sense when Unicode Library is not installed
	if ( encoding == kTextEncodingUndefined )
	{
		if ( fsVars->gIsUnicodeInstalled == false )
			fsVars->gDefaultBaseEncoding = encoding;
			
		return noErr;
	}

 	encoding = GetTextEncodingBasePriv(encoding);
 
	if ( !ValidMacEncoding(encoding) )
		return paramErr;	// we only support Mac encodings!

	// if Unicode Library is installed then setup context for this encoding
	// otherwise it will occur when the first HFS Plus volume gets mounted
	if ( fsVars->gIsUnicodeInstalled )
	{
		result = InitializeEncodingContext( encoding, fsVars );
		if ( result != noErr )
			return result;
	}	

	// make it the default...
	fsVars->gDefaultBaseEncoding = encoding;

	return noErr;
}


/*
	Get the encoding that matches font
*/

OSErr
GetTextEncodingForFont( ConstStr255Param fontName, UInt32 * textEncoding )
{
	FSVarsRec *		fsVars;
	OSErr			result;


	fsVars = (FSVarsRec*) LMGetFSMVars();

	// if Unicode Library is installed then we can get the encoding...
	if ( fsVars->gIsUnicodeInstalled )
	{
		result = UpgradeScriptInfoToTextEncoding_Glue (	kTextScriptDontCare,
														kTextLanguageDontCare,
														kTextRegionDontCare,
														fontName,
														textEncoding );
	}
	else // Unicode Library not installed so save font name for later...
	{
		StringPtr	savedFontName;
		UInt16		stringByteSize;
		
		stringByteSize = fontName[0] + 1;
		savedFontName = fsVars->gTextEncodingFontName;

		// if we already had one then get rid of it
		if ( savedFontName != NULL )
			DisposePtr( (Ptr) savedFontName );

		savedFontName = (StringPtr) NewPtrSys( stringByteSize );
		
		if ( savedFontName != NULL )
			BlockMoveData(fontName, savedFontName, stringByteSize);
		
		fsVars->gTextEncodingFontName = savedFontName;
		
		*textEncoding = kTextEncodingUndefined;
		result = noErr;
	}

	return result;
}


/*
	Count the number of encodings installed by the File System
*/

ItemCount
CountInstalledEncodings(void)
{
	FSVarsRec	*fsVars;	

	fsVars = (FSVarsRec*) LMGetFSMVars();

	return fsVars->gInstalledEncodings;
}


/*
	Convert a Unicode string to a Pascal string (Str31) for use in an HFS file/folder name.
*/
#if ( ! FORDISKFIRSTAID )	
OSErr
ConvertUnicodeToHFSName( ConstUniStr255Param unicodeName, TextEncoding encoding, CatalogNodeID cnid, Str31 hfsName )
{
	FSVarsRec		*fsVars;	
	ByteCount		unicodeByteLength;
	ByteCount		actualPascalBytes;
	ByteCount		pascalSizeLimit;
	ByteCount		unicodeBytesConverted;
	UInt32			index;
	OSErr			result;


	hfsName[0] = 0;		// in case we get errors, make sure output is valid
	unicodeByteLength = unicodeName->length * sizeof(UniChar);

	if ( unicodeByteLength == 0 )
		return noErr;

	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( cnid == kHFSRootFolderID )
		pascalSizeLimit = kHFSMaxVolumeNameChars;	// an HFS volume name
	else
		pascalSizeLimit = kHFSMaxFileNameChars;		// an HFS file name

	encoding = GetTextEncodingBasePriv(encoding);
	index = MapEncodingToIndex(encoding);
	
	if ( fsVars->gIsUnicodeInstalled )
	{
		LogStartTime(kTraceUnicodeToPString);

		// Optimization
		//
		// For Basic Latin Unicode (7-bit ASCII) do the conversion ourself
		// since Unicode to text conversions are currently too slow...
		//
		if ( encoding == kTextEncodingMacRoman &&
			 BasicLatinUnicodeToPascal(unicodeName->length, unicodeName->unicode, hfsName) )
		{
			LogEndTime(kTraceUnicodeToPString, noErr);
			return noErr;
		}
	
		result = ConvertFromUnicodeToText_Glue(fsVars->gConversionContext[index].fromUnicode,
										  unicodeByteLength,
										  unicodeName->unicode,
										  kUnicodeUseFallbacksMask,
										  0, nil,	0, nil,					// offsetCounts & offsetArrays
										  pascalSizeLimit,
										  &unicodeBytesConverted,
										  &actualPascalBytes,
										  &hfsName[1]);	

		hfsName[0] = actualPascalBytes;	// fill out length byte

		LogEndTime(kTraceUnicodeToPString, 1 /*result*/);
	}
	else
	{
		result = MockConvertFromUnicodeToPString( unicodeByteLength, unicodeName->unicode, hfsName );
		actualPascalBytes = hfsName[0];
		unicodeBytesConverted = actualPascalBytes * sizeof(UniChar);
	}
	

	// Check if name was too long or some characters were unrepresentable...
	// if so we need to mangle the name so that the file can be found by
	// name later

	if ( result == kTECOutputBufferFullStatus || result == kTECUsedFallbacksStatus )
	{
		Str15		fileIDStr;				// file ID as a pascal string
		Str15		extStr;					// dot extension as a pascal string

		GetFileIDString(cnid, fileIDStr);
		
		if ( pascalSizeLimit == kHFSMaxFileNameChars )
		{
			GetFilenameExtension(unicodeName->length, unicodeName->unicode, extStr);
			
			// remove extension chars from source
			unicodeByteLength -= StrLength(extStr) * sizeof(UniChar);
		}
		else
		{
			extStr[0] = (UInt8) 0;	// volumes don't have extensions
		}
	
		// calculate free space for filename prefix
		pascalSizeLimit -= StrLength(extStr) + StrLength(fileIDStr);
		
		// now generate the prefix name...
		result = ConvertFromUnicodeToText_Glue(fsVars->gConversionContext[index].fromUnicode,
										  unicodeByteLength,
										  unicodeName->unicode,
										  kUnicodeUseFallbacksMask,
										  0, nil,	0, nil,					// offsetCounts & offsetArrays
										  pascalSizeLimit,
										  &unicodeBytesConverted,
										  &actualPascalBytes,
										  &hfsName[1]);	
		
		if ( result != noErr
			 && result != kTECOutputBufferFullStatus
			 && result != kTECUsedFallbacksStatus )
			 return result;

		hfsName[0] = actualPascalBytes;	// fill out length byte			
	
		AppendPascalString(fileIDStr, hfsName);
		AppendPascalString(extStr, hfsName);
		
		result = noErr;
	}

	return result;

} // end ConvertUnicodeToHFSName
#endif

/*
	Convert a Pascal string (Str31, such as a file/folder name) to Unicode.
*/
#if ( ! FORDISKFIRSTAID )	
OSErr
ConvertHFSNameToUnicode( ConstStr31Param hfsName, TextEncoding encoding, UniStr255 *unicodeName )
{
	FSVarsRec	*fsVars;	
	ByteCount	unicodeByteLength;
	OSErr		result;
	

	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( fsVars->gIsUnicodeInstalled )
	{
		ByteCount	pascalCharsRead;
		UInt32		index;
		
		encoding = GetTextEncodingBasePriv(encoding);
		index = MapEncodingToIndex(encoding);

		LogStartTime(kTracePStringToUnicode);
		result = ConvertFromTextToUnicode_Glue(fsVars->gConversionContext[index].toUnicode,
										  hfsName[0],
										  &hfsName[1],
										  0,								// no control flag bits
										  0, NULL, 0, NULL,					// offsetCounts & offsetArrays
										  sizeof(unicodeName->unicode),		// output buffer size in bytes
										  &pascalCharsRead,						  
										  &unicodeByteLength,
										  unicodeName->unicode );
		LogEndTime(kTracePStringToUnicode, result);
	}
	else
	{
		result = MockConvertFromPStringToUnicode( hfsName, sizeof(unicodeName->unicode), &unicodeByteLength, unicodeName->unicode );
	}
	
	unicodeName->length = unicodeByteLength / sizeof(UniChar);	// Note: from byte count to char count

	return result;

} // end ConvertHFSNameToUnicode
#endif



/*
	MockConvertFromUnicodeToPString

	Only used during boot to find Text Encoding Library
 */
OSErr
MockConvertFromUnicodeToPString( ByteCount unicodeLength, ConstUniCharArrayPtr unicodeStr, Str31 pascalStr )
{
	UInt32		unicodeChars;
	OSErr		result;


	result = noErr;		// be optimistic
	unicodeChars = unicodeLength / sizeof(UniChar);		// length is passed in bytes

	if ( unicodeChars > 31 )
	{
		unicodeChars = 31;						// only do the first 31 chars
		result = kTECOutputBufferFullStatus;
	}

	// first try to do basic ascii conversion
	//
	// if that doesn't work use "???" and return an error
	// (kTECUsedFallbacksStatus) so that the name will
	// get mangled (ie have a file ID appended).

	if ( !BasicLatinUnicodeToPascal(unicodeChars, unicodeStr, pascalStr) )
	{
		*(UInt32*) &pascalStr[0] = 0x033F3F3F;		// "???"
		result = kTECUsedFallbacksStatus;
	}
	
	return result;
}


/*
	MockConvertFromPStringToUnicode

	Only used during boot to find Text Encoding Library
 */
OSErr
MockConvertFromPStringToUnicode(ConstStr31Param pascalStr, ByteCount maxUnicodeLen, ByteCount *actualUnicodeLen, UniCharArrayPtr unicodeStr)
{
#pragma unused(maxUnicodeLen)

	UInt32	unicodeChars;


	// first try to do basic ascii conversion
	//
	// if that doesn't work try using our special boot mapping table...

	if ( !PascalToBasicLatinUnicode (pascalStr, &unicodeChars, unicodeStr) )
	{
		FSVarsRec	*fsVars;
		PToUTable	*table;
		PToUEntry	*entry;
		UInt16		i;

		
		unicodeChars = 1;
		unicodeStr[0] = 0xFFFF;		// the value $FFFF is guaranteed not to be a Unicode character

		// see if table p2u table exists
		fsVars = (FSVarsRec*) LMGetFSMVars();
		table = (PToUTable*) fsVars->gBootPToUTable;

		if ( table != NULL )
			for (i = 0; i < table->entries; ++i )
			{
				entry = &table->entry[i];

				if ( PascalBinaryCompare(pascalStr, entry->pascalString) )
				{
					unicodeChars = entry->unicodeChars;
	
					BlockMoveData(entry->unicodeString, unicodeStr, unicodeChars * sizeof(UniChar));
					break;
				}
			}
		
	}

	*actualUnicodeLen = unicodeChars * sizeof(UniChar);		// return length in bytes

	return noErr;
}


/*
	Initialize the Unicode Converter library
	
	If the library cannot be initialized the wrapper code will default to using 7-bit ASCII or mangled names.
	
	WARNING: This cannot be called from within a file system call (since it calls the file system)!
*/
OSErr InitUnicodeConverter(Boolean forBootVolume)
{
	FSVarsRec			*fsVars;	
	Handle				resourceHandle;
	long				response;
	OSErr				err;

	
	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( fsVars->gIsUnicodeInstalled == true )					//	Has Unicode already been installed
		return ( noErr );
	
	err = Gestalt( gestaltSysArchitecture, &response );			//ÄÄ Runtime check to load static or dynamic libraries
	if ( (response == gestaltPowerPC) && (err == noErr) )
	{
		SInt32	savedOffsetToUTC;

		if (forBootVolume)
		{
			savedOffsetToUTC = fsVars->offsetToUTC;		// save offset from GMT to local time
			fsVars->offsetToUTC = 0;					// trick CFM into caching an older mod date for the extensions folder

			// For bootstrap PascalToUnicode conversions we need to load the p2u table
			resourceHandle = GetResource ('p2u#', 0);	// get our special mapping table
			if (resourceHandle != NULL)
			{
				HLock(resourceHandle);
				fsVars->gBootPToUTable = *resourceHandle;
			}
		}


		err = InitDynamicUnicodeConverter( forBootVolume );


		if (forBootVolume)
		{
			fsVars->offsetToUTC = savedOffsetToUTC;		// restore offset from GMT to local time

			// The real converters should be online now so we can jettison the table
			if (resourceHandle != NULL)
			{
				HUnlock(resourceHandle);
				fsVars->gBootPToUTable = NULL;
				ReleaseResource(resourceHandle);			// we no longer need this table
			}
		}
	}
	
	if ( (err != noErr) || (response != gestaltPowerPC) )		//	If we got an error or on a 68K mac
	{
		err = InitStaticUnicodeConverter();						//	Init the 68K static version of the converters
	}

	
	return ( err );
}


/*
	Initialize the Staticly linked 68K Unicode Converter library

	If the library cannot be initialized the wrapper code will default to using 7-bit ASCII or mangled names.
	WARNING: This cannot be called from within a file system call (since it calls the file system)!
*/

static OSErr
InitStaticUnicodeConverter(void)
{
	FSVarsRec			*fsVars;	
	THz					savedHeapZone;
	OSErr				err;
	Str31				textEncodingConverterName;

	// use the system context
	savedHeapZone = GetZone();
	SetZone( SystemZone() );

	GetIndString( textEncodingConverterName, kBaseHFSPlusResourceID, rTextEncodingConverterName );	//ÄÄ†need real string!!
	err = InitializeUnicode( textEncodingConverterName );
	ExitOnError( err );

	fsVars = (FSVarsRec*) LMGetFSMVars();
	fsVars->gUseDynamicUnicodeConverters = false;

	err = InstallSystemConversionContext( fsVars, false );
	ExitOnError( err );
	
	fsVars = (FSVarsRec*) LMGetFSMVars();
	fsVars->gIsUnicodeInstalled = true;


ErrorExit:
	
	SetZone( savedHeapZone );
	
	return( err );
}



/*
	Initialize the PPC CFM dynamically linked 68K Unicode Converter library
	If the library cannot be initialized the wrapper code will default to
	using the 68K staticly linked library.
*/
static OSErr
InitDynamicUnicodeConverter( Boolean forBootVolume )
{
	CFragConnectionID	unicodeLib = 0;
	CFragConnectionID	textLib = 0;
	Ptr					tempMainAddr;
	Str255				errMessage;
	FSVarsRec			*fsVars;	
	THz					savedHeapZone;
	OSErr				result;


	fsVars = (FSVarsRec*) LMGetFSMVars();
	
	// use the system context
	savedHeapZone = GetZone();
	SetZone( SystemZone() );
  #if 0
	  (void) BeginSystemMode();
  #endif

	result = GetSharedLibrary("\pUnicodeConverter", kPowerPCCFragArch, kPrivateCFragCopy, &unicodeLib, &tempMainAddr, errMessage);
	ExitOnError( result );

	result = GetSharedLibrary("\pTextCommon", kPowerPCCFragArch, kPrivateCFragCopy, &textLib, &tempMainAddr, errMessage);
	ExitOnError( result );


	fsVars->gUseDynamicUnicodeConverters	= true;

	result = InstallLibraryVector( unicodeLib,
								   "\pCreateTextToUnicodeInfo",
								   uupCreateTextToUnicodeInfoProcInfo,
								   &fsVars->uppCreateTextToUnicodeInfo );
	ExitOnError( result );


	result = InstallLibraryVector( unicodeLib,
								   "\pCreateUnicodeToTextInfo",
								   uppCreateUnicodeToTextInfoProcInfo,
								   &fsVars->uppCreateUnicodeToTextInfo );
	ExitOnError( result );


	result = InstallLibraryVector( unicodeLib,
								   "\pConvertFromTextToUnicode",
								   uppConvertFromTextToUnicodeProcInfo,
								   &fsVars->uppConvertFromTextToUnicode );	// don't set vector yet
	ExitOnError( result );


	result = InstallLibraryVector( unicodeLib,
								   "\pConvertFromUnicodeToText",
								   uppConvertFromUnicodeToTextProcInfo,
								   &fsVars->uppConvertFromUnicodeToText );	// don't set vector yet
	ExitOnError( result );


	result = InstallLibraryVector( unicodeLib,
								   "\pLockMappingTable",
								   uppLockMappingTableProcInfo,
								   &fsVars->uppLockMappingTable );
	ExitOnError( result );


	result = InstallLibraryVector( textLib,
								   "\pUpgradeScriptInfoToTextEncoding",
								   uppUpgradeScriptInfoToTextEncodingProcInfo,
								   &fsVars->uppUpgradeScriptInfoToTextEncoding );
	ExitOnError( result );

  #if 0
	result = InstallLibraryVector( textLib,
								   "\pRevertTextEncodingToScriptInfo",
								   uppRevertTextEncodingToScriptInfoProcInfo,
								   &fsVars->uppRevertTextEncodingToScriptInfo );
	ExitOnError( result );
  #endif
 
	// NOTE:  Real Unicode filename conversion is not enabled yet but that's OK.
	// We can still call the converter library since it finds it's data files by
	// matching file type and creator (not by name).

	result = InstallSystemConversionContext(fsVars, forBootVolume);
	ExitOnError( result );

	fsVars->gIsUnicodeInstalled = true;		//	Unicode binding and setup succesful, and now enabled

	// restore previous context
  #if 0
	  (void) EndSystemMode();
  #endif
	SetZone( savedHeapZone );

	return noErr;

ErrorExit:

	fsVars->uppCreateTextToUnicodeInfo = NULL;
	fsVars->uppCreateUnicodeToTextInfo = NULL;
	fsVars->uppConvertFromTextToUnicode = NULL;
	fsVars->uppConvertFromUnicodeToText = NULL;
	fsVars->uppLockMappingTable = NULL;
	
	if ( unicodeLib )
		(void) CloseConnection( &unicodeLib );

	if ( textLib )
		(void) CloseConnection( &textLib );

	#if 0
	  (void) EndSystemMode();
	#endif
	SetZone( savedHeapZone );
	
	return result;

} // end InitDynamicUnicodeConverter



//
// Install a conversion context for the system script (call early in boot)
//
static OSErr
InstallSystemConversionContext( FSVarsRec *fsVars, Boolean forBootVolume )
{
	TextEncoding		defaultEncoding;
	ScriptCode			script;
	RegionCode			region;
	OSErr				result;

	// The Script Manager does not setup the region until later in boot
	// so if we're booting from HFS Plus we need to "manually" determine
	// the system script and region using the 'itlc' resource.

	if ( forBootVolume )
	{
		Handle		 configResource;
		ItlcRecord	*itlConfig;
		
		configResource = GetResource( 'itlc', 0 );
		
		if ( configResource != NULL )
		{
			itlConfig = (ItlcRecord*) *configResource;
			
			script = itlConfig->itlcSystem;
			region = itlConfig->itlcRegionCode;
	
			ReleaseResource( configResource );
		}
		else
		{
			script = GetScriptManagerVariable(smSysScript);
			region = kTextRegionDontCare;
		}		
	}
	else
	{
		script = GetScriptManagerVariable(smSysScript);
		region = GetScriptManagerVariable(smRegionCode);
	}

	result = UpgradeScriptInfoToTextEncoding_Glue( script,
												   kTextLanguageDontCare,
												   region,
												   NULL,
												   &defaultEncoding );
	if ( result == paramErr )
	{
		// ok, last ditch effort to get an encoding...
		result = UpgradeScriptInfoToTextEncoding_Glue( script,
													   kTextLanguageDontCare,
													   kTextRegionDontCare,
													   NULL,
													   &defaultEncoding );
	}
	ReturnIfError(result);
	
	defaultEncoding = GetTextEncodingBasePriv(defaultEncoding);
	
	result = InitializeEncodingContext( defaultEncoding, fsVars );
	ReturnIfError(result);
	
	if ( defaultEncoding != kTextEncodingMacRoman )
	{
		result = InitializeEncodingContext( kTextEncodingMacRoman, fsVars );	// always install Roman
		ReturnIfError(result);
	}

	// Since a call to set the default text encoding can occur
	// before any HFS Plus volumes are mounted we need to check
	// gDefaultBaseEncoding and gTextEncodingFontName to see if
	// the default encoding needs to change...
	if ( !forBootVolume )
	{
		TextEncoding	requestedEncoding;

		requestedEncoding = fsVars->gDefaultBaseEncoding;

		if ( requestedEncoding == kTextEncodingUndefined  &&  fsVars->gTextEncodingFontName != NULL )
		{
			result = UpgradeScriptInfoToTextEncoding_Glue (	kTextScriptDontCare,
															kTextLanguageDontCare,
															kTextRegionDontCare,
															fsVars->gTextEncodingFontName,
															&requestedEncoding );
			if ( result != noErr )
				requestedEncoding = kTextEncodingUndefined;

			DisposePtr((Ptr) fsVars->gTextEncodingFontName);	// we no longer need font name
			fsVars->gTextEncodingFontName = NULL;
		}

		if ( requestedEncoding != kTextEncodingUndefined  &&  requestedEncoding != defaultEncoding )
		{
			result = InitializeEncodingContext( requestedEncoding, fsVars );
			if ( result == noErr )
				defaultEncoding = requestedEncoding;
		}
	}

	
	// remember the default text encoding...
	fsVars->gDefaultBaseEncoding = defaultEncoding;

	return noErr;

} // end InstallSystemConversionContext



//
// Install any addition scripts that were installed by the system (call late in boot)
//
OSErr
InstallConversionContextsForInstalledScripts( void )
{
	FSVarsRec*		fsVars;
	TextEncoding	encoding;
	UInt32			scriptCount;
	ScriptCode		systemScript;
	ScriptCode		script;
	OSErr			result;


	fsVars = (FSVarsRec*) LMGetFSMVars();

	scriptCount = GetScriptManagerVariable(smEnabled);
	systemScript = GetScriptManagerVariable(smSysScript);

	script = 0;

	while ( (scriptCount > 0) && (script <= smLargestScript) )
	{
		if ( GetScriptVariable(script, smScriptEnabled) != 0 )	// is this script enabled?
		{		
			--scriptCount;

			if ( script != systemScript )	// we already did systemScript
			{
				LangCode	language;

				language = GetScriptVariable( script, smScriptLang );
			
				result = UpgradeScriptInfoToTextEncoding_Glue( script, language, kTextRegionDontCare, NULL, &encoding );
				if ( result == noErr )
					(void) InitializeEncodingContext( encoding, fsVars );
			}
		}

		++script;	// on to the next one
	}

	return noErr;

} // end InstallConversionContextsForInstalledScripts



//
// Install any addition scripts that were used by an HFS Plus volume (called at volume mount time)
//
OSErr
InstallVolumeConversionContexts( UInt64 encodingsBitmap )
{
	FSVarsRec *	fsVars;
	UInt32		encodingMask;
	UInt32		index;
	UInt32		encoding;
	OSErr		result;


	fsVars = (FSVarsRec*) LMGetFSMVars();

	index = 0;
	encodingMask = 1;

	while ( encodingMask != 0 )
	{
		if ( encodingMask & encodingsBitmap.lo )							// encodings 0 - 31
		{
			encoding = MapIndexToEncoding(index);
			result = InitializeEncodingContext( encoding, fsVars );
		}

		if	( encodingMask & encodingsBitmap.hi )							// encodings 32 - 64
		{
			encoding = MapIndexToEncoding(index + 32);
			result = InitializeEncodingContext( encoding, fsVars );
		}

		encodingMask = encodingMask << 1;
		++index;
	}

	return noErr;

} // end InstallVolumeConversionContexts



//
// Initialze a conversion context for a single encoding (if not already installed)
//
OSErr InitializeEncodingContext( TextEncoding encoding, FSVarsRec *fsVars )
{
	UInt32			index;
	UnicodeMapping	unicodeMapping;
	THz				savedHeapZone;
	OSStatus		result;


	encoding = GetTextEncodingBasePriv( encoding );
	index = MapEncodingToIndex(encoding);

	if ( fsVars->gConversionContext[index].toUnicode != 0 )
		return noErr;	// this one is already installed!

	savedHeapZone = GetZone();
	SetZone( SystemZone() );	// always use the system heap since Conversion Contexts are global


	// Note: by default kTextEncodingUnicodeV2_0 allows corporate use characters
	unicodeMapping.unicodeEncoding = CreateTextEncodingPriv( kTextEncodingUnicodeV2_0, kUnicodeCanonicalDecompVariant, 0 );
	unicodeMapping.otherEncoding = encoding;
	unicodeMapping.mappingVersion = kUnicodeUseHFSPlusMapping;


	if ( fsVars->gUseDynamicUnicodeConverters == false )
		result = LockMappingTable( &unicodeMapping, true );
	else
		result = LockMappingTable_Glue( &unicodeMapping, true );
	ExitOnError( result );
	
	result = CreateTextToUnicodeInfo_Glue( &unicodeMapping, &fsVars->gConversionContext[index].toUnicode );
	ExitOnError( result );

	result = CreateUnicodeToTextInfo_Glue( &unicodeMapping, &fsVars->gConversionContext[index].fromUnicode );
	ExitOnError( result );

	++(fsVars->gInstalledEncodings);	// keep track of how many we've installed

#if 0	
	result = SetFallbackUnicodeToText( fsVars->gConversionContext[index].fromUnicode,
									   NewUnicodeToTextFallbackProc(FallbackProc),	// since we are compiled 68K no routine descriptor is needed
									   kUnicodeFallbackCustomOnly,
									   NULL );

#endif

ErrorExit:

	SetZone( savedHeapZone );

	return result;

} // end InitializeEncodingContext



pascal OSStatus
FallbackProc( UniChar * srcUniStr, ByteCount srcUniStrLen, ByteCount *srcConvLen,
			  TextPtr destStr, ByteCount destStrLen, ByteCount *destConvLen,
			  LogicalAddress contextPtr, ConstUnicodeMappingPtr unicodeMappingPtr)
{
	#pragma unused (srcUniStr, destStrLen, contextPtr, unicodeMappingPtr)
	*srcConvLen = srcUniStrLen;
	*destStr = '_';
	*destConvLen = sizeof(unsigned char);

	return noErr;
}


static OSErr
InstallLibraryVector ( CFragConnectionID connectionID, ConstStr255Param symbolName, ProcInfoType procInfo, UniversalProcPtr *vector)
{
	UniversalProcPtr	routineDescriptor;
	CFragSymbolClass	symClass;
	ProcPtr				tVector;
	OSErr				result;
	

	result = FindSymbol(connectionID, symbolName, (Ptr *) &tVector, &symClass);
	ReturnIfError( result );

	routineDescriptor = NewRoutineDescriptorTrap(tVector, procInfo, kPowerPCISA);
	
	if ( routineDescriptor != NULL )
		*vector = routineDescriptor;
	else
		result = memFullErr;
		
	return result;
}



//
// Get filename extension (if any) as a pascal string
//
static void
GetFilenameExtension( ItemCount length, ConstUniCharArrayPtr unicodeStr, Str15 extStr )
{
	UInt32	i;
	UniChar	c;
	UInt16	extChars;			// number of extension characters (excluding the dot)
	UInt16	maxExtChars;
	Boolean	foundExtension;


	extStr[0] = (UInt8) 0;		// assume there's no extension

	if ( length < 3 )
		return;					// sorry, "x.y" is smallest possible extension	
	
	if ( length < (kMaxFileExtensionChars + 2) )
		maxExtChars = length - 2;	// we need at least one prefix character and dot
	else
		maxExtChars = kMaxFileExtensionChars;

	i = length;
	extChars = 0;
	foundExtension = false;

	while ( extChars <= maxExtChars )
	{
		c = unicodeStr[--i];

		if ( c == (UniChar) '.' )		// look for leading dot
		{
			if ( extChars > 0 )			// cannot end with a dot
				foundExtension = true;
			break;
		}

		if ( Is7BitASCII(c) || IsSpecialUnicodeChar(c) )
			++extChars;
		else
			break;
	}
	
	// if we found one then copy it
	if ( foundExtension )
	{
		UInt8 *extStrPtr = extStr;
		const UniChar *unicodeStrPtr = &unicodeStr[i];	// point to dot char
		
		*(extStrPtr++) = extChars + 1;		// set length to extension chars plus dot

		for ( i = 0; i <= extChars; ++i )
		{
			c = *(unicodeStrPtr++);
			
			// map any special characters
			switch (c)
			{
				case 0x00B5:			// micro sign
				case 0x03BC:			// greek mu
					c = (UniChar) 'µ';
					break;

				case 0x03C0:			// greek pi
					c = (UniChar) 'º';
					break;

				case 0x2206:			// increment sign
				case 0x0394:			// greek capital delta
					c = (UniChar) 'ê';
					break;
			}

			*(extStrPtr++) = (UInt8) c;		// copy/convert to ascii
		}
	}

} // end GetFilenameExtension


//
// Count filename extension characters (if any)
//
static UInt32
CountFilenameExtensionChars( ConstStr31Param filename )
{
	UInt32	i;
	UniChar	c;
	UInt32	extChars;			// number of extension characters (excluding the dot)
	UInt32	length;
	UInt16	maxExtChars;
	Boolean	foundExtension;


	length = StrLength(filename);

	if ( length < 3 )
		return 0;					// sorry, "x.y" is smallest possible extension	
	
	if ( length < (kMaxFileExtensionChars + 2) )
		maxExtChars = length - 2;	// we need at least on prefix character and dot
	else
		maxExtChars = kMaxFileExtensionChars;

	extChars = 0;				// assume there's no extension
	i = length;					// index to last ascii character
	foundExtension = false;

	while ( extChars <= maxExtChars )
	{
		c = filename[i--];

		if ( c == (UInt8) '.' )		// look for leading dot
		{
			if ( extChars > 0 )			// cannot end with a dot
				return (extChars);

			break;
		}

		if ( Is7BitASCII(c) || IsSpecialASCIIChar(c) )
			++extChars;
		else
			break;
	}
	
	return 0;

} // end CountFilenameExtensionChars


//
// Convert file ID into a hexidecimal string with no leading zeros
//
static void
GetFileIDString( CatalogNodeID fileID, Str15 fileIDStr )
{
	SInt32	i, b;
	UInt8	*translate = (UInt8 *) "0123456789ABCDEF";
	UInt8	c;
	
	fileIDStr[1] = '#';

	for ( i = 1, b = 28; b >= 0; b -= 4 )
	{
		c = *(translate + ((fileID >> b) & 0x0000000F));
		
		// if its not a leading zero add it to our string
		if ( (c != (UInt8) '0') || (i > 1) || (b == 0) )
			fileIDStr[++i] = c;
	}

	fileIDStr[0] = (UInt8) i;

} // end GetFileIDString


//
// Append a suffix to a pascal string
//
static void
AppendPascalString( ConstStr15Param src, Str31 dst )
{
	UInt32	i, j;
	UInt32	srcLen;
	
	srcLen = StrLength(src);
	
	if ( (srcLen + StrLength(dst)) > 31 )	// safety net
		return;
	
	i = dst[0] + 1;		// get end of dst
	
	for (j = 1; j <= srcLen; ++j)
		dst[i++] = src[j];
		
	dst[0] += srcLen;

} // end AppendPascalString


CatalogNodeID
GetEmbeddedFileID( ConstStr31Param filename )
{
	short	length;
	short	extChars;
	short	i;
	UInt8	c;			// current character in filename


	if ( filename == NULL )
		return 0;

	length = StrLength(filename);

	if ( length < 4 )
		return 0;		// too small to have a file ID

	if ( length >= 6 )	// big enough for a file ID (#10) and an extension (.x) ?
		extChars = CountFilenameExtensionChars(filename);
	else
		extChars = 0;
	
	if ( extChars > 0 )
		length -= (extChars + 1);	// skip dot plus extension characters

	// scan for file id digits...
	for ( i = length; i > 0; --i)
	{
		c = filename[i];

		if ( c == '#' )		// look for file ID marker
		{
			if ( (length - i) < 2 )
				break;		// too small to be a file ID

			return HexStringToInteger(length - i, &filename[i+1]);
		}

		if ( !IsHexDigit(c) )
			break;			// file ID string must have hex digits	
	}

	return 0;

} // end GetEmbeddedFileID


//_______________________________________________________________________

static UInt32
HexStringToInteger (UInt32 length, const UInt8 *hexStr)
{
	UInt32		value;	// decimal value represented by the string
	short		i;
	UInt8		c;		// next character in buffer
	const UInt8	*p;		// pointer to character string

	value = 0;
	p = hexStr;

	for ( i = 0; i < length; ++i )
	{
		c = *p++;

		if (c >= '0' && c <= '9')
		{
			value = value << 4;
			value += (UInt32) c - (UInt32) '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			value = value << 4;
			value += 10 + ((unsigned int) c - (unsigned int) 'A');
		}
		else
		{
			return 0;	// oops, how did this character get in here?
		}
	}

	return value;

} // end HexStringToInteger

#endif

//_______________________________________________________________________
//
//	Routine:	FastRelString
//
//	Output:		returns -1 if str1 < str2
//				returns  1 if str1 > str2
//				return	 0 if equal
//
//_______________________________________________________________________

extern void gCompareTable(void);

SInt32	FastRelString( ConstStr255Param str1, ConstStr255Param str2 )
{
	UInt16*			compareTable;
	SInt32	 		bestGuess;
	UInt8 	 		length, length2;

	
	length = *(str1++);
	length2 = *(str2++);

	if (length == length2)
		bestGuess = 0;
	else if (length < length2)
		bestGuess = -1;
	else
	{
		bestGuess = 1;
		length = length2;
	}

	compareTable = (UInt16*) gCompareTable;

	while (length--)
	{
		UInt8	aChar, bChar;

		aChar = *(str1++);
		bChar = *(str2++);
		
		if (aChar != bChar)		//	If they don't match exacly, do case conversion
		{	
			UInt16	aSortWord, bSortWord;

			aSortWord = compareTable[aChar];
			bSortWord = compareTable[bChar];

			if (aSortWord > bSortWord)
				return 1;

			if (aSortWord < bSortWord)
				return -1;
		}
		
		//	If characters match exactly, then go on to next character immediately without
		//	doing any extra work.
	}
	
	//	if you got to here, then return bestGuess
	return bestGuess;
}	

#ifdef INVESTIGATE

static Boolean BasicLatinUnicodeToPascal (ItemCount unicodeChars, ConstUniCharArrayPtr unicodeString, Str31 pascalString)
{
	UInt8			*p;
	const UniChar	*u;
	UniChar			c;
	UniChar			mask;
	UInt16			i;


	if ( unicodeChars > 31 )
		return false;		// we don't do long name truncation here!

	mask = (UniChar) 0xFF80;
	p = &pascalString[1];
	u = unicodeString;

	for (i= 0; i < unicodeChars; ++i)
	{
		c = *(u++);

		if ( c & mask )		// make sure its seven bit ascii
		{
			if ( c == kUnicodeTradeMarkChar )		// we also handle 'Å'
				c = (UniChar) kASCIITradeMarkChar;
			else
				return false;
		}

		*(p++) = (UInt8) c;		// strip off high byte
	}
	
	pascalString[0] = (UInt8) unicodeChars;		// set pascal length byte
	
	return true;
}



static Boolean PascalToBasicLatinUnicode (ConstStr31Param pascalString, ItemCount *unicodeChars, UniCharArrayPtr unicodeString)
{
	const UInt8		*p;
	UniChar			*u;
	UInt16			pascalChars;
	UInt16			i;
	UInt8			c;


	p = pascalString;
	u = unicodeString;

	pascalChars = *(p++);	// pick up length byte

	if ( pascalChars > 31 )
		return false;

	for (i= 0; i < pascalChars; ++i)
	{
		c = *(p++);

		if ( (SInt8) c >= 0 )		// make sure its seven bit ascii
		{
			*(u++) = (UniChar) c;		//  pad high byte with zero
		}
		else
		{
			if ( c == (UInt8) kASCIITradeMarkChar )		// we also handle 'Å'
				*(u++) = kUnicodeTradeMarkChar;
			else
				return false;
		}
	}
	
	*unicodeChars = (UInt16) pascalChars;
	
	return true;
}

#endif

//
//	FastUnicodeCompare - Compare two Unicode strings; produce a relative ordering
//
//	    IF				RESULT
//	--------------------------
//	str1 < str2		=>	-1
//	str1 = str2		=>	 0
//	str1 > str2		=>	+1
//
//	The lower case table starts with 256 entries (one for each of the upper bytes
//	of the original Unicode char).  If that entry is zero, then all characters with
//	that upper byte are already case folded.  If the entry is non-zero, then it is
//	the _index_ (not byte offset) of the start of the sub-table for the characters
//	with that upper byte.  All ignorable characters are folded to the value zero.
//
//	In pseudocode:
//
//		Let c = source Unicode character
//		Let table[] = lower case table
//
//		lower = table[highbyte(c)]
//		if (lower == 0)
//			lower = c
//		else
//			lower = table[lower+lowbyte(c)]
//
//		if (lower == 0)
//			ignore this character
//
//	To handle ignorable characters, we now need a loop to find the next valid character.
//	Also, we can't pre-compute the number of characters to compare; the string length might
//	be larger than the number of non-ignorable characters.  Further, we must be able to handle
//	ignorable characters at any point in the string, including as the first or last characters.
//	We use a zero value as a sentinel to detect both end-of-string and ignorable characters.
//	Since the File Manager doesn't prevent the NUL character (value zero) as part of a filename,
//	the case mapping table is assumed to map u+0000 to some non-zero value (like 0xFFFF, which is
//	an invalid Unicode character).
//
//	Pseudocode:
//
//		while (1) {
//			c1 = GetNextValidChar(str1)			//	returns zero if at end of string
//			c2 = GetNextValidChar(str2)
//
//			if (c1 != c2) break					//	found a difference
//
//			if (c1 == 0)						//	reached end of string on both strings at once?
//				return 0;						//	yes, so strings are equal
//		}
//
//		// When we get here, c1 != c2.  So, we just need to determine which one is less.
//		if (c1 < c2)
//			return -1;
//		else
//			return 1;
//

extern void gLowerCaseTable(void);		//	Really an exported data symbol, a table

SInt32 FastUnicodeCompare ( register ConstUniCharArrayPtr str1, register ItemCount length1,
							register ConstUniCharArrayPtr str2, register ItemCount length2)
{
	register UInt16		c1,c2;
	register UInt16		temp;
	register UInt16*	lowerCaseTable;

#if ( TARGET_OS_MAC && FORDISKFIRSTAID )	
	lowerCaseTable = Get_gLowerCaseTable();
#else
	lowerCaseTable = (UInt16*) gLowerCaseTable;
#endif

	while (1) {
		//	Set default values for c1, c2 in case there are no more valid chars
		c1 = 0;
		c2 = 0;
		
		//	Find next non-ignorable char from str1, or zero if no more
		while (length1 && c1 == 0) {
			c1 = *(str1++);
			--length1;
			if ((temp = lowerCaseTable[c1>>8]) != 0)		//	is there a subtable for this upper byte?
				c1 = lowerCaseTable[temp + (c1 & 0x00FF)];	//	yes, so fold the char
		}
		
		
		//	Find next non-ignorable char from str2, or zero if no more
		while (length2 && c2 == 0) {
			c2 = *(str2++);
			--length2;
			if ((temp = lowerCaseTable[c2>>8]) != 0)		//	is there a subtable for this upper byte?
				c2 = lowerCaseTable[temp + (c2 & 0x00FF)];	//	yes, so fold the char
		}
		
		if (c1 != c2)		//	found a difference, so stop looping
			break;
		
		if (c1 == 0)		//	did we reach the end of both strings at the same time?
			return 0;		//	yes, so strings are equal
	}
	
	if (c1 < c2)
		return -1;
	else
		return 1;
}
