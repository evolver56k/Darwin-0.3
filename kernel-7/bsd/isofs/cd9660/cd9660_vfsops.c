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

/*	$NetBSD: cd9660_vfsops.c,v 1.18 1995/03/09 12:05:36 mycroft Exp $	*/

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)cd9660_vfsops.c	8.9 (Berkeley) 12/5/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/stat.h>

#include <isofs/cd9660/iso.h>
#include <isofs/cd9660/iso_rrip.h>
#include <isofs/cd9660/cd9660_node.h>
#include <isofs/cd9660/cd9660_mount.h>

extern int enodev ();

struct vfsops cd9660_vfsops = {
	cd9660_mount,
	cd9660_start,
	cd9660_unmount,
	cd9660_root,
	cd9660_quotactl,
	cd9660_statfs,
	cd9660_sync,
	cd9660_vget,
	cd9660_fhtovp,
	cd9660_vptofh,
	cd9660_init,
	cd9660_sysctl
};

/*
 * Called by vfs_mountroot when iso is going to be mounted as root.
 *
 * Name is updated by mount(8) after booting.
 */
#define ROOTNAME	"root_device"

static int iso_mountfs __P((struct vnode *devvp, struct mount *mp,
		struct proc *p, struct iso_args *argp));
static void 	DRGetTypeCreatorAndFlags(	struct iso_mnt * theMountPointPtr,
											struct iso_directory_record * theDirRecPtr, 
											u_int32_t * theTypePtr, 
											u_int32_t * theCreatorPtr, 
											u_int16_t * theFlagsPtr );

int				cd9660_vget_internal( 	struct mount *mp, 
										ino_t ino, 
										struct vnode **vpp, 
										int relocated, 
										struct iso_directory_record *isodir, 
										struct proc *p );


int
cd9660_mountroot()
{
	register struct mount *mp;
	extern struct vnode *rootvp;
	struct proc *p = current_proc();	/* XXX */
	struct iso_mnt *imp;
	size_t size;
	int error;
	struct iso_args args;
	
	
	/*
	 * Get vnodes for swapdev and rootdev.
	 */
#if 0
	if (bdevvp(swapdev, &swapdev_vp) || bdevvp(rootdev, &rootvp))
		panic("cd9660_mountroot: can't setup bdevvp's");
#else
	if ( bdevvp(rootdev, &rootvp))
		panic("cd9660_mountroot: can't setup bdevvp's");

#endif

	MALLOC_ZONE(mp, struct mount *,
			sizeof(struct mount), M_MOUNT, M_WAITOK);
	bzero((char *)mp, (u_long)sizeof(struct mount));
	mp->mnt_op = &cd9660_vfsops;
	mp->mnt_flag = MNT_RDONLY;
	LIST_INIT(&mp->mnt_vnodelist);
	args.flags = ISOFSMNT_ROOT;
	if ((error = iso_mountfs(rootvp, mp, p, &args))) {
		FREE_ZONE(mp, sizeof (struct mount), M_MOUNT);
		return (error);
	}
	simple_lock(&mountlist_slock);
    #if 0
	if (error = VFS_LOCK(mp)) {
		(void)cd9660_unmount(mp, 0, p);
		FREE_ZONE(mp, sizeof (struct mount), M_MOUNT);
		return (error);
	}
    #endif
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	mp->mnt_vnodecovered = NULLVP;
	imp = VFSTOISOFS(mp);
	(void) copystr("/", mp->mnt_stat.f_mntonname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);
	(void) copystr(ROOTNAME, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void)cd9660_statfs(mp, &mp->mnt_stat, p);
//	VFS_UNLOCK(mp);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
cd9660_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct iso_args args;
	size_t size;
	int error;
	struct iso_mnt *imp;
	
	if ( (error = copyin(data, (caddr_t)&args, sizeof (struct iso_args))) )
		return (error);
	
	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		return (EROFS);
	
	imp = VFSTOISOFS(mp); /* set up imp before use. */

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		if (args.fspec == 0)
			return (vfs_export(mp, &imp->im_export, &args.export));
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, p);
	if ( (error = namei(ndp)) )
		return (error);
	devvp = ndp->ni_vp;

	if (devvp->v_type != VBLK) {
		vrele(devvp);
		return ENOTBLK;
	}
	if (major(devvp->v_rdev) >= nblkdev) {
		vrele(devvp);
		return ENXIO;
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = iso_mountfs(devvp, mp, p, &args);
	else {
		if (devvp != imp->im_devvp)
			error = EINVAL;	/* needs translation */
		else
			vrele(devvp);
	}
	if (error) {
		vrele(devvp);
		return error;
	}

	/* Set the mount flag to indicate that we support volfs  */
    mp->mnt_flag |= MNT_DOVOLFS;

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	return 0;
}

/*
 * Common code for mount and mountroot
 */
static int
iso_mountfs(devvp, mp, p, argp)
	register struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
	struct iso_args *argp;
{
	register struct iso_mnt *isomp = (struct iso_mnt *)0;
	struct buf *bp = NULL;
	dev_t dev = devvp->v_rdev;
	int error = EINVAL;
	int needclose = 0;
	int ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	extern struct vnode *rootvp;
	int iso_bsize;
	int iso_blknum;
	struct iso_volume_descriptor *vdp;
	struct iso_primary_descriptor *pri;
	struct iso_directory_record *rootp;
	int logical_block_size;
	
	if (!ronly)
		return EROFS;
	
	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if ( (error = vfs_mountedon(devvp)) )
		return error;
	if (vcount(devvp) > 1 && devvp != rootvp)
		return EBUSY;
	if ( (error = vinvalbuf(devvp, V_SAVE, p->p_ucred, p, 0, 0)) )
		return (error);

	if ( (error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p)) )
		return error;
	needclose = 1;
	
	/* This is the "logical sector size".  The standard says this
	 * should be 2048 or the physical sector size on the device,
	 * whichever is greater.  For now, we'll just use a constant.
	 */
	iso_bsize = ISO_DEFAULT_BLOCK_SIZE;
	
	for (iso_blknum = 16; iso_blknum < 100; iso_blknum++) {

		if ( (error = bread(devvp, iso_blknum, iso_bsize, NOCRED, &bp)) )
		{
			goto out;
		};

		vdp = (struct iso_volume_descriptor *)bp->b_data;
		if (bcmp (vdp->id, ISO_STANDARD_ID, sizeof vdp->id) != 0) {
                        #ifdef DEBUG
		        printf("cd9660_vfsops.c: iso_mountfs: Invalid ID in volume desciptor.\n");
                        #endif
			error = EINVAL;
			goto out;
		}
		
		if (isonum_711 (vdp->type) == ISO_VD_END) {
                        #ifdef DEBUG
		        printf("cd9660_vfsops.c: iso_mountfs: isonum_711() != ISO_VD_END.\n");
                        #endif
			error = EINVAL;
			goto out;
		}
		
		if (isonum_711 (vdp->type) == ISO_VD_PRIMARY)
			break;
		brelse(bp);
	}
	
	if (isonum_711 (vdp->type) != ISO_VD_PRIMARY) {
                #ifdef DEBUG
	        printf("cd9660_vfsops.c: iso_mountfs: isnum_711() != ISO_VD_PRIMARY.\n");
		#endif
		error = EINVAL;
		goto out;
	}
	
	pri = (struct iso_primary_descriptor *)vdp;
	
	logical_block_size = isonum_723 (pri->logical_block_size);
	
	if (logical_block_size < DEV_BSIZE || logical_block_size > MAXBSIZE
	    || (logical_block_size & (logical_block_size - 1)) != 0) {
		error = EINVAL;
		goto out;
	}
	
	rootp = (struct iso_directory_record *)pri->root_directory_record;
	
	MALLOC(isomp, struct iso_mnt *, sizeof *isomp, M_ISOFSMNT, M_WAITOK);
	bzero((caddr_t)isomp, sizeof *isomp);
	isomp->logical_block_size = logical_block_size;
	isomp->volume_space_size = isonum_733 (pri->volume_space_size);
	bcopy (rootp, isomp->root, sizeof isomp->root);
	isomp->root_extent = isonum_733 (rootp->extent);
	isomp->root_size = isonum_733 (rootp->size);

	/*
	 * getattrlist wants the volume name, create date and modify date
	 */

    /* Remove any trailing white space */
    if ( strlen(pri->volume_id) ) {
    	char    	*myPtr;
    	
		myPtr = pri->volume_id + strlen( pri->volume_id ) - 1;
		while ( *myPtr == ' ' && myPtr >= pri->volume_id ) {
	    	*myPtr = 0x00;
	    	myPtr--;
		}
    }
	/* YYY need to use secondary volume descriptor name for kanji disks */
	bcopy(pri->volume_id, isomp->volume_id, sizeof(isomp->volume_id));
	cd9660_tstamp_conv17(pri->creation_date, &isomp->creation_date);
	cd9660_tstamp_conv17(pri->modification_date, &isomp->modification_date);

#if 0
	kprintf("\n************************************************************************* \n");
	kprintf( "iso_mountfs - pri->id is \"%c%c%c%c%c\" \n",
			pri->id[0], pri->id[1], pri->id[2], pri->id[3], pri->id[4] );
	kprintf( "iso_mountfs - pri->CDXASignature is \"%c%c%c%c%c%c%c%c\" \n",
			pri->CDXASignature[0], pri->CDXASignature[1], pri->CDXASignature[2], 
			pri->CDXASignature[3], pri->CDXASignature[4], pri->CDXASignature[5],
			pri->CDXASignature[6], pri->CDXASignature[7] );
#endif

	/* See if this is a CD-XA volume */
	if ( bcmp( pri->CDXASignature, ISO_XA_ID, sizeof(pri->CDXASignature) ) == 0 ) 
		isomp->im_flags2 |= IMF2_IS_CDXA;
			
	isomp->im_bmask = logical_block_size - 1;
	isomp->im_bshift = 0;
	while ((1 << isomp->im_bshift) < isomp->logical_block_size)
		isomp->im_bshift++;

	bp->b_flags |= B_AGE;
	brelse(bp);
	bp = NULL;

	mp->mnt_data = (qaddr_t)isomp;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
	mp->mnt_maxsymlinklen = 0;
	mp->mnt_flag |= MNT_LOCAL;
	isomp->im_mountp = mp;
	isomp->im_dev = dev;
	isomp->im_devvp = devvp;	

	devvp->v_specflags |= SI_MOUNTEDON;
	
	/* Check the Rock Ridge Extention support */
	if (!(argp->flags & ISOFSMNT_NORRIP)) {
		if ( (error = bread(isomp->im_devvp,
				  (isomp->root_extent + isonum_711(rootp->ext_attr_length)),
				  isomp->logical_block_size, NOCRED, &bp)) )
		    goto out;
		
		rootp = (struct iso_directory_record *)bp->b_data;
		
		if ((isomp->rr_skip = cd9660_rrip_offset(rootp,isomp)) < 0) {
		    argp->flags  |= ISOFSMNT_NORRIP;
		} else {
		    argp->flags  &= ~ISOFSMNT_GENS;
		}
		
		/*
		 * The contents are valid,
		 * but they will get reread as part of another vnode, so...
		 */
		bp->b_flags |= B_AGE;
		brelse(bp);
		bp = NULL;
	}
	isomp->im_flags = argp->flags&(ISOFSMNT_NORRIP|ISOFSMNT_GENS|ISOFSMNT_EXTATT);
	switch (isomp->im_flags&(ISOFSMNT_NORRIP|ISOFSMNT_GENS)) {
	default:
	    isomp->iso_ftype = ISO_FTYPE_DEFAULT;
	    break;
	case ISOFSMNT_GENS|ISOFSMNT_NORRIP:
	    isomp->iso_ftype = ISO_FTYPE_9660;
	    break;
	case 0:
	    isomp->iso_ftype = ISO_FTYPE_RRIP;
	    break;
	}
	
	return 0;
out:
	if (bp)
		brelse(bp);
	if (needclose)
		(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	if (isomp) {
		FREE((caddr_t)isomp, M_ISOFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}

	/* Clear the mounted on bit in the devvp If it 	 */
	/* not set, this is a nop and there is no way to */
	/* get here with it set unless we did it.  If you*/
	/* are making code changes which makes the above */
	/* assumption not true, change this code.        */

	devvp->v_specflags &= ~SI_MOUNTEDON;

	return error;
}

/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int
cd9660_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return 0;
}

/*
 * unmount system call
 */
int
cd9660_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	register struct iso_mnt *isomp;
	int error, flags = 0;
	
	if ( (mntflags & MNT_FORCE) )
		flags |= FORCECLOSE;
#if 0
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp))
		return EBUSY;
#endif
	if ( (error = vflush(mp, NULLVP, flags)) )
		return (error);

	isomp = VFSTOISOFS(mp);

#ifdef	ISODEVMAP
	if (isomp->iso_ftype == ISO_FTYPE_RRIP)
		iso_dunmap(isomp->im_dev);
#endif
	
	isomp->im_devvp->v_specflags &= ~SI_MOUNTEDON;
	error = VOP_CLOSE(isomp->im_devvp, FREAD, NOCRED, p);
	vrele(isomp->im_devvp);
	FREE((caddr_t)isomp, M_ISOFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Return root of a filesystem
 */
int
cd9660_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct iso_mnt *imp = VFSTOISOFS(mp);
	struct iso_directory_record *dp =
	    (struct iso_directory_record *)imp->root;
	ino_t ino = isodirino(dp, imp);
	
	/*
	 * With RRIP we must use the `.' entry of the root directory.
	 * Simply tell vget, that it's a relocated directory.
	 */
	return (cd9660_vget_internal(mp, ino, vpp,
	    imp->iso_ftype == ISO_FTYPE_RRIP, dp, current_proc()));
}

/*
 * Do operations associated with quotas, not supported
 */
/* ARGSUSED */
int
cd9660_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{

	return (EOPNOTSUPP);
}

/*
 * Get file system statistics.
 */
int
cd9660_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct iso_mnt *isomp;
	
	isomp = VFSTOISOFS(mp);

#ifdef COMPAT_09
	sbp->f_type = 5;
#else
	sbp->f_type = 0;
#endif
	sbp->f_bsize = isomp->logical_block_size;
	sbp->f_iosize = sbp->f_bsize;	/* XXX */
	sbp->f_blocks = isomp->volume_space_size;
	sbp->f_bfree = 0; /* total free blocks */
	sbp->f_bavail = 0; /* blocks free for non superuser */
	sbp->f_files =  0; /* total files */
	sbp->f_ffree = 0; /* free file nodes */
	if (sbp != &mp->mnt_stat) {
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}

	strncpy( sbp->f_fstypename, mp->mnt_vfc->vfc_name, (MFSNAMELEN - 1) );
	sbp->f_fstypename[(MFSNAMELEN - 1)] = '\0';

	/* Use the first spare for flags: */
	sbp->f_spare[0] = isomp->im_flags;
	return 0;
}

/* ARGSUSED */
int
cd9660_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is in range
 * - call iget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 * - check that the generation number matches
 */

struct ifid {
	ushort	ifid_len;
	ushort	ifid_pad;
	int	ifid_ino;
	long	ifid_start;
};

/* ARGSUSED */
int
cd9660_fhtovp(mp, fhp, nam, vpp, exflagsp, credanonp)
	register struct mount *mp;
	struct fid *fhp;
	struct mbuf *nam;
	struct vnode **vpp;
	int *exflagsp;
	struct ucred **credanonp;
{
	struct ifid *ifhp = (struct ifid *)fhp;
	register struct iso_node *ip;
	register struct netcred *np;
	register struct iso_mnt *imp = VFSTOISOFS(mp);
	struct vnode *nvp;
	int error;
	
#ifdef	ISOFS_DBG
	printf("fhtovp: ino %d, start %ld\n",
	       ifhp->ifid_ino, ifhp->ifid_start);
#endif
	
	/*
	 * Get the export permission structure for this <mp, client> tuple.
	 */
	np = vfs_export_lookup(mp, &imp->im_export, nam);
	if (np == NULL)
		return (EACCES);

	if ( (error = VFS_VGET(mp, &ifhp->ifid_ino, &nvp)) ) {
		*vpp = NULLVP;
		return (error);
	}
	ip = VTOI(nvp);
	if (ip->inode.iso_mode == 0) {
		vput(nvp);
		*vpp = NULLVP;
		return (ESTALE);
	}
	*vpp = nvp;
	*exflagsp = np->netc_exflags;
	*credanonp = &np->netc_anon;
	return (0);
}

int
cd9660_vget(mp, ino, vpp)
	struct mount *mp;
	void *ino;
	struct vnode **vpp;
{

	/*
	 * XXXX
	 * It would be nice if we didn't always set the `relocated' flag
	 * and force the extra read, but I don't want to think about fixing
	 * that right now.
	 */

	return ( cd9660_vget_internal( mp, *(ino_t*)ino, vpp, 0, 
								   (struct iso_directory_record *) 0,
								   current_proc()) );
}

int
cd9660_vget_internal(mp, ino, vpp, relocated, isodir, p)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
	int relocated;
	struct iso_directory_record *isodir;
    struct proc *p;
{
	register struct iso_mnt *imp;
	struct iso_node *ip;
	struct buf *bp;
	struct vnode *vp, *nvp;
	dev_t dev;
	int error;

	imp = VFSTOISOFS(mp);
	dev = imp->im_dev;
	if ((*vpp = cd9660_ihashget(dev, ino, p)) != NULLVP)
		return (0);

	/* Allocate a new vnode/iso_node. */
	if ( (error = getnewvnode(VT_ISOFS, mp, cd9660_vnodeop_p, &vp)) ) {
		*vpp = NULLVP;
		return (error);
	}
	MALLOC(ip, struct iso_node *, sizeof(struct iso_node), M_ISOFSNODE,
	    M_WAITOK);
	bzero((caddr_t)ip, sizeof(struct iso_node));
	lockinit(&ip->i_lock, PINOD,"isonode",0,0);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_dev = dev;
	ip->i_number = ino;

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	cd9660_ihashins(ip);

	if (isodir == 0) {
		int lbn, off;

		lbn = lblkno(imp, ino);
		if (lbn >= imp->volume_space_size) {
			vput(vp);
			printf("fhtovp: lbn exceed volume space %d\n", lbn);
			return (ESTALE);
		}
	
		off = blkoff(imp, ino);
		if (off + ISO_DIRECTORY_RECORD_SIZE > imp->logical_block_size) {
			vput(vp);
			printf("fhtovp: crosses block boundary %d\n",
			       off + ISO_DIRECTORY_RECORD_SIZE);
			return (ESTALE);
		}
	
		error = bread(imp->im_devvp,
			      lbn,
			      imp->logical_block_size, NOCRED, &bp);
		if (error) {
			vput(vp);
			brelse(bp);
			printf("fhtovp: bread error %d\n",error);
			return (error);
		}
		isodir = (struct iso_directory_record *)(bp->b_data + off);

		if (off + isonum_711(isodir->length) >
		    imp->logical_block_size) {
			vput(vp);
			if (bp != 0)
				brelse(bp);
			printf("fhtovp: directory crosses block boundary %d[off=%d/len=%d]\n",
			       off +isonum_711(isodir->length), off,
			       isonum_711(isodir->length));
			return (ESTALE);
		}

		/*
		 * for directories we can get parentID from adjacent parent directory record
		 */
		if ((isonum_711(isodir->flags) & directoryBit) && (isodir->name[0] == 0)) {
		 	struct iso_directory_record *pdp;
		 	
			pdp = (struct iso_directory_record *) ((char *)bp->b_data + isonum_711(isodir->length));
			if ((isonum_711(pdp->flags) & directoryBit) && (pdp->name[0] == 1))
				ip->i_parent = isodirino(pdp, imp);
			
		}
#if 0
		if (isonum_733(isodir->extent) +
		    isonum_711(isodir->ext_attr_length) != ifhp->ifid_start) {
			if (bp != 0)
				brelse(bp);
			printf("fhtovp: file start miss %d vs %d\n",
			       isonum_733(isodir->extent) + isonum_711(isodir->ext_attr_length),
			       ifhp->ifid_start);
			return (ESTALE);
		}
#endif
	} else
		bp = 0;

	ip->i_mnt = imp;
	ip->i_devvp = imp->im_devvp;
	VREF(ip->i_devvp);

	if (relocated) {
		/*
		 * On relocated directories we must
		 * read the `.' entry out of a dir.
		 */
		ip->iso_start = ino >> imp->im_bshift;
		if (bp != 0)
			brelse(bp);
		if ( (error = VOP_BLKATOFF(vp, (off_t)0, NULL, &bp)) ) {
			vput(vp);
			return (error);
		}
		isodir = (struct iso_directory_record *)bp->b_data;
	}

	/*
	 * get apple extensions to ISO directory record or use defaults
	 * when there are no apple extensions.
	 */
	if ( (isonum_711( isodir->flags ) & directoryBit) == 0 )
	{
		/* This is an ISO directory record for a file */
		DRGetTypeCreatorAndFlags( imp, isodir, &ip->i_FileType, 
								  &ip->i_Creator, &ip->i_FinderFlags );
	}

	ip->iso_extent = isonum_733(isodir->extent);
	ip->i_size = isonum_733(isodir->size);
	ip->iso_start = isonum_711(isodir->ext_attr_length) + ip->iso_extent;

	/*
	 * if we have a valid name, fill in i_name
	 */
	if (((u_char)isodir->name[0]) > 1) {
		u_short namelen;

		if (imp->iso_ftype == ISO_FTYPE_RRIP) {
			ino_t inump = 0;
	
			cd9660_rrip_getname(isodir, ip->i_name, &namelen, &inump, imp);
		} else {
			isofntrans (isodir->name, isonum_711(isodir->name_len),
					ip->i_name, &namelen,
					imp->iso_ftype == ISO_FTYPE_9660,
					isonum_711(isodir->flags) & associatedBit);
		}
	
		if (namelen > (sizeof(ip->i_name) - 1))
			namelen = sizeof(ip->i_name) - 1;
		ip->i_name[namelen] = '\0';
	}
	
	/*
	 * Setup time stamp, attribute
	 */
	vp->v_type = VNON;
	switch (imp->iso_ftype) {
	default:	/* ISO_FTYPE_9660 */
	    {
		struct buf *bp2;
		int off;
		if ((imp->im_flags & ISOFSMNT_EXTATT)
		    && (off = isonum_711(isodir->ext_attr_length)))
			VOP_BLKATOFF(vp, (off_t)-(off << imp->im_bshift), NULL,
				     &bp2);
		else
			bp2 = NULL;
		cd9660_defattr(isodir, ip, bp2);
		cd9660_deftstamp(isodir, ip, bp2);
		if (bp2)
			brelse(bp2);
		break;
	    }
	case ISO_FTYPE_RRIP:
		cd9660_rrip_analyze(isodir, ip, imp);
		break;
	}

	if (bp != 0)
		brelse(bp);

	/*
	 * Initialize the associated vnode
	 */
	switch (vp->v_type = IFTOVT(ip->inode.iso_mode)) {
	case VFIFO:
#if	FIFO
		vp->v_op = cd9660_fifoop_p;
		break;
#else
		vput(vp);
		return (EOPNOTSUPP);
#endif	/* FIFO */
	case VCHR:
	case VBLK:
		/*
		 * if device, look at device number table for translation
		 */
#ifdef	ISODEVMAP
		if (dp = iso_dmap(dev, ino, 0))
			ip->inode.iso_rdev = dp->d_dev;
#endif
		vp->v_op = cd9660_specop_p;
		if ( (nvp = checkalias(vp, ip->inode.iso_rdev, mp)) ) {
			/*
			 * Discard unneeded vnode, but save its iso_node.
			 */
			cd9660_ihashrem(ip);
			VOP_UNLOCK(vp, 0, p);
			nvp->v_data = vp->v_data;
			vp->v_data = NULL;
			vp->v_op = spec_vnodeop_p;
			vrele(vp);
			vgone(vp);
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			ip->i_vnode = vp;
			cd9660_ihashins(ip);
		}
		break;
	default:
		break;
	}
	
	if (ip->iso_extent == imp->root_extent) {
		vp->v_flag |= VROOT;
		ip->i_parent = 1;	/* root's parent is always 1 by convention */
	}
	/*
	 * XXX need generation number?
	 */

	*vpp = vp;
	return (0);
}


/************************************************************************
 *
 *  Function:	DRGetTypeCreatorAndFlags
 *
 *  Purpose:	Set up the fileType, fileCreator and fileFlags
 *
 *  Returns:	none
 *
 *  Side Effects:	sets *theTypePtr, *theCreatorPtr, and *theFlagsPtr
 *
 *  Description:
 *
 *  Revision History:
 *	28 Jul 88	BL¡B	Added a new extension type of 6, which allows
 *						the specification of four of the finder flags.
 *						We let the creator of the disk just copy over
 *						the finder flags, but we only look at always
 *						switch launch, system, bundle, and locked bits.
 *	15 Aug 88	BL¡B	The Apple extensions to ISO 9660 implemented the
 *						padding field at the end of a directory record
 *						incorrectly.
 *	19 Jul 89	BG		Rewrote routine to handle the "new" Apple
 *						Extensions definition, as well as take into
 *						account the possibility of "other" definitions.
 *	02 Nov 89	BG		Corrected the 'AA' SystemUseID processing to
 *						check for SystemUseID == 2 (HFS).  Was incorrectly
 *						checking for SystemUseID == 1 (ProDOS) before.
 *	18 Mar 92	CMP		Fixed the check for whether len_fi was odd or even.
 *						Before it would always assume even for an XA record.
 *	26 Dec 97	jwc		Swiped from MacOS implementation of ISO 9660 CD-ROM support
 *						and modified to work in Rhapsody file system.
 *
 *********************************************************************** */
 
static void DRGetTypeCreatorAndFlags(	struct iso_mnt * theMountPointPtr,
										struct iso_directory_record * theDirRecPtr, 
										u_int32_t * theTypePtr, 
										u_int32_t * theCreatorPtr, 
										u_int16_t * theFlagsPtr )
{
	int					foundStuff;
	u_int32_t			myType;
	u_int32_t			myCreator;
	AppleExtension		*myAppleExtPtr;
	NewAppleExtension	*myNewAppleExtPtr;
	u_int16_t			myFinderFlags;
	char				*myPtr;

	foundStuff = 1;
	myType = 0L;
	myCreator = 0L;
	myFinderFlags = 0;
	*theFlagsPtr = 0x0000;

	/* handle the fact that our original apple extensions didn't take
	   into account the padding byte on a file name */

	myPtr = &theDirRecPtr->name[ (isonum_711(theDirRecPtr->name_len)) ];
	
	/* if string length is even, bump myPtr for padding byte */
	if ( ((isonum_711(theDirRecPtr->name_len)) & 0x01) == 0 )
		myPtr++;
	myAppleExtPtr = (AppleExtension *) myPtr;

	/* checking for whether or not the new 'AA' code is being called (and if so, correctly) */
	if ( (isonum_711(theDirRecPtr->length)) <= 
		 ISO_DIRECTORY_RECORD_SIZE + (isonum_711(theDirRecPtr->name_len)) )
	{
		foundStuff = 0;
		goto DoneLooking;
	}

	if ( myAppleExtPtr->signature[0] == 'B' && myAppleExtPtr->signature[1] == 'A' )
	{
		int		i;

		switch ( isonum_711(myAppleExtPtr->systemUseID) )
		{
			case 0x00:	/* nothing there.  use default */
				foundStuff = 0;
				break;
			case 0x02:	/* no icon */
			case 0x04:	/* icon, no bundle bit */
				for ( i = 0; i < 4; i++ )
					myType = (myType << 8) | myAppleExtPtr->fileType[i];
					
				for ( i = 0; i < 4; i++ )
					myCreator = (myCreator << 8) | myAppleExtPtr->fileCreator[i];
					
				*theFlagsPtr |= fInitedBit;
				break;
			case 0x03:	/* no icon, bundle bit */
			case 0x05:	/* icon and bundle bit */
				for ( i = 0; i < 4; i++ )
					myType = (myType << 8) | myAppleExtPtr->fileType[i];
					
				for ( i = 0; i < 4; i++ )
					myCreator = (myCreator << 8) | myAppleExtPtr->fileCreator[i];
					
				*theFlagsPtr |= fInitedBit | fHasBundleBit;
				break;
			case 0x06:	/* finder flags in word at end */
				for ( i = 0; i < 4; i++ )
					myType = (myType << 8) | myAppleExtPtr->fileType[i];
					
				for ( i = 0; i < 4; i++ )
					myCreator = (myCreator << 8) | myAppleExtPtr->fileCreator[i];
					
				myFinderFlags = 
					(myAppleExtPtr->finderFlags[0] << 8) | myAppleExtPtr->finderFlags[1];
					
				/* just check the following four bits--all others always set to 0 */
				myFinderFlags &= (fAlwaysBit | fSystemBit | fHasBundleBit | fLockedBit);
				*theFlagsPtr |= (fInitedBit | myFinderFlags);
				break;
			default:
				foundStuff = 0;
		}
		goto DoneLooking;
	}
	
	/*
	 *		If signature[]s != 'BA', then (1) this is not an old-style disc and we
	 *		need to look at ALL available SystemUse possibilities;  (2)  this could
	 *		possibly be a CD-XA style disc and our offset calculations need to
	 *		take this into account.
	 */

	foundStuff = 0;	/* now we default to *false* until we find a good one */
	myPtr = (char *) myAppleExtPtr;

	if ( (theMountPointPtr->im_flags2 & IMF2_IS_CDXA) != 0 )
		myPtr += 14;/* add in CD-XA fixed record offset (tnx, Phillips) */
	myNewAppleExtPtr = (NewAppleExtension *) myPtr;

	/* calculate the "real" end of the directory record information */
	myPtr = ((char *) theDirRecPtr) + (isonum_711(theDirRecPtr->length));
	while( (char *) myNewAppleExtPtr < myPtr ) 	/* end of directory buffer */
	{
		/*
		 *	If we get here, we can assume that ALL further entries in this
		 *	directory record are of the form:
		 *
		 *		struct OptionalSystemUse
		 *		{
		 *			byte	Signature[2];
		 *			byte	systemUseID;
		 *			byte	OSULength;
		 *			byte	fileType[4];		# only if HFS
		 *			byte	fileCreator[4];		# only if HFS
		 *			byte	finderFlags[2];		# only if HFS
		 *		};
		 *
		 *	This means that we can examine the Signature bytes to see if they are 'AA'
		 *	(the NEW Apple extension signature).  If they are, deal with them.  If they
		 *	aren't, the OSULength field will tell us how long this extension info is
		 *	(including the signature and length bytes) and that will allow us to walk
		 *	the OptionalSystemUse records until we hit the end of them or run off the
		 *	end of the directory record.
		 */
		u_char				*myFromPtr, *myToPtr;
		union
		{
			u_int32_t		fourchars;
			u_char			chars[4];
		} myChars;

		if ( myNewAppleExtPtr->signature[0] == 'A' && myNewAppleExtPtr->signature[1] == 'A' )
		{
			if ( isonum_711(myNewAppleExtPtr->systemUseID) == 2 )	/* HFS */
			{
				foundStuff = 1;			/* we got one! */

				myFromPtr = &myNewAppleExtPtr->fileType[0]; 
				myToPtr = &myChars.chars[0];
				*myToPtr++ = *myFromPtr++; 
				*myToPtr++ = *myFromPtr++; 
				*myToPtr++ = *myFromPtr++; 
				*myToPtr = *myFromPtr;
				myType = myChars.fourchars;		/* copy file type to user var */

				myFromPtr = &myNewAppleExtPtr->fileCreator[0]; 
				myToPtr = &myChars.chars[0];
				*myToPtr++ = *myFromPtr++; 
				*myToPtr++ = *myFromPtr++; 
				*myToPtr++ = *myFromPtr++; 
				*myToPtr = *myFromPtr;
				myCreator = myChars.fourchars;	/* copy creator to user var */

				myFromPtr = &myNewAppleExtPtr->finderFlags[0]; 
				myToPtr = &myChars.chars[2];	/* *flags* is a short */
				myChars.fourchars = 0; 
				*myToPtr++ = *myFromPtr++; 
				*myToPtr = *myFromPtr;
				myFinderFlags = myChars.fourchars;
				myFinderFlags &= ( fAlwaysBit | fSystemBit | fHasBundleBit | fLockedBit );
				*theFlagsPtr = (myFinderFlags | fInitedBit); /* return Finder flags to user var */

				break;		/* exit the loop */
			}
		}

		/*
		 *	Check to see if we have a reasonable OSULength value.
		 *	ZERO is not an acceptable value.  Nor is any value less than 4.
		 */

		if ( (isonum_711(myNewAppleExtPtr->OSULength)) < 4 ) 
			break;	/* not acceptable - get out! */

		/* otherwise, step past this SystemUse record */
		(char *)myNewAppleExtPtr += (isonum_711(myNewAppleExtPtr->OSULength));
		
	} // end of while loop

DoneLooking:
	if ( foundStuff != 0 )
	{
		*theTypePtr    = myType;
		*theCreatorPtr = myCreator;
	}
	else
	{
		*theTypePtr = DEFAULTTYPE;
		*theCreatorPtr = DEFAULTCREATOR;
		*theFlagsPtr |= fInitedBit;
	}
	
	return;
	
} /* DRGetTypeCreatorAndFlags */


/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
cd9660_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	register struct iso_node *ip = VTOI(vp);
	register struct ifid *ifhp;
	
	ifhp = (struct ifid *)fhp;
	ifhp->ifid_len = sizeof(struct ifid);
	
	ifhp->ifid_ino = ip->i_number;
	ifhp->ifid_start = ip->iso_start;
	
#ifdef	ISOFS_DBG
	printf("vptofh: ino %d, start %ld\n",
	       ifhp->ifid_ino,ifhp->ifid_start);
#endif
	return 0;
}

/*
 * Fast-FileSystem only?
 */
int
cd9660_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
     int * name;
     u_int namelen;
     void* oldp;
     size_t * oldlenp;
     void * newp;
     size_t newlen;
     struct proc * p;
{
     return EOPNOTSUPP;
}

