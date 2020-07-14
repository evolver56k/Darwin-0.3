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
	File:		BTreeWrapper.c

	Contains:	Interface glue for new B-tree manager.

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

	  <CS19>	11/24/97	djb		Remove some debug code (Panic calls).
	  <CS18>	  9/4/97	msd		Fix ValidHFSRecord to determine the type of B-tree by FileID,
									not record size. Add better checking for attribute b-tree keys.
	  <CS17>	 8/22/97	djb		Get blockReadFromDisk flag from GetCacheBlock call.
	  <CS16>	 8/14/97	djb		Remove reserved field checks in ValidHFSRecord (radar #1649593).
									Only call if ValidHFSRecord DEBUG_BUILD is true.
	  <CS15>	 8/11/97	djb		Bug 1670441. In SetEndOfForkProc, don't DebugStr if the disk is
									full.
	  <CS14>	 7/25/97	DSH		Pass heuristicHint to BTSearchRecord from SearchBTreeRecord.
	  <CS13>	 7/24/97	djb		CallBackProcs now take a file refNum instead of an FCB.
									GetBlockProc now reports if block came from disk.
	  <CS12>	 7/22/97	djb		Move all trace points to BTree.c file.
	  <CS11>	 7/21/97	djb		LogEndTime now takes an error code.
	  <CS10>	 7/16/97	DSH		FilesInternal.x -> FileMgrInternal.x to avoid name collision
	   <CS9>	 7/15/97	msd		Bug #1664103.  OpenBTree is not propagating errors from
									BTOpenPath.
	   <CS8>	  7/9/97	djb		Remove maxCNID check from ValidHFSRecord (radar #1649593).
	   <CS7>	 6/13/97	djb		In ValidHFSRecord HFSPlus threads names can be > 31 chars.
	   <CS6>	  6/2/97	DSH		Also flush AlternateVolumeHeader whenever Attributes or Startup
									files change size.
	   <CS5>	 5/28/97	msd		In ValidHFSRecord, check for attribute keys.
	   <CS4>	 5/19/97	djb		Move summary traces from GetBTreeRecord to BTIterateRecord.
	   <CS3>	  5/9/97	djb		Get in sync with new FilesInternal.i.
	   <CS2>	  5/7/97	djb		Add summary traces to B-tree SPI.
	   <CS1>	 4/24/97	djb		first checked in
	 <HFS18>	 4/16/97	djb		Always use new B-tree code.
	 <HFS17>	  4/4/97	djb		Remove clumpSize test from ValidHFSRecord.
	 <HFS16>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS15>	 3/17/97	DSH		Casting for SC, BlockProcs are now not static.
	 <HFS14>	  3/3/97	djb		Call trash block after closing btree!
	 <HFS13>	 2/19/97	djb		Add support for accessing bigger B-tree nodes.
	 <HFS12>	  2/6/97	msd		In CheckBTreeKey, remove test and DebugStr for parent ID being
									too big.
	 <HFS11>	 1/23/97	DSH		SetEndOfForkProc now calls through to update the Alternate MDB
									or VolumeHeader.
	 <HFS10>	 1/16/97	djb		Switched to dynamic lengths for BufferDescriptor length field in
									SearchBTreeRecord and GetBTreeRecord. Round up to clump size in
									SetEndOfForkProc.
	  <HFS9>	 1/15/97	djb		Don't return errors for bad file ids in key.
	  <HFS8>	 1/13/97	djb		Adding support for getting current record. ValidHFSRecord now
									supports variable sized thread records.
	  <HFS7>	  1/9/97	djb		Call CheckBTreeKey before using key length in a BlockMoveData
									call.
	  <HFS6>	  1/6/97	djb		Implement SetEndOfForkProc.
	  <HFS5>	  1/6/97	djb		Added HFS Plus support to CheckBTreeKey and ValidHFSRecord.
	  <HFS4>	  1/3/97	djb		Added support for large keys. Integrated latest HFSVolumesPriv.h
									changes.
	  <HFS3>	12/23/96	djb		Fixed problem in SearchBTreeRecord (dataSize is an output so it
									was undefined). Added some debugging code.
	  <HFS2>	12/20/96	msd		Fix OpenBTree to use the real data type for the key compare proc
									pointer (not void *). Fixed problem in SearchBTreeRecord that
									assigns a pointer to a buffer size field (forgot to dereference
									the pointer).
	  <HFS1>	12/19/96	djb		first checked in

*/

#include <FileMgrInternal.h>

#include "BTreesInternal.h"
#include "BTreesPrivate.h"




// B-tree callbacks...
OSStatus	GetBlockProc ( SInt16 fileRefNum, UInt32 blockNum, GetBlockOptions options, BlockDescriptor *block );
OSStatus	ReleaseBlockProc ( SInt16 fileRefNum, BlockDescPtr blockPtr, ReleaseBlockOptions options );
OSStatus	SetBlockSizeProc ( SInt16 fileRefNum, ByteCount blockSize, ItemCount minBlockCount );

// local routines
static void		InvalidateBTreeIterator ( SInt16 refNum );
static OSErr	CheckBTreeKey(const BTreeKey *key, const BTreeControlBlock *btcb);
static Boolean	ValidHFSRecord(const void *record, const BTreeControlBlock *btcb, UInt16 recordSize);



OSErr OpenBTree(SInt16 refNum, KeyCompareProcPtr keyCompareProc)
{
	FCB					*fcb;
	OSStatus			 result;
	

	fcb = GetFileControlBlock(refNum);

	result = BTOpenPath (fcb, keyCompareProc, GetBlockProc, ReleaseBlockProc, SetEndOfForkProc, SetBlockSizeProc);
	ExitOnError(result);

	InvalidateBTreeIterator(refNum);
	
ErrorExit:

	return result;
}


OSErr CloseBTree(SInt16 refNum)
{
	OSErr	result;


	result = BTClosePath ( GetFileControlBlock(refNum) );
	ExitOnError(result);
	
	TrashCacheBlocks (refNum);

ErrorExit:


	return result;
}


OSErr FlushBTree(SInt16 refNum)
{
	OSErr	result;


	result = BTFlushPath( GetFileControlBlock(refNum) );

	return result;
}


OSErr SearchBTreeRecord(SInt16 refNum, const void* key, UInt32 hint, void* foundKey, void* data, UInt16 *dataSize, UInt32 *newHint)
{
	FSBufferDescriptor	 btRecord;
	BTreeIterator		 searchIterator;
	BTreeIterator		*resultIterator;
	FCB					*fcb;
	BTreeControlBlock	*btcb;
	OSStatus			 result;


	fcb = GetFileControlBlock(refNum);
	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;

	btRecord.bufferAddress = data;
	btRecord.itemCount = 1;
	if ( btcb->maxKeyLength == kSmallExtentKeyMaximumLength )
		btRecord.itemSize = sizeof(SmallExtentRecord);
	else if ( btcb->maxKeyLength == kLargeExtentKeyMaximumLength )
		btRecord.itemSize = sizeof(LargeExtentRecord);
	else
		btRecord.itemSize = sizeof(CatalogRecord);

	searchIterator.hint.writeCount = 0;	// clear these out for debugging...
	searchIterator.hint.reserved1 = 0;
	searchIterator.hint.reserved2 = 0;

	searchIterator.hint.nodeNum = hint;
	searchIterator.hint.index = 0;

	result = CheckBTreeKey((BTreeKey *) key, btcb);
	ExitOnError(result);

	BlockMoveData(key, &searchIterator.key, CalcKeySize(btcb, (BTreeKey *) key));		//€€ should we range check against maxkeylen?
	
	resultIterator = &btcb->lastIterator;

	//	We only optimize for catalog records
	if( btRecord.itemSize == sizeof(CatalogRecord) )
	{
		UInt32	heuristicHint;
		UInt32	*cachedHint;
		Ptr		hintCachePtr = fcb->fcbVPtr->hintCachePtr;

		//	We pass a 2nd hint/guess into BTSearchRecord.  The heuristicHint is a mapping of
		//	dirID and nodeNumber, in hopes that the current search will be in the same node
		//	as the last search with the same parentID.
		result = GetMRUCacheBlock( ((SmallCatalogKey *)key)->parentID, hintCachePtr, (Ptr *)&cachedHint );
		heuristicHint = (result == noErr) ? *cachedHint : kInvalidMRUCacheKey;

		result = BTSearchRecord( fcb, &searchIterator, heuristicHint, &btRecord, dataSize, resultIterator );

		InsertMRUCacheBlock( hintCachePtr, ((SmallCatalogKey *)key)->parentID, (Ptr) &(resultIterator->hint.nodeNum) );
	}
	else
	{
		result = BTSearchRecord( fcb, &searchIterator, kInvalidMRUCacheKey, &btRecord, dataSize, resultIterator );
	}

	if (result == noErr)
	{
		*newHint = resultIterator->hint.nodeNum;

		result = CheckBTreeKey(&resultIterator->key, btcb);
		ExitOnError(result);

		BlockMoveData(&resultIterator->key, foundKey, CalcKeySize(btcb, &resultIterator->key));	//€€ warning, this could overflow user's buffer!!!

		if ( DEBUG_BUILD && !ValidHFSRecord(data, btcb, *dataSize) )
			DebugStr("\pSearchBTreeRecord: bad record?");
	}

ErrorExit:

	return result;
}



//	Note
//	The new B-tree manager differs from the original b-tree in how it does iteration. We need
//	to account for these differences here.  We save an iterator in the BTree control block so
//	that we have a context in which to perfrom the iteration. Also note that the old B-tree
//	allowed you to specify any number relative to the last operation (including 0) whereas the
//	new B-tree only allows next/previous.

OSErr GetBTreeRecord(SInt16 refNum, SInt16 selectionIndex, void* key, void* data, UInt16 *dataSize, UInt32 *newHint)
{
	FSBufferDescriptor	btRecord;
	BTreeIterator		*iterator;
	FCB					*fcb;
	BTreeControlBlock	*btcb;
	OSStatus			result;
	UInt16				operation;


	fcb = GetFileControlBlock(refNum);

	// pick up our iterator in the BTCB for context...

	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;
	iterator = &btcb->lastIterator;

	btRecord.bufferAddress = data;
	btRecord.itemCount = 1;
	if ( btcb->maxKeyLength == kSmallExtentKeyMaximumLength )
		btRecord.itemSize = sizeof(SmallExtentRecord);
	else if ( btcb->maxKeyLength == kLargeExtentKeyMaximumLength )
		btRecord.itemSize = sizeof(LargeExtentRecord);
	else
		btRecord.itemSize = sizeof(CatalogRecord);
	
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
	else if (selectionIndex == (SInt16) 0x8001)
	{
		operation = kBTreeFirstRecord;
	}
	else if (selectionIndex == (SInt16) 0x7FFF)
	{
		operation = kBTreeLastRecord;
	}
	else if (selectionIndex > 1)
	{
		UInt32	i;

		for (i = 1; i < selectionIndex; ++i)
		{
			result = BTIterateRecord( fcb, kBTreeNextRecord, iterator, &btRecord, dataSize );
			ExitOnError(result);
		}
		operation = kBTreeNextRecord;
	}
	else // (selectionIndex < -1)
	{
		SInt32	i;

		for (i = -1; i > selectionIndex; --i)
		{
			result = BTIterateRecord( fcb, kBTreePrevRecord, iterator, &btRecord, dataSize );
			ExitOnError(result);
		}
		operation = kBTreePrevRecord;
	}

	result = BTIterateRecord( fcb, operation, iterator, &btRecord, dataSize );

	if (result == noErr)
	{
		*newHint = iterator->hint.nodeNum;

		result = CheckBTreeKey(&iterator->key, btcb);
		ExitOnError(result);
		
		BlockMoveData(&iterator->key, key, CalcKeySize(btcb, &iterator->key));	//€€ warning, this could overflow user's buffer!!!
		
		if ( DEBUG_BUILD && !ValidHFSRecord(data, btcb, *dataSize) )
			DebugStr("\pGetBTreeRecord: bad record?");

	}
	
ErrorExit:

	return result;
}


OSErr InsertBTreeRecord(SInt16 refNum, void* key, void* data, UInt16 dataSize, UInt32 *newHint)
{
	FSBufferDescriptor	btRecord;
	BTreeIterator		iterator;
	FCB					*fcb;
	BTreeControlBlock	*btcb;
	OSStatus			result;
	

	fcb = GetFileControlBlock(refNum);
	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;

	btRecord.bufferAddress = data;
	btRecord.itemSize = dataSize;
	btRecord.itemCount = 1;

	iterator.hint.nodeNum = 0;			// no hint

	result = CheckBTreeKey((BTreeKey *) key, btcb);
	ExitOnError(result);

	BlockMoveData(key, &iterator.key, CalcKeySize(btcb, (BTreeKey *) key));	//€€ should we range check against maxkeylen?

	if ( DEBUG_BUILD && !ValidHFSRecord(data, btcb, dataSize) )
		DebugStr("\pInsertBTreeRecord: bad record?");

	result = BTInsertRecord( fcb, &iterator, &btRecord, dataSize );

	*newHint = iterator.hint.nodeNum;

	InvalidateBTreeIterator(refNum);	// invalidate current record markers
	
ErrorExit:

	return result;
}


OSErr DeleteBTreeRecord(SInt16 refNum, void* key)
{
	BTreeIterator		iterator;
	FCB					*fcb;
	BTreeControlBlock	*btcb;
	OSStatus			result;
	

	fcb = GetFileControlBlock(refNum);
	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;
	
	iterator.hint.nodeNum = 0;			// no hint

	result = CheckBTreeKey((BTreeKey *) key, btcb);
	ExitOnError(result);

	BlockMoveData(key, &iterator.key, CalcKeySize(btcb, (BTreeKey *) key));	//€€ should we range check against maxkeylen?

	result = BTDeleteRecord( fcb, &iterator );

	InvalidateBTreeIterator(refNum);	// invalidate current record markers

ErrorExit:

	return result;
}


OSErr ReplaceBTreeRecord(SInt16 refNum, const void* key, UInt32 hint, void *newData, UInt16 dataSize, UInt32 *newHint)
{
	FSBufferDescriptor	btRecord;
	BTreeIterator		iterator;
	FCB					*fcb;
	BTreeControlBlock	*btcb;
	OSStatus			result;


	fcb = GetFileControlBlock(refNum);
	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;

	btRecord.bufferAddress = newData;
	btRecord.itemSize = dataSize;
	btRecord.itemCount = 1;

	iterator.hint.nodeNum = hint;

	result = CheckBTreeKey((BTreeKey *) key, btcb);
	ExitOnError(result);

	BlockMoveData(key, &iterator.key, CalcKeySize(btcb, (BTreeKey *) key));		//€€ should we range check against maxkeylen?

	if ( DEBUG_BUILD && !ValidHFSRecord(newData, btcb, dataSize) )
		DebugStr("\pReplaceBTreeRecord: bad record?");

	result = BTReplaceRecord( fcb, &iterator, &btRecord, dataSize );

	*newHint = iterator.hint.nodeNum;

	//€€ do we need to invalidate the iterator?

ErrorExit:

	return result;
}



OSStatus GetBlockProc ( SInt16 fileRefNum, UInt32 blockNum, GetBlockOptions options, BlockDescriptor *block )
{
	OSStatus	result;
	short		cacheOptions;


	if (options == kGetBlock)
		cacheOptions = gbDefault;	
	else if (options & kForceReadBlock)
		cacheOptions = gbReadMask;	
	else if (options & kGetEmptyBlock)
		cacheOptions = gbNoReadMask;	
	else if (DEBUG_BUILD)
		Panic ("\pGetBlockProc: Invalid option!");

	if ( block->blockSize == 512 )
	{
		result = GetBlock_glue( cacheOptions, blockNum, (Ptr *)&block->buffer, fileRefNum, GetFileControlBlock(fileRefNum)->fcbVPtr );
		block->blockReadFromDisk = BlockCameFromDisk();
	}
	else
	{
		result = GetCacheBlock( fileRefNum, blockNum, block->blockSize, cacheOptions, &block->buffer, &block->blockReadFromDisk);
	}

	return result;
}


OSStatus	ReleaseBlockProc ( SInt16 fileRefNum, BlockDescPtr blockPtr, ReleaseBlockOptions options )
{
#pragma unused (fileRefNum)

	OSStatus	result;
	short		cacheOptions;


	if (options == kReleaseBlock)
		cacheOptions = rbDefault;	
	else if (options & kMarkBlockDirty)
		cacheOptions = rbDirtyMask;	
	else if (options & kForceWriteBlock)
		cacheOptions = rbWriteMask;	
	else if (options & kTrashBlock)
		cacheOptions = rbTrashMask;	
	else if (DEBUG_BUILD)
		Panic ("\pGetBlockProc: Invalid option!");
		
	//€€ if kReleaseBlock and just markdirty should we call markblock??
	
	
	if ( blockPtr->blockSize == 512 )
		result = RelBlock_glue( blockPtr->buffer, cacheOptions );
	else
		result = ReleaseCacheBlock( blockPtr->buffer, cacheOptions );

	return result;
}



OSStatus
SetBlockSizeProc ( SInt16 fileRefNum, ByteCount blockSize, ItemCount minBlockCount)
{
#pragma unused (fileRefNum)

	OSStatus result;

	// for HFS Plus volumes we'll need to set up a big node cache...

	if ( blockSize > 512 )
		result = InitializeBlockCache (blockSize, minBlockCount);
	else
		result = noErr;
	
	return result;
}



OSStatus
SetEndOfForkProc ( SInt16 fileRefNum, FSSize minEOF, FSSize maxEOF )
{
#pragma unused (maxEOF)

	OSStatus	result;
	UInt32		actualBytesAdded;
	UInt32		bytesToAdd;
	ExtendedVCB	*vcb;
	FCB			*filePtr;


	filePtr = GetFileControlBlock(fileRefNum);

	if ( minEOF > filePtr->fcbEOF )
	{
		bytesToAdd = minEOF - filePtr->fcbEOF;

		if (bytesToAdd < filePtr->fcbClmpSize)
			bytesToAdd = filePtr->fcbClmpSize;		//€€ why not always be a mutiple of clump size ???
	}
	else
	{
		if ( DEBUG_BUILD )
			DebugStr("\pSetEndOfForkProc: minEOF is smaller than current size!");
		return -1;
	}

	vcb = filePtr->fcbVPtr;
	result = ExtendFileC ( vcb, filePtr, bytesToAdd, 0, &actualBytesAdded );
	ReturnIfError(result);

	if (DEBUG_BUILD && actualBytesAdded < bytesToAdd)
		DebugStr("\pSetEndOfForkProc: actualBytesAdded < bytesToAdd!");
		
	filePtr->fcbEOF = filePtr->fcbPLen;		// new B-tree looks at fcbEOF
	
	//
	//	Update the Alternate MDB or Alternate VolumeHeader
	//
	if ( vcb->vcbSigWord == kHFSPlusSigWord )
	{
		//	If any of the HFS+ private files change size, flush them back to the Alternate volume header
		if (	(filePtr->fcbFlNm == kHFSExtentsFileID) 
			 ||	(filePtr->fcbFlNm == kHFSCatalogFileID)
			 ||	(filePtr->fcbFlNm == kHFSStartupFileID)
			 ||	(filePtr->fcbFlNm == kHFSAttributesFileID) )
		{
			MarkVCBDirty( vcb );
			result = FlushAlternateVolumeControlBlock( vcb, true );
		}
	}
	else if ( vcb->vcbSigWord == kHFSSigWord )
	{
		if ( filePtr->fcbFlNm == kHFSExtentsFileID )
		{
			vcb->vcbXTAlBlks = filePtr->fcbPLen / vcb->blockSize;
			MarkVCBDirty( vcb );
			result = FlushAlternateVolumeControlBlock( vcb, false );
		}
		else if ( filePtr->fcbFlNm == kHFSCatalogFileID )
		{
			vcb->vcbCTAlBlks = filePtr->fcbPLen / vcb->blockSize;
			MarkVCBDirty( vcb );
			result = FlushAlternateVolumeControlBlock( vcb, false );
		}
	}
	
	return result;

} // end SetEndOfForkProc


static void
InvalidateBTreeIterator(SInt16 refNum)
{
	FCB					*fcb;
	BTreeControlBlock	*btcb;

	fcb = GetFileControlBlock(refNum);
	btcb = (BTreeControlBlock*) fcb->fcbBTCBPtr;

	(void) BTInvalidateHint	( &btcb->lastIterator );
}


static OSErr CheckBTreeKey(const BTreeKey *key, const BTreeControlBlock *btcb)
{
	UInt16	keyLen;
	
	if ( btcb->attributes & kBTBigKeysMask )
		keyLen = key->length16;
	else
		keyLen = key->length8;

	if ( (keyLen < 6) || (keyLen > btcb->maxKeyLength) )
	{
		if ( DEBUG_BUILD )
			DebugStr("\pCheckBTreeKey: bad key length!");
		return fsBTInvalidKeyLengthErr;
	}
	
	return noErr;
}


static Boolean ValidHFSRecord(const void *record, const BTreeControlBlock *btcb, UInt16 recordSize)
{
	UInt32			cNodeID;
	
	if ( btcb->maxKeyLength == kSmallExtentKeyMaximumLength )
	{
		return ( recordSize == sizeof(SmallExtentRecord) );
	}
	else if (btcb->maxKeyLength == kLargeExtentKeyMaximumLength )
	{
		return ( recordSize == sizeof(LargeExtentRecord) );
	}
	else if (btcb->maxKeyLength == kAttributeKeyMaximumLength )
	{
		AttributeRecord	*attributeRecord = (AttributeRecord *) record;
		
		switch (attributeRecord->recordType) {
			case kAttributeInlineData:
				break;
			
			case kAttributeForkData:
				break;
			
			case kAttributeExtents:
				break;
		}
	}
	else // Catalog record
	{
		CatalogRecord *catalogRecord = (CatalogRecord*) record;

		switch(catalogRecord->recordType)
		{
			case kSmallFolderRecord:
			{
				if ( recordSize != sizeof(SmallCatalogFolder) )
					return false;
				if ( catalogRecord->smallFolder.flags != 0 )
					return false;
				if ( catalogRecord->smallFolder.valence > 0x7FFF )
					return false;
					
				cNodeID = catalogRecord->smallFolder.folderID;
	
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
					return false;
			}
			break;

			case kLargeFolderRecord:
			{
				if ( recordSize != sizeof(LargeCatalogFolder) )
					return false;
				if ( catalogRecord->largeFolder.flags != 0 )
					return false;
				if ( catalogRecord->largeFolder.valence > 0x7FFF )
					return false;
					
				cNodeID = catalogRecord->largeFolder.folderID;
	
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
					return false;
			}
			break;
	
			case kSmallFileRecord:
			{
				UInt16					i;
				SmallExtentDescriptor	*dataExtent;
				SmallExtentDescriptor	*rsrcExtent;
				
				if ( recordSize != sizeof(SmallCatalogFile) )
					return false;								
				if ( (catalogRecord->smallFile.flags & ~(0x83)) != 0 )
					return false;
					
				cNodeID = catalogRecord->smallFile.fileID;
				
				if ( cNodeID < 16 )
					return false;
		
				// make sure 0 ¾ LEOF ¾ PEOF for both forks
				
				if ( catalogRecord->smallFile.dataLogicalSize < 0 )
					return false;
				if ( catalogRecord->smallFile.dataPhysicalSize < catalogRecord->smallFile.dataLogicalSize )
					return false;
				if ( catalogRecord->smallFile.rsrcLogicalSize < 0 )
					return false;
				if ( catalogRecord->smallFile.rsrcPhysicalSize < catalogRecord->smallFile.rsrcLogicalSize )
					return false;
		
				dataExtent = (SmallExtentDescriptor*) &catalogRecord->smallFile.dataExtents;
				rsrcExtent = (SmallExtentDescriptor*) &catalogRecord->smallFile.rsrcExtents;
	
				for (i = 0; i < kSmallExtentDensity; ++i)
				{
					if ( (dataExtent[i].blockCount > 0) && (dataExtent[i].startBlock == 0) )
						return false;
					if ( (rsrcExtent[i].blockCount > 0) && (rsrcExtent[i].startBlock == 0) )
						return false;
				}
			}
			break;
	
			case kLargeFileRecord:
			{
				UInt16					i;
				LargeExtentDescriptor	*dataExtent;
				LargeExtentDescriptor	*rsrcExtent;
				
				if ( recordSize != sizeof(LargeCatalogFile) )
					return false;								
				if ( (catalogRecord->largeFile.flags & ~(0x83)) != 0 )
					return false;
					
				cNodeID = catalogRecord->largeFile.fileID;
				
				if ( cNodeID < 16 )
					return false;
		
				// make sure 0 ¾ LEOF ¾ PEOF for both forks
				
				if ( catalogRecord->largeFile.dataFork.logicalSize.hi != 0 )
					return false;
				if ( catalogRecord->largeFile.resourceFork.logicalSize.hi != 0 )
					return false;
		
				dataExtent = (LargeExtentDescriptor*) &catalogRecord->largeFile.dataFork.extents;
				rsrcExtent = (LargeExtentDescriptor*) &catalogRecord->largeFile.resourceFork.extents;
	
				for (i = 0; i < kLargeExtentDensity; ++i)
				{
					if ( (dataExtent[i].blockCount > 0) && (dataExtent[i].startBlock == 0) )
						return false;
					if ( (rsrcExtent[i].blockCount > 0) && (rsrcExtent[i].startBlock == 0) )
						return false;
				}
			}
			break;

			case kSmallFolderThreadRecord:
			case kSmallFileThreadRecord:
			{
				if ( recordSize != sizeof(SmallCatalogThread) )
					return false;
	
				cNodeID = catalogRecord->smallThread.parentID;
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
					return false;
							
				if ( (catalogRecord->smallThread.nodeName[0] == 0) ||
					 (catalogRecord->smallThread.nodeName[0] > 31) )
					return false;
			}
			break;
		
			case kLargeFolderThreadRecord:
			case kLargeFileThreadRecord:
			{
				if ( recordSize > sizeof(LargeCatalogThread) || recordSize < (sizeof(LargeCatalogThread) - sizeof(UniStr255)))
					return false;
	
				cNodeID = catalogRecord->largeThread.parentID;
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
					return false;
							
				if ( (catalogRecord->largeThread.nodeName.length == 0) ||
					 (catalogRecord->largeThread.nodeName.length > 255) )
					return false;
			}
			break;

			default:
				return false;
		}
	}
	
	return true;	// record appears to be OK
}
