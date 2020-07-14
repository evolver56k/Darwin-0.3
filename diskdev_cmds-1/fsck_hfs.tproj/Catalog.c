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
	File:		Catalog.c

	Contains:	Catalog Manager Implementation

	Version:	HFS Plus 1.0

	Copyright:	© 1996-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			xxx put technology here xxx

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	  <CS36>	12/10/97	DSH		2201501, UpdateCatalogNode to only update CatalogRecords which
									are under 2 Gig by checking the overloaded valence field.
	  <CS35>	11/20/97	djb		Radar #2002357. Fixing retry mechanism.
	  <CS34>	11/17/97	djb		PrepareInputName routine now returns an error.
	  <CS33>	11/13/97	djb		Radar #1683572. Add new GetCatalogOffspringFile routine for
									PBGetFileInfo calls (support used to be in HFSPathnameCalls.a).
	  <CS32>	 11/7/97	msd		Change calls to the wrapper routine CompareUnicodeNames() to use
									the underlying routine FastUnicodeCompare() instead.
	  <CS31>	10/19/97	msd		Bug 1684586. GetCatInfo and SetCatInfo use only contentModDate.
	  <CS30>	10/17/97	djb		Change Catalog Create/Rename to use ConvertInputNameToUnicode.
	  <CS29>	10/13/97	djb		Update volumeNameEncodingHint when changing volume name. Change
									name of GetSystemTextEncoding to GetDefaultTextEncoding.
	  <CS28>	 10/1/97	djb		Add new catalog iterators and node cache to improve performance.
	  <CS27>	 9/12/97	msd		In CreateCatalogNode, make sure parent is a folder, not a file.
	  <CS26>	 9/10/97	msd		In RenameCatalogNodeUnicode, remove HFS-only code and make sure
									the conversion context is set up and marked in the volume's
									bitmap.
	  <CS25>	  9/9/97	DSH		Added RelString_Glue to avoid having to link DFAEngine with
									Interface.o
	  <CS24>	  9/8/97	msd		Make sure a folder's modifyDate is set whenever its
									contentModDate is set. In UpdateCatalogNode, make sure the
									modifyDate is greater or equal to contentModDate; do a DebugStr
									only for debug builds.
	  <CS23>	  9/7/97	djb		Make some DebuStrs DEBUG_BUILD only.
	  <CS22>	  9/4/97	djb		Add more Catalog Iterators, Instrument RelString.
	  <CS21>	  9/4/97	msd		Remove call to PropertyDeleteObject.
	  <CS20>	 8/18/97	DSH		Use RelString instead of FastRelString in DFA to avoid loading
									branch island instead of table.
	  <CS19>	 8/14/97	djb		Remove hard link support. Switch over to FastRelString.
	  <CS18>	  8/8/97	djb		Fixed bugs in LinkCatalogNode.
	  <CS17>	  8/5/97	djb		Don't restore vcbNxtCNID if thread exists (radar #1670614).
	  <CS16>	 7/25/97	DSH		Pass heuristicHint to BTSearchRecord from GetCatalogOffspring.
	  <CS15>	 7/18/97	msd		Include LowMemPriv.h. In LinkCatalogNode, now sets the
									kInsertedFileThread2 flag correctly; should only affect error
									recovery code.
	  <CS14>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	  <CS13>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	  <CS12>	 6/27/97	msd		Add PBLongRename SPI. Added RenameCatalogNodeUnicode call, which
									takes Unicode names for HFS Plus volumes. Removed calls to
									Attributes module when creating, renaming or moving nodes.
	  <CS11>	 6/24/97	djb		Validate the mangled name matches in
									LocateCatalogNodeByMangledName.
	  <CS10>	 6/24/97	djb		Add hard link support.
	   <CS9>	 6/20/97	msd		Use contentModDate and attributeModDate fields instead of
									modifyDate. Made CopyCatalogNodeData public.
	   <CS8>	 6/18/97	djb		Add routines LocateCatalogNodeWithRetry & UpdateVolumeEncodings.
									Add mangled name retry to DeleteCatalogNode, MoveCatalogNode and
									RenameCatalogNode.
	   <CS7>	 6/13/97	djb		Major changes for longname support and multiple scripts.
	   <CS6>	  6/9/97	msd		Instead of calling GetDateTime, call GetTimeUTC or GetTimeLocal.
									Dates on an HFS Plus volume need to be converted to/from UTC.
	   <CS5>	  6/4/97	djb		Set textEncoding hint in Rename and Create. TrashCatalogIterator
									was not always called with the correct folder ID.
	   <CS4>	 5/21/97	djb		Turn off recursive iterators.
	   <CS3>	 5/19/97	djb		Add support for B-tree iterators to GetCatalogOffspring.
	   <CS2>	  5/9/97	djb		Get in sync with FilesInternal.i.
	   <CS1>	 4/24/97	djb		First checked into Common System Project.
	 <HFS26>	 4/11/97	DSH		Use extended VCB fields catalogRefNum, and extentsRefNum.
	 <HFS25>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS24>	 3/31/97	djb		Additional HFS Plus optimization added to GetCatalogNode.
	 <HFS23>	 3/28/97	djb		Add Optimization to GetCatalogNode.
	 <HFS22>	 3/27/97	djb		Unicode conversion routines now use byte counts.
	 <HFS21>	 3/17/97	DSH		Casting to compile with SC, GetRecordSize ->
									GetCatalogRecordSize, moved some prototypes to extern.
	 <HFS20>	  3/5/97	msd		Add calls to Property Manager when catalog entries are created,
									deleted, moved, renamed.
	 <HFS19>	 2/19/97	djb		HFS Plus catalog keys no longer have a pad word.
	 <HFS18>	 1/24/97	DSH		(djb) GetCatalogOffSpring() fix volume->vcbDirIDM = 0
	 <HFS17>	 1/23/97	DSH		Truncate name to CMMaxCName characters in PrepareInputName().
	 <HFS16>	 1/14/97	djb		Fixed RenameCatalogNode for case when just a cnid is passed.
	 <HFS15>	 1/13/97	djb		Added support for varaible sized thread records in HFS+.
	 <HFS14>	 1/11/97	DSH		Moving PrepareInputName() declaration fo FilesInternal.h
	 <HFS13>	 1/10/97	djb		CopyCatalogNodeData was trashing the resource extents on HFS+.
	 <HFS12>	 1/10/97	djb		CopyCatalogNodeData was trashing dataLogicalSize on HFS+ disks.
	 <HFS11>	  1/9/97	djb		Get in sync with new HFSVolumesPriv.i.
	 <HFS10>	  1/6/97	djb		Added name length checking to CompareExtendedCatalogKeys. Fixed
									GetCatalogOffspring - it was not correctly passing the HFS+ flag
									to PrepareOutputName. Fixed BuildKey for HFS+ keys.
	  <HFS9>	  1/3/97	djb		Fixed termination bug in GetCatalogOffspring. Added support for
									large keys. Integrated latest HFSVolumesPriv.h changes.
	  <HFS8>	12/19/96	DSH		Changed call from C_FlushMDB to HFS+ savy
									FlushVolumeControlBlock()
	  <HFS7>	12/19/96	djb		Add new B-tree manager...
	  <HFS6>	12/13/96	djb		Fixing bugs for HFS+. Switch to HFSUnicodeWrappers routines.
	  <HFS5>	12/12/96	djb		Changed the SPI for GetCatalogNode, GetCatalogOffspring, and
									UpdateCatalogNode.
	  <HFS4>	12/12/96	DSH		Removed static function declarations for functions used by
									FileIDServices.c.
	  <HFS3>	11/11/96	djb		Added support for HFS+ Unicode names. Major changes throughout.
	  <HFS2>	 11/4/96	djb		Added FSSpec output to GetCatalogNode and GetCatalogOffspring
									routines.
	  <HFS1>	10/29/96	djb		first checked in

*/

#pragma segment Catalog

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
#include	"HFSInstrumentation.h"
#include	"CatalogPrivate.h"


// External routines

extern SInt32 FastRelString( ConstStr255Param str1, ConstStr255Param str2 );

extern SInt16 RelString_Glue(StringPtr pStr1, StringPtr pStr2);


// Internal routines

static OSErr	IterateCatalogNode( ExtendedVCB *volume, CatalogIterator *catalogIterator, UInt16 index,
					 				FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint );

#ifdef INVESTIGATE
//_________________________________________________________________________________
//	Exported Routines
//
//			CreateCatalogNode	-  Creates a new folder or file CNode.
//			DeleteCatalogNode	-  Deletes an existing folder or file CNode.
//			GetCatalogNode		-  Locates an existing folder or file CNode.
//			GetCatalogOffspringFile	-  Gets an offspring file record from a folder.
//			GetCatalogOffspring	-  Gets an offspring record from a folder.
//		    MoveCatalogNode	 	-  Moves an existing folder or file CNode to
//									another folder CNode.
//			RenameCatalogNode	-  Renames an existing folder or file CNode.
//			UpdateCatalogNode	-  Marks a Catalog BTree node as 'dirty'.
//			CompareCatalogKeys  -  Compares two catalog keys.
//
//_________________________________________________________________________________


//_________________________________________________________________________________
//
//	About date/time values:
//
//	Date/time values stored in control blocks and generic structures (such as
//	CatalogNodeData) are always stored in local time.  Values stored in HFS volume
//	format structures (such as B-tree records) are also stored in local time.
//	Values stored in HFS Plus format structures are stored in UTC.
//_________________________________________________________________________________


// Implementation

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CreateVolumeCatalogCache
//
//	Function:	
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
CreateVolumeCatalogCache(ExtendedVCB *volume)
{
	if ( volume->catalogDataCache == NULL )
	{
		CatalogDataCache	*cachePtr;

		cachePtr = (CatalogDataCache*) NewPtrSysClear( sizeof( CatalogDataCache ) );
		
		if ( cachePtr == NULL )
			return MemError();	
			
		volume->catalogDataCache = (LogicalAddress) cachePtr;		
	}

	return noErr;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	DisposeVolumeCatalogCache
//
//	Function:	
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
DisposeVolumeCatalogCache(ExtendedVCB *volume)
{
	if ( volume->catalogDataCache != NULL )
	{
		DisposePtr( (Ptr) volume->catalogDataCache);
		 volume->catalogDataCache = NULL;
	}	

	return noErr;
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CreateCatalogNode
//
//	Function: 	Creates a new folder or file CNode.	A new folder or file
//				record is added to the catalog BTree.  If a folder CNode is
//				being created, a new thread record is also added.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
CreateCatalogNode( ExtendedVCB *volume, CatalogNodeID parentID, ConstStr31Param name, SInt16 type,
					CatalogNodeID *catalogNodeID, UInt32 *catalogHint)
{
	CatalogNodeID	nodeID;
	CatalogKey		nodeKey;
	CatalogRecord	nodeData;
	UInt32			nodeDataSize;
	CatalogName		nodeName;
	CatalogRecord	tempData;
	CatalogNodeID	parentsParentID;
	CatalogName		parentNodeName;
	UInt32			tempHint;
	UInt32			timeStamp;
	UInt32			textEncoding;
	UInt16			tempSize;
	SInt16			threadType;
	OSErr			result;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);

	if (isHFSPlus)
		type = type >> 8;	// fabricate a type for HFS Plus
		
	TrashCatalogIterator(volume, parentID);		// invalidate any iterators for this parentID

	if (isHFSPlus)
		ConvertInputNameToUnicode(name, GetDefaultTextEncoding(), &textEncoding, &nodeName);
	else
		(void) PrepareInputName(name, isHFSPlus, 0, &nodeName); 	// encoding ignored for HFS

	//--- make sure parent exists (by locating the parent's thread record)
			
	result = LocateCatalogThread(volume, parentID, &tempData, &tempSize, &tempHint);
	ReturnIfError(result);
		
	// save copy of parent's parentID and name.

	if (isHFSPlus)
	{
		UInt32	parentNameSize;

		if (tempData.recordType != kLargeFolderThreadRecord)
		{
			if (DEBUG_BUILD)
				DebugStr("\pCreateCatalogNode: parent is not a folder!");
			return dirNFErr;
		}
		
		parentsParentID = tempData.largeThread.parentID;
		parentNameSize = sizeof(UniChar) * (tempData.largeThread.nodeName.length + 1);
		BlockMoveData(&tempData.largeThread.nodeName, &parentNodeName, parentNameSize);
	}
	else
	{
		if (tempData.recordType != kSmallFolderThreadRecord)
		{
			if (DEBUG_BUILD)
				DebugStr("\pCreateCatalogNode: parent is not a folder!");
			return dirNFErr;
		}
		
		parentsParentID = tempData.smallThread.parentID;
		BlockMoveData(tempData.smallThread.nodeName, &parentNodeName, tempData.smallThread.nodeName[0] + 1);
	}
	
	InvalidateCatalogNodeCache(volume, parentsParentID);	// invalidate cache for parent since its about to change

	
	//--- build key for new catalog node

	BuildCatalogKey(parentID, &nodeName, isHFSPlus, &nodeKey);	// passes a pascal or unicode string

	//--- initialize catalog data record (for folder or file)
	
	ClearMemory( &nodeData, sizeof(CatalogRecord) );	// first clear the record

		
	nodeData.recordType = type;
	nodeID = volume->vcbNxtCNID++;		// get CNID and bump next available CNode ID
	*catalogNodeID = nodeID;			// make sure it gets passed back
	
	if (isHFSPlus)
		timeStamp = GetTimeUTC();		// get current date/time (universal)
	else
		timeStamp = GetTimeLocal();		// get current local date/time

	switch (type)
	{
		// assign node ID and set dates...
		case kSmallFolderRecord:
			nodeData.smallFolder.folderID = nodeID;
			nodeData.smallFolder.createDate = timeStamp;
			nodeData.smallFolder.modifyDate = timeStamp;
			nodeDataSize = sizeof(SmallCatalogFolder);
			threadType = kSmallFolderThreadRecord;
			break;

		case kLargeFolderRecord:
			nodeData.largeFolder.folderID = nodeID;
			nodeData.largeFolder.createDate = timeStamp;
			nodeData.largeFolder.contentModDate = timeStamp;
			nodeData.largeFolder.textEncoding = textEncoding;
			nodeDataSize = sizeof(LargeCatalogFolder);
			threadType = kLargeFolderThreadRecord;
			break;

		case kSmallFileRecord:
			nodeData.smallFile.fileID = nodeID;
			nodeData.smallFile.createDate = timeStamp;
			nodeData.smallFile.modifyDate = timeStamp;
			nodeDataSize = sizeof(SmallCatalogFile);
			nodeID = 0;		// HFS files don't get a thread
			break;

		case kLargeFileRecord:
			nodeData.largeFile.fileID = nodeID;
			nodeData.largeFile.createDate = timeStamp;
			nodeData.largeFolder.contentModDate = timeStamp;
			nodeData.largeFile.flags |= kFileThreadExistsMask;
			nodeData.largeFile.textEncoding = textEncoding;
			nodeDataSize = sizeof(LargeCatalogFile);
			threadType = kLargeFileThreadRecord;
			break;

		default:
			return paramErr;	// invalid record type requested
	}

	if (isHFSPlus)
		UpdateVolumeEncodings(volume, textEncoding);

	//--- add new folder/file record to catalog BTree
			
	result = InsertBTreeRecord(volume->catalogRefNum, &nodeKey, &nodeData, nodeDataSize, catalogHint);
	if (result)
	{
		volume->vcbNxtCNID--;		// restore next available CNode ID
		if (result == btExists)
			result = cmExists;
		return result;
	}

	//--- build thread record for new CNode
			
	if (nodeID)
	{
		CatalogKey		threadKey;
		CatalogRecord	threadData;
		UInt32			threadSize;
		CatalogName*	threadNamePtr;
		
		BuildCatalogKey(nodeID, NULL, isHFSPlus, &threadKey);
		
		ClearMemory( &threadData, sizeof(CatalogRecord) );		// first clear the record

		threadData.recordType = threadType;

		if (isHFSPlus)
		{
		  //threadData.largeThread.reserved = 0;
			threadData.largeThread.parentID = parentID;			
			threadSize = sizeof(threadData.largeThread);
			// HFS Plus has varaible sized threads so adjust to actual length
			threadSize -= ( sizeof(threadData.largeThread.nodeName.unicode) - (nodeName.ustr.length * sizeof(UniChar)) );
			threadNamePtr = (CatalogName*) &threadData.largeThread.nodeName;
		}
		else // classic HFS
		{
		  //threadData.smallThread.reserved[0] = 0;
		  //threadData.smallThread.reserved[1] = 0;
			threadData.smallThread.parentID = parentID;	
			threadSize = sizeof(threadData.smallThread);
			threadNamePtr = (CatalogName*) threadData.smallThread.nodeName;
		}
		
		CopyCatalogName(&nodeName, threadNamePtr, isHFSPlus);

		//--- add thread record to catalog BTree
	
		result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadData, threadSize, &tempHint);

		//--- couldn't add thread record, delete newly created folder record and exit
		if (result)
		{
			(void) DeleteBTreeRecord(volume->catalogRefNum, &nodeKey);
			
			// if this key aready exists then don't restore vcbNxtCNID	 		<CS17>
			// so that next time around we'll use a different CNID				<CS17>

			if ( result == btExists )		// <CS17>
				result = cmExists;			// remap to a catalog error
			else
				volume->vcbNxtCNID--;		// restore next available CNode ID

			return result;
		}
	}


	//--- update counters...

	result = UpdateFolderCount( volume, parentsParentID, &parentNodeName, type, kNoHint, +1);
	ReturnIfError(result);	//€€ what about cleanup ???
	
	AdjustVolumeCounts(volume, type, +1);

	if (isHFSPlus)
		SetTextEncodingInHint(textEncoding, catalogHint);	// sneak in the encoding

	result = FlushCatalog(volume);

	return result;

} // end CreateCatalogNode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	DeleteCatalogNode
//
//	Function: 	Deletes an existing folder or file CNode. The thread record
//				is also deleted for directories and files that have thread
//				records.
//
//				The valence for a folder must be zero before it can be deleted.
//				The rootfolder cannot be deleted.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
DeleteCatalogNode(ExtendedVCB *volume, CatalogNodeID parentID, ConstStr31Param name, UInt32 hint)
{
	CatalogKey		key;
	CatalogRecord	data;
	UInt32			nodeHint;
	CatalogNodeID	nodeID;
	CatalogNodeID	nodeParentID;
	CatalogName		nodeName;
	UInt16			nodeType;
	OSErr			result;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	
	
	result = PrepareInputName(name, isHFSPlus, GetTextEncodingFromHint(hint), &nodeName);	// encoding ignored for HFS
	ReturnIfError(result);

	//--- locate subject catalog node

	result = LocateCatalogNode(volume, parentID, &nodeName, hint, &key, &data, &nodeHint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, parentID, name, &key, &data, &nodeHint);
	ReturnIfError(result);

	nodeParentID = isHFSPlus ? key.large.parentID : key.small.parentID;	// establish real parent cnid
	nodeType = data.recordType;		// establish cnode type
	nodeID = 0;
	
	TrashCatalogIterator(volume, nodeParentID);			// invalidate any iterators for this parentID
	InvalidateCatalogNodeCache(volume, nodeParentID);	// and invalidate node cache

	switch (nodeType)
	{
		case kSmallFolderRecord:
			if (data.smallFolder.valence != 0)		// is it empty?
				return cmNotEmpty;
			
			nodeID = data.smallFolder.folderID;
			break;

		case kLargeFolderRecord:
			if (data.largeFolder.valence != 0)		// is it empty?
				return cmNotEmpty;
			
			nodeID = data.largeFolder.folderID;
			break;

		case kSmallFileRecord:
			if (data.smallFile.flags & kFileThreadExistsMask)
				nodeID = data.smallFile.fileID;
			break;

		case kLargeFileRecord:
			nodeID = data.largeFile.fileID;	// note: HFS Plus files always have a thread
			break;
	}


	if (nodeID == fsRtDirID)	// is this the root folder?
		return cmRootCN;		// sorry, you can't delete the root!


	//--- delete catalog records for CNode and file threads if they exist

	result = DeleteBTreeRecord(volume->catalogRefNum, &key);
	ReturnIfError(result);

	if ( nodeID ) 
	{
		CatalogKey threadKey;
		
		BuildCatalogKey(nodeID, NULL, isHFSPlus, &threadKey);
		
		(void) DeleteBTreeRecord(volume->catalogRefNum, &threadKey);	// ignore errors for thread deletes
	}

	//--- update counters...

	result = UpdateFolderCount(volume, nodeParentID, NULL, nodeType, kNoHint, -1);
	ReturnIfError(result);

	AdjustVolumeCounts(volume, nodeType, -1);	// all done with this file or folder

	result = FlushCatalog(volume);

	return result;

} // end DeleteCatalogNode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetCatalogNode
//
//	Function: 	Locates an existing folder or file CNode and returns an FSSpec for
//				the CNode and a pointer to the CNode data record.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
GetCatalogNode( ExtendedVCB *volume, CatalogNodeID parentID, ConstStr31Param name, UInt32 hint,
				FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *newHint)
{
	FSVarsRec *			fsVars;
	CatalogKey			*key;
	CatalogName			*nodeName;
	Str31				tempNodeName;
	CatalogRecord		*record;
	CatalogDataCache	*catalogCache;
	OSErr				result;
	Boolean				isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);


	fsVars = (FSVarsRec*) LMGetFSMVars();
	
	// Optimization...
	// If we have a copy of the data then use it!
	// Any Catalog operations that can cause this data to
	// become stale must invalidate the gCatalogFSSpec by
	// calling InvalidateCatalogNodeCache
	
	if ( (nodeSpec == &fsVars->gCatalogFSSpec)		&&		// using global FSSpec
		 (nodeSpec->parID == parentID)				&&		// same parent
		 (nodeSpec->vRefNum == volume->vcbVRefNum)	&&		// same volume
		 (name != NULL)								&&		// same name
		 PascalBinaryCompare(nodeSpec->name, name)	)
	{
		// gCatalogFSSpec has FSSpec
		// gCatalogData has CatalogNodeData
		// gLastCatalogKey has actualt B-tree key (for UpdateCatalogNode calls)
		// gLastCatalogRecord has actual B-tree record (for UpdateCatalogNode calls)

		*newHint = hint;
		if (isHFSPlus)
			SetTextEncodingInHint(nodeData->textEncoding, newHint);	// sneak in the encoding

		return noErr;	// wow, that was fast!
	}
	
	fsVars->gCatalogFSSpec.parID = 0;	// invalidate node cache

	// Optimization...
	// Use space in fsVars instead of local variable so
	// that we end up with a copy of the key and record
	// which can be used by UpdateCatalogNode

	record = &fsVars->gLastCatalogRecord;
	key	   = &fsVars->gLastCatalogKey;

	catalogCache = (CatalogDataCache *) volume->catalogDataCache;
	
	if (isHFSPlus)
		nodeName = (CatalogName*) &catalogCache->lastNameUnicode;		// use unicode string cache as our name buffer too
	else
		nodeName = (CatalogName*) &tempNodeName;

	//--- Locate folder/file BTree record for the CNode
	
	if ( isHFSPlus )
	{
		result = LocateCatalogNodeWithRetry(volume, parentID, name, nodeName, hint, key, record, newHint);

		// if we did not find it by name, then look for an embedded file ID in a mangled name
		if ( result == cmNotFound )
			result = LocateCatalogNodeByMangledName(volume, parentID, name, key, record, newHint);
	}
	else
	{
		(void) PrepareInputName(name, false, 0, nodeName);
		result = LocateCatalogNode(volume, parentID, nodeName, hint, key, record, newHint);
	}

	ReturnIfError(result);

	//--- Fill out the FSSpec (output)
	
	nodeSpec->vRefNum = volume->vcbVRefNum;
	nodeSpec->parID = isHFSPlus ? key->large.parentID : key->small.parentID;
	
	if ( isHFSPlus )
	{
		if ( UnicodeBinaryCompare(&key->large.nodeName, &nodeName->ustr) )
		{
		// HFS Plus Optimization: since nodeName == key->ckrCName just copy the original name!
		// Omit _expensive_ conversion back to ascii

		BlockMoveData(name, nodeSpec->name, name[0] + 1);
		}
		else
		{
			TextEncoding	encoding;
			CatalogNodeID	cnid;
	
			cnid = record->largeFile.fileID;			// note: cnid is at same offset for folders
			encoding = record->largeFile.textEncoding;	// note: textEncoding is at same offset for folders

			(void) ConvertUnicodeToHFSName( &key->large.nodeName, encoding, cnid, nodeSpec->name );
		}
	}
	else // classic HFS
	{
		BlockMoveData( key->small.nodeName, nodeSpec->name, key->small.nodeName[0] + 1);
	}
	

	//--- Fill out universal data record...
	CopyCatalogNodeData(volume, record, nodeData);

	if (isHFSPlus)
		SetTextEncodingInHint(record->largeFile.textEncoding, newHint);	// sneak in the encoding

  #if DEBUG_BUILD
	if ( nodeData->nodeType != '????' )
		if ( nodeData->nodeID > volume->vcbNxtCNID || nodeData->nodeID == 0)
			DebugStr("\pGetCatalogOffspring: bad file ID found!");
  #endif
 
	return result;

} // end GetCatalogNode



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetCatalogOffspringFile
//
//	Function: 	Gets an offspring file from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring file is
//				indicated by the value of the file index (1 = 1st file offspring,
//				2 = 2nd file offspring, etc.).
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
GetCatalogOffspringFile( ExtendedVCB *volume, CatalogNodeID folderID, UInt16 fileIndex,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	CatalogIterator *	catalogIterator;
	UInt16				index;
	UInt16				fileCounter;
	OSErr				result;


	if ( folderID == 0 )
		return cmNotFound;

	// get the best catalog iterator...
	catalogIterator = GetCatalogIterator(volume, folderID, fileIndex, kIterateFilesOnly);

	index = catalogIterator->currentIndex;			// Get last search's folder offset

	// make sure we're not looking for a file beyond this one...
	if ( index == 0 || catalogIterator->fileIndex > fileIndex )
	{
		index = 1;					// Start with the first item in the folder
		fileCounter = fileIndex;	// Need to look at all files up to and including fileIndex
	}
	else
	{
		fileCounter = fileIndex - catalogIterator->fileIndex + 1;
		
		if (fileCounter == 2)		// if difference is one then no need to read previous file
		{
			++index;				// go to next entry
			fileCounter = 1;		// were looking for 1 file (the next one we find)
		}
	}


	while ( fileCounter > 0 ) 
	{
		result = IterateCatalogNode(volume, catalogIterator, index++, nodeSpec, nodeData, hint);

		if (result != noErr)
			break;

		if ( nodeData->nodeType == kCatalogFileNode )
			--fileCounter;			// one down
	}

	if ( result == noErr )
		catalogIterator->fileIndex = fileIndex;		// update current file index for next time

	return result;

} // end GetCatalogOffspringFile


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetCatalogOffspring
//
//	Function: 	Gets an offspring record from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring CNode is
//				indicated by the value of the offspring index (1 = 1st offspring
//				CNode, 2 = 2nd offspring CNode, etc.).
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
GetCatalogOffspring( ExtendedVCB *volume, CatalogNodeID folderID, UInt16 index,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	CatalogIterator *	catalogIterator;
	OSErr				result;


	if ( folderID == 0 )
		return cmNotFound;

	// get best catalog iterator...
	catalogIterator = GetCatalogIterator(volume, folderID, index, kIterateAll);
	
	result = IterateCatalogNode(volume, catalogIterator, index, nodeSpec, nodeData, hint);

	return result;

} // end GetCatalogOffspring


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	IterateCatalogNode
//
//	Function: 	Gets an offspring record from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring CNode is
//				indicated by the value of the offspring index (1 = 1st offspring
//				CNode, 2 = 2nd offspring CNode, etc.).
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

static OSErr
IterateCatalogNode( ExtendedVCB *volume, CatalogIterator *catalogIterator, UInt16 index,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	FSVarsRec *			fsVars;
	CatalogNodeID		offspringParentID;
	CatalogKey *		offspringKey;
	CatalogRecord *		offspringData;
	CatalogName *		offspringName;
	BTreeIterator		btreeIterator;
	FSBufferDescriptor	btRecord;
	FCB *				fcb;
	SInt16				selectionIndex;
	UInt16				tempSize;
	UInt16				operation;
	OSErr				result;
	Boolean				isHFSPlus;
	
	isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	
	fcb = GetFileControlBlock(volume->catalogRefNum);

	// Optimization...
	// use space in fsVars instead of local variable so
	// that we end up with a copy of the record

	fsVars = (FSVarsRec*) LMGetFSMVars();
	offspringData = &fsVars->gLastCatalogRecord;
	fsVars->gCatalogFSSpec.parID = 0;	// invalidate node cache

	// make a btree iterator from catalog iterator
	UpdateBtreeIterator(catalogIterator, &btreeIterator);

	btRecord.bufferAddress	= offspringData;
	btRecord.itemCount		= 1;
	btRecord.itemSize		= sizeof(CatalogRecord);

	//--- if neccessary position the iterator at the thread record for the specified folder

	if ( catalogIterator->currentIndex == 0 )	// is this a new iterator?
	{
		UInt32	heuristicHint;
		UInt32	*cachedHint;

		//	We pass a 2nd hint/guess into BTSearchRecord.  The heuristicHint is a mapping of
		//	dirID and nodeNumber, in hopes that the current search will be in the same node
		//	as the last search with the same parentID.
		result = GetMRUCacheBlock( catalogIterator->folderID, volume->hintCachePtr, (Ptr *)&cachedHint );
		heuristicHint = (result == noErr) ? *cachedHint : kInvalidMRUCacheKey;

		result = BTSearchRecord( fcb, &btreeIterator, heuristicHint, &btRecord, &tempSize, &btreeIterator );
		ExitOnError(result);
		
		UpdateCatalogIterator(&btreeIterator, catalogIterator);	// update btree hint and key

		InsertMRUCacheBlock( volume->hintCachePtr, catalogIterator->folderID, (Ptr) &btreeIterator.hint.nodeNum );
	}

	//--- get offspring record (relative to catalogIterator's position)

	selectionIndex = index - catalogIterator->currentIndex;

	// now we have to map index into next/prev operations...
	if (selectionIndex == 1)
	{
		operation = kBTreeNextRecord;
	}
	else if (selectionIndex == -1)
	{
		operation = kBTreePrevRecord;
	}
	else if (selectionIndex == 0)
	{
		operation = kBTreeCurrentRecord;
	}
	else if (selectionIndex > 1)
	{
		UInt32	i;
		
		for (i = 1; i < selectionIndex; ++i)
		{
			result = BTIterateRecord( fcb, kBTreeNextRecord, &btreeIterator, &btRecord, &tempSize );
			ExitOnError(result);
		}
		operation = kBTreeNextRecord;
	}
	else // (selectionIndex < -1)
	{
		SInt32	i;

		for (i = -1; i > selectionIndex; --i)
		{
			result = BTIterateRecord( fcb, kBTreePrevRecord, &btreeIterator, &btRecord, &tempSize );
			ExitOnError(result);
		}
		operation = kBTreePrevRecord;
	}

	result = BTIterateRecord( fcb, operation, &btreeIterator, &btRecord, &tempSize );
	ExitOnError(result);

	offspringKey = (CatalogKey*) &btreeIterator.key;

	if (isHFSPlus)
	{
		offspringParentID = offspringKey->large.parentID;
		offspringName = (CatalogName*) &offspringKey->large.nodeName;
	}
	else
	{
		offspringParentID = offspringKey->small.parentID;
		offspringName = (CatalogName*) offspringKey->small.nodeName;
	}

	if (offspringParentID != catalogIterator->folderID)		// different parent?
	{
		AgeCatalogIterator(catalogIterator);	// we reached the end, so don't hog the cache!

		result = cmNotFound;				// must be done with this folder
		goto ErrorExit;
	}

	UpdateCatalogIterator(&btreeIterator, catalogIterator);		// update btree hint and key
	catalogIterator->currentIndex = index;						// update the offspring index marker

	*hint = btreeIterator.hint.nodeNum;		// return an old-style hint

	//--- Fill out the FSSpec...

	nodeSpec->vRefNum = volume->vcbVRefNum;
	nodeSpec->parID = offspringParentID;

	if (isHFSPlus)
	{
		TextEncoding	encoding;

		encoding = offspringData->largeFile.textEncoding;		// note: field is at same offset for folders
		SetTextEncodingInHint(encoding, hint);					// sneak in the encoding

		(void) ConvertUnicodeToHFSName( &offspringName->ustr, encoding, offspringData->largeFile.fileID, nodeSpec->name );

		// save a copy of key for Catalog node cache
		BlockMoveData( offspringKey, &fsVars->gLastCatalogKey, offspringKey->large.keyLength + sizeof(UInt16));
	}
	else
	{
		BlockMoveData( &offspringName->pstr, nodeSpec->name, offspringName->pstr[0] + 1);
		
		// save a copy of key for Catalog node cache
		BlockMoveData( offspringKey, &fsVars->gLastCatalogKey, offspringKey->small.keyLength + sizeof(UInt8));
	}

	//--- Fill out universal data record...
	CopyCatalogNodeData(volume, offspringData, nodeData);


  #if DEBUG_BUILD
	if ( nodeData->nodeType != '????' )
		if ( nodeData->nodeID > volume->vcbNxtCNID || nodeData->nodeID == 0)
			DebugStr("\pGetCatalogOffspring: bad file ID found!");
  #endif
	
	return result;

ErrorExit:

	if ( result == btNotFound )
		result = cmNotFound;

	return result;

} // end IterateCatalogNode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	MoveCatalogNode
//
//	Function: 	Moves an existing folder or file CNode to another folder
//				CNode.	Note that for a folder, all decendants (its offspring,
//				their offspring, etc.) are also moved.
//
// Assumes hint contains a text encoding that was set by a GetCatalogNode call
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
MoveCatalogNode(ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param name, UInt32 srcHint,
				CatalogNodeID destFolderID, UInt32 *newHint)
{
	CatalogKey		destFolderKey;
	CatalogRecord	tempRecord;
	CatalogKey		srcKey;
	CatalogRecord	srcRecord;
	CatalogNodeID	srcParentID;
	CatalogNodeID	destFolderParentID;
	CatalogName		*destFolderNamePtr;
	UInt32			destFolderHint;
	CatalogName		nodeName;
	OSErr			result;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);


	result = PrepareInputName(name, isHFSPlus, GetTextEncodingFromHint(srcHint), &nodeName);	 // encoding ignored for HFS
	ReturnIfError(result);

	//--- make sure source record exists
	
	result = LocateCatalogNode(volume, folderID, &nodeName, srcHint, &srcKey, &srcRecord, &srcHint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, folderID, name, &srcKey, &srcRecord, &srcHint);
	ReturnIfError(result);

	//--- make sure destination folder exists

	result = LocateCatalogNode(volume, destFolderID, NULL, kNoHint, &destFolderKey, &tempRecord, &destFolderHint);
	ReturnIfError(result);
		
	if (isHFSPlus && tempRecord.recordType == kLargeFolderRecord)			// an HFS Plus folder record?
	{
		destFolderID	   = tempRecord.largeFolder.folderID;
		destFolderParentID = destFolderKey.large.parentID;
		destFolderNamePtr  = (CatalogName*) &destFolderKey.large.nodeName;
	}
	else if (!isHFSPlus && tempRecord.recordType == kSmallFolderRecord)		// an HFS folder record?
	{
		destFolderID = tempRecord.smallFolder.folderID;
		destFolderParentID = destFolderKey.small.parentID;
		destFolderNamePtr  = (CatalogName*) &destFolderKey.small.nodeName;
	}
	else
	{
		return badMovErr;
	}

	//--- if source is a folder, make sure its a proper move

	if (srcRecord.recordType == kSmallFolderRecord || srcRecord.recordType == kLargeFolderRecord)
	{
		CatalogNodeID srcFolderID;
		CatalogNodeID ancestorParentID;
		CatalogKey		tempKey;
		UInt32			tempHint;

		if (isHFSPlus)
		{
			srcFolderID = srcRecord.largeFolder.folderID;
			ancestorParentID = destFolderKey.large.parentID;
		}
		else
		{
			srcFolderID = srcRecord.smallFolder.folderID;
			ancestorParentID = destFolderKey.small.parentID;
		}

		if ( srcFolderID == fsRtDirID	||		// source == root?
			 srcFolderID == destFolderID	||		// source == destination?
			 srcFolderID == ancestorParentID )	// source == destination's parent?
		{
			return badMovErr;
		}

		while (ancestorParentID > fsRtDirID)	// loop until we reach the root folder
		{
			// locate next folder up the tree...	
			result = LocateCatalogNode(volume, ancestorParentID, NULL, kNoHint, &tempKey, &tempRecord, &tempHint);
			ReturnIfError(result);
			
			ancestorParentID = isHFSPlus ? tempKey.large.parentID : tempKey.small.parentID;

			if (srcFolderID == ancestorParentID)	// source = destination ancestor?
				return badMovErr;
		}
	}

	//--- remember source's original parent

	srcParentID = isHFSPlus ? srcKey.large.parentID : srcKey.small.parentID;

	if (srcParentID == destFolderID)		// source ParID = dest DirID?
	{
		*newHint = srcHint;					// they match, so we're all done!
	}
	else
	{
		CatalogNodeID	threadID;
		UInt16			recordSize;

		TrashCatalogIterator(volume, srcParentID);			// invalidate any iterators for source's parentID
		TrashCatalogIterator(volume, destFolderID);			// invalidate any iterators for destination parentID
		InvalidateCatalogNodeCache(volume, srcParentID);	// invalidate node cache since parent changed


		//--- insert new source CNode record in BTree with new key

		if ( isHFSPlus )
			srcKey.large.parentID = destFolderID;	// set ParID to new one
		else
			srcKey.small.parentID = destFolderID;	// set ParID to new one

		recordSize = GetCatalogRecordSize( &srcRecord );
		
		//
		//€€ Why do we set the mod date when a folder is moved, but not a file???
		//
		if (srcRecord.recordType == kSmallFolderRecord)
			srcRecord.smallFolder.modifyDate = GetTimeLocal();		// set date/time last modified
		else if (srcRecord.recordType == kLargeFolderRecord)
			srcRecord.largeFolder.contentModDate = GetTimeUTC();	// set date/time last modified
		
		result = InsertBTreeRecord(volume->catalogRefNum, &srcKey, &srcRecord, recordSize, newHint);
		if (result)
			return (result == btExists ? cmExists : result);
		
		//
		//€€ NOTE: from this point on we need to cleanup (ie delete this record) if we encounter errors!	
		//
		
		//--- update destination folder record

		result = UpdateFolderCount(volume, destFolderParentID, destFolderNamePtr, srcRecord.recordType, destFolderHint, +1);
		ReturnIfError(result);	//€€ NEED TO ADD CLEANUP CODE!!!!!!!!!!!!!
	
		//--- delete old source CNode record
			
		if ( isHFSPlus )
			srcKey.large.parentID = srcParentID;	// restore source key's original parent
		else
			srcKey.small.parentID = srcParentID;	// restore source key's original parent

		result = DeleteBTreeRecord(volume->catalogRefNum, &srcKey);
		ReturnIfError(result);

		//--- update source thread record
		
		threadID = 0;

		switch (srcRecord.recordType)
		{
			case kSmallFolderRecord:
				threadID = srcRecord.smallFolder.folderID;
				break;
		
			case kLargeFolderRecord:
				threadID = srcRecord.largeFolder.folderID;
				break;
		
			case kSmallFileRecord:
				if (srcRecord.smallFile.flags & kFileThreadExistsMask)
					threadID = srcRecord.smallFile.fileID;
				break;
	
			case kLargeFileRecord:
				threadID = srcRecord.largeFile.fileID;
				break;
		}
	
		if (threadID)
		{
			UInt32			threadHint;
			CatalogKey		threadKey;
			CatalogRecord	threadRecord;
			UInt16			threadSize;
	
			result = LocateCatalogRecord(volume, threadID, NULL, kNoHint, &threadKey, &threadRecord, &threadHint);
			ReturnIfError(result);
			
			// update the thread's parent ID
			if (isHFSPlus)
			{
				threadRecord.largeThread.parentID = destFolderID;
				threadSize = sizeof(LargeCatalogThread);
			}
			else
			{
				threadRecord.smallThread.parentID = destFolderID;
				threadSize = sizeof(SmallCatalogThread);
			}

			result = ReplaceBTreeRecord( volume->catalogRefNum, &threadKey, threadHint,
										 &threadRecord, threadSize, &threadHint);
			ReturnIfError(result);
		}
	
		//--- update source parent folder

		result = UpdateFolderCount(volume, srcParentID, NULL, srcRecord.recordType, kNoHint, -1);
		ReturnIfError(result);
	}

	if (isHFSPlus)
		SetTextEncodingInHint(srcRecord.largeFolder.textEncoding, newHint);	// sneak in the encoding

	//--- make sure changes get flushed out
	
	volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
	volume->vcbLsMod = GetTimeLocal();	// update last modified date
	result = FlushCatalog(volume);

	return result;

} // end MoveCatalogNode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	RenameCatalogNode
//
//	Function: 	Renames an existing folder or file CNode.
//
// Assumes hint contains a text encoding that was set by a GetCatalogNode call
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
RenameCatalogNode(ExtendedVCB *volume, CatalogNodeID folderID, ConstStr31Param oldName,
					ConstStr31Param newName, UInt32 inHint, UInt32 *outHint)
{
	CatalogKey		newKey;
	CatalogKey		oldKey;
	CatalogRecord	oldData;
	UInt32			newHint;
	UInt32			oldCNodeHint;
	Size			dataSize;
	CatalogNodeID	cnid;
	CatalogNodeID	oldParentID;
	UInt32			textEncoding;
	OSErr			result;
	CatalogName		oldNodeName;
	CatalogName		newNodeName;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);

	
	result = PrepareInputName(oldName, isHFSPlus, GetTextEncodingFromHint(inHint), &oldNodeName);	// encoding ignored for HFS
	ReturnIfError(result);

	if (isHFSPlus)
		ConvertInputNameToUnicode(newName, GetDefaultTextEncoding(), &textEncoding, &newNodeName);
	else
		(void) PrepareInputName(newName, false, 0, &newNodeName);	// encoding ignored for HFS


	//--- locate subject record at <folderID><oldName>
	
	result = LocateCatalogNode(volume, folderID, &oldNodeName, inHint, &oldKey, &oldData, &oldCNodeHint);
	
	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, folderID, oldName, &oldKey, &oldData, &oldCNodeHint);
	ReturnIfError(result);


	//€€ should LocateCatalogNode pass back data size?

	dataSize = GetCatalogRecordSize(&oldData);
	
	if ( isHFSPlus )
	{
		oldParentID = oldKey.large.parentID;
		
		// update textEncoding hint (works for folders and files)
		oldData.largeFolder.textEncoding = textEncoding;

		UpdateVolumeEncodings(volume, textEncoding);
	}
	else
	{
		oldParentID = oldKey.small.parentID;
	}
	
	TrashCatalogIterator(volume, oldParentID);			// invalidate any iterators for this folder
	InvalidateCatalogNodeCache(volume, oldParentID);	// and invalidate node cache

	//--- insert old CNode record in BTree with new key

	BuildCatalogKey(oldParentID, &newNodeName, isHFSPlus, &newKey);
			
	result = InsertBTreeRecord(volume->catalogRefNum, &newKey, &oldData, dataSize, &newHint);

	if (result == btExists)
	{
		//--- new CNode already exists, locate the existing one
		CatalogKey		existingKey;
		CatalogRecord	existingData;
		Boolean			sameNode = false;

		oldParentID = isHFSPlus ? oldKey.large.parentID : oldKey.small.parentID;

		result = LocateCatalogRecord(volume, oldParentID, &newNodeName, kNoHint, &existingKey,
									 &existingData, &newHint);
		ReturnIfError(result);

		//--- check if same CNode (same name but different upper/lower case)
			
		if ( oldData.recordType == existingData.recordType )
		{
			switch ( oldData.recordType )
			{
				case kSmallFolderRecord:
					if ( oldData.smallFolder.folderID == existingData.smallFolder.folderID )
						sameNode = true;
					break;

				case kSmallFileRecord:
					if ( oldData.smallFile.fileID == existingData.smallFile.fileID )
						sameNode = true;
					break;

				case kLargeFolderRecord:
					if ( oldData.largeFolder.folderID == existingData.largeFolder.folderID )
						sameNode = true;
					break;

				case kLargeFileRecord:
					if ( oldData.largeFile.fileID == existingData.largeFile.fileID )
						sameNode = true;
					break;
			}
		}

		ReturnErrorIf(!sameNode, cmExists);

		//--- same name but different case, so delete old and insert with new name...
	
		result = DeleteBTreeRecord(volume->catalogRefNum, &oldKey);
		ReturnIfError(result);

		result = InsertBTreeRecord(volume->catalogRefNum, &newKey, &oldData, dataSize, &newHint);
	}
	else if (result == noErr)
	{
		//--- insert went ok, delete old CNode record  at <folderID><oldName>
			
		result = DeleteBTreeRecord(volume->catalogRefNum, &oldKey);
	}
	else
	{
		result = cmExists;
	}

	ReturnIfError(result);
	
	//--- update thread record if a folder (or file with thread) was renamed

	cnid = 0;
	
	switch (oldData.recordType)
	{
		case kSmallFolderRecord:
			cnid = oldData.smallFolder.folderID;
			break;

		case kSmallFileRecord:
			if (oldData.smallFile.flags & kFileThreadExistsMask)
				cnid = oldData.smallFile.fileID;
			break;

		case kLargeFolderRecord:
			cnid = oldData.largeFolder.folderID;
			break;

		case kLargeFileRecord:
			cnid = oldData.largeFile.fileID;
			break;
	}

	if (cnid)
	{
		UInt32			threadHint;
		CatalogKey		threadKey;
		CatalogRecord	threadRecord;
		UInt16			threadSize;
		
		result = LocateCatalogRecord(volume, cnid, NULL, kNoHint,&threadKey, &threadRecord, &threadHint);
		ReturnIfError(result);

		// update the thread's node name

		if (isHFSPlus)
		{
			threadSize = sizeof(threadRecord.largeThread);
			// HFS Plus has varaible sized threads so adjust to actual length
			threadSize -= ( sizeof(threadRecord.largeThread.nodeName.unicode) - (newNodeName.ustr.length * sizeof(UniChar)) );

			CopyCatalogName(&newNodeName, (CatalogName *) &threadRecord.largeThread.nodeName, isHFSPlus);
			
			result = DeleteBTreeRecord(volume->catalogRefNum, &threadKey);
			ReturnIfError(result);

			result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadRecord, threadSize, &threadHint);
		}
		else
		{
			threadSize = sizeof(threadRecord.smallThread);

			CopyCatalogName(&newNodeName, (CatalogName *) threadRecord.smallThread.nodeName, isHFSPlus);
			
			// Thread records are fixed size in HFS so we can just replace it
			result = ReplaceBTreeRecord( volume->catalogRefNum, &threadKey, threadHint,
										 &threadRecord, threadSize, &threadHint);
		}
	}

	ReturnIfError(result);	//€€ need to do real cleanup!!

	if (isHFSPlus)
		SetTextEncodingInHint(textEncoding, outHint);	// sneak in the encoding

	//--- make sure changes get flushed out

	volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
	volume->vcbLsMod = GetTimeLocal();	// update last modified date
	result = FlushCatalog(volume);

	*outHint = newHint;			// return new hint		//€€ this drops the text encoding information!!!

	return result;

} // end RenameCatalogNode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	RenameCatalogNodeUnicode
//
//	Function: 	Renames an existing folder or file CNode.  For HFS Plus volumes,
//				both the old and new names are in Unicode.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
RenameCatalogNodeUnicode(ExtendedVCB *volume, CatalogNodeID folderID, const CatalogName *oldName,
					const CatalogName *newName, TextEncoding newEncoding, UInt32 inHint, UInt32 *outHint)
{
	CatalogKey		newKey;
	CatalogKey		oldKey;
	CatalogRecord	oldData;
	UInt32			newHint;
	UInt32			oldCNodeHint;
	Size			dataSize;
	CatalogNodeID	cnid;
	CatalogNodeID	oldParentID;
	OSErr			result;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);

	
	//	Make sure we've got a conversion context for the desired text encoding
	result = InitializeEncodingContext( newEncoding, (FSVarsRec *) LMGetFSMVars() );
	ReturnIfError(result);
	
	//--- locate subject record at <folderID><oldName>
	
	result = LocateCatalogNode(volume, folderID, oldName, inHint, &oldKey, &oldData, &oldCNodeHint);
	ReturnIfError(result);

	//€€ should LocateCatalogNode pass back data size?

	dataSize = GetCatalogRecordSize(&oldData);
	
	oldParentID = oldKey.large.parentID;
	
	// update textEncoding hint (works for folders and files)
	oldData.largeFolder.textEncoding = newEncoding;

	UpdateVolumeEncodings(volume, newEncoding);
	
	TrashCatalogIterator(volume, oldParentID);			// invalidate any iterators for this folder
	InvalidateCatalogNodeCache(volume, oldParentID);	// and invalidate node cache

	//--- insert old CNode record in BTree with new key

	BuildCatalogKey(oldParentID, newName, isHFSPlus, &newKey);
			
	result = InsertBTreeRecord(volume->catalogRefNum, &newKey, &oldData, dataSize, &newHint);

	if (result == btExists)
	{
		//--- new CNode already exists, locate the existing one
		CatalogKey		existingKey;
		CatalogRecord	existingData;
		Boolean			sameNode = false;

		oldParentID = isHFSPlus ? oldKey.large.parentID : oldKey.small.parentID;

		result = LocateCatalogRecord(volume, oldParentID, newName, kNoHint, &existingKey,
									 &existingData, &newHint);
		ReturnIfError(result);

		//--- check if same CNode (same name but different upper/lower case)
			
		if ( oldData.recordType == existingData.recordType )
		{
			switch ( oldData.recordType )
			{
				case kLargeFolderRecord:
					if ( oldData.largeFolder.folderID == existingData.largeFolder.folderID )
						sameNode = true;
					break;

				case kLargeFileRecord:
					if ( oldData.largeFile.fileID == existingData.largeFile.fileID )
						sameNode = true;
					break;
			}
		}

		ReturnErrorIf(!sameNode, cmExists);

		//--- same name but different case, so delete old and insert with new name...
	
		result = DeleteBTreeRecord(volume->catalogRefNum, &oldKey);
		ReturnIfError(result);

		result = InsertBTreeRecord(volume->catalogRefNum, &newKey, &oldData, dataSize, &newHint);
	}
	else if (result == noErr)
	{
		//--- insert went ok, delete old CNode record  at <folderID><oldName>
			
		result = DeleteBTreeRecord(volume->catalogRefNum, &oldKey);
	}
	else
	{
		result = cmExists;
	}

	ReturnIfError(result);
	
	//--- update thread record if a folder (or file with thread) was renamed

	cnid = 0;
	
	switch (oldData.recordType)
	{
		case kLargeFolderRecord:
			cnid = oldData.largeFolder.folderID;
			break;

		case kLargeFileRecord:
			cnid = oldData.largeFile.fileID;
			break;
	}

	if (cnid)
	{
		UInt32			threadHint;
		CatalogKey		threadKey;
		CatalogRecord	threadRecord;
		UInt16			threadSize;
		
		result = LocateCatalogRecord(volume, cnid, NULL, kNoHint,&threadKey, &threadRecord, &threadHint);
		ReturnIfError(result);

		// update the thread's node name

		threadSize = sizeof(threadRecord.largeThread);
		// HFS Plus has varaible sized threads so adjust to actual length
		threadSize -= ( sizeof(threadRecord.largeThread.nodeName.unicode) - (newName->ustr.length * sizeof(UniChar)) );

		CopyCatalogName(newName, (CatalogName *) &threadRecord.largeThread.nodeName, isHFSPlus);
		
		result = DeleteBTreeRecord(volume->catalogRefNum, &threadKey);
		ReturnIfError(result);

		result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadRecord, threadSize, &threadHint);
	}

	ReturnIfError(result);	//€€ need to do real cleanup!!

	SetTextEncodingInHint(newEncoding, &newHint);	// sneak in the encoding

	//--- make sure changes get flushed out

	volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
	volume->vcbLsMod = GetTimeLocal();	// update last modified date
	result = FlushCatalog(volume);

	*outHint = newHint;			// return new hint

	return result;

} // end RenameCatalogNodeUnicode


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	UpdateCatalogNode
//
//	Function: 	Marks the Catalog BTree node identified by the given catalog hint
//				as 'dirty'.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr
UpdateCatalogNode(ExtendedVCB *volume, const CatalogNodeData *nodeData, UInt32 catalogHint)
{
	CatalogKey		*key;
	CatalogRecord	*record;
	FSVarsRec *		fsVars;
	UInt32			hint;
	UInt16			recordSize;
	OSErr 			result;
	
	//€€ this assumes there is a global iterator for the B-tree (true for now but will change later)
	
				
	catalogHint = GetBTreeNodeFromHint(catalogHint);
	
	// use global copy instead of going to B-tree
	fsVars = (FSVarsRec*) LMGetFSMVars();
	key	   = (CatalogKey*) &fsVars->gLastCatalogKey;
	record = (CatalogRecord*) &fsVars->gLastCatalogRecord;

	// update user modifiable fields in the catalog node record...

	switch (record->recordType)
	{
		case kSmallFolderRecord:
		{
		  #if DEBUG_BUILD
			if (nodeData->nodeType != kCatalogFolderNode)
				DebugStr("\p UpdateCatalogNode: folder/file mismatch!");
		  #endif

			record->smallFolder.createDate = nodeData->createDate;
			record->smallFolder.modifyDate = nodeData->contentModDate;
			record->smallFolder.backupDate = nodeData->backupDate;

			*(DInfo*) &record->smallFolder.userInfo = *(DInfo*) &nodeData->finderInfo;
			*(DXInfo*) &record->smallFolder.finderInfo = *(DXInfo*) &nodeData->extFinderInfo;

			recordSize = sizeof(SmallCatalogFolder);
			break;
		}

		case kSmallFileRecord:
		{
			UInt32	i;
			
		  #if DEBUG_BUILD
			if (nodeData->nodeType != kCatalogFileNode)
				DebugStr("\p UpdateCatalogNode: folder/file mismatch!");
		  #endif

			record->smallFile.flags = (UInt8) nodeData->nodeFlags;
			record->smallFile.createDate = nodeData->createDate;
			record->smallFile.modifyDate = nodeData->contentModDate;
			record->smallFile.backupDate = nodeData->backupDate;

			record->smallFile.dataLogicalSize  = nodeData->dataLogicalSize;
			record->smallFile.dataPhysicalSize = nodeData->dataPhysicalSize;
			record->smallFile.rsrcLogicalSize  = nodeData->rsrcLogicalSize;
			record->smallFile.rsrcPhysicalSize = nodeData->rsrcPhysicalSize;

			*(FInfo*) &record->smallFile.userInfo = *(FInfo*) &nodeData->finderInfo;
			*(FXInfo*) &record->smallFile.finderInfo = *(FXInfo*) &nodeData->extFinderInfo;

			// copy extent info
			for (i = 0; i < kSmallExtentDensity; ++i)
			{
				record->smallFile.dataExtents[i].startBlock	= (UInt16) nodeData->dataExtents[i].startBlock;
				record->smallFile.dataExtents[i].blockCount	= (UInt16) nodeData->dataExtents[i].blockCount;

				record->smallFile.rsrcExtents[i].startBlock	= (UInt16) nodeData->rsrcExtents[i].startBlock;
				record->smallFile.rsrcExtents[i].blockCount	= (UInt16) nodeData->rsrcExtents[i].blockCount;
			}

			recordSize = sizeof(SmallCatalogFile);
			break;
		}

		case kLargeFolderRecord:
		{
			record->largeFolder.createDate		= LocalToUTC(nodeData->createDate);
			record->largeFolder.contentModDate	= LocalToUTC(nodeData->contentModDate);
			record->largeFolder.backupDate		= LocalToUTC(nodeData->backupDate);

			BlockMoveData(&nodeData->finderInfo, &record->largeFolder.userInfo, 32);	// copy all of finder data

			recordSize = sizeof(LargeCatalogFolder);
			break;
		}

		case kLargeFileRecord:
		{
			record->largeFile.flags			 = nodeData->nodeFlags;
			record->largeFile.createDate	 = LocalToUTC(nodeData->createDate);
			record->largeFile.contentModDate = LocalToUTC(nodeData->contentModDate);
			record->largeFile.backupDate	 = LocalToUTC(nodeData->backupDate);
			
			// only copy fork data for files < 2 GB
			if ((nodeData->valence & kLargeDataForkMask) == 0)
			{
				record->largeFile.dataFork.logicalSize.lo = nodeData->dataLogicalSize;
				record->largeFile.dataFork.totalBlocks = nodeData->dataPhysicalSize / volume->blockSize;
				BlockMoveData(&nodeData->dataExtents, &record->largeFile.dataFork.extents, sizeof(LargeExtentRecord));
			}

			// only copy fork data for files < 2 GB
			if ((nodeData->valence & kLargeRsrcForkMask) == 0)
			{
				record->largeFile.resourceFork.logicalSize.lo = nodeData->rsrcLogicalSize;
				record->largeFile.resourceFork.totalBlocks = nodeData->rsrcPhysicalSize / volume->blockSize;
				BlockMoveData(&nodeData->rsrcExtents, &record->largeFile.resourceFork.extents, sizeof(LargeExtentRecord));
			}

			BlockMoveData(&nodeData->finderInfo, &record->largeFile.userInfo, 32);	// copy all of 32 bytes of Finder data

			recordSize = sizeof(LargeCatalogFile);
			break;
		}

		default:
			return cmNotFound;
	}

	result = ReplaceBTreeRecord(volume->catalogRefNum, key, catalogHint, record, recordSize, &hint);
	
	if ( result == btNotFound )
	{
		result = cmNotFound;
	}
	else if ( result == noErr )
	{
		volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
		volume->vcbLsMod = GetTimeLocal();	// update last modified date
		result = FlushCatalog(volume);		// flush the catalog
	}

	return result;
}


#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CompareCatalogKeys
//
//	Function: 	Compares two catalog keys (a search key and a trial key).
//
// 	Result:		+n  search key > trial key
//				 0  search key = trial key
//				-n  search key < trial key
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

SInt32
CompareCatalogKeys(SmallCatalogKey *searchKey, SmallCatalogKey *trialKey)
{
	CatalogNodeID	searchParentID, trialParentID;
	SInt32	result;

	searchParentID = searchKey->parentID;
	trialParentID = trialKey->parentID;

	if ( searchParentID > trialParentID ) 	// parent dirID is unsigned
		result = 1;
	else if ( searchParentID < trialParentID )
		result = -1;
	else // parent dirID's are equal, compare names
	{
	  #if ( TARGET_OS_RHAPSODY || !FORDISKFIRSTAID )
		LogStartTime(kTraceRelString);

		result = FastRelString(searchKey->nodeName, trialKey->nodeName);

		LogEndTime(kTraceRelString, noErr);
	  #else
		result = (SInt32) RelString_Glue(searchKey->nodeName, trialKey->nodeName);
	  #endif
	}

	return result;
}

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CompareExtendedCatalogKeys
//
//	Function: 	Compares two large catalog keys (a search key and a trial key).
//
// 	Result:		+n  search key > trial key
//				 0  search key = trial key
//				-n  search key < trial key
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

SInt32
CompareExtendedCatalogKeys(LargeCatalogKey *searchKey, LargeCatalogKey *trialKey)
{
	SInt32			result;
	CatalogNodeID	searchParentID, trialParentID;

	searchParentID = searchKey->parentID;
	trialParentID = trialKey->parentID;
	
	if ( searchParentID > trialParentID ) 	// parent node IDs are unsigned
	{
		result = 1;
	}
	else if ( searchParentID < trialParentID )
	{
		result = -1;
	}
	else // parent node ID's are equal, compare names
	{
		if ( searchKey->nodeName.length == 0 || trialKey->nodeName.length == 0 )
			result = searchKey->nodeName.length - trialKey->nodeName.length;
		else
			result = FastUnicodeCompare(&searchKey->nodeName.unicode[0], searchKey->nodeName.length,
										&trialKey->nodeName.unicode[0], trialKey->nodeName.length);
	}

	return result;
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	LongRename
//
//	Function: 	SPI to set a file or folder's long Unicode name.
//
//	Inputs:
//		ioVRefNum }
//		ioDirID	  }	Combine to specify file to rename
//		ioNamePtr }
//		ioNewName	Pointer to new name in 16-bit Unicode (preceding 16-bit length)
// 	Result:
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr LongRename(CMovePBRec *pb)
{
	OSErr				err;
	UInt16				nameLength;
	UInt16				*newName;
	TextEncoding		newEncoding;
	ExtendedVCB			*vcb;
	CatalogNodeID		cnid;
	FindFileNameGlueRec	fileInfo;
	UInt32				hint;
	
	//
	//	Try to find the source file
	//
	err = FindFileName( (ParamBlockRec *) pb, &fileInfo );
	if (err != noErr)	goto ErrorExit;

	//
	//	Make sure this is an HFS Plus disk
	//
	cnid = fileInfo.data->nodeID;
	vcb = fileInfo.vcb;
	if (vcb->vcbSigWord != kHFSPlusSigWord) {
		err = wrgVolTypErr;
		goto ErrorExit;
	}
	
	//
	//	Make sure the volume is writable
	//
	err = VolumeWritable( vcb );
	if (err != noErr)	goto ErrorExit;

	//
	//	Make sure the file is not locked
	//
	if (fileInfo.data->nodeFlags & kFileLockedMask) {
		err = fLckdErr;
		goto ErrorExit;
	}
	
	//
	//	Make sure new name length is OK
	//
	newName = (UInt16 *) pb->ioNewName;
	nameLength = *newName;
	if (nameLength == 0 || nameLength > kHFSPlusMaxFileNameChars) {
		err = bdNamErr;
		goto ErrorExit;
	}
	newEncoding = pb->filler2;
	
	//
	//	Rename the catalog record.
	//
	//	The record is specified by ID only (This was a simplification on my
	//	part since I didn't want to write the code to build the original file's
	//	PString name and convert to Unicode.  If there was a variant of FindFileName
	//	that returned the file's Unicode name, or the actual catalog record, this
	//	would be much simpler.)
	//
	//	The new name's text encoding is stored in the filler2 field.  Got any
	//	better ideas?
	//
	err = RenameCatalogNodeUnicode(vcb, cnid, NULL, (CatalogName *) newName, newEncoding,
									kNoHint, &hint);
	if (err != noErr)	goto ErrorExit;

	//
	//	Update VCB or FCBs with new name
	//
	if (fileInfo.id == kHFSRootParentID) {
		//	Renamed a volume, so update VCB.
		err = ConvertUnicodeToHFSName((UniStr255 *) newName, newEncoding, cnid, vcb->vcbVN);
		vcb->volumeNameEncodingHint = newEncoding;	// save encoding for FSFindTextEncodingForVolume
		MarkVCBDirty(fileInfo.vcb);
		
		//€€ Should we also update the volume name in the wrapper?
	}
	else {
		//	Renamed a file or folder.  Update FCBs.
		UInt16	refnum;			//	Used for iterating over FCBs
		UInt16	maxRefnum;		//	First refnum beyond end of FCB table
		UInt16	lengthFCB;		//	Size of an FCB
		FCB		*fcb;
		Str31	filename;		//	Holds PString version of new name
		
		//	Convert the name just once.  We'll copy from here to the FCBs as needed.
		err = ConvertUnicodeToHFSName((UniStr255 *) newName, newEncoding, cnid, filename);
		if (err != noErr)	goto ErrorExit;
		
		maxRefnum = * ((UInt16 *) LMGetFCBSPtr());
		lengthFCB = LMGetFSFCBLen();

		for (refnum=kFirstFileRefnum; refnum<maxRefnum; refnum+=lengthFCB) {
			fcb = GetFileControlBlock(refnum);
			if (fcb->fcbFlNm == cnid && fcb->fcbVPtr == vcb) {
				BlockMoveData(filename, &fcb->fcbCName, sizeof(Str31));
			}
		}
	}
	
ErrorExit:
	return err;
}

#endif