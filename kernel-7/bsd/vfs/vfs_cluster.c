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

/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
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
 *	@(#)vfs_cluster.c	8.10 (Berkeley) 3/28/95
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/trace.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <libkern/libkern.h>
#include <kern/mapfs.h>

#include <kern/kdebug.h>

/*
 * Local declarations
 */
struct buf *cluster_rbuild __P((struct vnode *, u_quad_t, struct buf *,
	    daddr_t, daddr_t, long, int, long, long));
struct buf *cluster_create __P((struct vnode *, struct buf *, daddr_t, daddr_t, long,
	    int, long, daddr_t *, int));
int         cluster_block __P((struct vnode *, u_quad_t, struct buf *, long, long));
void	    cluster_wbuild __P((struct vnode *, struct buf *, long,
	    daddr_t, int, daddr_t, long, int));
struct cluster_save *cluster_collectbufs __P((struct vnode *, struct buf *));

#if DIAGNOSTIC
/*
 * Set to 1 if reads of block zero should cause readahead to be done.
 * Set to 0 treats a read of block zero as a non-sequential read.
 *
 * Setting to one assumes that most reads of block zero of files are due to
 * sequential passes over the files (e.g. cat, sum) where additional blocks
 * will soon be needed.  Setting to zero assumes that the majority are
 * surgical strikes to get particular info (e.g. size, file) where readahead
 * blocks will not be used and, in fact, push out other potentially useful
 * blocks from the cache.  The former seems intuitive, but some quick tests
 * showed that the latter performed better from a system-wide point of view.
 */
int	doclusterraz = 0;
#define ISSEQREAD(vp, blk) \
	(((blk) != 0 || doclusterraz) && \
	 ((blk) == (vp)->v_lastr + 1 || (blk) == (vp)->v_lastr))
#else
#define ISSEQREAD(vp, blk) \
	((blk) != 0 && ((blk) == (vp)->v_lastr + 1 || (blk) == (vp)->v_lastr))
#endif

/*
 * This replaces bread.  If this is a bread at the beginning of a file and
 * lastr is 0, we assume this is the first read and we'll read up to two
 * blocks if they are sequential.  After that, we'll do regular read ahead
 * in clustered chunks.
 *
 * There are 4 or 5 cases depending on how you count:
 *	Desired block is in the cache:
 *	    1 Not sequential access (0 I/Os).
 *	    2 Access is sequential, do read-ahead (1 ASYNC).
 *	Desired block is not in cache:
 *	    3 Not sequential access (1 SYNC).
 *	    4 Sequential access, next block is contiguous (1 SYNC).
 *	    5 Sequential access, next block is not contiguous (1 SYNC, 1 ASYNC)
 *
 * There are potentially two buffers that require I/O.
 * 	bp is the block requested.
 *	rbp is the read-ahead block.
 *	If either is NULL, then you don't have to do the I/O.
 */

cluster_read(vp, filesize, lblkno, size, cred, bpp, secsize, 
			 firstpass, resid, fp_sequential)
	struct vnode *vp;
	u_quad_t filesize;
	daddr_t lblkno;
	long size;
	struct ucred *cred;
	struct buf **bpp;
	long secsize;
	int firstpass;
	long resid;
	int *fp_sequential;
{
	struct buf *bp, *rbp, *cbp;
	daddr_t blkno, ioblkno;
	long flags;
	int error, num_ra, alreadyincore;
	long num;
	int sequential, case4;

#if DIAGNOSTIC
	if (size == 0)
		panic("cluster_read: size = 0");
#endif

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_START,
		                           lblkno,
		                           resid,
		                           firstpass,
		                           vp,
		                           0);
	error = 0;
	flags = B_READ;
	*bpp = bp = getblk(vp, lblkno, size, 0, 0);

	if (resid == PAGE_SIZE && lblkno && !ISSEQREAD(vp, lblkno) &&
	    (vp->v_mount->mnt_stat.f_iosize & (PAGE_SIZE - 1)) == 0) {
	        if (bp->b_flags & B_CACHE) {

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_END,
	                           lblkno,
	                           size,
	                           -1,
	                           0,
	                           0);

			vp->v_consumed += (bp->b_bcount/size);
			return (0);
		}
		bp->b_flags |= B_READ;

		if (cluster_block(vp, filesize, bp, size, secsize)) {

		        error = biowait(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_END,
	                           bp,
	                           0,
	                           0,
	                           0,
	                           0);

			return(error);
		}
	}
	/* round up resid count to nearest block size */
	if ( resid  > size )
	        resid += size - 1;

	if (bp->b_flags & B_CACHE) {
		/*
		 * Desired block is in cache; do any readahead ASYNC.
		 * Case 1, 2.
		 */
		trace(TR_BREADHIT, pack(vp, size), lblkno);
		flags |= B_ASYNC;
		if (resid > size)
			resid -= size;

		if (!fp_sequential)
		{
			ioblkno = lblkno + 1;
			alreadyincore = incore(vp, ioblkno) != NULL;
		}
		else
		{
			ioblkno = lblkno + (vp->v_ralen ? vp->v_ralen : 1);
			alreadyincore = incore(vp, ioblkno) != NULL;
		}
		/*
		 * treat this as a hit for purposes of speculative I/O around paging activity
		 */
		vp->v_consumed += (bp->b_bcount/size);

		bp = NULL;
	} else {
		/* Block wasn't in cache, case 3, 4, 5. */
		trace(TR_BREADMISS, pack(vp, size), lblkno);
		bp->b_flags |= B_READ;
		ioblkno = lblkno;
		alreadyincore = 0;
		current_proc()->p_stats->p_ru.ru_inblock++;		/* XXX */
	}
	/*
	 * XXX
	 * Replace 1 with a window size based on some permutation of
	 * maxcontig and rot_delay.  This will let you figure out how
	 * many blocks you should read-ahead (case 2, 4, 5).
	 *
	 * If the access isn't sequential, reset the window to 1.
	 * Note that a read to the same block is considered sequential.
	 * This catches the case where the file is being read sequentially,
	 * but at smaller than the filesystem block size.
	 */
	rbp = NULL;
	cbp = NULL;
	case4 = 0;

	if (!ISSEQREAD(vp, lblkno)) {
		vp->v_ralen = 0;
		vp->v_maxra = lblkno;
		sequential = 0;
	}
	else
	        sequential = 1;

	/* On first pass set the sequential state.
	 * Otherwise, just use the value passed in.
	 */
	if (firstpass)
	        *fp_sequential = sequential;

	if (resid > size || *fp_sequential) {
	  if (((u_quad_t)(ioblkno + 1)) * (u_quad_t)size <= filesize && !alreadyincore &&
	    !(error = VOP_BMAP(vp, ioblkno, NULL, &blkno, &num_ra)) &&
	    blkno != -1) {
		/*
		 * Reading sequentially, and the next block is not in the
		 * cache.  We are going to try reading ahead.
		 */
		if (num_ra) {
			/*
			 * If our desired readahead block had been read
			 * in a previous readahead but is no longer in
			 * core, then we may be reading ahead too far
			 * or are not using our readahead very rapidly.
			 * In this case we scale back the window.
			 */
		        if (*fp_sequential) {
			        if (!alreadyincore && ioblkno <= vp->v_maxra)
				        vp->v_ralen = max(vp->v_ralen >> 1, 1);
				/*
				 * There are more sequential blocks than our current
				 * window allows, scale up.  Ideally we want to get
				 * in sync with the filesystem maxcontig value.
				 */
				else if (num_ra > vp->v_ralen && lblkno != vp->v_lastr)
				        vp->v_ralen = vp->v_ralen ?
					min(num_ra, vp->v_ralen << 1) : 1;
			}
			num = max((resid/size)-1, vp->v_ralen);
			num_ra = min(num, num_ra);
		}

		if (num_ra) {				/* case 2, 4 */
			cbp = cluster_rbuild(vp, filesize,
					     bp, ioblkno, blkno, size, num_ra, flags, secsize);

			if ( !(cbp->b_flags & B_CALL)) {
			        if ((rbp = cbp) == bp)
				        rbp = NULL;
				cbp = NULL;
			} else
			        case4 = 1;
		} else if (ioblkno == lblkno) {
			bp->b_blkno = blkno;
			/* Case 5: check how many blocks to read ahead */
			++ioblkno;
			if (((u_quad_t)(ioblkno + 1)) * (u_quad_t)size > filesize ||
			    incore(vp, ioblkno) || (error = VOP_BMAP(vp,
			     ioblkno, NULL, &blkno, &num_ra)) || blkno == -1)
				goto skip_readahead;
			/*
			 * Adjust readahead as above.
			 * Don't check alreadyincore, we know it is 0 from
			 * the previous conditional.
			 */
			if (num_ra) {
			  if (*fp_sequential) {
				if (ioblkno <= vp->v_maxra)
					vp->v_ralen = max(vp->v_ralen >> 1, 1);
				else if (num_ra > vp->v_ralen &&
					 lblkno != vp->v_lastr)
					vp->v_ralen = vp->v_ralen ?
						min(num_ra,vp->v_ralen<<1) : 1;
			  }
			  num = max((resid/size)-1, vp->v_ralen);
			  num_ra = min(num, num_ra);
			}
			flags |= B_ASYNC;

			if (num_ra) {
				cbp = cluster_rbuild(vp, filesize,
				    NULL, ioblkno, blkno, size, num_ra, flags,
						     secsize);
				if ( !(cbp->b_flags & B_CALL)) {
				        rbp = cbp;
					cbp = NULL;
				}
			} else {
				rbp = getblk(vp, ioblkno, size, 0, 0);
				rbp->b_flags |= flags;
				rbp->b_blkno = blkno;
			}
		} else {
			/* case 2; read ahead single block */
			rbp = getblk(vp, ioblkno, size, 0, 0);
			rbp->b_flags |= flags;
			rbp->b_blkno = blkno;
		}
		if (cbp || rbp) {			/* case 2, 5 */
			trace(TR_BREADMISSRA,
			    pack(vp, (num_ra + 1) * size), ioblkno);
			current_proc()->p_stats->p_ru.ru_inblock++;	/* XXX */
		}
	  }
	}

skip_readahead:
	if (bp && !case4) {
		if (bp->b_flags & (B_DONE | B_DELWRI))
			panic("cluster_read: DONE bp");
		else {
		        /*
			 * issue the BMAP here if needed due to the block device's
			 * lack of a BMAP call in the strategy routine.... when being
			 * used by the filesystem/mount code, the blockno's being worked
			 * with are always physical so the strategy routine doesn't bother.
			 * Now that we are calling cluster read/write from spec_read/spec_write
			 * we have to use real logical blockno's in order to properly trigger
			 * the read-ahead and write-coalescing.
			 */
		        if (bp->b_lblkno == bp->b_blkno) {
			        VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
				
				if ((long)bp->b_blkno == -1)
				        clrbuf(bp);
			}
			error = VOP_STRATEGY(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_NONE,
		                           bp->b_lblkno,
		                           bp->b_bufsize,
		                           bp->b_bcount,
		                           1, 0 );
		}
	}
	if (rbp) {
		if (error || rbp->b_flags & (B_DONE | B_DELWRI)) {
			rbp->b_flags &= ~(B_ASYNC | B_READ);
			brelse(rbp);
		} else {
		        /*
			 * issue the BMAP here if needed due to the block device's
			 * lack of a BMAP call in the strategy routine.... when being
			 * used by the filesystem/mount code, the blockno's being worked
			 * with are always physical so the strategy routine doesn't bother.
			 * Now that we are calling cluster read/write from spec_read/spec_write
			 * we have to use real logical blockno's in order to properly trigger
			 * the read-ahead and write-coalescing.
			 */
		        if (rbp->b_lblkno == rbp->b_blkno) {
			        VOP_BMAP(vp, rbp->b_lblkno, NULL, &rbp->b_blkno, NULL);

				if ((long)rbp->b_blkno == -1)
				        clrbuf(rbp);
			}
			(void) VOP_STRATEGY(rbp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_NONE,
		                           rbp->b_lblkno,
		                           rbp->b_bufsize,
		                           rbp->b_bcount,
		                           2, 0 );
		}
	}
	if (cbp) {
		(void) VOP_STRATEGY(cbp);

		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_NONE,
		                           cbp->b_lblkno,
		                           cbp->b_bufsize,
		                           cbp->b_bcount,
		                           3, 0 );
	}
	/*
	 * Recalculate our maximum readahead
	 */
	if (rbp == NULL) {
	        if (cbp)
		        rbp = cbp;
	        else
		        rbp = bp;
	}
	if (rbp)
		vp->v_maxra = rbp->b_lblkno + (rbp->b_bcount / size) - 1;

	if (bp)
		error = biowait(bp);

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 11)) | DBG_FUNC_END,
		                           bp,
		                           rbp,
		                           cbp,
		                           vp->v_maxra,
		                           0);
	return(error);
}

struct pent {
  int mask;
  int num;
} pent[7] = {
  {0,0},
  {0,0},
  {~0,1},
  {~1,2},
  {~3,4},
  {~7,8},
  {~15,16}};


int cluster_block(vp, filesize, bp, size, secsize)
	struct vnode *vp;
	u_quad_t filesize;
	struct buf *bp;
	long size;
	long secsize;
{
	struct buf *cbp;
	daddr_t lblkno, blkno, ioblkno, lbn;
	int num_io, num;
	unsigned ratio;

#if 0 /* FIXED READS */
	/* calculate maximum number of blocks to read in */

	lblkno = bp->b_lblkno & ~0x07;     /* put us on a 32k (8 page boundary) boundary */
	num    = 8;
	num_io = 0;
#else /* ADAPTIVE READS */
	if (vp->v_bread > vp->v_trigger) {
	        ratio = (vp->v_consumed*100) / vp->v_bread;

		if (ratio < 50 && vp->v_power > 2) {
		        vp->v_power--;
			vp->v_trigger = vp->v_bread + (16 * pent[vp->v_power].num);
		} else if (ratio > 75 && vp->v_power < 6) {
		        vp->v_power++;
			vp->v_trigger = vp->v_bread + (16 * pent[vp->v_power].num);
		}
	}
	if ((num = pent[vp->v_power].num) == 1)
	        return (0);
	lblkno = bp->b_lblkno & pent[vp->v_power].mask;
	num_io = 0;
#endif

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 14)) | DBG_FUNC_START,
		                           lblkno,
		                           num,
		                           vp->v_flag,
		                           vp,
		                           0 );

	for (lbn = bp->b_lblkno; lbn > lblkno; lbn--) {
	        if (incore(vp, lbn - 1))
		        break;
	}
	num -= (lbn - lblkno);

	for (;;) {
	        if (VOP_BMAP(vp, lbn, NULL, &blkno, &num_io) || blkno == -1 || num_io == 0) {
		        if (lbn == bp->b_lblkno) {
			        KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 14)) | DBG_FUNC_END,
					     -1,
					     lbn,
					     blkno,
					     num_io,
					     0);
				return (0);
			}
		}
		if ((lbn + num_io) >= bp->b_lblkno)
		        break;
		lbn++;
		num--;
	}
	if ((num_io = min(num, num_io + 1)) == 1)
	        return (0);

	if ((u_quad_t)size * ((u_quad_t)(lbn + num_io)) > filesize)
	        num_io = (filesize - ((u_quad_t)size * (u_quad_t)lbn)) / size;

	cbp = cluster_create(vp, bp, lbn, blkno, size, num_io, secsize, &ioblkno, B_AGE);

	if (cbp) {
		(void) VOP_STRATEGY(cbp);
		vp->v_bread += (cbp->b_bcount / size);
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 14)) | DBG_FUNC_END,
			     cbp->b_lblkno,
			     cbp->b_blkno,
			     cbp->b_bcount,
			     0,
			     0 );

		return (1);
	}
	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 14)) | DBG_FUNC_END,
		     0,
		     0,
		     0,
		     0,
		     0);
	return (0);
}


/*
 * generate advisory I/O in as big of chunks as possible
 * and then parcel them up into logical blocks in the buffer hash table.
 */
advisory_read(vp, filesize, lblkno, size, runt_size, io_size, secsize)
	struct vnode *vp;
	u_quad_t filesize;
	daddr_t lblkno;
	long size;
	long runt_size;
	long io_size;
	long secsize;
{
	struct buf *bp, *cbp;
	daddr_t blkno, ioblkno;
	int error, num_io;
	long num;

	error = 0;

	/* calculate maximum number of blocks to read in */

	num = (io_size + (size - 1)) / size;

	if ((u_quad_t)size * ((u_quad_t)(lblkno + num)) > filesize) {
	        if (((u_quad_t)size * (u_quad_t)lblkno) >= filesize)
		        return(EFBIG);
	        io_size = filesize - ((u_quad_t)size * (u_quad_t)lblkno);

		num = io_size / size;
	} else
	        io_size = num * size;

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 13)) | DBG_FUNC_START,
		                           lblkno,
		                           io_size,
		                           num,
		                           vp,
		                           0 );

	while (num) {
	        if (error = VOP_BMAP(vp, lblkno, NULL, &blkno, &num_io))
		        break;
		    
		if (blkno == -1) {
			lblkno++;
			num--;
			io_size -= size;
			continue;
		}
		num_io = min(num, num_io + 1);

		cbp = cluster_create(vp, NULL, lblkno, blkno, size, num_io, secsize, &ioblkno, 0);

		if (cbp) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 13)) | DBG_FUNC_NONE,
				     cbp->b_lblkno,
				     cbp->b_blkno,
				     cbp->b_bcount,
				     3,
				     0 );

			(void) VOP_STRATEGY(cbp);
		} else {
		        if (ioblkno == lblkno) {
			        error = ENOMEM;
				break;
			}
		}
		io_size -= ((ioblkno - lblkno) * size);
		num -= ioblkno - lblkno;
		lblkno = ioblkno;
	}
	if (io_size && !error) {
	        bp = getblk(vp, lblkno, runt_size, 0, 0);

		if (bp->b_flags & (B_DONE | B_DELWRI))
		        brelse(bp);
		else {
		        bp->b_flags |= (B_READ | B_ASYNC);

			(void) VOP_STRATEGY(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 13)) | DBG_FUNC_NONE,
	                           bp->b_lblkno,
	                           bp->b_blkno,
	                           bp->b_bcount,
	                           4,
	                           0 );
		}
		io_size -= runt_size;
	}

	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 13)) | DBG_FUNC_END,
		                           lblkno,
		                           io_size,
		                           num,
		                           error,
		                           0);
	return(error);
}


/*
 * If blocks are contiguous on disk, use this to provide clustered
 * read ahead.  We will read as many blocks as possible sequentially
 * and then parcel them up into logical blocks in the buffer hash table.
 */
struct buf *
cluster_rbuild(vp, filesize, bp, lbn, blkno, size, run, flags, secsize)
	struct vnode *vp;
	u_quad_t filesize;
	struct buf *bp;
	daddr_t lbn;
	daddr_t blkno;
	long size;
	int run;
	long flags;
	long secsize;
{
	struct cluster_save *b_save;
	struct buf *tbp, *cbp;
	caddr_t cp;
	daddr_t bn;
	int i, inc;

#if DIAGNOSTIC
	if (size != vp->v_mount->mnt_stat.f_iosize)
		panic("cluster_rbuild: size %d != filesize %d\n",
			size, vp->v_mount->mnt_stat.f_iosize);
#endif
	if ((u_quad_t)size * ((u_quad_t)(lbn + run + 1)) > filesize)
		--run;
	if (run == 0) {
		if (!bp) {
			bp = getblk(vp, lbn, size, 0, 0);
			bp->b_blkno = blkno;
			bp->b_flags |= flags;
		}
		return(bp);
	}	
	b_save = _MALLOC(sizeof(struct buf *) * (run + 1) + sizeof(struct cluster_save),
	    M_SEGMENT, M_NOWAIT);

	if (b_save)
	        cbp = alloc_io_buf(vp);
	else
	        cbp = NULL;

	if (b_save == NULL || cbp == NULL) {
	        if (b_save)
		        _FREE(b_save, M_SEGMENT);
		if (cbp)
		        free_io_buf(cbp);
	        return (bp);
	}
	b_save->bs_bufsize = size;
	b_save->bs_nchildren = 0;
	b_save->bs_children = (struct buf **)(b_save + 1);

	cbp->b_saveaddr = (caddr_t)b_save;
	cbp->b_iodone = cluster_callback;
	cbp->b_blkno = blkno;
	cbp->b_lblkno = lbn;
	cbp->b_flags |= flags | B_CALL;

	inc = btodb(size, secsize);
	cp = (char *)cbp->b_data;
	tbp = bp;

	for (bn = blkno, i = 0; i <= run; ++i, bn += inc) {
	        if (tbp == NULL) {
		        if (incore(vp, lbn + i))
			        /*
				 * A component of the cluster is already in core,
				 * terminate the cluster early.
				 */
			        break;
		        tbp = getblk(vp, lbn + i, size, 0, 0);
		}
		pagemove(tbp->b_data, cp, size);
		cbp->b_bcount += size;
		cbp->b_bufsize += size;
		cp += size;

		if (bp != tbp)
		        tbp->b_flags |= flags | B_READ | B_ASYNC;
		tbp->b_bufsize -= size;
		tbp->b_blkno = bn;

		b_save->bs_children[i] = tbp;
		b_save->bs_nchildren++;

		tbp = NULL;
	}
	/*
	 * The cluster may have been terminated early
	 * If no cluster could be formed, deallocate the cluster save info.
	 */
	if (i == 0) {
		_FREE(b_save, M_SEGMENT);
		free_io_buf(cbp);
		return(bp);
	}
	return(cbp);
}



struct buf *
cluster_create(vp, bp, lbn, blkno, size, run, secsize, ioblkno, flags)
	struct vnode *vp;
	struct buf *bp;
	daddr_t lbn;
	daddr_t blkno;
	long size;
	int run;
	long secsize;
	daddr_t *ioblkno;
	int flags;
{
	struct cluster_save *b_save;
	struct buf *tbp, *cbp;
	caddr_t cp;
	daddr_t bn;
	int i, inc;

	inc = btodb(size, secsize);

	if (bp == NULL) {
	        while (run && (tbp = incore(vp, lbn))) {
		        /*
			 * if a block is already in core
			 * and is not busy
			 * then get and release to freshen it in the LRU
			 */
		        if ( !(tbp->b_flags & B_BUSY)) {
			        tbp = getblk(vp, lbn, size, 0, 0);
				brelse(tbp);
			}
			lbn++;
			run--;
			blkno += inc;
		}
		if (run == 0) {
		        *ioblkno = lbn;
			return (NULL);
		}
	}
	b_save = _MALLOC((sizeof(struct buf *) * run) + sizeof(struct cluster_save), M_SEGMENT, M_NOWAIT);

	if (b_save)
	        cbp = alloc_io_buf(vp);
	else
	        cbp = NULL;

	if (b_save == NULL || cbp == NULL) {
	        if (b_save)
		        _FREE(b_save, M_SEGMENT);
		if (cbp)
		        free_io_buf(cbp);
		*ioblkno = lbn;
		
	        return (NULL);
	}
	b_save->bs_bufsize = size;
	b_save->bs_nchildren = 0;
	b_save->bs_children = (struct buf **)(b_save + 1);

	cbp->b_saveaddr = (caddr_t)b_save;
	cbp->b_iodone = cluster_callback;
	cbp->b_blkno = blkno;
	cbp->b_lblkno = lbn;
	cbp->b_flags |= (B_READ | B_ASYNC | B_CALL);

	cp = (char *)cbp->b_data;

	for (bn = blkno, i = 0; i < run; ++i, bn += inc, ++lbn) {
	        if (bp && bp->b_lblkno == lbn)
		        tbp = bp;
		else {
		        if (tbp = incore(vp, lbn)) {
			        /*
				 * A component of the cluster is already in core,
				 * terminate the cluster early.
				 * if its not busy then also
				 * get and release to freshen it in the LRU
				 */
			        if ( !(tbp->b_flags & B_BUSY)) {
				        tbp = getblk(vp, lbn, size, 0, 0);
					brelse(tbp);
				}
				break;
			}
			tbp = getblk(vp, lbn, size, 0, 0);
		}
		pagemove(tbp->b_data, cp, size);

		tbp->b_bufsize -= size;
		tbp->b_blkno = bn;
		cbp->b_bcount += size;
		cbp->b_bufsize += size;
		cp += size;

		if (tbp != bp)
		        tbp->b_flags |= (B_READ | B_ASYNC | flags);
		b_save->bs_children[i] = tbp;
		b_save->bs_nchildren++;
	}
	*ioblkno = lbn;
	/*
	 * The cluster may have been terminated early
	 * If no cluster could be formed, deallocate the cluster save info.
	 */
	if (cbp->b_bcount == 0) {
		_FREE(b_save, M_SEGMENT);
		free_io_buf(cbp);
		return(NULL);
	}
	return(cbp);
}


/*
 * Cleanup after a clustered read or write.
 * This is complicated by the fact that any of the buffers might have
 * extra memory (if there were no empty buffer headers at allocbuf time)
 * that we will need to shift around.
 */
void
cluster_callback(bp)
	struct buf *bp;
{
	struct cluster_save *b_save;
	struct buf **bpp, *tbp;
	long bsize;
	int  xsize;
	int  n;
	caddr_t cp;
	int error = 0;

	/*
	 * Must propogate errors to all the components.
	 */
	if (bp->b_flags & B_ERROR)
		error = bp->b_error;
	b_save = (struct cluster_save *)(bp->b_saveaddr);

	bsize = b_save->bs_bufsize;
	xsize = bp->b_bcount - bp->b_resid;
	cp = (char *)bp->b_data;
	/*
	 * Move memory from the large cluster buffer into the component
	 * buffers and mark IO as done on these.
	 */
	for (bpp = b_save->bs_children; b_save->bs_nchildren--; ++bpp) {
		tbp = *bpp;
		pagemove(cp, tbp->b_data, bsize);
		tbp->b_bufsize += bsize;

		n = min(bsize, xsize);
		xsize -= n;

		if ((tbp->b_bcount = n) == 0)
		        tbp->b_flags |= B_INVAL;
		tbp->b_resid = bsize - n;

		if (error) {
			tbp->b_flags |= B_ERROR;
			tbp->b_error = error;
		}
		biodone(tbp);
		bp->b_bufsize -= bsize;
		cp += bsize;
	}
	_FREE(b_save, M_SEGMENT);

	free_io_buf(bp);
}


/*
 * on close, flush out any remaining cluster
 *
 */
cluster_close(vp, bsize, secsize)
        struct vnode *vp;
	int  bsize;
	long secsize;
{
        int cursize;

	if (vp->v_clen) {
	        cursize = vp->v_lastw - vp->v_cstart + 1;

		cluster_wbuild(vp, NULL, bsize, vp->v_cstart, cursize, -1, secsize, 0);

		vp->v_lasta = vp->v_clen = vp->v_cstart = vp->v_lastw = 0;
	}
}


/*
 * Do clustered write for FFS.
 *
 * Three cases:
 *	1. Write is not sequential (write asynchronously)
 *	Write is sequential:
 *	2.	beginning of cluster - begin cluster
 *	3.	middle of a cluster - add to cluster
 *	4.	end of a cluster - asynchronously write cluster
 */

cluster_write(bp, filesize, secsize)
        struct buf *bp;
	u_quad_t filesize;
	long secsize;
{
        struct vnode *vp;
        daddr_t lbn;
        daddr_t bn;
        int maxclen, cursize;
	int need_commit;
	int need_sync;
	int bsize;
	int error = 0;

	need_commit = (bp->b_flags & B_CLUST_COMMIT);
	need_sync   = (bp->b_flags & B_CLUST_SYNC);
	bp->b_flags &= ~(B_CLUST_COMMIT | B_CLUST_SYNC);
	
        vp = bp->b_vp;
	bn = bp->b_blkno;
        lbn = bp->b_lblkno;
	bsize = bp->b_bcount;

	if ((bsize & (PAGE_SIZE - 1)) || bsize > MAXBSIZE) {
	        bp->b_flags |= B_AGE;
		bawrite(bp);

		return (error);
	}
	/* Initialize vnode to beginning of file. */
	if (lbn == 0)
		vp->v_lasta = vp->v_clen = vp->v_cstart = vp->v_lastw = 0;


	KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_START,
		                           bp->b_lblkno,
		                           bp->b_bcount,
		                           vp->v_lasta,
		                           vp->v_clen,
		                           0);
       
        if (vp->v_clen == 0 || lbn != vp->v_lastw + 1 ||
	    (bn != vp->v_lasta + btodb(bsize, secsize)))
	{
		maxclen = (MAXPHYSIO / bsize) - 1;

		if (vp->v_clen != 0) {
			/*
			 * Next block is not sequential.
			 *
			 * If we are not writing at end of file, the process
			 * seeked to another point in the file since its
			 * last write, or we have reached our maximum
			 * cluster size, then push the previous cluster.
			 * Otherwise try reallocating to make it sequential.
			 */
			cursize = vp->v_lastw - vp->v_cstart + 1;
			if (((u_quad_t)(lbn + 1)) * (u_quad_t)bsize != filesize ||
			    lbn != vp->v_lastw + 1 || vp->v_clen <= cursize) {
				cluster_wbuild(vp, NULL, bsize,
				    vp->v_cstart, cursize, lbn, secsize, need_sync);
			} else {
				struct buf **bpp, **endbp;
				struct cluster_save *buflist;

				buflist = cluster_collectbufs(vp, bp);

				if (buflist == NULL) {
					cluster_wbuild(vp, NULL, bp->b_bcount,
					    vp->v_cstart, cursize, lbn, secsize, need_sync);
				} else {

				        endbp = &buflist->bs_children
					        [buflist->bs_nchildren - 1];
					if (VOP_REALLOCBLKS(vp, buflist)) {
					        /*
						 * Failed, push the previous cluster.
						 */
					        for (bpp = buflist->bs_children;
						     bpp < endbp; bpp++)
						        brelse(*bpp);
						_FREE(buflist, M_SEGMENT);

						cluster_wbuild(vp, NULL, bsize,
						    vp->v_cstart, cursize, lbn, secsize, need_sync);
					} else {
					        /*
						 * Succeeded, keep building cluster.
						 * don't bdwrite the last bp, we'll 
						 * first check to see if we now have a full
						 * cluster, or the caller has requested a SYNC write
						 */
					        for (bpp = buflist->bs_children;
						     bpp < endbp; bpp++)
						        bdwrite(*bpp);
						_FREE(buflist, M_SEGMENT);
						/*
						 * update the physical block number because,
						 * VOP_REALLOCBLKS will have changed it
						 */
						bn = bp->b_blkno;
						goto chk_cluster_full;
					}
				}
			}
		}
		/*
		 * Consider beginning a cluster.
		 * If at end of file, make cluster as large as possible,
		 * otherwise find size of existing cluster.
		 */
		if (((u_quad_t)(lbn + 1)) * (u_quad_t)bsize != filesize &&
		    (VOP_BMAP(vp, lbn, NULL, &bp->b_blkno, &maxclen) ||
		     bp->b_blkno == -1)) {
		        bn = bp->b_blkno;
			vp->v_clen = 0;
			vp->v_cstart = lbn + 1;

		        if (need_sync)
			        bwrite(bp);
			else
			        bawrite(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_END,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           vp->v_cstart,
		                           1 );
		        goto check_for_commit;
		}
	        bn = bp->b_blkno;

                if ((vp->v_clen = maxclen) == 0 || need_commit) {    /* I/O not contiguous or we're being asked to do IO_SYNC and this is the last */
		        vp->v_cstart = lbn + 1;                      /* chunk of the I/O request */

		        if (need_sync)
			        bwrite(bp);
			else
			        bawrite(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_END,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           vp->v_cstart,
		                           2 );
                } else {			/* Wait for rest of cluster */
			vp->v_cstart = lbn;

                        bdwrite(bp);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_END,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           vp->v_cstart,
		                           3 );
		}
		goto check_for_commit;
	}
chk_cluster_full:
	if ((lbn == vp->v_cstart + vp->v_clen) || need_commit) {
	        /*
		 * At end of cluster, write it out.
		 */
		cluster_wbuild(vp, bp, bsize, vp->v_cstart,
			           (lbn - vp->v_cstart) + 1, lbn, secsize, need_sync);

	        KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_END,
		                           vp->v_cstart,
		                           vp->v_clen + 1,
		                           lbn,
			                   0,
		                           4 );
		vp->v_clen = 0;
		vp->v_cstart = lbn + 1;
	} else {
		/*
		 * In the middle of a cluster, so just delay the
		 * I/O for now.
		 */
		bdwrite(bp);

	        KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_END,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           vp->v_cstart,
		                           5 );
	}
check_for_commit:
	vp->v_lastw = lbn;
	vp->v_lasta = bn;

        if (need_commit) {
		bp = getblk(vp, lbn, bsize, 0, 0);

		if (bp->b_flags & B_ERROR)
		        error = (bp->b_error ? bp->b_error : EIO);
		brelse(bp);
	}
	return (error);
}


/*
 * This is an awful lot like cluster_rbuild...wish they could be combined.
 * The last lbn argument is the current block on which I/O is being
 * performed.  Check to see that it doesn't fall in the middle of
 * the current block (if last_bp == NULL).
 */
void
cluster_wbuild(vp, last_bp, size, start_lbn, len, lbn, secsize, need_sync)
	struct vnode *vp;
	struct buf *last_bp;
	long size;
	daddr_t start_lbn;
	int len;
	daddr_t	lbn;
	long secsize;
	int need_sync;
{
	struct cluster_save *b_save;
	struct buf *bp, *tbp;
	caddr_t	cp;
	int i, s;

#if DIAGNOSTIC
	if (size != vp->v_mount->mnt_stat.f_iosize)
		panic("cluster_wbuild: size %d != filesize %d\n",
			size, vp->v_mount->mnt_stat.f_iosize);
#endif
redo:
        while ((!incore(vp, start_lbn) || start_lbn == lbn) && len) {
	        ++start_lbn;
		--len;
	}
	/* Get more memory for current buffer */
	if (len <= 1) {
		if (last_bp) {
			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
		                           last_bp->b_lblkno,
		                           last_bp->b_blkno,
		                           last_bp->b_bcount,
		                           1,
				           0 );
			if (need_sync)
			        bwrite(last_bp);
			else
			        bawrite(last_bp);
		} else if (len) {
			bp = getblk(vp, start_lbn, size, 0, 0);

			KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           2,
		                           0 );
			if (bp->b_flags & B_DELWRI) {
			        if (need_sync)
				        bwrite(bp);
				else
				        bawrite(bp);
			} else
			        brelse(bp);
		}
		return;
	}
	b_save = _MALLOC(sizeof(struct buf *) * len + sizeof(struct cluster_save),
			 M_SEGMENT, M_NOWAIT);
	if (b_save)
	        bp = alloc_io_buf(vp);
	else
	        bp = NULL;

	if (b_save == NULL || bp == NULL) {
		KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
		                           bp,
		                           b_save,
		                           0,
		                           4,
		                           0 );
		if (bp)
		        free_io_buf(bp);
		if (b_save)
		        _FREE(b_save, M_SEGMENT);

	        for (i = 0; i < len; ++i, ++start_lbn) {
		        if (!incore(vp, start_lbn))
			        continue;
			if (last_bp == NULL || start_lbn != lbn) {
			        tbp = getblk(vp, start_lbn, size, 0, 0);

				if (tbp->b_flags & B_DELWRI) {
					KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
						     tbp->b_lblkno,
						     tbp->b_blkno,
						     tbp->b_bcount,
						     5,
						     0 );

					if (need_sync)
					        bwrite(tbp);
					else
					        bawrite(tbp);
				} else
				        brelse(tbp);
			}
		}
		if (last_bp) {
		        KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
		                           last_bp->b_lblkno,
		                           last_bp->b_blkno,
		                           last_bp->b_bcount,
		                           6,
		                           0 );
		        if (need_sync)
			        bwrite(last_bp);
			else
			        bawrite(last_bp);
		}
		return;
	}
	b_save->bs_bufsize = size;
	b_save->bs_nchildren = 0;
	b_save->bs_children = (struct buf **)(b_save + 1);

	bp->b_saveaddr = (caddr_t)b_save;
	bp->b_iodone = cluster_callback;
        bp->b_flags |= (B_WRITEINPROG | B_CALL | B_ASYNC);

	cp = (char *)bp->b_data;

	for (start_lbn, i = 0; i < len; ++i, ++start_lbn) {
		/*
		 * Block is not in core or the non-sequential block
		 * ending our cluster was part of the cluster (in which
		 * case we don't want to write it twice).
		 */
		if (!incore(vp, start_lbn) ||
		    (last_bp == NULL && start_lbn == lbn))
			break;

		/*
		 * Get the desired block buffer (unless it is the final
		 * sequential block whose buffer was passed in explictly
		 * as last_bp).
		 */
		if (last_bp == NULL || start_lbn != lbn) {
			tbp = getblk(vp, start_lbn, size, 0, 0);
			if (!(tbp->b_flags & B_DELWRI)) {
				brelse(tbp);
				break;
			}
		} else
			tbp = last_bp;

		if (i == 0) {
			bp->b_blkno = tbp->b_blkno;
			bp->b_lblkno= tbp->b_lblkno;
		} else {
		        if (tbp->b_blkno != (bp->b_blkno + btodb(bp->b_bufsize, secsize))) {
				brelse(tbp);
				break;
			}
		}
		/* Move memory from children to parent */
		pagemove(tbp->b_data, cp, size);
		bp->b_bcount += size;
		bp->b_bufsize += size;
		cp += size;

		tbp->b_bufsize -= size;
		tbp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
		tbp->b_flags |= (B_ASYNC | B_AGE);

		s = splbio();
		reassignbuf(tbp, tbp->b_vp);		/* put on clean list */
		++tbp->b_vp->v_numoutput;
		splx(s);
 
		b_save->bs_children[i] = tbp;
		b_save->bs_nchildren++;
	}

        KERNEL_DEBUG((FSDBG_CODE(DBG_FSRW, 12)) | DBG_FUNC_NONE,
		                           bp->b_lblkno,
		                           bp->b_blkno,
		                           bp->b_bcount,
		                           7,
		                           0 );
	if (i == 0) {
		/* None to cluster */
	        free_io_buf(bp);
		_FREE(b_save, M_SEGMENT);
	} else {
	        if (bp->b_bcount > MAXPHYSIO)
		        panic("cluster_wbuild: bp->b_bcount = %x\n", bp->b_bcount);

	        VOP_STRATEGY(bp);
	}
	if (i < len) {
		len -= i + 1;
		start_lbn += 1;
		goto redo;
	}
}

/*
 * Collect together all the buffers in a cluster.
 * Plus add one additional buffer.
 */
struct cluster_save *
cluster_collectbufs(vp, last_bp)
	struct vnode *vp;
	struct buf *last_bp;
{
	struct cluster_save *buflist;
	daddr_t	lbn;
	int i, j, len;

	len = vp->v_lastw - vp->v_cstart + 1;
	buflist = _MALLOC(sizeof(struct buf *) * (len + 1) + sizeof(*buflist),
	    M_SEGMENT, M_NOWAIT);

	if (buflist == NULL)
	        return (NULL);

	buflist->bs_nchildren = 0;
	buflist->bs_children = (struct buf **)(buflist + 1);
	for (lbn = vp->v_cstart, i = 0; i < len; lbn++, i++) {
		    (void)bread(vp, lbn, last_bp->b_bcount, NOCRED,
			&buflist->bs_children[i]);
		    if(!(buflist->bs_children[i]->b_flags & B_DELWRI)) {
		      for (j=0; j<=i; j++)
			brelse(buflist->bs_children[j]);
		      _FREE(buflist, M_SEGMENT);
		      return(NULL);
		    }
	}
	buflist->bs_children[i] = last_bp;
	buflist->bs_nchildren = i + 1;
	return (buflist);
}
