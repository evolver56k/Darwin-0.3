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
	File:		Catalog.c

	Contains:	Catalog Manager Implementation

	Version:	HFS Plus 1.0

	Copyright:	© 1996-1999 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			xxx put technology here xxx

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):
	  <Rhap>	  2/2/99	djb		Fix CreateFileIDRef to copy entire name when creating thread record.
	  <Rhap>	  1/7/99	djb		Use a max bytes of 256 in calls to ConvertUnicodeToUTF8.
	  <Rhap>	 12/9/98	djb		UpdateCatalogNode only updates vcbLsMod if contentModDate changes.
	  <Rhap>	 11/5/98	djb		Add support for UTF-8 names.
 	  <Rhap>	 8/31/98	djb		GetTimeLocal now takes an input.
	  <Rhap>	  7/8/98	ser		Added accessDate and AttributeModDate init. to create routine.
 	  <Rhap>	  6/5/98	djb		Added CreateFileIDRef routine.
	  <Rhap>	  6/3/98	djb		Merge MoveCatalogRecord and RenameCatalogRecord into one routine.
	  <Rhap>	 4/17/98	djb		Add VCB locking.
	  <Rhap>	  4/6/98	djb		Catalog iterators now need to be released.
	  <Rhap>	  4/6/98	djb		Removed CreateVolumeCatalogCache and DisposeVolumeCatalogCache (obsolete).
	  <Rhap>	 3/31/98	djb		Make UpdateCatalogNode interface thread-safe.
	  <Rhap>	 3/31/98	djb		Sync up with final HFSVolumes.h header file.
	  <Rhap>	 3/17/98	djb		Fixed CreateCatalogNode interface to take kCatalogFolderNode and
	  								kCatalogFileNode as type input.

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

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
			#include	<Errors.h>
			#include	<Files.h>
			#include	<FSM.h>
			#include	<Memory.h>
			#include	<TextUtils.h>
			#include	<Types.h>
			#include	<LowMemPriv.h>
        #else
			#include "../headers/system/MacOSTypes.h"
			#include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#include	"../headers/FileMgrInternal.h"
#include	"../headers/BTreesInternal.h"
#include	"../headers/HFSVolumes.h"
#include	"../headers/system/HFSUnicodeWrappers.h"
#include	"../headers/system/HFSInstrumentation.h"
#include	"../headers/CatalogPrivate.h"


// External routines

extern SInt32 FastRelString( ConstStr255Param str1, ConstStr255Param str2 );

extern SInt16 RelString_Glue(StringPtr pStr1, StringPtr pStr2);


// Internal routines

static OSErr	IterateCatalogNode( ExtendedVCB *volume, CatalogIterator *catalogIterator, UInt16 index,
					 				FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint );

void InitCatalogThreadRecord(ExtendedVCB *volume, UInt32 nodeType, CatalogKey *nodeKey,
							 CatalogRecord *record, UInt32 *threadSize);

void InitCatalogRecord(ExtendedVCB *volume, UInt32 nodeType, UInt32 textEncoding,
					 CatalogRecord *record, UInt32 *recordSize, HFSCatalogNodeID *catalogNodeID);

#if TARGET_OS_RHAPSODY && DIAGNOSTIC
		#include <sys/systm.h>
	    #define PRINTIT(A) kprintf A;
#else
	    #define PRINTIT(A)
#endif

//_________________________________________________________________________________
//	Exported Routines
//
//		CreateCatalogNode		-  Creates a new folder or file CNode.
//		DeleteCatalogNode		-  Deletes an existing folder or file CNode.
//		GetCatalogNode			-  Locates an existing folder or file CNode.
//		GetCatalogOffspringFile	-  Gets an offspring file record from a folder.
//		GetCatalogOffspring		-  Gets an offspring record from a folder.
//		MoveRenameCatalogNode	-  Moves/Renames an existing folder or file CNode.
//		UpdateCatalogNode		-  Marks a Catalog BTree node as 'dirty'.
//		CreateFileIDRef			-  Creates a file thread record for hfs file node
//		CompareCatalogKeys		-  Compares two catalog keys.
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


//_________________________________________________________________________________
//	Routine:	CreateCatalogNode
//
//	Function: 	Creates a new folder or file CNode.	A new folder or file
//				record is added to the catalog BTree.  If a folder CNode is
//				being created, a new thread record is also added.
//
//_________________________________________________________________________________

OSErr
CreateCatalogNode ( ExtendedVCB *volume, HFSCatalogNodeID parentID, ConstUTF8Param name,
					UInt32 nodeType, HFSCatalogNodeID *catalogNodeID, UInt32 *catalogHint)
{
	CatalogKey		nodeKey;			// 518 bytes
	CatalogRecord	nodeData;			// 520 bytes
	UInt32			nodeDataSize;
	CatalogRecord	parentThreadData;	// 520 bytes
	HFSCatalogNodeID parentsParentID;
	CatalogName		*parentNamePtr;
	UInt32			tempHint;
	UInt32			textEncoding;
	UInt16			tempSize;
	OSErr			result;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);

	if (nodeType != kCatalogFolderNode && nodeType != kCatalogFileNode)
		return paramErr;

	//--- make sure parent exists (by locating the parent's thread record)
			
	result = LocateCatalogThread(volume, parentID, &parentThreadData, &tempSize, &tempHint);
	ReturnIfError(result);

	TrashCatalogIterator(volume, parentID);		// invalidate any iterators for this parentID

	// save copy of parent's parentID and name.

	if (isHFSPlus)
	{
		if (parentThreadData.recordType != kHFSPlusFolderThreadRecord)
			return dirNFErr;
		
		parentsParentID = parentThreadData.hfsPlusThread.parentID;
		parentNamePtr = (CatalogName*) &parentThreadData.hfsPlusThread.nodeName;
	}
	else
	{
		if (parentThreadData.recordType != kHFSFolderThreadRecord)
			return dirNFErr;
		
		parentsParentID = parentThreadData.hfsThread.parentID;
		parentNamePtr = (CatalogName*) &parentThreadData.hfsThread.nodeName;
	}
	
	// invalidate cache for parent since its about to change
	InvalidateCatalogNodeCache(volume, parentsParentID);	

	//--- build key for new catalog node
	result = BuildCatalogKeyUTF8(parentID, name, isHFSPlus, &nodeKey, &textEncoding);
	ReturnIfError(result);

	//--- initialize catalog data record (for folder or file)
	InitCatalogRecord(volume, nodeType, textEncoding, &nodeData, &nodeDataSize, catalogNodeID);

	//--- add new folder/file record to catalog BTree
	result = InsertBTreeRecord(volume->catalogRefNum, &nodeKey, &nodeData, nodeDataSize, catalogHint);
	if (result)
	{
		if (result == btExists)
			result = cmExists;
		return result;
	}

	//--- build thread record for new CNode
	if (isHFSPlus || nodeType == kCatalogFolderNode)
	{
		HFSCatalogKey	threadKey;	// use the smaller key since name is null
		CatalogRecord	threadData;	// 520 bytes
		UInt32			threadSize;
		
		BuildCatalogKey(*catalogNodeID, NULL, isHFSPlus, (CatalogKey*) &threadKey);

		InitCatalogThreadRecord(volume, nodeType, &nodeKey, &threadData, &threadSize);

		//--- add thread record to catalog BTree
	
		result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadData, threadSize, &tempHint);

		//--- couldn't add thread record, delete newly created folder record and exit
		if (result)
		{
			(void) DeleteBTreeRecord(volume->catalogRefNum, &nodeKey);

			if ( result == btExists )		// <CS17>
				result = cmExists;			// remap to a catalog error

			return result;
		}
	}

	//--- update counters...

	result = UpdateFolderCount( volume, parentsParentID, parentNamePtr, nodeData.recordType, kNoHint, +1);
	ReturnIfError(result);	/* XXX what about cleanup ??? */
	
	AdjustVolumeCounts(volume, nodeData.recordType, +1);

	result = FlushCatalog(volume);

	return result;

} // end CreateCatalogNode


/*
 * initialize catalog data record (for folder or file)
 */
void InitCatalogRecord(ExtendedVCB *volume, UInt32 nodeType, UInt32 textEncoding, CatalogRecord *record, UInt32 *recordSize, HFSCatalogNodeID *catalogNodeID)
{
	HFSCatalogNodeID nodeID;
	UInt32 timeStamp;

	ClearMemory(record, sizeof(CatalogRecord));	// first clear the record

	VCB_LOCK(volume);
	nodeID = volume->vcbNxtCNID++;		// get CNID and bump next available CNode ID
	VCB_UNLOCK(volume);
	*catalogNodeID = nodeID;			// make sure it gets passed back
	
	if (volume->vcbSigWord == kHFSPlusSigWord)
	{
		timeStamp = GetTimeUTC();		// get current date/time (universal)

		UpdateVolumeEncodings(volume, textEncoding);

		if (nodeType == kCatalogFolderNode )
		{
			record->recordType = kHFSPlusFolderRecord;
			record->hfsPlusFolder.folderID = nodeID;
			record->hfsPlusFolder.createDate = timeStamp;
			record->hfsPlusFolder.contentModDate = timeStamp;
            record->hfsPlusFolder.accessDate = timeStamp;
            record->hfsPlusFolder.attributeModDate	= timeStamp;
			record->hfsPlusFolder.textEncoding = textEncoding;
			*recordSize = sizeof(HFSPlusCatalogFolder);
		//	threadType = kHFSPlusFolderThreadRecord;
		}
		else if (nodeType == kCatalogFileNode )
		{
			record->recordType = kHFSPlusFileRecord;
			record->hfsPlusFile.fileID = nodeID;
			record->hfsPlusFile.createDate = timeStamp;
            record->hfsPlusFile.contentModDate = timeStamp;
            record->hfsPlusFile.accessDate = timeStamp;
            record->hfsPlusFile.attributeModDate = timeStamp;
			record->hfsPlusFile.flags |= kHFSThreadExistsMask;
			record->hfsPlusFile.textEncoding = textEncoding;
			*recordSize = sizeof(HFSPlusCatalogFile);
		//	threadType = kHFSPlusFileThreadRecord;
		}
	}
	else /* standard hfs */
	{
		timeStamp = GetTimeLocal(true);		// get current local date/time

		if (nodeType == kCatalogFolderNode )
		{
			record->recordType = kHFSFolderRecord;
			record->hfsFolder.folderID = nodeID;
			record->hfsFolder.createDate = timeStamp;
			record->hfsFolder.modifyDate = timeStamp;
			*recordSize = sizeof(HFSCatalogFolder);
		//	threadType = kHFSFolderThreadRecord;
		}
		else if (nodeType == kCatalogFileNode )
		{
			record->recordType = kHFSFileRecord;
			record->hfsFile.fileID = nodeID;
			record->hfsFile.createDate = timeStamp;
			record->hfsFile.modifyDate = timeStamp;
			*recordSize = sizeof(HFSCatalogFile);
		}
	}
}


void InitCatalogThreadRecord(ExtendedVCB *volume, UInt32 nodeType, CatalogKey *nodeKey,
							 CatalogRecord *record, UInt32 *threadSize)
{	
	ClearMemory(record, sizeof(CatalogRecord) ); 	// first clear the record

	if (volume->vcbSigWord == kHFSPlusSigWord)
	{
		if (nodeType == kCatalogFolderNode)
  			record->recordType = kHFSPlusFolderThreadRecord;
		else
			record->recordType = kHFSPlusFileThreadRecord;
		record->hfsPlusThread.parentID = nodeKey->hfsPlus.parentID;			
		*threadSize = sizeof(record->hfsPlusThread);

		// HFS Plus has varaible sized threads so adjust to actual length
		*threadSize -= ( sizeof(record->hfsPlusThread.nodeName.unicode) -
						(nodeKey->hfsPlus.nodeName.length * sizeof(UniChar)) );
		BlockMoveData(&nodeKey->hfsPlus.nodeName, &record->hfsPlusThread.nodeName,
					  sizeof(UniChar) * (nodeKey->hfsPlus.nodeName.length + 1));
    }
    else // classic HFS
    {
		if (nodeType == kCatalogFolderNode)
			record->recordType = kHFSFolderThreadRecord;
		else
			record->recordType = kHFSFileThreadRecord;
		record->hfsThread.parentID = nodeKey->hfs.parentID;	
		*threadSize = sizeof(record->hfsThread);
		BlockMoveData(&nodeKey->hfs.nodeName, &record->hfsThread.nodeName,
					  nodeKey->hfs.nodeName[0] + 1);
    }
}


//_________________________________________________________________________________
//	Routine:	DeleteCatalogNode
//
//	Function: 	Deletes an existing folder or file CNode. The thread record
//				is also deleted for directories and files that have thread
//				records.
//
//				The valence for a folder must be zero before it can be deleted.
//				The rootfolder cannot be deleted.
//
//_________________________________________________________________________________

OSErr
DeleteCatalogNode(ExtendedVCB *volume, HFSCatalogNodeID parentID, ConstUTF8Param name, UInt32 hint)
{
	CatalogKey			key;		// 518 bytes
	CatalogRecord		data;		// 520 bytes
	UInt32				nodeHint;
	HFSCatalogNodeID	nodeID;
	HFSCatalogNodeID	nodeParentID;
	UInt32				textEncoding;
	UInt16				nodeType;
	OSErr				result;
	Boolean				isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	
	//--- locate subject catalog node

	result = BuildCatalogKeyUTF8(parentID, name, isHFSPlus, &key, &textEncoding);
	ReturnIfError(result);

	result = LocateCatalogNodeByKey(volume, hint, &key, &data, &nodeHint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, parentID, name, &key, &data, &nodeHint);
	ReturnIfError(result);

	nodeParentID = isHFSPlus ? key.hfsPlus.parentID : key.hfs.parentID;	// establish real parent cnid
	nodeType = data.recordType;		// establish cnode type
	nodeID = 0;

	switch (nodeType)
	{
		case kHFSFolderRecord:
			if (data.hfsFolder.valence != 0)		// is it empty?
				return cmNotEmpty;
			
			nodeID = data.hfsFolder.folderID;
			break;

		case kHFSPlusFolderRecord:
			if (data.hfsPlusFolder.valence != 0)		// is it empty?
				return cmNotEmpty;
			
			nodeID = data.hfsPlusFolder.folderID;
			break;

		case kHFSFileRecord:
			if (data.hfsFile.flags & kHFSThreadExistsMask)
				nodeID = data.hfsFile.fileID;
			break;

		case kHFSPlusFileRecord:
			nodeID = data.hfsPlusFile.fileID;	// note: HFS Plus files always have a thread
			break;

		default:
			return cmNotFound;
	}


	if (nodeID == fsRtDirID)	// is this the root folder?
		return cmRootCN;		// sorry, you can't delete the root!

	TrashCatalogIterator(volume, nodeParentID);			// invalidate any iterators for this parentID
	InvalidateCatalogNodeCache(volume, nodeParentID);	// and invalidate node cache

	//--- delete catalog records for CNode and file threads if they exist

	result = DeleteBTreeRecord(volume->catalogRefNum, &key);
	ReturnIfError(result);

	if ( nodeID ) 
	{
		CatalogKey threadKey;	// 518 bytes
		
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


//_________________________________________________________________________________
//	Routine:	GetCatalogNode
//
//	Function: 	Locates an existing folder or file CNode and returns an FSSpec for
//				the CNode and a pointer to the CNode data record.
//
//_________________________________________________________________________________

OSErr
GetCatalogNode( ExtendedVCB *volume, HFSCatalogNodeID parentID, ConstUTF8Param name, UInt32 hint,
				FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *newHint)
{
	CatalogKey		*key;
	CatalogRecord	*record;
	OSErr			result = noErr;
	Boolean			isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	UInt32			textEncoding;
	CatalogRecord	catalogRecord;	// 520 bytes
	CatalogKey		catalogKey;		// 518 bytes
	ByteCount		actualDstLen;

	record = &catalogRecord;
	key	   = &catalogKey;

	//--- Locate folder/file BTree record for the CNode
	result = BuildCatalogKeyUTF8(parentID, name, isHFSPlus, key, &textEncoding);
	ReturnIfError(result);
	result = LocateCatalogNodeByKey(volume, hint, key, record, newHint);
 
   // if we did not find it by name, then look for an embedded file ID in a mangled name
    if ( result == cmNotFound && isHFSPlus)
        result = LocateCatalogNodeByMangledName(volume, parentID, name, key, record, newHint);
	ReturnIfError(result);

	//--- Fill out the FSSpec (output)
	
	nodeSpec->vRefNum = volume->vcbVRefNum;
	nodeSpec->parID = isHFSPlus ? key->hfsPlus.parentID : key->hfs.parentID;
	
	if ( isHFSPlus )
	{
	//	if ( UnicodeBinaryCompare(&key->hfsPlus.nodeName, &nodeName.ustr) )
	//	{
    //		// HFS Plus Optimization: since nodeName == key->ckrCName just copy the original name!
    //		// Omit _expensive_ conversion back to ascii
	//
    //		BlockMoveData(name, nodeSpec->name, name[0] + 1);
	//	}
	//	else
		{
			result = ConvertUnicodeToUTF8(key->hfsPlus.nodeName.length * sizeof(UniChar),
										  key->hfsPlus.nodeName.unicode,
										  NAME_MAX + 1,	/* 255 + termination byte */
					 					  &actualDstLen,
										  nodeSpec->name);
		}
	}
	else // classic HFS
	{
		result = ConvertMacRomanToUTF8(key->hfs.nodeName, NAME_MAX + 1, &actualDstLen, nodeSpec->name);
	}

	//--- Fill out universal data record...
	CopyCatalogNodeData(volume, record, nodeData);

  #if DEBUG_BUILD
	if ( nodeData->nodeType != '????' )
		if ( nodeData->nodeID > volume->vcbNxtCNID || nodeData->nodeID == 0)
			DebugStr("\pGetCatalogOffspring: bad file ID found!");
  #endif
 
	return result;

} // end GetCatalogNode



//_________________________________________________________________________________
//	Routine:	GetCatalogOffspringFile
//
//	Function: 	Gets an offspring file from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring file is
//				indicated by the value of the file index (1 = 1st file offspring,
//				2 = 2nd file offspring, etc.).
//
//_________________________________________________________________________________

OSErr
GetCatalogOffspringFile( ExtendedVCB *volume, HFSCatalogNodeID folderID, UInt16 fileIndex,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	CatalogIterator *	catalogIterator;
	UInt16				index;
	UInt16				fileCounter;
	OSErr				result = noErr;


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

	(void) ReleaseCatalogIterator(catalogIterator);

	return result;

} // end GetCatalogOffspringFile


//_________________________________________________________________________________
//	Routine:	GetCatalogOffspring
//
//	Function: 	Gets an offspring record from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring CNode is
//				indicated by the value of the offspring index (1 = 1st offspring
//				CNode, 2 = 2nd offspring CNode, etc.).
//
//_________________________________________________________________________________

OSErr
GetCatalogOffspring( ExtendedVCB *volume, HFSCatalogNodeID folderID, UInt16 index,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	CatalogIterator *	catalogIterator;
	OSErr				result;


	if ( folderID == 0 )
		return cmNotFound;

	// get best catalog iterator...
	catalogIterator = GetCatalogIterator(volume, folderID, index, kIterateAll);
	
	result = IterateCatalogNode(volume, catalogIterator, index, nodeSpec, nodeData, hint);

	(void) ReleaseCatalogIterator(catalogIterator);

	return result;

} // end GetCatalogOffspring


//_________________________________________________________________________________
//	Routine:	IterateCatalogNode
//
//	Function: 	Gets an offspring record from a specified folder. The folder
//				is identified by it's folderID.  The desired offspring CNode is
//				indicated by the value of the offspring index (1 = 1st offspring
//				CNode, 2 = 2nd offspring CNode, etc.).
//
//_________________________________________________________________________________

static OSErr
IterateCatalogNode( ExtendedVCB *volume, CatalogIterator *catalogIterator, UInt16 index,
					 FSSpec *nodeSpec, CatalogNodeData *nodeData, UInt32 *hint )
{
	HFSCatalogNodeID	offspringParentID;
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
	CatalogRecord		catalogRecord;	// 520 bytes
	ByteCount			actualDstLen;

	offspringData = &catalogRecord;

	isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	fcb = GetFileControlBlock(volume->catalogRefNum);

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
		offspringParentID = offspringKey->hfsPlus.parentID;
		offspringName = (CatalogName*) &offspringKey->hfsPlus.nodeName;
	}
	else
	{
		offspringParentID = offspringKey->hfs.parentID;
		offspringName = (CatalogName*) offspringKey->hfs.nodeName;
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
		result = ConvertUnicodeToUTF8(offspringName->ustr.length * sizeof(UniChar),
									  offspringName->ustr.unicode,
									  NAME_MAX + 1,	/* 255 + termination byte */
					 				  &actualDstLen,
									  nodeSpec->name);
	}
	else /* hfs name */
	{
		result = ConvertMacRomanToUTF8(offspringName->pstr, NAME_MAX + 1, &actualDstLen, nodeSpec->name);
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


//_________________________________________________________________________________
//	Routine:	MoveRenameCatalogNode
//
//	Function: 	Moves and/or rename an existing folder or file CNode.
//				Note that when moving a folder, all decendants (its offspring,
//				their offspring, etc.) are also moved.
//
// Assumes srcHint contains a text encoding that was set by a GetCatalogNode call
//_________________________________________________________________________________

OSErr
MoveRenameCatalogNode(ExtendedVCB *volume, HFSCatalogNodeID srcParentID, ConstUTF8Param srcName,
					  UInt32 srcHint, HFSCatalogNodeID dstParentID, ConstUTF8Param dstName, UInt32 *newHint)
{
	CatalogKey			srcKey;			// 518 bytes
	CatalogRecord		srcRecord;		// 520 bytes
	CatalogKey			dstKey;			// 518 bytes
	CatalogKey			dstFolderKey;	// 518 bytes
	HFSCatalogNodeID	dstFolderParentID = 0;
	UInt32				dstFolderHint;
	CatalogName		   *dstFolderNamePtr = NULL;
	CatalogRecord		tmpRecord;		// 520 bytes
	HFSCatalogNodeID	threadID;
	UInt32				textEncoding;
	OSErr				result;
	Boolean				isNewName;
	Boolean				isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	Boolean				isOrigDeleted = false;
	short srcNameLen;
	short dstNameLen;

 
	result = BuildCatalogKeyUTF8(srcParentID, srcName, isHFSPlus, &srcKey, &textEncoding);
	ReturnIfError(result);

	/* XXX can strlen and bcmp handle NULL pointers? */

	srcNameLen = strlen(srcName);
	dstNameLen = strlen(dstName);

	//--- check if names match

	if ((srcNameLen == dstNameLen) && (bcmp(srcName, dstName, srcNameLen) == 0))
	{
		isNewName = false;
		dstKey = srcKey;
        if ( isHFSPlus ) {
            dstKey.hfsPlus.parentID = dstParentID;			// set parent ID
        }
        else {
            dstKey.hfs.parentID		= dstParentID;			// set parent ID
		}
	}
	else /* names are different */
	{
		isNewName = true;
		result = BuildCatalogKeyUTF8(dstParentID, dstName, isHFSPlus, &dstKey, &textEncoding);
		ReturnIfError(result);
	}

	//--- make sure source record exists
	
	result = LocateCatalogNodeByKey(volume, srcHint, &srcKey, &srcRecord, &srcHint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, srcParentID, srcName, &srcKey, &srcRecord, &srcHint);
	ReturnIfError(result);

	srcParentID = (isHFSPlus ? srcKey.hfsPlus.parentID : srcKey.hfs.parentID);

	// if we're moving then do some additional preflighting...

	if (srcParentID != dstParentID)
	{
		//--- make sure destination folder exists

		result = LocateCatalogNode(volume, dstParentID, NULL, kNoHint, &dstFolderKey, &tmpRecord, &dstFolderHint);
		ReturnIfError(result);
			
		if (tmpRecord.recordType == kHFSPlusFolderRecord)
		{
			dstParentID = tmpRecord.hfsPlusFolder.folderID;
			dstFolderParentID = dstFolderKey.hfsPlus.parentID;
			dstFolderNamePtr = (CatalogName*) &dstFolderKey.hfsPlus.nodeName;
		}
		else if (tmpRecord.recordType == kHFSFolderRecord)
		{
			dstParentID = tmpRecord.hfsFolder.folderID;
			dstFolderParentID = dstFolderKey.hfs.parentID;
			dstFolderNamePtr = (CatalogName*) &dstFolderKey.hfs.nodeName;
		}
		else
		{
			return badMovErr;
		}
	
		//--- if source is a folder, make sure its a proper move
	
		if (srcRecord.recordType == kHFSPlusFolderRecord || srcRecord.recordType == kHFSFolderRecord)
		{
			HFSCatalogNodeID srcFolderID;
			HFSCatalogNodeID ancestorParentID;
			CatalogKey		tempKey;	// 518 bytes
			UInt32			tempHint;
	
			if (isHFSPlus)
			{
				srcFolderID = srcRecord.hfsPlusFolder.folderID;
				ancestorParentID = dstFolderKey.hfsPlus.parentID;
			}
			else
			{
				srcFolderID = srcRecord.hfsFolder.folderID;
				ancestorParentID = dstFolderKey.hfs.parentID;
			}
	
			if ( srcFolderID == fsRtDirID	||		// source == root?
				 srcFolderID == dstParentID	||		// source == destination?
				 srcFolderID == ancestorParentID )	// source == destination's parent?
			{
				return badMovErr;
			}
	
			while (ancestorParentID > fsRtDirID)	// loop until we reach the root folder
			{
				// locate next folder up the tree...	
				result = LocateCatalogNode(volume, ancestorParentID, NULL, kNoHint, &tempKey, &tmpRecord, &tempHint);
				ReturnIfError(result);
				
				ancestorParentID = isHFSPlus ? tempKey.hfsPlus.parentID : tempKey.hfs.parentID;
	
				if (srcFolderID == ancestorParentID)	// source = destination ancestor?
					return badMovErr;
			}
		}

		TrashCatalogIterator(volume, dstParentID);		// invalidate any iterators for destination parentID
	}
	else /* (srcParentID == dstParentID) */
	{
		if ( !isNewName )
		{
			*newHint = srcHint;		// they match, so we're all done!
			return noErr;
		}
	}

	TrashCatalogIterator(volume, srcParentID);			// invalidate any iterators for source's parentID
	InvalidateCatalogNodeCache(volume, srcParentID);	// invalidate node cache since parent changed

	if (isNewName && isHFSPlus)
	{
		// update textEncoding hint (works for folders and files)
		srcRecord.hfsPlusFolder.textEncoding = textEncoding;

		UpdateVolumeEncodings(volume, textEncoding);
	}

	//--- insert source CNode record in BTree with new key (a new parent id and/or new name)

	result = InsertBTreeRecord(volume->catalogRefNum, &dstKey, &srcRecord, GetCatalogRecordSize(&srcRecord), newHint);

	if (result == btExists)
	{
		UInt16 dataSize;

		/* XXX what about the case: move id1,foo to id2,FOO ?? */
		if (srcParentID != dstParentID || isNewName == false)
			return cmExists;

		//--- new CNode name already exists in the same folder, locate the existing one
		result = SearchBTreeRecord(volume->catalogRefNum, &dstKey, srcHint,
									&dstFolderKey, &tmpRecord, &dataSize, newHint);

		if (result == btNotFound)
			result = cmNotFound;	
		ReturnIfError(result);

		//--- check if its the same CNode (same name but different upper/lower case)
			
		if (srcRecord.recordType != tmpRecord.recordType)
			return cmExists;

		switch (srcRecord.recordType)
		{
			case kHFSPlusFileRecord:	/* HFS Plus records share same cnid location */
			case kHFSPlusFolderRecord:
				if (srcRecord.hfsPlusFolder.folderID != tmpRecord.hfsPlusFolder.folderID)
					return cmExists;
				break;

			case kHFSFolderRecord:
				if (srcRecord.hfsFolder.folderID != tmpRecord.hfsFolder.folderID)
					return cmExists;
				break;

			case kHFSFileRecord:
				if (srcRecord.hfsFile.fileID != tmpRecord.hfsFile.fileID)
					return cmExists;
				break;
			
			default:
				return cmExists;
		}

		//--- same name but different case, so delete old and insert with new name...
	
		result = DeleteBTreeRecord(volume->catalogRefNum, &srcKey);
		ReturnIfError(result);
        isOrigDeleted = true;	// So we dont delete it again down below

		result = InsertBTreeRecord(volume->catalogRefNum, &dstKey, &srcRecord, dataSize, newHint);
	}
	ReturnIfError(result);

	//
	// from this point on we need to cleanup (ie delete the new record) if we encounter errors!	
	//

	//--- update thread record for node (if it exists)
	
	switch (srcRecord.recordType)
	{
		case kHFSPlusFileRecord:
		case kHFSPlusFolderRecord:
			threadID = srcRecord.hfsPlusFolder.folderID;
			break;

		case kHFSFolderRecord:
			threadID = srcRecord.hfsFolder.folderID;
			break;

		case kHFSFileRecord:
			if (srcRecord.hfsFile.flags & kHFSThreadExistsMask)
			{
				threadID = srcRecord.hfsFile.fileID;
				break;
			}
			/* fall through if no thread... */

		default:
			threadID = 0;
	}

	if (threadID)
	{
		UInt32			threadHint;
		CatalogKey		threadKey;		// 518 bytes
		CatalogRecord	threadRecord;	// 520 bytes
		UInt16			threadSize;

		result = LocateCatalogRecord(volume, threadID, NULL, kNoHint, &threadKey, &threadRecord, &threadHint);
		if (result != noErr) goto Exit_Delete;
		
		if (isHFSPlus)
		{
			if (srcParentID != dstParentID)
				threadRecord.hfsPlusThread.parentID = dstParentID;
			if (isNewName)
				CopyCatalogName((CatalogName *)&dstKey.hfsPlus.nodeName, (CatalogName *) &threadRecord.hfsPlusThread.nodeName, isHFSPlus);

			threadSize = sizeof(threadRecord.hfsPlusThread);
			// HFS Plus has varaible sized threads so adjust to actual length
			threadSize -= ( sizeof(threadRecord.hfsPlusThread.nodeName.unicode) - (threadRecord.hfsPlusThread.nodeName.length * sizeof(UniChar)) );
		}
		else
		{
			if (srcParentID != dstParentID)
				threadRecord.hfsThread.parentID = dstParentID;
			if (isNewName)
				CopyCatalogName((CatalogName *)&dstKey.hfs.nodeName,(CatalogName *) threadRecord.hfsThread.nodeName, isHFSPlus);

			threadSize = sizeof(threadRecord.hfsThread);
		}

		result = DeleteBTreeRecord(volume->catalogRefNum, &threadKey);
		if (result != noErr) goto Exit_Delete;

		result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadRecord, threadSize, &threadHint);
		if (result != noErr) goto Exit_Delete;	//XXX exiting with a missing thread!
	}

	//--- we successfully added the new node so delete the old source CNode record

    if (! isOrigDeleted) {
        result = DeleteBTreeRecord(volume->catalogRefNum, &srcKey);
        if (result)
            {
            // uh oh, we could not delete the original
            // so we better get rid of the new node...

            (void) DeleteBTreeRecord(volume->catalogRefNum, &dstKey);

            //XXX also need to fix up the thread...

            return result;
            }
    }

	if (srcParentID != dstParentID)
	{
		result = UpdateFolderCount(volume, srcParentID, NULL, srcRecord.recordType, kNoHint, -1);
		result = UpdateFolderCount(volume, dstFolderParentID, dstFolderNamePtr, srcRecord.recordType, dstFolderHint, +1);
	}

	//--- make sure changes get flushed out
	VCB_LOCK(volume);
	 volume->vcbFlags |= 0xFF00;		// Mark the VCB dirty
	 volume->vcbLsMod = GetTimeLocal(!isHFSPlus);	// update last modified date
	VCB_UNLOCK(volume);

	(void) FlushCatalog(volume);
	
	return result;


Exit_Delete:
	(void) DeleteBTreeRecord(volume->catalogRefNum, &dstKey);

	return result;

} // end MoveRenameCatalogNode


//_________________________________________________________________________________
//	Routine:	UpdateCatalogNode
//
//	Function: 	Marks the Catalog BTree node identified by the given catalog hint
//				as 'dirty'.
//
//_________________________________________________________________________________


OSErr
UpdateCatalogNode(ExtendedVCB *volume, HFSCatalogNodeID parentID, ConstUTF8Param name,
				  UInt32 catalogHint, const CatalogNodeData *nodeData)
{
	CatalogKey		*key;
	CatalogRecord	*record;
	UInt32			hint;
	UInt16			recordSize;
	OSErr 			result;		
	TextEncoding	textEncoding;
	CatalogKey		catalogKey;		// 518 bytes
	CatalogRecord	catalogRecord;	// 520 bytes
	Boolean			isHFSPlus = volume->vcbSigWord == kHFSPlusSigWord;

	/* XXX no reason to have ptrs to local variables... */
	key	   = &catalogKey;
	record = &catalogRecord;
	
	result = BuildCatalogKeyUTF8(parentID, name, isHFSPlus, key, &textEncoding);
	ReturnIfError(result);

	//--- locate subject catalog node

	result = LocateCatalogNodeByKey(volume, catalogHint, key, record, &hint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, parentID, name, key, record, &hint);

	if (result == btNotFound)
		result = cmNotFound;

	if (catalogHint != hint)
		PRINTIT(("UpdateCatalogNode: catalogHint does not match (in: %ld, out: %ld)\n", catalogHint, hint));
	ReturnIfError(result);

	// update user modifiable fields in the catalog node record...

	switch (record->recordType)
	{
		case kHFSFolderRecord:
		{
		  #if DEBUG_BUILD
			if (nodeData->nodeType != kCatalogFolderNode)
				DebugStr("\p UpdateCatalogNode: folder/file mismatch!");
		  #endif

			record->hfsFolder.createDate = nodeData->createDate;
			record->hfsFolder.modifyDate = nodeData->contentModDate;
			record->hfsFolder.backupDate = nodeData->backupDate;

			*(DInfo*) &record->hfsFolder.userInfo = *(DInfo*) &nodeData->finderInfo;
			*(DXInfo*) &record->hfsFolder.finderInfo = *(DXInfo*) &nodeData->extFinderInfo;

			recordSize = sizeof(HFSCatalogFolder);
			break;
		}

		case kHFSFileRecord:
		{
			UInt32	i;
			
		  #if DEBUG_BUILD
			if (nodeData->nodeType != kCatalogFileNode)
				DebugStr("\p UpdateCatalogNode: folder/file mismatch!");
		  #endif

			record->hfsFile.flags = (UInt8) nodeData->nodeFlags;
			record->hfsFile.createDate = nodeData->createDate;
			record->hfsFile.modifyDate = nodeData->contentModDate;
			record->hfsFile.backupDate = nodeData->backupDate;

			record->hfsFile.dataLogicalSize  = nodeData->dataLogicalSize;
			record->hfsFile.dataPhysicalSize = nodeData->dataPhysicalSize;
			record->hfsFile.rsrcLogicalSize  = nodeData->rsrcLogicalSize;
			record->hfsFile.rsrcPhysicalSize = nodeData->rsrcPhysicalSize;

			*(FInfo*) &record->hfsFile.userInfo = *(FInfo*) &nodeData->finderInfo;
			*(FXInfo*) &record->hfsFile.finderInfo = *(FXInfo*) &nodeData->extFinderInfo;

			// copy extent info
			for (i = 0; i < kHFSExtentDensity; ++i)
			{
				record->hfsFile.dataExtents[i].startBlock	= (UInt16) nodeData->dataExtents[i].startBlock;
				record->hfsFile.dataExtents[i].blockCount	= (UInt16) nodeData->dataExtents[i].blockCount;

				record->hfsFile.rsrcExtents[i].startBlock	= (UInt16) nodeData->rsrcExtents[i].startBlock;
				record->hfsFile.rsrcExtents[i].blockCount	= (UInt16) nodeData->rsrcExtents[i].blockCount;
			}

			recordSize = sizeof(HFSCatalogFile);
			break;
		}

		case kHFSPlusFolderRecord:
		{
			record->hfsPlusFolder.createDate		= LocalToUTC(nodeData->createDate);
			record->hfsPlusFolder.contentModDate	= LocalToUTC(nodeData->contentModDate);
			record->hfsPlusFolder.backupDate		= LocalToUTC(nodeData->backupDate);
#if TARGET_OS_RHAPSODY
            record->hfsPlusFolder.accessDate	 	= LocalToUTC(nodeData->accessDate);
            record->hfsPlusFolder.attributeModDate	= LocalToUTC(nodeData->attributeModDate);
            record->hfsPlusFolder.permissions		= nodeData->permissions;
#endif

			BlockMoveData(&nodeData->finderInfo, &record->hfsPlusFolder.userInfo, 32);	// copy all of finder data

			recordSize = sizeof(HFSPlusCatalogFolder);
			break;
		}

		case kHFSPlusFileRecord:
		{
			record->hfsPlusFile.flags			 = nodeData->nodeFlags;
			record->hfsPlusFile.createDate	 = LocalToUTC(nodeData->createDate);
			record->hfsPlusFile.contentModDate = LocalToUTC(nodeData->contentModDate);
			record->hfsPlusFile.backupDate	 = LocalToUTC(nodeData->backupDate);
#if TARGET_OS_RHAPSODY
            record->hfsPlusFile.accessDate	 = LocalToUTC(nodeData->accessDate);
            record->hfsPlusFile.attributeModDate	 = LocalToUTC(nodeData->attributeModDate);
            record->hfsPlusFile.permissions	 = nodeData->permissions;
#endif

			record->hfsPlusFile.dataFork.logicalSize.lo = nodeData->dataLogicalSize;
			record->hfsPlusFile.dataFork.totalBlocks = nodeData->dataPhysicalSize / volume->blockSize;
			BlockMoveData(&nodeData->dataExtents, &record->hfsPlusFile.dataFork.extents, sizeof(HFSPlusExtentRecord));

			record->hfsPlusFile.resourceFork.logicalSize.lo = nodeData->rsrcLogicalSize;
			record->hfsPlusFile.resourceFork.totalBlocks = nodeData->rsrcPhysicalSize / volume->blockSize;
			BlockMoveData(&nodeData->rsrcExtents, &record->hfsPlusFile.resourceFork.extents, sizeof(HFSPlusExtentRecord));
			
			BlockMoveData(&nodeData->finderInfo, &record->hfsPlusFile.userInfo, 32);	// copy all of 32 bytes of Finder data

			recordSize = sizeof(HFSPlusCatalogFile);
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
		/* if we're just updating the accessDate then no need to change volume mod date */
		if (nodeData->contentModDate > volume->vcbLsMod ||
			(isHFSPlus && nodeData->attributeModDate > volume->vcbLsMod))
		{
			VCB_LOCK(volume);
			volume->vcbFlags |= 0xFF00;			// Mark the VCB dirty
			volume->vcbLsMod = GetTimeLocal(!isHFSPlus);	// update last modified date
			VCB_UNLOCK(volume);
		}

		result = FlushCatalog(volume);		// flush the catalog
	}

	return result;
}


//_________________________________________________________________________________
//	Routine:	CreateFileIDRef
//
//	Function: 	Creates a file thread record for hfs file node
//
//_________________________________________________________________________________

OSErr
CreateFileIDRef(ExtendedVCB *volume, HFSCatalogNodeID parentID, ConstUTF8Param name, UInt32 hint, HFSCatalogNodeID *threadID)
{
	CatalogKey			nodeKey;	// 518 bytes
	CatalogRecord		nodeData;	// 520 bytes
	HFSCatalogKey		threadKey;
	HFSCatalogThread	threadData;
	UInt32				nodeHint;
	UInt32				tempHint;
	OSErr				result;
	Boolean				isHFSPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	UInt32				textEncoding;
	
	*threadID = 0;
	
	result = BuildCatalogKeyUTF8(parentID, name, isHFSPlus, &nodeKey, &textEncoding);
	ReturnIfError(result);

	//--- locate subject catalog node

	result = LocateCatalogNodeByKey(volume, hint, &nodeKey, &nodeData, &nodeHint);

	// if we did not find it by name, then look for an embedded file ID in a mangled name
	if ( (result == cmNotFound) && isHFSPlus )
		result = LocateCatalogNodeByMangledName(volume, parentID, name, &nodeKey, &nodeData, &nodeHint);
	ReturnIfError(result);
	
    if (nodeData.recordType == kHFSPlusFileRecord)
	{
		*threadID = nodeData.hfsPlusFile.fileID;
		return noErr;	// already have one
	}
	
	if (nodeData.recordType != kHFSFileRecord)
	{
		return notAFileErr;
	}


	if (nodeData.hfsFile.flags & kHFSThreadExistsMask)
	{
		*threadID = nodeData.hfsFile.fileID;
		return noErr;	// already have one
	}

	result = VolumeWritable( volume );
	if ( result != noErr ) return result;

	//
	// need to insert a thread record
	//		
    BuildCatalogKey(nodeData.hfsFile.fileID, NULL, false, (CatalogKey *)&threadKey);
	
	ClearMemory(&threadData, sizeof(HFSCatalogThread));
    threadData.recordType = kHFSFileThreadRecord;
	threadData.parentID = nodeKey.hfs.parentID;	
	BlockMoveData(&nodeKey.hfs.nodeName, &threadData.nodeName, nodeKey.hfs.nodeName[0] + 1);

	result = InsertBTreeRecord(volume->catalogRefNum, &threadKey, &threadData, sizeof(HFSCatalogThread), &tempHint);
	if (result == btExists) result = noErr;		//XXX could return cmExists or fidExists
	ReturnIfError(result);

	//
	//	Finally, set the flag in the file record to say this file has a thread record.
	//
	nodeData.hfsFile.flags |= kHFSThreadExistsMask;
	result = ReplaceBTreeRecord(volume->catalogRefNum, &nodeKey, nodeHint, &nodeData, sizeof(HFSCatalogFile), &nodeHint );

	if (result == noErr) {
		(void) FlushCatalog(volume);
		*threadID = nodeData.hfsFile.fileID;
	}

	return result;
}


//_________________________________________________________________________________
//	Routine:	CompareCatalogKeys
//
//	Function: 	Compares two catalog keys (a search key and a trial key).
//
// 	Result:		+n  search key > trial key
//				 0  search key = trial key
//				-n  search key < trial key
//_________________________________________________________________________________

SInt32
CompareCatalogKeys(HFSCatalogKey *searchKey, HFSCatalogKey *trialKey)
{
	HFSCatalogNodeID	searchParentID, trialParentID;
	SInt32	result;

	searchParentID = searchKey->parentID;
	trialParentID = trialKey->parentID;

	if ( searchParentID > trialParentID ) 	// parent dirID is unsigned
		result = 1;
	else if ( searchParentID < trialParentID )
		result = -1;
	else // parent dirID's are equal, compare names
	{
	  #if ( ! FORDISKFIRSTAID )
		LogStartTime(kTraceRelString);

		result = FastRelString(searchKey->nodeName, trialKey->nodeName);

		LogEndTime(kTraceRelString, noErr);
	  #else
		result = (SInt32) RelString_Glue(searchKey->nodeName, trialKey->nodeName);
	  #endif
	}

	return result;
}


//_________________________________________________________________________________
//	Routine:	CompareExtendedCatalogKeys
//
//	Function: 	Compares two large catalog keys (a search key and a trial key).
//
// 	Result:		+n  search key > trial key
//				 0  search key = trial key
//				-n  search key < trial key
//_________________________________________________________________________________

SInt32
CompareExtendedCatalogKeys(HFSPlusCatalogKey *searchKey, HFSPlusCatalogKey *trialKey)
{
	SInt32			result;
	HFSCatalogNodeID	searchParentID, trialParentID;

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

