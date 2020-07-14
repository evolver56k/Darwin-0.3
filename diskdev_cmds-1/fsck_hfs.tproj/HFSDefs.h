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
/*------------------------------------------------------------------------------
	File:		HFSDefs.h

	Contains:	HFS data structure definitions.

	Version:	xxx put version here xxx

	Written by:	Bill Bruffey

	Copyright:	© 1986-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn

	Change History (most recent first):

	 <HFS10>	 12/8/97	DSH		Use ZeroBlocks
	  <HFS9>	 12/2/97	DSH		Defined NewPtrSys to be NewPtr so we don't have to
									conditionalize the code in the FileManager for DFA compiles.
	  <HFS8>	 10/6/97	DSH		kCalculatedRepairRefNum
	  <HFS7>	  9/4/97	msd		Add define for kCalculatedAttributesRefNum.
	  <HFS6>	 7/17/97	DSH		FilesInternal.i renamed to FileMgrInternal.i to avoid name
									collision.
		 <5>	 5/21/97	DSH		UsersAreConnected
		 <4>	 5/20/97	DSH		Added kBusErrorValue
	  <HFS3>	 4/25/97	DSH		Changed directoryName to be of type CatalogName.
	  <HFS2>	 3/27/97	DSH		Adding realFreeNodeCount to our BTreeExtensionsRec, for
									more accurate progress calculation.
	  <HFS1>	 3/17/97	DSH		first checked in
		 <9>	  9/4/96	djb		Fix Volume Attributes mask so that all upper 8 bits are valid.
		 <8>	 8/30/96	djb		Increased number of cache buffers to 64.
		 <7>	 2/14/95	djb		Added DCE bit definitions.
		 <5>	 2/1/94		DJB		Updated the Max_ABSiz constant for large volume support.

		 <7>	 3/19/92	PP		Add #ifndef for inclusion.
				02/21/91	ewa/BB	Change the max levels of directories to 100 to match Finder limit
		 <4>	 8/24/90	PK		
				 8/21/90	PK		MDB's DrAllocPtr changed 'short' to 'unsigned short'
				 7/17/90	PK		Added BTCBMSize.
		 <2>	 6/16/90	PK		cleaning up
		 <1>	 6/14/90	PK		first checked in
				 29 May 90 	PK		Added BB_FNum and VAtrb_BB for sparing
 				 6 Jul 89 	EN		Added cdrFThdRec for FileIDs support for BigBang. (changed comments too)
 				10 May 86 	BB 		New today.
				
				To Do:
 ------------------------------------------------------------------------------*/


#ifndef __HFSDEFS__
#define __HFSDEFS__

#include	<HFSVolumesPriv.h>
#include	<FileMgrInternal.h>
#include	"BTreesInternal.h"
#include	"BTreesPrivate.h"

//
//	Override the file systems System heap pointer creation with the
//	equivalent application heap creation routines.
//
#define NewPtrSys( byteCount )	( NewPtr( byteCount ) )
#define NewPtrSysClear( byteCount )	( NewPtrClear( byteCount ) )

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
	Log2BlkLo				= 9,					// number of left shifts to convert bytes to block.lo
	Log2BlkHi				= 23					// number of right shifts to convert bytes to block.hi
};

//
// Misc constants
//

#define	kBusErrorValue	0x50FF8001


//€€ Danger! This should not be hard coded
#define	kMaxClumpSize	0x100000	/* max clump size is 1MB (2048 btree nodes) */
#define	kMac_FSID	0				/* FSID of MFS/HFS */

#define	kMaxHFSVolumeNameLength	27

#define	MDB_FNum	1				/* file number representing the MDB */
#define	AMDB_FNum	-1				/* file number representing the alternate MDB */
#define	VBM_FNum	2				/* file number representing the volume bit map */
#define	MDB_BlkN	2				/* logical block number for the MDB */

#define	Vol_RefN	(-1)			/* refnum of volume being scavenged */
#define	kCalculatedExtentRefNum			0x0002			/* extent file refnum */
#define	kCalculatedCatalogRefNum		0x0060
#define	kCalculatedAllocationsRefNum	0x00BE
#define kCalculatedAttributesRefNum		0x011C
#define kCalculatedRepairRefNum			0x017A

#define	Max_ABSiz	0x7FFFFE00		/* max allocation block size (multiple of 512 */
#define	Blk_Size	512				/* size of a logical block */
#define	Num_CBufs	64				/* number of cache buffers */
#define	CBuf_Size	512				/* cache buffer size */

// only the lower 7 bits are considered to be invalid, all others are valid -djb
#define	VAtrb_Msk	0x007F			/* volume attribute mask - invalid bits */
#define	VAtrb_DFlt	0x0100			/* default volume attribute flags */
#define	VAtrb_Cons	0x0100			/* volume consistency flag */


/*------------------------------------------------------------------------------
 BTree data structures
------------------------------------------------------------------------------*/

/* misc BTree constants */

#define	BTMaxDepth	8				/* max tree depth */
#define	Num_HRecs	3				/* number of records in BTree Header node */
#define	Num_MRecs	1				/* number of records in BTree Map node */



//	DFA extensions to the HFS/HFS+ BTreeControlBlock
typedef struct BTreeExtensionsRec
{
	Ptr 				BTCBMPtr;			//	pointer to scavenger BTree bit map
	UInt32				BTCBMSize;			//	size of the bitmap, bytes
	BTreeControlBlock	*altBTCB;			//	BTCB DFA builds up
	UInt32				realFreeNodeCount;	//	Number of real free nodes, taken from disk, for more accurate progress information
} BTreeExtensionsRec;


	
/*
 * Scavenger BTree Path Record (STPR)
 */
typedef struct STPR {
	UInt32			TPRNodeN;		/* node number */
	SInt16			TPRRIndx;		/* record index */
	SInt16			unused;			/* not used - makes debugging easier */
	UInt32			TPRLtSib;		/* node number of left sibling node */
	UInt32			TPRRtSib;		/* node number of right sibling node */
	} STPR, *STPRPtr;
	
typedef	STPR SBTPT[BTMaxDepth]; 		/* BTree path table */
	
#define	LenSBTPT	( sizeof(STPR) * BTMaxDepth )	/* length of BTree Path Table */




/*------------------------------------------------------------------------------
 CM (Catalog Manager) data structures
 ------------------------------------------------------------------------------*/

//
//	Misc constants
//
#define CMMaxDepth	100				/* max catalog depth (Same as Finder 7.0) */

#define fNameLocked 4096


	
//
//	Scavenger Directory Path Record (SDPR)
//
typedef struct SDPR {
	UInt32			directoryID;		//	directory ID
	UInt32			offspringIndex;		//	offspring index
	UInt32			directoryHint;		//	BTree hint for directory record
	long			threadHint;			//	BTree hint for thread record
	CatalogNodeID	parentDirID;		//	parent directory ID
	CatalogName		directoryName;		//	directory CName
	} SDPR;
	
typedef	SDPR SDPT[CMMaxDepth]; 			//	directory path table
	
#define	LenSDPT	( sizeof(SDPR) * CMMaxDepth )	//	length of Tree Path Table







/*------------------------------------------------------------------------------
Master Directory Block (MDB) on disk 
------------------------------------------------------------------------------*/
#define	LenMFSMDB	64				/* length of VCB data from MDB (MFS) */
#define	LenHFSMDB	66				/* length of additional VCB data from MDB (HFS) */



/*------------------------------------------------------------------------------
 Low-level File System Error codes 
------------------------------------------------------------------------------*/

/* The DCE bits are defined as follows (for the word of flags): */

enum
{
	Is_AppleTalk		= 0,
	Is_Agent			= 1,			// future use
	FollowsNewRules		= 2,			// New DRVR Rules Bit
	Is_Open				= 5,
	Is_Ram_Based		= 6,
	Is_Active			= 7,
	Read_Enable			= 8,
	Write_Enable		= 9,
	Control_Enable		= 10,
	Status_Enable		= 11,
	Needs_Goodbye		= 12,
	Needs_Time			= 13,
	Needs_Lock			= 14,

	Is_AppleTalk_Mask	= 1 << Is_AppleTalk,
	Is_Agent_Mask		= 1 << Is_Agent,
	FollowsRules_Mask	= 1 << FollowsNewRules,
	Is_Open_Mask		= 1 << Is_Open,
	Is_Ram_Based_Mask	= 1 << Is_Ram_Based,
	Is_Active_Mask		= 1 << Is_Active,
	Read_Enable_Mask	= 1 << Read_Enable,
	Write_Enable_Mask	= 1 << Write_Enable,
	Control_Enable_Mask	= 1 << Control_Enable,
	Status_Enable_Mask	= 1 << Status_Enable,
	Needs_Goodbye_Mask	= 1 << Needs_Goodbye,
	Needs_Time_Mask		= 1 << Needs_Time,
	Needs_Lock_Mask		= 1 << Needs_Lock
};

#if !GENERATINGCFM
#pragma parameter __D0 CallA094(__D0, __A0)
#endif // !GENERATINGCFM

static OSErr CallA094(long selector, void* pb)
 ONEWORDINLINE(0xA094);

#endif
