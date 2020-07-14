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
/*
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *      hfs_vfsops.c
 *  derived from	@(#)ufs_vfsops.c	8.8 (Berkeley) 5/20/95
 *
 *      (c) Copyright 1997-1998 Apple Computer, Inc. All rights reserved.
 *
 *      hfs_vfsops.c -- VFS layer for loadable HFS file system.
 *
 *      HISTORY
 *              12-Nov-1998     Pat Dirks       Changed hfs_statfs to return volume's actual log. block size (#2286198).
 *		22-Aug-1998	Scott Roberts	Assign uid,gid, and mask for default on objects.
 *		29-Jul-1998	Pat Dirks		Fixed changed hfs_vget() to release complex node when retrying for data fork node.
 *		27-Jul-1998	Scott Roberts	Changes hfs_vget() to return data forks instead of complex.
 *         14-Jul-1998  CHW			Added check for use count of device node in hfs_mountfs
 *	    1-Jul-1998	Don Brady		Always set kHFSVolumeUnmountedMask bit of vcb->vcbAtrb in hfs_unmount.
 *	   30-Jun-1998	Don Brady		Removed hard-coded EINVAL error in hfs_mountfs (for radar #2249539).
 *	   24-Jun-1998	Don Brady		Added setting of timezone to hfs_mount (radar #2226387).
 *		4-Jun-1998	Don Brady		Use VPUT/VRELE macros instead of vput/vrele.
 *		6-May-1998	Scott Roberts	Updated hfs_vget with kernel changes.
 *		29-Apr-1998	Don Brady		Update hfs_statfs to actually fill in statfs fields (radar #2227092).
 *		23-Apr-1998	Pat Dirks		Cleaned up code to call brelse() on errors from bread().
 *		 4/20/1998	Don Brady		Remove course-grained hfs metadata locking.
 *		 4/18/1998	Don Brady		Add VCB locking.
 *		 4/16/1998	Don Brady	hfs_unmount now flushes the volume bitmap. Add b-tree locking to hfs_vget.
 *		  4/8/1998	Don Brady	Replace hfs_mdbupdate with hfs_flushvolumeheader and hfs_flushMDB.
 *		  4/8/1998	Don Brady	In hfs_unmount call hfs_mdbupdate before trashing metafiles!
 *		  4/3/1998	Don Brady	Call InitCatalogCache instead of PostInitFS.
 *		  4/1/1998	Don Brady	Get rid of gHFSFlags, gReqstVol and gFlushOnlyFlag globals (not used).
 *		 3/30/1998	Don Brady	In hfs_unmount use SKIPSYSTEM option on first vflush.
 *		 3/26/1998	Don Brady	Changed hfs_unmount to call vflush before calling hfsUnmount.
 *								In hfs_mountfs don't mount hfs-wrapper.
 *		 3/19/1998	Pat Dirks	Fixed bug in hfs_mount where device vnode was being
 *								released on way out.
 *      11/14/1997	Pat Dirks	Derived from hfs_vfsops.c
 */

#include <sys/types.h>
#include <sys/namei.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <mach/machine/vm_types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <libkern/libkern.h>
#include <bsd/dev/disk.h>
#include <mach/machine/simple_lock.h>
#include <miscfs/specfs/specdev.h>
#include <sys/systm.h>
#include <mach/features.h>
#include <kern/mapfs.h>

#include "hfs.h"
#include "hfs_dbg.h"
#include "mount_hfs.h"

#include "hfscommon/headers/FileMgrInternal.h"

typedef (*PFI)();

extern char *strncpy __P((char *, const char *, size_t));       /* Kernel already includes a copy of strncpy somewhere... */
extern int strcmp __P((const char *, const char *));            /* Kernel already includes a copy of strcmp somewhere... */

extern struct proc * current_proc_EXTERNAL();

/* The following refer to kernel global variables used in the loading/initialization: */
extern int maxvfsslots; 					/* Total number of slots in the system's vfsconf table */
extern int maxvfsconf;             			/* The highest fs type number [old-style ID] in use [dispite its name] */
extern int vfs_opv_numops; 					/* The total number of defined vnode operations */
extern int kdp_flag;

#if	DIAGNOSTIC
int hfs_dbg_all = 0;
int hfs_dbg_vfs = 0;
int hfs_dbg_vop = 1;
int hfs_dbg_load = 0;
int hfs_dbg_io = 1;
int hfs_dbg_utils = 1;
int hfs_dbg_rw = 1;
int hfs_dbg_DIT = 0;
int hfs_dbg_tree = 0;
int hfs_dbg_err = 0;
int hfs_dbg_test = 0;
#endif

/*
 * HFS File System globals:
 */
Ptr					gBufferAddress[BUFFERPTRLISTSIZE];
struct buf			*gBufferHeaderPtr[BUFFERPTRLISTSIZE];
int					gBufferListIndex;
simple_lock_data_t	gBufferPtrListLock;

static char hfs_fs_name[MFSNAMELEN] = "hfs";

/* The following represent information held in low-memory on the MacOS: */

struct FSVarsRec	*gFSMVars;

/*
 * Global variables defined in other modules:
 */
extern struct vnodeopv_desc hfs_vnodeop_opv_desc;


int hfs_reload(struct mount *mp, struct ucred *cred, struct proc *p);
int hfs_mountfs(struct vnode *devvp, struct mount *mp, struct proc *p);
int hfs_vget(struct mount *mp, void *objID, struct vnode **vpp);
void hfs_vhashinit();

static int hfs_statfs();


/*
 * VFS Operations.
 *
 * mount system call
 */

static int hfs_mount (mp, path, data, ndp, p)
register struct mount   *mp;
char                    *path;
caddr_t                 data;
struct nameidata        *ndp;
struct proc             *p;
{
    struct hfsmount			*hfsmp;
    struct vnode			*devvp;
    struct hfs_mount_args	args;
    size_t					size;
    int						retval = E_NONE;
    mode_t					accessmode;

    DBG_FUNC_NAME("hfs_mount");
    DBG_PRINT_FUNC_NAME();

#if DIAGNOSTIC
//  call_kdp();
#endif

#if FORCE_READONLY
//    DBG_ASSERT((mp->mnt_flag & MNT_RDONLY) == MNT_RDONLY);      /* Should already be set */
    mp->mnt_flag |= MNT_RDONLY;         /* ALWAYS act as if it's mounted read-only */
#endif  /* FORCE_READONLY */

    if ((retval = copyin(data, (caddr_t)&args, sizeof(args))))
        goto error_exit;

    /*
     * If updating, check whether changing from read-only to
     * read/write; if there is no device name, that's all we do.
     */
    if (mp->mnt_flag & MNT_UPDATE) {
        unsigned short flags;

        DBG_VFS(("hfs_mount: handling update...\n"));

        hfsmp = VFSTOHFS(mp);
        if (hfsmp->hfs_fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
            flags = WRITECLOSE;
            if (mp->mnt_flag & MNT_FORCE)
                flags |= FORCECLOSE;
            if ((retval = hfs_flushfiles(mp, flags)))
                goto error_exit;
            hfsmp->hfs_fs_clean = 1;
            hfsmp->hfs_fs_ronly = 1;
            if (HFSTOVCB(hfsmp)->vcbSigWord == kHFSPlusSigWord)
            	retval = hfs_flushvolumeheader(hfsmp, MNT_WAIT);
            else
            	retval = hfs_flushMDB(hfsmp, MNT_WAIT);
            
            if (retval) {
                hfsmp->hfs_fs_clean = 0;
                hfsmp->hfs_fs_ronly = 0;
                goto error_exit;
            }
        }

        if ((mp->mnt_flag & MNT_RELOAD) &&
            (retval = hfs_reload(mp, ndp->ni_cnd.cn_cred, p)))
            goto error_exit;

        if (hfsmp->hfs_fs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
            /*
             * If upgrade to read-write by non-root, then verify
             * that user has necessary permissions on the device.
             */
            if (p->p_ucred->cr_uid != 0) {
                devvp = hfsmp->hfs_devvp;
                vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
                if ((retval = VOP_ACCESS(devvp, VREAD | VWRITE, p->p_ucred, p))) {
                    VOP_UNLOCK(devvp, 0, p);
                    goto error_exit;
                }
                VOP_UNLOCK(devvp, 0, p);
            }
            hfsmp->hfs_fs_ronly = 0;
            hfsmp->hfs_fs_clean = 0;
           	if (HFSTOVCB(hfsmp)->vcbSigWord == kHFSPlusSigWord)
				retval = hfs_flushvolumeheader(hfsmp, MNT_WAIT);
			else
				retval = hfs_flushMDB(hfsmp, MNT_WAIT);

            if (retval != E_NONE)
                goto error_exit;
        }

#if 0   /* XXX PPD */
        if (args.ma_fspec == 0) {
            /*
             * Process export requests.
             */
            retval = (vfs_export(mp, &hfsmp->hfs_export, &args.ma_export));
        }
#endif

        goto std_exit;
    }

    /*
     * Not an update, or updating the name: look up the name
     * and verify that it refers to a sensible block device.
     */
    DBG_VFS(("hfs_mount: looking up device name (%s)...\n", args.ma_fspec));

    NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.ma_fspec, p);
    retval = namei(ndp);
    if (retval != E_NONE) {
        DBG_ERR(("hfs_mount: CAN'T GET DEVICE: %s, %x\n", args.ma_fspec, ndp->ni_vp->v_rdev));
        goto error_exit;
    }

    devvp = ndp->ni_vp;
    DBG_VFS(("hfs_mount: Device is %x\n",(uint)devvp));

    if (devvp->v_type != VBLK) {
        VRELE(devvp);
        retval = ENOTBLK;
        goto error_exit;
    };
    if (major(devvp->v_rdev) >= nblkdev) {
        VRELE(devvp);
        retval = ENXIO;
        goto error_exit;
    };

    /*
     * If mount by non-root, then verify that user has necessary
     * permissions on the device.
     */
    if (p->p_ucred->cr_uid != 0) {
        DBG_VFS(("hfs_mount: Checking mounting access\n"));
        accessmode = VREAD;
        if ((mp->mnt_flag & MNT_RDONLY) == 0)
            accessmode |= VWRITE;
        vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
        if ((retval = VOP_ACCESS(devvp, accessmode, p->p_ucred, p))) {
            VPUT(devvp);
            goto error_exit;
        }
        VOP_UNLOCK(devvp, 0, p);
    }

	/* establish the zimezone (not used by HFS Plus) */
	gTimeZone = args.ma_timezone;

    DBG_VFS(("hfs_mount: Attempting to mount on %s\n", path));
    retval = hfs_mountfs(devvp, mp, p);

    if (retval != E_NONE) {
        VRELE(devvp);
        goto error_exit;
    };

    hfsmp = VFSTOHFS(mp);

    hfsmp->hfs_uid = args.ma_uid;
    hfsmp->hfs_gid = args.ma_gid;
    hfsmp->hfs_dir_mask = args.ma_mask & ALLPERMS;
    if (args.ma_flags & HFSFSMNT_NOXONFILES)
        hfsmp->hfs_file_mask = (args.ma_mask & DEFFILEMODE);
    else
        hfsmp->hfs_file_mask = args.ma_mask & ALLPERMS;

    hfsmp->hfs_encoding = args.ma_encoding;
	
	/* Set the mount flag to indicate that we support volfs  */
    mp->mnt_flag |= MNT_DOVOLFS;
    

    (void) copyinstr(path, hfsmp->hfs_mountpoint, sizeof(hfsmp->hfs_mountpoint) - 1, &size);
    bzero(hfsmp->hfs_mountpoint + size, sizeof(hfsmp->hfs_mountpoint) - size);
    bcopy((caddr_t)hfsmp->hfs_mountpoint, (caddr_t)mp->mnt_stat.f_mntonname, MNAMELEN);
    (void) copyinstr(args.ma_fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, &size);
    bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
    (void)hfs_statfs(mp, &mp->mnt_stat, p);
    retval = E_NONE;
    DBG_VFS(("hfs_mount: SUCCESSFULL MOUNT of %s on %s\n", mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname));

    goto std_exit;

error_exit:
    /* No special exit-path processing to do */
        DBG_VFS(("hfs_mount: BAD MOUNT of %s on %s with an error of %d\n", mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname, retval));

std_exit:
        return (retval);

}


/*
 * Reload all incore data for a filesystem (used after running fsck on
                                            * the root filesystem and finding things to fix). The filesystem must
 * be mounted read-only.
 *
 * Things to do to update the mount:
 *      1) invalidate all cached meta-data.
 *      2) re-read superblock from disk.
 *      3) re-read summary information from disk.
 *      4) invalidate all inactive vnodes.
 *      5) invalidate all cached file data.
 *      6) re-read hfsnode data for all active vnodes.
 */
int hfs_reload(mountp, cred, p)
register struct mount *mountp;
struct ucred *cred;
struct proc *p;
{
#if 0 /* XXX PPD */
    register struct vnode *vp, *nvp, *devvp;
    struct hfsnode *hp;
    struct csum *space;
    struct buf *bp;
    struct hfsmount *hfsmp, *newfs;
    struct disk_label *lab;
    int i, blks, size, retval;
    int32_t *lp;

    if ((mountp->mnt_flag & MNT_RDONLY) == 0)
        return (EINVAL);
    /*
     * Step 1: invalidate all cached meta-data.
     */
    devvp = VFSTOUFS(mountp)->um_devvp;
    if (vinvalbuf(devvp, 0, cred, p, 0, 0))
        panic("ffs_reload: dirty1");
    /*
     * Step 2: re-read superblock from disk.
     */


    if (retval = bread(devvp, (ufs_daddr_t)(SBOFF/size), SBSIZE, NOCRED,&bp)) {
    	if (bp) brelse(bp);
        return (retval);
    };

    /*
     * Copy pointer fields back into superblock before copying in               XXX
     * new superblock. These should really be in the HFS mount point.   XXX
     * Note that important parameters (eg fs_ncg) are unchanged.
     */
    bcopy(&hfsmp->hfs_fs_csp[0], &newfs->fs_csp[0], sizeof(hfsmp->hfs_fs_csp));
    newfs->fs_maxcluster = hfsmp->hfs_fs_maxcluster;
    bcopy(newfs, hfsmp, (u_int)hfsmp->hfs_fs_sbsize);
    if (hfsmp->hfs_fs_sbsize < SBSIZE)
        bp->b_flags |= B_INVAL;
    brelse(bp);
    mountp->mnt_maxsymlinklen = 0;
    ffs_oldfscompat(hfsmp);
    /*
     * Step 3: re-read summary information from disk.
     */
    blks = howmany(hfsmp->hfs_fs_cssize, hfsmp->hfs_fs_fsize);
    space = hfsmp->hfs_fs_csp[0];
    for (i = 0; i < blks; i += hfsmp->hfs_fs_frag) {
        size = hfsmp->hfs_fs_bsize;
        if (i + hfsmp->hfs_fs_frag > blks)
            size = (blks - i) * hfsmp->hfs_fs_fsize;
        if (retval = bread(devvp, fsbtodb(hfsmp, hfsmp->hfs_fs_csaddr + i), size, NOCRED, &bp)) {
            if (bp) brelse(bp);
            return (retval);
        };
        bcopy(bp->b_data, hfsmp->hfs_fs_csp[fragstoblks(hfsmp, i)], (u_int)size);
        brelse(bp);
    }
    /*
     * We no longer know anything about clusters per cylinder group.
     */
    if (hfsmp->hfs_fs_contigsumsize > 0) {
        lp = hfsmp->hfs_fs_maxcluster;
        for (i = 0; i < hfsmp->hfs_fs_ncg; i++)
            *lp++ = hfsmp->hfs_fs_contigsumsize;
    }

loop:
    simple_lock(&mntvnode_slock);
    for (vp = mountp->mnt_vnodelist.lh_first; vp != NULL; vp = nvp) {
        if (vp->v_mount != mountp) {
            simple_unlock(&mntvnode_slock);
            goto loop;
        }
        nvp = vp->v_mntvnodes.le_next;
        /*
         * Step 4: invalidate all inactive vnodes.
         */
        if (vrecycle(vp, &mntvnode_slock, p))
            goto loop;
        /*
         * Step 5: invalidate all cached file data.
         */
        simple_lock(&vp->v_interlock);
        simple_unlock(&mntvnode_slock);
        if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK, p)) {
            goto loop;
        }
        if (vinvalbuf(vp, 0, cred, p, 0, 0))
            panic("hfs_reload: dirty2");
        /*
         * Step 6: re-read hfsnode data for all active vnodes.
         */
        hp = VTOI(vp);
        if ((retval = bread(devvp, fsbtodb(hfsmp, ino_to_fsba(hfsmp, hp->h_i_number)),
        						(int)hfsmp->hfs_fs_bsize, NOCRED, &bp))) {
            VPUT(vp);
            if (bp) brelse(bp);
            return (retval);
        }
        hp->h_i_din = *((struct dinode *)bp->b_data +
                      ino_to_fsbo(hfsmp, hp->h_i_number));
        brelse(bp);
        VPUT(vp);
        simple_lock(&mntvnode_slock);
    }
    simple_unlock(&mntvnode_slock);
#endif
    return (0);
}

/*
 * Common code for mount and mountroot
 */
int
hfs_mountfs(struct vnode *devvp, struct mount *mp, struct proc *p)
{
    int                         retval = E_NONE;
    register struct hfsmount	*hfsmp;
    struct buf					*bp;
    dev_t                       dev;
    HFSMasterDirectoryBlock		*mdbp;
    int                         size, ronly;
    struct ucred				*cred;

    DBG_VFS(("hfs_mountfs: mp = 0x%lX\n", (u_long)mp));

    dev = devvp->v_rdev;
    cred = p ? p->p_ucred : NOCRED;
    /*
     * Disallow multiple mounts of the same device.
     * Disallow mounting of a device that is currently in use
     * (except for root, which might share swap device for miniroot).
     * Flush out any old buffers remaining from a previous use.
     */
    if ((retval = vfs_mountedon(devvp)))
        return (retval);
    if (vcount(devvp) > 1 && devvp != rootvp)
                return (EBUSY);
    if ((retval = vinvalbuf(devvp, V_SAVE, cred, p, 0, 0)))
        return (retval);

    ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
    DBG_VFS(("hfs_mountfs: opening device...\n"));
    if ((retval = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p)))
        return (retval);

    size = 512;
    DBG_VFS(("hfs_mountfs: size = %d (DEV_BSIZE = %d).\n", size, DEV_BSIZE));

    bp = NULL;
    hfsmp = NULL;
    DBG_VFS(("hfs_mountfs: reading MDB [block no. %d + %d bytes, size %d bytes]...\n",
             IOBLKNOFORBLK(kMasterDirectoryBlock, size),
             IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, size),
             IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, size)));

    if ((retval = bread(devvp, IOBLKNOFORBLK(kMasterDirectoryBlock, size),
    						   IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, size), cred, &bp))) {
        if (bp) brelse(bp);
        goto error_exit;
	};
    mdbp = (HFSMasterDirectoryBlock*) ((char *)bp->b_data + IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, size));

    DBG_VFS(("hfs_mountfs: allocating hfsmount structure...\n"));
    MALLOC(hfsmp, struct hfsmount *, sizeof(struct hfsmount), M_HFSMNT, M_WAITOK);
    bzero(hfsmp, sizeof(struct hfsmount));

    DBG_VFS(("hfs_mountfs: Initializing hfsmount structure at 0x%lX...\n", (u_long)hfsmp));
    /*
     *  Init the volume information structure
     */
    mp->mnt_data = (qaddr_t)hfsmp;
    hfsmp->hfs_mp = mp;						/* Make VFSTOHFS work */
    hfsmp->hfs_vcb.vcb_hfsmp = hfsmp;		/* Make VCBTOHFS work */
    hfsmp->hfs_raw_dev = devvp->v_rdev;
    hfsmp->hfs_devvp = devvp;
    hfsmp->hfs_phys_block_size = size;
    /* The hfs_log_block_size field is updated in the respective hfs_MountHFS[Plus]Volume routine */
    hfsmp->hfs_log_block_size = BestBlockSizeFit(mdbp->drAlBlkSiz, MAXBSIZE, hfsmp->hfs_phys_block_size);
    hfsmp->hfs_fs_ronly = ronly;

	if (mdbp->drSigWord == kHFSPlusSigWord) {
		DBG_VFS(("hfs_mountfs: mounting wrapper-less HFS-Plus volume...\n"));
        retval = hfs_MountHFSPlusVolume(hfsmp, (HFSPlusVolumeHeader*) bp->b_data, 0, p);
	}
	else if (mdbp->drEmbedSigWord == kHFSPlusSigWord) {
		u_long				embBlkOffset;
		HFSPlusVolumeHeader	*vhp;

		embBlkOffset = (mdbp->drEmbedExtent.startBlock * (mdbp->drAlBlkSiz)/kHFSBlockSize) + mdbp->drAlBlSt;
        DBG_VFS(("hfs_mountfs: mounting embedded HFS-Plus volume at sector %ld...\n", embBlkOffset));
		brelse(bp);
		bp = NULL;		/* done with MDB, go grab Volume Header */
		mdbp = NULL;

		retval = bread(	devvp,
						IOBLKNOFORBLK(kMasterDirectoryBlock+embBlkOffset, size),
						IOBYTECCNTFORBLK(kMasterDirectoryBlock+embBlkOffset, kMDBSize, size),
						cred,
						&bp);
		if (retval) {
			if (bp) brelse(bp);
			goto error_exit;
		};
		vhp = (HFSPlusVolumeHeader*) ((char *)bp->b_data + IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, size));

		retval = hfs_MountHFSPlusVolume(hfsmp, vhp, embBlkOffset, p);
    }
    else { // pass all other volume formats to MountHFSVolume  
		DBG_VFS(("hfs_mountfs: mounting regular HFS volume...\n"));
		retval = hfs_MountHFSVolume( hfsmp, mdbp, p);
	}

    if ( retval ) {
		goto error_exit;
	}

    brelse(bp);
    bp = NULL;
	
    if (HFSTOVCB(hfsmp)->vcbSigWord == kHFSPlusSigWord)
        mp->mnt_stat.f_fsid.val[0] = (long)dev;
    else
        mp->mnt_stat.f_fsid.val[0] = (long)dev | HFSFSMNT_HFSSTDMASK;
    mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
    mp->mnt_maxsymlinklen = 0;
    devvp->v_specflags |= SI_MOUNTEDON;

    if (ronly == 0) {
        hfsmp->hfs_fs_clean = 0;
        if (HFSTOVCB(hfsmp)->vcbSigWord == kHFSPlusSigWord)
        	(void) hfs_flushvolumeheader(hfsmp, MNT_WAIT);
        else
        	(void) hfs_flushMDB(hfsmp, MNT_WAIT);
    }
    goto std_exit;

error_exit:
        DBG_VFS(("hfs_mountfs: exiting with error %d...\n", retval));

    if (bp)
        brelse(bp);
    (void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, cred, p);
    if (hfsmp) {
        FREE(hfsmp, M_HFSMNT);
        mp->mnt_data = (qaddr_t)0;
    }

std_exit:
        return (retval);
}


/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int hfs_start(mp, flags, p)
struct mount *mp;
int flags;
struct proc *p;
{
    DBG_FUNC_NAME("hfs_start");
    DBG_PRINT_FUNC_NAME();

    return (0);
}


/*
 * unmount system call
 */
int hfs_unmount(mp, mntflags, p)
struct mount *mp;
int mntflags;
struct proc *p;
{
    struct hfsmount		*hfsmp = VFSTOHFS(mp);
    int					retval = E_NONE;
    int					uflags;	/* vflush flag for user vnodes pass */

    DBG_VFS(("hfs_unmount: volume: 0x%x\n", (uint)hfsmp));
    uflags = SKIPSYSTEM;
    if (mntflags & MNT_FORCE)
        uflags |= FORCECLOSE;
        
	/*
	 *	Remove any user vnodes (non-system)
	 */
    retval = vflush(mp, NULL, uflags);
	
	/* Run it again, if we were busy...there might be complex nodes left */
	if (retval == EBUSY)
        retval = vflush(mp, NULL, uflags);

    if (retval != E_NONE)
        return (retval);

	/*
	 *	Flush out the volume bitmap and MDB/Volume Header
	 */
   if (hfsmp->hfs_fs_ronly == 0) {
        hfsmp->hfs_fs_clean = 1;
		HFSTOVCB(hfsmp)->vcbAtrb |=	kHFSVolumeUnmountedMask;
        if (HFSTOVCB(hfsmp)->vcbSigWord == kHFSPlusSigWord)
        	retval = hfs_flushvolumeheader(hfsmp, MNT_WAIT);
        else
        	retval = hfs_flushMDB(hfsmp, MNT_WAIT);
       
        if (retval == 0)
			retval = VOP_FSYNC(hfsmp->hfs_devvp, NOCRED, MNT_WAIT, p);

        if (retval) {
            hfsmp->hfs_fs_clean = 0;
            if ((mntflags & MNT_FORCE) == 0)
				return (retval);	/* could not flush everything */
		}
    }

	/*
	 *	Invalidate our caches and release metadata vnodes
	 */
    retval = hfsUnmount(hfsmp, p);
    if(retval != E_NONE) {
        DBG_ERR(("hfs_unmount: hfsUnmount FAILED: %d\n", retval));
    }

	/*
	 *	Remove the metadata vnodes (they should not be busy)
	 */
    retval = vflush(mp, NULL, 0);
    ASSERT(retval != EBUSY);
    if (retval != E_NONE)
        return (retval);	/* XXX leaving now would be bad - we're in an inconsistent state! */

    hfsmp->hfs_devvp->v_specflags &= ~SI_MOUNTEDON;

    retval = VOP_CLOSE(hfsmp->hfs_devvp, hfsmp->hfs_fs_ronly ? FREAD : FREAD|FWRITE,
                       NOCRED, p);
    VRELE(hfsmp->hfs_devvp);

    FREE(hfsmp, M_HFSMNT);
    mp->mnt_data = (qaddr_t)0;

    DBG_VFS(("hfs_unmount: Finished, returning %d\n", retval));

    return(retval);
}


/*
 * Return the root of a filesystem.
 *
 *              OUT - vpp, should be locked and vget()'d (to increment usecount and lock)
 */
int hfs_root(mp, vpp)
struct mount *mp;
struct vnode **vpp;
{
    struct vnode *nvp;
    int retval;
    UInt32 rootObjID = kRootDirID;

    DBG_FUNC_NAME("hfs_root");
    DBG_PRINT_FUNC_NAME();

    if ((retval = VFS_VGET(mp, &rootObjID, &nvp)))
        return (retval);

    *vpp = nvp;
    return (0);
}

/*
 * Do operations associated with quotas
 */
int hfs_quotactl(mp, cmds, uid, arg, p)
struct mount *mp;
int cmds;
uid_t uid;
caddr_t arg;
struct proc *p;
{
    DBG_FUNC_NAME("hfs_quotactl");
    DBG_PRINT_FUNC_NAME();

    return (EOPNOTSUPP);
}



/*
 * Get file system statistics.
 */
static int hfs_statfs(mp, sbp, p)
struct mount *mp;
register struct statfs *sbp;
struct proc *p;
{
    ExtendedVCB *vcb = VFSTOVCB(mp);
    struct hfsmount *hfsmp = VFSTOHFS(mp);
    u_long freeCNIDs;

    DBG_FUNC_NAME("hfs_statfs");
    DBG_PRINT_FUNC_NAME();

	freeCNIDs = (u_long)0xFFFFFFFF - (u_long)vcb->vcbNxtCNID;

	sbp->f_bsize = vcb->blockSize;
    sbp->f_iosize = hfsmp->hfs_log_block_size;
	sbp->f_blocks = vcb->totalBlocks;
	sbp->f_bfree = vcb->freeBlocks;
	sbp->f_bavail = vcb->freeBlocks;
	sbp->f_files = (u_long)vcb->vcbFilCnt + (u_long)vcb->vcbDirCnt;
	sbp->f_ffree = MIN(freeCNIDs, vcb->freeBlocks);
	
    sbp->f_type = 0;
    if (sbp != &mp->mnt_stat) {
        sbp->f_type = mp->mnt_vfc->vfc_typenum;
        bcopy((caddr_t)mp->mnt_stat.f_mntonname,
              (caddr_t)&sbp->f_mntonname[0], MNAMELEN);
        bcopy((caddr_t)mp->mnt_stat.f_mntfromname,
              (caddr_t)&sbp->f_mntfromname[0], MNAMELEN);
    }
    return (0);
}


/*
 * Go through the disk queues to initiate sandbagged IO;
 * go through the inodes to write those that have been modified;
 * initiate the writing of the super block if it has been modified.
 *
 * Note: we are always called with the filesystem marked `MPBUSY'.
 */
static int hfs_sync(mp, waitfor, cred, p)
struct mount *mp;
int waitfor;
struct ucred *cred;
struct proc *p;
{
    struct vnode 		*nvp, *vp, *btvp;
    struct hfsnode 		*hp;
    struct hfsmount		*hfsmp = VFSTOHFS(mp);
    ExtendedVCB			*vcb;
    int error, allerror = 0;

    DBG_FUNC_NAME("hfs_sync");
    DBG_PRINT_FUNC_NAME();

    hfsmp = VFSTOHFS(mp);
    if (hfsmp->hfs_fs_ronly != 0) {
        panic("update: rofs mod");
    };

    /*
     * Write back each 'modified' vnode
     */
    btvp = 0;
    simple_lock(&mntvnode_slock);

loop:;

    for (vp = mp->mnt_vnodelist.lh_first;
         vp != NULL;
         vp = nvp) {
        /*
         * If the vnode that we are about to sync is no longer
         * associated with this mount point, start over.
         */
        if (vp->v_mount != mp)
            goto loop;
        simple_lock(&vp->v_interlock);
        nvp = vp->v_mntvnodes.le_next;
        hp = VTOH(vp);
        if (H_FILEID(hp) == 3)
            btvp = vp;
        if (hp && hp->h_meta && (hp->h_meta->h_nodeflags & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0 &&
            vp->v_dirtyblkhd.lh_first == NULL) {
            simple_unlock(&vp->v_interlock);
            continue;
        }

        simple_unlock(&mntvnode_slock);
        error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK, p);
        DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
        if (error) {
            DBG_ERR(("hfs_sync: error %d on vget for vnode 0x%lX.\n", error, (u_long)vp));
            simple_lock(&mntvnode_slock);
            if (error == ENOENT)
                goto loop;
            continue;
        }

        if ((error = VOP_FSYNC(vp, cred, waitfor, p))) {
            DBG_ERR(("hfs_sync: error %d calling fsync on vnode 0x%X.\n", error, (u_int)vp));
            allerror = error;
        };
        DBG_ASSERT(*((volatile int *)(&(vp)->v_interlock))==0);
        VPUT(vp);
        simple_lock(&mntvnode_slock);
    };

    /* Now reprocess the BTree node, stored above */
    {
        /*
         * If the vnode that we are about to sync is no longer
         * associated with this mount point, start over.
         */
        if ((btvp==0) || (btvp->v_mount != mp))
            goto skipBtree;
        simple_lock(&btvp->v_interlock);
        hp = VTOH(btvp);
        if ((hp->h_meta->h_nodeflags & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0 &&
            btvp->v_dirtyblkhd.lh_first == NULL) {
            simple_unlock(&btvp->v_interlock);
            goto skipBtree;
        }
        simple_unlock(&mntvnode_slock);
        error = vget(btvp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK, p);
        if (error) {
            simple_lock(&mntvnode_slock);
            goto skipBtree;
        }
        if ((error = VOP_FSYNC(btvp, cred, waitfor, p)))
            allerror = error;
        VOP_UNLOCK(btvp, 0, p);
        VRELE(btvp);
        simple_lock(&mntvnode_slock);
    };

skipBtree:;

    simple_unlock(&mntvnode_slock);

    /*
     * Force stale file system control information to be flushed.
     */
    if ((error = VOP_FSYNC(hfsmp->hfs_devvp, cred, waitfor, p)))
        allerror = error;
    /*
     * Write back modified superblock.
     */
	vcb = HFSTOVCB(hfsmp);

    if (IsVCBDirty(vcb)) {
    	if (vcb->vcbSigWord == kHFSPlusSigWord)
    		error = hfs_flushvolumeheader(hfsmp, waitfor);
    	else
    		error = hfs_flushMDB(hfsmp, waitfor);
    	
        if (error)
            allerror = error;
    };

    return (allerror);
}


/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the hfsnode number is valid
 * - call hfs_vget() to get the locked hfsnode
 * - check for an unallocated hfsnode (i_mode == 0)
 * - check that the given client host has export rights and return
 *   those rights via. exflagsp and credanonp
 */
int
hfs_fhtovp(mp, fhp, nam, vpp, exflagsp, credanonp)
register struct mount *mp;
struct fid *fhp;
struct mbuf *nam;
struct vnode **vpp;
int *exflagsp;
struct ucred **credanonp;
{
    DBG_FUNC_NAME("hfs_fhtovp");
    DBG_PRINT_FUNC_NAME();

    return (EOPNOTSUPP);
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
static int hfs_vptofh(vp, fhp)
struct vnode *vp;
struct fid *fhp;
{
    DBG_FUNC_NAME("hfs_vptofh");
    DBG_PRINT_FUNC_NAME();

    return (EOPNOTSUPP);
}



/*
 * Initial HFS filesystems, done only once.
 */
int
hfs_init(vfsp)
struct vfsconf *vfsp;
{
    int i;
    static int done = 0;
    OSErr err;

    DBG_FUNC_NAME("hfs_init");
    DBG_PRINT_FUNC_NAME();

    if (done)
        return (0);
    done = 1;
    hfs_vhashinit();

    simple_lock_init (&gBufferPtrListLock);

    for (i = BUFFERPTRLISTSIZE - 1; i >= 0; --i) {
        gBufferAddress[i] = NULL;
        gBufferHeaderPtr[i] = NULL;
    };
    gBufferListIndex = 0;

    /*
     * Do any initialization that the MacOS/Rhapsody shared code relies on
     * (normally done as part of MacOS's startup):
     */
    MALLOC(gFSMVars, FSVarsRec *, sizeof(FSVarsRec), M_HFSMNT, M_WAITOK);
    bzero(gFSMVars, sizeof(FSVarsRec));

	/*
	 * Allocate Catalog Iterator cache...
	 */
	err = InitCatalogCache();
#if DIAGNOSTIC
    if (err) PANIC("hfs_init: Error returned from InitCatalogCache() call.");
#endif
	/*
	 * XXX do we need to setup the following?
	 *
	 * GMT offset, Unicode globals, CatSearch Buffers, BTSscanner
	 */

    return E_NONE;
}

/*
 * fast filesystem related variables.
 */
static int hfs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
int *name;
u_int namelen;
void *oldp;
size_t *oldlenp;
void *newp;
size_t newlen;
struct proc *p;
{
    DBG_FUNC_NAME("hfs_sysctl");
    DBG_PRINT_FUNC_NAME();

    return (EOPNOTSUPP);
}

/*	This will return a vnode or either a directory or a data vnode based on an object id. If
 *  it is a file id, its data fork will be returned.
 */

int hfs_vget(struct mount *mp,
             void *ino,
             struct vnode **vpp)
{
    struct hfsmount 	*hfsmp;
    struct proc			*p;
    dev_t 				dev;
    hfsCatalogInfo 		catInfo;
    UInt8				forkType;
    int 				retval = E_NONE;

    DBG_VFS(("hfs_vget: ino = %ld\n", *(UInt32 *)ino));

    hfsmp = VFSTOHFS(mp);
    p = CURRENT_PROC;
    dev = hfsmp->hfs_raw_dev;
	
	/* First check to see if it is in the cache */
	/* If it is, and it is complex, check again if its data fork is there */
    *vpp = hfs_vhashget(dev, *(UInt32 *)ino, kDirCmplx);
    if (*vpp && (*vpp)->v_type == VCPLX) {
        vput(*vpp);
        *vpp = hfs_vhashget(dev, *(UInt32 *)ino, kDataFork);
	}

	/* The vnode is not in the cache, so lets make it */
    if (*vpp == NULL)
      {
       /* lock catalog b-tree */
        retval = hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_SHARED, p);
        if (retval != E_NONE) goto Err_Exit;

        retval = hfsLookup(VFSTOVCB(mp), *(UInt32 *)ino, NULL, -1, 0, &catInfo);

        /* unlock catalog b-tree */
        (void) hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_RELEASE, p);

        if (retval != E_NONE) goto Err_Exit;

        forkType = (catInfo.nodeData.nodeType == kCatalogFolderNode) ? kDirCmplx : kDataFork;
        retval = hfsGet(VFSTOVCB(mp), &catInfo, forkType, NULL, vpp);
      }

#if MACH_NBC
    if (*vpp && ((*vpp)->v_type == VREG)  && !((*vpp)->v_vm_info)){
        vm_info_init(*vpp);
    }
#endif /* MACH_NBC */

Err_Exit:

	if (retval != E_NONE) {
        DBG_VFS(("hfs_vget: Error returned of %d\n", retval));
		}
	else {
        DBG_VFS(("hfs_vget: vp = 0x%x\n", (u_int)*vpp));
		}

    return (retval);

}

short hfs_flushfiles(struct mount *mp, unsigned short flags)
{
    int  retval;

    DBG_FUNC_NAME("hfs_flushfiles");
    DBG_PRINT_FUNC_NAME();

    retval = vflush(mp, VFSTOVCB(mp)->catalogRefNum, flags);
	if (retval) return retval;
	DBG_VFS(("\tAll files except Catalog is flushed\n"));

	/* Now 'close' the catalog Btree, and then flush it */
    VRELE(VFSTOVCB(mp)->catalogRefNum);
    retval = vflush(mp, NULL, flags);
    DBG_VFS(("\tCatalog is now flushed\n"));
   return (retval);
}


short hfs_flushMDB(struct hfsmount *hfsmp, int waitfor)
{
	ExtendedVCB 			*vcb = HFSTOVCB(hfsmp);
	FCB						*fcb;
	HFSMasterDirectoryBlock	*mdb;
	struct buf 				*bp;
	int						retval;
	int                     size = kMDBSize;	/* 512 */
	size_t					namelen;

	if (vcb->vcbSigWord != kHFSSigWord)
		return EINVAL;

    ASSERT(hfsmp->hfs_devvp != NULL);

	retval = bread(hfsmp->hfs_devvp, IOBLKNOFORBLK(kMasterDirectoryBlock, size),
					IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, size), NOCRED, &bp);
	if (retval) {
	    DBG_VFS((" hfs_flushMDB bread return error! (%d)\n", retval));
		if (bp) brelse(bp);
		return retval;
	}

    ASSERT(bp != NULL);
    ASSERT(bp->b_data != NULL);
    ASSERT(bp->b_bcount == size);

	mdb = (HFSMasterDirectoryBlock *)((char *)bp->b_data + IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, size));

	VCB_LOCK(vcb);
	mdb->drCrDate	= vcb->vcbCrDate;
	mdb->drLsMod	= vcb->vcbLsMod;
	mdb->drAtrb		= vcb->vcbAtrb;
	mdb->drNmFls	= vcb->vcbNmFls;
	mdb->drAllocPtr	= vcb->nextAllocation;
	mdb->drClpSiz	= vcb->vcbClpSiz;
	mdb->drNxtCNID	= vcb->vcbNxtCNID;
	mdb->drFreeBks	= vcb->freeBlocks;

    copystr(vcb->vcbVN, &mdb->drVN[1], sizeof(mdb->drVN)-1, &namelen);
    mdb->drVN[0] = namelen-1;
    mdb->drVN[namelen] = '\0';
	
	mdb->drVolBkUp	= vcb->vcbVolBkUp;
	mdb->drVSeqNum	= vcb->vcbVSeqNum;
	mdb->drWrCnt	= vcb->vcbWrCnt;
	mdb->drNmRtDirs	= vcb->vcbNmRtDirs;
	mdb->drFilCnt	= vcb->vcbFilCnt;
	mdb->drDirCnt	= vcb->vcbDirCnt;
	
	bcopy(vcb->vcbFndrInfo, mdb->drFndrInfo, sizeof(mdb->drFndrInfo));

	fcb = VTOFCB(vcb->extentsRefNum);
	bcopy(fcb->fcbExtRec, mdb->drXTExtRec, sizeof(HFSExtentRecord));
	mdb->drXTFlSize	= fcb->fcbPLen;
	mdb->drXTClpSiz	= fcb->fcbClmpSize;
	
	fcb = VTOFCB(vcb->catalogRefNum);
	bcopy(fcb->fcbExtRec, mdb->drCTExtRec, sizeof(HFSExtentRecord));
	mdb->drCTFlSize	= fcb->fcbPLen;
	mdb->drCTClpSiz	= fcb->fcbClmpSize;
	VCB_UNLOCK(vcb);


    if (waitfor != MNT_WAIT)
		bawrite(bp);
    else 
		retval = bwrite(bp);
 
	MarkVCBClean( vcb );

	return (retval);
}


short hfs_flushvolumeheader(struct hfsmount *hfsmp, int waitfor)
{
    ExtendedVCB 			*vcb = HFSTOVCB(hfsmp);
    ExtendedFCB				*extendedFCB;
    FCB						*fcb;
    HFSPlusVolumeHeader		*volumeHeader;
	struct vnode			*vp;
    int						retval;
    int                     size = sizeof(HFSPlusVolumeHeader);
    struct buf 				*bp;

	if (vcb->vcbSigWord != kHFSPlusSigWord)
		return EINVAL;

	retval = bread(hfsmp->hfs_devvp, IOBLKNOFORBLK((vcb->hfsPlusIOPosOffset / 512) + kMasterDirectoryBlock, size),
					IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, size), NOCRED, &bp);
	if (retval) {
	    DBG_VFS((" hfs_flushvolumeheader bread return error! (%d)\n", retval));
		if (bp) brelse(bp);
		return retval;
	}

    ASSERT(bp != NULL);
    ASSERT(bp->b_data != NULL);
    ASSERT(bp->b_bcount == size);

	volumeHeader = (HFSPlusVolumeHeader *)((char *)bp->b_data +
					IOBYTEOFFSETFORBLK((vcb->hfsPlusIOPosOffset / 512) + kMasterDirectoryBlock, size));

	/*
	 * For embedded HFS+ volumes, update create date if neccessary
	 */
	if (vcb->hfsPlusIOPosOffset != 0 && volumeHeader->createDate != vcb->vcbCrDate)
	  {
		struct buf 				*bp2;
		HFSMasterDirectoryBlock	*mdb;

		retval = bread(hfsmp->hfs_devvp, IOBLKNOFORBLK(kMasterDirectoryBlock, kMDBSize),
						IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, kMDBSize), NOCRED, &bp2);
		if (retval != E_NONE) {
			if (bp2) brelse(bp2);
		} else {
			mdb = (HFSMasterDirectoryBlock *)((char *)bp2->b_data + IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, kMDBSize));
			if ( mdb->drCrDate != vcb->vcbCrDate )
			  {
				mdb->drCrDate = vcb->vcbCrDate;		/* ppick up the new create date */
				(void) bwrite(bp2);					/* write out the changes */
			  }
			else
			  {
				brelse(bp2);						/* just release it */
			  }
		  }	
	  }

	VCB_LOCK(vcb);
	/* Note: only update the lower 16 bits worth of attributes */
	volumeHeader->attributes		 =	(volumeHeader->attributes & 0xFFFF0000) + (UInt16) vcb->vcbAtrb;
	volumeHeader->lastMountedVersion = kHFSPlusMountVersion;
	volumeHeader->createDate		 =	vcb->vcbCrDate;
	volumeHeader->modifyDate		 =	LocalToUTC(vcb->vcbLsMod);
	volumeHeader->backupDate		 =	LocalToUTC(vcb->vcbVolBkUp);
	volumeHeader->checkedDate		 =	vcb->checkedDate;
	volumeHeader->fileCount			 =	vcb->vcbFilCnt;
	volumeHeader->folderCount		 =	vcb->vcbDirCnt;
//	volumeHeader->blockSize			 =	vcb->blockSize;
//	volumeHeader->totalBlocks		 =	vcb->totalBlocks;
	volumeHeader->freeBlocks		 =	vcb->freeBlocks;
	volumeHeader->nextAllocation	 =	vcb->nextAllocation;
	volumeHeader->rsrcClumpSize		 =	vcb->vcbClpSiz;
	volumeHeader->dataClumpSize		 =	vcb->vcbClpSiz;
	volumeHeader->nextCatalogID		 =	vcb->vcbNxtCNID;
	volumeHeader->writeCount		 =	vcb->vcbWrCnt;
	volumeHeader->encodingsBitmap	 =	vcb->encodingsBitmap;

	//XXX djb should we use the vcb or fcb clumpSize values?
	volumeHeader->allocationFile.clumpSize	= vcb->allocationsClumpSize;
	volumeHeader->extentsFile.clumpSize		= vcb->vcbXTClpSiz;
	volumeHeader->catalogFile.clumpSize		= vcb->vcbCTClpSiz;

	bcopy( vcb->vcbFndrInfo, volumeHeader->finderInfo, sizeof(volumeHeader->finderInfo) );

	VCB_UNLOCK(vcb);

	vp = vcb->extentsRefNum;
	extendedFCB = GetParallelFCB(vp);
	bcopy( extendedFCB->extents, volumeHeader->extentsFile.extents, sizeof(HFSPlusExtentRecord) );
	fcb = VTOFCB(vp);
	volumeHeader->extentsFile.logicalSize.lo = fcb->fcbEOF;
	volumeHeader->extentsFile.logicalSize.hi = 0;
	volumeHeader->extentsFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;

	vp = vcb->catalogRefNum;
	extendedFCB = GetParallelFCB(vp);
	bcopy( extendedFCB->extents, volumeHeader->catalogFile.extents, sizeof(HFSPlusExtentRecord) );
	fcb = VTOFCB(vp);
	volumeHeader->catalogFile.logicalSize.lo = fcb->fcbPLen;
	volumeHeader->catalogFile.logicalSize.hi = 0;
	volumeHeader->catalogFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;

	vp = vcb->allocationsRefNum;
	extendedFCB = GetParallelFCB(vp);
	bcopy( extendedFCB->extents, volumeHeader->allocationFile.extents, sizeof(HFSPlusExtentRecord) );
	fcb = VTOFCB(vp);
	volumeHeader->allocationFile.logicalSize.lo = fcb->fcbPLen;
	volumeHeader->allocationFile.logicalSize.hi = 0;
	volumeHeader->allocationFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;

	vp = vcb->attributesRefNum;
	if (vp != 0)		//	Only update fields if an attributes file existed and was open
	  {
		extendedFCB = GetParallelFCB(vp);
		bcopy( extendedFCB->extents, volumeHeader->attributesFile.extents, sizeof(HFSPlusExtentRecord) );
		fcb = VTOFCB(vp);
		volumeHeader->attributesFile.logicalSize.lo = fcb->fcbPLen;
		volumeHeader->attributesFile.logicalSize.hi = 0;
		volumeHeader->attributesFile.clumpSize = fcb->fcbClmpSize;
		volumeHeader->attributesFile.totalBlocks = fcb->fcbPLen / vcb->blockSize;
	  }

    if (waitfor != MNT_WAIT)
        bawrite(bp);
    else 
		retval = bwrite(bp);
 
	MarkVCBClean( vcb );

	return (retval);
}


/*
 *      Moved here to avoid having to define prototypes
 */

/*
 * hfs vfs operations.
 */
struct vfsops hfs_vfsops = {
    hfs_mount,
    hfs_start,
    hfs_unmount,
    hfs_root,
    hfs_quotactl,
    hfs_statfs,
    hfs_sync,
    hfs_vget,
    hfs_fhtovp,
    hfs_vptofh,
    hfs_init,
    hfs_sysctl
};

void
load_hfs(int loadArgument) {
    struct vfsconf *vfsconflistentry;
    int entriesRemaining;
    struct vfsconf *newvfsconf = NULL;
    struct vfsconf *lastentry = NULL;
    int j;
    int (***opv_desc_vector_p)();
    int (**opv_desc_vector)();
    struct vnodeopv_entry_desc *opve_descp;

#pragma unused(loadArgument)

    /*
     * This routine is responsible for all the initialization that would
     * ordinarily be done as part of the system startup; it calls hfs_init
     * to do the initialization that is strictly HFS-specific.
     */

#if DIAGNOSTIC
//      call_kdp();
#endif
    DBG_LOAD(("load_hfs: Starting...\n"));

    /*
     prevvfsconf is supposed to be the entry preceding the new entry.
     To make sure we can always get hooked in SOMEWHERE in the list,
     start it out at the first entry of the list.  This assumes the
     first entry in the list will be non-empty and not HFS.

     This becomes irrelevant when HFS is compiled into the list.
     */
    DBG_LOAD(("load_hfs: Scanning vfsconf list...\n"));
    vfsconflistentry = vfsconf;
    for (entriesRemaining = maxvfsslots; entriesRemaining > 0; --entriesRemaining) {
        if (vfsconflistentry->vfc_vfsops != NULL) {
            /*
             * Check to see if we're reloading a new version of hfs during debugging
             * and overwrite the previously assigned entry if we find one:
             */
            if (strcmp(vfsconflistentry->vfc_name, hfs_fs_name) == 0) {
                newvfsconf = vfsconflistentry;
                break;
            } else {
                lastentry = vfsconflistentry;
            };
        } else {
            /*
             * This is at least a POSSIBLE place to insert the new entry...
             */
            newvfsconf = vfsconflistentry;
        };
        ++vfsconflistentry;
    };

    if (newvfsconf) {
        DBG_LOAD(("load_hfs: filling in vfsconf entry at 0x%X; lastentry = 0x%X.\n", (u_int)newvfsconf, (u_int)lastentry));
        newvfsconf->vfc_vfsops = &hfs_vfsops;
        strncpy(&newvfsconf->vfc_name[0], hfs_fs_name, MFSNAMELEN);
        newvfsconf->vfc_typenum = maxvfsconf++;
        newvfsconf->vfc_refcount = 0;
        newvfsconf->vfc_flags = 0;
        newvfsconf->vfc_mountroot = NULL;       /* Can't mount root of file system [yet] */

        /* Hook into the list: */
        newvfsconf->vfc_next = NULL;
        if (lastentry) {
            newvfsconf->vfc_next = lastentry->vfc_next;
            lastentry->vfc_next = newvfsconf;
        };

        /* Based on vfs_op_init and ... */
        opv_desc_vector_p = hfs_vnodeop_opv_desc.opv_desc_vector_p;

        DBG_LOAD(("load_hfs: Allocating and initializing VNode ops vector...\n"));

        /*
         * Allocate and init the vector.
         * Also handle backwards compatibility.
         */
        MALLOC(*opv_desc_vector_p, PFI *, vfs_opv_numops*sizeof(PFI), M_TEMP, M_WAITOK);
        bzero (*opv_desc_vector_p, vfs_opv_numops*sizeof(PFI));

        opv_desc_vector = *opv_desc_vector_p;
        for (j=0; hfs_vnodeop_opv_desc.opv_desc_ops[j].opve_op; j++) {
            opve_descp = &(hfs_vnodeop_opv_desc.opv_desc_ops[j]);

            /*
             * Sanity check:  is this operation listed
             * in the list of operations?  We check this
             * by seeing if its offest is zero.  Since
             * the default routine should always be listed
             * first, it should be the only one with a zero
             * offset.  Any other operation with a zero
             * offset is probably not listed in
             * vfs_op_descs, and so is probably an error.
             *
             * A panic here means the layer programmer
             * has committed the all-too common bug
             * of adding a new operation to the layer's
             * list of vnode operations but
             * not adding the operation to the system-wide
             * list of supported operations.
             */
            if (opve_descp->opve_op->vdesc_offset == 0 &&
                opve_descp->opve_op->vdesc_offset != VOFFSET(vop_default)) {
                printf("load_hfs: operation %s not listed in %s.\n",
                       opve_descp->opve_op->vdesc_name,
                       "vfs_op_descs");
                panic ("load_hfs: bad operation");
            }
            /*
             * Fill in this entry.
             */
            opv_desc_vector[opve_descp->opve_op->vdesc_offset] =
                opve_descp->opve_impl;
        }

        /*
         * Finally, go back and replace unfilled routines
         * with their default.  (Sigh, an O(n^3) algorithm.  I
                                 * could make it better, but that'd be work, and n is small.)
         */
        opv_desc_vector_p = hfs_vnodeop_opv_desc.opv_desc_vector_p;

        /*
         * Force every operations vector to have a default routine.
         */
        opv_desc_vector = *opv_desc_vector_p;
        if (opv_desc_vector[VOFFSET(vop_default)]==NULL) {
            panic("load_hfs: operation vector without default routine.");
        }
        for (j = 0;j<vfs_opv_numops; j++)
            if (opv_desc_vector[j] == NULL)
                opv_desc_vector[j] =
                    opv_desc_vector[VOFFSET(vop_default)];

        DBG_LOAD(("load_hfs: calling hfs_init()...\n"));
        hfs_init(newvfsconf);
    };
}

void
unload_hfs(int unloadArgument) {
    int entriesRemaining;
    struct vfsconf *vfsconflistentry;
    struct vfsconf *prevconf = NULL;
    struct vfsconf *hfsconf = NULL;

    DBG_LOAD(("unload_hfs: removing HFS from vfs conf. list...\n"));

    prevconf = vfsconflistentry = vfsconf;
    for (entriesRemaining = maxvfsslots;
         (entriesRemaining > 0) && (vfsconflistentry != NULL);
         --entriesRemaining) {
        if ((vfsconflistentry->vfc_vfsops != NULL) && (strcmp(vfsconflistentry->vfc_name, hfs_fs_name) == 0)) {
            hfsconf = vfsconflistentry;
            break;
        };
        prevconf = vfsconflistentry;
        vfsconflistentry = vfsconflistentry->vfc_next;
    };

    if (hfsconf != NULL) {
        if (prevconf != NULL) {
            /* Unlink the HFS entry from the list: */
            prevconf->vfc_next = hfsconf->vfc_next;
        } else {
            DBG_LOAD(("unload_hfs: no previous entry in list.\n"));
        }
    } else {
        DBG_LOAD(("unload_hfs: couldn't find hfs conf. list entry.\n"));
    };

    FREE(*hfs_vnodeop_opv_desc.opv_desc_vector_p, M_TEMP);
}
