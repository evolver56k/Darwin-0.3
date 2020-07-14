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

/*
	File:		VolumeCheck.c

	Contains:	Consistency checking code for HFS and HFS Plus volumes.
				This code was based off MtCheck (in TFSVol.a).

	Version:	HFS Plus 1.0

	Copyright:	© 1995-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(NG)	Nitin Ganatra
		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	  <Rhap>	  6/3/98	djb		Switch RenameCatalogNode to MoveRenameCatalogNode.
	  <Rhap>	  4/2/98	djb		FixOrphanFileThreads only works with MacOS.
	  <Rhap>	03/31/98	djb		Sync up with final HFSVolumes.h header file.
	  <Rhap>	03/26/98	djb		Minor cleanup.
	  <Rhap>	03/20/98	djb		Add leaf-order checking back in.  Set USE_BTREE_SCANNER to true
	  								to enable disk order checking.  Add VOLUME_CHECK_LOG messages.
	  								Use PascalBinaryCompare to compare volume names.

	  <CS16>	11/26/97	djb		Radar #2003656 (and others) - remove B-tree header fixing.
		<15>	10/31/97	DSH		Modify so DFA can call without actually writing to the disk,
									added consistencyStatus parameter to MountCheck and fixed bug in
									freeBlocks calculation in CheckBitmapAndHeader().
	  <CS14>	10/23/97	msd		Bug 1685113. Fix incorrect volume header creation dates.
	  <CS13>	 9/18/97	NG		Don't call Gestalt(osAttr) to check if Temp Mem is available.
									Instead, use GetExpandMemProcessMgrExists
	  <CS12>	  9/7/97	djb		Turn off DebugStr at MountCheck's ErrorExit.
	  <CS11>	  9/5/97	msd		Added CheckBitmapAndHeader to account for space used by the
									volume header, alternate volume header, and the allocation file
									(bitmap) itself.
	  <CS10>	  9/4/97	msd		Added CheckAttributes to account for space used by large
									attribute values (the ones that occupy extents).
	   <CS9>	 8/29/97	djb		Fixed bug in CheckExtentsOverflow (radar #1675472 and #1675417).
	   <CS8>	 8/19/97	msd		In CheckCatalog, don't increment the counts associated with the
									root directory twice; only do it in the bulk scan. When counting
									folders, don't count the root directory itself.
	   <CS7>	 8/11/97	djb		Changed vcb check of largestCNID from "<" to "<=" (radar
									#1670614). Use thread records when establishing the largest
									CNID.
	   <CS6>	 7/30/97	DSH		Casting for SC, needed for DFA compiles
	   <CS5>	 7/28/97	msd		Use fast new B-Tree scanner. CheckCatalog was using a variable
									before it was defined; the valance computations should have been
									wrong.
	   <CS4>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS3>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS2>	 6/12/97	djb		Get in sync with HFS Plus format changes.
	   <CS1>	 4/25/97	djb		first checked in

	 <HFS17>	 4/16/97	djb		Remove unused variables to prevent warnings.
	 <HFS16>	 4/16/97	djb		Always use new B-tree code.
	 <HFS15>	 4/11/97	DSH		use extended VCB fields catalogRefNum, and extentsRefNum.
	 <HFS14>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS13>	 2/19/97	djb		Use real B-tree control blocks. Removed caching code.
	 <HFS12>	 1/17/97	msd		Fix an illegal cast from one structure type to another in
									CheckExtentsOverflow.
	 <HFS11>	 1/16/97	djb		Fixing extents overflow check for HFS+ volumes.
	 <HFS10>	 1/15/97	djb		Added support for checking HFS+ volumes.
	  <HFS9>	  1/3/97	djb		Added support for large keys.Integrated HFSVolumesPriv.h
									changes.
	  <HFS8>	12/23/96	djb		Fixed bug in CheckExtentsOverflow (bad key).
	  <HFS8>	12/23/96	djb		Fixed bug in CheckExtentsOverflow (key was wrong).
	  <HFS7>	12/19/96	DSH		All refs to VCB are now refs to ExtendedVCB
	  <HFS6>	12/19/96	djb		Updated for new B-tree Manager interface.
	  <HFS5>	12/13/96	djb		Use new Catalog SPI for GetCatalogNode.
	  <HFS4>	 12/4/96	DSH		Precompiled Headers
	  <HFS3>	11/21/96	msd		Removed definition of kHFSSignature since it is now in
									FilesInternal.h.
	  <HFS2>	11/19/96	DSH		Removed FilesPriv.h, moved MountCheck prototype to
									FilesInternal.h.
	  <HFS1>	11/13/96	djb		first checked in

	To do:		
			- If a cnid of 5 is found in extents btree make sure vcb attribute is set correctly
*/
#define	USE_BTREE_SCANNER 0

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
	#include	<Gestalt.h>
	#include	<Errors.h>
	#include	<ExpandMemPriv.h>
	#include	<Files.h>
	#include	<StringComparePriv.h>
	#include	<Types.h>
        #else
	#include "../headers/system/MacOSTypes.h"
    #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#include	"../headers/FileMgrInternal.h"
#include	"../headers/HFSBtreesPriv.h"
#include	"../headers/HFSVolumes.h"
#include	"../headers/BTreesPrivate.h"
#include	"../headers/BTreeScanner.h"


enum {
	kTypeDataFork				= 0x00,
	kTypeResourceFork			= 0xFF,	
	
	kVCBAttrLocked				= 0x8080,			// volume locked (software or hardware)
	kVCBFlagsVolumeDirty		= 0x8000
};


enum {
	kBTreePrevious		= -1,
	kBTreeCurrent		= 0,
	kBTreeNext			= 1
};


enum {
	kHFSOrphanedExtents				= 0x00000001,
	kHFSOverlapingExtents			= 0x00000002,
	kHFSCatalogBTreeLoop			= 0x00000004,
	kHFSExtentsBTreeLoop			= 0x00000008,
	kHFSOrphanedThreadRecords		= 0x00000010,
	kHFSMissingThreadRecords		= 0x00000020,
	kHFSInvalidPEOF					= 0x00000040,
	kHFSInvalidLEOF					= 0x00000080,
	kHFSInvalidValence				= 0x00000100,
	kHFSInvalidBTreeHeader			= 0x00000200,
	kHFSInvalidCatalogRecordType	= 0x00000400,	
	kHFSInconsistentVolumeName		= 0x00000800,

	kHFSMinorRepairsWereMade		= 0x80000000
};

enum {
	kMinimumBtreeNodesForCache		= 100
};

#define	kMDBValidAttributesMask		0x8780

#define	badCatalogErr	-1311


typedef struct {
	Handle	tempBufferHand;
	UInt32	tempBufferSize;
	Ptr		savedBufPtr;
	Ptr		savedGetNode;
	Ptr		savedRelNode;
	Ptr		savedNodeCache;
} BTreeState;


// Macros

//void M_ExitOnError(OSErr result);
//#define		M_ExitOnError( result )		if ( ( result ) != noErr )	goto ErrorExit; else ;

BTreeControlBlock* GetBTreeControlBlock(short refNum);
#define		GetBTreeControlBlock(refNum)	((BTreeControlBlock*) GetFileControlBlock((refNum))->fcbBTCBPtr)

Ptr LMGetVector(Ptr vector);
#define 	LMGetVector(V)					(*(Ptr*) (V))

void LMSetVector(Ptr vector, Ptr address);
#define		LMSetVector(V,A)				(*(Ptr *) (V) = (A))

Boolean HFSEqualString(ConstStr255Param str1, ConstStr255Param str2);
#define		HFSEqualString(s1, s2)			PascalBinaryCompare((s1), (s2))

#if	DIAGNOSTIC
	#ifdef KERNEL
        #include <sys/systm.h>
        #define PRINTIT kprintf
    #else /* !KERNEL */
        #define PRINTIT printf
    #endif /* KERNEL */
	#define VOLUME_CHECK_LOG(s)		PRINTIT("   MountCheck: %s\n",(s))
#else
	#define VOLUME_CHECK_LOG(s)
#endif


#if TARGET_OS_MAC
#define	GET_SCANNER_BUFFER()		LMGetFSVars()->gAttributesBuffer
#define	GET_SCANNER_BUFFER_SIZE()	LMGetFSVars()->gAttributesBufferSize
#endif


#if TARGET_OS_RHAPSODY
#define	GET_SCANNER_BUFFER()		NULL
#define	GET_SCANNER_BUFFER_SIZE()	0
#endif


// local prototypes

static OSErr	CheckCatalog (ExtendedVCB* volume, UInt32* freeBlocks, UInt32 xCatalogBlocks, UInt32* consistencyStatus );

static OSErr	CheckExtentsOverflow ( ExtendedVCB* volume, UInt32 *freeBlocks, UInt32 *xCatalogBlocks, UInt32* consistencyStatus );

static OSErr	CheckAttributes ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 *consistencyStatus );

static OSErr	CheckBitmapAndHeader ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 *consistencyStatus );

static void		VolumeBitMapCheckExtents ( ExtendedVCB* volume, HFSPlusExtentDescriptor* theExtents, UInt32* consistencyStatus );

static OSErr	CheckCreateDate (ExtendedVCB* volume, UInt32* consistencyStatus );

static Boolean	ValidPEOF (	ExtendedVCB* volume, CatalogRecord *file );

#if TARGET_OS_MAC
	static OSErr	FixOrphanFileThreads ( ExtendedVCB* volume, UInt32 maxRecords, UInt32 *repairsDone);
#else
	#define FixOrphanFileThreads(v, m, f)	0
#endif

#if TARGET_OS_MAC
static Handle	GetTempBuffer (Size size);
#endif /* TARGET_OS_MAC */

static UInt32	CountExtentBlocks (HFSPlusExtentRecord extents, Boolean hfsPlus);

static void		ConvertToHFSPlusExtent (const HFSExtentRecord oldExtents, HFSPlusExtentRecord newExtents);

static OSErr	GetFileExtentRecord( const ExtendedVCB	*vcb, const FCB *fcb, HFSPlusExtentRecord extents);

// glue prototypes

extern OSErr CacheReadInPlace(ExtendedVCB *volume, HIOParam *iopb, UInt32 currentPosition, UInt32 maximumBytes, UInt32 *actualBytes);

extern pascal Ptr LowerBufPtr(long size);


// 68K prototypes

extern OSErr FastGetNode(void);

extern OSErr FastRelNode(void);


/*-------------------------------------------------------------------------------

Routine:	MountCheck

Function:	Do a quick check of the volume meta-data to make sure that
			it is consistent. Minor inconsistencies in the MDB or volume
			bitmap will be repaired all other inconsistencies will be
			detected and reported in the consistencyStatus output.
			
			This check will terminated if any unexpected BTree errors
			are encountered.

Input:		volume				- pointer to volume control block
			consistencyStatus	- ????

Output:		none

Result:		noErr				- success

-------------------------------------------------------------------------------*/
OSErr
MountCheck( ExtendedVCB* volume, UInt32 *consistencyStatus )
{
	OSErr					result;
	UInt32					freeBlocks;
	UInt32					bitmapFreeBlocks;
	UInt32					xCatalogBlocks;


	*consistencyStatus = 0;
	freeBlocks = volume->totalBlocks;	// initially all blocks are free

	if (volume->vcbSigWord == kHFSPlusSigWord)
	{
		//--- make sure the volume header's create date matches the MDB's create date
		VOLUME_CHECK_LOG("checking HFS Plus wrapper's create date...");
		result = CheckCreateDate (volume, consistencyStatus);
		M_ExitOnError(result);
	}

	//--- make sure the allocation file, volume header, and alternate volume header are allocated
	VOLUME_CHECK_LOG("checking allocation file...");
	result = CheckBitmapAndHeader ( volume, &freeBlocks, consistencyStatus );
	M_ExitOnError(result);

	//--- do a quick consistency check of the Extents Overflow b-tree ----------------------------
	VOLUME_CHECK_LOG("checking extents overflow b-tree...");
	result = CheckExtentsOverflow(volume, &freeBlocks, &xCatalogBlocks, consistencyStatus);
	M_ExitOnError(result);

	//--- do a quick consistency check of the Catalog b-tree ------------------------------------
	VOLUME_CHECK_LOG("checking catalog b-tree...");
	result = CheckCatalog( volume, &freeBlocks, xCatalogBlocks, consistencyStatus );
	M_ExitOnError( result );

	//--- do a quick consistency check of the Attributes b-tree ------------------------------------
	result = CheckAttributes( volume, &freeBlocks, consistencyStatus );
	M_ExitOnError( result );

	if( freeBlocks != volume->freeBlocks && volume->vcbAtrb & kVCBAttrLocked )
	{
		*consistencyStatus |= kHFSMinorRepairsWereMade;
		VOLUME_CHECK_LOG("repairing free block count");
	}

	(void) UpdateFreeCount( volume );	// update freeBlocks from volume bitmap

	// make sure there are not any doubly allocated blocks...
	// we are not concerned with orphaned blocks (since this is not fatal)
	
	bitmapFreeBlocks = volume->freeBlocks;
	
	if ( freeBlocks < bitmapFreeBlocks)			// there are doubly allocated blocks!
	{
		*consistencyStatus |= kHFSOverlapingExtents;
		VOLUME_CHECK_LOG("uh oh, there are doubly allocated file blocks!");
		goto ErrorExit;
	}
	else if ( freeBlocks > bitmapFreeBlocks)	// there are orphaned blocks
	{
		*consistencyStatus |= kHFSOrphanedExtents;
		VOLUME_CHECK_LOG("orphaned volume bitmap blocks detected");
	}

	return noErr;
	
ErrorExit:

	VOLUME_CHECK_LOG("a fatal inconsistency was found! :( ");

	return badMDBErr;
}


/*-------------------------------------------------------------------------------

Routine:	VolumeCheckCatalog

Function:	Make sure the name of the root matches the volume name in the MDB.
			Make sure the volume bitmap reflects the allocations of all file
			extents.

Assumption:	

Input:		volume			- pointer to volume control block
			freeBlocks
			xCatalogBlocks
			consistencyStatus

Output:		none

Result:		noErr			- success

-------------------------------------------------------------------------------*/
static OSErr
CheckCatalog ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 xCatalogBlocks, UInt32 *consistencyStatus )
{
	UInt32				maxRecords;
	UInt32				largestCNID;
	UInt32				fileCount;
	UInt16				rootFileCount;			//XXX can this overflow on HFS Plus volumes?
	UInt16				rootDirectoryCount;		//XXX can this overflow on HFS Plus volumes?
	UInt32				directoryCount;
	UInt32				fileThreads, directoryThreads;
	UInt32				fileThreadsExpected;
	UInt32				catalogValence;
	UInt32				recordsFound;
	UInt32				catalogBlocks;
	OSErr				result;
	CatalogNodeData		nodeData;
	CatalogRecord		*record;
	CatalogKey			*key;
	FSSpec				spec;
	UInt16				recordSize;
	BTreeControlBlock*	btree;
	UInt32				hint;
	FCB*				file;
	HFSPlusExtentRecord	extentRecord;
	BTScanState			scanState;
	Boolean				hfsPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	UInt16				operation;
	BTreeIterator		btreeIterator;
	FSBufferDescriptor	btRecord;
	CatalogRecord		catalogRecord;

	fileThreads = fileThreadsExpected = directoryThreads = 0;
	fileCount = directoryCount = 0;
	rootFileCount = rootDirectoryCount = 0;
	catalogValence = 0;
	recordsFound = 0;
	largestCNID = kHFSFirstUserCatalogNodeID;

	file = GetFileControlBlock(volume->catalogRefNum);
	
	result = GetFileExtentRecord(volume, file, extentRecord);
	M_ExitOnError(result);

	catalogBlocks = CountExtentBlocks(extentRecord, hfsPlus);	
	*freeBlocks -= catalogBlocks;
	VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);


	//	Read in the root directory's record so we can perform some special checks	
	hint = 0;
	result = GetCatalogNode(volume, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint);
	M_ExitOnError(result);

	if ( DEBUG_BUILD && nodeData.nodeID != kHFSRootFolderID )	// works for small and large folders
		DebugStr("\p CheckCatalog: nodeData.nodeID != kHFSRootFolderID");

//	We'll see the root directory's record later, during the full BTree scan, so don't
//	update the counts here.
//	++recordsFound;
//	catalogValence += nodeData.valence;
	
	/* UUU */
	// make sure the name of the root directory matches the volume name (HFS only)
	if ( !hfsPlus && false /*&& !HFSEqualString( spec.name, volume->vcbVN )*/ )
	{
		if ( volume->vcbAtrb & kVCBAttrLocked )
		{
			*consistencyStatus |= kHFSInconsistentVolumeName;
		}
		else // try and repair it
		{
			if (strlen( volume->vcbVN ) > 0)	// check length of volume name
			{
				if (volume->vcbVN[0] > kHFSMaxVolumeNameChars)	// if name is too long truncate it
					volume->vcbVN[0] = kHFSMaxVolumeNameChars;
					
				if ( (!FORDISKFIRSTAID) || (GetDFAStage() == kRepairStage) )
				{
					// the length of volume name is OK, rename the root directory
					result = MoveRenameCatalogNode(volume, kHFSRootFolderID, NULL, hint, kHFSRootFolderID, volume->vcbVN, &hint);
					M_ExitOnError(result);
				}
				*consistencyStatus |= kHFSMinorRepairsWereMade;
				
				// now we must reset for btree search below!
				result = GetCatalogNode(volume, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint);
				M_ExitOnError(result);
			}
			else if ( strlen( spec.name ) > 0 )	// check root name length
			{
				if ( spec.name[0] > kHFSMaxVolumeNameChars )	// if name is too long truncate it
					 spec.name[0] = kHFSMaxVolumeNameChars;
	
				// the root name length is OK, change the volume name to match root name
				BlockMoveData( spec.name, volume->vcbVN, spec.name[0]+1);
				volume->vcbFlags |= kVCBFlagsVolumeDirty;
				*consistencyStatus |= kHFSMinorRepairsWereMade;
			}
		}
		VOLUME_CHECK_LOG("inconsistent volume name (hfs disk)");
	}

	// make sure name locked bit is not set
//	if (record.hfsFolder.finderDirInfo.frFlags & kNameLocked)  //XXX need to use node data, not "record"
//	{
//	}
	
	// Make a conservative estimate of the upper limit of B*-Tree records that
	// could conceivably be encountered in a scan of all the leaf nodes.
	maxRecords = (file->fcbEOF) / sizeof(HFSCatalogFolder);

	if (USE_BTREE_SCANNER)	// scan the tree in physical order (faster)
	{
		result = BTScanInitialize(file, 0, 0, 0, GET_SCANNER_BUFFER(), GET_SCANNER_BUFFER_SIZE(), &scanState);
		if (result != noErr)
			goto ErrorExit;
	}
	else // scan in leaf node order (slower but more reliable)
	{
		operation = kBTreeFirstRecord;	// first leaf record

		key = (CatalogKey*) &btreeIterator.key;

		btRecord.bufferAddress	= record = &catalogRecord;
		btRecord.itemCount		= 1;
		btRecord.itemSize		= sizeof(catalogRecord);
	}


	// visit all the leaf node data records in the catalog
	while ( --maxRecords > 0 )
	{
		if (USE_BTREE_SCANNER)
			result = BTScanNextRecord(&scanState, false, (void**)&key, (void**)&record, (UInt32*) &recordSize);
		else
			result = BTIterateRecord(file, operation, &btreeIterator, &btRecord, &recordSize);
					
		if ( result != noErr )
			break;

		++recordsFound;
		operation = kBTreeNextRecord;

		switch (record->recordType)
		{
			case kHFSFolderRecord:
			{
				catalogValence += record->hfsFolder.valence;
	
				//	Count all directories except the root itself
				if (key->hfs.parentID != kHFSRootParentID)
				++directoryCount;

				//	Count directories directly inside the root
				if (key->hfs.parentID == kHFSRootFolderID)
					++rootDirectoryCount;
	
				if (record->hfsFolder.folderID > largestCNID)
					largestCNID = record->hfsFolder.folderID;
				break;
			}

			case kHFSPlusFolderRecord:
			{
				catalogValence += record->hfsPlusFolder.valence;
	
				//	Count all directories except the root itself
				if (key->hfsPlus.parentID != kHFSRootParentID)
					++directoryCount;

				//	Count directories directly inside the root
				if (key->hfsPlus.parentID == kHFSRootFolderID)
					++rootDirectoryCount;
					
				if (record->hfsPlusFolder.folderID > largestCNID)
					largestCNID = record->hfsPlusFolder.folderID;
				break;
			}

			case kHFSFileRecord:
			{
				++fileCount;
				if ( key->hfs.parentID == kHFSRootFolderID )
					++rootFileCount;
	
				if ( record->hfsFile.fileID > largestCNID )
					largestCNID = record->hfsFile.fileID;
	
				if ( record->hfsFile.flags & kHFSThreadExistsMask )
					++fileThreadsExpected;
	
				// check the blocks allocated to this file (both forks)
				
				ConvertToHFSPlusExtent(record->hfsFile.dataExtents, extentRecord);	
				*freeBlocks -= CountExtentBlocks(extentRecord, false);		
				VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);
		
				ConvertToHFSPlusExtent(record->hfsFile.rsrcExtents, extentRecord);
				*freeBlocks -= CountExtentBlocks(extentRecord, false);		
				VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);

				// check the LEOF and PEOF (both forks)
								
				if ( (record->hfsFile.rsrcLogicalSize > record->hfsFile.rsrcPhysicalSize)	||
					 (record->hfsFile.dataLogicalSize > record->hfsFile.dataPhysicalSize) )
				{
					*consistencyStatus |= kHFSInvalidLEOF;
					VOLUME_CHECK_LOG("inconsistent file leof (not fixed)");
				}

				if ( !ValidPEOF(volume, record) )
				{
					*consistencyStatus |= kHFSInvalidPEOF;
					VOLUME_CHECK_LOG("inconsistent file peof (not fixed)");
				}
				break;
			}

			case kHFSPlusFileRecord:
			{
				UInt32	blockSize = volume->blockSize;

				++fileCount;
				++fileThreadsExpected;	// HFS Plus files always have thread records

				if ( key->hfsPlus.parentID == kHFSRootFolderID )
					++rootFileCount;
	
				if ( record->hfsPlusFile.fileID > largestCNID )
					largestCNID = record->hfsPlusFile.fileID;
		
				// check the blocks allocated to this file (both forks)
				
				*freeBlocks -= CountExtentBlocks(record->hfsPlusFile.dataFork.extents, true);		
				VolumeBitMapCheckExtents(volume, record->hfsPlusFile.dataFork.extents, consistencyStatus);
		
				*freeBlocks -= CountExtentBlocks(record->hfsPlusFile.resourceFork.extents, true);		
				VolumeBitMapCheckExtents(volume, record->hfsPlusFile.resourceFork.extents, consistencyStatus);

				// check the LEOF and PEOF (both forks)
								
				if ( (record->hfsPlusFile.resourceFork.logicalSize.lo > record->hfsPlusFile.resourceFork.totalBlocks * blockSize) ||
					 (record->hfsPlusFile.dataFork.logicalSize.lo > record->hfsPlusFile.dataFork.totalBlocks * blockSize) )
				{
					*consistencyStatus |= kHFSInvalidLEOF;
					VOLUME_CHECK_LOG("inconsistent file leof (not fixed)");
				}

				if ( !ValidPEOF(volume, record) )
				{
					*consistencyStatus |= kHFSInvalidPEOF;
					VOLUME_CHECK_LOG("inconsistent file peof (not fixed)");
				}
				break;
			}

			case kHFSFolderThreadRecord:
			{
				++directoryThreads;
				if ( key->hfs.parentID > largestCNID )		// <CS7>
					largestCNID = key->hfs.parentID;				
				break;
			}

			case kHFSPlusFolderThreadRecord:
			{
				++directoryThreads;
				if ( key->hfsPlus.parentID > largestCNID )		// <CS7>
					largestCNID = key->hfsPlus.parentID;
				break;
			}

			case kHFSFileThreadRecord:
			{
				++fileThreads;
				if ( key->hfs.parentID > largestCNID )		// <CS7>
					largestCNID = key->hfs.parentID;				
				break;
			}

			case kHFSPlusFileThreadRecord:
			{
				++fileThreads;
				if ( key->hfsPlus.parentID > largestCNID )		// <CS7>
					largestCNID = key->hfsPlus.parentID;
				break;
			}
	
			default:
				*consistencyStatus |= kHFSInvalidCatalogRecordType;
				VOLUME_CHECK_LOG("unknown catalog record");
				break;
		}
	
	} // end while
	
	if ( result == noErr )
	{
		*consistencyStatus |= kHFSCatalogBTreeLoop;
		goto ErrorExit;		// punt on loop error
	}
	
	if ( result != fsBTRecordNotFoundErr )
		goto ErrorExit;		// punt on BTree errors


	// Check if calculated totals match the Volume Control Block
	
	if (volume->vcbNxtCNID <= largestCNID)			// <CS7>
	{
		volume->vcbNxtCNID = largestCNID + 1;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		VOLUME_CHECK_LOG("fixed vcbNxtCNID");
	}

	if (volume->vcbFilCnt != fileCount)
	{
		volume->vcbFilCnt = fileCount;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		VOLUME_CHECK_LOG("fixed vcbFilCnt (volume file count)");
	}

	if (volume->vcbDirCnt != directoryCount)
	{
		volume->vcbDirCnt = directoryCount;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		VOLUME_CHECK_LOG("fixed vcbDirCnt (volume directory count)");
	}

	if (!hfsPlus && volume->vcbNmFls != rootFileCount)
	{
		volume->vcbNmFls = rootFileCount;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		VOLUME_CHECK_LOG("fixed vcbNmFls (root file count)");
	}

	if (!hfsPlus && volume->vcbNmRtDirs != rootDirectoryCount)
	{
		volume->vcbNmRtDirs = rootDirectoryCount;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		VOLUME_CHECK_LOG("fixed vcbNmRtDirs (root directory count)");
	}
	
	if ( catalogValence != (fileCount + directoryCount) )
	{
		*consistencyStatus |= kHFSInvalidValence;			// valence inconsistency detected
		VOLUME_CHECK_LOG("valence inconsistency detected (not fixed)");
	}

#if 0	
	if (volume->vcbAtrb & ~kMDBValidAttributesMask)	// check for invalid bits
	{
		volume->vcbAtrb &= kMDBValidAttributesMask;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
	}
#endif
	
	if ( (volume->vcbFlags & kVCBFlagsVolumeDirty) && (volume->vcbAtrb & kVCBAttrLocked) == 0 )
		*consistencyStatus |= kHFSMinorRepairsWereMade;

	if (directoryThreads > (directoryCount + 1))
	{
		*consistencyStatus |= kHFSOrphanedThreadRecords;		// too many directory thread records
		VOLUME_CHECK_LOG("orphaned directory thread records detected (not fixed)");
	}
		
	if (fileThreads > fileThreadsExpected)
	{
		if ( volume->vcbAtrb & kVCBAttrLocked )
		{
			*consistencyStatus |= kHFSOrphanedThreadRecords;	// too many file thread records
			VOLUME_CHECK_LOG("orphaned file thread records detected (not fixed)");
		}
		else if ( !hfsPlus ) // try and repair orphaned thread records
		{
			// If we fix up some threads, see if we can come out even without counting all the file
			// threads again. If we don't, then looks like we have to keep count and do a second
			// pass of file records (gotta do that anyway when we fix the KHFSMissingThreadRecords
			// case below) and to count thread records again with the FixOrphanFileThreads call.
	
			if ( (!FORDISKFIRSTAID) || (GetDFAStage() == kRepairStage) )
			{
				UInt32 repairsDone = 0;
					
				(void) FixOrphanFileThreads(volume, recordsFound, &repairsDone);
				if (repairsDone)
				{
					*consistencyStatus |= kHFSMinorRepairsWereMade;
					VOLUME_CHECK_LOG("orphaned file thread records detected (fixed)");
				}
	
				if (fileThreads > (fileThreadsExpected + repairsDone) )
				{
					*consistencyStatus |= kHFSOrphanedThreadRecords;	// still too many thread records
					VOLUME_CHECK_LOG("orphaned file thread records detected (fix attempt failed)");
				}
			}
		}
	}

	if ((directoryThreads < (directoryCount + 1)) || (fileThreads < fileThreadsExpected))
	{
		*consistencyStatus |= kHFSMissingThreadRecords;		// not enough thread records
		VOLUME_CHECK_LOG("some catalog thread records are missing (not fixed)");
	}

	btree = GetBTreeControlBlock(volume->catalogRefNum);

	// okay, lets see if the blocks used by the catalog looks right
	catalogBlocks += xCatalogBlocks;
	if ( (catalogBlocks * volume->blockSize) != file->fcbPLen )
	{
		*consistencyStatus |= kHFSInvalidPEOF;
		VOLUME_CHECK_LOG("inconsistent catalog file size detected (not fixed)");
	}

	if ( btree->totalNodes != (file->fcbPLen / btree->nodeSize) )
	{
		*consistencyStatus |= kHFSInvalidBTreeHeader;
		VOLUME_CHECK_LOG("inconsistent catalog b-tree node count detected (not fixed)");
	}

	if (btree->leafRecords != recordsFound)
	{
		// We now use the B-tree scanner instead of iterating through all
		// the leaf nodes in order.  So let's be more conservative and not
		// fix the leafRecords field until we can do adequate testing of
		// this new method of visiting nodes. -DJB

		if (USE_BTREE_SCANNER || volume->vcbAtrb & kVCBAttrLocked)
		{
			*consistencyStatus |= kHFSInvalidBTreeHeader;		// invalid record count
			VOLUME_CHECK_LOG("inconsistent catalog b-tree record count detected (not fixed)");
		}
		else // go ahead and make this simple repair!
		{		
			btree->leafRecords = recordsFound;
			btree->flags |= kBTHeaderDirty;						// mark BTreeControlBlock dirty
			*consistencyStatus |= kHFSMinorRepairsWereMade;
			VOLUME_CHECK_LOG("inconsistent catalog b-tree record count detected (fixed)");
		}
	}
	
	if ( (!FORDISKFIRSTAID) || (GetDFAStage() == kRepairStage) )
	{
		if ( btree->flags & kBTHeaderDirty )	// if we made changes, write them out
			(void) BTFlushPath(file);
	}

	return noErr;
	
ErrorExit:
	return badCatalogErr;

} // end CheckCatalog


/*-------------------------------------------------------------------------------

Routine:	CheckExtentsOverflow

Function:	Make sure the volume bitmap reflects the allocations of all extents.

Assumption:	

Input:		volume			- pointer to volume control block
			freeBlocks
			xCatalogBlocks
			consistencyStatus

Output:		none

Result:		noErr			- success

-------------------------------------------------------------------------------*/
static OSErr
CheckExtentsOverflow ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 *xCatalogBlocks, UInt32 *consistencyStatus )
{
	OSErr				result;
	UInt32				maxRecords;
	UInt32				fileID;
	UInt32				largestCNID;
	UInt32				recordsFound; 
	UInt32				extentFileBlocks;
	UInt32				extentRecordBlocks;
	HFSPlusExtentKey	*extentKeyPtr;
	HFSPlusExtentRecord	*extentDataPtr;
	HFSPlusExtentRecord	extentRecord;
	UInt16				recordSize;
	BTreeControlBlock	*btree;
	FCB*				file;
	BTScanState			scanState;
	Boolean				hfsPlus = (volume->vcbSigWord == kHFSPlusSigWord);
	UInt16				operation;
	BTreeIterator		btreeIterator;
	FSBufferDescriptor	btRecord;

	*xCatalogBlocks = 0;  // set this early since its a return value
    largestCNID = kHFSFirstUserCatalogNodeID;
	
	file = GetFileControlBlock(volume->extentsRefNum);

	result = GetFileExtentRecord(volume, file, extentRecord);
	ReturnIfError(result);

	extentFileBlocks = CountExtentBlocks(extentRecord, hfsPlus);
	*freeBlocks -= extentFileBlocks;
	VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);

	// now look at the extent tree...

	// Make a conservative estimate of the upper limit of B*-Tree records that
	// could conceivably be encountered in a scan of all the leaf nodes.
	recordsFound = 0;
	maxRecords = (file->fcbEOF) / (hfsPlus ? sizeof(HFSPlusExtentRecord) : sizeof(HFSExtentRecord));

	if (USE_BTREE_SCANNER)	// scan the tree in physical order (faster)
	{
		result = BTScanInitialize(file, 0, 0, 0, GET_SCANNER_BUFFER(), GET_SCANNER_BUFFER_SIZE(), &scanState);
		if (result != noErr)
			return badMDBErr;
	}
	else // scan in leaf node order (slower but more reliable)
	{
		operation = kBTreeFirstRecord;	// first leaf record
		extentKeyPtr = (HFSPlusExtentKey*) &btreeIterator.key;

		btRecord.bufferAddress	= &extentRecord;
		btRecord.itemCount		= 1;
		btRecord.itemSize		= sizeof(extentRecord);
	}

	// visit all the leaf node data records in the extents B*-Tree
	while ( recordsFound < maxRecords )
	{
		if (USE_BTREE_SCANNER)
			result = BTScanNextRecord(&scanState, false, (void **) &extentKeyPtr, (void **) &extentDataPtr, (UInt32*) &recordSize);
		else
			result = BTIterateRecord(file, operation, &btreeIterator, &btRecord, &recordSize);

		if ( result != noErr )
			break;

		++recordsFound;

		// check the blocks allocated to this extent record
		
		if (USE_BTREE_SCANNER)
		{
			if ( hfsPlus )
				BlockMoveData(extentDataPtr, &extentRecord, sizeof(extentRecord));				// just make a copy
			else
				ConvertToHFSPlusExtent(*((HFSExtentRecord *) extentDataPtr), extentRecord);		// convert it
			}
		else
		{
			operation = kBTreeNextRecord;
			if ( !hfsPlus )
				ConvertToHFSPlusExtent(*(HFSExtentRecord*) &extentRecord, extentRecord);	// convert it in place (since its a copy)
		}

		extentRecordBlocks = CountExtentBlocks(extentRecord, hfsPlus);
		
		*freeBlocks -= extentRecordBlocks;		
		VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);
		
		if ( hfsPlus )
			fileID = extentKeyPtr->fileID;
		else
			fileID = ((HFSExtentKey*) extentKeyPtr)->fileID;
		
		// if these were catalog blocks, we need to keep track 
		if ( fileID == kHFSCatalogFileID )
			*xCatalogBlocks += extentRecordBlocks;
			
		if ( fileID > largestCNID )
			largestCNID = fileID;

	} // end while

	if ( result != fsBTRecordNotFoundErr) 
	{
		VOLUME_CHECK_LOG("extents b-tree has serious problems!");
		
		return badMDBErr;		// punt on error (loop error or BTree error)
	}

	if (volume->vcbNxtCNID < largestCNID)
	{
		volume->vcbNxtCNID = largestCNID + 1;
		volume->vcbFlags |= kVCBFlagsVolumeDirty;
		volume->vcbAtrb |= 1;	// need a constant for this!
	}	
	
	btree = GetBTreeControlBlock(volume->extentsRefNum);

	if ( (extentFileBlocks * volume->blockSize) != file->fcbPLen )
	{
		*consistencyStatus |= kHFSInvalidPEOF;
		VOLUME_CHECK_LOG("inconsistent extents file size detected (not fixed)");
	}

	if ( btree->totalNodes != (file->fcbPLen / btree->nodeSize) )
	{
		*consistencyStatus |= kHFSInvalidBTreeHeader;
		VOLUME_CHECK_LOG("inconsistent extents b-tree node count detected (not fixed)");
	}
	
	if (btree->leafRecords != recordsFound)
	{
		// We now use the B-tree scanner instead of iterating through all
		// the leaf nodes in order.  So let's be more conservative and not
		// fix the leafRecords field until we can do adequate testing of
		// this new method of visiting nodes. -DJB
	
		if (USE_BTREE_SCANNER || volume->vcbAtrb & kVCBAttrLocked)
		{
			*consistencyStatus |= kHFSInvalidBTreeHeader;
			VOLUME_CHECK_LOG("inconsistent extents b-tree record count detected (not fixed)");
		}
		else // go ahead and make this simple repair!
		{
			btree->leafRecords = recordsFound;
			btree->flags |= kBTHeaderDirty;				// mark BTreeControlBlock dirty
			*consistencyStatus |= kHFSMinorRepairsWereMade;
			VOLUME_CHECK_LOG("inconsistent extents b-tree record count detected (fixed)");
		}
	}
	
	if ( (!FORDISKFIRSTAID) || (GetDFAStage() == kRepairStage) )
	{
		if ( btree->flags & kBTHeaderDirty )	// if we made changes, write them out
			(void) BTFlushPath(file);
	}

	return noErr;

} // end CheckExtentsOverflow



/*-------------------------------------------------------------------------------

Routine:	CheckAttributes

Function:	Make sure the volume bitmap reflects the allocations of all attributes
			that occupy extents.

Assumption:	

Input:		volume			- pointer to volume control block
			freeBlocks
			consistencyStatus

Output:		none

Result:		noErr			- success

-------------------------------------------------------------------------------*/
static OSErr
CheckAttributes ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 *consistencyStatus )
{
	OSErr				result;
	UInt32				maxRecords;
	UInt32				recordsFound; 
	UInt32				attributesFileBlocks;
	UInt32				extentRecordBlocks;
	AttributeKey		*attributeKey;
	HFSPlusAttrRecord	*attributeData;
	HFSPlusExtentRecord	extentRecord;
	UInt32				recordSize;
	BTreeControlBlock	*btree;
	FCB*				file;
	BTScanState			scanState;
	Boolean				hfsPlus = (volume->vcbSigWord == kHFSPlusSigWord);

	//
	//	If there isn't an attributes B-tree, there's no work to do
	//
	if (volume->attributesRefNum == 0)
		return noErr;

	file = GetFileControlBlock(volume->attributesRefNum);

	result = GetFileExtentRecord(volume, file, extentRecord);
	ReturnIfError(result);

	attributesFileBlocks = CountExtentBlocks(extentRecord, hfsPlus);
	*freeBlocks -= attributesFileBlocks;
	VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);


	// now look at the attributes tree...

	// Make a conservative estimate of the upper limit of B*-Tree records that
	// could conceivably be encountered in a scan of all the leaf nodes.
	recordsFound = 0;
	maxRecords = (file->fcbEOF) / 8;		// 8 = minimum data length (for zero length inline attribute)

	//	Prepare to scan the tree in physical order
	result = BTScanInitialize(file, 0, 0, 0, GET_SCANNER_BUFFER(), GET_SCANNER_BUFFER_SIZE(), &scanState);
	if (result != noErr)
		return badMDBErr;

	// visit all the leaf node data records in the extents B*-Tree
	while ( recordsFound < maxRecords )
	{
		result = BTScanNextRecord(&scanState, false, (void **) &attributeKey, (void **) &attributeData, &recordSize);

		if ( result != noErr )
			break;

		++recordsFound;

		//	If this block has information about extents, make sure they're allocated
		switch (attributeData->recordType) {
			case kHFSPlusAttrInlineData:
				break;
            case kHFSPlusAttrForkData:
				extentRecordBlocks = CountExtentBlocks(attributeData->forkData.theFork.extents, true);
				*freeBlocks -= extentRecordBlocks;
				VolumeBitMapCheckExtents(volume, attributeData->forkData.theFork.extents, consistencyStatus);
				break;
            case kHFSPlusAttrExtents:
				extentRecordBlocks = CountExtentBlocks(attributeData->overflowExtents.extents, true);
				*freeBlocks -= extentRecordBlocks;
				VolumeBitMapCheckExtents(volume, attributeData->overflowExtents.extents, consistencyStatus);
				break;
			default:
				if (DEBUG_BUILD)
					DebugStr("\pHFS+: Unknown attribute record");
				break;
		}	// end switch
	} // end while

	if ( result != fsBTRecordNotFoundErr )
	{
		VOLUME_CHECK_LOG("extents b-tree has serious problems!");
		return badMDBErr;		// punt on error (loop error or BTree error)
	}

	btree = GetBTreeControlBlock(volume->attributesRefNum);

	if ( (attributesFileBlocks * volume->blockSize) != file->fcbPLen )
		*consistencyStatus |= kHFSInvalidPEOF;

	if ( btree->totalNodes != (file->fcbPLen / btree->nodeSize) )
		*consistencyStatus |= kHFSInvalidBTreeHeader;
	
	if (btree->leafRecords != recordsFound)
	{
		// We now use the B-tree scanner instead of iterating through all
		// the leaf nodes in order.  So let's be more conservative and not
		// fix the leafRecords field until we can do adequate testing of
		// this new method of visiting nodes. -DJB

		if (USE_BTREE_SCANNER || volume->vcbAtrb & kVCBAttrLocked)
		{
			*consistencyStatus |= kHFSInvalidBTreeHeader;
		}
		else // go ahead and make this simple repair!
		{
			btree->leafRecords = recordsFound;
			btree->flags |= kBTHeaderDirty;				// mark BTreeControlBlock dirty
			*consistencyStatus |= kHFSMinorRepairsWereMade;
		}
	}
	
	if ( (!FORDISKFIRSTAID) || (GetDFAStage() == kRepairStage) )
	{
		if ( btree->flags & kBTHeaderDirty )	// if we made changes, write them out
			(void) BTFlushPath(file);
	}
	return noErr;

} // end CheckAttributes



/*-------------------------------------------------------------------------------

Routine:	CheckBitmapAndHeader

Function:	Make sure the volume header, alternate volume header, and the bitmap
			itself are marked as used in the bitmap.

Assumption:	

Input:		volume			- pointer to volume control block
			freeBlocks
			consistencyStatus

Output:		none

Result:		noErr			- success

-------------------------------------------------------------------------------*/
static OSErr
CheckBitmapAndHeader ( ExtendedVCB *volume, UInt32 *freeBlocks, UInt32 *consistencyStatus )
{
	OSErr				result;
	UInt32				blocksUsed;
	FCB*				file;
	HFSPlusExtentRecord	extentRecord;

	//
	//	If there isn't an allocation file, there's no work to do
	//
	if (volume->allocationsRefNum == 0)
		return noErr;

	file = GetFileControlBlock(volume->allocationsRefNum);

	result = GetFileExtentRecord(volume, file, extentRecord);
	ReturnIfError(result);

	//	Mark the space used by the allocation file (bitmap) itself
	blocksUsed = CountExtentBlocks(extentRecord, true);
	*freeBlocks -= blocksUsed;
	VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);

	//	Now mark sectors zero through two (boot blocks and volume header)
	//	and the last two sectors (alternate volume header and a reserved
	//	sector).  The number of allocation blocks depends on the size of
	//	an allocation block.  What we do is create a fake extent record
	//	for these blocks, and call VolumeBitMapCheckExtents as usual.

	ClearMemory(&extentRecord, sizeof(extentRecord));
	
	extentRecord[0].startBlock = 0;
	extentRecord[0].blockCount = 1 + (1024 / volume->blockSize);	// sectors 0-2

	if (volume->blockSize == 512) {
		extentRecord[1].startBlock = volume->totalBlocks - 2;
		extentRecord[1].blockCount = 2;
	} else {
		extentRecord[1].startBlock = volume->totalBlocks - 1;
		extentRecord[1].blockCount = 1;
	}
	
	*freeBlocks -= extentRecord[0].blockCount + extentRecord[1].blockCount;
	VolumeBitMapCheckExtents(volume, extentRecord, consistencyStatus);

	return noErr;
}


//	For Disk First Aid, this routine will not update the BitMap, but will set the
//	kHFSMinorRepairsWereMade bit to indicate the bitmap needs updating.
static void VolumeBitMapCheckExtents (
							ExtendedVCB*			volume,
							HFSPlusExtentDescriptor*	theExtents,
							UInt32*					consistencyStatus )
{
	if ( BlockCheck(volume, theExtents) == -1 )
	{
		*consistencyStatus |= kHFSMinorRepairsWereMade;
		VOLUME_CHECK_LOG("repairing volume bitmap");
	}
}


/*-------------------------------------------------------------------------------

Routine:	ValidPEOF

Function:	Make sure the local extents match the PEOF.
			Currently this only checks the local extent.
			We could also check the oveflow extents for
			a more complete (but slower) test.

Assumption:	

Input:		

Output:		

Result:	

-------------------------------------------------------------------------------*/

static Boolean
ValidPEOF( ExtendedVCB* volume, CatalogRecord *file )
{
	UInt32	minPEOF;
	UInt32 	allocBlkSize = volume->blockSize;
	
	if (file->recordType == kHFSPlusFileRecord)
	{
		UInt32	minBlocks;

		minBlocks = CountExtentBlocks(file->hfsPlusFile.resourceFork.extents, true);
		
		if ( file->hfsPlusFile.resourceFork.totalBlocks < minBlocks )
			return false;
		else if ( (file->hfsPlusFile.resourceFork.totalBlocks > minBlocks) && (file->hfsPlusFile.resourceFork.extents[kHFSPlusExtentDensity-1].blockCount == 0) )
			return false;
	
		minBlocks = CountExtentBlocks(file->hfsPlusFile.dataFork.extents, true);
		
		if ( file->hfsPlusFile.dataFork.totalBlocks < minBlocks )
			return false;
		else if ( (file->hfsPlusFile.dataFork.totalBlocks > minBlocks) && (file->hfsPlusFile.dataFork.extents[kHFSPlusExtentDensity-1].blockCount == 0) )
			return false;
	}
	else
	{
		HFSPlusExtentRecord	extentRecord;
		
		ConvertToHFSPlusExtent(file->hfsFile.rsrcExtents, extentRecord);	
		minPEOF = CountExtentBlocks(extentRecord, false) * allocBlkSize;
		
		if ( file->hfsFile.rsrcPhysicalSize < minPEOF )
			return false;
		else if ( (file->hfsFile.rsrcPhysicalSize > minPEOF) && (extentRecord[kHFSExtentDensity-1].blockCount == 0) )
			return false;
	

		ConvertToHFSPlusExtent(file->hfsFile.dataExtents, extentRecord);	
		minPEOF = CountExtentBlocks(extentRecord, false) * allocBlkSize;
		
		if ( file->hfsFile.dataPhysicalSize < minPEOF )
			return false;
		else if ( (file->hfsFile.dataPhysicalSize > minPEOF) && (extentRecord[kHFSExtentDensity-1].blockCount == 0) )
			return false;
	}

		
	return true;
}


/*-------------------------------------------------------------------------------

Routine:	FixOrphanFileThreads

Function:	For orphan thread records during VolumeCheckConsistency have the catalog
			searched again for file records that match. If their thread record flag
			is off, mark it. If it is on, do nothing. If the file record doesn't exist,
			delete the thread record.

Assumption:	This is a function to be called only if the thread records detected during
			CheckCatalog was greater than the file records that had their flag set for
			file thread existance. I've thought of doing this differently, like keeping
			a table of all file records or threads scanned in the first pass, but the
			table gets large. You can't predict how large so may to hold upto the max
			fileIDs possible. (2**32). Using two such tables to match file threads
			against file records would be bad. Less space would be keeping a bitmap of
			all fileIDS (to 2**32) and marking those with threads. Then a separate
			bitmap for those file records with thread flags on marked.You do a byte by
			byte comparison across the whole bitmap. When they differ, you know if you're
			missing
			threads or orphans threads. (Note 0s would also mean IDs represented by that
			bit in the bitmap that is non-existant). Though, this sounds clever, it still
			seems it would take 8bits/byte * 512 bytes/sector * 8 sectors/ vmpage = 2**15
			bits per vmpage and 2**17 vmpages to cover the bitmap! Too large. SO, let's
			just scan the catalog again. Yep, it's timeconsuming, but we are only taking
			the hit in repair mode for that particular problem. No extra memory for tables
			or bitmaps are even required during the original scan.

Input:		
								
Output:		

Result:		E_NoError				- success
			!= E_NoError			- failure

-------------------------------------------------------------------------------*/
#if TARGET_OS_MAC
static OSErr FixOrphanFileThreads(ExtendedVCB* volume, UInt32 maxRecords, UInt32 *repairsDone)
{
	UInt16				recordSize;
	OSErr				tempResult, result = noErr;
	CatalogKey			key;
	FSSpec				spec;
	CatalogRecord		record;
	CatalogNodeData		nodeData;
	UInt32				hint = 0;
		
	// let's iterate the catalog looking for file thread records. As we hit each one, 
	// do a search for the file record. If it exists and the thread flag is off,set
	// it. If it exists, and the thread flag is on, all is fine and dandy. It it
	// doesn't exist, remove the orphaned thread record. 
	
	*repairsDone = 0;
	
	result = GetCatalogNode(volume, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint);
	M_ExitOnError(result);
	
	// visit all the leaf node data records in the catalog
	while ( --maxRecords > 0 )  // starts out at recordsFound value passed in
	{
		tempResult = GetBTreeRecord(volume->catalogRefNum, kBTreeNext, &key, &record,
								 &recordSize, &hint );
		if ( tempResult )
		{
			//	DebugStr("\p FixOrphanFileThreads: did we run out of records?");
			break;
		}
		
		// if this is a file thread record, check it out...
		if (record.recordType == kHFSFileThreadRecord)
		{
			CatalogKey	currentKey;
			CatalogKey	fileKey;
			UInt32		fileHint;
				
			// save our current position
			currentKey = key;	
			
			// create a key for the file record
			fileHint = 0;
			fileKey.hfs.parentID = record.hfsThread.parentID;
			BlockMoveData (	record.hfsThread.nodeName, fileKey.hfs.nodeName,
							record.hfsThread.nodeName[0] + 1);
			
			// try and get the file record for this thread record...
			tempResult = SearchBTreeRecord ( volume->catalogRefNum, &fileKey, fileHint, &key, &record, &recordSize, &fileHint );
				
			if (tempResult == noErr)
			{		
				if ((record.hfsFile.flags & kHFSThreadExistsMask) == 0)
				{
					// thread record exists but file record doesn't think so....set it
					record.hfsFile.flags |= kHFSThreadExistsMask;
					
					// update the file record on disk
					if ( ReplaceBTreeRecord(volume->catalogRefNum, &key, fileHint, &record, recordSize, &fileHint) == noErr )		
						++(*repairsDone);
				}
			}		
			else if (tempResult == btNotFound)
			{
				// BTDelete invalidates the current node mark so before we delete the thread record,
				// point currentKey and hint at something valid (ie the previous record)
				
				(void) SearchBTreeRecord( volume->catalogRefNum, &currentKey, hint, &key, &record, &recordSize, &hint );
				(void) GetBTreeRecord( volume->catalogRefNum, kBTreePrevious, &key, &record, &recordSize, &hint );

				// file record doesn't exist. Delete the thread record.
				if ( DeleteBTreeRecord( volume->catalogRefNum, &currentKey) == noErr )
					++(*repairsDone);  
				
				currentKey = key;	// now point to the record before the thread record
			}

			// now we need to reset the current node mark for GetBTreeRecord iteration...
			(void) SearchBTreeRecord( volume->catalogRefNum, &currentKey, hint, &key, &record, &recordSize, &hint );

		} // end if thread
	
	} // end while
	
	return( result );
ErrorExit:
	return ( result ); //XXX which error should it be ???
} // end FixOrphanFileThreads
#endif /* TARGET_OS_MAC */

//_______________________________________________________________________
//
//	GetTempBuffer
//	
//	This routine allocates a temporary buffer.
//
//	Note: At startup TempNewHandle may not be available.
//_______________________________________________________________________
#if TARGET_OS_MAC
static Handle GetTempBuffer (Size size)
{
	OSErr	error;
	Handle	tempBuffer;

	if ( GetExpandMemProcessMgrExists() )
		tempBuffer = TempNewHandle(size, &error);
	else
		tempBuffer = NewHandleSys(size);
	
	return tempBuffer;
}
#endif 	/* TARGET_OS_MAC */



static UInt32
CountExtentBlocks(HFSPlusExtentRecord extents, Boolean hfsPlus)
{
	UInt32	blocks;
	
	// grab the first 3
	blocks = extents[0].blockCount + extents[1].blockCount + extents[2].blockCount;

	if (hfsPlus)
	{
		UInt32	i;
		
		for (i = 3; i < kHFSPlusExtentDensity; ++i)
			blocks += extents[i].blockCount;		// HFS Plus has additional extents
	}

	return blocks;
}


static void
ConvertToHFSPlusExtent( const HFSExtentRecord oldExtents, HFSPlusExtentRecord newExtents)
{
	UInt16	i;

	// go backwards so we can convert in place!
	
	for (i = kHFSPlusExtentDensity-1; i > 2; --i)
	{
		newExtents[i].blockCount = 0;
		newExtents[i].startBlock = 0;
	}

	newExtents[2].blockCount = oldExtents[2].blockCount;
	newExtents[2].startBlock = oldExtents[2].startBlock;
	newExtents[1].blockCount = oldExtents[1].blockCount;
	newExtents[1].startBlock = oldExtents[1].startBlock;
	newExtents[0].blockCount = oldExtents[0].blockCount;
	newExtents[0].startBlock = oldExtents[0].startBlock;
}


static OSErr
GetFileExtentRecord( const ExtendedVCB	*vcb, const FCB *fcb, HFSPlusExtentRecord extents)
{
	OSErr				err;
	ExtendedFCB			*extendedFCB;

	err = noErr;
	
	if (vcb->vcbSigWord == kHFSSigWord)
	{
		ConvertToHFSPlusExtent(fcb->fcbExtRec, extents);
	}
	else
	{
/* XXX PPD: Is there any reason this isn't the same for EVERY platform? */
#if TARGET_OS_MAC
		extendedFCB = ParallelFCBFromRefnum( GetFileRefNumFromFCB(fcb) );
#else
		extendedFCB = GetParallelFCB( GetFileRefNumFromFCB(fcb) );
#endif

		if (extendedFCB == NULL)
		{
			if ( DEBUG_BUILD )
				DebugStr("\pFATAL: Extended FCB not found!");
			err = fsDSIntErr;
		}
		else
		{
			BlockMoveData(extendedFCB->extents, extents, sizeof(HFSPlusExtentRecord));
		}
	}
	
	return err;
}



//_______________________________________________________________________
//
//	CheckCreateDate
//	
//	For HFS Plus volumes, make sure the createDate in the VolumeHeader
//	is the same as in the MDB.  If not, just fix it.
//
//_______________________________________________________________________

static OSErr CheckCreateDate (ExtendedVCB* volume, UInt32* consistencyStatus )
{
	OSErr					err;
	HFSMasterDirectoryBlock	*mdb;
	UInt32					createDate;

	return noErr;
	
	//
	//	Read in the MDB
	//
#if TARGET_OS_MAC
        err = GetBlock_glue(gbReleaseMask, 2, (Ptr *) &mdb, volume->vcbVRefNum, volume);
#else
        err = GetBlock_glue(gbReleaseMask, 2, (Ptr *) &mdb, kNoFileReference, volume);
#endif
	if (err != noErr) return err;
	
	//
	//	Make sure it really is a wrappered volume with an MDB
	//
	if (mdb->drSigWord != kHFSSigWord || mdb->drEmbedSigWord != kHFSPlusSigWord)
		return noErr;			// must not be a wrapper, so nothing to fix
	
	createDate = mdb->drCrDate;
	
	if (volume->vcbCrDate != createDate) {
		volume->vcbCrDate = createDate;					// fix create date in the VCB
		volume->vcbFlags |= kVCBFlagsVolumeDirty;		// make it dirty so change will be written to volume header
		*consistencyStatus |= kHFSMinorRepairsWereMade;	// the problem is now fixed
	}
	
	return noErr;
}
