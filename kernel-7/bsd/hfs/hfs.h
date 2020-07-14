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

/*	@(#)hfs.h		3.0
*
*	(c) 1997-1999 Apple Computer, Inc.  All Rights Reserved
*	(c) 1990, 1992 NeXT Computer, Inc.  All Rights Reserved
*
*	hfs.h -- constants, structures, function declarations. etc.
*			for Macintosh file system vfs.
*
*	HISTORY
* 13-Jan-1999	Don Brady	Add ATTR_CMN_SCRIPT to HFS_ATTR_CMN_LOOKUPMASK (radar #2296613).
* 20-Nov-1998	Don Brady	Remove UFSToHFSStr and HFSToUFSStr prototypes (obsolete).
*							Move filename entry from FCB to hfsfilemeta, hfsdirentry
*							names are now 255 byte long.
* 10-Nov-1998	Pat Dirks	Added MAXLOGBLOCKSIZE and MAXLOGBLOCKSIZEBLOCKS and RELEASE_BUFFER flag.
*                           Added hfsLogicalBlockTableEntry and h_logicalblocktable field in struct hfsnode.
*  4-Sep-1998	Pat Dirks	Added hfs_log_block_size to hfsmount struct [again] and BestBlockSizeFit routine.
* 31-aug-1998	Don Brady	Add UL to MAC_GMT_FACTOR constant.
* 04-jun-1998	Don Brady	Add hfsMoveRename prototype to replace hfsMove and hfsRename.
*							Add VRELE and VPUT macros to catch bad ref counts.
* 28-May-1998	Pat Dirks	Move internal 'struct searchinfo' def'n here from attr.h
* 03-may-1998	Brent Knight	Add gTimeZone.
* 23-apr-1998	Don Brady	Add File type and creator for symbolic links.
* 22-apr-1998	Don Brady	Removed kMetaFile.
* 21-apr-1998	Don Brady	Add to_bsd_time and to_hfs_time prototypes.
* 20-apr-1998	Don Brady	Remove course-grained hfs metadata locking.
* 15-apr-1998	Don Brady	Add hasOverflowExtents and hfs_metafilelocking prototypes. Add kSysFile constant.
* 14-apr-1998	Deric Horn	Added searchinfo_t, definition of search criteria used by searchfs.
*  9-apr-1998	Don Brady	Added hfs_flushMDB and hfs_flushvolumeheader prototypes.
*  8-apr-1998	Don Brady	Add MAKE_VREFNUM macro.
* 26-mar-1998	Don Brady	Removed CloseBTreeFile and OpenBtreeFile prototypes.
*
* 12-nov-1997 scott
* 		Added changes for HFSPlus
*/

#ifndef __HFS__
#define __HFS__

#include <sys/types.h>
#include <sys/ucred.h> 			/* mount.h needs this */
#include <sys/mount.h>
#include <sys/malloc.h>
#include <mach/machine/vm_types.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/attr.h>
#include <sys/syslimits.h>
#include <mach/vm_param.h>		/* For PAGE_SIZE */
#include <sys/dir.h>			/* For MAXNAMLEN */

#include <libkern/libkern.h>
#include <kern/queue.h>
#include <kern/kalloc.h>
#include <mach/machine/simple_lock.h>
#include <mach/mach_types.h>

#ifndef __FILEMGRINTERNAL__
#include "hfscommon/headers/FileMgrInternal.h"
#endif

#ifndef __BTREESINTERNAL__
#include "hfscommon/headers/BTreesInternal.h"
#endif

#if KERNEL_PRIVATE
#include <kern/thread.h>
#define GET_CURRENT_THREAD current_thread()
#else
extern thread_t current_thread_EXTERNAL();
#define GET_CURRENT_THREAD current_thread_EXTERNAL();
#endif

#if KERNEL_PRIVATE
#define CURRENT_PROC current_proc()
#else
extern struct proc * current_proc_EXTERNAL();
#define CURRENT_PROC current_proc_EXTERNAL()
#endif

struct uio;				// This is more effective than #include <sys/uio.h> in case KERNEL is undefined...
struct hfslockf;		// For advisory locking

/*
 *	Just reported via MIG interface.
 */
#define	VERSION_STRING	"hfs-1 (1-nov-97)"

/*
 *	fork strings.
 */
#define	kDataForkNameStr	"data"
#define	kRsrcForkNameStr	"rsrc"

/*
 *	Set to force READ_ONLY.
 */
#define	FORCE_READONLY	0

enum { kMDBSize = 512 };				/* Size of I/O transfer to read entire MDB */

enum { kMasterDirectoryBlock = 2 };			/* MDB offset on disk in 512-byte blocks */
enum { kMDBOffset = kMasterDirectoryBlock * 512 };	/* MDB offset on disk in bytes */

enum {
    kUnknownID = 0,
    kRootParID = 1,
    kRootDirID = 2
};

enum {
    kDirCmplx = 1,
    kDataFork = 2,
    kRsrcFork = 3,
	kSysFile = 4
};


/*
 *	File type and creator for symbolic links
 */
enum {
	kSymLinkFileType	= 0x736C6E6B,	/* 'slnk' */
	kSymLinkCreator		= 0x72686170	/* 'rhap' */
};


#define MAXLOGBLOCKSIZE PAGE_SIZE
#define MAXLOGBLOCKSIZEBLOCKS (MAXLOGBLOCKSIZE/512)
/* NOTE: Special support will be needed for LOGBLOCKMAPENTRIES > kHFSPlusExtentDensity (=8) */
#define LOGBLOCKMAPENTRIES 8

#define BUFFERPTRLISTSIZE 25

extern Ptr gBufferAddress[BUFFERPTRLISTSIZE];
extern struct buf *gBufferHeaderPtr[BUFFERPTRLISTSIZE];
extern int gBufferListIndex;
extern  simple_lock_data_t gBufferPtrListLock;

extern struct timezone gTimeZone;

/* Flag values for bexpand: */
#define RELEASE_BUFFER 0x00000001


/*
 * NOTE: The code relies on being able to cast an ExtendedVCB* to a vfsVCB* in order
 *	 to gain access to the mount point pointer from a pointer
 *	 to an ExtendedVCB.  DO NOT INSERT OTHER FIELDS BEFORE THE vcb FIELD!!
 *
 * vcbFlags, vcbLsMod, vcbFilCnt, vcbDirCnt, vcbNxtCNID, etc
 * are locked by the hfs_lock simple lock.
 */
typedef struct vfsVCB {
    ExtendedVCB			vcb_vcb;
    struct hfsmount		*vcb_hfsmp;				/* Pointer to hfsmount structure */
} vfsVCB_t;

typedef struct hfsmount {
    u_char				hfs_mountpoint[MAXPATHLEN+1];
    u_long				hfs_mount_flags;
    Boolean				hfs_fs_clean;			/* Whether contents have been flushed in clean state */
    Boolean				hfs_fs_ronly;			/* Whether this was mounted as read-initially  */

    /* Physical Description */
    u_long				hfs_phys_block_count;	/* Num of PHYSICAL blocks of volume */
    u_long				hfs_phys_block_size;	/* Always a multiple of 512 */

    /* Access to VFS and devices */
    struct mount 		*hfs_mp;				/* filesystem vfs structure */
    struct vnode 		*hfs_devvp;				/* block device mounted vnode */
    dev_t				hfs_raw_dev;			/* device mounted */
    struct hfsnode		*hfs_rootDirectory;
    unsigned long		hfs_log_block_size;		/* Size of buffer cache buffer for I/O */
	
	/* Default values for HFS standard and non-init access */
    uid_t 				hfs_uid;				/* uid to set as owner of the files */
    gid_t 				hfs_gid;				/* gid to set as owner of the files */
    mode_t 				hfs_dir_mask;			/* mask to and with directory protection bits */
    mode_t 				hfs_file_mask;			/* mask to and with file protection bits */
	u_long				hfs_encoding;			/* Defualt encoding for non hfs+ volumes */	

   /* HFS Specific */
    struct vfsVCB		hfs_vcb;
} hfsmount_t;


/*****************************************************************************
*
*	hfsnode structure
*
*
*	m_closeable is an indication of whether the file can be closed at
*	any time.  This flag is implemented to help indicate whether
*	its OK to close a file which is being deleted or renamed onto
*	and also as a consistency check because we don't actually close the
*	file in hfs_close, but rather wait for hfs_inactive.  This flag
*	is set TRUE when the vnode is created, and only set FALSE by
*	hfs_open.
*
*	m_valid is used to indicate when a vnode no longer references a
*	file etc.  It is set to 0 when an active vnode is removed
*	or a file is renamed onto an active vnode.
*
*	To uniquely identify an entity for the HFS file system, we need
*	the volume descriptor, the parent directory id, and the name.
*
*****************************************************************************/

#define MAXHFSVNODELEN		31
typedef u_char FileNameStr[MAXHFSVNODELEN+1];

/*
 * NOTE: The code relies on being able to cast an FCB* to a vfsFCB* in order
 *	 to gain access to the extended FCB and the vnode pointer from a pointer
 *	 to an HFS FCB.  DO NOT INSERT OTHER FIELDS BEFORE THE fcb FIELD!!
 */
typedef struct vfsFCB {
    FCB					fcb_fcb;				/* File Control Block */
    ExtendedFCB			fcb_extFCB;
    struct vnode		*fcb_vp;				/* pointer to start of container vnode */
} vfsFCB;


typedef struct hfsfilemeta {
    struct lock__bsd__	h_fmetalock;	/* file meta lock. */
    struct vnode		*h_fork;			/* vnode for first fork. */
    u_int16_t			h_usecount;			/* Amount of users accessing this record XXX SER Should be deprecated*/
    u_int32_t			h_logBlockSize;		/* Size of a block for reading/writing */
    u_int16_t			h_physBlkPerLogBlk;	/* Amount of phys sectory per logical block */
    u_int16_t			h_mode;				/* IFMT, permissions; see below. */
    u_int32_t			h_pflags;			/* Permission flags (NODUMP, IMMUTABLE, APPEND etc.) */
    u_int32_t			h_uid;				/* File owner. */
    u_int32_t			h_gid;				/* File group. */
    dev_t				h_rdev;				/* Special device info for this node */
    u_int32_t			h_nodeflags;		/* flags, see below */
    u_int32_t			h_crtime;			/* UNIX-format creation date in secs. */
    u_int32_t			h_atime;			/* UNIX-format access date in secs. */
    u_int32_t			h_mtime;			/* UNIX-format mod date in seconds */
    u_int32_t			h_ctime;			/* UNIX-format status change date */
    u_int32_t			h_butime;			/* UNIX-format last backup date in secs. */
    u_long	 			h_hint;				/* Catalog hint for use on Close */
    u_long	 			h_dirID;			/* Parent Directory ID */
	u_short				h_namelen;			/* Length of name string */
    char *				h_namePtr;			/* Points the name of the file */
    FileNameStr 		h_fileName;			/* CName of file */
} hfsfilemeta;


struct hfsLogicalBlockTableEntry {
    u_long		logicalBlockCount;
    off_t		extentLength;
};


typedef struct hfsnode {
    LIST_ENTRY(hfsnode)	h_hash;				/* links on valid and free lists */
    struct lock__bsd__	h_lock;				/* node lock. */
    struct hfslockf 	*h_lockf;			/* Head of byte-level lock list. */
    struct vnode		*h_vp;				/* vnode associated with this inode. */
    struct vnode		*h_devvp;			/* vnode for block I/O. */
    struct vnode		*h_relative;		/* vnode for complex parent, or to default fork */
    struct vnode		*h_sibling;			/* vnode for sibling, if is a fork. */
    dev_t				h_dev;				/* Device associated with the inode. */
    u_long				h_nodeID;			/* specific id of this node */
    UInt8				h_type;				/* Type of info: dir, cmplx, data, rsrc */
	off_t				h_size;				/* Total physical size of object */
    struct hfsLogicalBlockTableEntry h_logicalblocktable[LOGBLOCKMAPENTRIES];
    off_t				h_optimizedblocksizelimit;	/* End of range covered by h_logicalblocktable */
    daddr_t				h_uniformblocksizestart;	/* First LBN in fixed-size log. block range */
    int32_t				h_lastfsync;		/* Last time that this was fsynced (used for meta data) */
	vfsFCB				*h_xfcb;			/* Ptr to fcb data, if this is a fork */
    hfsfilemeta			*h_meta;			/* Ptr to complex (or file meta) data */
    u_int32_t			h_valid;			/* is the vnode reference valid */
} hfsnode_t;
typedef struct hfsnode *hfsnodeptr;


/*
 *	Macros for quick access to fields buried in the fcb inside an hfs node:
 */
#define H_FILEID(HP) 	((HP)->h_nodeID)
#define H_FORKTYPE(HP)	((HP)->h_type)
#define H_DIRID(HP)		((HP)->h_meta->h_dirID)
#define H_NAME(HP)		((HP)->h_meta->h_namePtr)
#define H_HINT(HP)		((HP)->h_meta->h_hint)

/* These flags are kept in flags. */
#define	IN_ACCESS	0x0001		/* Access time update request. */
#define	IN_CHANGE	0x0002		/* Change time update request. */
#define	IN_UPDATE	0x0004		/* Modification time update request. */
#define	IN_MODIFIED	0x0008		/* Node has been modified. */
#define	IN_RENAME	0x0010		/* Node is being renamed. */
#define	IN_SHLOCK	0x0020		/* File has shared lock. */
#define	IN_EXLOCK	0x0040		/* File has exclusive lock. */
#define	IN_UNSETACCESS	0x0200		/* File has unset access. */
#define	IN_LONGNAME	0x0400		/* File has unset access. */

/* File permissions.stored in mode */
#define	IEXEC		0000100		/* Executable. */
#define	IWRITE		0000200		/* Writeable. */
#define	IREAD		0000400		/* Readable. */
#define	ISVTX		0001000		/* Sticky bit. */
#define	ISGID		0002000		/* Set-gid. */
#define	ISUID		0004000		/* Set-uid. */

/* File types. */
#define	IFMT		0170000		/* Mask of file type. */
#define	IFIFO		0010000		/* Named pipe (fifo). */
#define	IFCHR		0020000		/* Character device. */
#define	IFDIR		0040000		/* Directory file. */
#define	IFBLK		0060000		/* Block device. */
#define	IFREG		0100000		/* Regular file. */
#define	IFLNK		0120000		/* Symbolic link. */
#define	IFSOCK		0140000		/* UNIX domain socket. */
#define	IFWHT		0160000		/* Whiteout. */

/* Value to make sure vnode is real and defined */
#define	HFS_VNODE_MAGIC	0x4846532b	/* 'HFS+' */


/*
 *	Write check macro
 */
#define	WRITE_CK(VNODE, FUNC_NAME)	{				\
    if ((VNODE)->v_mount->mnt_flag & MNT_RDONLY) {			\
        DBG_ERR(("%s: ATTEMPT TO WRITE A READONLY VOLUME\n", 	\
                 FUNC_NAME));	\
                     return(EROFS);							\
    }									\
}


/*
 *	hfsmount locking and unlocking.
 *
 *	mvl_lock_flags
 */
#define MVL_LOCKED    0x00000001	/* debug only */

#if	DIAGNOSTIC
#define MVL_LOCK(mvip)    {				\
    (simple_lock(&(mvip)->mvl_lock));			\
        (mvip)->mvl_flags |= MVL_LOCKED;			\
}

#define MVL_UNLOCK(mvip)    {				\
    if(((mvip)->mvl_flags & MVL_LOCKED) == 0) {		\
        panic("MVL_UNLOCK - hfsnode not locked");	\
    }							\
    (simple_unlock(&(mvip)->mvl_lock));			\
        (mvip)->mvl_flags &= ~MVL_LOCKED;			\
}
#else	/* DIAGNOSTIC */
#define MVL_LOCK(mvip)		(simple_lock(&(mvip)->mvl_lock))
#define MVL_UNLOCK(mvip)	(simple_unlock(&(mvip)->mvl_lock))
#endif	/* DIAGNOSTIC */


/* structure to hold a directory entry, patterned after struct dirent */
typedef struct hfsdirentry {
    u_int32_t	fileno;			/* unique file number */
    u_int16_t	reclen;			/* length of this structure */
    u_int8_t	type;			/* dirent file type */
    u_int8_t	namelen;		/* len of filename */
    char		name[NAME_MAX+1];		/* filename */
} hfsdirentry;

/* structure to hold a catalog record information */
/* Of everything you wanted to know about a catalog entry, file and directory */
typedef struct hfsCatalogInfo {
    CatalogNodeData 	nodeData;
	FSSpec 				spec;					/* filename */
    UInt32				hint;
} hfsCatalogInfo;

//	structure definition of the searchfs system trap for the search criterea.
struct directoryInfoSpec
{
	u_long				numFiles;
};

struct fileInfoSpec
{
	off_t				dataLogicalLength;
	off_t				dataPhysicalLength;
	off_t				resourceLogicalLength;
	off_t				resourcePhysicalLength;
};

struct searchinfospec
{
	u_char				name[512];
	u_long				nameLength;
	char				attributes;		// see IM:Files 2-100
	u_long				parentDirID;
	struct timespec		creationDate;		
	struct timespec		modificationDate;		
	struct timespec		lastBackupDate;	
	u_long				finderInfo[8];
    struct fileInfoSpec f;
	struct directoryInfoSpec d;
};
typedef struct searchinfospec searchinfospec_t;

#define	HFSTIMES(hp, t1, t2) {						\
    if ((hp)->h_meta->h_nodeflags & (IN_ACCESS | IN_CHANGE | IN_UPDATE)) {	\
        (hp)->h_meta->h_nodeflags |= IN_MODIFIED;				\
        if ((hp)->h_meta->h_nodeflags & IN_ACCESS) {			\
            (hp)->h_meta->h_atime = (t1)->tv_sec;			\
        };											\
        if ((hp)->h_meta->h_nodeflags & IN_UPDATE) {			\
            (hp)->h_meta->h_mtime = (t2)->tv_sec;			\
        }											\
        if ((hp)->h_meta->h_nodeflags & IN_CHANGE) {			\
            (hp)->h_meta->h_ctime = time.tv_sec;			\
        };											\
        (hp)->h_meta->h_nodeflags &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);	\
    }								\
}

/*
 * Various ways to acquire a VNode pointer:
 */
#define HTOV(HP) ((HP)->h_vp)
#define FCBTOV(FCB) (((vfsFCB *)(FCB))->fcb_vp)

/*
 * Various ways to acquire an HFS Node pointer:
 */
#define VTOH(VP) ((struct hfsnode *)((VP)->v_data))
#define FCBTOH(FCB) ((struct hfsnode *)(((vfsFCB *)(FCB))->fcb_vp->v_data))

/*
 * Various ways to acquire an FCB pointer:
 */
#define VTOFCB(VP) (&(((struct hfsnode *)((VP)->v_data))->h_xfcb->fcb_fcb))
#define HTOFCB(HP) (&(HP)->h_xfcb->fcb_fcb)

/*
 * Various ways to acquire a VFS mount point pointer:
 */
#define VTOVFS(VP) ((VP)->v_mount)
#define	HTOVFS(HP) ((HP)->h_vp->v_mount)
#define FCBTOVFS(FCB) (((vfsFCB *)(FCB))->fcb_vp->v_mount)
#define HFSTOVFS(HFSMP) ((HFSMP)->hfs_mp)
#define VCBTOVFS(VCB) (((struct vfsVCB *)(VCB))->vcb_hfsmp->hfs_mp)

/*
 * Various ways to acquire an HFS mount point pointer:
 */
#define VTOHFS(VP) ((struct hfsmount *)((VP)->v_mount->mnt_data))
#define	HTOHFS(HP) ((struct hfsmount *)(HP)->h_vp->v_mount->mnt_data)
#define FCBTOHFS(FCB) ((struct hfsmount *)(((vfsFCB *)(FCB))->fcb_vp->v_mount->mnt_data))
#define	VFSTOHFS(MP) ((struct hfsmount *)(MP)->mnt_data)	
#define VCBTOHFS(VCB) (((struct vfsVCB *)(VCB))->vcb_hfsmp)

/*
 * Various ways to acquire a VCB pointer:
 */
#define VTOVCB(VP) (&(((struct hfsmount *)((VP)->v_mount->mnt_data))->hfs_vcb.vcb_vcb))
#define HTOVCB(HP) (&(((struct hfsmount *)((HP)->h_vp->v_mount->mnt_data))->hfs_vcb.vcb_vcb))
#define FCBTOVCB(FCB) (&(((struct hfsmount *)(((vfsFCB *)(FCB))->fcb_vp->v_mount->mnt_data))->hfs_vcb.vcb_vcb))
#define VFSTOVCB(MP) (&(((struct hfsmount *)(MP)->mnt_data)->hfs_vcb.vcb_vcb))
#define HFSTOVCB(HFSMP) (&(HFSMP)->hfs_vcb.vcb_vcb)

/*
 * Lock for HFS file metadata
 */
#define HFSFILEMETA_LOCK_EXCLUSIVE(HP, P) \
	lockmgr(&(HP)->h_meta->h_fmetalock, LK_EXCLUSIVE, (simple_lock_t) 0, P)

#define HFSFILEMETA_LOCK_SHARED(HP, P) \
	lockmgr(&(HP)->h_meta->h_fmetalock, LK_SHARED, (simple_lock_t) 0, P)

#define HFSFILEMETA_UNLOCK(HP, P) \
	lockmgr(&(HP)->h_meta->h_fmetalock, LK_RELEASE, (simple_lock_t) 0, P)

#define E_NONE	0
#define kHFSBlockSize 512
#define kHFSBlockShift 9	/* 2^9 = 512 */

#define IOBLKNOFORBLK(STARTINGBLOCK, BLOCKSIZEINBYTES) ((daddr_t)((STARTINGBLOCK) / ((BLOCKSIZEINBYTES) >> 9)))
#define IOBLKCNTFORBLK(STARTINGBLOCK, BYTESTOTRANSFER, BLOCKSIZEINBYTES) \
    ((int)(IOBLKNOFORBYTE(((STARTINGBLOCK) * 512) + (BYTESTOTRANSFER) - 1, (BLOCKSIZEINBYTES)) - \
           IOBLKNOFORBLK((STARTINGBLOCK), (BLOCKSIZEINBYTES)) + 1))
#define IOBYTECCNTFORBLK(STARTINGBLOCK, BYTESTOTRANSFER, BLOCKSIZEINBYTES) \
    (IOBLKCNTFORBLK((STARTINGBLOCK),(BYTESTOTRANSFER),(BLOCKSIZEINBYTES)) * (BLOCKSIZEINBYTES))
#define IOBYTEOFFSETFORBLK(STARTINGBLOCK, BLOCKSIZEINBYTES) \
    (((STARTINGBLOCK) * 512) - \
     (IOBLKNOFORBLK((STARTINGBLOCK), (BLOCKSIZEINBYTES)) * (BLOCKSIZEINBYTES)))

#define IOBLKNOFORBYTE(STARTINGBYTE, BLOCKSIZEINBYTES) ((daddr_t)((STARTINGBYTE) / (BLOCKSIZEINBYTES)))
#define IOBLKCNTFORBYTE(STARTINGBYTE, BYTESTOTRANSFER, BLOCKSIZEINBYTES) \
((int)(IOBLKNOFORBYTE((STARTINGBYTE) + (BYTESTOTRANSFER) - 1, (BLOCKSIZEINBYTES)) - \
           IOBLKNOFORBYTE((STARTINGBYTE), (BLOCKSIZEINBYTES)) + 1))
#define IOBYTECNTFORBYTE(STARTINGBYTE, BYTESTOTRANSFER, BLOCKSIZEINBYTES) \
    (IOBLKCNTFORBYTE((STARTINGBYTE),(BYTESTOTRANSFER),(BLOCKSIZEINBYTES)) * (BLOCKSIZEINBYTES))
#define IOBYTEOFFSETFORBYTE(STARTINGBYTE, BLOCKSIZEINBYTES) ((STARTINGBYTE) - (IOBLKNOFORBYTE((STARTINGBYTE), (BLOCKSIZEINBYTES)) * (BLOCKSIZEINBYTES)))

#define MAKE_VREFNUM(x)	((int32_t)((x) & 0xffff))
/*
 *	This is the straight GMT conversion constant:
 *	00:00:00 January 1, 1970 - 00:00:00 January 1, 1904
 *	(3600 * 24 * ((365 * (1970 - 1904)) + (((1970 - 1904) / 4) + 1)))
 */
#define MAC_GMT_FACTOR		2082844800UL

#define HFS_ATTR_CMN_LOOKUPMASK (ATTR_CMN_SCRIPT | ATTR_CMN_FNDRINFO | ATTR_CMN_NAMEDATTRCOUNT | ATTR_CMN_NAMEDATTRLIST)
#define HFS_ATTR_DIR_LOOKUPMASK (ATTR_DIR_LINKCOUNT | ATTR_DIR_ENTRYCOUNT)
#define HFS_ATTR_FILE_LOOKUPMASK (ATTR_FILE_LINKCOUNT | ATTR_FILE_TOTALSIZE | ATTR_FILE_ALLOCSIZE | \
									ATTR_FILE_DATALENGTH | ATTR_FILE_DATAALLOCSIZE | ATTR_FILE_DATAEXTENTS | \
									ATTR_FILE_RSRCLENGTH | ATTR_FILE_RSRCALLOCSIZE | ATTR_FILE_RSRCEXTENTS)

#define VPUT(vp)	if (((vp)->v_usecount > 0) && (*((volatile int *)(&(vp)->v_interlock))==0)) vput((vp));  else panic("hfs: vput bad ref cnt (%d)!",  (vp)->v_usecount)
#define VRELE(vp)	if (((vp)->v_usecount > 0) && (*((volatile int *)(&(vp)->v_interlock))==0)) vrele((vp)); else panic("hfs: vrele bad ref cnt (%d)!", (vp)->v_usecount)

u_int32_t to_bsd_time(u_int32_t hfs_time);
u_int32_t to_hfs_time(u_int32_t bsd_time);

struct vnode *hfs_vhashget(dev_t dev, UInt32 dirID, UInt8 forkType);
void hfs_vhashins(struct hfsnode *hp);
void hfs_vhashinslocked(struct hfsnode *hp);
void hfs_vhashrem(struct hfsnode *hp);

short hfs_flushfiles(struct mount *mp, unsigned short flags);
short hfs_flushMDB(struct hfsmount *hfsmp, int waitfor);
short hfs_flushvolumeheader(struct hfsmount *hfsmp, int waitfor);

short hfsLookup (ExtendedVCB *vcb, UInt32 dirID, char *name, short len, UInt32 hint, hfsCatalogInfo *catInfo);
short hfsMoveRename (ExtendedVCB *vcb, UInt32 oldDirID, char *oldName, UInt32 newDirID, char *newName, UInt32 *hint);
short hfsCreate (ExtendedVCB *vcb, UInt32 dirID, char *name, int mode);
short hfsCreateFileID (ExtendedVCB *vcb, UInt32 parentDirID, StringPtr name, UInt32 catalogHint, UInt32 *fileIDPtr);
short hfsGet (ExtendedVCB *vcb, hfsCatalogInfo *catInfo, UInt8 forkType, struct vnode *dvp, struct vnode **vpp);
short hfsDelete (ExtendedVCB *vcb, UInt32 parentDirID, StringPtr name, short isfile, UInt32 catalogHint);
short hfsUnmount(struct hfsmount *hfsmp, struct proc *p);

extern int hfs_metafilelocking(struct hfsmount *hfsmp, u_long fileID, u_int flags, struct proc *p);
extern int hasOverflowExtents(struct hfsnode *hp);

short MacToVFSError(OSErr err);
void MapFileOffset(struct hfsnode *hp, off_t filePosition, daddr_t *logBlockNumber, long *blockSize, long *blockOffset);
long LogicalBlockSize(struct hfsnode *hp, daddr_t logicalBlockNumber);
void UpdateBlockMappingTable(struct hfsnode *hp);
void CopyVNodeToCatalogNode (struct vnode *vp, struct CatalogNodeData *nodeData);
void CopyCatalogToHFSNode(struct hfsCatalogInfo *catalogInfo, struct hfsnode *hp);
int bexpand(struct buf *bp, int newsize, struct buf **nbpp, long flags);

short make_dir_entry(struct FCB **fileptr, char *name, UInt32 fileID);

int AttributeBlockSize(struct attrlist *attrlist);
void PackCommonAttributeBlock(struct attrlist *alist,
							  struct vnode *vp,
							  struct hfsCatalogInfo *catInfo,
							  void **attrbufptrptr,
							  void **varbufptrptr);
void PackVolAttributeBlock(struct attrlist *alist,
						   struct vnode *vp,
						   struct hfsCatalogInfo *catInfo,
						   void **attrbufptrptr,
						   void **varbufptrptr);
void PackFileDirAttributeBlock(struct attrlist *alist,
							   struct vnode *vp,
							   struct hfsCatalogInfo *catInfo,
							   void **attrbufptrptr,
							   void **varbufptrptr);
void PackForkAttributeBlock(struct attrlist *alist,
							struct vnode *vp,
							struct hfsCatalogInfo *catInfo,
							void **attrbufptrptr,
							void **varbufptrptr);
void PackAttributeBlock(struct attrlist *alist,
						struct vnode *vp,
						struct hfsCatalogInfo *catInfo,
						void **attrbufptrptr,
						void **varbufptrptr);
void PackCatalogInfoAttributeBlock (struct attrlist *alist,
						struct vnode * root_vp,
						struct hfsCatalogInfo *catInfo,
						void **attrbufptrptr,
						void **varbufptrptr);
void UnpackCommonAttributeBlock(struct attrlist *alist,
							  struct vnode *vp,
							  struct hfsCatalogInfo *catInfo,
							  void **attrbufptrptr,
							  void **varbufptrptr);
void UnpackAttributeBlock(struct attrlist *alist,
						struct vnode *vp,
						struct hfsCatalogInfo *catInfo,
						void **attrbufptrptr,
						void **varbufptrptr);
unsigned long BestBlockSizeFit(unsigned long allocationBlockSize,
                               unsigned long blockSizeLimit,
                               unsigned long baseMultiple);

OSErr	hfs_MountHFSVolume(struct hfsmount *hfsmp, HFSMasterDirectoryBlock *mdb, struct proc *p);
OSErr	hfs_MountHFSPlusVolume(struct hfsmount *hfsmp, HFSPlusVolumeHeader *vhp, u_long embBlkOffset, struct proc *p);
OSStatus  GetInitializedVNode(struct hfsmount *hfsmp, struct vnode **tmpvnode );

/* B-tree I/O callbacks (in hfs_btreio.c) */
extern OSStatus	GetBTreeBlock(FileReference fileRefNum, UInt32 blockNum, GetBlockOptions options, BlockDescriptor *block);
extern OSStatus	ReleaseBTreeBlock(FileReference fileRefNum, BlockDescPtr blockPtr, ReleaseBlockOptions options);
extern OSStatus	SetBTreeBlockSize(FileReference fileRefNum, ByteCount blockSize, ItemCount minBlockCount);
extern OSStatus ExtendBTreeFile(FileReference fileRefNum, FSSize minEOF, FSSize maxEOF);

#endif /* __HFS__ */
