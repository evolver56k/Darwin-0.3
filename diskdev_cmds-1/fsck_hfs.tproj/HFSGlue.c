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
	File:		HFSGlue.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn

	Change History (most recent first):

	  <HFS9>	 12/2/97	DSH		Unicode related routines for DFA
	  <HFS8>	11/18/97	DSH		2002639, fix simple unicode conversions.
	  <HFS7>	10/21/97	DSH		Remove CheckNode
	  <HFS6>	 8/20/97	DSH		M_DebugStr is a macro to do nothing or a DebugStr.
	  <HFS5>	 7/17/97	DSH		FilesInternal.i renamed to FileMgrInternal.i to avoid name
									collision.
	  <HFS4>	 7/10/97	DSH		Match InitializeUnicode to new API
	  <HFS3>	  7/9/97	DSH		Match InitializeUnicode() to new API
	  <HFS2>	 6/26/97	DSH		Added DFA_PrepareInputName() and some stub unicode conversion
									routines needed for linking.
*/

#include 	<FileMgrInternal.h>
#include	"HFSUnicodeWrappers.h"
#include	"BTreesPrivate.h"
#include    "ScavDefs.h"
#include	"Prototypes.h"


#ifdef INVESTIGATE

#pragma segment Glue



#define	fsBusyMask	0x0100
#define	fsAsyncMask	0x0400

void MarkFileSystemFree();
//€€ No longer needed
//#define MarkFileSystemFree()		( ((QHdrPtr)(LMGetFSQHdr()))->qFlags &= ~fsBusyMask )
#define MarkFileSystemFree()		;


//€€ No longer needed
#define MarkFileSystemBusy()		;
#if ( 0 )
	static void	MarkFileSystemBusy( void )
	{
		return;			// Just EXIT
		
		QHdrPtr	fsQHead = LMGetFSQHdr();
		
		while( (fsBusyMask & fsQHead->qFlags) != 0 )
		{
			;
		}
		
		fsQHead->qFlags |= fsBusyMask;
	//	fsQHead->qFlags |= ~fsAsyncMask;			//	async bit mask
	}
#endif

#endif

//		Defined in BTreesPrivate.h, but not implemented in the BTree code?
//		So... here's the implementation
SInt32	CompareKeys( BTreeControlBlockPtr btreePtr, KeyPtr searchKey, KeyPtr trialKey )
{
	KeyCompareProcPtr	compareProc = (KeyCompareProcPtr)btreePtr->keyCompareProc;
	
	return( compareProc(searchKey, trialKey) );
}

#ifdef INVESTIGATE

OSErr	GetBlock_FSGlue( int flags, UInt32 block, Ptr *buffer, int refNum, ExtendedVCB *vcb )
{
	OSErr	err;
	MarkFileSystemBusy();
	err = GetBlock_glue( flags, block, buffer, refNum, vcb );
	MarkFileSystemFree();
	return( err );
}


OSErr	RelBlock_FSGlue( Ptr buffer, int flags )
{
	OSErr	err;
	MarkFileSystemBusy();
	err = RelBlock_glue( buffer, flags );
	MarkFileSystemFree();
	return( err );
}


void DFA_PrepareInputName(ConstStr31Param name, Boolean isHFSPlus, CatalogName *catalogName)
{
	if (name == NULL)
	{
		catalogName->ustr.length = 0;									// set length byte (works for both unicode and pascal)
	}
	else
	{
		Size length = name[0];
	
		if (length > CMMaxCName)
			length = CMMaxCName;	// truncate to max
			
		if ( length == 0 )
		{
			catalogName->ustr.length = 0;
		}
		else if ( isHFSPlus )
		{
			Str31		truncatedName;
			StringPtr	pName;
			OSErr		result;
			ByteCount	actualUnicodeLen;
			
			if (name[0] <= CMMaxCName)
			{
				pName = (StringPtr)name;
			}
			else
			{
				BlockMoveData(&name[1], &truncatedName[1], CMMaxCName);
				truncatedName[0] = CMMaxCName;
				pName = truncatedName;
			}
	
			result = MockConvertFromPStringToUnicode( (ConstStr31Param)pName, sizeof(UniStr255), &actualUnicodeLen, catalogName->ustr.unicode ); 
			catalogName->ustr.length = actualUnicodeLen / sizeof(UniChar);	// Note: from byte count to char count
			if(result)
				M_DebugStr("\p DFA_PrepareInputName: ConvertPStringToUnicode returned an error!");

		}
		else
		{
			BlockMoveData(&name[1], &catalogName->pstr[1], length);
			catalogName->pstr[0] = length;								// set length byte (might be smaller than name[0]
		}
	}
}

#endif

UInt32	CatalogNameSize( const CatalogName *name, Boolean isHFSPlus)
{
	UInt32	length = CatalogNameLength( name, isHFSPlus );
	
	if ( isHFSPlus )
		length *= sizeof(UniChar);
	
	return( length );
}

#ifdef INVESTIGATE

OSStatus	InitializeUnicode(StringPtr TECFileName)
{
	M_DebugStr("\p Bummer, InitializeUnicode called from within DFA");
	return( fnfErr );
}

pascal OSStatus ConvertFromUnicodeToText(UnicodeToTextInfo iUnicodeToTextInfo, ByteCount iUnicodeLen, ConstUniCharArrayPtr iUnicodeStr, OptionBits iControlFlags, ItemCount iOffsetCount, ByteOffset iOffsetArray[], ItemCount *oOffsetCount, ByteOffset oOffsetArray[], ByteCount iBufLen, ByteCount *oInputRead, ByteCount *oOutputLen, LogicalAddress oOutputStr)
{
	M_DebugStr("\p Bummer, ConvertFromUnicodeToText called from within DFA");
	return( fnfErr );
}

pascal OSStatus ConvertFromTextToUnicode(TextToUnicodeInfo iTextToUnicodeInfo, ByteCount iSourceLen, ConstLogicalAddress iSourceStr, OptionBits iControlFlags, ItemCount iOffsetCount, ByteOffset iOffsetArray[], ItemCount *oOffsetCount, ByteOffset oOffsetArray[], ByteCount iBufLen, ByteCount *oSourceRead, ByteCount *oUnicodeLen, UniCharArrayPtr oUnicodeStr)
{
	M_DebugStr("\p Bummer, ConvertFromTextToUnicode called from within DFA");
	return( fnfErr );
}

pascal OSStatus UpgradeScriptInfoToTextEncoding(ScriptCode textScriptID, LangCode textLanguageID, RegionCode regionID, ConstStr255Param textFontname, TextEncoding *encoding)
{
	M_DebugStr("\p Bummer, UpgradeScriptInfoToTextEncoding called from within DFA");
	return( fnfErr );
}

OSStatus LockMappingTable(UnicodeMapping *unicodeMappingPtr, Boolean lockIt)
{
	M_DebugStr("\p Bummer, LockMappingTable called from within DFA");
	return( fnfErr );
}

pascal OSStatus CreateTextToUnicodeInfo(ConstUnicodeMappingPtr iUnicodeMapping, TextToUnicodeInfo *oTextToUnicodeInfo)
{
	M_DebugStr("\p Bummer, CreateTextToUnicodeInfo called from within DFA");
	return( fnfErr );
}

pascal OSStatus CreateUnicodeToTextInfo(ConstUnicodeMappingPtr iUnicodeMapping, UnicodeToTextInfo *oUnicodeToTextInfo)
{
	M_DebugStr("\p Bummer, CreateUnicodeToTextInfo called from within DFA");
	return( fnfErr );
}

OSErr	ConvertHFSNameToUnicode( ConstStr31Param hfsName, TextEncoding encoding, UniStr255 *unicodeName )
{
	unicodeName->length	= 0;
	return( 234 );
}

OSErr	ConvertUnicodeToHFSName( ConstUniStr255Param unicodeName, TextEncoding encoding, CatalogNodeID cnid, Str31 hfsName )
{
	hfsName[0]	= 0;
	return( 234 );
}

#endif