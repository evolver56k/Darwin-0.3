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
 * Copyright (c) 1997-1998 Apple Computer, Inc. All Rights Reserved
 *
 *		MODIFICATION HISTORY (most recent first):
 *
 *	   20-Aug-1998	Scott Roberts	Added uid, gid, and mask to hfs_mount_args.
 *	   24-Jun-1998	Don Brady		Added time zone info to hfs_mount_args (radar #2226387).
 *	   30-Jul-1997	Pat Dirks		created
 */


/*
 * Arguments to mount HFS-based filesystems
 */

struct hfs_mount_args {
	char 				*ma_fspec;		/* block special device to mount */
	struct export_args 	ma_export;		/* network export info */
    uid_t				ma_uid;			/* uid that owns hfs files (standard HFS only) */
    gid_t				ma_gid;			/* gid that owns hfs files (standard HFS only) */
    mode_t  			ma_mask;		/* mask to be applied for hfs perms  (standard HFS only) */
    u_long				ma_encoding;	/* default encoding for this volume (standard HFS only) */
	struct timezone 	ma_timezone;	/* user time zone info (standard HFS only) */
    int					ma_flags;		/* mounting flags, see below */
};

#define HFSFSMNT_NOXONFILES		0x1	/* Allow execute on files */

#define MXENCDNAMELEN			15	/* Maximun length of encoding name string */

struct hfs_mnt_encoding {
    char	encoding_name[MXENCDNAMELEN];	/* encoding type name */
    u_long	encoding_id;				/* encoding type number */
};

/*
 *	typedefs and macros to mark a fs as hfs standard
 */
#define HFSFSMNT_HFSSTDMASK		0x40000000			/* Set to determine if a volume is hfs standard */
#define ISHFSSTD(fsid)		((Int32)fsid & HFSFSMNT_ISHFSSTDMASK)	/* To determine if a volume is hfs */



/*
 * Lookup table for encoding strings
 */
struct hfs_mnt_encoding hfs_mnt_encodinglist[] = {

	{ "shift-JIS", 1 },
	{ "Shift-JIS", 1 }
};

