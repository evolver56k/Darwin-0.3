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
 * Copyright (c) 1982, 1986, 1989, 1993, 1995
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
 *	@(#)hfs_vnodeops.c	3.0
 *	derived from @(#)ufs_vnops.c	8.27 (Berkeley) 5/27/95
 *
 *	(c) 1997-1999	Apple Computer, Inc.  All Rights Reserved
 *	(c) 1990, 1992 NeXT Computer, Inc.  All Rights Reserved
 *	
 *
 *	hfs_vnodeops.c -- vnode layer for loadable Macintosh file system
 *
 *	MODIFICATION HISTORY:
 *       1-Mar-1999	Scott Roberts	h_meta is now released when the complex vnode is relesed
 *      26-Feb-1999	Pat Dirks (copied by Chw) Fixed hfs_lookup to check for
 *                                error return on vget.
 *      25-Feb-1999     Pat Dirks       Fixed hfs_remove to use a local copy of the h_sibling pointer around vnode_uncache.
 *	 3-Feb-1999	Pat Dirks		Changed to stop updating wrapper volume name in MDB since wrapper volume's
 *								catalog isn't updated and this inconsistency trips Disk First Aid's checks.
 *	22-Jan-1999	Pat Dirks		Changed hfs_rename, hfs_remove, and hfs_rmdir to call cache_purge.
 *	22-Jan-1999	Don Brady		After calling hfsMoveRename call hfsLookup to get new name.
 *	12-Jan-1999	Don Brady		Fixed the size of ATTR_CMN_NAME buffer to NAME_MAX + 1.
 *	 8-Jan-1999	Pat Dirks		Added hfs_writepermission and change hfs_setattrlist to use it instead of
 *								including an incorrect derivative of hfs_access in-line.
 *	15-Dec-1998 Pat Dirks		Changed setattrlist to do permission checking as appropriate (Radar #2290212).
 *	17-Nov-1998 Scott Roberts	Added support for long volume names in SetAttrList().
 *	6-Nov-1998 Don Brady		Add support for UTF-8 names.
 *	 3-Nov-1998	Umesh Vaishampayan	Changes to deal with "struct timespec"
 *						change in the kernel.	
 *  21-Oct-1998 Scott Roberts	Added support for advisory locking (Radar #2237914).
 *  25-Sep-1998 Don Brady		Changed hfs_exchange to call hfs_chid after updating catalog (radar #2276605).
 *	23-Sep-1998 Don Brady		hfs_setattrlist now calls hfs_chown and hfs_chmod to change values.
 *	15-Sep-1998 Pat Dirks		Cleaned up vnode unlocking on various error exit paths and changed
 *								to use new error stub routines in place of hfs_mknod and hfs_link.
 *  16-Sep-1998	Don Brady		When renaming a volume in hfs_setattrlist, also update hfs+ wrapper name (radar #2272925).
 *   1-Sep-1998	Don Brady		Fix uninitiazed time variable in hfs_makenode (radar #2270372).
 *  31-Aug-1998	Don Brady		Adjust change time for DST in hfs_update (radar #2265075).
 *  12-Aug-1998	Don Brady		Update complex node name in hfs_rename (radar #2262111).
 *   5-Aug-1998	Don Brady		In hfs_setattrlist call MacToVFSError after calling UpdateCatalogNode (radar #2261247).
 *  21-Jul-1998	Don Brady		Fixed broken preflight in hfs_getattrlist.
 *      17-Jul-1998	Clark Warner		Fixed the one left out case of freeing M_NAMEI in hfs_abort
 *	13-Jul-1998	Don Brady		Add uio_resid preflight check to hfs_search (radar #2251855).
 *	30-Jun-1998	Scott Roberts	        Changed hfs_makenode and its callers to free M_NAMEI.
 *	29-Jun-1998	Don Brady		Fix unpacking order in UnpackSearchAttributeBlock (radar #2249248).
 *	13-Jun-1998	Scott Roberts		Integrated changes to hfs_lock (radar #2237243).
 *	 4-Jun-1998	Pat Dirks		Split off hfs_lookup.c and hfs_readwrite.c
 *	 3-Jun-1998	Don Brady		Fix hfs_rename bugs (radar #2229259, #2239823, 2231108 and #2237380).
 *								Removed extra vputs in hfs_rmdir (radar #2240309).
 *	28-May-1998	Don Brady		Fix hfs_truncate to correctly extend files (radar #2237242).
 *	20-May-1998	Don Brady		In hfs_close shrink the peof to the smallest size neccessary (radar #2230094).
 *	 5-May-1998	Don Brady		Fixed typo in hfs_rename (apply H_FILEID macro to VTOH result).
 *	29-Apr-1998	Joe Sokol		Don't do cluster I/O when logical block size is not 4K multiple.
 *	28-Apr-1998	Pat Dirks		Cleaned up unused variable physBlockNo in hfs_write.
 *	28-Apr-1998	Joe Sokol		Touched up support for cluster_read/cluster_write and enabled it.
 *	27-Apr-1998	Don Brady		Remove some DEBUG_BREAK calls in DbgVopTest.
 *	24-Apr-1998	Pat Dirks		Fixed read logic to read-ahead only ONE block, and of only logBlockSize instead of 64K...
 *								Added calls to brelse() on errors from bread[n]().
 *								Changed logic to add overall length field to AttrBlockSize only on attribute return operations.
 *	23-Apr-1998	Don Brady		The hfs_symlink call is only supported on HFS Plus disks.
 *	23-Apr-1998	Deric Horn		Fixed hfs_search bug where matches were skipped when buffer was full.
 *	22-Apr-1998	Scott Roberts		Return on error if catalog mgr returns an error in truncate.
 *	21-Apr-1998	Don Brady		Fix up time/date conversions.
 *	20-Apr-1998	Don Brady		Remove course-grained hfs metadata locking.
 *	17-Apr-1998	Pat Dirks		Officially enabled searchfs in vops table.
 *	17-Apr-1998	Deric Horn		Bug fixes to hfs_search, reenabled searchfs trap for upcoming kernel build.
 *	15-Apr-1998	Don Brady		Add locking for HFS B-trees. Don't lock file meta lock for VSYSTEM files.
 *								Don't call VOP_UPDATE for system files. Roll set_time into hfs_update.
 *	14-Apr-1998	Pat Dirks		Cleaned up fsync to skip complex nodes and not hit sibling nodes.
 *	14-Apr-1998	Deric Horn		Added hfs_search() and related routines for searchfs() support.
 *	14-Apr-1998	Scott Roberts		Fixed paramaters to ExchangeFileIDs()
 *	13-Apr-1998	Pat Dirks		Changed to update H_HINT whenever hfsLookup was called.
 *	 8-Apr-1998	Pat Dirks		Added page-in and page-out passthrough routines to keep MapFS happy.
 *	 6-Apr-1998	Pat Dirks		Changed hfs_write to clean up code and fix bug that caused
 *								zeroes to be interspersed in data.  Added debug printf to hfs_read.
 *	 6-Apr-1998	Scott Roberts		Added complex file support.
 *	02-apr-1998	Don Brady		UpdateCatalogNode now takes parID and name as input.
 *	31-mar-1998	Don Brady		Sync up with final HFSVolumes.h header file.
 *	27-mar-1998	Don Brady		Check result from UFSToHFSStr to make sure hfs/hfs+ names are not greater than 31 characters.
 *	27-mar-1998	chw			minor link fixes.
 *	19-Mar-1998	ser			Added hfs_readdirattr.
 *	17-Mar-1998	ser			Removed CheckUserAccess. Added code to implement ExchangeFileIDs
 *	16-Mar-1998	Pat Dirks		Fixed logic in hfs_read to properly account for space
 *								remaining past selected offset and avoid premature panic.
 *	16-jun-1997	Scott Roberts
 *	   Dec-1991	Kevin Wells at NeXT:
 *			Significantly modified for Macintosh file system.
 *			Added support for NFS exportability.
 *	25-Jun-1990	Doug Mitchell at NeXT:
 *			Created (for DOS file system).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <mach/machine/vm_types.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/signalvar.h>
#include <sys/attr.h>
#include <miscfs/specfs/specdev.h>
#include <vfs/vfs_support.h>

#include <libkern/libkern.h>
#include <mach/machine/simple_lock.h>
#include <machine/spl.h>
#include <mach/features.h>
#include <kern/mapfs.h>

#include	"hfs.h"
#include	"hfs_lockf.h"
#include	"hfs_dbg.h"
#include	"hfscommon/headers/FileMgrInternal.h"
#include	"hfscommon/headers/CatalogPrivate.h"
#include 	"hfscommon/headers/system/HFSUnicodeWrappers.h"

//	Private description used in hfs_search
struct SearchState {
	long				searchBits;
	BTreeIterator		btreeIterator;
	short				vRefNum;		//	Volume reference of volume being searched
	char				isHFSPlus;		//	True if volume is HFS
	char				pad1[3];		//	long align the structure
};
typedef struct SearchState SearchState;


enum {
	MAXHFSFILESIZE = 0x7FFFFFFF		/* this needs to go in the mount structure */
};

#define OWNERSHIP_ONLY_ATTRS (ATTR_CMN_OWNERID | ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK | ATTR_CMN_FLAGS)

/* Global vfs data structures for hfs */
int (**hfs_vnodeop_p)();

/* external routines defined in vfs_cache.c */

void hfs_vhashrem(struct hfsnode *hp);

extern void cache_purge (struct vnode *vp);
extern int cache_lookup (struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
extern void cache_enter (struct vnode *dvp, struct vnode *vpp, struct componentname *cnp);

extern void vnode_pager_setsize( struct vnode *vp, u_long nsize);
extern int vnode_uncache( struct vnode	*vp);

extern groupmember(gid_t gid, struct ucred *cred);

static int hfs_makenode( int mode, struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);

static void hfs_chid(struct hfsnode *hp, u_int32_t fid, u_int32_t pid, char* name);

static int UnpackSearchAttributeBlock(struct vnode *vp, struct attrlist	*alist, searchinfospec_t *searchInfo, void *attributeBuffer);

Boolean CheckCriteria( ExtendedVCB *vcb, const SearchState *searchState, u_long searchBits, struct attrlist *attrList, CatalogRecord	*catalogRecord, CatalogKey *key, searchinfospec_t *searchInfo1, searchinfospec_t *searchInfo2 );

static int InsertMatch( struct vnode *vp, struct uio *a_uio, CatalogRecord *catalogRecord, CatalogKey *key, struct attrlist *returnAttrList, void *attributesBuffer, void *variableBuffer, u_long bufferSize, u_long * nummatches );

extern	Boolean ComparePartialName(ConstStr31Param thisName, ConstStr31Param substring);
extern	Boolean CompareFullName(ConstStr31Param thisName, ConstStr31Param testName);
extern	Boolean CompareMasked(const UInt32 *thisValue, const UInt32 *compareData,  const UInt32 *compareMask, UInt32 count);

static Boolean CompareRange(u_long val, u_long low, u_long high);

static Boolean CompareRange( u_long val, u_long low, u_long high )
{
	return( (val >= low) && (val <= high) );
}
//#define CompareRange(val, low, high)	((val >= low) && (val <= high))


static int hfs_chown( struct vnode *vp, uid_t uid, gid_t gid, struct ucred *cred, struct proc *p);
static int hfs_chmod( struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
static int hfs_chflags( struct vnode *vp, u_long flags, struct ucred *cred, struct proc *p);

/*
 * Enabling cluster read/write operations.
 */
extern int doclusterread;
extern int doclusterwrite;

int hfs_lookup();		/* in hfs_lookup.c */
int hfs_read();			/* in hfs_readwrite.c */
int hfs_write();		/* in hfs_readwrite.c */
int hfs_ioctl();		/* in hfs_readwrite.c */
int hfs_select();		/* in hfs_readwrite.c */
int hfs_mmap();			/* in hfs_readwrite.c */
int hfs_seek();			/* in hfs_readwrite.c */
int hfs_bmap();			/* in hfs_readwrite.c */
int hfs_strategy();		/* in hfs_readwrite.c */
int hfs_reallocblks();	/* in hfs_readwrite.c */
int hfs_truncate();		/* in hfs_readwrite.c */
int hfs_allocate();		/* in hfs_readwrite.c */
int hfs_pagein();		/* in hfs_readwrite.c */
int hfs_pageout();		/* in hfs_readwrite.c */

/*****************************************************************************
*
*	Operations on vnodes
*
*****************************************************************************/

/*
 * Create a regular file
#% create	dvp	L U U
#% create	vpp	- L -
#
 vop_create {
     IN WILLRELE struct vnode *dvp;
     OUT struct vnode **vpp;
     IN struct componentname *cnp;
     IN struct vattr *vap;
	
     We are responsible for freeing the namei buffer, it is done in hfs_makenode(), unless there is
	a previous error.

*/

static int
hfs_create(ap)
struct vop_create_args /* {
    struct vnode *a_dvp;
    struct vnode **a_vpp;
    struct componentname *a_cnp;
    struct vattr *a_vap;
} */ *ap;
{
	struct proc		*p = CURRENT_PROC;
    int				retval;
    int				mode = MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode);
    DBG_FUNC_NAME("create");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_POS);
    DBG_VOP_CONT(("\tva_type %d va_mode 0x%x\n",
             ap->a_vap->va_type, ap->a_vap->va_mode));

#if DIAGNOSTIC
    DBG_HFS_NODE_CHECK(ap->a_dvp);
    DBG_ASSERT(ap->a_dvp->v_type == VDIR);
    if(ap->a_vap == NULL) {
        panic("NULL attr on create");
    }

    switch(ap->a_vap->va_type) {
        case VDIR:
    		VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
            VPUT(ap->a_dvp);
            DBG_VOP_LOCKS_TEST(EISDIR);
            return (EISDIR);	/* use hfs_mkdir instead */
        case VREG:
        case VLNK:
            break;
        default:
            DBG_ERR(("%s: INVALID va_type: %d, %s, %s\n", funcname, ap->a_vap->va_type, H_NAME(VTOH(ap->a_dvp)), ap->a_cnp->cn_nameptr));
    		VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
            VPUT(ap->a_dvp);
            DBG_VOP_LOCKS_TEST(EINVAL);
            return (EINVAL);
            }
//    if(ap->a_vap->va_mode & (VSUID | VSGID | VSVTX)) {
//        DBG_ERR(("%s: INVALID va_mode (%o): %s, %s\n", funcname, ap->a_vap->va_mode, H_NAME(VTOH(ap->a_dvp)), ap->a_cnp->cn_nameptr));
//        DBG_VOP_LOCKS_TEST(EINVAL);
//		  VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
//        VPUT(ap->a_dvp);
//        return (EINVAL);		/* Can't do these */
//    };
#endif

	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
	if (retval != E_NONE) {
    	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
		VPUT(ap->a_dvp);
        DBG_VOP_LOCKS_TEST( retval);
        return (retval);
	}

	/* Create the vnode */
    retval = hfs_makenode(mode, ap->a_dvp, ap->a_vpp, ap->a_cnp);
    DBG_VOP_UPDATE_VP(1, *ap->a_vpp);

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_RELEASE, p);

    if (retval != E_NONE) {
        DBG_ERR(("%s: hfs_makenode FAILED: %s, %s\n", funcname, ap->a_cnp->cn_nameptr, H_NAME(VTOH(ap->a_dvp))));
	}
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}



#if 0	/* Now stubbed out in the vnode ops table with err_mknod */
/*
 * Mknod vnode call

#% mknod	dvp	L U U
#% mknod	vpp	- X -
#
 vop_mknod {
     IN WILLRELE struct vnode *dvp;
     OUT WILLRELE struct vnode **vpp;
     IN struct componentname *cnp;
     IN struct vattr *vap;
     */
/* ARGSUSED */

static int
hfs_mknod(ap)
struct vop_mknod_args /* {
    struct vnode *a_dvp;
    struct vnode **a_vpp;
    struct componentname *a_cnp;
    struct vattr *a_vap;
} */ *ap;
{
    DBG_FUNC_NAME("mknod");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_LOCKNOTNIL, VOPDBG_IGNORE, VOPDBG_POS);

    DBG_VOP(("%s: parent 0x%x name %s\n", funcname, (u_int)ap->a_dvp, ap->a_cnp->cn_nameptr));

    VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	VPUT(ap->a_dvp);
	
    DBG_VOP_LOCKS_TEST(EOPNOTSUPP);
    return (EOPNOTSUPP);
}
#endif


/*
 * mkcomplex vnode call
 *

#% mkcomplex	dvp	L U U
#% mkcomplex	vpp	- L -
#
vop_mkcomplex {
	IN WILLRELE struct vnode *dvp;
	OUT struct vnode **vpp;
	IN struct componentname *cnp;
	IN struct vattr *vap;
	IN u_long type;
}

 */
 
static int
hfs_mkcomplex(ap)
struct vop_mkcomplex_args /* {
	struct vnode *a_dvp;
	struct vnode **a_vpp;
	struct componentname *a_cnp;
	struct vattr *a_vap;
	u_long a_type;
} */ *ap;
{
    int		retval = E_NONE;
    DBG_FUNC_NAME("make_complex");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_POS);

    retval = VOP_CREATE(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap);

    DBG_VOP_LOCKS_TEST(retval);
    return retval;
}


/*
 * Open called.
#% open		vp	L L L
#
 vop_open {
     IN struct vnode *vp;
     IN int mode;
     IN struct ucred *cred;
     IN struct proc *p;
     */


static int
hfs_open(ap)
struct vop_open_args /* {
    struct vnode *a_vp;
    int  a_mode;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    struct hfsnode	*hp = VTOH(ap->a_vp);
    int				retval = E_NONE;
    DBG_FUNC_NAME("open");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_CONT(("\t"));DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    if (ap->a_vp->v_type == VREG)	 /* Only files */
      {	
        /*
         * Files marked append-only must be opened for appending.
         */
        if ((hp->h_meta->h_pflags & APPEND) &&
            (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
            retval = EPERM;
        }


    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}

/*
 * Close called.
 *
 * Update the times on the hfsnode.
#% close	vp	U U U
#
 vop_close {
     IN struct vnode *vp;
     IN int fflag;
     IN struct ucred *cred;
     IN struct proc *p;
     */


static int
hfs_close(ap)
struct vop_close_args /* {
    struct vnode *a_vp;
    int  a_fflag;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    register struct vnode	*vp = ap->a_vp;
    struct hfsnode 			*hp = VTOH(vp);
    struct proc				*p = ap->a_p;
	FCB						*fcb;
    struct timeval 			tv;
	off_t					leof;
	u_long					blks, blocksize;
    int 					retval = E_NONE;

    DBG_FUNC_NAME("close");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_CONT(("\t"));DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);

    simple_lock(&vp->v_interlock);
    if (vp->v_usecount > 1) {
		tv = time;
        HFSTIMES(hp, &tv, &tv);
	}
    simple_unlock(&vp->v_interlock);

	/*
	 * VOP_CLOSE can be called with vp locked (from vclean).
	 * We check for this case using VOP_ISLOCKED and bail.
	 *
	 * also, ignore complex nodes; there's no data associated with them.
	 */
    if (H_FORKTYPE(hp) == kDirCmplx || VOP_ISLOCKED(vp)) {
        DBG_VOP_LOCKS_TEST(E_NONE);
        return E_NONE;
    };

	fcb = HTOFCB(hp);
	leof = fcb->fcbEOF;
	
	if (leof != 0) {
		blocksize = HTOVCB(hp)->blockSize;
		blks = leof / blocksize;
		if ((blks * blocksize) != leof)
			blks++;
	
		/*
		 * Shrink the peof to the smallest size neccessary to contain the leof.
		 */
		if ((blks * blocksize) < fcb->fcbPLen) {
			vn_lock(vp, LK_EXCLUSIVE | LK_CANRECURSE, p);
	 		retval = VOP_TRUNCATE(vp, leof, 0, ap->a_cred, p);
			VOP_UNLOCK(vp, 0, p);
		}
	}

    /* File is already flushed, so just reset values */
    H_HINT(hp) = kNoHint;		/* reset catalog hint */

    if (doclusterwrite) {
            long devBlockSize = 0;

	    vn_lock(vp, LK_EXCLUSIVE | LK_CANRECURSE, p);

            VOP_DEVBLOCKSIZE(hp->h_devvp, &devBlockSize);
            cluster_close(vp, PAGE_SIZE, devBlockSize);

	    VOP_UNLOCK(vp, 0, p);
    }

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}

/*
#% access	vp	L L L
#
 vop_access {
     IN struct vnode *vp;
     IN int mode;
     IN struct ucred *cred;
     IN struct proc *p;

     */

static int
hfs_access(ap)
struct vop_access_args /* {
    struct vnode *a_vp;
    int  a_mode;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    struct vnode *vp 			= ap->a_vp;
    struct ucred *cred 			= ap->a_cred;
    struct hfsnode *hp 			= VTOH(vp);
    ExtendedVCB	*vcb			= HTOVCB(hp);
    register gid_t *gp;
    mode_t mask, mode;
    Boolean isHFSPlus;
    int retval 					= E_NONE;
    int i;
    DBG_FUNC_NAME("access");
    DBG_VOP_LOCKS_DECL(1);
//    DBG_VOP_PRINT_FUNCNAME();
//    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
   
	 mode 		= ap->a_mode;
     isHFSPlus	= (vcb->vcbSigWord == kHFSPlusSigWord );

     /*
      * Disallow write attempts on read-only file systems;
      * unless the file is a socket, fifo, or a block or
      * character device resident on the file system.
      */
     if (mode & VWRITE) {
         switch (vp->v_type) {
         case VDIR:
         case VLNK:
         case VREG:
             if (VTOVFS(vp)->mnt_flag & MNT_RDONLY)
                 return (EROFS);
             break;
		default:
			break;
         }
     }

     /* If immutable bit set, nobody gets to write it. */
     if ((mode & VWRITE) && (hp->h_meta->h_pflags & IMMUTABLE))
         return (EPERM);

     /* Otherwise, user id 0 always gets access. */
     if (ap->a_cred->cr_uid == 0) {
         retval = 0;
         goto Exit;
     };

     mask = 0;

    /* Otherwise, check the owner. */
    if (cred->cr_uid == hp->h_meta->h_uid) {
        if (mode & VEXEC)
            mask |= S_IXUSR;
        if (mode & VREAD)
            mask |= S_IRUSR;
        if (mode & VWRITE)
            mask |= S_IWUSR;
        retval =  ((hp->h_meta->h_mode & mask) == mask ? 0 : EACCES);
        goto Exit;
    }

    /* Otherwise, check the groups. */
    for (i = 0, gp = cred->cr_groups; i < cred->cr_ngroups; i++, gp++)
        if (hp->h_meta->h_gid == *gp) {
            if (mode & VEXEC)
                mask |= S_IXGRP;
            if (mode & VREAD)
                mask |= S_IRGRP;
            if (mode & VWRITE)
                mask |= S_IWGRP;
            retval = ((hp->h_meta->h_mode & mask) == mask ? 0 : EACCES);
			goto Exit;
        }

    /* Otherwise, check everyone else. */
    if (mode & VEXEC)
        mask |= S_IXOTH;
    if (mode & VREAD)
        mask |= S_IROTH;
    if (mode & VWRITE)
        mask |= S_IWOTH;
    retval = ((hp->h_meta->h_mode & mask) == mask ? 0 : EACCES);

Exit:
	DBG_VOP_LOCKS_TEST(retval);
	return (retval);    
}



/*
#% getattr	vp	= = =
#
 vop_getattr {
     IN struct vnode *vp;
     IN struct vattr *vap;
     IN struct ucred *cred;
     IN struct proc *p;

     */


/* ARGSUSED */
static int
hfs_getattr(ap)
struct vop_getattr_args /* {
    struct vnode *a_vp;
    struct vattr *a_vap;
    struct ucred *a_cred;
    struct proc *a_p;
} */ *ap;
{
    register struct vnode 	*vp = ap->a_vp;
    register struct hfsnode *hp = VTOH(vp);
    register struct vattr	*vap = ap->a_vap;
    struct timeval 			tv;
    DBG_FUNC_NAME("getattr");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_POS);

    DBG_HFS_NODE_CHECK(ap->a_vp);

    tv = time;
    HFSTIMES(hp, &tv, &tv);

    vap->va_fsid = hp->h_dev;
    vap->va_fileid = H_FILEID(hp);
    vap->va_mode = hp->h_meta->h_mode;
    vap->va_nlink = 1;
    vap->va_uid = hp->h_meta->h_uid;
    vap->va_gid = hp->h_meta->h_gid;
    vap->va_rdev = hp->h_meta->h_rdev;
#if	MACH_NBC
    if ((vp->v_type == VREG) && vp->v_vm_info && vp->v_vm_info->mapped &&
        (!vp->v_vm_info->filesize)) {
        vap->va_size = vp->v_vm_info->vnode_size;
    	} 
	else 
#endif /* MACH_NBC */
    if (vp->v_type == VDIR) {
        vap->va_size = hp->h_size;
        vap->va_bytes = 0;
    }
    else {
        vap->va_size = hp->h_xfcb->fcb_fcb.fcbEOF;
        vap->va_bytes = hp->h_size;
    }
    vap->va_atime.tv_nsec = 0;
    vap->va_atime.tv_sec = hp->h_meta->h_atime;
    vap->va_mtime.tv_nsec = 0;
    vap->va_mtime.tv_sec = hp->h_meta->h_mtime;
    vap->va_ctime.tv_nsec = 0;
    vap->va_ctime.tv_sec = hp->h_meta->h_ctime;
    vap->va_flags = hp->h_meta->h_pflags;
    vap->va_gen = 0;
    /* this doesn't belong here */
    if (vp->v_type == VBLK)
        vap->va_blocksize = BLKDEV_IOSIZE;
    else if (vp->v_type == VCHR)
        vap->va_blocksize = MAXPHYSIO;
    else
        vap->va_blocksize = VTOVFS(vp)->mnt_stat.f_iosize;
	vap->va_type = vp->v_type;
    vap->va_filerev = 0;

    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}

/*
 * Set attribute vnode op. called from several syscalls
#% setattr	vp	L L L
#
 vop_setattr {
     IN struct vnode *vp;
     IN struct vattr *vap;
     IN struct ucred *cred;
     IN struct proc *p;

     */

static int
hfs_setattr(ap)
struct vop_setattr_args /* {
struct vnode *a_vp;
struct vattr *a_vap;
struct ucred *a_cred;
struct proc *a_p;
} */ *ap;
{
    struct vnode 	*vp = ap->a_vp;
    struct hfsnode 	*hp = VTOH(vp);
    struct vattr 	*vap = ap->a_vap;
    struct ucred 	*cred = ap->a_cred;
    struct proc 	*p = ap->a_p;
    struct timeval 	atimeval, mtimeval;
    int				retval;
    DBG_FUNC_NAME("setattr");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    WRITE_CK(vp, funcname);
    DBG_HFS_NODE_CHECK(ap->a_vp);

    /*
     * Check for unsettable attributes.
     */
    if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
        (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
        (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
        ((int)vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
        retval = EINVAL;
        goto ErrorExit;
    }

    if (vap->va_flags != VNOVAL) {
        if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
            retval = EROFS;
            goto ErrorExit;
        };
        if ((retval = hfs_chflags(vp, vap->va_flags, cred, p))) {
            goto ErrorExit;
        };
        if (vap->va_flags & (IMMUTABLE | APPEND)) {
            retval = 0;
            goto ErrorExit;
        };
    }

    if (hp->h_meta->h_pflags & (IMMUTABLE | APPEND)) {
        retval = EPERM;
        goto ErrorExit;
    };
    /*
     * Go through the fields and update iff not VNOVAL.
     */
    if (vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL) {
        if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
            retval = EROFS;
            goto ErrorExit;
        };
        if ((retval = hfs_chown(vp, vap->va_uid, vap->va_gid, cred, p))) {
            goto ErrorExit;
        };
    }
    if (vap->va_size != VNOVAL) {
        /*
         * Disallow write attempts on read-only file systems;
         * unless the file is a socket, fifo, or a block or
         * character device resident on the file system.
         */
        switch (vp->v_type) {
            case VDIR:
                retval = EISDIR;
                goto ErrorExit;
            case VLNK:
            case VREG:
                if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
                    retval = EROFS;
                    goto ErrorExit;
                };
                break;
            default:
                break;
        }
        if ((retval = VOP_TRUNCATE(vp, vap->va_size, 0, cred, p))) {
            goto ErrorExit;
        };
    }
    hp = VTOH(vp);
    if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL) {
        if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
            retval = EROFS;
            goto ErrorExit;
        };
        if (cred->cr_uid != hp->h_meta->h_uid &&
            (retval = suser(cred, &p->p_acflag)) &&
            ((vap->va_vaflags & VA_UTIMES_NULL) == 0 ||
             (retval = VOP_ACCESS(vp, VWRITE, cred, p)))) {
            goto ErrorExit;
        };
        if (vap->va_atime.tv_sec != VNOVAL)
            hp->h_meta->h_nodeflags |= IN_ACCESS;
        if (vap->va_mtime.tv_sec != VNOVAL)
            hp->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;
        atimeval.tv_sec = vap->va_atime.tv_sec;
        atimeval.tv_usec = 0;
        mtimeval.tv_sec = vap->va_mtime.tv_sec;
        mtimeval.tv_usec = 0;
        if ((retval = VOP_UPDATE(vp, &atimeval, &mtimeval, 1))) {
            goto ErrorExit;
        };
    }
    retval = 0;
    if (vap->va_mode != (mode_t)VNOVAL) {
        if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
            retval = EROFS;
            goto ErrorExit;
        };
        retval = hfs_chmod(vp, (int)vap->va_mode, cred, p);
    };

ErrorExit: ;

    DBG_VOP(("hfs_setattr: returning %d...\n", retval));
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*

#
#% getattrlist	vp	= = =
#
 vop_getattrlist {
     IN struct vnode *vp;
     IN struct attrlist *alist;
     INOUT struct uio *uio;
     IN struct ucred *cred;
     IN struct proc *p;
 };

 */

static int
hfs_getattrlist(ap)
struct vop_getattrlist_args /* {
struct vnode *a_vp;
struct attrlist *a_alist
struct uio *a_uio;
struct ucred *a_cred;
struct proc *a_p;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct hfsnode *hp = VTOH(vp);
    struct attrlist *alist = ap->a_alist;
    int error = 0;
    struct hfsCatalogInfo catalogInfo;
    struct hfsCatalogInfo *catInfoPtr = NULL;
    int fixedblocksize;
    int attrblocksize;
    int attrbufsize;
    void *attrbufptr;
    void *attrptr;
    void *varptr;
    u_long fileID;
    DBG_FUNC_NAME("getattrlist");
    DBG_VOP_LOCKS_DECL(1);

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_POS);
    DBG_HFS_NODE_CHECK(ap->a_vp);
    DBG_VOP(("%s: Common attr:0x%lx, buff size Ox%lX,\n",funcname, (u_long)alist->commonattr,(u_long)ap->a_uio->uio_resid));

    DBG_ASSERT(ap->a_uio->uio_rw == UIO_READ);

    if ((alist->bitmapcount != ATTR_BIT_MAP_COUNT) ||
        ((alist->commonattr & ~ATTR_CMN_VALIDMASK) != 0) ||
        ((alist->volattr & ~ATTR_VOL_VALIDMASK) != 0) ||
        ((alist->dirattr & ~ATTR_DIR_VALIDMASK) != 0) ||
        ((alist->fileattr & ~ATTR_FILE_VALIDMASK) != 0) ||
        ((alist->forkattr & ~ATTR_FORK_VALIDMASK) != 0)) {
        DBG_ERR(("%s: bad attrlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
        };

    /* Requesting volume information requires setting the ATTR_VOL_INFO bit and
        volume info requests are mutually exclusive with all other info requests: */
   if ((alist->volattr != 0) && (((alist->volattr & ATTR_VOL_INFO) == 0) ||
        (alist->dirattr != 0) || (alist->fileattr != 0) || (alist->forkattr != 0)
		)) {
        DBG_ERR(("%s: conflicting information requested\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
        };

    /* Reject requests for unsupported options for now: */
    if ((alist->commonattr & (ATTR_CMN_NAMEDATTRCOUNT | ATTR_CMN_NAMEDATTRLIST)) ||
        (alist->fileattr & (ATTR_FILE_FILETYPE | ATTR_FILE_FORKCOUNT | ATTR_FILE_FORKLIST))) {
        DBG_ERR(("%s: illegal bits in attlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
        };

	/* Requesting volume information requires root vnode */ 
    if ((alist->volattr) && (H_FILEID(hp) != kRootDirID)) {
        DBG_ERR(("%s: not root vnode\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    	};

    /* If a FileID (ATTR_CMN_OBJPERMANENTID) is requested on an HFS volume we must be sure
        to create the thread record before returning it:
        */
    if (((vp->v_type == VREG) || (vp->v_type == VCPLX)) &&
        (alist->commonattr & ATTR_CMN_OBJPERMANENTID)) {
        /* Only HFS-Plus volumes are guaranteed to have a thread record in place already: */
        if (VTOVCB(vp)->vcbSigWord != kHFSPlusSigWord) {
            /* Create a thread record and return the FileID [which is the file's fileNumber] */
            /* lock catalog b-tree */
            error = hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_EXCLUSIVE, ap->a_p);
            error = hfsCreateFileID(VTOVCB(vp), H_DIRID(hp), H_NAME(hp), H_HINT(hp), &fileID);
            (void) hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_RELEASE, ap->a_p);
            if (error) {
                DBG_VOP_LOCKS_TEST(error);
                DBG_ERR(("hfs_getattrlist: error %d on CreateFileIDRef.\n", error));
                return error;
            };
            ASSERT(fileID == H_FILEID(hp));
        };
    };

    /*
	 * Avoid unnecessary catalog lookups for volume info which is available directly
	 * in the VCB and root vnode, or can be synthesized.
	 */
    if (((alist->volattr == 0) && ((alist->commonattr & HFS_ATTR_CMN_LOOKUPMASK) != 0)) ||
        ((alist->dirattr & HFS_ATTR_DIR_LOOKUPMASK) != 0) ||
        ((alist->fileattr & HFS_ATTR_FILE_LOOKUPMASK) != 0)) {

        /* lock catalog b-tree */
        error = hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_SHARED, ap->a_p);
        if (error) {
            DBG_VOP_LOCKS_TEST(error);
            return (error);
            }

        if (alist->volattr != 0) {
            /* Look up the root info, regardless of the vnode provided */
            error = hfsLookup(VTOVCB(vp), 2, NULL,  -1, 0, &catalogInfo);
            } else {
                error = hfsLookup(VTOVCB(vp), H_DIRID(hp), H_NAME(hp),  -1, 0, &catalogInfo);
                if (error == 0) H_HINT(hp) = catalogInfo.hint;						/* Remember the last valid hint */
                };
        catInfoPtr = &catalogInfo;

        /* unlock catalog b-tree */
        (void) hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_RELEASE, ap->a_p);
    };

    fixedblocksize = AttributeBlockSize(alist);
    attrblocksize = fixedblocksize + (sizeof(u_long));							/* u_long for length longword */
    if (alist->commonattr & ATTR_CMN_NAME) attrblocksize += NAME_MAX + 1;
    if (alist->commonattr & ATTR_CMN_NAMEDATTRLIST) attrblocksize += 0;			/* XXX PPD */
    if (alist->volattr & ATTR_VOL_MOUNTPOINT) attrblocksize += PATH_MAX;
    if (alist->volattr & ATTR_VOL_NAME) attrblocksize += NAME_MAX + 1;
    if (alist->fileattr & ATTR_FILE_FORKLIST) attrblocksize += 0;				/* XXX PPD */

    attrbufsize = MIN(ap->a_uio->uio_resid, attrblocksize);
    DBG_VOP(("hfs_getattrlist: allocating Ox%X byte buffer (Ox%X + Ox%X) for attributes...\n",
             attrblocksize,
             fixedblocksize,
             attrblocksize - fixedblocksize));
    MALLOC(attrbufptr, void *, attrblocksize, M_TEMP, M_WAITOK);
    attrptr = attrbufptr;
    *((u_long *)attrptr) = 0;									/* Set buffer length in case of errors */
    ++((u_long *)attrptr);										/* Reserve space for length field */
    varptr = ((char *)attrptr) + fixedblocksize;				/* Point to variable-length storage */
    DBG_VOP(("hfs_getattrlist: attrptr = 0x%08X, varptr = 0x%08X...\n", (u_int)attrptr, (u_int)varptr));

    PackAttributeBlock(alist, vp, catInfoPtr, &attrptr, &varptr);
    *((u_long *)attrbufptr) = (varptr - attrbufptr);		/* Store length of fixed + var block */
    attrbufsize = MIN(attrbufsize, varptr - attrbufptr);	/* Don't copy out more data than was generated */
    DBG_VOP(("hfs_getattrlist: copying Ox%X bytes to user address 0x%08X.\n", attrbufsize, (u_int)ap->a_uio->uio_iov->iov_base));
    error = uiomove((caddr_t)attrbufptr, attrbufsize, ap->a_uio);
    if (error != E_NONE) {
        DBG_ERR(("hfs_getattrlist: error %d on uiomove.\n", error));
        };

    FREE(attrbufptr, M_TEMP);

    DBG_VOP_LOCKS_TEST(error);
    return error;
}



/* This is a special permission-checking routine that tests for "write" access without
   regard for the UF_IMMUTABLE or SF_IMMUTABLE bits in the flags.  hfs_setattrlist cannot
   use VOP_ACCESS for this check so this is a derivative of that code that does this check.
 */
static int hfs_writepermission(struct vnode *vp, struct ucred *cred, struct proc *p) {
    struct hfsnode *hp = VTOH(vp);
    int i;
    register gid_t *gp;

	/* Start by checking for root */
	if (suser(cred, &p->p_acflag) == 0) return 0;
	
	/* Check the owner: */
	if (cred->cr_uid == hp->h_meta->h_uid) {
		return (hp->h_meta->h_mode & S_IWUSR) ? 0 : EACCES;
	};

	/* Otherwise, check the groups: */
    for (i = 0, gp = cred->cr_groups; i < cred->cr_ngroups; i++, gp++) {
       	if (hp->h_meta->h_gid == *gp) {
			return (hp->h_meta->h_mode & S_IWGRP) ? 0 : EACCES;
        };
   };

   /* Otherwise, check everyone else. */
   return (hp->h_meta->h_mode & S_IWOTH) ? 0 : EACCES;
}



/*

#
#% setattrlist	vp	L L L
#
 vop_setattrlist {
     IN struct vnode *vp;
     IN struct attrlist *alist;
     INOUT struct uio *uio;
     IN struct ucred *cred;
     IN struct proc *p;
 };

 */

static int
hfs_setattrlist(ap)
struct vop_setattrlist_args /* {
struct vnode *a_vp;
struct attrlist *a_alist
struct uio *a_uio;
struct ucred *a_cred;
struct proc *a_p;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct hfsnode *hp = VTOH(vp);
    struct attrlist *alist = ap->a_alist;
    struct ucred *cred = ap->a_cred;
    struct proc *p = ap->a_p;
    int error;
    struct hfsCatalogInfo catalogInfo;
    int attrblocksize;
    void *attrbufptr = NULL;
    void *attrptr;
    void *varptr = NULL;
	uid_t saved_uid;
	gid_t saved_gid;
	mode_t saved_mode;
    u_long saved_flags;
    int retval = 0;

    DBG_FUNC_NAME("setattrlist");
    DBG_VOP_LOCKS_DECL(1);

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_SAME, VOPDBG_POS);
    DBG_HFS_NODE_CHECK(ap->a_vp);
    DBG_VOP(("%s: Common attr:0x%x, buff size Ox%X,\n",funcname, (u_int)alist->commonattr,(u_int)ap->a_uio->uio_resid));

    DBG_ASSERT(ap->a_uio->uio_rw == UIO_WRITE);

    if ((alist->bitmapcount != ATTR_BIT_MAP_COUNT) ||
        ((alist->commonattr & ~ATTR_CMN_SETMASK) != 0) ||
        ((alist->volattr & ~ATTR_VOL_SETMASK) != 0) ||
        ((alist->dirattr & ~ATTR_DIR_SETMASK) != 0) ||
        ((alist->fileattr & ~ATTR_FILE_SETMASK) != 0) ||
        ((alist->forkattr & ~ATTR_FORK_SETMASK) != 0)) {
        DBG_ERR(("%s: Bad attrlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
        };

    if ((alist->volattr != 0) && 							/* Setting volume info */
		(((alist->volattr & ATTR_VOL_INFO) == 0) ||			/* Not explicitly indicating this or ... */
		 (alist->commonattr & ~ATTR_CMN_VOLSETMASK)))		/* ... setting invalid attributes for volume */
      {
        DBG_ERR(("%s: Bad attrlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
      };

    if (VTOVFS(vp)->mnt_flag & MNT_RDONLY) {
        DBG_VOP_LOCKS_TEST(EROFS);
        return EROFS;
    };

	/* NOTE: The following isn't ENTIRELY complete: even if you're the superuser
			 you cannot change the flags as long as SF_IMMUTABLE or SF_APPEND is
			 set and securelevel > 0.  This is verified in hfs_chflags which gets
			 invoked to do the actual flags field change so this check is sufficient
			 for now.
	 */
	if (alist->commonattr & (OWNERSHIP_ONLY_ATTRS)) {
		/* Check to see if the user owns the object [or is superuser]: */
    	if (cred->cr_uid != hp->h_meta->h_uid &&
        	(retval = suser(cred, &p->p_acflag))) {
        	DBG_VOP_LOCKS_TEST(retval);
        	return retval;
        };
	};
	
	/* For any other attributes, check to see if the user has write access to
	    the object in question [unlike VOP_ACCESS, ignore IMMUTABLE here]: */
	    
	if ((((alist->commonattr & ~(OWNERSHIP_ONLY_ATTRS)) != 0) ||
		 (alist->volattr != 0) ||
		 (alist->dirattr != 0) ||
		 (alist->fileattr != 0) ||
		 (alist->forkattr != 0)) &&
		((retval = hfs_writepermission(vp, cred, p)) != 0)) {
        DBG_VOP_LOCKS_TEST(retval);
        return retval;
	}; /* end of if ownership attr */
	
    /* Allocate the buffer now to minimize the time we might be blocked holding the catalog lock */
    attrblocksize = ap->a_uio->uio_resid;
    if (attrblocksize < AttributeBlockSize(alist)) {
        DBG_ERR(("%s: bad attrblocksize\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    };

   MALLOC(attrbufptr, void *, attrblocksize, M_TEMP, M_WAITOK);
    
    /* lock catalog b-tree */
    error = hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
    if (error != E_NONE) {
        goto FreeBuffer;
    };

	error = hfsLookup(VTOVCB(vp), H_DIRID(hp), H_NAME(hp), -1, 0,  &catalogInfo);
    if (error != E_NONE) {
        DBG_ERR(("%s: Lookup failed on file '%s'\n", funcname,  H_NAME(hp)));
        goto ErrorExit;
    };
    H_HINT(hp) = catalogInfo.hint;						/* Remember the last valid hint */

    error = uiomove((caddr_t)attrbufptr, attrblocksize, ap->a_uio);
    if (error) goto ErrorExit;

    if ((alist->volattr) && (H_FILEID(hp) != kRootDirID)) {
        error = EINVAL;
        goto ErrorExit;
    };

	/* do we have permission to change the dates? */
//  if (alist->commonattr & (ATTR_CMN_CRTIME | ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | ATTR_CMN_ACCTIME | ATTR_CMN_BKUPTIME)) {
    if (alist->commonattr & (ATTR_CMN_CHGTIME | ATTR_CMN_ACCTIME)) {
        if (cred->cr_uid != hp->h_meta->h_uid && (error = suser(cred, &p->p_acflag))) {
            goto ErrorExit;
        };
    };

    /* save these in case hfs_chown() or hfs_chmod() fail */
	saved_uid = hp->h_meta->h_uid;
	saved_gid = hp->h_meta->h_gid;
    saved_mode = hp->h_meta->h_mode;
    saved_flags = hp->h_meta->h_pflags;

    attrptr = attrbufptr;
    UnpackAttributeBlock(alist, vp, &catalogInfo, &attrptr, &varptr);

	/* if unpacking changed the owner or group then call hfs_chown() */
    if (saved_uid != hp->h_meta->h_uid || saved_gid != hp->h_meta->h_gid) {
		uid_t uid;
		gid_t gid;
		
		uid = hp->h_meta->h_uid;
 		hp->h_meta->h_uid = saved_uid;
		gid = hp->h_meta->h_gid;
		hp->h_meta->h_gid = saved_gid;
        if ((error = hfs_chown(vp, uid, gid, cred, p)))
			goto ErrorExit;
    }

	/* if unpacking changed the mode then call hfs_chmod() */
	if (saved_mode != hp->h_meta->h_mode) {
		mode_t mode;

		mode = hp->h_meta->h_mode;
		hp->h_meta->h_mode = saved_mode;
		if ((error = hfs_chmod(vp, mode, cred, p)))
			goto ErrorExit;
	};

    /* if unpacking changed the flags then call hfs_chflags */
    if (saved_flags != hp->h_meta->h_pflags) {
        u_long flags;

        flags = hp->h_meta->h_pflags;
        hp->h_meta->h_pflags = saved_flags;
        if ((error = hfs_chflags(vp, flags, cred, p)))
            goto ErrorExit;
    };

	if (alist->volattr == 0) {
    	error = MacToVFSError( UpdateCatalogNode(HTOVCB(hp), H_DIRID(hp), H_NAME(hp), H_HINT(hp), &catalogInfo.nodeData));
	}

    if (alist->volattr & ATTR_VOL_NAME) {
        ExtendedVCB *vcb 	= VTOVCB(vp);
        int			namelen = strlen(vcb->vcbVN);
    	
        error = MoveRenameCatalogNode(vcb, kRootParID, H_NAME(hp), H_HINT(hp), kRootParID, vcb->vcbVN, &H_HINT(hp));
        if (error) {
            VCB_LOCK(vcb);
            copystr(H_NAME(hp), vcb->vcbVN, sizeof(vcb->vcbVN), NULL);	/* Restore the old name in the VCB */
            vcb->vcbFlags |= 0xFF00;		// Mark the VCB dirty
            VCB_UNLOCK(vcb);
            goto ErrorExit;
        };

        if (namelen > hp->h_meta->h_namelen) {
            if (hp->h_meta->h_nodeflags & IN_LONGNAME)
              {
                FREE(H_NAME(hp), M_HFSNODE);
                MALLOC(H_NAME(hp), char *, namelen+1, M_HFSNODE, M_WAITOK);
              }
            else if (namelen > MAXHFSVNODELEN) {
                MALLOC(H_NAME(hp), char *, namelen+1, M_HFSNODE, M_WAITOK);
                hp->h_meta->h_nodeflags |= IN_LONGNAME;
            }
        };
        hp->h_meta->h_namelen = namelen;
        copystr(vcb->vcbVN, H_NAME(hp), hp->h_meta->h_namelen+1, NULL);
        hp->h_meta->h_nodeflags |= IN_CHANGE;
        
#if 0
        /* if hfs wrapper exists, update its name too */
        if (vcb->vcbSigWord == kHFSPlusSigWord && vcb->vcbAlBlSt != 0) {
			HFSMasterDirectoryBlock *mdb;
			struct buf *bp = NULL;
			int size = kMDBSize;	/* 512 */
            int volnamelen = MIN(sizeof(Str27), namelen);

			if ( bread(VTOHFS(vp)->hfs_devvp, IOBLKNOFORBLK(kMasterDirectoryBlock, size),
				 IOBYTECCNTFORBLK(kMasterDirectoryBlock, kMDBSize, size), NOCRED, &bp) == 0) {

				mdb = (HFSMasterDirectoryBlock *)((char *)bp->b_data + IOBYTEOFFSETFORBLK(kMasterDirectoryBlock, size));
                if (mdb->drSigWord == kHFSSigWord) {
                    /* Convert the string to MacRoman, ignoring any errors, */
                    (void) ConvertUTF8ToMacRoman(volnamelen, vcb->vcbVN, mdb->drVN);
                    bawrite(bp);
                    bp = NULL;
                }
			}

			if (bp) brelse(bp);
        }
#endif
    };

ErrorExit:
    /* unlock catalog b-tree */
    (void) hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_RELEASE, p);

FreeBuffer:
    if (attrbufptr) FREE(attrbufptr, M_TEMP);

    DBG_VOP_LOCKS_TEST(error);
    return error;
}

/*
 * Change the mode on a file.
 * Inode must be locked before calling.
 */
static int
hfs_chmod(vp, mode, cred, p)
register struct vnode *vp;
register int mode;
register struct ucred *cred;
struct proc *p;
{
    register struct hfsnode *hp = VTOH(vp);
    int retval;

    if (VTOVCB(vp)->vcbSigWord != kHFSPlusSigWord)
        return E_NONE;

    if (cred->cr_uid != hp->h_meta->h_uid &&
        (retval = suser(cred, &p->p_acflag)))
        return (retval);
    if (cred->cr_uid) {
        if (vp->v_type != VDIR && (mode & S_ISTXT))
            return (EFTYPE);
        if (!groupmember(hp->h_meta->h_gid, cred) && (mode & ISGID))
            return (EPERM);
    }
    hp->h_meta->h_mode &= ~ALLPERMS;
    hp->h_meta->h_mode |= (mode & ALLPERMS);
    hp->h_meta->h_nodeflags &= ~IN_UNSETACCESS;
    hp->h_meta->h_nodeflags |= IN_CHANGE;
    if ((vp->v_flag & VTEXT) && (hp->h_meta->h_mode & S_ISTXT) == 0)
        (void) vnode_uncache(vp);
    return (0);
}


/*
 * Change the flags on a file or directory.
 * Inode must be locked before calling.
 */
static int
hfs_chflags(vp, flags, cred, p)
register struct vnode *vp;
register u_long flags;
register struct ucred *cred;
struct proc *p;
{
    register struct hfsnode *hp = VTOH(vp);
    int retval;

    if (cred->cr_uid != hp->h_meta->h_uid &&
        (retval = suser(cred, &p->p_acflag)))
        return retval;

    if (cred->cr_uid == 0) {
        if ((hp->h_meta->h_pflags & (SF_IMMUTABLE | SF_APPEND)) &&
            securelevel > 0) {
            return EPERM;
        };
        hp->h_meta->h_pflags = flags;
    } else {
        if (hp->h_meta->h_pflags & (SF_IMMUTABLE | SF_APPEND) ||
            (flags & UF_SETTABLE) != flags) {
            return EPERM;
        };
        hp->h_meta->h_pflags &= SF_SETTABLE;
        hp->h_meta->h_pflags |= (flags & UF_SETTABLE);
    }
    hp->h_meta->h_nodeflags &= ~IN_UNSETACCESS;
    hp->h_meta->h_nodeflags |= IN_CHANGE;

    return 0;
}


/*
 * Perform chown operation on hfsnode hp;
 * hfsnode must be locked prior to call.
 */
static int
hfs_chown(vp, uid, gid, cred, p)
register struct vnode *vp;
uid_t uid;
gid_t gid;
struct ucred *cred;
struct proc *p;
{
    register struct hfsnode *hp = VTOH(vp);
    uid_t ouid;
    gid_t ogid;
    int retval = 0;

    if (VTOVCB(vp)->vcbSigWord != kHFSPlusSigWord)
        return EOPNOTSUPP;

    if (uid == (uid_t)VNOVAL)
        uid = hp->h_meta->h_uid;
    if (gid == (gid_t)VNOVAL)
        gid = hp->h_meta->h_gid;
    /*
     * If we don't own the file, are trying to change the owner
     * of the file, or are not a member of the target group,
     * the caller must be superuser or the call fails.
     */
    if ((cred->cr_uid != hp->h_meta->h_uid || uid != hp->h_meta->h_uid ||
         (gid != hp->h_meta->h_gid && !groupmember((gid_t)gid, cred))) &&
        (retval = suser(cred, &p->p_acflag)))
        return (retval);
    ogid = hp->h_meta->h_gid;
    ouid = hp->h_meta->h_uid;

    hp->h_meta->h_gid = gid;
    hp->h_meta->h_uid = uid;

    hp->h_meta->h_nodeflags &= ~IN_UNSETACCESS;
    if (ouid != uid || ogid != gid)
        hp->h_meta->h_nodeflags |= IN_CHANGE;
    if (ouid != uid && cred->cr_uid != 0)
        hp->h_meta->h_mode &= ~ISUID;
    if (ogid != gid && cred->cr_uid != 0)
        hp->h_meta->h_mode &= ~ISGID;
    return (0);
}



/*
#
#% exchange fvp		L L L
#% exchange tvp		L L L
#
 vop_exchange {
     IN struct vnode *fvp;
     IN struct vnode *tvp;
     IN struct ucred *cred;
     IN struct proc *p;
 };

 */
static int
hfs_exchange(ap)
struct vop_exchange_args /* {
struct vnode *a_fvp;
struct vnode *a_tvp;
struct ucred *a_cred;
struct proc *a_p;
} */ *ap;
{
    struct hfsnode *fromhp, *tohp;
    struct hfsmount *hfsmp;
//	struct hfsCatalogInfo catalogInfo;
	u_char tmp_name[NAME_MAX+1];		/* 256 bytes! */
    ExtendedVCB *vcb;
	u_int32_t fromFileID;
	u_int32_t fromParID;
	u_int32_t tmpLong;
    Boolean isHFSPlus;
    int retval = E_NONE;
    DBG_FUNC_NAME("exchange");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_fvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_PRINT_VNODE_INFO(ap->a_tvp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_fvp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,ap->a_tvp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    fromhp 	= VTOH(ap->a_fvp);
    tohp 	= VTOH(ap->a_tvp);
    hfsmp	= VTOHFS(ap->a_fvp);
    vcb 	= HTOVCB(fromhp);
    isHFSPlus = (vcb->vcbSigWord == kHFSPlusSigWord);

    if (ap->a_fvp->v_mount != ap->a_tvp->v_mount) {
        DBG_VOP_LOCKS_TEST(EXDEV);
        return EXDEV;
    }

    /* Can only exchange file objects */
    if (ap->a_fvp->v_type != VREG || ap->a_tvp->v_type != VREG) {
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    }


    /* Make sure buffers are written out and trashed */
    if ((retval = vinvalbuf(ap->a_fvp, V_SAVE, ap->a_cred, ap->a_p, 0, 0)))
        goto Err_Exit;

    if ((retval = vinvalbuf(ap->a_tvp, V_SAVE, ap->a_cred, ap->a_p, 0, 0)))
        goto Err_Exit;

    if (VTOH(ap->a_fvp)->h_sibling) {
        if ((retval = vinvalbuf(VTOH(ap->a_fvp)->h_sibling, V_SAVE, ap->a_cred, ap->a_p, 0, 0)))
            goto Err_Exit;
        }

    if (VTOH(ap->a_tvp)->h_sibling) {
        if ((retval = vinvalbuf(VTOH(ap->a_tvp)->h_sibling, V_SAVE, ap->a_cred, ap->a_p, 0, 0)))
            goto Err_Exit;
        }

    /* lock catalog b-tree */
    retval = hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_EXCLUSIVE, ap->a_p);
    if (retval) goto Err_Exit;

    /* lock extents b-tree iff there are overflow extents */
    /* XXX SER ExchangeFileIDs() always tries to delete the virtual extent id for exchanging files
        so we neeed the tree to be always locked.
    */
    retval = hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_EXCLUSIVE, ap->a_p);
    if (retval) goto Err_Exit_Relse;

    /* Do the exchange */
    retval = MacToVFSError( ExchangeFileIDs(vcb, H_NAME(fromhp), H_NAME(tohp), H_DIRID(fromhp), H_DIRID(tohp), H_HINT(fromhp), H_HINT(tohp) ));

    (void) hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_RELEASE, ap->a_p);

    if (retval != E_NONE) {
        DBG_ERR(("/tError trying to exchange: %d\n", retval));
        goto Err_Exit_Relse;
    }

	
    /* Now exchange fileID, parID, name for the vnode itself */
    fromFileID = H_FILEID(fromhp);
    fromParID = H_DIRID(fromhp);
    copystr(H_NAME(fromhp), (char*) tmp_name, strlen(H_NAME(fromhp))+1, NULL);
    hfs_chid(fromhp, H_FILEID(tohp), H_DIRID(tohp), H_NAME(tohp));
    hfs_chid(tohp, fromFileID, fromParID, (char*) tmp_name);
	
	/* copy rest */
    tmpLong = HTOFCB(fromhp)->fcbFlags;
    HTOFCB(fromhp)->fcbFlags = HTOFCB(tohp)->fcbFlags;
    HTOFCB(tohp)->fcbFlags = tmpLong;

     tmpLong = fromhp->h_meta->h_crtime;
    fromhp->h_meta->h_crtime = tohp->h_meta->h_crtime;
    tohp->h_meta->h_crtime = tmpLong;

    tmpLong = fromhp->h_meta->h_butime;
    fromhp->h_meta->h_butime = tohp->h_meta->h_butime;
    tohp->h_meta->h_butime = tmpLong;

    tmpLong = fromhp->h_meta->h_atime;
    fromhp->h_meta->h_atime = tohp->h_meta->h_atime;
    tohp->h_meta->h_atime = tmpLong;

    tmpLong = fromhp->h_meta->h_ctime;
    fromhp->h_meta->h_ctime = tohp->h_meta->h_ctime;
    tohp->h_meta->h_ctime = tmpLong;


    tmpLong = fromhp->h_meta->h_gid;
    fromhp->h_meta->h_gid = tohp->h_meta->h_gid;
    tohp->h_meta->h_gid = tmpLong;

    tmpLong = fromhp->h_meta->h_uid;
    fromhp->h_meta->h_uid = tohp->h_meta->h_uid;
    tohp->h_meta->h_uid = tmpLong;

    tmpLong = fromhp->h_meta->h_pflags;	
    fromhp->h_meta->h_pflags = tohp->h_meta->h_pflags;
    tohp->h_meta->h_pflags = tmpLong;

    tmpLong = fromhp->h_meta->h_mode;	
    fromhp->h_meta->h_mode = tohp->h_meta->h_mode;
    tohp->h_meta->h_mode = tmpLong;

    tmpLong = fromhp->h_meta->h_rdev;	
    fromhp->h_meta->h_rdev = tohp->h_meta->h_rdev;
    tohp->h_meta->h_rdev = tmpLong;

    tmpLong = fromhp->h_size;	
    fromhp->h_size = tohp->h_size;
    tohp->h_size = tmpLong;

	

Err_Exit_Relse:;
    /* unlock catalog b-tree */
    (void) hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_RELEASE, ap->a_p);

Err_Exit:;

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*
 * Change a vnode's file id, parent id and name
 * 
 * Assumes the vnode is locked and is of type VREG
 */
static void
hfs_chid(struct hfsnode *hp, u_int32_t fid, u_int32_t pid, char* name)
{
	struct hfsnode *tp;
	int		namelen = strlen(name);

	DBG_ASSERT(HTOV(hp)->v_type == VREG);

	hfs_vhashrem(hp);
	H_HINT(hp) = 0;
	H_FILEID(hp) = fid;					/* change h_nodeID */
	hp->h_xfcb->fcb_fcb.fcbFlNm = fid;	/* and this one too */
	H_DIRID(hp) = pid;
    if (namelen > hp->h_meta->h_namelen) {
        if (hp->h_meta->h_nodeflags & IN_LONGNAME)
          {
            FREE(H_NAME(hp), M_HFSNODE);
            MALLOC(H_NAME(hp), char *, namelen+1, M_HFSNODE, M_WAITOK);
          }
        else if (namelen > MAXHFSVNODELEN) {
            MALLOC(H_NAME(hp), char *, namelen+1, M_HFSNODE, M_WAITOK);
            hp->h_meta->h_nodeflags |= IN_LONGNAME;
        }
    };
    hp->h_meta->h_namelen = namelen;
    copystr(name, H_NAME(hp), hp->h_meta->h_namelen+1, NULL);
	hfs_vhashinslocked(hp);

	/* Change complex node also */
	tp = VTOH(hp->h_relative);
	if (tp) {
		cache_purge(HTOV(tp));

		hfs_vhashrem(tp);
		H_HINT(tp) = 0;
 		H_FILEID(tp) = fid;
		tp->h_xfcb->fcb_fcb.fcbFlNm = fid;
		hfs_vhashinslocked(tp);
	}

	/* Now change sibling (if any) */
	if (hp->h_sibling && (tp = VTOH(hp->h_sibling))) {
		hfs_vhashrem(tp);
		H_HINT(tp) = 0;
		H_FILEID(tp) = fid;
		tp->h_xfcb->fcb_fcb.fcbFlNm = fid;
		hfs_vhashinslocked(tp);
	}
}


/*

#% fsync	vp	L L L
#
 vop_fsync {
     IN struct vnode *vp;
     IN struct ucred *cred;
     IN int waitfor;
     IN struct proc *p;

     */


static int
hfs_fsync(ap)
struct vop_fsync_args /* {
    struct vnode *a_vp;
    struct ucred *a_cred;
    int a_waitfor;
    struct proc *a_p;
} */ *ap;
{
    struct vnode 		*vp = ap->a_vp ;
    struct hfsnode 		*hp;
    int					retval = 0;
    register struct buf *bp;
    struct timeval 		tv;
    struct buf 			*nbp;
    int 				s;

    DBG_FUNC_NAME("fsync");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("  "));
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_ZERO);
    DBG_HFS_NODE_CHECK(ap->a_vp);
	
#if DIAGNOSTIC
    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
#endif

    if (vp->v_type == VCPLX) {
		/* Ignore complex nodes; there's no data associated with them. */
		DBG_ASSERT(vp->v_dirtyblkhd.lh_first == NULL);
		DBG_VOP_LOCKS_TEST(E_NONE);
    	return E_NONE;
    };
	

    /*
     * Flush all dirty buffers associated with a vnode.
     */
loop:
	hp = VTOH(vp);
    s = splbio();
    for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = nbp) {
        nbp = bp->b_vnbufs.le_next;
        if ((bp->b_flags & B_BUSY))
            continue;
        if ((bp->b_flags & B_DELWRI) == 0)
            panic("hfs_fsync: not dirty");
        bremfree(bp);
        bp->b_flags |= B_BUSY;
        bp->b_flags &= ~B_LOCKED;	/* Clear flag, should only be set on meta files */
        splx(s);
        /*
         * Wait for I/O associated with indirect blocks to complete,
         * since there is no way to quickly wait for them below.
         */
        DBG_VOP(("\t\t\tFlushing out phys block %d == log block %d\n", bp->b_blkno, bp->b_lblkno));
        if (bp->b_vp == vp || ap->a_waitfor == MNT_NOWAIT)
            (void) bawrite(bp);
        else
            (void) bwrite(bp);
        goto loop;
    }

    if (ap->a_waitfor == MNT_WAIT) {
        while (vp->v_numoutput) {
            vp->v_flag |= VBWAIT;
            tsleep((caddr_t)&vp->v_numoutput, PRIBIO + 1, "hfs_fsync", 0);
        }

        /* I have seen this happen for swapfile. So it is safer to
         * check for dirty buffers again.  --Umesh
         */
        if (vp->v_dirtyblkhd.lh_first) {
            vprint("hfs_fsync: dirty", vp);
            splx(s);
            goto loop;
        }
    }
    splx(s);

#if DIAGNOSTIC
    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
#endif

    tv = time;
    hp->h_lastfsync = tv.tv_sec;
	if (H_FORKTYPE(hp) != kSysFile) {
    	retval = VOP_UPDATE(ap->a_vp, &tv, &tv, ap->a_waitfor == MNT_WAIT);

    	if (retval != E_NONE) {
        	DBG_ERR(("%s: FLUSH FAILED: %s\n", funcname, H_NAME(hp)));
    	}
    }
	else
        hp->h_meta->h_nodeflags &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);

    if (ap->a_waitfor == MNT_WAIT) {
      DBG_ASSERT(vp->v_dirtyblkhd.lh_first == NULL);
    };
    DBG_VOP_LOCKS_TEST(retval);
    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
    return (retval);
}



/*

#% remove	dvp	L U U
#% remove	vp	L U U
#
 vop_remove {
     IN WILLRELE struct vnode *dvp;
     IN WILLRELE struct vnode *vp;
     IN struct componentname *cnp;

     */

int
hfs_remove(ap)
struct vop_remove_args /* {
    struct vnode *a_dvp;
    struct vnode *a_vp;
    struct componentname *a_cnp;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct vnode *dvp = ap->a_dvp;
    struct vnode *ttp,*tp = NULL;
    struct hfsnode *hp = VTOH(ap->a_vp);
    struct hfsmount *hfsmp = HTOHFS(hp);
    struct proc *p = CURRENT_PROC;
    struct timeval tv;
    int retval;
    struct vnode *sibling_vp;
    DBG_FUNC_NAME("remove");

    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    /* see if they really meant to call VOP_RMDIR */
    if (vp->v_type == VDIR)
        return VOP_RMDIR(dvp, vp, ap->a_cnp);

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,ap->a_vp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);

    if ((hp->h_meta->h_pflags & (IMMUTABLE | APPEND)) ||
        (VTOH(dvp)->h_meta->h_pflags & APPEND)) {
        retval = EPERM;
        goto out;
    }

    /*  HFS does not have unlinking, do we cannot delete a file that is inuse */
    DBG_VOP(("\tusecount = %d", vp->v_usecount));
    if (VTOH(vp)->h_sibling) {
        DBG_VOP_CONT((", sibling usecount = %d\n", VTOH(vp)->h_sibling->v_usecount));
    }
    else {
        DBG_VOP_CONT(("\n"));
    }
    if (vp->v_usecount > 1) {
        retval = EBUSY;
        goto out;
    }
    /* Check other siblings for in use also */
    tp = vp;
	/* XXX SER Process all siblings here */
	/* XXX SER MUST BE CHANGED BY MACOS X */
    /* This unlocks the vnode!!!! Because vnode_uncache does the same
	 * so it depends on no context switches.
     * See the comment in vnode_uncache()
	 */
	/* Uncache everything */
 sibling_loop:
    sibling_vp = VTOH(tp)->h_sibling;	/* Note - this is a local copy for good reason! */
    if (sibling_vp) {
    	VOP_UNLOCK(vp, 0, p);
        if (vget(sibling_vp, LK_EXCLUSIVE | LK_RETRY, p)) {
	  /* Couldn't get the sibling - it might have gone away.
	     Clean up to try again from scratch */
	  vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	  goto sibling_loop;
	};
        (void) vnode_uncache(sibling_vp);
        vput(sibling_vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
    };

    while (VTOH(tp)->h_sibling && (VTOH(tp)->h_sibling != vp)) {
        simple_lock(&tp->v_interlock);
        if (VTOH(tp)->h_sibling->v_usecount > 0) {
            retval = EBUSY;
            simple_unlock(&tp->v_interlock);
            goto out;
        }
		ttp = tp;
        tp = VTOH(tp)->h_sibling;
        simple_unlock(&ttp->v_interlock);
    }	

	/* Flush out any catalog changes */
	/* XXX SER: This is a hack, becasue hfsDelete reads the data from the disk
	 * and not from memory which is more correct
	 */
    if ((hp->h_meta->h_nodeflags & IN_MODIFIED) || (HTOFCB(hp)->fcbFlags & fcbModifiedMask))
        {
        DBG_ASSERT((hp->h_meta->h_nodeflags & IN_MODIFIED) != 0);
        tv = time;
        VOP_UPDATE(vp, &tv, &tv, 0);
        }

    /* lock catalog b-tree */
    retval = hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_EXCLUSIVE, p);
    if (retval != E_NONE) {
        retval = EBUSY;
        goto out;
        }

    /* lock extents b-tree (also protects volume bitmap) */
    retval = hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_EXCLUSIVE, p);
    if (retval != E_NONE) {
        retval = EBUSY;
        goto out2;	/* unlock catalog b-tree on the way out */
        }

    /* remove the entry from the namei cache: */
    cache_purge(VTOH(vp)->h_relative);

    /* remove entry from catalog and free any blocks used */
    retval = hfsDelete (HTOVCB(hp), H_DIRID(hp), H_NAME(hp), TRUE, H_HINT(hp));

    hp->h_meta->h_mode = 0;				/* Makes the node go away...see inactive */
    tp = VTOH(vp)->h_sibling;			/* Remember any siblings, to inactivate later */

    (void) hfs_metafilelocking(hfsmp, kHFSExtentsFileID, LK_RELEASE, p);
out2:
    (void) hfs_metafilelocking(hfsmp, kHFSCatalogFileID, LK_RELEASE, p);

out:;

    if (! retval)
   		VTOH(dvp)->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;

    if (dvp == vp) {
        VRELE(vp);
    } else {
        VPUT(vp);
    };

	/* Before leaving, remove any siblings also */
	/* Note: vp and all its values are gone, so do not use */
    if ((! retval) && (tp != NULL)) {
        if (! vget (tp, LK_EXCLUSIVE, p)) {
            VTOH(tp)->h_meta->h_mode = 0;				/* Makes the node go away...see inactive */
            VPUT(tp);
		}
    }

    VPUT(dvp);
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


#if 0	/* Now stubbed out in the vnode ops table with err_mknod */
/*
 * link vnode call
 */
/*
 * HFS filesystems don't know what links are. But since we already called
 * lookup() with create and lockparent, the parent is locked so we
 * have to free it before we return the retval.
#% link		vp	U U U
#% link		targetPar_vp	L U U
#
 vop_link {
     IN WILLRELE struct vnode *vp;
     IN struct vnode *targetPar_vp;
     IN struct componentname *cnp;

     */
static int
hfs_link(ap)
struct vop_link_args /* {
    struct vnode *a_vp;
    struct vnode *a_tdvp;
    struct componentname *a_cnp;
} */ *ap;
{
    DBG_FUNC_NAME("link");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,ap->a_tdvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);

    VOP_UNLOCK(ap->a_vp, 0, ap->a_cnp->cn_proc);

    VOP_ABORTOP(ap->a_tdvp, ap->a_cnp);
    VPUT(ap->a_tdvp);
    
    DBG_VOP_LOCKS_TEST(EOPNOTSUPP);
    return (EOPNOTSUPP);
}
#endif

/*

#% rename	sourcePar_vp	U U U
#% rename	source_vp		U U U
#% rename	targetPar_vp	L U U
#% rename	target_vp		X U U
#
 vop_rename {
     IN WILLRELE struct vnode *sourcePar_vp;
     IN WILLRELE struct vnode *source_vp;
     IN struct componentname *source_cnp;
     IN WILLRELE struct vnode *targetPar_vp;
     IN WILLRELE struct vnode *target_vp;
     IN struct componentname *target_cnp;


     */
/*
* On entry:
*	source's parent directory is unlocked
*	source file or directory is unlocked
*	destination's parent directory is locked
*	destination file or directory is locked if it exists
*
* On exit:
*	all denodes should be released
*
*/

static int
hfs_rename(ap)
struct vop_rename_args  /* {
    struct vnode *a_fdvp;
    struct vnode *a_fvp;
    struct componentname *a_fcnp;
    struct vnode *a_tdvp;
    struct vnode *a_tvp;
    struct componentname *a_tcnp;
} */ *ap;
{
    struct vnode 			*target_vp = ap->a_tvp;
    struct vnode 			*targetPar_vp = ap->a_tdvp;
    struct vnode 			*source_vp = ap->a_fvp;
    struct vnode 			*sourcePar_vp = ap->a_fdvp;
    struct vnode 			*tp;
    struct componentname 	*target_cnp = ap->a_tcnp;
    struct componentname 	*source_cnp = ap->a_fcnp;
    struct proc 			*p = source_cnp->cn_proc;
    struct hfsnode 			*target_hp, *targetPar_hp, *source_hp, *sourcePar_hp;
    u_short					doingdirectory = 0, oldparent = 0, newparent = 0;
    int 					retval = 0;
    struct timeval 			tv;
	struct hfsCatalogInfo catInfo;
    int targetnamelen;
    DBG_VOP_LOCKS_DECL(4);

    DBG_FUNC_NAME("rename");DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("Source:\t"));DBG_VOP_PRINT_VNODE_INFO(ap->a_fvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("SourcePar: "));DBG_VOP_PRINT_VNODE_INFO(ap->a_fdvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("Target:\t"));DBG_VOP_PRINT_VNODE_INFO(ap->a_tvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("TargetPar: "));DBG_VOP_PRINT_VNODE_INFO(ap->a_tdvp);DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("SourceName:\t"));DBG_VOP_PRINT_CPN_INFO(ap->a_fcnp);DBG_VOP_CONT(("\n"));
    DBG_VOP_CONT(("\t"));DBG_VOP_CONT(("TargetName:\t"));DBG_VOP_PRINT_CPN_INFO(ap->a_tcnp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_fdvp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,ap->a_fvp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(2,ap->a_tdvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(3,ap->a_tvp, VOPDBG_LOCKNOTNIL, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    WRITE_CK(ap->a_fdvp, funcname);
    DBG_HFS_NODE_CHECK(ap->a_fdvp);
    DBG_HFS_NODE_CHECK(ap->a_tdvp);

#if DIAGNOSTIC
    if ((target_cnp->cn_flags & HASBUF) == 0 ||
        (source_cnp->cn_flags & HASBUF) == 0)
        panic("hfs_rename: no name");
#endif

    ASSERT((ap->a_fdvp->v_type == VDIR) && (ap->a_tdvp->v_type == VDIR));
    target_hp = targetPar_hp = source_hp = sourcePar_hp = 0;
	targetnamelen = target_cnp->cn_namelen;

    /*
     * Check for cross-device rename.
     */
    if ((source_vp->v_mount != targetPar_vp->v_mount) ||
        (target_vp && (source_vp->v_mount != target_vp->v_mount))) {
        retval = EXDEV;
        goto abortit;
    }

    /*
     * Check for access permissions
     */
    if (target_vp && ((VTOH(target_vp)->h_meta->h_pflags & (IMMUTABLE | APPEND)) ||
                      (VTOH(targetPar_vp)->h_meta->h_pflags & APPEND))) {
        retval = EPERM;
        goto abortit;
    }

    if ((retval = vn_lock(source_vp, LK_EXCLUSIVE, p)))
        goto abortit;

    sourcePar_hp = VTOH(sourcePar_vp);
    source_hp = VTOH(source_vp);
    oldparent = H_FILEID(sourcePar_hp);
    if ((source_hp->h_meta->h_pflags & (IMMUTABLE | APPEND)) || (sourcePar_hp->h_meta->h_pflags & APPEND)) {
        VOP_UNLOCK(source_vp, 0, p);
        retval = EPERM;
        goto abortit;
    }

    /*
     * Be sure we are not renaming ".", "..", or an alias of ".". This
     * leads to a crippled directory tree.  It's pretty tough to do a
     * "ls" or "pwd" with the "." directory entry missing, and "cd .."
     * doesn't work if the ".." entry is missing.
     */
    if ((source_hp->h_meta->h_mode & IFMT) == IFDIR) {
        if ((source_cnp->cn_namelen == 1 && source_cnp->cn_nameptr[0] == '.')
            || sourcePar_hp == source_hp
            || (source_cnp->cn_flags&ISDOTDOT)
            || (source_hp->h_meta->h_nodeflags & IN_RENAME)) {
            VOP_UNLOCK(source_vp, 0, p);
            retval = EINVAL;
            goto abortit;
        }
        source_hp->h_meta->h_nodeflags |= IN_RENAME;
        doingdirectory = TRUE;
    }

    // Transit between abort and bad

    targetPar_hp = VTOH(targetPar_vp);
    target_hp = target_vp ? VTOH(target_vp) : NULL;
    newparent = H_FILEID(targetPar_hp);

    retval = VOP_ACCESS(source_vp, VWRITE, target_cnp->cn_cred, target_cnp->cn_proc);
    if (doingdirectory && (newparent != oldparent)) {
        if (retval)		/* write access check above */
            goto bad;
    }

    /*
     * If the destination exists, then be sure its type (file or dir)
     * matches that of the source.  And, if it is a directory make sure
     * it is empty.  Then delete the destination.
     */
    if (target_vp) {
        if (target_hp->h_dev != targetPar_hp->h_dev || target_hp->h_dev != source_hp->h_dev)
            panic("rename: EXDEV");

        /*
         * If the parent directory is "sticky", then the user must
         * own the parent directory, or the destination of the rename,
         * otherwise the destination may not be changed (except by
                                                         * root). This implements append-only directories.
         */
        if ((targetPar_hp->h_meta->h_mode & S_ISTXT) && target_cnp->cn_cred->cr_uid != 0 &&
            target_cnp->cn_cred->cr_uid != targetPar_hp->h_meta->h_uid &&
            target_hp->h_meta->h_uid != target_cnp->cn_cred->cr_uid) {
            retval = EPERM;
            goto bad;
        }

		/*
		 * VOP_REMOVE will vput targetPar_vp so we better bump 
		 * its ref count and relockit, always set target_vp to
		 * NULL afterwards to indicate that were done with it.
		 */
		VREF(targetPar_vp);
		if (target_vp->v_type == VREG) cache_purge(VTOH(target_vp)->h_relative);
		retval = VOP_REMOVE(targetPar_vp, target_vp, target_cnp);
		(void) vn_lock(targetPar_vp, LK_EXCLUSIVE, p);

		target_vp = NULL;
        target_hp = NULL;		
		
        if (retval) goto bad;

    } else {
        if (targetPar_hp->h_dev != source_hp->h_dev)
            panic("rename: EXDEV");
    };


	if (newparent != oldparent)
		vn_lock(sourcePar_vp, LK_EXCLUSIVE | LK_RETRY, p);

	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(source_vp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
	if (retval) goto badcataloglock;
	
	/* remove the existing entry from the namei cache: */
	if (source_vp->v_type == VREG) cache_purge(VTOH(source_vp)->h_relative);

	retval = hfsMoveRename(	HTOVCB(source_hp), H_DIRID(source_hp), H_NAME(source_hp),
							H_FILEID(VTOH(targetPar_vp)), target_cnp->cn_nameptr, &H_HINT(source_hp));

	if (retval == 0) {	
	    /* Look up the catalog entry just renamed since it might have been auto-decomposed */
	    catInfo.hint = H_HINT(source_hp);
	    retval = hfsLookup(HTOVCB(source_hp), H_FILEID(VTOH(targetPar_vp)), target_cnp->cn_nameptr, -1, 0, &catInfo);
		targetnamelen = strlen(catInfo.spec.name);
    }

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(source_vp), kHFSCatalogFileID, LK_RELEASE, p);

	if (newparent != oldparent)
		VOP_UNLOCK(sourcePar_vp, 0, p);

	if (retval)  goto bad;

	H_DIRID(source_hp) = H_FILEID(VTOH(targetPar_vp));

    if (targetnamelen > source_hp->h_meta->h_namelen) {
        if (source_hp->h_meta->h_nodeflags & IN_LONGNAME)
          {
            FREE(H_NAME(source_hp), M_HFSNODE);
            MALLOC(H_NAME(source_hp), char *, targetnamelen+1, M_HFSNODE, M_WAITOK);
          }
        else if (targetnamelen > MAXHFSVNODELEN) {
            MALLOC(H_NAME(source_hp), char *, targetnamelen+1, M_HFSNODE, M_WAITOK);
            source_hp->h_meta->h_nodeflags |= IN_LONGNAME;
        }
    };

    source_hp->h_meta->h_namelen = targetnamelen;
    copystr(catInfo.spec.name, H_NAME(source_hp), targetnamelen+1, NULL);
    source_hp->h_meta->h_nodeflags &= ~IN_RENAME;

#if 0
    source_hp->h_meta->h_nodeflags |= IN_CHANGE;
    tv = time;
    if ((retval = VOP_UPDATE(source_vp, &tv, &tv, 1))) {
        VOP_UNLOCK(source_vp, 0, p);
        goto bad;
    }
#endif

	/* Copy common fcb info to complex node */
	tp = source_hp->h_relative;
    if (tp) {
		H_FILEID(VTOH(tp)) = H_FILEID(source_hp);
	}

	/* Now copy common fcb info to siblings */
	tp = source_hp->h_sibling;
    while (tp) {
		H_FILEID(VTOH(tp)) = H_FILEID(source_hp);
        tp = VTOH(tp)->h_sibling;
        if (tp == source_vp)
			break;
	}

	/* Timestamp the parents, if this is a move */
    if (newparent != oldparent) {
        targetPar_hp->h_meta->h_nodeflags |= IN_UPDATE;
        sourcePar_hp->h_meta->h_nodeflags |= IN_UPDATE;
        tv = time;
        HFSTIMES(targetPar_hp, &tv, &tv);
        HFSTIMES(sourcePar_hp, &tv, &tv);
        };

	VPUT(targetPar_vp);
	VRELE(sourcePar_vp);
    VPUT(source_vp);

    DBG_VOP_LOCKS_TEST(retval);
    if (retval != E_NONE) {
        DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("\tReturning with error %d\n",retval));
    }
    return (retval);

badcataloglock:
	if (newparent != oldparent)
		VOP_UNLOCK(sourcePar_vp, 0, p);
	
bad:;
    if (retval && doingdirectory)
    	source_hp->h_meta->h_nodeflags &= ~IN_RENAME;

    if (targetPar_vp == target_vp)
	    VRELE(targetPar_vp);
    else
	    VPUT(targetPar_vp);

    if (target_vp)
	    VPUT(target_vp);

	VRELE(sourcePar_vp);

    if (VOP_ISLOCKED(source_vp))
        VPUT(source_vp);
	else
    	VRELE(source_vp);

    DBG_VOP_LOCKS_TEST(retval);
    if (retval != E_NONE) {
        DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("\tReturning with error %d\n",retval));
    }
    return (retval);

abortit:;

    VOP_ABORTOP(targetPar_vp, target_cnp); /* XXX, why not in NFS? */

    if (targetPar_vp == target_vp)
	    VRELE(targetPar_vp);
    else
	    VPUT(targetPar_vp);

    if (target_vp)
	    VPUT(target_vp);

    VOP_ABORTOP(sourcePar_vp, source_cnp); /* XXX, why not in NFS? */

	VRELE(sourcePar_vp);
    VRELE(source_vp);

    DBG_VOP_LOCKS_TEST(retval);
    if (retval != E_NONE) {
        DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT(("\tReturning with error %d\n",retval));
    }
    return (retval);
}



/*
 * Mkdir system call
#% mkdir	dvp	L U U
#% mkdir	vpp	- L -
#
 vop_mkdir {
     IN WILLRELE struct vnode *dvp;
     OUT struct vnode **vpp;
     IN struct componentname *cnp;
     IN struct vattr *vap;

     We are responsible for freeing the namei buffer, it is done in hfs_makenode(), unless there is
    a previous error.

*/

int
hfs_mkdir(ap)
struct vop_mkdir_args /* {
    struct vnode *a_dvp;
    struct vnode **a_vpp;
    struct componentname *a_cnp;
    struct vattr *a_vap;
} */ *ap;
{
	struct proc		*p = CURRENT_PROC;
    int				retval;
    int				mode = MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode);

    DBG_FUNC_NAME("mkdir");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_LOCKED, VOPDBG_IGNORE, VOPDBG_POS);

    DBG_VOP(("%s: parent 0x%x (%s)  ap->a_cnp->cn_nameptr %s\n", funcname, (u_int)VTOH(ap->a_dvp), H_NAME(VTOH(ap->a_dvp)), ap->a_cnp->cn_nameptr));
    WRITE_CK( ap->a_dvp, funcname);
    DBG_HFS_NODE_CHECK(ap->a_dvp);
    ASSERT(ap->a_dvp->v_type == VDIR);

 	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
    if (retval != E_NONE) {
    	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
		VPUT(ap->a_dvp);
        DBG_VOP_LOCKS_TEST( retval);
        return (retval);
    }

	/* Create the vnode */
    DBG_ASSERT((ap->a_cnp->cn_flags & SAVESTART) == 0);
	retval = hfs_makenode(mode, ap->a_dvp, ap->a_vpp, ap->a_cnp);
    DBG_VOP_UPDATE_VP(1, *ap->a_vpp);

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_RELEASE, p);

    if (retval != E_NONE) {
        DBG_ERR(("%s: hfs_makenode FAILED: %s, %s\n", funcname, ap->a_cnp->cn_nameptr, H_NAME(VTOH(ap->a_dvp))));
        DBG_VOP_LOCKS_TEST(retval);
        return (retval);		
    }

    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}

/*
 * Rmdir system call.
#% rmdir	dvp	L U U
#% rmdir	vp	L U U
#
 vop_rmdir {
     IN WILLRELE struct vnode *dvp;
     IN WILLRELE struct vnode *vp;
     IN struct componentname *cnp;

     */

int
hfs_rmdir(ap)
struct vop_rmdir_args /* {
    struct vnode *a_dvp;
    struct vnode *a_vp;
    struct componentname *a_cnp;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct vnode *dvp = ap->a_dvp;
    struct hfsnode *hp = VTOH(vp);
	struct proc *p = CURRENT_PROC;
    int retval;
    DBG_FUNC_NAME("rmdir");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP(("\tParent: "));DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);DBG_VOP_CONT(("\n"));
    DBG_VOP(("\tTarget: "));DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP(("\tTarget Name: "));DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,ap->a_vp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);

    if (dvp == vp) {
        VRELE(vp);
        VPUT(vp);
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

    if (vp->v_usecount > 2) {
        DBG_ERR(("%s: dir is busy, usecount is %d\n", funcname, vp->v_usecount ));
		retval = EBUSY;
		goto Err_Exit;
    }

	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
	if (retval != E_NONE) {
		goto Err_Exit;
	}

	/* remove the entry from the namei cache: */
	cache_purge(vp);

	/* remove entry from catalog */
    retval = hfsDelete (HTOVCB(hp), H_DIRID(hp), H_NAME(hp), FALSE, H_HINT(hp));
    hp->h_meta->h_mode = 0;				/* Makes the vnode go away...see inactive */

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(vp), kHFSCatalogFileID, LK_RELEASE, p);

	/* Set the parent to be updated */
    if (! retval)
    	VTOH(dvp)->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;

Err_Exit:;
    if (dvp != 0) 
		VPUT(dvp);
    VPUT(vp);

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}

/*
 * symlink -- make a symbolic link
#% symlink	dvp	L U U
#% symlink	vpp	- U -
#
# XXX - note that the return vnode has already been VRELE'ed
#	by the filesystem layer.  To use it you must use vget,
#	possibly with a further namei.
#
 vop_symlink {
     IN WILLRELE struct vnode *dvp;
     OUT WILLRELE struct vnode **vpp;
     IN struct componentname *cnp;
     IN struct vattr *vap;
     IN char *target;

     We are responsible for freeing the namei buffer, it is done in hfs_makenode(), unless there is
    a previous error.


*/

int
hfs_symlink(ap)
    struct vop_symlink_args /* {
        struct vnode *a_dvp;
        struct vnode **a_vpp;
        struct componentname *a_cnp;
        struct vattr *a_vap;
        char *a_target;
    } */ *ap;
{
    register struct vnode *vp, **vpp = ap->a_vpp;
	struct proc *p = CURRENT_PROC;
    int len, retval;
    DBG_FUNC_NAME("symlink");
    DBG_VOP_LOCKS_DECL(2);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    DBG_VOP_LOCKS_INIT(1,*ap->a_vpp, VOPDBG_IGNORE, VOPDBG_UNLOCKED, VOPDBG_IGNORE, VOPDBG_POS);

    if (VTOVCB(ap->a_dvp)->vcbSigWord != kHFSPlusSigWord) {
    	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
        VPUT(ap->a_dvp);
        DBG_VOP((" ...sorry HFS disks don't support symbolic links.\n"));
        DBG_VOP_LOCKS_TEST(EOPNOTSUPP);
        return (EOPNOTSUPP);
    }

	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_EXCLUSIVE, p);
	if (retval != E_NONE) {
    	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
    	VPUT(ap->a_dvp);
        DBG_VOP_LOCKS_TEST( retval);
        return (retval);
	}

	/* Create the vnode */
	retval = hfs_makenode(IFLNK | ap->a_vap->va_mode, ap->a_dvp, vpp, ap->a_cnp);
    DBG_VOP_UPDATE_VP(1, *ap->a_vpp);

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(ap->a_dvp), kHFSCatalogFileID, LK_RELEASE, p);

    if (retval != E_NONE) {
        DBG_VOP_LOCKS_TEST(retval);
        return (retval);
	}


    vp = *vpp;
    len = strlen(ap->a_target);
    retval = vn_rdwr(UIO_WRITE, vp, ap->a_target, len, (off_t)0,
                     UIO_SYSSPACE, IO_NODELOCKED, ap->a_cnp->cn_cred, (int *)0,
                     (struct proc *)0);


    VPUT(vp);
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}

/*
 * Dummy dirents to simulate the "." and ".." entries of the directory
 * in a hfs filesystem.  HFS doesn't provide these. Note that each entry
 * must be the same size as a hfs directory entry (44 bytes).
 */
static hfsdirentry  rootdots[2] = {
    {
        1,								/* d_fileno			 */
        sizeof(struct hfsdirentry),		/* d_reclen			 */
        DT_DIR,							/* d_type			 */
        1,								/* d_namlen			 */
        "."								/* d_name			 */
    },
    {
        1,								/* d_fileno			 */
        sizeof(struct hfsdirentry),		/* d_reclen			 */
        DT_DIR,							/* d_type			 */
        2,								/* d_namlen			 */
        ".."							/* d_name			 */
    }
};


/*	4.3 Note:
*	There is some confusion as to what the semantics of uio_offset are.
*	In ufs, it represents the actual byte offset within the directory
*	"file."  HFS, however, just uses it as an entry counter - essentially
*	assuming that it has no meaning except to the hfs_readdir function.
*	This approach would be more efficient here, but some callers may
*	assume the uio_offset acts like a byte offset.  NFS in fact
*	monkeys around with the offset field a lot between readdir calls.
*
*	We could also speed things up by remembering the last offset position,
*	but its not clear that that would buy us much (do readdirs need to
                                                 *	be fast?) ??
*
*	The use of the resid uiop->uio_resid and uiop->uio_iov->iov_len
*	fields is a mess as well.  The libc function readdir() returns
*	NULL (indicating the end of a directory) when either
*	the getdirentries() syscall (which calls this and returns
                               *	the size of the buffer passed in less the value of uiop->uio_resid)
*	returns 0, or a direct record with a d_reclen of zero.
*	nfs_server.c:rfs_readdir(), on the other hand, checks for the end
*	of the directory by testing uiop->uio_resid == 0.  The solution
*	is to pad the size of the last struct direct in a given
*	block to fill the block if we are not at the end of the directory.
*/

/*
 * NOTE: We require a minimal buffer size of DIRBLKSIZ for two reasons. One, it is the same value
 * returned be stat() call as the block size. This is mentioned in the man page for getdirentries():
 * "Nbytes must be greater than or equal to the block size associated with the file,
 * see stat(2)". Might as well settle on the same size of ufs. Second, this makes sure there is enough
 * room for the . and .. entries that have to added manually.
 */

/* 			
#% readdir	vp	L L L
#
vop_readdir {
    IN struct vnode *vp;
    INOUT struct uio *uio;
    IN struct ucred *cred;
    INOUT int *eofflag;
    OUT int *ncookies;
    INOUT u_long **cookies;
    */


static int
hfs_readdir(ap)
struct vop_readdir_args /* {
    struct vnode *vp;
    struct uio *uio;
    struct ucred *cred;
    int *eofflag;
    int *ncookies;
    u_long **cookies;
} */ *ap;
{
    register struct uio *uio = ap->a_uio;
    struct hfsnode 		*hp = VTOH(ap->a_vp);
	struct proc			*p = CURRENT_PROC;
    ExtendedVCB 		*vcb = HTOVCB(hp);
    off_t 				off = uio->uio_offset;
    size_t 				count;
    size_t				lost = 0;
    CatalogNodeData 	nodeData;
    struct hfsdirentry	catalogEntry;
    FSSpec 				fileSpec;	/* 264 bytes */
    UInt32 				dirID = H_FILEID(hp);
    UInt32				index = 0;
    UInt32				origOffset;
    UInt32				hint;
    Boolean				eofReached = FALSE;
    int					retval = 0;
    OSErr				result = noErr;

    DBG_FUNC_NAME("readdir");
    DBG_VOP_LOCKS_DECL(1);

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_HFS_NODE_CHECK(ap->a_vp);

    /* We assume it's all one big buffer... */
    if (uio->uio_iovcnt > 1) DEBUG_BREAK_MSG(("hfs_readdir: uio->uio_iovcnt = %d?\n", uio->uio_iovcnt));

    origOffset = uio->uio_offset;
    count = uio->uio_resid;
    /* Make sure we don't return partial entries.  */
    count -= ((uio->uio_offset + count) % sizeof(hfsdirentry));
    if (count <= 0) {
        DBG_ERR(("%s: Not enough buffer to read in entries\n",funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

    /* Adjust uio to be on correct boundaries */
    lost = uio->uio_resid - count;
    uio->uio_resid = count;
    uio->uio_iov->iov_len = count;

    DBG_VOP(("%s: offset Ox%lX, bytes Ox%lX\n",funcname,
             (u_long)uio->uio_offset, uio->uio_iov->iov_len));

    /* Create the entries for . and ..
        * We do it here since we are assuming that the directory is guarenteed to exist
        */
    index = 0;
    if (uio->uio_offset < (2 * sizeof(struct hfsdirentry))) {
        if ((uio->uio_offset > 0) && (uio->uio_offset != sizeof(struct hfsdirentry))) {
            retval = EINVAL;
            goto Exit;
        }
        index = 2;
        if (uio->uio_offset == sizeof(struct hfsdirentry)) {
            index = 1;
        }

		/* copy in the correct d_fileno */
        rootdots[0].fileno = dirID;
        rootdots[1].fileno = H_DIRID(hp);

        retval = uiomove((caddr_t) (rootdots + uio->uio_offset), index * sizeof(struct hfsdirentry), uio);
		if (retval != 0)
			goto Exit;

    };

    /* Compute the starting index in the directory */
    index = (uio->uio_offset - sizeof(struct hfsdirentry)) / sizeof(struct hfsdirentry);

	/* lock catalog b-tree */
	retval = hfs_metafilelocking(VTOHFS(ap->a_vp), kHFSCatalogFileID, LK_SHARED, p);
    if (retval != E_NONE) {
		goto Exit;
    };

    hint = kNoHint;
    while (uio->uio_resid > sizeof(struct hfsdirentry))
      {
        result = GetCatalogOffspring(vcb, dirID, index, &fileSpec, &nodeData, &hint);
        if (result != noErr) {
            if (result == cmNotFound) {
                eofReached = TRUE;
                if (origOffset == uio->uio_offset) {		/* we were already past eof */
                    uio->uio_offset = 0;
                    retval = E_NONE;
					/* unlock catalog b-tree */
 					(void) hfs_metafilelocking(VTOHFS(ap->a_vp),
 												kHFSCatalogFileID, LK_RELEASE, p);
					goto Exit;
                }
                result = noErr;
            }
            retval = MacToVFSError(result);
            break;
        }

        /* Copy entry into the buffer */
        catalogEntry.fileno = nodeData.nodeID;
        catalogEntry.reclen = sizeof(struct hfsdirentry);
        catalogEntry.type = (nodeData.nodeType == kCatalogFolderNode) ? DT_DIR : DT_REG;
        catalogEntry.namelen = strlen(fileSpec.name);
		(void) strncpy(catalogEntry.name, fileSpec.name, sizeof(catalogEntry.name));

        /* copy this entry into the user's buffer: */
        retval = uiomove((caddr_t) &catalogEntry, sizeof(struct hfsdirentry), uio);
        if (retval != E_NONE) {
            DBG_ERR(("%s: uiomove returned %d.\n", funcname, retval));
            break;
        };
        ++index;
      };

	/* unlock catalog b-tree */
	(void) hfs_metafilelocking(VTOHFS(ap->a_vp), kHFSCatalogFileID, LK_RELEASE, p);

    if (retval != E_NONE) {
        DBG_ERR(("%s: retval %d when trying to read directory %ld: %s\n",funcname, retval,
                H_FILEID(hp), H_NAME(hp)));
		goto Exit;
    }
    else if (vcb->vcbSigWord == kHFSPlusSigWord)
    	hp->h_meta->h_nodeflags |= IN_ACCESS;

    /* Bake any cookies */
    if (!retval && ap->a_ncookies != NULL) {
        struct dirent* dpStart;
        struct dirent* dpEnd;
        struct dirent* dp;
        int ncookies;
        u_long *cookies;
        u_long *cookiep;

        /*
        * Only the NFS server uses cookies, and it loads the
        * directory block into system space, so we can just look at
        * it directly.
        */
        if (uio->uio_segflg != UIO_SYSSPACE || uio->uio_iovcnt != 1)
            panic("hfs_readdir: unexpected uio from NFS server");
        dpStart = (struct dirent *)
            (uio->uio_iov->iov_base - (uio->uio_offset - off));
        dpEnd = (struct dirent *) uio->uio_iov->iov_base;
        for (dp = dpStart, ncookies = 0;
            dp < dpEnd && dp->d_reclen != 0;
            dp = (struct dirent *)((caddr_t)dp + dp->d_reclen))
            ncookies++;
        MALLOC(cookies, u_long *, ncookies * sizeof(u_long), M_TEMP,
            M_WAITOK);
        for (dp = dpStart, cookiep = cookies;
            dp < dpEnd;
            dp = (struct dirent *)((caddr_t) dp + dp->d_reclen)) {
            off += dp->d_reclen;
            *cookiep++ = (u_long) off;
        }
        *ap->a_ncookies = ncookies;
        *ap->a_cookies = cookies;
    }

Exit:;
	uio->uio_resid += lost;

    if (ap->a_eofflag)
        *ap->a_eofflag = eofReached;

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*
 * readdirattr operation
 */

/* 			

#
#% readdirattr	vp	L L L
#
vop_readdirattr {
	IN struct vnode *vp;
	IN struct attrlist *alist;
	INOUT struct uio *uio;
	INOUT int index;
	INOUT int *eofflag;
	OUT u_long *ncookies;
	INOUT u_long **cookies;
	IN struct ucred *cred;
};

*/


static int
hfs_readdirattr(ap)
struct vop_readdirattr_args /* {
    struct vnode *vp;
    struct attrlist *alist;
    struct uio *uio;
    int index;
    int *eofflag;
    u_long *ncookies;
    u_long **cookies;
    struct ucred *cred;
} */ *ap;
{
    struct vnode 		*vp = ap->a_vp;
    struct attrlist 	*alist = ap->a_alist;
    register struct uio *uio = ap->a_uio;
    struct hfsnode 		*hp = VTOH(ap->a_vp);
    ExtendedVCB 		*vcb = HTOVCB(hp);
    off_t 				off = uio->uio_offset;
    size_t 				count;
    struct hfsCatalogInfo catalogInfo;
    struct hfsCatalogInfo *catInfoPtr = NULL;
    UInt32 				dirID = H_FILEID(hp);
    UInt32				index = 0;
    OSErr				result = noErr;
    UInt32				origOffset;
    Boolean				eofReached = FALSE;
    int					retval = 0;
    u_long 				fixedblocksize;
    u_long 				maxattrblocksize;
	u_long				currattrbufsize;
    void 				*attrbufptr = NULL;
    void 				*attrptr;
    void 				*varptr;
    DBG_FUNC_NAME("readdirattr");
    DBG_VOP_LOCKS_DECL(1);
    
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_HFS_NODE_CHECK(ap->a_vp);

    if ((alist->bitmapcount != ATTR_BIT_MAP_COUNT) ||
        ((alist->commonattr & ~ATTR_CMN_VALIDMASK) != 0) ||
        ((alist->dirattr & ~ATTR_DIR_VALIDMASK) != 0) ||
        ((alist->fileattr & ~ATTR_FILE_VALIDMASK) != 0) ||
        ((alist->forkattr & ~ATTR_FORK_VALIDMASK) != 0)) {
        DBG_ERR(("%s: bad attrlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    };

    /* Requesting volume information is illegal : */
    if (alist->volattr != 0) {
        DBG_ERR(("%s: requesting volume attribute is illegal\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    };

    /* Reject requests for unsupported options for now: */
    if ((alist->commonattr & (ATTR_CMN_NAMEDATTRCOUNT | ATTR_CMN_NAMEDATTRLIST)) ||
        (alist->fileattr & (ATTR_FILE_FILETYPE | ATTR_FILE_FORKCOUNT | ATTR_FILE_FORKLIST))) {
        DBG_ERR(("%s: illegal bits in attlist\n", funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return EINVAL;
    };

    origOffset 	= uio->uio_offset;
    count 		= uio->uio_resid;

  /* Make sure we don't return partial entries.  */
    count -= ((uio->uio_offset + count) % sizeof(hfsdirentry));
    if (count <= 0) {
        DBG_ERR(("%s: Not enough buffer to read in entries\n",funcname));
        DBG_VOP_LOCKS_TEST(EINVAL);
        return (EINVAL);
    }

    DBG_VOP(("%s: offset Ox%lX, bytes Ox%lX\n",funcname,
             (u_long)uio->uio_offset, (u_long)uio->uio_iov->iov_len));

	/* Preflight and alloc buffer to do packings */
    maxattrblocksize = fixedblocksize = (sizeof(u_long) + AttributeBlockSize(alist));	/* u_long for length longword */
    if (alist->commonattr & ATTR_CMN_NAME) maxattrblocksize += NAME_MAX + 1;
    if (alist->commonattr & ATTR_CMN_NAMEDATTRLIST) maxattrblocksize += 0;			/* XXX PPD */
    if (alist->fileattr & ATTR_FILE_FORKLIST) maxattrblocksize += 0;				/* XXX PPD */

    DBG_VOP(("%s: allocating Ox%lX byte buffer (Ox%lX + Ox%lX) for attributes...\n",
                funcname,
             	maxattrblocksize,
                fixedblocksize,
             	maxattrblocksize - fixedblocksize));
    MALLOC(attrbufptr, void *, maxattrblocksize, M_TEMP, M_WAITOK);
    attrptr = attrbufptr;
    varptr = attrbufptr + fixedblocksize;					/* Point to variable-length storage */
    DBG_VOP(("%s: attrptr = 0x%08X, varptr = 0x%08X...\n", funcname, (u_int)attrptr, (u_int)varptr));

#if 0
    /* Create the entries for . and ..
     * We do it here since we are assuming that the directory is guarenteed to exist
     */
    index = 0;
    if (uio->uio_offset < (2 * sizeof(struct hfsdirentry))) {
        if ((uio->uio_offset > 0) && (uio->uio_offset != sizeof(struct hfsdirentry))) {
            retval = EINVAL;
            goto Exit;
        }
        index = 2;
        if (uio->uio_offset == sizeof(struct hfsdirentry)) {
            index = 1;
        }
        retval = uiomove((caddr_t) (rootdots + uio->uio_offset), index * sizeof(struct hfsdirentry), uio);
    };
#endif

   /* Compute the starting index in the directory */
    index = (uio->uio_offset - sizeof(struct hfsdirentry)) / sizeof(struct hfsdirentry);

    catalogInfo.hint = kNoHint;
    while (uio->uio_resid > sizeof(struct hfsdirentry))
      {

        result = GetCatalogOffspring(vcb, dirID, index, &catalogInfo.spec, &catalogInfo.nodeData, &catalogInfo.hint);
        if (result != noErr) {
            if (result == cmNotFound) {
                eofReached = TRUE;
                if (origOffset == uio->uio_offset) {		/* we were already past eof */
                    uio->uio_offset = 0;
                    retval = E_NONE;
                    goto Err_Exit;
                }
                result = noErr;
            }
            retval = MacToVFSError(result);
            break;
        }
        catInfoPtr = &catalogInfo;

        *((u_long *)attrptr)++ = 0;			/* Reserve space for length field */
        PackAttributeBlock(alist, vp, catInfoPtr, &attrptr, &varptr);
        currattrbufsize = *((u_long *)attrbufptr) = (varptr - attrbufptr);		/* Store length of fixed + var block */

        /* Make sure that there is enough room to copy to */
        if (currattrbufsize > uio->uio_resid)
          {
            if (uio->uio_offset == origOffset)
              {
                DBG_ERR(("%s: Not enough buffer to read in entries\n",funcname));
                retval = EINVAL;
                goto Err_Exit;
              }
            break;
          }

        DBG_VOP(("%s: copying Ox%lX bytes to user address 0x%08X.\n",funcname, currattrbufsize, (u_int)ap->a_uio->uio_iov->iov_base));
        retval = uiomove((caddr_t)attrbufptr, currattrbufsize, ap->a_uio);
        if (retval != E_NONE) {
            DBG_ERR(("%s: error %d on uiomove.\n",funcname, retval));
            break;
        };
        attrptr = (void *)((u_long)attrptr + currattrbufsize);
        ++index;
      };

   if (retval != E_NONE) {
        DBG_ERR(("%s: retval %d when trying to read directory %ld: %s\n",funcname, retval,
                H_FILEID(hp), H_NAME(hp)));
       retval = EINVAL;
    };

    /* Bake any cookies */
    if (!retval && ap->a_ncookies != NULL) {
        struct dirent* dpStart;
        struct dirent* dpEnd;
        struct dirent* dp;
        int ncookies;
        u_long *cookies;
        u_long *cookiep;

        /*
        * Only the NFS server uses cookies, and it loads the
        * directory block into system space, so we can just look at
        * it directly.
        */
        if (uio->uio_segflg != UIO_SYSSPACE || uio->uio_iovcnt != 1)
            panic("hfs_readdir: unexpected uio from NFS server");
        dpStart = (struct dirent *)
            (uio->uio_iov->iov_base - (uio->uio_offset - off));
        dpEnd = (struct dirent *) uio->uio_iov->iov_base;
        for (dp = dpStart, ncookies = 0;
            dp < dpEnd && dp->d_reclen != 0;
            dp = (struct dirent *)((caddr_t)dp + dp->d_reclen))
            ncookies++;
        MALLOC(cookies, u_long *, ncookies * sizeof(u_long), M_TEMP,
            M_WAITOK);
        for (dp = dpStart, cookiep = cookies;
            dp < dpEnd;
            dp = (struct dirent *)((caddr_t) dp + dp->d_reclen)) {
            off += dp->d_reclen;
            *cookiep++ = (u_long) off;
        }
        *ap->a_ncookies = ncookies;
        *ap->a_cookies = cookies;
    }

Err_Exit:;

    if (attrbufptr != NULL)
		FREE(attrbufptr, M_TEMP);

    if (ap->a_eofflag)
    	*ap->a_eofflag = eofReached;

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*
 * Return target name of a symbolic link
#% readlink	vp	L L L
#
 vop_readlink {
     IN struct vnode *vp;
     INOUT struct uio *uio;
     IN struct ucred *cred;
     */

int
hfs_readlink(ap)
struct vop_readlink_args /* {
struct vnode *a_vp;
struct uio *a_uio;
struct ucred *a_cred;
} */ *ap;
{
    int retval;
    DBG_FUNC_NAME("readlink");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);
    retval = VOP_READ(ap->a_vp, ap->a_uio, 0, ap->a_cred);
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);

}


/*
 * hfs abort op, called after namei() when a CREATE/DELETE isn't actually
 * done. If a buffer has been saved in anticipation of a CREATE, delete it.
#% abortop	dvp	= = =
#
 vop_abortop {
     IN struct vnode *dvp;
     IN struct componentname *cnp;

     */

/* ARGSUSED */

static int
hfs_abortop(ap)
struct vop_abortop_args /* {
    struct vnode *a_dvp;
    struct componentname *a_cnp;
} */ *ap;
{
    DBG_FUNC_NAME("abortop");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_dvp);
    DBG_VOP_PRINT_CPN_INFO(ap->a_cnp);DBG_VOP_CONT(("\n"));


    DBG_VOP_LOCKS_INIT(0,ap->a_dvp, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);

    if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF) {
        FREE_ZONE(ap->a_cnp->cn_pnbuf, ap->a_cnp->cn_pnlen, M_NAMEI);
    }
    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}

// int	prthfsactive = 0;		/* 1 => print out reclaim of active vnodes */

/*
#% inactive	vp	L U U
#
 vop_inactive {
     IN struct vnode *vp;
     IN struct proc *p;

     */

static int
hfs_inactive(ap)
struct vop_inactive_args /* {
    struct vnode *a_vp;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct hfsnode *hp = VTOH(vp);
    struct proc *p = ap->a_p;
    struct timeval tv;
    extern int prtactive;

    DBG_FUNC_NAME("inactive");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_ZERO);

    /*
        NOTE: vnodes need careful handling because fork vnodes that failed to be created
              in their entirity could be getting cleaned up here, in which case h_meta == NULL...
     */
    if (prtactive && vp->v_usecount <= 0)
        vprint("hfs_inactive: pushing active", vp);

    if (vp->v_usecount != 0)
        DBG_VOP(("%s: bad usecount = %d\n",funcname,vp->v_usecount ));

    /* 
	 * Skip all complex nodes 
	 * At this point, there should be no fork nodes existing for its complex 
	 */
    if (vp->v_type == VCPLX) {
#if DIAGNOSTIC
        if (vp->v_usecount == 0) {
            struct vnode *tvp;
            tvp  = hfs_vhashget(hp->h_dev, H_FILEID(hp), kDataFork);
            DBG_ASSERT(tvp == NULL);
            if (tvp)
                vput (tvp);
            tvp  = hfs_vhashget(hp->h_dev, H_FILEID(hp), kRsrcFork);
            DBG_ASSERT(tvp == NULL);
            if (tvp)
                vput (tvp);
        }
#endif	/* DIAGNOSTIC */
        hp->h_meta->h_mode = 0;				/* Makes the node go away */
        goto out;
    }

	/*
     * Ignore inodes related to stale file handles.
     */
    if ((vp->v_type == VNON) || (hp->h_meta->h_mode == 0))
        goto out;
	
    if (hp->h_meta->h_nodeflags & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) {
        tv = time;
        VOP_UPDATE(vp, &tv, &tv, 0);
    }

out:
    VOP_UNLOCK(vp, 0, p);
    /*
     * If we are done with the inode, reclaim it
     * so that it can be reused immediately.
     */
    if ((vp->v_type == VNON) || (hp->h_meta->h_mode == 0))
        vrecycle(vp, (struct slock *)0, p);

    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}

/*
 Ignored since the locks are gone......
#% reclaim	vp	U I I
#
 vop_reclaim {
     IN struct vnode *vp;
     IN struct proc *p;

     */

static int
hfs_reclaim(ap)
struct vop_reclaim_args /* {
    struct vnode *a_vp;
} */ *ap;
{
    struct vnode *vp = ap->a_vp;
    struct vnode *tp = NULL;
    struct hfsnode *hp = VTOH(vp);
    extern int prtactive;
    DBG_FUNC_NAME("reclaim");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));

    DBG_VOP_LOCKS_INIT(0, ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_ZERO);

    /*
        NOTE: vnodes need careful handling because fork vnodes that failed to be
              created in their entirity could be getting cleaned up here, in which
              case v_type == VNON and h_meta == NULL...
     */

    if (prtactive && vp->v_usecount != 0)
        vprint("hfs_reclaim(): pushing active", vp);

    hfs_vhashrem(hp);	

    /* release the file meta */
    if ((vp->v_type == VREG) || (vp->v_type == VLNK)) {
        DBG_ASSERT(hp->h_meta != NULL);

        if (H_FORKTYPE(hp) == kSysFile) {
            /* XXX SER Here we release meta for kSysFile, which do not need it!!! */
            FREE(hp->h_meta, M_HFSNODE);
            hp->h_meta = NULL;
        }
        else {
            HFSFILEMETA_LOCK_EXCLUSIVE(hp, CURRENT_PROC);
            hp->h_meta->h_usecount--;							/* XXX SER eventually remove h_usecount */

			/*
			 * if there are still more forks, make the fork ptr point to another
			 */
            if ((hp->h_meta->h_usecount > 0) && (hp->h_meta->h_fork == vp)) {
                hp->h_meta->h_fork = hp->h_sibling;
			}
				

            if ((H_FORKTYPE(hp) == kDataFork) && hp->h_relative) {
                DBG_ASSERT(VTOH(hp->h_relative)->h_relative != NULL);
                VTOH(hp->h_relative)->h_relative = NULL;		/* Mark the default node as NULL from a complex view */
            };

            if (hp->h_sibling) {
                tp = vp;
                do {
                    tp = VTOH(tp)->h_sibling;
                } while (VTOH(tp)->h_sibling != vp);
                VTOH(tp)->h_sibling = hp->h_sibling;
                if (VTOH(tp)->h_sibling == tp)					/* pointing to ourselves */
                    VTOH(tp)->h_sibling = NULL;
                hp->h_sibling = NULL;
            };

            HFSFILEMETA_UNLOCK(hp, CURRENT_PROC);
            hp->h_meta = NULL;

            /* h_relative could be NULL, if a force unmount was done, and the complex node is already gone */
            if (hp->h_relative) {
                DBG_ASSERT(H_FORKTYPE(VTOH(hp->h_relative)) == kDirCmplx);
                DBG_ASSERT((hp->h_relative)->v_type == VCPLX);
                VRELE (hp->h_relative);								/* release the complex node */
                hp->h_relative = NULL;
			}
        }
    }
    else if (vp->v_type == VCPLX) {
#if DIAGNOSTIC
		/* Unless a force unmount, no other forks should exist */
        if (vp->v_usecount == 0) {
            struct vnode *tvp;
            tvp  = hfs_vhashget(hp->h_dev, H_FILEID(hp), kDataFork);
            DBG_ASSERT(tvp == NULL);
            if (tvp)
                vput (tvp);
            tvp  = hfs_vhashget(hp->h_dev, H_FILEID(hp), kRsrcFork);
            DBG_ASSERT(tvp == NULL);
            if (tvp)
                vput (tvp);
        }
#endif	/* DIAGNOSTIC */

        DBG_ASSERT(hp->h_meta != NULL);
		tp = hp->h_meta->h_fork;
		if (tp == NULL) {
				/* No more forks, so do nothing here */
		     }
		 else {
		 	/* This should only occur when a force unmount occurs */
		 	/* so, usually, no other forks should exist */
		 	VTOH(tp)->h_relative = NULL;
		 	if (VTOH(tp)->h_sibling)
		  		VTOH(VTOH(tp)->h_sibling)->h_relative = NULL;
		  	}
		
		  if (hp->h_meta->h_nodeflags & IN_LONGNAME) {
        	DBG_ASSERT(H_NAME(hp) != NULL);
		  	FREE(H_NAME(hp), M_HFSNODE);
		  	}
		
		  FREE(hp->h_meta, M_HFSNODE);
		  hp->h_meta = NULL;
		  hp->h_relative = NULL;

    	}
    else if (vp->v_type == VDIR) {
        if (hp->h_meta->h_nodeflags & IN_LONGNAME)
            FREE(H_NAME(hp), M_HFSNODE);

        FREE(hp->h_meta, M_HFSNODE);
        hp->h_meta = NULL;
        }
	else
	    DBG_ASSERT(0);	/* Unexpected v_type !!!! XXX SER Change to panic */

    /*
     * Purge old data structures associated with the inode.
     */
    cache_purge(vp);
    if ((vp->v_type != VNON) && hp->h_devvp) {
        VRELE(hp->h_devvp);
        hp->h_devvp = 0;
    }

    /* Free our data structs */
    if (vp->v_type != VNON) {
        DBG_ASSERT(hp->h_sibling == NULL);
        DBG_ASSERT(hp->h_relative == NULL);
        DBG_ASSERT(hp->h_meta == NULL);
        FREE(hp->h_xfcb, M_HFSNODE);
        FREE(vp->v_data, M_HFSNODE);
    };
    vp->v_data = NULL;

    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}


/*
 * Lock an hfsnode. If its already locked, set the WANT bit and sleep.
#% lock		vp	U L U
#
 vop_lock {
     IN struct vnode *vp;
     IN int flags;
     IN struct proc *p;
     */

static int
hfs_lock(ap)
struct vop_lock_args /* {
    struct vnode *a_vp;
    int a_flags;
    struct proc *a_p;
} */ *ap;
{
    struct vnode * vp = ap->a_vp;
    struct hfsnode *hp = VTOH(ap->a_vp);
    int			retval;

    DBG_FUNC_NAME("lock");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();DBG_VOP_CONT((" "));
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT((" flags = 0x%08X.\n", ap->a_flags));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_ZERO);

#if DIAGNOSTIC
    DBG_ASSERT(hp != (struct hfsnode *)NULL);
    if (ap->a_flags & LK_INTERLOCK) {
        DBG_ASSERT(*((int*)&vp->v_interlock) != 0);
    } else {
        DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
    };
    if (H_FORKTYPE(hp) != kDirCmplx && (vp->v_flag & VSYSTEM) == 0) {
        DBG_ASSERT(hp->h_meta != NULL);
        if ((lockstatus(&hp->h_meta->h_fmetalock)) != (lockstatus(&hp->h_lock)))
            DBG_VOP(("hfs_lock: Warning, mismatched locks. h_lock = %d.meta = %d\n", lockstatus(&hp->h_lock), 									lockstatus(&hp->h_meta->h_fmetalock)));
    }

    /* Attempting to lock the vnode */
    if (lockstatus(&hp->h_lock) && ((ap->a_flags & LK_NOWAIT) == 0)) {
        DBG_VOP(("hfs_lock: waiting for vnode lock (forkype = %d)...\n", H_FORKTYPE(hp)));
    };
#endif	/* DIAGNOSTIC */

    retval = lockmgr(&hp->h_lock, ap->a_flags, &vp->v_interlock, ap->a_p);
    if (retval != E_NONE) {
        if ((ap->a_flags & LK_NOWAIT) == 0)
            DBG_ERR(("hfs_lock: error %d trying to lock vnode (flags = 0x%08X).\n", retval, ap->a_flags));
        goto Err_Exit;
    };

    /* Now get a lock on the file meta lock if necessary */
    if (H_FORKTYPE(hp) != kDirCmplx && (vp->v_flag & VSYSTEM) == 0) {
        if (lockstatus(&hp->h_meta->h_fmetalock) && ((ap->a_flags & LK_NOWAIT) == 0))
            DBG_VOP(("hfs_lock: waiting for meta-data lock...\n"));

        retval = lockmgr(&hp->h_meta->h_fmetalock, ap->a_flags & ~LK_INTERLOCK, (simple_lock_t) 0, ap->a_p);
        if (retval != E_NONE) {
            if ((ap->a_flags & LK_NOWAIT) == 0) {
                DBG_ERR(("hfs_lock: error %d trying to lock meta-data (flags = 0x%08X).\n", retval, ap->a_flags));
            };
            (void)lockmgr(&hp->h_lock, LK_RELEASE, (simple_lock_t) 0, ap->a_p);
        };
    };

Err_Exit:;
    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}

/*
 * Unlock an hfsnode.
#% unlock	vp	L U L
#
 vop_unlock {
     IN struct vnode *vp;
     IN int flags;
     IN struct proc *p;

     */
int
hfs_unlock(ap)
struct vop_unlock_args /* {
    struct vnode *a_vp;
    int a_flags;
    struct proc *a_p;
} */ *ap;
{
    struct hfsnode *hp = VTOH(ap->a_vp);
    struct vnode *vp = ap->a_vp;
    int		retval = E_NONE;

    DBG_FUNC_NAME("unlock");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(vp);DBG_VOP_CONT((" flags = 0x%08X.\n", ap->a_flags));
    DBG_VOP_LOCKS_INIT(0,vp, VOPDBG_LOCKED, VOPDBG_UNLOCKED, VOPDBG_LOCKED, VOPDBG_ZERO);

#if DIAGNOSTIC
    DBG_ASSERT(hp != (struct hfsnode *)NULL);
    if (ap->a_flags & LK_INTERLOCK) {
        DBG_ASSERT(*((int*)&vp->v_interlock) != 0);
    } else {
        DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
    };
    if (H_FORKTYPE(hp) != kDirCmplx && (vp->v_flag & VSYSTEM) == 0) {
        DBG_ASSERT(hp->h_meta != NULL);
    };
#endif	/* DIAGNOSTIC */

    DBG_ASSERT((ap->a_flags & (LK_EXCLUSIVE|LK_SHARED)) == 0);
    retval = lockmgr(&hp->h_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock, ap->a_p);
    if (retval != E_NONE) {
        DEBUG_BREAK_MSG(("hfs_unlock: error %d trying to unlock vnode (forktype = %d).\n", retval, H_FORKTYPE(hp)));
    };

    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);

    /* Release lock on the file meta lock */
    if ((retval == E_NONE) && (H_FORKTYPE(hp) != kDirCmplx) && ((vp->v_flag & VSYSTEM) == 0)) {
        retval = lockmgr(&hp->h_meta->h_fmetalock, (ap->a_flags & ~LK_INTERLOCK) | LK_RELEASE, (simple_lock_t) 0, ap->a_p);
        if (retval != E_NONE) {
            /* XXX PPD Wouldn't this be the perfect time to panic?  We're now half-unlocked! */
            DEBUG_BREAK_MSG(("hfs_unlock: error %d trying to unlock meta-data (forktype = %d).\n", retval, H_FORKTYPE(hp)));
        };
    };

    DBG_ASSERT(*((int*)&vp->v_interlock) == 0);
    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}


/*
 * Print out the contents of an hfsnode.
#% print	vp	= = =
#
 vop_print {
     IN struct vnode *vp;
     */
int
hfs_print(ap)
struct vop_print_args /* {
    struct vnode *a_vp;
} */ *ap;
{
    register struct vnode * vp = ap->a_vp;
    register struct hfsnode *hp = VTOH( vp);
    DBG_FUNC_NAME("print");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_POS);

    printf("tag VT_HFS, dirID %ld, on dev %d, %d", H_DIRID(hp),
           major(hp->h_dev), minor(hp->h_dev));
    /* lockmgr_printinfo(&hp->h_lock); */
    printf("\n");
    DBG_VOP_LOCKS_TEST(E_NONE);
    return (E_NONE);
}


/*
 * Check for a locked hfsnode.
#% islocked	vp	= = =
#
 vop_islocked {
     IN struct vnode *vp;

     */
int
hfs_islocked(ap)
struct vop_islocked_args /* {
    struct vnode *a_vp;
} */ *ap;
{
    int		lockStatus;
    //DBG_FUNC_NAME("islocked");
    //DBG_VOP_LOCKS_DECL(1);
    //DBG_VOP_PRINT_FUNCNAME();
    //DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);

    //DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_IGNORE, VOPDBG_ZERO);

    lockStatus = lockstatus(&VTOH( ap->a_vp)->h_lock);
    //DBG_VOP_LOCKS_TEST(E_NONE);
    return (lockStatus);
}

/*

#% pathconf	vp	L L L
#
 vop_pathconf {
     IN struct vnode *vp;
     IN int name;
     OUT register_t *retval;

     */
static int
hfs_pathconf(ap)
struct vop_pathconf_args /* {
    struct vnode *a_vp;
    int a_name;
    int *a_retval;
} */ *ap;
{
    int retval = E_NONE;
    DBG_FUNC_NAME("pathconf");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);

    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_POS);

    DBG_HFS_NODE_CHECK (ap->a_vp);

    switch (ap->a_name) {
        case _PC_LINK_MAX:
            *ap->a_retval = 1;
            break;
        case _PC_NAME_MAX:
            *ap->a_retval = NAME_MAX;
            break;
        case _PC_PATH_MAX:
            *ap->a_retval = PATH_MAX; /* 1024 */
            break;
        case _PC_CHOWN_RESTRICTED:
            *ap->a_retval = 1;
            break;
        case _PC_NO_TRUNC:
            *ap->a_retval = 0;
            break;
        default:
            retval = EINVAL;
    }

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}





/*
 * Advisory record locking support
#% advlock	vp	U U U
#
 vop_advlock {
     IN struct vnode *vp;
     IN caddr_t id;
     IN int op;
     IN struct flock *fl;
     IN int flags;

     */
int
hfs_advlock(ap)
struct vop_advlock_args /* {
    struct vnode *a_vp;
    caddr_t  a_id;
    int  a_op;
    struct flock *a_fl;
    int  a_flags;
} */ *ap;
{
    register struct hfsnode *hp = VTOH(ap->a_vp);
    register struct flock *fl = ap->a_fl;
    register struct hfslockf *lock;
    off_t start, end;
    int retval;
    DBG_FUNC_NAME("advlock");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_UNLOCKED, VOPDBG_POS);
    /*
     * Avoid the common case of unlocking when inode has no locks.
     */
    if (hp->h_lockf == (struct hfslockf *)0) {
        if (ap->a_op != F_SETLK) {
            fl->l_type = F_UNLCK;
            return (0);
        }
    }
    /*
     * Convert the flock structure into a start and end.
     */
    start = 0;
    switch (fl->l_whence) {
        case SEEK_SET:
        case SEEK_CUR:
            /*
             * Caller is responsible for adding any necessary offset
             * when SEEK_CUR is used.
             */
            start = fl->l_start;
            break;

        case SEEK_END:
            start = HTOFCB(hp)->fcbEOF + fl->l_start;
            break;

        default:
            return (EINVAL);
    }

    if (start < 0)
        return (EINVAL);
    if (fl->l_len == 0)
        end = -1;
    else
        end = start + fl->l_len - 1;

    /*
     * Create the hfslockf structure
     */
    MALLOC(lock, struct hfslockf *, sizeof *lock, M_LOCKF, M_WAITOK);
    lock->lf_start = start;
    lock->lf_end = end;
    lock->lf_id = ap->a_id;
    lock->lf_hfsnode = hp;
    lock->lf_type = fl->l_type;
    lock->lf_next = (struct hfslockf *)0;
    TAILQ_INIT(&lock->lf_blkhd);
    lock->lf_flags = ap->a_flags;
    /*
     * Do the requested operation.
     */
    switch(ap->a_op) {
        case F_SETLK:
            retval = hfs_setlock(lock);
            break;

        case F_UNLCK:
            retval = hfs_clearlock(lock);
            FREE(lock, M_LOCKF);
            break;

        case F_GETLK:
            retval = hfs_getlock(lock, fl);
            FREE(lock, M_LOCKF);
            break;

        default:
            retval = EINVAL;
            _FREE(lock, M_LOCKF);
            break;
    }

    DBG_VOP_LOCKS_TEST(retval);
    return (retval);
}



/*
 * Update the access, modified, and node change times as specified by the
 * IACCESS, IUPDATE, and ICHANGE flags respectively. The IMODIFIED flag is
 * used to specify that the node needs to be updated but that the times have
 * already been set. The access and modified times are taken from the second
 * and third parameters; the node change time is always taken from the current
 * time. If waitfor is set, then wait for the disk write of the node to
 * complete.
 */
/*
#% update	vp	L L L
	IN struct vnode *vp;
	IN struct timeval *access;
	IN struct timeval *modify;
	IN int waitfor;
*/

int
hfs_update(ap)
    struct vop_update_args /* {
        struct vnode *a_vp;
        struct timeval *a_access;
        struct timeval *a_modify;
        int a_waitfor;
    } */ *ap;
{
    struct hfsnode 	*hp;
    struct proc 	*p;
    CatalogNodeData nodeData;
    FSSpec 			nodeSpec;	/* 264 bytes */
    int 			retval;
    DBG_FUNC_NAME("update");
    DBG_VOP_LOCKS_DECL(1);
    DBG_VOP_PRINT_FUNCNAME();
    DBG_VOP_PRINT_VNODE_INFO(ap->a_vp);DBG_VOP_CONT(("\n"));
    DBG_VOP_LOCKS_INIT(0,ap->a_vp, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_LOCKED, VOPDBG_ZERO);

    hp = VTOH(ap->a_vp);

    DBG_ASSERT(*((int*)&ap->a_vp->v_interlock) == 0);
    DBG_ASSERT(ap->a_vp->v_type != VCPLX);
    if (ap->a_vp->v_type == VCPLX) {	//XXX SER Shoulf never be true, change to a panic before ship
        DBG_VOP_LOCKS_TEST(0);
		return (0);
    };

    if (H_FORKTYPE(hp) == kSysFile) {
        hp->h_meta->h_nodeflags &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
        DBG_VOP_LOCKS_TEST(0);
        return (0);
    }

    if (VTOVFS(ap->a_vp)->mnt_flag & MNT_RDONLY) {
        hp->h_meta->h_nodeflags &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
        DBG_VOP_LOCKS_TEST(0);
        DBG_VOP(("hfs_update: returning 0 (all flags were cleared because the volume is read-only.\n"));
        return (0);
    }
	
    /* Check to see if MacOS set the fcb to be dirty, if so, translate it to IN_MODIFIED */
    if (HTOFCB(hp)->fcbFlags &fcbModifiedMask)
        hp->h_meta->h_nodeflags |= IN_MODIFIED;

    if ((hp->h_meta->h_nodeflags & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0) {
        DBG_VOP_LOCKS_TEST(0);
        DBG_VOP(("hfs_update: returning 0 because the metadata is unchanged.\n"));
        return (0);
    };

    if (hp->h_meta->h_nodeflags & IN_ACCESS)
        hp->h_meta->h_atime = ap->a_access->tv_sec;
    if (hp->h_meta->h_nodeflags & IN_UPDATE)
        hp->h_meta->h_mtime = ap->a_modify->tv_sec;
    if (hp->h_meta->h_nodeflags & IN_CHANGE) {
        hp->h_meta->h_ctime = time.tv_sec;
		/*
		 * HFS dates that WE set must be adjusted for DST
		 */
		if ((HTOVCB(hp)->vcbSigWord == kHFSSigWord) && gTimeZone.tz_dsttime) {
			hp->h_meta->h_ctime += 3600;
			hp->h_meta->h_mtime = hp->h_meta->h_ctime;
		}
	}

	p = CURRENT_PROC;
	/*
	 * Since VOP_UPDATE can be called from withing another VOP (eg VOP_RENAME),
	 * the Catalog b-tree may aready be locked by the current thread. So we
	 * allow recursive locking of the Catalog from within VOP_UPDATE.
	 */

	/* Lock the Catalog b-tree file */
	retval = hfs_metafilelocking(HTOHFS(hp), kHFSCatalogFileID, LK_EXCLUSIVE | LK_CANRECURSE, p);
	if (retval) {
        DBG_VOP_LOCKS_TEST(retval);
        DBG_ASSERT(*((int*)&ap->a_vp->v_interlock) == 0);
        return (retval);
    };
    DBG_ASSERT(*((int*)&ap->a_vp->v_interlock) == 0);

    retval = GetCatalogNode(HTOVCB(hp), H_DIRID(hp), H_NAME(hp), H_HINT(hp), &nodeSpec, &nodeData, &(H_HINT(hp)));
    if (retval != noErr) {
        DBG_ERR(("hfs_update: error %d on GetCatalogNode...\n", retval));
        retval = MacToVFSError(retval);
        goto exit_relse;
    };

	/* If there is a sibling (resource fork), copy its contents first */
	/* This makes sure all changes in the FCB are reflected. i.e. LEOF, etc */
	if (hp->h_sibling)
    	CopyVNodeToCatalogNode (hp->h_sibling, &nodeData);

    CopyVNodeToCatalogNode (HTOV(hp), &nodeData);

    retval = UpdateCatalogNode(HTOVCB(hp), H_DIRID(hp), H_NAME(hp), H_HINT(hp), &nodeData);
    if (retval != noErr) {
 		DBG_ERR(("hfs_update: error %d on UpdateCatalogNode...\n", retval));
        retval = MacToVFSError(retval);
        goto exit_relse;
    };

	/* After the updates are finished, clear the flags */
    hp->h_meta->h_nodeflags &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
	HTOFCB(hp)->fcbFlags &= ~fcbModifiedMask;

	/* Update general data */
    if (ap->a_vp->v_type == VDIR) {
        hp->h_size = sizeof (hfsdirentry) * (nodeData.valence + 2);
	}
	else {
        hp->h_size = nodeData.rsrcPhysicalSize + nodeData.dataPhysicalSize;
	}


exit_relse:    
	 /* unlock the Catalog b-tree file */
	(void) hfs_metafilelocking(HTOHFS(hp), kHFSCatalogFileID, LK_RELEASE, p);

    DBG_VOP(("hfs_update: returning %d.\n", retval));
    DBG_VOP_LOCKS_TEST(retval);
    DBG_ASSERT(*((int*)&ap->a_vp->v_interlock) == 0);
	return (retval);
}




/*
 * Allocate a new node
 *
 * Assumes that the catalog b-tree is locked
 *
 * Upon leaving, namei buffer must be freed.
 *
 */
static int
hfs_makenode(mode, dvp, vpp, cnp)
    int mode;
    struct vnode *dvp;
    struct vnode **vpp;
    struct componentname *cnp;
{
    register struct hfsnode *hp, *parhp;
    struct timeval 			tv;
    struct vnode 			*tvp;
    struct hfsCatalogInfo 	catInfo;
    ExtendedVCB				*vcb;
    UInt8					forkType;
    int 					retval;
    DBG_FUNC_NAME("makenode");

    parhp	= VTOH(dvp);
    vcb		= HTOVCB(parhp);
    *vpp	= NULL;
	tvp 	= NULL;
    if ((mode & IFMT) == 0)
        mode |= IFREG;

#if DIAGNOSTIC
    if ((cnp->cn_flags & HASBUF) == 0)
        panic("hfs_makenode: no name");
#endif

    /* Create the Catalog B*-Tree entry */
    retval = hfsCreate(vcb, H_FILEID(parhp), cnp->cn_nameptr, mode);
    if (retval != E_NONE) {
        DBG_ERR(("%s: hfsCreate FAILED: %s, %s\n", funcname, cnp->cn_nameptr, H_NAME(parhp)));
        goto bad1;
    }

    /* Look up the catalog entry just created: */
    retval = hfsLookup(vcb, H_FILEID(parhp), cnp->cn_nameptr, cnp->cn_namelen, 0, &catInfo);
    if (retval != E_NONE) {
        DBG_ERR(("%s: hfsLookup FAILED: %s, %s\n", funcname, cnp->cn_nameptr, H_NAME(parhp)));
        goto bad1;
    }

    /* Create a vnode for the object just created: */
    forkType = (catInfo.nodeData.nodeType == kCatalogFolderNode) ? kDirCmplx : kDataFork;
    if ((retval = hfsGet(vcb, &catInfo, forkType, dvp, &tvp))) {
        goto bad1;
    }

    /* Set other vnode values not init'd in getnewvnode(). */
    tvp->v_type = IFTOVT(mode);

	/* reinit hp from the passed in mode, only if hfsplus */
    if (vcb->vcbSigWord == kHFSPlusSigWord) {
        hp = VTOH(tvp);
        hp->h_meta->h_gid = parhp->h_meta->h_gid;
        if ((mode & IFMT) == IFLNK)
            hp->h_meta->h_uid = parhp->h_meta->h_uid;
        else
            hp->h_meta->h_uid = cnp->cn_cred->cr_uid;

        hp->h_meta->h_nodeflags &= ~IN_UNSETACCESS;
        hp->h_meta->h_nodeflags |= IN_ACCESS | IN_CHANGE | IN_UPDATE;

        hp->h_meta->h_mode = mode;
        if ((hp->h_meta->h_mode & ISGID) && !groupmember(hp->h_meta->h_gid, cnp->cn_cred) &&
            suser(cnp->cn_cred, NULL))
            hp->h_meta->h_mode &= ~ISGID;

        if (cnp->cn_flags & ISWHITEOUT)
            hp->h_meta->h_pflags |= UF_OPAQUE;

        /* Write out the new values */
        tv = time;
        if ((retval = VOP_UPDATE(tvp, &tv, &tv, 1)))
            goto bad2;
    }

	VTOH(dvp)->h_meta->h_nodeflags |= IN_CHANGE | IN_UPDATE;
    tv = time;
    if ((retval = VOP_UPDATE(dvp, &tv, &tv, 1)))
        goto bad2;

    if ((cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF) {
        FREE_ZONE(cnp->cn_pnbuf, cnp->cn_pnlen, M_NAMEI);
    };
    VPUT(dvp);
#if MACH_NBC
    if ((tvp->v_type == VREG)  && !(tvp->v_vm_info)){
        vm_info_init(tvp);
    }
#endif /* MACH_NBC */
    *vpp = tvp;
    return (0);

bad2:
    /*
     * Write retval occurred trying to update the node
     * or the directory so must deallocate the node.
    */
    /* XXX SER In the future maybe set *vpp to 0xdeadbeef for testing */
    VPUT(tvp);

bad1:
    if ((cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF) {
        FREE_ZONE(cnp->cn_pnbuf, cnp->cn_pnlen, M_NAMEI);
    };
    VPUT(dvp);

    return (retval);

}


/************************************************************************/
/*	Entry for searchfs() 		  										*/
/************************************************************************/

#define	errSearchBufferFull	101		//	Internal search errors
/*
#
#% searchfs	vp	L L L
#
vop_searchfs {
    IN struct vnode *vp;
    IN off_t length;
    IN int flags;
    IN struct ucred *cred;
    IN struct proc *p;
};
*/

//XXX	Optimize this call for calls being made from VDI
int	hfs_search( ap )
struct vop_searchfs_args *ap; /*
 struct vnodeop_desc *a_desc;
 struct vnode *a_vp;
 void *a_searchparams1;
 void *a_searchparams2;
 struct attrlist *a_searchattrs;
 u_long a_maxmatches;
 struct timeval *a_timelimit;
 struct attrlist *a_returnattrs;
 u_long *a_nummatches;
 u_long a_scriptcode;
 u_long a_options;
 struct uio *a_uio;
 struct searchstate *a_searchstate;
*/
{
	CatalogRecord		catalogRecord;
	BTreeKey			*key;
	FSBufferDescriptor	btRecord;
	FCB*				catalogFCB;
	SearchState			*searchState;
	searchinfospec_t	searchInfo1;
	searchinfospec_t	searchInfo2;
	void				*attributesBuffer;
	void				*variableBuffer;
	short				recordSize;
	short				operation;
	u_long				fixedBlockSize;
	u_long				eachReturnBufferSize;
	struct proc			*p						= CURRENT_PROC;
	u_long				nodesToCheck			= 30;				//	After we search 30 nodes we must give up time
	u_long				lastNodeNum				= 0XFFFFFFFF;
	ExtendedVCB			*vcb					= VTOVCB(ap->a_vp);
	int					err						= E_NONE;

    DBG_FUNC_NAME("hfs_search");
	DBG_VOP_LOCKS_DECL(1);

    //XXX Parameter check a_searchattrs

	*(ap->a_nummatches)	= 0;

	if ( ap->a_options & ~SRCHFS_VALIDOPTIONSMASK )
		return( EINVAL );

	if (ap->a_uio->uio_resid <= 0)
		return (EINVAL);

	searchState	= (SearchState *)ap->a_searchstate;

	//	Check if this is the first time we are being called.
	//	If it is, allocate SearchState and we'll move it to the users space on exit
	if ( ap->a_options & SRCHFS_START )
	{
		bzero( (caddr_t)searchState, sizeof(SearchState) );
		searchState->isHFSPlus	= ( vcb->vcbSigWord == kHFSPlusSigWord );
		operation	= kBTreeFirstRecord;
		ap->a_options &= ~SRCHFS_START;		//	clear the bit
	}
	else
	{
		operation	= kBTreeCurrentRecord;
	}

	//	UnPack the search boundries, searchInfo1, searchInfo2
	err = UnpackSearchAttributeBlock( ap->a_vp, ap->a_searchattrs, &searchInfo1, ap->a_searchparams1 );
	if (err) return err;
	err = UnpackSearchAttributeBlock( ap->a_vp, ap->a_searchattrs, &searchInfo2, ap->a_searchparams2 );
	if (err) return err;

	btRecord.itemCount			= 1;
	btRecord.itemSize			= sizeof( catalogRecord );
	btRecord.bufferAddress		= &catalogRecord;
	catalogFCB					= VTOFCB( vcb->catalogRefNum );
	key							= (BTreeKey*) &(searchState->btreeIterator.key);
	fixedBlockSize 				= sizeof(u_long) + AttributeBlockSize( ap->a_returnattrs );	/* u_long for length longword */
	eachReturnBufferSize		= fixedBlockSize;
	
	if ( ap->a_returnattrs->commonattr & ATTR_CMN_NAME )				//XXX should be more robust
		eachReturnBufferSize += NAME_MAX + 1;

    MALLOC( attributesBuffer, void *, eachReturnBufferSize, M_TEMP, M_WAITOK );
	variableBuffer				= attributesBuffer + fixedBlockSize;

	//	Lock catalog b-tree
	err = hfs_metafilelocking( VTOHFS(ap->a_vp), kHFSCatalogFileID, LK_SHARED, p );
	if ( err != E_NONE )
	{
        DBG_VOP_LOCKS_TEST( err );
        goto ExitThisRoutine;
    };

    //
	//	Iterate over all the catalog btree records
	//
	
	err = BTIterateRecord( catalogFCB, operation, &(searchState->btreeIterator), &btRecord, &recordSize );

	while( err == E_NONE )
	{
		if ( CheckCriteria( vcb, searchState, ap->a_options, ap->a_searchattrs, &catalogRecord, (CatalogKey *)key, &searchInfo1, &searchInfo2 ) == true )
		{
			err = InsertMatch( ap->a_vp, ap->a_uio, &catalogRecord, (CatalogKey *)key, ap->a_returnattrs, attributesBuffer, variableBuffer, eachReturnBufferSize, ap->a_nummatches );
			if ( err != E_NONE )
				break;
		}
		
		err = BTIterateRecord( catalogFCB, kBTreeNextRecord, &(searchState->btreeIterator), &btRecord, &recordSize );
		
			if  ( *(ap->a_nummatches) >= ap->a_maxmatches )
				break;

  		if ( searchState->btreeIterator.hint.nodeNum != lastNodeNum )
		{
			lastNodeNum	= searchState->btreeIterator.hint.nodeNum;
			if ( --nodesToCheck == 0 )
				break;								//	We must leave the kernel to give up time
		}
	}

	//	Unlock catalog b-tree
	(void) hfs_metafilelocking( VTOHFS(ap->a_vp), kHFSCatalogFileID, LK_RELEASE, p );


    if ( err == E_NONE )
	{
		err = EAGAIN;								//	signal to the user to call searchfs again
	}
    else if ( err == errSearchBufferFull )
	{
		if ( *(ap->a_nummatches) > 0 )
			err = EAGAIN;
 		else
			err = ENOBUFS;
	}
	else if ( err == btNotFound )
	{
		err = E_NONE;								//	the entire disk has been searched
	}

ExitThisRoutine:
        FREE( attributesBuffer, M_TEMP );

	return( err );
}


static Boolean
ComparePartialUnicodeName ( register ConstUniCharArrayPtr str, register ItemCount s_len,
							register ConstUniCharArrayPtr find, register ItemCount f_len )
{
	if (f_len == 0 || s_len == 0)
		return FALSE;

	do {
		if (s_len-- < f_len)
			return FALSE;
	} while (FastUnicodeCompare(str++, f_len, find, f_len) != 0);

	return TRUE;
}


static Boolean
ComparePartialPascalName ( register ConstStr31Param str, register ConstStr31Param find )
{
	register u_char s_len = str[0];
	register u_char f_len = find[0];
	register u_char *tsp;
	Str31 tmpstr;

	if (f_len == 0 || s_len == 0)
		return FALSE;

	bcopy(str, tmpstr, s_len + 1);
	tsp = &tmpstr[0];

	while (s_len-- >= f_len) {
		*tsp = f_len;

		if (FastRelString(tsp++, find) == 0)
			return TRUE;
	}

	return FALSE;
}

//
//	CheckCriteria()
//
Boolean CheckCriteria( ExtendedVCB *vcb, const SearchState *searchState, u_long searchBits,
						struct attrlist *attrList, CatalogRecord *catalogRecord, CatalogKey *key,
						searchinfospec_t  *searchInfo1, searchinfospec_t *searchInfo2 )
{
	Boolean			matched;
	CatalogNodeData	catData;
	attrgroup_t		searchAttributes;
	
	switch ( catalogRecord->recordType )
	{
		case kHFSFolderRecord:
		case kHFSPlusFolderRecord:
			if ( (searchBits & SRCHFS_MATCHDIRS) == 0 )		//	If we are NOT searching folders
			{
				matched	= false;
				goto TestDone;
			}
			break;
			
		case kHFSFileRecord:
		case kHFSPlusFileRecord:
				if ( (searchBits & SRCHFS_MATCHFILES) == 0 )		//	If we are NOT searching files
			{
				matched	= false;
				goto TestDone;
			}
			break;

		default:						// Never match a thread record or any other type.
			return( false );				// Not a file or folder record, so can't search it
	}
	
	//	Change the catalog record data into a single common form
	matched				= true;							//	Assume we got a match
	catData.nodeType	= 0;							//	mark this record as not in use
	CopyCatalogNodeData( vcb, catalogRecord, &catData );

	/* First, attempt to match the name -- either partial or complete */
	if ( attrList->commonattr & ATTR_CMN_NAME ) {
		if ( searchState->isHFSPlus ) {
			/* Check for partial/full HFS Plus name match */

			if ( searchBits & SRCHFS_MATCHPARTIALNAMES ) {
				matched = ComparePartialUnicodeName( key->hfsPlus.nodeName.unicode,
													 key->hfsPlus.nodeName.length,
													 (UniChar*)searchInfo1->name,
													 searchInfo1->nameLength );
			} else /* full HFS Plus name match */ { 
				matched = (FastUnicodeCompare(	key->hfsPlus.nodeName.unicode,
												key->hfsPlus.nodeName.length,
												(UniChar*)searchInfo1->name,
												searchInfo1->nameLength ) == 0);
			}
		} else {
			/* Check for partial/full HFS name match */

			if ( searchBits & SRCHFS_MATCHPARTIALNAMES )
				matched = ComparePartialPascalName(key->hfs.nodeName, (u_char*)searchInfo1->name);
			else /* full HFS name match */
				matched = (FastRelString(key->hfs.nodeName, (u_char*)searchInfo1->name) == 0);
		}
		
		if ( matched == false || (searchBits & ~SRCHFS_MATCHPARTIALNAMES) == 0 )
			goto TestDone;	/* no match, or nothing more to compare */
	}
	
	
	//	Now that we have a record worth searching, see if it matches the search attributes
	if ( (catData.nodeType == kCatalogFileNode) && ((attrList->fileattr & ATTR_FILE_VALIDMASK) != 0) )
	{
		searchAttributes	= attrList->fileattr;
		//
		//	File logical length (data fork)
		//
		if ( searchAttributes & ATTR_FILE_DATALENGTH )
		{
			matched = CompareRange( catData.dataLogicalSize, searchInfo1->f.dataLogicalLength, searchInfo2->f.dataLogicalLength );
			if (matched == false) goto TestDone;
		}

		//
		//	File physical length (data fork)
		//
		if ( searchAttributes & ATTR_FILE_DATAALLOCSIZE )
		{
			matched = CompareRange( catData.dataPhysicalSize, searchInfo1->f.dataPhysicalLength, searchInfo2->f.dataPhysicalLength );
			if (matched == false) goto TestDone;
		}

		//
		//	File logical length (resource fork)
		//
		if ( searchAttributes & ATTR_FILE_RSRCLENGTH )
		{
			matched = CompareRange( catData.rsrcLogicalSize, searchInfo1->f.resourceLogicalLength, searchInfo2->f.resourceLogicalLength );
			if (matched == false) goto TestDone;
		}
		
		//
		//	File physical length (resource fork)
		//
		if ( searchAttributes & ATTR_FILE_RSRCALLOCSIZE )
		{
			matched = CompareRange( catData.rsrcPhysicalSize, searchInfo1->f.resourcePhysicalLength, searchInfo2->f.resourcePhysicalLength );
			if (matched == false) goto TestDone;
		}
		
		//
		//	File logical length (resource + data fork)
		//
		if ( searchAttributes & ATTR_FILE_TOTALSIZE )
		{
			matched = CompareRange( catData.rsrcLogicalSize + catData.dataLogicalSize, 
									searchInfo1->f.resourceLogicalLength + searchInfo1->f.dataLogicalLength, 
									searchInfo2->f.resourceLogicalLength + searchInfo2->f.dataLogicalLength );
			if (matched == false) goto TestDone;
		}
		
		//
		//	File physical length (resource + data fork)
		//
		if ( searchAttributes & ATTR_FILE_TOTALSIZE )
		{
			matched = CompareRange( catData.rsrcPhysicalSize + catData.dataPhysicalSize, 
									searchInfo1->f.resourcePhysicalLength + searchInfo1->f.dataPhysicalLength, 
									searchInfo2->f.resourcePhysicalLength + searchInfo2->f.dataPhysicalLength );
			if (matched == false) goto TestDone;
		}
	}
	//
	//	Check the directory attributes
	//
	else if ( (catData.nodeType == kCatalogFolderNode) && ((attrList->dirattr & ATTR_DIR_VALIDMASK) != 0) )
	{
		searchAttributes	= attrList->dirattr;
		
		//
		//	Directory valence
		//
		if ( searchAttributes & ATTR_DIR_ENTRYCOUNT )
		{
			matched = CompareRange( catData.valence, searchInfo1->d.numFiles, searchInfo2->d.numFiles );
			if (matched == false) goto TestDone;
		}
	}
	
	//
	//	Check the common attributes
	//
	searchAttributes	= attrList->commonattr;
	if ( (searchAttributes & ATTR_CMN_VALIDMASK) != 0 )
	{
		//
		//	Parent ID
		//
		if ( searchAttributes & ATTR_CMN_PAROBJID )
		{
			HFSCatalogNodeID	parentID;
			
			if (searchState->isHFSPlus)
				parentID = key->hfsPlus.parentID;
			else
				parentID = key->hfs.parentID;
				
			matched = CompareRange( parentID, searchInfo1->parentDirID, searchInfo2->parentDirID );
			if (matched == false) goto TestDone;
		}

		//
		//	Finder Info & Extended Finder Info where extFinderInfo is last 32 bytes
		//
		if ( searchAttributes & ATTR_CMN_FNDRINFO )
		{
			UInt32 *thisValue;
			thisValue = (UInt32 *) &catData.finderInfo[0];

			//	Note: ioFlFndrInfo and ioDrUsrWds have the same offset in search info, so
			//	no need to test the object type here.
			matched = CompareMasked( thisValue, (UInt32 *) &searchInfo1->finderInfo, (UInt32 *) &searchInfo2->finderInfo, 8 );	//	8 * UInt32
			if (matched == false) goto TestDone;
		}

		//
		//	Create date
		//
		if ( searchAttributes & ATTR_CMN_CRTIME )
		{
			matched = CompareRange( to_bsd_time(catData.createDate), searchInfo1->creationDate.tv_sec,  searchInfo2->creationDate.tv_sec );
			if (matched == false) goto TestDone;
		}
	
		//
		//	Mod date
		//
		if ( searchAttributes & ATTR_CMN_MODTIME )
		{
			matched = CompareRange( to_bsd_time(catData.contentModDate), searchInfo1->modificationDate.tv_sec, searchInfo2->modificationDate.tv_sec );
			if (matched == false) goto TestDone;
		}
	
		//
		//	Backup date
		//
		if ( searchAttributes & ATTR_CMN_BKUPTIME )
		{
			matched = CompareRange( to_bsd_time(catData.backupDate), searchInfo1->lastBackupDate.tv_sec, searchInfo2->lastBackupDate.tv_sec );
			if (matched == false) goto TestDone;
		}
	
	}

	
TestDone:
	//
	//	Finally, determine whether we need to negate the sense of the match (i.e. find
	//	all objects that DON'T match).
	//
	if ( searchBits & SRCHFS_NEGATEPARAMS )
		matched = !matched;
	
	return( matched );
}


//
//	Adds another record to the packed array for output
//
static int InsertMatch( struct vnode *root_vp, struct uio *a_uio, CatalogRecord *catalogRecord,
						CatalogKey *key, struct attrlist *returnAttrList, void *attributesBuffer,
						void *variableBuffer, u_long bufferSize, u_long * nummatches )
{
	int						err;
	void					*rovingAttributesBuffer;
	void					*rovingVariableBuffer;
	struct hfsCatalogInfo	catalogInfo;
	u_long					packedBufferSize;
	ExtendedVCB				*vcb			= VTOVCB(root_vp);
	Boolean					isHFSPlus		= vcb->vcbSigWord == kHFSPlusSigWord;

	rovingAttributesBuffer	= attributesBuffer + sizeof(u_long);		//	Reserve space for length field
	rovingVariableBuffer	= variableBuffer;

	CopyCatalogNodeData( vcb, catalogRecord, &catalogInfo.nodeData );

	catalogInfo.spec.parID = isHFSPlus ? key->hfsPlus.parentID : key->hfs.parentID;

	if ( returnAttrList->commonattr & ATTR_CMN_NAME )
	{
		u_long actualDstLen;

		/*	Return result in UTF-8 */
		if ( isHFSPlus ) {
			err = ConvertUnicodeToUTF8(	key->hfsPlus.nodeName.length * sizeof(UniChar),
										key->hfsPlus.nodeName.unicode,
										sizeof(catalogInfo.spec.name),
										&actualDstLen,
										catalogInfo.spec.name);
		 } else {
			err = ConvertMacRomanToUTF8( key->hfs.nodeName,
										 sizeof(catalogInfo.spec.name),
										 &actualDstLen,
										 catalogInfo.spec.name);
		}
	}

	PackCatalogInfoAttributeBlock( returnAttrList,root_vp,  &catalogInfo, &rovingAttributesBuffer, &rovingVariableBuffer );

	packedBufferSize = rovingVariableBuffer - attributesBuffer;

	if ( packedBufferSize > a_uio->uio_resid )
		return( errSearchBufferFull );

   	(* nummatches)++;
	
	*((u_long *)attributesBuffer) = packedBufferSize;	//	Store length of fixed + var block
	
	err = uiomove( (caddr_t)attributesBuffer, packedBufferSize, a_uio );		//	XXX should be packedBufferSize
	
	if (err != E_NONE)
		DBG_ERR(("InsertMatch: error %d on uiomove.\n", err));
	
	return( err );
}


static int
UnpackSearchAttributeBlock( struct vnode *vp, struct attrlist	*alist, searchinfospec_t *searchInfo, void *attributeBuffer )
{
	attrgroup_t		a;
	u_long			bufferSize;

	ASSERT(searchInfo != NULL);

	bufferSize =  *((u_long *)attributeBuffer);
	if (bufferSize == 0)
		return (EINVAL);	/* XXX -DJB is a buffer size of zero ever valid for searchfs? */

	++((u_long *)attributeBuffer);	//	advance past the size
	
	//
	//	UnPack common attributes
	//
	a	= alist->commonattr;
	if ( a != 0 )
	{
		if ( a & ATTR_CMN_NAME )
		{
			char *s	= attributeBuffer + ((attrreference_t *) attributeBuffer)->attr_dataoffset;
			size_t len = ((attrreference_t *) attributeBuffer)->attr_length;

			if (len > sizeof(searchInfo->name))
				return (EINVAL);

			if (VTOVCB(vp)->vcbSigWord == kHFSPlusSigWord) {
				ByteCount actualDstLen;
				/* Convert name to Unicode to match HFS Plus B-Tree names */

				if (len > 0) {
					if (ConvertUTF8ToUnicode(len-1, s, sizeof(searchInfo->name),
							&actualDstLen, (UniChar*)searchInfo->name) != 0)
						return (EINVAL);


					searchInfo->nameLength = actualDstLen / sizeof(UniChar);
				} else {
					searchInfo->nameLength = 0;
				}
				++((attrreference_t *)attributeBuffer);

			} else {
				/* Convert name to pascal string to match HFS B-Tree names */

				if (len > 0) {
					if (ConvertUTF8ToMacRoman(len-1, s, (u_char*)searchInfo->name) != 0)
						return (EINVAL);

					searchInfo->nameLength = searchInfo->name[0];
				} else {
					searchInfo->name[0] = searchInfo->nameLength = 0;
				}
				++((attrreference_t *)attributeBuffer);
			}
		}
		if ( a & ATTR_CMN_PAROBJID )
		{
			searchInfo->parentDirID	= ((fsobj_id_t *) attributeBuffer)->fid_objno;	/* ignore fid_generation */
			++((fsobj_id_t *)attributeBuffer);
		}
		if ( a & ATTR_CMN_CRTIME )
		{
			searchInfo->creationDate = *((struct timespec *)attributeBuffer);
			++((struct timespec *)attributeBuffer);
		}
		if ( a & ATTR_CMN_MODTIME )
		{
			searchInfo->modificationDate = *((struct timespec *)attributeBuffer);
			++((struct timespec *)attributeBuffer);
		}
		if ( a & ATTR_CMN_BKUPTIME )
		{
			searchInfo->lastBackupDate = *((struct timespec *)attributeBuffer);
			++((struct timespec *)attributeBuffer);
		}
		if ( a & ATTR_CMN_FNDRINFO )
		{
		   bcopy( attributeBuffer, searchInfo->finderInfo, sizeof(u_long) * 8 );
		   attributeBuffer += (sizeof(u_long) * 8 );
		}
	}

	a	= alist->dirattr;
	if ( a != 0 )
	{
		if ( a & ATTR_DIR_ENTRYCOUNT )
		{
			searchInfo->d.numFiles = *((u_long *)attributeBuffer);
			++((u_long *)attributeBuffer);
		}
	}

	a	= alist->fileattr;
	if ( a != 0 )
	{
		if ( a & ATTR_FILE_DATALENGTH )
		{
			searchInfo->f.dataLogicalLength	= *((off_t *)attributeBuffer);
			++((off_t *)attributeBuffer);
		}
		if ( a & ATTR_FILE_DATAALLOCSIZE )
		{
			searchInfo->f.dataPhysicalLength = *((off_t *)attributeBuffer);
			++((off_t *)attributeBuffer);
		}
		if ( a & ATTR_FILE_RSRCLENGTH )
		{
			searchInfo->f.resourceLogicalLength	= *((off_t *)attributeBuffer);
			++((off_t *)attributeBuffer);
		}
		if ( a & ATTR_FILE_RSRCALLOCSIZE )
		{
			searchInfo->f.resourcePhysicalLength = *((off_t *)attributeBuffer);
			++((off_t *)attributeBuffer);
		}
	}

	return (0);
}




#if DBG_VOP_TEST_LOCKS


void DbgVopTest( int maxSlots,
                 int retval,
                 VopDbgStoreRec *VopDbgStore,
                 char *funcname)
{
    int index;

    for (index = 0; index < maxSlots; index++)
      {
        if (VopDbgStore[index].id != index) {
            DEBUG_BREAK_MSG(("%s: DBG_VOP_LOCK: invalid id field (%d) in target entry (#%d).\n", funcname, VopDbgStore[index].id, index));
        };

        if ((VopDbgStore[index].vp != NULL) &&
            ((VopDbgStore[index].vp->v_data==NULL) || (VTOH(VopDbgStore[index].vp)->h_valid != HFS_VNODE_MAGIC)))
            continue;

        switch (VopDbgStore[index].inState)
          {
            case VOPDBG_IGNORE:
            case VOPDBG_SAME:
                /* Do Nothing !!! */
                break;
            case VOPDBG_LOCKED:
            case VOPDBG_UNLOCKED:
            case VOPDBG_LOCKNOTNIL:
              {
                  if (VopDbgStore[index].vp == NULL && (VopDbgStore[index].inState != VOPDBG_LOCKNOTNIL)) {
                      DBG_ERR (("%s: InState check: Null vnode ptr in entry #%d\n", funcname, index));
                  } else if (VopDbgStore[index].vp != NULL) {
                      switch (VopDbgStore[index].inState)
                        {
                          case VOPDBG_LOCKED:
                          case VOPDBG_LOCKNOTNIL:
                              if (VopDbgStore[index].inValue == 0)
                                {
                                  kprintf ("%s: Entry: not LOCKED:", funcname);
                                  DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                }
                              break;
                          case VOPDBG_UNLOCKED:
                              if (VopDbgStore[index].inValue != 0)
                                {
                                  kprintf ("%s: Entry: not UNLOCKED:", funcname);
                                  DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                }
                              break;
                        }
                  }
                  break;
              }
            default:
                DBG_ERR (("%s: DBG_VOP_LOCK on entry: bad lock test value: %d\n", funcname, VopDbgStore[index].errState));
          }


        if (retval != 0)
          {
            switch (VopDbgStore[index].errState)
              {
                case VOPDBG_IGNORE:
                    /* Do Nothing !!! */
                    break;
                case VOPDBG_LOCKED:
                case VOPDBG_UNLOCKED:
                case VOPDBG_SAME:
                  {
                      if (VopDbgStore[index].vp == NULL) {
                          DBG_ERR (("%s: ErrState check: Null vnode ptr in entry #%d\n", funcname, index));
                      } else {
                          VopDbgStore[index].outValue = lockstatus(&VTOH(VopDbgStore[index].vp)->h_lock);
                          switch (VopDbgStore[index].errState)
                            {
                              case VOPDBG_LOCKED:
                                  if (VopDbgStore[index].outValue == 0)
                                    {
                                      kprintf ("%s: Error: not LOCKED:", funcname);
                                      DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                   }
                                  break;
                              case VOPDBG_UNLOCKED:
                                  if (VopDbgStore[index].outValue != 0)
                                    {
                                      kprintf ("%s: Error: not UNLOCKED:", funcname);
                                      DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                    }
                                  break;
                              case VOPDBG_SAME:
                                  if (VopDbgStore[index].outValue != VopDbgStore[index].inValue)
                                      DBG_ERR (("%s: Error: In/Out locks are DIFFERENT: 0x%x, inis %d and out is %d\n", funcname, (u_int)VopDbgStore[index].vp, VopDbgStore[index].inValue, VopDbgStore[index].outValue));
                                  break;
                            }
                      }
                      break;
                  }
                case VOPDBG_LOCKNOTNIL:
                    if (VopDbgStore[index].vp != NULL) {
                        VopDbgStore[index].outValue = lockstatus(&VTOH(VopDbgStore[index].vp)->h_lock);
                        if (VopDbgStore[index].outValue == 0)
                            DBG_ERR (("%s: Error: Not LOCKED: 0x%x\n", funcname, (u_int)VopDbgStore[index].vp));
                    }
                    break;
                default:
                    DBG_ERR (("%s: Error: bad lock test value: %d\n", funcname, VopDbgStore[index].errState));
              }
          }
        else
          {
            switch (VopDbgStore[index].outState)
              {
                case VOPDBG_IGNORE:
                    /* Do Nothing !!! */
                    break;
                case VOPDBG_LOCKED:
                case VOPDBG_UNLOCKED:
                case VOPDBG_SAME:
                    if (VopDbgStore[index].vp == NULL) {
                        DBG_ERR (("%s: OutState: Null vnode ptr in entry #%d\n", funcname, index));
                    };
                    if (VopDbgStore[index].vp != NULL)
                      {
                        VopDbgStore[index].outValue = lockstatus(&VTOH(VopDbgStore[index].vp)->h_lock);
                        switch (VopDbgStore[index].outState)
                          {
                            case VOPDBG_LOCKED:
                                if (VopDbgStore[index].outValue == 0)
                                  {
                                    kprintf ("%s: Out: not LOCKED:", funcname);
                                    DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                 }
                                break;
                            case VOPDBG_UNLOCKED:
                                if (VopDbgStore[index].outValue != 0)
                                  {
                                    kprintf ("%s: Out: not UNLOCKED:", funcname);
                                    DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                                  }
                                break;
                            case VOPDBG_SAME:
                                if (VopDbgStore[index].outValue != VopDbgStore[index].inValue)
                                    DBG_ERR (("%s: Out: In/Out locks are DIFFERENT: 0x%x, in is %d and out is %d\n", funcname, (u_int)VopDbgStore[index].vp, VopDbgStore[index].inValue, VopDbgStore[index].outValue));
                                break;
                          }
                      }
                    break;
                case VOPDBG_LOCKNOTNIL:
                    if (VopDbgStore[index].vp != NULL) {
                        if (&VTOH(VopDbgStore[index].vp)->h_lock == NULL) {
                            DBG_ERR (("%s: DBG_VOP_LOCK on out: Null lock on vnode 0x%x\n", funcname, (u_int)VopDbgStore[index].vp));
                        }
                        else {
                            VopDbgStore[index].outValue = lockstatus(&VTOH(VopDbgStore[index].vp)->h_lock);
                            if (VopDbgStore[index].outValue == 0)
                              {
                                kprintf ("%s: DBG_VOP_LOCK on out: Should be LOCKED:", funcname);
                                DBG_VOP_PRINT_VNODE_INFO(VopDbgStore[index].vp); kprintf ("\n");
                              }
                        }
                    }
                    break;
                default:
                    DBG_ERR (("%s: DBG_VOP_LOCK on out: bad lock test value: %d\n", funcname, VopDbgStore[index].outState));
              }
          }

        VopDbgStore[index].id = -1;		/* Invalidate the entry to allow panic-free re-use */
      }	
}

#endif /* DBG_VOP_TEST_LOCKS */

/*****************************************************************************
*
*	VOP Table
*
*****************************************************************************/


struct vnodeopv_entry_desc hfs_vnodeop_entries[] = {
    { &vop_default_desc, vn_default_error },
    { &vop_lookup_desc, hfs_lookup },			/* lookup */
    { &vop_create_desc, hfs_create },			/* create */
    { &vop_mknod_desc, err_mknod },				/* mknod (NOT SUPPORTED) */
    { &vop_open_desc, hfs_open },				/* open */
    { &vop_close_desc, hfs_close },				/* close */
    { &vop_access_desc, hfs_access },			/* access */
    { &vop_getattr_desc, hfs_getattr },			/* getattr */
    { &vop_setattr_desc, hfs_setattr },			/* setattr */
    { &vop_read_desc, hfs_read },				/* read */
    { &vop_write_desc, hfs_write },				/* write */
    { &vop_ioctl_desc, hfs_ioctl },				/* ioctl */
    { &vop_select_desc, hfs_select },			/* select */
    { &vop_exchange_desc, hfs_exchange },		/* exchange */
    { &vop_mmap_desc, hfs_mmap },				/* mmap */
    { &vop_fsync_desc, hfs_fsync },				/* fsync */
    { &vop_seek_desc, hfs_seek },				/* seek */
    { &vop_remove_desc, hfs_remove },			/* remove */
    { &vop_link_desc, err_link },				/* link (NOT SUPPORTED) */
    { &vop_rename_desc, hfs_rename },			/* rename */
    { &vop_mkdir_desc, hfs_mkdir },				/* mkdir */
    { &vop_rmdir_desc, hfs_rmdir },				/* rmdir */
    { &vop_mkcomplex_desc,hfs_mkcomplex },		/* mkcomplex */
    { &vop_getattrlist_desc,hfs_getattrlist },  /* getattrlist */
    { &vop_setattrlist_desc,hfs_setattrlist },  /* setattrlist */
    { &vop_symlink_desc, hfs_symlink },			/* symlink */
    { &vop_readdir_desc, hfs_readdir },			/* readdir */
    { &vop_readdirattr_desc, hfs_readdirattr },	/* readdirattr */
    { &vop_readlink_desc, hfs_readlink },		/* readlink */
    { &vop_abortop_desc, hfs_abortop },			/* abortop */
    { &vop_inactive_desc, hfs_inactive },		/* inactive */
    { &vop_reclaim_desc, hfs_reclaim },			/* reclaim */
    { &vop_lock_desc, hfs_lock },				/* lock */
    { &vop_unlock_desc, hfs_unlock },			/* unlock */
    { &vop_bmap_desc, hfs_bmap },				/* bmap */
    { &vop_strategy_desc, hfs_strategy },		/* strategy */
    { &vop_print_desc, hfs_print },				/* print */
    { &vop_islocked_desc, hfs_islocked },		/* islocked */
    { &vop_pathconf_desc, hfs_pathconf },		/* pathconf */
    { &vop_advlock_desc, hfs_advlock },			/* advlock */
    { &vop_reallocblks_desc, hfs_reallocblks },	/* reallocblks */
    { &vop_truncate_desc, hfs_truncate },		/* truncate */
    { &vop_allocate_desc, hfs_allocate },		/* allocate */
    { &vop_update_desc, hfs_update },			/* update */
	{ &vop_searchfs_desc,hfs_search },			/* search fs */
    { &vop_bwrite_desc, vn_bwrite },			/* bwrite */
    { &vop_pagein_desc, hfs_pagein },           /* pagein */
    { &vop_pageout_desc, hfs_pageout },         /* pageout */
    { NULL, NULL }
};

struct vnodeopv_desc hfs_vnodeop_opv_desc =
{ &hfs_vnodeop_p, hfs_vnodeop_entries };
