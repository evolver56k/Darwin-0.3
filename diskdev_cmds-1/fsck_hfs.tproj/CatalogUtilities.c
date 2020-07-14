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
	File:		CatalogUtilities.c

	Contains:	Private Catalog Manager support routines.

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	  <CS23>	12/15/97	djb		Radar #2202860, In LocateCatalogNodeByMangledName remap
									cmParentNotFound error code to cmNotFound.
	  <CS22>	12/10/97	DSH		2201501, Pin the leof and peof to multiple of allocation blocks
									under 2 Gig.
	  <CS21>	 12/9/97	DSH		2201501, Pin returned leof values to 2^31-1 (SInt32), instead of
									2^32-1
	  <CS20>	11/26/97	djb		Radar #2005688, 2005461 - need to handle kTextMalformedInputErr.
	  <CS19>	11/25/97	djb		Radar #2002357 (again) fix new bug introduced in <CS18>.
	  <CS18>	11/17/97	djb		PrepareInputName routine now returns an error.
	  <CS17>	10/19/97	msd		Bug 1684586. GetCatInfo and SetCatInfo use only contentModDate.
	  <CS16>	10/17/97	djb		Add ConvertInputNameToUnicode for Catalog Create/Rename.
	  <CS15>	10/14/97	djb		Fix LocateCatalogNode's MakeFSSpec optimization (radar #1683166)
	  <CS14>	10/13/97	djb		Copy text encoding in CopyCatalogNodeData. Fix cut/paste error
									in VolumeHasEncodings macro. When accessing encoding bitmap use
									the MapEncodingToIndex and MapIndexToEncoding macros.
	  <CS13>	 10/1/97	djb		Remove old Catalog Iterator code...
	  <CS12>	  9/8/97	msd		Make sure a folder's modifyDate is set whenever its
									contentModDate is set.
	  <CS11>	  9/4/97	djb		Add MakeFSSpec optimization.
	  <CS10>	  9/4/97	msd		In CatalogNodeData, change attributeModDate to modifyDate.
	   <CS9>	 8/26/97	djb		Back out <CS4> (UpdateFolderCount must maintain vcbNmFls for HFS
									Plus volumes too).
	   <CS8>	 8/14/97	djb		Remove hard link support.
	   <CS7>	 7/18/97	msd		Include LowMemPriv.h.
	   <CS6>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS5>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS4>	 6/27/97	msd		UpdateFolderCount should update number of root files/folders for
									HFS volumes, not HFS Plus.
	   <CS3>	 6/24/97	djb		LocateCatalogNodeWithRetry did not always set result code.
	   <CS2>	 6/24/97	djb		Add LocateCatalogNodeByMangledName routine
	   <CS1>	 6/24/97	djb		first checked in
*/


#if TARGET_OS_MAC
#if ( PRAGMA_LOAD_SUPPORTED )
	#pragma load PrecompiledHeaders
#else
	#include	<Errors.h>
	#include	<Files.h>
	#include	<FSM.h>
	#include	<Memory.h>
	#include	<TextUtils.h>
	#include	<Types.h>
	#include	<LowMemPriv.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

#include	"FileMgrInternal.h"
#include	"BTreesInternal.h"
#include	"HFSVolumesPriv.h"
#include	"HFSUnicodeWrappers.h"
#include	"CatalogPrivate.h"


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LocateCatalogNode
//
// Function: 	Locates the catalog record for an existing folder or file
//				CNode and returns pointers to the key and data records.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
LocateCatalogNode(const ExtendedVCB *volume, CatalogNodeID folderID, const CatalogName *name,
					UInt32 hint, CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *newHint)
{
	OSErr			result;
	CatalogName 	*nodeName;
	CatalogNodeID	threadParentID;


	result = LocateCatalogRecord(volume, folderID, name, hint, keyPtr, dataPtr, newHint);
	//
	// MakeFSSpec Optimization...report a missing parent
	//
	if ( (result == cmNotFound) && (LMGetHFSFlags() & (1 << reportMissingParent)) )
	{
		CatalogNodeID	parentID;
		UInt16			dataSize;
		OSErr			err;
		Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
		
		result = cmParentNotFound;	// assume directory is missing
		
		if ( (name != NULL) && (CatalogNameLength(name, isHFSPlus) > 0) )
		{
			// Look at the previous record's parent ID to determine if the
			// parent of the CNode we were looking for actually exists
	
			err = GetBTreeRecord(volume->catalogRefNum, -1, keyPtr, dataPtr, &dataSize, newHint);
			if (err == noErr)
			{
				if ( isHFSPlus )
					parentID = keyPtr->large.parentID;
				else
					parentID = keyPtr->small.parentID;
				
				if ( folderID == parentID )		// did parent exists?
					result = cmNotFound;	// yes, so report a missing file to MakeFSSpec
			}
		}
	}

	ReturnIfError(result);
	
	// if we got a thread record, then go look up real record
	switch ( dataPtr->recordType )
	{
		case kSmallFileThreadRecord:
		case kSmallFolderThreadRecord:
			threadParentID = dataPtr->smallThread.parentID;
			nodeName = (CatalogName *) &dataPtr->smallThread.nodeName;
			break;

		case kLargeFileThreadRecord:
		case kLargeFolderThreadRecord:
			threadParentID = dataPtr->largeThread.parentID;
			nodeName = (CatalogName *) &dataPtr->largeThread.nodeName;	
			break;

		default:
			threadParentID = 0;
			break;
	}
	
	if ( threadParentID )		// found a thread
		result = LocateCatalogRecord(volume, threadParentID, nodeName, kNoHint, keyPtr, dataPtr, newHint);
	
	return result;
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LocateCatalogNodeWithRetry
//
// Function: 	Locates the catalog record for an existing folder or file node.
//				For HFS Plus volumes a retry is performed when a catalog node is
//				not found and the volume contains more than one text encoding.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

#define VolumeHasEncodings(v) \
	( ((v)->encodingsBitmap.lo | (v)->encodingsBitmap.hi) != 0 )

#define	EncodingInstalled(i) \
	( (fsVars)->gConversionContext[(i)].toUnicode != 0 )

#define	EncodingUsedByVolume(v,i) \
	( i < 32 ? ((v)->encodingsBitmap.lo & (1 << (i))) : ((v)->encodingsBitmap.hi & (1 << (i-32))) )


OSErr
LocateCatalogNodeWithRetry (const ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param pascalName, CatalogName *unicodeName,
							UInt32 hint, CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *newHint)
{
	CatalogDataCache	*catalogCache;
	TextEncoding		defaultEncoding;
	TextEncoding		cachedEncoding = kTextEncodingUndefined;
	TextEncoding		encoding;
	ItemCount			encodingsToTry;
	FSVarsRec			*fsVars;
	OSErr				result = cmNotFound;


	fsVars = (FSVarsRec*) LMGetFSMVars();	// used by macros
	catalogCache = (CatalogDataCache *) volume->catalogDataCache;

	defaultEncoding = GetDefaultTextEncoding();
	encodingsToTry = CountInstalledEncodings();

	// if we already have the last name, just go get the record...
	if ( pascalName != NULL && PascalBinaryCompare(pascalName, catalogCache->lastNamePascal) )
	{
		--encodingsToTry;
		cachedEncoding = catalogCache->lastEncoding;

		result = LocateCatalogNode(volume, folderID, unicodeName, hint, keyPtr, dataPtr, newHint);
		if ( result != cmNotFound || encodingsToTry == 0)
			return result;
	}
	else
	{
		// HFS Plus Optimization: save a copy of the latest conversion...
		// Unicode string will be auto saved (since its also our string buffer)

		CopyCatalogName((CatalogName*) pascalName, (CatalogName*) catalogCache->lastNamePascal, false);	// save Pascal string
	}

	// 1. Try finding file using default encoding (typical case)

	if ( defaultEncoding != cachedEncoding )	// if our cached name was not based on the default...
	{
		--encodingsToTry;
		catalogCache->lastEncoding = defaultEncoding;
		result = PrepareInputName(pascalName, true, defaultEncoding, unicodeName);
		if (result == noErr)
			result = LocateCatalogNode(volume, folderID, unicodeName, hint, keyPtr, dataPtr, newHint);
		else
			result = cmNotFound;

		if ( result != cmNotFound || encodingsToTry == 0)
			return result;
	}

	//
	//€€ if the pascal string contains all 7-bit ascii then we don't need to do anymore retries
	//


	// 2. Try finding file using Mac Roman (if not already tried above)

	if ( (defaultEncoding != kTextEncodingMacRoman) && (cachedEncoding != kTextEncodingMacRoman) )
	{
		--encodingsToTry;
		catalogCache->lastEncoding = kTextEncodingMacRoman;
		result = PrepareInputName(pascalName, true, kTextEncodingMacRoman, unicodeName);
		if (result == noErr)
			result = LocateCatalogNode(volume, folderID, unicodeName, hint, keyPtr, dataPtr, newHint);
		else
			result = cmNotFound;

		if ( result != cmNotFound || encodingsToTry == 0 )
			return result;
	}

	// 3. Try with encodings from disk (if any)

	if ( VolumeHasEncodings(volume) )	// any left to try?
	{
		UInt32	index;

		index = 0;	// since we pre increment this will skip MacRoman (which was already tried above)

		while ( index < kMacBaseEncodingCount )
		{
			++index;
			
			encoding = MapIndexToEncoding(index);

			if ( encoding == defaultEncoding || encoding == cachedEncoding )
				continue;		// we did this one already

			if ( EncodingInstalled(index) && EncodingUsedByVolume(volume, index) )
			{
				--encodingsToTry;
				catalogCache->lastEncoding = encoding;
				result = PrepareInputName(pascalName, true, encoding, unicodeName);
				if (result == noErr)
					result = LocateCatalogNode(volume, folderID, unicodeName, hint, keyPtr, dataPtr, newHint);
				else
					result = cmNotFound;

				if ( result != cmNotFound || encodingsToTry == 0 )
					return result;
			}
		}
	}

	// 4. Try any remaining encodings (if any)

	{
		UInt32	index;

		index = 0;	// since we pre increment this will skip MacRoman (which was already tried above)
	
		while ( (encodingsToTry > 0) && (index < kMacBaseEncodingCount) )
		{
			++index;
	
			encoding = MapIndexToEncoding(index);
	
			if ( encoding == defaultEncoding || encoding == cachedEncoding )
				continue;		// we did this one already
	
			if ( EncodingInstalled(index) && EncodingUsedByVolume(volume, index) == false )
			{
				--encodingsToTry;
				catalogCache->lastEncoding = encoding;
				result = PrepareInputName(pascalName, true, encoding, unicodeName);
				if (result == noErr)
					result = LocateCatalogNode(volume, folderID, unicodeName, hint, keyPtr, dataPtr, newHint);
				else
					result = cmNotFound;

				if ( result != cmNotFound || encodingsToTry == 0 )
					return result;
			}
		}
	}

	return cmNotFound;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LocateCatalogNodeByMangledName
//
// Function: 	Locates the catalog record associated with a mangled name (if any)
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
LocateCatalogNodeByMangledName( const ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param name,
								CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *hintPtr )
{
	CatalogNodeID 	fileID;
	Str31			nodeName;
	OSErr			result;


	fileID = GetEmbeddedFileID(name);

	if ( fileID == 0 )
		return cmNotFound;
		
	result = LocateCatalogNode(volume, fileID, NULL, kNoHint, keyPtr, dataPtr, hintPtr);
	if ( result == cmParentNotFound )	// GetCatalogNode already handled cmParentNotFound case 	<CS23>
		result = cmNotFound;			// so remap													<CS23>
	ReturnIfError(result);
		
	// first make sure that the parents match
	if ( folderID != keyPtr->large.parentID )
		return cmNotFound;			// not the same folder so this is a false match

	(void) ConvertUnicodeToHFSName( &keyPtr->large.nodeName, dataPtr->largeFile.textEncoding,
									dataPtr->largeFile.fileID, nodeName );
		
	if ( !PascalBinaryCompare(nodeName, name) )
		return cmNotFound;	// mangled names didn't match so this is a false match
	
	return noErr;	// we found it
}

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LocateCatalogRecord
//
// Function: 	Locates the catalog record associated with folderID and name
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
LocateCatalogRecord(const ExtendedVCB *volume, CatalogNodeID folderID, const CatalogName *name,
					UInt32 hint, CatalogKey *keyPtr, CatalogRecord *dataPtr, UInt32 *newHint)
{
	OSErr			result;
	CatalogKey		tempKey;
	UInt16			tempSize;

	BuildCatalogKey(folderID, name, (volume->vcbSigWord == kHFSPlusSigWord), &tempKey);

	if ( name == NULL )
		hint = kNoHint;			// no CName given so clear the hint

	result = SearchBTreeRecord(volume->catalogRefNum, &tempKey, hint, keyPtr, dataPtr, &tempSize, newHint);
	
	return (result == btNotFound ? cmNotFound : result);	
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LocateCatalogThread
//
//	Function: 	Locates a catalog thread record in the catalog BTree file and 
//				returns a pointer to the data record.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
LocateCatalogThread(const ExtendedVCB *volume, CatalogNodeID nodeID, CatalogRecord *threadData, UInt16 *threadSize, UInt32 *threadHint)
{
	CatalogKey	threadKey;
	OSErr		result;
	CatalogKey	key;

	//--- build key record

	BuildCatalogKey(nodeID, NULL, (volume->vcbSigWord == kHFSPlusSigWord), &threadKey);

	//--- locate thread record in BTree

	result = SearchBTreeRecord( volume->catalogRefNum, &threadKey, kNoHint, &key,
								threadData, threadSize, threadHint);
	
	return (result == btNotFound ? cmNotFound : result);	
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	BuildCatalogKey
//
//	Function: 	Constructs a catalog key record (ckr) given the parent
//				folder ID and CName.  Works for both classic and extended
//				HFS volumes.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

void
BuildCatalogKey(CatalogNodeID parentID, const CatalogName *cName, Boolean isHFSPlus, CatalogKey *key)
{
	if ( isHFSPlus )
	{
		key->large.keyLength		= kLargeCatalogKeyMinimumLength;	// initial key length (4 + 2)
		key->large.parentID			= parentID;		// set parent ID
		key->large.nodeName.length	= 0;			// null CName length
		if ( cName != NULL )
		{
			CopyCatalogName(cName, (CatalogName *) &key->large.nodeName, isHFSPlus);
			key->large.keyLength += sizeof(UniChar) * cName->ustr.length;	// add CName size to key length
		}
	}
	else
	{
		key->small.keyLength	= kSmallCatalogKeyMinimumLength;	// initial key length (1 + 4 + 1)
		key->small.reserved		= 0;				// clear unused byte
		key->small.parentID		= parentID;			// set parent ID
		key->small.nodeName[0]	= 0;				// null CName length
		if ( cName != NULL )
		{
			UpdateCatalogName(cName->pstr, key->small.nodeName);
			key->small.keyLength += key->small.nodeName[0];		// add CName size to key length
		}
	}
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	FlushCatalog
//
// Function: 	Flushes the catalog for a specified volume.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
FlushCatalog(ExtendedVCB *volume)
{
	OSErr	result;
	
	result = FlushBTree(volume->catalogRefNum);

	if (result == noErr)
	{
		//--- check if catalog's fcb is dirty...
		
		if ( GetFileControlBlock(volume->catalogRefNum)->fcbFlags & fcbModifiedMask )
		{
			volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
			volume->vcbLsMod = GetTimeLocal();	// update last modified date
			result = FlushVolumeControlBlock(volume);
		}
	}
	
	return result;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	InvalidateCatalogNodeCache
//
//	Function: 	Invalidates the Catalog node cache if volume and folderID match
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
InvalidateCatalogNodeCache(ExtendedVCB *volume, CatalogNodeID parentID)
{
	FSVarsRec	*fsVars;

	fsVars = (FSVarsRec*) LMGetFSMVars();
	
	if ( (fsVars->gCatalogFSSpec.parID == parentID) && (volume->vcbVRefNum == fsVars->gCatalogFSSpec.vRefNum) )
	{
		fsVars->gCatalogFSSpec.parID = 0;
		fsVars->gCatalogFSSpec.vRefNum = 0;
	}
}

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	UpdateCatalogName
//
//	Function: 	Updates a CName.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

void
UpdateCatalogName(ConstStr31Param srcName, Str31 destName)
{
	Size length = srcName[0];
	
	if (length > CMMaxCName)
		length = CMMaxCName;				// truncate to max

	destName[0] = length;					// set length byte
	
	BlockMoveData(&srcName[1], &destName[1], length);
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	AdjustVolumeCounts
//
//	Function: 	Adjusts the folder and file counts in the VCB
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

void
AdjustVolumeCounts(ExtendedVCB *volume, SInt16 type, SInt16 delta)
{
	//€€ also update extended VCB fields...

	if (type == kSmallFolderRecord || type == kLargeFolderRecord)
		volume->vcbDirCnt += delta;			// adjust volume folder count, €€ worry about overflow?
	else
		volume->vcbFilCnt += delta;			// adjust volume file count
	
	volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
	volume->vcbLsMod = GetTimeLocal();	// update last modified date
}

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

void
UpdateVolumeEncodings(ExtendedVCB *volume, TextEncoding encoding)
{
	UInt32	index;

	encoding &= 0x7F;
	
	index = MapEncodingToIndex(encoding);

	if ( index < 32 )
		volume->encodingsBitmap.lo |= (1 << index);
	else
		volume->encodingsBitmap.hi |= (1 << (index - 32));
		
	// vcb should already be marked dirty
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
UpdateFolderCount( ExtendedVCB *volume, CatalogNodeID parentID, const CatalogName *name, SInt16 newType,
					UInt32 hint, SInt16 valenceDelta)
{
	CatalogKey		tempKey;
	CatalogRecord	tempData;
	UInt32			tempHint;
	CatalogNodeID	folderID;
	UInt16			recordSize;
	OSErr			result;


	result = LocateCatalogNode(volume, parentID, name, hint, &tempKey, &tempData, &tempHint);
	ReturnIfError(result);

	if ( volume->vcbSigWord == kHFSPlusSigWord ) // HFS Plus
	{
		UInt32		timeStamp;
		
		if ( DEBUG_BUILD && tempData.recordType != kLargeFolderRecord )
			DebugStr("\p UpdateFolder: found HFS folder on HFS+ volume!");

		timeStamp = GetTimeUTC();
		tempData.largeFolder.valence += valenceDelta;		// adjust valence
		tempData.largeFolder.contentModDate = timeStamp;	// set date/time last modified
		folderID = tempData.largeFolder.folderID;
		recordSize = sizeof(tempData.largeFolder);
	}
	else // classic HFS
	{
		if ( DEBUG_BUILD && tempData.recordType != kSmallFolderRecord )
			DebugStr("\p UpdateFolder: found HFS+ folder on HFS volume!");

		tempData.smallFolder.valence += valenceDelta;				// adjust valence
		tempData.smallFolder.modifyDate = GetTimeLocal();			// set date/time last modified
		folderID = tempData.smallFolder.folderID;
		recordSize = sizeof(tempData.smallFolder);
	}
	
	result = ReplaceBTreeRecord(volume->catalogRefNum, &tempKey, tempHint, &tempData, recordSize, &tempHint);
	ReturnIfError(result);

	if ( folderID == kHFSRootFolderID )
	{
		if (newType == kSmallFolderRecord || newType == kLargeFolderRecord)
			volume->vcbNmRtDirs += valenceDelta;				// adjust root folder count (undefined for HFS Plus)
		else
			volume->vcbNmFls += valenceDelta;					// adjust root file count (used by GetVolInfo)
	}

	//€€ also update extended VCB fields...

	return result;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

UInt16	
GetCatalogRecordSize(const CatalogRecord *dataRecord)
{
	switch (dataRecord->recordType)
	{
		case kSmallFileRecord:
			return sizeof(SmallCatalogFile);

		case kSmallFolderRecord:
			return sizeof(SmallCatalogFolder);

		case kLargeFileRecord:
			return sizeof(LargeCatalogFile);

		case kLargeFolderRecord:
			return sizeof(LargeCatalogFolder);
			
		case kSmallFolderThreadRecord:
		case kSmallFileThreadRecord:
			return sizeof(SmallCatalogThread);

		case kLargeFolderThreadRecord:
		case kLargeFileThreadRecord:
			return sizeof(LargeCatalogThread);

		default:
			return 0;
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

void
CopyCatalogNodeData(const ExtendedVCB *volume, const CatalogRecord *dataPtr, CatalogNodeData *nodeData)
{	
	switch (dataPtr->recordType)
	{
		case kSmallFolderRecord:
		{
			nodeData->nodeType			= kCatalogFolderNode;
			nodeData->nodeFlags			= dataPtr->smallFolder.flags;
			nodeData->nodeID			= dataPtr->smallFolder.folderID;
			nodeData->createDate		= dataPtr->smallFolder.createDate;
			nodeData->contentModDate	= dataPtr->smallFolder.modifyDate;
			nodeData->backupDate		= dataPtr->smallFolder.backupDate;
			nodeData->valence			= dataPtr->smallFolder.valence;

			BlockMoveData(&dataPtr->smallFolder.userInfo, &nodeData->finderInfo, 32);		// copy 32 bytes of finder data	
			break;
		}

		case kSmallFileRecord:
		{
			UInt32	i;

			nodeData->nodeType			= kCatalogFileNode;
			nodeData->nodeFlags			= dataPtr->smallFile.flags;
			nodeData->nodeID			= dataPtr->smallFile.fileID;
			nodeData->createDate		= dataPtr->smallFile.createDate;
			nodeData->contentModDate	= dataPtr->smallFile.modifyDate;
			nodeData->backupDate		= dataPtr->smallFile.backupDate;
			nodeData->valence			= 0;
	
			BlockMoveData(&dataPtr->smallFile.userInfo, &nodeData->finderInfo, 16);		// copy finder data
			BlockMoveData(&dataPtr->smallFile.finderInfo, &nodeData->extFinderInfo, 16);	// copy finder data

			nodeData->dataLogicalSize  = dataPtr->smallFile.dataLogicalSize;
			nodeData->dataPhysicalSize = dataPtr->smallFile.dataPhysicalSize;
			nodeData->rsrcLogicalSize  = dataPtr->smallFile.rsrcLogicalSize;
			nodeData->rsrcPhysicalSize = dataPtr->smallFile.rsrcPhysicalSize;

			for (i = 0; i < kSmallExtentDensity; ++i)
			{
				nodeData->dataExtents[i].startBlock = (UInt32) (dataPtr->smallFile.dataExtents[i].startBlock);
				nodeData->dataExtents[i].blockCount = (UInt32) (dataPtr->smallFile.dataExtents[i].blockCount);

				nodeData->rsrcExtents[i].startBlock = (UInt32) (dataPtr->smallFile.rsrcExtents[i].startBlock);
				nodeData->rsrcExtents[i].blockCount = (UInt32) (dataPtr->smallFile.rsrcExtents[i].blockCount);
			}
			break;
		}

		case kLargeFolderRecord:
		{
			nodeData->nodeType			= kCatalogFolderNode;
			nodeData->nodeFlags			= dataPtr->largeFolder.flags;
			nodeData->nodeID			= dataPtr->largeFolder.folderID;
			nodeData->textEncoding		= dataPtr->largeFolder.textEncoding;
			nodeData->createDate		= UTCToLocal(dataPtr->largeFolder.createDate);
			nodeData->contentModDate	= UTCToLocal(dataPtr->largeFolder.contentModDate);
			nodeData->backupDate		= UTCToLocal(dataPtr->largeFolder.backupDate);
			if (dataPtr->largeFolder.valence > 0xffff)
				nodeData->valence = 0xffff;		// pass maximum 16-bit value
			else
				nodeData->valence = dataPtr->largeFolder.valence;

			BlockMoveData(&dataPtr->largeFolder.userInfo, &nodeData->finderInfo, 32);	// copy finder data
			break;
		}

		case kLargeFileRecord:
		{
			UInt32	largestFileSizeUnder2Gig;
			
			nodeData->nodeType			= kCatalogFileNode;
			nodeData->nodeFlags			= dataPtr->largeFile.flags;
			nodeData->nodeID			= dataPtr->largeFile.fileID;
			nodeData->textEncoding		= dataPtr->largeFile.textEncoding;
			nodeData->createDate		= UTCToLocal(dataPtr->largeFile.createDate);
			nodeData->contentModDate	= UTCToLocal(dataPtr->largeFile.contentModDate);
			nodeData->backupDate		= UTCToLocal(dataPtr->largeFile.backupDate);
			nodeData->valence			= 0;

			BlockMoveData(&dataPtr->largeFile.userInfo, &nodeData->finderInfo, 32);	// copy finder data
		
			//	2201501, Pin values to 2^31-1 (SInt32)
			largestFileSizeUnder2Gig	=  (0x7FFFFFFF / volume->blockSize) * volume->blockSize;
			
			if ( (dataPtr->largeFile.dataFork.logicalSize.hi == 0) && (dataPtr->largeFile.dataFork.logicalSize.lo <= largestFileSizeUnder2Gig) )
			{
				nodeData->dataLogicalSize	= dataPtr->largeFile.dataFork.logicalSize.lo;
				nodeData->dataPhysicalSize	= (dataPtr->largeFile.dataFork.totalBlocks * volume->blockSize);
			}
			else
			{
				nodeData->dataPhysicalSize	=  largestFileSizeUnder2Gig;
				nodeData->dataLogicalSize	=  nodeData->dataPhysicalSize;		// signal to Open that this file is too big
				nodeData->valence			|= kLargeDataForkMask;				// overload the valence for files over 2Gig
			}

			if ( (dataPtr->largeFile.resourceFork.logicalSize.hi == 0) && (dataPtr->largeFile.resourceFork.logicalSize.lo <= largestFileSizeUnder2Gig) )
			{
				nodeData->rsrcLogicalSize	= dataPtr->largeFile.resourceFork.logicalSize.lo;
				nodeData->rsrcPhysicalSize	= (SInt32) ((SInt32)dataPtr->largeFile.resourceFork.totalBlocks * (SInt32)volume->blockSize);
			}
			else
			{
				nodeData->rsrcPhysicalSize	=  largestFileSizeUnder2Gig;
				nodeData->rsrcLogicalSize	=  nodeData->rsrcPhysicalSize;		// signal to Open that this file is too big
				nodeData->valence			|= kLargeRsrcForkMask;					// overload the valence for files over 2Gig
			}

			// copy data and rsrc extents
			BlockMoveData(&dataPtr->largeFile.dataFork.extents, &nodeData->dataExtents, sizeof(LargeExtentRecord));
			BlockMoveData(&dataPtr->largeFile.resourceFork.extents, &nodeData->rsrcExtents, sizeof(LargeExtentRecord));
			break;
		}
		
		default:
			nodeData->nodeType = '????';	// must have hit a thread record
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
PrepareInputName(ConstStr31Param name, Boolean unicode, TextEncoding encoding, CatalogName *catalogName)
{
	OSErr	result = noErr;
	
	if (name == NULL)
	{
		catalogName->ustr.length = 0;	// set length byte (works for both unicode and pascal)
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
		else if (unicode)
		{
			Str31		truncatedName;
			StringPtr	pName;
	
			if (name[0] <= CMMaxCName)	//€€ should be CMMaxCName
			{
				pName = (StringPtr)name;
			}
			else
			{
				BlockMoveData(&name[1], &truncatedName[1], CMMaxCName);
				truncatedName[0] = CMMaxCName;
				pName = truncatedName;
			}
	
			//€€ need to pass pascal string length (could be clipped above)
			result = ConvertHFSNameToUnicode (	pName, encoding, &catalogName->ustr );
		}
		else
		{
			BlockMoveData(&name[1], &catalogName->pstr[1], length);
			catalogName->pstr[0] = length;					// set length byte (might be smaller than name[0]
		}
	}
	
	return result;
}


//_______________________________________________________________________

void
ConvertInputNameToUnicode(ConstStr31Param name, TextEncoding encodingHint, TextEncoding *actualEncoding, CatalogName *catalogName)
{
	Size		length;
	OSErr		result;
	
	catalogName->ustr.length = 0;	// setup defaults...
	*actualEncoding = encodingHint;

	if (name == NULL)
		return;		// catalogName is empty string

	length = name[0];

	if (length == 0 || length > CMMaxCName)
		return;		// catalogName is empty string

	result = ConvertHFSNameToUnicode ( name, encodingHint, &catalogName->ustr );
	
	if (result != noErr)
	{
		// The converter didn't like our string, try again with roman encoding
		// This takes care of errors like:	kTextMalformedInputErr
		//									kTECPartialCharErr
		//									kTECUnmappableElementErr
		//									kTECIncompleteElementErr
		//
		if ( encodingHint != kTextEncodingMacRoman )
		{
			*actualEncoding = kTextEncodingMacRoman;
			result = ConvertHFSNameToUnicode ( name, kTextEncodingMacRoman, &catalogName->ustr );
			
		}

		if (DEBUG_BUILD && result != noErr)
			DebugStr("\p ConvertHFSNameToUnicode could not convert string!");
	}
}

#endif

//_______________________________________________________________________

void
CopyCatalogName(const CatalogName *srcName, CatalogName *dstName, Boolean isHFSPLus)
{
	UInt32	length;
	
	if ( srcName == NULL )
	{
		if ( dstName != NULL )
			dstName->ustr.length = 0;	// set length byte to zero (works for both unicode and pascal)		
		return;
	}
	
	if (isHFSPLus)
		length = sizeof(UniChar) * (srcName->ustr.length + 1);
	else
		length = sizeof(UInt8) + srcName->pstr[0];

	if ( length > 1 )
		BlockMoveData(srcName, dstName, length);
	else
		dstName->ustr.length = 0;	// set length byte to zero (works for both unicode and pascal)		
}


//_______________________________________________________________________

UInt32
CatalogNameLength(const CatalogName *name, Boolean isHFSPlus)
{
	if (isHFSPlus)
		return name->ustr.length;
	else
		return name->pstr[0];
}

