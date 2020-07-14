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
 * Copyright (c) 1989, 1993
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
 *	@(#)hfs_lookup.c	1.0
 *	derived from @(#)ufs_lookup.c	8.15 (Berkeley) 6/16/95
 *
 *	(c) 1998       Apple Computer, Inc.  All Rights Reserved
 *	(c) 1990, 1992 NeXT Computer, Inc.  All Rights Reserved
 *	
 *
 *	hfs_lookup.c -- code to handle directory traversal on HFS/HFS+ volume
 *
 *	MODIFICATION HISTORY:
 *      25-Feb-1999 Clark Warner	Fixed the error case of VFS_VGGET when
 *                                      processing DotDot (..) to relock parent
 *	23-Feb-1999 Pat Dirks		Finish cleanup around Don's last fix in "." and ".." handling.
 *	11-Nov-1998	Don Brady		Take out VFS_VGET that got added as part of previous fix.
 *	14-Oct-1998	Don Brady		Fix locking policy volation in hfs_lookup for ".." case
 *								(radar #2279902).
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

#include	"hfs.h"
#include	"hfs_dbg.h"
#include	"hfscommon/headers/FileMgrInternal.h"
#include	"hfscommon/headers/CatalogPrivate.h"

extern void cache_purge (struct vnode *vp);
extern int cache_lookup (struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
extern void cache_enter (struct vnode *dvp, struct vnode *vpp, struct componentname *cnp);

#if DBG_VOP_TEST_LOCKS
extern void DbgVopTest(int maxSlots, int retval, VopDbgStoreRec *VopDbgStore, char *funcname);
#endif

/*****************************************************************************
*
*	Operations on vnodes
*
*****************************************************************************/

/* From FreeBSD 2.2 MSDOS_LOOKUP
 * When we search a directory the blocks containing directory entries are
 * read and examined.  The directory entries contain information that would
 * normally be in the hfsnode of a unix filesystem.  This means that some of
 * a directory's contents may also be in memory resident denodes (sort of
 * an hfsnode).  This can cause problems if we are searching while some other
 * process is modifying a directory.  To prevent one process from accessing
 * incompletely modified directory information we depend upon being the
 * soul owner of a directory block.  bread/brelse provide this service.
 * This being the case, when a process modifies a directory it must first
 * acquire the disk block that contains the directory entry to be modified.
 * Then update the disk block and the denode, and then write the disk block
 * out to disk.  This way disk blocks containing directory entries and in
 * memory denode's will be in synch.
 */

/*	FROM UNIX 63
 * Convert a component of a pathname into a pointer to a locked hfsnode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation (see comments in code below).
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it, the parent directory is returned
 * locked. If flag has WANTPARENT or'ed into it, the parent directory is
 * returned unlocked. Otherwise the parent directory is not returned. If
 * the target of the pathname exists and LOCKLEAF is or'ed into the flag
 * the target is returned locked, otherwise it is returned unlocked.
 * When creating or renaming and LOCKPARENT is specified, the target may not
 * be ".".  When deleting and LOCKPARENT is specified, the target may be ".".
 *
 * Overall outline of hfs_lookup:
 *
 *	check accessibility of directory
 *	look for name in cache, if found, then if at end of path
 *	  and deleting or creating, drop it, else return name
 *	search for name in directory, to found or notfound
 * DoesntExist:
 *	if creating, return locked directory, leaving info on available slots
 *	else return retval
 * Exists:
 *	if at end of path and deleting, return information to allow delete
 *	if at end of path and rewriting (RENAME and LOCKPARENT), lock target
 *	  hfsnode and return info to allow rewrite
 *	if not at end, add name to cache; if at end and neither creating
 *	  nor deleting, add name to cache
 *
 * NOTE: (LOOKUP | LOCKPARENT) currently returns the parent hfsnode unlocked.
 */

/*	
 *	Lookup *nm in directory *pvp, return it in *a_vpp.
 *	**a_vpp is held on exit.
 *	We create a hfsnode for the file, but we do NOT open the file here.

#% lookup	dvp	L ? ?
#% lookup	vpp	- L -

	IN struct vnode *dvp - Parent node of file;
	INOUT struct vnode **vpp - node of target file, its a new node if the target vnode did not exist;
	IN struct componentname *cnp - Name of file;

 *	When should we lock parent_hp in here ??
 */

int
hfs_lookup(ap)
struct vop_lookup_args /* {
    struct vnode *a_dvp;
    struct vnode **a_vpp;
    struct componentname *a_cnp;
} */ *ap;
{
    struct vnode					*parent_vp;
    struct vnode 					*target_vp;
    struct vnode					*tparent_vp;
    struct hfsnode 					*parent_hp;				/* parent */
    struct hfsnode 					*target_hp;				/* target */
    struct hfsmount 				*parent_hfs;
    struct componentname 			*cnp;
    struct ucred 					*cred;
    struct proc						*p;
    struct hfsCatalogInfo			catInfo;
    u_long							parent_id;
    u_long 							lockparent;						/* !0 => lockparent flag is set */
    u_long 							wantparent;						/* !0 => wantparent or lockparent flag */
    u_short							targetLen;
    int 							nameiop;
    int								retval;
    u_char							isDot, isDotDot;
    UInt32							nodeID;
    UInt16							forkType;
    DBG_FUNC_NAME("lookup");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_POS);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_CONT(("\tTarget: "));DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));
    DBG_HFS_NODE_CHECK(ap->a_dvp);
    DBG_ASSERT(ap->a_dvp!=NULL);

    
    /*
     * Do initial setup
     */
    parent_vp 		= ap->a_dvp;
    cnp 			= ap->a_cnp;
    parent_hp 		= VTOH(parent_vp);				/* parent */
    target_vp 		= NULL;
    parent_hfs 		= VTOHFS(parent_vp);
    targetLen 		= cnp->cn_namelen;
    nameiop 		= cnp->cn_nameiop;
    cred 			= cnp->cn_cred;
    p 				= cnp->cn_proc;
    lockparent 		= cnp->cn_flags & LOCKPARENT;
    wantparent 		= cnp->cn_flags & (LOCKPARENT|WANTPARENT);
    parent_id 		= H_FILEID(parent_hp);
    nodeID			= kUnknownID;
    isDot 			= FALSE;
    isDotDot		= cnp->cn_flags & ISDOTDOT;
    catInfo.hint 	= 0;
    retval 			= E_NONE;
   	forkType 		= kDirCmplx;


    /*
     * Check accessiblity of directory.
     */
    if ((parent_vp->v_type != VDIR) && (parent_vp->v_type != VCPLX)) {
        DBG_ERR(("%s: Parent VNode '%s' is not a directory, its %d\n",funcname, H_NAME(parent_hp), parent_vp->v_type));
        retval = ENOTDIR;
        goto Err_Exit;
    };
	
    /*
     * Check accessiblity of directory. Only if it is non complex, since it would of been checked
	 * 	when looking up the complex node. The 'first' lookup's parent should alsways be a directory
     */
    if ((parent_vp->v_type != VCPLX) && ((retval = VOP_ACCESS(parent_vp, VEXEC, cred, p)) != E_NONE)) {
        DBG_ERR(("%s: No acces to parent %s, %s\n",funcname, H_NAME(parent_hp), ap->a_cnp->cn_nameptr));
        goto Err_Exit;
    };

	/* Check for accessibility of the file...is this a read-only volume */
    if ((cnp->cn_flags & ISLASTCN) && (VTOVFS(parent_vp)->mnt_flag & MNT_RDONLY) &&
             (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)) {
        DBG_ERR(("%s: Invalid cn_nameiop for read-only volume %s, %s\n",funcname, H_NAME(parent_hp), ap->a_cnp->cn_nameptr));
        retval = EROFS;
        goto Err_Exit;
    };

    /*
     * We now have a segment name to search for, and a directory to search.
     *
     * Before tediously performing a linear scan of the directory,
     * check the name cache to see if the directory/name pair
     * we are looking for is known already.
     */
    retval = cache_lookup(parent_vp, &target_vp, cnp);
    if (retval != E_NONE) {
        u_long vpid;	/* capability number of vnode */

        DBG_VOP(("\tFound in name cache: "));
        if (retval == ENOENT) {
            DBG_VOP_CONT((" As a non-entry\n"));
            goto Err_Exit;
		}
        DBG_VOP_PRINT_VNODE_INFO(target_vp);DBG_VOP_CONT(("\n"));
        /*
         * Get the next vnode in the path.
         * See comment below starting `Step through' for
         * an explaination of the locking protocol.
         */
        tparent_vp 	= parent_vp;
        parent_vp	= target_vp;
        vpid 		= target_vp->v_id;
        if (tparent_vp == parent_vp) {					/* Lookup on '.' */
            VREF(parent_vp);
            retval = E_NONE;
        } else if (cnp->cn_flags & ISDOTDOT) {   		/* If getting our parent */
            VOP_UNLOCK(tparent_vp, 0, p);
            retval = vget(parent_vp, LK_EXCLUSIVE, p);
            if (!retval && lockparent && (cnp->cn_flags & ISLASTCN))
                retval = vn_lock(tparent_vp, LK_EXCLUSIVE, p);
        } else {
            retval = vget(parent_vp, LK_EXCLUSIVE, p);
            if ((!retval) && (parent_vp->v_type == VCPLX) && (cnp->cn_flags & ISLASTCN)) {
                target_vp = VTOH(parent_vp)->h_relative;
                VPUT(parent_vp);
                if (target_vp == NULL) {
                    parent_vp = tparent_vp;
                    goto Skip_NameCache;
                }
                parent_vp = target_vp;	/* do the 'skipping' */
                vpid = parent_vp->v_id;
                retval = vget(parent_vp, LK_EXCLUSIVE, p);
            };
            if (!lockparent || retval || !(cnp->cn_flags & ISLASTCN))
                VOP_UNLOCK(tparent_vp, 0, p);
        };

        /*
         * Check that the node number did not change
         * while we were waiting for the lock.
         */
        if (retval == E_NONE) {
            if (vpid == parent_vp->v_id) {
                goto Err_Exit;		/* Did not change, Success! We have the node..we are finished */
            };

            VPUT(parent_vp);
            if (lockparent && tparent_vp != parent_vp && (cnp->cn_flags & ISLASTCN))
                VOP_UNLOCK(tparent_vp, 0, p);
        };

        retval = vn_lock(tparent_vp, LK_EXCLUSIVE, p);
        if (retval) goto Err_Exit;

        parent_vp = tparent_vp;
        target_vp = NULL;
    };

Skip_NameCache:	;


    /*
     *	Trivial cases for current and parent directories
     */
	if(cnp->cn_nameptr[0] == (char)'.') {
		/* . */
        if(targetLen == 1) {										/* self */
            target_vp = parent_vp;
            nodeID = H_FILEID(parent_hp);
            isDot = TRUE;
            catInfo.nodeData.nodeType = kCatalogFolderNode;
            goto Exists;
        };

		/* .. */
		if((cnp->cn_nameptr[1] == '.') && (targetLen == 2)) {			/* parent */
            isDotDot = TRUE;				/* XXX Make doubly-sure this is set for now */
            nodeID = H_DIRID(parent_hp);
            VOP_UNLOCK(parent_vp, 0, p);
            retval = VFS_VGET(HTOVFS(parent_hp), &nodeID, &target_vp);
	    if (retval) {
                        vn_lock(parent_vp, LK_EXCLUSIVE | LK_RETRY, p);
                        goto Err_Exit;
                        }
          
            if (lockparent && (cnp->cn_flags & ISLASTCN)) {
                 retval = vn_lock(parent_vp, LK_EXCLUSIVE, p);
                 if (retval) {
                             VPUT(target_vp);
               		     goto Err_Exit;
          	}
            }	 

            if (target_vp == NULL) {
                DBG_ERR(("%s: VFS_VGET FAILED on getting dir %ld: %d\n",funcname, H_DIRID(parent_hp), retval));
                goto Err_Exit;
            } else {
                target_hp = VTOH(target_vp);
                nodeID = H_FILEID(target_hp);
                catInfo.nodeData.nodeType = kCatalogFolderNode;
               goto Exists;
            };
        };
    };


    /*
    * The file is not in the cache, nor is it a dot (or dot-dot), so do a hfs lookup
    */

    /* lock catalog b-tree */
	retval = hfs_metafilelocking(parent_hfs, kHFSCatalogFileID, LK_SHARED, p);	/*XXX p might be wrongone! */
    if (retval)
        goto Err_Exit;
    
    if (parent_vp->v_type == VCPLX) {
        retval = hfsLookup (HFSTOVCB(parent_hfs), H_DIRID(parent_hp), H_NAME(parent_hp), -1, H_HINT(parent_hp), &catInfo);
        }
    else {
        retval = hfsLookup (HFSTOVCB(parent_hfs), parent_id, cnp->cn_nameptr, targetLen, kNoHint, &catInfo);
        }

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(parent_hfs, kHFSCatalogFileID, LK_RELEASE, p);

	nodeID = catInfo.nodeData.nodeID;
    if (retval == E_NONE) {
    	goto Exists;
    } else {
   		goto DoesntExist;
   	};
    

DoesntExist:	;

/*
 * If we get here we didn't find the entry we were looking for. But
 * that's ok if we are creating or renaming and are at the end of
 * the pathname and the directory hasn't been removed.
 */
    retval = E_NONE;
	if ((nameiop == CREATE || nameiop == RENAME) && (cnp->cn_flags & ISLASTCN)) {
        retval = VOP_ACCESS(parent_vp, VWRITE, cred, p);
        if (retval)
            goto Err_Exit;

        cnp->cn_flags |= SAVENAME;
        if (!lockparent)/* leave searched dir locked?	 */
            VOP_UNLOCK(parent_vp, 0, p);
        retval = EJUSTRETURN;
        goto Err_Exit;
    }

    /*
    * XXX SER - Here we would store the name in cache as non-existant if not trying to create it, but,
	* the name cache IS case-sensitive, thus maybe showing a negative hit, when the name
	* is only different by case. So hfs does not support negative caching. Something to look at.
	* (See radar 2293594 for a failed example)
    */

    retval = ENOENT;
    goto Err_Exit;


Exists:	;
	
    DBG_ASSERT(nodeID != kUnknownID);
    retval = E_NONE;
	
    if (target_vp) {
        forkType = H_FORKTYPE(VTOH(target_vp));
    } else {
        /*
         * Here we have to decide what type of vnode to create
         * If the parent is a directory, then the child can be one of four types:
         * 1. VDIR, nodeType is kCatalogFolderNode, forkType will be kDirCmplx
         * 2. VLINK, nodeType is kCatalogFileNode, forkType will be kDataFork
         * 3. VREG, nodeType is kCatalogFileNode, forkType will be kDataFork
         * 4. VCPLX, nodeType is kCatalogFileNode, forkType will be kDirCmplx
         * To determine which of the latter three, we can use this algorithm:
         * a. VREG iff ISLASTCN is set (as in the case of the default fork e.g. data/foo).
         * b. VLINK iff the mode is IFLNK (esp. if it is a link to a directory e.g. bar/link/foo)
         * c. VCPLX iff it is followed by a valid fork name. (foo/data)
         * If the parent is a complex, then the child must have a valid fork name
         */

        if (parent_vp->v_type == VDIR) {
            if (catInfo.nodeData.nodeType == kCatalogFolderNode)
                forkType = kDirCmplx;
            else if ((cnp->cn_flags & ISLASTCN) || ((catInfo.nodeData.permissions.permissions & IFMT)==IFLNK)) {
                forkType = kDataFork;			/* The defualt */
                //cnp->cn_flags &= ~MAKEENTRY;
            }
            else
                forkType = kDirCmplx;			/* complex */
        }
        else if (parent_vp->v_type == VCPLX) {
            if (! strcmp(cnp->cn_nameptr, kDataForkNameStr))
                forkType = kDataFork;
            else if (! strcmp(cnp->cn_nameptr, kRsrcForkNameStr))
                forkType = kRsrcFork;
            else {
                if (!lockparent)					/* leave parent dir locked?	 */
                    VOP_UNLOCK(parent_vp, 0, p);
                DBG_ERR(("%s: Searching for an illegal fork: %s\n", funcname, cnp->cn_nameptr));
                retval =  ENOENT;
                goto Err_Exit;
            }
        }
        else
            panic("Unexpected parent type for lookup");
        DBG_VOP(("\tExists: Parent v_type:%d, forkType:%d, target nodeType:0x%x\n", parent_vp->v_type, forkType, (u_int)catInfo.nodeData.nodeType));
    };
	
    
   /*
    * If deleting and at the end of the path, then if we matched on
    * "." then don't hfsget() Otherwise
    * hfsget() the directory entry.
    */
    /*
    * If deleting, and at end of pathname, return
    * parameters which can be used to remove file.
    * If the wantparent flag isn't set, we return only
    * the directory (in ndp->ndvp), otherwise we go
    * on and lock the hfsnode, being careful with ".".
    */
	if (nameiop == DELETE && (cnp->cn_flags & ISLASTCN)) {
        /*
        * Write access to directory required to delete files.
        */
        retval = VOP_ACCESS(parent_vp, VWRITE, cred, p);
        if (retval)
            goto Err_Exit;

        if (isDot) {
            VREF(parent_vp);
            target_vp = parent_vp;
            goto Err_Exit;
            }
        else if (target_vp == NULL) {
            target_vp = hfs_vhashget(parent_hp->h_dev, nodeID, forkType);
            if (target_vp == NULL)
                retval = hfsGet (HTOVCB(parent_hp), &catInfo, forkType, parent_vp, &target_vp);
            if (retval != E_NONE)	
                goto Err_Exit;
        }
        /*
         * If directory is "sticky", then user must own
         * the directory, or the file in it, else she
         * may not delete it (unless she's root). This
         * implements append-only directories.
         */
        if ((parent_hp->h_meta->h_mode & ISVTX) &&
            	cred->cr_uid != 0 &&
            	cred->cr_uid != parent_hp->h_meta->h_uid &&
            	target_vp->v_type != VLNK &&
            	VTOH(target_vp)->h_meta->h_uid != cred->cr_uid) {
            VPUT(target_vp);
            retval = EPERM;
            goto Err_Exit;
        }
  
        if (!lockparent && !isDotDot)
            VOP_UNLOCK(parent_vp, 0, p);
        goto Err_Exit;
     };

    /*
     * If rewriting 'RENAME', return the hfsnode and the
     * information required to rewrite the present directory
     */
    if (nameiop == RENAME && wantparent && (cnp->cn_flags & ISLASTCN)) {

        /* cant change root */
        if ((parent_id == kRootDirID) && isDot) {
            retval = EISDIR;
            goto Err_Exit;
        };

        if (target_vp == NULL) {

            /* Make sure that this is the correct name ie. same case */
            if (strncmp(cnp->cn_nameptr, catInfo.spec.name, targetLen)) {
                if (!lockparent)/* leave searched dir locked?	 */
                    VOP_UNLOCK(parent_vp, 0, p);
                retval = EJUSTRETURN;
                goto Err_Exit;
            };
			
			/* Create the vnode, looking in the cache first */
            target_vp = hfs_vhashget(parent_hp->h_dev, nodeID, forkType);
            if (target_vp == NULL)
                retval = hfsGet (HTOVCB(parent_hp), &catInfo, forkType, parent_vp, &target_vp);
            if (retval != E_NONE)
                goto Err_Exit;
        };

        if (!lockparent && !isDotDot) VOP_UNLOCK(parent_vp, 0, p);
        cnp->cn_flags |= SAVENAME;
        goto Err_Exit;
		/* Finished...all is well, goto the end */
     };

    /*
     * Step through the translation in the name.  We do not `vput' the
     * directory because we may need it again if a symbolic link
     * is relative to the current directory.  Instead we save it
     * unlocked as "tparent_vp".  We must get the target hfsnode before unlocking
     * the directory to insure that the hfsnode will not be removed
     * before we get it.  We prevent deadlock by always fetching
     * inodes from the root, moving down the directory tree. Thus
     * when following backward pointers ".." we must unlock the
     * parent directory before getting the requested directory.
     * There is a potential race condition here if both the current
     * and parent directories are removed before the VFS_VGET for the
     * hfsnode associated with ".." returns.  We hope that this occurs
     * infrequently since we cannot avoid this race condition without
     * implementing a sophisticated deadlock detection algorithm.
     * Note also that this simple deadlock detection scheme will not
     * work if the file system has any hard links other than ".."
     * that point backwards in the directory structure.
     */

    tparent_vp = parent_vp;
	if (cnp->cn_flags & ISDOTDOT) {
#if 0
		VOP_UNLOCK(tparent_vp, 0, p);
        /* radar #2286192
         * UFS does a VFS_VGET here but the code below is causing a hang so
         * I'm taking it back out for now - DJB
         */
		if ((retval = VFS_VGET(parent_vp->v_mount, &H_FILEID(parent_hp), &target_vp))) {
			vn_lock(tparent_vp, LK_EXCLUSIVE | LK_RETRY, p);
			goto Err_Exit;
		}
		if (lockparent && (cnp->cn_flags & ISLASTCN)
			&& (retval = vn_lock(tparent_vp, LK_EXCLUSIVE | LK_RETRY, p))) {
                VPUT(target_vp);
                goto Err_Exit;
		}
#endif
    } else if (isDot) {		/* "." */
        VREF(parent_vp);
        target_vp = parent_vp;
    }  else {
        if (target_vp == NULL)
        	target_vp = hfs_vhashget(parent_hp->h_dev, nodeID, forkType);
        if (target_vp == NULL)
            retval = hfsGet(HTOVCB(parent_hp), &catInfo, forkType, parent_vp, &target_vp);
        if (retval)
            goto Err_Exit;
        if (!lockparent || !(cnp->cn_flags & ISLASTCN))
            VOP_UNLOCK(tparent_vp, 0, p);
    }

    /*
    * Insert name in cache if wanted.
    */
    if (cnp->cn_flags & MAKEENTRY) {
        struct vnode *tp;

        /* If this is bypassing the complex vnode, get it, so we only store complex vnodes */
        if (target_vp->v_type != VREG) {
            tp = target_vp;
        } else {
            if (parent_vp->v_type == VDIR) {
                tp = VTOH(target_vp)->h_relative;
            } else {
                /* Parent node is complex node [already cached in earlier lookup];
                don't add anything else in the cache now.

                XXX Someday we may want to enter data/rsrc fork vnodes in the name cache...
                */
                tp = NULL;
            };
        };
        if (tp) {
            DBG_ASSERT((tp->v_type==VDIR) || (tp->v_type==VCPLX) || (tp->v_type==VLNK));
            DBG_VOP(("\tStoring into name cache: target: 0x%x ", (u_int)target_vp)); DBG_VOP_PRINT_CPN_INFO(cnp);
            DBG_VOP_CONT((" parent: "));DBG_VOP_PRINT_VNODE_INFO(parent_vp); DBG_VOP_CONT(("\n"));
            cache_enter(parent_vp, tp, cnp);
        };
    };

Err_Exit:

    *ap->a_vpp = target_vp;

    DBG_VOP_UPDATE_VP(1, *ap->a_vpp);
	DBG_VOP_LOOKUP_TEST (funcname, cnp, parent_vp, target_vp);
    DBG_VOP_LOCKS_TEST(E_NONE);
    DBG_VOP_PRINT_FUNCNAME();
    if (retval == E_NONE) {
        DBG_VOP_CONT(("Success\n"));
    } else {
        DBG_VOP_CONT(("Fails\n"));
    }

    return (retval);
}




#if DBG_VOP_TEST_LOCKS

void DbgLookupTest(	char *funcname, struct componentname  *cnp, struct vnode *dvp, struct vnode *vp)
{
    int 		flags = cnp->cn_flags;
    int 		nameiop = cnp->cn_nameiop;

    kprintf ("%X: %s: Action is: ", CURRENT_PROC->p_pid, funcname);
    switch (nameiop)
        {
        case LOOKUP:
            kprintf ("LOOKUP");
            break;
        case CREATE:
            kprintf ("CREATE");
            break;
        case DELETE:
            kprintf ("DELETE");
            break;
        case RENAME:
            kprintf ("RENAME");
            break;
        default:
            DBG_ERR (("!!!UNKNOWN!!!!"));
            break;
            }
    kprintf (" flags: 0x%x ",flags );
    if (flags & LOCKPARENT)
        kprintf (" LOCKPARENT");
    if (flags & ISLASTCN)
        kprintf (" ISLASTCN");
    kprintf ("\n");

    if (dvp) {
        kprintf ("%X: %s: Parent vnode exited ", CURRENT_PROC->p_pid, funcname);
        if (lockstatus(&VTOH(dvp)->h_lock)) {
            kprintf ("LOCKED\n");
        }
        else {
            kprintf ("UNLOCKED\n");
        }
    }

    if (vp) {
        if (vp==dvp)
          {
            kprintf ("%X: %s: Target and Parent are the same\n", CURRENT_PROC->p_pid, funcname);
          }
        else {
            kprintf ("%X: %s: Found vnode exited ", CURRENT_PROC->p_pid, funcname);
            if (lockstatus(&VTOH(vp)->h_lock)) {
                kprintf ("LOCKED\n");
            }
            else {
                kprintf ("UNLOCKED\n");
            }
        }
        kprintf ("%X: %s: Found vnode 0x%x has vtype of %d\n ", CURRENT_PROC->p_pid, funcname, (u_int)vp, vp->v_type);
    }
    else
        kprintf ("%X: %s: Found vnode exited NULL\n", CURRENT_PROC->p_pid, funcname);


}

#endif /* DBG_VOP_TEST_LOCKS */

