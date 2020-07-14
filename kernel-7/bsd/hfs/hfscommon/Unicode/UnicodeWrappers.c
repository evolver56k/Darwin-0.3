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
	File:		UnicodeWrappers.c

	Contains:	Wrapper routines for Unicode conversion and comparison.

	Version:	HFS Plus 1.0

	Written by:	Mark Day

	Copyright:	© 1996-1999 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		Don Brady

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):
      <Rhap>	 2/09/99	djb		Fix UnicodeToMacRoman to handle a terminating decomposed char.
      <Rhap>	 1/22/99	djb		Add more TARGET_OS_MAC conditionals to remove orphaned code.
      <Rhap>	  7/6/98	djb		Handle hi-bit Mac Roman characters in basic latin conversions (radar #2247519).
      <Rhap>	 6/11/98	PPD		Added a few special-case ASCII/Unicode mappings to cover installer's needs.
	  <Rhap>	 3/31/98	djb		Sync up with final HFSVolumes.h header file.

	  <CS41>	 1/28/98	msd		Bug 2207446: When mangling a name, check to see if the Unicode
									Converter is installed before we call it.
	  <CS40>	 1/21/98	msd		Bug 2206836: If a name contains a colon, change it to question
									mark and mangle the name.
	  <CS39>	12/11/97	msd		For Metrowerks and test tools, call the Get_xxx routines to get
									the Unicode table addresses.
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
	  <CS23>	  9/7/97	djb		Handle 'ª' char in BasicLatinUnicode converter.
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

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
        #include	<CodeFragments.h>
        #include	<Errors.h>
        #include	<MixedModePriv.h>
        #include	<Script.h>
        #include	<UnicodeConverter.h>
        #include	<LowMemPriv.h>
        #include	<Gestalt.h>
        #include	<TextUtils.h>
        #else
        #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#if TARGET_OS_MAC
#include	<IntlResources.h>
#include	<TextCommonPriv.h>
#include	<UnicodeConverterPriv.h>
#include 	<FileMgrResources.h>
#endif 	/* TARGET_OS_MAC */

#if TARGET_OS_RHAPSODY
#include "UCStringCompareData.h"
#endif /* TARGET_OS_RHAPSODY */

#include "../headers/FileMgrInternal.h"
#include "../headers/system/HFSUnicodeWrappers.h"
#include "../headers/system/HFSInstrumentation.h"

#include "ConvertUTF.h"

#ifdef __MWERKS__
#define USE_TABLE_ACCESSORS	1
#endif

#ifndef USE_TABLE_ACCESSORS
#define USE_TABLE_ACCESSORS	0
#endif


#if TARGET_OS_MAC
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
#endif 	/* TARGET_OS_MAC */


enum {
	smLargestScript = 32,
	
	kMinFileExtensionChars = 1,		// does not include dot
	kMaxFileExtensionChars = 5		// does not include dot
};

#define kASCIIPiSymbol				0xB9
#define kASCIIMicroSign				0xB5
#define kASCIIGreekDelta			0xC6


#define Is7BitASCII(c)				( (c) >= 0x20 && (c) <= 0x7F )

#define	IsSpecialASCIIChar(c)		( (c) == (UInt8) kASCIIMicroSign || (c) == (UInt8) kASCIIPiSymbol || (c) == (UInt8) kASCIIGreekDelta )

// Note:	'µ' has two Unicode representations 0x00B5 (micro sign) and 0x03BC (greek)
//			'Æ' has two Unicode representations 0x2206 (increment) and 0x0394 (greek)
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

#if TARGET_OS_MAC
static OSErr InitStaticUnicodeConverter(void);
static OSErr InitDynamicUnicodeConverter( Boolean forBootVolume );
#endif	/* TARGET_OS_MAC */


#if TARGET_OS_MAC
static OSErr InstallConversionContexts( FSVarsRec *fsVars );

static OSErr	InstallLibraryVector( CFragConnectionID connectionID, ConstStr255Param symbolName,
									  ProcInfoType procInfo, UniversalProcPtr *vector );

static OSErr	InstallSystemConversionContext( FSVarsRec *fsVars, Boolean forBootVolume );
#endif	/* TARGET_OS_MAC */

static void	GetFilenameExtension( ItemCount length, ConstUniCharArrayPtr unicodeStr, Str15 extStr );

static void	GetFileIDString( HFSCatalogNodeID fileID, Str15 fileIDStr );

static void AppendPascalString( ConstStr15Param src, Str31 dst );

static UInt32 HexStringToInteger( UInt32 length, const UInt8 *hexStr );


pascal OSStatus FallbackProc( UniChar *srcUniStr, ByteCount srcUniStrLen, ByteCount *srcConvLen,
							  TextPtr destStr, ByteCount destStrLen, ByteCount *destConvLen,
							  LogicalAddress contextPtr, ConstUnicodeMappingPtr unicodeMappingPtr );


static OSErr MacRomanToUnicode (ConstStr255Param pascalString, ItemCount *unicodeChars, UniCharArrayPtr unicodeString);
static OSErr UnicodeToMacRoman (ItemCount unicodeChars, ConstUniCharArrayPtr unicodeString, Str31 pascalString);


/*
	Get the base encoding used by the File System
	
	If no HFS Plus volumes have been mounted yet then
	the default encoding could be kTextEncodingUndefined
*/
#if TARGET_OS_MAC
TextEncoding
GetDefaultTextEncoding(void)
{
	FSVarsRec	*fsVars;	

	fsVars = (FSVarsRec*) LMGetFSMVars();

	return fsVars->gDefaultBaseEncoding;
}
#endif


/*
	Set the base encoding used by the File System
*/
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */

/*
	Get the encoding that matches font
*/

#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */

/*
	Count the number of encodings installed by the File System
*/
#if TARGET_OS_MAC
ItemCount
CountInstalledEncodings(void)
{
	FSVarsRec	*fsVars;	

	fsVars = (FSVarsRec*) LMGetFSMVars();

	return fsVars->gInstalledEncodings;
}
#endif

/*
	Convert a Unicode string to a Pascal string (Str31) for use in an HFS file/folder name.
*/
#if 0
OSErr
ConvertUnicodeToHFSName( ConstHFSUniStr255Param unicodeName, TextEncoding encoding, HFSCatalogNodeID cnid, Str31 hfsName )
{
	ByteCount		unicodeByteLength;
	ByteCount		pascalSizeLimit;
	OSErr			result;
	
	hfsName[0] = 0;		// in case we get errors, make sure output is valid
	unicodeByteLength = unicodeName->length * sizeof(UniChar);

	if ( unicodeByteLength == 0 )
		return noErr;

	if ( cnid == kHFSRootFolderID )
		pascalSizeLimit = kHFSMaxVolumeNameChars;	// an HFS volume name
	else
		pascalSizeLimit = kHFSMaxFileNameChars;		// an HFS file name

	result = MockConvertFromUnicodeToPString( unicodeByteLength, unicodeName->unicode, hfsName );

	// Check if name was too long or some characters were unrepresentable...
	// if so we need to mangle the name so that the file can be found by
	// name later

	if ( result == kTECOutputBufferFullStatus || result == kTECUsedFallbacksStatus )
	{
		Str15		fileIDStr;				// file ID as a pascal string
		Str15		extStr;					// dot extension as a pascal string

		GetFileIDString(cnid, fileIDStr);
		
		//	Get a filename extension only if it is a file.
		if ( pascalSizeLimit == kHFSMaxFileNameChars)
			GetFilenameExtension(unicodeName->length, unicodeName->unicode, extStr);
		else
			extStr[0] = (UInt8) 0;	// volumes don't have extensions
	
		// calculate free space for filename prefix
		pascalSizeLimit -= StrLength(extStr) + StrLength(fileIDStr);
		
		//	Generate the prefix part of the name (before extension or File ID string).
		//	Since the Unicode converter wasn't installed, use the PString we already have,
		//	shortening it if needed.
		if (hfsName[0] > pascalSizeLimit)
			hfsName[0] = pascalSizeLimit;
		else
			hfsName[0] -= StrLength(extStr);	// remove extension chars (if any) from source
		
		strcat(hfsName, fileIDStr);
		strcat(hfsName, extStr);
		
		result = noErr;
	}

	return result;

} // end ConvertUnicodeToHFSName
#endif

/*
	Convert a Pascal string (Str31, such as a file/folder name) to Unicode.
*/
#if TARGET_OS_MAC
OSErr
ConvertHFSNameToUnicode( ConstStr31Param hfsName, TextEncoding encoding, HFSUniStr255 *unicodeName )
{
	ByteCount	unicodeByteLength;
	OSErr		result;
	

	result = MockConvertFromPStringToUnicode( hfsName, sizeof(unicodeName->unicode), &unicodeByteLength, unicodeName->unicode );
	
	unicodeName->length = unicodeByteLength / sizeof(UniChar);	// Note: from byte count to char count

	return result;

} // end ConvertHFSNameToUnicode
#endif


/*
	MockConvertFromUnicodeToPString
 */
#if TARGET_OS_MAC
OSErr
MockConvertFromUnicodeToPString( ByteCount unicodeLength, ConstUniCharArrayPtr unicodeStr, Str31 pascalStr )
{
	return UnicodeToMacRoman(unicodeLength / sizeof(UniChar), unicodeStr, pascalStr);
}
#endif

/*
	MockConvertFromPStringToUnicode
 */
#if TARGET_OS_MAC
OSErr
MockConvertFromPStringToUnicode(ConstStr31Param pascalStr, ByteCount maxUnicodeLen, ByteCount *actualUnicodeLen, UniCharArrayPtr unicodeStr)
{
#pragma unused(maxUnicodeLen)

	UInt32	unicodeChars;
	OSErr	result;

	result = MacRomanToUnicode (pascalStr, &unicodeChars, unicodeStr);

	*actualUnicodeLen = unicodeChars * sizeof(UniChar);		// return length in bytes

	return result;
}
#endif

/*
	Initialize the Unicode Converter library
	
	If the library cannot be initialized the wrapper code will default to using 7-bit ASCII or mangled names.
	
	WARNING: This cannot be called from within a file system call (since it calls the file system)!
*/
#if TARGET_OS_MAC
OSErr InitUnicodeConverter(Boolean forBootVolume)
{
	FSVarsRec			*fsVars;	
	Handle				resourceHandle;
	long				response;
	OSErr				err;

	
	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( fsVars->gIsUnicodeInstalled == true )					//	Has Unicode already been installed
		return ( noErr );
	
	err = Gestalt( gestaltSysArchitecture, &response );			//¥¥ Runtime check to load static or dynamic libraries
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
#endif /* TARGET_OS_MAC */	


/*
	Initialize the Staticly linked 68K Unicode Converter library

	If the library cannot be initialized the wrapper code will default to using 7-bit ASCII or mangled names.
	WARNING: This cannot be called from within a file system call (since it calls the file system)!
*/
#if TARGET_OS_MAC
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

	GetIndString( textEncodingConverterName, kBaseHFSPlusResourceID, rTextEncodingConverterName );	//¥¥Êneed real string!!
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
#endif /* TARGET_OS_MAC */	



/*
	Initialize the PPC CFM dynamically linked 68K Unicode Converter library
	If the library cannot be initialized the wrapper code will default to
	using the 68K staticly linked library.
*/
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */	



//
// Install a conversion context for the system script (call early in boot)
//
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */	



//
// Install any addition scripts that were installed by the system (call late in boot)
//
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */	



//
// Install any addition scripts that were used by an HFS Plus volume (called at volume mount time)
//
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */	


//
// Initialze a conversion context for a single encoding (if not already installed)
//
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */	


#if TARGET_OS_MAC
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
#endif

#if TARGET_OS_MAC
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
#endif


//
// Get filename extension (if any) as a pascal string
//
#if TARGET_OS_MAC
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
					c = (UniChar) '¹';
					break;

				case 0x2206:			// increment sign
				case 0x0394:			// greek capital delta
					c = (UniChar) 'Æ';
					break;
			}

			*(extStrPtr++) = (UInt8) c;		// copy/convert to ascii
		}
	}

} // end GetFilenameExtension
#endif /* TARGET_OS_MAC */


//
// Count filename extension characters (if any)
//
static UInt32
CountFilenameExtensionChars( const unsigned char * filename )
{
	UInt32	i;
	UniChar	c;
	UInt32	extChars;			// number of extension characters (excluding the dot)
	UInt32	length;
	UInt16	maxExtChars;
	Boolean	foundExtension;


	length = strlen(filename);

	if ( length < 3 )
		return 0;					// sorry, "x.y" is smallest possible extension	
	
	if ( length < (kMaxFileExtensionChars + 2) )
		maxExtChars = length - 2;	// we need at least on prefix character and dot
	else
		maxExtChars = kMaxFileExtensionChars;

	extChars = 0;				// assume there's no extension
	i = length - 1;				// index to last ascii character
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
#if TARGET_OS_MAC
static void
GetFileIDString( HFSCatalogNodeID fileID, Str15 fileIDStr )
{
	SInt32	i, b;
	static UInt8 *translate = (UInt8 *) "0123456789ABCDEF";
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
#endif /* TARGET_OS_MAC */


//
// Append a suffix to a pascal string
//
#if TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */


HFSCatalogNodeID
GetEmbeddedFileID(const unsigned char * filename, UInt32 *prefixLength)
{
	short	length;
	short	extChars;
	short	i;
	UInt8	c;			// current character in filename

	*prefixLength = 0;

	if ( filename == NULL )
		return 0;

	length = strlen(filename);

	if ( length < 4 )
		return 0;		// too small to have a file ID

	if ( length >= 6 )	// big enough for a file ID (#10) and an extension (.x) ?
		extChars = CountFilenameExtensionChars(filename);
	else
		extChars = 0;
	
	if ( extChars > 0 )
		length -= (extChars + 1);	// skip dot plus extension characters

	// scan for file id digits...
	for ( i = length - 1; i >= 0; --i)
	{
		c = filename[i];

		if ( c == '#' )		// look for file ID marker
		{
			if ( (length - i) < 3 )
				break;		// too small to be a file ID

			*prefixLength = i;
			return HexStringToInteger(length - i - 1, &filename[i+1]);
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


//_______________________________________________________________________
//
//	Routine:	FastRelString
//
//	Output:		returns -1 if str1 < str2
//				returns  1 if str1 > str2
//				return	 0 if equal
//
//_______________________________________________________________________

#if USE_TABLE_ACCESSORS
	UInt16 *Get_gCompareTable(void);
#else
	extern unsigned short gCompareTable[];
#endif

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

#if USE_TABLE_ACCESSORS
	compareTable = Get_gCompareTable();
#else
	compareTable = (UInt16*) gCompareTable;
#endif

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



/* 0x00A0 - 0x00FF = Latin 1 Supplement (30 total) */
UInt8 gLatin1Table[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x00A0 */	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4,  '?', 0xA4, 0xAC, 0xA9, 0xBB, 0xC7, 0xC2,  '?', 0xA8, 0xF8,
	/* 0x00B0 */	0xA1, 0XB1,  '?',  '?', 0xAB, 0xB5, 0xA6, 0xe1, 0xFC,  '?', 0xBC, 0xC8,  '?',  '?',  '?', 0xC0,
	/* 0x00C0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xAE,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x00D0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xAF,  '?',  '?',  '?',  '?',  '?',  '?', 0xA7,
	/* 0x00E0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xBE,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x00F0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD6, 0xBF,  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* 0x02C0 - 0x02DF = Spacing Modifiers (8 total) */
UInt8 gSpaceModsTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x02C0 */	 '?',  '?',  '?',  '?',  '?',  '?', 0xF6, 0xFF,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x02D0 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xF9, 0xFA, 0xFB, 0xFE, 0xF7, 0xFD,  '?',  '?'
};


/* 0x20xx = General Punctuation (16 total) */
UInt8 gPunctTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x2000 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2010 */	 '?',  '?',  '?', 0xd0, 0xd1,  '?',  '?',  '?', 0xd4, 0xd5, 0xe2,  '?', 0xd2, 0xd3, 0xe3,  '?',
	/* 0x2020 */	0xa0, 0xe0, 0xa5,  '?',  '?',  '?', 0xc9,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2030 */	0xe4,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xdc, 0xdd,  '?',  '?',  '?',  '?',  '?',
	/* 0x2040 */	 '?',  '?',  '?',  '?', 0xda,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* 0x2200 = Mathematical Operators (11 total) */
UInt8 gMathTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x2200 */	 '?',  '?', 0xb6,  '?',  '?',  '?', 0xc6,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xb8,
	/* 0x2210 */	 '?', 0xb7,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xc3,  '?',  '?',  '?', 0xb0,  '?',
	/* 0x2220 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xba,  '?',  '?',  '?',  '?',
	/* 0x2230 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2240 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xc5,  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2250 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	/* 0x2260 */	0xad,  '?',  '?',  '?', 0xb2, 0xb3,  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


/* */
UInt8 gReverseCombTable[] =
{                 /*  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
	/* 0x40 */		0xDA, 0x40, 0xDA, 0xDA, 0xDA, 0x56, 0xDA, 0xDA, 0xDA, 0x6C, 0xDA, 0xDA, 0xDA, 0xDA, 0x82, 0x98,
	/* 0x50 */		0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xAE, 0xDA, 0xDA, 0xDA, 0xC4, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA,
	/* 0x60 */		0xDA, 0x4B, 0xDA, 0xDA, 0xDA, 0x61, 0xDA, 0xDA, 0xDA, 0x77, 0xDA, 0xDA, 0xDA, 0xDA, 0x8D, 0xA3,
	/* 0x70 */		0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xB9, 0xDA, 0xDA, 0xDA, 0xCF, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA, 0xDA,

	/* Combining Diacritical Marks (0x0300 - 0x030A) */

	/*                0     1     2     3     4     5     6     7     8     9     A  */
	/*  'A'   */
	/* 0x0300 */	0xCB, 0xE7, 0xE5, 0xCC,  '?',  '?',  '?',  '?', 0x80,  '?', 0x81,

	/*  'a'   */
	/* 0x0300 */	0x88, 0x87, 0x89, 0x8B,  '?',  '?',  '?',  '?', 0x8A,  '?', 0x8C,

	/*  'E'   */
	/* 0x0300 */	0xE9, 0x83, 0xE6,  '?',  '?',  '?',  '?',  '?', 0xE8,  '?',  '?',

	/*  'e'   */
	/* 0x0300 */	0x8F, 0x8E, 0x90,  '?',  '?',  '?',  '?',  '?', 0x91,  '?',  '?',

	/*  'I'   */
	/* 0x0300 */	0xED, 0xEA, 0xEB,  '?',  '?',  '?',  '?',  '?', 0xEC,  '?',  '?',

	/*  'i'   */
	/* 0x0300 */	0x93, 0x92, 0x94,  '?',  '?',  '?',  '?',  '?', 0x95,  '?',  '?',

	/*  'N'   */
	/* 0x0300 */	 '?',  '?',  '?', 0x84,  '?',  '?',  '?',  '?',  '?',  '?',  '?',

	/*  'n'   */
	/* 0x0300 */	 '?',  '?',  '?', 0x96,  '?',  '?',  '?',  '?',  '?',  '?',  '?',

	/*  'O'   */
	/* 0x0300 */	0xF1, 0xEE, 0xEF, 0xCD,  '?',  '?',  '?',  '?', 0x85,  '?',  '?',

	/*  'o'   */
	/* 0x0300 */	0x98, 0x97, 0x99, 0x9B,  '?',  '?',  '?',  '?', 0x9A,  '?',  '?',

	/*  'U'   */
	/* 0x0300 */	0xF4, 0xF2, 0xF3,  '?',  '?',  '?',  '?',  '?', 0x86,  '?',  '?',
	
	/*  'u'   */
	/* 0x0300 */	0x9D, 0x9C, 0x9E,  '?',  '?',  '?',  '?',  '?', 0x9F,  '?',  '?',

	/*  'Y'   */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD9,  '?',  '?',

	/*  'y'   */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?', 0xD8,  '?',  '?',

	/*  else  */
	/* 0x0300 */	 '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?'
};


static OSErr UnicodeToMacRoman (ItemCount unicodeChars, ConstUniCharArrayPtr unicodeString, Str31 pascalString)
{
	UInt8			*p;
	const UniChar	*u;
	UniChar			c;
	UniChar			mask;
	UInt16			inputChars;
	UInt16			pascalChars;
	OSErr			result = noErr;
	UInt8			lsb;
	UInt8			prevChar;
	UInt8			mc;


	mask = (UniChar) 0xFF80;
	p = &pascalString[1];
	u = unicodeString;
	inputChars = unicodeChars;
	pascalChars = prevChar = 0;
	
	while (inputChars) {
		c = *(u++);
		lsb = (UInt8) c;

		/*
		 * If its not 7-bit ascii, then we need to map it
		 */
		if ( c & mask ) {
			mc = '?';
			switch (c & 0xFF00) {
				case 0x0000:
					if (lsb >= 0xA0)
						mc = gLatin1Table[lsb - 0xA0];
					break;

				case 0x0200:
					if (lsb >= 0xC0 && lsb <= 0xDF)
						mc = gSpaceModsTable[lsb - 0xC0];
					break;

				case 0x2000:
					if (lsb <= 0x4F)
						mc = gPunctTable[lsb];
					break;

				case 0x2200:
					if (lsb <= 0x6F)
						mc = gMathTable[lsb];
					break;

				case 0x0300:
					if (c <= 0x030A) {
						if (prevChar >= 'A' && prevChar < 'z') {
							mc = gReverseCombTable[gReverseCombTable[prevChar - 0x40] + lsb];
							--p;	/* backup over base char */
							--pascalChars;
						}
					}
					else {
						switch (c) {
							case 0x0327:	/* combining cedilla */
								if (prevChar == 'C')
									mc = 0x82;
								else if (prevChar == 'c')
									mc = 0x8D;
								else
									break;
								--p;	/* backup over base char */
								--pascalChars;
								break;

							case 0x03A9: mc = 0xBD; break;	/* omega */

							case 0x03C0: mc = 0xB9; break;	/* pi */
						}
					}
					break;
					
				default:
					switch (c) {
						case 0x0131: mc = 0xf5; break;	/* dotless i */

						case 0x0152: mc = 0xce; break;	/* OE */

						case 0x0153: mc = 0xcf; break;	/* oe */

						case 0x0192: mc = 0xc4; break;	/* Ä */

						case 0x2122: mc = 0xaa; break;	/* TM */

						case 0x25ca: mc = 0xd7; break;	/* diamond */

						case 0xf8ff: mc = 0xf0; break;	/* apple logo */

						case 0xfb01: mc = 0xde; break;	/* fi */

						case 0xfb02: mc = 0xdf; break;	/* fl */
					}
			} /* end switch (c & 0xFF00) */
			
			/*
			 * If we have an unmapped character then we need to mangle the name...
			 */
			if (mc == '?')
				result = kTECUsedFallbacksStatus;
			
			prevChar = 0;
			lsb = mc;

		} /* end if (c & mask) */
		else {
			prevChar = lsb;
		}

		if (pascalChars >= 31)
			break;

		*(p++) = lsb;
		++pascalChars;
		--inputChars;

	} /* end while */
	
	pascalString[0] = pascalChars;
	
	if (inputChars > 0)
		result = kTECOutputBufferFullStatus;	/* ran out of room! */

	return result;
}


UniChar	gHiBitBaseUnicode[128] =
{
	/* 0x80 */	0x0041, 0x0041, 0x0043, 0x0045, 0x004e, 0x004f, 0x0055, 0x0061, 
	/* 0x88 */	0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0063, 0x0065, 0x0065, 
	/* 0x90 */	0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069, 0x006e, 0x006f, 
	/* 0x98 */	0x006f, 0x006f, 0x006f, 0x006f, 0x0075, 0x0075, 0x0075, 0x0075, 
	/* 0xa0 */	0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
	/* 0xa8 */	0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
	/* 0xb0 */	0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
	/* 0xb8 */	0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
	/* 0xc0 */	0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
	/* 0xc8 */	0x00bb, 0x2026, 0x00a0, 0x0041, 0x0041, 0x004f, 0x0152, 0x0153, 
	/* 0xd0 */	0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
	/* 0xd8 */	0x0079, 0x0059, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02, 
	/* 0xe0 */	0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x0041, 0x0045, 0x0041, 
	/* 0xe8 */	0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004f, 0x004f, 
	/* 0xf0 */	0xf8ff, 0x004f, 0x0055, 0x0055, 0x0055, 0x0131, 0x02c6, 0x02dc, 
	/* 0xf8 */	0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

UniChar	gHiBitCombUnicode[128] =
{
	/* 0x80 */	0x0308, 0x030a, 0x0327, 0x0301, 0x0303, 0x0308, 0x0308, 0x0301, 
	/* 0x88 */	0x0300, 0x0302, 0x0308, 0x0303, 0x030a, 0x0327, 0x0301, 0x0300, 
	/* 0x90 */	0x0302, 0x0308, 0x0301, 0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 
	/* 0x98 */	0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 0x0300, 0x0302, 0x0308, 
	/* 0xa0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	/* 0xa8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc8 */	0x0000, 0x0000, 0x0000, 0x0300, 0x0303, 0x0303, 0x0000, 0x0000, 
	/* 0xd0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xd8 */	0x0308, 0x0308, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xe0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0302, 0x0302, 0x0301, 
	/* 0xe8 */	0x0308, 0x0300, 0x0301, 0x0302, 0x0308, 0x0300, 0x0301, 0x0302, 
	/* 0xf0 */	0x0000, 0x0300, 0x0301, 0x0302, 0x0300, 0x0000, 0x0000, 0x0000, 
	/* 0xf8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};


static OSErr MacRomanToUnicode (ConstStr255Param pascalString, ItemCount *unicodeChars, UniCharArrayPtr unicodeString)
{
	const UInt8		*p;
	UniChar			*u;
	UInt16			pascalChars;
	UInt8			c;


	p = pascalString;
	u = unicodeString;

	*unicodeChars = pascalChars = *(p++);	// pick up length byte

	while (pascalChars--) {
		c = *(p++);

		if ( (SInt8) c >= 0 ) {		// make sure its seven bit ascii
			*(u++) = (UniChar) c;		//  pad high byte with zero
		}
		else {	/* its a hi bit character */
			UniChar uc;

			c &= 0x7F;
			*(u++) = uc = gHiBitBaseUnicode[c];
			
			/*
			 * if the unicode character (uc) is an alpha char
			 * then we have an additional combining character
			 */
			if ((uc <= (UniChar) 'z') && (uc >= (UniChar) 'A')) {
				*(u++) = gHiBitCombUnicode[c];
				++(*unicodeChars);
			}
		}
	}
	
	return noErr;
}



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

#if TARGET_OS_MAC
extern void gLowerCaseTable(void);		//	Really an exported data symbol, a table
#else
extern UInt16 gLowerCaseTable[];
#endif

SInt32 FastUnicodeCompare ( register ConstUniCharArrayPtr str1, register ItemCount length1,
							register ConstUniCharArrayPtr str2, register ItemCount length2)
{
	register UInt16		c1,c2;
	register UInt16		temp;
	register UInt16*	lowerCaseTable;

#if ( FORDISKFIRSTAID || USE_TABLE_ACCESSORS)	
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


OSErr
ConvertUTF8ToUnicode(ByteCount srcLen, const unsigned char* srcStr, ByteCount maxDstLen,
					 ByteCount *actualDstLen, UniCharArrayPtr dstStr)
{
	ConversionResult result;
	UTF8* sourceStart;
	UTF8* sourceEnd;
	UTF16* targetStart;
	UTF16* targetEnd;

	sourceStart = (UTF8*) srcStr;
	sourceEnd = sourceStart + srcLen;
	targetStart = (UTF16*) dstStr;
	targetEnd = targetStart + maxDstLen/2;

	result = ConvertUTF8toUTF16 (&sourceStart, sourceEnd, &targetStart, targetEnd);
	
	*actualDstLen = (targetStart - dstStr) * sizeof(UniChar);
	
	if (result == targetExhausted)
		return kTECOutputBufferFullStatus;
	else if (result == sourceExhausted)
		return kTextMalformedInputErr;

	return noErr;
}


OSErr
ConvertUnicodeToUTF8(ByteCount srcLen, ConstUniCharArrayPtr srcStr, ByteCount maxDstLen,
					 ByteCount *actualDstLen, unsigned char* dstStr)
{
	ConversionResult result;
	UTF16* sourceStart;
	UTF16* sourceEnd;
	UTF8* targetStart;
	UTF8* targetEnd;
	ByteCount outputLength;

	sourceStart = (UTF16*) srcStr;
	sourceEnd = (UTF16*) ((char*) srcStr + srcLen);
	targetStart = (UTF8*) dstStr;
	targetEnd = targetStart + maxDstLen;
	
	result = ConvertUTF16toUTF8 (&sourceStart, sourceEnd, &targetStart, targetEnd);
	
	*actualDstLen = outputLength = targetStart - dstStr;

	if (result == targetExhausted)
		return kTECOutputBufferFullStatus;
	else if (result == sourceExhausted)
		return kTECPartialCharErr;

	if (outputLength >= maxDstLen)
		return kTECOutputBufferFullStatus;
		
	dstStr[outputLength] = 0;	/* also add null termination */

	return noErr;
}


OSErr
ConvertUTF8ToMacRoman(ByteCount srcLen, const unsigned char* srcStr, Str31 dstStr)
{
	UniChar uniStr[64];
	ByteCount uniLen;
	OSErr result;

    result = ConvertUTF8ToUnicode(srcLen, srcStr, sizeof(uniStr), &uniLen, uniStr);
	if (result == 0)
		result = UnicodeToMacRoman(uniLen / sizeof(UniChar), uniStr, dstStr);

	return result;
}


OSErr
ConvertMacRomanToUTF8(Str31 srcStr, ByteCount maxDstLen, ByteCount *actualDstLen,
					  unsigned char* dstStr)
{
	UniChar uniStr[64];
	ItemCount unicodeChars;
	OSErr result;

	result = MacRomanToUnicode(srcStr, &unicodeChars, uniStr);

	if (result == 0)
		result = ConvertUnicodeToUTF8(unicodeChars * sizeof(UniChar), uniStr,
									  maxDstLen, actualDstLen, dstStr);
	return result;
}

