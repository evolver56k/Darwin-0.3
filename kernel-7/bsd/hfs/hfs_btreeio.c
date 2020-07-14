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

/*	@(#)hfs_btreeio.c
*
*	(c) 1998 Apple Computer, Inc.  All Rights Reserved
*
*	hfs_btreeio.c -- I/O Routines for the HFS B-tree files.
*
*	HISTORY
*	16-Jul-1998	Don Brady		In ExtendBtreeFile force all b-tree nodes to be contiguous on disk.
*	 4-Jun-1998	Pat Dirks		Changed to do all B*-Tree writes synchronously (FORCESYNCBTREEWRITES = 1)
*	18-apr-1998	Don Brady		Call brelse on bread failure.
*	17-Apr-1998	Pat Dirks		Fixed ReleaseBTreeBlock to not call brelse when bwrite or bdwrite is called.
*	13-apr-1998	Don Brady		Add ExtendBTreeFile routine (from BTreeWrapper.c).
*	26-mar-1998	Don Brady		SetBTreeBlockSize was incorrectly excluding 512 byte blockSize.
*	18-feb-1998	Don Brady		Initially created file.
*
*/

#include <sys/types.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <machine/spl.h>

#include "hfs.h"
#include "hfs_dbg.h"
#include "hfscommon/headers/system/MacOSTypes.h"
#include "hfscommon/headers/FileMgrInternal.h"
#include "hfscommon/headers/BTreesInternal.h"

#define FORCESYNCBTREEWRITES 0

OSStatus SetBTreeBlockSize(FileReference vp, ByteCount blockSize, ItemCount minBlockCount)
{
    if (blockSize > MAXBSIZE )
        return (fsBTBadNodeSize);

    DBG_TREE(("SetBlockSizeProc: blockSize=%ld for file %ld\n", blockSize, VTOFCB(vp)->fcbFlNm));

    VTOH(vp)->h_meta->h_logBlockSize = blockSize;
    VTOH(vp)->h_meta->h_physBlkPerLogBlk = blockSize / kHFSBlockSize;

    return (E_NONE);
}


OSStatus GetBTreeBlock(FileReference vp, UInt32 blockNum, GetBlockOptions options, BlockDescriptor *block)
{
    OSStatus	 retval = E_NONE;
    struct buf   *bp = NULL;

    //XXX DJB - what about kForceReadBlock option?

//	DBG_TREE(("GetBlockProc: block=%ld, blockSize=%ld\n", blockNum, block->blockSize));

    if (options & kGetEmptyBlock)
        bp = getblk (vp,
                    IOBLKNOFORBLK(blockNum, VTOHFS(vp)->hfs_phys_block_size),
                    IOBYTECCNTFORBLK(blockNum, block->blockSize, VTOHFS(vp)->hfs_phys_block_size),
                    0,
                    0);
    else
        retval = bread (vp,
                        IOBLKNOFORBLK(blockNum, VTOHFS(vp)->hfs_phys_block_size),
                        IOBYTECCNTFORBLK(blockNum, block->blockSize, VTOHFS(vp)->hfs_phys_block_size),
                        NOCRED,
                        &bp);

//	DBG_TREE(("GetBlockProc: bp->b_bufsize=%ld, bp->b_bcount=%ld, bp->b_lblkno=%ld\n", bp->b_bufsize, bp->b_bcount, bp->b_lblkno));

    DBG_ASSERT(bp != NULL);
    DBG_ASSERT(bp->b_data != NULL);
    DBG_ASSERT(bp->b_bcount == block->blockSize);
    DBG_ASSERT(bp->b_lblkno == blockNum);

    if (bp == NULL)
        retval = -1;	//XXX need better error

    if (retval == E_NONE) {
        block->blockHeader = bp;
        block->buffer = bp->b_data + IOBYTEOFFSETFORBLK(bp->b_blkno, VTOHFS(vp)->hfs_phys_block_size);
        block->blockReadFromDisk = (bp->b_flags & B_CACHE) == 0;	/* not found in cache ==> came from disk */
    } else {
    	if (bp)
   			brelse(bp);
        block->blockHeader = NULL;
        block->buffer = NULL;
    }

    return (retval);
}


OSStatus ReleaseBTreeBlock(FileReference vp, BlockDescPtr blockPtr, ReleaseBlockOptions options)
{
    OSStatus	retval = E_NONE;
    struct buf *bp = NULL;
    int s;

    bp = (struct buf *) blockPtr->blockHeader;

    if (bp == NULL) {
        DBG_TREE(("ReleaseBlockProc: blockHeader is zero!\n"));
        retval = -1;
        goto exit;
    }

//	DBG_TREE(("ReleaseBlockProc: bp->b_lblkno=%ld, bp->b_bcount=%ld\n\n", bp->b_lblkno, bp->b_bcount));

    if (options & kTrashBlock) {
        bp->b_flags |= B_INVAL;
    	brelse(bp);	/* note: B-tree code will clear blockPtr->blockHeader and blockPtr->buffer */
    } else {
        if (options & kForceWriteBlock) {
            bp->b_flags |= B_DIRTY;
            retval = bwrite(bp);
        } else if (options & kMarkBlockDirty) {
            bp->b_flags |= B_DIRTY;
#if FORCESYNCBTREEWRITES
            bwrite(bp);
#else

            /*
             *
             * Set the B_LOCKED flag and unlock the buffer, causing brelse to move
             * the buffer onto the LOCKED free list.  This is necessary, otherwise
             * getnewbuf() would try to reclaim the buffers using bawrite, which
             * isn't going to work.
             *
             */
            bp->b_flags |= B_LOCKED;
            bdwrite(bp);

#endif
        } else {
    		brelse(bp);	/* note: B-tree code will clear blockPtr->blockHeader and blockPtr->buffer */
        };
    };

exit:
    return (retval);
}


OSStatus ExtendBTreeFile(FileReference vp, FSSize minEOF, FSSize maxEOF)
{
#pragma unused (maxEOF)

	OSStatus	retval;
	UInt32		actualBytesAdded;
	UInt32		bytesToAdd;
    UInt32		extendFlags;
	BTreeInfoRec btInfo;
	ExtendedVCB	*vcb;
	FCB			*filePtr;
    struct proc *p = NULL;


	filePtr = GetFileControlBlock(vp);

	if ( minEOF > filePtr->fcbEOF )
	{
		bytesToAdd = minEOF - filePtr->fcbEOF;

		if (bytesToAdd < filePtr->fcbClmpSize)
			bytesToAdd = filePtr->fcbClmpSize;		//XXX why not always be a mutiple of clump size?
	}
	else
	{
		DBG_TREE((" ExtendBTreeFile: minEOF is smaller than current size!"));
		return -1;
	}

	vcb = filePtr->fcbVPtr;
	
	/*
	 * The Extents B-tree can't have overflow extents. ExtendFileC will
	 * return an error if an attempt is made to extend the Extents B-tree
	 * when the resident extents are exhausted.
	 */
    /* XXX warning - this can leave the volume bitmap unprotected during ExtendFileC call */
	if(filePtr->fcbFlNm != kHFSExtentsFileID)
	{
		p = CURRENT_PROC;
		/* lock extents b-tree (also protects volume bitmap) */
		retval = hfs_metafilelocking(VTOHFS(vp), kHFSExtentsFileID, LK_EXCLUSIVE, p);
		if (retval)
			return (retval);
	}

    (void) BTGetInformation(filePtr, 0, &btInfo);

	/*
     * The b-tree code expects nodes to be contiguous. So when
	 * the allocation block size is less than the b-tree node
     * size, we need to force disk allocations to be contiguous.
     */
	if (vcb->blockSize >= btInfo.nodeSize) {
		extendFlags = 0;
	} else {
		/* Ensure that all b-tree nodes are contiguous on disk */
		extendFlags = kEFAllMask | kEFContigMask;
	}

    retval = ExtendFileC(vcb, filePtr, bytesToAdd, extendFlags, &actualBytesAdded );

	if(filePtr->fcbFlNm != kHFSExtentsFileID)
		(void) hfs_metafilelocking(VTOHFS(vp), kHFSExtentsFileID, LK_RELEASE, p);

	if (retval)
		return (retval);

	if (actualBytesAdded < bytesToAdd)
		DBG_TREE((" ExtendBTreeFile: actualBytesAdded < bytesToAdd!"));
		
	filePtr->fcbEOF = filePtr->fcbPLen;		// new B-tree looks at fcbEOF
	
	/*
	 * Update the Alternate MDB or Alternate VolumeHeader
	 */
	if ( vcb->vcbSigWord == kHFSPlusSigWord )
	{
		//	If any of the HFS+ private files change size, flush them back to the Alternate volume header
		if (	(filePtr->fcbFlNm == kHFSExtentsFileID) 
			 ||	(filePtr->fcbFlNm == kHFSCatalogFileID)
			 ||	(filePtr->fcbFlNm == kHFSStartupFileID)
			 ||	(filePtr->fcbFlNm == kHFSAttributesFileID) )
		{
			MarkVCBDirty( vcb );
			retval = FlushAlternateVolumeControlBlock( vcb, true );
		}
	}
	else if ( vcb->vcbSigWord == kHFSSigWord )
	{
		if ( filePtr->fcbFlNm == kHFSExtentsFileID )
		{
			vcb->vcbXTAlBlks = filePtr->fcbPLen / vcb->blockSize;
			MarkVCBDirty( vcb );
			retval = FlushAlternateVolumeControlBlock( vcb, false );
		}
		else if ( filePtr->fcbFlNm == kHFSCatalogFileID )
		{
			vcb->vcbCTAlBlks = filePtr->fcbPLen / vcb->blockSize;
			MarkVCBDirty( vcb );
			retval = FlushAlternateVolumeControlBlock( vcb, false );
		}
	}
	
	return retval;

}

