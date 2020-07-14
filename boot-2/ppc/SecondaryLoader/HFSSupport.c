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
/* -*- mode:C++; tab-width: 4 -*- */
/* Functions to support booting from HFS volumes */

#ifndef HFS_SUPPORT
#define HFS_SUPPORT	0
#endif

#include <Types.h>
#include <BootBlocksPriv.h>
// #include "/usr/include/setjmp.h"
#include <stdio.h>
#include <memory.h>

#include "HFSSupport.h"
#include "SecondaryLoaderOptions.h"
#include <SecondaryLoader.h>


static UInt32 blessedDirID;
static UInt32 firstAllocationBlockNumber;
static UInt32 allocationBlockSizeInBlocks;
static int partitionNumber;


FileRecord *fFindFileRecord (UInt32 node, BTreeNode *nodeBufferP,
							ExtentsArray *catalogExtentsP, CatalogKey *leafKeyP)
{
#if HFS_SUPPORT
	for (;;) {
		int nRecords = VCALL(ReadHFSNode) (node, catalogExtentsP, nodeBufferP);
		int rec;

		for (rec = 0; rec < nRecords; ++rec) {
			CatalogKey *keyP = VCALL(FindBTreeRecord) (nodeBufferP, rec);
			int compareResult = VCALL(FileNameCompare) (keyP, leafKeyP);

#ifdef DEBUG
                        if( slDebugFlag & kDebugLots) {
				CIArgs ciArgs;
		
				ciArgs.service = "interpret";
				ciArgs.nArgs = 3;
				ciArgs.nReturns = 1;
				ciArgs.args.interpret_2_0.forth =
					".( FFR cmp=) s. .( key:) dup dup c@ 4+ dumpl cr "
					"dup 2+ @ . "
					"7 + dup 1- c@ 22 emit type 22 emit cr cr";
				ciArgs.args.interpret_2_0.arg1 = compareResult;
				ciArgs.args.interpret_2_0.arg2 = (CICell) keyP;
				CallCI (&ciArgs);
			}
#endif

			if (compareResult == 0xDEADBEEF) return nil;		// FILE NOT FOUND!

			if (compareResult == 0)
				return (FileRecord *) ((char *) keyP + keyP->keyLength + 1);
		}

		node = nodeBufferP->fLink;			// Move to next leaf node in the sequence
	}
#else
	return nil;
#endif
}


void *fFindBTreeRecord (BTreeNode *nodeBufferP, int recordNumber)
{
#if HFS_SUPPORT
	// recordNumber ^ 0xFF gets us what we want: a mapping starting
	// with 0->0xFF, 1->0xFE, 2->0xFD, ... 0xFF->0.
	return (void *) ((char *) nodeBufferP
					 + ((UInt16 *) nodeBufferP)[recordNumber ^ 0xFF]);
#else
	return nil;
#endif
}


void fGetFileExtents (ExtentsArray *extentsTreeExtentsP, UInt32 fileID,
					  ExtentsArray *extentsBufferP)
{
#if HFS_SUPPORT
	ExtentsKey leafKey;
	UInt32 node;
	int nRecords;
	BTreeNode nodeBuffer;

	// Set up a key to compare against so we find the first leaf node
	// whose file ID is fileID
	leafKey.keyLength = sizeof (leafKey);
	leafKey.fileID = fileID;
	leafKey.allocationBlockNumber = 0;

	node = VCALL(FindLeafNode) (extentsTreeExtentsP, (CatalogKey *) &leafKey,
								gSecondaryLoaderVectors->ExtentsIDCompare,
								&nodeBuffer, &nRecords);

	if (node == 0) {

#ifdef DEBUG
        if( slDebugFlag & kDebugLots)
		VCALL(ShowMessage) ("GFE no leaf");
#endif

		return;			// Do nothing if no match
	}

	for (;;) {
		int recNum;
		
		for (recNum = 0; recNum < nRecords; ++recNum) {
			ExtentsKey *thisKeyP = VCALL(FindBTreeRecord) (&nodeBuffer, recNum);
			long delta = thisKeyP->fileID - fileID;
			
#ifdef DEBUG
                if( slDebugFlag & kDebugLots) {
				CIArgs ciArgs;
		
				ciArgs.service = "interpret";
				ciArgs.nArgs = 3;
				ciArgs.nReturns = 1;
				ciArgs.args.interpret_2_0.forth = ".( GFE r# n#=) . . cr";
				ciArgs.args.interpret_2_0.arg1 = recNum;
				ciArgs.args.interpret_2_0.arg2 = node;
				CallCI (&ciArgs);
			}
#endif

			if (delta == 0) {
				// Append this extent trio to the end of our array
				VCALL(MoveBytes) (extentsBufferP->extents + extentsBufferP->nExtents,
							thisKeyP + 1,
							3 * sizeof (Extent));
				extentsBufferP->nExtents += 3; // Remember we added as many as 3 entries
			} else if (delta > 0) {
				return;					// We're past the interesting records--just exit
			}
		}

		node = nodeBuffer.fLink;			// Get node # of next node in leaf sequence
		if (node == 0) break;				// NIL forward link means we're done
		nRecords = VCALL(ReadHFSNode) (node, extentsTreeExtentsP, &nodeBuffer);
	}
#endif
}


int fReadHFSNode (UInt32 nodeNumber,
				  ExtentsArray *extentsBufferP,
				  BTreeNode *nodeBufferP)
{
#if HFS_SUPPORT
	SInt32 n = -1;
	int extent;
	UInt32 blocksToGo = nodeNumber;

#ifdef DEBUG
        if( slDebugFlag & kDebugLots) {
		CIArgs ciArgs;

		ciArgs.service = "interpret";
		ciArgs.nArgs = 3;
		ciArgs.nReturns = 1;
		ciArgs.args.interpret_2_0.forth = ".( RN extents: [) . .( ]) 20 dumpl cr";
		ciArgs.args.interpret_2_0.arg1 = extentsBufferP->nExtents;
		ciArgs.args.interpret_2_0.arg2 = (CICell) extentsBufferP->extents;
		CallCI (&ciArgs);
	}
#endif
	// Loop through extents in the B*Tree file's extent list until we
	// find the one containing nodeNumber's node.
	for (extent = 0; extent < extentsBufferP->nExtents; ++extent) {
		UInt16 thisLengthInBlocks =
			extentsBufferP->extents[extent].length * allocationBlockSizeInBlocks;

#ifdef DEBUG
                if( slDebugFlag & kDebugLots) {
			CIArgs ciArgs;
	
			ciArgs.service = "interpret";
			ciArgs.nArgs = 4;
			ciArgs.nReturns = 1;
			ciArgs.args.interpret_3_0.forth = ".( RN n# Strt Len=) . . . cr";
			ciArgs.args.interpret_3_0.arg1 = nodeNumber;
			ciArgs.args.interpret_3_0.arg2 = extentsBufferP->extents[extent].start;
			ciArgs.args.interpret_3_0.arg3 = extentsBufferP->extents[extent].length;
			CallCI (&ciArgs);
		}
#endif
		if (blocksToGo < thisLengthInBlocks) {
			n = blocksToGo
				+ extentsBufferP->extents[extent].start * allocationBlockSizeInBlocks;
			break;
		}
		
		blocksToGo -= thisLengthInBlocks;
	}
	
	if (n < 0) {
		VCALL(FatalError) ("?node#");
		return 0;
	}

	// Read block containing the node we've been asked to retrieve	
#ifdef DEBUG
        if( slDebugFlag & kDebugLots) VCALL(ShowMessage) ("RdN");
#endif
	VCALL(ReadPartitionBlocks) (partitionNumber, nodeBufferP,
								firstAllocationBlockNumber + n, 1);
	return nodeBufferP->nRecords;
#else
	return 0;
#endif
}


UInt32 fFindLeafNode (ExtentsArray *extentsBufferP, CatalogKey *keyToFindP,
					  int (*compareFunction) (CatalogKey *key1, CatalogKey *key2),
					  BTreeNode *nodeBufferP, int *nRecordsP)
{
#if HFS_SUPPORT
	int node;

	// Find the root node number of the B*Tree file our extents list describes
	(void) VCALL(ReadHFSNode) (0, extentsBufferP, nodeBufferP);
	node = ((BTreeHeaderNode *) nodeBufferP)->root;
	if (node == 0) return 0;			// Empty B*Trees are boring

	for (;;) {
		int nRecords = VCALL(ReadHFSNode) (node, extentsBufferP, nodeBufferP);
		int recordToFollow = nRecords - 1;
		int rec;
		CatalogKey *keyP;

#ifdef DEBUG
                if( slDebugFlag & kDebugLots) {
				CIArgs ciArgs;
		
				ciArgs.service = "interpret";
				ciArgs.nArgs = 3;
				ciArgs.nReturns = 1;
				ciArgs.args.interpret_2_0.forth = ".( FLN nt n#=) . . cr";
				ciArgs.args.interpret_2_0.arg1 = nodeBufferP->nodeType;
				ciArgs.args.interpret_2_0.arg2 = node;
				CallCI (&ciArgs);
			}
#endif
		// Bail if we find a leaf node -- we're done!
		if (nodeBufferP->nodeType == kLeafNodeType) return node;
		
		for (rec = 0; rec < nRecords; ++rec) {
			int compareResult =
				(*compareFunction) (VCALL(FindBTreeRecord) (nodeBufferP, rec),
									keyToFindP);

#ifdef DEBUG
                    if( slDebugFlag & kDebugLots) {
				CIArgs ciArgs;
		
				ciArgs.service = "interpret";
				ciArgs.nArgs = 4;
				ciArgs.nReturns = 1;
				ciArgs.args.interpret_3_0.forth =
					".( FLN r# rtf#=) . . "
					".( key:) dup dup c@ 4+ dumpl cr "
					"dup 2+ @ . "
					"7 + dup 1- c@ 22 emit type 22 emit cr cr";
				ciArgs.args.interpret_3_0.arg1 = rec;
				ciArgs.args.interpret_3_0.arg2 = recordToFollow;
				ciArgs.args.interpret_3_0.arg3 =
					(CICell) VCALL(FindBTreeRecord) (nodeBufferP, rec);
				CallCI (&ciArgs);
			}
#endif
			if (compareResult == 0) {
				recordToFollow = rec;
				break;
			} else if (compareResult > 0) {

				if (rec > 0) {
					recordToFollow = rec - 1;
					break;
				} else {
					*nRecordsP = 0;
					return 0;
				}
			}
		}
		
		// Find address of record we have chosen to follow
		keyP = VCALL(FindBTreeRecord) (nodeBufferP, recordToFollow);

#ifdef DEBUG
                if( slDebugFlag & kDebugLots) {
			CIArgs ciArgs;
	
			ciArgs.service = "interpret";
			ciArgs.nArgs = 2;
			ciArgs.nReturns = 1;
			ciArgs.args.interpret_1_0.forth =
				".( FLN follow key:) dup dup c@ 4+ dumpl cr "
				"dup 2+ @ . "
				"7 + dup 1- c@ 22 emit type 22 emit cr cr";
			ciArgs.args.interpret_1_0.arg1 = (CICell) keyP;
			CallCI (&ciArgs);
		}
#endif
		// Follow pointer just after that record in the node
		node = *(UInt32 *) ((char *) keyP + keyP->keyLength + 1);
	}
	
#endif
	return 0;
}


// Compare catalog B*Tree record key1 with key2 looking at the parent
// ID part of the key.  This is used to follow the index links to find
// the left-most leaf node containing key2's parent ID.  This is the
// first record in the sequence of records for the blessed folder
// directory.  Returns ZERO if key1's parent ID field is equal to
// key2's parent ID and the length of the following filename string is
// zero.  Otherwise returns negative if key1's parent ID is less than
// key2's parent ID, and positive if key1's parent ID is greater than
// key2's parent ID.
int fFileIDCompare (CatalogKey *key1, CatalogKey *key2)
{
#if HFS_SUPPORT
	long delta = key1->parentID - key2->parentID;

	if (delta != 0) return delta;

	// Directory IDs are equal, so return length byte for name following parent ID field
	return StrLength (key1->name);
#else
	return 0;
#endif
}


// Compare record keys of Extents B*Tree normally with a static key
int fExtentsIDCompare (CatalogKey *key1, CatalogKey *key2)
{
#if HFS_SUPPORT
	ExtentsKey *ekey1 = (ExtentsKey *) key1;
	ExtentsKey *ekey2 = (ExtentsKey *) key2;
	
	long delta = ekey1->fileID - ekey2->fileID;

	if (delta != 0) return delta;
	
	if (ekey1->resourceForkFlag) return 1;

	// File ID matches, so return negative allocation block number of extent	
	return -ekey1->allocationBlockNumber;
#else
	return 0;
#endif
}


// Returns nonzero only if the two Pascal strings are identical byte for byte.
int fPStringsArePreciselyEqual (StringPtr string1, StringPtr string2)
{
	int n = StrLength (string1);
	
	if (n != StrLength (string2)) return false;

	for (; n > 0; --n)
		if (string1[n] != string2[n]) return false;

	return true;
}


// Compare catalog B*Tree record key1 with key2 looking first the parent ID
// part of the key and then FOR EQUALITY ONLY the name string part of the key.
// If the strings are unequal, assume key2 > key1 so we keep looking through
// all keys with the same parent ID until we find a match are all are exhausted.
int fFileNameCompare (CatalogKey *key1, CatalogKey *key2)
{
#if HFS_SUPPORT
	long delta = key1->parentID - key2->parentID;

	if (delta < 0) return delta;
	
	if (delta == 0)
		return !VCALL(PStringsArePreciselyEqual) (key1->name, key2->name);

	VCALL(FatalError) ("?File");
	return 0xDEADBEEF;			// Special value to indicate we died
#else
	return 0;
#endif
}
