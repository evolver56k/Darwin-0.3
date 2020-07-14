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
	File:		SVerify1.c

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

	 <HFS23>	 12/8/97	DSH		Override MountChecks vcbNxtCNID with our calculated ID.
	 <HFS22>	 12/2/97	DSH		maskBit in RepairOrder
	 <HFS21>	11/18/97	DSH		CustomIconCheck() moved in OrphanedFileCheck() moved out to
									Repair code.
	 <HFS20>	 11/4/97	DSH		Add MountCheck check
	 <HFS19>	10/21/97	DSH		Call InvalidateCalculatedVolumeBitMap()
	 <HFS18>	 10/6/97	DSH		Added BuildExtentKey()
	 <HFS17>	 9/18/97	DSH		Add support for the TextEncodings bitmap in the calculatedVCB.
	 <HFS16>	 9/17/97	DSH		Wrapperless HFS+ volume support.
	 <HFS15>	  9/5/97	DSH		Use idBlock when getting VH in
									CreateAttributesBTreeControlBlock()
	 <HFS14>	  9/4/97	msd		Add routine to check the attributes B-tree. Fixed some illegal
									casting of structures (as opposed to pointers to structures).
									Add CreateAttributesBTreeControlBlock.
	 <HFS13>	  9/2/97	DSH		VolumeHeader is now 3rd sector in, and AlternateVolumeHeader is
									2nd to last sector.
	 <HFS12>	 8/18/97	DSH		Check VolumeHeader version if HFSPlus disk & integrate
									ValidHFSRecord() code.
	 <HFS11>	 8/11/97	DSH		CheckFileExtents(forkType) SInt8 to UInt8
	 <HFS10>	 6/27/97	DSH		To avoid unicode conversion, we get the name of the volume for
									the VCB from the MDB.
	  <HFS9>	 6/26/97	DSH		Eliminate unicode dependencies
	  <HFS8>	  6/2/97	DSH		Handle extended attributes and startup B*Tree files.
	  <HFS7>	 5/19/97	DSH		If we get a btNotFound error when searching for bad blocks,
									reset it to noErr.
	  <HFS6>	 4/28/97	DSH		Added clause check for the bad block entry in the extent
									overflow file, and mark them off in the bitmap.
	  <HFS5>	 4/25/97	DSH		Eliminating Unicode dependency.
	  <HFS4>	  4/4/97	DSH		Fixed bug in Volume BitMap size calculation & removed DebugStrs.
	  <HFS3>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS2>	 3/27/97	DSH		More preflighting was needed to calculate itemsToProcess for
								CheckDisk, added CreateExtentsBTreeControlBlock(), and
								CreateCatalogBTreeControlBlock().
	 <HFS1>	 3/17/97	DSH		Initial Check-In
*/

#include	"ScavDefs.h"
#include	"Prototypes.h"
#include	"BTreeScanner.h"

#include 	"DFALowMem.h"

SInt32 CompareExtentKeysPlus( const LargeExtentKey *searchKey, const LargeExtentKey *trialKey );
SInt32 CompareExtentKeys( const SmallExtentKey *searchKey, const SmallExtentKey *trialKey );

//	internal routine prototypes

static	int	RcdValErr( SGlobPtr GPtr, OSErr type, UInt32 correct, UInt32 incorrect, CatalogNodeID parid );

static	int	RcdFThdErr( SGlobPtr GPtr, UInt32 fid );

static	int	RcdNoDirErr( SGlobPtr GPtr, UInt32 did );
		
static	int	RcdBadClump( SGlobPtr GPtr, OSErr type, UInt32 incorrect, UInt32 parid );

static	int	RcdNameLockedErr( SGlobPtr GPtr, OSErr type, UInt32 incorrect );

static	int	RcdCustomIconErr( SGlobPtr GPtr, OSErr type, UInt32 incorrect );
	
static	OSErr	RcdOrphanedExtentErr ( SGlobPtr GPtr, SInt16 type, void *theKey );

static	OSErr	CustomIconCheck ( SGlobPtr GPtr, CatalogNodeID folderID, UInt16 frFlags );

static	OSErr	CheckNodesFirstOffset( SGlobPtr GPtr, BTreeControlBlock *btcb );


/*------------------------------------------------------------------------------

Function:	IVChk - (Initial Volume Check)

Function:	Performs an initial check of the volume to be scavenged to confirm
			that the volume can be accessed and that it is a HFS/HFS+ volume.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		IVChk	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

#pragma segment InitialVolumeCheck

OSErr IVChk( SGlobPtr GPtr )
{
	#define					kBitsPerSector	4096
	#define					kLog2SectorSize	9
	UInt32					bitMapSizeInSectors;
	OSErr					err;
	MasterDirectoryBlock	*alternateMDB;
	VolumeHeader			*alternateVolumeHeader;
	UInt32					numABlks;
	UInt32					alternateBlockLocation;
	UInt32					minABlkSz;
	UInt32					totalSectors;
	UInt32					maxNumberOfAllocationBlocks;
	UInt32					realAllocationBlockSize;
	UInt32					realTotalBlocks;
	UInt32					hfsBlockSize;
	UInt32					hfsBlockCount;
	UInt16					i;
	UInt32					hfsPlusIOPosOffset;
	ExtendedVCB				*calculatedVCB = (ExtendedVCB *)GPtr->calculatedVCB;
	FCB						*fcb;
	BTreeControlBlock		*btcb;
	
	//  Set up
	
	GPtr->TarID = AMDB_FNum;									//	target = alt MDB
		
	// Determine volume size
	totalSectors = CGetVolSiz( GPtr->DrvPtr );
	if ( totalSectors < 3 )
		return( 123 );
	
	//	Get the Alternate MDB, 2nd to last block on disk
	//	On HFS+ disks this is still the HFS wrapper altMDB
	//	On HFS+ wrapperless disks, it's the AltVH
	alternateBlockLocation = totalSectors - 2;
	err = GetBlock_FSGlue( gbDefault, alternateBlockLocation, (Ptr *)&alternateMDB, calculatedVCB->vcbVRefNum, calculatedVCB );	//	no Flags, MDB is always block
	ReturnIfError( err );

	if ( alternateMDB->drSigWord == kHFSPlusSigWord )			//	Is it a wrapperless HFS+ volume
	{
		alternateVolumeHeader	= (VolumeHeader *)alternateMDB;
		GPtr->pureHFSPlusVolume	= true;
		GPtr->isHFSPlus			= true;
	}
	else if ( alternateMDB->drSigWord == kHFSSigWord )			//  Deturmine if its an HFS volume
	{
		GPtr->pureHFSPlusVolume	= false;

		if ( alternateMDB->drEmbedSigWord == kHFSPlusSigWord )	//	Wrappered HFS+ volume
		{
			GPtr->isHFSPlus	= true;
		}
		else													//	plain old HFS
		{
			GPtr->isHFSPlus	= false;
			calculatedVCB->allocationsRefNum	= 0;
			calculatedVCB->attributesRefNum		= 0;
		}
	}
	else
	{
		err = R_BadSig;											//	doesn't bear the HFS signature
		goto ReleaseAndBail;
	}
	
	//
	//	If this is an HFS+ disk
	//
	
	if ( GPtr->isHFSPlus == true )
	{
		WriteMsg( GPtr, M_CheckingHFSPlusVolume, kStatusMessage );

		GPtr->numExtents			= kLargeExtentDensity;
		calculatedVCB->vcbSigWord	= kHFSPlusSigWord;
		
		//	Read the HFS+ VolumeHeader
		if ( GPtr->pureHFSPlusVolume )
		{
			hfsPlusIOPosOffset	=	0;			//	alternateBlockLocation is already set up
			HFSBlocksFromTotalSectors( totalSectors, &hfsBlockSize, (UInt16*)&hfsBlockCount );
		}
		else
		{
			hfsPlusIOPosOffset		=	(alternateMDB->drEmbedExtent.startBlock * alternateMDB->drAlBlkSiz) + ( alternateMDB->drAlBlSt * 512 );
			alternateBlockLocation	= (hfsPlusIOPosOffset / 512) + (alternateMDB->drEmbedExtent.blockCount * (alternateMDB->drAlBlkSiz / 512)) - 2;	//	2nd to last sector
			
			err = GetBlock_glue( gbDefault, alternateBlockLocation, (Ptr*)&alternateVolumeHeader, calculatedVCB->vcbVRefNum, calculatedVCB );	//	Alt VH is always last sector of HFS+ partition
			if ( err != noErr )	goto JustBail;

			totalSectors	= alternateMDB->drEmbedExtent.blockCount * ( alternateMDB->drAlBlkSiz / Blk_Size );
			hfsBlockSize	= alternateMDB->drAlBlkSiz;
			hfsBlockCount	= alternateMDB->drNmAlBlks;
		}
		
		err = ValidVolumeHeader( alternateVolumeHeader );
		
		//	If the alternate VolumeHeader is bad, just use the real VolumeHeader
		if ( err != noErr )
		{
			alternateBlockLocation	= (hfsPlusIOPosOffset / 512) + 2;
			err = RelBlock_FSGlue( (Ptr)alternateVolumeHeader, 0 );
			if ( err != noErr )	goto JustBail;
			err = GetBlock_glue( gbDefault, (hfsPlusIOPosOffset / 512) + 2, (Ptr*)&alternateVolumeHeader, calculatedVCB->vcbVRefNum, calculatedVCB );	//	VH is always 3rd sector of HFS+ partition
			if ( err != noErr )	goto JustBail;
			err = ValidVolumeHeader( alternateVolumeHeader );
			if ( err != noErr )
			{	//	If the VolumeHeader version is not kHFSPlusVersion assume DFA cannot fix it.
				if ( alternateVolumeHeader->version != kHFSPlusVersion )
				{
					err				= E_VolumeHeaderTooNew;						//	Version of VolumeHeader not supported by DFA
					GPtr->TarID		= alternateVolumeHeader->version;			//	RcdError, also displays the values in TarID, and TarBlock
					GPtr->TarBlock	= kHFSPlusVersion;
					RcdError( GPtr, E_VolumeHeaderTooNew );
				}
				else
				{
					err = R_BadVolumeHeader;									//	doesn't bear the HFS signature
				}
				goto ReleaseAndBail;
			}
		}
	
		//	Further populate the VCB with VolumeHeader info
		calculatedVCB->vcbAlBlSt			= hfsPlusIOPosOffset / 512;
		calculatedVCB->hfsPlusIOPosOffset	= hfsPlusIOPosOffset;

		maxNumberOfAllocationBlocks	= 0xFFFFFFFF;
		realAllocationBlockSize		= alternateVolumeHeader->blockSize;
		realTotalBlocks				= alternateVolumeHeader->totalBlocks;
		calculatedVCB->vcbNxtCNID	= alternateVolumeHeader->nextCatalogID;
		calculatedVCB->vcbCrDate	= alternateVolumeHeader->createDate;
		
		if ( alternateVolumeHeader->attributesFile.totalBlocks == 0 )
			calculatedVCB->attributesRefNum	= 0;

		//	Make sure the Extents B-Tree is set to use 16-bit key lengths.  We access it before completely setting
		//	up the control block.
		fcb = GetFileControlBlock(calculatedVCB->extentsRefNum);
		btcb = (BTreeControlBlock *) fcb->fcbBTCBPtr;
		btcb->attributes |= kBTBigKeysMask;
	}
	else	//	It's an HFS disk
	{
		WriteMsg( GPtr, M_CheckingHFSVolume, kStatusMessage );

		GPtr->numExtents			= kSmallExtentDensity;
		calculatedVCB->vcbSigWord	= alternateMDB->drSigWord;
		totalSectors				= alternateBlockLocation;
		maxNumberOfAllocationBlocks	= 0xFFFF;
		calculatedVCB->vcbNxtCNID	= alternateMDB->drNxtCNID;			//	set up next file ID, CheckBTreeKey makse sure we are under this value
		calculatedVCB->vcbCrDate	= alternateMDB->drCrDate;

		realAllocationBlockSize		= alternateMDB->drAlBlkSiz;
		realTotalBlocks				= alternateMDB->drNmAlBlks;
		hfsBlockSize				= alternateMDB->drAlBlkSiz;
		hfsBlockCount				= alternateMDB->drNmAlBlks;
	}
	
	
	GPtr->idSector	= alternateBlockLocation;							//	Location of ID block, AltMDB, MDB, AltVH or VH
	GPtr->TarBlock	= alternateBlockLocation;							//	target block = alt MDB

	//  verify volume allocation info 
 	numABlks = totalSectors;
 	minABlkSz = Blk_Size;												//	init minimum ablock size
	for( i = 2; numABlks > maxNumberOfAllocationBlocks; i++ )			//	loop while #ablocks won't fit
	{
		minABlkSz = i * Blk_Size;										//	jack up minimum
		numABlks  = alternateBlockLocation / i;							//	recompute #ablocks, assuming this size
	}
	
	if ((realAllocationBlockSize >= minABlkSz) && (realAllocationBlockSize <= Max_ABSiz) && ((realAllocationBlockSize % Blk_Size) == 0))
	{
		calculatedVCB->vcbAlBlkSiz	= hfsBlockSize;
		calculatedVCB->blockSize	= realAllocationBlockSize;
		numABlks = totalSectors / ( realAllocationBlockSize / Blk_Size );	//	max # of alloc blks
	}
	else
	{
		RcdError( GPtr, E_ABlkSz );
		err = E_ABlkSz;													//	bad allocation block size
		goto ReleaseAndBail;
	}		
	
	//	Calculate the volume bitmap size
	bitMapSizeInSectors	= ( numABlks + kBitsPerSector - 1 ) / kBitsPerSector;			//	VBM size in blocks
	
	calculatedVCB->vcbNmAlBlks	= hfsBlockCount;
	calculatedVCB->vcbFreeBks	= LongToShort( realTotalBlocks );
	calculatedVCB->totalBlocks	= realTotalBlocks;
	calculatedVCB->freeBlocks	= realTotalBlocks;
	
	//	Only do these tests on HFS volumes, since they are either irrellivent
	//	or, getting the VolumeHeader would have already failed.
	if ( GPtr->isHFSPlus == false )
	{
		//€€	Calculate the validaty of HFS+ Allocation blocks, I think realTotalBlocks == numABlks
		numABlks = (totalSectors - 3 - bitMapSizeInSectors) / (realAllocationBlockSize / Blk_Size);	//	actual # of alloc blks

		if ( realTotalBlocks > numABlks )
		{
			RcdError( GPtr, E_NABlks );
			err = E_NABlks;								//	invalid number of allocation blocks
			goto ReleaseAndBail;
		}

		if ( alternateMDB->drVBMSt <= MDB_BlkN )
		{
			RcdError(GPtr,E_VBMSt);
			err = E_VBMSt;								//	invalid VBM start block
			goto ReleaseAndBail;
		}	
		calculatedVCB->vcbVBMSt = alternateMDB->drVBMSt;
		
		if (alternateMDB->drAlBlSt < (alternateMDB->drVBMSt + bitMapSizeInSectors))
		{
			RcdError(GPtr,E_ABlkSt);
			err = E_ABlkSt;								//	invalid starting alloc block
			goto ReleaseAndBail;
		}
		calculatedVCB->vcbAlBlSt	= alternateMDB->drAlBlSt;
	}
	
	//
	//	allocate memory for DFA's volume bit map
	//
	{
		Handle	h;
		UInt32	safeTempMemSize;
		UInt32	numberOfBitMapBuffers;									//	how many buffers do we need to read the bitmap
		UInt32	bitmapSizeInBytes;										//	 round up to the nearest sector or allocation block
		UInt32	bitsPerBlock		= realAllocationBlockSize * 8;
		if ( GPtr->isHFSPlus == true )
			bitmapSizeInBytes	= ( ( alternateVolumeHeader->totalBlocks + bitsPerBlock - 1 ) / bitsPerBlock ) * realAllocationBlockSize;	//	round up to the nearest allocation block
		else
			bitmapSizeInBytes = (((totalSectors / (realAllocationBlockSize >> kLog2SectorSize)) + kBitsPerSector-1) / kBitsPerSector) * Blk_Size;	//	round up to the nearest sector
		
		safeTempMemSize = CalculateSafePhysicalTempMem();
		safeTempMemSize = ( safeTempMemSize / Blk_Size ) * Blk_Size;	//	keep it a multiple of Blk_Size bytes
		
		if ( bitmapSizeInBytes < safeTempMemSize )
			safeTempMemSize = bitmapSizeInBytes;
			
		//	split it into portions for large bit maps
		numberOfBitMapBuffers = ( bitmapSizeInBytes + safeTempMemSize - 1 ) / safeTempMemSize;
		
		GPtr->volumeBitMapPtr = (VolumeBitMapHeader *) NewPtrClear( numberOfBitMapBuffers * sizeof(BitMapRec) + sizeof(VolumeBitMapHeader) );
		
		//	Allocate space from the heap if our buffer is under 64K
		if ( safeTempMemSize <= 64*1024 )
		{
			h = NewHandleClear( safeTempMemSize );
		}
		else	//	Allocate it from TempMem
		{
			h = TempNewHandle( safeTempMemSize, &err );
			if ( err )
				h = nil;
		}

		if ( h == nil ) 
		{
			err = R_NoMem;												//	not enough memory
			goto ReleaseAndBail;
		}
	
		HLock( h );
		GPtr->volumeBitMapPtr->buffer = *h;
		
		//	Set up the Volume bit map structure fields, all other fields are 0/false
		ClearMemory ( GPtr->volumeBitMapPtr->buffer, safeTempMemSize );
		GPtr->volumeBitMapPtr->bitMapSizeInBytes	= bitmapSizeInBytes;
		GPtr->volumeBitMapPtr->numberOfBuffers		= numberOfBitMapBuffers;
		GPtr->volumeBitMapPtr->bufferSize			= safeTempMemSize;

		InvalidateCalculatedVolumeBitMap( GPtr );	//	no buffer read yet
	}

ReleaseAndBail:
	if ( GPtr->isHFSPlus )
		(void) RelBlock_FSGlue( (Ptr)alternateVolumeHeader, 0 );		//	release it, to unlock buffer
JustBail:
	if ( GPtr->pureHFSPlusVolume == false )
		(void) RelBlock_FSGlue( (Ptr)alternateMDB, 0 );					//	release it, to unlock buffer
	
	return( err );		
}


/*------------------------------------------------------------------------------

Function:	CreateExtentsBTreeControlBlock

Function:	Create the calculated ExtentsBTree Control Block
			
Input:		GPtr	-	pointer to scavenger global area

Output:				-	0	= no error
						n 	= error code 
------------------------------------------------------------------------------*/

OSErr	CreateExtentsBTreeControlBlock( SGlobPtr GPtr )
{
	OSErr					err;
	SInt32					size;
	UInt32					numABlks;
	HeaderRec				*header;
	BTreeControlBlock		*btcb		= GPtr->calculatedExtentsBTCB;
	Boolean					isHFSPlus	= GPtr->isHFSPlus;

	//	Set up
	GPtr->TarID		= kHFSExtentsFileID;										//	target = extent file
	GPtr->TarBlock	= kHeaderNodeNum;											//	target block = header node
 
	//
	//	check out allocation info for the Extents File 
	//
	if ( isHFSPlus == true )
	{
		VolumeHeader			*volumeHeader;
		
		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );			//	get the alternate VH
		ReturnIfError( err );
	
		BlockMoveData( (Ptr)volumeHeader->extentsFile.extents, (Ptr)GPtr->extendedExtentsFCB->extents, sizeof(LargeExtentRecord) );
		
		err = CheckFileExtents( GPtr, kHFSExtentsFileID, 0, false, (void *)GPtr->extendedExtentsFCB->extents, &numABlks );	//	check out extent info
		ReturnIfError( err );													//	error, invalid file allocation
	
		if ( volumeHeader->extentsFile.totalBlocks != numABlks )				//	check out the PEOF
		{
			RcdError( GPtr, E_ExtPEOF );
			return( E_ExtPEOF );												//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedExtentsFCB->fcbEOF  = volumeHeader->extentsFile.logicalSize.lo;							//	Set Extents tree's LEOF
			GPtr->calculatedExtentsFCB->fcbPLen = volumeHeader->extentsFile.totalBlocks * volumeHeader->blockSize;	//	Set Extents tree's PEOF
		}

		//
		//	Set up the minimal BTreeControlBlock structure
		//
		
		//	Read the BTreeHeader from disk & also validate it's node size.
		err = GetBTreeHeader( GPtr, kCalculatedExtentRefNum, &header );
		ReturnIfError( err );
		
		btcb->maxKeyLength		= kLargeExtentKeyMaximumLength;					//	max key length
		btcb->keyCompareProc	= (void *)CompareExtentKeysPlus;
		btcb->attributes		|=kBTBigKeysMask;								//	HFS+ Extent files have 16-bit key length
		btcb->leafRecords		= header->leafRecords;
		btcb->treeDepth			= header->treeDepth;
		btcb->rootNode			= header->rootNode;
		btcb->firstLeafNode		= header->firstLeafNode;
		btcb->lastLeafNode		= header->lastLeafNode;

		btcb->nodeSize			= header->nodeSize;
		btcb->totalNodes		= ( GPtr->calculatedExtentsFCB->fcbPLen / btcb->nodeSize );
		btcb->freeNodes			= btcb->totalNodes;								//	start with everything free

		//	Make sure the header nodes size field is correct by looking at the 1st record offset
		err	= CheckNodesFirstOffset( GPtr, btcb );
		if ( (err != noErr) && (btcb->nodeSize != 1024) )		//	default HFS+ Extents node size is 1024
		{
			btcb->nodeSize			= 1024;
			btcb->totalNodes		= ( GPtr->calculatedExtentsFCB->fcbPLen / btcb->nodeSize );
			btcb->freeNodes			= btcb->totalNodes;								//	start with everything free
			
			err	= CheckNodesFirstOffset( GPtr, btcb );
			ReturnIfError( err );
			
			GPtr->EBTStat |= S_BTH;								//	update the Btree header
		}
	}
	else	//	HFS
	{
		MasterDirectoryBlock	*alternateMDB;
		
		err = GetVBlk( GPtr, GPtr->idSector, (void**)&alternateMDB );			//	get the alternate MDB
		ReturnIfError( err );
	
		BlockMoveData( (Ptr)alternateMDB->drXTExtRec, (Ptr)GPtr->calculatedExtentsFCB->fcbExtRec, sizeof(SmallExtentRecord) );
		
		err = CheckFileExtents( GPtr, kHFSExtentsFileID, 0, false, (void *)&GPtr->calculatedExtentsFCB->fcbExtRec[0], &numABlks );	/* check out extent info */	
		ReturnIfError( err );													//	error, invalid file allocation
	
		if (alternateMDB->drXTFlSize != (numABlks * GPtr->calculatedVCB->blockSize))//	check out the PEOF
		{
			RcdError(GPtr,E_ExtPEOF);
			return(E_ExtPEOF);													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedExtentsFCB->fcbPLen = alternateMDB->drXTFlSize;		//	set up PEOF and EOF in FCB
			GPtr->calculatedExtentsFCB->fcbEOF = GPtr->calculatedExtentsFCB->fcbPLen;
		}

		//
		//	Set up the minimal BTreeControlBlock structure
		//
			
		//	Read the BTreeHeader from disk & also validate it's node size.
		err = GetBTreeHeader( GPtr, kCalculatedExtentRefNum, &header );
		ReturnIfError( err );

		btcb->maxKeyLength	= kSmallExtentKeyMaximumLength;						//	max key length
		btcb->keyCompareProc = (void *)CompareExtentKeys;
		btcb->leafRecords	= header->leafRecords;
		btcb->treeDepth		= header->treeDepth;
		btcb->rootNode		= header->rootNode;
		btcb->firstLeafNode	= header->firstLeafNode;
		btcb->lastLeafNode	= header->lastLeafNode;
		
		btcb->nodeSize		= header->nodeSize;
		btcb->totalNodes	= (GPtr->calculatedExtentsFCB->fcbPLen / btcb->nodeSize );
		btcb->freeNodes		= btcb->totalNodes;									//	start with everything free

		//	Make sure the header nodes size field is correct by looking at the 1st record offset
		err	= CheckNodesFirstOffset( GPtr, btcb );
		ReturnIfError( err );
	}
	

	//
	//	set up our DFA extended BTCB area.  Will we have enough memory on all HFS+ volumes.
	//
	btcb->refCon = (UInt32) NewPtrClear( sizeof(BTreeExtensionsRec) );			// allocate space for our BTCB extensions
	if ( btcb->refCon == (UInt32) nil )
		return( R_NoMem );														//	no memory for BTree bit map
	size = (btcb->totalNodes + 7) / 8;											//	size of BTree bit map
	((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr = NewPtrClear(size);			//	get precleared bitmap
	if ( ((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr == nil )
	{
		return( R_NoMem );														//	no memory for BTree bit map
	}

	((BTreeExtensionsRec*)btcb->refCon)->BTCBMSize			= size;				//	remember how long it is
	((BTreeExtensionsRec*)btcb->refCon)->realFreeNodeCount	= header->freeNodes;//	keep track of real free nodes for progress
	
	return( noErr );
}


/*------------------------------------------------------------------------------

Function:	CheckNodesFirstOffset

Function:	Minimal check verifies that the 1st offset is within bounds.  If it's not
			the nodeSize may be wrong.  In the future this routine could be modified
			to try different size values until one fits.
			
------------------------------------------------------------------------------*/
#define GetRecordOffset(btreePtr,node,index)		(*(short *) ((UInt8 *)(node) + (btreePtr)->nodeSize - ((index) << 1) - kOffsetSize))
static	OSErr	CheckNodesFirstOffset( SGlobPtr GPtr, BTreeControlBlock *btcb )
{
	NodeRec		nodeRec;
	UInt16		offset;
	OSErr		err;
			
	err = GetNode( btcb, kHeaderNodeNum, &nodeRec );
	
	if ( err == noErr )
	{
		offset	= GetRecordOffset( btcb, (NodeDescPtr)nodeRec.buffer, 0 );
		if ( (offset < sizeof (BTNodeDescriptor)) ||			// offset < minimum
			 (offset & 1) ||									// offset is odd
			 (offset >= btcb->nodeSize) )						// offset beyond end of node
		{
			err	= fsBTInvalidNodeErr;
		}
	}
	
	if ( err != noErr )
		RcdError( GPtr, E_InvalidNodeSize );

	return( err );
}



/*------------------------------------------------------------------------------

Function:	ExtBTChk - (Extent BTree Check)

Function:	Verifies the extent BTree structure.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		ExtBTChk	-	function result:			
								0	= no error
								n 	= error code 
------------------------------------------------------------------------------*/

OSErr ExtBTChk( SGlobPtr GPtr )
{
	OSErr					err;

	//	Set up
	GPtr->TarID		= kHFSExtentsFileID;										//	target = extent file
	GPtr->TarBlock	= GPtr->idSector;											//	target block = ID sector
 
	//
	//	check out the BTree structure
	//

	err = BTCheck( GPtr, kCalculatedExtentRefNum );
	ReturnIfError( err );														//	invalid extent file BTree

	//
	//	check out the allocation map structure
	//

	err = BTMapChk( GPtr, kCalculatedExtentRefNum );
	ReturnIfError( err );														//	Invalid extent BTree map

	//
	//	compare BTree header record on disk with scavenger's BTree header record 
	//

	err = CmpBTH( GPtr, kCalculatedExtentRefNum );
	ReturnIfError( err );

	//
	//	compare BTree map on disk with scavenger's BTree map
	//

	err = CmpBTM( GPtr, kCalculatedExtentRefNum );

	return( err );
}



/*------------------------------------------------------------------------------

Function:	ExtFlChk - (Extent File Check)

Function:	Verifies the extent file structure.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		ExtFlChk		-	function result:			
								0	= no error
								+n 	= error code
------------------------------------------------------------------------------*/

OSErr ExtFlChk( SGlobPtr GPtr )
{
	UInt32			attributes;
	void			*p;
	OSErr			result;

	//
	//	process the bad block extents (created by the disk init pkg to hide badspots)
	//
 
	result = GetVBlk( GPtr, GPtr->idSector, &p );				//	get the alternate ID block
	ReturnIfError( result );									//	error, could't get it

	attributes = GPtr->isHFSPlus == true ? ((VolumeHeader*)p)->attributes : ((MasterDirectoryBlock*)p)->drAtrb;

	//€€ Does HFS+ honnor the same mask?
	if ( attributes & kVolumeSparedBlocksMask )					//	if any badspots
	{
		LargeExtentRecord		zeroXdr;						//	dummy passed to 'CheckFileExtents'
		UInt32					numBadBlocks;
		
		ClearMemory ( zeroXdr, sizeof( LargeExtentRecord ) );
		result = CheckFileExtents( GPtr, kHFSBadBlockFileID, 0, false, (void *)zeroXdr, &numBadBlocks );	//	check and mark bitmap
		ReturnIfError( result );								//	if error, propogate back up
	}
 
	return( noErr );
}


/*------------------------------------------------------------------------------

Function:	CreateCatalogBTreeControlBlock

Function:	Create the calculated ExtentsBTree Control Block
			
Input:		GPtr	-	pointer to scavenger global area

Output:				-	0	= no error
						n 	= error code 
------------------------------------------------------------------------------*/
OSErr	CreateCatalogBTreeControlBlock( SGlobPtr GPtr )
{
	OSErr					err;
	SInt32					size;
	UInt32					numABlks;
	HeaderRec				*header;
	BTreeControlBlock		*btcb			= GPtr->calculatedCatalogBTCB;
	Boolean					isHFSPlus		= GPtr->isHFSPlus;
	ExtendedVCB				*calculateVCB	= GPtr->calculatedVCB;

	//	Set up
	GPtr->TarID		= kHFSCatalogFileID;											//	target = catalog file
	GPtr->TarBlock	= kHeaderNodeNum;												//	target block = header node
 

	//
	//	check out allocation info for the Catalog File 
	//

	if ( isHFSPlus )
	{
		VolumeHeader			*volumeHeader;

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );				//	get the alternate VH
		ReturnIfError( err );

		BlockMoveData( (Ptr)volumeHeader->catalogFile.extents, (Ptr)GPtr->extendedCatalogFCB->extents, sizeof(LargeExtentRecord) );

		err = CheckFileExtents( GPtr, kHFSCatalogFileID, 0, false, (void *)GPtr->extendedCatalogFCB->extents, &numABlks );	/* check out extent info */	
		ReturnIfError( err );														//	error, invalid file allocation

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );				//	get the alternate VH
		ReturnIfError( err );

		if ( volumeHeader->catalogFile.totalBlocks != numABlks )					//	check out the PEOF
		{
			RcdError( GPtr, E_CatPEOF );
			return( E_CatPEOF );													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedCatalogFCB->fcbEOF  = volumeHeader->catalogFile.logicalSize.lo;							//	Set Catalog tree's LEOF
			GPtr->calculatedCatalogFCB->fcbPLen = volumeHeader->catalogFile.totalBlocks * volumeHeader->blockSize;	//	Set Catalog tree's PEOF
		}

		//
		//	Set up the minimal BTreeControlBlock structure
		//

		//	read the BTreeHeader from disk & also validate it's node size.
		err = GetBTreeHeader( GPtr, kCalculatedCatalogRefNum, &header );
		ReturnIfError( err );

		btcb->maxKeyLength		= kLargeCatalogKeyMaximumLength;					//	max key length
		btcb->keyCompareProc	= (void *)CompareExtendedCatalogKeys;
		btcb->leafRecords		= header->leafRecords;
		btcb->nodeSize			= header->nodeSize;
		btcb->totalNodes		= ( GPtr->calculatedCatalogFCB->fcbPLen / btcb->nodeSize );
		btcb->freeNodes			= btcb->totalNodes;									//	start with everything free
		btcb->attributes		|=(kBTBigKeysMask + kBTVariableIndexKeysMask);		//	HFS+ Catalog files have large, variable-sized keys

		btcb->treeDepth		= header->treeDepth;
		btcb->rootNode		= header->rootNode;
		btcb->firstLeafNode	= header->firstLeafNode;
		btcb->lastLeafNode	= header->lastLeafNode;


		//	Make sure the header nodes size field is correct by looking at the 1st record offset
		err	= CheckNodesFirstOffset( GPtr, btcb );
		if ( (err != noErr) && (btcb->nodeSize != 4096) )		//	default HFS+ Catalog node size is 4096
		{
			btcb->nodeSize			= 4096;
			btcb->totalNodes		= ( GPtr->calculatedCatalogFCB->fcbPLen / btcb->nodeSize );
			btcb->freeNodes			= btcb->totalNodes;								//	start with everything free
			
			err	= CheckNodesFirstOffset( GPtr, btcb );
			ReturnIfError( err );
			
			GPtr->CBTStat |= S_BTH;								//	update the Btree header
		}
	}
	else	//	HFS
	{
		MasterDirectoryBlock	*alternateMDB;

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&alternateMDB );				//	get the alternate MDB
		ReturnIfError( err );

		BlockMoveData( alternateMDB->drCTExtRec, GPtr->calculatedCatalogFCB->fcbExtRec, sizeof(SmallExtentRecord) );

		err = CheckFileExtents( GPtr, kHFSCatalogFileID, 0, false, (void *)GPtr->calculatedCatalogFCB->fcbExtRec, &numABlks );	/* check out extent info */	
		ReturnIfError( err );														//	error, invalid file allocation

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&alternateMDB );				//	get the alternate MDB again
		ReturnIfError( err );														//	error, could't get alt MDB

		if (alternateMDB->drCTFlSize != (numABlks * calculateVCB->blockSize))	//	check out the PEOF
		{
			RcdError( GPtr, E_CatPEOF );
			return( E_CatPEOF );													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedCatalogFCB->fcbPLen	= alternateMDB->drCTFlSize;			//	set up PEOF and EOF in FCB
			GPtr->calculatedCatalogFCB->fcbEOF	= GPtr->calculatedCatalogFCB->fcbPLen;
		}

		//
		//	Set up the minimal BTreeControlBlock structure
		//

		//	read the BTreeHeader from disk & also validate it's node size.
		err = GetBTreeHeader( GPtr, kCalculatedCatalogRefNum, &header );
		ReturnIfError( err );

		btcb->maxKeyLength		= kSmallCatalogKeyMaximumLength;					//	max key length
		btcb->keyCompareProc	= (void *) CompareCatalogKeys;
		btcb->leafRecords		= header->leafRecords;
		btcb->nodeSize			= header->nodeSize;
		btcb->totalNodes		= (GPtr->calculatedCatalogFCB->fcbPLen / btcb->nodeSize );
		btcb->freeNodes			= btcb->totalNodes;									//	start with everything free

		btcb->treeDepth		= header->treeDepth;
		btcb->rootNode		= header->rootNode;
		btcb->firstLeafNode	= header->firstLeafNode;
		btcb->lastLeafNode	= header->lastLeafNode;

		//	Make sure the header nodes size field is correct by looking at the 1st record offset
		err	= CheckNodesFirstOffset( GPtr, btcb );
		ReturnIfError( err );
	}
	

	//
	//	set up our DFA extended BTCB area.  Will we have enough memory on all HFS+ volumes.
	//

	btcb->refCon = (UInt32) NewPtrClear( sizeof(BTreeExtensionsRec) );			// allocate space for our BTCB extensions
	if ( btcb->refCon == (UInt32)nil )
		return( R_NoMem );														//	no memory for BTree bit map
	size = (btcb->totalNodes + 7) / 8;											//	size of BTree bit map
	((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr = NewPtrClear(size);			//	get precleared bitmap
	if ( ((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr == nil )
	{
		return( R_NoMem );														//	no memory for BTree bit map
	}

	((BTreeExtensionsRec*)btcb->refCon)->BTCBMSize			= size;						//	remember how long it is
	((BTreeExtensionsRec*)btcb->refCon)->realFreeNodeCount	= header->freeNodes;		//	keep track of real free nodes for progress

	return( noErr );
}


/*------------------------------------------------------------------------------

Function:	CreateExtendedAllocationsFCB

Function:	Create the calculated ExtentsBTree Control Block
			
Input:		GPtr	-	pointer to scavenger global area

Output:				-	0	= no error
						n 	= error code 
------------------------------------------------------------------------------*/
OSErr	CreateExtendedAllocationsFCB( SGlobPtr GPtr )
{
	OSErr					err;
	UInt32					numABlks;

	//	Set up
	GPtr->TarID		= kHFSAllocationFileID;											//	target = Allocation file
	GPtr->TarBlock	= GPtr->idSector;												//	target block = ID sector
 
	//
	//	check out allocation info for the allocation File 
	//

	if ( GPtr->isHFSPlus )
	{
		VolumeHeader			*volumeHeader;

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );				//	get the alternate VH
		ReturnIfError( err );

		BlockMoveData( (Ptr)volumeHeader->allocationFile.extents, (Ptr)GPtr->extendedAllocationsFCB->extents, sizeof(LargeExtentRecord) );

		err = CheckFileExtents( GPtr, kHFSAllocationFileID, 0, false, (void *)GPtr->extendedAllocationsFCB->extents, &numABlks );	/* check out extent info */	
		ReturnIfError( err );														//	error, invalid file allocation

		if ( volumeHeader->allocationFile.totalBlocks != numABlks )					//	check out the PEOF
		{
			RcdError( GPtr, E_CatPEOF );
			return( E_CatPEOF );													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedAllocationsFCB->fcbEOF  = volumeHeader->allocationFile.logicalSize.lo;							//	Set allocationFile LEOF
			GPtr->calculatedAllocationsFCB->fcbPLen = volumeHeader->allocationFile.totalBlocks * volumeHeader->blockSize;	//	Set allocationFile PEOF
		}
	}
	
	return( noErr );
}



/*------------------------------------------------------------------------------

Function:	CatBTChk - (Catalog BTree Check)

Function:	Verifies the catalog BTree structure.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		CatBTChk	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

OSErr CatBTChk( SGlobPtr GPtr )
{
	OSErr		err;

	//	Set up
	GPtr->TarID		= kHFSCatalogFileID;							/* target = catalog file */
	GPtr->TarBlock	= GPtr->idSector;								//	target block = ID sector
 
	//
	//	check out the BTree structure
	//

	err = BTCheck( GPtr, kCalculatedCatalogRefNum );
	ReturnIfError( err );														//	invalid extent file BTree

	//
	//	check out the allocation map structure
	//

	err = BTMapChk( GPtr, kCalculatedCatalogRefNum );
	ReturnIfError( err );														//	invalid extent BTree map

	//
	//	compare BTree header record on disk with scavenger's BTree header record 
	//

	err = CmpBTH( GPtr, kCalculatedCatalogRefNum );
	ReturnIfError( err );

	//
	//	compare BTree map on disk with scavenger's BTree map
	//

	err = CmpBTM( GPtr, kCalculatedCatalogRefNum );

	return( err );
}



/*------------------------------------------------------------------------------

Function:	CatFlChk - (Catalog File Check)

Function:	Verifies the catalog file structure.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		CatFlChk	-	function result:			
								0	= no error
								n 	= error code 
------------------------------------------------------------------------------*/

OSErr CatFlChk( SGlobPtr GPtr )
{
	UInt16 				recordSize;
	UInt16				frFlags;
	SInt16				selCode;
	UInt32				hint;
	UInt32				numABlks;
	CatalogKey			key;
	CatalogKey			foundKey;
	CatalogRecord		record;
	CatalogNodeID		folderID;
	UInt32				nextCatalogNodeID;
	UInt32				*reserved;
	UInt32				cNodeID;
	OSErr				result					= noErr;
	UInt32				fileThreadBallance		= 0;			//	files containing thread records - # threads, should be 0
	UInt32				folderThreadBallance	= 0;
	Boolean				isHFSPlus	= GPtr->isHFSPlus;
	ExtendedVCB			*calculatedVCB			= GPtr->calculatedVCB;

	//  set up
	calculatedVCB->vcbDirIDM	= 0;
	GPtr->TarBlock				= 0;							//	no target block yet

#if(0)
	GPtr->TarID					= kHFSCatalogFileID;			//	target = catalog file

	//	First locate the root directory and verify its custom icon.
	result = GetBTreeRecord( kCalculatedCatalogRefNum, 0x8001, &foundKey, &record, &recordSize, &hint );
	if ( result == noErr )
	{
		if ( isHFSPlus )
		{
			frFlags		= record.largeFolder.userInfo.frFlags;
			folderID	= record.largeFolder.folderID;
			GPtr->ParID	= foundKey.large.parentID;
			GPtr->CNType= kLargeFolderRecord;
			CopyCatalogName( (const CatalogName *)&foundKey.large.nodeName, &GPtr->CName, isHFSPlus );
		}
		else
		{
			frFlags		= record.smallFolder.userInfo.frFlags;
			folderID	= record.smallFolder.folderID;
			GPtr->ParID	= foundKey.small.parentID;
			GPtr->CNType= kSmallFolderRecord;
			CopyCatalogName( (const CatalogName *)&foundKey.small.nodeName, &GPtr->CName, isHFSPlus );
		}
		if ( frFlags & kHasCustomIcon )
			(void) CustomIconCheck( GPtr, folderID, frFlags );
	}

#endif

	//	locate the thread record for the root directory
	BuildCatalogKey( kHFSRootFolderID, (const CatalogName*) nil, isHFSPlus, &key );
	result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recordSize, &hint );

	GPtr->TarBlock = hint;										//	set target block
	if ( result != noErr )
	{
		if ( result == btNotFound )
		{
			RcdError( GPtr,E_NoRtThd );
			return( E_NoRtThd );
		}
		else
		{		
			result = IntError( GPtr,result );					//	error from BTSearch
			return( result );
		}
	}
#if(0)
	selCode			= 0;										//	start enumeration with root thread rec
	GPtr->ParID		= kHFSRootFolderID;
	folderThreadBallance--;										//	#folderthreads - #directories should == 0

#endif

	calculatedVCB->vcbNxtCNID	= -2;							//	initialize next free CNID
	nextCatalogNodeID			= kFirstFreeCatalogNodeID;
	

//€€€€€€€€€€€€€€€€€€€
	GPtr->ParID					= kHFSRootParentID;
	selCode						= 0x8001;
	folderThreadBallance		= 0;
	calculatedVCB->vcbDirCnt	= -1;							//	don't count root
//€€€€€€€€€€€€€€€€€€€
	
	//
	//	Sequentially traverse the entire catalog
	//
 	while ( result == noErr )
	{
		GPtr->TarID = kHFSCatalogFileID;						//	target = catalog file
	
		result = GetBTreeRecord( kCalculatedCatalogRefNum, selCode, &foundKey, &record, &recordSize, &hint );
		
		GPtr->TarBlock = hint;									//	set target block
		if ( result == btNotFound )
		{
			break;				 								//	no more catalog records
		}
		else if ( result != noErr )
		{
			result = IntError( GPtr, result ); 					//	error from BTGetRecord
			return( result );
		}
		
		selCode = 1;											//	get next rec from now on

		#if ( StandAloneEngine )
			GPtr->itemsProcessed = U64Add( GPtr->itemsProcessed, U64SetU( 1 ) );
		#endif
		
		//	Volume specific specific checks
		if ( isHFSPlus )
		{
			if ( record.recordType & 0xFF00 )
			{
				M_DebugStr("\p New Test FAILED, E_BadRecordType");
				result = E_BadRecordType;
			}
			else if ( recordSize > sizeof(CatalogRecord) )
			{
				result = E_LenCDR;
			}
		}
		else
		{
			if ( record.recordType & 0x00FF )						//	HFS volumes should have 0 here
			{
				M_DebugStr("\p New Test FAILED, E_BadRecordType");
				result = E_BadRecordType;
			}
			else if ( foundKey.small.nodeName[0] > 31 )				// too big?  bad news
			{
				result = E_BadFileName;
			}
			else if ( recordSize > sizeof(SmallCatalogFile) )
			{
				result = E_LenCDR;
			}
		}
		
		if ( result )
		{
			RcdError ( GPtr, result );
			break;
		}
		
 		result = CheckForStop( GPtr ); ReturnIfError( result );				//	Permit the user to interrupt
		
		//
		//	Record Checks
		//
		{
			CatalogRecord		catalogRecord;
			SmallCatalogFolder	*smallCatalogFolderP;
			SmallCatalogThread	*smallCatalogThreadP;
			UInt32				threadHint;
			SmallCatalogFile	*smallCatalogFileP;
			LargeCatalogFolder	*largeCatalogFolderP;
			LargeCatalogThread	*largeCatalogThreadP;
			LargeCatalogFile	*largeCatalogFileP;
			
			
			/* copy the key and data records */
			BlockMoveData( (Ptr) &foundKey, (Ptr) &key, CalcKeySize( GPtr->calculatedCatalogBTCB, (BTreeKey *)&foundKey ) );
			BlockMoveData( (Ptr) &record, (Ptr) &catalogRecord, recordSize );	
		
			//
			//	For an HFS+ directory record ...
			//
			if ( catalogRecord.recordType == kLargeFolderRecord )
			{
				largeCatalogFolderP		= (LargeCatalogFolder *) &catalogRecord;
				GPtr->TarID	= cNodeID	= largeCatalogFolderP->folderID;		//	target ID = directory ID
				GPtr->CNType			= catalogRecord.recordType;				//	target CNode type = directory ID
				CopyCatalogName( (const CatalogName *)&key.large.nodeName, &GPtr->CName, isHFSPlus );

				if ( recordSize != sizeof(LargeCatalogFolder) )
				{
					RcdError( GPtr, E_LenDir );
					result = E_LenDir;
					break;
				}
				if ( key.large.parentID != GPtr->ParID )
				{
					RcdError( GPtr, E_NoThd );
					result = E_NoThd;
					break;
				}
				
				if ( largeCatalogFolderP->flags != 0 )
				{
					RcdError( GPtr, E_CatalogFlagsNotZero );
					GPtr->CBTStat |= S_ReservedNotZero;
				}
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}

				if ( largeCatalogFolderP->folderID >= nextCatalogNodeID )
					nextCatalogNodeID = largeCatalogFolderP->folderID +1;
	
				folderThreadBallance--;													//	#folderthreads - #directories should == 0
				calculatedVCB->vcbDirCnt++;
				
				if ( GPtr->ParID == kHFSRootFolderID )
					calculatedVCB->vcbNmRtDirs++;
					
				//	If the hasCustomIcon bit is set, verify the Icon\n file exists
				frFlags	= largeCatalogFolderP->userInfo.frFlags;
#ifdef INVESTIGATE
				if ( frFlags & kHasCustomIcon )
					(void) CustomIconCheck( GPtr, largeCatalogFolderP->folderID, frFlags );
#endif
				//	Update the TextEncodings bitmap in the VCB for each file
				UpdateVolumeEncodings( calculatedVCB, largeCatalogFolderP->textEncoding );
			}
	
			//
			//	For an HFS+ thread record ...
			//
			else if ( (catalogRecord.recordType == kLargeFolderThreadRecord) || (catalogRecord.recordType == kLargeFileThreadRecord) )
			{
				largeCatalogThreadP		= (LargeCatalogThread *) &catalogRecord;
				GPtr->TarID	 = cNodeID	= key.large.parentID;					//	target ID = directory ID
				GPtr->CNType			= catalogRecord.recordType;				//	target CNode type = thread
				GPtr->CName.ustr.length	= 0;									//	no target CName
			
				catalogRecord.recordType == kLargeFolderThreadRecord ? folderThreadBallance++ : fileThreadBallance++;	//	number of threads must match number of files/folders

				if ( (recordSize > sizeof(LargeCatalogThread)) || (recordSize < sizeof(LargeCatalogThread) - sizeof(UniStr255)) )
				{
					RcdError( GPtr, E_LenThd );
					result = E_LenThd;
					break;
				}
				
				if ( key.large.nodeName.length != 0 )
				{
					RcdError( GPtr, E_ThdKey );
					result = E_ThdKey;
					break;
				}
				result = ChkCName( GPtr, (const CatalogName*)&largeCatalogThreadP->nodeName, isHFSPlus );
				if ( result != noErr )
				{
					RcdError( GPtr, E_ThdCN );
					result = E_ThdCN;
					break;
				}	
				
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}				
				
				GPtr->ParID = key.large.parentID;							//	we have a new parent directory (or fthd)
				
				threadHint = hint;											//	remember the thread hint
	
				//	locate the directory record for this thread or the file record for the fthd
				BuildCatalogKey( largeCatalogThreadP->parentID, (const CatalogName*) &largeCatalogThreadP->nodeName, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recordSize, &hint );
	
				if ( result != noErr )
				{
					if ( result == btNotFound )
					{
						if ( GPtr->CNType == kLargeFileThreadRecord )
							RcdFThdErr( GPtr, GPtr->TarID );				//	record error for possible later repair
						else
							RcdNoDirErr( GPtr, GPtr->TarID );				//	record error for possible later repair
					}
					else
					{		
						result = IntError( GPtr, result );					//	error from BTSearch
						return( result );
					}
				}
				else if ( (GPtr->CNType == kLargeFolderThreadRecord) && (record.recordType != kLargeFolderRecord) )
				{
					RcdError( GPtr, E_NoDir );								//	should have been a directory
					return( E_NoDir );
				}
				
				//	we know result == noErr at this point
				if ( GPtr->CNType == kLargeFileThreadRecord )				//	dealing with files
				{
					if ( result != btNotFound )
					{
						if ( record.recordType != kLargeFileRecord )
						{
							RcdError( GPtr, E_NoFile );
							return( E_NoFile );
						}
						//€€ Delete this test, LargeFileThreadRecords don't use the mask, and assume all records have threads
						#if(0)
						else
						{
							largeCatalogFileP = (LargeCatalogFile *) &record;
							if ( (largeCatalogFileP->flags & kFileThreadExistsMask) == 0 )		//	file thread flag not set
							{	//	treat it as if the file were not found, will delete the thread later -- KST
								RcdFThdErr( GPtr, GPtr->TarID );								//	record error for possible later repair
							}
						}
						#endif
					}
				}
					
				
				//	now re-locate the thread
	
				BuildCatalogKey( GPtr->ParID, (const CatalogName*) nil, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, threadHint, &foundKey, &record, &recordSize, &hint );
				if ( result != noErr )
				{
					result = IntError( GPtr, result );										//	error from BTSearch
					return( result );
				}
			}
			
			//
			//	For an HFS+ file record ...
			//
			else if ( catalogRecord.recordType == kLargeFileRecord )
			{
				largeCatalogFileP = (LargeCatalogFile *) &catalogRecord;
	
				GPtr->TarID = cNodeID = largeCatalogFileP->fileID;							//	target ID = file number
				GPtr->CNType	= catalogRecord.recordType;									//	target CNode type = kLargeFileRecord
				CopyCatalogName( (const CatalogName *) &key.large.nodeName, &GPtr->CName, isHFSPlus );	// copy the string name
			
				if ( recordSize != sizeof(LargeCatalogFile) )
				{
					RcdError( GPtr, E_LenFil );
					result = E_LenFil;
					break;
				}
				if ( key.large.parentID != GPtr->ParID )
				{
					RcdError( GPtr, E_NoThd );
					result = E_NoThd;
					break;
				}
				
				//	Check reserved fields
				if (   ( (largeCatalogFileP->flags & ~(0X83)) != 0 ) )
				{
					RcdError( GPtr, E_CatalogFlagsNotZero );
					GPtr->CBTStat |= S_ReservedNotZero;
				}
				if ( cNodeID < 16 )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}

				result = CheckFileExtents( GPtr, largeCatalogFileP->fileID, 0, false, (void *)largeCatalogFileP->dataFork.extents, &numABlks );
				if ( result != noErr )
				{
					break;
				}
					
				if ( largeCatalogFileP->dataFork.totalBlocks > numABlks )
				{
					RcdError( GPtr, E_PEOF );
					result = E_PEOF;
					break;
				}
					
				if ( largeCatalogFileP->dataFork.logicalSize.lo > largeCatalogFileP->dataFork.totalBlocks * calculatedVCB->blockSize )
				{
					RcdError( GPtr, E_LEOF );
					result = E_LEOF;
					break;
				}
				
				result = CheckFileExtents( GPtr, largeCatalogFileP->fileID, 0xFF, false, (void *)largeCatalogFileP->resourceFork.extents, &numABlks );
				if ( result != noErr )
				{
					break;
				}
					
				if (largeCatalogFileP->resourceFork.totalBlocks > numABlks )
				{
					RcdError( GPtr, E_PEOF );
					result = E_PEOF;
					break;
				}
				if ( largeCatalogFileP->resourceFork.logicalSize.lo > largeCatalogFileP->resourceFork.totalBlocks * calculatedVCB->blockSize )
				{
					RcdError( GPtr, E_LEOF );
					result = E_LEOF;
					break;
				}
				
				if ( largeCatalogFileP->fileID >= nextCatalogNodeID )
					nextCatalogNodeID = largeCatalogFileP->fileID +1;
	
				//	Update the TextEncodings bitmap in the VCB for each file
				UpdateVolumeEncodings( calculatedVCB, largeCatalogFileP->textEncoding );

				fileThreadBallance--;													//	#filethreads - #files with thread records should == 0
				calculatedVCB->vcbFilCnt++;
				if ( GPtr->ParID == kHFSRootFolderID )
					calculatedVCB->vcbNmFls++;
			}

			//
			//	for an HFS directory record ...
			//
			else if ( catalogRecord.recordType == kSmallFolderRecord )
			{
				smallCatalogFolderP = (SmallCatalogFolder *) &catalogRecord;
				GPtr->TarID = cNodeID = smallCatalogFolderP->folderID;					//	target ID = directory ID
				GPtr->CNType	= catalogRecord.recordType;								//	target CNode type = directory ID
				CopyCatalogName( (const CatalogName *) &key.small.nodeName, &GPtr->CName, isHFSPlus );
			
				if ( recordSize != sizeof(SmallCatalogFolder) )
				{
					RcdError( GPtr, E_LenDir );
					result = E_LenDir;
					break;
				}
				if ( key.small.parentID != GPtr->ParID )
				{
					RcdError( GPtr, E_NoThd );
					result = E_NoThd;
					break;
				}
				if ( smallCatalogFolderP->flags != 0 )
				{
					RcdError( GPtr, E_CatalogFlagsNotZero );
					GPtr->CBTStat |= S_ReservedNotZero;
				}
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}
				
				if ( smallCatalogFolderP->folderID >= nextCatalogNodeID )
					nextCatalogNodeID = smallCatalogFolderP->folderID +1;
	
				folderThreadBallance--;											//	#folderthreads - #directories should == 0
				calculatedVCB->vcbDirCnt++;										//	track total number of directories
				if ( GPtr->ParID == kHFSRootFolderID )
					calculatedVCB->vcbNmRtDirs++;

				//	If the hasCustomIcon bit is set, verify the Icon\n file exists
#ifdef INVESTIGATE
				frFlags	= smallCatalogFolderP->userInfo.frFlags;
				if ( frFlags & kHasCustomIcon )
					(void) CustomIconCheck( GPtr, smallCatalogFolderP->folderID, frFlags );
#endif
			}
	
			//
			//	for a thread record ...
			//
			else if ( (catalogRecord.recordType == kSmallFolderThreadRecord) || (catalogRecord.recordType == kSmallFileThreadRecord) )
			{
				smallCatalogThreadP 	= (SmallCatalogThread *) &catalogRecord;
				GPtr->TarID	= cNodeID	= key.small.parentID;					//	target ID = directory ID
				GPtr->CNType			= catalogRecord.recordType;				//	target CNode type = thread
				GPtr->CName.ustr.length	= 0;									//	no target CName
			
				catalogRecord.recordType == kSmallFolderThreadRecord ? folderThreadBallance++ : fileThreadBallance++;	//	number of threads must match number of files/folders

				if ( recordSize != sizeof(SmallCatalogThread) )
				{
					RcdError( GPtr, E_LenThd );
					result = E_LenThd;
					break;
				}
				if ( key.small.nodeName[0] != 0 )
				{
					RcdError( GPtr, E_ThdKey );
					result = E_ThdKey;
					break;
				}
				result = ChkCName( GPtr, (const CatalogName*) &smallCatalogThreadP->nodeName, isHFSPlus );
				if ( result != noErr )
				{
					RcdError( GPtr, E_ThdCN );
					result = E_ThdCN;
					break;
				}	
				
				reserved = (UInt32*) &(smallCatalogThreadP->reserved);
				if ( reserved[0] || reserved[1] )
				{
					RcdError( GPtr, E_CatalogFlagsNotZero );
					GPtr->CBTStat |= S_ReservedNotZero;
				}
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}
				
				GPtr->ParID = key.small.parentID;							//	we have a new parent directory (or fthd)
				
				threadHint = hint;											//	remember the thread hint
	
				//	locate the directory record for this thread or the file record for the fthd
				BuildCatalogKey( smallCatalogThreadP->parentID, (const CatalogName*) smallCatalogThreadP->nodeName, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recordSize, &hint );
	
				if ( result != noErr )
				{
					if ( result == btNotFound )
					{
						if ( GPtr->CNType == kSmallFileThreadRecord )
							RcdFThdErr( GPtr, GPtr->TarID );				//	record error for possible later repair
						else
							RcdNoDirErr( GPtr, GPtr->TarID );				//	record error for possible later repair
					}
					else
					{		
						result = IntError( GPtr, result );					//	error from BTSearch
						return( result );
					}
				}
				else if ( (GPtr->CNType == kSmallFolderThreadRecord) && (record.recordType != kSmallFolderRecord) )
				{
					RcdError( GPtr, E_NoDir );								//	should have been a directory
					return( E_NoDir );
				}
				
				//	we know result == noErr at this point
				if ( GPtr->CNType == kSmallFileThreadRecord )				//	dealing with files
				{
					if ( result != btNotFound )
					{
						if ( record.recordType != kSmallFileRecord )
						{
							RcdError( GPtr, E_NoFile );
							return( E_NoFile );
						}
						else
						{
							smallCatalogFileP = (SmallCatalogFile *) &record;
							if ( (smallCatalogFileP->flags & kFileThreadExistsMask) == 0 )	//	file thread flag not set
							{	//	treat it as if the file were not found, will delete the thread later -- KST
								RcdFThdErr( GPtr, GPtr->TarID );				//	record error for possible later repair
							}
						}
					}
				}
					
				
				//	now re-locate the thread
	
				BuildCatalogKey( GPtr->ParID, (const CatalogName*) nil, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, threadHint, &foundKey, &record, &recordSize, &hint );
				if ( result != noErr )
				{
					result = IntError( GPtr, result );						//	error from BTSearch
					return( result );
				}
			}
			
			//
			//	for a file record ...
			//
			else if ( catalogRecord.recordType == kSmallFileRecord )
			{
				smallCatalogFileP = (SmallCatalogFile *) &catalogRecord;
	
				GPtr->TarID = cNodeID = smallCatalogFileP->fileID;								//	target ID = file number
				GPtr->CNType	= catalogRecord.recordType;										//	target CNode type = kSmallFileRecord
				CopyCatalogName( (const CatalogName *) &key.small.nodeName, &GPtr->CName, isHFSPlus );	// copy the string name
			
				if ( (smallCatalogFileP->flags & kFileThreadExistsMask) != 0 )					//	file should have a thread record
					fileThreadBallance--;
				
				if ( recordSize != sizeof(SmallCatalogFile) )
				{
					RcdError( GPtr, E_LenFil );
					result = E_LenFil;
					break;
				}
				if ( key.small.parentID != GPtr->ParID )
				{
					RcdError( GPtr, E_NoThd );
					result = E_NoThd;
					break;
				}
				
				//	Check reserved fields
				if (   ( (smallCatalogFileP->flags & ~(0X83)) != 0 ) 
					|| ( smallCatalogFileP->dataStartBlock != 0 )
					|| ( smallCatalogFileP->rsrcStartBlock != 0 )
					|| ( smallCatalogFileP->reserved != 0 ) )
				{
					RcdError( GPtr, E_CatalogFlagsNotZero );
					GPtr->CBTStat |= S_ReservedNotZero;
				}
				if ( (cNodeID == 0) || (cNodeID < 16 && cNodeID > 2) )
				{
					RcdError( GPtr, E_InvalidID );
					result = E_InvalidID;
					break;
				}


				result = CheckFileExtents( GPtr, smallCatalogFileP->fileID, 0, false, (void *)smallCatalogFileP->dataExtents, &numABlks );
				if ( result != noErr )
				{
					break;
				}
					
				if ( smallCatalogFileP->dataPhysicalSize > (numABlks * calculatedVCB->blockSize) )
				{
					RcdError( GPtr, E_PEOF );
					result = E_PEOF;
					break;
				}
					
				if ( smallCatalogFileP->dataLogicalSize > smallCatalogFileP->dataPhysicalSize )
				{
					RcdError( GPtr, E_LEOF );
					result = E_LEOF;
					break;
				}
				
				result = CheckFileExtents( GPtr, smallCatalogFileP->fileID, 0xFF, false, (void *)smallCatalogFileP->rsrcExtents, &numABlks );
				if ( result != noErr )
				{
					break;
				}
					
				if (smallCatalogFileP->rsrcPhysicalSize > (numABlks * calculatedVCB->blockSize))
				{
					RcdError( GPtr, E_PEOF );
					result = E_PEOF;
					break;
				}
				if ( smallCatalogFileP->rsrcLogicalSize > smallCatalogFileP->rsrcPhysicalSize )
				{
					RcdError( GPtr, E_LEOF );
					result = E_LEOF;
					break;
				}
				
				if ( smallCatalogFileP->fileID >= nextCatalogNodeID )
					nextCatalogNodeID = smallCatalogFileP->fileID +1;
	
				//	Keeping handle in globals of file ID's for HFS volume only
				if ( PtrAndHand( &smallCatalogFileP->fileID, (Handle) GPtr->validFilesList, sizeof(UInt32) ) )
					return( R_NoMem );
					
				calculatedVCB->vcbFilCnt++;
				if ( GPtr->ParID == kHFSRootFolderID )
					calculatedVCB->vcbNmFls++;
	
			}
			else											//	invalid record type
			{
				RcdError( GPtr, E_CatRec );
				result = E_CatRec;
				break;
			}
		}	//	End Record Checks
	}	//	End while
	
	if ( (folderThreadBallance != 0) || (fileThreadBallance != 0) )
	{
		GPtr->EBTStat |= S_Orphan;
	}
	
	if ( result == btNotFound )
		result = noErr;				 						//	all done, no more catalog records

	calculatedVCB->vcbNxtCNID = nextCatalogNodeID;			//	update the field
	
	
	//	Run MountCheck to at least verify that we cover the same checks
	//	MountCheck will also test for orphaned files.
	if ( result == noErr )
	{
		UInt32			consistencyStatus;
		CatalogNodeID	vcbNxtCNID			= 	calculatedVCB->vcbNxtCNID;
		
		result	= MountCheck( calculatedVCB, &consistencyStatus );
		
		if ( result != R_UInt )
		{			
			if ( result != noErr )				//	Serious errors at this point indicate there may be orphaned files.
			{
				WriteMsg( GPtr, M_MountCheckMajorError, kErrorMessage );
				GPtr->EBTStat |= S_OrphanedExtent;
			}
				
			if ( consistencyStatus != 0 )
			{
				if ( result == noErr )
					WriteMsg( GPtr, M_MountCheckMinorError, kErrorMessage );
				
				GPtr->VIStat	|=	S_MountCheckMinorErrors;
				
				if ( consistencyStatus & kHFSInvalidPEOF )
					GPtr->CBTStat	|= S_RebuildBTree;
				
				if ( (consistencyStatus & kHFSOrphanedExtents) || (vcbNxtCNID < calculatedVCB->vcbNxtCNID) )
					GPtr->EBTStat	|= S_OrphanedExtent;

				calculatedVCB->vcbNxtCNID	= vcbNxtCNID;	//	Since MountCheck will not shrink this value to our calculated value
			}
			result	= noErr;
		}
	}


	return( result );
}


/*------------------------------------------------------------------------------

Function:	CatHChk - (Catalog Hierarchy Check)

Function:	Verifies the catalog hierarchy.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		CatHChk	-	function result:			
								0	= no error
								n 	= error code 
------------------------------------------------------------------------------*/

OSErr CatHChk( SGlobPtr GPtr )
{
	SInt16				i;
	OSErr				result;
	UInt16 				recSize;
	SInt16				selCode;
	UInt32				hint;
	UInt32				dirCnt;
	UInt32				filCnt;
	SInt16				rtdirCnt;
	SInt16				rtfilCnt;
	ExtendedVCB			*calculatedVCB;
	SDPR				*dprP;
	SDPR				*dprP1;
	CatalogKey			foundKey;
	Boolean				validKeyFound;
	CatalogKey			key;
	CatalogRecord		record;
	CatalogRecord		record2;
	LargeCatalogFolder	*largeCatalogFolderP;
	LargeCatalogFile	*largeCatalogFileP;
	SmallCatalogFile	*smallCatalogFileP;
	SmallCatalogFolder	*smallCatalogFolderP;
	CatalogName			catalogName;
	UInt32				valence;
	CatalogRecord		threadRecord;
	CatalogNodeID		parID;
	Boolean				isHFSPlus	= GPtr->isHFSPlus;

	//	set up
	calculatedVCB	= GPtr->calculatedVCB;
	GPtr->TarID		= kHFSCatalogFileID;						/* target = catalog file */
	GPtr->TarBlock	= 0;										/* no target block yet */

	//
	//	position to the beginning of catalog
	//
	
	//€€ Can we ignore this part by just taking advantage of setting the selCode = 0x8001;
	{ 
		BuildCatalogKey( 1, (const CatalogName *)nil, isHFSPlus, &key );
		result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &threadRecord, &recSize, &hint );
	
		GPtr->TarBlock = hint;									/* set target block */
		if ( result != btNotFound )
		{
			RcdError( GPtr, E_CatRec );
			return( E_CatRec );
		}
	}
	
	GPtr->DirLevel		= 1;
	dprP				= &(*GPtr->DirPTPtr)[0];					
	dprP->directoryID	= 1;
	dirCnt				= filCnt = rtdirCnt = rtfilCnt = 0;

	result	= noErr;
	selCode = 0x8001;											/* start with root directory */			

	//
	//	enumerate the entire catalog 
	//
	while ( (GPtr->DirLevel > 0) && (result == noErr) )
	{
		dprP = &(*GPtr->DirPTPtr)[GPtr->DirLevel -1];
		
		validKeyFound = true;
		record.recordType = 0;
		
		//	get the next record
		result = GetBTreeRecord( kCalculatedCatalogRefNum, selCode, &foundKey, &record, &recSize, &hint );
		
		GPtr->TarBlock = hint;									/* set target block */
		if ( result != noErr )
		{
			if ( result == btNotFound )
			{
				result = noErr;
				validKeyFound = false;
			}
			else
			{
				result = IntError( GPtr, result );				/* error from BTGetRecord */
				return( result );
			}
		}
		selCode = 1;											/* get next rec from now on */

		#if ( StandAloneEngine )
			GPtr->itemsProcessed = U64Add( GPtr->itemsProcessed, U64SetU( 1 ) );
		#endif
		
		//
		//	 if same ParID ...
		//
		parID = isHFSPlus == true ? foundKey.large.parentID : foundKey.small.parentID;
		if ( (validKeyFound == true) && (parID == dprP->directoryID) )
		{
			dprP->offspringIndex++;								/* increment offspring index */

			//	if new directory ...
	
			if ( record.recordType == kLargeFolderRecord )
			{
 				result = CheckForStop( GPtr ); ReturnIfError( result );				//	Permit the user to interrupt
			
				largeCatalogFolderP = (LargeCatalogFolder *) &record;				
				GPtr->TarID = largeCatalogFolderP->folderID;				//	target ID = directory ID 
				GPtr->CNType = record.recordType;							//	target CNode type = directory ID 
				CopyCatalogName( (const CatalogName *) &foundKey.large.nodeName, &GPtr->CName, isHFSPlus );

				if ( dprP->directoryID > 1 )
				{
					GPtr->DirLevel++;										//	we have a new directory level 
					dirCnt++;
				}
				if ( dprP->directoryID == kHFSRootFolderID )				//	bump root dir count 
					rtdirCnt++;

				if ( GPtr->DirLevel > CMMaxDepth )
				{
					RcdError(GPtr,E_CatDepth);								//	error, exceeded max catalog depth 
					return(E_CatDepth);
				}				
				dprP = &(*GPtr->DirPTPtr)[GPtr->DirLevel -1];
				dprP->directoryID		= largeCatalogFolderP->folderID;
				dprP->offspringIndex	= 1;
				dprP->directoryHint		= hint;
				dprP->parentDirID		= foundKey.large.parentID;
				CopyCatalogName( (const CatalogName *) &foundKey.large.nodeName, &dprP->directoryName, isHFSPlus );

				for ( i = 1; i < GPtr->DirLevel; i++ )
				{
					dprP1 = &(*GPtr->DirPTPtr)[i -1];
					if (dprP->directoryID == dprP1->directoryID)
					{
						RcdError( GPtr,E_DirLoop );							//	loop in directory hierarchy 
						return( E_DirLoop );
					}
				}
								
				//	Find thread record
				BuildCatalogKey( dprP->directoryID, (const CatalogName *) nil, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &threadRecord, &recSize, &hint );
				
				if  ( result != noErr )
				{
					result = IntError( GPtr,result );							//	error from BTSearch 
					return( result );
				}	
				dprP->threadHint	= hint;										//	save hint for thread 
				GPtr->TarBlock		= hint;										//	set target block 
			}

			//	LargeCatalogFile
			else if ( record.recordType == kLargeFileRecord )
			{
				largeCatalogFileP = (LargeCatalogFile *) &record;
				GPtr->TarID = largeCatalogFileP->fileID;					//	target ID = file number 
				GPtr->CNType = record.recordType;							//	target CNode type = thread 
				CopyCatalogName( (const CatalogName *) &foundKey.large.nodeName, &GPtr->CName, isHFSPlus );
				filCnt++;
				if (dprP->directoryID == kHFSRootFolderID)
					rtfilCnt++;
			}	

			else if ( record.recordType == kSmallFolderRecord )
			{
 				result = CheckForStop( GPtr ); ReturnIfError( result );				//	Permit the user to interrupt
			
				smallCatalogFolderP = (SmallCatalogFolder *) &record;				
				GPtr->TarID = smallCatalogFolderP->folderID;				/* target ID = directory ID */
				GPtr->CNType = record.recordType;							/* target CNode type = directory ID */
				CopyCatalogName( (const CatalogName *) &key.small.nodeName, &GPtr->CName, isHFSPlus );	/* target CName = directory name */

				if (dprP->directoryID > 1)
				{
					GPtr->DirLevel++;										/* we have a new directory level */
					dirCnt++;
				}
				if (dprP->directoryID == kHFSRootFolderID)					/* bump root dir count */
					rtdirCnt++;

				if (GPtr->DirLevel > CMMaxDepth)
				{
					RcdError(GPtr,E_CatDepth);								/* error, exceeded max catalog depth */
					return(E_CatDepth);
				}				
				dprP = &(*GPtr->DirPTPtr)[GPtr->DirLevel -1];			
				dprP->directoryID		= smallCatalogFolderP->folderID;
				dprP->offspringIndex	= 1;
				dprP->directoryHint		= hint;
				dprP->parentDirID		= foundKey.small.parentID;

				CopyCatalogName( (const CatalogName *) &foundKey.small.nodeName, &dprP->directoryName, isHFSPlus );

				for (i = 1; i < GPtr->DirLevel; i++)
				{
					dprP1 = &(*GPtr->DirPTPtr)[i -1];
					if (dprP->directoryID == dprP1->directoryID)
					{
						RcdError( GPtr,E_DirLoop );				/* loop in directory hierarchy */
						return( E_DirLoop );
					}
				}
								
				BuildCatalogKey( dprP->directoryID, (const CatalogName *)0, isHFSPlus, &key );
				result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &threadRecord, &recSize, &hint );
				if  (result != noErr )
				{
					result = IntError(GPtr,result);				/* error from BTSearch */
					return(result);
				}	
				dprP->threadHint	= hint;						/* save hint for thread */
				GPtr->TarBlock		= hint;						/* set target block */
			}

			//	SmallCatalogFile...
			else if ( record.recordType == kSmallFileRecord )
			{
				smallCatalogFileP = (SmallCatalogFile *) &record;
				GPtr->TarID = smallCatalogFileP->fileID;							/* target ID = file number */
				GPtr->CNType = record.recordType;									/* target CNode type = thread */
				CopyCatalogName( (const CatalogName *) &foundKey.small.nodeName, &GPtr->CName, isHFSPlus );	/* target CName = directory name */
				filCnt++;
				if (dprP->directoryID == kHFSRootFolderID)
					rtfilCnt++;
			}
			
			//	Unknown/Bad record type
			else
			{
				M_DebugStr("\p Unknown-Bad record type");
				return( 123 );
			}
		} 
		
		//
		//	 if not same ParID or no record
		//
		else if ( (record.recordType == kSmallFileThreadRecord) || (record.recordType == kLargeFileThreadRecord) )			/* it's a file thread, skip past it */
		{
			GPtr->TarID				= parID;						//	target ID = file number
			GPtr->CNType			= record.recordType;			//	target CNode type = thread
			GPtr->CName.ustr.length	= 0;							//	no target CName
		}
		
		else
		{
			GPtr->TarID = dprP->directoryID;						/* target ID = current directory ID */
			GPtr->CNType = record.recordType;						/* target CNode type = directory */
//			DFA_PrepareOutputName( (const CatalogName *) &dprP->directoryName, isHFSPlus, GPtr->CName );
			CopyCatalogName( (const CatalogName *) &dprP->directoryName, &GPtr->CName, isHFSPlus );	// copy the string name

			//	re-locate current directory
			CopyCatalogName( (const CatalogName *) &dprP->directoryName, &catalogName, isHFSPlus );
			BuildCatalogKey( dprP->parentDirID, (const CatalogName *)&catalogName, isHFSPlus, &key );
			result = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, dprP->directoryHint, &foundKey, &record2, &recSize, &hint );
			
			if ( result != noErr )
			{
				result = IntError(GPtr,result);						/* error from BTSearch */
				return(result);
			}
			GPtr->TarBlock = hint;									/* set target block */
			

			valence = isHFSPlus == true ? record2.largeFolder.valence : (UInt32)record2.smallFolder.valence;

			if ( valence != dprP->offspringIndex -1 ) 				/* check its valence */
				if ( result = RcdValErr( GPtr, E_DirVal, dprP->offspringIndex -1, valence, dprP->parentDirID ) )
					return( result );

			GPtr->DirLevel--;										/* move up a level */			
			
			if(GPtr->DirLevel > 0)
			{										
				dprP = &(*GPtr->DirPTPtr)[GPtr->DirLevel -1];
				GPtr->TarID	= dprP->directoryID;					/* target ID = current directory ID */
				GPtr->CNType = record.recordType;					/* target CNode type = directory */
				CopyCatalogName( (const CatalogName *) &dprP->directoryName, &GPtr->CName, isHFSPlus );
			}
		}
	}		//	end while

	//
	//	verify directory and file counts (all nonfatal, repairable errors)
	//
	if ( rtdirCnt != calculatedVCB->vcbNmRtDirs ) 						/* check count of dirs in root */
		if ( result = RcdValErr(GPtr,E_RtDirCnt,rtdirCnt,calculatedVCB->vcbNmRtDirs,0) )
			return( result );

	if ( rtfilCnt != calculatedVCB->vcbNmFls ) 							/* check count of files in root */
		if ( result = RcdValErr(GPtr,E_RtFilCnt,rtfilCnt,calculatedVCB->vcbNmFls,0) )
			return( result );
	
	if (dirCnt != calculatedVCB->vcbDirCnt ) 							/* check count of dirs in volume */
		if ( result = RcdValErr(GPtr,E_DirCnt,dirCnt,calculatedVCB->vcbDirCnt,0) )
			return( result );
		
	if ( filCnt != calculatedVCB->vcbFilCnt ) 							/* check count of files in volume */
		if ( result = RcdValErr(GPtr,E_FilCnt,filCnt,calculatedVCB->vcbFilCnt,0) )
			return( result );

	return( noErr );

}	/* end of CatHChk */


/*------------------------------------------------------------------------------

Function:	CreateAttributesBTreeControlBlock

Function:	Create the calculated AttributesBTree Control Block
			
Input:		GPtr	-	pointer to scavenger global area

Output:				-	0	= no error
						n 	= error code 
------------------------------------------------------------------------------*/
OSErr	CreateAttributesBTreeControlBlock( SGlobPtr GPtr )
{
	OSErr					err;
	SInt32					size;
	UInt32					numABlks;
	HeaderRec				*header;
	BTreeControlBlock		*btcb			= GPtr->calculatedAttributesBTCB;
	Boolean					isHFSPlus		= GPtr->isHFSPlus;
	ExtendedVCB				*calculateVCB	= GPtr->calculatedVCB;

	//	Set up
	GPtr->TarID		= kHFSAttributesFileID;											//	target = attributes file
	GPtr->TarBlock	= kHeaderNodeNum;												//	target block = header node
 

	//
	//	check out allocation info for the Attributes File 
	//

	if ( isHFSPlus )
	{
		VolumeHeader			*volumeHeader;

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );				//	get the alternate VH
		ReturnIfError( err );

		BlockMoveData( (Ptr)volumeHeader->attributesFile.extents, (Ptr)GPtr->extendedAttributesFCB->extents, sizeof(LargeExtentRecord) );

		err = CheckFileExtents( GPtr, kHFSAttributesFileID, 0, false, (void *)GPtr->extendedAttributesFCB->extents, &numABlks );	/* check out extent info */	
		ReturnIfError( err );														//	error, invalid file allocation

		if ( volumeHeader->attributesFile.totalBlocks != numABlks )					//	check out the PEOF
		{
			RcdError( GPtr, E_CatPEOF );
			return( E_CatPEOF );													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedAttributesFCB->fcbEOF  = volumeHeader->attributesFile.logicalSize.lo;							//	Set Attributes tree's LEOF
			GPtr->calculatedAttributesFCB->fcbPLen = volumeHeader->attributesFile.totalBlocks * volumeHeader->blockSize;	//	Set Attributes tree's PEOF
		}

		//
		//	See if we actually have an attributes BTree
		//
		if (numABlks == 0)
		{
			btcb->maxKeyLength		= 0;
			btcb->keyCompareProc	= 0;
			btcb->leafRecords		= 0;
			btcb->nodeSize			= 0;
			btcb->totalNodes		= 0;
			btcb->freeNodes			= 0;
			btcb->attributes		= 0;

			btcb->treeDepth		= 0;
			btcb->rootNode		= 0;
			btcb->firstLeafNode	= 0;
			btcb->lastLeafNode	= 0;
			
			GPtr->calculatedVCB->attributesRefNum	= 0;
		}
		else
		{
			//	read the BTreeHeader from disk & also validate it's node size.
			err = GetBTreeHeader( GPtr, kCalculatedAttributesRefNum, &header );
			ReturnIfError( err );

			btcb->maxKeyLength		= kAttributeKeyMaximumLength;					//	max key length
			btcb->keyCompareProc	= (void *)CompareAttributeKeys;
			btcb->leafRecords		= header->leafRecords;
			btcb->nodeSize			= header->nodeSize;
			btcb->totalNodes		= ( GPtr->calculatedAttributesFCB->fcbPLen / btcb->nodeSize );
			btcb->freeNodes			= btcb->totalNodes;									//	start with everything free
			btcb->attributes		|=(kBTBigKeysMask + kBTVariableIndexKeysMask);		//	HFS+ Attributes files have large, variable-sized keys

			btcb->treeDepth		= header->treeDepth;
			btcb->rootNode		= header->rootNode;
			btcb->firstLeafNode	= header->firstLeafNode;
			btcb->lastLeafNode	= header->lastLeafNode;

			//
			//	Make sure the header nodes size field is correct by looking at the 1st record offset
			//
			err	= CheckNodesFirstOffset( GPtr, btcb );
			ReturnIfError( err );
		}
	}
	else
	{
		btcb->maxKeyLength		= 0;
		btcb->keyCompareProc	= 0;
		btcb->leafRecords		= 0;
		btcb->nodeSize			= 0;
		btcb->totalNodes		= 0;
		btcb->freeNodes			= 0;
		btcb->attributes		= 0;

		btcb->treeDepth		= 0;
		btcb->rootNode		= 0;
		btcb->firstLeafNode	= 0;
		btcb->lastLeafNode	= 0;
			
		GPtr->calculatedVCB->attributesRefNum	= 0;
	}
	

	//
	//	set up our DFA extended BTCB area.  Will we have enough memory on all HFS+ volumes.
	//
	btcb->refCon = (UInt32) NewPtrClear( sizeof(BTreeExtensionsRec) );			// allocate space for our BTCB extensions
	if ( btcb->refCon == (UInt32)nil )
		return( R_NoMem );														//	no memory for BTree bit map

	if (btcb->totalNodes == 0)
	{
		((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr			= nil;
		((BTreeExtensionsRec*)btcb->refCon)->BTCBMSize			= 0;
		((BTreeExtensionsRec*)btcb->refCon)->realFreeNodeCount	= 0;
	}
	else
	{
		if ( btcb->refCon == (UInt32)nil )
			return( R_NoMem );														//	no memory for BTree bit map
		size = (btcb->totalNodes + 7) / 8;											//	size of BTree bit map
		((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr = NewPtrClear(size);			//	get precleared bitmap
		if ( ((BTreeExtensionsRec*)btcb->refCon)->BTCBMPtr == nil )
		{
			return( R_NoMem );														//	no memory for BTree bit map
		}

		((BTreeExtensionsRec*)btcb->refCon)->BTCBMSize			= size;						//	remember how long it is
		((BTreeExtensionsRec*)btcb->refCon)->realFreeNodeCount	= header->freeNodes;		//	keep track of real free nodes for progress
	}
	
	return( noErr );
}


/*------------------------------------------------------------------------------

Function:	AttrBTChk - (Attributes BTree Check)

Function:	Verifies the attributes BTree structure.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		ExtBTChk	-	function result:			
								0	= no error
								n 	= error code 
------------------------------------------------------------------------------*/

OSErr AttrBTChk( SGlobPtr GPtr )
{
	OSErr					err;

	//
	//	If this volume has no attributes BTree, then skip this check
	//
	if (GPtr->calculatedVCB->attributesRefNum == 0)
		return noErr;
	
	//	Write the status message here to avoid potential confusion to user.
	WriteMsg( GPtr, M_AttrBTChk, kStatusMessage );

	//	Set up
	GPtr->TarID		= kHFSAttributesFileID;										//	target = attributes file
	GPtr->TarBlock	= GPtr->idSector;											//	target block = ID Block
 
	//
	//	check out the BTree structure
	//

	err = BTCheck( GPtr, kCalculatedAttributesRefNum );
	ReturnIfError( err );														//	invalid attributes file BTree

	//
	//	check out the allocation map structure
	//

	err = BTMapChk( GPtr, kCalculatedAttributesRefNum );
	ReturnIfError( err );														//	Invalid attributes BTree map

	//
	//	compare BTree header record on disk with scavenger's BTree header record 
	//

	err = CmpBTH( GPtr, kCalculatedAttributesRefNum );
	ReturnIfError( err );

	//
	//	compare BTree map on disk with scavenger's BTree map
	//

	err = CmpBTM( GPtr, kCalculatedAttributesRefNum );

	return( err );
}


/*------------------------------------------------------------------------------

Name:		RcdFThdErr - (record file thread error)

Function:	Allocates a RepairOrder node describing a dangling file thread record,
			most likely caused by discarding a file with system 6 (or less) that
			had an alias created by system 7 (or greater).  System 6 isn't aware
			of aliases, and so won't remove the accompanying thread record.

Input:		GPtr 		- the scavenger globals
			fid			- the File ID in the thread record key

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate the record
------------------------------------------------------------------------------*/

static int RcdFThdErr( SGlobPtr	GPtr, UInt32 fid )			//	the dangling file ID
{
	RepairOrderPtr	p;										//	the node we compile
	
	RcdError( GPtr, E_NoFile );								//	first, record the error
	
	p = AllocMinorRepairOrder( GPtr, 0 );					//	then get a repair order node (std size)
	if ( p==NULL )											//	quit if out of room
		return( R_NoMem );
	
	p->type = E_NoFile;										//	repair type
	p->parid = fid;											//	this is the only info we need
	GPtr->CatStat |= S_FThd;								//	set flag to trigger repair
	
	return( noErr );										//	successful return
}


/*------------------------------------------------------------------------------

Name:		RcdNoDirErr - (record missing direcotury record error)

Function:	Allocates a RepairOrder node describing a missing directory record,
			most likely caused by disappearing folder bug.  This bug causes some
			folders to jump to Desktop from the root window.  The catalog directory
			record for such a folder has the Desktop folder as the parent but its
			thread record still the root directory as its parent.

Input:		GPtr 		- the scavenger globals
			did			- the directory ID in the thread record key

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate the record
------------------------------------------------------------------------------*/

static int RcdNoDirErr( SGlobPtr GPtr, UInt32 did )			//	the directory ID in the thread record key
{
	RepairOrderPtr	p;										//	the node we compile
	
	RcdError( GPtr, E_NoDir );								//	first, record the error
	
	p = AllocMinorRepairOrder( GPtr, 0 );					//	then get a repair order node (std size)
	if ( p==NULL )											//	quit if out of room
		return ( R_NoMem );
	
	p->type = E_NoDir;										//	repair type
	p->parid = did;											//	this is the only info we need
	GPtr->CatStat |= S_NoDir;								//	set flag to trigger repair
	
	return( noErr );										//	successful return
}


/*------------------------------------------------------------------------------

Name:		RcdValErr - (Record Valence Error)

Function:	Allocates a RepairOrder node and linkg it into the 'GPtr->RepairP'
			list, to describe an incorrect valence count for possible repair.

Input:		GPtr		- ptr to scavenger global data
			type		- error code (E_xxx), which should be >0
			correct		- the correct valence, as computed here
			incorrect	- the incorrect valence as found in volume
			parid		- the parent id, if S_Valence error

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate record
------------------------------------------------------------------------------*/

static int RcdValErr( SGlobPtr GPtr, OSErr type, UInt32 correct, UInt32 incorrect, CatalogNodeID parid )										/* the ParID, if needed */
{
	RepairOrderPtr	p;										/* the new node we compile */
	SInt16			n;										/* size of node we allocate */
	
	RcdError( GPtr, type );									/* first, record the error */
	
	if (type == E_DirVal)									/* if normal directory valence error */
		n = CatalogNameSize( &GPtr->CName, GPtr->isHFSPlus);
	else
		n = 0;												/* other errors don't need the name */
	
	p = AllocMinorRepairOrder(GPtr,n);						/* get the node */
	if (p==NULL) 											/* quit if out of room */
		return (R_NoMem);
	
	p->type			= type;									/* save error info */
	p->correct		= correct;
	p->incorrect	= incorrect;
	p->parid		= parid;
	
	if ( n != 0 ) 											/* if name needed */
		CopyCatalogName( (const CatalogName *) &GPtr->CName, (CatalogName*)&p->name, GPtr->isHFSPlus );
	
	GPtr->CatStat |= S_Valence;								/* set flag to trigger repair */
	
	return( noErr );										/* successful return */
}

#ifdef INVESTIGATE

#if(0)	//	We just check and fix them in SRepair.c
/*------------------------------------------------------------------------------

Name:		RcdOrphanedExtentErr 

Function:	Allocates a RepairOrder node and linkg it into the 'GPtr->RepairP'
			list, to describe an locked volume name for possible repair.

Input:		GPtr		- ptr to scavenger global data
			type		- error code (E_xxx), which should be >0
			incorrect	- the incorrect file flags as found in file record

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate record
------------------------------------------------------------------------------*/

static OSErr RcdOrphanedExtentErr ( SGlobPtr GPtr, SInt16 type, void *theKey )
{
	RepairOrderPtr	p;										/* the new node we compile */
	SInt16			n;										/* size of node we allocate */
	
	RcdError( GPtr,type );									/* first, record the error */
	
	if ( GPtr->isHFSPlus )
		n = sizeof( LargeExtentKey );
	else
		n = sizeof( SmallExtentKey );
	
	p = AllocMinorRepairOrder( GPtr, n );					/* get the node */
	if ( p == NULL ) 										/* quit if out of room */
		return( R_NoMem );
	
	BlockMoveData( theKey, p->name, n );					/* copy in the key */
	
	p->type = type;											/* save error info */
	
	GPtr->EBTStat |= S_OrphanedExtent;						/* set flag to trigger repair */
	
	return( noErr );										/* successful return */
}
#endif

#endif

/*------------------------------------------------------------------------------

Function:	VInfoChk - (Volume Info Check)

Function:	Verifies volume level information.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		VInfoChk	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

OSErr VInfoChk( SGlobPtr GPtr )
{
	OSErr					result;
	SInt32					i;
	UInt32					hint;
	UInt32					timeNow;
	UInt32					maxClump;
	ExtendedVCB				*calculatedVCB;
	CatalogRecord			record;
	CatalogKey				foundKey;
	UInt16					recSize;
	Boolean					isHFSPlus = GPtr->isHFSPlus;
	
	calculatedVCB = GPtr->calculatedVCB;
	GetDateTime( &timeNow );									/* get the current date/time */
	

	// locate the catalog record for the root directoryŠ
	result = GetBTreeRecord( kCalculatedCatalogRefNum, 0x8001, &foundKey, &record, &recSize, &hint );
	GPtr->TarID = kHFSCatalogFileID;							/* target = catalog */
	GPtr->TarBlock = hint;										/* target block = returned hint */
	if ( result != noErr )
	{
		result = IntError( GPtr, result );
		return( result );
	}

	
	if ( isHFSPlus )
	{
		VolumeHeader			*volumeHeader;
		VolumeHeader			*alternateVolumeHeader;
			
		GPtr->TarID		= AMDB_FNum;								//	target = alternate MDB
		GPtr->TarBlock	= GPtr->idSector;							//	target block =  ID block (Alternate VolumeHeader)

		result = GetVBlk( GPtr, GPtr->idSector, (void**)&alternateVolumeHeader );
		ReturnIfError( result );

		GPtr->TarID		= MDB_FNum;								/* target = MDB */
		GPtr->TarBlock	= MDB_BlkN;								/* target block = MDB */

		result = GetVBlk( GPtr, calculatedVCB->vcbAlBlSt+2, (void**)&volumeHeader );	//	VH is 3rd sector in
		ReturnIfError( result );
	
		maxClump = (calculatedVCB->totalBlocks / 4) * calculatedVCB->blockSize; /* max clump = 1/4 volume size */

		//	check out creation and last mod dates
		calculatedVCB->vcbCrDate	= alternateVolumeHeader->createDate;		//	use creation date in alt MDB
		calculatedVCB->vcbLsMod		= volumeHeader->modifyDate;					//	don't change last mod date
		calculatedVCB->checkedDate		= volumeHeader->checkedDate;			//	don't change checked date

		//	verify volume attribute flags
		if ( ((UInt16)volumeHeader->attributes & VAtrb_Msk) == 0 )
			calculatedVCB->vcbAtrb = (UInt16)volumeHeader->attributes;
		else 
			calculatedVCB->vcbAtrb = VAtrb_DFlt;
	
		//	verify allocation map ptr
		if ( volumeHeader->nextAllocation < calculatedVCB->totalBlocks )
			calculatedVCB->nextAllocation = volumeHeader->nextAllocation;
		else
			calculatedVCB->nextAllocation = 0;

		
		//	verify default clump size
		if ( (volumeHeader->rsrcClumpSize > 0) && (volumeHeader->rsrcClumpSize <= kMaxClumpSize) && ((volumeHeader->rsrcClumpSize % calculatedVCB->blockSize) == 0) )
			calculatedVCB->vcbClpSiz = volumeHeader->rsrcClumpSize;
		else if ( (alternateVolumeHeader->rsrcClumpSize > 0) && (alternateVolumeHeader->rsrcClumpSize <= kMaxClumpSize) && ((alternateVolumeHeader->rsrcClumpSize % calculatedVCB->blockSize) == 0) )
			calculatedVCB->vcbClpSiz = alternateVolumeHeader->rsrcClumpSize;
		else
			calculatedVCB->vcbClpSiz = 4 * calculatedVCB->blockSize;
	
		if ( calculatedVCB->vcbClpSiz > kMaxClumpSize )
			calculatedVCB->vcbClpSiz = calculatedVCB->blockSize;	/* for very large volumes, just use 1 allocation block */
	
		//	verify next CNode ID
		if ( (volumeHeader->nextCatalogID > calculatedVCB->vcbNxtCNID) && (volumeHeader->nextCatalogID <= (calculatedVCB->vcbNxtCNID + 4096)) )
			calculatedVCB->vcbNxtCNID = volumeHeader->nextCatalogID;
			
		//€€TBD location and unicode? volumename
		//	verify the volume name
		result = ChkCName( GPtr, (const CatalogName*) &foundKey.large.nodeName, isHFSPlus );

		//	verify last backup date and backup seqence number
		calculatedVCB->vcbVolBkUp = volumeHeader->backupDate;					/* don't change last backup date */
		calculatedVCB->vcbVSeqNum = 0;											/* don't change last backup sequence # */
		
		//	verify write count
		calculatedVCB->vcbWrCnt = volumeHeader->writeCount;						/* don't change write count */


		//	check out extent file clump size
		if ( ((volumeHeader->extentsFile.clumpSize % calculatedVCB->blockSize) == 0) && (volumeHeader->extentsFile.clumpSize <= maxClump) )
			calculatedVCB->vcbXTClpSiz = volumeHeader->extentsFile.clumpSize;
		else if ( ((alternateVolumeHeader->extentsFile.clumpSize % calculatedVCB->blockSize) == 0) && (alternateVolumeHeader->extentsFile.clumpSize <= maxClump) )
			calculatedVCB->vcbXTClpSiz = alternateVolumeHeader->extentsFile.clumpSize;
		else		
			calculatedVCB->vcbXTClpSiz = (alternateVolumeHeader->extentsFile.extents[0].blockCount * calculatedVCB->blockSize);
			
		//	check out extent file clump size
		if ( ((volumeHeader->catalogFile.clumpSize % calculatedVCB->blockSize) == 0) && (volumeHeader->catalogFile.clumpSize <= maxClump) )
			calculatedVCB->vcbCTClpSiz = volumeHeader->catalogFile.clumpSize;
		else if ( ((alternateVolumeHeader->catalogFile.clumpSize % calculatedVCB->blockSize) == 0) && (alternateVolumeHeader->catalogFile.clumpSize <= maxClump) )
			calculatedVCB->vcbCTClpSiz = alternateVolumeHeader->catalogFile.clumpSize;
		else
			calculatedVCB->vcbCTClpSiz = (alternateVolumeHeader->catalogFile.extents[0].blockCount * calculatedVCB->blockSize);
	
		//	check out allocations file clump size
		if ( ((volumeHeader->allocationFile.clumpSize % calculatedVCB->blockSize) == 0) && (volumeHeader->allocationFile.clumpSize <= maxClump) )
			calculatedVCB->allocationsClumpSize = volumeHeader->allocationFile.clumpSize;
		else if ( ((alternateVolumeHeader->allocationFile.clumpSize % calculatedVCB->blockSize) == 0) && (alternateVolumeHeader->allocationFile.clumpSize <= maxClump) )
			calculatedVCB->allocationsClumpSize = alternateVolumeHeader->allocationFile.clumpSize;
		else
			calculatedVCB->allocationsClumpSize = (alternateVolumeHeader->allocationFile.extents[0].blockCount * calculatedVCB->blockSize);
	

		BlockMoveData( volumeHeader->finderInfo, calculatedVCB->vcbFndrInfo, sizeof(calculatedVCB->vcbFndrInfo) );
	
		//	just copy cache parameters for now
		calculatedVCB->vcbEmbedSigWord				= 0;
		calculatedVCB->vcbEmbedExtent.startBlock	= 0;
		calculatedVCB->vcbEmbedExtent.blockCount	= 0;
	
		//	Now compare verified MDB info with MDB info on disk
		result = CompareVolumeHeader( GPtr );
		
		if ( result != noErr )
		{
			//WriteError (E_MDBDamaged, 0, 0);	//€€ log the error
			return( result );
		}

	}
	else		//	HFS
	{
		MasterDirectoryBlock	*mdbP;
		MasterDirectoryBlock	*alternateMDB;
		
		//	
		//	get volume name from BTree Key
		// 
		
		GPtr->TarID		= AMDB_FNum;								/* target = alternate MDB */
		GPtr->TarBlock	= GPtr->idSector;							/* target block =  alt MDB */

		result = GetVBlk( GPtr, GPtr->idSector, (void**)&alternateMDB );
		ReturnIfError( result );
	 
		GPtr->TarID		= MDB_FNum;								/* target = MDB */
		GPtr->TarBlock	= MDB_BlkN;								/* target block = MDB */
		
		result = GetVBlk( GPtr, MDB_BlkN, (void**)&mdbP );
		ReturnIfError( result );
	
		maxClump = (calculatedVCB->totalBlocks / 4) * calculatedVCB->blockSize; /* max clump = 1/4 volume size */

		//	check out creation and last mod dates
		calculatedVCB->vcbCrDate	= alternateMDB->drCrDate;		/* use creation date in alt MDB */	
		calculatedVCB->vcbLsMod		= mdbP->drLsMod;				/* don't change last mod date */

		//	verify volume attribute flags
		if ( (mdbP->drAtrb & VAtrb_Msk) == 0 )
			calculatedVCB->vcbAtrb = mdbP->drAtrb;
		else 
			calculatedVCB->vcbAtrb = VAtrb_DFlt;
	
		//	verify allocation map ptr
		if ( mdbP->drAllocPtr < calculatedVCB->totalBlocks )
			calculatedVCB->nextAllocation = mdbP->drAllocPtr;
		else
			calculatedVCB->nextAllocation = 0;

		//	verify default clump size
		if ( (mdbP->drClpSiz > 0) && (mdbP->drClpSiz <= maxClump) && ((mdbP->drClpSiz % calculatedVCB->blockSize) == 0) )
			calculatedVCB->vcbClpSiz = mdbP->drClpSiz;
		else if ( (alternateMDB->drClpSiz > 0) && (alternateMDB->drClpSiz <= maxClump) && ((alternateMDB->drClpSiz % calculatedVCB->blockSize) == 0) )
			calculatedVCB->vcbClpSiz = alternateMDB->drClpSiz;
		else
			calculatedVCB->vcbClpSiz = 4 * calculatedVCB->blockSize;
	
		if ( calculatedVCB->vcbClpSiz > kMaxClumpSize )
			calculatedVCB->vcbClpSiz = calculatedVCB->blockSize;	/* for very large volumes, just use 1 allocation block */
	
		//	verify next CNode ID
		if ( (mdbP->drNxtCNID > calculatedVCB->vcbNxtCNID) && (mdbP->drNxtCNID <= (calculatedVCB->vcbNxtCNID + 4096)) )
			calculatedVCB->vcbNxtCNID = mdbP->drNxtCNID;
			
		//	verify the volume name
		result = ChkCName( GPtr, (const CatalogName*) &calculatedVCB->vcbVN, isHFSPlus );
		if ( result == noErr )		
			if ( CmpBlock( mdbP->drVN, calculatedVCB->vcbVN, calculatedVCB->vcbVN[0] + 1 ) == 0 )
				BlockMoveData( mdbP->drVN, calculatedVCB->vcbVN, kMaxHFSVolumeNameLength + 1 ); /* ...we have a good one */		

		//	verify last backup date and backup seqence number
		calculatedVCB->vcbVolBkUp = mdbP->drVolBkUp;					/* don't change last backup date */
		calculatedVCB->vcbVSeqNum = mdbP->drVSeqNum;					/* don't change last backup sequence # */
		
		//	verify write count
		calculatedVCB->vcbWrCnt = mdbP->drWrCnt;						/* don't change write count */

		//	check out extent file and catalog clump sizes
		if ( ((mdbP->drXTClpSiz % calculatedVCB->blockSize) == 0) && (mdbP->drXTClpSiz <= maxClump) )
			calculatedVCB->vcbXTClpSiz = mdbP->drXTClpSiz;
		else if ( ((alternateMDB->drXTClpSiz % calculatedVCB->blockSize) == 0) && (alternateMDB->drXTClpSiz <= maxClump) )
			calculatedVCB->vcbXTClpSiz = alternateMDB->drXTClpSiz;
		else		
			calculatedVCB->vcbXTClpSiz = (alternateMDB->drXTExtRec[0].blockCount * calculatedVCB->blockSize);
			
		if ( ((mdbP->drCTClpSiz % calculatedVCB->blockSize) == 0) && (mdbP->drCTClpSiz <= maxClump) )
			calculatedVCB->vcbCTClpSiz = mdbP->drCTClpSiz;
		else if ( ((alternateMDB->drCTClpSiz % calculatedVCB->blockSize) == 0) && (alternateMDB->drCTClpSiz <= maxClump) )
			calculatedVCB->vcbCTClpSiz = alternateMDB->drCTClpSiz;
		else
			calculatedVCB->vcbCTClpSiz = (alternateMDB->drCTExtRec[0].blockCount * calculatedVCB->blockSize);
	
		//	just copy Finder info for now
		for ( i = 0; i < 8; i++ )
			calculatedVCB->vcbFndrInfo[i] = mdbP->drFndrInfo[i];
	
		//	just copy cache parameters for now
		calculatedVCB->vcbEmbedSigWord = mdbP->drEmbedSigWord;
		calculatedVCB->vcbEmbedExtent.startBlock = mdbP->drEmbedExtent.startBlock;
		calculatedVCB->vcbEmbedExtent.blockCount = mdbP->drEmbedExtent.blockCount;
	
		//	now compare verified MDB info with MDB info on disk
		result = CmpMDB( GPtr );
		if ( result !=noErr )
		{
			//WriteError (E_MDBDamaged, 0, 0);	//€€ log the error
			return( result );
		}
	}

	return( noErr );											/* all done */
	
}	/* end of VInfoChk */

#ifdef INVESTIGATE

/*------------------------------------------------------------------------------

Function:	CustomIconCheck - (Custom Icon Check)

Function:	Makes sure there is an icon file when the custom icon bit is set.
			
Input:		GPtr		-	pointer to scavenger global area
			folderID	-	ID of folder with hasCustomIcon bit set.
			frFlags		-	current folder flags

Output:		OSErr	-	function result:			
								0	= no error
								n 	= error code
								
Assume:
			GPtr->CName		folder name
			GPtr->ParID		folders parent ID

------------------------------------------------------------------------------*/

static	OSErr CustomIconCheck ( SGlobPtr GPtr, CatalogNodeID folderID, UInt16 frFlags )
{
	UInt32				hint;
	UInt16				recordSize;
	OSErr				err;
	CatalogKey			key;
	CatalogKey			foundKey;
	CatalogName			iconFileName;
	CatalogRecord		record;
	BTreeIterator		savedIterator;
	BTreeControlBlock	*btcb;
	Boolean				isHFSPlus			= GPtr->isHFSPlus;

	// We can call DFA_PrepareInputName() because we know "\pIcon\n" is the same in all text encodings and languages
	DFA_PrepareInputName( (ConstStr31Param)"\pIcon\n", isHFSPlus, &iconFileName );
	btcb	= (BTreeControlBlock *) (GetFileControlBlock(kCalculatedCatalogRefNum))->fcbBTCBPtr;

	BuildCatalogKey( folderID, &iconFileName, isHFSPlus, &key );

	//	Save and restore BTree iterator around call to SearchBTreeRecord()
	BlockMoveData( &btcb->lastIterator, &savedIterator, sizeof(BTreeIterator) );
	err = SearchBTreeRecord( kCalculatedCatalogRefNum, &key, kNoHint, &foundKey, &record, &recordSize, &hint );
	BlockMoveData( &savedIterator, &btcb->lastIterator, sizeof(BTreeIterator) );
	
	if ( err == btNotFound )
	{
		RcdCustomIconErr( GPtr, E_MissingCustomIcon, frFlags );
		err	= noErr;
	}
	
	return( err );
}

#endif

/*------------------------------------------------------------------------------

Name:		RcdCustomIconErr 

Function:	Allocates a RepairOrder node and linkg it into the 'GPtr->RepairP'
			list, to describe a missing custom icon for possible repair.

Input:		GPtr		- ptr to scavenger global data
			type		- error code (E_xxx), which should be > 0
			incorrect	- the incorrect finder flags as found in directory record

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate record
------------------------------------------------------------------------------*/

static int RcdCustomIconErr( SGlobPtr GPtr, SInt16 type, UInt32 incorrect )									/* for a consistency check */
{
	RepairOrderPtr	p;										/* the new node we compile */
	int				n;										/* size of node we allocate */
	
	RcdError( GPtr, type );									/* first, record the error */
	
	n = CatalogNameSize( &GPtr->CName, GPtr->isHFSPlus );
	
	p = AllocMinorRepairOrder( GPtr, n );					/* get the node */
	if ( p == NULL ) 										/* quit if out of room */
		return( R_NoMem );
	
	CopyCatalogName( (const CatalogName *) &GPtr->CName, (CatalogName*)&p->name, GPtr->isHFSPlus );	/* copy in the name */
	
	p->type				= type;								/* save error info */
	p->correct	 		= incorrect & ~kHasCustomIcon;		/* mask off the custom icon bit */
	p->incorrect		= incorrect;
	p->maskBit			= (UInt16)kHasCustomIcon;
	p->parid			= GPtr->ParID;
	
	GPtr->CatStat |= S_BadCustomIcon;						/* set flag to trigger repair */
	
	return( noErr );										/* successful return */
}


/*------------------------------------------------------------------------------

Function:	VLockedChk - (Volume Name Locked Check)

Function:	Makes sure the volume name isn't locked.  If it is locked, generate a repair order.

			This function is not called if file sharing is operating.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		VInfoChk	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

OSErr VLockedChk ( SGlobPtr GPtr )
{
	UInt32				hint;
	CatalogKey			foundKey;
	CatalogRecord		record;
	UInt16				recSize;
	OSErr				result;
	UInt16				frFlags;
	Boolean				isHFSPlus		= GPtr->isHFSPlus;
	ExtendedVCB			*calculatedVCB	= GPtr->calculatedVCB;
	
	GPtr->TarID		= kHFSCatalogFileID;								/* target = catalog file */
	GPtr->TarBlock	= 0;									/* no target block yet */
	
	//
	//	locate the catalog record for the root directory
	//
	result = GetBTreeRecord( kCalculatedCatalogRefNum, 0x8001, &foundKey, &record, &recSize, &hint );
	
	//	put the vloume name in the VCB
	if ( isHFSPlus == false )
	{
		BlockMoveData( foundKey.small.nodeName, calculatedVCB->vcbVN, sizeof(calculatedVCB->vcbVN) );
	}
	else if ( GPtr->pureHFSPlusVolume == false )
	{
		MasterDirectoryBlock	*mdbP;
		
		result = GetVBlk( GPtr, MDB_BlkN, (void**)&mdbP );
		ReturnIfError( result );
		BlockMoveData( mdbP->drVN, calculatedVCB->vcbVN, sizeof(mdbP->drVN) );
	}
	else		//	Because we don't have the unicode converters, just fill it with a dummy name.
	{
		BlockMoveData( "\pPure HFS Plus", calculatedVCB->vcbVN, sizeof(Str27) );
	}
	
		
	GPtr->TarBlock = hint;
	if ( isHFSPlus )
		CopyCatalogName( (const CatalogName *)&foundKey.large.nodeName, &GPtr->CName, isHFSPlus );
	else
		CopyCatalogName( (const CatalogName *)&foundKey.small.nodeName, &GPtr->CName, isHFSPlus );
	
	if ( result)		//€€ We would have died long before this case shows up!, we can get rid of the repair also
	{
		RcdError( GPtr, E_EntryNotFound );
		return( E_EntryNotFound );
	}

	if ( (record.recordType == kLargeFolderRecord) || (record.recordType == kSmallFolderRecord) )
	{
		frFlags = record.recordType == kLargeFolderRecord ? record.largeFolder.userInfo.frFlags : record.smallFolder.userInfo.frFlags;
	
		if ( frFlags & fNameLocked )												// name locked bit set?
			RcdNameLockedErr( GPtr, E_LockedDirName, frFlags );
	}	
	
	return( noErr );
}


/*------------------------------------------------------------------------------

Name:		RcdNameLockedErr 

Function:	Allocates a RepairOrder node and linkg it into the 'GPtr->RepairP'
			list, to describe an locked volume name for possible repair.

Input:		GPtr		- ptr to scavenger global data
			type		- error code (E_xxx), which should be >0
			incorrect	- the incorrect file flags as found in file record

Output:		0 			- no error
			R_NoMem		- not enough mem to allocate record
------------------------------------------------------------------------------*/

static int RcdNameLockedErr( SGlobPtr GPtr, SInt16 type, UInt32 incorrect )									/* for a consistency check */
{
	RepairOrderPtr	p;										/* the new node we compile */
	int				n;										/* size of node we allocate */
	
	RcdError( GPtr, type );									/* first, record the error */
	
	n = CatalogNameSize( &GPtr->CName, GPtr->isHFSPlus );
	
	p = AllocMinorRepairOrder( GPtr, n );					/* get the node */
	if ( p==NULL ) 											/* quit if out of room */
		return ( R_NoMem );
	
	CopyCatalogName( (const CatalogName *) &GPtr->CName, (CatalogName*)&p->name, GPtr->isHFSPlus );
	
	p->type				= type;								/* save error info */
	p->correct			= incorrect & ~fNameLocked;			/* mask off the name locked bit */
	p->incorrect		= incorrect;
	p->maskBit			= (UInt16)fNameLocked;
	p->parid			= 1;
	
	GPtr->CatStat |= S_LockedDirName;						/* set flag to trigger repair */
	
	return( noErr );										/* successful return */
}


OSErr	CheckVolumeBitMap( SGlobPtr GPtr )
{
	OSErr				err;
	VolumeBitMapHeader	*volumeBitMap;
	SInt32				currentBufferNumber;
	Boolean				isHFSPlus = GPtr->isHFSPlus;
	
	//	Set up the fcb for the HFS+ Allocations file (Volume BitMap file)
	if ( isHFSPlus )
	{
		VolumeHeader			*volumeHeader;
		UInt32					calculatedBlockCount;

		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );				//	get the alternate VH
		ReturnIfError( err );
	
		BlockMoveData( (Ptr)volumeHeader->allocationFile.extents, (Ptr)GPtr->extendedAllocationsFCB->extents, sizeof(LargeExtentRecord) );

		err = CheckFileExtents( GPtr, kHFSAllocationFileID, 0, false, (void *)GPtr->extendedAllocationsFCB->extents, &calculatedBlockCount );	//	check out extent info
		ReturnIfError( err );													//	error, invalid file allocation
	
		if (volumeHeader->allocationFile.totalBlocks != calculatedBlockCount )						//	check out the PEOF
		{
			RcdError( GPtr, E_ExtPEOF );
			return( E_ExtPEOF );													//	error, invalid extent file PEOF
		}
		else
		{
			GPtr->calculatedAllocationsFCB->fcbEOF  = volumeHeader->allocationFile.logicalSize.lo;							//	Set allocationFile tree's LEOF
			GPtr->calculatedAllocationsFCB->fcbPLen = volumeHeader->allocationFile.totalBlocks * volumeHeader->blockSize;	//	Set allocationFile tree's PEOF
		}
	}
	
	
	//
	//	Loop through each of the buffers to cover the entire bit map
	//
	volumeBitMap = GPtr->volumeBitMapPtr;
	for ( currentBufferNumber = 0 ; currentBufferNumber < volumeBitMap->numberOfBuffers ; currentBufferNumber++ )
	{

		err = CreateVolumeBitMapBuffer( GPtr, currentBufferNumber );
		if ( err != noErr )
		{
			M_DebugStr("\p Trouble in CreateVolumeBitMapBuffer");
			ReturnIfError( err );
		}

		//	Volume BitMap buffer is full, compare it to the on disk version.
		err = CompareVolumeBitMap( GPtr, volumeBitMap->currentBuffer );
		
		if ( err != noErr )
		{
			if ( err == E_VBMDamaged )				//	it's been marked for repair
			{
				err = noErr;
			}
			else
				return( err );
		}
	}
	
	return( err );
}


OSErr	CreateVolumeBitMapBuffer( SGlobPtr GPtr, SInt32 bufferNumber )
{
	OSErr				err;
	void				*extents;
	SInt16				selectionCode;
	UInt32				hint;
	UInt16 				recordSize;
	UInt32				blocksUsed;
	ExtendedVCB			*calculatedVCB	= GPtr->calculatedVCB;
	VolumeBitMapHeader	*volumeBitMap	= GPtr->volumeBitMapPtr;
	Boolean				isHFSPlus		= GPtr->isHFSPlus;
	Boolean				hasAttributes	= false;


	//	First check if this is even necessary.  Since this is the only routine
	//	that sets volumeBitMap->currentBuffer, if it is already equal to bufferNumber
	//	that buffer has already been created.
	if ( volumeBitMap->currentBuffer == bufferNumber )
		return( noErr );

	volumeBitMap->currentBuffer	= bufferNumber;									//	Set the current buffer
	volumeBitMap->bitMapRecord[volumeBitMap->currentBuffer].count		= 0;
	volumeBitMap->bitMapRecord[volumeBitMap->currentBuffer].isAMatch	= false;


	ClearMemory ( volumeBitMap->buffer, volumeBitMap->bufferSize );				//	start with an empty bitmap buffer
	

	//	Allocate the extents in the volume bitmap
	extents = isHFSPlus == true ? (void *)GPtr->extendedExtentsFCB->extents : (void *)GPtr->calculatedExtentsFCB->fcbExtRec;	
	err = CheckFileExtents( GPtr, kHFSExtentsFileID, 0, true, extents, &blocksUsed );
	ReturnIfError( err );
	
	//	Allocate the catalog in the volume bitmap
	extents = isHFSPlus == true ? (void *)GPtr->extendedCatalogFCB->extents : (void *)GPtr->calculatedCatalogFCB->fcbExtRec;	
	err = CheckFileExtents( GPtr, kHFSCatalogFileID, 0, true, extents, &blocksUsed );
	ReturnIfError( err );
	
	//	Allocate the HFS+ private files
	if ( isHFSPlus )
	{
		VolumeHeader			*volumeHeader;
		
		err = GetVBlk( GPtr, GPtr->idSector, (void**)&volumeHeader );			//	get the alternate VH
		ReturnIfError( err );

		//	Allocate the extended attributes file in the volume bitmap
		if ( volumeHeader->attributesFile.totalBlocks != 0 )
		{
			hasAttributes = true;
			err = CheckFileExtents( GPtr, kHFSAttributesFileID, 0, true, volumeHeader->attributesFile.extents, &blocksUsed );
			ReturnIfError( err );
		}
		//	Allocate the Startup file in the volume bitmap
		if ( volumeHeader->startupFile.totalBlocks != 0 )
		{
			err = CheckFileExtents( GPtr, kHFSStartupFileID, 0, true, volumeHeader->startupFile.extents, &blocksUsed );
			ReturnIfError( err );
		}
	}

	//	Adding clause check for the bad block entry in the extent overflow file, and mark them off in the bitmap.
	{
		LargeExtentKey		extentKey;
		LargeExtentRecord	extentRecord;

		//	Set up the extent key
		if ( isHFSPlus )
		{
			extentKey.keyLength		= kLargeExtentKeyMaximumLength;
			extentKey.forkType		= 0;
			extentKey.pad				= 0;
			extentKey.fileID			= kHFSBadBlockFileID;
			extentKey.startBlock		= 0;
		}
		else
		{
			SmallExtentKey *hfsKey	= (SmallExtentKey*) &extentKey;
	
			hfsKey->keyLength	= kSmallExtentKeyMaximumLength;
			hfsKey->forkType	= 0;
			hfsKey->fileID		= kHFSBadBlockFileID;
			hfsKey->startBlock	= 0;
		}

		err = SearchBTreeRecord( kCalculatedExtentRefNum, &extentKey, kNoHint, (void *) &extentKey, (void *) &extentRecord, &recordSize, &hint );

		if ( err == noErr )
		{
			err = CheckFileExtents( GPtr, kHFSBadBlockFileID, 0, true, (void *) &extentRecord, &blocksUsed );
			ReturnIfError( err );
		}
		else if ( err == btNotFound )
		{
			err = noErr;
		}
	}

	
	if ( isHFSPlus )
	{
		UInt16	burnedBlocks;
		
		//	Allocate the BitMap in the volume bitmap
		extents = (void *)GPtr->extendedAllocationsFCB->extents;	
		err = CheckFileExtents( GPtr, kHFSAllocationFileID, 0, true, extents, &blocksUsed );
		ReturnIfError( err );

		//	Allocate the VolumeHeader in the volume bitmap.
		//	Since the VH is the 3rd sector in we may need to burn the allocation
		//	blocks before it, if there are any.
		if ( calculatedVCB->blockSize == 512 )
			burnedBlocks	= 2;
		else if ( calculatedVCB->blockSize == 1024 )
			burnedBlocks	= 1;
		else
			burnedBlocks	= 0;
		
		err = AllocExt( GPtr, 0, 1 + burnedBlocks );
		ReturnIfError( err );

		//	Allocate the AlternateVolumeHeader in the volume bitmap.
		if ( calculatedVCB->blockSize == 512 )
			burnedBlocks	= 1;
		else
			burnedBlocks	= 0;
		
		err = AllocExt( GPtr, calculatedVCB->totalBlocks - 1 - burnedBlocks, 1 + burnedBlocks );
		ReturnIfError( err );
	}


	//
	//	Sequentially traverse the entire catalog
	//
	{
		CatalogKey			foundKey;
		CatalogRecord		record;

		selectionCode = 0x8001;										//	first record
		
		while ( err == noErr )
		{
 			err = CheckForStop( GPtr ); ReturnIfError( err );		//	Permit the user to interrupt
	
			//€€ Will we need to SearchBTree with the hint, then get the next record, cuz CheckFileExtents changes itterator
			GPtr->TarID = kHFSCatalogFileID;						//	target = catalog file
			err = GetBTreeRecord( kCalculatedCatalogRefNum, selectionCode, &foundKey, &record, &recordSize, &hint );
	
			GPtr->TarBlock = hint;									//	set target block
			
			if ( err != noErr )
			{
				if ( err == btNotFound )
				{
					err = noErr;									//	no more catalog records
					break;											//	continue the rest of the checks
				}
				else
				{
					return( err );
				}
			}
			
			selectionCode = 1;										//	next record
			
			#if ( StandAloneEngine )
				GPtr->itemsProcessed = U64Add( GPtr->itemsProcessed, U64SetU( 1 ) );
			#endif
	
			if ( record.recordType == kLargeFileRecord )
			{
				LargeCatalogFile	*largeCatalogFileP = (LargeCatalogFile *) &record;
				err = CheckFileExtents( GPtr, largeCatalogFileP->fileID, 0, true, (void *)largeCatalogFileP->dataFork.extents, &blocksUsed );
				if ( err == noErr )
					err = CheckFileExtents( GPtr, largeCatalogFileP->fileID, 0xFF, true, (void *)largeCatalogFileP->resourceFork.extents, &blocksUsed );
			}
			else if ( record.recordType == kSmallFileRecord )
			{
				SmallCatalogFile	*smallCatalogFileP = (SmallCatalogFile *) &record;
				err = CheckFileExtents( GPtr, smallCatalogFileP->fileID, 0, true, (void *)smallCatalogFileP->dataExtents, &blocksUsed );
				if ( err == noErr )
					err = CheckFileExtents( GPtr, smallCatalogFileP->fileID, 0xFF, true, (void *)smallCatalogFileP->rsrcExtents, &blocksUsed );
			}
		}
	}
	//
	//	If there is an attributes B-Tree, traverse all the records
	//
	if (hasAttributes)
	{
		UInt32			recordType;
		AttributeKey	foundKey;
		unsigned char	record[kAttributeMinimumNodeSize];

		selectionCode = 0x8001;										//	first record
		
		while ( err == noErr )
		{
 			err = CheckForStop( GPtr ); ReturnIfError( err );		//	Permit the user to interrupt

			GPtr->TarID = kHFSAttributesFileID;						//	target = attributes file
			err = GetBTreeRecord( kCalculatedAttributesRefNum, selectionCode, &foundKey, record, &recordSize, &hint );

			GPtr->TarBlock = hint;									//	set target block
			
			if ( err != noErr )
			{
				if ( err == btNotFound )
				{
					err = noErr;									//	no more records
					break;											//	continue the rest of the checks
				}
				else
				{
					return( err );
				}
			}
			
			selectionCode = 1;										//	next record
			
			#if ( StandAloneEngine )
				GPtr->itemsProcessed = U64Add( GPtr->itemsProcessed, U64SetU( 1 ) );
			#endif

			recordType = ((AttributeRecord *)record)->recordType;
			
			if ( recordType == kAttributeForkData )
			{
				//	Mark the extents as used in the bitmap
				AttributeForkData	*attributeRecord = (AttributeForkData *) record;
				SInt16				i;
				
				err = ChkExtRec( GPtr, attributeRecord->theFork.extents );	//	checkout the extent record first
				if (err == noErr)
				{
					for (i=0; i<GPtr->numExtents ; i++ )
					{
						err = AllocExt( GPtr, attributeRecord->theFork.extents[i].startBlock, attributeRecord->theFork.extents[i].blockCount );
						if ( err != noErr )
						{
							M_DebugStr("\p Problem Allocating Extents");
							return( err );
						}
					}
				}
			}
			else if ( recordType == kAttributeExtents )
			{
				//	Mark the extents as used in the bitmap
				AttributeExtents	*attributeRecord = (AttributeExtents *) record;
				SInt16				i;
				
				err = ChkExtRec( GPtr, attributeRecord->extents );			//	checkout the extent record first
				if (err == noErr)
				{
					for (i=0; i<GPtr->numExtents ; i++ )
					{
						err = AllocExt( GPtr, attributeRecord->extents[i].startBlock, attributeRecord->extents[i].blockCount );
						if ( err != noErr )
						{
							M_DebugStr("\p Problem Allocating Extents");
							return( err );
						}
					}
				}
			}
		}
	}

	volumeBitMap->bitMapRecord[volumeBitMap->currentBuffer].processed = true;
	
	return( err );
}


/*------------------------------------------------------------------------------

Function:	CheckFileExtents - (Check File Extents)

Function:	Verifies the extent info for a file.
			
Input:		GPtr		-	pointer to scavenger global area
			checkBitMap	-	Should we also check the BitMap
			fileNumber	-	file number
			forkType	-	fork type ($00 = data fork, $FF = resource fork)
			extents		-	ptr to 1st extent record for the file

Output:
			CheckFileExtents	-	function result:			
								noErr	= no error
								n 		= error code
			blocksUsed	-	number of allocation blocks allocated to the file
------------------------------------------------------------------------------*/

OSErr	CheckFileExtents( SGlobPtr GPtr, UInt32 fileNumber, UInt8 forkType, Boolean checkBitMap, void *extents, UInt32 *blocksUsed )
{
	UInt32				blockCount;
	UInt32				extentBlockCount;
	UInt32				extentStartBlock;
	UInt32				hint;
	LargeExtentKey		key;
	LargeExtentKey		extentKey;
	LargeExtentRecord	extentRecord;
	UInt16 				recSize;
	OSErr				err;
	SInt16				i;
	Boolean				firstRecord;
	Boolean				isHFSPlus;

	isHFSPlus	= GPtr->isHFSPlus;
	firstRecord	= true;
	err			= noErr;
	blockCount	= 0;
	
	while ( (extents != nil) && (err == noErr) )
	{
		err = ChkExtRec( GPtr, extents );			//	checkout the extent record first
		if ( err != noErr )							//	Bad extent record, don't mark it
			break;
			
		for ( i=0 ; i<GPtr->numExtents ; i++ )		//	now checkout the extents
		{
			//	HFS+/HFS moving extent fields into local variables for evaluation
			if ( isHFSPlus == true )
			{
				extentBlockCount = ((LargeExtentDescriptor *)extents)[i].blockCount;
				extentStartBlock = ((LargeExtentDescriptor *)extents)[i].startBlock;
			}
			else
			{
				extentBlockCount = ((SmallExtentDescriptor *)extents)[i].blockCount;
				extentStartBlock = ((SmallExtentDescriptor *)extents)[i].startBlock;
			}
	
			if ( extentBlockCount == 0 )
				break;
			
			if ( checkBitMap )
			{
				err = AllocExt( GPtr, extentStartBlock, extentBlockCount );
				if ( err != noErr )
				{
					M_DebugStr("\p Problem Allocating Extents");
					return( err );
				}
			}
			
			
			blockCount += extentBlockCount;
		}
		
		if ( fileNumber == kHFSExtentsFileID )		//	Extents file has no extents
			break;
			
		if ( firstRecord == true )
		{
			firstRecord = false;

			//	Set up the extent key
			BuildExtentKey( isHFSPlus, forkType, fileNumber, blockCount, (void *)&key );

			err = SearchBTreeRecord( kCalculatedExtentRefNum, &key, kNoHint, (void *) &extentKey, (void *) &extentRecord, &recSize, &hint );
			
			if ( err == btNotFound )
			{
				err = noErr;								//	 no more extent records
				extents = nil;
				break;
			}
			else if ( err != noErr )
			{
		 		err = IntError( GPtr, err );		//	error from SearchBTreeRecord
				return( err );
			}
		}
		else
		{
			err = GetBTreeRecord( kCalculatedExtentRefNum, 1, &extentKey, extentRecord, &recSize, &hint );
			
			if ( err == btNotFound )
			{
				err = noErr;								//	 no more extent records
				extents = nil;
				break;
			}
			else if ( err != noErr )
			{
		 		err = IntError( GPtr, err ); 		/* error from BTGetRecord */
				return( err );
			}
			
			//	Check same file and fork
			if ( isHFSPlus )
			{
				if ( (extentKey.fileID != fileNumber) || (extentKey.forkType != forkType) )
					break;
			}
			else
			{
				if ( (((SmallExtentKey *) &extentKey)->fileID != fileNumber) || (((SmallExtentKey *) &extentKey)->forkType != forkType) )
					break;
			}
		}
		
		extents = (void *) &extentRecord;
	}
	
	*blocksUsed = blockCount;
	
	return( err );
}


void	BuildExtentKey( Boolean isHFSPlus, UInt8 forkType, CatalogNodeID fileNumber, UInt32 blockNumber, void * key )
{
	if ( isHFSPlus )
	{
		LargeExtentKey *hfsPlusKey	= (LargeExtentKey*) key;
		
		hfsPlusKey->keyLength	= kLargeExtentKeyMaximumLength;
		hfsPlusKey->forkType	= forkType;
		hfsPlusKey->pad			= 0;
		hfsPlusKey->fileID		= fileNumber;
		hfsPlusKey->startBlock	= blockNumber;
	}
	else
	{
		SmallExtentKey *hfsKey	= (SmallExtentKey*) key;

		hfsKey->keyLength		= kSmallExtentKeyMaximumLength;
		hfsKey->forkType		= forkType;
		hfsKey->fileID			= fileNumber;
		hfsKey->startBlock		= (UInt16) blockNumber;
	}
}

