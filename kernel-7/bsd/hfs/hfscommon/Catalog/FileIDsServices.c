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
	File:		FileIDServices.c

	Contains:	File ID manipulating routines.

	Version:	HFS Plus 1.0

	Written by:	Deric Horn

	Copyright:	© 1996-1999 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(JL)	Jim Luther
		(msd)	Mark Day
		(djb)	Don Brady
		(DSH)	Deric Horn

	Change History (most recent first):
	  <Rhap>	 3/2/98		djb		Fix extents corruption bug in MoveExtents (radar #2309434).
	  <Rhap>	11/20/98	djb		Add support for UTF-8 names.
	  <Rhap>	  4/2/98	djb		Switch over to real BTree interface in MoveExtents and DeleteExtents.
	  <Rhap>	 3/31/98	djb		Sync up with final HFSVolumes.h header file.

	  <CS21>	11/17/97	djb		PrepareInputName routine now returns an error.
	  <CS20>	11/13/97	djb		Radar #2001699 ResolveFileID needs to use CMNotFound error.
	  <CS19>	10/31/97	JL		#2000184 - CreateFileThreadID and ExchangeFiles now return the
									WDCBRecPtr or NULL for external file systems. ExchangeFiles no
									longer returns length of FCB table to caller since that wasn't
									ever needed.
		<18>	10/23/97	DSH		1685058, Fix ExchangeFiles by invalidating the node cache before
									switching the files.
	  <CS17>	10/19/97	msd		Bug 1684586. GetCatInfo and SetCatInfo use only contentModDate.
	  <CS16>	10/16/97	DSH		Return badFidErr in ResolveFileID if LocateCatalogThread fails
	  <CS15>	10/15/97	DSH		CreateFileThreadID(), remap btExists to fidExists.
	  <CS14>	  9/7/97	djb		Turn off some DebugStr calls.
	  <CS13>	  9/4/97	msd		Remove call to PropertyExchangeObjects.
	  <CS12>	 8/14/97	djb		Remove hard link support.
	  <CS11>	 7/18/97	msd		Include LowMemPriv.h.
	  <CS10>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS9>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS8>	 6/24/97	djb		Add hard link support to ResolveFileID and CreateFileIDRef.
	   <CS7>	 6/20/97	msd		Use contentModDate and attributeModDate fields instead of
									modifyDate.
	   <CS6>	 6/13/97	djb		Switch over from PrepareOutputName to ConvertUnicodeToHFSName.
									PrepareInputName now takes an encoding.
	   <CS5>	 5/28/97	msd		Move the declaration of FindFileName to FilesInternal.i.
	   <CS4>	 5/19/97	djb		No longer need to invalidate vcbDirIDM field.
	   <CS3>	 5/16/97	msd		In ExchangeFiles, change srcNamePtr from char * to StringPtr
									(fixes warnings).
	   <CS2>	 4/28/97	djb		(DSH) Added VolumeWritable check back into CreateFileIDThread.
	   <CS1>	 4/24/97	djb		first checked in
	 <HFS23>	 4/11/97	DSH		Use extended VCB fields catalogRefNum, and extentsRefNum.
	 <HFS22>	  4/9/97	msd		Rewrite CreateFileThreadID so that it properly handles
									pathnames, and doesn't overwrite the ioNamePtr. The data field
									of FindFileNameGlueRec points to a CatalogNodeData, not
									CatalogRecord.
	 <HFS21>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS20>	 3/31/97	djb		Change ClearMem to ClearMemory.
	 <HFS19>	 3/17/97	DSH		C_FlushCache prototype to FilesInternal.h
	 <HFS18>	  3/5/97	msd		ExchangeFiles needs to call PropertyExchangeObjects.
	 <HFS17>	 2/13/97	msd		Fix MoveExtents and DeleteExtents to work with HFS+ extent
									records.
	 <HFS16>	 1/31/97	msd		In MoveExtents, when a record isn't found and you want the next
									record in order, use the "next record" offset = 1 instead of
									"current record" offset = 0. DeleteExtents would always exit
									without doing anything because it was searching for an invalid
									key. Removed several DebugStrs that were used as cheap code
									coverage.
	 <HFS15>	 1/15/97	DSH		Resolve wasn't passing the name back for HFS
	 <HFS14>	 1/13/97	djb		LocateCatalogThread now passes back the thread record size.
	 <HFS13>	 1/11/97	DSH		HFS+, fixed some Unicode/Pascal strings related bugs for use on
									HFS+ volumes.
	 <HFS12>	  1/9/97	DSH		Fix ExchangeFiles extents
	 <HFS11>	  1/6/97	DSH		pass VCB in CloseFile() routine.
	 <HFS10>	  1/6/97	djb		Fixed ResolveFileID - it was not returning a directory ID!
	  <HFS9>	  1/3/97	msd		Fix prototype for C_FlushCache. Fix prototype for
									TrashFileBlocks.
	  <HFS8>	  1/3/97	djb		Integrate latest HFSVolumesPriv.h changes.
	  <HFS7>	  1/2/97	DSH		C port of ExchangeFileIDs
	  <HFS6>	12/20/96	djb		Fixed bug in CreateFileID.
	  <HFS5>	12/19/96	DSH		All refs to VCB are now refs to ExtendedVCB
	  <HFS4>	12/19/96	msd		Use kFileThreadExistsMask (from HFSVolumesPriv.h) instead of
									kFileThreadMask (from FilesInternal.h) since the latter was
									incorrectly defined and has now been removed.
	  <HFS3>	12/19/96	djb		Updated for new B-tree Manager interface.
	  <HFS2>	12/18/96	msd		GetFileThreadID was using a bitwise-OR (|) instead of
									bitwise-AND (&) to test for a bit being set.
	  <HFS1>	12/12/96	DSH		first checked in

*/

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
        #include	<Types.h>
        #include	<Files.h>
        #include	<LowMemPriv.h>
        #else
        #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#include	"../headers/FileMgrInternal.h"
#include	"../headers/HFSVolumes.h"
#include	"../headers/system/HFSUnicodeWrappers.h"
#include	"../headers/CatalogPrivate.h"


struct ExtentsRecBuffer {
	ExtentKey		extentKey;
	ExtentRecord	extentData;
};
typedef struct ExtentsRecBuffer ExtentsRecBuffer;


OSErr	CreateFileID( ExtendedVCB *vcb, HFSCatalogNodeID fileID, CatalogName *name, HFSCatalogNodeID *threadID );
OSErr	GetFileThreadID( ExtendedVCB *vcb, HFSCatalogNodeID id, const CatalogName *name, Boolean isHFSPlus, UInt32 *threadID );

UInt32	CheckExtents( void *extents, UInt32 blocks, Boolean isHFSPlus );
OSErr	DeleteExtents( ExtendedVCB *vcb, UInt32 fileNumber, Boolean isHFSPlus );
OSErr	MoveExtents( ExtendedVCB *vcb, UInt32 srcFileID, UInt32 destFileID, Boolean isHFSPlus );
void	CopyCatalogNodeInfo( CatalogRecord *src, CatalogRecord *dest );
void	CopyBigCatalogNodeInfo( CatalogRecord *src, CatalogRecord *dest );

void	CopyExtentInfo( ExtentKey *key, ExtentRecord *data, ExtentsRecBuffer *buffer, UInt16 bufferCount );
extern	void	TrashFileBlocks( ExtendedVCB *vcb, UInt32 fileNumber );



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	ResolveFileID from Asm : FIDResolveID
//
//	Function: 	ResolveFileID finds the dirID and CName for a file, given it's thread.
//
// 	Output:		OSErr				err
//				Str31				name - resolved name
//				HFSCatalogNodeID	parentID - resolved parent ID
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

#if TARGET_OS_MAC

OSErr	ResolveFileID( ExtendedVCB *vcb, HFSCatalogNodeID fileID, HFSCatalogNodeID *parentID, Str31 name )
{
	OSErr			err;
	CatalogRecord	threadData;
	CatalogKey		key;
	CatalogRecord	data;
	UInt32 			threadHint;
	UInt32 			tempHint;
	UInt16			threadSize;
	Boolean			isHFSPlus;
	
	
	err = CheckVolumeOffLine( vcb );
	ReturnIfError( err );

	isHFSPlus = ( vcb->vcbSigWord == kHFSPlusSigWord );
	
	//-- if it's not an HFS Plus volume, make sure it's an HFS volume
	if ( (! isHFSPlus) && (vcb->vcbSigWord != kHFSSigWord) )
		return( wrgVolTypErr );

	threadHint = kNoHint;
	
	//-- Locate the file thread
	
	err = LocateCatalogThread( vcb, fileID, &threadData, &threadSize, &threadHint );	//	This call returns nodeName as either Str31 or HFSUniStr255, no need to call PrepareInputName()
	if ( err != noErr )
		return( cmNotFound );	//	treat it as fidNotFound error
	

	if ( (threadData.recordType != kHFSFileThreadRecord) && (threadData.recordType != kHFSPlusFileThreadRecord) )
		return( cmFThdDirErr );
		
		
	isHFSPlus = ( vcb->vcbSigWord == kHFSPlusSigWord );

	if ( isHFSPlus )
	{
		TextEncoding	encoding;
		ByteCount actualDstLen;

		//-- Check if the file exists

		err = LocateCatalogNode( vcb, threadData.hfsPlusThread.parentID, (const CatalogName *) &(threadData.hfsPlusThread.nodeName), kNoHint, &key, &data, &tempHint);

		//-- We found the file, but is this the file we really want?

		if ( err || data.hfsPlusFile.fileID != fileID )
			return( badFidErr );

		encoding = data.hfsPlusFile.textEncoding;	// note: text encoding and fileID is at same offset for folders

		//	Copy the name back for output
		err = ConvertUnicodeToUTF8(	threadData.hfsPlusThread.nodeName.length * sizeof(UniChar),
									threadData.hfsPlusThread.nodeName.unicode,
									NAME_MAX+1,
					 				&actualDstLen,
									name);
		*parentID = threadData.hfsPlusThread.parentID;
	}
	else
	{
		//-- Check if the file exists
		//		No need to prepare name for in/output, it will stay a Str31 in HFS
	
		err = LocateCatalogNode( vcb, threadData.hfsThread.parentID, (const CatalogName *) &(threadData.hfsThread.nodeName), kNoHint, &key, &data, &tempHint );

		//-- We found the file, but is this the file we really want?
		
		if ( err || data.hfsFile.fileID != fileID )
			return( badFidErr );

		//	Copy the name back for output
		BlockMoveData( threadData.hfsThread.nodeName, name, StrLength(threadData.hfsThread.nodeName) + 1 );
		*parentID = threadData.hfsThread.parentID;
	}
	
	return( noErr );
}

#endif	/* TARGET_OS_MAC */


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetFileThreadID from Asm : FIDGetID
//
//	Function: 	GetFileThreadID returns a file thread to a file if it exists.
//
//	Input:		ExtendedVCB*		-  VCB pointer
//				HFSCatalogNodeID	-  DirID
//				CatalogName			-  CName pointer
//
//	Output:		OSErr	-  result code
//							 0 = ok
//							 CMnotfound = CNode not found
//							 cmFThdGone = File thread not found
//							 cmFThdDirErr = file is a directory, not a file
//							 -n = IO Error
//				UInt32	-  file thread ID
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

#if TARGET_OS_MAC

OSErr	GetFileThreadID( ExtendedVCB *vcb, HFSCatalogNodeID id, const CatalogName *name, Boolean isHFSPlus, UInt32 *threadID )
{
	OSErr			err;
	CatalogKey		key;
	CatalogRecord	data;
	UInt32 			hint;
	

	*threadID = 0;
	
	//-- Locate the file thread
	
	err = LocateCatalogNode( vcb, id, name, kNoHint, &key, &data, &hint );
	ReturnIfError( err );
	
	if ( isHFSPlus )
	{
		if ( data.recordType != kHFSPlusFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"
		
		*threadID = data.hfsPlusFile.fileID;
	}
	else
	{
		if ( data.recordType != kHFSFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"

		if ( (data.hfsFile.flags & kHFSThreadExistsMask) != 0 )
			*threadID = data.hfsFile.fileID;
		else
			err = cmFThdGone;
	}
	
	return( err );
}
#endif	/* TARGET_OS_MAC */



#if TARGET_OS_MAC
OSErr	CreateFileThreadID( FIDParam *filePB, WDCBRecPtr *wdcbPtr )
{
	OSErr				err;
	Boolean				isHFSPlus;
	UInt32				fileHint;
	UInt32				threadHint;
	Str31				name;
	FindFileNameGlueRec	fileInfo;
	HFSCatalogKey		threadKey;
	HFSCatalogThread	threadData;
	CatalogKey			fileKey;
	CatalogRecord		fileData;
	
	
	err = FindFileName( (ParamBlockRec *) filePB, &fileInfo );
	*wdcbPtr = fileInfo.wdcb;	// return the WDCB pointer to the glue so it can set up A3.
	if (err != noErr)	goto ErrorExit;
	
	isHFSPlus = ( fileInfo.vcb->vcbSigWord == kHFSPlusSigWord );

	//
	//	Make sure we just found a file record.
	//
	if (fileInfo.data->nodeType != kCatalogFileNode) {
		err = notAFileErr;
		goto ErrorExit;
	}
	
	//
	//	Figure out which FileID this is, and return it.
	//
	filePB->ioFileID = fileInfo.data->nodeID;
	
	//
	//	If the volume is HFS, see if we need to create the thread record.  For HFS Plus, there
	//	is always a thread record.
	//

	if (isHFSPlus || fileInfo.data->nodeFlags & kFileThreadExistsMask) {
		err = fidExists;
		goto ErrorExit;
	}

	err = VolumeWritable( fileInfo.vcb );
	if ( err != noErr )
		goto ErrorExit;

	
	//
	//	If we get here, we need to create a thread record on an HFS volume.
	//	Start by building finding the original catalog record, so we can get
	//	its key with correct capitalization, etc.
	//
	//€€ Is there any way to get the actual key back from FindFileName?
	//
	name[0] = fileInfo.nameLength;
	BlockMoveData(fileInfo.nameBuffer, &name[1], fileInfo.nameLength);
	err = LocateCatalogNode( fileInfo.vcb, fileInfo.id, (CatalogName *) &name, fileInfo.hint, &fileKey, &fileData, &fileHint );
	if (err != noErr)	goto ErrorExit;
		
	//
	//	Build the key and data for the thread record.
	//

	BuildCatalogKey( filePB->ioFileID, NULL, false, (CatalogKey *) &threadKey );

	ClearMemory( (Ptr)&threadData, sizeof(HFSCatalogThread) );
	threadData.recordType	= kHFSFileThreadRecord;
	threadData.parentID	= fileKey.hfs.parentID;
	BlockMoveData(&fileKey.hfs.nodeName[0], &threadData.nodeName[0], fileKey.hfs.nodeName[0]+1);

	//
	//	Insert the thread record
	//
	err = InsertBTreeRecord( fileInfo.vcb->catalogRefNum, &threadKey, &threadData, sizeof(HFSCatalogThread), &threadHint );
	if (err != noErr)
	{
		if ( err == btExists )	err = fidExists;		// Remap btExists to fidExists
		goto ErrorExit;
	}

	//
	//	Finally, set the flag in the file record to say this file has a thread record.
	//
	fileData.hfsFile.flags |= kFileThreadExistsMask;
	err = ReplaceBTreeRecord( fileInfo.vcb->catalogRefNum, &fileKey, fileHint, &fileData, sizeof(HFSCatalogFile), &fileHint );
	if (err != noErr)	goto ErrorExit;

	FlushCatalog( fileInfo.vcb );
	
ErrorExit:

	return err;
}
#endif	/* TARGET_OS_MAC */


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	DeleteFileID from Asm : FIDDeleteID
//
//	Function: 	DeleteFileID invalidates a file id, by removing the thread from the 
//				from the cat file and by turning off the file CNode link flag.
//
// 	Output:		OSErr	err
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//
//	We don't know why anyone would use this routine, so in HFS Plus we want to obsolete it
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

#if( 0 )
	OSErr	DeleteFileID( ExtendedVCB *vcb, HFSCatalogNodeID fileID )
	{
		OSErr			err;
		OSErr			btError;
		Boolean			isHFSPlus;
		CatalogRecord	*threadData;
		CatalogKey		key;
		CatalogRecord	record;
		UInt32 			threadHint;
		UInt16			threadSize;
		CatalogKey		key;
	
		
		//-- Locate the file thread
	
		threadHint = kNoHint;
		
		err = LocateCatalogThread( vcb, fileID, &threadData, &threadSize, &threadHint );
		
		if ( err != noErr )
		{		//	translate the error so callee knows it wasn't a fnfErr
			if ( err != cmNotFound )
				err = cmFThdGone;
			return( err );
		}
	
		if ( (threadData->recordType != kHFSFileThreadRecord) && (threadData->recordType != kHFSPlussFileThreadRecord) )
			return( cmFThdDirErr );
	
		//-- Find the file
	
		isHFSPlus = ( vcb->vcbSigWord == kHFSPlusSigWord );
	
		if ( isHFSPlus )
		{
			err = LocateCatalogNode( vcb, threadData->hfsPlusThread.parentID, (const CatalogName *) &(threadData->hfsPlusThread.nodeName), kNoHint, &key, &record, &threadHint );
	
			if ( err == noErr )
			{		//	Turn off thread flag in file CNode
				record.hfsPlusFile.flags &= ~kFileThreadExistsMask;		//	clear the fThreadFlag bit
				err = ReplaceBTreeRecord( vcb->catalogRefNum, &key, threadHint, &record, sizeof(CatalogThreadRecord), &threadHint);	//	tell the B-Tree
				ReturnIfError( err );
			}
		}
		else
		{
			err = LocateCatalogNode( vcb, threadData->hfsThread.parentID, (const CatalogName *) &(threadData->hfsThread.nodeName), kNoHint, &key, &record, &threadHint );
	
			if ( err == noErr )
			{		//	Turn off thread flag in file CNode
				record.hfsFile.flags &= ~kFileThreadExistsMask;			//	clear the fThreadFlag bit
				err = ReplaceBTreeRecord( vcb->catalogRefNum, &key, threadHint, &record, sizeof(CatThdRec), &threadHint);			//	tell the B-Tree
				ReturnIfError( err );
			}
		}
	
		//--	Delete the file thread in the catalog btree
			
		BuildCatalogKey( fileID, nil, isHFSPlus, &key );
		
		btError = DeleteBTreeRecord( vcb->catalogRefNum, &key );
		ReturnIfError( btError );										//	we know it exists, so must be IOError...
																		//	...Oh well, leave the cnode's fthread flag off.
		//-- Get it out to disk. 
		(void) FlushCatalog( vcb );		
		
		return( err );
	}



#if TARGET_OS_MAC
OSErr	ExchangeFiles( FIDParam *filePB, WDCBRecPtr *wdcbPtr )
{
	FindFileNameGlueRec		srcFileInfo;
	FindFileNameGlueRec		destFileInfo;
	OSErr					err;
	ExtendedVCB				*vcb;
	CatalogName				srcName;
	CatalogName				destName;
	Str31					pSrcName;
	Str31					pDestName;
	CatalogKey				srcKey;
	CatalogRecord			srcData;
	CatalogKey				destKey;
	CatalogRecord			destData;
	HFSCatalogNodeID 		srcFileID;
	HFSCatalogNodeID 		destFileID;
	UInt32					srcHint;
	UInt32					destHint;
	UInt32					srcDirID;
	StringPtr				srcNamePtr;
	Boolean					isHFSPlus;
	Ptr						fcbs;
	UInt16					fcbIndex;
	FCB						*fcb;
	
	err = FindFileName( (ParamBlockRec *) filePB, &srcFileInfo );
	*wdcbPtr = srcFileInfo.wdcb;	// return the WDCB pointer to the glue so it can set up A3.
	ReturnIfError( err );

	srcNamePtr	= filePB->ioNamePtr;
	srcDirID	= filePB->ioSrcDirID;
	
	filePB->ioNamePtr	= filePB->ioDestNamePtr;
	filePB->ioSrcDirID	= filePB->ioDestDirID;

	err = FindFileName( (ParamBlockRec *) filePB, &destFileInfo );

	filePB->ioNamePtr	= srcNamePtr;
	filePB->ioSrcDirID	= srcDirID;

	ReturnIfError( err );

	if ( srcFileInfo.vcb != destFileInfo.vcb )
		return( diffVolErr );

	//-- since the vcb's are the same, let's use a local variable
	vcb = srcFileInfo.vcb;

	err = VolumeWritable( vcb );
	ReturnIfError( err );

	err = CheckVolumeOffLine( vcb );
	ReturnIfError( err );

	isHFSPlus = ( vcb->vcbSigWord == kHFSPlusSigWord );
	
	//-- if it's not an HFS Plus volume, make sure it's an HFS volume
	if ( (! isHFSPlus) && (vcb->vcbSigWord != kHFSSigWord) )
		return( wrgVolTypErr );

	//-- Set up the names into pascal strings
	pDestName[0] = destFileInfo.nameLength;
	BlockMoveData( destFileInfo.nameBuffer, &(pDestName[1]), pDestName[0] );
	
	pSrcName[0] = srcFileInfo.nameLength;
	BlockMoveData( srcFileInfo.nameBuffer, &(pSrcName[1]), pSrcName[0] );
	
	//-- Now prepare the pascal names for input
	err = PrepareInputName( pSrcName, isHFSPlus, GetTextEncodingFromHint(srcFileInfo.hint), &srcName );
	if ( err != noErr ) goto ParseError;

	err = PrepareInputName( pDestName, isHFSPlus, GetTextEncodingFromHint(destFileInfo.hint), &destName );
	if ( err != noErr ) goto ParseError;

	//--	locate the source file
	err = LocateCatalogNode( vcb, srcFileInfo.id, &srcName, srcFileInfo.hint, &srcKey, &srcData, &srcHint );
	if ( err != noErr ) goto ParseError;

	//--	locate the dest file
	err = LocateCatalogNode( vcb, destFileInfo.id, &destName, destFileInfo.hint, &destKey, &destData, &destHint );
	if ( err != noErr ) goto ParseError;

	//--	Are we trying to exchange the file with itself?
	if ( isHFSPlus )
	{
		srcFileID = srcData.hfsPlusFile.fileID;
		destFileID = destData.hfsPlusFile.fileID;
	}
	else
	{
		srcFileID = srcData.hfsFile.fileID;
		destFileID = destData.hfsFile.fileID;
	}
	
	if ( srcFileID == destFileID )
		return( sameFileErr );

		
	//	Flush any open paths with the files in question! (release cache buffers)							<KSCT>
	
	//	Note: The cache takes fcb refnum as an argument for flushing. The only way to get
	//	this value is to walk the fcb array and find matches of the file number. If a match
	//	is found, that fcb refnum can be used for flushing. BUT, the fcb walk continues, since
	//	there can be more than one open path per file.
	
	//	FINE TUNING: Flag if either file is open. THEN after FidExchangeFiles, if none, skip fcb walking.

	Get1stFileControlBlock( &fcbIndex, &fcbs );
	do
	{
		fcb = GetFileControlBlock( fcbIndex );
		if ( fcb->fcbVPtr == (ExtendedVCB *) vcb )						//	make sure we're on the right volume
		{
			if ( (fcb->fcbFlNm == srcFileID) || (fcb->fcbFlNm == destFileID) )
			{
				//--	Flush and continue
				LMSetFlushOnly( 0xFF );
				err = CloseFile( vcb, fcbIndex, fcbs );
				ReturnIfError( err );
				
				LMSetCacheFlag( 0xFF );

				(void) C_FlushCache( vcb, fcTrashBit, fcbIndex );
			}
		}
	} while ( GetNextFileControlBlock( &fcbIndex, fcbs ) == false );	//	while there's still some left

	//	valid buffer in the free queue has a file number tag to it in the header, however,
	//	after FIDExchangeFiles the file number is no longer valid. So we have to call TrashFBlocks
	//	to invalidate all the buffer associated with the files.					<16Apr91 #12 ksct>

	TrashCatalogIterator( vcb, srcFileInfo.id );					//	invalidate any iterators for this parentID
	InvalidateCatalogNodeCache( vcb, srcFileInfo.id );				//	invalidate node cache
	if ( srcFileInfo.id != destFileInfo.id )
	{
		TrashCatalogIterator( vcb, destFileInfo.id );				//	invalidate any iterators for this parentID
		InvalidateCatalogNodeCache( vcb, destFileInfo.id );			//	invalidate node cache
	}
	TrashFileBlocks( vcb, destFileID );
	TrashFileBlocks( vcb, srcFileID );
	
	
	//--	Do the stuff!
	err = ExchangeFileIDs( vcb, &srcName, &destName, srcFileInfo.id, destFileInfo.id, srcHint, destHint );
	if ( DEBUG_BUILD && err != noErr)
	{
		DebugStr("\pError from ExchangeFileIDs");
	}

	//--	Now get the names ready for output
	if ( isHFSPlus )
	{
		err = ConvertUnicodeToHFSName( &srcName.ustr, srcData.hfsPlusFile.textEncoding, srcData.hfsPlusFile.fileID, pSrcName );
		if ( err != noErr )
			goto ParseError;

		err = ConvertUnicodeToHFSName( &destName.ustr, destData.hfsPlusFile.textEncoding, srcData.hfsPlusFile.fileID, pDestName );
	}
	else
	{
		BlockMoveData( srcName.pstr, pSrcName, StrLength(srcName.pstr) + 1 );
		BlockMoveData( destName.pstr, pDestName, StrLength(srcName.pstr) + 1 );
	}

	if ( err != noErr )
		goto ParseError;
		
	//--	Exchange fcb info (file number, file name, and file parID only!)
	Get1stFileControlBlock( &fcbIndex, &fcbs );
	do
	{
		fcb = GetFileControlBlock( fcbIndex );
		if ( fcb->fcbVPtr == vcb )
		{
			if ( fcb->fcbFlNm == srcFileID )
			{
				fcb->fcbCatPos	= 0;
				fcb->fcbFlNm	= destFileID;
				fcb->fcbDirID	= destFileInfo.id;
				
				BlockMoveData( pDestName, fcb->fcbCName, pDestName[0]+1 );
			}
			else if ( fcb->fcbFlNm == destFileID )
			{
				fcb->fcbCatPos	= 0;
				fcb->fcbFlNm	= srcFileID;
				fcb->fcbDirID	= srcFileInfo.id;
				
				BlockMoveData( pSrcName, fcb->fcbCName, pSrcName[0]+1 );
			}
		}
	} while ( GetNextFileControlBlock( &fcbIndex, fcbs ) == false );	//	while there's still some left
		
	return( err );

ParseError:
	if		( err == cmNotFound )	return( fnfErr );
	else if	( err == cmFThdDirErr )	return( notAFileErr );
	else							return( err );
}
#endif	/* TARGET_OS_MAC */
#endif


OSErr ExchangeFileIDs( ExtendedVCB *vcb, ConstUTF8Param srcName, ConstUTF8Param destName, HFSCatalogNodeID srcID, HFSCatalogNodeID destID, UInt32 srcHint, UInt32 destHint )
{
	CatalogKey		srcKey;		// 518 bytes
	CatalogRecord	srcData;	// 520 bytes
	CatalogKey		destKey;	// 518 bytes
	CatalogRecord	destData;	// 520 bytes
	CatalogRecord	swapData;	// 520 bytes
	SInt16			numSrcExtentBlocks;
	SInt16			numDestExtentBlocks;
	UInt32			textEncoding;
	OSErr			err;
	Boolean			isHFSPlus = ( vcb->vcbSigWord == kHFSPlusSigWord );

    TrashCatalogIterator(vcb, srcID);	//	invalidate any iterators for this parentID
    TrashCatalogIterator(vcb, destID);	//	invalidate any iterators for this parentID

	err = BuildCatalogKeyUTF8(srcID, srcName, isHFSPlus, &srcKey, &textEncoding);
	ReturnIfError(err);

	err = BuildCatalogKeyUTF8(destID, destName, isHFSPlus, &destKey, &textEncoding);
	ReturnIfError(err);

	if ( isHFSPlus )
	{
		//--	Step 1: Check the catalog nodes for extents
		
		//--	locate the source file, test for extents in extent file, and copy the cat record for later
		err = LocateCatalogNodeByKey( vcb, srcHint, &srcKey, &srcData, &srcHint );
		ReturnIfError( err );
	
		if ( srcData.recordType != kHFSPlusFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"
			
		//--	Check if there are any extents in the source file
		//€€	I am only checling the extents in the low 32 bits, routine will fail if files extents after 2 gig are in overflow
		numSrcExtentBlocks = CheckExtents( srcData.hfsPlusFile.dataFork.extents, srcData.hfsPlusFile.dataFork.totalBlocks, isHFSPlus );
		if ( numSrcExtentBlocks == 0 )					//	then check the resource fork extents
			numSrcExtentBlocks = CheckExtents( srcData.hfsPlusFile.resourceFork.extents, srcData.hfsPlusFile.resourceFork.totalBlocks, isHFSPlus );

		//--	Check if there are any extents in the destination file
		err = LocateCatalogNodeByKey( vcb, destHint, &destKey, &destData, &destHint );
		ReturnIfError( err );
	
		if ( destData.recordType != kHFSPlusFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"

		numDestExtentBlocks = CheckExtents( destData.hfsPlusFile.dataFork.extents, destData.hfsPlusFile.dataFork.totalBlocks, isHFSPlus );
		if ( numDestExtentBlocks == 0 )					//	then check the resource fork extents
			numDestExtentBlocks = CheckExtents( destData.hfsPlusFile.resourceFork.extents, destData.hfsPlusFile.resourceFork.totalBlocks, isHFSPlus );

		//--	Step 2: Exchange the Extent key in the extent file
		
		//--	Exchange the extents key in the extent file
		err = DeleteExtents( vcb, kHFSBogusExtentFileID, isHFSPlus );
		ReturnIfError( err );
		
		if ( numSrcExtentBlocks && numDestExtentBlocks )	//	if both files have extents
		{
			//--	Change the source extents file ids to our known bogus value
			err = MoveExtents( vcb, srcData.hfsPlusFile.fileID, kHFSBogusExtentFileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );
				else
					goto ExUndo1a;
			}
			
			//--	Change the destination extents file id's to the source id's
			err = MoveExtents( vcb, destData.hfsPlusFile.fileID, srcData.hfsPlusFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

ExUndo2aPlus:	err = DeleteExtents( vcb, srcData.hfsPlusFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

                err = MoveExtents( vcb, kHFSBogusExtentFileID, srcData.hfsPlusFile.fileID, isHFSPlus );	//	Move the extents back
				ReturnIfError( err );					//	we are doomed. Just QUIT!
					
				goto ExUndo1a;
			}
			
			//--	Change the bogus extents file id's to the dest id's
            err = MoveExtents( vcb, kHFSBogusExtentFileID, destData.hfsPlusFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, destData.hfsPlusFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				err = MoveExtents( vcb, srcData.hfsPlusFile.fileID, destData.hfsPlusFile.fileID, isHFSPlus );	//	Move the extents back
				ReturnIfError( err );					//	we are doomed. Just QUIT!
					
				goto ExUndo2aPlus;
			}
			
		}
		else if ( numSrcExtentBlocks )	//	just the source file has extents
		{
			err = MoveExtents( vcb, srcData.hfsPlusFile.fileID, destData.hfsPlusFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, srcData.hfsPlusFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				goto FlushAndReturn;
			}
		}
		else if ( numDestExtentBlocks )	//	just the destination file has extents
		{
			err = MoveExtents( vcb, destData.hfsPlusFile.fileID, srcData.hfsPlusFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, destData.hfsPlusFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				goto FlushAndReturn;
			}
		}

		//--	Step 3: Change the data in the catalog nodes
		
		//--	find the source cnode and put dest info in it
		err = LocateCatalogNodeByKey( vcb, srcHint, &srcKey, &srcData, &srcHint );
		if ( err != noErr )
			return( cmBadNews );
		
		BlockMoveData( &srcData, &swapData, sizeof(CatalogRecord) );
		CopyBigCatalogNodeInfo( &destData, &srcData );
		
		err = ReplaceBTreeRecord( vcb->catalogRefNum, &srcKey, srcHint, &srcData, sizeof(HFSPlusCatalogFile), &srcHint );
		ReturnIfError( err );

		//	find the destination cnode and put source info in it		
		err = LocateCatalogNodeByKey( vcb, destHint, &destKey, &destData, &destHint );
		if ( err != noErr )
			return( cmBadNews );
			
		CopyBigCatalogNodeInfo( &swapData, &destData );
		err = ReplaceBTreeRecord( vcb->catalogRefNum, &destKey, destHint, &destData, sizeof(HFSPlusCatalogFile), &destHint );
		ReturnIfError( err );
	}
	else		//	HFS	//
	{
		//--	Step 1: Check the catalog nodes for extents
		
		//--	locate the source file, test for extents in extent file, and copy the cat record for later
		err = LocateCatalogNodeByKey( vcb, srcHint, &srcKey, &srcData, &srcHint );
		ReturnIfError( err );
	
		if ( srcData.recordType != kHFSFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"
			
		//--	Check if there are any extents in the source file
		numSrcExtentBlocks = CheckExtents( srcData.hfsFile.dataExtents, srcData.hfsFile.dataPhysicalSize / vcb->blockSize, isHFSPlus );
		if ( numSrcExtentBlocks == 0 )					//	then check the resource fork extents
			numSrcExtentBlocks = CheckExtents( srcData.hfsFile.rsrcExtents, srcData.hfsFile.rsrcPhysicalSize / vcb->blockSize, isHFSPlus );
		
		
		//€€	Do we save the found source node for later use?
		
				
		//--	Check if there are any extents in the destination file
		err = LocateCatalogNodeByKey( vcb, destHint, &destKey, &destData, &destHint );
		ReturnIfError( err );
	
		if ( destData.recordType != kHFSFileRecord )
			return( cmFThdDirErr );					//	Error "cmFThdDirErr = it is a directory"

		numDestExtentBlocks = CheckExtents( destData.hfsFile.dataExtents, destData.hfsFile.dataPhysicalSize / vcb->blockSize, isHFSPlus );
		if ( numDestExtentBlocks == 0 )					//	then check the resource fork extents
			numDestExtentBlocks = CheckExtents( destData.hfsFile.rsrcExtents, destData.hfsFile.rsrcPhysicalSize / vcb->blockSize, isHFSPlus );
			
		//€€	Do we save the found destination node for later use?


		//--	Step 2: Exchange the Extent key in the extent file
		
		//--	Exchange the extents key in the extent file
        err = DeleteExtents( vcb, kHFSBogusExtentFileID, isHFSPlus );
		ReturnIfError( err );
		
		if ( numSrcExtentBlocks && numDestExtentBlocks )	//	if both files have extents
		{
			//--	Change the source extents file ids to our known bogus value
        err = MoveExtents( vcb, srcData.hfsFile.fileID, kHFSBogusExtentFileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

ExUndo1a:		err = DeleteExtents( vcb, kHFSBogusExtentFileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				err = FlushCatalog( vcb );   			//	flush the catalog
				err = FlushExtentFile( vcb );			//	flush the extent file (unneeded for common case, but it's cheap)			
				return( dskFulErr );
			}
			
			//--	Change the destination extents file id's to the source id's
			err = MoveExtents( vcb, destData.hfsFile.fileID, srcData.hfsFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

ExUndo2a:		err = DeleteExtents( vcb, srcData.hfsFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

                err = MoveExtents( vcb, kHFSBogusExtentFileID, srcData.hfsFile.fileID, isHFSPlus );	//	Move the extents back
				ReturnIfError( err );					//	we are doomed. Just QUIT!
					
				goto ExUndo1a;
			}
			
			//--	Change the bogus extents file id's to the dest id's
            err = MoveExtents( vcb, kHFSBogusExtentFileID, destData.hfsFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, destData.hfsFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				err = MoveExtents( vcb, srcData.hfsFile.fileID, destData.hfsFile.fileID, isHFSPlus );	//	Move the extents back
				ReturnIfError( err );					//	we are doomed. Just QUIT!
					
				goto ExUndo2a;
			}
			
		}
		else if ( numSrcExtentBlocks )	//	just the source file has extents
		{
			err = MoveExtents( vcb, srcData.hfsFile.fileID, destData.hfsFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, srcData.hfsFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				goto FlushAndReturn;
			}
		}
		else if ( numDestExtentBlocks )	//	just the destination file has extents
		{
			err = MoveExtents( vcb, destData.hfsFile.fileID, srcData.hfsFile.fileID, isHFSPlus );
			if ( err != noErr )
			{
				if ( err != dskFulErr )
					return( err );

				err = DeleteExtents( vcb, destData.hfsFile.fileID, isHFSPlus );
				ReturnIfError( err );					//	we are doomed. Just QUIT!

				goto FlushAndReturn;
			}
		}

		//--	Step 3: Change the data in the catalog nodes
		
		//--	find the source cnode and put dest info in it
		err = LocateCatalogNodeByKey( vcb, srcHint, &srcKey, &srcData, &srcHint );
		if ( err != noErr )
			return( cmBadNews );
		
		BlockMoveData( &srcData, &swapData, sizeof(CatalogRecord) );
		//€€	Asm source copies from the saved dest catalog node
		CopyCatalogNodeInfo( &destData, &srcData );
		
		err = ReplaceBTreeRecord( vcb->catalogRefNum, &srcKey, srcHint, &srcData, sizeof(HFSCatalogFile), &srcHint );
		ReturnIfError( err );

		
		//	find the destination cnode and put source info in it		
		err = LocateCatalogNodeByKey( vcb, destHint, &destKey, &destData, &destHint );
		if ( err != noErr )
			return( cmBadNews );
			
		CopyCatalogNodeInfo( &swapData, &destData );
		err = ReplaceBTreeRecord( vcb->catalogRefNum, &destKey, destHint, &destData, sizeof(HFSCatalogFile), &destHint );
		ReturnIfError( err );
	}
	
	err = noErr;

	//--	Step 4: Error Handling section


FlushAndReturn:
	err = FlushCatalog( vcb );   			//	flush the catalog
	err = FlushExtentFile( vcb );			//	flush the extent file (unneeded for common case, but it's cheap)			
	return( err );
}


void	CopyCatalogNodeInfo( CatalogRecord *src, CatalogRecord *dest )
{
//	dest->hfsFile.filStBlk = src->hfsFile.filStBlk;
	dest->hfsFile.dataLogicalSize	= src->hfsFile.dataLogicalSize;
	dest->hfsFile.dataPhysicalSize = src->hfsFile.dataPhysicalSize;
//	dest->hfsFile.filRStBlk = src->hfsFile.filRStBlk;
	dest->hfsFile.rsrcLogicalSize	= src->hfsFile.rsrcLogicalSize;
	dest->hfsFile.rsrcPhysicalSize = src->hfsFile.rsrcPhysicalSize;
	dest->hfsFile.modifyDate = src->hfsFile.modifyDate;
	BlockMoveData( src->hfsFile.dataExtents, dest->hfsFile.dataExtents, sizeof(HFSExtentRecord) );
	BlockMoveData( src->hfsFile.rsrcExtents, dest->hfsFile.rsrcExtents, sizeof(HFSExtentRecord) );
}

void	CopyBigCatalogNodeInfo( CatalogRecord *src, CatalogRecord *dest )
{
	BlockMoveData( &src->hfsPlusFile.dataFork, &dest->hfsPlusFile.dataFork, sizeof(HFSPlusForkData) );
	BlockMoveData( &src->hfsPlusFile.resourceFork, &dest->hfsPlusFile.resourceFork, sizeof(HFSPlusForkData) );
	dest->hfsPlusFile.contentModDate = src->hfsPlusFile.contentModDate;
}


OSErr	MoveExtents( ExtendedVCB *vcb, UInt32 srcFileID, UInt32 destFileID, Boolean isHFSPlus )
{
	FCB *				fcb;
	ExtentsRecBuffer	extentsBuffer[kNumExtentsToCache];
	ExtentKey *			extentKeyPtr;
	ExtentRecord		extentData;
	BTreeIterator		btIterator;
	FSBufferDescriptor	btRecord;
	UInt16				btKeySize;
	UInt16				btRecordSize;
	SInt16				i, j;
	OSErr				err;
	

	fcb = GetFileControlBlock(vcb->extentsRefNum);
	
	(void) BTInvalidateHint(&btIterator);
	extentKeyPtr = (ExtentKey*) &btIterator.key;
	btRecord.bufferAddress = &extentData;
	btRecord.itemCount = 1;

	//--	Collect the extent records

	//
	//	A search on the following key will cause the BTree to be positioned immediately
	//	before the first extent record for file #srcFileID, but not actually positioned
	//	on any record.  This is because there cannot be an extent record with FABN = 0
	//	(the first extent of the fork, which would be in the catalog entry, not an extent
	//	record).
	//
	//	Using BTIterateRecord with kBTreeNextRecord will then get that first extent record.
	//
	if (isHFSPlus) {
		btRecord.itemSize = sizeof(HFSPlusExtentRecord);
		btKeySize = sizeof(HFSPlusExtentKey);

		extentKeyPtr->hfsPlus.keyLength	 = kHFSPlusExtentKeyMaximumLength;
		extentKeyPtr->hfsPlus.forkType	 = 0;
		extentKeyPtr->hfsPlus.pad		 = 0;
		extentKeyPtr->hfsPlus.fileID	 = srcFileID;
		extentKeyPtr->hfsPlus.startBlock = 0;
	}
	else {
		btRecord.itemSize = sizeof(HFSExtentRecord);
		btKeySize = sizeof(HFSExtentKey);

		extentKeyPtr->hfs.keyLength	 = kHFSExtentKeyMaximumLength;
		extentKeyPtr->hfs.forkType	 = 0;
		extentKeyPtr->hfs.fileID	 = srcFileID;
		extentKeyPtr->hfs.startBlock = 0;
	}
	
	//
	//	We do an initial BTSearchRecord to position the BTree's iterator just before any extent
	//	records for srcFileID.  We then do a few BTIterateRecord and BTInsertRecord of those found
	//	records, but with destFileID as the file number in the key.  Keep doing this sequence of
	//	BTIterateRecord and BTInsertRecord until we find an extent for another file, or there are
	//	no more extent records in the tree.
	//
	//	Basically, we're copying records kNumExtentsToCache at a time.  The copies have their file ID
	//	set to destFileID.
	//
	//	This depends on BTInsertRecord not effecting the iterator used by BTIterateRecord.  If it
	//	_did_ effect the iterator, then we would need to do a BTSearchRecord before each series
	//	of BTIterateRecord.  We'd need to set up the key for BTSearchRecord to find the last record
	//	we found, so that BTIterateRecord would get the next one (the first we haven't processed).
	//

	err = BTSearchRecord(fcb, &btIterator, kInvalidMRUCacheKey, &btRecord, &btRecordSize, &btIterator);
	
	//	We expect a btNotFound here, since there shouldn't be an extent record with FABN = 0.
	if (err != btNotFound)
	{
		if ( DEBUG_BUILD )
			DebugStr("\pUnexpected error from SearchBTreeRecord");
		
		if (err == noErr)			//	If we found such a bogus extent record, then the tree is really messed up
			err = cmBadNews;		//	so return an error that conveys the disk is hosed.
		
		return err;
	}

	do
	{
		btRecord.bufferAddress = &extentData;
		btRecord.itemCount = 1;

		for ( i=0 ; i<kNumExtentsToCache ; i++ )
		{
			HFSCatalogNodeID	foundFileID;

			err = BTIterateRecord(fcb, kBTreeNextRecord, &btIterator, &btRecord, &btRecordSize);
			if ( err == btNotFound )		//	Did we run out of extent records in the extents tree?
				break;						//	if xkrFNum(A0) is cleared on this error, then this test is bogus!
			else if ( err != noErr )
				return( err );				//	must be ioError
			
			foundFileID = isHFSPlus ? extentKeyPtr->hfsPlus.fileID : extentKeyPtr->hfs.fileID;
			if ( foundFileID == srcFileID )
			{
				CopyExtentInfo(extentKeyPtr, &extentData, extentsBuffer, i);
			}
			else
			{
				break;
			}
		}
		
		//--	edit each extent key, and reinsert each extent record in the extent file
		if (isHFSPlus)
			btRecordSize = sizeof(HFSPlusExtentRecord);
		else
			btRecordSize = sizeof(HFSExtentRecord);

		for ( j=0 ; j<i ; j++ )
		{
			BTreeIterator tmpIterator;

			if (isHFSPlus)
				extentsBuffer[j].extentKey.hfsPlus.fileID = destFileID;	//	change only the id in the key to dest ID
			else
				extentsBuffer[j].extentKey.hfs.fileID = destFileID;	//	change only the id in the key to dest ID

			// get iterator and buffer descriptor ready...
			(void) BTInvalidateHint(&tmpIterator);
			BlockMoveData(&(extentsBuffer[j].extentKey), &tmpIterator.key, btKeySize);
			btRecord.bufferAddress = &(extentsBuffer[j].extentData);

			err = BTInsertRecord(fcb, &tmpIterator, &btRecord, btRecordSize);
			if ( err != noErr )
			{									//	parse the error
				if ( err == btExists )
				{
					if ( DEBUG_BUILD )
						DebugStr("\pCan't insert record -- already exists");
					return( cmBadNews );
				}
				else
					return( err );
			}
		}
		
		//--	okay, done with this buffered batch, go get the next set of extent records
		//	If our buffer is not full, we must be done, or recieved an error
		
		if ( i != kNumExtentsToCache )			//	if the buffer is not full, we must be done
		{
			err = DeleteExtents( vcb, srcFileID, isHFSPlus );	//	Now delete all the extent entries with the sourceID
			if ( DEBUG_BUILD && err != noErr )
				DebugStr("\pError from DeleteExtents");
			break;									//	we're done!
		}
	} while ( true );
	
	return( err );
}


void	CopyExtentInfo( ExtentKey *key, ExtentRecord *data, ExtentsRecBuffer *buffer, UInt16 bufferCount )
{
	BlockMoveData( key, &(buffer[bufferCount].extentKey), sizeof( ExtentKey ) );
	BlockMoveData( data, &(buffer[bufferCount].extentData), sizeof( ExtentRecord ) );
}


//--	Delete all extents in extent file that have the ID given.
OSErr	DeleteExtents( ExtendedVCB *vcb, UInt32 fileID, Boolean isHFSPlus )
{
	FCB *				fcb;
	ExtentKey *			extentKeyPtr;
	ExtentRecord		extentData;
	BTreeIterator		btIterator;
	FSBufferDescriptor	btRecord;
	UInt16				btRecordSize;
	OSErr				err;

	fcb = GetFileControlBlock(vcb->extentsRefNum);

	(void) BTInvalidateHint(&btIterator);
	extentKeyPtr = (ExtentKey*) &btIterator.key;
	btRecord.bufferAddress = &extentData;
	btRecord.itemCount = 1;

	//	The algorithm is to position the BTree just before any extent records for fileID.
	//	Then just keep getting successive records.  If the record is still for fileID,
	//	then delete it.
	
	if (isHFSPlus) {
		btRecord.itemSize = sizeof(HFSPlusExtentRecord);

		extentKeyPtr->hfsPlus.keyLength	 = kHFSPlusExtentKeyMaximumLength;
		extentKeyPtr->hfsPlus.forkType	 = 0;
		extentKeyPtr->hfsPlus.pad		 = 0;
		extentKeyPtr->hfsPlus.fileID	 = fileID;
		extentKeyPtr->hfsPlus.startBlock = 0;
	}
	else {
		btRecord.itemSize = sizeof(HFSExtentRecord);

		extentKeyPtr->hfs.keyLength	 = kHFSExtentKeyMaximumLength;
		extentKeyPtr->hfs.forkType	 = 0;
		extentKeyPtr->hfs.fileID	 = fileID;
		extentKeyPtr->hfs.startBlock = 0;
	}

	err = BTSearchRecord(fcb, &btIterator, kInvalidMRUCacheKey, &btRecord, &btRecordSize, &btIterator);
	if ( err != btNotFound )
	{
		if (err == noErr) {		//	Did we find a bogus extent record?
			err = cmBadNews;	//	Yes, so indicate things are messed up.
		}
		
		return err;				//	Got some unexpected error, so return it
	}

	do
	{
		BTreeIterator 		tmpIterator;
		HFSCatalogNodeID	foundFileID;

		err = BTIterateRecord(fcb, kBTreeNextRecord, &btIterator, &btRecord, &btRecordSize);
		if ( err != noErr )
		{
			if (err == btNotFound)	//	If we hit the end of the BTree
				err = noErr;		//		then it's OK
				
			break;					//	We're done now.
		}
		
		foundFileID = isHFSPlus ? extentKeyPtr->hfsPlus.fileID : extentKeyPtr->hfs.fileID;
		if ( foundFileID != fileID )
			break;					//	numbers don't match, we must be done

		tmpIterator = btIterator;
		err = BTDeleteRecord( fcb, &tmpIterator );
		if (err != noErr)
			break;
	}	while ( true );
	
	return( err );
}


//	Check if there are extents represented in the extents overflow file.
UInt32	CheckExtents( void *extents, UInt32 totalBlocks, Boolean isHFSPlus )
{
	UInt32		extentAllocationBlocks;
	UInt16		i;


	if ( totalBlocks == 0 )
		return( 0 );
		
	extentAllocationBlocks = 0;
	
	if ( isHFSPlus )
	{
		for ( i = 0 ; i < kHFSPlusExtentDensity ; i++ )
		{
			extentAllocationBlocks += ((HFSPlusExtentDescriptor *)extents)[i].blockCount;
			if ( extentAllocationBlocks >= totalBlocks )		//	greater than or equal (extents can add past eof if 'Close" crashes w/o truncating new clump)
				return( 0 );
		}
	}
	else
	{
		for ( i = 0 ; i < kHFSExtentDensity ; i++ )
		{
			extentAllocationBlocks += ((HFSExtentDescriptor *)extents)[i].blockCount;
			if ( extentAllocationBlocks >= totalBlocks )		//	greater than or equal (extents can add past eof if 'Close" crashes w/o truncating new clump)
				return( 0 );
		}
	}
	
	return( extentAllocationBlocks );
}
