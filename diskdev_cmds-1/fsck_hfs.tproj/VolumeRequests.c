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
	File:		VolumeRequests.c

	Contains:	MountVolume and related utility routines for HFS & HFS Plus

	Version:	HFS Plus 1.0

	Written by:	Deric Horn

	Copyright:	© 1996-1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contacts:		Mark Day, Don Brady

		Technology:			File Systems

	Writers:

		(JL)	Jim Luther
		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	  <CS46>	12/12/97	DSH		2003877, when vcbAllocPtr was copied to nextAllocation it was
									getting sign extended.
	  <CS45>	11/26/97	DSH		2003459, fcbs was not being initialized if volume was offline
									and we are executing an unconditional unmount.
	  <CS44>	11/24/97	DSH		2005507, FlushVolumeControlBlock() keeps MDB drCrDate in sync
									with VolumeHeader createDate.
	  <CS43>	11/11/97	DSH		1685873, RemountWrappedVolumes was only remounting the first
									HFS+ volume in the queue, causing HFS wrappers to be mounted if
									multiple volumes had been mounted before InitHFSPlus.
	  <CS42>	 11/4/97	DSH		Clear FCB when getting a new one.
	  <CS41>	 11/3/97	JL		#2001483 - Removed unneeded parameters from MountVolume,
									MountHFSVolume, MountHFSPlusVolume, GetVolumeInformation,
									GetXVolumeInformation and AddVCB (and added local variables as
									needed). Return WDCBRecPtr from UnMountVolume. Set wdcb
									parameter to NULL in GetXVolumeInformation if working directory
									was not specified.
	  <CS40>	10/31/97	DSH		Added consistencyStatus parameter to MountCheck
	  <CS39>	10/23/97	msd		Bug 1685113. The VolumeHeader's createDate should be in local
									time (not GMT) and identical to the MDB's drCrDate (and VCB's
									vcbCrDate). When checking for a remount of an offline HFS Plus
									volume, compare write counts instead of mod dates (which could
									be fooled by the user changing time zones). Force MountCheck to
									run if the volume was last mounted by Bride 1.0b2 or earlier.
	  <CS38>	10/17/97	msd		Conditionalize DebugStrs.
	  <CS37>	10/13/97	djb		Update volumeNameEncodingHint when updating the volume name.
	  <CS36>	10/10/97	msd		Bug 1683571. The dates in the volume header are in GMT, so be
									sure to convert them when mounting a volume or flushing the
									volume header.
	  <CS35>	 10/2/97	DSH		In UnmountVolume() check that the drive is on line before
									determining if wrapper volume needs to be renamed causing IO.
	  <CS34>	 10/1/97	DSH		Run on disk version of MountCheck instead of ROM version for
									boot volumes1682475.
	  <CS33>	 10/1/97	djb		Add calls to InvalidateCatalogCache (part of radar #1678833).
	  <CS32>	 9/26/97	DSH		Removed debugging code: support for 'W' key wrapper mounting.
	  <CS31>	 9/17/97	DSH		hfsPlusIOPosOffset was uninitialized for Wrapperless volumes.
	  <CS30>	  9/5/97	djb		In MountVol initialize Catalog cache before calling Catalog!
	  <CS29>	  9/4/97	msd		PropertyCloseVolume renamed to AttributesCloseVolume. Remove
									call to AttributesOpenVolume (it no longer exists).
	  <CS28>	  9/2/97	DSH		VolumeHeader is now 3rd sector in partition, altVH is 2nd to
									last cor compatability.  Initial support for wrapperless
									volumes.
	  <CS27>	 8/26/97	djb		Only call CountRootFiles during MountVol.
	  <CS26>	 8/20/97	msd		If the HFS Plus volume version doesn't match, mount the wrapper
									instead.
	  <CS25>	 8/19/97	djb		Add error handling to RenameWrapperVolume.
	  <CS24>	 8/15/97	msd		Bug 1673999. In MakeVCBsExtendedVCBs, copy old VCB's vcbAllocPtr
									to new VCB's nextAllocation field.
	  <CS23>	 8/12/97	djb		Fixed GetXVolInfo to only use extended vcb fields for local
									volumes (radar# 1673177)
	  <CS22>	 8/11/97	DSH		vcbNmAlBlks is now taken from the embededExtent.blockCount
									(1669121).
	  <CS21>	 8/11/97	djb		Return actual count of files in root directory for HFS Plus
									volumes (Radar #1669118). Added local CountRootFiles routine.
									8/5/97 msd Make sure version field in VolumeHeader is exactly
									kHFSPlusVersion. 8/1/97 djb GetXVolumeInformation now returns
									extFSErr when FSID is nonzero (Radar #1649503).
	  <CS20>	 7/25/97	DSH		Init and Dispose of GenericMRUCache within ExtendedVCB.
	  <CS19>	 7/16/97	DSH		FilesInternal.x -> FileMgrInternal.x to avoid name collision
	  <CS18>	 7/15/97	DSH		Remount Wrapper volumes mounted before HFS+ initialization
									(166729)
	  <CS17>	 7/15/97	djb		Remove ioXVersion checking in GetXVolInfo (radar #1666217).
	  <CS16>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	  <CS15>	  7/7/97	djb		Add GetVolumeNameFromCatalog routine.
	  <CS14>	  7/7/97	DSH		GetNewVRefNum now get's a recycled vRefNum. Bug 1664445 in
									Installer was cacheing the vRefNum while CheckDisk unmounts and
									remounts disk.
	  <CS13>	 6/30/97	DSH		shadowing values obsoleteVCBXTRef, and obsoleteVCBCTRef when
									HFS+ volume is mounted.
	  <CS12>	 6/26/97	DSH		GetVolInfo returns HFS signature for HFS+ volumes, GetXVolInfo
									returns real signature.
	  <CS11>	 6/24/97	DSH		MakeVCBsExtendedVCBs was using wdcb->count as count not byte
									count.
	  <CS10>	 6/18/97	djb		Set/get volume encodingsBitmap.
	   <CS9>	 6/16/97	msd		Include String.h and Disks.h.
	   <CS8>	 6/12/97	djb		Get in sync with HFS Plus format changes.
	   <CS7>	 6/11/97	msd		Make GetXVolumeInformation return true allocation block size. It
									now checks the ioXVersion field.
	   <CS6>	 5/28/97	msd		When flushing the volume header, write out the allocation file's
									clump size (from the FCB). When mounting an HFS Plus volume,
									zero the entire FCB extent record, not just the first extent,
									for the various volume control files.
	   <CS5>	 5/19/97	djb		Add calls to CreateVolumeCatalogCache,
									DisposeVolumeCatalogCache.
	   <CS4>	  5/9/97	djb		Get in sync with new FilesInternal.i
	   <CS3>	  5/8/97	DSH		Only mount HFS+ volumes with version < 2.0 in the VolumeHeader.
									Return wrgVolTypErr if too new.
	   <CS2>	  5/2/97	djb		Disable Manual Eject code since its buggy!
	   <CS1>	 4/25/97	djb		first checked in

	 <HFS32>	 4/11/97	DSH		MountHFSPlusVolume gets volume name from catalog, and
									UnmountVolume shadows the name back to the wrapper partition.
	 <HFS31>	  4/8/97	msd		Once volume is mounted, call AttributesOpenVolume to allow a
									buffer to be allocated.
	 <HFS30>	  4/7/97	msd		In FlushVolumeControlBlock, don't update the attributes BTree
									fields in the Volume Header unless an attributes BTree was
									already open.
	 <HFS29>	  4/7/97	msd		In SetupFCB, add case for attributes BTree. Add code to set up
									the attributes BTree. Remove call to PropertyOpenVolume. In
									FlushVolumeControlBlock, write out any changes to the attributes
									BTree.
	 <HFS28>	  4/4/97	djb		Get in sync with volume format changes.
	 <HFS27>	 3/31/97	djb		Added catalogDataCache to VCB; Remove ClearMem routine.
	 <HFS26>	 3/18/97	msd		In MountHFSPlusVolume, the free blocks calculation can overflow,
									setting vcbFreeBks to a too-small value.
	 <HFS25>	 3/17/97	DSH		Added some utility functions AddVCB, GetParallelFCBFromRefNum,
									casting for SC, and made some functions extern for DFA.
	 <HFS24>	  3/5/97	msd		Add calls to Property Manager to open and close the volume. When
									unmounting an HFS+ volume, the allocation (bitmap) file now gets
									closed.
	 <HFS23>	 2/19/97	djb		Update to 16-bit HFS Plus signature.
	 <HFS22>	 2/12/97	msd		In GetXVolumeInformation, the result code could be
									uninitialized.
	 <HFS21>	 1/23/97	DSH		UpdateAlternateVoumeControlBlock()
	 <HFS20>	 1/15/97	djb		Remove MountCheckStub. Add file names to fcbs for debugging.
	 <HFS19>	 1/13/97	DSH		Use ExtendedVCB nextAllocation instead of vcbAllocPtr through
									all code.
	 <HFS18>	  1/9/97	djb		Get in sync with new VolumeHeader and Extended VCB.
	 <HFS17>	  1/6/97	djb		Changed API to ParallelFCBFromRefnum (pragma parameter was
									broken).
	 <HFS16>	  1/6/97	msd		Set only the defined bits in the MDB drAtrb field (when copying
									from VCB vcbAtrb field).
	 <HFS15>	  1/6/97	DSH		CloseFile requires VCB to be passed in.
	 <HFS14>	  1/6/97	djb		FlushVolumeControlBlock was writing to absolute block 0 instead
									of to block zero of the embedded volume.
	 <HFS13>	12/20/96	msd		A comparison was using "=" instead of "=="; might have caused
									the wrong volume to be set as the default.
	 <HFS12>	12/19/96	DSH		Setting up ExtendedVCBs
	 <HFS11>	12/19/96	djb		Updated for new B-tree Manager interface.
	 <HFS10>	12/18/96	msd		Change GetVCBRefNum so it can actually return a VCB pointer.
	  <HFS9>	12/12/96	djb		Use new SPI for GetCatalogNode.
	  <HFS8>	12/12/96	msd		Fix a bunch of errors (mostly type mismatch) when compiling with
									Metrowerks.
	  <HFS7>	12/12/96	DSH		adding some util functions
	  <HFS6>	12/10/96	msd		Check PRAGMA_LOAD_SUPPORTED before loading precompiled headers.
	  <HFS5>	 12/4/96	DSH		Ported GetVolumeInformation & GetXVolumeInformation.
		<3*>	11/20/96	DSH		HFS Plus support to MountVolume
	  <HFS3>	11/20/96	DSH		Added UnmountVol and related routines, also backed out <2>
									because C_FXMKeyCmp is passed as a parameter from C but called
									from Asm in BTOpen so we need a Case ON Asm entry point.
	  <HFS2>	11/20/96	msd		Use CompareExtentKeys() instead of CFXMKeyCmp().
	  <HFS1>	11/19/96	DSH		first checked in
		 <1>	11/19/96	DSH		first checked in

*/


#if TARGET_OS_MAC
#if ( PRAGMA_LOAD_SUPPORTED )
	#pragma	load	PrecompiledHeaders
#else
	#include	<Files.h>
	#include	<FSM.h>
	#include	<OSUtils.h>
	#include	<LowMemPriv.h>
	#include	<LowMem.h>
	#include	<StringCompare.h>
	#include	<Errors.h>
	#include	<Devices.h>
	#include	<SonyPriv.h>
	#include	<EDiskPriv.h>
	#include	<String.h>
	#include	<Disks.h>
#endif
#include	<HardwarePriv.h>
#include	<HFSVolumesPriv.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#include "myFiles.h"
#endif
#endif 	/* TARGET_OS_MAC */
#include	<FileMgrInternal.h>

#define		kIDSectorOffset	2

// Exported routine prototypes
#ifdef INVESTIGATE
OSErr	MountVolume( VolumeParam *volume, Boolean forceHFSWrapperToMount );

OSErr	MountHFSVolume( short driveNumber, IOParam* lowMemParamBlock, OSErr err );

OSErr	MountHFSPlusVolume( short driveNumber, IOParam* lowMemParamBlock, OSErr err, Boolean pureHFSPlusVolume );

OSErr	GetVolumeInformation( HVolumeParam *volume, WDCBRecPtr *wdcb );

OSErr	GetXVolumeInformation( XVolumeParam *volume, WDCBRecPtr *wdcb );
#endif

//	Utility function prototypes
OSErr	FindFileControlBlock( UInt16 *index, Ptr *fcbsH );

Boolean	GetNextFileControlBlock( UInt16 *index, Ptr fcbsPtr );

void	Get1stFileControlBlock( UInt16 *index, Ptr *fcbsH );

void	DisposeFileControlBlocks( ExtendedVCB *vcb );

OSErr	AccessBTree( ExtendedVCB *vcb, SInt16 refNum, UInt32 fileID, UInt32 fileClumpSize, void *CompareRoutine );

UInt16	DivUp( UInt32 byteRun, UInt32 blockSize );

Boolean	IsARamDiskDriver( void );

OSErr	GetVCBRefNum( ExtendedVCB **vcb, short vRefNum );

OSErr	ValidMasterDirectoryBlock( MasterDirectoryBlock *mdb );

void	RenameWrapperVolume( Str27 newVolumeName, UInt16 driveNumber );

OSErr	CheckExternalFileSystem( ExtendedVCB *vcb );

OSErr	FlushVolume( ExtendedVCB *vcb );

FCB		*SetupFCB( ExtendedVCB *vcb, SInt16 refNum, UInt32 fileID, UInt32 fileClumpSize );

static OSErr GetDiskBlocks( short driveNumber, unsigned long *numBlocks);

void	AddVCB( ExtendedVCB	*vcb, short driveNumber, short ioDRefNum );

short	IsPressed( unsigned short k );

SInt16	GetNewVRefNum();

OSErr	GetVolumeNameFromCatalog(ExtendedVCB *vcb);

static UInt16 CountRootFiles(ExtendedVCB *vcb);


#if ( hasHFSManualEject )
static void SetVCBManEject(ExtendedVCB *vcb);
#endif

// External routines

extern	OSErr	C_FlushMDB( ExtendedVCB *volume );

extern	OSErr	DisposeVolumeCacheBlocks( ExtendedVCB *vcb );

extern	void	DisposeVolumeControlBlock( ExtendedVCB *vcb );

extern	OSErr	DeturmineVolume1( VolumeParam *iopb, short *nameLength, Boolean *nameSpecified, ExtendedVCB **vcb, WDCBRec **wdcb, unsigned char **pathName );

extern	OSErr	DeturmineVolume3( VolumeParam *iopb, short *nameLength, Boolean *nameSpecified, ExtendedVCB **vcb, WDCBRec **wdcb, unsigned char **pathName );

extern	OSErr	FlushVolumeBuffers( ExtendedVCB *vcb );

extern	void	MultiplyUInt32IntoUInt64( UInt64 *wideResult, UInt32 num1, UInt32 num2 );


#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	MountVolume
//
// Function: 	The VCB pointers are checked to see if a volume in the
//				specified drive is already on-line (error if so).  Reads in the
//				directory master block and allocates memory for VCB and additional
//				data structures, depending on the volume type:
//
//				- For MFS volumes (handled in MFSVol), space is allocated for
//				  a volume buffer, as well as the block map, which is read in
//				  from disk).
//
//				- For TFS volumes, space is allocated for a bitmap cache, a volume
//				  control cache (for B*-Tree use), and a volume cache.	In addition,
//				  two FCBs are used by the B*-Trees.
//
//				No new VCB storage is allocated for remounts.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr	MountVolume( VolumeParam *volume, Boolean forceHFSWrapperToMount )
{
	Byte					blockBuffer[512];
	IOParam					*lowMemParamBlock;
	short					ioRefNum;
	short					vRefNum;
	OSErr					err;
	DrvQEl					*dqe;
	Boolean					pureHFSPlusVolume;
	
	
	//--- FSQueueSync was already called by the MountVol glue.
	
	vRefNum = volume->ioVRefNum;
	err = FindDrive( &ioRefNum, &dqe, vRefNum );							//	Get driver refnum
	if ( err == noErr )
	{
		lowMemParamBlock				= (IOParam *) LMParamBlock;
		lowMemParamBlock->ioVRefNum		= vRefNum;
		lowMemParamBlock->ioRefNum		= ioRefNum;
		lowMemParamBlock->ioBuffer		= (char *)blockBuffer;
		lowMemParamBlock->ioPosMode		= fsFromStart;
		lowMemParamBlock->ioPosOffset	= kIDSectorOffset * 512;			//	MDB is always at block #2
		lowMemParamBlock->ioReqCount	= 512;								//	MDB is always 1 512 byte block
		
		err = PBReadSync( (ParmBlkPtr) lowMemParamBlock );					//	Read the MDB
		
		pureHFSPlusVolume	= ((VolumeHeader *)blockBuffer)->signature == kHFSPlusSigWord;
		
		if ( 	( ((MasterDirectoryBlock *)blockBuffer)->drEmbedSigWord == kHFSPlusSigWord	//	Wrapped HFS+ volume
			||	pureHFSPlusVolume == true )									//	Pure HFS+ volume
			&&	err == noErr
			&&	forceHFSWrapperToMount == false )							//	if we don't want to force the wrapper to mount
		{
			err = MountHFSPlusVolume( vRefNum, lowMemParamBlock, err, pureHFSPlusVolume );
		}
		else																//	pass all other volume formats / errors to MountHFSVolume
		{
			err = MountHFSVolume( vRefNum, lowMemParamBlock, err );
		}
	}
	
	return( err );															//	pass the error back to CmdDone
}




//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	MountHFSPlusVolume
//
// Function: 	Called by MountVolume()
//
//	At this point we know that the MDB's Embeded signature indicates that
//	this is an HFS Plus disk with an HFS wrapper.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	MountHFSPlusVolume( short driveNumber, IOParam* lowMemParamBlock, OSErr err, Boolean pureHFSPlusVolume )
{
	MasterDirectoryBlock	*mdb;
	Byte					blockBuffer[512];
	VolumeHeader			*volumeHeader;
	ExtendedVCB				*vcb;
	QHdr					*vcbQHdr;
	Byte					*flagsP;
	Byte					*pbFlagsP;
	SInt16					ioRefNum;
	FSVarsRec				*fsVars;
	DrvQEl					*dqe;
	IOParam					*currentIORequestPB;
	FCB						*fcb;
	Ptr 					fcbsP;
	ExtendedFCB				*extendedFCB;
	UInt32					hfsPlusIOPosOffset;
	UInt16					hfsBlockCount;
	UInt32					hfsBlockSize;
	UInt32					lastMountedVersion;
	UInt16					fcbIndexP;

	//--	Now read the HFS Plus VolumeHeader located at the first block within the embeded volume

	ioRefNum = lowMemParamBlock->ioRefNum;
	
	if ( pureHFSPlusVolume == false )
	{
		mdb = (MasterDirectoryBlock *) lowMemParamBlock->ioBuffer;

		hfsPlusIOPosOffset				=	(mdb->drEmbedExtent.startBlock * mdb->drAlBlkSiz) + ( mdb->drAlBlSt * 512 );
		
		lowMemParamBlock->ioPosMode		=	fsFromStart;				
		lowMemParamBlock->ioPosOffset	=	hfsPlusIOPosOffset + (kIDSectorOffset * 512);		//	VolumeHeader is 2 sectors in
		lowMemParamBlock->ioBuffer		=	(char *)blockBuffer;

		err = PBReadSync( (ParmBlkPtr) lowMemParamBlock );						//	Read the VolumeHeader
	}
	else
	{
		hfsPlusIOPosOffset	= 0;		//	From beginning of volume.
	}
	
	volumeHeader = (VolumeHeader *) lowMemParamBlock->ioBuffer;
	
	err = ValidVolumeHeader( volumeHeader );

	if (err == badMDBErr || err == noMacDskErr)
		return err;															// it's not valid, so don't try to mount it

	if ( err == noErr )
	{
		err		= notARemountErr;											//	cuz we're looking for remounts only here
		vcbQHdr	= LMGetVCBQHdr();											//	search the queue of VCBs

		for ( vcb = (ExtendedVCB*)vcbQHdr->qHead ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
		{
			if (   (vcb->vcbDrvNum	== 0)									//	matching volume better be off-line (0 drive num)
				&& (vcb->vcbSigWord	== volumeHeader->signature)				//	Same signature?
				&& (vcb->vcbCrDate	== volumeHeader->createDate)			//	Same create date?
				&& (vcb->vcbWrCnt	== volumeHeader->writeCount)			//	Same write count?
			   )
			{
				//	At this point, we're fairly certain that the two volumes are, in fact, the same,
				//	but just to make sure (and avoid problems with stuck clocks), we'll compare the
				//	volume names as well:
				
				//€€	How do I compare unicode names?
				//€€	do we just skip this check?
		//		if ( EqualString( vcb->volumeName, volumeHeader->volumeName, true, true) )	//	caseSensitive, diacSensitive
					break;													//	Remount
			}
		}
		if ( vcb == nil ) goto RemountExit;


		((CntrlParam*)lowMemParamBlock)->csCode = drvStsCode;				//	Drive status!
		((CntrlParam*)lowMemParamBlock)->csParam[1] &= 0x00ff;				//	Clear status byte for return
		
		err = PBStatusSync( (ParmBlkPtr)lowMemParamBlock );					//	refnum and drivenum set up by read call


		//--	Now propagate the write protect status byte back to the vcb
		flagsP = (Byte *)((Ptr)(&(vcb->vcbAtrb))) + 1;
		pbFlagsP = (Byte *)((Ptr)(((CntrlParam*)lowMemParamBlock)->csParam)+2);
		*flagsP = *pbFlagsP & kVolumeHardwareLockMask;

		vcb->vcbDrvNum	= driveNumber;
		vcb->vcbDRefNum	= ioRefNum;											//	Driver RefNum
		
		#if ( hasHFSManualEject )
			SetVCBManEject(vcb);
		#endif


		if ( !(*flagsP & kVolumeHardwareLockMask) )							//	if bit is NOT set
		{
			vcb->vcbAtrb &= ~kVolumeUnmountedMask;							//	clear the kVolumeUnmounted attribute
			MarkVCBDirty( vcb );
			err = FlushVolumeControlBlock( vcb );
		}
		
		err = noErr;

	}


RemountExit:
	if ( err == noErr ) goto CmdDone;										//	noErr implies a successful remount			
		
	//--	if we are allowing new mounts, let the ROM handle the details.  Otherwise, let the error stand
	fsVars = (FSVarsRec*)LMGetFSMVars();
	if ( fsVars->fsFlags & fsNoAllocateMask )
		goto CmdDone;
		
	LMSetNewMount( -1 );													//	assume now that it's a new volume
	
	
	
	//--	First see if the disk is already mounted.
mv_Start:
	err = paramErr;
	driveNumber = lowMemParamBlock->ioVRefNum;
	if ( driveNumber <= 0 )	goto MtVolDone;
	err = GetVCBDriveNum( &vcb, driveNumber );
	if ( err == noErr )
	{
		if ( vcb->vcbDrvNum != 0 )
		{
			err = volOnLinErr;
			goto MtVolDone;
		}
	}
	
	//--	Find the appropriate drive queue entry and allocate a VCB.
	err = FindDrive( &ioRefNum, &dqe, driveNumber );
	if ( err != noErr ) goto MtVolDone;
	
	vcb = (ExtendedVCB *) NewPtrSysClear( sizeof( ExtendedVCB ) );		//	Zero out the Extended VCB

	if ( vcb == nil ) goto MtVolDone;									//	exit if we got no memory
	
	AddVCB( vcb, driveNumber, ioRefNum );								//	prep the VCB for cache calls
	
	//--	Now try reading in the Volume Header.
	//	We use "hfsPlusIOPosOffset / 512" cuz the VCB is not set up yet, so it's an absolute read
	err = GetBlock_glue( gbDefault, (hfsPlusIOPosOffset / 512) + kIDSectorOffset , (Ptr*)&volumeHeader, vcb->vcbVRefNum, vcb );	//	no Flags, VH is always block #0, vcb->vcbBufAdr == CacheQueueHeader

	if ( err != noErr )	goto MtVolEr2;									//	Quit & Release VCB on errors
	
	//--	Now check what manner of animal we have before us:
	err = RelBlock_glue( (Ptr)volumeHeader, rbTrashMask );
	
	err = ValidVolumeHeader( volumeHeader );
	if ( err != noErr )
	{
		if ( err == noMacDskErr )
			goto MtVolEr2;
		else
			goto MtVolEr1;
	}
	
	//	The VolumeHeader seems OK: transfer info from buffer into VCB.

	//	Initialize our dirID/nodePtr cache associated with this volume.
	err = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(vcb->hintCachePtr) );
	ReturnIfError( err );

	//--	Copy fields between [vcbSigWord, vcbVN]

	vcb->vcbCrDate					=	volumeHeader->createDate;					// NOTE: local time, not GMT!
	vcb->vcbLsMod					=	UTCToLocal(volumeHeader->modifyDate);
	vcb->vcbAtrb					=	(UInt16)volumeHeader->attributes;			//	VCB uses 16 bits, next 16 are for future API changes
	vcb->vcbClpSiz					=	volumeHeader->rsrcClumpSize;
	vcb->vcbNxtCNID					=	volumeHeader->nextCatalogID;
	vcb->vcbVolBkUp					=	UTCToLocal(volumeHeader->backupDate);
	vcb->vcbWrCnt					=	volumeHeader->writeCount;
	vcb->vcbXTClpSiz				=	volumeHeader->extentsFile.clumpSize;
	vcb->vcbCTClpSiz				=	volumeHeader->catalogFile.clumpSize;
	vcb->attributesClumpSize		=	volumeHeader->attributesFile.clumpSize;
	vcb->vcbFilCnt					=	volumeHeader->fileCount;
	vcb->vcbDirCnt					=	volumeHeader->folderCount;
	BlockMoveData( volumeHeader->finderInfo, vcb->vcbFndrInfo, sizeof( SInt32 ) * 8 );
	
	if ( pureHFSPlusVolume == true )
	{
		UInt32	totalSectors		=	(volumeHeader->blockSize / 512) * volumeHeader->totalBlocks;
		HFSBlocksFromTotalSectors( totalSectors, &hfsBlockSize, &hfsBlockCount );
	}
	else
	{
		hfsBlockSize				=	mdb->drAlBlkSiz;
		hfsBlockCount				=	mdb->drEmbedExtent.blockCount;
	}
	
	//	vcbFreeBks is the number of free bytes divided by the (HFS compatible) allocation block size.  Since the
	//	number of free bytes could overflow a UInt32, we actually divide both free bytes and allocation block size by
	//	512 before doing the division.  This prevents intermediate values from overflowing until the volume size is
	//	2 TB.  It's OK to divide by 512 since both HFS and HFS Plus require allocation blocks to be a multiple of 512.
	vcb->vcbFreeBks = (volumeHeader->freeBlocks * (volumeHeader->blockSize / 512)) / (hfsBlockSize / 512);
	

	//--	Fields taken from the MDB
	vcb->vcbSigWord					=	kHFSPlusSigWord;						//	new field in VCB space
	vcb->vcbNmAlBlks				=	hfsBlockCount;							//	number of HFS allocation blocks HFS+ volume
	vcb->vcbAlBlkSiz				=	hfsBlockSize;							//	add to extended vcb

	//--	Now fill in the Extended VCB info
	vcb->vcbSigWord					=	volumeHeader->signature;
	vcb->nextAllocation				=	volumeHeader->nextAllocation;
	vcb->totalBlocks				=	volumeHeader->totalBlocks;
	vcb->freeBlocks					=	volumeHeader->freeBlocks;
	vcb->blockSize					=	volumeHeader->blockSize;
	vcb->allocationsClumpSize		=	volumeHeader->allocationFile.clumpSize;
	vcb->checkedDate				=	volumeHeader->checkedDate;
	vcb->encodingsBitmap			=	volumeHeader->encodingsBitmap;
	
	//	Set the allocation block start for this HFS Plus wrapped volume
	vcb->vcbAlBlSt					=	hfsPlusIOPosOffset / 512;
	vcb->hfsPlusIOPosOffset			=	hfsPlusIOPosOffset;
	
	//€€	Questionables
	vcb->vcbVBMSt					= 0;										//€€ (We should set this) BitMap start NOT used in HFS Plus
	vcb->vcbEmbedSigWord			= 0;
	vcb->vcbEmbedExtent.startBlock	= 0;
	vcb->vcbEmbedExtent.blockCount	= 0;


	//--	Mark all the unused VCB fields with 0
	vcb->vcbAllocPtr				=	0;										//	use extended VCB value				
	
	//	Remember which implementation last mounted this volume
	lastMountedVersion				=	volumeHeader->lastMountedVersion;
	
	currentIORequestPB = (IOParam *) ((QHdr *)LMGetFSQHdr())->qHead;			//	current request
	currentIORequestPB->ioVRefNum = vcb->vcbVRefNum;							//	return volume refnum to user
	
	lowMemParamBlock = (IOParam *) LMParamBlock;
	((CntrlParam*)lowMemParamBlock)->csCode = drvStsCode;						//	drive status!
	((CntrlParam*)lowMemParamBlock)->csParam[1] &= 0x00ff;						//	Clear status byte for return
	
	err = PBStatusSync( (ParmBlkPtr)lowMemParamBlock );
	
	flagsP = (Byte *)((Ptr)(&(vcb->vcbAtrb))) + 1;
	pbFlagsP = (Byte *)((Ptr)(((CntrlParam*)lowMemParamBlock)->csParam)+2);
	*flagsP = *pbFlagsP & kVolumeHardwareLockMask;
	
	if ( IsARamDiskDriver() )
	{
		vcb->vcbAtrb |= kVolumeNoCacheRequiredMask;								//	yes: mark VCB as a RAM Disk
	}
	
	vcb->vcbXTAlBlks = 0;	//	Unused, use fcb.fcbPLen instead
	vcb->vcbCTAlBlks = 0;	//	Unused, use fcb.fcbPLen instead
	
	vcb->vcbWrCnt++;															//	Compensate for write of MDB on last flush
	
	
	//--	Try to find three free FCBs for use by the volume control B*-Trees:
	
	//
	//--	Set up the Extent FCB
	//
	err = FindFileControlBlock( &fcbIndexP, &fcbsP );									//	Get new FCB
	if ( err != noErr )	goto MtVolErr;
	
	fcb = GetFileControlBlock( fcbIndexP );
	fcb->fcbEOF = volumeHeader->extentsFile.logicalSize.lo;								//	Set extent tree's LEOF
	fcb->fcbPLen = volumeHeader->extentsFile.totalBlocks * volumeHeader->blockSize;		//	Set extent tree's PEOF

	vcb->extentsRefNum		= fcbIndexP;												//	Set Extents B*-Tree file RefNum

	ClearMemory( (Ptr)fcb->fcbExtRec, sizeof(SmallExtentRecord) );

	extendedFCB = ParallelFCBFromRefnum( fcbIndexP );
	BlockMoveData( volumeHeader->extentsFile.extents, extendedFCB->extents, sizeof(LargeExtentRecord) );
	
	//
	//--	Set up the Catalog FCB
	//
	err = FindFileControlBlock( &fcbIndexP, &fcbsP );
	if ( err != noErr )	goto MtVolErr;
	
	fcb = GetFileControlBlock( fcbIndexP );
	fcb->fcbEOF = volumeHeader->catalogFile.logicalSize.lo;								//	Set catalog tree's LEOF
	fcb->fcbPLen = volumeHeader->catalogFile.totalBlocks * volumeHeader->blockSize;		//	Set catalog tree's PEOF

	vcb->catalogRefNum		= fcbIndexP;												//	Set Catalog B*-Tree file RefNum

	ClearMemory( (Ptr)fcb->fcbExtRec, sizeof(SmallExtentRecord) );
	
	extendedFCB = ParallelFCBFromRefnum( fcbIndexP );
	BlockMoveData( volumeHeader->catalogFile.extents, extendedFCB->extents, sizeof(LargeExtentRecord) );

	
	//
	//--	Set up the Allocations FCB
	//
	err = FindFileControlBlock( &fcbIndexP, &fcbsP );
	if ( err != noErr )	goto MtVolErr;
	
	fcb = GetFileControlBlock( fcbIndexP );
	fcb->fcbEOF = volumeHeader->allocationFile.logicalSize.lo;							//	Set catalog tree's LEOF
	fcb->fcbPLen = volumeHeader->allocationFile.totalBlocks * volumeHeader->blockSize;	//	Set catalog tree's PEOF

	vcb->allocationsRefNum	= fcbIndexP;												//	Set Catalog B*-Tree file RefNum

	ClearMemory( (Ptr)fcb->fcbExtRec, sizeof(SmallExtentRecord) );

	extendedFCB = ParallelFCBFromRefnum( fcbIndexP );
	BlockMoveData( volumeHeader->allocationFile.extents, extendedFCB->extents, sizeof(LargeExtentRecord) );

	
	//
	//--	Set up the Attributes FCB
	//
	vcb->attributesRefNum = 0;		//	In case there isn't an attributes BTree
	
	if (volumeHeader->attributesFile.extents[0].blockCount != 0)
	{
		err = FindFileControlBlock( &fcbIndexP, &fcbsP );
		if ( err != noErr )	goto MtVolErr;
		
		fcb = GetFileControlBlock( fcbIndexP );
		fcb->fcbEOF = volumeHeader->attributesFile.logicalSize.lo;							//	Set attribute tree's LEOF
		fcb->fcbPLen = volumeHeader->attributesFile.totalBlocks * volumeHeader->blockSize;	//	Set attribute tree's PEOF

		vcb->attributesRefNum	= fcbIndexP;												//	Set Catalog B*-Tree file RefNum

		ClearMemory( (Ptr)fcb->fcbExtRec, sizeof(SmallExtentRecord) );

		extendedFCB = ParallelFCBFromRefnum( fcbIndexP );
		BlockMoveData( volumeHeader->attributesFile.extents, extendedFCB->extents, sizeof(LargeExtentRecord) );
	}
	
	
	vcb->vcbMAdr	= (Ptr)*(long *)SysBMCPtr;					//	Set up the volume bitmap cache
	vcb->vcbCtlBuf	= (Ptr)*(long *)SysCtlCPtr;					//	Set up the volume control cache
	
	//
	//--	Finish setting up the FCBs for the extent B*-Tree and the catalog B*-Tree:
	//

	err = AccessBTree( vcb, vcb->extentsRefNum, kHFSExtentsFileID, vcb->vcbXTClpSiz, (void *) CompareExtentKeysPlus );
	if ( err != noErr )	goto MtChkErr;
	
	err = AccessBTree( vcb, vcb->catalogRefNum, kHFSCatalogFileID, vcb->vcbCTClpSiz, (void *) CompareExtendedCatalogKeys );
	if ( err != noErr )	goto MtChkErr;
	
	if (vcb->attributesRefNum)
	{
		err = AccessBTree( vcb, vcb->attributesRefNum, kHFSAttributesFileID, vcb->attributesClumpSize, (void *) CompareAttributeKeys );
		if ( err != noErr )	goto MtChkErr;
	}

	(void) SetupFCB( vcb, vcb->allocationsRefNum, kHFSAllocationFileID, volumeHeader->allocationFile.clumpSize );
	

	err = CreateVolumeCatalogCache(vcb);
	if ( err )
		goto MtChkErr;

	// set up root file count...
	
	vcb->vcbNmFls = CountRootFiles(vcb);
	
	
	//	Get the volume name from the catalog file
	(void) GetVolumeNameFromCatalog(vcb);
	
	//	If the volume was not unmounted properly, then run MountCheck.
	//	If the volume was last mounted by Bride 1.0b2 ('8.01'), then the create date may be wrong, so
	//		we should run MountCheck to fix it.
	if ( !(vcb->vcbAtrb & (short)kVolumeUnmountedMask) ||
			lastMountedVersion == '8.01')
	{
		UInt32		consistencyStatus;
		
		vcb->vcbAtrb &=	~kVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
		*(Ptr)FSCallAsync = 0;
		err = MountCheck( vcb, &consistencyStatus );
		if ( err )
			goto MtChkErr;
	}
	else
	{
		vcb->vcbAtrb &=	~kVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
	}

CheckRemount:

	//	€€This comment was in TFSVol.a, should look into it
	//	Deleted stuff patched out by KillCheckRemountNiceWay in FileMgrPatches.a <SM1>


MtVolOK:
	err = noErr;

MtVolDone:
	if ( err == noErr )
	{
		if ( !(vcb->vcbAtrb & kVolumeHardwareLockMask) )		//	if the disk is not write protected
		{
			MarkVCBDirty( vcb );								//	mark VCB dirty so it will be written
			err = FlushVolumeControlBlock( vcb );				//	Go flush the VCB info BEFORE close
		}
	}
	goto	CmdDone;



MtChkErr:
	(void) DisposeVolumeCatalogCache(vcb);
	InvalidateCatalogCache( vcb );

	*((OSErr *)HFSDSErr) = err;
	err = badMDBErr;
	
	//--	Release any resources allocated so far before exiting with an error:
MtVolErr:
	DisposeFileControlBlocks( vcb );								//	deallocate control file FCBs
	
MtVolEr1:
	DisposeVolumeCacheBlocks( vcb );								//	invalidate any cache blocks for this volume
	
MtVolEr2:
	if ( (Byte)LMGetNewMount() == 0 )								//	only deallocate VCB for new mounts
	{
		//		Remount case -- Must zero buffer and map ptrs in old VCB.
		//		Restore VCB to its original off-line state (ie clear VCBDrvNum,
		//		and move drive number to VCBDRefNum field)
		vcb->vcbDRefNum	= vcb->vcbDrvNum;							//	write drv number to VCBDRefNum field
		vcb->vcbDRefNum	= 0;										//	clear drive number field of VCB
		vcb->vcbMAdr	= nil;										//	clear mapptr in VCB
		vcb->vcbBufAdr	= nil;										//	clear buffptr in VCB
	}
	else
	{
		DisposeVolumeControlBlock( vcb );
	}

	if ( err <= 0 )	goto CmdDone;
	
	currentIORequestPB = (IOParam *) ((QHdr *)LMGetFSQHdr())->qHead;
	goto mv_Start;	

CmdDone:
return( err );

}



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	MountHFSVolume
//
// Function: 	The VCB pointers are checked to see if a volume in the
//				specified drive is already on-line (error if so).  Reads in the
//				directory master block and allocates memory for VCB and additional
//				data structures, depending on the volume type:
//
//				- For MFS volumes (handled in MFSVol), space is allocated for
//				  a volume buffer, as well as the block map, which is read in
//				  from disk).
//
//				- For TFS volumes, space is allocated for a bitmap cache, a volume
//				  control cache (for B*-Tree use), and a volume cache.	In addition,
//				  two FCBs are used by the B*-Trees.
//
//				No new VCB storage is allocated for remounts.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr	MountHFSVolume( short driveNumber, IOParam* lowMemParamBlock, OSErr err )
{
	MasterDirectoryBlock	*mdb;
	QHdr					*vcbQHdr;
	Byte					*flagsP;
	Byte					*mdbFlagsP;
	FSVarsRec				*fsVars;
	IOParam					*currentIORequestPB;
	Ptr 					fcbsP;
	FCB						*fcb;
	ExtendedVCB				*vcb;
	DrvQEl					*dqe;
	short					ioRefNum;
	UInt16					fcbIndexP;
	
	if ( err != noErr )
		goto RemountExit;
	
	ioRefNum = lowMemParamBlock->ioRefNum;
	mdb = (MasterDirectoryBlock *) lowMemParamBlock->ioBuffer;			//	€€ figure out how we use VolumeHeader & MasterDirectoryBlock, size of structures, etc.	
	
	err = ValidMasterDirectoryBlock( mdb );								//	is this an HFS/HFS PlusDisk
	if ( err == noErr )
	{
		err = notARemountErr;											//	cuz we're looking for remounts only here
		vcbQHdr = LMGetVCBQHdr();										//	search the queue of VCBs
		
		//	CheckRemountLoop:
		for ( vcb = (ExtendedVCB*)vcbQHdr->qHead ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
		{
			if (   (vcb->vcbDrvNum	== 0)								//	matching volume better be off-line (0 drive num)
				&& (vcb->vcbSigWord	== mdb->drSigWord)					//	Same signature?
				&& (vcb->vcbCrDate	== mdb->drCrDate)					//	Same create date?
				&& (vcb->vcbLsMod	== mdb->drLsMod)					//	Same mod date?
			   )
			{
				//	At this point, we're fairly certain that the two volumes are, in fact, the same,
				//	but just to make sure (and avoid problems with stuck clocks), we'll compare the
				//	volume names as well:
				//€€	Modify this call to use the extended VCB ascii name, not the Unicode name
				if ( EqualString( vcb->vcbVN, mdb->drVN, true, true) )	//	caseSensitive, diacSensitive
					break;												//	Remount
			}
		}
		if ( vcb == nil ) goto RemountExit;
	
		//	We are now convinced that we have a remount.  We will rebuild the attributes
		//	byte, which is the only one that could have changed.  We'll also store the
		//	current drive number and driver refNum, and mark the volume dirty.
		//	Remount:
	
		((CntrlParam*)lowMemParamBlock)->csCode = drvStsCode;			//	Drive status!
		((CntrlParam*)lowMemParamBlock)->csParam[1] &= 0x00ff;			//	Clear status byte for return
		
		err = PBStatusSync( (ParmBlkPtr)lowMemParamBlock );				//	refnum and drivenum set up by read call
		
		//--	Now propagate the write protect status byte back to the vcb
		flagsP = ((Byte *)(&(vcb->vcbAtrb))) + 1;
		mdbFlagsP = ((Byte *)(((CntrlParam*)lowMemParamBlock)->csParam)+2);
		*flagsP = *mdbFlagsP & kVolumeHardwareLockMask;

		vcb->vcbDrvNum	= driveNumber;
		vcb->vcbDRefNum	= ioRefNum;										//	Driver RefNum
		
		#if ( hasHFSManualEject )
			SetVCBManEject(vcb);
		#endif
		
		if ( !(*flagsP & kVolumeHardwareLockMask) )						//	if bit is NOT set
		{
			vcb->vcbAtrb &= ~kVolumeUnmountedMask;						//	clear the kVolumeUnmounted attribute
			MarkVCBDirty( vcb );
			err = FlushVolumeControlBlock( vcb );						//	Go flush the VCB info BEFORE close
		}
		
		err = noErr;
	}


RemountExit:
	if ( err == noErr )
		goto CmdDone;													//	noErr implies a successful remount			
		
	//--	if we are allowing new mounts, let the ROM handle the details.  Otherwise, let the error stand
	fsVars = (FSVarsRec*)LMGetFSMVars();								//	$BB8
	if ( fsVars->fsFlags & fsNoAllocateMask )
		goto CmdDone;
		
	//€€ This sets the word to -1, Asm only set the byte to -1
	LMSetNewMount( -1 );												//	assume now that it's a new volume
	
	
	
	//--	First see if the disk is already mounted.
mv_Start:
	err = paramErr;
	driveNumber = lowMemParamBlock->ioVRefNum;
	if ( driveNumber <= 0 )
		goto MtVolDone;
	
	err = GetVCBDriveNum( &vcb, driveNumber );
	if ( err == noErr )
	{
		if ( vcb->vcbDrvNum != 0 )
		{
			err = volOnLinErr;
			goto MtVolDone;
		}
	}
	
	//--	Find the appropriate drive queue entry and allocate a VCB.
	err = FindDrive( &ioRefNum, &dqe, driveNumber );
	if ( err != noErr ) goto MtVolDone;
	
	vcb = (ExtendedVCB *) NewPtrSysClear( sizeof( ExtendedVCB ) );

	if ( vcb == nil ) goto MtVolDone;									//	exit if we got no memory
	
	AddVCB( vcb, driveNumber, ioRefNum );								//	prep the VCB for cache calls

	//--	Now try reading in the master directory block.
	err = GetBlock_glue( gbDefault, kIDSectorOffset, (Ptr*)&mdb, vcb->vcbVRefNum, vcb );	//	no Flags, MDB is always block #2, vcb->vcbBufAdr == CacheQueueHeader
	if ( err != noErr )	goto MtVolEr2;									//	Quit & Release VCB on errors
	
	//--	Now check what manner of animal we have before us:
	err = RelBlock_glue( (Ptr)mdb, rbTrashMask );
	
	err = ValidMasterDirectoryBlock( mdb );
	if ( err != noErr )
	{
		if ( err == noMacDskErr )
			goto MtVolEr2;
		else
			goto MtVolEr1;
	}
	
	//	The MDB seems OK: transfer all master directory info from buffer into VCB.
	
	//	vcbSigWord is the start of duplicate mdb/vcb info. Asm VCBDInfoSt Equ 8
	//	copy all fields in mdb till the drVolBkUp field
	
	BlockMoveData( mdb, &(vcb->vcbSigWord),  (Size)(&(mdb->drVolBkUp)) - (Size)mdb );

	//	Initialize our dirID/nodePtr cache associated with this volume.
	err = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(vcb->hintCachePtr) );
	ReturnIfError( err );

	//-- Some fields, vcbNmAlBlks, vcbAlBlkSiz, and vcbFreeBks are just around for API compatability.
	//-- The HFS/HFS Plus code we will use the corresponding ExtendedVCB fields. vcbNmAlBlks * vcbAlBlkSiz
	//-- still gives correct volume size.
	vcb->totalBlocks	=	vcb->vcbNmAlBlks;
	vcb->freeBlocks		=	vcb->vcbFreeBks;
	vcb->blockSize		=	vcb->vcbAlBlkSiz;

	
	currentIORequestPB = (IOParam *) ((QHdr *)LMGetFSQHdr())->qHead;			//	current request
	currentIORequestPB->ioVRefNum = vcb->vcbVRefNum;							//	return volume refnum to user
	
	lowMemParamBlock = (IOParam *) LMParamBlock;
	((CntrlParam*)lowMemParamBlock)->csCode = drvStsCode;						//	drive status!
	((CntrlParam*)lowMemParamBlock)->csParam[1] &= 0x00ff;						//	Clear status byte for return
	
	err = PBStatusSync( (ParmBlkPtr)lowMemParamBlock );
	
	flagsP = ((Byte *)(&(vcb->vcbAtrb))) + 1;
	mdbFlagsP = ((Byte *)(((CntrlParam*)lowMemParamBlock)->csParam)+2);
	*flagsP = *mdbFlagsP & kVolumeHardwareLockMask;
	
	if ( IsARamDiskDriver() )
	{
		vcb->vcbAtrb |= kVolumeNoCacheRequiredMask;								//	yes: mark VCB as a RAM Disk
	}
	
	//--	copy all fields in mdb between drXTFlSize & drVolBkUp
	BlockMoveData( &(mdb->drVolBkUp), &(vcb->vcbVolBkUp), (Size)&(mdb->drXTFlSize) - (Size)&(mdb->drVolBkUp) ); //	66 bytes
	
	vcb->vcbXTAlBlks = DivUp( mdb->drXTFlSize, vcb->blockSize );				//	Pick up PEOF of extent B*-Tree
	vcb->vcbCTAlBlks = DivUp( mdb->drCTFlSize, vcb->blockSize );				//	Pick up PEOF of catalog B*-Tree
	
																				//	€€€ WHY ARE WE RESTARTING THE ROAVING PTR
	vcb->nextAllocation				=	mdb->drAllocPtr;
	vcb->vcbAllocPtr = 0;														//	Restart the roving allocation ptr
	vcb->vcbWrCnt++;															//	Compensate for write of MDB on last flush
	
	
	//--	Try to find two free FCBs for use by the volume control B*-Trees:
	
	//--	Set up the Extent FCB
	err = FindFileControlBlock( &fcbIndexP, &fcbsP );
	if ( err != noErr )	goto MtVolErr;
	
	fcb = GetFileControlBlock( fcbIndexP );
	fcb->fcbEOF = mdb->drXTFlSize;												//	Set catalog tree's LEOF
	fcb->fcbPLen = mdb->drXTFlSize;												//	Set catalog tree's PEOF

	vcb->extentsRefNum		= fcbIndexP;										//	Set Extents B*-Tree file RefNum
	BlockMoveData( mdb->drXTExtRec, fcb->fcbExtRec, sizeof(SmallExtentRecord) );
	
	
	//--	Set up the Catalog FCB
	err = FindFileControlBlock( &fcbIndexP, &fcbsP );
	if ( err != noErr )	goto MtVolErr;
	
	fcb = GetFileControlBlock( fcbIndexP );
	fcb->fcbEOF = mdb->drCTFlSize;								//	Set catalog tree's LEOF
	fcb->fcbPLen = mdb->drCTFlSize;								//	Set catalog tree's PEOF

	vcb->catalogRefNum		= fcbIndexP;										//	Set Catalog B*-Tree file RefNum
	BlockMoveData( mdb->drCTExtRec, fcb->fcbExtRec, sizeof(SmallExtentRecord) );
	
	
	vcb->vcbMAdr	= (Ptr)*(long *)SysBMCPtr;					//	Set up the volume bitmap cache
	vcb->vcbCtlBuf	= (Ptr)*(long *)SysCtlCPtr;					//	Set up the volume control cache
	

	//--	Finish setting up the FCBs for the extent B*-Tree and the catalog B*-Tree:

	err = AccessBTree( vcb, vcb->extentsRefNum, kHFSExtentsFileID, vcb->vcbXTClpSiz, (void *) CompareExtentKeys );
	if ( err != noErr )	goto MtChkErr;
	
	err = AccessBTree( vcb, vcb->catalogRefNum, kHFSCatalogFileID, vcb->vcbCTClpSiz, (void *) CompareCatalogKeys );
	if ( err != noErr )	goto MtChkErr;

	err = CreateVolumeCatalogCache(vcb);
	if ( err )	goto MtChkErr;

	if ( !(vcb->vcbAtrb & (short)kVolumeUnmountedMask) )
	{
		UInt32		consistencyStatus;

		vcb->vcbAtrb &=	~kVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
		*(Ptr)FSCallAsync = 0;
		err = MountCheck( vcb, &consistencyStatus );
		if ( err )	goto MtChkErr;
	}
	else
	{
		vcb->vcbAtrb &=	~kVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
	}

CheckRemount:

	//	€€This comment was in TFSVol.a, should look into it
	//	Deleted stuff patched out by KillCheckRemountNiceWay in FileMgrPatches.a <SM1>


MtVolOK:
	err = noErr;

MtVolDone:
	if ( err == noErr )
	{
		if ( !(vcb->vcbAtrb & kVolumeHardwareLockMask) )		//	if the disk is not write protected
		{
			MarkVCBDirty( vcb );								//	mark VCB dirty so it will be written
			err = FlushVolumeControlBlock( vcb );				//	Go flush the VCB info BEFORE close
		}
	}
	goto	CmdDone;



MtChkErr:
	(void) DisposeVolumeCatalogCache(vcb);
	InvalidateCatalogCache( vcb );


	*((OSErr *)HFSDSErr) = err;
	err = badMDBErr;
	
	//--	Release any resources allocated so far before exiting with an error:
MtVolErr:
	DisposeFileControlBlocks( vcb );							//	deallocate control file FCBs
	
MtVolEr1:
	DisposeVolumeCacheBlocks( vcb );							//	invalidate any cache blocks for this volume
	
MtVolEr2:
	if ( (Byte)LMGetNewMount() == 0 )							//	only deallocate VCB for new mounts
	{
		//		Remount case -- Must zero buffer and map ptrs in old VCB.
		//		Restore VCB to its original off-line state (ie clear VCBDrvNum,
		//		and move drive number to VCBDRefNum field)
		vcb->vcbDRefNum	= vcb->vcbDrvNum;						//	write drv number to VCBDRefNum field
		vcb->vcbDRefNum	= 0;									//	clear drive number field of VCB
		vcb->vcbMAdr	= nil;									//	clear mapptr in VCB
		vcb->vcbBufAdr	= nil;									//	clear buffptr in VCB
	}
	else
	{
		DisposeVolumeControlBlock( vcb );
	}

	if ( err <= 0 )
		goto CmdDone;
	currentIORequestPB = (IOParam *) ((QHdr *)LMGetFSQHdr())->qHead;
	goto mv_Start;	

CmdDone:
	return( err );

}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	UnMountVolume
//
//	Function: 	All files on the volume in the drive are closed and any changed
//				directory information is written out to the diskette.  Memory
//				for the VCB, volume buffer, and block map is deallocated.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr	UnMountVolume( VolumeParam *volume, WDCBRecPtr *wdcb )
{
	FCB						*fcb;
	OSErr					err;
	SInt16					nameLength;
	Boolean					nameSpecified;
	ExtendedVCB				*vcb;
	unsigned char			*pathName;
	UInt16					index;
	Ptr						fcbs;
	Boolean					doneSearching;
	Boolean					isHFSPlus;
	FSSpec					spec;
	UInt16					driveNumber;
	MasterDirectoryBlock	*mdb;
	Boolean					mustRenameWrapperVolume = false;

	LMSetFlushOnly( 0 );										//	Setup same as UnmountVol
	err = DeturmineVolume3( volume, &nameLength, &nameSpecified, &vcb, wdcb, &pathName );
	if ( err != noErr )
		goto CmdDone;
		
	isHFSPlus	= ( vcb->vcbSigWord == kHFSPlusSigWord );
	
	//--	Get the volume name from the catalog file to set in the HFS wrapper later
	if ( (isHFSPlus) && (vcb->vcbDrvNum != 0) )					//	HFS+ volume and on-line
	{
		UInt32					hint;
		CatalogNodeData			nodeData;
		
		err = GetCatalogNode( vcb, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint );
		if ( err == noErr )
		{
			err = GetBlock_glue( gbDefault, kIDSectorOffset, (Ptr*)&mdb, vcb->vcbVRefNum, vcb );	//	Read the MDB
			if ( err == noErr )
			{
				if ( mdb->drSigWord == kHFSSigWord )			//	it's a wrapped volume
				{
					if ( memcmp( spec.name, mdb->drVN, spec.name[0]+1 ) != 0 )	//	Has the name changed?
					{
						driveNumber = vcb->vcbDrvNum;
						mustRenameWrapperVolume = true;
					}
				}
				err = RelBlock_glue( (Ptr)mdb, rbTrashMask );
			}
		}
	}
	
	//	Initialize fcbs pointer
	Get1stFileControlBlock( &index, &fcbs );

	if ( !(volume->ioTrap & kHFSMask) )							//	if NOT Unconditional unmount?
	{
		//--	Search the FCB array for open files that reference the volume...
		do
		{
			fcb = GetFileControlBlock( index );
			if ( (fcb->fcbFlNm != 0) && (fcb->fcbVPtr == vcb) )	//	The file is currently open and on this volume
			{
				if ( fcb->fcbFlNm >= fsUsrCNID )				//	is it a user file?
				{
					//--	Found an open user file on the volume, so return a busy condition...
					err = fBsyErr;
					break;
				}
			}
			
			doneSearching = GetNextFileControlBlock( &index, fcbs );
			
		} while ( !doneSearching );
	}
	
	if ( err == noErr )
	{
		err = CheckExternalFileSystem( vcb );
		if ( err == noErr )
		{
			FlushVolume( vcb );
			//	Unmounting a TFS volume: close the volume control B*-Tree and use the
			//	volume buffer to write out the MDB

			//	From UnmountForTheNineties in FileMgrPatches.a
			//	Here's the change:  we no longer check whether the volume is offline.  Instead we always
			//	go ahead and close the control files, since we no longer close them on _Offline or _Eject.
	
			if ( vcb->vcbDrvNum != 0 )						//	Check drive number: vol. offline?
			{
				err = FlushVolumeBuffers( vcb );
				if ( err != noErr )
					goto CmdDone;							//	Punt on errors
			}
			else
			{
				//	here begins CloseControlFiles, which is derived from FlushBuffers
				//	Now we have to close the control files because we no longer close them 
				//	in Eject and Offline
				
				if ( LMGetFlushOnly() == 0 )				//	volume needs flushing
				{
					err = CloseFile( vcb, vcb->catalogRefNum, fcbs );
					if ( err != noErr )
						goto CmdDone;						//	Punt on errors
					
					err = CloseFile( vcb, vcb->extentsRefNum, fcbs );
					if ( err != noErr )
						goto CmdDone;						//	Punt on errors
						
					if ( isHFSPlus )
					{
						err = AttributesCloseVolume( vcb );
						if ( err != noErr )
							goto CmdDone;

						err = CloseFile( vcb, vcb->allocationsRefNum, fcbs );
						if ( err != noErr )
							goto CmdDone;
					}

					(void) DisposeMRUCache(vcb->hintCachePtr);
					(void) DisposeVolumeCatalogCache(vcb);
					InvalidateCatalogCache( vcb );
					DisposeVolumeCacheBlocks( vcb );
				}
			}

			if ( LMGetFlushOnly() == 0 )					//	volume needs flushing
			{
				//	For UnMountVol, dequeue the VCB and trash any WDCBs for this volume ...
				DisposeVolumeControlBlock( vcb );
			}
			
			//	The volume is now unmounted, if it's an HFS+ volume, and the name does not match what's
			//	in the MDB, mount the HFS wrapper rename the wrapper volume and unmount it.
			if ( mustRenameWrapperVolume )
			{
				RenameWrapperVolume( spec.name, driveNumber );
			}
			
			err = noErr;
		}
	}

CmdDone:
	return( err );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	RenameWrapperVolume	
//
//	Function: 	Called during UnmountVolume if the HFS+ volume name does not
//				match the name stored in the wrapper MDB.  This function
//				renames the HFS wrapper volume.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void	RenameWrapperVolume( Str27 newVolumeName, UInt16 driveNumber )
{
	VolumeParam		volume;
	ExtendedVCB		*vcb;
	OSErr			err;
	UInt32			hint;
	FSSpec			spec;
	CatalogNodeData	nodeData;
	Ptr				pmsp;
	UInt32			modDate;
	WDCBRecPtr		wdcbPtr;
	
	ClearMemory( &volume, sizeof(VolumeParam) );
	volume.ioVRefNum	= driveNumber;
	
	err = MountVolume( &volume, true );
	if ( err != noErr )
		return;		// punt on errors
	
	err = GetVCBDriveNum( &vcb, driveNumber );
	if ( err != noErr )
		goto ErrorExit;		// punt on errors

	modDate	= vcb->vcbLsMod;
	
	err = GetCatalogNode( vcb, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint );
	if ( err != noErr )
	{
		#if DEBUG_BUILD
			DebugStr("\p GetCatalogNode failed");
		#endif
		goto ErrorExit;		// punt on errors
	}
	
	err = RenameCatalogNode( vcb, kHFSRootParentID, spec.name, newVolumeName, hint, &hint );
	if ( err != noErr )
	{
		#if DEBUG_BUILD
			DebugStr("\p RenameCatalogNode failed");
		#endif
		goto ErrorExit;		// punt on errors
	}

	BlockMoveData( newVolumeName, vcb->vcbVN, sizeof(Str27) );
	vcb->vcbLsMod	= modDate;
	MarkVCBDirty( vcb );

	pmsp	= LMGetPMSPPtr();
	*((short*)(pmsp + kPoorMansSearchIndex))	= 0;			//	Invalidate Poor Mans search path

ErrorExit:

	(void) UnMountVolume( &volume, &wdcbPtr );
}




//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CheckExternalFileSystem		Asm: CkExtFS
//
//	Function: 	Simple routine to separate out calls to be handled by an
//				external file system.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	CheckExternalFileSystem( ExtendedVCB *vcb )
{
	if ( vcb->vcbFSID == 0 )
	{
		return( noErr );
	}
	else
	{
		return( extFSErr );
	}
}



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CheckVolumeConsistancy
//
//	Function: 	Check kBootVolumeInconsistentMask, and run MountCheck if necessary
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	CheckVolumeConsistency( ExtendedVCB *vcb )
{
	UInt32		consistencyStatus;
	OSErr		err	= noErr;
		
	if ( ( *((UInt16*)&(((ROMHeaderRec *) LMGetROMBase())->MachineNumber)) == 0x077D ) &&	//	is supermario based
		 ( vcb->vcbSigWord == kHFSSigWord ) )					//	and is an HFS disk
	{
		if ( vcb->vcbAtrb & kBootVolumeInconsistentMask )		//	was marked inconsistant
		{
			err = MountCheck( vcb, &consistencyStatus );
			
			if ( err == noErr )
				vcb->vcbAtrb &= ~kVolumeSoftwareLockMask;		//	clear the bit
		}

		if ( LMGetBootDrive() == vcb->vcbDrvNum )				//	this is the boot volume
			vcb->vcbAtrb |= kBootVolumeInconsistentMask;
	}
	
	return( err );
}




//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	VolumeWritable		Asm: CVFlgs
//
//	Function: 	Check the volume's flags to see if modify requests are allowed.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	VolumeWritable( ExtendedVCB *vcb )
{
	if ( !(vcb->vcbAtrb & 0x8000) )		//	if the volume is not locked
	{
		if ( ! (*((Ptr)&(vcb->vcbAtrb) + 1) & kVolumeHardwareLockMask) )	//	if it's not write protected
			return( noErr );
		else
			return( wPrErr );
	}
	else
	{
		return( vLckdErr );
	}
}






//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	FlushVolume		Asm: FlushVFiles
//
//	Function: 	All file buffers on the volume are flushed and any changed
//				directory information is written out to the disk.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	FlushVolume( ExtendedVCB *vcb )
{
	UInt16			index;
	Ptr				fcbs;
	FCB				*fcb;
	OSErr			err;
	Boolean			doneSearching;
	
	Get1stFileControlBlock( &index, &fcbs );

//ckNxtFCB
	err = noErr;
	do
	{
		fcb = GetFileControlBlock( index );
		if ( (fcb->fcbFlNm != 0) && (fcb->fcbVPtr == vcb) )	//	The file is currently open and on this volume
		{
			if ( fcb->fcbFlNm >= fsUsrCNID )				//	is it a user file?
			{
				//--	Found an open user file on the volume, so return a busy condition...
				err = CloseFile( vcb, index, fcbs );
				if ( err != noErr )
					break;
			}
		}
		
		doneSearching = GetNextFileControlBlock( &index, fcbs );
		
	} while ( !doneSearching );
	
	return( err );
}



//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	DisposeFileControlBlocks from Asm: DsposFCBs
//
//	Function: 	Clears the extent B*-Tree refNum & catalog refnum
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void	DisposeFileControlBlocks( ExtendedVCB *vcb )
{
	OSErr	err;
	
	err = AttributesCloseVolume( vcb );
#if DEBUG_BUILD
	if ( err != noErr ) DebugStr("\pHFS+: Can't close Property Manager");
#endif

	if ( vcb->extentsRefNum != 0 )
	{
		//--	Clear extent B*-Tree refNum file number in FCB as unused
		( (FCB *) ( LMGetFCBSPtr() + vcb->extentsRefNum ) )->fcbFlNm = 0L;
	}
	if ( vcb->catalogRefNum != 0 )
	{
		//--	Clear catalog refnum file number in FCB as unused
		( (FCB *) ( LMGetFCBSPtr() + vcb->catalogRefNum ) )->fcbFlNm = 0L;
	}
	
	if ( vcb->vcbSigWord == kHFSPlusSigWord && vcb->allocationsRefNum != 0)
	{
		//--	Clear allocation (bitmap) file number in FCB as unused
		( (FCB *) ( LMGetFCBSPtr() + vcb->allocationsRefNum ) )->fcbFlNm = 0L;
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	AccessBTree from Asm: AccessBT
//
//	Function:	Create an access path for a B*-Tree file:
//				1. Initialize the FCB
//				2. Attach a B*-Tree control block
//
//	Note: 		For now the compare proc is the entry to the assembly glue calling a C compare function.
//				Once BTOpen is ported to C, we can pass in C comparison routines.
//
// 	Result:		I/O result from BTOpen
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	AccessBTree( ExtendedVCB *vcb, SInt16 refNum, UInt32 fileID, UInt32 fileClumpSize, void *CompareRoutine )
{
	FCB				*fcb;
	OSErr			err;

	fcb = SetupFCB( vcb, refNum, fileID, fileClumpSize );
	
	err = OpenBTree( refNum, CompareRoutine );
	
	return( err );
}

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	SetupFCB fills in the FCB info
//
//	Returns:	The filled up FCB
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
FCB	*SetupFCB( ExtendedVCB *vcb, SInt16 refNum, UInt32 fileID, UInt32 fileClumpSize )
{
	FCB				*fcb;

	fcb = (FCB *) GetFileControlBlock( refNum );
	
	fcb->fcbFlNm		= fileID;
	fcb->fcbVPtr		= vcb;
	fcb->fcbClmpSize	= fileClumpSize;
	
	// for debugging...	
	switch (fileID)
	{
		case kHFSExtentsFileID:
			BlockMoveData( "\p**** EXTENTS B-TREE", fcb->fcbCName, sizeof("\p**** EXTENTS B-TREE") );
			break;
		case kHFSCatalogFileID:
			BlockMoveData( "\p**** CATALOG B-TREE", fcb->fcbCName, sizeof("\p**** CATALOG B-TREE") );
			break;
		case kHFSAllocationFileID:
			BlockMoveData( "\p**** VOLUME BITMAP", fcb->fcbCName,  sizeof("\p**** VOLUME BITMAP") );
			break;
		case kHFSAttributesFileID:
			BlockMoveData( "\p**** ATTRIBUTES B-TREE", fcb->fcbCName,  sizeof("\p**** ATTRIBUTES B-TREE") );
			break;
	}
	
	return( fcb );
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	FindFileControlBlock from Asm: FindFCB
//
//	Function:	Find an unused FCB
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	FindFileControlBlock( UInt16 *index, Ptr *fcbsH )
{
	Boolean			doneSearching;
	OSErr			err;
	FCB				*fcb;
	
	Get1stFileControlBlock( index, fcbsH );
	
	do
	{
		doneSearching = GetNextFileControlBlock( index, *fcbsH );
		if ( doneSearching == false )
		{
			fcb = GetFileControlBlock( *index );
			if ( fcb->fcbFlNm == 0 )										//	FCB unused?
			{
				ClearMemory( fcb, sizeof( FCB ) );
				fcb->fcbFlNm = -1;											//	Mark it in use
				err = noErr;
				doneSearching = true;
			}
		}
		else
		{
			err = tmfoErr;
		}
	}	while ( doneSearching == false );
	
	return( err );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	Get1stFileControlBlock from Asm: Gt1stFCB
//
//	Function:	Point to the first FCB
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void	Get1stFileControlBlock( UInt16 *index, Ptr *fcbsH )
{
	//		struct FCBSRec {
	//			UInt16					length;								//	first word is FCB part length
	//			FCB						fcb[1];								//	fcb array
	//		};
	*fcbsH = LMGetFCBSPtr();
	*index = 2;
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetNextFileControlBlock from Asm: GtNxtFCB
//
//	Function:	Increment to the next FCB
//					Check if we've hit the end fcb yet
//
// 	Result:		Return true if end was reached
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
Boolean	GetNextFileControlBlock( UInt16 *index, Ptr fcbsP )
{

	*index += LMGetFSFCBLen();
	 
	return ( *index == *((UInt16 *)fcbsP) );								//	first word is length
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	DivUp from Asm: DivUp
//
//	Function:	Given a number of bytes and block size, calculate the number of
//				blocks needd to hold all the bytes.
//
// 	Result:		Number of physical blocks needed
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
UInt16	DivUp( UInt32 byteRun, UInt32 blockSize )
{
	UInt32	blocks;
	
	blocks = (byteRun + blockSize - 1) / blockSize;							//	Divide up, remember this is integer math.
	
	if ( blocks > 0xffff )													//	maximum 16 bit value
		blocks = 0xffff;
		
	return( (UInt16) blocks );
}


#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	HFSBlocksFromTotalSectors
//
//	Function:	Given the total number of sectors on the volume, calculate
//				the 16Bit number of allocation blocks, and allocation block size.
//
// 	Result:		none
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
void	HFSBlocksFromTotalSectors( UInt32 totalSectors, UInt32 *blockSize, UInt16 *blockCount )
{
	UInt16	newBlockSizeInSectors	= 1;
	UInt32	newBlockCount			= totalSectors;
	
	while ( newBlockCount > 0XFFFF )
	{
		newBlockSizeInSectors++;
		newBlockCount	=  totalSectors / newBlockSizeInSectors;
	}
	
	*blockSize	= newBlockSizeInSectors * 512;
	*blockCount	= newBlockCount;
}


#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	ValidMasterDirectoryBlock
//
//	Function:	Run some sanity checks to make sure the MDB is valid
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	ValidMasterDirectoryBlock( MasterDirectoryBlock *mdb )
{
	OSErr	err;
	
	if ( (mdb->drSigWord == kHFSPlusSigWord) || (mdb->drSigWord == kHFSSigWord) )	//	if HFS or HFS Plus volume
	{
		if ( (mdb->drAlBlkSiz != 0) && ((mdb->drAlBlkSiz & 0x01FF) == 0) )			//	non zero multiple of 512
			err = noErr;
		else
			err = badMDBErr;
	}
	else
	{
		err = noMacDskErr;
	}
	
	return( err );
}

#endif

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	ValidVolumeHeader
//
//	Function:	Run some sanity checks to make sure the VolumeHeader is valid
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	ValidVolumeHeader( VolumeHeader *volumeHeader )
{
	OSErr	err;
	
	if ( volumeHeader->signature == kHFSPlusSigWord && volumeHeader->version == kHFSPlusVersion )
	{
		if ( (volumeHeader->blockSize != 0) && ((volumeHeader->blockSize & 0x01FF) == 0) )			//	non zero multiple of 512
			err = noErr;
		else
			err = badMDBErr;							//€€	I want badVolumeHeaderErr in Errors.i
	}
	else
	{
		err = noMacDskErr;
	}
	
	return( err );
}

#ifdef INVESTIGATE

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	FindDrive from Asm: FindDrive
//
//	Function:	Given a drive number, this routine returns the RefNum
//				for the driver by searching the system drive queue.
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	FindDrive( short *driverRefNum, DrvQEl **dqe, short driveNumber )
{
	OSErr	err		=	nsDrvErr;						//	initialize as "drive not found"
	
	*dqe = ((DrvQEl*)LMGetDrvQHdr()->qHead);			//	get first DQE
	while ( *dqe != 0 )									//	end of list?
	{
		if ( (**dqe).dQDrive != driveNumber )			//	find the drive we're looking for?
		{
			*dqe = (DrvQEl*)(**dqe).qLink;				//	get ptr to next
		}
		else
		{
			*driverRefNum = (**dqe).dQRefNum;			//	return driver refnum
			if ( (**dqe).dQFSID == 0 )					//	is this a Mac Filesystem?
				err = noErr;
			else
				err = extFSErr;							//	no, return error
			break;
		}
	}
	
	return( err );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetVCBDriveNum from Asm: GetVCBDrv
//
//	Function:	Determine VCB from DriveNum/VRefNum field
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	GetVCBDriveNum( ExtendedVCB **vcbResult, short driveNumber )
{
	ExtendedVCB	*vcb;
	
	for ( vcb = (ExtendedVCB*)((LMGetVCBQHdr())->qHead) ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
	{
		if ( vcb->vcbDrvNum == driveNumber )					//	success!
			break;
		if ( vcb->vcbDrvNum == 0 )								//	offline
		{
			if ( vcb->vcbDRefNum == driveNumber * (-1) )		//	offline match
				break;
		}
	}
	
	*vcbResult = vcb;

	if ( vcb == nil )
		return( nsvErr );
	else
		return( noErr );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	GetVCBRefNum from Asm: GetVCBRfn
//
//	Function:	Determine VCB from DriveNum/VRefNum field
//
// 	Result:		error
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
OSErr	GetVCBRefNum( ExtendedVCB **vcbResult, short vRefNum )
{
	ExtendedVCB	*vcb;
	
	for ( vcb = (ExtendedVCB*)((LMGetVCBQHdr())->qHead) ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
	{
		if ( vcb->vcbVRefNum == vRefNum )					//	success!
		{
			*vcbResult = vcb;
			return( noErr );
		}
	}
	
	return( nsvErr );
}

//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	IsARamDiskDriver from Asm: IsRAMDiskDriver
//
//	Function:	Test if disk driver is a RAM disk
//
// 	Result:		true if RAM disk
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
Boolean	IsARamDiskDriver( void )
{
	OSErr			err;
	ParamBlockRec	*pb;
	Byte			diskType;
	
	pb = (ParamBlockRec *) LMParamBlock;					//	general purpose I/O param block
	pb->ioParam.ioCompletion	= 0;						//	no completion routine
	pb->cntrlParam.csCode		= infoCC;					//	pass control code
	pb->cntrlParam.csParam[0]	= 0;						//	start out clean
	pb->cntrlParam.csParam[1]	= 0;
	
	err = PBControlSync( pb );								//	get driver infoŠ
	
	if ( err == noErr )										//	this control call supported
	{
		diskType = ((Byte *)(pb->cntrlParam.csParam))[3];
		if ( (diskType == ramDiskType) || (diskType == romDiskType) )
			return( true );
	}
	
	return( false );
}


OSErr	MakeVCBsExtendedVCBs( void )
{
	VCB						*oVCB;
	VCB						*nextVCB;
	ExtendedVCB				*exVCB;
	ExtendedVCB				*firstNewVCBInQueue	= nil;
	QHdr					*vcbQHdr;
	UInt16					fcbIndex;
	UInt16					i;
	Ptr						fcbs;
	FCB						*fcb;
	WDCBArray				*wdcbsAry;
	OSErr					err;
		
	vcbQHdr	= LMGetVCBQHdr();												//	search the queue of VCBs

	for ( oVCB = (VCB *)vcbQHdr->qHead ; (oVCB != nil) && (oVCB != (VCB *)firstNewVCBInQueue) ; )
	{
		nextVCB = (VCB *) oVCB->qLink;
		
		if ( (oVCB->vcbSigWord != kHFSPlusSigWord) && (oVCB->vcbFSID == 0) )	//	if it's an HFS Plus VCB, assume it is already of ExtendedVCB type
		{
			exVCB = (ExtendedVCB *) NewPtrSysClear( sizeof(ExtendedVCB) );	//	allocate space for the new ExtendedVCB
			ReturnIfError( MemError() );
			BlockMoveData( oVCB, exVCB, sizeof(VCB) );						//	copy the original VCB over
			
			if ( firstNewVCBInQueue == nil )
				firstNewVCBInQueue = exVCB;
			
			//--	Set up the Extended fields, used internally in the HFS source
			exVCB->totalBlocks		=	exVCB->vcbNmAlBlks;
			exVCB->freeBlocks		=	exVCB->vcbFreeBks;
			exVCB->blockSize		=	exVCB->vcbAlBlkSiz;
			exVCB->nextAllocation	=	(UInt16)exVCB->vcbAllocPtr;			//	Almost certain to be zero
			// Note: SuperMario ROMs clear vcbAllocPtr and then flush the MDB.
			// So, when an HFS disk gets mounted, the allocation pointer is cleared both in
			// memory and on disk.  The only way to fix this is to put HFS Plus in ROM.

			//--	Set up the caches for this volume
 			err = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(exVCB->hintCachePtr) );
			ReturnIfError( err );
			
			err = CreateVolumeCatalogCache(exVCB);
			ReturnIfError( err );
			
			//--	If we are dealing with the default VCB, set the corresponding ExtendedVCB as the default
			if ( oVCB == (VCB*) LMGetDefVCBPtr() )
				LMSetDefVCBPtr( (Ptr) exVCB );
			
			//--	Remove and deallocate the original VCB
			err = Dequeue( (QElemPtr) oVCB, (QHdrPtr) vcbQHdr );			//	Dequeue the VCB from the VCB Queue
			ReturnIfError( err );
			
			//--	Search the FCB list for refs to the old VCB pointer and update them
			Get1stFileControlBlock( &fcbIndex, &fcbs );
			do
			{
				fcb = GetFileControlBlock( fcbIndex );
				if ( fcb->fcbVPtr == (ExtendedVCB *) oVCB )	
					fcb->fcbVPtr = exVCB;
			} while ( GetNextFileControlBlock( &fcbIndex, fcbs ) == false );//	while there's still some left
			
			//--	Search the WDCB list for refs to the old VCB pointer and update them
			wdcbsAry = (WDCBArray*)LMGetWDCBsPtr();							//	Point to WDCB array
			for ( i=0 ; i< (wdcbsAry->count / sizeof(WDCBRec)) ; i++ )
			{
				if ( wdcbsAry->wdcb[i].wdVCBPtr == oVCB )
					wdcbsAry->wdcb[i].wdVCBPtr = (VCB *) exVCB;
			}
			
			//--	Deallocate memory used by the old VCB
			DisposePtr( (Ptr) oVCB );

			//--	Insert the new extended vcb in the queue
			Enqueue( (QElemPtr) exVCB, (QHdrPtr) vcbQHdr );
		}
		oVCB = nextVCB;
	}
	
	return( noErr );
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	RemountWrappedVolumes
//
//	Function:	Called by InitHFSPlus, Attempt to re-mount all HFS+ volumes
//				previously mounted as HFS wrappers.
//
//	Note:		This routine assumes vcbEmbedSigWord is cleared during mount.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
extern	void	RemountWrappedVolumes( void )
{
	ExtendedVCB		*vcb;
	ExtendedVCB		*nextVCB;
	OSErr			err;

	for ( vcb = (ExtendedVCB *) ((QHdr*)LMGetVCBQHdr())->qHead ; vcb != nil ; )
	{
		//	PBMountVol and UnmountVol modify the vcb queue, so save the link
		nextVCB = (ExtendedVCB *) vcb->qLink;

		if ( (vcb->vcbEmbedSigWord == kHFSPlusSigWord) && ( vcb->vcbFSID == 0) )
		{	
			err = UnmountVol( nil, vcb->vcbVRefNum );
			if ( err == noErr )
			{
				ParamBlockRec theParam;
				
				theParam.ioParam.ioCompletion	= nil;
				theParam.ioParam.ioNamePtr		= nil;
				theParam.ioParam.ioVRefNum		= vcb->vcbDrvNum;
				(void) PBMountVol( &theParam );
			}
		}
		
		vcb = nextVCB;
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	FillHFSStack
//
//	Function:	Called by HFSInit.  Fills the newly created HFS stack so we 
//				can identify how much of the stack is actually being used.
//
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
extern	void	FillHFSStack( void )
{
	Ptr		p;
	SInt16	len		=	0;
	char	*s		=	"HFS+ By Don Brady, Mark Day, and Deric Horn.   ";
	
	while ( s[len] != '\0' )
		++len;
		
	for ( p = LMGetHFSStkTop() - kFileSystemStackSize ; p + len < LMGetHFSStkTop() ; p += len )
	{
		BlockMoveData( s, p, len );
	}
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//	Routine:	CheckVolumeOffLine from Asm: ExtOffLinCk
//
//	Function:	Simple routine to separate out calls to be handled by an
//				external file system.
//
// 	Result:		noErr if we handle it
//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//€€ Same function appears in CatSearch.c as ExtOffLineCheck(), but the code is slightly different
//€€ we should delete one copy.
OSErr	CheckVolumeOffLine( ExtendedVCB *vcb )
{
	OSErr		err;
	
	err = CheckExternalFileSystem( vcb );
	if ( err == noErr && vcb->vcbDrvNum == 0 )
		err = volOffLinErr;
	
	LMSetReqstVol( (Ptr)vcb );
	
	return( err );
}




//_______________________________________________________________________
//
//	Routine:	CountRootFiles
//
//	Input:		pointer to VCB
//
//	Function: 	Return a count of the number of files and folders in
//				the root directory of a volume.  For HFS volumes, this
//				is maintained in the VCB (and MDB).  For HFS Plus volumes,
//				we get the valence of the root directory from its catalog
//				record.
//_______________________________________________________________________

static UInt16 CountRootFiles(ExtendedVCB *vcb)
{
	OSErr			err;
	FSSpec			spec;
	CatalogNodeData	catNodeData;
	UInt32			hint;
	UInt16			rootCount;
	
//	if (vcb->vcbSigWord == kHFSSigWord || vcb->vcbFSID != 0) {
//		return vcb->vcbNmFls;
//	}
	
	//	Here, it's an HFS Plus volume, so get the valence from the root
	//	directory's catalog record.
	
	rootCount = 0;
	
	err = GetCatalogNode( vcb, kHFSRootFolderID, nil, kNoHint, &spec, &catNodeData, &hint );
	if ( err == noErr ) {
		if (catNodeData.valence < 65536)
			rootCount = catNodeData.valence;
		else
			rootCount = 65535;			//	if the value is too large, pin it
	}

	return rootCount;
}



//_______________________________________________________________________
//
//	Routine:	GetVolumeInformation	:	GetVolInfo
//	Arguments:	HVolumeParam	*volume
//	Output:		WDCBRec			**wdcb		(Needed for ExtFsHook in CmdDone)
//				OSErr			err
//
//	Function: 	Return information about the volume in a mounted drive.
//				If the IOVolIndex field is 0, the name of the default
//				volume is returned; if non-zero, the name of the nth
//				mounted volume is returned. The maximum length of a
//				volume name is 27 bytes.  The drive number for the
//				volume is also returned.
//_______________________________________________________________________

OSErr	GetVolumeInformation( HVolumeParam *volume, WDCBRecPtr *wdcb )
{
	FSSpec			spec;
	UInt32			maxBlocks;
	SInt16			nameLength;
	SInt16			volumeIndex;
	SInt16			vRefNum;
	OSErr			err;
	Ptr				fcbs;
	FCB				*fcb;
	ExtendedVCB		*vcb;
	unsigned char	*pathName;
	CatalogNodeData	catNodeData;
	Boolean			doneSearching;
	Boolean			nameSpecified;
	WDCBArray		*wdcbsAry;
	UInt16			fcbIndex;

	volumeIndex = volume->ioVolIndex;
	if ( volumeIndex == 0 )
	{
		err = DeturmineVolume1( (VolumeParam*)volume, &nameLength, &nameSpecified, (ExtendedVCB **)&vcb, wdcb, &pathName );
		if ( err != noErr )	goto CmdDone;
	}
	else if ( volume->ioVolIndex < 0 )
	{
		err = DeturmineVolume3( (VolumeParam*)volume, &nameLength, &nameSpecified, (ExtendedVCB **)&vcb, wdcb, &pathName );
		if ( err != noErr )	goto CmdDone;
	}
	else
	{
		nameSpecified = false;							//	Never any name specified (ioNamePtr is output arg)
		volume->ioVRefNum = 0;

		for ( vcb = (ExtendedVCB*)((LMGetVCBQHdr())->qHead) ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
		{
			if ( --volumeIndex == 0 )
				break;
		}
		if ( vcb == nil )
		{
			err = nsvErr;
			goto CmdDone;
		}
	}

	//	first copy the heart of the VCB into the parameter block
	volume->ioVCrDate	=	vcb->vcbCrDate;
	volume->ioVLsMod	=	vcb->vcbLsMod;
	volume->ioVAtrb		=	vcb->vcbAtrb;
	volume->ioVNmFls	=	vcb->vcbNmFls;				// don't call CountRootFiles(vcb) anymore
	volume->ioVBitMap	=	vcb->vcbVBMSt;
	volume->ioVNmAlBlks	=	vcb->vcbNmAlBlks;			//	we return the HFS volume format equivalents cuz ioVNmAlBlks is a short
	volume->ioVAlBlkSiz	=	vcb->vcbAlBlkSiz;
	volume->ioVClpSiz	=	vcb->vcbClpSiz;
	volume->ioAlBlSt	=	vcb->vcbAlBlSt;
	volume->ioVNxtCNID	=	vcb->vcbNxtCNID;
	volume->ioVFrBlk	=	vcb->vcbFreeBks;
	
	if ( vcb->vcbFSID != 0 )
		volume->ioAllocPtr	=	vcb->vcbAllocPtr;		// external file system
	else if ( vcb->nextAllocation < 0xFFFF )
		volume->ioAllocPtr	=	vcb->nextAllocation;
	else
		volume->ioAllocPtr	=	0xFFFF;
	

	Get1stFileControlBlock( &fcbIndex, &fcbs );
	doneSearching		=	false;

	do
	{
		fcb = GetFileControlBlock( fcbIndex );
		if ( fcb->fcbVPtr == (ExtendedVCB *) vcb )		//	The file is currently open and on this volume
		{
			if ( fcb->fcbFlNm >= fsUsrCNID )
				break;
		}
		doneSearching = GetNextFileControlBlock( &fcbIndex, fcbs );
	} while ( doneSearching == false );
	
	if ( doneSearching == true )
		volume->ioVAtrb		&=	~kFilesOpenMask;		//	Clear if no open files match
	else
		volume->ioVAtrb		|=	kFilesOpenMask;			//	Set if any files are opened

	if ( vcb == (ExtendedVCB *) LMGetDefVCBPtr() )		//	Are we supposedly the default?
		volume->ioVAtrb		|=	kDefaultVolumeMask;		//	Set volume as the default
	else
		volume->ioVAtrb		&=	~kDefaultVolumeMask;	//	Clear volume as the default
	
	*wdcb = nil;
	
	if ( (volume->ioVolIndex <= 0) && (nameSpecified == 0) )
	{
		vRefNum = volume->ioVRefNum;
		if ( vRefNum > 0 )								//	If driveNum, ignore it
		{
			goto	retVRef;
		}
		else if ( vRefNum == 0 )
		{
			vRefNum = 2;
		}
		else if ( vRefNum <= WDRfnMax )
		{
			//	a WDRefNum was supplied as VRefNum: change the number of files to be the	<21Sep85>
			//	valence of the directory in question: 										<21Sep85>
			vRefNum -= WDRfnMin;						//	Change WDRefNum to index
		}
		else
		{
			goto retVRef;
		}
		
		wdcbsAry = (WDCBArray*)LMGetWDCBsPtr();			//	Point to WDCB array
		*wdcb = (WDCBRec*)((Ptr)wdcbsAry + vRefNum);

		err = CheckVolumeOffLine( vcb );				//	check external FS err
		if ( err == noErr )
		{
			if ( (**wdcb).wdDirID != fsRtDirID )
			{
				//€€	This will change to handle Don's meta struct
				err = GetCatalogNode( (ExtendedVCB*)vcb, (**wdcb).wdDirID, nil, (**wdcb).wdCatHint, &spec, &catNodeData, (UInt32*)&((**wdcb).wdCatHint) );
				if ( err == noErr )
				{
					volume->ioVNmFls = (UInt16) catNodeData.valence;
				}
			}
		}
	}
	
	//	Do some hairy checking: if a working directory was specified, it shouldn't
	//	be overwritten by the volRefNum.	However, if a volume NAME was used, the
	//	volRefNum field is ignored, and SHOULD be filled in with the volume's VRefNum.
retVRef:
	if( (nameSpecified != 0) || (volume->ioVRefNum >= 0) )
	{
		volume->ioVRefNum = vcb->vcbVRefNum;
	}
	//	Copy the volume name field if specified.
	if ( volume->ioNamePtr != 0 )
		BlockMoveData( vcb->vcbVN, volume->ioNamePtr, vcb->vcbVN[0]+1 );


	//	Many applications call GetVolInfo for preflighting when saving files. However,
	//	they tend to use signed integers when calculating the volume's free space
	//	(vcbFreeBlks * vcbAlBlkSize).  If the free space is greater than 2 gigabytes they
	//	get a negative result and think there is not enough space left on the volume.
	//	
	//	To fix this problem we high water mark vcbFreeBlks and vcbNmAlBlks so that when
	//	multiplied by vcbAlBlkSiz the result is always less than or equal to 2GB.

	if ( volume->ioVAlBlkSiz > 0x8000 )					//	Do the mock up
	{
		//	Calculate the maximum number of allocation blocks that can fit within 2 gigabytesŠ		<LgVol>
		//	Divide 2 gig by block allocation size
		maxBlocks = 0x7FFFFFFF / volume->ioVAlBlkSiz;
		
		//	Clip total and free to the maximumŠ	
		if ( volume->ioVNmAlBlks > maxBlocks )
		{
			volume->ioVNmAlBlks = maxBlocks;			//	pin the value limit
			
			if ( volume->ioVFrBlk > maxBlocks )
			{
				volume->ioVFrBlk = maxBlocks;
			}
		}
	}
	
	//	Subtract the physical sizes of the extent B*-Tree and the catalog B*-Tree <21Sep85>
	//	from the volume size, so that the difference between the volume size and	<21Sep85>
	//	the free space reflects only the user's allocations:                      <21Sep85>
	volume->ioVNmAlBlks	-= vcb->vcbXTAlBlks;			//	Take out XT allocation
	volume->ioVNmAlBlks	-= vcb->vcbCTAlBlks;			//	Take out CT allocation
	
	if ( !(volume->ioTrap & kHFSMask) )					//	TFGetVolInfo vs. plain GetVolInfo?
	{													//	GetVolInfo
		if ( volume->ioVNmAlBlks > kMaxHFSAllocationBlocks )
			volume->ioVNmAlBlks = kMaxHFSAllocationBlocks;
		
		if ( volume->ioVFrBlk > kMaxHFSAllocationBlocks )
			volume->ioVFrBlk = kMaxHFSAllocationBlocks;
	}
	else		//	Return some additional information for TGetVolInfo requests:
	{
		volume->ioVRefNum	=	vcb->vcbVRefNum;		//	ALWAYS return VRefNum
		volume->ioVDrvInfo	=	vcb->vcbDrvNum;			//	Driver number
		volume->ioVDRefNum	=	vcb->vcbDRefNum;		//	Driver RefNum
		volume->ioVFSID		=	vcb->vcbFSID;			//	File System ID
		
		//	We return  kHFSSigWord for HFS+ volumes for copatability with
		//	existing applications.  All StuffIt products, AppleShare, FileSharing,
		//	and others incorrectly assume the volume is unsupported or MFS if the
		//	sigWord is not 'BD'. GetXVolInfo() returns the "real" value.
		volume->ioVSigWord	= vcb->vcbSigWord == kHFSPlusSigWord ? kHFSSigWord : vcb->vcbSigWord;
		
		
//volume->ioVFilCnt	=	0;								//	Clear
//volume->ioVFilCnt	=	vcb->vcbNmFls;					//	Stuff low word as if MFS
		
		volume->ioVBkUp		=	vcb->vcbVolBkUp;		//	Last backup date
		volume->ioVSeqNum	=	vcb->vcbVSeqNum;		//	Volume sequence number
		volume->ioVWrCnt	=	vcb->vcbWrCnt;			//	Volume write count
		volume->ioVFilCnt	=	vcb->vcbFilCnt;			//	Files count for volume
		volume->ioVDirCnt	=	vcb->vcbDirCnt;			//	Directory count for volume

		BlockMoveData( vcb->vcbFndrInfo, volume->ioVFndrInfo, sizeof( SInt32 ) * 8 );
	}

GVIDone:
	LMSetReqstVol( (Ptr)vcb );

	err = CheckExternalFileSystem( vcb );
	
CmdDone:
	return( err );

}




//_______________________________________________________________________
//
//	Routine:	GetXVolumeInformation	:	GetXVolInfo
//	Arguments:	XVolumeParam	*volume
//	Output:		WDCBRecPtr		*wdcb		(Needed for ExtFsHook in CmdDone)
//				OSErr			err
//
//	Function: 	Return information about the volume in a mounted drive.
//				If the IOVolIndex field is 0, the name of the default
//				volume is returned; if non-zero, the name of the nth
//				mounted volume is returned. The maximum length of a
//				volume name is 27 bytes.  The drive number for the
//				volume is also returned.
//_______________________________________________________________________

OSErr	GetXVolumeInformation( XVolumeParam *volume, WDCBRecPtr *wdcb )
{
	FSSpec			spec;
	SInt16			nameLength;
	SInt16			volumeIndex;
	SInt16			vRefNum;
	OSErr			err;
	Ptr				fcbs;
	FCB				*fcb;
	ExtendedVCB		*vcb;
	unsigned char	*pathName;
	CatalogNodeData	catNodeData;
	Boolean			doneSearching;
	Boolean			nameSpecified;
	WDCBArray		*wdcbsAry;
	UInt16			fcbIndex;

// The Installer doesn't setup ioXVersion (radar #1666217)
#if 0
	if (volume->ioXVersion != 0)
		return paramErr;
#endif

	err = noErr;		//	Assume things will work
	
	volumeIndex = volume->ioVolIndex;
	if ( volumeIndex == 0 )
	{
		err = DeturmineVolume1( (VolumeParam*)volume, &nameLength, &nameSpecified, (ExtendedVCB **)&vcb, wdcb, &pathName );
		if ( err != noErr )	goto CmdDone;
	}
	else if ( volume->ioVolIndex < 0 )
	{
		err = DeturmineVolume3( (VolumeParam*)volume, &nameLength, &nameSpecified, (ExtendedVCB **)&vcb, wdcb, &pathName );
		if ( err != noErr )	goto CmdDone;
	}
	else
	{
		nameSpecified = false;							//	Never any name specified (ioNamePtr is output arg)
		volume->ioVRefNum = 0;

		for ( vcb = (ExtendedVCB*)((LMGetVCBQHdr())->qHead) ; vcb != nil ; vcb = (ExtendedVCB*)vcb->qLink )	//	search the queue of VCBs
		{
			if ( --volumeIndex == 0 )
				break;
		}
		if ( vcb == nil )
		{
			err = nsvErr;
			goto CmdDone;
		}
	}

	//	first copy the heart of the VCB into the parameter block
	volume->ioVCrDate	=	vcb->vcbCrDate;
	volume->ioVLsMod	=	vcb->vcbLsMod;
	volume->ioVAtrb		=	vcb->vcbAtrb;
	volume->ioVBitMap	=	vcb->vcbVBMSt;
	volume->ioVNmAlBlks	=	vcb->vcbNmAlBlks;			//	16 bit value
	volume->ioVClpSiz	=	vcb->vcbClpSiz;
	volume->ioAlBlSt	=	vcb->vcbAlBlSt;
	volume->ioVNxtCNID	=	vcb->vcbNxtCNID;
	volume->ioVFrBlk	=	vcb->vcbFreeBks;
	volume->ioVNmFls	=	vcb->vcbNmFls;				// don't call CountRootFiles(vcb) anymore

	if ( vcb->vcbFSID == 0 )	// an HFS or HFS Plus disk
	{
		volume->ioVAlBlkSiz	= vcb->blockSize;			//	return true allocation blocks

		if ( vcb->nextAllocation < 0xFFFF )
			volume->ioAllocPtr	=	vcb->nextAllocation;
		else
			volume->ioAllocPtr	=	0xFFFF;
	}
	else	//	an external file system
	{
		volume->ioVAlBlkSiz	=	vcb->vcbAlBlkSiz;			
		volume->ioAllocPtr	=	vcb->vcbAllocPtr;
	}


	Get1stFileControlBlock( &fcbIndex, &fcbs );
	doneSearching		=	false;

	do
	{
		fcb = GetFileControlBlock( fcbIndex );
		if ( fcb->fcbVPtr == vcb )						//	The file is currently open and on this volume
		{
			if ( fcb->fcbFlNm >= fsUsrCNID )
				break;
		}
		doneSearching = GetNextFileControlBlock( &fcbIndex, fcbs );
	} while ( doneSearching == false );
	
	if ( doneSearching == true )
		volume->ioVAtrb		&=	~kFilesOpenMask;		//	Clear if no open files match
	else
		volume->ioVAtrb		|=	kFilesOpenMask;			//	Set if any files are opened

	if ( vcb == (ExtendedVCB *) LMGetDefVCBPtr() )		//	Are we supposedly the default?
		volume->ioVAtrb		|=	kDefaultVolumeMask;		//	Set volume as the default
	else
		volume->ioVAtrb		&=	~kDefaultVolumeMask;	//	Clear volume as the default
	
	
	*wdcb = nil;
	
	if ( (volume->ioVolIndex <= 0) && (nameSpecified == 0) )
	{
		vRefNum = volume->ioVRefNum;
		if ( vRefNum > 0 )								//	If driveNum, ignore it
		{
			goto	retVRef;
		}
		else if ( vRefNum == 0 )
		{
			vRefNum = 2;
		}
		else if ( vRefNum <= WDRfnMax )
		{
			//	a WDRefNum was supplied as VRefNum: change the number of files to be the	<21Sep85>
			//	valence of the directory in question: 										<21Sep85>
			vRefNum -= WDRfnMin;						//	Change WDRefNum to index
		}
		else
		{
			goto retVRef;
		}
		
		wdcbsAry = (WDCBArray*)LMGetWDCBsPtr();			//	Point to WDCB array
		*wdcb = (WDCBRec*)((Ptr)wdcbsAry + vRefNum);

		err = CheckVolumeOffLine( (ExtendedVCB*)vcb );	//	check external FS err
		if ( err == noErr )
		{
			if ( (**wdcb).wdDirID != fsRtDirID )
			{
				//€€	This will change to handle Don's meta struct
				err = GetCatalogNode( (ExtendedVCB*)vcb, (**wdcb).wdDirID, nil, (**wdcb).wdCatHint, &spec, &catNodeData, (UInt32*)&((**wdcb).wdCatHint) );
				if ( err == noErr )
				{
					volume->ioVNmFls = (UInt16) catNodeData.valence;
				}
			}
		}
	}
	
	//	If a working directory was specified, it shouldn't be overwritten by
	//	the volRefNum.  However, if a volume NAME was used, the volRefNum
	//	field is ignored, and SHOULD be filled in with the volume's VRefNum.
retVRef:
	if( (nameSpecified != 0) || (volume->ioVRefNum >= 0) )
	{
		volume->ioVRefNum = vcb->vcbVRefNum;
	}
	//	Copy the volume name field if specified.
	if ( volume->ioNamePtr != 0 )
		BlockMoveData( vcb->vcbVN, volume->ioNamePtr, vcb->vcbVN[0]+1 );


	//	Calculate the total and free bytes available on the volume as a 64 bit integer:

	if ( vcb->vcbSigWord == kHFSPlusSigWord )
	{
		//--	Change to use ExtendedVCB fields for HFS Plus
		MultiplyUInt32IntoUInt64( &(volume->ioVTotalBytes), vcb->totalBlocks, vcb->blockSize );
		MultiplyUInt32IntoUInt64( &(volume->ioVFreeBytes), vcb->freeBlocks, vcb->blockSize );
	}
	else
	{
		//--	It's just an HFS volume or some other external file system
		MultiplyUInt32IntoUInt64( &(volume->ioVTotalBytes), volume->ioVNmAlBlks, volume->ioVAlBlkSiz );
		MultiplyUInt32IntoUInt64( &(volume->ioVFreeBytes), volume->ioVFrBlk, volume->ioVAlBlkSiz );
	}
	
	
	volume->ioVRefNum	=	vcb->vcbVRefNum;			//	VRefNum
	volume->ioVSigWord	=	vcb->vcbSigWord;			//	Volume signature
	volume->ioVDrvInfo	=	vcb->vcbDrvNum;				//	Driver number
	volume->ioVDRefNum	=	vcb->vcbDRefNum;			//	Driver RefNum
	volume->ioVFSID		=	vcb->vcbFSID;				//	File System ID
	volume->ioVBkUp		=	vcb->vcbVolBkUp;			//	Last backup date
	volume->ioVSeqNum	=	vcb->vcbVSeqNum;			//	Volume sequence number
	volume->ioVWrCnt	=	vcb->vcbWrCnt;				//	Volume write count
	volume->ioVFilCnt	=	vcb->vcbFilCnt;				//	Files count for volume
	volume->ioVDirCnt	=	vcb->vcbDirCnt;				//	Directory count for volume

	BlockMoveData( vcb->vcbFndrInfo, volume->ioVFndrInfo, sizeof( SInt32 ) * 8 );

CmdDone:
	if ( err == noErr && volume->ioVFSID != 0)
	{
		LMSetReqstVol((Ptr) vcb);
		err = extFSErr;		// not an HFS or HFS Plus volume, let others have a crack at this call...
	}

	return( err );
}

#endif

//_______________________________________________________________________
//
//	Routine:	FlushVolumeControlBlock
//	Arguments:	ExtendedVCB		*vcb
//	Output:		OSErr			err
//
//	Function: 	Flush volume information to either the VolumeHeader of the Master Directory Block
//_______________________________________________________________________

OSErr	FlushVolumeControlBlock( ExtendedVCB *vcb )
{
	OSErr			err;
	VolumeHeader	*volumeHeader;
	UInt32			index;
	FCB				*fcb;
	ExtendedFCB		*extendedFCB;
	
	if ( ! IsVCBDirty( vcb ) )			//	if it's not dirty
		return( noErr );

	if ( vcb->vcbSigWord == kHFSPlusSigWord )
	{		
		err = GetBlock_glue( gbDefault, (vcb->hfsPlusIOPosOffset / 512) + kIDSectorOffset, (Ptr*)&volumeHeader, vcb->vcbVRefNum, vcb );	//	no Flags, VH is always block #0, vcb->vcbBufAdr == CacheQueueHeader
		ReturnIfError( err );
		
		//	2005507, Keep the MDB creation date and VolumeHeader creation date in sync.
		if ( vcb->hfsPlusIOPosOffset != 0 )								//	It's a wrapped HFS+ volume
		{
			MasterDirectoryBlock	*mdb;
			err = GetBlock_glue( gbDefault, kIDSectorOffset, (Ptr*)&mdb, vcb->vcbVRefNum, vcb );
			if ( err == noErr )
			{
				if ( mdb->drCrDate	!= vcb->vcbCrDate )					//	The creation date changed
				{
					mdb->drCrDate	= vcb->vcbCrDate;
					err = RelBlock_glue( (Ptr)mdb, rbWriteMask );		//	Force it to be written
				}
				else
				{
					err = RelBlock_glue( (Ptr)mdb, rbFreeMask );		//	Just release it
				}
			}
		}

	//	volumeHeader->signature			=	vcb->signature;
	//	volumeHeader->version			=
		volumeHeader->attributes		=	vcb->vcbAtrb;				//	VCB uses 16 bits, next 16 are for future API changes
		volumeHeader->lastMountedVersion=	kHFSPlusMountVersion;		//	Let others know we've mounted volume for writing
	//€€	volumeName
		volumeHeader->createDate		=	vcb->vcbCrDate;				//	NOTE: local time, not GMT!
		volumeHeader->modifyDate		=	LocalToUTC(vcb->vcbLsMod);
		volumeHeader->backupDate		=	LocalToUTC(vcb->vcbVolBkUp);
		volumeHeader->checkedDate		=	vcb->checkedDate;
		volumeHeader->fileCount			=	vcb->vcbFilCnt;
		volumeHeader->folderCount		=	vcb->vcbDirCnt;
		volumeHeader->blockSize			=	vcb->blockSize;
		volumeHeader->totalBlocks		=	vcb->totalBlocks;
		volumeHeader->freeBlocks		=	vcb->freeBlocks;
		volumeHeader->nextAllocation	=	vcb->nextAllocation;
		volumeHeader->rsrcClumpSize		=	vcb->vcbClpSiz;
		volumeHeader->dataClumpSize		=	vcb->vcbClpSiz;
		volumeHeader->nextCatalogID		=	vcb->vcbNxtCNID;
		volumeHeader->writeCount		=	vcb->vcbWrCnt;
		volumeHeader->encodingsBitmap	=	vcb->encodingsBitmap;

		//€€ should we use the vcb or fcb clumpSize values ????? -djb
		volumeHeader->allocationFile.clumpSize	= vcb->allocationsClumpSize;
		volumeHeader->extentsFile.clumpSize	= vcb->vcbXTClpSiz;
		volumeHeader->catalogFile.clumpSize	= vcb->vcbCTClpSiz;
		
		BlockMoveData( vcb->vcbFndrInfo, volumeHeader->finderInfo, sizeof(volumeHeader->finderInfo) );
	
		index = vcb->extentsRefNum;
		extendedFCB = GetParallelFCB( index );
		BlockMoveData( extendedFCB->extents, volumeHeader->extentsFile.extents, sizeof(LargeExtentRecord) );
		fcb = GetFileControlBlock( index );
		volumeHeader->extentsFile.logicalSize.lo = fcb->fcbEOF;
		volumeHeader->extentsFile.logicalSize.hi = 0;
		volumeHeader->extentsFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;
	
		index = vcb->catalogRefNum;
		extendedFCB = GetParallelFCB( index );
		BlockMoveData( extendedFCB->extents, volumeHeader->catalogFile.extents, sizeof(LargeExtentRecord) );
		fcb = GetFileControlBlock( index );
		volumeHeader->catalogFile.logicalSize.lo = fcb->fcbPLen;
		volumeHeader->catalogFile.logicalSize.hi = 0;
		volumeHeader->catalogFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;

		index = vcb->allocationsRefNum;
		extendedFCB = GetParallelFCB( index );
		BlockMoveData( extendedFCB->extents, volumeHeader->allocationFile.extents, sizeof(LargeExtentRecord) );
		fcb = GetFileControlBlock( index );
		volumeHeader->allocationFile.logicalSize.lo = fcb->fcbPLen;
		volumeHeader->allocationFile.logicalSize.hi = 0;
		volumeHeader->allocationFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;
	
		index = vcb->attributesRefNum;
		if (index != 0)		//	Only update fields if an attributes file existed and was open
		{
			extendedFCB = GetParallelFCB( index );
			BlockMoveData( extendedFCB->extents, volumeHeader->attributesFile.extents, sizeof(LargeExtentRecord) );
			fcb = GetFileControlBlock( index );
			volumeHeader->attributesFile.logicalSize.lo = fcb->fcbPLen;
			volumeHeader->attributesFile.logicalSize.hi = 0;
			volumeHeader->attributesFile.clumpSize = fcb->fcbClmpSize;
			volumeHeader->attributesFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;
		}
		
		//--	Write the MDB out by releasing the block dirty
		
		err = RelBlock_glue( (Ptr)volumeHeader, rbWriteMask );		//	force it to be written
	
		#if( hasHFSManualEject )
			// The critical volume info that needs to be maintained
			// for volumes in manual-eject drives has just been flushed.
			if (err == noErr)
				vcb->vcbFlags &= ~vcbFlushCriticalInfoMask;
		#endif
		
		MarkVCBClean( vcb );
	}
	else
	{
		// This routine doesn't really return an error!!!
		// So for now, we will just return noErr
		err = C_FlushMDB( vcb );		//	Go flush the VCB info BEFORE close
		return( noErr );
	}

	return( err );
}


ExtendedFCB* GetParallelFCB(short fref)
{
	ParallelFCB*	pfcbArray;
	FSVarsRec*		fsVars;
	short			index;

	index = fref / sizeof(FCB);
	
	fsVars = (FSVarsRec*) LMGetFSMVars();

	pfcbArray = (ParallelFCB*) fsVars->fcbPBuf;

	return &pfcbArray->extendedFCB[index];
}


//_______________________________________________________________________
//
//	Routine:	FlushAlternateVolumeControlBlock
//	Arguments:	ExtendedVCB		*vcb
//				Boolean			ifHFSPlus
//	Output:		OSErr			err
//
//	Function: 	Flush volume information to either the Alternate VolumeHeader of the 
//				Alternate Master Directory Block.  Called by the BTree when the catalog
//				or extent files grow.  Simply BlockMoves the original to the alternate
//				location.
//_______________________________________________________________________

OSErr	FlushAlternateVolumeControlBlock( ExtendedVCB *vcb, Boolean isHFSPlus )
{
	OSErr			err;
	UInt32			alternateBlockLocation;

	err = FlushVolumeControlBlock( vcb );
	
	if ( isHFSPlus )															//	Flush the VolumeHeader
	{
		VolumeHeader	*volumeHeader;
		VolumeHeader	*alternateVolumeHeader;

		//--	Get the VolumeHeader Block, 1st block in the HFS Plus partition
		err = GetBlock_glue( gbDefault, (vcb->hfsPlusIOPosOffset / 512) + kIDSectorOffset, (Ptr*)&volumeHeader, vcb->vcbVRefNum, vcb );	//	VH is always 1st sector of HFS Plus partition
		ReturnIfError( err );
		
		//--	Get the Alternate VolumeHeader Block, last sector in the HFS Plus partition
		alternateBlockLocation = (vcb->hfsPlusIOPosOffset / 512) + vcb->totalBlocks * (vcb->blockSize / 512) - 2;
		err = GetBlock_glue( gbDefault, alternateBlockLocation, (Ptr*)&alternateVolumeHeader, vcb->vcbVRefNum, vcb );	//	Alt VH is always 2nd to last sector of HFS Plus partition
		if ( err == noErr )
		{
			//--	Copy the VolumeHeader to the alternate location
			BlockMoveData( volumeHeader, alternateVolumeHeader, sizeof(VolumeHeader) );
	
			//--	And write it back to the disk, NOW!
			err = RelBlock_glue( (Ptr)volumeHeader, rbFreeMask );				//	free the MDB
			err = RelBlock_glue( (Ptr)alternateVolumeHeader, rbWriteMask );		//	force it to be written
		}
		else
		{			
			err = RelBlock_glue( (Ptr)volumeHeader, rbFreeMask );				//	free the VH
		}
	}
	else																		//	Flush the MasterDirectoryBlock
	{
		MasterDirectoryBlock	*mdb;
		MasterDirectoryBlock	*alternateMDB;
		UInt32					numBlocks;

		
		err = GetDiskBlocks( vcb->vcbDrvNum, &numBlocks );
		ReturnIfError( err );

		//--	Get the MDB, 2nd block on HFS disk
		err = GetBlock_glue( gbDefault, kIDSectorOffset, (Ptr*)&mdb, vcb->vcbVRefNum, vcb );	//	no Flags, MDB is always block
		ReturnIfError( err );

		//--	Get the Alternate MDB, 2nd to last block on disk
		alternateBlockLocation = numBlocks - 2;
		err = GetBlock_glue( gbDefault, alternateBlockLocation, (Ptr*)&alternateMDB, vcb->vcbVRefNum, vcb );	//	no Flags, VH is always block #0, vcb->vcbBufAdr == CacheQueueHeader
		if ( err == noErr )
		{
			//--	Copy the MDB to the alternate location
			BlockMoveData( mdb, alternateMDB, sizeof(MasterDirectoryBlock) );
			
			//--	And write it back to the disk, NOW!
			err = RelBlock_glue( (Ptr)mdb, rbFreeMask );						//	free the MDB
			err = RelBlock_glue( (Ptr)alternateMDB, rbWriteMask );				//	force it to be written
		}
		else
		{
			err = RelBlock_glue( (Ptr)mdb, rbFreeMask );						//	free the MDB
		}
	}

	return( err );
}

#ifdef INVESTIGATE

OSErr GetDiskBlocks( short driveNumber, unsigned long *numBlocks)
{
	/* Various constants for GetDiskBlocks() */
	enum
	{
		/* return format list status code */
		kFmtLstCode = 6,
		
		/* reference number of .SONY driver */
		kSonyRefNum = 0xfffb,
		
		/* values returned by DriveStatus in DrvSts.twoSideFmt */
		kSingleSided = 0,
		kDoubleSided = -1,
		kSingleSidedSize = 800,		/* 400K */
		kDoubleSidedSize = 1600,	/* 800K */
		
		/* values in DrvQEl.qType */
		kWordDrvSiz = 0,
		kLongDrvSiz = 1,
		
		/* more than enough formatListRecords */
		kMaxFormatListRecs = 16
	};
	
	DrvQEl			*driveQElementPtr;
	unsigned long	blocks;
	ParamBlockRec	pb;
	DrvSts			status;
	FormatListRec	formatListRecords[kMaxFormatListRecs];
	short			formatListRecIndex;
	OSErr			err;
	short 			driverRefNum;

	blocks = 0;
	
	//	Find the drive queue element for this volume
	err = FindDrive( &driverRefNum, &driveQElementPtr, driveNumber );
	ReturnIfError( err );
	
	//	Make sure this is a real driver (dQRefNum < 0).
	//	AOCE's Mail Enclosures volume uses 0 for dQRefNum which will cause
	//	problems if you try to use it as a driver refNum.

	if ( driveQElementPtr->dQRefNum >= 0 )
	{
		err = paramErr;
	}
	else
	{
		//	Attempt to get the drive's format list.
		//	(see the Technical Note "What Your Sony Drives For You")
		
		pb.cntrlParam.ioVRefNum = driveQElementPtr->dQDrive;
		pb.cntrlParam.ioCRefNum = driveQElementPtr->dQRefNum;
		pb.cntrlParam.csCode = kFmtLstCode;
		pb.cntrlParam.csParam[0] = kMaxFormatListRecs;
		*(long *)&pb.cntrlParam.csParam[1] = (long)&formatListRecords[0];
		
		err = PBStatusSync(&pb);
		
		if ( err == noErr )
		{
			//	The drive supports ReturnFormatList status call.
			
			//	Get the current disk's size.
			for( formatListRecIndex = 0;
				 formatListRecIndex < pb.cntrlParam.csParam[0];
	    		 ++formatListRecIndex )
	    	{
	    		if ( (formatListRecords[formatListRecIndex].formatFlags &
	    			  diCIFmtFlagsCurrentMask) != 0 )
	    		{
	    			blocks = formatListRecords[formatListRecIndex].volSize;
	    		}
			}
    		if ( blocks == 0 )
    		{
    			//	This should never happen
    			err = paramErr;
    		}
		}
		else if ( driveQElementPtr->dQRefNum == (short)kSonyRefNum )
		{
			//	The drive is a non-SuperDrive floppy which only supports 400K and 800K disks
			
			err = DriveStatus(driveQElementPtr->dQDrive, &status);
			if ( err == noErr )
			{
				switch ( status.twoSideFmt )
				{
				case kSingleSided:
					blocks = kSingleSidedSize;
					break;
				case kDoubleSided:
					blocks = kDoubleSidedSize;
					break;
				default:
					//	This should never happen
					err = paramErr;
					break;
				}
			}
		}
		else
		{
			//	The drive is not a floppy and it doesn't support ReturnFormatList
			//	so use the dQDrvSz field(s)
			
			err = noErr;			//	reset err
			switch ( driveQElementPtr->qType )
			{
			case kWordDrvSiz:
				blocks = driveQElementPtr->dQDrvSz;
				break;
			case kLongDrvSiz:
				blocks = ((unsigned long)driveQElementPtr->dQDrvSz2 << 16) +
						 driveQElementPtr->dQDrvSz;
				break;
			default:
				//	This should never happen
				err = paramErr;
				break;
			}
		}
	}
	
	*numBlocks = blocks;
	
	return ( err );
}


//
//	Sets up a VCB for calls to GetBlock()
//
void	AddVCB( ExtendedVCB	*vcb, short driveNumber, short ioRefNum )
{
	OSErr			err			= noErr;
	WDCBArray		*wdcbsAry;
	QHdr			*vcbQHdr;
	WDCBRecPtr		wdcb;
			
	vcbQHdr = LMGetVCBQHdr();
	if ( vcbQHdr->qHead == nil )
	{																	//	 Queue is empty, new vcb is the default now
		LMSetDefVCBPtr( (Ptr) vcb );
		wdcbsAry = (WDCBArray*)LMGetWDCBsPtr();							//	Point to WDCB array
		
		wdcb = &(wdcbsAry->wdcb[0]);
		wdcb->wdVCBPtr	= (VCB *) vcb;									//	Set default VCB pointer in default WDCB
		wdcb->wdDirID	= fsRtDirID;									//	Default to root directory
		wdcb->wdCatHint	= 0;											//	Clear catalog hint
		wdcb->wdProcID	= 0;											//	And procID of WDCB 'owner'
		vcbQHdr = LMGetVCBQHdr();										//	Reset for VCB insertion
	}
	

	Enqueue( (QElemPtr)vcb, (QHdrPtr)vcbQHdr );							//	Insert the new VCB in the queue

	vcb->vcbDrvNum	= driveNumber;
	vcb->vcbDRefNum	= ioRefNum;
	
	#if ( hasHFSManualEject )
		SetVCBManEject(vcb);											//	set vcb maneject flag appropriately	<SM7> <BH 03Aug93>
	#endif


	vcb->vcbBufAdr = LMGetSysVolCPtr();									//	always use system-wide cache Š

	vcb->vcbVRefNum	= GetNewVRefNum();									//	Assign a refNum to this new volume.

	if ( vcb == (ExtendedVCB *) LMGetDefVCBPtr() )						//	Are we supposedly the default?
	{
		LMSetDefVRefNum( vcb->vcbVRefNum );
	}
}


//--------------------------------------------------------------------------------------
//	Routine:	SetVCBManEject
//	Input:		VCB ptr
//	Called by:	MountVol
//	Function:	Sets or clears the manual-eject bit in the flags word of the input VCB
//				based on the type of the drive indicated by vcbDrvNum.  All registers
//				are preserved.  The manual-eject flag should only be changed at MountVol
//				time.
//--------------------------------------------------------------------------------------

#if ( hasHFSManualEject )

	static void SetVCBManEject(ExtendedVCB *vcb)
	{
		DrvQEl	*driveQElementPtr;
		DrvSts	*driveStatsPtr;
		short	driverRefNum;
		OSErr	err;
	
		//	Find the drive queue element for this volume
		err = FindDrive( &driverRefNum, &driveQElementPtr, vcb->vcbDrvNum );
		
		if (err) return;
		
		driveStatsPtr = (DrvSts*) ((Ptr) driveQElementPtr - offsetof(DrvSts, qLink));
	
		if ( driveStatsPtr->installed & (1 << kdqManualEjectBit) )		// manual eject drive asserted?
			vcb->vcbFlags |= vcbManualEjectMask;
		else
			vcb->vcbFlags &= ~vcbManualEjectMask;
	}

#endif


//
//	Simple utility function to deturmine if a keyboard key is pressed.
//
short IsPressed( UInt16 k )		// k =  any keyboard scan code, 0-127
{
	unsigned char	km[16];

	GetKeys( *( (KeyMap*) &km ) );										//	Ugly casting to compile with MPWC & SC
	return ( ( km[k>>3] >> (k & 7) ) & 1);
}


//
//	Utility function to allocate a new vRefNum. 
//	This function cycles through all vRefNums, rather than reuse them.
//
SInt16	GetNewVRefNum()
{
	SInt16		refNum;
	ExtendedVCB	*tempVCB;
	OSErr		err			= noErr;

	#if (0)		//	See bug 1664445
	{
		FSVarsRec	*fsVars;
	
	fsVars = (FSVarsRec*)LMGetFSMVars();
	for ( refNum = fsVars->nextVRefNum ; err == noErr ; refNum-- )
	{
		if ( refNum >= 0x8001 )											//	if we've cycled through all the possible vRef nums, start over
			refNum = -1;
			
		err = GetVCBRefNum( &tempVCB, refNum );							//	Check if already assigned
	}
	
	fsVars->nextVRefNum	= refNum;
	return( ++refNum );													//	for loop post decrements
	}
	#else
	{
		for ( refNum = -1 ; err == noErr ; refNum-- )
		{
			err = GetVCBRefNum( &tempVCB, refNum );						//	Check if already assigned
		}
		return( ++refNum );												//	for loop post decrements
	}
	#endif
}


//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹

OSErr GetVolumeNameFromCatalog( ExtendedVCB *vcb )
{
	FSSpec			spec;
	CatalogNodeData	nodeData;
	UInt32			hint;
	FSVarsRec		*fsVars;
	OSErr			err;
	
	fsVars = (FSVarsRec*) LMGetFSMVars();								//	$BB8

	if ( fsVars->gIsUnicodeInstalled )
	{
		err = GetCatalogNode( vcb, kHFSRootFolderID, NULL, kNoHint, &spec, &nodeData, &hint );
	
		if ( err == noErr )
		{
			BlockMoveData( spec.name, vcb->vcbVN, sizeof(Str27) );
			vcb->volumeNameEncodingHint = nodeData.textEncoding;		// save encoding for FSFindTextEncodingForVolume
		}
	}
	else
	{
		err = paramErr;
	}
		
	return err;
}

#endif
