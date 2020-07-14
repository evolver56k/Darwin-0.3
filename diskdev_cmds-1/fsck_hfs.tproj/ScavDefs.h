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
	File:		ScavDefs.h

	Contains:	Data structure definitions for the high level scavenging routines.

	Version:	xxx put version here xxx

	Written by:	Bill Bruffey

	Copyright:	© 1985-1994, 1996-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	 <HFS16>	 12/8/97	DSH		Add lastTickCount
	 <HFS15>	 12/2/97	DSH		Add maskBit to RepairOrder, usersAreConnected, VeryMinor_Repair,
									more messages.
	 <HFS14>	11/18/97	DSH		kBlockCameFromDisk selector, Mt Check messages
	 <HFS13>	 11/4/97	DSH		Add kDFAStage
	 <HFS12>	10/30/97	DSH		UseDFALowMems is now a bit in ApplScratch
	 <HFS11>	10/21/97	DSH		Added M_RebuildingBTree
	 <HFS10>	 10/6/97	DSH		RepairBtree
	  <HFS9>	 9/17/97	DSH		Wrapperless HFS+ volume support.
	  <HFS8>	  9/5/97	DSH		Added E_VolumeHeaderDamaged, Volume Header Damaged
	  <HFS7>	  9/4/97	msd		Add attributes structures to SGlob.
	  <HFS6>	  9/2/97	DSH		Adding new global idSector containing the location of the alt
									MDB or VH.
	  <HFS5>	 8/18/97	DSH		Added some more error codes
	  <HFS4>	 6/26/97	DSH		Eliminated Unicode conversion dependency, changed fields to
									CatalogName.
	  <HFS3>	  4/4/97	DSH		Adding parameters for MessageProcs
	  <HFS2>	 3/27/97	DSH		64Bit changes for CheckDisk
	  <HFS1>	 1/28/97	DSH		first checked in
		 <6>	 8/30/96	djb		Added progress info for CheckDisk API.
		 <6>	 8/29/96	djb		Get in sync with Master Interfaces.

		 <5>	 9/16/94	djb		Fixing Radar bug #1166262 - added S_BadCustomIcon and E_MissingCustomIcon.

		 <9>	  4/7/92	gs		Add Unrepairable to Repair Levels, change No_Repair to
									No_RepairNeeded
		 <8>	 3/31/92	PP		For missing directory record error, define S_NoDir catalog file
									flag.
		 <7>	 3/23/92	gs		Added defines for Repair Levels to ScavCtrl Interface.
		 <6>	 3/19/92	PP		Add #ifndef for include files. Add Catalog file status for DF
									bug and an Error code for DF bug.

				15 Jan 91 	KST		Removed E_NoFThdFlg error.
				10 Jul 90 	PK		Added Repair Order concept, and S_FThd.
				 2 Jul 90 	PK		Renumbered and cleaned up error#s, to track STR# 1202.
				27 Jun 90 	PK		Added support for valence repair.
				16 Apr 90 	KST		Changed E_NoFile to positive number so that it will be recoverable.
				 6 Jul 89 	EN		Added -548, -549 for FileIDs support for BigBang.
				15 Apr 86 	BB 		New today.

*/

#ifndef __SCAVDEFS__
#define __SCAVDEFS__

#if TARGET_OS_MAC
#include <Types.h>
#include <Memory.h>
#include <Files.h>
#include <OSUtils.h>
#include <Devices.h>
#include <Finder.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */
#include "HFSDefs.h"

#if ( StandAloneEngine )
#include "CheckDisk.h"
#else
enum {
	kStatusMessage			= 0x0000,
	kTitleMessage			= 0x0001,
	kErrorMessage			= 0x0002
};
#endif

enum {
	kHighLevelInfo	= 1100,
	kBasicInfo		= 1200,
	kErrorInfo		= 1202
};


/*------------------------------------------------------------------------------
 Minor Repair Interface (records compiled during scavenge, later repaired)
 Note that not all repair types use all of these fields.
 -----------------------------------------------------------------------------*/
 
 typedef struct	RepairOrder	{		/* a node describing a needed minor repair */
 	struct RepairOrder	*link;		/* link to next node, or NULL */
	SInt16			type;			/* type of error, as an error code (E_DirVal etc) */
	SInt16			n;				/* temp */
	UInt32			correct;		/* correct valence */
	UInt32			incorrect;		/* valence as found in volume (for consistency chk) */
	UInt32			maskBit;		//	incorrect bit
	UInt32			parid;			/* parent ID */
	unsigned char	name[1];		/* name of dirrectory, if needed */
 } RepairOrder, *RepairOrderPtr;



/*------------------------------------------------------------------------------
 Scavenger Global Area - (SGlob) 
------------------------------------------------------------------------------*/
typedef struct BitMapRec
{
	UInt32			count;						//	count of set bits in buffer
	Boolean			processed;					//	has this buffer been processed
	Boolean			isAMatch;					//	did it match the on disk equivalent
} BitMapRec;

typedef struct VolumeBitMapHeader
{
	UInt32			numberOfBuffers;			//	how many buffers do we need to read the bitmap
	UInt32			bitMapSizeInBytes;
	SInt32			bufferSize;
	SInt32			buffersProcessed;			//	how many buffers we've processed so far
	SInt32			currentBuffer;				//	Which BitMap buffer is currently in memory
	Ptr				buffer;
	BitMapRec		bitMapRecord[1];			//	bit counts of all the processed buffers
} VolumeBitMapHeader;



typedef struct SGlob {
	SInt16			DrvNum;			/* drive number of target drive */
	SInt16			RepLevel;		/* repair level, 1 = minor repair, 2 = major repair */
	SInt16			ScavRes;		/* scavenge result code */
	OSErr			ErrCode;    	/* error code */
	OSErr			IntErr;     	/* internal error code */
	SInt16			UserCmd;		/* last user command from CheckPause */
	SInt16			StatMsg;		/* current status message id */
	UInt16			VIStat;			/* scavenge status flags for volume info  */
	UInt16			ABTStat;		/* scavenge status flags for Attributes BTree  */
	UInt16			EBTStat;		/* scavenge status flags for extent BTree  */
	UInt16			CBTStat;		/* scavenge status flags for catalog BTree  */
	UInt16			CatStat;		/* scavenge status flags for catalog file */
	UInt16			FilStat;		/* scavenge status flags for user files */
	DrvQEl			*DrvPtr;		/* pointer to driveQ element for target drive */
	UInt32			idSector;		/* location of id block alt MDB or VH */
	UInt32			TarID;			/* target ID (CNID of data structure being verified) */
	UInt32			TarBlock;		/* target block/node number being verified */
	SInt16			BTLevel;		/* current BTree enumeration level */
	SBTPT			*BTPTPtr;		/* BTree path table pointer */
	SInt16			DirLevel;		/* current directory enumeration level */
	SDPT			*DirPTPtr;		/* directory path table pointer */
	SInt16			CNType;			/* current CNode type */
	UInt32			ParID;				/* current parent DirID */
	CatalogName		CName;				/* current CName */
	RepairOrderPtr	MinorRepairsP;		/* ptr to list of problems for later repair */
	ExtendedVCB		*realVCB;			/* ptr to real VCB, or NULL if not mounted */
	UInt32			wrCnt;				/* if mounted, initial vcbWrCnt */
	Boolean			usersAreConnected;	//	true if user are connected
	Boolean			fileSharingOn;		/* true if file sharing is on */
	Ptr 			FCBAPtr;			/* pointer to scavenger FCB array */
	UInt32			**validFilesList;	//	List of valid HFS file IDs

	Ptr				hfsStackTop;		/* save and restore when calling the cache */

#if ( StandAloneEngine )
	UserCancelUPP	userCancelProc;
	UserMessageUPP	userMessageProc;
	void			*userContext;

	UInt64			itemsToProcess;
	UInt64			itemsProcessed;
	UInt64			lastProgress;
#endif

	long				lastTickCount;
	UInt32				savedApplScratch[3];

	ParallelFCB			*fcbPBuf;			//	Parallel FCB array
	VolumeBitMapHeader	*volumeBitMapPtr;
	OSErr				volumeErrorCode;
	
	ExtendedVCB			*calculatedVCB;
	
	FCB					*calculatedExtentsFCB;
	FCB					*calculatedCatalogFCB;
	FCB					*calculatedAllocationsFCB;
	FCB					*calculatedAttributesFCB;
	FCB					*calculatedRepairFCB;
	
	ExtendedFCB			*extendedExtentsFCB;
	ExtendedFCB			*extendedCatalogFCB;
	ExtendedFCB			*extendedAllocationsFCB;
	ExtendedFCB			*extendedAttributesFCB;
	ExtendedFCB			*extendedRepairFCB;
	
	BTreeControlBlock	*calculatedExtentsBTCB;
	BTreeControlBlock	*calculatedCatalogBTCB;
	BTreeControlBlock	*calculatedRepairBTCB;
	BTreeControlBlock	*calculatedAttributesBTCB;

	UInt32				altBlockLocation;
	Boolean				checkingWrapper;
	Boolean				isHFSPlus;
	Boolean				pureHFSPlusVolume;
	SInt16				numExtents;

} SGlob, *SGlobPtr;
	


enum										//	Application Scratch Indexes
{
	kHFSStackTopScratchIndex		= 0,
	kDFALowMemIndex					= 1,
	kDFAFlagsIndex					= 2,
	
	kUseDFALowMemsMask				= 0x0001,

	kDFAGlobals						= 0,
	kCacheGlobals,
	kDFAStage,
	kBlockCameFromDisk,
	kLMVCBQHdr,
	kLMDrvQHdr,
	kLMFCBSPtr,
	kLMHFSFlags,
	kLMFSFCBLen,
	kLMDefVCBPtr,
	kLMFSMVars,
	kLMNewMount,
	kLMFlushOnly,
	kLMWDCBsPtr,
	kLMSReqstVol,
	kLMSysVolCPtr,
	kLMDefVRefNum,
	
	kNumLowMemsToOverride		//	17
};

/* scavenger flags */	
	
/* volume info status flags (contents of VIStat) */

#define	S_MDB					0x8000	/* MDB damaged */
#define	S_AltMDB				0x4000	//	Unused	/* alternate MDB damaged */
#define	S_VBM					0x2000	/* volume bit map damaged */
#define	S_MountCheckMinorErrors	0x1000	//	MountCheck reported fixable errors

/* BTree status flags (contents of EBTStat and CBTStat) */

#define	S_BTH					0x8000	/* BTree header damaged */
#define	S_BTM					0x4000	/* BTree map damaged */
#define	S_Indx					0x2000	//	Unused	/* index structure damaged */
#define	S_Leaf					0x1000	//	Unused	/* leaf structure damaged */
#define S_Orphan				0x0800  // orphaned file
#define S_OrphanedExtent		0x0400  // orphaned extent
#define S_ReservedNotZero		0x0200  // the flags or reserved fields are not zero
#define S_RebuildBTree			0x0100  // similar to S_Indx, S_Leaf, but if one is bad we stop checking and the other may also be bad.

/* catalog file status flags (contents of CatStat) */

#define	S_RName					0x8000	//	Unused	/* root CName not equal to volume name */
#define	S_Valence				0x4000	/* a directory valence is out of sync */
#define	S_FThd					0x2000	/* dangling file thread records exist */
#define	S_DFCorruption			0x1000	/* disappearing folder corruption detected */
#define	S_NoDir					0x0800	/* missing directory record */
#define S_LockedDirName			0x0400  // locked dir name
#define S_BadCustomIcon			0x0200	// custom icon is missing

/* user file status flags (contents of FilStat) */

#define S_LockedName			0x4000  // locked file name

/*------------------------------------------------------------------------------
 ScavCtrl Interface
------------------------------------------------------------------------------*/

/* Command Codes (commands to ScavControl) */

#define	Op_IVChk	1	/* start initial volume check */
#define	Op_Verify	2	/* start verify operation */
#define Op_Repair	3	/* start repair opeation */
#define	Op_Term		4	/* cleanup after scavenge */


/* Repair Levels */

#define Unrepairable		-1
#define	No_RepairNeeded		0
#define	Minor_Repair		1
#define	Major_Repair		2
#define	VeryMinor_Repair	3		//	CheckDisk returns noErr if volume cannot be unmounted and  only very minor errors exist


/* Response Codes */

#define	Rsp_End		1		/* end of operation */
#define	Rsp_UInt	2		/* user interrupt */

#define No_Cmd		0		/* no command from user */



/*------------------------------------------------------------------------------
 Status Messages (appear in panel window)

------------------------------------------------------------------------------*/

#ifndef OBSOLETE
#define	M_Finish	2			/* "Scavenging successful" */
#define	M_Abort		3			/* "Scavenging stopped" */
#define	M_Initial	6			/* "Checking disk volume" */
#define	M_Verify	7			/* "verifying volume" */
#endif
// moved M_Repair down to messages written to summary




/*------------------------------------------------------------------------------
Messages written to summary (resource ID = 1200)
------------------------------------------------------------------------------*/

#define	M_IVChk						1			//	"Checking disk volume."
#define	M_ExtBTChk					2			//	"Checking extent file BTree."
#define	M_ExtFlChk					3			//	"Checking extent file."
#define	M_CatBTChk					4			//	"Checking catalog BTree."
#define	M_CatFlChk					5			//	"Checking catalog file."
#define	M_CatHChk					6			//	"Checking catalog hierarchy."
#define	M_VInfoChk					7			//	"Checking volume information."
#define M_Missing					8			//	"Checking for missing folders."
#define	M_Repair					9			//	"repairing volume"
#define M_LockedVol 				10			//	"Checking for locked volume name"
#define M_Orphaned					11			//	"Checking for orphaned extents"
#define M_DTDBCheck					12			//	"Checking desktop database."
#define M_VolumeBitMapChk			13			//	"Checking volume bit map."
#define M_CheckingHFSVolume			14			//	"Checking "HFS" volume structures."
#define M_CheckingHFSPlusVolume		15			//	"Checking "HFS Plus" volume structures."
#define	M_AttrBTChk					16			//	"Checking attributes BTree."
#define	M_RebuildingExtentsBTree	17			//	"Rebuilding Extents BTree."
#define	M_RebuildingCatalogBTree	18			//	"Rebuilding Catalog BTree."
#define	M_RebuildingAttributesBTree	19			//	"Rebuilding Attributes BTree."
#define	M_MountCheckMajorError		20			//	"MountCheck found serious errors."
#define	M_MountCheckMinorError		21			//	"MountCheck found minor errors."

/*------------------------------------------------------------------------------
 Scavenger Result/Error Codes
------------------------------------------------------------------------------*/

/* scavenge result codes (reported to user) */

enum {
	R_NoMem				= 1,	/* not enough memory to do scavenge */
	R_IntErr			= 2,	/* internal Scavenger error */
	R_NoVol				= 3,	/* no volume in drive */
	R_RdErr				= 4,	/* unable to read from disk */
	R_WrErr				= 5,	/* unable to write to disk */
	R_BadSig			= 6,	/* not HFS signature */
	R_VFail				= 7,	/* verify failed */
	R_RFail				= 8,	/* repair failed */
	R_UInt				= 9,	/* user interrupt */
	R_Modified			= 10,	/* volume modifed by another app */
	R_BadVolumeHeader	= 11,	/* Invalid VolumeHeader */
	R_FileSharingIsON	= 12,	//	File Sharing is on, and may have interrupted the verification.

	Max_RCode			= 12	/* maximum result code */
};

/* Scavenger errors.  If negative, they are unrecoverable (scavenging terminates).
   If positive, they are recoverable (scavenging continues).  Note that the STR#
   resource 1202 (in DFA_text.r) tracks these errors, so change it when you add
   new errors or redefine existing ones.  */
	
enum {
	E_PEOF					=  500,	/* invalid PEOF */
	E_LEOF					=  501,	/* invalid LEOF */
	E_DirVal				=  502,	/* invalid directory valence */
	E_CName					=  503,	/* invalid CName */
	E_NHeight				=  504,	/* invalid node height */
	E_NoFile				=  505,	/* missing file record for file thread */
	E_ABlkSz				= -506,	/* invalid allocation block size */
	E_NABlks				= -507,	/* invalid number of allocation blocks */
	E_VBMSt					= -508,	/* invalid VBM start block */
	E_ABlkSt				= -509,	/* invalid allocation block start */
	E_ExtEnt				= -510,	/* invalid extent entry */
	E_OvlExt				= -511,	/* overlapped extent allocation */
	E_LenBTH				= -512,	/* invalid BTH length */
	E_ShortBTM				= -513,	/* BT map too short to repair */
	E_BTRoot				= -514,	/* invalid root node number */
	E_NType					= -515,	/* invalid node type */
	E_NRecs					= -517,	/* invalid record count */
	E_IKey					= -518,	/* invalid index key */
	E_IndxLk				= -519,	/* invalid index link */
	E_SibLk					= -520,	/* invalid sibling link */
	E_BadNode				= -521,	/* invalid node structure */
	E_OvlNode				= -522,	/* overlapped node allocation */
	E_MapLk					= -523,	/* invalid map node linkage */
	E_KeyLen				= -524,	/* invalid key length */
	E_KeyOrd				= -525,	/* Keys out of order */
	E_BadMapN				= -526,	/* invalid map node */
	E_BadHdrN				= -527,	/* invalid header node */
	E_BTDepth				= -528,	/* exceeded maximum BTree depth */

	E_CatRec				= -530,	/* invalid catalog record type */
	E_LenDir				= -531,	/* invalid directory record length */
	E_LenThd				= -532,	/* invalid thread record length */
	E_LenFil				= -533,	/* invalid file record length */
	E_NoRtThd				= -534,	/* missing thread record for root directory */
	E_NoThd					= -535,	/* missing thread record */
	E_NoDir					= -536,	/* missing directory record */
	E_ThdKey				= -537,	/* invalid key for thread record */
	E_ThdCN					= -538,	/* invalid  parent CName in thread record */
	E_LenCDR				= -539,	/* invalid catalog record length */
	E_DirLoop				= -540,	/* loop in directory hierarchy */
	E_RtDirCnt				=  541,	/* invalid root directory count */
	E_RtFilCnt				=  542,	/* invalid root file count */
	E_DirCnt				=  543,	/* invalid volume directory count */
	E_FilCnt				=  544,	/* invalid volume file count */
	E_CatPEOF				= -545,	/* invalid catalog PEOF */
	E_ExtPEOF				= -546,	/* invalid extent file PEOF */
	E_CatDepth				= -547,	/* exceeded maximum catalog depth */

	E_NoFThdFlg				= -549,	/* file thread flag not set in file record */
	E_DFCorruption			= -550,	/* disappearing folder corruption */

	E_BadFileName			= -551,	// invalid file/folder name problem
	E_InvalidClumpSize		=  552,	// bad file clump size
	E_InvalidBTreeHeader	= -553,	// invalid btree header
	E_LockedDirName			=  554,	//	inappropriate locked folder name
	E_EntryNotFound			= -555,	// volume catalog entry not found
	E_MissingCustomIcon		=  556,	//	custom icon bit set but no icon file

	E_MDBDamaged			=  557,	//	MDB Damaged
	E_VolumeHeaderDamaged	=  558,	//	Volume Header Damaged
	E_VBMDamaged			=  559,	//	Volume Bit Map needs minor repair
	
	E_InvalidNodeSize		= -560,	//	Bad BTree node size
	E_BadRecordType			=  561,	//	HFS read a non HFS catalog record
	E_UninitializedBuffer	=  562,
	
	//	Merge tests from ValidHFSRecord here
	E_CatalogFlagsNotZero	=  563,
	E_InvalidID				=  564,
	E_VolumeHeaderTooNew	=  565,

	E_DiskFull				=  566,

};


//	Internal DFA error codes
enum {
	errRebuildBtree				= -1001,	//	BTree requires rebuilding.
};



#endif


