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
	File:		SRepair.c

	Contains:	This file contains the Scavenger repair routines.
	
	Version:	xxx put version here xxx

	Written by:	Bill Bruffey

	Copyright:	© 1986, 1990, 1992,1994-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	 <HFS21>	12/18/97	DSH		2200185, FixOrphanedFiles wasn't repairing missing threads.
	 <HFS20>	 12/8/97	DSH		Use ZeroFileBlocks(), updated DelFThd(), and UpdBTM()
	 <HFS19>	 12/2/97	DSH		Better handling of out of memory errors
	 <HFS18>	 12/2/97	DSH		Clean up RebuildBTree to be called on multiple BTrees per
									repair.
	 <HFS17>	11/18/97	DSH		MarkVCBDirty(), after MountCheck and before Flush.
	 <HFS16>	11/18/97	DSH		Rewrote FixOrphanedExtent() and added ClearBlocks() to zero new
									BTree on disk before rebuilding.
	 <HFS15>	 11/4/97	DSH		SetDFAStage( UInt32 stage )
	 <HFS14>	10/30/97	DSH		Use BTFlushPath in UpdateHeader()
	 <HFS13>	10/21/97	DSH		Added RebuildBTree routines
	 <HFS12>	 10/6/97	DSH		ifdefed out code for BoB b2 build
	 <HFS11>	 10/6/97	DSH		Rebuild BTrees (first pass, error handling to follow)
	 <HFS10>	  9/5/97	DSH		Deleted bullet item
	  <HFS9>	  9/4/97	msd		Fix illegal structure casts.
	  <HFS8>	 8/20/97	DSH		Implement RepairReservedBTreeFields
	  <HFS7>	 8/18/97	DSH		Fix UpdateBTreeHeader
	  <HFS6>	 6/26/97	DSH		Eliminated unicode conversion dependency, we keep names in their
									native volume format type.
	  <HFS5>	 4/11/97	DSH		Use extended VCB fields catalogRefNum, and extentsRefNum.
	  <HFS4>	  4/4/97	djb		Get in sync with volume format changes.
	  <HFS3>	  4/1/97	DSH		Changing ClearMem to ClearMemory
		<HFS3>	 3/18/97	DSH		Added FixOrphanedFiles() to work with HFS+
		<HFS2>	 1/28/97	DSH		Calling FileSystem routines
		<HFS1>	 1/27/97	djb		first checked in

		 <3>	 8/28/96	djb		Get in sync with SCpp and Master Interfaces.
		 <2>	 2/15/95	djb		Remove _bogus_ check for invalid clump size! Cleaned up compile warnings.
		 <1>	 9/16/94	DJB		Fixing Radar bug #1166262 - added E_MissingCustomIcon case. Changed the
		 							name of FixLockedDirName to FixFinderFlags. Also now uses BlockMoveData.
									
				 6/15/93	HHS		Updated to version 7.2a1 -- added code to repair invalid clump sizes.

		 		  4/7/92	gs		Make changes to use major and minor repair queues.
		 		 3/31/92	PP		Add call to disappearing folder fix and missing directory record
									fix.

	Previous Modification History:
 
 	10 Jul 90	PK	Repair Order concept, support for deleting dangling file threads.
    28 Jun 90	PK	Converted to use prototypes.
    27 Jun 90   PK  Support for valence error repair.
	18 Jul 86	BB  New today.
*/


#include	"ScavDefs.h"
#include	"Prototypes.h"
#include	"BTreeScanner.h"

#include <string.h>
#include <stdlib.h>

enum {
	clearBlocks,
	addBitmapBit,
	deleteExtents,
};


//	internal routine prototypes

void	SetOffset (void *buffer, UInt16 btNodeSize, SInt16 recOffset, SInt16 vecOffset);
#define SetOffset(buffer,nodesize,offset,record)		(*(SInt16 *) ((Byte *) (buffer) + (nodesize) + (-2 * (record))) = (offset))
static	OSErr	UpdateBTreeHeader( SGlobPtr GPtr, short refNum);
static	OSErr	UpdBTM( SGlobPtr GPtr, short refNum);
static	OSErr	UpdateVolumeBitMap( SGlobPtr GPtr );
static	int		DoMinorOrders( SGlobPtr GPtr );
static	OSErr	UpdVal( SGlobPtr GPtr, RepairOrderPtr rP );
static	int		DelFThd( SGlobPtr GPtr, UInt32 fid );
static	OSErr	FixDirThread( SGlobPtr GPtr, UInt32 did );
static	OSErr	FixOrphanedFiles ( SGlobPtr GPtr );
static	OSErr	RepairReservedBTreeFields ( SGlobPtr GPtr );
static	OSErr	RebuildBTree( SGlobPtr GPtr, SInt16 refNum );
static	OSErr	FixFinderFlags( SGlobPtr GPtr, RepairOrderPtr p );
static	OSErr	FixLockedFileName( SGlobPtr GPtr, RepairOrderPtr p );
static	OSErr	FixOrphanedExtent( SGlobPtr GPtr );
static	OSErr	CreateAndOpenRepairBtree( SGlobPtr GPtr, SInt16 refNum, SInt16 *newBTreeRefNum );
static	OSErr	CreateMapNodes( UInt32 firstMapNode, UInt32	numberOfNewMapNodes, UInt16 nodeSize );
static	OSErr	ClearBlocks( SGlobPtr GPtr, FCB *fcb, UInt32 startBlock, UInt32 blockCount );
extern	OSErr	FindExtentRecord( const ExtendedVCB *vcb, UInt8 forkType, UInt32 fileID, UInt32 startBlock, Boolean allowPrevious, LargeExtentKey *foundKey, LargeExtentRecord foundData, UInt32 *foundHint);
extern	OSErr	DeleteExtentRecord( const ExtendedVCB *vcb, UInt8 forkType, UInt32 fileID, UInt32 startBlock );
OSErr	GetFCBExtentRecord( const ExtendedVCB *vcb, const FCB *fcb, LargeExtentRecord extents );
static	OSErr	DeleteFilesOverflowExtents( SGlobPtr GPtr, FCB *fcb  );
static	OSErr	ZeroFileBlocks( ExtendedVCB *vcb, FCB *fcb, UInt32 byteOffset, UInt32 numberOfBytes );


/*------------------------------------------------------------------------------

Routine:	MRepair - (Minor Repair)

Function:	Performs minor repair operations.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		MRepair	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

int MRepair( SGlobPtr GPtr )
{
	OSErr			err;
	OSErr			rebuildErr		= noErr;
	SInt16			repairLoops		= 1;							//	some routines force us to loop again, and we need a stop condition
	ExtendedVCB		*calculatedVCB	= GPtr->calculatedVCB;
	Boolean			isHFSPlus		= GPtr->isHFSPlus;
 
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt
	
	SetDFAStage( kRepairStage );

RebuildBtrees:	
	if ( rebuildErr == noErr )
	{
		if ( GPtr->EBTStat & S_RebuildBTree )
		{
			rebuildErr	= RebuildBTree( GPtr, kCalculatedExtentRefNum );
			GPtr->EBTStat &= ~S_RebuildBTree;
		}
		if ( GPtr->CBTStat & S_RebuildBTree )
		{
			rebuildErr	= RebuildBTree( GPtr, kCalculatedCatalogRefNum );
			GPtr->CBTStat &= ~S_RebuildBTree;
		}
		if ( GPtr->ABTStat & S_RebuildBTree )
		{
			rebuildErr	= RebuildBTree( GPtr, kCalculatedAttributesRefNum );
			GPtr->ABTStat &= ~S_RebuildBTree;
		}
	}
	
	//  Handle repair orders.  Note that these must be done *BEFORE* the MDB is updated.
	err = DoMinorOrders( GPtr );
	ReturnIfError( err );
 
	//
	//	we will update the following data structures regardless of whether we have done
	//	major or minor repairs, so we might end up doing this multiple times. Look into this.
	//

	//
	//	Update the MDB
	//
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt
				
	if ( (GPtr->VIStat & S_MDB) != 0 )								//	update MDB / VolumeHeader
	{
		MarkVCBDirty( calculatedVCB );
		err = FlushAlternateVolumeControlBlock( calculatedVCB, isHFSPlus );	//	Writes real & alt blocks
		ReturnIfError( err );
	}
	
	//
	//	Update the volume bit map
	//
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt

	if ( (GPtr->VIStat & S_VBM) != 0 )
	{
		err = UpdateVolumeBitMap( GPtr );							//	update VolumeBitMap
		ReturnIfError( err );
		InvalidateCalculatedVolumeBitMap( GPtr );					//	Invalidate our BitMap
	}
	
	//
	//	Update the extent BTree header and bit map
	//
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt

	if ( (GPtr->EBTStat & S_BTH) != 0 )
	{
		err = UpdateBTreeHeader( GPtr, kCalculatedExtentRefNum );	//	update extent file BTH
		ReturnIfError( err );
	}

	if ( (GPtr->EBTStat & S_BTM) != 0 )
	{
		err = UpdBTM( GPtr, kCalculatedExtentRefNum );				//	update extent file BTM
		ReturnIfError( err );
	}
	
	//
	//	FixOrphanedFiles
	//
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt
				
	if ( (GPtr->EBTStat & S_Orphan) != 0 )							//	update MDB / VolumeHeader
	{
		err = FixOrphanedFiles ( GPtr );
		ReturnIfError( err );
	}

	//
	//	FixOrphanedExtent records
	//
	if ( (GPtr->EBTStat & S_OrphanedExtent) != 0 )					//	Orphaned extents were found
	{
		err = FixOrphanedExtent( GPtr );
		GPtr->EBTStat &= ~S_OrphanedExtent;
		if ( err == errRebuildBtree )
			goto RebuildBtrees;
		ReturnIfError( err );
	}

	//
	//	If MountCheck found minor errors, let it fix them now, during the repair stage
	//	Note: SetDFAStage( kRepairStage ); was called above
	//
 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt
				
	if ( (GPtr->VIStat & S_MountCheckMinorErrors) != 0 )			//	MountCheck found minor errors
	{
		UInt32	consistencyStatus;
		err	= MountCheck( calculatedVCB, &consistencyStatus );
		ReturnIfError( err );
		MarkVCBDirty( calculatedVCB );
		err = FlushAlternateVolumeControlBlock( calculatedVCB, isHFSPlus );	//	Writes real & alt blocks
		DFAFlushCache();
		
		GPtr->VIStat &= ~S_MountCheckMinorErrors;

		//	Because the scanner and bitmap check work differently, if an orphaned extent goes undetected
		//	by MountCheck we wind up with UpdateVolumeBitMap() and MountCheck conflicting on what the bitmap
		//	should look like, so we will rebuild the extent btree to get rid of the orphan.
		if ( (consistencyStatus) && (repairLoops < 2) )
		{
			if ( GPtr->VIStat & S_VBM )
				GPtr->EBTStat |= S_RebuildBTree;						//	rebuild the extents tree

			if ( consistencyStatus & kHFSInvalidPEOF )
				GPtr->CBTStat |= S_RebuildBTree;
			
			repairLoops++;
			goto RebuildBtrees;
		}
	}

	//
	//	Update the catalog BTree header and bit map
	//

 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt

	if ( (GPtr->CBTStat & S_BTH) != 0 )
	{
		err = UpdateBTreeHeader( GPtr, kCalculatedCatalogRefNum );	//	update catalog BTH
		ReturnIfError( err );
	}

	if ( (GPtr->CBTStat & S_BTM) != 0 )
	{
		err = UpdBTM( GPtr, kCalculatedCatalogRefNum );				//	update catalog BTM
		ReturnIfError( err );
	}

	if ( (GPtr->CBTStat & S_ReservedNotZero) != 0 )
	{
		err = RepairReservedBTreeFields( GPtr );					//	update catalog fields
		ReturnIfError( err );
	}

	#if ( 0 )
		if ( (GPtr->CatStat & S_RName ) != 0)
		{
			err = CCMRenameCN(&(vcbP->vcb),kHFSRootFolderID,0,&vcbP->vcb.vcbVN[0],0,&hint);
			ReturnIfError( err );
		}
	#endif

 	err = CheckForStop( GPtr ); ReturnIfError( err );				//	Permit the user to interrupt
			
	DFAFlushCache();
	err = C_FlushCache( calculatedVCB, 0, calculatedVCB->vcbVRefNum );
	
	if ( err == noErr )
		err	= rebuildErr;
	
	return( err );													//	all done
}



		

//
//	Internal Routines
//
		

/*------------------------------------------------------------------------------

Routine:	UpdateBTreeHeader - (Update BTree Header)

Function:	Replaces a BTH on disk with info from a scavenger BTCB.
			
Input:		GPtr		-	pointer to scavenger global area
			refNum		-	file refnum

Output:		UpdateBTreeHeader	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

static	OSErr	UpdateBTreeHeader( SGlobPtr GPtr, short refNum )
{
	OSErr				err;
	FCB					*fcb;

	fcb = GetFileControlBlock( refNum );

	M_BTreeHeaderDirty( ((BTreeControlBlockPtr) fcb->fcbBTCBPtr) );	
	err	= BTFlushPath( fcb );

	return( err );
	
}	//	End UpdateBTreeHeader


		

/*------------------------------------------------------------------------------

Routine:	UpdBTM - (Update BTree Map)

Function:	Replaces a BTM on disk with a scavenger BTM.
			
Input:		GPtr		-	pointer to scavenger global area
			refNum		-	file refnum

Output:		UpdBTM	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

static	OSErr	UpdBTM( SGlobPtr GPtr, short refNum )
{
	OSErr				err;
	UInt16				recSize;
	SInt16				mapSize;
	SInt16				size;
	SInt16				recIndx;
	Ptr					p;
	Ptr					btmP;
	Ptr					sbtmP;
	UInt32				nodeNum;
	FCB					*fcb;
	BTreeControlBlock	*calculatedBTCB;
	NodeRec				node;
	UInt32				fLink;

	//	set up
	fcb				= GetFileControlBlock( refNum );
	calculatedBTCB	= (BTreeControlBlock *)fcb->fcbBTCBPtr;
	mapSize			= ((BTreeExtensionsRec*)calculatedBTCB->refCon)->BTCBMSize;

	//
	//	update the map records
	//
	if ( mapSize > 0 )
	{
		nodeNum	= 0;
		recIndx	= 2;
		sbtmP	= ((BTreeExtensionsRec*)calculatedBTCB->refCon)->BTCBMPtr;
		
		do
		{
			GPtr->TarBlock = nodeNum;								//	set target node number
				
			err = GetNode( calculatedBTCB, nodeNum, &node );
			ReturnIfError( err );									//	could't get map node
	
			//	Locate the map record
			recSize = GetRecordSize( calculatedBTCB, (BTNodeDescriptor *)node.buffer, recIndx );
			btmP = (Ptr)GetRecordAddress( calculatedBTCB, (BTNodeDescriptor *)node.buffer, recIndx );
			
			fLink = ((NodeDescPtr)node.buffer)->fLink;
		
			size = recSize;
			if ( size  > mapSize )
				size = mapSize;
				
			BlockMoveData( sbtmP, btmP, size );						//	update it
			
			err = UpdateNode( calculatedBTCB, &node );				//	write it, and unlock buffer
			
			mapSize	-= size;										//	move to next map record
			if ( mapSize == 0 )										//	more to go?
				break;												//	no, zero remainder of record
			if ( fLink == 0 )										//	out of bitmap blocks in file?
			{
				RcdError( GPtr, E_ShortBTM );
				return( E_ShortBTM );
			}
				
			nodeNum	= fLink;
			sbtmP	+= size;
			recIndx	= 0;
			
		} while ( mapSize > 0 );

		//	clear the unused portion of the map record
		for ( p = btmP + size ; p < btmP + recSize; p++ )
			*p = 0; 

		err = UpdateNode( calculatedBTCB, &node );					//	Write it, and unlock buffer
	}
	
	return( noErr );												//	All done
	
}	//	end UpdBTM


		

/*------------------------------------------------------------------------------

Routine:	UpdateVolumeBitMap - (Update Volume Bit Map)

Function:	Replaces the VBM on disk with the scavenger VBM.
			
Input:		GPtr			-	pointer to scavenger global area

Output:		UpdateVolumeBitMap			- 	function result:
									0 = no error
									n = error
			GPtr->VIStat	-	S_VBM flag set if VBM is damaged.
------------------------------------------------------------------------------*/

static	OSErr	UpdateVolumeBitMap( SGlobPtr GPtr )
{
	OSErr				err;
	Ptr					vbmBlockP;
	SInt32				bufferNumber;
	UInt32				startBlock;
	UInt32				blockCount;
	UInt32				i;
	VolumeBitMapHeader	*volumeBitMap	= GPtr->volumeBitMapPtr;
	Boolean				isHFSPlus		= GPtr->isHFSPlus;

	//	set up

	GPtr->TarID = VBM_FNum;							//	target file = volume bit map
	
	for ( bufferNumber=0 ; bufferNumber<volumeBitMap->numberOfBuffers ; bufferNumber++ )
	{
		BitMapRec	*bufferInfo	= &(volumeBitMap->bitMapRecord[bufferNumber]);
		
		//	This code must cycle through the BitMap buffers and if they have already been processed:
		
		//	if this buffer is completely empty
		if ( (bufferInfo->processed) && (bufferInfo->count == 0) )
		{
			ClearMemory ( volumeBitMap->buffer, volumeBitMap->bufferSize );		//	start with an empty bitmap buffer
		}
		//	if this buffer is completely full
		else if ( (bufferInfo->processed) && (bufferInfo->count ==  volumeBitMap->bufferSize * 8) )
		{
			memset( volumeBitMap->buffer, 0xFF, volumeBitMap->bufferSize );
		}
		//	or else we have to recreate the bitmap buffer
		else
		{
			err = CreateVolumeBitMapBuffer( GPtr, bufferNumber );
			ReturnIfError( err );
		}
		
		//
		//	Now replace the existing BitMap file with our recreated file
		//
		if ( isHFSPlus )
		{
			//	First make sure the allocations file is the correct size
			if ( GPtr->calculatedAllocationsFCB->fcbPLen < volumeBitMap->bitMapSizeInBytes ) //	Volume bitmap must be at least this big
			{
				M_DebugStr("\p Allocations file size does not match bitMapSizeInBytes");
				return( 123 );
				//€€	Call ExtendFile() here
			}
		}

		startBlock	= bufferNumber * ( volumeBitMap->bufferSize / Blk_Size );	//	whenever whichBuffer is non zero it's a multiple of 512.

		if ( bufferNumber == volumeBitMap->numberOfBuffers-1 )
			blockCount	= ( ( volumeBitMap->bitMapSizeInBytes - (bufferNumber * volumeBitMap->bufferSize) + Blk_Size - 1 ) / Blk_Size );		//	last block may not be 512 byte alligned.
		else
			blockCount	= volumeBitMap->bufferSize / Blk_Size;
		
		for ( i=0 ; i<blockCount ; i++ )
		{
			#if ( ! StandAloneEngine )
				SpinCursor( 4 ); 												//	rotate cursor
			#endif

			GPtr->TarBlock = startBlock+i;										//	set target block number
			
			if ( isHFSPlus )
			{
				err = GetBlock_FSGlue( gbReleaseMask, startBlock+i, (Ptr *)&vbmBlockP, kCalculatedAllocationsRefNum, GPtr->calculatedVCB );
				ReturnIfError( err );
			}
			else
			{
				err = GetVBlk( GPtr, startBlock + GPtr->calculatedVCB->vcbVBMSt + i, (void**)&vbmBlockP );	//	get map block from disk
				ReturnIfError( err );
			}
			
			BlockMoveData( (Ptr)(volumeBitMap->buffer + i*Blk_Size), vbmBlockP, Blk_Size );
			RelBlock_FSGlue( vbmBlockP, rbWriteMask );
		}
	}
	
	return( noErr );
}


/*------------------------------------------------------------------------------

Routine:	DoMinorOrders

Function:	Execute minor repair orders.

Input:		GPtr	- ptr to scavenger global data

Outut:		function result:
				0 - no error
				n - error
------------------------------------------------------------------------------*/

static	int	DoMinorOrders( SGlobPtr GPtr )				//	the globals
{
	RepairOrderPtr		p;
	OSErr				err	= noErr;					//	initialize to "no error"
	
	while( (p = GPtr->MinorRepairsP) && (err == noErr) )	//	loop over each repair order
	{
		GPtr->MinorRepairsP = p->link;					//	unlink next from list
		
		switch( p->type )								//	branch on repair type
		{
			case E_RtDirCnt:							//	the valence errors
			case E_RtFilCnt:							//	(of which there are several)
			case E_DirCnt:
			case E_FilCnt:
			case E_DirVal:
				err = UpdVal( GPtr, p );				//	handle a valence error
				break;
			
			case E_LockedDirName:
			case E_MissingCustomIcon:
				err = FixFinderFlags( GPtr, p );
				break;
			
			case E_NoFile:								//	dangling file thread
				err = DelFThd( GPtr, p->parid );		//	delete the dangling thread
				break;

			//€€	E_NoFile case is never hit since VLockedChk() registers the error, 
			//€€	and returns the error causing the verification to quit.
			case E_EntryNotFound:
				GPtr->EBTStat |= S_OrphanedExtent;
				break;

			//€€	Same with E_NoDir
			case E_NoDir:								//	missing directory record
				err = FixDirThread( GPtr, p->parid );	//	fix the directory thread record
				break;
			
			#if ( 0	)
			case E_InvalidClumpSize:
				err = FixClumpSize( GPtr, p );
				break;
			#endif
				
			default:									//	unknown repair type
				err = IntError( GPtr, R_IntErr );		//	treat as an internal error
				break;
		}
		
		DisposePtr( (Ptr) p );							//	free the node
	}
	
	return( err );										//	return error code to our caller
}


/*------------------------------------------------------------------------------

Routine:	DelFThd - (delete file thread)

Function:	Executes the delete dangling file thread repair orders.  These are typically
			threads left after system 6 deletes an aliased file, since system 6 is not
			aware of aliases and thus will not delete the thread along with the file.

Input:		GPtr	- global data
			fid		- the thread record's key's parent-ID

Output:		0 - no error
			n - deletion failed
Modification History:
	29Oct90		KST		CBTDelete was using "k" as key which points to cache buffer.
-------------------------------------------------------------------------------*/

static	int	DelFThd( SGlobPtr GPtr, UInt32 fid )				//	the file ID
{
	CatalogRecord		record;
	CatalogKey			foundKey;
	CatalogKey			key;
	UInt32				hint;								//	as returned by CBTSearch
	OSErr				result;								//	status return
	UInt16				recSize;
	ExtentRecord		zeroExtents;
	
	BuildCatalogKey( fid, (const CatalogName*) nil, GPtr->isHFSPlus, &key );
	result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recSize, &hint );
	
	if ( result )	return ( IntError( GPtr, result ) );
	
	if ( (record.recordType != kSmallFileThreadRecord) && (record.recordType != kLargeFileThreadRecord) )	//	quit if not a file thread
		return ( IntError( GPtr, R_IntErr ) );
	
	//	Zero the record on disk
	ClearMemory( (Ptr)&zeroExtents, sizeof(ExtentRecord) );
	result	= ReplaceBTreeRecord( kCalculatedCatalogRefNum, &key, hint, &zeroExtents, recSize, &hint );
	if ( result )	return ( IntError( GPtr, result ) );
	
	result	= DeleteBTreeRecord( kCalculatedCatalogRefNum, &key );
	if ( result )	return ( IntError( GPtr, result ) );
	
	//	After deleting a record, we'll need to write back the BT header and map,
	//	to reflect the updated record count etc.
	   
	GPtr->CBTStat |= S_BTH + S_BTM;							//	set flags to write back hdr and map

	return( noErr );										//	successful return
}
	

/*------------------------------------------------------------------------------

Routine:	FixDirThread - (fix directory thread record's parent ID info)

Function:	Executes the missing directory record repair orders most likely caused by 
			disappearing folder bug.  This bug causes some folders to jump to Desktop 
			from the root window.  The catalog directory record for such a folder has 
			the Desktop folder as the parent but its thread record still the root 
			directory as its parent.

Input:		GPtr	- global data
			did		- the thread record's key's parent-ID

Output:		0 - no error
			n - deletion failed
-------------------------------------------------------------------------------*/

static	OSErr	FixDirThread( SGlobPtr GPtr, UInt32 did )	//	the dir ID
{
	FCB					*fcb;						//	the catalog btree fcb
	UInt8				*dataPtr;
	UInt32				hint;							//	as returned by CBTSearch
	OSErr				result;							//	status return
	UInt16				recSize;
	CatalogName			catalogName;					//	temporary name record
	CatalogName			*keyName;						//	temporary name record
	register short 		index;							//	loop index for all records in the node
	UInt32  			curLeafNode;					//	current leaf node being checked
	UInt32				newParDirID	= 0;				//	the parent ID where the dir record is really located
	CatalogRecord		record;
	CatalogKey			foundKey;
	CatalogKey			key;
	CatalogKey		 	*keyP;
	SInt16				recordType;
	UInt32				folderID;
	Boolean				isHFSPlus	= GPtr->isHFSPlus;
	BTreeControlBlock	*calculatedBTCB;
	NodeRec				node;
	NodeDescPtr			nodeDescP;
	
	
	fcb	= GetFileControlBlock( kCalculatedCatalogRefNum );
	calculatedBTCB = (BTreeControlBlock *)fcb->fcbBTCBPtr;

	BuildCatalogKey( did, (const CatalogName*) nil, isHFSPlus, &key );
	result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recSize, &hint );
	
	if ( result )
		return( IntError( GPtr, result ) );
	if ( (record.recordType != kSmallFolderThreadRecord) && (record.recordType != kLargeFolderThreadRecord) )			//	quit if not a directory thread
		return ( IntError( GPtr, R_IntErr ) );
		
	curLeafNode = ( (BTreeControlBlock *)fcb->fcbBTCBPtr)->freeNodes;
	
	while ( curLeafNode )
	{
		result = GetNode( calculatedBTCB, curLeafNode, &node );
		if ( result != noErr ) return( IntError( GPtr, result ) );
		
		nodeDescP = node.buffer;

		// loop on number of records in node
		for ( index = 0 ; index < nodeDescP->numRecords ; index++ )
		{
			GetRecordByIndex( (BTreeControlBlock *)fcb->fcbBTCBPtr, (NodeDescPtr)nodeDescP, index, (BTreeKey **)&keyP, &dataPtr, &recSize );
			
			recordType	= ((CatalogRecord *)dataPtr)->recordType;
			folderID	= recordType == kLargeFolderRecord ? ((LargeCatalogFolder *)dataPtr)->folderID : ((SmallCatalogFolder *)dataPtr)->folderID;
			
			// did we locate a directory record whode dirID match the the thread's key's parent dir ID?
			if ( (folderID == did) && ( recordType == kLargeFolderRecord || recordType == kSmallFolderRecord ) )
			{
				newParDirID	= recordType == kLargeFolderRecord ? keyP->large.parentID : keyP->small.parentID;
				keyName		= recordType == kLargeFolderRecord ? (CatalogName *)&keyP->large.nodeName : (CatalogName *)&keyP->small.nodeName;
				CopyCatalogName( keyName, &catalogName, isHFSPlus );
				break;
			}
		}

		if ( newParDirID )
			break;

		curLeafNode = nodeDescP->fLink;	 // sibling of this leaf node
	}
		
	if ( newParDirID == 0 )
	{
		return ( IntError( GPtr, R_IntErr ) ); // €€  Try fixing by creating a new directory record?
	}
	else
	{
		(void) SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recSize, &hint );

		if ( isHFSPlus )
		{
			LargeCatalogThread	*largeCatalogThreadP	= (LargeCatalogThread *) &record;
			
			largeCatalogThreadP->parentID = newParDirID;
			CopyCatalogName( &catalogName, (CatalogName *) &largeCatalogThreadP->nodeName, isHFSPlus );
		}
		else
		{
			SmallCatalogThread	*smallCatalogThreadP	= (SmallCatalogThread *) &record;
			
			smallCatalogThreadP->parentID = newParDirID;
			CopyCatalogName( &catalogName, (CatalogName *)&smallCatalogThreadP->nodeName, isHFSPlus );
		}
		
		result = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recSize, &hint );
	}

	return( noErr );										//	successful return
}
	

/*------------------------------------------------------------------------------

Routine:	UpdVal - (Update Valence)

Function:	Replaces out of date valences with correct vals computed during scavenge.
			
Input:		GPtr			-	pointer to scavenger global area
			p				- 	pointer to the repair order

Output:		UpdVal			- 	function result:
									0 = no error
									n = error
------------------------------------------------------------------------------*/

static	OSErr	UpdVal( SGlobPtr GPtr, RepairOrderPtr p )					//	the valence repair order
{
	OSErr				result;						//	status return
	UInt32				hint;						//	as returned by CBTSearch
	UInt16				recSize;
	CatalogRecord		record;
	CatalogKey			foundKey;
	CatalogKey			key;
	ExtendedVCB			*calculatedVCB = GPtr->calculatedVCB;
	
	switch( p->type )													//	branch on error type
	{
		case E_RtDirCnt:												//	invalid count of Dirs in Root
			if ( p->incorrect != calculatedVCB->vcbNmRtDirs )			//	do we know what we're doing?
				return ( IntError( GPtr, R_IntErr ) );					//	no, crump
			calculatedVCB->vcbNmRtDirs = p->correct;					//	yes, update
			GPtr->VIStat |= S_MDB;										//	make sure we update the MDB
			break;
			
		case E_RtFilCnt:												//	invalid count of Files in Root
			if ( p->incorrect != calculatedVCB->vcbNmFls )				//	do we know what we're doing?
				return ( IntError( GPtr, R_IntErr ) );					//	no, crump
			calculatedVCB->vcbNmFls = p->correct;						//	yes, update
			GPtr->VIStat |= S_MDB;										//	make sure we update the MDB
			break;
			
		case E_DirCnt:													//	invalid count of Dirs in volume
			if ( p->incorrect != calculatedVCB->vcbDirCnt )				//	do we know what we're doing?
				return ( IntError( GPtr, R_IntErr ) );					//	no, crump
			calculatedVCB->vcbDirCnt = p->correct;						//	yes, update
			GPtr->VIStat |= S_MDB;										//	make sure we update the MDB
			break;
			
		case E_FilCnt:													//	invalid count of FIles in volume
			if ( p->incorrect != calculatedVCB->vcbFilCnt )				//	do we know what we're doing?
				return ( IntError( GPtr, R_IntErr ) );					//	no, crump
			calculatedVCB->vcbFilCnt = p->correct;						//	yes, update
			GPtr->VIStat |= S_MDB;										//	make sure we update the MDB
			break;
	
		case E_DirVal:													//	invalid interior directory valence
//			CopyCatalogName( (const CatalogName *) &p->name, (CatalogName*)&catalogName, GPtr->isHFSPlus );
//			PrepareInputName( p->name, GPtr->isHFSPlus, &catalogName );
			BuildCatalogKey( p->parid, (CatalogName *)&p->name, GPtr->isHFSPlus, &key );
			result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recSize, &hint );
			if ( result )
				return ( IntError( GPtr, result ) );
				
			if ( record.recordType == kLargeFolderRecord )
			{
				if ( p->incorrect != record.largeFolder.valence)	//	do we know what we're doing?
					return ( IntError( GPtr, R_IntErr ) );
				record.largeFolder.valence = p->correct;			//	yes, update
			}
			else
			{
				if ( p->incorrect != record.smallFolder.valence )	//	do we know what we're doing?
					return ( IntError( GPtr, R_IntErr ) );
				record.smallFolder.valence = p->correct;			//	yes, update
			}
				
			result = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &key, hint, &record, recSize, &hint );
			if ( result )
				return ( IntError( GPtr, result ) );
			break;
	}
		
	return( noErr );														//	no error
}



#if ( 0 )
	// HHS added FixClumpSize
	/*------------------------------------------------------------------------------
	
	Routine:	FixClumpSize
	
	Function:	Replaces invalid file clump sizes with a tolerable value.
				
	Input:		GPtr			-	pointer to scavenger global area
				p				- 	pointer to the repair order
	
	Output:		FixClumpSize	- 	function result:
										0 = no error
										n = error
	------------------------------------------------------------------------------*/
	
	static	int	FixClumpSize(
		SGlobPtr		GPtr,							/* the globals */
		RepairOrderPtr	p)								/* the valence repair order */
	{
		FCB					*f;								/* the catalog btree fcb */
		SVCBPtr				v = GPtr->VCBPtr;				/* our internal copy of VCB */
		SmallCatalogFolder	*d;								/* ptr to btree directory record */
		SmallCatalogKey		*k;								/* ptr to btree key record */
		SmallCatalogKey		cKey;							/* key for search of catalog btree */
		UInt32				hint;							/* as returned by CBTSearch */
		OSErr				result;							/* status return */
		UInt16				recSize;
		fil					*filP;	
		
		f = (FCB*) (GPtr->FCBAPtr + kCalculatedCatalogRefNum);		/* generate ptr to cat fcb */
		
		CBldCKey(p->parid,p->name,&cKey);				/* build key to the directory in error */
		result = CBTSearch(f,&cKey,0,&k,&d,&recSize,&hint);	/* fetch the record */
		filP = (fil*) d;
		
		if ( result )										/* crump on error */
			return( IntError( GPtr, result ) );
		if ( p->incorrect != filP->filClpSize )			/* do we know what we're doing? */
			return ( IntError( GPtr, R_IntErr ) );			/* no, crump */
		filP->filClpSize = p->correct;					/* yes, update */
		result = CBTUpdate( f, hint );						/* write the node back to the file */
		if ( result )										/* crump on error */
			return ( IntError( GPtr, result ) );
			
		return	0;										/* no error */
	}
#endif

/*------------------------------------------------------------------------------

Routine:	FixFinderFlags

Function:	Changes some of the Finder flag bits for directories.
			
Input:		GPtr			-	pointer to scavenger global area
			p				- 	pointer to the repair order

Output:		FixFinderFlags	- 	function result:
									0 = no error
									n = error
------------------------------------------------------------------------------*/

static	OSErr	FixFinderFlags( SGlobPtr GPtr, RepairOrderPtr p )				//	the repair order
{
	CatalogRecord		record;
	CatalogKey			foundKey;
	CatalogKey			key;
	UInt32				hint;												//	as returned by CBTSearch
	OSErr				result;												//	status return
	UInt16				recSize;
	UInt32				timeStamp;
	
	BuildCatalogKey( p->parid, (CatalogName *)&p->name, GPtr->isHFSPlus, &key );

	result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recSize, &hint );
	if ( result )
		return ( IntError( GPtr, result ) );

	GetDateTime( &timeStamp );					// also update the modify date! -DJB

	if ( record.recordType == kLargeFolderRecord )
	{
		LargeCatalogFolder	*largeCatalogFolderP	= (LargeCatalogFolder *) &record;	
		if ( p->incorrect != largeCatalogFolderP->userInfo.frFlags )		//	do we know what we're doing?
		{
			//	Another repar order may have affected the flags
			if ( p->correct < p->incorrect )
				largeCatalogFolderP->userInfo.frFlags &= ~((UInt16)p->maskBit);
			else
				largeCatalogFolderP->userInfo.frFlags |= (UInt16)p->maskBit;
			//return ( IntError( GPtr, R_IntErr ) );
		}
		else
		{
			largeCatalogFolderP->userInfo.frFlags = p->correct;					//	yes, update
		}
		largeCatalogFolderP->contentModDate		= timeStamp;
	}
	else
	{
		SmallCatalogFolder	*smallCatalogFolderP	= (SmallCatalogFolder *) &record;	
		if ( p->incorrect != smallCatalogFolderP->userInfo.frFlags )		//	do we know what we're doing?
		{
			//	Another repar order may have affected the flags
			if ( p->correct < p->incorrect )
				smallCatalogFolderP->userInfo.frFlags &= ~((UInt16)p->maskBit);
			else
				smallCatalogFolderP->userInfo.frFlags |= (UInt16)p->maskBit;
			//return ( IntError( GPtr, R_IntErr ) );
		}
		else
		{
			smallCatalogFolderP->userInfo.frFlags = p->correct;					//	yes, update
		}
		
		smallCatalogFolderP->modifyDate = timeStamp;						// also update the modify date! -DJB
	}

	result = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recSize, &hint );	//	write the node back to the file
	if ( result )
		return( IntError( GPtr, result ) );
		
	return( noErr );														//	no error
}




struct OrphanRepairOrder
{
	struct RepairOrder	*link;			//	link to next node, or NULL
	OSErr				type;			//	type of error, as an error code (E_DirVal etc)
	short				n;				//	temp
	UInt32				correct;		//	correct valence
	UInt32				incorrect;		//	valence as found in volume (for consistency chk)
	UInt32				parid;			//	parent ID
	SmallExtentKey		theKey;			// the key to delete
};
typedef struct OrphanRepairOrder OrphanRepairOrder;


//
//	Entries in the extents BTree which do not have a corresponding catalog entry get fixed here
//	This routine will run slowly if the extents file is large because we require a Catalog BTree
//	search for each extent record.
//
static	OSErr	FixOrphanedExtent( SGlobPtr GPtr )
{
	OSErr				err;
	UInt32				hint;
	UInt32				recordSize;
	UInt32				maxRecords;
	UInt32				numberOfFilesInList;
	ExtentKey			*extentKeyPtr;
	ExtentRecord		*extentDataPtr;
	ExtentRecord		extents;
	ExtentRecord		zeroExtents;
	CatalogKey			foundExtentKey;
	CatalogRecord		catalogData;
	CatalogRecord		threadData;
	CatalogNodeID		fileID;
	BTScanState			scanState;

	CatalogNodeID		lastFileID			= -1;
	UInt32				recordsFound		= 0;
	Boolean				mustRebuildBTree	= false;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;
	ExtendedVCB			*calculatedVCB		= GPtr->calculatedVCB;
	UInt32				**dataHandle		= GPtr->validFilesList;
	FCB					*fcb				= GetFileControlBlock( kCalculatedExtentRefNum );
	FSVarsRec			*fsVars				= (FSVarsRec *) LMGetFSMVars();

	//	Set Up
	//
	//	Use the BTree scanner since we use MountCheck to find orphanded extents, and MountCheck uses the scanner
	err = BTScanInitialize( fcb, 0, 0, 0, fsVars->gAttributesBuffer, fsVars->gAttributesBufferSize, &scanState );
	if ( err != noErr )	return( badMDBErr );

	ClearMemory( (Ptr)&zeroExtents, sizeof(ExtentRecord) );

	if ( isHFSPlus )
	{
		maxRecords	= fcb->fcbEOF / sizeof(LargeExtentRecord);
	}
	else
	{
		maxRecords	= fcb->fcbEOF / sizeof(SmallExtentRecord);
		numberOfFilesInList	= GetHandleSize((Handle) dataHandle) / sizeof(UInt32);
		qsort( *dataHandle, numberOfFilesInList, sizeof (UInt32), cmpLongs );			//	Sort the list of found file IDs
	}


	while ( recordsFound < maxRecords )
	{
		err = BTScanNextRecord( &scanState, false, (void **) &extentKeyPtr, (void **) &extentDataPtr, &recordSize );

		if ( err != noErr )
		{
			if ( err == btNotFound )
				err	= noErr;
			break;
		}

		++recordsFound;
		fileID = (isHFSPlus == true) ? extentKeyPtr->large.fileID : extentKeyPtr->small.fileID;
		
		if ( (fileID > kHFSBadBlockFileID) && (lastFileID != fileID) )	// Keep us out of reserved file trouble
		{
			lastFileID	= fileID;
			
			if ( isHFSPlus )
			{
				err = LocateCatalogThread( calculatedVCB, fileID, &threadData, (UInt16*)&recordSize, &hint );	//	This call returns nodeName as either Str31 or UniStr255, no need to call PrepareInputName()
				
				if ( err == noErr )									//	Thread is found, just verify actual record exists.
				{
					err = LocateCatalogNode( calculatedVCB, threadData.largeThread.parentID, (const CatalogName *) &(threadData.largeThread.nodeName), kNoHint, &foundExtentKey, &catalogData, &hint );
				}
				else if ( err == cmNotFound )
				{
					err = SearchBTreeRecord( kCalculatedExtentRefNum, extentKeyPtr, kNoHint, &foundExtentKey, &extents, (UInt16*)&recordSize, &hint );
					if ( err == noErr )
					{	//€€ can't we just delete btree record?
						err = ReplaceBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey, hint, &zeroExtents, recordSize, &hint );
						err	= DeleteBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey );	//	Delete the orphaned extent
					}
				}
					
				if ( err != noErr )
					mustRebuildBTree	= true;						//	if we have errors here we should rebuild the extents btree
			}
			else
			{
				if ( ! bsearch( &fileID, *dataHandle, numberOfFilesInList, sizeof(UInt32), cmpLongs ) )
				{
					err = SearchBTreeRecord( kCalculatedExtentRefNum, extentKeyPtr, kNoHint, &foundExtentKey, &extents, (UInt16*)&recordSize, &hint );
					if ( err == noErr )
					{	//€€ can't we just delete btree record?
						err = ReplaceBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey, hint, &zeroExtents, recordSize, &hint );
						err = DeleteBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey );	//	Delete the orphaned extent
					}
					
					if ( err != noErr )
						mustRebuildBTree	= true;						//	if we have errors here we should rebuild the extents btree
				}
			}
		}
	}

	if ( mustRebuildBTree == true )
	{
		GPtr->EBTStat |= S_RebuildBTree;
		err	= errRebuildBtree;
	}

	return( err );
}


//
//	File records, which have the kFileThreadExistsMask set, but do not have a corresponding
//	thread, or threads which do not have corresponding records get fixed here.
//
static	OSErr	FixOrphanedFiles ( SGlobPtr GPtr )
{
	CatalogKey			key;
	CatalogKey			foundKey;
	CatalogKey			threadKey;
	CatalogKey			tempKey;
	CatalogRecord		record;
	CatalogRecord		threadRecord;
	CatalogRecord		record2;
	CatalogName			nodeName;
	CatalogNodeID		parentID;
	CatalogNodeID		cNodeID;
	UInt32				hint;
	UInt32				hint2;
	UInt32				threadHint;
	OSErr				err;
	UInt16				recordSize;
	SInt16				recordType;
	SInt16				threadRecordType;
	SInt16				selCode				= 0x8001;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;

	do
	{
		err = GetBTreeRecord( kCalculatedCatalogRefNum, selCode, &foundKey, &record, &recordSize, &hint );
		if ( err != noErr ) break;
	
		selCode		= 1;														//	 kNextRecord			
		recordType	= record.recordType;
		
		switch( recordType )
		{
			case kSmallFileRecord:
				//	If the small file is not supposed to have a thread, just break
				if ( ( record.smallFile.flags & kFileThreadExistsMask ) == 0 )
					break;
			
			case kSmallFolderRecord:
			case kLargeFolderRecord:
			case kLargeFileRecord:
				
				//	Locate the thread associated with this record
				
				#if ( ! StandAloneEngine )
					SpinCursor( 4 ); 												//	rotate cursor
				#endif

				parentID	= isHFSPlus == true ? foundKey.large.parentID : foundKey.small.parentID;
				threadHint	= hint;
				
				switch( recordType )
				{
					case kSmallFolderRecord:	cNodeID		= record.smallFolder.folderID;	break;
					case kSmallFileRecord:		cNodeID		= record.smallFile.fileID;		break;
					case kLargeFolderRecord:	cNodeID		= record.largeFolder.folderID;	break;
					case kLargeFileRecord:		cNodeID		= record.largeFile.fileID;		break;
				}

				//-- Build the key for the file thread
				BuildCatalogKey( cNodeID, nil, isHFSPlus, &key );

				err = SearchBTreeRecord ( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &threadRecord, &recordSize, &hint2 );

				if ( err != noErr )
				{
					//	For missing thread records, just create the thread
					if ( err == btNotFound )
					{
						//	Create the missing thread record.
						switch( recordType )
						{
							case kSmallFolderRecord:	threadRecordType	= kSmallFolderThreadRecord;		break;
							case kSmallFileRecord:		threadRecordType	= kSmallFileThreadRecord;		break;
							case kLargeFolderRecord:	threadRecordType	= kLargeFolderThreadRecord;		break;
							case kLargeFileRecord:		threadRecordType	= kLargeFileThreadRecord;		break;
						}

						//-- Fill out the data for the new file thread
						
						if ( isHFSPlus )
						{
							LargeCatalogThread		threadData;
							
							ClearMemory( (Ptr)&threadData, sizeof(LargeCatalogThread) );
							threadData.recordType	= threadRecordType;
							threadData.parentID		= parentID;
							CopyCatalogName( (CatalogName *)&foundKey.large.nodeName, (CatalogName *)&threadData.nodeName, isHFSPlus );
							err = InsertBTreeRecord( kCalculatedCatalogRefNum, &key, &threadData, sizeof(LargeCatalogThread), &threadHint );
						}
						else
						{
							SmallCatalogThread		threadData;
							
							ClearMemory( (Ptr)&threadData, sizeof(SmallCatalogThread) );
							threadData.recordType	= threadRecordType;
							threadData.parentID		= parentID;
							CopyCatalogName( (CatalogName *)&foundKey.small.nodeName, (CatalogName *)&threadData.nodeName, isHFSPlus );
							err = InsertBTreeRecord( kCalculatedCatalogRefNum, &key, &threadData, sizeof(SmallCatalogThread), &threadHint );
						}
					}
					else
					{
						break;
					}
				}
			
				break;
			
			
			case kSmallFolderThreadRecord:
			case kSmallFileThreadRecord:
			case kLargeFolderThreadRecord:
			case kLargeFileThreadRecord:
				
				//	Find the catalog record, if it does not exist, delete the existing thread.
				if ( isHFSPlus )
					BuildCatalogKey( record.largeThread.parentID, (const CatalogName *)&record.largeThread.nodeName, isHFSPlus, &key );
				else
					BuildCatalogKey( record.smallThread.parentID, (const CatalogName *)&record.smallThread.nodeName, isHFSPlus, &key );
				
				err = SearchBTreeRecord ( kCalculatedCatalogRefNum, &key, kNoHint, &tempKey, &record2, &recordSize, &hint2 );
				if ( err != noErr )
				{
					err = DeleteBTreeRecord( kCalculatedCatalogRefNum, &foundKey );
				}
				
				break;

		}
	} while ( err == noErr );

	if ( err == btNotFound )
		err = noErr;				 						//	all done, no more catalog records

	return( err );
}


static	OSErr	RepairReservedBTreeFields ( SGlobPtr GPtr )
{
	CatalogRecord		record;
	CatalogKey			foundKey;
	UInt16 				recordSize;
	SInt16				selCode;
	UInt32				hint;
	UInt32				*reserved;
	OSErr				err;

	selCode = 0x8001;															//	 start with 1st record			

	err = GetBTreeRecord( kCalculatedCatalogRefNum, selCode, &foundKey, &record, &recordSize, &hint );
	if ( err != noErr ) goto EXIT;

	selCode = 1;																//	 get next record from now on		
	
	do
	{
		switch( record.recordType )
		{
			case kLargeFolderRecord:
				if ( record.largeFolder.flags != 0 )
				{
					record.largeFolder.flags = 0;
					err = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recordSize, &hint );
				}
				break;
				
			case kLargeFileRecord:
				if ( ( record.largeFile.flags  & ~(0X83) ) != 0 )
				{
					record.largeFile.flags |= 0X83;
					err = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recordSize, &hint );
				}
				break;

			case kSmallFolderRecord:
				if ( record.smallFolder.flags != 0 )
				{
					record.smallFolder.flags = 0;
					err = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recordSize, &hint );
				}
				break;

			case kSmallFolderThreadRecord:
			case kSmallFileThreadRecord:
				reserved = (UInt32*) &(record.smallThread.reserved);
				if ( reserved[0] || reserved[1] )
				{
					reserved[0]	= 0;
					reserved[1]	= 0;
					err = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recordSize, &hint );
				}
				break;

			case kSmallFileRecord:
				if ( 	( ( record.smallFile.flags  & ~(0X83) ) != 0 )
					||	( record.smallFile.dataStartBlock != 0 )	
					||	( record.smallFile.rsrcStartBlock != 0 )	
					||	( record.smallFile.reserved != 0 )			)
				{
					record.smallFile.flags			|= 0X83;
					record.smallFile.dataStartBlock	= 0;
					record.smallFile.rsrcStartBlock	= 0;
					record.smallFile.reserved		= 0;
					err = ReplaceBTreeRecord( kCalculatedCatalogRefNum, &foundKey, hint, &record, recordSize, &hint );
				}
				break;
				
			default:
				break;
		}

		if ( err != noErr ) goto EXIT;
		
		err = GetBTreeRecord( kCalculatedCatalogRefNum, selCode, &foundKey, &record, &recordSize, &hint );
	
	} while ( err == noErr );

	if ( err == btNotFound )
		err = noErr;				 						//	all done, no more catalog records

EXIT:
	return( err );
}


//	Traverse the leaf nodes forward until a bad node is found then...
//	Traverse the leaf nodes backwards untill a bad node is found, all the while...
//	Inserting each record into a new BTree.  Once we are done call ExchangeFileIDs,
//	and update the entries in the VH.
static	OSErr	RebuildBTree( SGlobPtr GPtr, SInt16 refNum )
{	
	CatalogRecord		record;
	CatalogKey			foundKey;
	UInt16 				recordSize;
	UInt16				i;
	SInt16				selCode;
	SInt16				newBTreeRefNum;
	UInt32				returnedHint;
	UInt32				hint;
	UInt32				blocksUsed;
	OSErr				err;
	FIDParam			pb;
	Boolean				hasOverflowExtents;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;
	FCB					*fcb				= GetFileControlBlock( refNum );
	FCB					*repairFCB			= GPtr->calculatedRepairFCB;
	ExtendedVCB			*vcb				= GPtr->calculatedVCB;



	//	Mark the volume inconsistant
	//	If any errors, ie loss of power, occur before we update the bitmap, MountCheck will clean them up
	vcb->vcbAtrb	&= ~kVolumeUnmountedMask;
	MarkVCBDirty( vcb );
	err = FlushVolumeControlBlock( vcb );
	ReturnIfError( err );											//	If we can't write to our id block, we are in real trouble

	//	Create our "Repair" BTree
	err = CreateAndOpenRepairBtree( GPtr, refNum, &newBTreeRefNum );
	ReturnIfError( err );

	//
	//-	First traverse the tree forwards inserting each record into the new BTree
	//

	selCode = 0x8001;												//	 Start with 1st record			

	err = GetBTreeRecord( refNum, selCode, &foundKey, &record, &recordSize, &hint );
	if ( err == noErr )
	{
		selCode = 1;													//	 Get next record from now on		
	
		do
		{
			#if ( ! StandAloneEngine )
			if ( ++i > 10 ) { SpinCursor( 4 ); i = 0; }					//	Spin the cursor every 10 entries
			#endif
	
			err = InsertBTreeRecord( newBTreeRefNum, &foundKey, &record, recordSize, &returnedHint );
			ExitOnError( err );
			
			err = GetBTreeRecord( refNum, selCode, &foundKey, &record, &recordSize, &hint );
		} while ( err == noErr );
	}
	
	//
	//- Now	traverse the tree backwards inserting each record into the new BTree
	//
#if(0)
	if ( (err) && (err != btNotFound) )
	{
		selCode = 0x7FFF;											//	 Start with last record			
	
		err = GetBTreeRecord( refNum, selCode, &foundKey, &record, &recordSize, &hint );
		ExitOnError( err );											//	If the last node is damaged, just bail
	
		selCode = -1;												//	Get previous record from now on		
	
		do
		{
			#if ( ! StandAloneEngine )
			if ( ++i > 10 ) { SpinCursor( -4 ); i = 0; }			//	Spin the cursor (backwards) every 10 entries
			#endif

			err = InsertBTreeRecord( newBTreeRefNum, &foundKey, &record, recordSize, &returnedHint );
			
			if ( err == noErr )
				err = GetBTreeRecord( refNum, selCode, &foundKey, &record, &recordSize, &hint );

		} while ( err == noErr );
	}
	//	clean up error here
#endif

	if ( err != btNotFound )		//	Exit if we have an unexpected error
		ExitOnError( err );
	
	//
	//	Delete the corrupted BTree file
	//	Delete overflow extents, and bitmap bits
	//
	//	Delete old BTree extents from extents overflow file  ( We will be rebuilding the BitMap  GPtr->VIStat |= S_VBM )
	(void) ProcessFileExtents( GPtr, fcb, 0, deleteExtents, (fcb->fcbFlNm == kHFSExtentsFileID), &hasOverflowExtents, &blocksUsed );
	(void) DeleteFilesOverflowExtents( GPtr, fcb );
	if ( hasOverflowExtents == true )
	{
		//	Flush the extents BTree
		M_BTreeHeaderDirty( ((BTreeControlBlockPtr) GPtr->calculatedExtentsFCB->fcbBTCBPtr) );	
		(void) BTFlushPath( GPtr->calculatedExtentsFCB );						//	Flush the header out to disk.
	}
	
	//	Flush the cache before we do any fcb switching
	DFAFlushCache();

	//
	//	Set our globals to point to the new BTree file
	//
	GPtr->calculatedRepairBTCB->fileRefNum	= refNum;
	BlockMoveData( GPtr->calculatedRepairBTCB, fcb->fcbBTCBPtr, sizeof( BTreeControlBlock ) );
	BlockMoveData( GPtr->extendedRepairFCB, GetParallelFCB(refNum), sizeof( ExtendedFCB ) );

	//	Copy the FCB
	BlockMoveData( fcb->fcbCName, repairFCB->fcbCName, sizeof(fcb->fcbCName) );
	repairFCB->fcbFlNm		= fcb->fcbFlNm;
	repairFCB->fcbBTCBPtr	= fcb->fcbBTCBPtr;
	BlockMoveData( repairFCB, fcb, sizeof( FCB ) );
	
	//	Write the new BTree file to disk.
	err = UpdateBTreeHeader( GPtr, refNum );	//	update catalog BTH
	ExitOnError( err );

	//
	//	Update the VH MDB to point to our new BTree
	//
	MarkVCBDirty( GPtr->calculatedVCB );
	err = FlushAlternateVolumeControlBlock( GPtr->calculatedVCB, isHFSPlus );	//	Writes real & alt blocks
	ExitOnError( err );


ErrorExit:
	if ( err )
		(void) BTClosePath( repairFCB );

	return( err );
}





static	OSErr	CreateAndOpenRepairBtree( SGlobPtr GPtr, SInt16 refNum, SInt16 *newBTreeRefNum )
{
	OSErr				err;
	UInt16				nodeSize;
	SInt32				clumpSize;
	UInt32				mapNodes;
	UInt32				actualBytesAdded;
	UInt32				blocksUsed;
	Boolean				readFromDisk;
	Boolean				hasOverflowExtents;
	BTreeControlBlock	*corruptedBTCB;
	HeaderPtr			buffer;
	FCB					*corruptedFCB;
	FCB					*fcb;
	UInt16				recordCount			= 0;
	ExtendedVCB			*vcb				= GPtr->calculatedVCB;
	BTreeControlBlock	*btcb				= GPtr->calculatedRepairBTCB;	//	calculatedRepairBTCB
	CatalogNodeID		fileID				= vcb->vcbNxtCNID + 1;			//€€ Verify fileID is not in use


	//
	//	Clear all the Repair structures in case they are being reused
	//
	ClearMemory( (Ptr) GPtr->calculatedRepairFCB, sizeof(FCB) );
	ClearMemory( (Ptr) GPtr->extendedRepairFCB, sizeof(ExtendedFCB) );
	ClearMemory( (Ptr) GPtr->calculatedRepairBTCB, sizeof(BTreeControlBlock) );

	//€€	We don't know the state of our calculated btcb structures since the BTree was damaged,
	//€€	so we may want to read the btree header from disk.
	//	HeaderRec				*header;
	//	err = GetBTreeHeader( GPtr, kCalculatedExtentRefNum, &header );

	if ( refNum == kCalculatedExtentRefNum )
	{
		WriteMsg( GPtr, M_RebuildingExtentsBTree, kStatusMessage );
		corruptedBTCB	= GPtr->calculatedExtentsBTCB;
		corruptedFCB	= GPtr->calculatedExtentsFCB;
		clumpSize		= vcb->vcbXTClpSiz;
		GPtr->EBTStat	|= S_BTH;
		GPtr->TarID		= kHFSExtentsFileID;
	}
	else if ( refNum == kCalculatedCatalogRefNum )
	{
		WriteMsg( GPtr, M_RebuildingCatalogBTree, kStatusMessage );
		corruptedBTCB	= GPtr->calculatedCatalogBTCB;
		corruptedFCB	= GPtr->calculatedCatalogFCB;
		clumpSize		= vcb->vcbCTClpSiz;
		GPtr->CBTStat	|= S_BTH;
		GPtr->TarID		= kHFSCatalogFileID;
		recordCount		= 0;
	}
	else if ( refNum == kCalculatedAttributesRefNum )
	{
		WriteMsg( GPtr, M_RebuildingAttributesBTree, kStatusMessage );
		corruptedBTCB	= GPtr->calculatedAttributesBTCB;
		corruptedFCB	= GPtr->calculatedAttributesFCB;
		clumpSize		= vcb->attributesClumpSize;
		GPtr->TarID		= kHFSAttributesFileID;
	}
	else
	{
		return( notBTree );
	}

	GPtr->VIStat |= ( S_VBM + S_MDB );									//	Force bitmap and MDB/VH to be updated
	InvalidateCalculatedVolumeBitMap( GPtr );							//	Invalidate our BitMap

	//
	//	Create and open new BTree
	//
	
	//	Create Calculated Repair FCB

	fcb	= SetupFCB( vcb, kCalculatedRepairRefNum, fileID, clumpSize );
	fcb->fcbBTCBPtr	= nil;
	fcb->fcbEOF		= 0;
	fcb->fcbPLen	= 0;
	BlockMoveData( "\pCalculated Repair FCB", fcb->fcbCName, sizeof("\pCalculated Repair FCB") );


	//	Make the new BTree is the same size as the corrupted BTree
	err = ExtendFileC ( vcb, fcb, corruptedFCB->fcbEOF, 0, &actualBytesAdded );
	ReturnIfError( err );
	
	fcb->fcbEOF		= fcb->fcbPLen;							// new B-tree looks at fcbEOF
	*newBTreeRefNum	= kCalculatedRepairRefNum;

	//	Verify that new BTree fits in our extent, (no overflow extents)
	err	= ProcessFileExtents( GPtr, fcb, 0, clearBlocks, (refNum == kCalculatedExtentRefNum), &hasOverflowExtents, &blocksUsed );
	if ( err || hasOverflowExtents )
	{
		if ( hasOverflowExtents )
		{
			err	= E_DiskFull;
			GPtr->TarBlock	= blocksUsed;
			RcdError( GPtr, err );
		}
		(void) ProcessFileExtents( GPtr, fcb, 0, deleteExtents, (refNum == kCalculatedExtentRefNum), &hasOverflowExtents, &blocksUsed );
		return( err );
	}

	//€€€	Faster than ClearBlocks
	err	= ZeroFileBlocks( vcb, fcb, 0, fcb->fcbEOF );


	//
	//	Initialize the b-tree.  Write out the header.
	//
	nodeSize	= corruptedBTCB->nodeSize;
	err = GetCacheBlock( kCalculatedRepairRefNum, 0, nodeSize, gbDefault, (void**)&buffer, &readFromDisk );
	ReturnIfError( err );
	
	InitBTreeHeader( fcb->fcbEOF, fcb->fcbClmpSize, nodeSize, recordCount, corruptedBTCB->maxKeyLength, corruptedBTCB->attributes, &mapNodes, (void*)buffer );
	
	err = ReleaseCacheBlock( buffer, rbWriteMask );
	ReturnIfError( err );
		
	if ( mapNodes > 0 )										// do we have any map nodes?
	{
		err = CreateMapNodes( recordCount+1, mapNodes, nodeSize );				// write map nodes to disk
		ReturnIfError( err );
	}

	//	Finally, prepare for using the B-tree		
	err = OpenBTree( kCalculatedRepairRefNum, corruptedBTCB->keyCompareProc );
	if ( err != noErr )
	{
		M_DebugStr("\pCould not OpenBTree");
		(void) BTClosePath( fcb );
		return( err );
	}
	
	//	Move the new btcb into our space, calculatedRepairBTCB
	BlockMoveData( fcb->fcbBTCBPtr , btcb, sizeof(BTreeControlBlock) );
	fcb->fcbBTCBPtr	= (Ptr) btcb;
	DisposePtr( fcb->fcbBTCBPtr );
	
	return( err );
}



OSErr	ProcessFileExtents( SGlobPtr GPtr, FCB *fcb, UInt8 forkType, UInt16 flags, Boolean isExtentsBTree, Boolean *hasOverflowExtents, UInt32 *blocksUsed  )
{
	UInt32				extentBlockCount;
	UInt32				extentStartBlock;
	UInt32				hint;
	SInt16				i;
	UInt16 				recordSize;
	ExtendedFCB			*extendedFCB;
	LargeExtentKey		key;
	LargeExtentKey		extentKey;
	LargeExtentRecord	extents;
	Boolean				done				= false;
	ExtendedVCB			*vcb				= GPtr->calculatedVCB;
	UInt32				fileNumber			= fcb->fcbFlNm;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;
	UInt32				blockCount			= 0;
	Boolean				firstRecord			= true;
	OSErr				err					= noErr;

	
	*hasOverflowExtents	= false;							//	default
	
	err = GetFCBExtentRecord( vcb, fcb, extents );			//	Gets extents in a LargeExtentRecord

	while ( (done == false) && (err == noErr) )
	{
//		err = ChkExtRec( GPtr, extents );					//	checkout the extent record first
//		if ( err != noErr )		break;

		for ( i=0 ; i<GPtr->numExtents ; i++ )				//	now checkout the extents
		{
			extentBlockCount = extents[i].blockCount;
			extentStartBlock = extents[i].startBlock;
		
			if ( extentBlockCount == 0 )
				break;
			
			//	Clear the sectors on disk, the btree scanner cannot handle unallocated space
			#if(0)	//	ZeroFileBlocks is faster and uses less memory
				if ( flags == clearBlocks )
				{
					err = ClearBlocks( GPtr, fcb, blockCount, extentBlockCount );
					if ( err != noErr )
					{
						M_DebugStr("\p Problem Clearing Blocks");
						break;
					}
				}
			#endif
	
			if ( flags == addBitmapBit )
			{
				err = AllocExt( GPtr, extentStartBlock, extentBlockCount );
				if ( err != noErr )
				{
					M_DebugStr("\p Problem Allocating Extents");
					break;
				}
			}
	
			blockCount += extentBlockCount;
		}
		
		
		if ( (err != noErr) || isExtentsBTree )				//	Extents file has no extents
			break;


		err = FindExtentRecord( vcb, forkType, fileNumber, blockCount, false, &key, extents, &hint );
		if ( err == noErr )
		{
			*hasOverflowExtents	= true;
		}
		else if ( err == btNotFound )
		{
			err		= noErr;								//	 no more extent records
			done	= true;
			break;
		}
		else if ( err != noErr )
		{
			err = IntError( GPtr, err );
			break;
		}
		
		if ( flags == deleteExtents )
		{
			err = DeleteExtentRecord( vcb, forkType, fileNumber, blockCount );
			if ( err != noErr ) break;
			
			vcb->freeBlocks += extentBlockCount;
			UpdateVCBFreeBlks( vcb );
			MarkVCBDirty( vcb );
		}
	}
	
	*blocksUsed = blockCount;
	
	return( err );
}



static	OSErr	DeleteFilesOverflowExtents( SGlobPtr GPtr, FCB *fcb  )
{
	BTScanState			scanState;
	ExtentKey			*extentKeyPtr;
	ExtentRecord		*extentDataPtr;
	ExtentRecord		zeroExtents;
	UInt32				maxRecords;
	UInt32				recordSize;
	UInt32				hint;
	OSErr				err;
	ExtentRecord		extents;
	ExtentKey			foundExtentKey;
	UInt32				recordsFound		= 0;
	ExtendedVCB			*vcb				= GPtr->calculatedVCB;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;
	FSVarsRec			*fsVars				= (FSVarsRec *) LMGetFSMVars();


	ClearMemory( (Ptr)&zeroExtents, sizeof(ExtentRecord) );
	maxRecords = (fcb->fcbEOF) / (isHFSPlus ? sizeof(LargeExtentRecord) : sizeof(SmallExtentRecord));

	err = BTScanInitialize( GPtr->calculatedExtentsFCB, 0, 0, 0, fsVars->gAttributesBuffer, fsVars->gAttributesBufferSize, &scanState );
	if ( err != noErr )	return( badMDBErr );

	// visit all the leaf node data records in the extents B*-Tree
	while ( recordsFound < maxRecords )
	{
		err = BTScanNextRecord( &scanState, false, (void **) &extentKeyPtr, (void **) &extentDataPtr, &recordSize );

		if ( err != noErr )	break;

		++recordsFound;

		if ( isHFSPlus && (fcb->fcbFlNm == extentKeyPtr->large.fileID) )
		{
			err = SearchBTreeRecord( kCalculatedExtentRefNum, extentKeyPtr, kNoHint, &foundExtentKey, &extents, (UInt16*)&recordSize, &hint );
			err = ReplaceBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey, hint, &zeroExtents, recordSize, &hint );
//			err = DeleteExtentRecord( vcb, extentKeyPtr->large.forkType, fcb->fcbFlNm, extentKeyPtr->large.startBlock );
		}
		else if ( !isHFSPlus && (fcb->fcbFlNm == extentKeyPtr->small.fileID) )
		{
			err = SearchBTreeRecord( kCalculatedExtentRefNum, extentKeyPtr, kNoHint, &foundExtentKey, &extents, (UInt16*)&recordSize, &hint );
			err = ReplaceBTreeRecord( kCalculatedExtentRefNum, &foundExtentKey, hint, &zeroExtents, recordSize, &hint );
		}
	}
	
	if ( err = btNotFound )
		err = noErr;
		
	return( err );
}


static	OSErr	CreateMapNodes( UInt32 firstMapNode, UInt32	numberOfNewMapNodes, UInt16 nodeSize )
{
	void				*buffer;
	Boolean				readFromDisk;
	UInt32				mapRecordBytes;
	UInt32				i;
	UInt32				mapNodeNum		= firstMapNode;
	OSErr				err				= noErr;
	
	for ( i = 0 ; i < numberOfNewMapNodes ; i++  )
	{
		err = GetCacheBlock( kCalculatedRepairRefNum, mapNodeNum, nodeSize, gbDefault, &buffer, &readFromDisk );
		ReturnIfError( err );
		ClearMemory( buffer, nodeSize );							// start with clean node

		((NodeDescPtr)buffer)->numRecords	= 1;
		((NodeDescPtr)buffer)->type			= kMapNode;
		
		// set free space offset
//		*(UInt16 *)((Ptr)buffer + nodeSize - 4) = nodeSize - 6;
		mapRecordBytes = nodeSize - sizeof(BTNodeDescriptor) - 2*sizeof(SInt16) - 2;		// must belong word aligned (hence the extra -2)
	
		SetOffset( buffer, nodeSize, sizeof(BTNodeDescriptor), 1);							// set offset to map record (1st)
		SetOffset( buffer, nodeSize, sizeof(BTNodeDescriptor) + mapRecordBytes, 2);		// set offset to free space (2nd)

		if ( (i+1) < numberOfNewMapNodes )
			((BTNodeDescriptor*)buffer)->fLink = mapNodeNum+1;	// point to next map node
		else
			((BTNodeDescriptor*)buffer)->fLink = 0;					// this is the last map node
			
		err = ReleaseCacheBlock( buffer, rbWriteMask );
		ReturnIfError( err );
		
#if( 0 )	//	Already handled in InitBTreeHeader
		err = MarkMapNodeAllocated( kCalculatedRepairRefNum, mapNodeNum );
		ReturnIfError( err );
#endif
		
		++mapNodeNum;
	}
	
	//€€	Mark off the new map nodes
	
	return( err );
}

#if( 0 )	//	Already handled in InitBTreeHeader
//	return error if node was already allocated
static	OSErr	MarkMapNodeAllocated( short refNum, UInt32 nodeNum )
{	
	OSStatus				err;
	BlockDescriptor			node;
	UInt16					*mapPtr;
	UInt16					bitInWord;
	UInt16					mapSize;
	BTreeControlBlockPtr	btreePtr;
	
	btreePtr		= (BTreeControlBlockPtr) ( GetFileControlBlock( refNum ) )->fcbBTCBPtr;
	node.buffer		= nil;							// clear node.buffer to get header node

	err = GetMapNode( btreePtr, &node, &mapPtr, &mapSize );
	
	if ( nodeNum > (mapSize * 8) )					//	Number of bits in map, max nodeNum in this map
	{
		M_DebugStr("\pnodeNum will not fit in header map node");
		err = 123;
		M_ExitOnError( err );
	}	
	
	mapPtr		+= (nodeNum / 16);					//	Advance mapPtr to word representing bit to be allocated
	bitInWord	=  15 - ( nodeNum - ( (nodeNum / 16) * 16 ) );
	
	if ( *mapPtr & ( 1 << bitInWord ) )
	{
		M_DebugStr("\p Map Node allready allocated");
		err = 123;
		M_ExitOnError( err );
	}
	
	*mapPtr		|= ( 1 << bitInWord );


ErrorExit:
	
	(void) ReleaseNode (btreePtr, &node);
	
	return(	err );
}

#endif


static	OSErr	ClearBlocks( SGlobPtr GPtr, FCB *fcb, UInt32 startBlock, UInt32 blockCount )
{
	UInt32			blockNumber;
	LogicalAddress	buffer;
	Boolean			readFromDisk;
	OSErr			err				= noErr;
	UInt32			blockSize		= GPtr->calculatedVCB->blockSize;
	
	for ( blockNumber = startBlock ; blockNumber < blockCount ; blockNumber++ )
	{
		err	= GetCacheBlock( GetFileRefNumFromFCB(fcb), blockNumber, blockSize, 0, &buffer, &readFromDisk );
		ReturnIfError( err );

		ClearMemory( buffer, blockSize );	// clear out our buffer

		err	= ReleaseCacheBlock( buffer, rbWriteMask );

		#if ( ! StandAloneEngine )
			SpinCursor( -4 ); 												//	rotate cursor
		#endif
	}
	
	return( err );
}

/*------------------------------------------------------------------------------

Function:	cmpLongs

Function:	compares two longs.
			
Input:		*a:  pointer to first number
			*b:  pointer to second number

Output:		<0 if *a < *b
			0 if a == b
			>0 if a > b
------------------------------------------------------------------------------*/

int cmpLongs ( const void *a, const void *b )
{
	return( *(long*)a - *(long*)b );
}




//_______________________________________________________________________
//	Notes:	byteOffset and numberOfBytes are assumed to be UInt32
//_______________________________________________________________________


//	This structure *should* replace gAttributesBuffer so we can use it throughout the file system
//	fsBuffer = &((FSVarsRec *) LMGetFSMVars())->fsBuffer;
struct BlockBufferDescriptor {
	Ptr 					buffer;
	UInt32 					bufferSize;
	Boolean 				inUse;
};
typedef struct BlockBufferDescriptor BlockBufferDescriptor;


static	OSErr	ZeroFileBlocks( ExtendedVCB *vcb, FCB *fcb, UInt32 byteOffset, UInt32 numberOfBytes )
{
#ifdef INVESTIGATE
	OSErr					err;
	BlockBufferDescriptor	fsBuffer;
	UInt32					contiguousBytes;
	UInt32					diskBlock;
	XIOParam				pb;
	UInt32					bytesWrittenInThisExtent;
	UInt32					bytesToWrite;
	SInt32					i;
	UInt32					bytesWritten				= 0;
	UInt32					maximumBytes				= numberOfBytes;
	
	if ( (byteOffset % vcb->blockSize != 0) || (numberOfBytes % vcb->blockSize != 0) )
		return( -50 );

	fsBuffer.inUse		= false;
	fsBuffer.buffer		= ((FSVarsRec *) LMGetFSMVars())->gAttributesBuffer;
	fsBuffer.bufferSize	= ((FSVarsRec *) LMGetFSMVars())->gAttributesBufferSize;

	if ( fsBuffer.inUse	== false )								//	Free to use fsBuffer
	{
		fsBuffer.inUse	= true;										//	Mark the buffer as inUse
		ClearMemory( fsBuffer.buffer, fsBuffer.bufferSize );		//	Zero our buffer
		
		pb.ioVRefNum	= vcb->vcbDrvNum;
		pb.ioRefNum		= vcb->vcbDRefNum;
		pb.ioPosMode	= fsFromStart;
		pb.ioBuffer		= fsBuffer.buffer;

		do
		{
			err = MapFileBlockC( vcb, fcb, maximumBytes, byteOffset + bytesWritten, &diskBlock, &contiguousBytes );
			bytesWrittenInThisExtent	= 0;
			
			do
			{
				#if ( ! StandAloneEngine )
				if ( ++i > 10 ) { SpinCursor( 4 ); i = 0; }					//	Spin the cursor every 10 entries
				#endif
				
				if ( (contiguousBytes - bytesWrittenInThisExtent)  > fsBuffer.bufferSize )
					bytesToWrite	= fsBuffer.bufferSize;
				else
					bytesToWrite	= (contiguousBytes - bytesWrittenInThisExtent);
							
				pb.ioReqCount	= bytesToWrite;
				
				if ( diskBlock > kMaximumBlocksIn4GB )
				{
					pb.ioWPosOffset.lo	=  diskBlock << Log2BlkLo;		//	calculate lower 32 bits
					pb.ioWPosOffset.hi	=  diskBlock >> Log2BlkHi;		//	calculate upper 32 bits
					pb.ioPosMode		|= (1 << kWidePosOffsetBit);	//	set wide positioning mode
				}
				else
				{
					((IOParam*)&pb)->ioPosOffset = diskBlock << Log2BlkLo;
				}
				
				err = PBWriteSync( (ParamBlockRec *)&pb );
				
				bytesWrittenInThisExtent	+= pb.ioActCount;
				diskBlock					+= (pb.ioActCount / 512);
				
			} while ( bytesWrittenInThisExtent < contiguousBytes );
			
			bytesWritten	+= bytesWrittenInThisExtent;
			
		} while ( bytesWritten < numberOfBytes );
	}
	
	fsBuffer.inUse	= false;										//	Mark the buffer as inUse

	return( err );
#else
	return 0;
#endif

}

