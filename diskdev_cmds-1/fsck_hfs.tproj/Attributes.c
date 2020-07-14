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
	File:		Attributes.c

	Contains:	Extended Attributes Manager
				Routines for managing attributes (additional catalog-like data) associated
				with files or folders on HFS Plus volumes.

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		Greg Parks

		Technology:			HFS Plus

	Writers:

		(djb)	Don Brady
		(DSH)	Deric Horn
		(msd)	Mark Day

	Change History (most recent first):

	  <CS13>	 11/7/97	msd		Change calls to the wrapper routine CompareUnicodeNames() to use
									the underlying routine FastUnicodeCompare() instead.
	  <CS12>	10/17/97	msd		Remove DebugStrs.
	  <CS11>	10/13/97	DSH		Added InitBTreeHeader() fileSize parameter.
	  <CS10>	  9/4/97	msd		Remove unneeded/unused routines.
	   <CS9>	 8/22/97	djb		Add readFromDisk flag to GetCacheBlock.
	   <CS8>	 7/25/97	DSH		Pass kInvalidMRUCacheKey to BTSearchRecord as the heuristicHint.
	   <CS7>	 7/24/97	djb		SetEndOfForkProc takes a refNum instead of an FCB.
	   <CS6>	 7/18/97	msd		Include LowMemPriv.h.
	   <CS5>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS4>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS3>	  6/2/97	DSH		On creation make sure we call through the SetEndOfFileProc so
									the AlternateVolumeHeader is updated.
	   <CS2>	 5/28/97	msd		Change keys to {cnid, flags, creator, selector}. Prototype an
									SPI for testing.
	   <CS1>	 4/24/97	djb		first checked in
	  <HFS5>	 4/10/97	msd		In PropertyDeleteObject, use the global buffer instead of an
									automatic variable.
	  <HFS4>	  4/8/97	msd		Add AttributesOpenVolume to allocate appropriate buffer space.
	  <HFS3>	  4/7/97	msd		Start adding real code. Removed PropertyOpenVolume; that code
									now resides in Volumes.c.
	  <HFS2>	 3/17/97	DSH		SC needs parameters to the stub functions.
	  <HFS1>	  3/5/97	msd		first checked in
*/

/*
Public routines:
	AttributesCloseVolume
				Clean up and finalize for a given volume.  Close a property database
				file if needed.  In general, clean up after PropertyOpenVolume.
				This routine should be called before closing the Catalog and Extents
				files on the volume.
*/
	
#if TARGET_OS_MAC
#if ( PRAGMA_LOAD_SUPPORTED )
	#pragma	load	PrecompiledHeaders
#else
	#include <Types.h>
	#include <LowMemPriv.h>
#endif
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

#include "HFSVolumesPriv.h"
#include "FileMgrInternal.h"
#include "BTreesInternal.h"
#include "HFSBTreesPriv.h"
#include "HFSUnicodeWrappers.h"

#if HFSInstrumentation
	#include "Instrumentation.h"
#endif

#define min(x,y)	((x < y) ? x : y)

//
//	Values for flags in AttributesKey.flags
//
enum {
	kAttributeNodeSize		= 4096			// Size of our Btree node
};


#ifdef INVESTIGATE

OSErr AttributesCloseVolume(ExtendedVCB *		vcb)
{
	OSErr	err;
	
	err = noErr;
	
	if (vcb->vcbSigWord == kHFSPlusSigWord && vcb->attributesRefNum != 0) {
		err = CloseFile( vcb, vcb->attributesRefNum, LMGetFCBSPtr() );
		vcb->attributesRefNum = 0;
	}
	
	return err;
}

#endif

SInt32 CompareAttributeKeys( const void *inSearchKey, const void *inTrialKey )
{
	const AttributeKey *searchKey	= inSearchKey;
	const AttributeKey *trialKey	= inTrialKey;
	SInt32				temp;
		
	//
	//	First, compare the CNID's
	//
	if (searchKey->cnid != trialKey->cnid) {
		return searchKey->cnid < trialKey->cnid ? -1 : 1;
	}

	//
	//	CNID's are equal; compare names
	//
	if (searchKey->attributeName.length == 0 || trialKey->attributeName.length == 0)
		return searchKey->attributeName.length < trialKey->attributeName.length ? -1 : 1;
	temp = FastUnicodeCompare(&searchKey->attributeName.unicode[0], searchKey->attributeName.length,
							  &trialKey->attributeName.unicode[0], trialKey->attributeName.length);
	if (temp != 0)
		return temp;

	//
	//	Names are equal; compare startBlock
	//
	if (searchKey->startBlock == trialKey->startBlock)
		return 0;
	else
		return searchKey->startBlock < trialKey->startBlock ? -1 : 1;
}

#ifdef INVESTIGATE

static OSErr CreateAndOpenBtree(ExtendedVCB *vcb)
{
	OSStatus		err;
	UInt16			refnum;				//	attribute file's refnum
	Ptr				fcbHeader;			//	points to start of FCBs (unused)
	FCB				*fcb;				//	attribute file's FCB
	ExtendedFCB		*extendedFCB;		//	attribute file's extended FCB
	UInt32			clumpSize;
	UInt32			mapNodes;
	LogicalAddress	header;
	Boolean			readFromDisk;
	
	clumpSize = 32 * vcb->blockSize;			//€€ Need a better value.

	//	Find a free FCB for the attributes file
	err = FindFileControlBlock(&refnum, &fcbHeader);
	ReturnIfError(err);
//	fcb = GetFileControlBlock(refnum);
	fcb = SetupFCB(vcb, refnum, kHFSAttributesFileID, clumpSize);

	//	Mark file as empty.
	fcb->fcbEOF = 0;
	fcb->fcbPLen = 0;
	vcb->attributesRefNum = refnum;
	ClearMemory( (Ptr)fcb->fcbExtRec, sizeof(SmallExtentRecord) );
	extendedFCB = ParallelFCBFromRefnum( refnum );
	ClearMemory( (Ptr)extendedFCB->extents, sizeof(LargeExtentRecord) );

	//	Allocate some space to the file
	err = SetEndOfForkProc(refnum, clumpSize, clumpSize);
	if (err) {
		ClearMemory(fcb, sizeof(FCB));
		goto ErrorExit;
	}
	fcb->fcbEOF = fcb->fcbPLen;		// grow the file to include the allocated space
	
	//	Initialize the b-tree.  Write out the header.
	err = GetCacheBlock(refnum, 0, kAttributeNodeSize, gbDefault, &header, &readFromDisk);
	ClearMemory(header, kAttributeNodeSize);
	InitBTreeHeader(fcb->fcbEOF, clumpSize, kAttributeNodeSize, 0, kAttributeKeyMaximumLength, kBTBigKeysMask, &mapNodes, header);
	err = ReleaseCacheBlock(header, rbWriteMask);
	
	//	Finally, prepare for using the B-tree		
	err = OpenBTree( refnum, (KeyCompareProcPtr) CompareAttributeKeys );
	if (err) {
		ClearMemory(fcb, sizeof(FCB));
		goto ErrorExit;
	}
	
	//€€ Should we update the volume header and alternate volume header?

ErrorExit:	
	return err;
}



static OSErr PreflightAttributeCall(AttributeParam *pb, FindFileNameGlueRec *fileInfo, Boolean needsWrite)
{
	OSErr				err;
	ExtendedVCB			*vcb;

	//
	//	See if the file exists.  Get its FileID.
	//
	err = FindFileName((ParamBlockRec *) pb, fileInfo);
	ReturnIfError( err );
	
	//
	//	Make sure the volume is on-line and writable.
	//
	vcb = fileInfo->vcb;
	
	if (needsWrite) {
		err = VolumeWritable( vcb );
		ReturnIfError( err );
	}
	
	err = CheckVolumeOffLine( vcb );
	ReturnIfError( err );
	
	//
	//	And make sure it is HFS Plus.
	//
	if (vcb->vcbSigWord != kHFSPlusSigWord)
		return wrgVolTypErr;
	
	//
	//€€	Make sure there is an attribute file.  Create and open it if needed.
	//
	if (vcb->attributesRefNum == 0) {
		
		if (needsWrite == false)		//	If we only wanted to read,
			return notBTree;			//	tell caller there is no b-tree
		//
		//	Create and open b-tree
		//
		err = CreateAndOpenBtree(vcb);
	}
	
	return err;
}



OSErr CreateAttribute(AttributeParam *pb)
{
	OSErr				err;
	ExtendedVCB			*vcb;
	UInt32				length;
	UInt32				numberOfExtents;
	UInt32				i,extentsThisRecord;
	UInt32				blocksAllocated = 0;
	UInt32				hint;
	AttributeKey		key;
	FindFileNameGlueRec	fileInfo;

	err = PreflightAttributeCall(pb, &fileInfo, true);
	ReturnIfError( err );
	
	vcb = fileInfo.vcb;
	
	//
	//	Set up the first key
	//
	length = sizeof(UniChar) * (pb->ioAttributeName->length+1);		// length in bytes of Unicode name

	key.keyLength	= length + offsetof(AttributeKey, attributeName);
	key.pad			= 0;
	key.cnid		= fileInfo.data->nodeID;
	key.startBlock	= 0;
	BlockMoveData(pb->ioAttributeName, &key.attributeName, length);	// copy attributeName
	
	//
	//	Determine the first record type based on the number of extents
	//
	numberOfExtents = pb->filler1;
	if (numberOfExtents == 0) {
		AttributeInlineData		record;
		
		record.recordType = kAttributeInlineData;
		record.logicalSize = 0;
		length = offsetof(AttributeInlineData, userData);
		err = InsertBTreeRecord(vcb->attributesRefNum, &key, &record, length, &hint);
	}
	else {
		{
			AttributeForkData		record;
			
			//	Set up first record, with ForkData
			ClearMemory(&record, sizeof(record));
			record.recordType				= kAttributeForkData;
			record.theFork.logicalSize.lo	= numberOfExtents * vcb->blockSize;		// each extent is one block
			record.theFork.logicalSize.hi	= 0;
			record.theFork.clumpSize		= vcb->attributesClumpSize;
			record.theFork.totalBlocks		= numberOfExtents;						// one block per extent
			extentsThisRecord = min(numberOfExtents, kLargeExtentDensity);
			for (i=0; i<extentsThisRecord; i++) {
				err = BlockAllocateAny(vcb, 0, vcb->totalBlocks, 1, &record.theFork.extents[i].startBlock,
																	&record.theFork.extents[i].blockCount);
				ReturnIfError(err);
				++blocksAllocated;
			}
			err = InsertBTreeRecord(vcb->attributesRefNum, &key, &record, sizeof(record), &hint);
			ReturnIfError(err);
		}
		
		{
			AttributeExtents		record;
			
			//	Handle overflow extent records
			ClearMemory(&record, sizeof(record));
			numberOfExtents -= extentsThisRecord;
			record.recordType	= kAttributeExtents;
			while (numberOfExtents) {
				extentsThisRecord = min(numberOfExtents, kLargeExtentDensity);
				key.startBlock += kLargeExtentDensity;
				for (i=0; i<extentsThisRecord; i++) {
					err = BlockAllocateAny(vcb, 0, vcb->totalBlocks, 1, &record.extents[i].startBlock,
																		&record.extents[i].blockCount);
					ReturnIfError(err);
					++blocksAllocated;
				}
				for (; i<kLargeExtentDensity; i++) {
					record.extents[i].startBlock = 0;
					record.extents[i].blockCount = 0;
				}
				err = InsertBTreeRecord(vcb->attributesRefNum, &key, &record, sizeof(record), &hint);
				ReturnIfError(err);
				numberOfExtents -= extentsThisRecord;
			}
		}
	}
	
	if (blocksAllocated) {
		vcb->freeBlocks -= blocksAllocated;
		UpdateVCBFreeBlks( vcb );
		MarkVCBDirty(vcb);
	}
	
	if (err == fsBTDuplicateRecordErr)
		err = btDupRecErr;
			
	return err;
}

#endif