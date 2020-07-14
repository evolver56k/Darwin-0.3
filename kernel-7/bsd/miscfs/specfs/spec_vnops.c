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
 * Copyright (c) 1989, 1993, 1995
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
 *	@(#)spec_vnops.c	8.14 (Berkeley) 5/21/95
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#ifdef NeXT
#include <sys/malloc.h>
#include <bsd/dev/disk.h>
#else
#include <sys/disklabel.h>
#endif
#include <miscfs/specfs/specdev.h>
#include <vfs/vfs_support.h>

#ifdef NeXT
extern int doclusterread, doclusterwrite;
#endif

struct vnode *speclisth[SPECHSZ];

/* symbolic sleep message strings for devices */
char	devopn[] = "devopn";
char	devio[] = "devio";
char	devwait[] = "devwait";
char	devin[] = "devin";
char	devout[] = "devout";
char	devioc[] = "devioc";
char	devcls[] = "devcls";

int (**spec_vnodeop_p)();
struct vnodeopv_entry_desc spec_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, err_create },		/* create */
	{ &vop_mknod_desc, err_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, spec_close },		/* close */
	{ &vop_access_desc, spec_access },		/* access */
	{ &vop_getattr_desc, spec_getattr },		/* getattr */
	{ &vop_setattr_desc, spec_setattr },		/* setattr */
	{ &vop_read_desc, spec_read },			/* read */
	{ &vop_write_desc, spec_write },		/* write */
	{ &vop_lease_desc, nop_lease },		/* lease */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_select_desc, spec_select },		/* select */
	{ &vop_revoke_desc, nop_revoke },		/* revoke */
	{ &vop_mmap_desc, err_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
	{ &vop_seek_desc, err_seek },			/* seek */
	{ &vop_remove_desc, err_remove },		/* remove */
	{ &vop_link_desc, err_link },			/* link */
	{ &vop_rename_desc, err_rename },		/* rename */
	{ &vop_mkdir_desc, err_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, err_rmdir },		/* rmdir */
	{ &vop_symlink_desc, err_symlink },		/* symlink */
	{ &vop_readdir_desc, err_readdir },		/* readdir */
	{ &vop_readlink_desc, err_readlink },		/* readlink */
	{ &vop_abortop_desc, err_abortop },		/* abortop */
	{ &vop_inactive_desc, nop_inactive },		/* inactive */
	{ &vop_reclaim_desc, nop_reclaim },		/* reclaim */
	{ &vop_lock_desc, nop_lock },			/* lock */
	{ &vop_unlock_desc, nop_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, spec_print },		/* print */
	{ &vop_islocked_desc, nop_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, err_advlock },		/* advlock */
	{ &vop_blkatoff_desc, err_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, err_valloc },		/* valloc */
	{ &vop_vfree_desc, err_vfree },		/* vfree */
	{ &vop_truncate_desc, nop_truncate },		/* truncate */
	{ &vop_update_desc, nop_update },		/* update */
	{ &vop_bwrite_desc, spec_bwrite },		/* bwrite */
#ifdef NeXT
	{ &vop_devblocksize_desc, spec_devblocksize },  /* devblocksize */
#endif /* NeXT */
	{ &vop_pagein_desc, spec_pagein },		/* Pagein */
	{ &vop_pageout_desc, spec_pageout },		/* Pageout */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc spec_vnodeop_opv_desc =
	{ &spec_vnodeop_p, spec_vnodeop_entries };

/*
 * Trivial lookup routine that always fails.
 */
int
spec_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap;
{

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

#ifdef NeXT
void
set_blocksize(struct vnode *vp, dev_t dev)
{
    int (*size)();
    int rsize;

    if ((major(dev) < nblkdev) && (size = bdevsw[major(dev)].d_psize)) {
        rsize = (*size)(dev);
	if (rsize <= 0)        /* did size fail? */
	    vp->v_specsize = DEV_BSIZE;
	else
	    vp->v_specsize = rsize;
    }
    else
	    vp->v_specsize = DEV_BSIZE;
}
#endif  /* NeXT */

/*
 * Open a special file.
 */
/* ARGSUSED */
spec_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct proc *p = ap->a_p;
	struct vnode *bvp, *vp = ap->a_vp;
	dev_t bdev, dev = (dev_t)vp->v_rdev;
	int maj = major(dev);
	int error;

	/*
	 * Don't allow open if fs is mounted -nodev.
	 */
	if (vp->v_mount && (vp->v_mount->mnt_flag & MNT_NODEV))
		return (ENXIO);

	switch (vp->v_type) {

	case VCHR:
		if ((u_int)maj >= nchrdev)
			return (ENXIO);
		if (ap->a_cred != FSCRED && (ap->a_mode & FWRITE)) {
			/*
			 * When running in very secure mode, do not allow
			 * opens for writing of any disk character devices.
			 */
			if (securelevel >= 2 && isdisk(dev, VCHR))
				return (EPERM);
			/*
			 * When running in secure mode, do not allow opens
			 * for writing of /dev/mem, /dev/kmem, or character
			 * devices whose corresponding block devices are
			 * currently mounted.
			 */
			if (securelevel >= 1) {
				if ((bdev = chrtoblk(dev)) != NODEV &&
				    vfinddev(bdev, VBLK, &bvp) &&
				    bvp->v_usecount > 0 &&
				    (error = vfs_mountedon(bvp)))
					return (error);
				if (iskmemdev(dev))
					return (EPERM);
			}
		}
		if (cdevsw[maj].d_type == D_TTY)
			vp->v_flag |= VISTTY;
		VOP_UNLOCK(vp, 0, p);
		error = (*cdevsw[maj].d_open)(dev, ap->a_mode, S_IFCHR, p);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);

	case VBLK:
		if ((u_int)maj >= nblkdev)
			return (ENXIO);
		/*
		 * When running in very secure mode, do not allow
		 * opens for writing of any disk block devices.
		 */
		if (securelevel >= 2 && ap->a_cred != FSCRED &&
		    (ap->a_mode & FWRITE) && bdevsw[maj].d_type == D_DISK)
			return (EPERM);
		/*
		 * Do not allow opens of block devices that are
		 * currently mounted.
		 */
		if (error = vfs_mountedon(vp))
			return (error);
#ifdef NeXT
		error = (*bdevsw[maj].d_open)(dev, ap->a_mode, S_IFBLK, p);
		if (!error) {
		    set_blocksize(vp, dev);
		}
		return(error);
#else
		return ((*bdevsw[maj].d_open)(dev, ap->a_mode, S_IFBLK, p));
#endif /* NeXT */
	}
	return (0);
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
spec_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
 	struct proc *p = uio->uio_procp;
	struct buf *bp;
	daddr_t bn, nextbn;
	long bsize, bscale;
#ifdef NeXT
        int devBlockSize=0;
	int firstpass, seq;
#else
	struct partinfo dpart;
#endif /* NeXT */
	int n, on, majordev, (*ioctl)();
	int error = 0;
	dev_t dev;

#if DIAGNOSTIC
	if (uio->uio_rw != UIO_READ)
		panic("spec_read mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != current_proc())
		panic("spec_read proc");
#endif
	if (uio->uio_resid == 0)
		return (0);

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0, p);
		error = (*cdevsw[major(vp->v_rdev)].d_read)
			(vp->v_rdev, uio, ap->a_ioflag);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);

	case VBLK:
		if (uio->uio_offset < 0)
			return (EINVAL);
#ifdef NeXT
		bsize = PAGE_SIZE;
#else
		bsize = BLKDEV_IOSIZE;
#endif /* NeXT */

		dev = vp->v_rdev;
#ifndef NeXT
		if ((majordev = major(dev)) < nblkdev &&
		    (ioctl = bdevsw[majordev].d_ioctl) != NULL &&
		    (*ioctl)(dev, DIOCGPART, (caddr_t)&dpart, FREAD, p) == 0 &&
		    dpart.part->p_fstype == FS_BSDFFS &&
		    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0)
			bsize = dpart.part->p_frag * dpart.part->p_fsize;
#endif /* NeXT */
#ifdef NeXT /*[ */
		devBlockSize = vp->v_specsize;
	        bscale = bsize / devBlockSize;
	        firstpass = TRUE;

		do {
			on = uio->uio_offset % bsize;

		        if (doclusterread && doclusterwrite) {
			        bn = uio->uio_offset / bsize;

			        error = cluster_read(vp, (u_quad_t)0x7fffffffffffffff, bn, bsize, NOCRED,
					       &bp, devBlockSize, firstpass, uio->uio_resid+on, &seq);
			} else {
			        bn = (uio->uio_offset / devBlockSize) &~ (bscale - 1);

				if (vp->v_lastr + bscale == bn) {
				        nextbn = bn + bscale;
					error = breadn(vp, bn, (int)bsize, &nextbn,
						       (int *)&bsize, 1, NOCRED, &bp);
				} else
				        error = bread(vp, bn, (int)bsize, NOCRED, &bp);
			}
			firstpass = FALSE;

			vp->v_lastr = bn;
			n = bsize - bp->b_resid;
			if ((on > n) || error) {
			        if (!error)
				        error = EINVAL;
				brelse(bp);
				return (error);
			}
			n = min((unsigned)(n  - on), uio->uio_resid);

			error = uiomove((char *)bp->b_data + on, n, uio);
			if (n + on == bsize)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
#else /*][ */
		bscale = bsize / DEV_BSIZE;
		do {
			bn = (uio->uio_offset / DEV_BSIZE) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			if (vp->v_lastr + bscale == bn) {
				nextbn = bn + bscale;
				error = breadn(vp, bn, (int)bsize, &nextbn,
					(int *)&bsize, 1, NOCRED, &bp);
			} else
				error = bread(vp, bn, (int)bsize, NOCRED, &bp);
			vp->v_lastr = bn;
			n = min(n, bsize - bp->b_resid);
			if (error) {
				brelse(bp);
				return (error);
			}
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (n + on == bsize)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
#endif NeXT /* ] */
		return (error);

	default:
		panic("spec_read type");
	}
	/* NOTREACHED */
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
spec_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	struct buf *bp;
	daddr_t bn;
	int bsize, blkmask;
#ifdef NeXT
	register int io_sync;
	register int io_size;
	register int forcewrite;
        int devBlockSize=0;
#else
	struct partinfo dpart;
#endif
	register int n, on;
	int error = 0;
	dev_t dev;

#if DIAGNOSTIC
	if (uio->uio_rw != UIO_WRITE)
		panic("spec_write mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != current_proc())
		panic("spec_write proc");
#endif

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0, p);
		error = (*cdevsw[major(vp->v_rdev)].d_write)
			(vp->v_rdev, uio, ap->a_ioflag);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);

	case VBLK:
		if (uio->uio_resid == 0)
			return (0);
		if (uio->uio_offset < 0)
			return (EINVAL);

#ifdef NeXT
		bsize = PAGE_SIZE;
		io_sync = (ap->a_ioflag & IO_SYNC);
		io_size = uio->uio_resid;
#else
		bsize = BLKDEV_IOSIZE;
#endif /* NeXT */
		dev = (vp->v_rdev);
#ifndef NeXT
		if ((*bdevsw[major(vp->v_rdev)].d_ioctl)(vp->v_rdev, DIOCGPART,
		    (caddr_t)&dpart, FREAD, p) == 0) {
			if (dpart.part->p_fstype == FS_BSDFFS &&
			    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0)
				bsize = dpart.part->p_frag *
				    dpart.part->p_fsize;
		}
#endif /* NeXT */
#ifdef NeXT  /* [ */
		devBlockSize = vp->v_specsize;
		blkmask = (bsize / devBlockSize) - 1;

		do {
		        forcewrite = 0;

			if (doclusterread && doclusterwrite)
			        bn = uio->uio_offset / bsize;
			else
			        bn = (uio->uio_offset / devBlockSize) &~ blkmask;
			on = uio->uio_offset % bsize;

			n = min((unsigned)(bsize - on), uio->uio_resid);

			if (doclusterread && doclusterwrite) {
			        bp = getblk(vp, bn, bsize, 0, 0);
			        
				VOP_BMAP(vp, bn, NULL, &bp->b_blkno, NULL);

				if (n != bsize && !(bp->b_flags & (B_DONE|B_DELWRI))) {
				        if ((on % devBlockSize) || (n % devBlockSize)) {
					        bp->b_flags |= B_READ;
						VOP_STRATEGY(bp);
						error = biowait(bp);
					} else {
					        if (on) {
						        bp->b_blkno += (on / devBlockSize);
							on = 0;
						}
						forcewrite = 1;
					        bp->b_bcount = n;
					        bp->b_flags |= B_INVAL;
					}
				}
			} else {
			        if (n == bsize)
				        bp = getblk(vp, bn, bsize, 0, 0);
				else
				        error = bread(vp, bn, bsize, NOCRED, &bp);
			}
			if (error) {
				brelse(bp);
				return (error);
			}
			n = min(n, bsize - bp->b_resid);

			error = uiomove((char *)bp->b_data + on, n, uio);

			if (forcewrite) {
			        if (uio->uio_resid == 0)
				        bwrite(bp);
				else
				        bawrite(bp);
			} else if (((io_size >= bsize) || ((n + on) == bsize)) && doclusterread && doclusterwrite) {
			        if (io_sync) {
				        if (uio->uio_resid < bsize)
					        bp->b_flags |= (B_CLUST_SYNC | B_CLUST_COMMIT);
					else
					        bp->b_flags |= B_CLUST_SYNC;

					error = cluster_write(bp, (u_quad_t)0x7fffffffffffffff, devBlockSize);
				} else
				        cluster_write(bp, (u_quad_t)0x7fffffffffffffff, devBlockSize);
			} else {
			        bp->b_flags |= B_AGE;

			        if (io_sync) 
				        bwrite(bp);
				else {
				        if ((n + on) == bsize)
					        bawrite(bp);
					else
					        bdwrite(bp);
				}
		        }
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
#else /*][ */
		blkmask = (bsize / DEV_BSIZE) - 1;
		do {
			bn = (uio->uio_offset / DEV_BSIZE) &~ blkmask;
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			if (n == bsize)
				bp = getblk(vp, bn, bsize, 0, 0);
			else
				error = bread(vp, bn, bsize, NOCRED, &bp);
			n = min(n, bsize - bp->b_resid);
			if (error) {
				brelse(bp);
				return (error);
			}
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
#endif NeXT /* ] */
		return (error);

	default:
		panic("spec_write type");
	}
	/* NOTREACHED */
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
spec_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		int  a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	dev_t dev = ap->a_vp->v_rdev;

	switch (ap->a_vp->v_type) {

	case VCHR:
		return ((*cdevsw[major(dev)].d_ioctl)(dev, ap->a_command, ap->a_data,
		    ap->a_fflag, ap->a_p));

	case VBLK:
		if (ap->a_command == 0 && (int)ap->a_data == B_TAPE)
			if (bdevsw[major(dev)].d_type == D_TAPE)
				return (0);
			else
				return (1);
		return ((*bdevsw[major(dev)].d_ioctl)(dev, ap->a_command, ap->a_data,
		   ap->a_fflag, ap->a_p));

	default:
		panic("spec_ioctl");
		/* NOTREACHED */
	}
}

/* ARGSUSED */
spec_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register dev_t dev;

	switch (ap->a_vp->v_type) {

	default:
		return (1);		/* XXX */

	case VCHR:
		dev = ap->a_vp->v_rdev;
		return (*cdevsw[major(dev)].d_select)(dev, ap->a_which, ap->a_p);
	}
}
/*
 * Synch buffers associated with a block device
 */
/* ARGSUSED */
int
spec_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int  a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct buf *bp;
	struct buf *nbp;
	int s;

	if (vp->v_type == VCHR)
		return (0);
	/*
	 * Flush all dirty buffers associated with a block device.
	 */
loop:
	s = splbio();
	for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = nbp) {
		nbp = bp->b_vnbufs.le_next;
		if ((bp->b_flags & B_BUSY))
			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("spec_fsync: not dirty");
		bremfree(bp);
		bp->b_flags |= B_BUSY;
		splx(s);
		bawrite(bp);
		goto loop;
	}
	if (ap->a_waitfor == MNT_WAIT) {
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			tsleep((caddr_t)&vp->v_numoutput, PRIBIO + 1, "spec_fsync", 0);
		}
#if DIAGNOSTIC
		if (vp->v_dirtyblkhd.lh_first) {
			vprint("spec_fsync: dirty", vp);
			splx(s);
			goto loop;
		}
#endif
	}
	splx(s);
	return (0);
}

/*
 * Just call the device strategy routine
 */
spec_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	(*bdevsw[major(ap->a_bp->b_dev)].d_strategy)(ap->a_bp);
	return (0);
}

/*
 * This is a noop, simply returning what one has been given.
 */
spec_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap;
{

	if (ap->a_vpp != NULL)
		*ap->a_vpp = ap->a_vp;
	if (ap->a_bnp != NULL)
#ifdef NeXT
		*ap->a_bnp = ap->a_bn * (PAGE_SIZE / ap->a_vp->v_specsize);
#else
		*ap->a_bnp = ap->a_bn;
#endif
	if (ap->a_runp != NULL)
#ifdef NeXT
	        *ap->a_runp = (MAXPHYSIO / PAGE_SIZE) - 1;
#else
		*ap->a_runp = 0;
#endif
	return (0);
}

/*
 * Device close routine
 */
/* ARGSUSED */
spec_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	dev_t dev = vp->v_rdev;
	int (*devclose) __P((dev_t, int, int, struct proc *));
	int mode, error;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Hack: a tty device that is a controlling terminal
		 * has a reference from the session structure.
		 * We cannot easily tell that a character device is
		 * a controlling terminal, unless it is the closing
		 * process' controlling terminal.  In that case,
		 * if the reference count is 2 (this last descriptor
		 * plus the session), release the reference from the session.
		 */
		if (vcount(vp) == 2 && ap->a_p &&
		    vp == ap->a_p->p_session->s_ttyvp) {
			vrele(vp);
			ap->a_p->p_session->s_ttyvp = NULL;
		}
		/*
		 * If the vnode is locked, then we are in the midst
		 * of forcably closing the device, otherwise we only
		 * close on last reference.
		 */
		if (vcount(vp) > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		devclose = cdevsw[major(dev)].d_close;
		mode = S_IFCHR;
		break;

	case VBLK:
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		if (error = vinvalbuf(vp, V_SAVE, ap->a_cred, ap->a_p, 0, 0))
			return (error);
		/*
		 * We do not want to really close the device if it
		 * is still in use unless we are trying to close it
		 * forcibly. Since every use (buffer, vnode, swap, cmap)
		 * holds a reference to the vnode, and because we mark
		 * any other vnodes that alias this device, when the
		 * sum of the reference counts on all the aliased
		 * vnodes descends to one, we are on last close.
		 */
		if (vcount(vp) > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		devclose = bdevsw[major(dev)].d_close;
		mode = S_IFBLK;
		break;

	default:
		panic("spec_close: not special");
	}

	return ((*devclose)(dev, ap->a_fflag, mode, ap->a_p));
}

/*
 * Print out the contents of a special device vnode.
 */
spec_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_NON, dev %d, %d\n", major(ap->a_vp->v_rdev),
		minor(ap->a_vp->v_rdev));
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
spec_pathconf(ap)
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		int *a_retval;
	} */ *ap;
{

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

int
spec_devblocksize(ap)
        struct vop_devblocksize_args /* {
	        struct vnode *a_vp;
	        int *a_retval;
        } */ *ap;
{
        *ap->a_retval = (ap->a_vp->v_specsize);
        return (0);
}

/*
 * Special device failed operation
 */
spec_ebadf()
{

	return (EBADF);
}

/*
 * Special device bad operation
 */
spec_badop()
{

	panic("spec_badop called");
	/* NOTREACHED */
}
/* Pagein  */
spec_pagein(ap)
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

/* Pageout  */
spec_pageout(ap)
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
