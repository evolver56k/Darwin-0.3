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

/*	@(#)hfs_vfsutils.c	4.0
*
*	(c) 1997-1999 Apple Computer, Inc.  All Rights Reserved
*
*	hfs_vfsutils.c -- Routines that go between the HFS layer and the VFS.
*
*	Change History (most recent first):
*
*	 1-Mar-1999	Scott Roberts	Dont double MALLOC on long names.
*	23-Feb-1999	Pat Dirks		Change incrementing of meta refcount to be done BEFORE lock is acquired.
*	 2-Feb-1999	Pat Dirks		For volume ATTR_CMN_SCRIPT use vcb->volumeNameEncodingHint instead of 0.
*	18-Jan-1999	Pat Dirks		Changed CopyCatalogToHFSNode to start with ACCESSPERMS instead of adding
*								write access only for unlocked files (now handled via IMMUTABLE setting)
*	 7-Dec-1998 Pat Dirks		Changed PackCatalogInfoFileAttributeBlock to return proper I/O block size.
*	 7-Dec-1998	Don Brady		Pack the real text encoding instead of zero.
*	16-Dec-1998	Don Brady		Use the root's crtime intead of vcb create time for getattrlist.
*	16-Dec-1998	Don Brady		Use the root's crtime intead of vcb create time for getattrlist.
*	 2-Dec-1998	Scott Roberts	Copy the mdbVN correctly into the vcb.
*    3-Dec-1998 Pat Dirks		Added support for ATTR_VOL_MOUNTFLAGS.
*	20-Nov-1998	Don Brady		Add support for UTF-8 names.
*   18-Nov-1998	Pat Dirks		Changed UnpackCommonAttributeBlock to call wait for hfs_chflags to update catalog entry when changing flags
*   13-Nov-1998 Pat Dirks       Changed BestBlockSizeFit to try PAGE_SIZE only and skip check for MAXBSIZE.
*	10-Nov-1998	Pat Dirks		Changed CopyCatalogToHFSNode to ensure consistency between lock flag and IMMUTABLE bits.
*   10-Nov-1998	Pat Dirks		Added MapFileOffset(), LogicalBlockSize() and UpdateBlockMappingTable() routines.
*	18-Nov-1998	Pat Dirks		Changed PackVolAttributeBlock to return proper logical block size
*                               for ATTR_VOL_IOBLOCKSIZE attribute.
*	 3-Nov-1998	Umesh Vaishampayan	Changes to deal with "struct timespec"
*								change in the kernel.	
*	23-Sep-1998	Don Brady		In UnpackCommonAttributeBlock simplified setting of gid, uid and mode.
*   10-Nov-1998	Pat Dirks		Added MapFileOffset(), LogicalBlockSize() and UpdateBlockMappingTable() routines.
*	17-Sep-1998	Pat Dirks		Changed BestBlockSizeFit to try MAXBSIZE and PAGE_SIZE first.
*	 8-Sep-1998	Don Brady		Fix CopyVNodeToCatalogNode to use h_mtime for contentModDate (instead of h_ctime).
*	 4-Sep-1998	Pat Dirks		Added BestBlockSizeFit routine.
*	18-Aug-1998	Don Brady		Change DEBUG_BREAK_MSG to a DBG_UTILS in MacToVFSError (radar #2262802).
*	30-Jun-1998	Don Brady		Add calls to MacToVFSError to hfs/hfsplus mount routines (for radar #2249539).
*	22-Jun-1998	Don Brady		Add more error cases to MacToVFSError; all HFS Common errors are negative.
*								Changed hfsDelete to call DeleteFile for files.
*	 4-Jun-1998	Pat Dirks		Changed incorrect references to 'vcbAlBlkSize' to 'blockSize';
*								Added hfsCreateFileID.
*	 4-Jun-1998	Don Brady		Add hfsMoveRename to replace hfsMove and hfsRename. Use VPUT/VRELE macros
*								instead of vput/vrele to catch bad ref counts.
*	28-May-1998	Pat Dirks		Adjusted for change in definition of ATTR_CMN_NAME and removed ATTR_CMN_RAWDEVICE.
*	 7-May-1998	Don Brady		Added check for NULL vp to hfs_metafilelocking (radar #2233832).
*	24-Apr-1998	Pat Dirks		Fixed AttributeBlockSize to return only length of variable attribute block.
*	4/21/1998	Don Brady		Add SUPPORTS_MAC_ALIASES conditional (for radar #2225419).
*	4/21/1998	Don Brady		Map cmNotEmpty errors to ENOTEMPTY (radar #2229259).
*	4/21/1998	Don Brady		Fix up time/date conversions.
*	4/20/1998	Don Brady		Remove course-grained hfs metadata locking.
*	4/18/1998	Don Brady		Add VCB locking.
*	4/17/1998	Pat Dirks		Fixed PackFileAttributeBlock to return more up-to-date EOF/PEOF info from vnode.
*	4/15/1998	Don Brady		Add hasOverflowExtents and hfs_metafilelocking. Use ExtendBTreeFile instead
*								of SetEndOfForkProc. Set forktype for system files.
*	4/14/1998	Deric Horn		PackCatalogInfoAttributeBlock(), and related packing routines to
*								pack attribute data given hfsCatalogInfo, without the objects vnode;
*	4/14/1998 	Scott Roberts	Add execute priviledges to all hfs objects.
*	 4/9/1998	Don Brady		Add MDB/VolumeHeader flushing to hfsUnmount;
*	 4/8/1998	Don Brady		Make sure vcbVRefNum field gets initialized (use MAKE_VREFNUM).
*	 4/6/1998	Don Brady		Removed calls to CreateVolumeCatalogCache (obsolete).
*	4/06/1998	Scott Roberts	Added complex file support.
*	4/02/1998	Don Brady		UpdateCatalogNode now takes parID and name as input.
*	3/31/1998	Don Brady		Sync up with final HFSVolumes.h header file.
*	3/31/1998	Don Brady		Check result from UFSToHFSStr to make sure hfs/hfs+ names are not greater
*								than 31 characters.
*	3/30/1998	Don Brady		In InitMetaFileVNode set VSYSTEM bit in vnode's v_flag.
*	3/26/1998	Don Brady		Cleaned up hfs_MountXXX routines. Removed CloseBtreeFile and OpenBTreeFile.
*								Simplified hfsUnmount (removed MacOS specific code).
*	3/17/1998	Don Brady		AttributeBlockSize calculation did not account for the size field (4bytes).
*	  							PackVolCommonAttributes and PackCommonAttributeBlock for ATTR_CMN_NAME
*	  							were not setting up the name correctly.
*	3/17/1998	Don Brady		Changed CreateCatalogNode interface to take kCatalogFolderNode and
*								kCatalogFileNode as type input. Also, force MountCheck to always run.
*	12-nov-1997	Scott Roberts	Initially created file.
*	17-Mar-98	ser				Broke out and created CopyCatalogToHFSNode()
*
*/
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/attr.h>
#include <sys/mount.h>
#include <libkern/libkern.h>
#include <mach/machine/simple_lock.h>
#include <mach/vm_param.h>
#include <kern/mapfs.h>

#include "hfs.h"
#include "hfs_dbg.h"

#include "hfscommon/headers/system/MacOSTypes.h"
#include "hfscommon/headers/system/MacOSStubs.h"
#include "hfscommon/headers/FileMgrInternal.h"
#include "hfscommon/headers/BTreesPrivate.h"
#include "hfscommon/headers/system/HFSUnicodeWrappers.h"

#define		SUPPORTS_MAC_ALIASES	0
#define		kMaxSecsForFsync	5

#define BYPASSBLOCKINGOPTIMIZATION 0

extern int (**hfs_vnodeop_p)();
extern int count_lock_queue __P((void));


OSErr	ValidMasterDirectoryBlock( HFSMasterDirectoryBlock *mdb );
Boolean	IsARamDiskDriver( void );
UInt16	DivUp( UInt32 byteRun, UInt32 blockSize );

extern struct tm *localtime __P((const time_t *));
extern time_t mktime __P((struct tm *));

int memcmp __P((const void *,const void *, size_t));

extern UInt16 CountRootFiles(ExtendedVCB *vcb);
extern OSErr GetVolumeNameFromCatalog(ExtendedVCB *vcb);

static int InitMetaFileVNode(struct vnode *vp, u_long eof, u_long clumpSize, void *extents,
							 HFSCatalogNodeID fileID, void * keyCompareProc);

static void ReleaseMetaFileVNode(struct vnode *vp);


//*******************************************************************************
//	Routine:	hfs_MountHFSVolume
//
//
//*******************************************************************************

OSErr	hfs_MountHFSVolume( register struct hfsmount *hfsmp, HFSMasterDirectoryBlock *mdb, struct proc *p)
{
    ExtendedVCB 			*vcb = HFSTOVCB(hfsmp);
    struct vnode 			*tmpvnode;
    OSErr					err;
    DBG_FUNC_NAME("hfs_MountHFSVolume");
    DBG_PRINT_FUNC_NAME();

    if (hfsmp == nil || mdb == nil)				/* exit if bad paramater */
		return (EINVAL);

    err = ValidMasterDirectoryBlock( mdb );		/* make sure this is an HFS disk */
    if (err)
    	return MacToVFSError(err);

    /*
     * The MDB seems OK: transfer info from it into VCB
	 * Note - the VCB starts out clear (all zeros)
	 *
	 * vcbSigWord is the start of duplicate mdb/vcb info.
     * copy all fields in mdb till the drVN field
	 */

	DBG_ASSERT((hfsmp->hfs_raw_dev & 0xFFFF0000) == 0);
	vcb->vcbVRefNum = MAKE_VREFNUM(hfsmp->hfs_raw_dev);
    bcopy( mdb, &(vcb->vcbSigWord),  (Size)(&(mdb->drVN)) - (Size)mdb );

	/*
     * Copy the drVN field, which is a Pascal String to the vcb, which is a cstring
	 */

    bcopy( &mdb->drVN[1], vcb->vcbVN,  mdb->drVN[0]);
    vcb->vcbVN[mdb->drVN[0]] = '\0';


    //	Initialize our dirID/nodePtr cache associated with this volume.
    err = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(vcb->hintCachePtr) );
    ReturnIfError( err );

	/*
	 * Some fields like vcbNmAlBlks, vcbAlBlkSiz, and vcbFreeBks are just
	 * around for API compatability and the HFS/HFS Plus code uses new
	 * ExtendedVCB fields.
	 * note:  vcbNmAlBlks * vcbAlBlkSiz still gives correct volume size
	 */
    vcb->totalBlocks	=	vcb->vcbNmAlBlks;
    vcb->freeBlocks		=	vcb->vcbFreeBks;
    vcb->blockSize		=	vcb->vcbAlBlkSiz;

    hfsmp->hfs_log_block_size = BestBlockSizeFit(vcb->blockSize, MAXBSIZE, hfsmp->hfs_phys_block_size);

    // XXX PPD: Should check here for hardware lock flag and set flags in VCB/MP appropriately
    if ( IsARamDiskDriver() )
      {
        vcb->vcbAtrb |= kHFSVolumeNoCacheRequiredMask;		//	yes: mark VCB as a RAM Disk
      }

    //--	copy all fields in mdb between drXTFlSize & drVolBkUp
    bcopy( &(mdb->drVolBkUp), &(vcb->vcbVolBkUp), (Size)&(mdb->drXTFlSize) - (Size)&(mdb->drVolBkUp) ); //	66 bytes

    vcb->vcbXTAlBlks 	= DivUp( mdb->drXTFlSize, vcb->blockSize );	//	Pick up PEOF of extent B*-Tree
    vcb->vcbCTAlBlks 	= DivUp( mdb->drCTFlSize, vcb->blockSize );	//	Pick up PEOF of catalog B*-Tree
    vcb->nextAllocation	= mdb->drAllocPtr;
    vcb->vcbAllocPtr	= 0;				//	Restart the roving allocation ptr
    vcb->vcbWrCnt++;					//	Compensate for write of MDB on last flush

	VCB_LOCK_INIT(vcb);

	/*
	 * Set up Extents B-tree vnode...
	 */ 
	err = GetInitializedVNode(hfsmp, &tmpvnode);
	if (err) goto MtVolErr;
	err = InitMetaFileVNode(tmpvnode, mdb->drXTFlSize, vcb->vcbXTClpSiz, &mdb->drXTExtRec,
							kHFSExtentsFileID, CompareExtentKeys);
    if (err) goto MtVolErr;

	/*
	 * Set up Catalog B-tree vnode...
	 */ 
	err = GetInitializedVNode(hfsmp, &tmpvnode);
	if (err) goto MtVolErr;
	err = InitMetaFileVNode(tmpvnode, mdb->drCTFlSize, vcb->vcbCTClpSiz, &mdb->drCTExtRec,
							kHFSCatalogFileID, CompareCatalogKeys);
	if (err) goto MtVolErr;

    if ( !(vcb->vcbAtrb & (short)kHFSVolumeUnmountedMask) )
      {
        UInt32  consistencyStatus;

        vcb->vcbAtrb &=	~kHFSVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
		DBG_VFS(("hfs_MountHFSVolume: calling MountCheck...\n"));
        err = MountCheck(vcb, &consistencyStatus);
		DBG_VFS(("hfs_MountHFSVolume: MountCheck returned %d and %08lx\n",err, consistencyStatus));
        if ( err )	goto MtChkErr;
      }
    else
      {
        vcb->vcbAtrb &=	~kHFSVolumeUnmountedMask;					//	From now until _Unmount	(clear the bit)
      }

	/*
	 * all done with b-trees so we can unlock now...
	 */
    VOP_UNLOCK(vcb->catalogRefNum, 0, p);
    VOP_UNLOCK(vcb->extentsRefNum, 0, p);

    err = noErr;

    if ( err == noErr )
      {
        if ( !(vcb->vcbAtrb & kHFSVolumeHardwareLockMask) )		//	if the disk is not write protected
          {
            MarkVCBDirty( vcb );								//	mark VCB dirty so it will be written
          }
      }
    goto	CmdDone;


MtChkErr:;
    InvalidateCatalogCache( vcb );

	err = EBADF;

    //--	Release any resources allocated so far before exiting with an error:
MtVolErr:;
	ReleaseMetaFileVNode(vcb->catalogRefNum);
	ReleaseMetaFileVNode(vcb->extentsRefNum);

CmdDone:;
    return( err );

}

//*******************************************************************************
//	Routine:	hfs_MountHFSPlusVolume
//
//
//*******************************************************************************

OSErr hfs_MountHFSPlusVolume( register struct hfsmount *hfsmp, HFSPlusVolumeHeader *vhp, u_long embBlkOffset, struct proc *p)
{
    register ExtendedVCB	*vcb;
    HFSPlusForkData			*fdp;
    struct vnode 			*tmpvnode;
    OSErr					retval;

    if (hfsmp == nil || vhp == nil)		/*	exit if bad paramater */
		return (EINVAL);

	DBG_VFS(("hfs_MountHFSPlusVolume: signature=0x%x, version=%d, blockSize=%ld\n", vhp->signature, vhp->version, vhp->blockSize));

    retval = ValidVolumeHeader(vhp);	/*	make sure this is an HFS Plus disk */
    if (retval)
    	return MacToVFSError(retval);
    
    /*
     * The VolumeHeader seems OK: transfer info from it into VCB
	 * Note - the VCB starts out clear (all zeros)
	 */
	vcb = HFSTOVCB(hfsmp);

	DBG_ASSERT((hfsmp->hfs_raw_dev & 0xFFFF0000) == 0);
	vcb->vcbVRefNum		=	MAKE_VREFNUM(hfsmp->hfs_raw_dev);
	vcb->vcbSigWord		=	vhp->signature;
	vcb->vcbCrDate		=	vhp->createDate;				// NOTE: local time, not GMT!
	vcb->vcbLsMod		=	UTCToLocal(vhp->modifyDate);
	vcb->vcbAtrb		=	(UInt16) vhp->attributes;		// VCB only uses lower 16 bits
	vcb->vcbClpSiz		=	vhp->rsrcClumpSize;
	vcb->vcbNxtCNID		=	vhp->nextCatalogID;
	vcb->vcbVolBkUp		=	UTCToLocal(vhp->backupDate);
	vcb->vcbWrCnt		=	vhp->writeCount;
	vcb->vcbXTClpSiz	=	vhp->extentsFile.clumpSize;
	vcb->vcbCTClpSiz	=	vhp->catalogFile.clumpSize;
	vcb->vcbFilCnt		=	vhp->fileCount;
	vcb->vcbDirCnt		=	vhp->folderCount;
	
	/* copy 32 bytes of Finder info */
	bcopy(vhp->finderInfo, vcb->vcbFndrInfo, sizeof(vhp->finderInfo));    

	/*
	 *	vcbFreeBks is the number of free bytes divided by the (HFS compatible) allocation block size.  Since the
	 *	number of free bytes could overflow a UInt32, we actually divide both free bytes and allocation block size by
	 *	512 before doing the division.  This prevents intermediate values from overflowing until the volume size is
	 *	2 TB.  It's OK to divide by 512 since HFS Plus requires allocation blocks to be a multiple of 512.
	 */
	HFSBlocksFromTotalSectors( (vhp->blockSize / 512) * vhp->totalBlocks, &vcb->vcbAlBlkSiz, &vcb->vcbNmAlBlks );

	vcb->vcbFreeBks = (vhp->freeBlocks * (vhp->blockSize / 512)) / (vcb->vcbAlBlkSiz / 512);
	vcb->vcbAlBlSt = 0;		/* hfs+ allocation blocks start at first block of volume */
	vcb->vcbWrCnt++;		/* compensate for write of Volume Header on last flush */

	VCB_LOCK_INIT(vcb);

	/*	Now fill in the Extended VCB info */
	vcb->nextAllocation			=	vhp->nextAllocation;
	vcb->totalBlocks			=	vhp->totalBlocks;
	vcb->freeBlocks				=	vhp->freeBlocks;
	vcb->blockSize				=	vhp->blockSize;
	vcb->allocationsClumpSize	=	vhp->allocationFile.clumpSize;
	vcb->attributesClumpSize	=	vhp->attributesFile.clumpSize;
	vcb->checkedDate			=	vhp->checkedDate;
	vcb->encodingsBitmap		=	vhp->encodingsBitmap;
	
	vcb->vcbAlBlSt				=	embBlkOffset;
	vcb->hfsPlusIOPosOffset		=	embBlkOffset * 512;

    /* Update the logical block size in the mount struct (currently set up from the wrapper MDB)
       using the new blocksize value: */
    hfsmp->hfs_log_block_size = BestBlockSizeFit(vcb->blockSize, MAXBSIZE, hfsmp->hfs_phys_block_size);

    // XXX PPD: Should check here for hardware lock flag and set flags in VCB/MP appropriately
    // vcb->vcbAtrb |= kVolumeHardwareLockMask;	// XXX this line for debugging only!!!!

    if ( IsARamDiskDriver() )
		vcb->vcbAtrb |= kHFSVolumeNoCacheRequiredMask;		//	yes: mark VCB as a RAM Disk

    //	Initialize our dirID/nodePtr cache associated with this volume.
    retval = InitMRUCache( sizeof(UInt32), kDefaultNumMRUCacheBlocks, &(vcb->hintCachePtr) );
    if (retval != noErr) goto ErrorExit;

	/*
	 * Set up Extents B-tree vnode...
	 */ 
	retval = GetInitializedVNode(hfsmp, &tmpvnode);
	if (retval) goto ErrorExit;
	fdp = &vhp->extentsFile;
	retval = InitMetaFileVNode(tmpvnode, fdp->logicalSize.lo, fdp->clumpSize, &fdp->extents,
								kHFSExtentsFileID, CompareExtentKeysPlus);
    if (retval) goto ErrorExit;

	/*
	 * Set up Catalog B-tree vnode...
	 */ 
	retval = GetInitializedVNode(hfsmp, &tmpvnode);
	if (retval) goto ErrorExit;
	fdp = &vhp->catalogFile;
	retval = InitMetaFileVNode(tmpvnode, fdp->logicalSize.lo, fdp->clumpSize, &fdp->extents,
								kHFSCatalogFileID, CompareExtendedCatalogKeys);
	if (retval) goto ErrorExit;

	/*
	 * Set up Allocation file vnode...
	 */  
	retval = GetInitializedVNode(hfsmp, &tmpvnode);
	if (retval) goto ErrorExit;
    VTOH(tmpvnode)->h_meta->h_logBlockSize = kHFSBlockSize;		/* XXX does Allocation use GetBlock? */
    VTOH(tmpvnode)->h_meta->h_physBlkPerLogBlk = 1;
	fdp = &vhp->allocationFile;
	retval = InitMetaFileVNode(tmpvnode, fdp->logicalSize.lo, fdp->clumpSize, &fdp->extents,
								kHFSAllocationFileID, NULL);
	if (retval) goto ErrorExit;
 
	/*
	 * Set up Attribute B-tree vnode (optional)...
	 */
	if (vhp->attributesFile.logicalSize.lo > 0) {
		retval = GetInitializedVNode(hfsmp, &tmpvnode);
		if (retval) goto ErrorExit;
		fdp = &vhp->attributesFile;
		retval = InitMetaFileVNode(tmpvnode, fdp->logicalSize.lo, fdp->clumpSize, &fdp->extents,
									kHFSAttributesFileID, CompareAttributeKeys);
		if (retval) goto ErrorExit;
	}

	/*
	 * Now that Catalog file is open...
	 *    get the root file count
	 *    get the volume name from the catalog
	 */
	vcb->vcbNmFls = CountRootFiles(vcb);	/* set up root file count */
	DBG_VFS(("hfs_MountHFSPlusVolume: root file count was %d\n", vcb->vcbNmFls));

	retval = MacToVFSError( GetVolumeNameFromCatalog(vcb) );	
    if (retval != noErr) goto ErrorExit;

	/*
	 * if volume was not cleanly unmounted then do a consistency check
	 */
    if ( !(vcb->vcbAtrb & (short)kHFSVolumeUnmountedMask) )
	{
		UInt32  consistencyStatus;

        vcb->vcbAtrb &=	~kHFSVolumeUnmountedMask;			//	From now until _Unmount	(clear the bit)
		DBG_VFS(("hfs_MountHFSPlusVolume: calling MountCheck...\n"));
		retval = MacToVFSError( MountCheck(vcb, &consistencyStatus) );
		DBG_VFS(("hfs_MountHFSPlusVolume: MountCheck returned %d and %08lx\n",retval, consistencyStatus));
		if (retval) goto ErrorExit;
	}
    else
	{
        vcb->vcbAtrb &=	~kHFSVolumeUnmountedMask;			//	From now until _Unmount	(clear the bit)
	}

	/*
	 * all done with metadata files so we can unlock now...
	 */
	if (vcb->attributesRefNum != NULL)
    	VOP_UNLOCK(vcb->attributesRefNum, 0, p);
    VOP_UNLOCK(vcb->allocationsRefNum, 0, p);
    VOP_UNLOCK(vcb->catalogRefNum, 0, p);
    VOP_UNLOCK(vcb->extentsRefNum, 0, p);

	if ( !(vcb->vcbAtrb & kHFSVolumeHardwareLockMask) )		//	if the disk is not write protected
	{
		MarkVCBDirty( vcb );								//	mark VCB dirty so it will be written
	}
	
	DBG_VFS(("hfs_MountHFSPlusVolume: returning (%d)\n", retval));

	return (0);


ErrorExit:
	/*
	 * A fatal error occured and the volume cannot be mounted
	 * release any resources that we aquired...
	 */

	DBG_VFS(("hfs_MountHFSPlusVolume: fatal error (%d)\n", retval));

    InvalidateCatalogCache(vcb);
    
	ReleaseMetaFileVNode(vcb->allocationsRefNum);
	ReleaseMetaFileVNode(vcb->attributesRefNum);
	ReleaseMetaFileVNode(vcb->catalogRefNum);
	ReleaseMetaFileVNode(vcb->extentsRefNum);

	return (retval);
}


/*
 * ReleaseMetaFileVNode
 *
 * vp	L - -
 */
static void ReleaseMetaFileVNode(struct vnode *vp)
{
	if (vp)
	{
		FCB *fcb = VTOFCB(vp);

		if (fcb->fcbBTCBPtr != NULL)
			(void) BTClosePath(fcb);	/* ignore errors since there is only one path open */

		/* release the node even if BTClosePath fails */
		if (VOP_ISLOCKED(vp))
			VPUT(vp);
		else
			VRELE(vp);
	}
}


/*
 * InitMetaFileVNode
 *
 * vp	U L L
 */
static int InitMetaFileVNode(struct vnode *vp, u_long eof, u_long clumpSize, void *extents,
							 HFSCatalogNodeID fileID, void * keyCompareProc)
{
	FCB				*fcb;
	ExtendedVCB		*vcb;
	int				result = 0;

	DBG_ASSERT(vp != NULL);
    DBG_ASSERT(vp->v_data != NULL);
    DBG_ASSERT(VTOH(vp)->h_xfcb->fcb_vp == vp);

	vcb = VTOVCB(vp);
	fcb = VTOFCB(vp);

 	switch (fileID)
	{
		case kHFSExtentsFileID:
			vcb->extentsRefNum = vp;
			break;

		case kHFSCatalogFileID:
			vcb->catalogRefNum = vp;
			break;

		case kHFSAttributesFileID:
			vcb->attributesRefNum = vp;
			break;

		case kHFSAllocationFileID:
			vcb->allocationsRefNum = vp;
            VTOH(vp)->h_meta->h_logBlockSize = kHFSBlockSize;
            VTOH(vp)->h_meta->h_physBlkPerLogBlk = 1;
			break;

		default:
			panic("InitMetaFileVNode: invalid fileID!");
	}

	fcb->fcbVPtr = vcb;
	fcb->fcbEOF = eof;
	fcb->fcbPLen = eof;
	fcb->fcbClmpSize = clumpSize;
	fcb->fcbFlNm = fileID;
    H_FILEID(VTOH(vp)) = fileID;
    H_FORKTYPE(VTOH(vp)) = kSysFile;

	if (vcb->vcbSigWord == kHFSPlusSigWord) {
    	ExtendedFCB		*xfcb;

		xfcb = &(((struct vfsFCB *)(fcb))->fcb_extFCB);		//XXX use a macro instead?!
		bcopy(extents, xfcb->extents, sizeof(HFSPlusExtentRecord));
	}
	else {
    	bcopy(extents, fcb->fcbExtRec, sizeof(HFSExtentRecord));
	}

    /*
     * Lock the hfsnode and insert the hfsnode into the hash queue:
     */
    hfs_vhashins(VTOH(vp));
	vp->v_flag |= VSYSTEM;	/* tag our metadata files (used by vflush call) */
    
    if (keyCompareProc != NULL) {
		result = BTOpenPath(fcb,
							(KeyCompareProcPtr) keyCompareProc,
							GetBTreeBlock,
							ReleaseBTreeBlock,
							ExtendBTreeFile,
							SetBTreeBlockSize);
		result = MacToVFSError(result);
	}

    return (result);
}


/*************************************************************
*
* Unmounts a hfs volume.
*	At this point vflush() has been called (to dump all non-metadata files)
*
*************************************************************/

short hfsUnmount( register struct hfsmount *hfsmp, struct proc *p)
{
    ExtendedVCB	*vcb = HFSTOVCB(hfsmp);
    int			retval = E_NONE;

	(void) DisposeMRUCache(vcb->hintCachePtr);
	InvalidateCatalogCache( vcb );
	// XXX PPD: Should dispose of any allocated volume cache here: call DisposeVolumeCacheBlocks( vcb )?

	(void) hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_EXCLUSIVE, p);
	(void) hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_EXCLUSIVE, p);

	if (vcb->vcbSigWord == kHFSPlusSigWord) {
		(void) hfs_metafilelocking(hfsmp, kHFSAttributesFileID, LK_EXCLUSIVE, p);

		ReleaseMetaFileVNode(vcb->allocationsRefNum);
		ReleaseMetaFileVNode(vcb->attributesRefNum);
	}

	ReleaseMetaFileVNode(vcb->catalogRefNum);
	ReleaseMetaFileVNode(vcb->extentsRefNum);

	return (retval);
}


/*
 * Performs a lookup on the given dirID, name. Returns the catalog info
 */
/* XXX SER catInfo points to a hint, why do we need to pass one in ? */

short hfsLookup (ExtendedVCB *vcb, UInt32 parentDirID, char *name, short len, UInt32 hint, hfsCatalogInfo *catInfo)
{
	char tmpname[NAME_MAX + 1]; /* UUU */
	OSErr result;
	
	if (len != -1 ) {
		bcopy(name, tmpname, len);
		tmpname[len] = '\0';
        result = GetCatalogNode(vcb, parentDirID, tmpname, hint, &catInfo->spec, &catInfo->nodeData, &catInfo->hint);
	} else
        result = GetCatalogNode(vcb, parentDirID, name, hint, &catInfo->spec, &catInfo->nodeData, &catInfo->hint);

	if (result)
        DBG_ERR(("on Lookup, GetCatalogNode returned: %d: dirid: %ld name: %s\n", result, parentDirID, name));

	return MacToVFSError(result);
}



short hfsDelete (ExtendedVCB *vcb, UInt32 parentDirID, StringPtr name, short isfile, UInt32 catalogHint)
{
    OSErr result = noErr;
    
    /* XXX have all the file's blocks been flushed/trashed? */

	/*
	 * DeleteFile will delete the catalog node and then
	 * free up any disk space used by the file.
	 */
	if (isfile)
		result = DeleteFile(vcb, parentDirID, name, catalogHint);
	else /* is a directory */
		result = DeleteCatalogNode(vcb, parentDirID, name, catalogHint);

    if (result)
        DBG_ERR(("on Delete, DeleteFile returned: %d: dirid: %ld name: %s\n", result, parentDirID, name));
		
   	return MacToVFSError(result);
}


short hfsMoveRename (ExtendedVCB *vcb, UInt32 oldDirID, char *oldName, UInt32 newDirID, char *newName, UInt32 *hint)
{
    OSErr result = noErr;

    result = MoveRenameCatalogNode(vcb, oldDirID,oldName, *hint, newDirID, newName, hint);

    if (result)
        DBG_ERR(("on hfsMoveRename, MoveRenameCatalogNode returned: %d: newdirid: %ld newname: %s\n", result, newDirID, newName));
        

    return MacToVFSError(result);
}

/* XXX SER pass back the hint so other people can use it */


short hfsCreate(ExtendedVCB *vcb, UInt32 dirID, char *name, int	mode)
{
    OSErr				result = noErr;
    HFSCatalogNodeID 	catalogNodeID;
    UInt32 				catalogHint;
    UInt32				type;

	/* just test for directories, the default is to create a file (like symlinks) */
	if ((mode & IFMT) == IFDIR)
		type = kCatalogFolderNode;
	else
		type = kCatalogFileNode;

    result = CreateCatalogNode (vcb, dirID, name, type, &catalogNodeID, &catalogHint);
 
    return MacToVFSError(result);
}


short hfsCreateFileID (ExtendedVCB *vcb, UInt32 parentDirID, StringPtr name, UInt32 catalogHint, UInt32 *fileIDPtr)
{
    return MacToVFSError(CreateFileIDRef(vcb, parentDirID, name, catalogHint, fileIDPtr));
}


/************************************************************************/
/*	hfsGet - Returns a vnode derived from hfs  							*/
/*											 							*/
/************************************************************************/

short hfsGet( ExtendedVCB *vcb, hfsCatalogInfo *catInfo, UInt8 forkType, struct vnode *dvp, struct vnode **vpp)
{
    struct hfsnode		*hp;
    struct vnode 		*vp;
    struct vnode 		*cp;
    struct hfsmount 	*hfsmp;
    struct hfsfilemeta *fm;
    struct mount		*mp;
    struct vfsFCB 		*xfcb;
    dev_t 				dev;
    int					retval;

    DBG_ASSERT(vcb != NULL);
    DBG_ASSERT(catInfo != NULL);
    DBG_ASSERT(vpp != NULL);

    hfsmp 	= VCBTOHFS(vcb);
    mp 		= HFSTOVFS(hfsmp);
    dev 	= hfsmp->hfs_raw_dev;

    DBG_UTILS(("\thfsGet: On '%s' with forktype of %d, nodeType of 0x%08lX\n", catInfo->spec.name, forkType, (unsigned long)catInfo->nodeData.nodeType));

    /* Must malloc() here, since getnewvnode() can sleep */
    MALLOC(hp, struct hfsnode *, sizeof(struct hfsnode), M_HFSNODE, M_WAITOK);
    bzero((caddr_t)hp, sizeof(struct hfsnode));
    lockinit(&hp->h_lock, PINOD, "hfsnode", 0, 0);

    /* getnewvnode() does a VREF() on the vnode */
    /* Allocate a new vnode. */
    if ((retval = getnewvnode(VT_HFS, mp, hfs_vnodeop_p, &vp))) {
        *vpp = NULL;
        goto Err_Exit;
    }

    hp->h_vp = vp;									/* Make HFSTOV work */
    hp->h_dev = hfsmp->hfs_raw_dev;
    vp->v_data = hp;								/* Make VTOH work */
    H_FILEID(hp) = catInfo->nodeData.nodeID;
    H_FORKTYPE(hp) = forkType;

    /*
     * Lock the hfsnode and insert the hfsnode into the hash queue:
     * NOTE: THIS IS DANGEROUS IN CASES WHERE WE NEED TO BACK OUT OF THE CREATION?
     */
    hfs_vhashins(hp);
    hp->h_valid = HFS_VNODE_MAGIC;

    /*
     * Allocate and init the right data structs:
     */
    xfcb = NULL;
    if (catInfo->nodeData.nodeType == kCatalogFolderNode)
      {
        /* Just allocate the data */
        MALLOC(fm, struct hfsfilemeta *, sizeof(struct hfsfilemeta), M_HFSNODE, M_WAITOK);
        bzero(fm, sizeof(struct hfsfilemeta));
        MALLOC(xfcb, struct vfsFCB *, sizeof(struct vfsFCB), M_HFSNODE, M_WAITOK);
        bzero(xfcb, sizeof(struct vfsFCB));
        fm->h_logBlockSize = hfsmp->hfs_log_block_size;
        fm->h_physBlkPerLogBlk =  fm->h_logBlockSize/kHFSBlockSize;
      }
    else
      {
        if (forkType==kDirCmplx)
          {
            /* Just allocate the meta part */
            MALLOC(fm, struct hfsfilemeta *, sizeof(struct hfsfilemeta), M_HFSNODE, M_WAITOK);
            bzero(fm, sizeof(struct hfsfilemeta));
            lockinit(&fm->h_fmetalock, PINOD, "hfsfilemeta", 0, 0);
            fm->h_logBlockSize = hfsmp->hfs_log_block_size;
            fm->h_physBlkPerLogBlk =  fm->h_logBlockSize/kHFSBlockSize;
          }
        else
          {
            Boolean 	unlockPar = false;
			
			/* Get the complex vnode, creating it if none existed */
            if ((dvp != NULL) && (dvp->v_type == VCPLX)) {
                cp = dvp;
            }
            else {
                cp = hfs_vhashget(hp->h_dev, H_FILEID(hp), kDirCmplx);
                unlockPar = TRUE;
            }
            
            if (cp == NULL)
              {
                if ((retval = hfsGet(vcb, catInfo, kDirCmplx, NULL, &cp)))
                  {
                    /* Couldn't get a complex node: back out of this vnode's creation
                       since an hfs vnode can't stand alone: */
                    vp->v_flag |= VSYSTEM;		/* Prevent hfs_lock et al. from going for meta lock... */
                    VPUT(vp);
                    goto Err_Exit;
                  };
                unlockPar = TRUE;
              }

            fm = VTOH(cp)->h_meta;
			DBG_ASSERT(fm);
            /* Increment the usecount, to make sure it doesnt go away on us */
            if (++fm->h_usecount == 0)
		panic("hfsGet: h_usecount");
            lockmgr(&fm->h_fmetalock, LK_EXCLUSIVE, (struct slock *)0, CURRENT_PROC);
            hp->h_relative = cp;
            if (forkType == kDataFork)					/* Init downlink */
                VTOH(cp)->h_relative = vp;
            if (fm->h_fork == NULL)
                fm->h_fork = vp;
            else {
                hp->h_sibling = fm->h_fork;
                VTOH(fm->h_fork)->h_sibling = vp;
            }
            /* Increment the complex usecount, to make sure it doesnt go inactive on us */
            VREF(cp);
            if  (unlockPar)
                VPUT(cp);
          }
        MALLOC(xfcb, struct vfsFCB *, sizeof(struct vfsFCB), M_HFSNODE, M_WAITOK);
        bzero(xfcb, sizeof(struct vfsFCB));
      }

    hp->h_xfcb = xfcb;
    hp->h_meta = fm;

    /*
     * Finish filling in the record
     */
    if (xfcb) {
        xfcb->fcb_vp = vp;					/* Make FCBTOV work */
    }

    /*
     * Copy the Catalog info to the new macnode
     */
    CopyCatalogToHFSNode(catInfo, hp);


	/* Lets do some testing here */
    DBG_ASSERT(hp->h_meta);
    DBG_ASSERT(VTOH(vp)==hp);
    DBG_ASSERT(HTOV(hp)==vp);
    if (hp->h_xfcb) {
        DBG_ASSERT(VTOH(vp)->h_xfcb->fcb_vp == vp);
        DBG_ASSERT(FCBTOV(&hp->h_xfcb->fcb_fcb) == vp);
        DBG_ASSERT(FCBTOH(&hp->h_xfcb->fcb_fcb) == hp);
        DBG_ASSERT(&hp->h_xfcb->fcb_fcb == VTOFCB(vp));
        DBG_ASSERT(&hp->h_xfcb->fcb_fcb == HTOFCB(hp));
        DBG_ASSERT(VTOVCB(vp) == FCBTOVCB(&hp->h_xfcb->fcb_fcb));
    }
    if (hp->h_sibling)	{
        DBG_ASSERT(VTOH(hp->h_sibling)->h_sibling == vp);
        DBG_ASSERT(VTOH(hp->h_sibling)->h_relative == hp->h_relative);
        DBG_ASSERT(VTOH(hp->h_sibling)->h_meta == hp->h_meta);
    }
    if (H_FORKTYPE(hp) == kDirCmplx)	{
        DBG_ASSERT(hp->h_meta->h_usecount==0);
    }
	else {
        DBG_ASSERT(hp->h_meta->h_usecount>0 && hp->h_meta->h_usecount<=2);
	}
	
    /*
     * Finish inode initialization now that aliasing has been resolved.
     */
    hp->h_devvp = hfsmp->hfs_devvp;
    VREF(hp->h_devvp);
    *vpp = vp;
    return 0;

Err_Exit:;
    FREE (hp, M_HFSNODE);
    *vpp = NULL;
    return (retval);
}


int hasOverflowExtents(struct hfsnode *hp)
{
	ExtendedVCB		*vcb = HTOVCB(hp);
	FCB				*fcb = HTOFCB(hp);
	u_long			blocks;

	if (vcb->vcbSigWord == kHFSPlusSigWord)
	  {
		ExtendedFCB *xfcb;

		xfcb = GetParallelFCB(HTOV(hp));
		if (xfcb->extents[7].blockCount == 0)
			return false;
		
		blocks = xfcb->extents[0].blockCount +
				 xfcb->extents[1].blockCount +
				 xfcb->extents[2].blockCount +
				 xfcb->extents[3].blockCount +
				 xfcb->extents[4].blockCount +
				 xfcb->extents[5].blockCount +
				 xfcb->extents[6].blockCount +
				 xfcb->extents[7].blockCount;	
	  }
	else
	  {
		if (fcb->fcbExtRec[2].blockCount == 0)
			return false;
		
		blocks = fcb->fcbExtRec[0].blockCount +
				 fcb->fcbExtRec[1].blockCount +
				 fcb->fcbExtRec[2].blockCount;	
	  }

	return ((fcb->fcbPLen / vcb->blockSize) > blocks);
}


int hfs_metafilelocking(struct hfsmount *hfsmp, u_long fileID, u_int flags, struct proc *p)
{
	ExtendedVCB		*vcb;
	struct vnode	*vp = NULL;
    struct timeval tv;
	int				numOfLockedBuffs;
	int	retval = 0;

	vcb = HFSTOVCB(hfsmp);

    DBG_VFS(("  hfs_metafilelocking: vol: %d, file: %ld %s%s%s\n", vcb->vcbVRefNum, fileID,
            ((flags & LK_TYPE_MASK) == LK_RELEASE ? "RELEASE" : ""),
            ((flags & LK_TYPE_MASK) == LK_EXCLUSIVE ? "EXCLUSIVE" : ""),
            ((flags & LK_TYPE_MASK) == LK_SHARED ? "SHARED" : "") ));

   /*kprintf("file: %ld flags: 0x%x %s%s%s\n",fileID, flags,
            ((flags & LK_TYPE_MASK) == LK_RELEASE ? "RELEASE" : ""),
            ((flags & LK_TYPE_MASK) == LK_EXCLUSIVE ? "EXCLUSIVE" : ""),
            ((flags & LK_TYPE_MASK) == LK_SHARED ? "SHARED" : "") );
*/

 	switch (fileID)
	{
		case kHFSExtentsFileID:
			vp = vcb->extentsRefNum;
			break;

		case kHFSCatalogFileID:
			vp = vcb->catalogRefNum;
			break;

		case kHFSAttributesFileID:
			if (vcb->vcbSigWord == kHFSPlusSigWord) {
				vp = vcb->attributesRefNum;
				break;
			}
			// fall through for hfs...

		case kHFSAllocationFileID:
			// bitmap is covered by Extents B-tree locking
		default:
			panic("hfs_lockmetafile: invalid fileID");
	}
    /*kprintf("\tBefore: %d\n", lockstatus(&VTOH(vp)->h_lock));
*/
    if (vp != NULL) {

        /* Release, if necesary any locked buffer caches */
        if (flags & LK_RELEASE) {
            numOfLockedBuffs =  count_lock_queue();
            tv = time;
            if ((numOfLockedBuffs > 32) || ((numOfLockedBuffs>1) && ((tv.tv_sec - VTOH(vp)->h_lastfsync) > kMaxSecsForFsync))) {
               // kprintf("Synching meta deta...locked buffers = %d, fsync gap = %ld\n", numOfLockedBuffs, (tv.tv_sec - VTOH(vp)->h_lastfsync));
                VOP_FSYNC(vp, NOCRED, MNT_NOWAIT, p);
            };
        };

        retval = lockmgr(&VTOH(vp)->h_lock, flags, &vp->v_interlock, p);
    };

	return retval;
}


void CopyVNodeToCatalogNode (struct vnode *vp, struct CatalogNodeData *nodeData)
{
    ExtendedVCB 			*vcb;
    FCB						*fcb;
    struct hfsnode 			*hp;
    Boolean					isHFSPlus, isResource;
    HFSPlusExtentDescriptor	*extents, *parellextents;
    ExtendedFCB				*extendedFCB;

    hp = VTOH(vp);
    vcb = HTOVCB(hp);
    fcb = HTOFCB(hp);
    isResource = (H_FORKTYPE(hp) == kRsrcFork);
    isHFSPlus = (vcb->vcbSigWord == kHFSPlusSigWord);

    nodeData->contentModDate = to_hfs_time(hp->h_meta->h_mtime);		/* date and time of last fork modification */

    DBG_ASSERT(fcb->fcbFlNm == H_FILEID(hp));

    if (isHFSPlus) {
        nodeData->attributeModDate = to_hfs_time(hp->h_meta->h_ctime);	/* date and time of last modification (any kind) */
        nodeData->accessDate = to_hfs_time(hp->h_meta->h_atime);		/* date and time of last access (Rhapsody only) */
        if (! (hp->h_meta->h_nodeflags & IN_UNSETACCESS)) {
            /* This is a tricky stuff-job: the pflags longword has two bytes of significance and they're
               combined with the mode field to yield a 32-bit permissions field as follows:

                                         +------------------------------------+
                   hp->h_meta->h_pflags: |XXXXXXXX|    A    |XXXXXXXX|    B   |
                                         +------------------------------------+

                                                            |
                                                            V

                                         +------------------------------------+
                            permissions: |    A    |    B   |      mode       |
                                         +------------------------------------+
             */
            nodeData->permissions.permissions = (((hp->h_meta->h_pflags << 8)  & 0xFF000000) |	/* A */
                                                 ((hp->h_meta->h_pflags << 16) & 0x00FF0000) |	/* B */
                                                 (hp->h_meta->h_mode           & 0x0000FFFF));
            nodeData->permissions.ownerID =  hp->h_meta->h_uid;
            nodeData->permissions.groupID = hp->h_meta->h_gid;
            nodeData->permissions.specialDevice = hp->h_meta->h_rdev;
            };
    };

	/* the rest only applies to files */
	if (nodeData->nodeType == kCatalogFileNode) {
        if (hp->h_meta->h_pflags & (SF_IMMUTABLE | UF_IMMUTABLE)) {
            /* The file is locked: set the locked bit in the catalog. */
            nodeData->nodeFlags |= kHFSFileLockedMask;
        } else {
            /* The file is unlocked: make sure the locked bit in the catalog is clear. */
            nodeData->nodeFlags &= ~kHFSFileLockedMask;
        };
		if (isResource) {
			extents = nodeData->rsrcExtents;
			nodeData->rsrcLogicalSize = fcb->fcbEOF;
			nodeData->rsrcPhysicalSize = fcb->fcbPLen;
		} else {
			extents = nodeData->dataExtents;
			nodeData->dataLogicalSize = fcb->fcbEOF;
			nodeData->dataPhysicalSize = fcb->fcbPLen;
		};

	    /* Copy the extents from their correct location */
	    if ( ! isHFSPlus) {
	        /* HFS, Copy the extents from the FCB part */
	        int i;
	        for (i = 0; i < kHFSExtentDensity; ++i) {
	            extents[i].startBlock = (UInt16) (fcb->fcbExtRec[i].startBlock);
	            extents[i].blockCount = (UInt16) (fcb->fcbExtRec[i].blockCount);
	            }
	    } else {
	        /* HFSPlus, Copy the extents from the parallel extents */
	        extendedFCB = GetParallelFCB (HTOV(hp));
	        parellextents = extendedFCB->extents;
	        bcopy ( parellextents, extents, sizeof(HFSPlusExtentRecord));
	    };
	
	    if (vp->v_type == VLNK) {
	        ((struct FInfo *)(&nodeData->finderInfo))->fdType = kSymLinkFileType;
	        ((struct FInfo *)(&nodeData->finderInfo))->fdCreator = kSymLinkCreator;
	
			/* Set this up as an alias */
			#if SUPPORTS_MAC_ALIASES
				((struct FInfo *)(&nodeData->finderInfo))->fdFlags |= kIsAlias;
			#endif
		}
	}
 }



void MapFileOffset(struct hfsnode *hp,
                   off_t filePosition,
                   daddr_t *logBlockNumber,
                   long *blockSize,
                   long *blockOffset) {
    off_t extentOffset = filePosition;
    daddr_t precedingBlockCount;
    unsigned long logicalBlockSize = 0;
    unsigned long spaceRemaining;

    DBG_IO(("MapFileOffset: hp = 0x%08lX, vp = 0x%08lX ('%s'):\n", (u_long)hp, (u_long)HTOV(hp), H_NAME(hp)));
    // DBG_IO(("\tfilePosition 0x%08lX", (u_long)filePosition));

    if (filePosition < hp->h_optimizedblocksizelimit) {
        int extent;

        precedingBlockCount = 0;
        for (extent = 0; extent < LOGBLOCKMAPENTRIES; ++extent) {
            if ((hp->h_logicalblocktable[extent].logicalBlockCount > 0) &&
                (extentOffset < hp->h_logicalblocktable[extent].extentLength)) {
                logicalBlockSize = MAXLOGBLOCKSIZE;
                spaceRemaining = hp->h_logicalblocktable[extent].extentLength - ((extentOffset / logicalBlockSize) * logicalBlockSize);
                *blockSize = MIN(logicalBlockSize, spaceRemaining);
                // DBG_IO(("\tUsing entry #%d: preceding blocks = 0x%X, position = 0x%8lX:\n",
                //         extent, precedingBlockCount, (u_long)extentOffset));
                break;
            };
            precedingBlockCount += hp->h_logicalblocktable[extent].logicalBlockCount;
            extentOffset -= hp->h_logicalblocktable[extent].extentLength;
        };
        DBG_ASSERT(logicalBlockSize > 0);
    } else {
        // DBG_IO(("\tUsing h_uniformblocksizestart = 0x%X and h_optimizedblocksizelimit = 0x%lX:\n",
        //         hp->h_uniformblocksizestart, (u_long)hp->h_optimizedblocksizelimit));
        precedingBlockCount = hp->h_uniformblocksizestart;
        extentOffset -= hp->h_optimizedblocksizelimit;
        logicalBlockSize = hp->h_meta->h_logBlockSize;
        *blockSize = logicalBlockSize;
    };

    *logBlockNumber =  precedingBlockCount + (extentOffset / logicalBlockSize);
    *blockOffset = extentOffset % logicalBlockSize;
    DBG_IO(("\tfilePosition 0x%08lX -> logBlockNo = 0x%X, blockSize = 0x%lX, blockOffset = 0x%lX.\n",
                 (u_long)filePosition,
                 *logBlockNumber,
                 *blockSize,
                 *blockOffset));
}


long LogicalBlockSize(struct hfsnode *hp, daddr_t logicalBlockNumber) {
    if (logicalBlockNumber < hp->h_uniformblocksizestart) {
        off_t fragmentOffset = 0;
        int extent;

        for (extent = 0; extent < LOGBLOCKMAPENTRIES; ++extent) {
            if ((hp->h_logicalblocktable[extent].logicalBlockCount > 0) &&
                (logicalBlockNumber < hp->h_logicalblocktable[extent].logicalBlockCount)) {
                // *filePosition = fragmentOffset + (logicalBlockNumber * MAXLOGBLOCKSIZE);
                return MIN(MAXLOGBLOCKSIZE, hp->h_logicalblocktable[extent].extentLength - (logicalBlockNumber * MAXLOGBLOCKSIZE));
            };
            logicalBlockNumber -= hp->h_logicalblocktable[extent].logicalBlockCount;
            fragmentOffset += hp->h_logicalblocktable[extent].extentLength;
        };
    } else {
        // *filePosition = hp->h_optimizedblocksizelimit + ((logicalBlockNumber - hp->h_uniformblocksizestart) * hp->h_meta->h_logBlockSize);
        return hp->h_meta->h_logBlockSize;
    };

    DBG_ASSERT(0 /* cannot be reached */);
    return 0;
};


void UpdateBlockMappingTableEntry(struct hfsnode *hp, int index, daddr_t firstFragmentBlockNumber, long newFragmentLength) {
//  off_t					currentFragmentLength;

    /* Compute the number of logical blocks, rounding up to include any small trailing fragments: */
    hp->h_logicalblocktable[index].logicalBlockCount = (newFragmentLength + MAXLOGBLOCKSIZE - 1) / MAXLOGBLOCKSIZE;

    DBG_IO(("\tblocktable[%d]: length = 0x%lX, # blocks = 0x%lX.\n",
            index,
            newFragmentLength,
            (u_long)hp->h_logicalblocktable[index].logicalBlockCount));

#if 0
    currentFragmentLength = hp->h_logicalblocktable[index].extentLength;
    if ((newFragmentLength > currentFragmentLength) && (currentFragmentLength > 0)) {
        daddr_t					currentLastBlockNumber;
        long					currentLastBlockSize;
        long					newLastBlockSize;
        struct buf				*bp;
        int						retval;

        currentLastBlockNumber = firstFragmentBlockNumber + (currentFragmentLength / MAXLOGBLOCKSIZE);
        currentLastBlockSize = currentFragmentLength % MAXLOGBLOCKSIZE;
        if (currentLastBlockSize > 0) {
            newLastBlockSize = MIN(MAXLOGBLOCKSIZE,
                                   newFragmentLength -
                                   ((currentLastBlockNumber - firstFragmentBlockNumber) * MAXLOGBLOCKSIZE));
            if (newLastBlockSize != currentLastBlockSize) {
                DBG_IO(("\t\t(Adjusting block 0x%lX from 0x%lX to 0x%lX...)\n",
                        (unsigned long)currentLastBlockNumber,
                        currentLastBlockSize,
                        newLastBlockSize));
                retval = bread(HTOV(hp), currentLastBlockNumber, currentLastBlockSize, NOCRED, &bp);
                if (retval == 0) {
                    bexpand(bp, newLastBlockSize, NULL, RELEASE_BUFFER);
                } else {
                    DBG_IO(("\t\terror (%d.) acquiring block 0x%lX; bp = 0x%08lX\n",
                            retval,
                            (unsigned long)currentLastBlockNumber,
                            (unsigned long)bp));
                    if (bp) brelse(bp);
                };
            };
        };
    };
#endif
    hp->h_logicalblocktable[index].extentLength = newFragmentLength;
}


void UpdateBlockMappingTable(struct hfsnode *hp)
{
    ExtendedVCB				*vcb = HTOVCB(hp);
    FCB						*fcb = HTOFCB(hp);
    ExtendedFCB				*extendedFCB;
    Boolean					isHFSPlus;
    HFSPlusExtentDescriptor *parellextents;
    off_t					rangeMappedSoFar;
    u_long					logicalBlocksMappedSoFar;
    int						i;
    off_t					newFragmentLength;

    isHFSPlus = (vcb->vcbSigWord == kHFSPlusSigWord);
    DBG_IO(("UpdateBlockMappingTable: hp = 0x%08lX, vp = 0x%08lX ('%s'):\n", (u_long)hp, (u_long)HTOV(hp), H_NAME(hp)));
    if (H_FORKTYPE(hp) != kDirCmplx) {
        rangeMappedSoFar = 0;
        logicalBlocksMappedSoFar = 0;

        if ( ! isHFSPlus) {
            DBG_ASSERT(kHFSExtentDensity <= LOGBLOCKMAPENTRIES);
            for (i = 0; i < kHFSExtentDensity; ++i) {
                newFragmentLength = fcb->fcbExtRec[i].blockCount * vcb->blockSize;
                UpdateBlockMappingTableEntry(hp, i, logicalBlocksMappedSoFar, newFragmentLength);
                rangeMappedSoFar += newFragmentLength;
                logicalBlocksMappedSoFar += hp->h_logicalblocktable[i].logicalBlockCount;
            }
            /* Zero out any remaining entries: */
            for (i = kHFSExtentDensity; i < LOGBLOCKMAPENTRIES; ++i) {
                hp->h_logicalblocktable[i].extentLength = 0;
            };
#if DIAGNOSTIC
            if (rangeMappedSoFar < fcb->fcbPLen) DBG_ASSERT(hp->h_logicalblocktable[kHFSExtentDensity-1].extentLength > 0);
#endif
        } else {
            extendedFCB = GetParallelFCB (FCBTOV(fcb));
            parellextents = extendedFCB->extents;
            for (i = 0; i < kHFSPlusExtentDensity; ++i) {
                newFragmentLength = parellextents[i].blockCount * vcb->blockSize;
                UpdateBlockMappingTableEntry(hp, i, logicalBlocksMappedSoFar, newFragmentLength);
                rangeMappedSoFar += newFragmentLength;
                logicalBlocksMappedSoFar += hp->h_logicalblocktable[i].logicalBlockCount;
            }
            /* No need to zero out the remaining entries [there are none]: */
            DBG_ASSERT(LOGBLOCKMAPENTRIES <= kHFSPlusExtentDensity);
#if DIAGNOSTIC
            if (rangeMappedSoFar < fcb->fcbPLen) DBG_ASSERT(hp->h_logicalblocktable[kHFSPlusExtentDensity-1].extentLength > 0);
#endif
        };
        
        hp->h_optimizedblocksizelimit = rangeMappedSoFar;
        hp->h_uniformblocksizestart = logicalBlocksMappedSoFar;
        DBG_IO(("\th_optimizedblocksizelimit = 0x%lX, h_uniformblocksizestart = 0x%lX.\n",
                (u_long)hp->h_optimizedblocksizelimit, (u_long)hp->h_uniformblocksizestart));

#if BYPASSBLOCKINGOPTIMIZATION
        hp->h_optimizedblocksizelimit = 0;
        hp->h_uniformblocksizestart = 0;
#endif
    };
}

void CopyCatalogToHFSNode(struct hfsCatalogInfo *catalogInfo, struct hfsnode *hp)
{
    FCB						*fcb;
	ExtendedVCB				*vcb = HTOVCB(hp);
    Boolean					isHFSPlus, isDirectory, isResource;
    HFSPlusExtentDescriptor	*extents, *parellextents;
    ExtendedFCB				*extendedFCB;
    ushort					finderFlags;
	UInt8					forkType;

    DBG_ASSERT (hp != NULL);
    DBG_ASSERT (hp->h_valid == HFS_VNODE_MAGIC);
    DBG_ASSERT (hp->h_meta != NULL);

    DBG_UTILS(("\tCopying to vnode Ox%08lX: name:%s, type:%d, nodeid:%ld\n", (unsigned long)HTOV(hp), catalogInfo->spec.name, H_FORKTYPE(hp), catalogInfo->nodeData.nodeID));
    forkType 		= H_FORKTYPE(hp);
    isResource 		= (forkType == kRsrcFork);
    isDirectory		= (catalogInfo->nodeData.nodeType == kCatalogFolderNode);
    isHFSPlus 		= (vcb->vcbSigWord == kHFSPlusSigWord);
    finderFlags 	= ((struct FInfo *)(&catalogInfo->nodeData.finderInfo))->fdFlags;
    DBG_UTILS(("\t\t forkType:%d, isResource:%d, isDirectory:%d, isHFSPlus:%d\n", forkType, isResource, isDirectory, isHFSPlus));

	/* Copy over the name if NOT set yet */
	if (H_NAME(hp) == NULL) {
	    hp->h_meta->h_namelen = strlen(catalogInfo->spec.name);
	    if (hp->h_meta->h_namelen <= MAXHFSVNODELEN) {
	        H_NAME(hp) = hp->h_meta->h_fileName;
	    }
	    else {
	        MALLOC(H_NAME(hp), char *, hp->h_meta->h_namelen+1, M_HFSNODE, M_WAITOK);
	        hp->h_meta->h_nodeflags |= IN_LONGNAME;
	    };
    	bcopy(catalogInfo->spec.name, H_NAME(hp), hp->h_meta->h_namelen+1);
	 }
	 else {
		 DBG_ASSERT (hp->h_meta->h_namelen == strlen(catalogInfo->spec.name));
		 DBG_ASSERT (strcmp(H_NAME(hp), catalogInfo->spec.name) == 0);
	 };

	/* Copy over the dirid, and hint */
    H_FILEID(hp) 	= catalogInfo->nodeData.nodeID;
    H_DIRID(hp)		= catalogInfo->spec.parID;
    H_HINT(hp) 		= catalogInfo->hint;

    if (hp->h_xfcb != NULL) {

        /* Init the fcb */
        fcb 			= HTOFCB(hp);
        fcb->fcbVPtr 	= vcb;
        fcb->fcbFlNm	= catalogInfo->nodeData.nodeID;
        fcb->fcbFlags	= catalogInfo->nodeData.nodeFlags;

        if (forkType != kDirCmplx) {
            fcb->fcbFlags &= kHFSFileLockedMask;		/* Clear resource, dirty bits */
            if (fcb->fcbFlags != 0)						/* if clear, its not locked, then.. */
                fcb->fcbFlags = fcbFileLockedMask;		/* duplicate the bit for later use */

            fcb->fcbClmpSize = vcb->vcbClpSiz;	/*XXX why not use the one in catalogInfo? */

            if (isResource)	
                extents = catalogInfo->nodeData.rsrcExtents;
            else
                extents = catalogInfo->nodeData.dataExtents;

            /* Copy the extents to their correct location: */
            if ( ! isHFSPlus) {
                int i;
                for (i = 0; i < kHFSExtentDensity; ++i) {
                    fcb->fcbExtRec[i].startBlock = (UInt32) (extents[i].startBlock);
                    fcb->fcbExtRec[i].blockCount = (UInt32) (extents[i].blockCount);
                }
            } else {
                /* HFSPlus, Copy the extents to the parallel extents */
                extendedFCB = GetParallelFCB (FCBTOV(fcb));
                parellextents = extendedFCB->extents;
                bcopy (extents, parellextents, sizeof(HFSPlusExtentRecord));
            };
            UpdateBlockMappingTable(hp);

            /* the valence field is overloaded to flag files over 2 gig */
            /* since we are working with a single fork, isolate it flags <why?> */
            /*  secondFlags = catalogInfo->nodeData.valence; */

            if (isResource)	{	
                fcb->fcbEOF = catalogInfo->nodeData.rsrcLogicalSize;
                fcb->fcbPLen = catalogInfo->nodeData.rsrcPhysicalSize;
                fcb->fcbFlags |= fcbResourceMask;
            }  else {
                fcb->fcbEOF = catalogInfo->nodeData.dataLogicalSize;
                fcb->fcbPLen = catalogInfo->nodeData.dataPhysicalSize;
            };
        };
	};

    hp->h_meta->h_mtime = to_bsd_time(catalogInfo->nodeData.contentModDate);	/* UNIX-format mod date in seconds */
    hp->h_meta->h_crtime = to_bsd_time(catalogInfo->nodeData.createDate);		/* UNIX-format creation date in secs. */
    hp->h_meta->h_butime = to_bsd_time(catalogInfo->nodeData.backupDate);		/* UNIX-format backup date in secs. */
    if (isHFSPlus) {
        hp->h_meta->h_atime = to_bsd_time(catalogInfo->nodeData.accessDate);	/* UNIX-format mod date in seconds */
        hp->h_meta->h_ctime = to_bsd_time(catalogInfo->nodeData.attributeModDate);	/* UNIX-format status change date */
    }
    else {
        hp->h_meta->h_atime = to_bsd_time(catalogInfo->nodeData.contentModDate); /* UNIX-format mod date in seconds */
        hp->h_meta->h_ctime = to_bsd_time(catalogInfo->nodeData.contentModDate); /* UNIX-format status change date */
    }

    /* Now the rest */
    if (isHFSPlus && (catalogInfo->nodeData.permissions.permissions & IFMT)) {
        hp->h_meta->h_uid = catalogInfo->nodeData.permissions.ownerID;
        hp->h_meta->h_gid = catalogInfo->nodeData.permissions.groupID;
        /* The 32-bit permissions field is unpacked to yield the two significant bytes of flags and the
           mode as follows:
                                     +------------------------------------+
                        permissions: |    A    |    B   |      mode       |
                                     +------------------------------------+

                                                        |
                                                        V

                                     +------------------------------------+
               hp->h_meta->h_pflags: |XXXXXXXX|    A    |XXXXXXXX|    B   |
                                     +------------------------------------+

         */
        hp->h_meta->h_pflags = (((catalogInfo->nodeData.permissions.permissions & 0xFF000000) >> 8) |	/* A */
                                ((catalogInfo->nodeData.permissions.permissions & 0x00FF0000) >> 16));	/* B */
        hp->h_meta->h_mode = (mode_t)(catalogInfo->nodeData.permissions.permissions & 0x0000FFFF);
        hp->h_meta->h_rdev = catalogInfo->nodeData.permissions.specialDevice;
    } else {
        /*
         *	Set the permissions as determined by the mount auguments
         *	but keep in account if the file or folder is hfs locked
         */	
        hp->h_meta->h_nodeflags |= IN_UNSETACCESS;			
        hp->h_meta->h_uid = HTOHFS(hp)->hfs_uid;
        hp->h_meta->h_gid = HTOHFS(hp)->hfs_gid;
        /* Default access is full read/write/execute: */
        hp->h_meta->h_mode = ACCESSPERMS;	/* 0777: rwxrwxrwx */
        
        /* ... but no more than that permitted by the mount point's: */
        if (isDirectory) {
            hp->h_meta->h_mode &= HTOHFS(hp)->hfs_dir_mask;
		}
		else {
            hp->h_meta->h_mode &= HTOHFS(hp)->hfs_file_mask;
		}
        
        if(isDirectory)
            hp->h_meta->h_mode |= IFDIR;
        else if (SUPPORTS_MAC_ALIASES && (finderFlags & kIsAlias))	/* aliases will be symlinks in the future */
            hp->h_meta->h_mode |= IFLNK;
        else
            hp->h_meta->h_mode |= IFREG;

    };

    /* Make sure the IMMUTABLE bits are in sync with the locked flag in the catalog: */
    if (!isDirectory) {
        if (catalogInfo->nodeData.nodeFlags & kHFSFileLockedMask) {
            /* The file's supposed to be locked:
               Make sure at least one of the IMMUTABLE bits is set: */
            if ((hp->h_meta->h_pflags & (SF_IMMUTABLE | UF_IMMUTABLE)) == 0) {
                hp->h_meta->h_pflags |= UF_IMMUTABLE;				/* Set the user-changable IMMUTABLE bit */
            };
        } else {
            /* The file's supposed to be unlocked: */
            hp->h_meta->h_pflags &= ~(SF_IMMUTABLE | UF_IMMUTABLE);
        };
    };

	if (isDirectory)
    	hp->h_size = sizeof (hfsdirentry) * (catalogInfo->nodeData.valence + 2);
	else
    	hp->h_size = catalogInfo->nodeData.rsrcPhysicalSize + catalogInfo->nodeData.dataPhysicalSize;
	
    /* finish up some vnode stuff */
    /* Set the root flag if necesary */
   	switch (hp->h_meta->h_mode & IFMT) {
        case IFDIR:
            HTOV(hp)->v_type = VDIR;
            if (H_FILEID(hp) == kRootDirID)
                HTOV(hp)->v_flag |= VROOT;
            break;
        case IFREG:
            if (forkType == kDirCmplx)
                HTOV(hp)->v_type = VCPLX;
            else
                HTOV(hp)->v_type = VREG;
            break;
        case IFLNK:
            if (forkType == kDirCmplx)
                HTOV(hp)->v_type = VCPLX;
            else
            	HTOV(hp)->v_type = VLNK;
            break;
        default:
            DBG_ASSERT(false);
            HTOV(hp)->v_type = VBAD;
    };		
}

int AttributeBlockSize(struct attrlist *attrlist) {
	int size;
	attrgroup_t a;
	
#if ((ATTR_CMN_NAME			| ATTR_CMN_DEVID			| ATTR_CMN_FSID 			| ATTR_CMN_OBJTYPE 		| \
      ATTR_CMN_OBJTAG		| ATTR_CMN_OBJID			| ATTR_CMN_OBJPERMANENTID	| ATTR_CMN_PAROBJID		| \
      ATTR_CMN_SCRIPT		| ATTR_CMN_CRTIME			| ATTR_CMN_MODTIME			| ATTR_CMN_CHGTIME		| \
      ATTR_CMN_ACCTIME		| ATTR_CMN_BKUPTIME			| ATTR_CMN_FNDRINFO			| ATTR_CMN_OWNERID		| \
      ATTR_CMN_GRPID		| ATTR_CMN_ACCESSMASK		| ATTR_CMN_NAMEDATTRCOUNT	| ATTR_CMN_NAMEDATTRLIST| \
      ATTR_CMN_FLAGS) != ATTR_CMN_VALIDMASK)
#error AttributeBlockSize: Missing bits in common mask computation!
#endif
          DBG_ASSERT((attrlist->commonattr & ~ATTR_CMN_VALIDMASK) == 0);

#if ((ATTR_VOL_FSTYPE		| ATTR_VOL_SIGNATURE		| ATTR_VOL_SIZE				| ATTR_VOL_SPACEFREE 	| \
      ATTR_VOL_SPACEAVAIL	| ATTR_VOL_MINALLOCATION	| ATTR_VOL_ALLOCATIONCLUMP	| ATTR_VOL_IOBLOCKSIZE	| \
      ATTR_VOL_OBJCOUNT		| ATTR_VOL_FILECOUNT		| ATTR_VOL_DIRCOUNT			| ATTR_VOL_MAXOBJCOUNT	| \
      ATTR_VOL_MOUNTPOINT	| ATTR_VOL_NAME				| ATTR_VOL_MOUNTFLAGS       | ATTR_VOL_INFO) != ATTR_VOL_VALIDMASK)
#error AttributeBlockSize: Missing bits in volume mask computation!
#endif
          DBG_ASSERT((attrlist->volattr & ~ATTR_VOL_VALIDMASK) == 0);

#if ((ATTR_DIR_LINKCOUNT | ATTR_DIR_ENTRYCOUNT) != ATTR_DIR_VALIDMASK)
#error AttributeBlockSize: Missing bits in directory mask computation!
#endif
      DBG_ASSERT((attrlist->dirattr & ~ATTR_DIR_VALIDMASK) == 0);
#if ((ATTR_FILE_LINKCOUNT	| ATTR_FILE_TOTALSIZE		| ATTR_FILE_ALLOCSIZE 		| ATTR_FILE_IOBLOCKSIZE 	| \
      ATTR_FILE_CLUMPSIZE	| ATTR_FILE_DEVTYPE			| ATTR_FILE_FILETYPE		| ATTR_FILE_FORKCOUNT		| \
      ATTR_FILE_FORKLIST	| ATTR_FILE_DATALENGTH		| ATTR_FILE_DATAALLOCSIZE	| ATTR_FILE_DATAEXTENTS		| \
      ATTR_FILE_RSRCLENGTH	| ATTR_FILE_RSRCALLOCSIZE	| ATTR_FILE_RSRCEXTENTS) != ATTR_FILE_VALIDMASK)
#error AttributeBlockSize: Missing bits in file mask computation!
#endif
          DBG_ASSERT((attrlist->fileattr & ~ATTR_FILE_VALIDMASK) == 0);

#if ((ATTR_FORK_TOTALSIZE | ATTR_FORK_ALLOCSIZE) != ATTR_FORK_VALIDMASK)
#error AttributeBlockSize: Missing bits in fork mask computation!
#endif
      DBG_ASSERT((attrlist->forkattr & ~ATTR_FORK_VALIDMASK) == 0);

	size = 0;
	
	if ((a = attrlist->commonattr) != 0) {
        if (a & ATTR_CMN_NAME) size += sizeof(struct attrreference);
		if (a & ATTR_CMN_DEVID) size += sizeof(dev_t);
		if (a & ATTR_CMN_FSID) size += sizeof(fsid_t);
		if (a & ATTR_CMN_OBJTYPE) size += sizeof(fsobj_type_t);
		if (a & ATTR_CMN_OBJTAG) size += sizeof(fsobj_tag_t);
		if (a & ATTR_CMN_OBJID) size += sizeof(fsobj_id_t);
        if (a & ATTR_CMN_OBJPERMANENTID) size += sizeof(fsobj_id_t);
		if (a & ATTR_CMN_PAROBJID) size += sizeof(fsobj_id_t);
		if (a & ATTR_CMN_SCRIPT) size += sizeof(text_encoding_t);
		if (a & ATTR_CMN_CRTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_MODTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_CHGTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_ACCTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_BKUPTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_FNDRINFO) size += 32 * sizeof(UInt8);
		if (a & ATTR_CMN_OWNERID) size += sizeof(uid_t);
		if (a & ATTR_CMN_GRPID) size += sizeof(gid_t);
		if (a & ATTR_CMN_ACCESSMASK) size += sizeof(u_long);
		if (a & ATTR_CMN_NAMEDATTRCOUNT) size += sizeof(u_long);
		if (a & ATTR_CMN_NAMEDATTRLIST) size += sizeof(struct attrreference);
		if (a & ATTR_CMN_FLAGS) size += sizeof(u_long);
	};
	if ((a = attrlist->volattr) != 0) {
		if (a & ATTR_VOL_FSTYPE) size += sizeof(u_long);
		if (a & ATTR_VOL_SIGNATURE) size += sizeof(u_long);
		if (a & ATTR_VOL_SIZE) size += sizeof(off_t);
		if (a & ATTR_VOL_SPACEFREE) size += sizeof(off_t);
		if (a & ATTR_VOL_SPACEAVAIL) size += sizeof(off_t);
		if (a & ATTR_VOL_MINALLOCATION) size += sizeof(off_t);
		if (a & ATTR_VOL_ALLOCATIONCLUMP) size += sizeof(off_t);
		if (a & ATTR_VOL_IOBLOCKSIZE) size += sizeof(u_long);
		if (a & ATTR_VOL_OBJCOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_FILECOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_DIRCOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_MAXOBJCOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_MOUNTPOINT) size += sizeof(struct attrreference);
        if (a & ATTR_VOL_NAME) size += sizeof(struct attrreference);
        if (a & ATTR_VOL_MOUNTFLAGS) size += sizeof(u_long);
	};
	if ((a = attrlist->dirattr) != 0) {
		if (a & ATTR_DIR_LINKCOUNT) size += sizeof(u_long);
		if (a & ATTR_DIR_ENTRYCOUNT) size += sizeof(u_long);
	};
	if ((a = attrlist->fileattr) != 0) {
		if (a & ATTR_FILE_LINKCOUNT) size += sizeof(u_long);
		if (a & ATTR_FILE_TOTALSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_ALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_IOBLOCKSIZE) size += sizeof(size_t);
		if (a & ATTR_FILE_CLUMPSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_DEVTYPE) size += sizeof(u_long);
		if (a & ATTR_FILE_FILETYPE) size += sizeof(u_long);
		if (a & ATTR_FILE_FORKCOUNT) size += sizeof(u_long);
		if (a & ATTR_FILE_FORKLIST) size += sizeof(struct attrreference);
		if (a & ATTR_FILE_DATALENGTH) size += sizeof(off_t);
		if (a & ATTR_FILE_DATAALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_DATAEXTENTS) size += sizeof(extentrecord);
		if (a & ATTR_FILE_RSRCLENGTH) size += sizeof(off_t);
		if (a & ATTR_FILE_RSRCALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_RSRCEXTENTS) size += sizeof(extentrecord);
	};
	if ((a = attrlist->forkattr) != 0) {
		if (a & ATTR_FORK_TOTALSIZE) size += sizeof(off_t);
		if (a & ATTR_FORK_ALLOCSIZE) size += sizeof(off_t);
	};

    return size;
}



void PackVolCommonAttributes(struct attrlist *alist,
							 struct vnode *root_vp,
			   				 struct hfsCatalogInfo *root_catInfo,
							 void **attrbufptrptr,
							 void **varbufptrptr) {
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
    struct hfsnode *root_hp = VTOH(root_vp);
    struct mount *mp = VTOVFS(root_vp);
    struct hfsmount *hfsmp = VTOHFS(root_vp);
    struct ExtendedVCB *vcb = HFSTOVCB(hfsmp);
	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;

    if ((a = alist->commonattr) != 0) {
        if (a & ATTR_CMN_NAME) {
            attrlength = root_hp->h_meta->h_namelen+1;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = varbufptr - attrbufptr;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
            (void) strncpy((unsigned char *)varbufptr, H_NAME(root_hp), attrlength);

            /* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
        };
		if (a & ATTR_CMN_DEVID) *((dev_t *)attrbufptr)++ = hfsmp->hfs_raw_dev;
		if (a & ATTR_CMN_FSID) *((fsid_t *)attrbufptr)++ = mp->mnt_stat.f_fsid;
		if (a & ATTR_CMN_OBJTYPE) *((fsobj_type_t *)attrbufptr)++ = 0;
		if (a & ATTR_CMN_OBJTAG) *((fsobj_tag_t *)attrbufptr)++ = VT_HFS;
		if (a & ATTR_CMN_OBJID)	{
			((fsobj_id_t *)attrbufptr)->fid_objno = 0;
			((fsobj_id_t *)attrbufptr)->fid_generation = 0;
			++((fsobj_id_t *)attrbufptr);
		};
        if (a & ATTR_CMN_OBJPERMANENTID) {
            ((fsobj_id_t *)attrbufptr)->fid_objno = 0;
            ((fsobj_id_t *)attrbufptr)->fid_generation = 0;
            ++((fsobj_id_t *)attrbufptr);
        };
		if (a & ATTR_CMN_PAROBJID) {
            ((fsobj_id_t *)attrbufptr)->fid_objno = 0;
			((fsobj_id_t *)attrbufptr)->fid_generation = 0;
			++((fsobj_id_t *)attrbufptr);
		};
		VCB_LOCK(vcb);
        if (a & ATTR_CMN_SCRIPT) *((text_encoding_t *)attrbufptr)++ = vcb->volumeNameEncodingHint;
		/* NOTE: all VCB dates are in local Mac OS time */
		if (a & ATTR_CMN_CRTIME) {
			/*
			 * HFS Plus stores the volume create date in *local*
			 * time in the volume header. So don't use the create
			 * date from the vcb. Use the root's crtime instead.
			 */
			if (vcb->vcbSigWord == kHFSPlusSigWord) {
				((struct timespec *)attrbufptr)->tv_sec = root_hp->h_meta->h_crtime;
			} else {
				((struct timespec *)attrbufptr)->tv_sec = to_bsd_time(vcb->vcbCrDate);
			}
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_MODTIME) {
			((struct timespec *)attrbufptr)->tv_sec = to_bsd_time(vcb->vcbLsMod);
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_CHGTIME) {
			((struct timespec *)attrbufptr)->tv_sec = to_bsd_time(vcb->vcbLsMod);
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_ACCTIME) {
			((struct timespec *)attrbufptr)->tv_sec = to_bsd_time(vcb->vcbLsMod);
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_BKUPTIME) {
			((struct timespec *)attrbufptr)->tv_sec = to_bsd_time(vcb->vcbVolBkUp);
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_FNDRINFO) {
            bcopy (&vcb->vcbFndrInfo, attrbufptr, sizeof(vcb->vcbFndrInfo));
            attrbufptr += sizeof(vcb->vcbFndrInfo);
		};
		VCB_UNLOCK(vcb);
		if (a & ATTR_CMN_OWNERID) *((uid_t *)attrbufptr)++ = root_hp->h_meta->h_uid;
		if (a & ATTR_CMN_GRPID) *((gid_t *)attrbufptr)++ = root_hp->h_meta->h_gid;
		if (a & ATTR_CMN_ACCESSMASK) *((u_long *)attrbufptr)++ = (u_long)root_hp->h_meta->h_mode;
		if (a & ATTR_CMN_NAMEDATTRCOUNT) *((u_long *)attrbufptr)++ = 0;			/* XXX PPD TBC */
		if (a & ATTR_CMN_NAMEDATTRLIST) {
			attrlength = 0;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = 0;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
		};
		if (a & ATTR_CMN_FLAGS) *((u_long *)attrbufptr)++ = root_hp->h_meta->h_pflags;
	};
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}



void PackVolAttributeBlock(struct attrlist *alist,
						   struct vnode *root_vp,
			   			   struct hfsCatalogInfo *root_catInfo,
						   void **attrbufptrptr,
						   void **varbufptrptr) {
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
    struct mount *mp = VTOVFS(root_vp);
    struct hfsmount *hfsmp = VTOHFS(root_vp);
    struct ExtendedVCB *vcb = HFSTOVCB(hfsmp);
	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;
	
	if ((a = alist->volattr) != 0) {
		VCB_LOCK(vcb);
		if (a & ATTR_VOL_FSTYPE) *((u_long *)attrbufptr)++ = (u_long)mp->mnt_vfc->vfc_typenum;
		if (a & ATTR_VOL_SIGNATURE) *((u_long *)attrbufptr)++ = (u_long)vcb->vcbSigWord;
        if (a & ATTR_VOL_SIZE) *((off_t *)attrbufptr)++ = (off_t)vcb->totalBlocks * (off_t)vcb->blockSize;
        if (a & ATTR_VOL_SPACEFREE) *((off_t *)attrbufptr)++ = (off_t)vcb->freeBlocks * (off_t)vcb->blockSize;
        if (a & ATTR_VOL_SPACEAVAIL) *((off_t *)attrbufptr)++ = (off_t)vcb->freeBlocks * (off_t)vcb->blockSize;
        if (a & ATTR_VOL_MINALLOCATION) *((off_t *)attrbufptr)++ = (off_t)vcb->blockSize;
		if (a & ATTR_VOL_ALLOCATIONCLUMP) *((off_t *)attrbufptr)++ = (off_t)vcb->allocationsClumpSize;
        if (a & ATTR_VOL_IOBLOCKSIZE) *((u_long *)attrbufptr)++ = (u_long)hfsmp->hfs_log_block_size;
		if (a & ATTR_VOL_OBJCOUNT) *((u_long *)attrbufptr)++ = (u_long)vcb->vcbFilCnt + (u_long)vcb->vcbDirCnt;
		if (a & ATTR_VOL_FILECOUNT) *((u_long *)attrbufptr)++ = (u_long)vcb->vcbFilCnt;
		if (a & ATTR_VOL_DIRCOUNT) *((u_long *)attrbufptr)++ = (u_long)vcb->vcbDirCnt;
		if (a & ATTR_VOL_MAXOBJCOUNT) *((u_long *)attrbufptr)++ = 0xFFFFFFFF;
		if (a & ATTR_VOL_MOUNTPOINT) {
			attrlength = strlen(hfsmp->hfs_mountpoint) + 1;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = varbufptr - attrbufptr;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
			(void) strcpy((unsigned char *)varbufptr, hfsmp->hfs_mountpoint);
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
		};
        if (a & ATTR_VOL_NAME) {
            attrlength = VTOH(root_vp)->h_meta->h_namelen + 1;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = varbufptr - attrbufptr;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
            strncpy((char *)varbufptr, H_NAME(VTOH(root_vp)), attrlength);

            /* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
        };
        if (a & ATTR_VOL_MOUNTFLAGS) *((u_long *)attrbufptr)++ = (u_long)mp->mnt_flag;
		VCB_UNLOCK(vcb);
	};
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}




void PackVolumeInfo(struct attrlist *alist,
                    struct vnode *root_vp,
                    struct hfsCatalogInfo *root_catinfo,
                    void **attrbufptrptr,
                    void **varbufptrptr) {

    PackVolCommonAttributes(alist, root_vp, root_catinfo, attrbufptrptr, varbufptrptr);
    PackVolAttributeBlock(alist, root_vp, root_catinfo, attrbufptrptr, varbufptrptr);
};

// Pack the common attribute contents of an objects hfsCatalogInfo
void PackCommonCatalogInfoAttributeBlock(struct attrlist		*alist,
							 			struct vnode			*root_vp,
										struct hfsCatalogInfo	*catalogInfo,
										void					**attrbufptrptr,
										void					**varbufptrptr )
{
	struct hfsnode	*hp;
	void			*attrbufptr;
	void			*varbufptr;
	attrgroup_t		a;
	u_long			attrlength;
	
	hp			= VTOH(root_vp);
	attrbufptr	= *attrbufptrptr;
	varbufptr	= *varbufptrptr;
	
	if ((a = alist->commonattr) != 0)
	{
		if (a & ATTR_CMN_NAME)										
            {
            attrlength = strlen(catalogInfo->spec.name) + 1;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = varbufptr - attrbufptr;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
            (void) strncpy((unsigned char *)varbufptr, (unsigned char *) &(catalogInfo->spec.name), attrlength);
            //	((char *) varbufptr)[attrlength-1] = 0;									//	Now it's a C string

            /* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
            };
		if (a & ATTR_CMN_DEVID) *((dev_t *)attrbufptr)++			= hp->h_dev;
		if (a & ATTR_CMN_FSID) *((fsid_t *)attrbufptr)++			= VTOVFS(root_vp)->mnt_stat.f_fsid;
		if (a & ATTR_CMN_OBJTYPE)
		{
			if ( catalogInfo->nodeData.nodeType == kCatalogFolderNode )
				*((fsobj_type_t *)attrbufptr)++	= VDIR;
			else														//XXX could be more robust in object type handling
				*((fsobj_type_t *)attrbufptr)++	= VREG;
		}
		if (a & ATTR_CMN_OBJTAG) *((fsobj_tag_t *)attrbufptr)++		= root_vp->v_tag;
        if (a & ATTR_CMN_OBJID)
        {
            ((fsobj_id_t *)attrbufptr)->fid_objno					= catalogInfo->nodeData.nodeID;	//	H_FILEID(hp);
            ((fsobj_id_t *)attrbufptr)->fid_generation				= 0;
            ++((fsobj_id_t *)attrbufptr);
        };
        if (a & ATTR_CMN_OBJPERMANENTID)
        {
            ((fsobj_id_t *)attrbufptr)->fid_objno					= catalogInfo->nodeData.nodeID;	//	H_FILEID(hp);
            ((fsobj_id_t *)attrbufptr)->fid_generation				= 0;
            ++((fsobj_id_t *)attrbufptr);
        };
		if (a & ATTR_CMN_PAROBJID)
		{
            ((fsobj_id_t *)attrbufptr)->fid_objno					= catalogInfo->spec.parID;		//	H_DIRID(hp);
			((fsobj_id_t *)attrbufptr)->fid_generation				= 0;
			++((fsobj_id_t *)attrbufptr);
		};
        if (a & ATTR_CMN_SCRIPT)
	  {
	    if (HTOVCB(hp)->vcbSigWord == kHFSPlusSigWord) {
			*((text_encoding_t *)attrbufptr)++ = catalogInfo->nodeData.textEncoding;
	    } else {
			*((text_encoding_t *)attrbufptr)++ = 0;
	    }
	  };
		if (a & ATTR_CMN_CRTIME)
		{
			((struct timespec *)attrbufptr)->tv_sec					= to_bsd_time(catalogInfo->nodeData.createDate);
			((struct timespec *)attrbufptr)->tv_nsec				= 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_MODTIME)
		{
			((struct timespec *)attrbufptr)->tv_sec					= to_bsd_time(catalogInfo->nodeData.contentModDate);
			((struct timespec *)attrbufptr)->tv_nsec				= 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_CHGTIME)
		{
			((struct timespec *)attrbufptr)->tv_sec					= to_bsd_time(catalogInfo->nodeData.attributeModDate);
			((struct timespec *)attrbufptr)->tv_nsec				= 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_ACCTIME)
		{
			((struct timespec *)attrbufptr)->tv_sec					= to_bsd_time(catalogInfo->nodeData.accessDate);
			((struct timespec *)attrbufptr)->tv_nsec				= 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_BKUPTIME)
		{
			((struct timespec *)attrbufptr)->tv_sec					= to_bsd_time(catalogInfo->nodeData.backupDate);
			((struct timespec *)attrbufptr)->tv_nsec				= 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_FNDRINFO)
		{
			bcopy (&catalogInfo->nodeData.finderInfo, attrbufptr, sizeof(catalogInfo->nodeData.finderInfo));
			attrbufptr += sizeof(catalogInfo->nodeData.finderInfo);
			bcopy (&catalogInfo->nodeData.extFinderInfo, attrbufptr, sizeof(catalogInfo->nodeData.extFinderInfo));
			attrbufptr += sizeof(catalogInfo->nodeData.extFinderInfo);
		};
		if (a & ATTR_CMN_OWNERID) *((uid_t *)attrbufptr)++			= catalogInfo->nodeData.permissions.ownerID;
		if (a & ATTR_CMN_GRPID) *((gid_t *)attrbufptr)++			= catalogInfo->nodeData.permissions.groupID;
		if (a & ATTR_CMN_ACCESSMASK) *((u_long *)attrbufptr)++		= (u_long)(mode_t)(catalogInfo->nodeData.permissions.permissions & 0x0000FFFF);
		if (a & ATTR_CMN_NAMEDATTRCOUNT) *((u_long *)attrbufptr)++	= 0;			/* XXX PPD TBC */
		if (a & ATTR_CMN_NAMEDATTRLIST)
		{
			attrlength = 0;
			((struct attrreference *)attrbufptr)->attr_dataoffset	= 0;
			((struct attrreference *)attrbufptr)->attr_length		= attrlength;
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
			varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
			++((struct attrreference *)attrbufptr);
		};
        if (a & ATTR_CMN_FLAGS) *((u_long *)attrbufptr)++			= (u_long)((((catalogInfo->nodeData.permissions.permissions & 0xFFFF0000) >> 8)  & 0x00FF0000) |
                                                                               (((catalogInfo->nodeData.permissions.permissions & 0xFFFF0000) >> 24) & 0x000000FF));
	};
	
	*attrbufptrptr	= attrbufptr;
	*varbufptrptr	= varbufptr;
}


void PackCommonAttributeBlock(struct attrlist *alist,
							  struct vnode *vp,
							  struct hfsCatalogInfo *catInfo,
							  void **attrbufptrptr,
							  void **varbufptrptr) {
	struct hfsnode *hp;
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
	u_long attrlength;
	
	hp = VTOH(vp);
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;
	
    if ((a = alist->commonattr) != 0) {
        if (a & ATTR_CMN_NAME) {
            attrlength = hp->h_meta->h_namelen + 1;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = varbufptr - attrbufptr;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
            (void) strncpy((unsigned char *)varbufptr, H_NAME(hp), attrlength);

            /* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
        };
		if (a & ATTR_CMN_DEVID) *((dev_t *)attrbufptr)++ = hp->h_dev;
		if (a & ATTR_CMN_FSID) *((fsid_t *)attrbufptr)++ = VTOVFS(vp)->mnt_stat.f_fsid;
		if (a & ATTR_CMN_OBJTYPE) *((fsobj_type_t *)attrbufptr)++ = vp->v_type;
		if (a & ATTR_CMN_OBJTAG) *((fsobj_tag_t *)attrbufptr)++ = vp->v_tag;
        if (a & ATTR_CMN_OBJID)	{
            ((fsobj_id_t *)attrbufptr)->fid_objno = H_FILEID(hp);
			((fsobj_id_t *)attrbufptr)->fid_generation = 0;
			++((fsobj_id_t *)attrbufptr);
		};
        if (a & ATTR_CMN_OBJPERMANENTID)	{
            ((fsobj_id_t *)attrbufptr)->fid_objno = H_FILEID(hp);
            ((fsobj_id_t *)attrbufptr)->fid_generation = 0;
            ++((fsobj_id_t *)attrbufptr);
        };
		if (a & ATTR_CMN_PAROBJID) {
            ((fsobj_id_t *)attrbufptr)->fid_objno = H_DIRID(hp);
			((fsobj_id_t *)attrbufptr)->fid_generation = 0;
			++((fsobj_id_t *)attrbufptr);
		};
        if (a & ATTR_CMN_SCRIPT)
	  {
	    if (HTOVCB(hp)->vcbSigWord == kHFSPlusSigWord) {
			*((text_encoding_t *)attrbufptr)++ = catInfo->nodeData.textEncoding;
	    } else {
			*((text_encoding_t *)attrbufptr)++ = 0;
	    }
	  };
		if (a & ATTR_CMN_CRTIME) {
			((struct timespec *)attrbufptr)->tv_sec = hp->h_meta->h_crtime;
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_MODTIME) {
			((struct timespec *)attrbufptr)->tv_sec = hp->h_meta->h_mtime;
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_CHGTIME) {
			((struct timespec *)attrbufptr)->tv_sec = hp->h_meta->h_ctime;
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_ACCTIME) {
			((struct timespec *)attrbufptr)->tv_sec = hp->h_meta->h_atime;
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_BKUPTIME) {
			((struct timespec *)attrbufptr)->tv_sec = hp->h_meta->h_butime;
			((struct timespec *)attrbufptr)->tv_nsec = 0;
			++((struct timespec *)attrbufptr);
		};
		if (a & ATTR_CMN_FNDRINFO) {
            bcopy (&catInfo->nodeData.finderInfo, attrbufptr, sizeof(catInfo->nodeData.finderInfo));
            attrbufptr += sizeof(catInfo->nodeData.finderInfo);
            bcopy (&catInfo->nodeData.extFinderInfo, attrbufptr, sizeof(catInfo->nodeData.extFinderInfo));
            attrbufptr += sizeof(catInfo->nodeData.extFinderInfo);
		};
		if (a & ATTR_CMN_OWNERID) *((uid_t *)attrbufptr)++ = hp->h_meta->h_uid;
		if (a & ATTR_CMN_GRPID) *((gid_t *)attrbufptr)++ = hp->h_meta->h_gid;
		if (a & ATTR_CMN_ACCESSMASK) *((u_long *)attrbufptr)++ = (u_long)hp->h_meta->h_mode;
		if (a & ATTR_CMN_NAMEDATTRCOUNT) *((u_long *)attrbufptr)++ = 0;			/* XXX PPD TBC */
		if (a & ATTR_CMN_NAMEDATTRLIST) {
			attrlength = 0;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = 0;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
		};
		if (a & ATTR_CMN_FLAGS) *((u_long *)attrbufptr)++ = hp->h_meta->h_pflags;
	};
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}


//	Pack the directory attributes given hfsCatalogInfo
void PackCatalogInfoDirAttributeBlock( struct attrlist *alist, struct hfsCatalogInfo *catInfo, void **attrbufptrptr, void **varbufptrptr )
{
	void		*attrbufptr;
	attrgroup_t	a;
	
	attrbufptr	= *attrbufptrptr;
	a			= alist->dirattr;
	
	if ( (catInfo->nodeData.nodeType == kCatalogFolderNode) && (a != 0) )
	{
		/* The 'link count' is faked as 1 (the parent entry and ".").  In reality it would include at least  an additional link for the ".." in each subdirectory. */
		if (a & ATTR_DIR_LINKCOUNT) *((u_long *)attrbufptr)++	= 2;
		if (a & ATTR_DIR_ENTRYCOUNT) *((u_long *)attrbufptr)++	= catInfo->nodeData.valence;
	};
	
	*attrbufptrptr = attrbufptr;
}


void PackDirAttributeBlock(struct attrlist *alist,
						   struct vnode *vp,
						   struct hfsCatalogInfo *catInfo,
						   void **attrbufptrptr,
						   void **varbufptrptr) {
    void *attrbufptr;
//	void *varbufptr;
    attrgroup_t a;
//	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
//	varbufptr = *varbufptrptr;
	
	a = alist->dirattr;
	if ((vp->v_type == VDIR) && (a != 0)) {
		/* The 'link count' is faked as 1 (the parent entry and ".").  In reality it would include at least
		   an additional link for the ".." in each subdirectory. */
		if (a & ATTR_DIR_LINKCOUNT) *((u_long *)attrbufptr)++ = 2;
		if (a & ATTR_DIR_ENTRYCOUNT) *((u_long *)attrbufptr)++ = catInfo->nodeData.valence;
	};
	
	*attrbufptrptr = attrbufptr;
//	*varbufptrptr = varbufptr;
}



//	Pack the file attributes from the hfsCatalogInfo for the file.
void PackCatalogInfoFileAttributeBlock( struct attrlist *alist, struct hfsCatalogInfo *catInfo, struct vnode *root_vp, void **attrbufptrptr, void **varbufptrptr )
{
	void			*attrbufptr;
	void			*varbufptr;
	attrgroup_t		a;
	u_long			attrlength;
	ExtendedVCB		*vcb			= VTOVCB(root_vp);
	
	attrbufptr	= *attrbufptrptr;
	varbufptr	= *varbufptrptr;
	
	a = alist->fileattr;
	if ( (catInfo->nodeData.nodeType == kCatalogFileNode) && (a != 0) )
	{
		if (a & ATTR_FILE_LINKCOUNT) *((u_long *)attrbufptr)++		= 1;		/* There ARE no hard links... */
		if (a & ATTR_FILE_TOTALSIZE) *((off_t *)attrbufptr)++		= (off_t)catInfo->nodeData.dataLogicalSize + (off_t)catInfo->nodeData.rsrcLogicalSize;
		if (a & ATTR_FILE_ALLOCSIZE) *((off_t *)attrbufptr)++		= (off_t)catInfo->nodeData.dataPhysicalSize + (off_t)catInfo->nodeData.rsrcPhysicalSize;
        if (a & ATTR_FILE_IOBLOCKSIZE) *((u_long *)attrbufptr)++	= (u_long)(VTOHFS(root_vp)->hfs_log_block_size);
		if (a & ATTR_FILE_CLUMPSIZE) *((u_long *)attrbufptr)++		= vcb->vcbClpSiz;	//XXX Yikes, this is just the vcb clump size
		if (a & ATTR_FILE_DEVTYPE) *((u_long *)attrbufptr)++		= (u_long)catInfo->nodeData.permissions.specialDevice;
		if (a & ATTR_FILE_FILETYPE) *((u_long *)attrbufptr)++		= 0;			/* XXX PPD */
		if (a & ATTR_FILE_FORKCOUNT) *((u_long *)attrbufptr)++		= 2;			/* XXX PPD */
		if (a & ATTR_FILE_FORKLIST)
		{
			attrlength = 0;
			((struct attrreference *)attrbufptr)->attr_dataoffset	= 0;
			((struct attrreference *)attrbufptr)->attr_length		= attrlength;
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
			varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
			++((struct attrreference *)attrbufptr);
		};
		if (a & ATTR_FILE_DATALENGTH) *((off_t *)attrbufptr)++		= (off_t)catInfo->nodeData.dataLogicalSize;
		if (a & ATTR_FILE_DATAALLOCSIZE) *((off_t *)attrbufptr)++	= (off_t)catInfo->nodeData.dataPhysicalSize;
		if (a & ATTR_FILE_DATAEXTENTS)
		{
			bcopy(&catInfo->nodeData.dataExtents, attrbufptr, sizeof(extentrecord));
			attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
		};
		if (a & ATTR_FILE_RSRCLENGTH) *((off_t *)attrbufptr)++		= (off_t)catInfo->nodeData.rsrcLogicalSize;
		if (a & ATTR_FILE_RSRCALLOCSIZE) *((off_t *)attrbufptr)++	= (off_t)catInfo->nodeData.rsrcPhysicalSize;
		if (a & ATTR_FILE_RSRCEXTENTS)
		{
			bcopy(&catInfo->nodeData.rsrcExtents, attrbufptr, sizeof(extentrecord));
			attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
		};
	};
	
	*attrbufptrptr	= attrbufptr;
	*varbufptrptr	= varbufptr;
}


void PackFileAttributeBlock(struct attrlist *alist,
							struct vnode *vp,
							struct hfsCatalogInfo *catInfo,
							void **attrbufptrptr,
							void **varbufptrptr) {
    struct hfsnode *hp = VTOH(vp);
    FCB *fcb = HTOFCB(hp);
    ExtendedFCB *extendedFCB;
    HFSPlusExtentDescriptor *parellextents;
	ExtendedVCB *vcb = HTOVCB(hp);
    Boolean isHFSPlus = (vcb->vcbSigWord == kHFSPlusSigWord);
    void *attrbufptr = *attrbufptrptr;
    void *varbufptr = *varbufptrptr;
    attrgroup_t a = alist->fileattr;
	u_long attrlength;
	
	if ((vp->v_type == VREG) && (a != 0)) {
		if (a & ATTR_FILE_LINKCOUNT) *((u_long *)attrbufptr)++ = 1;		/* There ARE no hard links... */
		if (a & ATTR_FILE_TOTALSIZE) *((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.dataLogicalSize +
																(off_t)catInfo->nodeData.rsrcLogicalSize;
		if (a & ATTR_FILE_ALLOCSIZE) {
			switch (H_FORKTYPE(hp)) {
			  case kDataFork:
				*((off_t *)attrbufptr)++ = (off_t)fcb->fcbPLen + (off_t)catInfo->nodeData.rsrcPhysicalSize;
			  case kRsrcFork:
				*((off_t *)attrbufptr)++ = (off_t)fcb->fcbPLen + (off_t)catInfo->nodeData.dataPhysicalSize;
			  default:
				*((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.dataPhysicalSize +
										   (off_t)catInfo->nodeData.rsrcPhysicalSize;
		  };
		}; 
		if (a & ATTR_FILE_IOBLOCKSIZE) *((u_long *)attrbufptr)++ = hp->h_meta->h_logBlockSize;
		if (a & ATTR_FILE_CLUMPSIZE) *((u_long *)attrbufptr)++ = fcb->fcbClmpSize;
		if (a & ATTR_FILE_DEVTYPE) *((u_long *)attrbufptr)++ = (u_long)catInfo->nodeData.permissions.specialDevice;
		if (a & ATTR_FILE_FILETYPE) *((u_long *)attrbufptr)++ = 0;			/* XXX PPD */
		if (a & ATTR_FILE_FORKCOUNT) *((u_long *)attrbufptr)++ = 2;			/* XXX PPD */
		if (a & ATTR_FILE_FORKLIST) {
			attrlength = 0;
            ((struct attrreference *)attrbufptr)->attr_dataoffset = 0;
            ((struct attrreference *)attrbufptr)->attr_length = attrlength;
			
			/* Advance beyond the space just allocated and round up to the next 4-byte boundary: */
            varbufptr += attrlength + ((4 - (attrlength & 3)) & 3);
            ++((struct attrreference *)attrbufptr);
		};
		if (H_FORKTYPE(hp) == kDataFork) {
			if (a & ATTR_FILE_DATALENGTH)
#if	MACH_NBC
                if ((vp->v_type == VREG) && vp->v_vm_info && vp->v_vm_info->mapped &&
                    (!vp->v_vm_info->filesize)) {
                    *((off_t *)attrbufptr)++ = vp->v_vm_info->vnode_size;
                }
                else
#endif /* MACH_NBC */
                   *((off_t *)attrbufptr)++ = fcb->fcbEOF;
			if (a & ATTR_FILE_DATAALLOCSIZE) *((off_t *)attrbufptr)++ = fcb->fcbPLen;
			if (a & ATTR_FILE_DATAEXTENTS) {
			    if ( ! isHFSPlus) {
			        /* HFS, Copy the extents from the FCB part */
			        int i;
			        HFSPlusExtentRecord extents;
			        for (i = 0; i < kHFSExtentDensity; ++i) {
			            extents[i].startBlock = (UInt16) (fcb->fcbExtRec[i].startBlock);
			            extents[i].blockCount = (UInt16) (fcb->fcbExtRec[i].blockCount);
			        };
			        bcopy ( extents, attrbufptr, sizeof(extentrecord));
			    } else {
			        /* HFSPlus, Copy the extents from the parallel extents */
			        extendedFCB = GetParallelFCB (HTOV(hp));
			        parellextents = extendedFCB->extents;
			        bcopy ( parellextents, attrbufptr, sizeof(extentrecord));
			    };
				attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
			};
		} else {
			if (a & ATTR_FILE_DATALENGTH) *((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.dataLogicalSize;
			if (a & ATTR_FILE_DATAALLOCSIZE) *((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.dataPhysicalSize;
			if (a & ATTR_FILE_DATAEXTENTS) {
				bcopy(&catInfo->nodeData.dataExtents, attrbufptr, sizeof(extentrecord));
				attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
			};
		};
		if (H_FORKTYPE(hp) == kRsrcFork) {
			if (a & ATTR_FILE_RSRCLENGTH) 
#if	MACH_NBC
                if ((vp->v_type == VREG) && vp->v_vm_info && vp->v_vm_info->mapped &&
                    (!vp->v_vm_info->filesize)) {
                    *((off_t *)attrbufptr)++ = vp->v_vm_info->vnode_size;
                }
                else
#endif /* MACH_NBC */
                   *((off_t *)attrbufptr)++ = fcb->fcbEOF;
			if (a & ATTR_FILE_RSRCALLOCSIZE) *((off_t *)attrbufptr)++ = fcb->fcbPLen;
			if (a & ATTR_FILE_RSRCEXTENTS) {
			    if ( ! isHFSPlus) {
			        /* HFS, Copy the extents from the FCB part */
			        int i;
			        HFSPlusExtentRecord extents;
			        for (i = 0; i < kHFSExtentDensity; ++i) {
			            extents[i].startBlock = (UInt16) (fcb->fcbExtRec[i].startBlock);
			            extents[i].blockCount = (UInt16) (fcb->fcbExtRec[i].blockCount);
			        };
			        bcopy ( extents, attrbufptr, sizeof(extentrecord));
			    } else {
			        /* HFSPlus, Copy the extents from the parallel extents */
			        extendedFCB = GetParallelFCB (HTOV(hp));
			        parellextents = extendedFCB->extents;
			        bcopy ( parellextents, attrbufptr, sizeof(extentrecord));
			    };
				attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
			};
		} else {
			if (a & ATTR_FILE_RSRCLENGTH) *((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.rsrcLogicalSize;
			if (a & ATTR_FILE_RSRCALLOCSIZE) *((off_t *)attrbufptr)++ = (off_t)catInfo->nodeData.rsrcPhysicalSize;
			if (a & ATTR_FILE_RSRCEXTENTS) {
				bcopy(&catInfo->nodeData.rsrcExtents, attrbufptr, sizeof(extentrecord));
				attrbufptr += sizeof(extentrecord) + ((4 - (sizeof(extentrecord) & 3)) & 3);
			};
		};
	};
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}

#if 0
void PackForkAttributeBlock(struct attrlist *alist,
							struct vnode *vp,
							struct hfsCatalogInfo *catInfo,
							void **attrbufptrptr,
							void **varbufptrptr) {
	/* XXX PPD TBC */
}
#endif


//	This routine takes catInfo, and alist, as inputs and packs it into an attribute block.
void PackCatalogInfoAttributeBlock ( struct attrlist *alist, struct vnode *root_vp, struct hfsCatalogInfo *catInfo, void **attrbufptrptr, void **varbufptrptr)
{
	//XXX	Preflight that alist only contains bits with fields in catInfo

	PackCommonCatalogInfoAttributeBlock( alist, root_vp, catInfo, attrbufptrptr, varbufptrptr );
	
	switch ( catInfo->nodeData.nodeType )
	{
		case kCatalogFolderNode:
			PackCatalogInfoDirAttributeBlock( alist, catInfo, attrbufptrptr, varbufptrptr );
			break;
		
	  	case kCatalogFileNode:
			PackCatalogInfoFileAttributeBlock( alist, catInfo, root_vp, attrbufptrptr, varbufptrptr );
			break;
	  
	 	default:	/* Without this the compiler complains about VNON,VBLK,VCHR,VLNK,VSOCK,VFIFO,VBAD and VSTR not being handled... */
			/* XXX PPD - Panic? */
			break;
	}
}



void PackAttributeBlock(struct attrlist *alist,
						struct vnode *vp,
						struct hfsCatalogInfo *catInfo,
						void **attrbufptrptr,
						void **varbufptrptr) {

//  DBG_FUNC_NAME("PackAttributeBlock");

	if (alist->volattr != 0) {
		DBG_ASSERT((vp->v_flag & VROOT) != 0);
		PackVolumeInfo(alist,vp, catInfo, attrbufptrptr, varbufptrptr);
	} else {
		PackCommonAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
		
		switch (vp->v_type) {
		  case VDIR:
			PackDirAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
			break;
			
		  case VREG:
	   /* case VCPLX: */			/* XXX PPD TBC */
			PackFileAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
			break;
		  
	#if 0							/* XXX PPD TBC */
		  case VFORK:
			PackForkAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
			break;
	#endif
		  
		  /* Without this the compiler complains about VNON,VBLK,VCHR,VLNK,VSOCK,VFIFO,VBAD and VSTR
		     not being handled...
		   */
		  default:
			/* XXX PPD - Panic? */
			break;
		};
	};
};



void UnpackVolumeAttributeBlock(struct attrlist *alist,
								struct vnode *root_vp,
								ExtendedVCB *vcb,
								void **attrbufptrptr,
								void **varbufptrptr) {
	void *attrbufptr = *attrbufptrptr;
	attrgroup_t a;
	text_encoding_t nameEncoding;
	
    if ((alist->commonattr == 0) && (alist->volattr == 0)) {
        return;		/* Get out without dirtying the VCB */
    };

    VCB_LOCK(vcb);

	a = alist->commonattr;
	
	if (a & ATTR_CMN_SCRIPT) {
		/* XXX PPD No use for this info right now... */
		nameEncoding = *(((text_encoding_t *)attrbufptr)++);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_SCRIPT;
#endif
	};
	if (a & ATTR_CMN_CRTIME) {
		/*
		 * HFS Plus stores the volume create date in *local*
		 * time in the volume header. So don't set the create
		 * date in the vcb. Set the root's crtime instead.
		 */
		if (vcb->vcbSigWord == kHFSPlusSigWord) {
			VTOH(root_vp)->h_meta->h_crtime =
					(UInt32)((struct timespec *)attrbufptr)->tv_sec;
		} else {
			vcb->vcbCrDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		}
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_CRTIME;
#endif
	};
	if (a & ATTR_CMN_MODTIME) {
		vcb->vcbLsMod = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_MODTIME;
#endif
	};
	if (a & ATTR_CMN_BKUPTIME) {
		vcb->vcbVolBkUp = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_BKUPTIME;
#endif
	};
	if (a & ATTR_CMN_FNDRINFO) {
		bcopy (attrbufptr, &vcb->vcbFndrInfo, sizeof(vcb->vcbFndrInfo));
		attrbufptr += sizeof(vcb->vcbFndrInfo);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_FNDRINFO;
#endif
	};
	
	DBG_ASSERT(a == 0);				/* All common attributes for volumes must've been handled by now... */

	a = alist->volattr & ~ATTR_VOL_INFO;
	if (a & ATTR_VOL_NAME) {
        copystr(((char *)attrbufptr) + *((u_long *)attrbufptr), vcb->vcbVN, sizeof(vcb->vcbVN), NULL);
        attrbufptr += sizeof(struct attrreference);
#if DIAGNOSTIC
		a &= ~ATTR_VOL_NAME;
#endif
	};
	
	DBG_ASSERT(a == 0);				/* All common attributes for volumes must've been handled by now... */

    vcb->vcbFlags |= 0xFF00;		// Mark the VCB dirty

    VCB_UNLOCK(vcb);
}


void UnpackCommonAttributeBlock(struct attrlist *alist,
								struct vnode *vp,
								struct hfsCatalogInfo *catInfo,
								void **attrbufptrptr,
								void **varbufptrptr) {
	struct hfsnode *hp = VTOH(vp);
    void *attrbufptr;
    attrgroup_t a;
	
	attrbufptr = *attrbufptrptr;

    DBG_ASSERT(catInfo != NULL);
	
	a = alist->commonattr;
	if (a & ATTR_CMN_SCRIPT) {
		/* XXX PPD No use for this info right now... */
		++((text_encoding_t *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_SCRIPT;
#endif
	};
	if (a & ATTR_CMN_CRTIME) {
		catInfo->nodeData.createDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		VTOH(vp)->h_meta->h_crtime = (UInt32)((struct timespec *)attrbufptr)->tv_sec;
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_CRTIME;
#endif
	};
	if (a & ATTR_CMN_MODTIME) {
		catInfo->nodeData.contentModDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		VTOH(vp)->h_meta->h_mtime = (UInt32)((struct timespec *)attrbufptr)->tv_sec;
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_MODTIME;
#endif
	};
	if (a & ATTR_CMN_CHGTIME) {
		catInfo->nodeData.attributeModDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		VTOH(vp)->h_meta->h_ctime = (UInt32)((struct timespec *)attrbufptr)->tv_sec;
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_CHGTIME;
#endif
	};
	if (a & ATTR_CMN_ACCTIME) {
		catInfo->nodeData.accessDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		VTOH(vp)->h_meta->h_atime = (UInt32)((struct timespec *)attrbufptr)->tv_sec;
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_ACCTIME;
#endif
	};
	if (a & ATTR_CMN_BKUPTIME) {
		catInfo->nodeData.backupDate = to_hfs_time((UInt32)((struct timespec *)attrbufptr)->tv_sec);
		VTOH(vp)->h_meta->h_butime = (UInt32)((struct timespec *)attrbufptr)->tv_sec;
		++((struct timespec *)attrbufptr);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_BKUPTIME;
#endif
	};
	if (a & ATTR_CMN_FNDRINFO) {
		bcopy (attrbufptr, &catInfo->nodeData.finderInfo, sizeof(catInfo->nodeData.finderInfo));
		attrbufptr += sizeof(catInfo->nodeData.finderInfo);
		bcopy (attrbufptr, &catInfo->nodeData.extFinderInfo, sizeof(catInfo->nodeData.extFinderInfo));
		attrbufptr += sizeof(catInfo->nodeData.extFinderInfo);
#if DIAGNOSTIC
		a &= ~ATTR_CMN_FNDRINFO;
#endif
	};
	if (a & ATTR_CMN_OWNERID) {
        if (VTOVCB(vp)->vcbSigWord == kHFSPlusSigWord) {
			u_int32_t uid = (u_int32_t)*((uid_t *)attrbufptr)++;
			if (uid != (uid_t)VNOVAL)
				hp->h_meta->h_uid = uid;	/* catalog will get updated by hfs_chown() */
        }
		else {
            ((uid_t *)attrbufptr)++;
		}
#if DIAGNOSTIC
		a &= ~ATTR_CMN_OWNERID;
#endif
	};
	if (a & ATTR_CMN_GRPID) {
        u_int32_t gid = (u_int32_t)*((gid_t *)attrbufptr)++;
        if (VTOVCB(vp)->vcbSigWord == kHFSPlusSigWord) {
            if (gid != (gid_t)VNOVAL)
                hp->h_meta->h_gid = gid;					/* catalog will get updated by hfs_chown() */
        };
#if DIAGNOSTIC
		a &= ~ATTR_CMN_GRPID;
#endif
	};
	if (a & ATTR_CMN_ACCESSMASK) {
        u_int16_t mode = (u_int16_t)*((u_long *)attrbufptr)++;
        if (VTOVCB(vp)->vcbSigWord == kHFSPlusSigWord) {
            if (mode != (mode_t)VNOVAL) {
                hp->h_meta->h_mode &= ~ALLPERMS;
                hp->h_meta->h_mode |= (mode & ALLPERMS);	/* catalog will get updated by hfs_chmod() */
            }
        };
#if DIAGNOSTIC
		a &= ~ATTR_CMN_ACCESSMASK;
#endif
	};
	if (a & ATTR_CMN_FLAGS) {
		u_long flags = *((u_long *)attrbufptr)++;
        /* Flags are settable only on HFS+ volumes.  A special exception is made for the IMMUTABLE
           flags (SF_IMMUTABLE and UF_IMMUTABLE), which can be set on HFS volumes as well: */
        if ((VTOVCB(vp)->vcbSigWord == kHFSPlusSigWord) ||
            ((VTOVCB(vp)->vcbSigWord == kHFSSigWord) && ((flags & ~IMMUTABLE) == 0))) {
            if (flags != (u_long)VNOVAL) {
                hp->h_meta->h_pflags = flags;				/* catalog will get updated by hfs_chflags */
            };
        };
#if DIAGNOSTIC
		a &= ~ATTR_CMN_FLAGS;
#endif
	};

#if DIAGNOSTIC
	if (a != 0) {
		DEBUG_BREAK_MSG(("UnpackCommonAttributes: unhandled bit: 0x%08X\n", a));
	};
#endif

	*attrbufptrptr = attrbufptr;
//	*varbufptrptr = varbufptr;
}



#if 0
void UnpackDirAttributeBlock(struct attrlist *alist,
							 struct vnode *vp,
							 struct hfsCatalogInfo *catInfo,
							 void **attrbufptrptr,
							 void **varbufptrptr) {
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;
	
	/* XXX PPD TBC */
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}
#endif



#if 0
void UnpackFileAttributeBlock(struct attrlist *alist,
							  struct vnode *vp,
							  struct hfsCatalogInfo *catInfo,
							  void **attrbufptrptr,
							  void **varbufptrptr) {
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;
	
	/* XXX PPD TBC */
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}
#endif



#if 0
void UnpackForkAttributeBlock(struct attrlist *alist,
							struct vnode *vp,
							struct hfsCatalogInfo *catInfo,
							void **attrbufptrptr,
							void **varbufptrptr) {
    void *attrbufptr;
    void *varbufptr;
    attrgroup_t a;
	u_long attrlength;
	
	attrbufptr = *attrbufptrptr;
	varbufptr = *varbufptrptr;
	
	/* XXX PPD TBC */
	
	*attrbufptrptr = attrbufptr;
	*varbufptrptr = varbufptr;
}
#endif



void UnpackAttributeBlock(struct attrlist *alist,
						  struct vnode *vp,
						  struct hfsCatalogInfo *catInfo,
						  void **attrbufptrptr,
						  void **varbufptrptr) {


	if (alist->volattr != 0) {
		UnpackVolumeAttributeBlock(alist, vp, VTOVCB(vp), attrbufptrptr, varbufptrptr);
		return;
	};
	
	/* We're dealing with a vnode object here: */
	UnpackCommonAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
	
#if 0
	switch (vp->v_type) {
	  case VDIR:
		UnpackDirAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
		break;

	  case VREG:
   /* case VCPLX: */			/* XXX PPD TBC */
		UnpackFileAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
		break;

	  case VFORK:
		UnpackForkAttributeBlock(alist, vp, catInfo, attrbufptrptr, varbufptrptr);
		break;

	  /* Without this the compiler complains about VNON,VBLK,VCHR,VLNK,VSOCK,VFIFO,VBAD and VSTR
	     not being handled...
	   */
	  default:
		/* XXX PPD - Panic? */
		break;
	};
#endif

};


unsigned long BestBlockSizeFit(unsigned long allocationBlockSize,
                               unsigned long blockSizeLimit,
                               unsigned long baseMultiple) {
    /*
       Compute the optimal (largest) block size (no larger than allocationBlockSize) that is less than the
       specified limit but still an even multiple of the baseMultiple.
     */
    int baseBlockCount, blockCount;
    unsigned long trialBlockSize;

    if (allocationBlockSize % baseMultiple != 0) {
        /*
           Whoops: the allocation blocks aren't even multiples of the specified base:
           no amount of dividing them into even parts will be a multiple, either then!
        */
        return 512;		/* Hope for the best */
    };

    /* Try the obvious winner first, to prevent 12K allocation blocks, for instance,
       from being handled as two 6K logical blocks instead of 3 4K logical blocks.
       Even though the former (the result of the loop below) is the larger allocation
       block size, the latter is more efficient: */
    if (allocationBlockSize % PAGE_SIZE == 0) return PAGE_SIZE;

    /* No clear winner exists: pick the largest even fraction <= MAXBSIZE: */
    baseBlockCount = allocationBlockSize / baseMultiple;				/* Now guaranteed to be an even multiple */

    for (blockCount = baseBlockCount; blockCount > 0; --blockCount) {
        trialBlockSize = blockCount * baseMultiple;
        if (allocationBlockSize % trialBlockSize == 0) {				/* An even multiple? */
            if ((trialBlockSize <= blockSizeLimit) &&
                (trialBlockSize % baseMultiple == 0)) {
                return trialBlockSize;
            };
        };
    };

    /* Note: we should never get here, since blockCount = 1 should always work,
       but this is nice and safe and makes the compiler happy, too ... */
    return 512;
}


/*
 * Map HFS Common errors (negative) to BSD error codes (positive).
 * Positive errors (ie BSD errors) are passed through unchanged.
 */
short MacToVFSError(OSErr err)
{
    if (err >= 0) {
        if (err > 0) {
            DBG_ERR(("MacToVFSError: passing error #%d unchanged...\n", err));
        };
        return err;
    };

    if (err != 0) {
        DBG_ERR(("MacToVFSError: mapping error code %d...\n", err));
    };
    
	switch (err) {
	  case dirFulErr:							/*    -33 */
	  case dskFulErr:							/*    -34 */
	  case btNoSpaceAvail:						/* -32733 */
	  case fxOvFlErr:							/* -32750 */
		return ENOSPC;							/*    +28 */

	  case btBadNode:							/* -32731 */
	  case ioErr:								/*   -36 */
		return EIO;								/*    +5 */

	  case badMDBErr:							/*   -60 */
	  case fnOpnErr:							/*   -38 */
		return EBADF;							/*    +9 */

	  case mFulErr:								/*   -41 */
	  case memFullErr:							/*  -108 */
		return ENOMEM;							/*   +12 */

	  case tmfoErr:								/*   -42 */
		/* Consider EMFILE (Too many open files, 24)? */	
		return ENFILE;							/*   +23 */

	  case nsvErr:								/*   -35 */
	  case fnfErr:								/*   -43 */
	  case dirNFErr:							/*  -120 */
	  case fidNotFound:							/* -1300 */
		return ENOENT;							/*    +2 */

	  case wPrErr:								/*   -44 */
	  case vLckdErr:							/*   -46 */
	  case fsDSIntErr:							/*  -127 */
		return EROFS;							/*   +30 */

	  case opWrErr:								/*   -49 */
	  case fLckdErr:							/*   -45 */
		return EACCES;							/*   +13 */

	  case permErr:								/*   -54 */
	  case wrPermErr:							/*   -61 */
		return EPERM;							/*    +1 */

	  case fBsyErr:								/*   -47 */
		return EBUSY;							/*   +16 */

	  case dupFNErr:							/*    -48 */
	  case fidExists:							/*  -1301 */
	  case cmExists:							/* -32718 */
	  case btExists:							/* -32734 */
		return EEXIST;							/*    +17 */

	  case rfNumErr:							/*   -51 */
		return EBADF;							/*    +9 */

	  case notAFileErr:							/* -1302 */
		return EISDIR;							/*   +21 */

	  case cmNotFound:							/* -32719 */
	  case btNotFound:							/* -32735 */	
		return ENOENT;							/*     28 */

	  case cmNotEmpty:							/* -32717 */
		return ENOTEMPTY;						/*     66 */

	  case cmFThdDirErr:						/* -32714 */
		return EISDIR;							/*     21 */

	  case fxRangeErr:							/* -32751 */
		return EIO;								/*      5 */

	  case bdNamErr:							/*   -37 */
		return ENAMETOOLONG;					/*    63 */

	  case eofErr:								/*   -39 */
	  case posErr:								/*   -40 */
	  case paramErr:							/*   -50 */
	  case badMovErr:							/*  -122 */
	  case sameFileErr:							/* -1306 */
	  case badFidErr:							/* -1307 */
	  case fileBoundsErr:						/* -1309 */
		return EINVAL;							/*   +22 */

	  default:
		DBG_UTILS(("Unmapped MacOS error: %d\n", err));
		return EIO;								/*   +5 */
	}
}


/*
 * All of our debugging functions
 */

#if DIAGNOSTIC

void debug_vn_status (char* introStr, struct vnode *vn)
{
    DBG_VOP(("%s:\t",introStr));
    if (vn != NULL)
      {
        if (vn->v_tag != VT_HFS)
          {
            DBG_VOP(("NON-HFS VNODE Ox%08lX\n", (unsigned long)vn));
          }
        else if(vn->v_tag==VT_HFS && (vn->v_data==NULL || VTOH((vn))->h_valid != HFS_VNODE_MAGIC))
          {
            DBG_VOP(("BAD VNODE PRIVATE DATA!!!!\n"));
          }
        else
          {
            DBG_VOP(("r: %d & ", vn->v_usecount));
            if (lockstatus(&VTOH(vn)->h_lock))
              {
                DBG_VOP_CONT(("is L\n"));
              }
            else
              {
                DBG_VOP_CONT(("is U\n"));
              }
          }
      }
    else
      {
        DBG_VOP(("vnode is NULL\n"));
      };
}

void debug_vn_print (char* introStr, struct vnode *vn)
{
//  DBG_FUNC_NAME("DBG_VN_PRINT");
    DBG_ASSERT (vn != NULL);
    DBG_VFS(("%s: ",introStr));
    DBG_VFS_CONT(("vnode: 0x%x is a ", (uint)vn));
    switch (vn->v_tag)
      {
        case VT_UFS:
            DBG_VFS_CONT(("%s","UFS"));
            break;
        case VT_HFS:
            DBG_VFS_CONT(("%s","HFS"));
            break;
        default:
            DBG_VFS_CONT(("%s","UNKNOWN"));
            break;
      }

    DBG_VFS_CONT((" vnode\n"));
    if (vn->v_tag==VT_HFS)
      {
        if (vn->v_data==NULL)
          {
            DBG_VFS(("BAD VNODE PRIVATE DATA!!!!\n"));
          }
        else
          {
            DBG_VFS(("     Name: %s Id: %ld ",H_NAME(VTOH(vn)), H_FILEID(VTOH(vn))));
          }
      }
    else
        DBG_VFS(("     "));

    DBG_VFS_CONT(("Refcount: %d\n", vn->v_usecount));
    if (VOP_ISLOCKED(vn))
      {
        DBG_VFS(("     The vnode is locked\n"));
      }
    else
      {
        DBG_VFS(("     The vnode is not locked\n"));
      }
}

void debug_rename_test_locks (char* 			introStr,
                            struct vnode 	*fvp,
                            struct vnode 	*fdvp,
                            struct vnode 	*tvp,
                            struct vnode 	*tdvp,
                            int				fstatus,
                            int				fdstatus,
                            int				tstatus,
                            int				tdstatus
)
{
    DBG_VOP(("\t%s: ", introStr));
    if (fvp) {if(lockstatus(&VTOH(fvp)->h_lock)){DBG_VFS_CONT(("L"));} else {DBG_VFS_CONT(("U"));}} else { DBG_VFS_CONT(("X"));};
    if (fdvp) {if(lockstatus(&VTOH(fdvp)->h_lock)){DBG_VFS_CONT(("L"));} else {DBG_VFS_CONT(("U"));}} else { DBG_VFS_CONT(("X"));};
    if (tvp) {if(lockstatus(&VTOH(tvp)->h_lock)){DBG_VFS_CONT(("L"));} else {DBG_VFS_CONT(("U"));}} else { DBG_VFS_CONT(("X"));};
    if (tdvp) {if(lockstatus(&VTOH(tdvp)->h_lock)){DBG_VFS_CONT(("L"));} else {DBG_VFS_CONT(("U"));}} else { DBG_VFS_CONT(("X"));};
    DBG_VFS_CONT(("\n"));

    if (fvp) {
        if (lockstatus(&VTOH(fvp)->h_lock)) {
            if (fstatus==VOPDBG_UNLOCKED) {
                DBG_VOP(("\tfvp should be NOT LOCKED and it is\n"));
            }
        } else if (fstatus == VOPDBG_LOCKED) {
            DBG_VOP(("\tfvp should be LOCKED and it isnt\n"));
        }
    }

    if (fdvp) {
        if (lockstatus(&VTOH(fdvp)->h_lock)) {
            if (fdstatus==VOPDBG_UNLOCKED) {
                DBG_VOP(("\tfdvp should be NOT LOCKED and it is\n"));
            }
        } else if (fdstatus == VOPDBG_LOCKED) {
            DBG_VOP(("\tfdvp should be LOCKED and it isnt\n"));
        }
    }

    if (tvp) {
        if (lockstatus(&VTOH(tvp)->h_lock)) {
            if (tstatus==VOPDBG_UNLOCKED) {
                DBG_VOP(("\ttvp should be NOT LOCKED and it is\n"));
            }
        } else if (tstatus == VOPDBG_LOCKED) {
            DBG_VOP(("\ttvp should be LOCKED and it isnt\n"));
        }
    }

    if (tdvp) {
        if (lockstatus(&VTOH(tdvp)->h_lock)) {
            if (tdstatus==VOPDBG_UNLOCKED) {
                DBG_VOP(("\ttdvp should be NOT LOCKED and it is\n"));
            }
        } else if (tdstatus == VOPDBG_LOCKED) {
            DBG_VOP(("\ttdvp should be LOCKED and it isnt\n"));

        }
    }

}
#endif /* DIAGNOSTIC */


#if DIAGNOSTIC
void debug_check_buffersizes(struct vnode *vp, struct hfsnode *hp, struct buf *bp) {
    DBG_ASSERT(bp->b_validoff == 0);
    DBG_ASSERT(bp->b_dirtyoff == 0);
    if (bp->b_lblkno < hp->h_uniformblocksizestart) {
        DBG_ASSERT((bp->b_bcount == MAXLOGBLOCKSIZE) ||
                   (bp->b_bcount == LogicalBlockSize(hp, bp->b_lblkno)) ||
                   ((bp->b_bcount % 512 == 0) &&
                    (bp->b_validend > 0) &&
                    (bp->b_dirtyend >= 0) &&							/* Could be partial block due to file growth */
                    (bp->b_bcount < LogicalBlockSize(hp, bp->b_lblkno))));
    } else {
        DBG_ASSERT((bp->b_bcount == hp->h_meta->h_logBlockSize) ||
                   ((bp->b_bcount % 512 == 0) &&
                    (bp->b_validend > 0) &&
                    (bp->b_dirtyend > 0) &&
                    (bp->b_bcount < hp->h_meta->h_logBlockSize)));
    };

    if (bp->b_validend == 0) {
        DBG_ASSERT(bp->b_dirtyend == 0);
    } else {
        DBG_ASSERT(bp->b_validend == bp->b_bcount);
        DBG_ASSERT(bp->b_dirtyend <= bp->b_bcount);
    };

    if ((bp->b_lblkno == 0x21) || (bp->b_lblkno == 0x22)) DBG_ASSERT((hp->h_uniformblocksizestart > 0x21) || (bp->b_bcount != MAXLOGBLOCKSIZE));
}


void debug_check_blocksizes(struct vnode *vp) {
    struct hfsnode *hp = VTOH(vp);
    struct buf *bp;

    if (vp->v_flag & VSYSTEM) return;

    for (bp = vp->v_cleanblkhd.lh_first; bp != NULL; bp = bp->b_vnbufs.le_next) {
        debug_check_buffersizes(vp, hp, bp);
    };

    for (bp = vp->v_dirtyblkhd.lh_first; bp != NULL; bp = bp->b_vnbufs.le_next) {
        debug_check_buffersizes(vp, hp, bp);
    };
}

#endif /* DIAGNOSTIC */
