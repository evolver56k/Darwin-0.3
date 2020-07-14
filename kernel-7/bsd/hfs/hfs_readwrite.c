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
/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)hfs_readwrite.c	1.0
 *	derived from @(#)ufs_readwrite.c	8.11 (Berkeley) 5/8/95
 *
 *	(c) 1998       Apple Computer, Inc.  All Rights Reserved
 *	(c) 1990, 1992 NeXT Computer, Inc.  All Rights Reserved
 *	
 *
 *	hfs_readwrite.c -- vnode operations to deal with reading and writing files.
 *
 *	MODIFICATION HISTORY:
 *	 3-Feb-1999	Pat Dirks		Merged in Joe's change to hfs_truncate to skip vinvalbuf if LEOF isn't changing (#2302796)
 *								Removed superfluous (and potentially dangerous) second call to vinvalbuf() in hfs_truncate.
 *	 2-Dec-1998	Pat Dirks		Added support for read/write bootstrap ioctls.
 *	10-Nov-1998	Pat Dirks		Changed read/write/truncate logic to optimize block sizes for first extents of a file.
 *                              Changed hfs_strategy to correct I/O sizes from cluser code I/O requests in light of
 *                              different block sizing.  Changed bexpand to handle RELEASE_BUFFER flag.
 *	22-Sep-1998	Don Brady		Changed truncate zero-fill to use bwrite after several bawrites have been queued.
 *	11-Sep-1998	Pat Dirks		Fixed buffering logic to not rely on B_CACHE, which is set for empty buffers that
 *								have been pre-read by cluster_read (use b_validend > 0 instead).
 *  27-Aug-1998	Pat Dirks		Changed hfs_truncate to use cluster_write in place of bawrite where possible.
 *	25-Aug-1998	Pat Dirks		Changed hfs_write to do small device-block aligned writes into buffers without doing
 *								read-ahead of the buffer.  Added bexpand to deal with incomplete [dirty] buffers.
 *								Fixed can_cluster macro to use MAXPHYSIO instead of MAXBSIZE.
 *	19-Aug-1998	Don Brady		Remove optimization in hfs_truncate that prevented extra physical blocks from
 *								being truncated (radar #2265750). Also set fcb->fcbEOF before calling vinvalbuf.
 *	 7-Jul-1998	Pat Dirks		Added code to honor IO_NOZEROFILL in hfs_truncate.
 *	16-Jul-1998	Don Brady		In hfs_bmap use MAXPHYSIO instead of MAXBSIZE when calling MapFileBlockC (radar #2263753).
 *	16-Jul-1998	Don Brady		Fix error handling in hfs_allocate (radar #2252265).
 *	04-Jul-1998	chw				Synchronized options in hfs_allocate with flags in call to ExtendFileC
 *	25-Jun-1998	Don Brady		Add missing blockNo incrementing to zero fill loop in hfs_truncate.
 *	22-Jun-1998	Don Brady		Add bp = NULL assignment after brelse in hfs_read.
 *	 4-Jun-1998	Pat Dirks		Split off from hfs_vnodeops.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <mach/machine/vm_types.h>
#include <sys/vnode.h>
//#include <sys/vnode_if.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/signalvar.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/attr.h>
#include <miscfs/specfs/specdev.h>


#include <sys/vm.h>
#include <libkern/libkern.h>
#include <mach/machine/simple_lock.h>
#include <machine/spl.h>
#include <mach/features.h>
#if MACH_NBC
#include <kern/mapfs.h>
#endif /* MACH_NBC */

#include	"hfs.h"
#include	"hfs_dbg.h"
#include	"hfscommon/headers/FileMgrInternal.h"
#include	"hfscommon/headers/CatalogPrivate.h"

#define can_cluster(size) ((((size & (4096-1))) == 0) && (size <= (MAXPHYSIO/2)))

enum {
	MAXHFSFILESIZE = 0x7FFFFFFF		/* this needs to go in the mount structure */
};

extern void vnode_pager_setsize( struct vnode *vp, u_long nsize);
extern int vnode_uncache( struct vnode	*vp);

/*
 * Enabling cluster read/write operations.
 */
extern int doclusterread;
extern int doclusterwrite;

#if DBG_VOP_TEST_LOCKS
extern void DbgVopTest(int maxSlots, int retval, VopDbgStoreRec *VopDbgStore, char *funcname);
#endif

int bexpand(struct buf *bp, int newsize, struct buf **nbpp, long flags);

#if DIAGNOSTIC
void debug_check_blocksizes(struct vnode *vp);
#endif

/*****************************************************************************
*
*	Operations on vnodes
*
*****************************************************************************/

/*
#% read		vp	L L L
#
 vop_read {
     IN struct vnode *vp;
     INOUT struct uio *uio;
     IN int ioflag;
     IN struct ucred *cred;

     */

int
hfs_read(ap)
struct vop_read_args /* {
    struct vnode *a_vp;
    struct uio *a_uio;
    int a_ioflag;
    struct ucred *a_cred;
} */ *ap;
{
    register struct vnode 	*vp;
    struct hfsnode 			*hp;
    register struct uio 	*uio;
    struct buf 				*bp;
    daddr_t 				logBlockNo;
	u_long					fragSize, moveSize, startOffset, ioxfersize;
    long					devBlockSize = 0;
    off_t 					bytesRemaining;
    int 					retval;
    u_short 				mode;
	FCB						*fcb;
    Boolean 				firstpass;		/* Used for cluster reading */
    int 					seq;			/* Also used for cluster reading */

    DBG_FUNC_NAME("hfs_read");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    vp = ap->a_vp;
    hp = VTOH(vp);
    fcb = HTOFCB(hp);
    mode = hp->h_meta->h_mode;
    uio = ap->a_uio;

#if DIAGNOSTIC
    if (uio->uio_rw != UIO_READ)
        panic("%s: mode", funcname);
#endif

    /* Can only read files */
    if (ap->a_vp->v_type != VREG && ap->a_vp->v_type != VLNK) {		
        DBG_VOP_LOCKS_TEST(EISDIR);
        return (EISDIR);
    }
    DBG_VOP(("\tfile size Ox%lX\n", (UInt32)fcb->fcbEOF));
    DBG_VOP(("\tstarting at offset Ox%X of file, length Ox%X\n", (u_int)uio->uio_offset, (u_int)uio->uio_resid));

#if DIAGNOSTIC
    debug_check_blocksizes(vp);
#endif

    /*
     * If they didn't ask for any data, then we are done.
     */
    if (uio->uio_resid == 0) {
        DBG_VOP_LOCKS_TEST(E_NONE);
        return (E_NONE);
    }

    /* cant read from a negative offset */
    if (uio->uio_offset < 0) {
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

	if ((u_int64_t)uio->uio_offset > (u_int64_t)fcb->fcbEOF) {
		if ((u_int64_t)uio->uio_offset > (u_int64_t)MAXHFSFILESIZE)
			retval = EFBIG;
		else
        	retval = E_NONE;

		DBG_VOP_LOCKS_TEST(retval);
		return (retval);
	}

    VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);

    for (retval = 0, bp = NULL, firstpass = TRUE; uio->uio_resid > 0; bp = NULL) {

        if ((bytesRemaining = (fcb->fcbEOF - uio->uio_offset)) <= 0)
            break;

        MapFileOffset(hp, uio->uio_offset, &logBlockNo, &fragSize, &startOffset);

        DBG_VOP(("\tat logBlockNo Ox%X, with Ox%lX left to read\n", logBlockNo, (UInt32)uio->uio_resid));
        moveSize = ioxfersize = fragSize;
        DBG_VOP(("\tmoveSize = Ox%lX; ioxfersize = Ox%lX; startOffset = Ox%lX.\n",
					moveSize, ioxfersize, startOffset));
        DBG_ASSERT(moveSize >= startOffset);
        moveSize -= startOffset;
        if (bytesRemaining < moveSize) moveSize = bytesRemaining;
        
        if (uio->uio_resid < moveSize) {
        	moveSize = uio->uio_resid;
            DBG_VOP(("\treducing moveSize to Ox%lX (uio->uio_resid).\n", moveSize)); 
        };

        if (moveSize == 0) {
            break;
        };
        DBG_VOP(("\tat logBlockNo Ox%X, extent of Ox%lX, xfer of Ox%lX; moveSize = Ox%lX\n", logBlockNo, fragSize, ioxfersize, moveSize));
        if (( uio->uio_offset + fragSize) >= fcb->fcbEOF) {
            retval = bread(vp, logBlockNo, ioxfersize, NOCRED, &bp);
        } else if (doclusterread && !(vp->v_flag & VRAOFF) && can_cluster(fragSize)) {
            retval = cluster_read(vp, fcb->fcbEOF, logBlockNo, ioxfersize, NOCRED, &bp,
						   devBlockSize, firstpass, (uio->uio_resid + startOffset), &seq);
        } else if (logBlockNo - 1 == vp->v_lastr && !(vp->v_flag & VRAOFF)) {
            daddr_t nextLogBlockNo = logBlockNo + 1;
            long nextsize;
            int nextsize_for_bsd;
            long nextblkoffset;

            MapFileOffset(hp, uio->uio_offset + fragSize, &nextLogBlockNo, &nextsize, &nextblkoffset);
            /* Yuck!  This copy exists just because I refuse to write the interface to MapFileOffset relying on 'int' to be 'long'... */
            nextsize_for_bsd = nextsize;
            retval = breadn(vp, logBlockNo, ioxfersize, &nextLogBlockNo, &nextsize_for_bsd, 1, NOCRED, &bp);
        } else {
            retval = bread(vp, logBlockNo, ioxfersize, NOCRED, &bp);
        };

        if (retval != E_NONE) {
            if (bp) {
                brelse(bp);
                bp = NULL;
            }
            break;
        };

        firstpass = FALSE;

        if (bp->b_validend > 0) {
            /*
               b_validoff, b_validend, b_dirtyoff, and b_dirtyend are valid for blocks in the cache:
               The only blocks that are incomplete (b_validend < fragSize) are blocks that have been
               partially written from the start of the buffer:
             */
            if (bp->b_validend < bp->b_bcount) {
                DBG_ASSERT((bp->b_dirtyoff == 0) && (bp->b_validoff == 0) && (bp->b_dirtyend <= bp->b_validend));
                /* Incomplete blocks must have only device-block multiples of data... */
                DBG_ASSERT((bp->b_validend % devBlockSize) == 0);
                bp->b_bcount = bp->b_validend;

                if (bp->b_validend < (startOffset + moveSize)) {		/* buffer doesn't hold enough valid data for this read request */
                    /* We stumbled onto an incomplete [but successfully acquired] buffer:
                       try to get the full contents now because it's the only way to get
                       the data in this logical block...
                     */
                    retval = bexpand(bp, ioxfersize, &bp, 0);
                };
            };
        } else {
            /* This block was newly allocated; b_validoff, b_validend, b_dirtyoff, and b_dirtyend are
               all set to zero, which is fine except for b_validend: */
            DBG_ASSERT(bp->b_validoff == 0);
            bp->b_validend = bp->b_bcount;
            DBG_ASSERT(bp->b_dirtyoff == 0);
            DBG_ASSERT(bp->b_dirtyend == 0);
        };
	
        vp->v_lastr = logBlockNo;

        /*
         * We should only get non-zero b_resid when an I/O retval
         * has occurred, which should cause us to break above.
         * However, if the short read did not cause an retval,
         * then we want to ensure that we do not uiomove bad
         * or uninitialized data.
         */
        ioxfersize -= bp->b_resid;
        if (ioxfersize < moveSize) {			/* XXX PPD This should take the offset into account, too! */
            if (ioxfersize == 0)
                break;
            moveSize = ioxfersize;
        }

        DBG_VOP(("\tcopying Ox%lX bytes from %p; resid = Ox%lX...\n", moveSize, (char *)bp->b_data + startOffset, bp->b_resid));
        if ((retval =
             uiomove((caddr_t)bp->b_data + startOffset, (int)moveSize, uio)))
            break;

        if (S_ISREG(mode) &&
            (((startOffset + moveSize) == fragSize) || (uio->uio_offset == fcb->fcbEOF))) {
            bp->b_flags |= B_AGE;
        };

        DBG_ASSERT(bp->b_bcount == bp->b_validend);
        brelse(bp);
        /* Start of loop resets bp to NULL before reaching outside this block... */
    }

    if (bp != NULL) {
        DBG_ASSERT(bp->b_bcount == bp->b_validend);
        brelse(bp);
    };
	if (HTOVCB(hp)->vcbSigWord == kHFSPlusSigWord)
    	hp->h_meta->h_nodeflags |= IN_ACCESS;

    DBG_VOP_LOCKS_TEST(retval);

#if DIAGNOSTIC
    debug_check_blocksizes(vp);
#endif

    return (retval);
}

/*
 * Write data to a file or directory.
#% write	vp	L L L
#
 vop_write {
     IN struct vnode *vp;
     INOUT struct uio *uio;
     IN int ioflag;
     IN struct ucred *cred;

     */
int
hfs_write(ap)
struct vop_write_args /* {
    struct vnode *a_vp;
    struct uio *a_uio;
    int a_ioflag;
    struct ucred *a_cred;
} */ *ap;
{
    struct hfsnode 		*hp = VTOH(ap->a_vp);
    struct uio 			*uio = ap->a_uio;
    struct vnode 		*vp = ap->a_vp ;
    struct vnode 		*dev;
    struct buf 			*bp;
    struct proc 		*p, *cp;
    FCB					*fcb = HTOFCB(hp);
    ExtendedVCB			*vcb = HTOVCB(hp);
    long				devBlockSize = 0;
    daddr_t 			logBlockNo;
    long				fragSize;
    off_t 				origFileSize, currOffset, writelimit, bytesToAdd;
    u_long				blkoffset, resid, xfersize, clearSize;				
    UInt32				actualBytesAdded;
    int					flags, ioflag;
    int 				retval;
    DBG_FUNC_NAME("write");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP(("\thfsnode 0x%x (%s)\n", (u_int)hp, H_NAME(hp)));
    DBG_VOP(("\tstarting at offset Ox%lX of file, length Ox%lX\n", (UInt32)uio->uio_offset, (UInt32)uio->uio_resid));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    dev = hp->h_devvp;

#if DIAGNOSTIC
    debug_check_blocksizes(vp);
#endif

    if (uio->uio_offset < 0) {
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

    if (uio->uio_resid == 0) {
        DBG_VOP_LOCKS_TEST(E_NONE);
        return (E_NONE);
    }

    if (ap->a_vp->v_type != VREG && ap->a_vp->v_type != VLNK) {		/* Can only write files */
        DBG_VOP_LOCKS_TEST(EISDIR);
        return (EISDIR);
    };

#if MACH_NBC
	(void)vnode_uncache(vp);			/* XXX PPD Is this the right place to call this?  It'll VOP_UNLOCK the vnode */
#endif /* MACH_NBC */

#if DIAGNOSTIC
    if (uio->uio_rw != UIO_WRITE)
    	panic("%s: mode", funcname);
#endif

    ioflag = ap->a_ioflag;
    uio = ap->a_uio;
    vp = ap->a_vp;

    if (ioflag & IO_APPEND)
    	uio->uio_offset = fcb->fcbEOF;
    if ((hp->h_meta->h_pflags & APPEND) && uio->uio_offset != fcb->fcbEOF)
    	return (EPERM);

    writelimit = uio->uio_offset + uio->uio_resid;

    /*
    * Maybe this should be above the vnode op call, but so long as
    * file servers have no limits, I don't think it matters.
    */
    p = uio->uio_procp;
    if (vp->v_type == VREG && p &&
        writelimit > p->p_rlimit[RLIMIT_FSIZE].rlim_cur) {
        psignal(p, SIGXFSZ);
        return (EFBIG);
    };
    VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);

    resid = uio->uio_resid;
    origFileSize = fcb->fcbPLen;
    flags = ioflag & IO_SYNC ? B_SYNC : 0;

    DBG_VOP(("\tLEOF is 0x%lX, PEOF is 0x%lX.\n", fcb->fcbEOF, fcb->fcbPLen));

    /*
    NOTE:	In the following loop there are two positions tracked:
    currOffset is the current I/O starting offset.  currOffset is never >LEOF; the
    LEOF is nudged along with currOffset as data is zeroed or written.
    uio->uio_offset is the start of the current I/O operation.  It may be arbitrarily
    beyond currOffset.

    The following is true at all times:

    currOffset <= LEOF <= uio->uio_offset <= writelimit
    */
    currOffset = MIN(uio->uio_offset, fcb->fcbEOF);

    DBG_VOP(("\tstarting I/O loop at 0x%lX.\n", (u_long)currOffset));

    cp = CURRENT_PROC;

    for (retval = 0; uio->uio_resid > 0;) {
        /* Now test if we need to extend the file */
        /* Doing so will adjust the fcbPLen for us */
        if (writelimit > (off_t)fcb->fcbPLen) {
            /* XXX PPD Allocate only 2GB at a time until ExtendFileC's interface changes... */
            bytesToAdd = writelimit - (off_t)fcb->fcbPLen;
            if (bytesToAdd > (off_t)0x80000000) bytesToAdd = (off_t)0x80000000;
            DBG_VOP(("\textending file by 0x%lX bytes; 0x%lX blocks free", (unsigned long)bytesToAdd, (unsigned long)vcb->vcbFreeBks));
            /* lock extents b-tree (also protects volume bitmap) */
            retval = hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_EXCLUSIVE, cp);
            if (retval != E_NONE)
                break;

            retval = MacToVFSError(
                                ExtendFileC (vcb,
                                                fcb,
                                                (unsigned long)bytesToAdd,
                                                kEFContigBit,
                                                &actualBytesAdded));

            (void) hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_RELEASE, cp);
            DBG_VOP_CONT(("\tactual bytes added = 0x%lX bytes, retval = %d...\n", actualBytesAdded, retval));
            if ((actualBytesAdded == 0) && (retval == 0)) retval = ENOSPC;
            if (retval != E_NONE)
                break;

            UpdateBlockMappingTable(hp);

            /* We successfully extended the file: take a fresh look to see how things have changed... */
            continue;
        };

        /* What block are we starting the write */
        MapFileOffset(hp, currOffset, &logBlockNo, &fragSize, &blkoffset);

        xfersize = fragSize - blkoffset;

        DBG_VOP(("\tcurrOffset = Ox%lX, logBlockNo = Ox%X, blkoffset = Ox%lX, xfersize = Ox%lX, fragSize = Ox%lX.\n",
                (unsigned long)currOffset, logBlockNo, blkoffset, xfersize, fragSize));

        /* Make any adjustments for boundary conditions */
        if (currOffset + (off_t)xfersize > writelimit) {
            xfersize = writelimit - currOffset;
            DBG_VOP(("\ttrimming xfersize to 0x%lX to match writelimit (uio_resid)...\n", xfersize));
        };
        
        /*
        * There is no need to read into bp if:
        * We start on a block boundary and will overwrite the whole block
        *
        *						OR
        *
        * The transfer starts on a device block boundary and is an even
        * multiple of the device block size (in which case we'll write
        * through the data presented)
        *
        */
        if ((blkoffset == 0) && ((xfersize >= fragSize) || (xfersize % devBlockSize == 0))) {
            DBG_VOP(("\tRequesting %ld-byte block Ox%lX w/o read...\n", fragSize, (long)logBlockNo));
            bp = getblk(vp, logBlockNo, fragSize, 0, 0);
            retval = 0;
            if (bp->b_blkno == -1) {
                brelse(bp);
                retval = EIO;		/* XXX */
                break;
            }

			if (bp->b_flags & (B_DELWRI | B_DONE | B_CACHE)) {
				/*
                   This buffer is fully pre-read and may already have some modified data in it:
				   it doesn't need special-casing for write-through.
                 */
			} else {
				/* This is an empty buffer just allocated for our use:
                   b_validoff, b_validend, b_dirtyoff, and b_dirtyend are all zero,
                   which is exactly right (b_bcount is set to fragSize). */
				
				/* XXX PPD Can't this be skipped if b_blkno != logBlockNo? */
				
                /* Setting up the physical block number is required
                   (1) to keep cluster_write informed about which blocks can be clustered, and
                   (2) to avoid a second call to bmap() in strategy() on the write.
                 */
                if ((retval = VOP_BMAP(vp, logBlockNo, NULL, &bp->b_blkno, NULL))) {
                    brelse(bp);
                    break;
                };

                bp->b_bcount = 0;		/* No valid data in block yet */
                bp->b_validend = 0;		/* No valid data in block yet */
            };
        } else {
            /*
            * This I/O transfer is not sufficiently aligned, so read the affected block into a buffer:
            */
            DBG_VOP(("\tRequesting block Ox%X, size = 0x%08lX...\n", logBlockNo, fragSize));
            retval = bread(vp, logBlockNo, fragSize, ap->a_cred, &bp);
            if (retval != E_NONE) {
                if (bp) brelse(bp);
                break;
            }
        }
        
        /* Fix up the anciliary fields for this buffer, depending on whether they've been initialized yet: */
        if (bp->b_validend > 0) {
            /*
               b_validoff, b_validend, b_dirtyoff, and b_dirtyend are valid for blocks in the cache:
               The only blocks that are incomplete (b_validend < fragSize) are blocks that have been
               partially written from the start of the buffer:
             */
            if (bp->b_validend < bp->b_bcount) {
                DBG_ASSERT((bp->b_dirtyoff == 0) && (bp->b_validoff == 0) && (bp->b_dirtyend <= bp->b_validend));
                /* Incomplete blocks must have only device-block multiples of data... */
                DBG_ASSERT((bp->b_validend % devBlockSize) == 0);
                bp->b_bcount = bp->b_validend;

                if ((bp->b_validend < blkoffset) ||		/* ... valid data does not overlap (or at least abut) start of new write */
                    (bp->b_bufsize < (blkoffset + xfersize)) ||		/* ... or isn't enough buffer space to hold entire transfer */
                    ((blkoffset + xfersize) % devBlockSize != 0)) {	/* ... or won't leave blocksize-multiple of dirty bytes */
                    /* We stumbled onto an incomplete [but successfully acquired] buffer:
                       try to get the full contents now because it's the only way to get
                       the data in this logical block...
                     */
                    retval = bexpand(bp, fragSize, &bp, 0);
                };
            };
        } else {
            /* This buffer was either just allocated or just read in its entirity [see b_bcount]:
               b_validoff, b_validend, b_dirtyoff, and b_dirtyend are all zeroed, which is almost correct. */
            DBG_ASSERT(bp->b_validoff == 0);
            bp->b_validend = bp->b_bcount;	/* b_bcount > 0 iff block was actually read from disk in bread */
            DBG_ASSERT(bp->b_dirtyoff == 0);
            DBG_ASSERT(bp->b_dirtyend == 0);
        };

        /* See if we are starting to write within file boundaries:
            If not, then we need to present a "hole" for the area between
            the current EOF and the start of the current I/O operation:

            Note that currOffset is only less than uio_offset if uio_offset > LEOF...
            */
        if (uio->uio_offset > currOffset) {
            clearSize = MIN(uio->uio_offset - currOffset, xfersize);
            DBG_VOP(("\tzeroing Ox%lX bytes Ox%lX bytes into block Ox%X...\n", clearSize, blkoffset, logBlockNo));
            bzero(bp->b_data + blkoffset, clearSize);
            currOffset += clearSize;
            blkoffset += clearSize;
            xfersize -= clearSize;
        };

        if (xfersize > 0) {
            DBG_VOP(("\tCopying Ox%lX bytes Ox%lX bytes into block Ox%X... ioflag == 0x%X\n",
                     xfersize, blkoffset, logBlockNo, ioflag));
            retval = uiomove((caddr_t)bp->b_data + blkoffset, (int)xfersize, uio);
            currOffset += xfersize;
        };

        if (blkoffset + xfersize > bp->b_dirtyend) bp->b_dirtyend = blkoffset + xfersize;	/* Newly written data is now [also] dirty */
        if (bp->b_dirtyend > bp->b_validend) bp->b_validend = bp->b_dirtyend;				/* Data just written is now valid, too */
        bp->b_bcount = bp->b_validend;

        DBG_ASSERT((bp->b_bcount % devBlockSize) == 0);
        DBG_ASSERT(bp->b_bcount == bp->b_validend);
        if (ioflag & IO_SYNC) {
            (void)bwrite(bp);
            //DBG_VOP(("\tissuing bwrite\n"));
        } else if ((xfersize + blkoffset) == fragSize) {
            if (doclusterwrite && can_cluster(fragSize)) {
                //DBG_VOP(("\tissuing cluster_write\n"));
                cluster_write(bp, fcb->fcbPLen, devBlockSize);
            } else {
                //DBG_VOP(("\tissuing bawrite\n"));
                bp->b_flags |= B_AGE;
                bawrite(bp);
            }
        } else {
            //DBG_VOP(("\tissuing bdwrite\n"));
            bdwrite(bp);
        };

        /* Update the EOF if we just extended the file
           (the PEOF has already been moved out and the block mapping table has been updated): */
        if (currOffset > fcb->fcbEOF) {
            DBG_VOP(("\textending EOF to 0x%lX...\n", fcb->fcbEOF));
        fcb->fcbEOF = currOffset;
#if MACH_NBC
            if ((vp->v_type == VREG) && (vp->v_vm_info && !(vp->v_vm_info->mapped))) {
#endif /* MACH_NBC */
            vnode_pager_setsize(vp, (u_long)fcb->fcbEOF);
#if MACH_NBC
            }
#endif /* MACH_NBC */
        };

        if (retval || (resid == 0))
            break;
        hp->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;
    };
    /*
    * If we successfully wrote any data, and we are not the superuser
    * we clear the setuid and setgid bits as a precaution against
    * tampering.
    */
    if (resid > uio->uio_resid && ap->a_cred && ap->a_cred->cr_uid != 0)
    	hp->h_meta->h_mode &= ~(ISUID | ISGID);
    if (retval) {
        if (ioflag & IO_UNIT) {
            (void)VOP_TRUNCATE(vp, origFileSize,
                            ioflag & IO_SYNC, ap->a_cred, uio->uio_procp);
            uio->uio_offset -= resid - uio->uio_resid;
            uio->uio_resid = resid;
        }
    } else if (resid > uio->uio_resid && (ioflag & IO_SYNC))
    	retval = VOP_UPDATE(vp, &time, &time, 1);
    
#if DIAGNOSTIC
    debug_check_blocksizes(vp);
#endif

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*

#% ioctl	vp	U U U
#
 vop_ioctl {
     IN struct vnode *vp;
     IN u_long command;
     IN caddr_t data;
     IN int fflag;
     IN struct ucred *cred;
     IN struct proc *p;

     */


/* ARGSUSED */
int
hfs_ioctl(ap)
struct vop_ioctl_args /* {
    struct vnode *a_vp;
    int  a_command;
    caddr_t  a_data;
    int  a_fflag;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    DBG_FUNC_NAME("ioctl");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);

    switch (ap->a_command) {
	
    case 1:
    {   register struct hfsnode *hp;
        register struct vnode *vp;
	register struct radvisory *ra;
	FCB *fcb;
	int devBlockSize = 0;
	int error;
	long size;
	daddr_t lbn;

	vp = ap->a_vp;

	VOP_LEASE(vp, ap->a_p, ap->a_cred, LEASE_READ);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, ap->a_p);

	ra = (struct radvisory *)(ap->a_data);
	hp = VTOH(vp);

	fcb = HTOFCB(hp);

	if ((u_int64_t)ra->ra_offset >= (u_int64_t)fcb->fcbEOF) {
	    VOP_UNLOCK(vp, 0, ap->a_p);
	    DBG_VOP_LOCKS_TEST(EFBIG);
	    return (EFBIG);
	}
	VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);
	size = hp->h_meta->h_logBlockSize;

	if ( !(can_cluster(size))) {
	    VOP_UNLOCK(vp, 0, ap->a_p);
	    DBG_VOP_LOCKS_TEST(EINVAL);
	    return (EINVAL);
	}
	size = MIN(size, MAXBSIZE);
	lbn = ra->ra_offset / size;

	error = advisory_read(vp, fcb->fcbEOF, lbn, size, size, ra->ra_count, devBlockSize);
	VOP_UNLOCK(vp, 0, ap->a_p);

	DBG_VOP_LOCKS_TEST(error);
	return (error);
    }

    case 2: /* F_READBOOTBLOCKS */
    case 3: /* F_WRITEBOOTBLOCKS */
      {
	    struct vnode *vp = ap->a_vp;
	    struct hfsnode *hp = VTOH(vp);
	    struct fbootstraptransfer *btd = (struct fbootstraptransfer *)ap->a_data;
	    u_long devBlockSize;
	    int error;
	    struct iovec aiov;
	    struct uio auio;
	    u_long blockNumber;
	    u_long blockOffset;
	    u_long xfersize;
	    struct buf *bp;

        if ((vp->v_flag & VROOT) == 0) return EINVAL;
        if (btd->fbt_offset + btd->fbt_length > 1024) return EINVAL;
	    
	    aiov.iov_base = btd->fbt_buffer;
	    aiov.iov_len = btd->fbt_length;
	    
	    auio.uio_iov = &aiov;
	    auio.uio_iovcnt = 1;
	    auio.uio_offset = btd->fbt_offset;
	    auio.uio_resid = btd->fbt_length;
	    auio.uio_segflg = UIO_USERSPACE;
	    auio.uio_rw = (ap->a_command == 3) ? UIO_WRITE : UIO_READ; /* F_WRITEBOOTSTRAP / F_READBOOTSTRAP */
	    auio.uio_procp = ap->a_p;

	    VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);

	    while (auio.uio_resid > 0) {
	      blockNumber = auio.uio_offset / devBlockSize;
	      error = bread(hp->h_devvp, blockNumber, devBlockSize, ap->a_cred, &bp);
	      if (error) {
              if (bp) brelse(bp);
              return error;
          };

          blockOffset = auio.uio_offset % devBlockSize;
	      xfersize = devBlockSize - blockOffset;
	      error = uiomove((caddr_t)bp->b_data + blockOffset, (int)xfersize, &auio);
          if (error) {
              brelse(bp);
              return error;
          };
          if (auio.uio_rw == UIO_WRITE) {
              error = bwrite(bp);
              if (error) return error;
          } else {
              brelse(bp);
          };
        };
      };
      return 0;

    default:
        DBG_VOP_LOCKS_TEST(ENOTTY);
        return (ENOTTY);
    }

    return 0;
}

/* ARGSUSED */
int
hfs_select(ap)
struct vop_select_args /* {
    struct vnode *a_vp;
    int  a_which;
    int  a_fflags;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    DBG_FUNC_NAME("select");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);

    /*
     * We should really check to see if I/O is possible.
     */
    DBG_VOP_LOCKS_TEST(1);
    return (1);
}



/*
 * Mmap a file
 *
 * NB Currently unsupported.
# XXX - not used
#
 vop_mmap {
     IN struct vnode *vp;
     IN int fflags;
     IN struct ucred *cred;
     IN struct proc *p;

     */

/* ARGSUSED */

int
hfs_mmap(ap)
struct vop_mmap_args /* {
    struct vnode *a_vp;
    int  a_fflags;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    DBG_FUNC_NAME("mmap");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);

    DBG_VOP_LOCKS_TEST(EINVAL);
    return (EINVAL);
}



/*
 * Seek on a file
 *
 * Nothing to do, so just return.
# XXX - not used
# Needs work: Is newoff right?  What's it mean?
#
 vop_seek {
     IN struct vnode *vp;
     IN off_t oldoff;
     IN off_t newoff;
     IN struct ucred *cred;
     */
/* ARGSUSED */
int
hfs_seek(ap)
struct vop_seek_args /* {
    struct vnode *a_vp;
    off_t  a_oldoff;
    off_t  a_newoff;
    struct ucred *a_cred;
} */ *ap;
{
    DBG_FUNC_NAME("seek");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);

    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}


/*
 * Bmap converts a the logical block number of a file to its physical block
 * number on the disk.
 */

/*
 * vp  - address of vnode file the file
 * bn  - which logical block to convert to a physical block number.
 * vpp - returns the vnode for the block special file holding the filesystem
 *	 containing the file of interest
 * bnp - address of where to return the filesystem physical block number
#% bmap		vp	L L L
#% bmap		vpp	- U -
#
 vop_bmap {
     IN struct vnode *vp;
     IN daddr_t bn;
     OUT struct vnode **vpp;
     IN daddr_t *bnp;
     OUT int *runp;
     */
/*
 * Converts a logical block number to a physical block, and optionally returns
 * the amount of remaining blocks in a run. The logical block is based on hfsNode.logBlockSize.
 * The physical block number is based on the device block size, currently its 512.
 * The block run is returned in logical blocks, and is the REMAINING amount of blocks
 */

int
hfs_bmap(ap)
struct vop_bmap_args /* {
    struct vnode *a_vp;
    daddr_t a_bn;
    struct vnode **a_vpp;
    daddr_t *a_bnp;
    int *a_runp;
} */ *ap;
{
    struct hfsnode 		*hp = VTOH(ap->a_vp);
    struct hfsmount 	*hfsmp = VTOHFS(ap->a_vp);
    int					retval = E_NONE;
    daddr_t				logBlockSize;
    UInt32				bytesContAvail = 0;
    struct proc			*p = NULL;
    int					lockExtBtree;

#define DEBUG_BMAP 0
#if DEBUG_BMAP
    DBG_FUNC_NAME("bmap");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP((": %d --> ", ap->a_bn));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    if (ap->a_vpp != NULL) {
        DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_UNLOCKED, VOPDBG_IGNORE, VOPDBG_POS);
    } else {
        DBG_VOP_LOCKS_INIT(1,NULL, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);
	};
#endif

    /*
     * Check for underlying vnode requests and ensure that logical
     * to physical mapping is requested.
     */
    if (ap->a_vpp != NULL)
        *ap->a_vpp = VTOH(ap->a_vp)->h_devvp;
    if (ap->a_bnp == NULL)
        return (0);

    lockExtBtree = hasOverflowExtents(hp);
    if (lockExtBtree)
    {
        p = CURRENT_PROC;
        retval = hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_EXCLUSIVE | LK_CANRECURSE, p);
        if (retval)
            return (retval);
    }


    if (ap->a_bn < hp->h_uniformblocksizestart) {
        int targetLogicalBlockNo = ap->a_bn;
        off_t fileOffset = 0;
        int extent;
        long extentSize;

        logBlockSize = MAXLOGBLOCKSIZE;
        for (extent = 0; extent < LOGBLOCKMAPENTRIES; ++extent) {
            if ((hp->h_logicalblocktable[extent].logicalBlockCount > 0) &&
                (targetLogicalBlockNo < hp->h_logicalblocktable[extent].logicalBlockCount)) {
                retval = MacToVFSError(
                                       MapFileBlockC (HFSTOVCB(hfsmp),
                                                      HTOFCB(hp),
                                                      MAXPHYSIO,
                                                      fileOffset + (targetLogicalBlockNo * MAXLOGBLOCKSIZE),
                                                      (UInt32 *)ap->a_bnp,
                                                      &bytesContAvail));
                /* Take the current FCB's extent length info: we could be mid-update */
                if (HTOVCB(hp)->vcbSigWord == kHFSSigWord) {
                    extentSize = hp->h_xfcb->fcb_fcb.fcbExtRec[extent].blockCount * HTOVCB(hp)->blockSize;
                } else {
                    extentSize = hp->h_xfcb->fcb_extFCB.extents[extent].blockCount * HTOVCB(hp)->blockSize;
                };
                DBG_ASSERT((bytesContAvail == MAXPHYSIO) || (bytesContAvail == (extentSize - (targetLogicalBlockNo * MAXLOGBLOCKSIZE))));
                break;
            };
            targetLogicalBlockNo -= hp->h_logicalblocktable[extent].logicalBlockCount;
            fileOffset += hp->h_logicalblocktable[extent].extentLength;
        };
    } else {
        logBlockSize = hp->h_meta->h_logBlockSize;
        retval = MacToVFSError(
                               MapFileBlockC (HFSTOVCB(hfsmp),
                                              HTOFCB(hp),
                                              MAXPHYSIO,
                                              hp->h_optimizedblocksizelimit +
                                                  ((ap->a_bn - hp->h_uniformblocksizestart) * logBlockSize),
                                              (UInt32 *)ap->a_bnp,
                                              &bytesContAvail));

    };

    if (lockExtBtree) (void) hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_RELEASE, p);

    if (retval == E_NONE) {
        /* Figure out how many read ahead blocks there are */
        if (ap->a_runp != NULL) {
            if (can_cluster(logBlockSize)) {
                /* Make sure this result never goes negative: */
                *ap->a_runp = (bytesContAvail < logBlockSize) ? 0 : (bytesContAvail / logBlockSize) - 1;
            } else {
                *ap->a_runp = 0;
            };
        };
    };

#if DEBUG_BMAP
    DBG_VOP(("%d:%d.\n", *ap->a_bnp, (bytesContAvail < logBlockSize) ? 0 : (bytesContAvail / logBlockSize) - 1));

    DBG_VOP_LOCKS_TEST(retval);
#endif

    if (ap->a_runp) {
        DBG_ASSERT((*ap->a_runp * logBlockSize) < bytesContAvail);							/* At least *ap->a_runp blocks left and ... */
        if (can_cluster(logBlockSize)) {
            DBG_ASSERT(bytesContAvail - (*ap->a_runp * logBlockSize) < (2*logBlockSize));	/* ... at most 1 logical block accounted for by current block */
                                                                                            /* ... plus some sub-logical block sized piece */
        };
    };

    return (retval);
}


/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
#
#vop_strategy {
#	IN struct buf *bp;
    */
int
hfs_strategy(ap)
struct vop_strategy_args /* {
    struct buf *a_bp;
} */ *ap;
{
    register struct buf *bp = ap->a_bp;
    register struct vnode *vp = bp->b_vp;
    register struct hfsnode *hp;
    long logBlockSize;
    int retval = 0;
	DBG_FUNC_NAME("strategy");

//	DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("\n"));

    hp = VTOH(vp);
    if (vp->v_type == VBLK || vp->v_type == VCHR)
        panic("hfs_strategy: device vnode passed!");

    /*
     * If we don't already know the filesystem relative block number
     * then get it using VOP_BMAP().  If VOP_BMAP() returns the block
     * number as -1 then we've got a hole in the file.  HFS filesystems
     * don't allow files with holes, so we shouldn't ever see this.
     */
    if (bp->b_blkno == bp->b_lblkno) {
        if ((retval = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL))) {
            bp->b_error = retval;
            bp->b_flags |= B_ERROR;
            biodone(bp);
            return (retval);
        }
        if ((long)bp->b_blkno == -1)
            clrbuf(bp);
    }
    if ((long)bp->b_blkno == -1) {
        biodone(bp);
        return (0);
    }
	
	/* Make sure some over-eager cluster code didn't generate an excessively large read: */
	if (bp->b_bcount != hp->h_meta->h_logBlockSize) {
		logBlockSize = LogicalBlockSize(hp, bp->b_lblkno);
        if ((bp->b_bcount > logBlockSize) && !(can_cluster(logBlockSize))) bp->b_bcount = logBlockSize;
	};
	
	if (bp->b_validend == 0) {
		/* Record the exact size of the I/O transfer about to be made: */
		DBG_ASSERT(bp->b_validoff == 0);
		bp->b_validend = bp->b_bcount;
		DBG_ASSERT(bp->b_dirtyoff == 0);
	};
	
    vp = hp->h_devvp;
    bp->b_dev = vp->v_rdev;
    DBG_VOP(("\t\t>>>%s: continuing w/ vp: 0x%x with logBlk Ox%X and phyBlk Ox%X\n", funcname, (u_int)vp, bp->b_lblkno, bp->b_blkno));

    return VOCALL (vp->v_op, VOFFSET(vop_strategy), ap);
}


/*
#% reallocblks	vp	L L L
#
 vop_reallocblks {
     IN struct vnode *vp;
     IN struct cluster_save *buflist;

     */

int
hfs_reallocblks(ap)
struct vop_reallocblks_args /* {
    struct vnode *a_vp;
    struct cluster_save *a_buflist;
} */ *ap;
{
    DBG_FUNC_NAME("reallocblks");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    /* Currently no support for clustering */		/* XXX */
    DBG_VOP_LOCKS_TEST(ENOSPC);
    return (ENOSPC);
}



/*
#
#% truncate	vp	L L L
#
vop_truncate {
    IN struct vnode *vp;
    IN off_t length;
    IN int flags;	(IO_SYNC)
    IN struct ucred *cred;
    IN struct proc *p;
};
 * Truncate the hfsnode hp to at most length size, freeing (or adding) the
 * disk blocks.
 */
int hfs_truncate(ap)
    struct vop_truncate_args /* {
        struct vnode *a_vp;
        off_t a_length;
        int a_flags;
        struct ucred *a_cred;
        struct proc *a_p;
    } */ *ap;
{
    register struct vnode *vp = ap->a_vp;
    register struct hfsnode *hp = VTOH(vp);
    off_t length = ap->a_length;
    long vflags;
    struct timeval tv;
    int retval;
    FCB *fcb;
    off_t bytesToAdd;
    UInt32 actualBytesAdded;
    long devBlockSize = 512;
    DBG_FUNC_NAME("truncate");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

#if DIAGNOSTIC
    debug_check_blocksizes(ap->a_vp);
#endif

    if (length < 0) {
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
	}

    if (length > (off_t)MAXHFSFILESIZE) {	/* XXX need to distinguish between hfs and hfs+ */
		DBG_VOP_LOCKS_TEST(EFBIG);
		return (EFBIG);
	}

    if (vp->v_type != VREG && vp->v_type != VLNK) {		
        DBG_VOP_LOCKS_TEST(EISDIR);
        return (EISDIR);		/* hfs doesn't support truncating of directories */
    }

    fcb = HTOFCB(hp);
    tv = time;
	retval = E_NONE;

    DBG_VOP(("%s: truncate from Ox%lX to Ox%X bytes\n", funcname, fcb->fcbPLen, (u_int)length));

	/* 
	 * we cannot just check if fcb->fcbEOF == length (as an optimization)
	 * since there may be extra physical blocks that also need truncation
	 */

#if MACH_NBC
    retval = mapfs_trunc(vp, (vm_offset_t)length);
    if (retval) {
        DBG_VOP_LOCKS_TEST(retval);
        return (retval);
    }
#endif /* MACH_NBC */

    /*
     * Lengthen the size of the file. We must ensure that the
     * last byte of the file is allocated. Since the smallest
     * value of fcbEOF is 0, length will be at least 1.
     */
	if ((u_long)length > fcb->fcbEOF) {
        off_t filePosition;
		daddr_t logBlockNo;
		long logBlockSize;
		long blkOffset;
		off_t bytestoclear;
		int blockZeroCount;
		struct buf *bp=NULL;

    	/*
    	 * If we don't have enough physical space then
    	 * we need to extend the physical size.
    	 */
        if ((u_long)length > fcb->fcbPLen) {
			/* lock extents b-tree (also protects volume bitmap) */
			retval = hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_EXCLUSIVE, ap->a_p);
            if (retval) goto Err_Exit;

            while (((u_long)length > fcb->fcbPLen) && (retval == E_NONE)) {
                /* XXX PPD Allocate only 2GB at a time until ExtendFileC's interface changes... */
                bytesToAdd = length - (off_t)fcb->fcbPLen;
                if (bytesToAdd > (off_t)0x80000000) bytesToAdd = (off_t)0x80000000;
                retval = MacToVFSError(
                                       ExtendFileC (HTOVCB(hp),
                                                    fcb,
                                                    (u_long)bytesToAdd,
                                                    kEFAllMask,	/* allocate all requested bytes or none */
                                                    &actualBytesAdded));

				if (actualBytesAdded == 0 && retval == E_NONE) {
					if ((u_long)length > fcb->fcbPLen)
						length = (off_t)fcb->fcbPLen;
					break;
				}

                /* If we just successfully extended the file by only 2GB, try again (just to be sure): */
                if (bytesToAdd == (off_t)0x80000000) continue;
			} 
			(void) hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_RELEASE, ap->a_p);
			if (retval) goto Err_Exit;

            DBG_ASSERT((u_long)length <= fcb->fcbPLen);

            UpdateBlockMappingTable(hp);

#if MACH_NBC
        	if ((vp->v_type == VREG) && (vp->v_vm_info && !(vp->v_vm_info->mapped))) {
#endif /* MACH_NBC */
                vnode_pager_setsize(vp, (u_long)length);
                vnode_uncache(vp);
#if MACH_NBC
        	}
#endif /* !MACH_NBC */  
		}
 
 		if (! (ap->a_flags & IO_NOZEROFILL)) {
	      /*
	       * zero out any new logical space...
	       */
            VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);

			bytestoclear = length - (off_t)fcb->fcbEOF;
            filePosition = (off_t)fcb->fcbEOF;
			while (bytestoclear > 0) {
                MapFileOffset(hp, filePosition, &logBlockNo, &logBlockSize, &blkOffset);
				blockZeroCount = MIN(bytestoclear, logBlockSize - blkOffset);
				if ((blkOffset == 0) && (bytestoclear >= logBlockSize)) {
					bp = getblk(vp, logBlockNo, logBlockSize, 0, 0);
                    retval = 0;
                    if (bp->b_flags & (B_DELWRI | B_DONE | B_CACHE)) {
                        /*
                           This buffer is fully pre-read and may already have some modified data in it:
                           it doesn't need special-casing for write-through.
                         */
                    } else {
                        /* This is an empty buffer just allocated for our use: */

                        /* XXX PPD Can't this be skipped if b_blkno != logBlockNo? */

                        /* Setting up the physical block number is required
                           (1) to keep cluster_write informed about which blocks can be clustered, and
                           (2) to avoid a second call to bmap() in strategy() on the write.
                         */
                        if ((retval = VOP_BMAP(vp, logBlockNo, NULL, &bp->b_blkno, NULL))) {
                            brelse(bp);
                            break;
                        };

                        bp->b_bcount = 0;		/* No valid data in block yet */
                        bp->b_validend = 0;		/* No valid data in block yet */
                    };
				} else {
					retval = bread(vp, logBlockNo, logBlockSize, ap->a_cred, &bp);
                    if (retval) {
                        brelse(bp);
                        goto Err_Exit;
                    }
				}
	
                /*
                   Fix up the anciliary fields for this buffer, depending on whether they've been initialized yet,
                   and check to see whether we need to get more data than what was just found in the cache:
                 */
                if (bp->b_validend > 0) {
                    /*
                       b_validoff, b_validend, b_dirtyoff, and b_dirtyend are valid for blocks in the cache:
                       The only blocks that are incomplete (b_validend < fragSize) are blocks that have been
                       partially written from the start of the buffer:
                     */
                    if (bp->b_validend < bp->b_bcount) {
                        /*
                           We stumbled onto an incomplete [but successfully acquired] buffer:
                           try to get the full contents now because it's the only way to get
                           the data in this logical block...
                         */
                        DBG_ASSERT((bp->b_dirtyoff == 0) && (bp->b_validoff == 0) && (bp->b_dirtyend <= bp->b_validend));
                        /* Incomplete blocks must have only device-block multiples of data... */
                        DBG_ASSERT((bp->b_validend % devBlockSize) == 0);
                        bp->b_bcount = bp->b_validend;

                        retval = bexpand(bp, logBlockSize, &bp, 0);
                    };
                } else {
                    /* This buffer was either just allocated or just read in its entirity [see b_bcount]: */
                    DBG_ASSERT(bp->b_validoff == 0);
                    bp->b_validend = bp->b_bcount;	/* b_bcount > 0 iff block was actually read from disk in bread */
                    DBG_ASSERT(bp->b_dirtyoff == 0);
                    DBG_ASSERT(bp->b_dirtyend == 0);
                };

				bzero((char *)bp->b_data + blkOffset, blockZeroCount);
				
                if (blkOffset + blockZeroCount > bp->b_dirtyend) bp->b_dirtyend = blkOffset + blockZeroCount;
                if (bp->b_dirtyend > bp->b_validend) bp->b_validend = bp->b_dirtyend;
                if (bp->b_validend > bp->b_bcount) bp->b_bcount = bp->b_validend;
                DBG_ASSERT(bp->b_bcount % devBlockSize == 0);
                DBG_ASSERT(bp->b_bcount == bp->b_validend);
                bp->b_flags |= B_DIRTY | B_AGE;
                if (ap->a_flags & IO_SYNC) {
                    bwrite(bp);
                } else if (doclusterwrite &&  can_cluster(logBlockSize)) {
                    cluster_write(bp, fcb->fcbPLen, devBlockSize);
				} else if (logBlockNo % 32) {
					bawrite(bp);
               } else {
                    bwrite(bp);	/* wait after we issue 32 requests */
               };

				bytestoclear -= blockZeroCount;
				if (blkOffset > 0)
					blkOffset = 0;

                filePosition += blockZeroCount;
			}
		}

		fcb->fcbEOF = (u_long)length;

   } else { /* Shorten the size of the file */

    	if (fcb->fcbEOF > (u_long)length) {
	        /*
		 * Any buffers that are past the truncation point need to be
		 * invalidated (to maintain buffer cache consistency).  For
		 * simplicity, we invalidate all the buffers by calling vinvalbuf.
		 */
	        vflags = ((length > 0) ? V_SAVE : 0);	/* XXX PPD Should we set SAVE_META? */
		retval = vinvalbuf(vp, vflags, ap->a_cred, ap->a_p, 0, 0);
	}
        /* lock extents b-tree (also protects volume bitmap) */
        retval = hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_EXCLUSIVE, ap->a_p);
        if (retval) goto Err_Exit;

        retval = MacToVFSError(
                               TruncateFileC(	
                                             HTOVCB(hp),
                                             fcb,
                                             (u_long)length,
                                             false));
        (void) hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_RELEASE, ap->a_p);
        if (retval) goto Err_Exit;

		fcb->fcbEOF = (u_long)length;
        if (fcb->fcbFlags &fcbModifiedMask)
            hp->h_meta->h_nodeflags |= IN_MODIFIED;

        UpdateBlockMappingTable(hp);

#if MACH_NBC
    if ((vp->v_type == VREG) && (vp->v_vm_info && !(vp->v_vm_info->mapped))) {
#endif /* MACH_NBC */
        vnode_pager_setsize(vp, (u_long)length);
#if MACH_NBC
    }
#endif /* MACH_NBC */

    }

	hp->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;
	retval = VOP_UPDATE(vp, &tv, &tv, MNT_WAIT);

Err_Exit:;

#if DIAGNOSTIC
    debug_check_blocksizes(ap->a_vp);
#endif

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}



/*
#
#% allocate	vp	L L L
#
vop_allocate {
    IN struct vnode *vp;
    IN off_t length;
    IN int flags;
    IN struct ucred *cred;
    IN struct proc *p;
};
 * allocate the hfsnode hp to at most length size
 */
int hfs_allocate(ap)
    struct vop_allocate_args /* {
        struct vnode *a_vp;
        off_t a_length;
        u_int32_t  a_flags;
	off_t *a_bytesallocated;
        struct ucred *a_cred;
        struct proc *a_p;
    } */ *ap;
{
    register struct vnode *vp = ap->a_vp;
    register struct hfsnode *hp = VTOH(vp);
    off_t length = ap->a_length;
    long vflags;
    u_long startingPEOF;
    struct timeval tv;
    int retval, retval2;
    FCB *fcb;
    UInt32 actualBytesAdded;
    UInt32 extendFlags =0;   /* For call to ExtendFileC */
    DBG_FUNC_NAME("allocate");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    /* Set the number of bytes allocated to 0 so that the caller will know that we
       did nothing.  ExtendFileC will fill this in for us if we actually allocate space */

    *(ap->a_bytesallocated) = 0; 

    /* Now for some error checking */

    if (length < (off_t)0) {
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

    if (length > (u_int64_t)0x7FFFFFFF) {   /* XXX need to distinguish between hfs and hfs+ */
        DBG_VOP_LOCKS_TEST(EFBIG);
        return (EFBIG);
    }

    if (vp->v_type != VREG && vp->v_type != VLNK) {
        DBG_VOP_LOCKS_TEST(EISDIR);
        return (EISDIR);        /* hfs doesn't support truncating of directories */
    }

    /* Fill in the flags word for the call to Extend the file */

	if (ap->a_flags & ALLOCATECONTIG) {
		extendFlags |= kEFContigMask;
	}

    if (ap->a_flags & ALLOCATEALL) {
		extendFlags |= kEFAllMask;
	}

    fcb = HTOFCB(hp);
    tv = time;
    retval = E_NONE;
    startingPEOF = fcb->fcbPLen;

    if (ap->a_flags & ALLOCATEFROMPEOF) {
		length += fcb->fcbPLen;
	}

    DBG_VOP(("%s: allocate from Ox%lX to Ox%X bytes\n", funcname, fcb->fcbPLen, (u_int)length));

    /* If no changes are necesary, then we're done */
    if (fcb->fcbPLen == (u_long)length)
    	goto Std_Exit;

    /*
    * Lengthen the size of the file. We must ensure that the
    * last byte of the file is allocated. Since the smallest
    * value of fcbPLen is 0, length will be at least 1.
    */
    if ((u_long)length > fcb->fcbPLen) {

		/* lock extents b-tree (also protects volume bitmap) */
		retval = hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_EXCLUSIVE, ap->a_p);
		if (retval) goto Err_Exit;

		retval = MacToVFSError(
								ExtendFileC(HTOVCB(hp),
											fcb,
											(u_long)length - fcb->fcbPLen,
											extendFlags,
											&actualBytesAdded));

		*(ap->a_bytesallocated) = actualBytesAdded;

		(void) hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_RELEASE, ap->a_p);

		DBG_ASSERT((u_long)length <= fcb->fcbPLen);

		UpdateBlockMappingTable(hp);
		
		/*
		 * if we get an error and no changes were made then exit
		 * otherwise we must do the VOP_UPDATE to reflect the changes
		 */
        if (retval && (startingPEOF == fcb->fcbPLen)) goto Err_Exit;

    } else { /* Shorten the size of the file */

    	if (fcb->fcbEOF > (u_long)length) {
			/*
			 * Any buffers that are past the truncation point need to be
			 * invalidated (to maintain buffer cache consistency).  For
			 * simplicity, we invalidate all the buffers by calling vinvalbuf.
			 */
			vflags = ((length > 0) ? V_SAVE : 0) | V_SAVEMETA;
			(void) vinvalbuf(vp, vflags, ap->a_cred, ap->a_p, 0, 0);
		}

       /* lock extents b-tree (also protects volume bitmap) */
        retval = hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_EXCLUSIVE, ap->a_p);
        if (retval) goto Err_Exit;

        retval = MacToVFSError(
                            TruncateFileC(
                                            HTOVCB(hp),
                                            fcb,
                                            (u_long)length,
                                            false));
        (void) hfs_metafilelocking(HTOHFS(hp), kHFSExtentsFileID, LK_RELEASE, ap->a_p);

		/*
		 * if we get an error and no changes were made then exit
		 * otherwise we must do the VOP_UPDATE to reflect the changes
		 */
		if (retval && (startingPEOF == fcb->fcbPLen)) goto Err_Exit;
        if (fcb->fcbFlags & fcbModifiedMask)
            hp->h_meta->h_nodeflags |= IN_MODIFIED;

        DBG_ASSERT((u_long)length <= fcb->fcbPLen)  // DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG

        if (fcb->fcbEOF > fcb->fcbPLen) {
			fcb->fcbEOF = fcb->fcbPLen;

            UpdateBlockMappingTable(hp);

#if MACH_NBC
            if ((vp->v_type == VREG) && (vp->v_vm_info && !(vp->v_vm_info->mapped))) {
#endif /* MACH_NBC */
                vnode_pager_setsize(vp, (u_long)fcb->fcbEOF);
#if MACH_NBC
            }
#endif /* MACH_NBC */
        }
        (void)vnode_uncache(vp);
    }

Std_Exit:
    hp->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;
	retval2 = VOP_UPDATE(vp, &tv, &tv, MNT_WAIT);

    if (retval == 0) retval = retval2;

Err_Exit:
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}




/* Pass-through pagein for HFS filesystem */
int
hfs_pagein(ap)
	struct vop_pagein_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	/* pass thru to read */ 
	return (VOP_READ(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred));
}

/* Pass-through pageout for HFS filesystem */
int
hfs_pageout(ap)
	struct vop_pageout_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	/* pass thru to write */ 
	return (VOP_WRITE(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred));
}



/*
	Try to expand the range of a buffer.

    Possible values for 'flags' are:

        RELEASE_BUFFER - to specify that the expanded buffer need not be held
	
	Calling bexpand with nbpp == &bp to expand a given buffer is OK;
	even on errors, the current buffer (bp) is always brelse-ed!
 */
int
bexpand(struct buf *bp, int newsize, struct buf **nbpp, long flags)
{
	struct vnode *ovp;
	daddr_t olblkno;
	struct ucred *ocred;
	int odirtyend;
	int releasebp = 1;
	struct buf *tbp = NULL;
    struct buf *nbp = NULL;
	int retval = 0;
	
    DBG_ASSERT(bp != NULL);
    if (nbpp == NULL) DBG_ASSERT(flags & RELEASE_BUFFER);

	if (bp->b_validend >= newsize) {
        /* Sufficient amount already read or written into buffer: */
        nbp = bp;								/* nbp is checked against bp in exit path */
		retval = 0;
		goto stdexit;
	};
	
	if (nbpp) *nbpp = NULL;						/* Just to be clean, and to ensure != bp... */

	ovp = bp->b_vp;
	olblkno = bp->b_lblkno;
	ocred = bp->b_rcred;
	odirtyend = bp->b_dirtyend;
	
	if (bp->b_flags & B_DELWRI) {
		/* This buffer holds dirtied data that must be preserved: */
		/* XXX PPD 9/28/98 Should assert that b_dirtyoff == 0 here, but not this late in the game... */
		tbp = geteblk(odirtyend);				/* Grab a new [temporary] buffer big enough to
												   hold the dirty parts of this buffer for a sec. */
		if (tbp == NULL) {
			retval = ENOMEM;
			goto errexit;
		};
		bcopy(bp->b_data, tbp->b_data, odirtyend);
	};
	
	/* It's now safe to trash the entire current contents of the buffer */
	bp->b_flags |= B_INVAL;
    DBG_ASSERT((bp->b_validend == 0) || (bp->b_validend == bp->b_bcount));
	brelse(bp);
	releasebp = 0;

//  if (((flags & RELEASE_BUFFER) == 0) || (tbp != NULL)) {
    if (1) {
        retval = bread(ovp, olblkno, newsize, ocred, &nbp);	/* Read in the requested data */
        if (retval != 0) goto errexit;

        nbp->b_validoff = 0;
        nbp->b_validend = newsize;
        nbp->b_dirtyoff = 0;
        nbp->b_dirtyend = 0;

        if (tbp) {
            /* Restore the modified data from the old buffer: */
            bcopy(tbp->b_data, nbp->b_data, odirtyend);
            nbp->b_dirtyend = odirtyend;
            DBG_ASSERT((bp->b_validend == 0) || (bp->b_validend == bp->b_bcount));
            bdwrite(nbp);										/* Mark this block dirty and move to appropriate buffer */

            if (flags & RELEASE_BUFFER) {
                nbp = NULL;										/* Forget about the new buffer just written */
            } else {
                /* Get the buffer back on behalf of our caller, even reading it back in in the
                   unlikely case it's been flushed and re-used since the bdwrite(), above: */
                retval = bread(ovp, olblkno, newsize, ocred, nbpp);
                if (retval != 0) goto errexit;
                nbp->b_validoff = 0;
                nbp->b_validend = newsize;
                nbp->b_dirtyoff = 0;
                nbp->b_dirtyend = odirtyend;
            };

            brelse(tbp);
            tbp = NULL;											/* Just in case we hit errors later */
        };
    };
	goto stdexit;

errexit:
	if (tbp) brelse(tbp);

    if (releasebp && (bp != nbp)) {
        DBG_ASSERT((bp->b_validend == 0) || (bp->b_validend == bp->b_bcount));
        brelse(bp);
    };

stdexit:;
    if (nbpp) *nbpp = nbp;

    if ((flags & RELEASE_BUFFER) && (nbp != NULL)) {
        brelse(nbp);
    };

#if DIAGNOSTIC
    debug_check_blocksizes(bp->b_vp);
#endif

	return retval;
}
