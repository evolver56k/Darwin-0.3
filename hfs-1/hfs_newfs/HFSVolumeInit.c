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
	File:		HFSVolumeInit.c

	Contains:	Initialization code for HFS and HFS Plus volumes.

 	Version:	HFS Plus 1.0

	Copyright:	© 1984-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			File System

	Writers:

		(NG)	Nitin Ganatra
		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	30-Jul-1998	Don Brady			Added to hfs_newfs project.

	  <CS31>	  2/9/98	msd		Include HFSVolumes.h instead of HFSVolumesPriv.h.
	  <CS30>	 1/19/98	DSH		In InitHFSVolume() dispose memory allocated to the mdb on exit.
	  <CS29>	10/29/97	djb		Add allocation file clump size to HFS Plus default settings.
	  <CS28>	10/23/97	msd		Bug 1685113. Add a createDate parameter to
									InitializeHFSPlusVolume.
	  <CS27>	10/19/97	msd		Bug 1684586. GetCatInfo and SetCatInfo use only contentModDate.
	  <CS26>	10/17/97	djb		Handle kTECUnmappableElementErr in volume name conversion.
	  <CS25>	10/13/97	DSH		Add some room for the catalog file to grow, adjusted
									nextAllocation field.
	  <CS24>	10/13/97	djb		Use MapEncodingToIndex macro when setting encoding bitmap.
	  <CS23>	 9/18/97	NG		Don't call Gestalt(osAttr) to check if Temp Mem is available.
									Instead, use GetExpandMemProcessMgrExists.
	  <CS22>	 9/15/97	djb		InitUnicodeConverter now takes a boolean input.
	  <CS21>	 9/11/97	djb		Moved alignment calculations into InitMasterDirectoryBlock.
	  <CS20>	  9/7/97	djb		Set textEncoding field for root folder.
	  <CS19>	  9/5/97	msd		Add a parameter to InitHFSVolume to indicate alignment of
									allocation blocks.
	  <CS18>	  9/4/97	msd		HFS Plus Extents B-Tree uses 16-bit key length ("large" keys).
									Fix calculation bug in MarkBitInAllocationBuffer.
	  <CS17>	  9/2/97	DSH		Moving HFSPlusVolumeHeader to sector 2 of volume and alternate VH to
									2nd to last sector.
	  <CS16>	  8/5/97	msd		When creating the volume header, set the version to
									kHFSPlusVersion.
	  <CS15>	 7/23/97	msd		InitBTreeHeader is setting the backward link, not the forward
									link, of the header node to point to an extra map node.
	  <CS14>	 7/16/97	DSH		FilesInternal.x -> FileMgrInternal.x to avoid name collision
	  <CS13>	 7/15/97	msd		Bug #1664103. Use 32-bit value for sector numbers in
									InitHFSVolume.
	  <CS12>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	  <CS11>	  7/7/97	djb		Call InitUnicodeConverter before using ConvertHFSNameToUnicode.
	  <CS10>	  7/7/97	msd		Bug #1666458. Initialization was hanging or crashing if the
									Extents tree node size was larger than the Catalog node size;
									the node buffer that was allocated was too small.
	   <CS9>	 6/20/97	msd		Use contentModDate and attributeModDate fields instead of
									modifyDate.
	   <CS8>	 6/18/97	djb		Initialize volume encodingsBitmap.
		 <7>	 6/13/97	djb		Get in sync with HFS Plus format changes. Change
									HFSNameToUnicode call to ConvertHFSNameToUnicode call.
	   <CS6>	  6/9/97	msd		Change calls to GetDateTime to use GetTimeUTC or GetTimeLocal.
		 <5>	  6/6/97	DSH		In ClearDisk() pin bufferSizeInSectors to 256K, VM guarantees
									256K is holdable.
	   <CS4>	  6/4/97	djb		Fix catalog key lengths. Remove test folder code.
	   <CS3>	 5/28/97	msd		Make InitBTreeHeader an external routine; added an attributes
									parameter as input.
	   <CS2>	 5/16/97	msd		In InitializeHFSPlusVolume(), change a Ptr to (UInt8 *) since it
									is manipulating unsigned bytes.
	   <CS1>	 4/24/97	djb		first checked in
	 <HFS22>	 4/12/97	DSH		Mark AltVH in allocations file, and update freeBlocks
									accordingly.
	 <HFS21>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS20>	 3/31/97	djb		Change ClearBuffer to ClearMemory.
	 <HFS19>	 3/27/97	djb		Unicode string lengths are now returned as a byte count.
	 <HFS18>	 3/24/97	DSH		InitRootFolder() was setting thread offset to
									sizeof(HFSPlusCatalogThread), overflowing 512 byte nodes, changed
									it to use calculated size of variable sized thread.
	 <HFS17>	  3/3/97	djb		New API - defaults are always passed in now.
	 <HFS16>	 2/27/97	msd		Make sure BTree EOF is a multiple of the node size.
	 <HFS15>	 2/19/97	djb		HFS+ B-Tree node is 4K. Use B-tree header from HFSBTreesPriv.h
	 <HFS14>	 2/10/97	msd		If making the test folder in the root, need to increment the
									root directory's valence.
	 <HFS13>	 1/23/97	DSH		Changed location of AlternateVolumeHeader to be last sector in
									HFS+ partition.
	 <HFS12>	 1/16/97	djb		Removed DebugStr calls. Set volume attributes to 0x0100.
	 <HFS11>	 1/10/97	djb		Another fix to InitBitmap - it wasn't clearing enough bytes.
	 <HFS10>	  1/9/97	djb		Get in sync with new headers. Fixed InitBitmap so it can handle
									more than one sector worth of bits.
	  <HFS9>	  1/6/97	djb		HFS+ B-trees had the wrong maxKeyLength values. Fixed HFS+
									catalog key lengths. Added TestFolder for debugging HFS+
									volumes.
	  <HFS8>	  1/3/97	djb		Integrated latest HFSVolumesPriv.h changes.
	  <HFS7>	12/13/96	djb		Switching to HFSUnicodeWrappers...
	  <HFS6>	12/10/96	msd		Check PRAGMA_LOAD_SUPPORTED before loading precompiled headers.
	  <HFS5>	 12/4/96	DSH		Precompiled Headers
	  <HFS4>	 12/4/96	djb		Don't set reserved fields (drVCSize, etc).
	  <HFS3>	 12/2/96	djb		MyConvertPStringToUnicode was not setting result code.
	  <HFS2>	11/27/96	djb		Fixed rouding bug in CalcBTreeClumpSize. Also set volume unmount
									OK bit!
	  <HFS1>	11/25/96	djb		first checked in
*/

#if TARGET_OS_MAC

#include <Devices.h>
#include <DiskInit.h>
#include <ExpandMemPriv.h>
#include <Files.h>
#include <FSM.h>
#include <Gestalt.h>
#include <Memory.h>
#include <Types.h>
#include <HFSVolumes.h>
#include <FileMgrInternal.h>
#include <HFSBTreesPriv.h>
#include <DiskInitInternal.h>
#include "HFSUnicodeWrappers.h"

#else

#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "MacOSTypes.h"
#include "HFSVolumes.h"
#include "HFSBtreesPriv.h"
#include "newfs_hfs.h"

#endif


// ConstantsÉ

enum {
	kWriteSeqNum			= 2,
	kHeaderBlocks			= 3,				// boot blocks + MDB
	kTailBlocks				= 2,				// blocks used by alternate MDB
	kMDBStart				= 2,
	kVolBitMapStart			= kHeaderBlocks,
	
	kOneSector				= 1
};


// prototypes for our local routinesÉ

static void		InitMasterDirectoryBlock (HFSDefaults *defaults, UInt32 driveBlocks, UInt32 alignment,
										  ConstStr31Param volName, HFSMasterDirectoryBlock *mdb);
static void		InitVolumeHeader (HFSPlusDefaults *defaults, UInt32 sectors, HFSPlusVolumeHeader *header);
static void		InitBTreeHeader (UInt32 fileSize, UInt32 clumpSize, UInt16 nodeSize, UInt16 recordCount,
								UInt16 keySize, UInt32 attributes, UInt32 *mapNodes, void *buffer);
static void		InitBitmap (UInt16 alBlksUsed, UInt32 sectors, Byte *buffer);
static void		SetupCatalogRecords (ConstStr31Param theVolName, UInt16 btNodeSize, void * buffer);
static void		InitRootFolder (ConstHFSUniStr255Param volName, UInt16 btNodeSize, void * nodeBuffer);
static OSErr	WriteMapNodes (const DriveInfo *driveInfo, UInt32 diskStart, UInt32 firstMapNode,
								UInt32 mapNodes, UInt16 btNodeSize, void *buffer);
static OSErr	ClearDisk (const DriveInfo *driveInfo, UInt32 startingSector, UInt32 numberOfSectors, void *sectorData);
static OSErr	WriteToDisk (const DriveInfo *driveInfo, UInt32 startingSector, UInt32 numberOfSectors, void *sectorData);
static UInt32	Largest ( UInt32 a, UInt32 b, UInt32 c, UInt32 d );
static void		MarkBitInAllocationBuffer( HFSPlusVolumeHeader *header, UInt32 allocationBlock, void* sectorBuffer, UInt32 *sector );


#if !TARGET_OS_MAC

#define BlockMoveData(src, dst, size)		bcopy((src), (dst), (size))

#define ClearMemory(base, size)				bzero((void*)(base), (size_t)(size))

#define NewPtr(size)						valloc((size_t)(size))

#define DisposePtr(p)						free((p))

#define MemError()							(errno)

#define GetDefaultTextEncoding()			(0)

#define MapEncodingToIndex(e)				(e)

#define GetTimeLocal()						UTCToLocal(GetTimeUTC())

static UInt32	GetTimeUTC();

static UInt32	UTCToLocal(UInt32 utcTime);

static UInt32	DivideAndRoundUp(UInt32 numerator, UInt32 denominator);

static OSErr UTF8ToUnicode (const unsigned char* utf8, ItemCount *unicodeChars, UniCharArrayPtr unicodeString);

#endif

// macros

#define 	M_ExitOnError(result)	 if ((result) != noErr) goto Exit;

void SetOffset (void *buffer, UInt16 btNodeSize, SInt16 recOffset, SInt16 vecOffset);
#define SetOffset(buffer,nodesize,offset,record)		(*(SInt16 *) ((Byte *) (buffer) + (nodesize) + (-2 * (record))) = (offset))


//_______________________________________________________________________
//
//	InitHFSVolume
//	
//	This routine writes an initial HFS volume structure onto a volume.
//	It is assumed that the disk has already been formatted and verified.
//	
//	Note: Support for large volumes (ie > 4GB) has been added.
//
//	For information on the HFS volume format see "Data Organization on Volumes"
//	in "Inside Macintosh: Files" (p. 2-52).
//
//	If the alignment parameter is non-zero, it indicates the aligment (in 512
//	byte sectors) that should be used for allocation blocks.  For example, if
//	alignment is 8, then allocation blocks will begin on a 4K boundary
//	relative to the start of the partition.
//_______________________________________________________________________

OSErr
#if TARGET_OS_MAC
	InitHFSVolume (const DriveInfo *driveInfo, ConstStr31Param theVolName, HFSDefaults *defaults, UInt32 alignment)
#else
	InitHFSVolume (const DriveInfo *driveInfo, char* volumeName, HFSDefaults *defaults, UInt32 alignment)
#endif
{
	UInt16					btNodeSize;
	UInt32					alBlkStart;
	UInt32					diskBlocksUsed;
	UInt32					mapNodes;
	UInt32					sectorsPerNode;
	void					*nodeBuffer = NULL;
	HFSMasterDirectoryBlock	*mdb = NULL;
	Str31					theVolName;
	OSErr					err;
	
	theVolName[0] = strlen(volumeName);
	bcopy(volumeName, &theVolName[1], theVolName[0]);
	
	btNodeSize = kBytesPerSector;

	//--- CREATE A MASTER DIRECTORY BLOCK:

	mdb = (HFSMasterDirectoryBlock*) NewPtr(kBytesPerSector);
	nodeBuffer = NewPtr(btNodeSize);
	if (nodeBuffer == NULL || mdb == NULL)
	{
		err = MemError();
		goto  Exit;
	}	
	
	InitMasterDirectoryBlock(defaults, driveInfo->totalSectors, alignment, theVolName, mdb);

	
	//--- ZERO OUT BEGINNING OF DISK:
	
	diskBlocksUsed = (mdb->drAlBlSt + 1) + (mdb->drXTFlSize + mdb->drCTFlSize) / kBytesPerSector;
	err = ClearDisk(driveInfo, 0, diskBlocksUsed, nodeBuffer);
	M_ExitOnError(err);


	//--- WRITE MASTER DIRECTORY BLOCK TO DISK:

	err = WriteToDisk(driveInfo, kMDBStart, kOneSector, mdb);			// write master directory block to disk
	M_ExitOnError(err);
	err = WriteToDisk(driveInfo, driveInfo->totalSectors - 2, kOneSector, mdb);		// place a spare copy at the end of the disk
	M_ExitOnError(err);

		
	//--- WRITE ALLOCATION BITMAP TO DISK:

	//XXX This will fail if the B-trees occupy more than 8*512 = 4K allocation blocks (out of 64K total
	//XXX allocation blocks).
	InitBitmap(mdb->drXTExtRec[0].blockCount + mdb->drCTExtRec[0].blockCount, kOneSector, nodeBuffer);

	err = WriteToDisk(driveInfo, mdb->drVBMSt, kOneSector, nodeBuffer);
	M_ExitOnError(err);
	

	
	sectorsPerNode = btNodeSize/kBytesPerSector;
	alBlkStart = mdb->drAlBlSt;
	
	//--- WRITE FILE EXTENTS B*-TREE TO DISK:

	InitBTreeHeader(mdb->drXTFlSize, mdb->drXTFlSize, btNodeSize, 0, kHFSExtentKeyMaximumLength, 0, &mapNodes, nodeBuffer);				 
	err = WriteToDisk(driveInfo, alBlkStart, sectorsPerNode, nodeBuffer);		// write the header node
	M_ExitOnError(err);
	
	if (mapNodes > 0)		// do we have any map nodes?
	{
		err = WriteMapNodes(driveInfo, (alBlkStart + sectorsPerNode), 1, mapNodes, btNodeSize, nodeBuffer);	// write map nodes to disk
		M_ExitOnError(err);
	}
		
	alBlkStart += (mdb->drXTFlSize/kBytesPerSector);		// update disk starting point
	
	//--- WRITE CATALOG B*-TREE TO DISK:

	InitBTreeHeader(mdb->drCTFlSize, mdb->drCTFlSize, btNodeSize, 2, kHFSCatalogKeyMaximumLength, 0, &mapNodes, nodeBuffer);
	err = WriteToDisk(driveInfo, alBlkStart, kOneSector, nodeBuffer);		// write the header node
	M_ExitOnError(err);

	alBlkStart += sectorsPerNode;		// update disk starting point

	SetupCatalogRecords(theVolName, btNodeSize, nodeBuffer);
	err = WriteToDisk(driveInfo, alBlkStart, kOneSector, nodeBuffer);		// write the root records
	M_ExitOnError(err);
	
	alBlkStart += sectorsPerNode;		// update disk starting point

	if (mapNodes > 0)
		err = WriteMapNodes(driveInfo, alBlkStart, 2, mapNodes, btNodeSize, nodeBuffer);

Exit:
	if (nodeBuffer)
		DisposePtr(nodeBuffer);
		
	if (mdb)
		DisposePtr((Ptr)mdb);
	

	return err;
}


//_______________________________________________________________________
//
//	InitializeHFSPlusVolume
//	
//_______________________________________________________________________

OSErr
#if TARGET_OS_MAC
	InitializeHFSPlusVolume (const DriveInfo *driveInfo, ConstStr31Param volumeName, HFSPlusDefaults *defaults)
#else
	InitializeHFSPlusVolume (const DriveInfo *driveInfo, char* volumeName, HFSPlusDefaults *defaults)
#endif
{
	OSErr			err;
	UInt16			btNodeSize;
	UInt32			sector;
	UInt32			sectorsPerBlock;
	UInt32			volumeBlocksUsed;
	UInt32			mapNodes;
	UInt32			sectorsPerNode;
	void			*nodeBuffer = NULL;
	HFSPlusVolumeHeader	*header = NULL;
	HFSUniStr255		hfsVolumeName;
	UInt32			temp;

#if TARGET_OS_MAC
	(void) InitUnicodeConverter( false );	// make sure converter is initialized!
	err = ConvertHFSNameToUnicode(	volumeName, GetDefaultTextEncoding(), &hfsVolumeName );
	if ( err == kTECPartialCharErr || err == kTECUnmappableElementErr )
		err = bdNamErr;		// report back that this name is bad
	M_ExitOnError(err);
#else
	err = UTF8ToUnicode(volumeName, &temp, (UniCharArrayPtr) &hfsVolumeName.unicode);
	hfsVolumeName.length = temp;
	M_ExitOnError(err);
#endif
	

	//--- Create an HFS Plus header:

	header = (HFSPlusVolumeHeader*) NewPtr (kBytesPerSector);
	if (header == NULL)
	{
		err = MemError();
		goto  Exit;
	}	

	InitVolumeHeader(defaults, driveInfo->totalSectors, header);

	sectorsPerBlock = header->blockSize / kBytesPerSector;

	//--- ZERO OUT BEGINNING OF DISK:

	// (-1) because volumeBlocksUsed refers to volume blocks used at the begining of the disk,
	//	so we take out the alternate volume header, and block after altVH if blocksize is 512.
	volumeBlocksUsed = header->totalBlocks - header->freeBlocks - 1;
	if ( header->blockSize == 512 )
		volumeBlocksUsed--;
	err = ClearDisk(driveInfo, 0, volumeBlocksUsed * sectorsPerBlock, nodeBuffer);
	M_ExitOnError(err);

	//--- Allocate a buffer for the rest of our I/O:

	temp = Largest( defaults->catalogNodeSize, defaults->extentsNodeSize, header->blockSize, DivideAndRoundUp(volumeBlocksUsed,8) );

	if ( (temp & 0x01FF) != 0 )							// is size a mutiple of 512?
		temp = (temp + kBytesPerSector) & 0xFFFFFE00;	// no, so round up to nearest sector	
	
	nodeBuffer = NewPtr (temp);
	if (nodeBuffer == NULL)
	{
		err = MemError();
		goto  Exit;
	}	


	//--- WRITE VOLUME HEADER TO DISK:

	err = WriteToDisk(driveInfo, 2, kOneSector, header);							// write the main volume header at block zero
	M_ExitOnError(err);

	err = WriteToDisk(driveInfo, (header->totalBlocks * (header->blockSize/kBytesPerSector)) - 2, kOneSector, header);	// place a spare copy in last sector of HFS Plus partition.
	M_ExitOnError(err);

		
	//--- WRITE ALLOCATION BITMAP BITS TO DISK:

	temp = ((volumeBlocksUsed / 8) + kBytesPerSector-1) / kBytesPerSector;	// how many blocks needed
	InitBitmap(volumeBlocksUsed, temp, nodeBuffer);

	sector = header->allocationFile.extents[0].startBlock * sectorsPerBlock;	// usally starts at allocation block 1
	err = WriteToDisk(driveInfo, sector, temp, nodeBuffer);
	M_ExitOnError(err);

	//--- Write alternate Volume Header bitmap bit to allocations file
	//--- 2nd to last sector on HFS+ volume
	ClearMemory(nodeBuffer, kBytesPerSector);
	MarkBitInAllocationBuffer( header, header->totalBlocks - 1, nodeBuffer, &sector );

	if ( header->blockSize == 512  )
	{
		UInt32	sector2;
		MarkBitInAllocationBuffer( header, header->totalBlocks - 2, nodeBuffer, &sector2 );
		
		//	For the case when altVH and last block are on different bitmap sectors.
		if ( sector2 != sector )
		{
			ClearMemory(nodeBuffer, kBytesPerSector);
			MarkBitInAllocationBuffer( header, header->totalBlocks - 1, nodeBuffer, &sector );
			err = WriteToDisk(driveInfo, sector, kOneSector, nodeBuffer);
			ReturnIfError( err );
			ClearMemory(nodeBuffer, kBytesPerSector);
			MarkBitInAllocationBuffer( header, header->totalBlocks - 2, nodeBuffer, &sector );
		}
	}
	err = WriteToDisk(driveInfo, sector, kOneSector, nodeBuffer);
	ReturnIfError( err );

	//--- WRITE FILE EXTENTS B-TREE TO DISK:

	btNodeSize = defaults->extentsNodeSize;
	sectorsPerNode = btNodeSize/kBytesPerSector;

	InitBTreeHeader(header->extentsFile.logicalSize.lo, header->extentsFile.clumpSize, btNodeSize, 0, kHFSPlusExtentKeyMaximumLength, kBTBigKeysMask, &mapNodes, nodeBuffer);				 

	sector = header->extentsFile.extents[0].startBlock * sectorsPerBlock;
	err = WriteToDisk(driveInfo, sector, sectorsPerNode, nodeBuffer);		// write the header node
	M_ExitOnError(err);
	
	if (mapNodes > 0)		// do we have any map nodes?
	{
		err = WriteMapNodes(driveInfo, (sector + sectorsPerNode), 1, mapNodes, btNodeSize, nodeBuffer);	// write map nodes to disk
		M_ExitOnError(err);
	}

	
	//--- WRITE CATALOG B-TREE TO DISK:
	
	btNodeSize = defaults->catalogNodeSize;
	sectorsPerNode = btNodeSize/kBytesPerSector;

	InitBTreeHeader (header->catalogFile.logicalSize.lo, header->catalogFile.clumpSize, btNodeSize, 2, kHFSPlusCatalogKeyMaximumLength,
					 kBTVariableIndexKeysMask + kBTBigKeysMask, &mapNodes, nodeBuffer);

	sector = header->catalogFile.extents[0].startBlock * sectorsPerBlock;
	err = WriteToDisk(driveInfo, sector, sectorsPerNode, nodeBuffer);		// write the header node
	M_ExitOnError(err);

	sector += sectorsPerNode;		// update disk starting point

	InitRootFolder(&hfsVolumeName, btNodeSize, nodeBuffer);
	err = WriteToDisk(driveInfo, sector, sectorsPerNode, nodeBuffer);		// write the root records
	M_ExitOnError(err);

	if (mapNodes > 0)
		err = WriteMapNodes(driveInfo, (sector + sectorsPerNode), 2, mapNodes, btNodeSize, nodeBuffer);

Exit:
	if ( nodeBuffer != NULL )
		DisposePtr(nodeBuffer);

	if ( header != NULL )
		DisposePtr((Ptr) header);

	return err;

} // end InitializeHFSPlusVolume


//_______________________________________________________________________
//
//	InitMasterDirectoryBlock
//	
//	This routine initializes a Master Directory Block (MDB) record.
//_______________________________________________________________________

static void
InitMasterDirectoryBlock(HFSDefaults *defaults, UInt32 driveBlocks, UInt32 alignment, ConstStr31Param volName, HFSMasterDirectoryBlock *mdb)
{
	UInt32	alBlkSize;
	UInt16	numAlBlks;
	UInt16	btAlBlks;
	UInt32	btClumpSize;
	UInt32	timeStamp;
	UInt16	bitmapBlocks;


	ClearMemory(mdb, kBytesPerSector);			// start with a clean slate
	
	alBlkSize = defaults->abSize;

	// calculate the number of sectors needed for bitmap (rounded up)
	bitmapBlocks = ((driveBlocks / (alBlkSize >> kLog2SectorSize)) + kBitsPerSector-1) / kBitsPerSector;

	mdb->drAlBlSt = kVolBitMapStart + bitmapBlocks;		// in disk blocks

	//	If requested, round up allocation block start to a multiple of "alignment" blocks
	if (alignment != 0)
		mdb->drAlBlSt = ((mdb->drAlBlSt + alignment - 1) / alignment) * alignment;
	
	//  Now find out how many whole allocation blocks remain...
	numAlBlks = (driveBlocks - mdb->drAlBlSt - kTailBlocks) / (alBlkSize >> kLog2SectorSize);
	
	btClumpSize = defaults->btClpSize;
	btAlBlks = btClumpSize / alBlkSize;

	timeStamp = GetTimeLocal();				// get current date & time
	
	mdb->drSigWord	= *(UInt16*) defaults->sigWord;
	mdb->drCrDate	= timeStamp;
	mdb->drLsMod	= timeStamp;
	mdb->drAtrb		= 0x0100;
  //mdb->drNmFls	= 0;
	mdb->drVBMSt	= kVolBitMapStart;
  //mdb->drAllocPtr	= 0;

	mdb->drNmAlBlks	= numAlBlks;
	mdb->drAlBlkSiz	= alBlkSize;
	mdb->drClpSiz	= defaults->clpSize;
	mdb->drNxtCNID	= defaults->nxFreeFN;
	mdb->drFreeBks	= numAlBlks - (2 * btAlBlks);
	
	BlockMoveData(volName, mdb->drVN, volName[0]+1);
	
	mdb->drWrCnt	= kWriteSeqNum;
  //mdb->drNmRtDirs	= 0;
  //mdb->drFilCnt	= 0;
  //mdb->drDirCnt	= 0;

	mdb->drXTClpSiz	= btClumpSize;
	mdb->drCTClpSiz = btClumpSize;

//	mdb->drVCSize	= 0;
//	mdb->drVBMCSize	= 0;
	
	mdb->drXTFlSize					= btClumpSize;
  //mdb->drXTExtRec[0].startBlock	= 0;
	mdb->drXTExtRec[0].blockCount	= btAlBlks;
	mdb->drCTFlSize					= btClumpSize;
	mdb->drCTExtRec[0].startBlock	= btAlBlks;
	mdb->drCTExtRec[0].blockCount	= btAlBlks;
}

//_______________________________________________________________________
//
//	InitVolumeHeader
//	
//	This routine initializes an HFS Plus volume header.
//_______________________________________________________________________

static void
InitVolumeHeader (HFSPlusDefaults *defaults, UInt32 sectors, HFSPlusVolumeHeader *header)
{
	UInt32	blockSize;
	UInt32	blockCount;
	UInt32	blocksUsed;
	UInt32	bitmapBlocks;
	UInt32	utcTime;
	UInt32	index;
	UInt16	burnedBlocksBeforeVH	= 0;
	UInt16	burnedBlocksAfterAltVH	= 0;
	
	ClearMemory(header, kBytesPerSector);			// start with a clean slate

	blockSize = defaults->blockSize;
	blockCount = sectors / (blockSize >> kLog2SectorSize);

	// HFSPlusVolumeHeader is located at sector 2, so we may need to invalidate blocks before HFSPlusVolumeHeader
	if ( blockSize == 512 )
	{
		burnedBlocksBeforeVH	= 2;				//	2 before VH
		burnedBlocksAfterAltVH	= 1;				//	1 after altVH
	}
	else if ( blockSize == 1024 )
	{
		burnedBlocksBeforeVH = 1;
	}

	bitmapBlocks = defaults->allocationClumpSize / blockSize;

	blocksUsed = 2 + burnedBlocksBeforeVH + burnedBlocksAfterAltVH + bitmapBlocks;	// (2) for the Alternate VH, and VH

	header->signature			= kHFSPlusSigWord;			// this is the final sigword
	header->version				= kHFSPlusVersion;			// current version of HFS Plus
	header->attributes			= kHFSVolumeUnmountedMask;		// set unmount flag
	header->lastMountedVersion	= kHFSPlusMountVersion;		// Tell others which OS created/mounted volume

	utcTime = GetTimeUTC();
	header->createDate	= UTCToLocal(utcTime);		// NOTE: create date is in local time, not GMT!
	header->modifyDate	= utcTime;
  //header->backupDate	= 0;
	header->checkedDate	= utcTime;

  //header->fileCount		= 0;
  //header->folderCount		= 0;

	header->blockSize		= blockSize;
	header->totalBlocks		= blockCount;
	header->freeBlocks		= blockCount;	// will be adjusted at the end

	header->rsrcClumpSize	= defaults->rsrcClumpSize;
	header->dataClumpSize	= defaults->dataClumpSize;
	header->nextCatalogID	= defaults->nextFreeFileID;
  //header->writeCount		= 0;

	index = GetDefaultTextEncoding() & 0x7F;
	index = MapEncodingToIndex(index);
	if ( index < 32 )
		header->encodingsBitmap.lo |= (1 << index);
	else
		header->encodingsBitmap.hi |= (1 << (index - 32));

	header->allocationFile.clumpSize				= defaults->allocationClumpSize;
	header->allocationFile.logicalSize.lo			= defaults->allocationClumpSize;
	header->allocationFile.totalBlocks				= bitmapBlocks;
  	header->allocationFile.extents[0].startBlock	= 1 + burnedBlocksBeforeVH;		// starts at block after HFSPlusVolumeHeader
	header->allocationFile.extents[0].blockCount	= bitmapBlocks;
	
	header->extentsFile.clumpSize				= defaults->extentsClumpSize;
	header->extentsFile.logicalSize.lo			= defaults->extentsClumpSize;
	header->extentsFile.totalBlocks				= defaults->extentsClumpSize / blockSize;
	header->extentsFile.extents[0].startBlock	= header->allocationFile.extents[0].startBlock +
												  header->allocationFile.extents[0].blockCount;
	header->extentsFile.extents[0].blockCount	= header->extentsFile.totalBlocks;
	blocksUsed								   += header->extentsFile.totalBlocks;

	header->catalogFile.clumpSize				= defaults->catalogClumpSize;
	header->catalogFile.logicalSize.lo			= defaults->catalogClumpSize;
	header->catalogFile.totalBlocks				= defaults->catalogClumpSize / blockSize;
	header->catalogFile.extents[0].startBlock	= header->extentsFile.extents[0].startBlock +
												  header->extentsFile.extents[0].blockCount;
	header->catalogFile.extents[0].blockCount	= header->catalogFile.totalBlocks;
	blocksUsed								   += header->catalogFile.totalBlocks;

	header->freeBlocks -= blocksUsed;
	//	Add some room for the catalog file to grow (header->catalogFile.clumpSize / header->blockSize)
	header->nextAllocation	= blocksUsed - 1 - burnedBlocksAfterAltVH + (header->catalogFile.clumpSize / header->blockSize);	//XXX this will have to change when we support sparing!, (-1) subtract out the AltVH
}


//_______________________________________________________________________
//
//	InitBitmap
//	
//	This routine initializes the Allocation Bitmap. Allocation blocks
//	that are in use have their corresponding bit set.
//
//	It assumes that initially there are no gaps between allocated blocks.
//
//  It also assumes the buffer is big enough to hold all the bits (ie its
//	at least (alBlksUsed/8) bytes in size rounded up to nearest sector).
//_______________________________________________________________________

static void
InitBitmap (UInt16 alBlksUsed, UInt32 sectors, Byte *buffer)
{
	SInt16	i;
	UInt16	bytes, bits;
	Byte	*bmPtr = buffer;
	
	ClearMemory(buffer, kBytesPerSector * sectors);

	bytes = alBlksUsed >> 3;		// number of $FF bytes	
	bits  = alBlksUsed & 0x0007;	// number of bits left in byte
	
	for (i = 0; i < bytes; i++)
		*bmPtr++ = 0xFF;

	if (bits)
		*bmPtr = (0xFF00 >> bits) & 0xFF;
}


//_______________________________________________________________________
//
//	InitBTreeHeader
//	
//	This routine initializes a B-Tree header.
//
//	Note: Since large volumes will have bigger b-trees they need to
//	have map nodes setup.
//_______________________________________________________________________

static void
InitBTreeHeader (UInt32 fileSize, UInt32 clumpSize, UInt16 nodeSize, UInt16 recordCount, UInt16 keySize,
				 UInt32 attributes, UInt32 *mapNodes, void *buffer)
{
	UInt32		 nodeCount;
	UInt32		 usedNodes;
	UInt32		 nodeBitsInHeader;
	HeaderPtr	 bth;
	UInt32		*bitMapPtr;
	SInt16		*offsetPtr;


	ClearMemory(buffer, nodeSize);			// start out with clean node
	
	nodeCount = fileSize / nodeSize;
	nodeBitsInHeader = 8 * (nodeSize - sizeof(HeaderRec) - kBTreeHeaderUserBytes - 4*sizeof(SInt16));
	
	usedNodes =	1;							// header takes up one node
	*mapNodes = 0;							// number of map nodes initially (0)


	// FILL IN THE NODE DESCRIPTOR:
	bth = (HeaderPtr) buffer;				// point to header & node descriptor

	bth->node.type = kHeaderNode;			// this node contains the B-tree header
	bth->node.numRecords = 3;				// there are 3 records (header, map, and user)

	if (nodeCount > nodeBitsInHeader)		// do we need additional map nodes?
	{
		UInt32	nodeBitsInMapNode;
		
		nodeBitsInMapNode = 8 * (nodeSize - sizeof(BTNodeDescriptor) - 2*sizeof(SInt16) - 2);

		if (recordCount > 0)				// catalog B-tree?
			bth->node.fLink = 2;			// link points to initial map node
											//XXX Assumes all records will fit in one node.  It would be better
											//XXX to put the map node(s) first, then the records.
		else
			bth->node.fLink = 1;			// link points to initial map node

		*mapNodes = (nodeCount - nodeBitsInHeader + (nodeBitsInMapNode - 1)) / nodeBitsInMapNode;
		usedNodes += *mapNodes;
	}

	
	// FILL IN THE HEADER RECORD:
	if (recordCount > 0)
	{
		++usedNodes;								// one more node will be used

		bth->treeDepth = 1;							// tree depth is one level (leaf)
		bth->rootNode  = 1;							// root node is also leaf
		bth->firstLeafNode = 1;						// first leaf node
		bth->lastLeafNode = 1;						// last leaf node
	}

	bth->attributes	  = attributes;					// flags for 16-bit key lengths, and variable sized index keys
	bth->leafRecords  = recordCount;				// total number of data records
	bth->nodeSize	  = nodeSize;					// size of a node
	bth->maxKeyLength = keySize;					// maximum length of a key
	bth->totalNodes	  = nodeCount;					// total number of nodes
	bth->freeNodes	  = nodeCount - usedNodes;		// number of free nodes
	bth->clumpSize	  = clumpSize;					//
  //bth->btreeType	  = 0;							// 0 = meta data B-tree


	// FILL IN THE MAP RECORD:
	bitMapPtr = (UInt32*) ((Byte*) buffer + sizeof(HeaderRec) + kBTreeHeaderUserBytes);	// point to bitmap

	// MARK NODES THAT ARE IN USE:
	// Note - worst case (32MB alloc blk) will have only 18 nodes in use.
	*bitMapPtr = ~((UInt32) 0xFFFFFFFF >> usedNodes);
	

	// PLACE RECORD OFFSETS AT THE END OF THE NODE:
	offsetPtr = (SInt16*) ((Byte*) buffer + nodeSize - 4*sizeof(SInt16));

	*offsetPtr++ = sizeof(HeaderRec) + kBTreeHeaderUserBytes + nodeBitsInHeader/8;	// offset to free space
	*offsetPtr++ = sizeof(HeaderRec) + kBTreeHeaderUserBytes;						// offset to allocation map
	*offsetPtr++ = sizeof(HeaderRec);												// offset to user space
	*offsetPtr   = sizeof(BTNodeDescriptor);										// offset to BTH
}


//_______________________________________________________________________
//
//	SetupCatalogRecords
//	
//	This routine initializes the initial catalog records used by the
//	Catalog B-tree.  (HFS volumes only)
//_______________________________________________________________________

static void
SetupCatalogRecords (ConstStr31Param volName, UInt16 btNodeSize, void * buffer)
{
	BTNodeDescriptor	*nd = (BTNodeDescriptor *) buffer;
	HFSCatalogKey		*cdk;
	HFSCatalogFolder	*cdr;
	HFSCatalogThread	*ctr;
	SInt16				offset;
	UInt32				timeStamp;

	ClearMemory(buffer, btNodeSize);
	
	timeStamp = GetTimeLocal();						// get current date & time

	// Set up the first leaf node in the tree:

	// First set up the node descriptor (most of it zeros)...

	nd->type = kLeafNode;			// leaf node type
	nd->numRecords = 2;				// two records in node
	nd->height = 1;					// set node height to one for 2nd node

	// The first record is the root directory CNode, <1>'Root':

	// point to it and stuff an offset to it

	offset = sizeof(BTNodeDescriptor);
	SetOffset(buffer, btNodeSize, offset, 1);				// set offset to first key record (1st)

	// stuff the key
	cdk = (HFSCatalogKey*) ((Byte*) buffer + offset);
	cdk->keyLength = 1 + 4 + ((volName[0] + 2) & 0xFE);		// key length padded to word
  //cdk->reserved = 0;										// clear filler byte 
	cdk->parentID = kHFSRootParentID;						// parent ID of root (1)
	BlockMoveData(volName, cdk->nodeName, volName[0]+1);

	offset += cdk->keyLength + 1;			// offset to directory record

	cdr = (HFSCatalogFolder*) ((Byte*) buffer + offset);
	cdr->recordType = kHFSFolderRecord;	// data record type (directory CNode)
  //cdr->flags = 0;
  //cdr->valence = 0;						// empty
	cdr->folderID = kHFSRootFolderID;		// root node's directory ID
	cdr->createDate = timeStamp;			// creation date time
	cdr->modifyDate = timeStamp;			// date and time last modified
	
	offset += sizeof(HFSCatalogFolder);
	
	SetOffset(buffer, btNodeSize, offset, 2);	// set offset to thread record (2nd)

	// stuff the thread key
	cdk = (HFSCatalogKey *) ((Byte*) buffer + offset);
	cdk->keyLength = kHFSCatalogKeyMinimumLength;	// key length
  //cdk->reserved = 0;						// clear filler byte 
	cdk->parentID = kHFSRootFolderID;		// DirID in key
  //cdk->name[0] = 0;						// CName string (null)
	
	offset += cdk->keyLength + 2;			// offset to thread record (adds pad byte too)

	// stuff the thread record
	ctr = (HFSCatalogThread*) ((Byte*) buffer + offset);
	ctr->recordType = kHFSFolderThreadRecord;					// Record type is thread 
	ctr->parentID = kHFSRootParentID;							// Root's parent ID
	BlockMoveData(volName, ctr->nodeName, volName[0]+1);		// Copy in the volume name

	offset += sizeof(HFSCatalogThread);

	SetOffset(buffer, btNodeSize, offset, 3);				// set the offset to the empty record (3rd)
}


//_______________________________________________________________________
//
//	InitRootFolder
//	
//	This routine initializes the initial catalog records used by the
//	Catalog B-tree.
//_______________________________________________________________________

static void
InitRootFolder (ConstHFSUniStr255Param volName, UInt16 btNodeSize, void *nodeBuffer)
{
	BTNodeDescriptor		*nd = (BTNodeDescriptor *) nodeBuffer;
	HFSPlusCatalogKey			*key;
	HFSPlusCatalogFolder		*folder;
	HFSPlusCatalogThread		*thread;
	SInt16					offset;
	UInt32					timeStamp;

	ClearMemory(nodeBuffer, btNodeSize);

	timeStamp = GetTimeUTC();							// get current date & time

	// Set up the first leaf node in the tree:

	// First set up the node descriptor (most of it zeros)...

	nd->type = kLeafNode;			// leaf node type
	nd->numRecords = 2;				// two records in node
	nd->height = 1;					// set node height to one for 2nd node

	// The first record is the root directory CNode, <1>'Root':

	// point to it and stuff an offset to it

	offset = sizeof(BTNodeDescriptor);
	SetOffset(nodeBuffer, btNodeSize, offset, 1);				// set offset to first key record (1st)

	//--- stuff the key

	key = (HFSPlusCatalogKey*) ((Byte*) nodeBuffer + offset);
	key->keyLength = kHFSPlusCatalogKeyMinimumLength + sizeof(UniChar) * (volName->length);
	key->parentID = kHFSRootParentID;			// parent ID of root (1)
	BlockMoveData(volName, &key->nodeName, key->keyLength - kHFSPlusCatalogKeyMinimumLength + sizeof(UInt16));

	offset += key->keyLength + 2;				// offset to folder record

	//--- stuff the folder record
	folder = (HFSPlusCatalogFolder*) ((Byte*) nodeBuffer + offset);

	folder->recordType		= kHFSPlusFolderRecord;	// data record type (directory CNode)
  //folder->flags			= 0;					// no flag bits
	folder->folderID		= kHFSRootFolderID;		// root node's directory ID
  //folder->valence			= 0;					// empty
	folder->createDate		= timeStamp;			// creation date time
	folder->contentModDate	= timeStamp;			// date and time last modified
  //folder->backupDate		= 0;					// date and time last backed up
	folder->textEncoding	= GetDefaultTextEncoding();
	
	offset += sizeof(HFSPlusCatalogFolder);
	SetOffset(nodeBuffer, btNodeSize, offset, 2);	// set offset to thread record (2nd)


	//--- stuff the thread key
	key = (HFSPlusCatalogKey *) ((Byte*) nodeBuffer + offset);

	key->keyLength = kHFSPlusCatalogKeyMinimumLength;	// key length
	key->parentID = kHFSRootFolderID;		// DirID in key
  //key->nameLength = 0;					// CName string (empty)
	
	offset += key->keyLength + 2;			// offset to thread record

	// stuff the thread record
	thread = (HFSPlusCatalogThread*) ((Byte*) nodeBuffer + offset);

	thread->recordType = kHFSPlusFolderThreadRecord;
  //thread->reserved = 0;
	thread->parentID = kHFSRootParentID;	// Root's parent ID
	BlockMoveData(volName, &thread->nodeName, sizeof(UniChar) * (volName->length + 1));

	offset += (sizeof(HFSPlusCatalogThread) - (sizeof(thread->nodeName.unicode) - (sizeof(UniChar) * volName->length)));
	SetOffset(nodeBuffer, btNodeSize, offset, 3);			// set the offset to the empty record (3rd)
}


//_______________________________________________________________________
//
//	WriteMapNodes
//	
//	This routine initializes a B-tree map node and writes it out to disk.
//_______________________________________________________________________

static OSErr
WriteMapNodes (const DriveInfo *driveInfo, UInt32 diskStart, UInt32 firstMapNode, UInt32 mapNodes, UInt16 btNodeSize, void *buffer)
{
	UInt32	sectorsPerNode;
	UInt32	mapRecordBytes;
	UInt16	i;
	OSErr	err = noErr;
	BTNodeDescriptor *nd = (BTNodeDescriptor *) buffer;

	ClearMemory(buffer, btNodeSize);				// start with clean node

	nd->type = kMapNode;							// map node type
	nd->numRecords = 1;								// one record in node
	
	mapRecordBytes = btNodeSize - sizeof(BTNodeDescriptor) - 2*sizeof(SInt16) - 2;		// must belong word aligned (hence the extra -2)

	SetOffset(buffer, btNodeSize, sizeof(BTNodeDescriptor), 1);							// set offset to map record (1st)
	SetOffset(buffer, btNodeSize, sizeof(BTNodeDescriptor) + mapRecordBytes, 2);		// set offset to free space (2nd)
	
	sectorsPerNode = btNodeSize/kBytesPerSector;
	
	// Note - worst case (32MB alloc blk) will have
	// only 18 map nodes. So don't bother optimizing
	// this section to do multiblock writes!
	
	for (i = 0; i < mapNodes; i++)
	{
		if ((i + 1) < mapNodes)
			nd->fLink = ++firstMapNode;			// link points to next map node
		else
			nd->fLink = 0;						// this is the last map node

		err = WriteToDisk(driveInfo, diskStart, sectorsPerNode, buffer);		// write this map node
		if (err != noErr)
			break;
			
		diskStart += sectorsPerNode;
	}

	return err;
}

//_______________________________________________________________________
//
//	ClearDisk
//	
//	This routine will clear out consecutive sectors on the disk.
//
//	Since disk I/O is expensive, we try and get the biggest buffer
//	possible from temp memory to minimize the number of I/O transactions.
//_______________________________________________________________________

static OSErr
ClearDisk (const DriveInfo *driveInfo, UInt32 startingSector, UInt32 numberOfSectors, void *sectorData)
{
	UInt32	bufferSizeInSectors;
	void *tempBuffer = NULL;
	OSErr	error = noErr;
	
	// if we have more than 1 sector to clear, allocate a buffer in temp memoryÉ
	
	if ( numberOfSectors > 0x200 )		//	pin at 256K, VM guarantees 256K is holdable
		bufferSizeInSectors = 0x200;
	else
		bufferSizeInSectors = numberOfSectors;

	tempBuffer = NewPtr(bufferSizeInSectors << kLog2SectorSize);
	if (tempBuffer == 0)
		exit(12);

	ClearMemory(tempBuffer, bufferSizeInSectors << kLog2SectorSize);	// clear out our buffer

	while (numberOfSectors > 0)
	{
		error = WriteToDisk(driveInfo, startingSector, bufferSizeInSectors, tempBuffer);
		if (error)
			break;
	
		startingSector += bufferSizeInSectors;	
		numberOfSectors -= bufferSizeInSectors;
		
		if (numberOfSectors < bufferSizeInSectors)		// is remainder less than size of buffer?
			bufferSizeInSectors = numberOfSectors;		// adjust it for last pass
	}
	
	if (tempBuffer)
		DisposePtr(tempBuffer);

	return error;
}


//_______________________________________________________________________
//
//	WriteToDisk
//	
//	This routine will write blocks to the disk.
//
//	Since we need to pass the disk offset in bytes in just 32 bits,
//	we better check for overflow!  If it won't fit in 32 bits we
//	use an extended IO parameter block which has a 64 bit offset.
//_______________________________________________________________________

static OSErr
WriteToDisk (const DriveInfo *driveInfo, UInt32 startingSector, UInt32 numberOfSectors, void *sectorData)
{
#if TARGET_OS_MAC
	XIOParam	pb;
	UInt32		sectorOffset;
	OSErr 		error;
	
	pb.ioCompletion	= NULL;
	pb.ioVRefNum	= driveInfo->driveNumber;
	pb.ioRefNum		= driveInfo->driverRefNum;
	pb.ioBuffer		= sectorData;
	pb.ioActCount	= 0;
	pb.ioReqCount	= numberOfSectors << kLog2SectorSize;	// numberOfSectors * kBytesPerSector

	sectorOffset = driveInfo->sectorOffset + startingSector;

	// check to see if ioPosOffset will overflowÉ
	
	if (sectorOffset > kMaximumBlocksIn4GB)
	{
		// use 64 bit valueÉ
		// since we know we're multiplying by 512 we can just use
		// shifts instead of calling a 32x32->64 multiply routine.
		
		pb.ioWPosOffset.lo = sectorOffset << kLog2SectorSize;				
		pb.ioWPosOffset.hi = sectorOffset >> (32 - kLog2SectorSize);
		pb.ioPosMode	   = fsFromStart | (1<<kWidePosOffsetBit);
	}
	else
	{
		// use 32 bit valueÉ
		(*(IOParam*) &pb).ioPosOffset = sectorOffset << kLog2SectorSize;
		(*(IOParam*) &pb).ioPosMode	  = fsFromStart;
	}
	
	error = PBWriteSync((ParmBlkPtr) &pb);
	
	return error;

#else

	int n, size;

	if (lseek(driveInfo->fd, (off_t)startingSector * driveInfo->sectorSize, SEEK_SET) < 0) {
		printf("seek error: %ld\n", startingSector);
		perror("wtfs");
		exit(35);
	}
	size = numberOfSectors * kBytesPerSector;
	n = write(driveInfo->fd, sectorData, size);
	if (n != size) {
		printf("write error: %ld\n", startingSector);
		perror("wtfs");
		exit(36);
	}

	return noErr;
#endif
}


//_______________________________________________________________________
//
//	Largest
//	
//	Return the largest of four integers
///_______________________________________________________________________

static UInt32 Largest ( UInt32 a, UInt32 b, UInt32 c, UInt32 d )
{
	//	a := max(a,b)
	if (a < b)
		a = b;
	//	c := max(c,d)
	if (c < d)
		c = d;
	
	//	return max(a,c)
	if (a > c)
		return a;
	else
		return c;
}

//_______________________________________________________________________
//
//	MarkBitInAllocationBuffer
//	
//	Given a buffer and allocation block, will mark off the corresponding
//	bitmap bit, and return the sector number the block belongs in.
///_______________________________________________________________________
static void MarkBitInAllocationBuffer( HFSPlusVolumeHeader *header, UInt32 allocationBlock, void* sectorBuffer, UInt32 *sector )
{

	UInt8	*byteP;
	UInt8	mask;
	UInt32	sectorsPerBlock;
	UInt16	bitInSector					= allocationBlock % kBitsPerSector;
	UInt16	bitPosition					= allocationBlock % 8;
	
	sectorsPerBlock = header->blockSize / kBytesPerSector;

	*sector = ( header->allocationFile.extents[0].startBlock * sectorsPerBlock ) + ( allocationBlock / kBitsPerSector );
	
	byteP		= (UInt8 *) sectorBuffer + (bitInSector >> 3);
	mask		= ( 0x80 >> bitPosition );
	*byteP		|= mask;
}


/*
 * GetTimeUTC - get Mac OS GMT time
 */
static UInt32 GetTimeUTC()
{
	time_t utime;

	time(&utime);
	
	return (utime + MAC_GMT_FACTOR);
}


/*
 * UTCToLocal - convert from Mac OS GMT time to Mac OS local time
 */
static UInt32 UTCToLocal(UInt32 utcTime)
{
	UInt32 localTime = utcTime;
	struct timezone timeZone;
	struct timeval	timeVal; /* gettimeofday() crashes if the first argument is NULL */
	
	if (localTime != 0) {

	/* HFS volumes need timezone info to convert local to GMT */
	(void) gettimeofday( &timeVal, &timeZone );


		localTime -= (timeZone.tz_minuteswest * 60);
		if (timeZone.tz_dsttime)
			localTime += 3600;
	}
    return (localTime);
}


/*
;_______________________________________________________________________
;
; Routine:	DivideAndRoundUp
;
; Function:	Divide numerator by denominator, rounding up the result if there
;			was a remainder.  This is frequently used for computing the number
;			of whole and/or partial blocks used by some count of bytes.
;_______________________________________________________________________
*/
static UInt32 DivideAndRoundUp(UInt32 numerator, UInt32 denominator)
{
	UInt32	quotient;
	
	quotient = numerator / denominator;
	if (quotient * denominator != numerator)
		quotient++;
	
	return quotient;
}


/*XXX need real UTF8 conversion here! */

static OSErr UTF8ToUnicode (const unsigned char* utf8, ItemCount *unicodeChars, UniCharArrayPtr unicodeString)
{
	const UInt8		*src;
	UniChar			*dst;
	UInt16			n;
	UInt8			c;


	src = utf8;
	dst = unicodeString;
	n = 0;

	while ((c = *(src++)) != 0) {
		++n;

		if ( (SInt8) c >= 0 ) {			// make sure its seven bit ascii
			*(dst++) = (UniChar) c;		//  pad high byte with zero
		}
		else {	/* its a hi bit character */
			return bdNamErr;
		}
	}
	
	*unicodeChars = n;
	
	return noErr;
}
