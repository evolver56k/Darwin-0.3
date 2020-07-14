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
	File:		CatalogIterators.c

	Contains:	Catalog Iterator Implementation

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			Mac OS File System

	Writers:

		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	   <CS3>	11/13/97	djb		Radar #1683572 - Fix for indexed GetFileInfo.
	   <CS2>	10/17/97	msd		Bug 1683506. Add support for long Unicode names in
									CatalogIterators. Added a single global buffer for long Unicode
									names; it is used by at most one CatalogIterator at a time.
	   <CS1>	 10/1/97	djb		first checked in
*/

#if TARGET_OS_MAC
#if ( PRAGMA_LOAD_SUPPORTED )
	#pragma load PrecompiledHeaders
#else
	#include	<Errors.h>
	#include	<Files.h>
	#include	<Memory.h>
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
#include	"CatalogPrivate.h"

#include	"HFSInstrumentation.h"


static CatalogCacheGlobals* GetCatalogCacheGlobals( void );

static void	InsertCatalogIteratorAsMRU( CatalogCacheGlobals *cacheGlobals, CatalogIterator *iterator );

static void InsertCatalogIteratorAsLRU( CatalogCacheGlobals *cacheGlobals, CatalogIterator *iterator );

static void PrepareForLongName( CatalogIterator *iterator );


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	InitCatalogCache
//
//	Function: 	Allocates cache, and initializes all the cache structures.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr
InitCatalogCache( Ptr *catalogCache )
{
	CatalogCacheGlobals *	cacheGlobals;
	CatalogIterator *		iterator;
	UInt32					cacheSize;
	UInt16					i;
	UInt16					lastIterator;
	OSErr					err;


	cacheSize = sizeof(CatalogCacheGlobals) + ( kCatalogIteratorCount * sizeof(CatalogIterator) );
	cacheGlobals = (CatalogCacheGlobals *) NewPtrSysClear( cacheSize );

	err = MemError();
	if (err != noErr)
	{
		*catalogCache = nil;
		return err;
	}

	cacheGlobals->iteratorCount = kCatalogIteratorCount;

	lastIterator = kCatalogIteratorCount - 1;		//	last iterator number, since they start at 0
	
	//	Initialize the MRU order for the cache
	cacheGlobals->mru = (CatalogIterator *) ( (Ptr)cacheGlobals + sizeof(CatalogCacheGlobals) );

	//	Initialize the LRU order for the cache
	cacheGlobals->lru = (CatalogIterator *) ( (Ptr)(cacheGlobals->mru) + (lastIterator * sizeof(CatalogIterator)) );
	

	//	Traverse iterators, setting initial mru, lru, and default values
	for ( i = 0, iterator = cacheGlobals->mru; i < kCatalogIteratorCount ; i++, iterator = iterator->nextMRU )
	{
		if ( i == lastIterator )
			iterator->nextMRU = nil;	// terminate the list
		else
			iterator->nextMRU = (CatalogIterator *) ( (Ptr)iterator + sizeof(CatalogIterator) );

		if ( i == 0 )
			iterator->nextLRU = nil;	// terminate the list	
		else
			iterator->nextLRU = (CatalogIterator *) ( (Ptr)iterator - sizeof(CatalogIterator) );
	}
	
	*catalogCache = (Ptr) cacheGlobals;	//	return cacheGlobals to caller
	
	return noErr;
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	InvalidateCatalogCache
//
//	Function: 	Trash any interators matching volume parameter
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
InvalidateCatalogCache( ExtendedVCB *volume )
{
	TrashCatalogIterator( volume, 0 );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	TrashCatalogIterator
//
//	Function: 	Trash any interators matching volume and folder parameters
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
TrashCatalogIterator( const ExtendedVCB *volume, CatalogNodeID folderID )
{
	CatalogIterator		*iterator;
	CatalogCacheGlobals	*cacheGlobals = GetCatalogCacheGlobals();
	SInt16				volRefNum;


	volRefNum = volume->vcbVRefNum;

	for ( iterator = cacheGlobals->mru ; iterator != nil ; iterator = iterator->nextMRU )
	{
		top:
		// first match the volume
		if ( iterator->volRefNum != volRefNum )
			continue;

		// now match the folder (or all folders if 0)
		if ( (folderID == 0) || (folderID == iterator->folderID) )
		{
			CatalogIterator	*next;

			iterator->volRefNum = 0;	// trash it
			iterator->folderID = 0;

			next = iterator->nextMRU;	// remember the next iterator
			
			// if iterator is not already last then make it last
			if ( next != nil )
			{
				InsertCatalogIteratorAsLRU( cacheGlobals, iterator );
				
				// iterator->nextMRU will always be zero (since we moved it to the end)
				// so set up the next iterator manually (we know its not nil)
				iterator = next;	
				goto top;			// process the next iterator
			}
		}
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	AgeCatalogIterator
//
//	Function: 	Move iterator to the end of the list...
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
AgeCatalogIterator ( CatalogIterator *catalogIterator )
{
	InsertCatalogIteratorAsLRU( GetCatalogCacheGlobals(), catalogIterator );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetCatalogIterator
//
//	Function: 	Returns an iterator associated with the volume, folderID, index,
//				and iterationType (kIterateFilesOnly or kIterateAll).
//				Searches the cache in MRU order.
//				Inserts the resulting iterator at the head of mru automatically
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
CatalogIterator*
GetCatalogIterator( const ExtendedVCB *volume, CatalogNodeID folderID, UInt16 index, UInt16 iterationType)
{
	CatalogCacheGlobals *	cacheGlobals = GetCatalogCacheGlobals();
	CatalogIterator *		iterator;
	CatalogIterator *		bestIterator;
	UInt16					bestDelta;
	SInt16					volRefNum;
	Boolean					newIterator = false;


	LogStartTime(kGetCatalogIterator);	
	
	volRefNum = volume->vcbVRefNum;
	bestDelta = 0xFFFF;				// assume the best thing is to start from scratch
	bestIterator = nil;

	for ( iterator = cacheGlobals->mru ; iterator != nil ; iterator = iterator->nextMRU )
	{
		UInt16	delta;
		UInt16	iteratorIndex;

		// first make sure volume, folder id and type matches
		if ( (iterator->volRefNum != volRefNum) ||
			 (iterator->folderID != folderID)	||
			 (iterator->iteratorType != iterationType) )
		{
			continue;
		}
		
		// determine which index to match
		if ( iterationType == kIterateFilesOnly )
			iteratorIndex = iterator->fileIndex;
		else
			iteratorIndex = iterator->currentIndex;	
	
		// we matched volume, folder id and type, now check the index
		if ( iteratorIndex == index )
		{
			bestDelta = 0;
			bestIterator = iterator;	// we scored!
			break;						// so get out of this loop
		}

		// calculate how far this iterator is from the requested index
		if ( index > iteratorIndex )
			delta = index - iteratorIndex;
		else if ( iterationType == kIterateAll )
			delta = iteratorIndex - index;
		else
			delta = 0xFFFF; 	// for files iterator cannot be greater so skip this one

		
		// remember the best iterator so far (there could be more than one)
		if ( delta < bestDelta )
		{
			bestDelta = delta;			// we found a better one!
			bestIterator = iterator;	// so remember it
			if ( delta == 1 )
				break;					// just one away is good enough!
		}

	} // end for


	// check if we didn't get one or if the one we got is too far away...
	if ( (bestIterator == nil) || (index < bestDelta) )
	{
		bestIterator = cacheGlobals->lru;					// start over with a new iterator
		bestIterator->volRefNum = volRefNum;				// update the iterator's volume
		bestIterator->folderID = folderID;					// ... and folderID
		bestIterator->iteratorType = iterationType;			// ... and type
		bestIterator->currentIndex = 0;						// ... and offspring index marker
		bestIterator->fileIndex = 0xFFFF;					// ... and file index marker
		
		bestIterator->btreeNodeHint = 0;
		bestIterator->btreeIndexHint = 0;
		bestIterator->parentID = folderID;					// set key to folderID + empty name
		bestIterator->folderName.unicodeName.length = 0;	// clear pascal/unicode name

		if ( volume->vcbSigWord == kHFSPlusSigWord )
			bestIterator->nameType = kShortUnicodeName;
		else
			bestIterator->nameType = kShortPascalName;

		newIterator = true;
	}

	// put this iterator at the front of the list
	InsertCatalogIteratorAsMRU( cacheGlobals, bestIterator );
	
	LogEndTime(kGetCatalogIterator, newIterator);

	return bestIterator;	// return our best shot

} // end GetCatalogIterator


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	UpdateBtreeIterator
//
//	Function: 	Fills in a BTreeIterator from a CatalogIterator
//
// assumes catalogIterator->nameType is correctly initialized!
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
UpdateBtreeIterator(const CatalogIterator *catalogIterator, BTreeIterator *btreeIterator)
{
	CatalogName *	nodeName;
	Boolean			isHFSPlus;


	btreeIterator->hint.writeCount  = 0;
	btreeIterator->hint.nodeNum 	= catalogIterator->btreeNodeHint;
	btreeIterator->hint.index		= catalogIterator->btreeIndexHint;

	switch (catalogIterator->nameType)
	{
		case kShortPascalName:
			if ( catalogIterator->folderName.pascalName[0] > 0 )
				nodeName  = (CatalogName *) catalogIterator->folderName.pascalName;
			else
				nodeName = NULL;

			isHFSPlus = false;
			break;

		case kShortUnicodeName:
			if ( catalogIterator->folderName.unicodeName.length > 0 )
				nodeName  = (CatalogName *) &catalogIterator->folderName.unicodeName;
			else
				nodeName = NULL;

			isHFSPlus = true;
			break;

		case kLongUnicodeName:
			if ( catalogIterator->folderName.longNamePtr->length > 0 )
				nodeName  = (CatalogName *) catalogIterator->folderName.longNamePtr;
			else
				nodeName = NULL;

			isHFSPlus = true;
			break;

		default:
			return;
	}

	BuildCatalogKey(catalogIterator->parentID, nodeName, isHFSPlus, (CatalogKey*) &btreeIterator->key);
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	UpdateCatalogIterator
//
//	Function: 	Updates a CatalogIterator from a BTreeIterator
//
// assumes catalogIterator->nameType is correctly initialized!
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void
UpdateCatalogIterator (const BTreeIterator *btreeIterator, CatalogIterator *catalogIterator)
{
	void *			srcName;
	void *			dstName;
	UInt16			nameSize;
	CatalogKey *	catalogKey;


	catalogIterator->btreeNodeHint  = btreeIterator->hint.nodeNum;
	catalogIterator->btreeIndexHint = btreeIterator->hint.index;

	catalogKey = (CatalogKey*) &btreeIterator->key;

	switch (catalogIterator->nameType)
	{
		case kShortPascalName:
			catalogIterator->parentID = catalogKey->small.parentID;

			dstName  = catalogIterator->folderName.pascalName;
			srcName	 = catalogKey->small.nodeName;
			nameSize = catalogKey->small.nodeName[0] + sizeof(UInt8);
			break;

		case kShortUnicodeName:
			catalogIterator->parentID = catalogKey->large.parentID;

			dstName  = &catalogIterator->folderName.unicodeName;
			srcName  = &catalogKey->large.nodeName;
			nameSize = (catalogKey->large.nodeName.length + 1) * sizeof(UInt16);

			//	See if we need to make this iterator use long names
			if ( nameSize > sizeof(catalogIterator->folderName.unicodeName) )
			{
				PrepareForLongName(catalogIterator);		//	Find a long name buffer to use
				dstName  = catalogIterator->folderName.longNamePtr;
			}
			break;

		case kLongUnicodeName:
			catalogIterator->parentID = catalogKey->large.parentID;

			dstName  = catalogIterator->folderName.longNamePtr;
			srcName  = &catalogKey->large.nodeName;
			nameSize = (catalogKey->large.nodeName.length + 1) * sizeof(UInt16);
			break;

		default:
			return;
	}

	BlockMoveData(srcName, dstName, nameSize);

} // end UpdateCatalogIterator


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetCatalogCacheGlobals
//
//	Function: 	Get pointer to catalog cache globals
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
static CatalogCacheGlobals* 
GetCatalogCacheGlobals (void)
{
	FSVarsRec	*fsVars;

	fsVars = (FSVarsRec*) LMGetFSMVars();
	
	return (CatalogCacheGlobals*) fsVars->gCatalogCacheGlobals;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	InsertCatalogIteratorAsMRU
//
//	Function: 	Moves catalog iterator to head of mru order in double linked list
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
static void
InsertCatalogIteratorAsMRU ( CatalogCacheGlobals *cacheGlobals, CatalogIterator *iterator )
{
	CatalogIterator	*swapIterator;

	if ( cacheGlobals->mru != iterator )					//	if it's not already the mru iterator
	{
		swapIterator = cacheGlobals->mru;						//	put it in the front of the double queue
		cacheGlobals->mru = iterator;
		iterator->nextLRU->nextMRU = iterator->nextMRU;
		if ( iterator->nextMRU != nil )
			iterator->nextMRU->nextLRU = iterator->nextLRU;
		else
			cacheGlobals->lru= iterator->nextLRU;
		iterator->nextMRU	= swapIterator;
		iterator->nextLRU	= nil;
		swapIterator->nextLRU	= iterator;
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	InsertCatalogIteratorAsLRU
//
//	Function: 	Moves catalog iterator to head of lru order in double linked list
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
static void
InsertCatalogIteratorAsLRU ( CatalogCacheGlobals *cacheGlobals, CatalogIterator *iterator )
{
	CatalogIterator	*swapIterator;

	if ( cacheGlobals->lru != iterator )
	{
		swapIterator = cacheGlobals->lru;
		cacheGlobals->lru = iterator;
		iterator->nextMRU->nextLRU = iterator->nextLRU;
		if ( iterator->nextLRU != nil )
			iterator->nextLRU->nextMRU = iterator->nextMRU;
		else
			cacheGlobals->mru= iterator->nextMRU;
		iterator->nextLRU	= swapIterator;
		iterator->nextMRU	= nil;
		swapIterator->nextMRU	= iterator;
	}
}



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	PrepareForLongName
//
//	Function: 	Takes a CatalogIterator whose nameType is kShortUnicodeName, and
//				changes the nameType to kLongUnicodeName.
//
//  Since long Unicode names aren't stored in the CatalogIterator itself, we have
//	to point to a UniStr255 for storage.  In the current implementation, we have
//	just one such global buffer in the cache globals.  We'll set the iterator to
//	point to the global buffer and invalidate any other iterators that were using
//	it (i.e. any existing iterator whose nameType is kLongUnicodeName).
//
//	Eventually, we might want to have a list of long name buffers which we recycle
//	using an LRU algorithm.  Or perhaps, some other way....
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
static void
PrepareForLongName ( CatalogIterator *iterator )
{
	CatalogCacheGlobals	*cacheGlobals = GetCatalogCacheGlobals();
	CatalogIterator		*iter;
	CatalogIterator		*next;
	
	if (DEBUG_BUILD && iterator->nameType != kShortUnicodeName)
		DebugStr("\p PrepareForLongName: nameType is wrong!");
	
	//
	//	Walk through all the iterators.  Any iterator whose nameType is
	//	kLongUnicodeName is invalidated (because it is using the global
	//	long name buffer).
	//
	for ( iter = cacheGlobals->mru ; iter != nil ; iter = iter->nextMRU )
	{
		LOOP_TOP:
		
		if (iter->nameType == kLongUnicodeName)
		{
			iter->volRefNum = 0;	// trash it
			iter->folderID = 0;

			next = iter->nextMRU;	// remember the next iterator
			
			// if iterator is not already last then make it last
			if ( next != nil )
			{
				InsertCatalogIteratorAsLRU( cacheGlobals, iter );
				
				// iter->nextMRU will always be zero (since we moved it to the end)
				// so set up the next iterator manually (we know its not nil)
				iter = next;	
				goto LOOP_TOP;			// process the next iterator
			}
		}
	}
	
	//
	//	Change the nameType of this iterator and point to the global
	//	long name buffer.
	//
	iterator->nameType = kLongUnicodeName;
	iterator->folderName.longNamePtr = &cacheGlobals->longName;
}

#endif
