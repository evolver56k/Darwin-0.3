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
	File:		SControl.c

	Contains:	This file contains the routines which control the scavenging operations.

	Version:	xxx put version here xxx

	Written by:	Bill Bruffey

	Copyright:	© 1985, 1986, 1992-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	 <HFS19>	 12/8/97	DSH		CheckForStop to be keyed off TickCount
	 <HFS18>	 12/2/97	DSH		usersAreConnected, and 2200106
	 <HFS17>	11/18/97	DSH		CustomIconCheck has been rolled into CatFlChk
	 <HFS16>	 11/4/97	DSH		Add gAttributesBuffer
	 <HFS15>	10/30/97	DSH		UseDFALowMems is now a bit in ApplScratch
	 <HFS14>	10/21/97	DSH		Put Checking Attributes message in actual routine.
	 <HFS13>	 10/6/97	DSH		Add ABTStat
	 <HFS12>	 9/10/97	DSH		1609836, Check pointers before disposing
	 <HFS11>	  9/4/97	msd		Set up attributes B-tree.
	 <HFS10>	 7/28/97	DSH		VCB uses MRUCache
	  <HFS9>	  7/7/97	DSH		No longer have obsoleteVCBXTRef and obsoleteVCBCTRef fields.
	  <HFS8>	 5/21/97	DSH		Set checkedDate for HFS+ volumes only
		 <7>	 5/21/97	DSH		Set checkedDate on successful verifies, and check if users are
									connected instead of checking if filesharing is on.
		 <6>	 5/20/97	DSH		kFileSystemStackSize
	  <HFS5>	 5/19/97	DSH		Moved Sharing check, and locked check to top of routine.
	  <HFS4>	 4/11/97	DSH		Use extended VCB fields catalogRefNum, and extentsRefNum.
	  <HFS3>	  4/4/97	DSH		Adding parameters for MessageProcs
	 <HFS2>		 3/27/97	DSH		Changes for CheckDisk
	 <HFS1>		 1/28/97	DSH		Using HFS+ defined types
		 <5>	 8/30/96	djb		Added progress data for CheckDisk.
		<4>	 8/28/96	djb		Get in sync with SCpp and Master Interfaces.
		<3>	 8/23/96	djb		Make sure driveQ element pointer is initialized!
		<2>	 2/14/95	djb		Use NewPtrClear instead of clearing memory by hand! Also changed
									DisposPtr to DisposePtr.
		<1>	 9/16/94	DJB		Fixing Radar bug #1166262 - added call to CustomIconCheck in ScavCtrl.

			 5/29/93	hhs		Minor changes for the new HIF. Added an info area message for
									DF problems.  Changed the callbacks to the HIF from pascal to
									c formats.
		 <9>	 5/29/92	gs		with PP. Set RepLevel for non-Repair Order problems in ScavCtl.
		 <8>	 4/23/92	gs		Clean up change history.
		 <7>	 4/23/92	gs		Move WriteMsg(M_Term) from ScavCntl to DoScav in SMain.p to
									prevent premature notification of scavenge termination.
		 <6>	  4/8/92	gs		Use defined constants for RepLevel.
		 <5>	  4/7/92	PP		If the verify result is negative, set the repair level to
									unrepairable.
		 <4>	  4/7/92	gs		Make changes to use major and minor repair queues.
		 <3>	  4/7/92	PP		Integrate with changes in main loop code so that things trigger
									via repair level.
		 <2>	 3/31/92	PP		After checking for DF corruption, make sure that the repair is
									triggered.
		 <1>	 3/19/92	PP		Check for Disappearing folder corruption after doing other
									verification.

				Aug 22 90	PK		Flush volume (if mounted) just before saving vcbWrCnt.
				Jul 11 90	PK		Check vcbWrCnt in CheckForSTop, to detect modification of
									volume behind our back.  Remove use of response codes in
									ScavCtrl, which carried no information not also in the error
									code.
				Jul  4 90	PK		Removed pause/resume functionality, which is no longer used
									due to incompatibility w multifinder.
				Jun 28 90	PK		Converted to use prototypes.
				Jun 27 90	PK		Handle cleanup of valence error list.
				Apr 21 89	jb		Fixed up pointer assignments; MPW 3.0 type checking
				Apr 15 86	BB		New today.
				
*/

/*------------------------------------------------------------------------------

External
 Routines:		ScavCtrl	- Controls the scavenging process.

Internal
 Routines:	
				ScavSetUp	- Sets up scavenger globals for a new scavenge. 
				ScavTerm	- Terminates the current scavenging operation.
 				CheckForStop - Has the user hit the "STOP" button?

------------------------------------------------------------------------------*/

#include <unistd.h>
#include <stdio.h>

#ifdef __MWERKS__
int open(const char *path, int oflag);
#endif

#include "ScavDefs.h"
#include "Prototypes.h"
#include <Math64.h>

#include "DFALowMem.h"

static int ScavSetUp( SGlobPtr GPtr );

static int ScavTerm( SGlobPtr GPtr );

 

/*------------------------------------------------------------------------------

Function:	ScavCtrl - (Scavenger Control)

Function:	Controls the scavenging process.  Interfaces with the User Interface
			Layer (written in PASCAL).

Input:		ScavOp		-	scavenging operation to be performed:

								Op_IVChk	= start initial volume check
								Op_Verify	= start verify
								Op_Repair	= start repair
								Op_Term		= finished scavenge 

			GPtr		-	pointer to scavenger global area			


Output:		ScavRes		-	scavenge result code (R_xxx, or 0 if no error)
								
			UserCmd		-	user command

------------------------------------------------------------------------------*/

void ScavCtrl( SGlobPtr GPtr, UInt32 ScavOp, short *ScavRes, short *UserCmd )
{
	OSErr			result;
	short			stat;

	//
	//	initialize some stuff
	//
	result			= noErr;						//	assume good status
	*ScavRes		= 0;	
	*UserCmd		= No_Cmd;
	GPtr->ScavRes	= 0;
	GPtr->UserCmd	= No_Cmd;
	
	//		
	//	dispatch next scavenge operation
	//
	switch ( ScavOp)
	{
		case Op_IVChk:								//	INITIAL VOLUME CHECK
			GPtr->StatMsg = M_Initial;
			WriteMsg( GPtr, M_IVChk, kStatusMessage );
			
			if (result = ScavSetUp(GPtr))			//	set up BEFORE CheckForStop
				break;
			if (result = CheckForStop(GPtr))		//	in order to initialize wrCnt
				break;
			
			result = IVChk(GPtr);
			break;
	
		case Op_Verify:								//	VERIFY
			GPtr->StatMsg = M_Verify;
			if (result = CheckForStop(GPtr))
				break;

			if ( result = CreateExtentsBTreeControlBlock( GPtr ) )	//	Create the calculated BTree structures
				break;
			if ( result = CreateCatalogBTreeControlBlock( GPtr ) )
				break;
			if ( result = CreateAttributesBTreeControlBlock( GPtr ) )
				break;
			if ( result = CreateExtendedAllocationsFCB( GPtr ) )
				break;

			//	Now that preflight of the BTree structures is calculated, compute the CheckDisk items
			#if ( StandAloneEngine )
				CalculateItemCount( GPtr, &GPtr->itemsToProcess );
			#endif
			
			if ( !GPtr->usersAreConnected )
			{
				WriteMsg( GPtr, M_LockedVol, kStatusMessage );
				if ( result = VLockedChk( GPtr ) )
					break;
			}

			WriteMsg( GPtr, M_ExtBTChk, kStatusMessage );
			if (result = ExtBTChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
			
			WriteMsg( GPtr, M_ExtFlChk, kStatusMessage );
			if (result = ExtFlChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
			
			WriteMsg( GPtr, M_CatBTChk, kStatusMessage );
			if (result = CatBTChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
			
			WriteMsg( GPtr, M_CatFlChk, kStatusMessage );
			if (result = CatFlChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
			
			WriteMsg( GPtr, M_CatHChk, kStatusMessage );
			if (result = CatHChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
			
//			WriteMsg( GPtr, M_AttrBTChk, kStatusMessage );	//	(DSH) moved to AttrBTChk()
			if (result = AttrBTChk(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;

			WriteMsg( GPtr, M_VolumeBitMapChk, kStatusMessage );
			if (result = CheckVolumeBitMap(GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;

#if ( 0 )	//	We rely on MountCheck catching orphaned files,
			//	if MountCheck gets an error, then we'll try and fix orphans
			WriteMsg( GPtr, M_Orphaned, kStatusMessage );
			if (result = OrphanedFileCheck (GPtr))
				break;
			if (result = CheckForStop(GPtr))
				break;
#endif	

			WriteMsg( GPtr, M_VInfoChk, kStatusMessage );
			if (result = VInfoChk(GPtr))
				break;
	
			stat =	GPtr->VIStat  | GPtr->ABTStat | GPtr->EBTStat | GPtr->CBTStat | 
					GPtr->CatStat | GPtr->FilStat;
			
			if ( stat != 0 )
			{
				if ( (GPtr->RepLevel == No_RepairNeeded) || (GPtr->RepLevel == Minor_Repair) )
				{
					#if ( StandAloneEngine )
					{
						//	2200106, We isolate very minor errors so that if the volume cannot be unmounted
						//	CheckDisk will just return noErr
						short minorErrors =	(GPtr->CatStat & ( ~(S_BadCustomIcon | S_LockedDirName) ) )	|
										GPtr->VIStat  | GPtr->ABTStat | GPtr->EBTStat					|
										GPtr->CBTStat | GPtr->FilStat;
						if ( minorErrors == 0 )
							GPtr->RepLevel =  VeryMinor_Repair;
						else
							GPtr->RepLevel =  Minor_Repair;
					}
					#else
					{
						GPtr->RepLevel =  Minor_Repair;
					}
					#endif
				}
			}
			else if ( GPtr->RepLevel == No_RepairNeeded )
			{
#ifdef INVESTIGATE
				if ( GPtr->isHFSPlus == true )
				{
					long	gestaltResult;
					OSErr	err;
					
					//	If the real VCB is an ExtendedVCB (2200861)
					err = Gestalt( gestaltFSAttr, &gestaltResult );
					if ( (err == noErr) && (gestaltResult & 1L<<gestaltFSSupportsHFSPlusVols) )
						GetDateTime( &(GPtr->realVCB->checkedDate) );
				}
#endif
			}
			
			result = CheckForStop(GPtr);				//	one last check for modified volume
			break;
							
		case Op_Repair:									//	REPAIR
			GPtr->StatMsg = M_Repair;
			if ( result = CheckForStop(GPtr) )
				break;
			WriteMsg( GPtr, M_Repair, kStatusMessage );
			result = MRepair( GPtr );
			break;
		
		case Op_Term:									//	CLEANUP AFTER SCAVENGE
			result = ScavTerm(GPtr);
	//		WriteMsg( GPtr, M_Term, kStatusMessage );	// commented out for <12>
			break;
	}													//	end ScavOp switch

	
#if(0)
	//
	//	Check for the disappearing folder corruption
	//
 	if ( ScavOp == Op_Verify && result != R_UInt )
		{
			WriteMsg( GPtr, M_Missing, kStatusMessage );
			result = CheckForDFCorruption( GPtr, result );
		}
#endif

	//
	//	map internal error codes to scavenger result codes
	//
	
	if ( result < 0 || result > Max_RCode )
	{
		switch ( ScavOp )
		{	
			case Op_IVChk:
			case Op_Verify:
				if ( result == ioErr )
					result = R_RdErr;
				else
					result = R_VFail;
				GPtr->RepLevel = Unrepairable;
				break;
			case Op_Repair:
				result = R_RFail;
				break;
			default:
				result = R_IntErr;
		}
	}
	
	GPtr->ScavRes = result;

	*ScavRes = result;
	*UserCmd = GPtr->UserCmd;
		
}	//	end of ScavCtrl



/*------------------------------------------------------------------------------

Function: 	CheckForStop

Function:	Checks for the user hitting the "STOP" button during a scavenge,
			which interrupts the operation.  Additionally, we monitor the write
			count of a mounted volume, to be sure that the volume is not
			modified by another app while we scavenge.

Input:		GPtr		-  	pointer to scavenger global area

Output:		Function result:
						0 - ok to continue
						R_UInt - STOP button hit
						R_Modified - another app has touched the volume
-------------------------------------------------------------------------------*/

short CheckForStop( SGlob *GPtr )
{
	OSErr	err		= noErr;							//	Initialize err to noErr
	long	ticks	= TickCount();

	if ( (ticks - 10) > GPtr->lastTickCount )			//	To reduce cursor flicker on fast machines, call through on a timed interval
	{
		#if ( StandAloneEngine )
		{
			if ( GPtr->userCancelProc != nil )
			{
				UInt64	progress;
				UInt64	remainder;
				UInt64	oneHundred;
				
				progress = U64Multiply( GPtr->itemsProcessed, U64SetU(100) );
				progress = U64Divide( progress, GPtr->itemsToProcess, &remainder );
		
				err = CallUserCancelProc( GPtr->userCancelProc, (UInt16)progress.lo, ( U64Compare( progress, GPtr->lastProgress) != 0 ), GPtr->userContext );
		
				GPtr->lastProgress = progress;
			}
		}
		#else
		{
			err = CheckPause( GPtr->StatMsg, &GPtr->UserCmd );
		}
		#endif
		
		if ( err != noErr )
			err = R_UInt;
		if ( GPtr->realVCB )							//	If the volume is mounted
			if ( GPtr->realVCB->vcbWrCnt != GPtr->wrCnt )
				err = R_Modified;						//	Its been modified behind our back

		GPtr->lastTickCount	= ticks;
	}
			
	return ( err );
}
		


/*------------------------------------------------------------------------------

Function:	ScavSetUp - (Scavenger Set Up)

Function:	Sets up scavenger globals for a new scavenge operation.  Memory is 
			allocated for the Scavenger's static data structures (VCB, FCBs,
			BTCBs, and TPTs). The contents of the data structures are
			initialized to zero.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		ScavSetUp	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

struct ScavStaticStructures {
	ExtendedVCB			vcb;
	FCBArray			fcbList;			//	contains one FCB
	FCB					fcb[4];				//		plus 4 more FCB's
	ParallelFCB			parallelFCBList;	//	contains one extended FCB
	ExtendedFCB			extendedFCB[4];		//		plus 4 more extendedFCB's
	BTreeControlBlock	btcb[4];			//	4 btcb's
	SDPT				dirPath;			//	scavenger directory path table
	SBTPT				btreePath;			//	scavenger BTree path table
};
typedef struct ScavStaticStructures ScavStaticStructures;

extern char device[32];

static int ScavSetUp( SGlob *GPtr)
{
	DrvQEl				*drvP;
	OSErr				err;
	short				ioRefNum;
	SInt16				i;
	UInt32				*dfaLowMems;

#ifdef TARGET_OS_RHAPSODY
	int					fd;
#endif

	//
	//	Save off ApplScratch values used in DFA engine
	//
	GPtr->savedApplScratch[0]	= GetApplScratchValue( 0 );
	GPtr->savedApplScratch[1]	= GetApplScratchValue( 1 );
	GPtr->savedApplScratch[2]	= GetApplScratchValue( 2 );

	SetApplScratchValue( 0, (UInt32)nil );
	SetApplScratchValue( 1, (UInt32)nil );
	SetApplScratchValue( 2, (UInt32)nil );
	UseDFALowMems( false );						//	we'll use the real LowMems 

	dfaLowMems	= (UInt32 *) NewPtrClear( kNumLowMemsToOverride * sizeof(UInt32) );
	
	dfaLowMems[kDFAGlobals] = (UInt32) GPtr;
	SetApplScratchValue( kDFALowMemIndex, (UInt32)dfaLowMems );		//	DFA LowMem replacement values

	GPtr->MinorRepairsP = nil;
	
#if ( StandAloneEngine )
	GPtr->itemsProcessed	= U64SetU( 0L );
	GPtr->lastProgress		= U64SetU( 0L );
#endif
	
	err = InitDFACache( Blk_Size );				//	Initialize the block cache of 512 byte blocks
	if ( err != noErr )
	{
		err = IntError( GPtr, err );
		return( err );							//	could't get cache
	}


	//
	//	allocate the static data structures (VCB, FCB's, BTCB'S, DPT and BTPT)
	//
 	{
		ScavStaticStructures	*pointer;
		
		pointer = (ScavStaticStructures *) NewPtrClear( sizeof(ScavStaticStructures) );
		if ( pointer == nil )
			return( R_NoMem );					//	not enough memory
	
		GPtr->calculatedVCB				= &pointer->vcb;
		
		GPtr->FCBAPtr					= (Ptr) &pointer->fcbList;
		GPtr->calculatedExtentsFCB		= &pointer->fcbList.fcb[0];
		GPtr->calculatedCatalogFCB		= &pointer->fcbList.fcb[1];
		GPtr->calculatedAllocationsFCB	= &pointer->fcbList.fcb[2];
		GPtr->calculatedAttributesFCB	= &pointer->fcbList.fcb[3];
		GPtr->calculatedRepairFCB		= &pointer->fcbList.fcb[4];

		GPtr->fcbPBuf					= &pointer->parallelFCBList;
		GPtr->fcbPBuf->count			= 4;
		GPtr->fcbPBuf->unitSize			= sizeof(ExtendedFCB);
		GPtr->extendedExtentsFCB		= &pointer->parallelFCBList.extendedFCB[0];
		GPtr->extendedCatalogFCB		= &pointer->parallelFCBList.extendedFCB[1];
		GPtr->extendedAllocationsFCB	= &pointer->parallelFCBList.extendedFCB[2];
		GPtr->extendedAttributesFCB		= &pointer->parallelFCBList.extendedFCB[3];
		GPtr->extendedRepairFCB			= &pointer->parallelFCBList.extendedFCB[4];
		
		GPtr->calculatedExtentsBTCB		= &pointer->btcb[0];
		GPtr->calculatedCatalogBTCB		= &pointer->btcb[1];
		GPtr->calculatedRepairBTCB		= &pointer->btcb[2];
		GPtr->calculatedAttributesBTCB	= &pointer->btcb[3];
		
		GPtr->BTPTPtr					= &pointer->btreePath;
		GPtr->DirPTPtr					= &pointer->dirPath;
	}
	
	//
	//--	Allocate & set up our HFS Stack
	//		Make sure GPtr->hfsStackTop is in A6 whenever calling a routine that calls
	//		the cache or a cache routine itself.
	//
	{
		Ptr	pointer;
		pointer = NewPtrClear( kFileSystemStackSize );
		SetApplScratchValue( kHFSStackTopScratchIndex, (UInt32)(pointer + kFileSystemStackSize - 4) );
	}
	
	{
		FSVarsRec	*fsVars;

		UseDFALowMems( true );							//	we'll use the DFA LowMems 
		
		SetDFAStage( kVerifyStage );

		LMSetFSFCBLen( sizeof(FCB) );
		LMSetFCBSPtr( GPtr->FCBAPtr );
		
		fsVars = (FSVarsRec *) NewPtrClear( sizeof(FSVarsRec) );
		fsVars->fcbPBuf = GPtr->fcbPBuf;

		fsVars->gAttributesBuffer = NewPtrClear( 32768 );
		fsVars->gAttributesBufferSize	= 32768;
		err	= InitCatalogCache( &fsVars->gCatalogCacheGlobals );
		ReturnIfError( err );

		LMSetFSMVars( (Ptr)fsVars );
	}


	//
	//	locate the driveQ element for drive being scavenged
	//
 	GPtr->DrvPtr = 0;								//	<8> initialize so we can know if drive disappears
 	GPtr->realVCB = NULL;							//	assume volume not mounted

	//
	//	Set up Real structures
	//
	UseDFALowMems( false );							//	we'll use the DFA LowMems 
	err = FindDrive( &ioRefNum, &(GPtr->DrvPtr), GPtr->DrvNum );
	
	err = GetVCBDriveNum( &GPtr->realVCB, GPtr->DrvNum );

	UseDFALowMems( true );							//	default to DFA's LowMemAccessors

	if ( GPtr->DrvPtr == NULL )						//	<8> drive is no longer there!
		return ( R_NoVol );
	else
		drvP = GPtr->DrvPtr;

	//	Save current value of vcbWrCnt, to detect modifications to volume by other apps etc
	FlushVol( NULL, GPtr->realVCB->vcbVRefNum );	//	Ask HFS to update all changes to disk
	GPtr->wrCnt	= GPtr->realVCB->vcbWrCnt;			//	Remember write count after writing changes

	//	Finish initializing the VCB

	//	The calculated structures
#ifdef INVESTIGATE
	GPtr->calculatedVCB->vcbDrvNum	= drvP->dQDrive;
	GPtr->calculatedVCB->vcbDRefNum = drvP->dQRefNum;
	GPtr->calculatedVCB->vcbFSID	= drvP->dQFSID;
#endif
#ifdef TARGET_OS_RHAPSODY
	if ((fd = open(device, 2))==-1)
		return R_NoVol;
	GPtr->calculatedVCB->vcbDrvNum	= drvP->fd = (SInt16)fd;
#endif
	GPtr->calculatedVCB->vcbVRefNum = Vol_RefN;
	
	err = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(GPtr->calculatedVCB->hintCachePtr) );
	if ( err != noErr )		return( IntError( GPtr, err ) );		//	could't get cache

	err = CreateVolumeCatalogCache( GPtr->calculatedVCB );
	if ( err != noErr )		return( IntError( GPtr, err ) );		//	could't get cache


	//
	//	finish initializing the FCB's
	//
	{
		FCB		*fcb;
		
		//	Create Calculated Extents FCB
		fcb										= GPtr->calculatedExtentsFCB;
		fcb->fcbFlNm							= kHFSExtentsFileID;
		fcb->fcbVPtr							= (ExtendedVCB *)GPtr->calculatedVCB;
		fcb->fcbBTCBPtr							= (Ptr)GPtr->calculatedExtentsBTCB;
		GPtr->calculatedVCB->extentsRefNum		= kCalculatedExtentRefNum;
		BlockMoveData( "\pCalculated Extents FCB", fcb->fcbCName, sizeof("\pCalculated Extents FCB") );
			
		//	Create Calculated Catalog FCB
		fcb										= GPtr->calculatedCatalogFCB;
		fcb->fcbFlNm 							= kHFSCatalogFileID;
		fcb->fcbVPtr							= (ExtendedVCB *)GPtr->calculatedVCB;
		fcb->fcbBTCBPtr							= (Ptr)GPtr->calculatedCatalogBTCB;
		GPtr->calculatedVCB->catalogRefNum		= kCalculatedCatalogRefNum;
		BlockMoveData( "\pCalculated Catalog FCB", GPtr->calculatedCatalogFCB->fcbCName, sizeof("\pCalculated Catalog FCB") );
	
		//	Create Calculated Allocations FCB
		fcb										= GPtr->calculatedAllocationsFCB;
		fcb->fcbFlNm							= kHFSAllocationFileID;
		fcb->fcbVPtr 							= (ExtendedVCB *)GPtr->calculatedVCB;
		fcb->fcbBTCBPtr							= (Ptr) 0;					//	no BitMap B-Tree
		GPtr->calculatedVCB->allocationsRefNum	= kCalculatedAllocationsRefNum;
		BlockMoveData( "\pCalculated Allocations FCB", GPtr->calculatedAllocationsFCB->fcbCName, sizeof("\pCalculated Allocations FCB") );
	
		//	Create Calculated Attributes FCB
		fcb										= GPtr->calculatedAttributesFCB;
		fcb->fcbFlNm							= kHFSAttributesFileID;
		fcb->fcbVPtr 							= (ExtendedVCB *)GPtr->calculatedVCB;
		fcb->fcbBTCBPtr							= (Ptr)GPtr->calculatedAttributesBTCB;
		GPtr->calculatedVCB->attributesRefNum	= kCalculatedAttributesRefNum;
		BlockMoveData( "\pCalculated Attributes FCB", GPtr->calculatedAttributesFCB->fcbCName, sizeof("\pCalculated Attributes FCB") );

	}
	
	//	finish initializing the BTCB's
	{
		BTreeControlBlock	*btcb;
		
		btcb					= GPtr->calculatedExtentsBTCB;		//	calculatedExtentsBTCB
		btcb->fileRefNum		= kCalculatedExtentRefNum;
		btcb->getBlockProc		= GetBlockProc;
		btcb->releaseBlockProc	= ReleaseBlockProc;
		btcb->setEndOfForkProc	= SetEndOfForkProc;
		
		btcb					= GPtr->calculatedCatalogBTCB;		//	calculatedCatalogBTCB
		btcb->fileRefNum		= kCalculatedCatalogRefNum;	
		btcb->getBlockProc		= GetBlockProc;
		btcb->releaseBlockProc	= ReleaseBlockProc;
		btcb->setEndOfForkProc	= SetEndOfForkProc;
	
		btcb					= GPtr->calculatedAttributesBTCB;	//	calculatedAttributesBTCB
		btcb->fileRefNum		= kCalculatedAttributesRefNum;	
		btcb->getBlockProc		= GetBlockProc;
		btcb->releaseBlockProc	= ReleaseBlockProc;
		btcb->setEndOfForkProc	= SetEndOfForkProc;
	}

	
	//
	//	Initialize some global stuff
	//

	GPtr->RepLevel	= No_RepairNeeded;
	GPtr->ErrCode	= 0;
	GPtr->IntErr	= noErr;
	GPtr->VIStat	= 0;
	GPtr->ABTStat	= 0;
	GPtr->EBTStat	= 0;
	GPtr->CBTStat	= 0;
	GPtr->CatStat	= 0;
	GPtr->FilStat	= 0;

 
	//	Keep a valid file id list for HFS volumes
	GPtr->validFilesList = (UInt32**)NewHandle( 0 );
	if ( GPtr->validFilesList == nil )
		return( R_NoMem );


	return( noErr );

}	//	end of ScavSetUp

		


/*------------------------------------------------------------------------------

Function:	ScavTerm - (Scavenge Termination))

Function:	Terminates the current scavenging operation.  Memory for the
			VCB, FCBs, BTCBs, volume bit map, and BTree bit maps is
			released.
			
Input:		GPtr		-	pointer to scavenger global area

Output:		ScavTerm	-	function result:			
								0	= no error
								n 	= error code
------------------------------------------------------------------------------*/

static int ScavTerm( SGlobPtr GPtr )
{
	FCB					*fcbP;
	BTreeControlBlock	*btcbP;
	RepairOrderPtr		rP;
	Ptr					p;
	OSErr				err;

	UseDFALowMems( true );							//	we'll use the DFA LowMems 

	while( (rP = GPtr->MinorRepairsP) != nil )		/* loop freeing leftover (undone) repair orders */
	{
		GPtr->MinorRepairsP = rP->link;				/* (in case repairs were not made) */
		DisposePtr((Ptr) rP);
		err = MemError();
	}
#ifdef INVESTIGATE
	//	Dispose the HFS Stack
	p = (Ptr) GetApplScratchValue( kHFSStackTopScratchIndex );
	DisposePtr( p - kFileSystemStackSize + 4 );
	err = MemError();
#endif
	//	Dispose our LowMem alterations
	p = LMGetFSMVars( );
	if ( p != nil )
	{
		if ( ((FSVarsRec *)p)->gAttributesBuffer != nil )
			DisposePtr( ((FSVarsRec *)p)->gAttributesBuffer );

		DisposePtr( p );
		err = MemError();
	}
	
	if( GPtr->validFilesList != nil )
		DisposeHandle( (Handle) GPtr->validFilesList );
	
	if( GPtr->calculatedVCB == nil )								//	already freed?
		return( noErr );

	//	If the FCB's and BTCB's have been set up, dispose of them
	fcbP = GetFileControlBlock(  kCalculatedExtentRefNum );	//	release extent file BTree bit map
	if ( fcbP != nil )
	{
		btcbP = (BTreeControlBlock*)fcbP->fcbBTCBPtr;
		if ( btcbP != nil )
		{
			if( btcbP->refCon != (UInt32)nil )
			{
				if(((BTreeExtensionsRec*)btcbP->refCon)->BTCBMPtr != nil)
				{
					DisposePtr(((BTreeExtensionsRec*)btcbP->refCon)->BTCBMPtr);
					err = MemError();
				}
		
				DisposePtr( (Ptr)btcbP->refCon );
				err = MemError();
				btcbP->refCon = (UInt32)nil;
			}
				
			fcbP = GetFileControlBlock( kCalculatedCatalogRefNum );	/* release catalog BTree bit map */
			btcbP = (BTreeControlBlock*)fcbP->fcbBTCBPtr;

			if( btcbP->refCon != (UInt32)nil )
			{
				if(((BTreeExtensionsRec*)btcbP->refCon)->BTCBMPtr != nil)
				{
#ifdef INVESTIGATE
					DisposePtr(((BTreeExtensionsRec*)btcbP->refCon)->BTCBMPtr);
#endif
					err = MemError();
				}
				
				DisposePtr( (Ptr)btcbP->refCon );
				err = MemError();
				btcbP->refCon = (UInt32)nil;
			}
		}
	}

	p = (Ptr) GetApplScratchValue( kDFALowMemIndex );
	if ( p )
	{
		DisposePtr( p );
		err = MemError();
	}

	if ( GPtr->volumeBitMapPtr != nil )
	{
		if( GPtr->volumeBitMapPtr->buffer != nil )					//	release volume bit map
		{
#ifdef INVESTIGATE
			Handle	h = RecoverHandle( GPtr->volumeBitMapPtr->buffer );
			DisposeHandle( h );
			err = MemError();
#endif
		}
		DisposePtr( (Ptr) GPtr->volumeBitMapPtr );
		err = MemError();
	}

	(void) DisposeMRUCache( GPtr->calculatedVCB->hintCachePtr );
	(void) DisposeVolumeCatalogCache( GPtr->calculatedVCB );

#ifdef TARGET_OS_RHAPSODY
	close(GPtr->calculatedVCB->vcbDrvNum);
#endif

	DisposePtr( (Ptr) GPtr->calculatedVCB );						//	Release our block of data structures	
	err = MemError();

	GPtr->calculatedVCB = nil;
		
	err = DisposeDFACache();

	SetApplScratchValue( 0, GPtr->savedApplScratch[0] );
	SetApplScratchValue( 1, GPtr->savedApplScratch[1] );
	SetApplScratchValue( 2, GPtr->savedApplScratch[2] );

	return( noErr );
}	//	end of ScavTerm

