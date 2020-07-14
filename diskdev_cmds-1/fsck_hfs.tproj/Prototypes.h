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
	File:		Prototypes.h

	Contains:	Function Prototypes...

	Version:	xxx put version here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1992,1994, 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Deric Horn

		Other Contact:		Don Brady

		Technology:			File Systems

	Writers:

		(msd)	Mark Day
		(DSH)	Deric Horn
		(djb)	Don Brady

	Change History (most recent first):

	 <HFS18>	 12/2/97	DSH		Boolean FileSharingIsOn()
	 <HFS17>	11/18/97	DSH		CustomIconCheck is now a static function
	 <HFS16>	 11/4/97	DSH		void SetDFAStage( UInt32 stage )
	 <HFS15>	10/30/97	DSH		Modify ProcessFileExtents parameters
	 <HFS14>	10/21/97	DSH		InvalidateCalculatedVolumeBitMap()
	 <HFS13>	 10/6/97	DSH		BuildExtentKey
	 <HFS12>	  9/8/97	DSH		Added FlushBlockCache prototype
	 <HFS11>	  9/4/97	msd		Add prototypes for AttrBTChk, CreateAttributesBTreeControlBlock.
	 <HFS10>	 8/18/97	DSH		M_DebugStr, M_Debugger is a macro to do nothing or a DebugStr.
	  <HFS9>	 8/11/97	DSH		forkType, SInt8 to UInt8
	  <HFS8>	 7/28/97	DSH		New prototypes for GetBlockProc,  and ReleaseBlockProc.
	  <HFS7>	 6/26/97	DSH		Integration with latest HFS+
	  <HFS6>	  6/6/97	DSH		SetEOF not the same
	  <HFS5>	 5/21/97	DSH		UsersAreConnected( void );
	  <HFS4>	  4/4/97	DSH		Adding parameters for MessageProcs
	  <HFS3>	 3/27/97	DSH		Math64 functions defined in Math64.h
	  <HFS2>	 3/27/97	DSH		Added CheckDisk related prototypes
	  <HFS1>	 3/17/97	DSH		first checked in
		 <4>	 2/14/95	djb		Added ValidDrive prototype.
		 <7>	 9/16/94	DJB		Added prototype for CustomIconCheck.
		 <6>	  4/7/92	gs		Add AllocMajorRepairOrder and AllocMinorRepairOrder.
		 <5>	 3/19/92	PP		Add prototypes for external routines of DF bug repair.

*/



/* ------------------------------ "C" routines -------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#include "BTreesPrivate.h"
#include "CatalogPrivate.h"
#include <Math64.h>

short CheckPause( short MsgID, short *UserCmd );

void WriteMsg( SGlobPtr GPtr, short messageID, short messageType );

void WriteError( short MsgID, UInt32 TarID, UInt32 TarBlock );


/* ------------------------------- From SControl.c ------------------------------- */

void ScavCtrl( SGlobPtr GPtr, UInt32 ScavOp, short *ScavRes, short *UserCmd );

extern short CheckForStop( SGlobPtr GPtr );
	

/* ------------------------------- From SRepair.c -------------------------------- */

extern int MRepair( SGlobPtr GPtr );

extern int CheckForDFCorruption( const SGlobPtr GPtr, OSErr errorCode );

extern int FixDFCorruption( const SGlobPtr GPtr, RepairOrderPtr DFOrderP );

extern	OSErr	ProcessFileExtents( SGlobPtr GPtr, FCB *fcb, UInt8 forkType, UInt16 flags, Boolean isExtentsBTree, Boolean *hasOverflowExtents, UInt32 *blocksUsed  );

/* ------------------------------- From SUtils.c --------------------------------- */

extern int AllocBTN( SGlobPtr GPtr, short FilRefN, UInt32 NodeNum );

extern OSErr AllocExt( SGlobPtr GPtr, const UInt32 startBlock, const UInt32 blockCount );

extern int GetVBlk(	SGlobPtr GPtr, UInt32 BlkNum, void **BufPtr );

extern int IntError( SGlobPtr GPtr, OSErr ErrCode );

extern void RcdError( SGlobPtr GPtr, OSErr ErrCode );

extern RepairOrderPtr AllocMinorRepairOrder( SGlobPtr GPtr, int extraBytes );

extern Boolean	ValidDrive (DrvQElPtr dqPtr);

extern	void	SetApplScratchValue( short whichLong, UInt32 value );	//	[0, 1, 2]
extern	UInt32	GetApplScratchValue( short whichLong );
extern	void	SetDFAStage( UInt32 stage );
extern	UInt32	GetDFAGlobals( void );

extern void	InvalidateCalculatedVolumeBitMap( SGlobPtr GPtr );

extern	Boolean FileSharingIsOn( void );

/* ------------------------------- From SVerify1.c -------------------------------- */

extern OSErr CatBTChk( SGlobPtr GPtr );		//	catalog btree check

extern OSErr CatFlChk( SGlobPtr GPtr );		//	catalog file check
	
extern OSErr CatHChk( SGlobPtr GPtr );		//	catalog hierarchy check

extern OSErr ExtBTChk( SGlobPtr GPtr );		//	extent btree check

extern OSErr ExtFlChk( SGlobPtr GPtr );		//	extent file check

extern OSErr AttrBTChk( SGlobPtr GPtr );	//	attributes btree check

extern OSErr IVChk( SGlobPtr GPtr );

extern OSErr VInfoChk( SGlobPtr GPtr );

extern OSErr VLockedChk( SGlobPtr GPtr );

extern	void BuildExtentKey( Boolean isHFSPlus, UInt8 forkType, CatalogNodeID fileNumber, UInt32 blockNumber, void * key );

extern	OSErr	OrphanedFileCheck( SGlobPtr GPtr, Boolean *problemsFound );

extern	int cmpLongs (const void *a, const void *b);

/* ------------------------------- From SVerify2.c -------------------------------- */

extern int BTCheck( SGlobPtr GPtr, short refNum );

extern int BTMapChk( SGlobPtr GPtr, short FilRefN );

extern OSErr ChkCName( SGlobPtr GPtr, const CatalogName *name, Boolean unicode );	//	check catalog name

extern OSErr CmpBTH( SGlobPtr GPtr, SInt16 fileRefNum );

extern int CmpBTM( SGlobPtr GPtr, short FilRefN );

extern int CmpMDB( SGlobPtr GPtr );

extern int CmpVBM( SGlobPtr GPtr );

extern OSErr CmpBlock( void *block1P, void *block2P, UInt32 length ); /* same as 'memcmp', but EQ/NEQ only */
	
extern OSErr ChkExtRec ( SGlobPtr GPtr, const void *extents );
OSErr	CheckVolumeBitMap( SGlobPtr GPtr );

extern	OSErr	GetBlock_FSGlue( int flags, UInt32 block, Ptr *buffer, int refNum, ExtendedVCB *vcb );
extern	OSErr	RelBlock_FSGlue( Ptr buffer, int flags );
extern	OSErr	FlushBlockCache ( void );

OSErr	CheckFileExtents( SGlobPtr GPtr, UInt32 fileNumber, UInt8 forkType, Boolean checkBitMap, void *extents, UInt32 *blocksUsed );
OSErr	GetBTreeHeader( SGlobPtr GPtr, SInt16 fileRefNum, HeaderRec **header );
OSErr	CompareVolumeBitMap( SGlobPtr GPtr, SInt32 whichBuffer );
OSErr	CompareVolumeHeader( SGlobPtr GPtr );
OSErr	CreateVolumeBitMapBuffer( SGlobPtr GPtr, SInt32 bufferNumber );
OSErr	CreateExtentsBTreeControlBlock( SGlobPtr GPtr );
OSErr	CreateCatalogBTreeControlBlock( SGlobPtr GPtr );
OSErr	CreateAttributesBTreeControlBlock( SGlobPtr GPtr );
OSErr	CreateExtendedAllocationsFCB( SGlobPtr GPtr );

OSErr	InitDFACache( UInt32 blockSize );
OSErr	DisposeDFACache( void );
SInt32	CalculateSafePhysicalTempMem( void );
void	DFAFlushCache( void );
OSErr	DFAGetBlock( UInt16 flags, UInt32 blockNum, Ptr *buffer, SInt16 refNum, ExtendedVCB *vcb, UInt32 blockSize );
OSErr	DFAReleaseBlock( Ptr buffer, SInt8 flags, UInt32 blockSize );

extern UInt32 CGetVolSiz( DrvQEl *DrvPtr );

extern OSStatus GetBlockProc ( SInt16 fileRefNum, UInt32 blockNum, GetBlockOptions options, BlockDescriptor *block );
extern OSStatus	ReleaseBlockProc ( SInt16 fileRefNum, BlockDescPtr blockPtr, ReleaseBlockOptions options );

void DFA_PrepareInputName(ConstStr31Param name, Boolean isHFSPlus, CatalogName *catalogName);

extern UInt32	CatalogNameSize( const CatalogName *name, Boolean isHFSPlus);



extern	void	CalculateItemCount( SGlob *GPtr, UInt64 *itemCount );
extern Boolean	UsersAreConnected( void );


#if ( DiskFuckUpEnabled )
	void	DoCorruptDisk( SInt16 driveNumber );
#endif



//	Macros
extern pascal void M_Debugger(void);
extern pascal void M_DebugStr(ConstStr255Param debuggerMsg);
#if ( DEBUG_BUILD )
	#define	M_Debuger()					Debugger()
	#define	M_DebugStr( debuggerMsg )	DebugStr( debuggerMsg )
#else
	#define	M_Debuger()
	#define	M_DebugStr( debuggerMsg )
#endif



#ifdef __cplusplus
};
#endif

