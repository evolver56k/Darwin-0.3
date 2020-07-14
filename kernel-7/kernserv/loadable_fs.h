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

/*	@(#)loadable_fs.h	2.0	26/06/90	(c) 1990 NeXT	*/

/* 
 * loadable_fs.h - message struct for loading and initializing loadable
 *		   file systems.
 *
 * HISTORY
 * 26-Jun-90	Doug Mitchell at NeXT
 *	Created.
 * 5-Nov-91	Lee Boynton at NeXT
 *	Added support for initialization, labels, and WSM options
 */

#ifndef	_LOADABLE_FS_
#define _LOADABLE_FS_

#import <mach/message.h>

/*
 * This message is used in an RPC between a lightweight app and a loaded
 * kernel server. 
 */
struct load_fs_msg {
	msg_header_t	lfm_header;
	msg_type_t	lfm_status_t;
	unsigned int	lfm_status;	/* status returned by server */
};

/*
 * msg_id values for load_fs_msg
 */
#define LFM_ID			0x123456	/* base */
#define LFM_DUMP_MOUNT		(LFM_ID+1)	/* dump active FS and VNODE 
						 * list */
#define LFM_TERMINATE		(LFM_ID+2)	/* terminate */
#define LFM_PURGE		(LFM_ID+3)	/* purge all vnodes except 
						 * root */
#define LFM_DUMP_LOCAL		(LFM_ID+4)	/* dump local FS info */
#define LFM_ENABLE_TRACE	(LFM_ID+5)	/* enable debug trace */
#define LFM_DISABLE_TRACE	(LFM_ID+6)	/* disable debug trace */
					 
/*
 * lfm_stat values 
 */
#define LFM_STAT_GOOD		0	/* file system loaded OK */
#define LFM_STAT_NOSPACE	1	/* no space in vfssw */
#define LFM_STAT_BADMSG		2	/* bad message received */
#define LFM_STAT_UNDEFINED	3

/*
 * Constants for Loadabls FS Utilities (in "/usr/FileSystems")
 *
 * Example of a /usr/filesystems directory
 *
 * /usr/filesystems/dos.fs/dos.util		utility with which WSM 
 *							communicates
 * /usr/filesystems/dos.fs/dos.name 		"DOS Floppy" 
 * /usr/filesystems/dos.fs/dos_reloc		actual loadable filesystem
 * /usr/filesystems/dos.fs/dos.openfs.tiff	"open folder" icon 
 * /usr/filesystems/dos.fs/dos.fs.tiff		"closed folder" icon 
 */
#define FS_DIR_LOCATION		"/usr/filesystems"	
#define FS_DIR_SUFFIX		".fs"
#define FS_UTIL_SUFFIX		".util"
#define FS_OPEN_SUFFIX		".openfs.tiff"
#define FS_CLOSED_SUFFIX	".fs.tiff"
#define FS_NAME_SUFFIX		".name"
#define FS_LABEL_SUFFIX		".label"

/*
 * .util program commands - all sent in the form "-p" or "-m" ... as argv[1].
 */
#define FSUC_PROBE		'p'	/* probe FS for mount or init */
	/* example usage: foo.util -p fd0 removable writable */ 

#define FSUC_PROBEFORINIT	'P'	/* probe FS for init only */
	/* example usage: foo.util -P fd0 removable */ 

#define FSUC_MOUNT		'm'	/* mount FS */
	/* example usage: foo.util -m fd0 /bar removable writable */ 

#define FSUC_REPAIR		'r'	/* repair ('fsck') FS */ 
	/* example usage: foo.util -r fd0 removable */

#define	FSUC_INITIALIZE		'i'	/* initialize FS */
	/* example usage: foo.util -i fd0 removable */ 

#define FSUC_UNMOUNT		'u'	/* unmount FS */
	/* example usage: foo.util -u fd0 /bar */ 

/* The following is not used by Workspace Manager */
#define FSUC_MOUNT_FORCE	'M'	/* like FSUC_MOUNT, but proceed even on
					 * error. */
/*
 * Return codes from .util program
 */
#define FSUR_RECOGNIZED		(-1)	/* response to FSUC_PROBE; implies that
					 * a mount is possible */
#define FSUR_UNRECOGNIZED	(-2)	/* negative response to FSUC_PROBE */
#define FSUR_IO_SUCCESS		(-3)	/* mount, unmount, repair succeeded */
#define FSUR_IO_FAIL		(-4)	/* unrecoverable I/O error */
#define FSUR_IO_UNCLEAN		(-5)	/* mount failed, file system not clean 
					 */
#define FSUR_INVAL		(-6)	/* invalid argument */
#define FSUR_LOADERR		(-7)	/* kern_loader error */
#define FSUR_INITRECOGNIZED	(-8)	/* response to FSUC_PROBE or 
					 * FSUC_PROBEFORINIT, implies that
					 * initialization is possible */

/*
 *	mount parameters passed from WSM to the .util program.
 */
#define	DEVICE_READONLY		"readonly"
#define	DEVICE_WRITABLE		"writable"

#define	DEVICE_REMOVABLE	"removable"
#define	DEVICE_FIXED		"fixed"

/*
 *	Additional parameters to the mount command - used by WSM when they
 *	appear in the /etc/mtab file.
 */
#define	MNTOPT_FS		"filesystem=" /* e.g. "filesystem=DOS" */
#define	MNTOPT_REMOVABLE	"removable"

#endif	/* _LOADABLE_FS_ */
