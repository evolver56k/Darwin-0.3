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
	File:		BTreeScanner.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(msd)	Mark Day

	Change History (most recent first):

	   <CS1>	 7/28/97	msd		first checked in
*/

#include <FileMgrInternal.h>
#include <HFSBTreesPriv.h>
#include "BTreesPrivate.h"


/*
	BTScanState - This structure is used to keep track of the current state
	of a BTree scan.  It contains both the dynamic state information (like
	the current node number and record number) and information that is static
	for the duration of a scan (such as buffer pointers).
	
	NOTE: recordNum may equal or exceed the number of records in the node
	number nodeNum.  If so, then the next attempt to get a record will move
	to a new node number.
*/
struct BTScanState {
	//	The following fields are set up once at initialization time.
	//	They are not changed during a scan.
	void *				buffer;
	UInt32				bufferSize;
	BTreeControlBlock *	btcb;
	
	//	The following fields are the dynamic state of the current scan.
	UInt32				nodeNum;			// zero is first node
	UInt32				recordNum;			// zero is first record
	BTNodeDescriptor *	currentNode;		// points to current node within buffer
	UInt32				nodesLeftInBuffer;	// number of valid nodes still in the buffer
	UInt32				recordsFound;		// number of leaf records seen so far
	UInt32				lastCachedNode;		// last node put in cache
};
typedef struct BTScanState BTScanState;


OSErr BTScanInitialize(
	const FCB *		btreeFile,
	UInt32			startingNode,
	UInt32			startingRecord,
	UInt32			recordsFound,
	void *			buffer,
	UInt32			bufferSize,
	BTScanState	*	scanState);

OSErr BTScanTerminate(
	const BTScanState *	scanState,
	UInt32 *			startingNode,
	UInt32 *			startingRecord,
	UInt32 *			recordsFound);

OSErr BTScanNextRecord(
	BTScanState *	scanState,
	Boolean			avoidIO,
	void * *		key,
	void * *		data,
	UInt32 *		dataSize);

OSErr BTScanCacheNode(BTScanState *scanState);
