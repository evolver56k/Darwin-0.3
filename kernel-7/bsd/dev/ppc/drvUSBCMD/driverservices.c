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

// Get Prototypes of the Exported Services First,...

#include "driverservices.h"
#include "rhap_local.h"

// The Driver Services Follow,...

/////////////////////////////////////////////////////////////////////////////////
//
//	Queues
//
//

/*-------------------------------------------------------------------------------

Routine:	PBQueueInit	-	Initialize a Queue (@ NON-Interrupt-lvl and SIH-lvl)

-------------------------------------------------------------------------------*/

OSErr PBQueueInit(QHdrPtr qHeader)
{
	qHeader -> qHead = nil;
	qHeader -> qTail = nil;
	qHeader -> qFlags = 0x0000;
	
	return noErr;
}

/*-------------------------------------------------------------------------------

Routine:	PBQueueCreate	-	Initialize a Queue (@ NON-Interrupt lvl)

-------------------------------------------------------------------------------*/

OSErr PBQueueCreate(QHdrPtr *qHeader)
{
	Ptr **theQueue;
	
	if (CurrentExecutionLevel() != kTaskLevel)
	{
		// SysDebugStr ((StringPtr)"\pPBQueueCreate not called at Task Level!");
		// 11/30/98 Adam return -1;
	}
	
	*qHeader = nil;
	theQueue = (Ptr **)PoolAllocateResident (sizeof(QHdr)+sizeof(short)+sizeof(Ptr),true);
	if (theQueue != nil)
	{
		// Store orig ptr in first 4 bytes.
		*theQueue = (Ptr *)theQueue;
		theQueue++;
		
		// Are we not aligned?
		if ( ((UInt32)theQueue & 3) == 0 )
		{
			short *pattern = (short *)theQueue;
			
			*pattern++	= 0x4D4E;
			*qHeader	= (QHdrPtr)pattern;
			return noErr;
		}
		else
		{
			*qHeader	= (QHdrPtr)theQueue;
			return noErr;
		}
	}
	else
		return mFulErr;
}

/*-------------------------------------------------------------------------------

Routine:	PBQueueDelete	-	Finalize a Queue (@ NON-Interrupt lvl)

-------------------------------------------------------------------------------*/

OSErr PBQueueDelete(QHdrPtr qHeader)
{
	Ptr	  **theMemory;
	UInt16 *pattern = (UInt16 *)qHeader;
	
	if (CurrentExecutionLevel() != kTaskLevel)
	{
		// SysDebugStr ((StringPtr)"\pPBQueueDelete not called at Task Level!");
		return -1;
	}
	
	if (*--pattern == 0x4D4E)
		qHeader = (QHdrPtr)((UInt32)(qHeader) - sizeof(UInt16));
		
	theMemory = (Ptr **)qHeader;
	theMemory--;
	
	if (*theMemory != (Ptr *)theMemory)
	{
		// SysDebugStr ((StringPtr)"\pPBQueueDelete not a Queue Header!");
		return -1;
	}

	(void)PoolDeallocate(theMemory);
	return noErr;
}

/*-------------------------------------------------------------------------------

Routine:	PBEnqueueLast	-	Add an IOPB to the end of a Request Queue. (@ Any Exec-lvl)
								(Optimized for queue size of 1 or less)

-------------------------------------------------------------------------------*/

OSErr PBEnqueueLast(QElemPtr qElement, QHdrPtr qHeader)
{
	QElemPtr currElemPtr;
	UInt16 sr;
	
	// New element will be at end of queue.
	qElement -> qLink = nil;

	// Currently an empty queue?
	while ( qHeader -> qHead == nil )
	{
		if (CompareAndSwap ((UInt32) nil, (UInt32) qElement, (UInt32 *) &qHeader->qHead))
			return noErr;
	}
	
	// Multi-element queue,...
	
	// Do it the sure, but slow way.
	sr = Disable68kInterrupts();

	// Entire queue empty?
	if ( qHeader -> qHead == nil )
	{
		// Add at head of queue
		qHeader -> qHead = qElement;
	}
	else
	{
		// Else, Get first element in queue.
		currElemPtr	= (QElemPtr)qHeader->qHead;
		
		// Move toward the end of the queue,...
		while ( currElemPtr -> qLink != nil )
		{	
			currElemPtr	= currElemPtr -> qLink;
		}
	
		currElemPtr -> qLink = qElement;
	}
	
	Restore68kInterrupts(sr);
	return noErr;
}

/*-------------------------------------------------------------------------------

Routine:	PBEnqueue	-	Add an IOPB to Head of Request Queue. (@ Any Exec-lvl)

Function:	Returns (nothing)
			
Result:		void	
-------------------------------------------------------------------------------*/

void PBEnqueue(QElemPtr qElement, QHdrPtr qHeader)
{
	do {
		qElement -> qLink = qHeader->qHead;
	} while (CompareAndSwap ((UInt32) qElement->qLink, (UInt32) qElement,
											(UInt32 *) &qHeader->qHead) == false);
}


/*-------------------------------------------------------------------------------

Routine:	PBDequeue	-	Remove an IOPB from a Request Queue.  (@ Any Exec-lvl)

Function:	Returns (nothing)
			
Result:		void	
-------------------------------------------------------------------------------*/

static OSErr PBDequeueSync(QElemPtr qElement, QHdrPtr qHeader)
{
	QElemPtr prevElemPtr, currElemPtr;
	OSErr	 result = noErr;
	UInt16	sr;
	
	// Validate params.
	if ( (qElement == nil) || (qHeader == nil) )
		return paramErr;
		
	sr = Disable68kInterrupts();
	
	currElemPtr = (QElemPtr)qHeader->qHead;
	prevElemPtr = nil;
	
	// Find the one we want.
	while ( currElemPtr && (currElemPtr != qElement) )
	{
		// Advance the cause.
		prevElemPtr = currElemPtr;
		currElemPtr = currElemPtr -> qLink;
	}
	
	// We find it?
	if ( currElemPtr )
	{
		// Middle of the queue somewhere?
		if ( prevElemPtr )
			prevElemPtr -> qLink = currElemPtr ->qLink;
		else
			qHeader->qHead = currElemPtr ->qLink;
		
		currElemPtr -> qLink = nil;
	}
	else
		result = qErr; // not on the list.
	
	Restore68kInterrupts(sr);
	return result;
}


OSErr PBDequeue(QElemPtr qElement, QHdrPtr qHeader)
{
	// Optimize for common case of 1 element on the queue.
	
	// First one on the queue?
	while ( qHeader -> qHead == qElement )
	{
		// If the element is still at the head of the queue,... Remove it, and we are done.
		if (CompareAndSwap ((UInt32) qElement,
							(UInt32) qElement -> qLink,
							(UInt32 *) &qHeader->qHead) == true)
		{
			qElement -> qLink = nil;

			return noErr;
		}
	}
	
	// Multiple elements on the queue.
	return PBDequeueSync (qElement, qHeader);
}

/*-------------------------------------------------------------------------------

Routine:	PBDequeueFirst	-	Remove First Element on a Queue  (@ Any Exec-lvl)

Result:		noErr or qErr
-------------------------------------------------------------------------------*/

OSErr PBDequeueFirst(QHdrPtr qHeader, QElemPtr *theFirstqElem)
{
	QElemPtr qElement;

	do {
		// Get Queue Head Ptr
		qElement = qHeader -> qHead;
		
		// Empty Queue?
		if ( qElement == nil )
		{
			if ( theFirstqElem )
				*theFirstqElem = nil;
				
			return qErr;	
		}
		
		// If the element is still at the head of the queue,... Remove it, and we are done.
		if (CompareAndSwap ((UInt32) qElement,
							(UInt32) qElement -> qLink,
							(UInt32 *) &qHeader->qHead) == true)
		{
			if ( theFirstqElem )
				*theFirstqElem = qElement;
				
			qElement -> qLink = nil;

			return noErr;
		}
	} while (1);
}

/*-------------------------------------------------------------------------------

Routine:	PBDequeueLast	-	Remove Last Element on a Queue  (@ Any Exec-lvl)

Result:		noErr or qErr
-------------------------------------------------------------------------------*/

OSErr PBDequeueLast(QHdrPtr qHeader, QElemPtr *theLastqElem)
{
	QElemPtr currElemPtr, prevElemPtr;
	OSErr	result = noErr;
	UInt16 sr;
	
	if ( theLastqElem )
		*theLastqElem = nil;
		
	sr = Disable68kInterrupts();

	// Get Queue Head Ptr and our initial previous ptr
	if ((prevElemPtr = qHeader -> qHead) == nil)
		result = qErr; // q is empty
	else
	{
		// Get head of queue
		currElemPtr	= (QElemPtr)qHeader->qHead;
		
		// Move toward the end of the queue,...
		while ( currElemPtr -> qLink != nil )
		{	
			prevElemPtr	= currElemPtr;
			currElemPtr	= currElemPtr ->qLink;
		}
	
		// Remove last one from the queue.
		prevElemPtr->qLink = nil;
		
		// If last was also the first, nil out qheader.
		if ( currElemPtr == qHeader -> qHead )
			qHeader -> qHead = nil;
	
		// Return last element to caller.
		if ( theLastqElem )
			*theLastqElem = currElemPtr;
	}
	
	Restore68kInterrupts(sr);
	return result;
}
