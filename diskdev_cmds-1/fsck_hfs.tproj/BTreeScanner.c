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
	File:		BTreeScanner.c

	Contains:	Routines for quick reading of B-Tree records (not in sorted order).
	
				Using a caller-supplied buffer, these routines read in a buffer full
				of B-tree nodes.  Pointers to the leaf records' key and data are
				returned one at a time.  The buffer is refilled as needed.  It is the
				caller's responsibility to flush any changes to the B-tree before
				starting a scan.
				
				These routines were designed for use by CatSearch and MountCheck.
				
				NOTE: Records will not be returned in sorted order.  Within a given node,
				they will be in order, but the nodes may not be read in sorted order.

	Version:	HFS Plus 1.0

	Written by:	Mark Day

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

		 <4>	10/31/97	DSH		Added CheckForStop() for DiskFirstAid builds.
	   <CS3>	  8/1/97	msd		Don't prefill node buffer in BTScanInitialize (fixes MountCheck
									error with empty extents B-Tree). Remove extraneous enum for
									timeOutErr.
	   <CS2>	 7/30/97	DSH		Casting for SC, needed for DFA compiles.
	   <CS1>	 7/28/97	msd		first checked in
	   <0>	 7/24/97	msd		Begin implementation.

*/

#include <FileMgrInternal.h>
#include <HFSBTreesPriv.h>
#include "BTreesPrivate.h"
#include "BTreeScanner.h"

#if ( FORDISKFIRSTAID )
	#include	"ScavDefs.h"
	#include	"Prototypes.h"
#endif


//======================================================================
//	Exported Routines:
//
//	BTScanInitialize	- Set up to start or resume a scan
//	BTScanTerminate		- Obtain state to later resume a scan
//	BTScanNextRecord	- Get the next record
//	BTScanCacheNode		- Make sure the current node is in the cache
//
//	Internal Routines:
//
//	FindNextLeafNode	- Point to the next leaf node after the current
//						  node.  More nodes will be read if needed (and
//						  allowed by the caller).
//	ReadMultipleNodes	- Read one or more nodes into a buffer.
//======================================================================




OSErr FindNextLeafNode(BTScanState *scanState, Boolean avoidIO);
OSErr ReadMultipleNodes(BTScanState *scanState);




//_________________________________________________________________________________
//
//	Routine:	BTScanInitialize
//
//	Purpose:	Prepare to start a new BTree scan, or resume a previous one.
//
//	Inputs:
//		btreeFile		The B-Tree's file control block
//		startingNode	Initial node number
//		startingRecord	Initial record number within node
//		recordsFound	Number of valid records found so far
//		buffer			Start of buffer used to hold B-Tree data
//		bufferSize		Size (in bytes) of buffer
//
//	Outputs:
//		scanState		Scanner's current state; pass to other scanner calls
//
//	Notes:
//		To begin a new scan and see all records in the B-Tree, pass zeroes for
//		startingNode, startingRecord, and recordsFound.
//
//		To resume a scan from the point of a previous BTScanTerminate, use the
//		values returned by BTScanTerminate as input for startingNode, startingRecord,
//		and recordsFound.
//
//		When resuming a scan, the caller should check the B-tree's write count.  If
//		it is different from the write count when the scan was terminated, then the
//		tree may have changed and the current state may be incorrect.  In particular,
//		you may see some records more than once, or never see some records.  Also,
//		the scanner may not be able to detect when all leaf records have been seen,
//		and will have to scan through many empty nodes.
//
//		€€ Perhaps the write count should be managed by BTScanInitialize and
//		€€ BTScanTerminate?  This would avoid the caller having to peek at
//		€€ internal B-Tree structures.
//_________________________________________________________________________________

OSErr BTScanInitialize(
	const FCB *		btreeFile,
	UInt32			startingNode,
	UInt32			startingRecord,
	UInt32			recordsFound,
	void *			buffer,
	UInt32			bufferSize,
	BTScanState	*	scanState)
{
	BTreeControlBlock	*btcb;
	
	//
	//	Make sure this is a valid B-Tree file
	//
	btcb = (BTreeControlBlock *) btreeFile->fcbBTCBPtr;
	if (btcb == NULL)
		return fsBTInvalidFileErr;
	
	//
	//	Make sure buffer size is big enough, and a multiple of the
	//	B-Tree node size
	//
	if (buffer == NULL || bufferSize < btcb->nodeSize)
		return paramErr;
	bufferSize = (bufferSize / btcb->nodeSize) * btcb->nodeSize;

	//
	//	Set up the scanner's state
	//
	scanState->buffer				= buffer;
	scanState->bufferSize			= bufferSize;
	scanState->btcb					= btcb;
	scanState->nodeNum				= startingNode;
	scanState->recordNum			= startingRecord;
	scanState->currentNode			= buffer;
	scanState->nodesLeftInBuffer	= 0;		// no nodes currently in buffer
	scanState->recordsFound			= recordsFound;
	scanState->lastCachedNode		= 0;		// no nodes have been cached

	
	return noErr;
}



//_________________________________________________________________________________
//
//	Routine:	BTScanTerminate
//
//	Purpose:	Return state information about a scan so that it can be resumed
//				later via BTScanInitialize.
//
//	Inputs:
//		scanState		Scanner's current state
//
//	Outputs:
//		nextNode		Node number to resume a scan (pass to BTScanInitialize)
//		nextRecord		Record number to resume a scan (pass to BTScanInitialize)
//		recordsFound	Valid records seen so far (pass to BTScanInitialize)
//_________________________________________________________________________________

OSErr BTScanTerminate(
	const BTScanState *	scanState,
	UInt32 *			startingNode,
	UInt32 *			startingRecord,
	UInt32 *			recordsFound)
{
	*startingNode	= scanState->nodeNum;
	*startingRecord	= scanState->recordNum;
	*recordsFound	= scanState->recordsFound;
	
	return noErr;
}



//_________________________________________________________________________________
//
//	Routine:	BTScanNextRecord
//
//	Purpose:	Return the next leaf record in a scan.
//
//	Inputs:
//		scanState		Scanner's current state
//		avoidIO			If true, don't do any I/O to refill the buffer
//
//	Outputs:
//		key				Key of found record (points into buffer)
//		data			Data of found record (points into buffer)
//		dataSize		Size of data in found record
//
//	Result:
//		noErr			Found a valid record
//		btNotFound		No more records
//		???				Needed to do I/O to get next node, but avoidIO set
//
//	Notes:
//		This routine returns pointers to the found record's key and data.  It
//		does not copy the key or data to a caller-supplied buffer (like
//		GetBTreeRecord would).  The caller must not modify the key or data.
//_________________________________________________________________________________

OSErr BTScanNextRecord(
	BTScanState *	scanState,
	Boolean			avoidIO,
	void * *		key,
	void * *		data,
	UInt32 *		dataSize)
{
	OSStatus	err;
	UInt16		dataSizeShort;
	
	err = noErr;

	//
	//	If this is the first call, there won't be any nodes in the buffer, so go
	//	find the first first leaf node (if any).
	//	
	if (scanState->nodesLeftInBuffer == 0)
		err = FindNextLeafNode(scanState, avoidIO);

	while (err == noErr) {
		//	See if we have a record in the current node
		err = GetRecordByIndex(scanState->btcb, scanState->currentNode, scanState->recordNum,
								(KeyPtr *) key, (UInt8 **) data, &dataSizeShort);
		if (err == noErr) {
			++scanState->recordsFound;
			++scanState->recordNum;
			*dataSize = dataSizeShort;
			return noErr;
		}
		
		//	We're done with the current node.  See if we've returned all the records
		if (scanState->recordsFound >= scanState->btcb->leafRecords)
			return btNotFound;

		//	Move to the first record of the next leaf node
		scanState->recordNum = 0;
		err = FindNextLeafNode(scanState, avoidIO);
	}

	//
	//	If we got an EOF error from FindNextLeafNode, then there are no more leaf
	//	records to be found.
	//
	if (err == eofErr)
		err = btNotFound;
	
	return err;
}



//_________________________________________________________________________________
//
//	Routine:	BTScanCacheNode
//
//	Purpose:	Make sure that the node just returned is in the cache.
//
//	Inputs:
//		scanState		Scanner's current state
//_________________________________________________________________________________

OSErr BTScanCacheNode(BTScanState *scanState)
{
	OSStatus	err;
	NodeRec		nodeRec;
	
	err = noErr;
	
	if (scanState->lastCachedNode != scanState->nodeNum) {
		err = GetNode(scanState->btcb, scanState->nodeNum, &nodeRec);
		if (err == noErr)
			err = ReleaseNode(scanState->btcb, &nodeRec);
		scanState->lastCachedNode = scanState->nodeNum;
	}
	
	return err;
}



//_________________________________________________________________________________
//
//	Routine:	FindNextLeafNode
//
//	Purpose:	Point to the next leaf node in the buffer.  Read more nodes
//				into the buffer if needed (and allowed).
//
//	Inputs:
//		scanState		Scanner's current state
//		avoidIO			If true, don't do any I/O to refill the buffer
//
//	Result:
//		noErr			Found a valid record
//		eofErr			No more nodes in file
//		???				Needed to do I/O to get next node, but avoidIO set
//_________________________________________________________________________________

OSErr FindNextLeafNode(BTScanState *scanState, Boolean avoidIO)
{
	OSErr	err;
	
	err = noErr;		// Assume everything will be OK
	
	while (1) {
		if (scanState->nodesLeftInBuffer == 0) {
			//	Time to read some more nodes into the buffer
			if (avoidIO) {
				return fsBTTimeOutErr;
			}
			else {
				//	read some more nodes into buffer
				scanState->currentNode = scanState->buffer;
				err = ReadMultipleNodes(scanState);
				if (err != noErr) break;
			}
		}
		else {
			//	Adjust the node counters and point to the next node in the buffer
			++scanState->nodeNum;
			--scanState->nodesLeftInBuffer;
			(UInt8 *) scanState->currentNode += scanState->btcb->nodeSize;
			
			//	If we've looked at all nodes in the tree, then we're done
			if (scanState->nodeNum > scanState->btcb->totalNodes)
				return eofErr;
		}
		
		//€€ CheckNode returns an error on empty nodes (all zeros)
//		err = CheckNode(scanState->btcb, scanState->currentNode);
//		if (err != noErr) break;
		
		if (scanState->currentNode->type == kLeafNode)
			break;
	}
	
	return err;
}



//_________________________________________________________________________________
//
//	Routine:	ReadMultipleNodes
//
//	Purpose:	Read one or more nodes into the buffer.
//
//	Inputs:
//		scanState		Scanner's current state
//
//	Result:
//		noErr			One or nodes were read
//		eofErr			No nodes left in file, none in buffer
//_________________________________________________________________________________

OSErr ReadMultipleNodes(BTScanState *scanState)
{
	OSErr			err;
	UInt32			nodeSize;			// size of one B-Tree node
	UInt32			currentPosition;	// current offset in bytes from start of B-Tree file
	UInt32			bytesToRead;		// number of bytes left to read into buffer
	ExtendedVCB		*vcb;				// volume that B-Tree resides on
	UInt32			actualBytes;		// bytes actually read by CacheReadInPlace
	HIOParam		iopb;				// used by CacheReadInPlace

	#if ( FORDISKFIRSTAID )
		err = CheckForStop( ((SGlobPtr)GetDFAGlobals()) );						//	Permit the user to interrupt
		ReturnIfError( err );
	#endif

	nodeSize = scanState->btcb->nodeSize;
	vcb = GetFileControlBlock(scanState->btcb->fileRefNum)->fcbVPtr;
#ifdef INVESTIGATE
	iopb.ioRefNum 	= scanState->btcb->fileRefNum;
	iopb.ioPermssn	= fsRdPerm;
	iopb.ioBuffer 	= scanState->buffer;
	iopb.ioPosMode	= fsAtMark | (noCacheMask << 8);
	iopb.ioActCount	= 0;
#endif	
	currentPosition	= scanState->nodeNum * nodeSize;
	bytesToRead		= scanState->bufferSize;
	
	while (bytesToRead > 0) {
		err = CacheReadInPlace(vcb, &iopb, currentPosition, bytesToRead, &actualBytes);
		
		if (err != noErr || actualBytes == 0)
			break;
		bytesToRead		-= actualBytes;
#ifdef INVESTIGATE
		iopb.ioActCount	+= actualBytes;
#endif	
		currentPosition	+= actualBytes;
	}
	
#ifdef INVESTIGATE
	scanState->nodesLeftInBuffer = iopb.ioActCount / nodeSize;
	
	if (err == fxRangeErr && iopb.ioActCount == 0)
		err = eofErr;
	else
#endif	
		err = noErr;
		
	if (scanState->nodesLeftInBuffer == 0 && err == noErr)
		err = eofErr;
	
	return err;
}
