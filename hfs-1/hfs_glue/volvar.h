/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
    Copyright (c) 1987-98 Apple Computer, Inc.
    All Rights Reserved.

    THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
    The copyright notice above does not evidence any actual or
    intended publication of such source code.

    About volvar.h:
    This file contains the API definitions for the Rhapsody Volume/Directory ID (VDI) interface.

    To do:
    Look for "PPD" for unresolved issues

    Change History:
     3-Dec-1998 Pat Dirks       Changed ATTR_GVI_VOLUMEBITMAP to include ATTR_VOL_MOUNTFLAGS.
     7-Jul-1998	Pat Dirks		Changed VolInfoReturnBuf to move volumeName field into VolAttrBuf struct.
    11-May-1998	Don Brady		Change VOLFS_PREFIX to "/.vol"
    11-May-1998	Don Brady		Use O_EXCL instead of O_TRUNC in DO_CREATE macro (radar #2220488).
    17-Apr-1998	Pat Dirks		Changed DO_CREATE macro to call open directly (w. O_CREAT + O_TRUNC)
     8-Apr-1998 chw				Changed arguments to DO_USERACCESS to match kernel routine
     7-Apr-1998	Pat Dirks		Changed DO_CHECKUSERACCESS to DO_USERACCESS.
     1-May-1998	jwc				Birth of a file.
*/
#ifndef __VOLVAR__
#define __VOLVAR__

// ************************************ I N C L U D E S ************************************

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/attr.h>
#include "vol.h"

// ************************************** D E F I N E S **************************************

#define VOLFS_PREFIX "/.vol"

#define FST_EOF 					(-1)

#define ATTR_GCI_COMMONBITMAP		(ATTR_CMN_DEVID | ATTR_CMN_FSID | ATTR_CMN_OBJTYPE | \
                                    ATTR_CMN_OBJID | ATTR_CMN_PAROBJID | ATTR_CMN_SCRIPT | \
                                    ATTR_CMN_CRTIME | ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | \
                                    ATTR_CMN_BKUPTIME | ATTR_CMN_FNDRINFO | ATTR_CMN_OWNERID | \
                               	 	ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK | ATTR_CMN_FLAGS)

#define ATTR_GCI_DIRECTORYBITMAP	ATTR_DIR_ENTRYCOUNT

#define ATTR_GCI_FILEBITMAP			(ATTR_FILE_TOTALSIZE | ATTR_FILE_IOBLOCKSIZE | \
                                    /* ATTR_FILE_FORKCOUNT  | */ ATTR_FILE_DATALENGTH | \
                               		ATTR_FILE_DATAALLOCSIZE | ATTR_FILE_RSRCLENGTH |\
                               		ATTR_FILE_RSRCALLOCSIZE)

#define ATTR_GVI_VOLUMEBITMAP		(ATTR_VOL_INFO | ATTR_VOL_FSTYPE | ATTR_VOL_SIGNATURE | \
									ATTR_VOL_SIZE | ATTR_VOL_SPACEFREE | ATTR_VOL_MINALLOCATION | \
                                    ATTR_VOL_ALLOCATIONCLUMP | ATTR_VOL_FILECOUNT | ATTR_VOL_DIRCOUNT | \
                                    ATTR_VOL_NAME | ATTR_VOL_MOUNTFLAGS)

#define ATTR_SCI_COMMONBITMAP		(ATTR_CMN_SCRIPT | ATTR_CMN_CRTIME | ATTR_CMN_MODTIME | ATTR_CMN_CHGTIME | \
									ATTR_CMN_ACCTIME | ATTR_CMN_BKUPTIME | ATTR_CMN_FNDRINFO | \
									ATTR_CMN_OWNERID | ATTR_CMN_GRPID | ATTR_CMN_ACCESSMASK)

#define ATTR_SVI_COMMONBITMAP		(ATTR_CMN_SCRIPT | ATTR_CMN_CRTIME | ATTR_CMN_MODTIME | \
									ATTR_CMN_BKUPTIME | ATTR_CMN_FNDRINFO)

// ************************************ T Y P E D E F S ************************************


/* No defined options are currently passed on to the underlying OS: */
#define kOSOptionsMask 0

#define kMaxNameBufferSize	512

/* The contents of a common attributes buffer with commonBitmap = ATTR_GCI_COMMONBITMAP: */
struct CommonAttrBuf {
    dev_t				device;
    fsid_t				fsid;
    fsobj_type_t		objectType;
    fsobj_id_t			objectID;	
    fsobj_id_t			parentDirID;
    text_encoding_t		nameEncoding;
    struct timespec		creationTime;
    struct timespec		lastModificationTime;
    struct timespec		lastChangeTime;
    struct timespec		lastBackupTime;
    u_long				finderInfo[8];
    uid_t				uid;
    gid_t				gid;
    u_long				mode;
    u_long				flags;
};

struct DirAttrBuf {
	u_long			numEntries;
};

struct FileAttrBuf {
	off_t			totalSize;		/* size of all forks, in blocks */
	u_long			blockSize;		/* Optimal file I/O blocksize */
//	u_long			numForks;		/* Number of forks in the file */
	off_t			dataLogicalLength;
	off_t			dataPhysicalLength;
	off_t			resourceLogicalLength;
	off_t			resourcePhysicalLength;
};

struct catalogAttrBlock {
    struct CommonAttrBuf c;
    union
      {
          struct FileAttrBuf f;
          struct DirAttrBuf d;
      } u;
};

typedef struct NamelessGetCatAttrBuf {
    u_long size;
    struct catalogAttrBlock ca;
} NamelessGetCatAttrBuf;

typedef struct NamedGetCatAttrBuf {
    u_long size;
    attrreference_t objectName;
    struct catalogAttrBlock ca;
    char objectNameBuffer[kMaxNameBufferSize];
} NamedGetCatAttrBuf;

typedef union GetCatAttributeBuffer {
    NamelessGetCatAttrBuf a;
    NamedGetCatAttrBuf na;
} GetCatAttributeBuffer;

struct VolAttrBuf {
	u_long				fstype;
	u_long				signature;
	off_t				size;
	off_t				spacefree;
	off_t				minallocation;
	off_t				allocationclump;
	u_long				filecount;
	u_long				dircount;
    attrreference_t		volumename;
    u_long				mountflags;
};

typedef struct VolInfoReturnBuf {
    u_long size;
	struct CommonAttrBuf c;
	struct VolAttrBuf v;
    char n[kMaxNameBufferSize];
} VolInfoReturnBuf;

typedef struct VolInfoChangeBuf {
    text_encoding_t		nameEncoding;
    struct timespec		creationTime;
    struct timespec		lastModificationTime;
    struct timespec		lastBackupTime;
    u_long				finderInfo[8];
    attrreference_t		volumeNameRef;
    char				volumeName[kMaxNameBufferSize];
} VolInfoChangeBuf;

typedef struct SetCommAttrBuf {
    struct timespec			cr_time;
    struct timespec			m_time;
    struct timespec			c_time;
    struct timespec			a_time;
    struct timespec			b_time;
    u_int8_t				finderinfo[32];
    uid_t					owner;
    gid_t					group;
    u_long					mode;
    u_long					flags;
} SetCommAttrBuf, *SetCommAttrBufPtr;

typedef struct CreateFileIDReturnBuf {
    u_long					size;
    fsobj_id_t				permanentID;
} CreateFileIDReturnBuf, *CreateFileIDReturnBufPtr;

typedef struct FileIDResolveBuf {
    u_long					size;
    attrreference_t			name;
    fsobj_id_t				parentDirID;
    text_encoding_t			nameEncoding;
    char					nameBuf[kMaxNameBufferSize];
} FileIDResolveBuf, *FileIDResolveBufPtr;


// ************************************** M A C R O S **************************************

#define GET_ATTR_REF_ADDR(attref) (void*)(((u_long) &(attref)) + (attref).attr_dataoffset)

#endif /* __VOLVAR__ */

