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
	File:		SVerify2.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(djb)	Don Brady
		(DSH)	Deric Horn

	Change History (most recent first):

	 <HFS18>	 12/8/97	DSH		Replace PLstrcmp with CmpBlock
	 <HFS17>	 12/2/97	DSH		clean up
	 <HFS16>	11/18/97	DSH		CheckMDB easier to read in debugger
	 <HFS15>	 11/4/97	DSH		Make CompareVolumeHeader easier to read from debugger
	 <HFS14>	10/21/97	DSH		Repair damaged BTrees
	 <HFS13>	 9/18/97	DSH		Added VolumeHeader check of the encodingsBitmap.
	 <HFS12>	 9/17/97	DSH		Wrapperless HFS+ volume support.
	 <HFS11>	  9/5/97	DSH		Report E_VolumeHeaderDamaged for HFS+ volumes.
	 <HFS10>	  9/4/97	msd		Clean up some nasty pointer math.
	  <HFS9>	  9/2/97	DSH		Adding new global idSector containing the location of the alt
									MDB or VH.
	  <HFS8>	 8/20/97	DSH		Dont check the roaving pointer on HFS disks, if we repair, just
									zero it.
	  <HFS7>	 8/18/97	DSH		Added call to CheckForStop() in CompareVolumeBitMap
	  <HFS6>	 8/13/97	msd		Bug #1673016. In ChkCName, the maximum length of an HFS Plus
									filename is kHFSPlusMaxFileNameChars, not CMMaxCName. For HFS
									disks, use the constant kHFSMaxFileNameChars.
	  <HFS5>	 6/26/97	DSH		Eliminated unicode conversion dependency, we keep names in their
									native volume format type.
	  <HFS4>	 4/28/97	DSH		In CmpMDB cast (UInt16)calculatedVCB->vcbAtrb for compare
	  <HFS3>	  4/4/97	djb		Get in sync with volume format changes.
		 <HFS2>	 3/27/97	DSH		Cleaning out DebugStrs, CheckDisk uses 64Bit fields.
		 <HFS1>	 3/17/97	DSH		Initial Check-In
*/

#include "ScavDefs.h"
#include "Prototypes.h"
#include "BTreesInternal.h"
#include "HFSBTreesPriv.h"
#include "BTreesPrivate.h"

#include "DFALowMem.h"

#include <string.h>

/* prototypes for internal subroutines */

static int BTKeyChk( SGlobPtr GPtr, NodeDescPtr nodeP, BTreeControlBlock *btcb );

		


/*------------------------------------------------------------------------------

Routine:	ChkExtRec (Check Extent Record)

Function:	Checks out a generic extent record.
			
Input:		GPtr		-	pointer to scavenger global area.
			extP		-	pointer to extent data record.
			
Output:		ChkExtRec	-	function result:			
								0 = no error
								n = error
------------------------------------------------------------------------------*/

OSErr ChkExtRec ( SGlobPtr GPtr, const void *extents )
{
	short		i;
	UInt32		numABlks;
	UInt32		maxNABlks;
	UInt32		extentBlockCount;
	UInt32		extentStartBlock;

	maxNABlks = GPtr->calculatedVCB->totalBlocks;
	numABlks = 1;

	for ( i=0 ; i<GPtr->numExtents ; i++ )
	{
		//	HFS+/HFS moving extent fields into local variables for evaluation
		if ( GPtr->isHFSPlus )
		{
			extentBlockCount = ((LargeExtentDescriptor *)extents)[i].blockCount;
			extentStartBlock = ((LargeExtentDescriptor *)extents)[i].startBlock;
		}
		else
		{
			extentBlockCount = ((SmallExtentDescriptor *)extents)[i].blockCount;
			extentStartBlock = ((SmallExtentDescriptor *)extents)[i].startBlock;
		}
		
		if ( extentStartBlock >= maxNABlks )
		{
			RcdError( GPtr, E_ExtEnt );
			return( E_ExtEnt );						/* invalid extent entry */
		}
		if ( extentBlockCount >= maxNABlks )
		{
			RcdError( GPtr, E_ExtEnt );
			return( E_ExtEnt );						/* invalid extent entry */
		}			
		if ( numABlks == 0 )
		{
			if ( extentBlockCount != 0 )
			{
				RcdError( GPtr, E_ExtEnt );
				return( E_ExtEnt );					/* invalid extent entry */
			}
		}
		numABlks = extentBlockCount;
	}
	
	return( noErr );								/* all is well */
	
} /* end of ChkExtRec */


/*------------------------------------------------------------------------------

Routine:	BTCheck - (BTree Check)

Function:	Checks out the internal structure of a Btree file.  The BTree 
			structure is enunumerated top down starting from the root node.
			
Input:		GPtr		-	pointer to scavenger global area
			realRefNum		-	file refnum

Output:		BTCheck	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

int BTCheck( SGlobPtr GPtr, short refNum )
{
	OSErr				result;
	short				i;
	short				strLen;
	UInt32				nodeNum;
	short				numRecs;
	short				index;
	UInt16				recSize;
	UInt8				parKey[ kMaxKeyLength + 2 + 2 ];		//	BTreeKeyLimits
	char				*p;
	UInt8				*dataPtr;
	FCB					*calculatedFCB;
	STPR				*tprP;
	STPR				*parentP;
	KeyPtr				keyPtr;
	HeaderRec			*header;
	BTreeControlBlock	*calculatedBTCB;
	NodeRec				node;
	NodeDescPtr			nodeDescP;
	UInt16				*statusFlag;
	UInt32				leafRecords	= 0;

	//	set up

	calculatedFCB	= (FCB *) ( GPtr->FCBAPtr + refNum );
	calculatedBTCB	= (BTreeControlBlock *) calculatedFCB->fcbBTCBPtr;

	if ( refNum == kCalculatedCatalogRefNum )
		statusFlag	= &(GPtr->CBTStat);
	else if ( refNum == kCalculatedExtentRefNum )
		statusFlag	= &(GPtr->EBTStat);
	else
		statusFlag	= &(GPtr->ABTStat);

	//
	//	check out BTree header node 
	//

	GPtr->TarBlock = 0;									/* set target node number */

	result = GetNode( calculatedBTCB, kHeaderNodeNum, &node );
	if ( result != noErr )
	{
		if ( result == fsBTInvalidNodeErr )				//	CheckNode failed
		{
			RcdError( GPtr, E_BadNode );
			result	= E_BadNode;						/* invalid node structure */
		}
		return( result );
	}

	nodeDescP = node.buffer;

	result = AllocBTN( GPtr, refNum, 0 );
	ReturnIfError( result );							/* node already allocated */
	
	if ( nodeDescP->type != kHeaderNode )
	{
		RcdError( GPtr, E_BadHdrN );
		return( E_BadHdrN );							/* error, not a header node */
	}	
	if ( nodeDescP->numRecords != Num_HRecs )
	{
		RcdError( GPtr, E_BadHdrN );
		return( E_BadHdrN );							/* error, invalid record count */
	}	
	if ( nodeDescP->height != 0 )
	{
		RcdError( GPtr, E_NHeight );					/* invalid node height */
		return( E_NHeight );
	}

	//		
	//	check out BTree Header record
	//

	header = (HeaderPtr)nodeDescP;

	recSize = GetRecordSize( (BTreeControlBlock *)calculatedBTCB, (BTNodeDescriptor *)nodeDescP, 0 );	
	
	if ( recSize != sizeof(HeaderRec) - sizeof(BTNodeDescriptor) )
	{
		RcdError( GPtr, E_LenBTH );
		return( E_LenBTH );								/* invalid record length */
	}
	if ( (header->treeDepth < 0) || (header->treeDepth > BTMaxDepth) )
	{
		RcdError( GPtr, E_BTDepth );
		return( E_BTDepth );							/* bad tree depth */
	}
	calculatedBTCB->treeDepth = header->treeDepth;
	
	if ((header->rootNode < 0) || (header->rootNode >= calculatedBTCB->totalNodes))
	{
		RcdError( GPtr, E_BTRoot );
		return( E_BTRoot );								/* bad root node number */
	}
	calculatedBTCB->rootNode = header->rootNode;

	if ( (calculatedBTCB->treeDepth == 0) || (calculatedBTCB->rootNode == 0) )
	{
		if ( calculatedBTCB->treeDepth == calculatedBTCB->rootNode )
		{
			return( noErr );							/* empty BTree */
		}
		else
		{
			RcdError( GPtr, E_BTDepth );
			return( E_BTDepth );						/* depth doesn't agree with root */
		}
	}		
		
	//
	//	set up tree path record for root level
	//
 
 	GPtr->BTLevel	= 1;
	tprP			= &(*GPtr->BTPTPtr)[0];
	tprP->TPRNodeN	= calculatedBTCB->rootNode;
	tprP->TPRRIndx	= -1;
	tprP->TPRLtSib	= 0;
	tprP->TPRRtSib	= 0;
	parKey[0]		= 0;
		
	//
	//	now enumerate the entire BTree
	//

	while ( GPtr->BTLevel > 0 )
	{
		tprP	= &(*GPtr->BTPTPtr)[GPtr->BTLevel -1];
		nodeNum	= tprP->TPRNodeN;
		index	= tprP->TPRRIndx;

		GPtr->TarBlock = nodeNum;						/* set target node number */

		result = GetNode( calculatedBTCB, nodeNum, &node );
		if ( result != noErr )
		{
			if ( result == fsBTInvalidNodeErr )				//	CheckNode failed
			{
				RcdError( GPtr, E_BadNode );
				result	= E_BadNode;						/* invalid node structure */
			}
			return( result );
		}

		nodeDescP = node.buffer;
			
		//
		//	check out and allocate the node if its the first time its been seen
		//
	 			
		if ( index < 0 )
		{
			result = AllocBTN( GPtr, refNum, nodeNum );
			ReturnIfError( result );					/* node already allocated */
				
			result = BTKeyChk( GPtr, nodeDescP, calculatedBTCB );
			ReturnIfError( result );					/* invalid key structure */
				
			if ( nodeDescP->bLink != tprP->TPRLtSib )
			{
				RcdError( GPtr, E_SibLk );
				return( E_SibLk );						/* invalid backward link */
			}	
			if ( tprP->TPRRtSib == -1 )
			{
				tprP->TPRRtSib = nodeNum;				/* set Rt sibling for later verification */		
			}
			else
			{
				if ( nodeDescP->fLink != tprP->TPRRtSib )
				{				
					RcdError( GPtr, E_SibLk );
					return( E_SibLk );					/* invalid forward link */
				}
			}
			
			if ( (nodeDescP->type != kIndexNode) && (nodeDescP->type != kLeafNode) )
			{
				*statusFlag |= S_RebuildBTree;
				RcdError( GPtr, E_NType );
				result		=  noErr;
//				return( E_NType );						/* invalid node type */
			}	
			if ( nodeDescP->height != calculatedBTCB->treeDepth - GPtr->BTLevel + 1 )
			{
				*statusFlag |= S_RebuildBTree;
				RcdError( GPtr, E_NHeight );			/* invalid node height */
				result		=  noErr;
//				return( E_NHeight );					// HHS added exit out
			}
				
			if ( parKey[0] != 0 )
			{
				GetRecordByIndex( (BTreeControlBlock *)calculatedBTCB, nodeDescP, 0, &keyPtr, &dataPtr, &recSize );
				if ( CompareKeys( (BTreeControlBlockPtr)calculatedBTCB, (BTreeKey *)parKey, keyPtr ) != 0 )
				{
					*statusFlag |= S_RebuildBTree;
					RcdError( GPtr, E_IKey );
//					return( E_IKey );					/* invalid index key */ 
				}
			}
			if ( nodeDescP->type == kIndexNode )		/* permit the user to interrupt */
 			{
				if ( result = CheckForStop( GPtr ) )
					return( result );
			}
			
			#if ( StandAloneEngine )
				GPtr->itemsProcessed = U64Add( GPtr->itemsProcessed, U64SetU( 1 ) );
			#endif
		}
		
		numRecs = nodeDescP->numRecords;
	
		//
		//	for an index node ...
		//
		
		if ( nodeDescP->type == kIndexNode )
		{
			index++;									/* bump to next index record */
			if ( index >= numRecs )
			{
				GPtr->BTLevel--;
				continue;								/* no more records */
			}
			tprP->TPRRIndx = index;
			parentP = tprP;
			
			GPtr->BTLevel++;

			if ( GPtr->BTLevel > BTMaxDepth )
			{
				*statusFlag |= S_RebuildBTree;
				RcdError( GPtr, E_BTDepth );			/* error, exceeded max BTree depth */
				result		=  noErr;
//				return( E_BTDepth );
			}				
			tprP = &(*GPtr->BTPTPtr)[GPtr->BTLevel -1];
			
			GetRecordByIndex( (BTreeControlBlock *)calculatedBTCB, nodeDescP, index, &keyPtr, &dataPtr, &recSize );
			
			nodeNum = *(UInt32*)dataPtr;
			if ( (nodeNum <= 0) || (nodeNum >= calculatedBTCB->totalNodes) )
			{
				RcdError( GPtr, E_IndxLk );
				return( E_IndxLk );						/* invalid index link */
			}	
			p = (Ptr)keyPtr;							/* save parents key */
			
			
			if ( calculatedBTCB->attributes & kBTBigKeysMask )
				strLen = keyPtr->length16;
			else
				strLen = keyPtr->length8;
				
			for ( i = 0; i <= strLen; i++ )
				parKey[i] = *p++;
				
			tprP->TPRNodeN = nodeNum;
			tprP->TPRRIndx = -1;
			
			tprP->TPRLtSib = 0;							/* set up left sibling */
			if ( index > 0 )
			{
				GetRecordByIndex( (BTreeControlBlock *)calculatedBTCB, nodeDescP, index-1, &keyPtr, &dataPtr, &recSize );

				nodeNum = *(UInt32*)dataPtr;
				if ( (nodeNum <= 0) || (nodeNum >= calculatedBTCB->totalNodes) )
				{
					*statusFlag |= S_RebuildBTree;
					RcdError( GPtr, E_IndxLk );
					result		=  noErr;						//	FLASHING
//					return( E_IndxLk );					/* invalid index link */
				}
				tprP->TPRLtSib = nodeNum;
			}
			else
			{
				if ( parentP->TPRLtSib != 0 )
					tprP->TPRLtSib = tprP->TPRRtSib;	/* fill in the missing link */
			}
				
			tprP->TPRRtSib = 0;							/* set up right sibling */		
			if ( index < (numRecs -1) )
			{
				GetRecordByIndex( (BTreeControlBlock *)calculatedBTCB, nodeDescP, index+1, &keyPtr, &dataPtr, &recSize );
				nodeNum = *(UInt32*)dataPtr;
				if ( (nodeNum <= 0) || (nodeNum >= calculatedBTCB->totalNodes) )
				{
					*statusFlag |= S_RebuildBTree;
					RcdError( GPtr, E_IndxLk );
					result		=  noErr;						//	FLASHING
//					return( E_IndxLk );					/* invalid index link */
				}
				tprP->TPRRtSib = nodeNum;
			}
			else
			{
				if ( parentP->TPRRtSib != 0 )
					tprP->TPRRtSib = -1;				/* link to be filled in later */ 
			}
		}			
	
		//
		//	for a leaf node ...
		//
		else
		{
			if ( tprP->TPRLtSib == 0 )
				calculatedBTCB->firstLeafNode = nodeNum;
			if ( tprP->TPRRtSib == 0 )
				calculatedBTCB->lastLeafNode = nodeNum;
			leafRecords	+= nodeDescP->numRecords;
				
			GPtr->BTLevel--;
			continue;
		}		
	} 	//	end while

	calculatedBTCB->leafRecords	= leafRecords;
	
	return( result );

}	//	end of BTCheck



/*------------------------------------------------------------------------------

Routine:	BTMapChk - (BTree Map Check)

Function:	Checks out the structure of a BTree allocation map.
			
Input:		GPtr		-	pointer to scavenger global area
			fileRefNum	-	refnum of BTree file

Output:		BTMapChk	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

int BTMapChk( SGlobPtr GPtr, short fileRefNum )
{
	OSErr				result;
	UInt16				recSize;
	SInt16				mapSize;
	UInt32				nodeNum;
	SInt16				recIndx;
	SInt16				*statP;
	FCB					*calculatedFCB;
	BTreeControlBlock	*calculatedBTCB;
	NodeRec				node;
	NodeDescPtr			nodeDescP;

	//	set up
	
	calculatedFCB = GetFileControlBlock( fileRefNum );
	calculatedBTCB = (BTreeControlBlock *)calculatedFCB->fcbBTCBPtr;

	statP = (short *) &GPtr->EBTStat;
	if ( fileRefNum == kCalculatedCatalogRefNum )
		statP = (short *) &GPtr->CBTStat;
	
	nodeNum = 0;												/* start with header node */
	recIndx = 2;	
	mapSize = ( calculatedBTCB->totalNodes + 7 ) / 8;			/* map size (in bytes) */

	//
	//	enumerate the map structure starting with the map record in the header node
	//

	while ( mapSize > 0 )
	{
		GPtr->TarBlock = nodeNum;								/* set target node number */
			
		result = GetNode( calculatedBTCB, nodeNum, &node );
		if ( result != noErr )
		{
			if ( result == fsBTInvalidNodeErr )				//	CheckNode failed
			{
				RcdError( GPtr, E_BadNode );
				result	= E_BadNode;						/* invalid node structure */
			}
			return( result );
		}
		
		nodeDescP = node.buffer;
		
		//	check out the node if its not the header node	

		if ( nodeNum != 0 )
		{
			result = AllocBTN( GPtr, fileRefNum, nodeNum );		/* allocate the node */
			ReturnIfError( result );							/* error, node already allocated */
				
			if ( nodeDescP->type != kMapNode )
			{
				RcdError( GPtr, E_BadMapN );
				return( E_BadMapN );							/* error, not a map node */
			}	
			if ( nodeDescP->numRecords != Num_MRecs )
			{
				RcdError( GPtr, E_BadMapN );
				return( E_BadMapN );							/* error, invalid record count */
			}	
			if ( nodeDescP->height != 0 )
				RcdError( GPtr, E_NHeight );					/* error, invalid node height */
		}

		//	move on to the next map node
		recSize =	GetRecordSize( (BTreeControlBlock *)calculatedBTCB, (BTNodeDescriptor *)nodeDescP, recIndx );
		mapSize -=	recSize;									/* adjust remaining map size */

		recIndx	= 0;											/* map record is now record 0 */			
		nodeNum	= nodeDescP->fLink;								/* next node number */						
		if (nodeNum == 0)
			break;
	
	}	/* end while */

	if ( (nodeNum != 0) || (mapSize > 0) )
	{
		RcdError( GPtr, E_MapLk);
		return( E_MapLk);										/* error, bad map node linkage */
	}
	
	return( noErr );											/* all done */
	
}	/* end BTMapChk */



/*------------------------------------------------------------------------------

Routine:	CmpBTH - (Compare BTree Header)

Function:	Compares the scavenger BTH info with the BTH on disk.
			
Input:		GPtr		-	pointer to scavenger global area
			fileRefNum		-	file refnum

Output:		CmpBTH	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

OSErr CmpBTH( SGlobPtr GPtr, SInt16 fileRefNum )
{
	OSErr				result;
	SInt16				*statP;
	FCB					*calculatedFCB;
	BTreeControlBlock	*calculatedBTCB;
	HeaderPtr			bTreeHeader;

	//	Set up
	calculatedFCB = GetFileControlBlock( fileRefNum );
	calculatedBTCB = (BTreeControlBlock *)calculatedFCB->fcbBTCBPtr;

	if ( fileRefNum == kCalculatedCatalogRefNum )
		statP = (short *) &GPtr->CBTStat;
	else
		statP = (short *) &GPtr->EBTStat;

	//
	//	Get BTree header node from disk 
	//

	GPtr->TarBlock = 0;											/* set target node number */

	result = GetBTreeHeader( GPtr, fileRefNum, (HeaderRec **)&bTreeHeader );
	ReturnIfError( result );									/* could't get header node */


	//
	//	Compare them
	//
	result = CmpBlock( &calculatedBTCB->treeDepth, &bTreeHeader->treeDepth, offsetof(HeaderRec, reserved1) - offsetof(HeaderRec, treeDepth) );
	if ( result != noErr )
	{
		*statP = *statP | S_BTH;								/* didn't match, mark it damaged */
		WriteError( E_InvalidBTreeHeader, 0, 0 );
	}
	
	return( noErr );											/* all done */
	
}	/* end CmpBTH */



/*------------------------------------------------------------------------------

Routine:	CmpBlock

Function:	Compares two data blocks for equality.
			
Input:		Blk1Ptr		-	pointer to 1st data block.
			Blk2Ptr		-	pointer to 2nd data block.
			len			-	size of the blocks (in bytes)

Output:		CmpBlock	-	result code	
								0 = equal
								1 = not equal
------------------------------------------------------------------------------*/

OSErr CmpBlock( void *block1P, void *block2P, UInt32 length )
{
	Byte	*blk1Ptr = block1P;
	Byte	*blk2Ptr = block2P;

	while ( length-- ) 
		if ( *blk1Ptr++ != *blk2Ptr++ )
			return( -1 );
	
	return( noErr );
	
}	/* end of CmpBlock */



/*------------------------------------------------------------------------------

Routine:	CmpBTM - (Compare BTree Map)

Function:	Compares the scavenger BTM with the BTM on disk.
			
Input:		GPtr		-	pointer to scavenger global area
			fileRefNum		-	file refnum

Output:		CmpBTM	-	function result:			
								0	= no error
								n 	= error
------------------------------------------------------------------------------*/

int CmpBTM( SGlobPtr GPtr, short fileRefNum )
{
	OSErr				result;
	UInt16				recSize;
	short				mapSize;
	short				size;
	UInt32				nodeNum;
	short				recIndx;
	char				*p;
	short				*statP;
	char				*sbtmP;
	UInt8 *				dataPtr;
	FCB					*calculatedFCB;
	BTreeControlBlock	*calculatedBTCB;
	NodeRec				node;
	NodeDescPtr			nodeDescP;


	calculatedFCB = GetFileControlBlock( fileRefNum );
	calculatedBTCB = (BTreeControlBlock *)calculatedFCB->fcbBTCBPtr;

	if ( fileRefNum == kCalculatedCatalogRefNum )
		statP = (short *) &GPtr->CBTStat;
	else
		statP = (short *) &GPtr->EBTStat;
	
	nodeNum	= 0;														/* start with header node */
	recIndx	= 2;	
	mapSize	= (calculatedBTCB->totalNodes + 7) / 8;						/* map size (in bytes) */
	sbtmP	= ((BTreeExtensionsRec*)calculatedBTCB->refCon)->BTCBMPtr;

	//
	//	Enumerate BTree map records starting with map record in header node
	//

	while ( mapSize > 0 )
	{
		GPtr->TarBlock = nodeNum;										/* set target node number */
			
		result = GetNode( calculatedBTCB, nodeNum, &node );
		ReturnIfError( result );										/* error, could't get map node */

		nodeDescP = node.buffer;

		recSize = GetRecordSize( (BTreeControlBlock *)calculatedBTCB, (BTNodeDescriptor *)nodeDescP, recIndx );
		dataPtr = GetRecordAddress( (BTreeControlBlock *)calculatedBTCB, (BTNodeDescriptor *)nodeDescP, recIndx );
	
		size = recSize;
		if (size  > mapSize)
			size = mapSize;
			
		result = CmpBlock( sbtmP, dataPtr, size );						/* compare them */
		if ( result != noErr )
		{ 	
			*statP = *statP | S_BTM;									/* didn't match, mark it damaged */
			return( noErr );
		}
	
		recIndx	= 0;													/* map record is now record 0 */			
		mapSize	= mapSize - size;										/* adjust remaining map size */
		sbtmP	= sbtmP + size;
		nodeNum	= nodeDescP->fLink;										/* next node number */						
		if (nodeNum == 0)
			break;
	
	}	/* end while */

	//
	//	Make sure the unused portion of the last map record is zero
	//
	for (p = (Ptr)dataPtr + size; p < (Ptr)dataPtr + recSize; p++)
		if (*p != 0) 
			*statP = *statP | S_BTM;									/* didn't match, mark it damaged */

	return( noErr );													/* all done */
	
}	/* end CmpBTM */



/*------------------------------------------------------------------------------

Routine:	BTKeyChk - (BTree Key Check)

Function:	Checks out the key structure within a Btree node.
			
Input:		GPtr		-	pointer to scavenger global area
			NodePtr		-	pointer to target node
			BTCBPtr		-	pointer to BTreeControlBlock

Output:		BTKeyChk	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

static int BTKeyChk( SGlobPtr GPtr, NodeDescPtr nodeP, BTreeControlBlock *btcb )
{
	SInt16				index;
	UInt16				dataSize;
	KeyPtr 				keyPtr;
	UInt8				*dataPtr;
	KeyPtr				prevkeyP	= nil;

	//
	//	loop on number of records in node
	//

	for ( index = 0; index < nodeP->numRecords; index++)
	{
		GetRecordByIndex( (BTreeControlBlock *)btcb, nodeP, (UInt16) index, &keyPtr, &dataPtr, &dataSize );

		if ( keyPtr->length8 > btcb->maxKeyLength )
		{
			RcdError( GPtr, E_KeyLen );
			return( E_KeyLen );								/* invalid key length */
		}

		if ( prevkeyP != nil )
		{
			//if ( CBTKeyCmp( prevkeyP, keyP , btcb ) >= 0)
			if ( CompareKeys( (BTreeControlBlockPtr)btcb, prevkeyP, keyPtr ) >= 0 )
			{
				RcdError( GPtr, E_KeyOrd );
				return( E_KeyOrd );							/* invalid key order */
			}
		}
		prevkeyP = keyPtr;
	}

	return( noErr );										/* all is fine */
}	/* end of BTKeyChk */


/*------------------------------------------------------------------------------

Routine:	ChkCName (Check Catalog Name)

Function:	Checks out a generic catalog name.
			
Input:		GPtr		-	pointer to scavenger global area.
			CNamePtr	-	pointer to CName.
			
Output:		ChkCName	-	function result:			
								0 = CName is OK
								E_CName = invalid CName
------------------------------------------------------------------------------*/

OSErr ChkCName( SGlobPtr GPtr, const CatalogName *name, Boolean unicode )
{
	UInt32	length;
	OSErr	err		= noErr;
	
	length = CatalogNameLength( name, unicode );
	
	if ( unicode )
	{
		if ( (length == 0) || (length > kHFSPlusMaxFileNameChars) )
			err = E_CName;
	}
	else
	{
		if ( (length == 0) || (length > kHFSMaxFileNameChars) )
			err = E_CName;
	}
	
	return( err );
}


/*------------------------------------------------------------------------------

Routine:	CompareVolumeBitMap - (Compare Volume Bit Map)

Function:	Compares the scavenger VBM with the VBM on disk.
			
------------------------------------------------------------------------------*/
OSErr	CompareVolumeBitMap( SGlobPtr GPtr, SInt32 whichBuffer )
{
	UInt32				startBlock;
	UInt32				blockCount;
	UInt32				i;
	UInt8				*vbmBlockP;
	OSErr				err;
	UInt32				size;
	
	VolumeBitMapHeader	*volumeBitMap	= GPtr->volumeBitMapPtr;
	
	if ( volumeBitMap->bitMapRecord[whichBuffer].count == -1 )
	{
		return( E_UninitializedBuffer );
	}
	
	//	If it hasn't yet been compared, and no VBM errors have been reported yet.
	if ( (volumeBitMap->bitMapRecord[whichBuffer].isAMatch == false) && (GPtr->VIStat | S_VBM != 0) )
	{
		//	Calculate the start and length of represented BitMap buffer.
		startBlock	= whichBuffer * ( volumeBitMap->bufferSize / Blk_Size );				//	whenever whichBuffer is non zero it's a multiple of 512.
		
		if ( whichBuffer == volumeBitMap->numberOfBuffers-1 )
			blockCount	= ( ( volumeBitMap->bitMapSizeInBytes - (whichBuffer * volumeBitMap->bufferSize) + Blk_Size - 1 ) / Blk_Size );		//	last block may not be 512 byte alligned.
		else
			blockCount	= volumeBitMap->bufferSize / Blk_Size;
			
		//	Loop through all the blocks composing the physical buffer.
		for ( i=0 ; i<blockCount ; i++ )
		{
			if ( err = CheckForStop(GPtr) )													//	permit the user to interrupt
				return( err );

			GPtr->TarBlock = startBlock+i;													//	 set target block number
			
			if ( GPtr->isHFSPlus )
				err = GetBlock_FSGlue( gbReleaseMask, startBlock+i, (Ptr *)&vbmBlockP, kCalculatedAllocationsRefNum, GPtr->calculatedVCB );
			else
				err = GetVBlk( GPtr, startBlock + GPtr->calculatedVCB->vcbVBMSt + i, (void**)&vbmBlockP );	//	get map block from disk
			ReturnIfError( err );
	
			//	check if we don't fill an entire block
			if ( (whichBuffer == volumeBitMap->numberOfBuffers - 1) && (i == blockCount - 1) && (volumeBitMap->bitMapSizeInBytes % Blk_Size != 0) )	//	very last bitmap block
				size = volumeBitMap->bitMapSizeInBytes % Blk_Size;
			else
				size = Blk_Size;
			
			err = memcmp( (volumeBitMap->buffer + i*Blk_Size), vbmBlockP, size );			//	Compare them
			if ( err )																		//€€ Debugging code
				err = CmpBlock( (volumeBitMap->buffer + i*Blk_Size), vbmBlockP, size );
			
			if ( err != noErr )
			{ 
				RcdError( GPtr, E_VBMDamaged );
				GPtr->VIStat = GPtr->VIStat | S_VBM;										//	didn't match, mark it damaged
				return( E_VBMDamaged );
			}
		}
		
		//	Must have done a successful comparrison
		volumeBitMap->bitMapRecord[whichBuffer].isAMatch = true;
	}
	
	return( noErr );
}


/*------------------------------------------------------------------------------

Routine:	CmpMDB - (Compare Master Directory Block)

Function:	Compares the scavenger MDB info with the MDB on disk.
			
Input:		GPtr			-	pointer to scavenger global area

Output:		CmpMDB			- 	function result:
									0 = no error
									n = error
			GPtr->VIStat	-	S_MDB flag set in VIStat if MDB is damaged.
------------------------------------------------------------------------------*/

int CmpMDB( SGlobPtr GPtr )
{
	OSErr					result;
	short					i;
	ExtendedVCB				*calculatedVCB;
	FCB						*fcbP;
	MasterDirectoryBlock	*mdbP;

	//	set up

	calculatedVCB = GPtr->calculatedVCB;
	GPtr->TarID = MDB_FNum;										/* target = MDB */
	
	result = GetVBlk( GPtr, MDB_BlkN, (void**)&mdbP );
	if ( result != noErr )
		return( result );										/* could't get MDB */

	//
	//	compare VCB info with MDB
	//	Instead of comparing blocks, let's compare the fields, so we can reduce the dependencies
	//
	if	( mdbP->drSigWord		!=	calculatedVCB->vcbSigWord )				goto MDBDamaged;
	if	( mdbP->drCrDate		!=	calculatedVCB->vcbCrDate )				goto MDBDamaged;
	if	( mdbP->drLsMod			!=	calculatedVCB->vcbLsMod )				goto MDBDamaged;
	if	( mdbP->drAtrb			!=	(UInt16)calculatedVCB->vcbAtrb )		goto MDBDamaged;
	if	( mdbP->drVBMSt			!=	calculatedVCB->vcbVBMSt )				goto MDBDamaged;
	if	( mdbP->drNmAlBlks		!=	calculatedVCB->vcbNmAlBlks )			goto MDBDamaged;
	if	( mdbP->drClpSiz		!=	calculatedVCB->vcbClpSiz )				goto MDBDamaged;
	if	( mdbP->drAlBlSt		!=	calculatedVCB->vcbAlBlSt )				goto MDBDamaged;
	if	( mdbP->drNxtCNID		!=	calculatedVCB->vcbNxtCNID )				goto MDBDamaged;
	if	( CmpBlock( mdbP->drVN, calculatedVCB->vcbVN, mdbP->drVN[0]+1 ) )	goto MDBDamaged;
	goto ContinueChecking;

MDBDamaged:
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark it damaged */
		WriteError ( E_MDBDamaged, 1, 0 );
		return( noErr );
	}
	
ContinueChecking:
	result = CmpBlock( &mdbP->drVolBkUp, &calculatedVCB->vcbVolBkUp, LenHFSMDB );	/* compare HFS part */
	if ( result != 0 )
	{ 
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_MDBDamaged, 2, 0 );
		return( noErr );
	}

	//
	//	compare extent file allocation info with MDB
	//
			
	fcbP = (FCB *) ( GPtr->FCBAPtr + kCalculatedExtentRefNum );	/* compare PEOF for extent file */
	if ( mdbP->drXTFlSize != fcbP->fcbPLen )
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_MDBDamaged, 3, 0 );
		return( noErr );
	}
	for ( i = 0; i < GPtr->numExtents; i++ )					/* compare extent rec for extent file */
	{
		if ( (mdbP->drXTExtRec[i].startBlock != fcbP->fcbExtRec[i].startBlock) || (mdbP->drXTExtRec[i].blockCount != fcbP->fcbExtRec[i].blockCount) )
		{
			GPtr->VIStat = GPtr->VIStat | S_MDB;				/* didn't match, mark MDB damaged */
			WriteError ( E_MDBDamaged, 4, 0 );
			return( noErr );
		}
	}

	//
	//	compare catalog file allocation info with MDB
	//
			
	fcbP = (FCB *) (GPtr->FCBAPtr + kCalculatedCatalogRefNum);	/* compare PEOF for catalog file */
	if ( mdbP->drCTFlSize != fcbP->fcbPLen )
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_MDBDamaged, 5, 0 );
		return( noErr );
	}
	for ( i = 0; i < GPtr->numExtents; i++ )					/* compare extent rec for catalog file */
	{
		if ( (mdbP->drCTExtRec[i].startBlock != fcbP->fcbExtRec[i].startBlock) || (mdbP->drCTExtRec[i].blockCount != fcbP->fcbExtRec[i].blockCount) )
		{
			GPtr->VIStat = GPtr->VIStat | S_MDB;				/* didn't match, mark MDB damaged */
			WriteError ( E_MDBDamaged, 6, 0 );
			return( noErr );
		}
	}
		
	return( noErr );
}	/* end CmpMDB */



/*------------------------------------------------------------------------------

Routine:	CompareVolumeHeader - (Compare VolumeHeader Block)

Function:	Compares the scavenger VolumeHeader info with the VolumeHeader on disk.
			
Input:		GPtr			-	pointer to scavenger global area

Output:		CmpMDB			- 	function result:
									0 = no error
									n = error
			GPtr->VIStat	-	S_MDB flag set in VIStat if MDB is damaged.
------------------------------------------------------------------------------*/

OSErr CompareVolumeHeader( SGlobPtr GPtr )
{
	OSErr					err;
	SInt16					i;
	ExtendedVCB				*calculatedVCB;
	FCB						*fcbP;
	VolumeHeader			*volumeHeader;
	MasterDirectoryBlock	*mdb;
	UInt32					hfsPlusIOPosOffset;

	//	set up

	calculatedVCB = GPtr->calculatedVCB;
	GPtr->TarID = MDB_FNum;								/* target = MDB */
	
	err = GetVBlk( GPtr, calculatedVCB->vcbAlBlSt+2, (void**)&volumeHeader );	//	VH is 3rd sector in
	ReturnIfError( err );

	//
	//	Instead of comparing blocks, let's compare the fields, so we can reduce the dependencies
	//
	hfsPlusIOPosOffset	=	calculatedVCB->hfsPlusIOPosOffset;

	if ( kHFSPlusSigWord						!=	calculatedVCB->vcbSigWord )			goto VolumeHeaderDamaged;
	if ( volumeHeader->encodingsBitmap.lo		!=	calculatedVCB->encodingsBitmap.lo )	goto VolumeHeaderDamaged;
	if ( volumeHeader->encodingsBitmap.hi		!=	calculatedVCB->encodingsBitmap.hi )	goto VolumeHeaderDamaged;
	if ( hfsPlusIOPosOffset/512					!=	calculatedVCB->vcbAlBlSt )			goto VolumeHeaderDamaged;
	if ( volumeHeader->createDate				!=	calculatedVCB->vcbCrDate )			goto VolumeHeaderDamaged;
	if ( volumeHeader->modifyDate				!=	calculatedVCB->vcbLsMod )			goto VolumeHeaderDamaged;
	if ( volumeHeader->backupDate				!=	calculatedVCB->vcbVolBkUp )			goto VolumeHeaderDamaged;
	if ( volumeHeader->checkedDate				!=	calculatedVCB->checkedDate )		goto VolumeHeaderDamaged;
	if ( 0										!=	calculatedVCB->vcbVBMSt )			goto VolumeHeaderDamaged;
	if ( 0										!=	calculatedVCB->vcbAllocPtr )		goto VolumeHeaderDamaged;
	if ( volumeHeader->rsrcClumpSize			!=	calculatedVCB->vcbClpSiz )			goto VolumeHeaderDamaged;
	if ( volumeHeader->extentsFile.clumpSize	!=	calculatedVCB->vcbXTClpSiz )		goto VolumeHeaderDamaged;
	if ( volumeHeader->catalogFile.clumpSize	!=	calculatedVCB->vcbCTClpSiz )		goto VolumeHeaderDamaged;
	if ( volumeHeader->allocationFile.clumpSize	!=	calculatedVCB->allocationsClumpSize )	goto VolumeHeaderDamaged;
	if ( volumeHeader->nextCatalogID			!=	calculatedVCB->vcbNxtCNID )			goto VolumeHeaderDamaged;
	if ( volumeHeader->writeCount				!=	calculatedVCB->vcbWrCnt )			goto VolumeHeaderDamaged;
	if ( volumeHeader->fileCount				!=	calculatedVCB->vcbFilCnt )			goto VolumeHeaderDamaged;
	if ( volumeHeader->folderCount				!=	calculatedVCB->vcbDirCnt )			goto VolumeHeaderDamaged;
	if ( volumeHeader->nextAllocation			!=	calculatedVCB->nextAllocation )		goto VolumeHeaderDamaged;
	if ( volumeHeader->totalBlocks				!=	calculatedVCB->totalBlocks )		goto VolumeHeaderDamaged;
	if ( volumeHeader->freeBlocks				!=	calculatedVCB->freeBlocks )			goto VolumeHeaderDamaged;
	if ( volumeHeader->blockSize				!=	calculatedVCB->blockSize )			goto VolumeHeaderDamaged;
	if ( (UInt16)volumeHeader->attributes		!=	calculatedVCB->vcbAtrb )			goto VolumeHeaderDamaged;
	if ( CmpBlock( volumeHeader->finderInfo, calculatedVCB->vcbFndrInfo, sizeof(calculatedVCB->vcbFndrInfo) ) )	goto VolumeHeaderDamaged;
	goto ContinueChecking;
	
		
VolumeHeaderDamaged:
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark it damaged */
		WriteError ( E_VolumeHeaderDamaged, 1, 0 );
		return( noErr );

ContinueChecking:

	//
	//	compare extent file allocation info with VolumeHeader
	//
			
	fcbP = (FCB *) ( GPtr->FCBAPtr + kCalculatedExtentRefNum );	/* compare PEOF for extent file */
	if ( volumeHeader->extentsFile.totalBlocks * calculatedVCB->blockSize != fcbP->fcbPLen )
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_VolumeHeaderDamaged, 3, 0 );
		return( noErr );
	}
	for ( i=0; i < GPtr->numExtents; i++ )						/* compare extent rec for extent file */
	{
		if ( (volumeHeader->extentsFile.extents[i].startBlock != GPtr->extendedExtentsFCB->extents[i].startBlock) || (volumeHeader->extentsFile.extents[i].blockCount != GPtr->extendedExtentsFCB->extents[i].blockCount) )
		{
			GPtr->VIStat = GPtr->VIStat | S_MDB;				/* didn't match, mark MDB damaged */
			WriteError ( E_VolumeHeaderDamaged, 4, 0 );
			return( noErr );
		}
	}

	//
	//	compare catalog file allocation info with MDB
	//
			
	fcbP = (FCB *) (GPtr->FCBAPtr + kCalculatedCatalogRefNum);	/* compare PEOF for catalog file */
	if ( volumeHeader->catalogFile.totalBlocks * calculatedVCB->blockSize != fcbP->fcbPLen )
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_VolumeHeaderDamaged, 5, 0 );
		return( noErr );
	}
	for ( i=0; i < GPtr->numExtents; i++ )						/* compare extent rec for extent file */
	{
		if ( (volumeHeader->catalogFile.extents[i].startBlock != GPtr->extendedCatalogFCB->extents[i].startBlock) || (volumeHeader->catalogFile.extents[i].blockCount != GPtr->extendedCatalogFCB->extents[i].blockCount) )
		{
			GPtr->VIStat = GPtr->VIStat | S_MDB;				/* didn't match, mark MDB damaged */
			WriteError ( E_VolumeHeaderDamaged, 6, 0 );
			return( noErr );
		}
	}


	//
	//	compare bitmap file allocation info with MDB
	//
			
	fcbP = (FCB *) (GPtr->FCBAPtr + kCalculatedAllocationsRefNum);	/* compare PEOF for catalog file */
	if ( volumeHeader->allocationFile.totalBlocks * calculatedVCB->blockSize != fcbP->fcbPLen )
	{
		GPtr->VIStat = GPtr->VIStat | S_MDB;					/* didn't match, mark MDB damaged */
		WriteError ( E_VolumeHeaderDamaged, 7, 0 );
		return( noErr );
	}
	for ( i=0; i < GPtr->numExtents; i++ )						/* compare extent rec for extent file */
	{
		if ( (volumeHeader->allocationFile.extents[i].startBlock != GPtr->extendedAllocationsFCB->extents[i].startBlock) || (volumeHeader->allocationFile.extents[i].blockCount != GPtr->extendedAllocationsFCB->extents[i].blockCount) )
		{
			GPtr->VIStat = GPtr->VIStat | S_MDB;				/* didn't match, mark MDB damaged */
			WriteError ( E_VolumeHeaderDamaged, 8, 0 );
			return( noErr );
		}
	}

	return( noErr );
}
