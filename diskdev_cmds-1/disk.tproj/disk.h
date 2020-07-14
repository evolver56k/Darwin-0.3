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
/*	@(#)disk.h	1.0	08/28/87	(c) 1987 NeXT	*/

/* 
 * HISTORY
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added exec_time for scsi_req support.
 */
 
#include <bsd/dev/scsireg.h>

/* commands */
#define	CMD_SEEK	0
#define	CMD_READ	1
#define	CMD_WRITE	2
#define	CMD_VERIFY	3
#define	CMD_EJECT	4
#define	CMD_RESPIN	5
#define	CMD_ERASE	6

/* flags */
#define	SPEC_RETRY	1

/* errors */
#define	ERR_UNKNOWN	0
#define	ERR_ECC		1

/* return status */
#define	S_NEVER		1	/* should "never" happen */
#define	S_EMPTY		2	/* no media inserted */
#define	S_MEDIA		3	/* media is bad (can't write label) */

/*
 * PPC has 4K page_size so making a 4K filesystem is good for  paging
 * performance
 */
#ifdef __ppc__
#define	BLKSIZE		4096
#else
#define	BLKSIZE		8192
#endif
#define FRGSIZE     1024    /* Not used: FRGSIZE = devbsize */

#define DEV_BSIZE	1024	/* No longer in the kernel */
#define FDFORM_PATH	"/usr/sbin/fdform"
#define SDFORM_PATH	"/usr/sbin/sdform"

struct dtype_sw {
	char	*ds_type;	/* drive "type" from disktab "ty" entry */
	int	ds_typemask;	/* or of TY_* bits */
	int	(*ds_config)();	/* device configuration */
	int	(*ds_devinit)();/* complete device initialization */
	int	(*ds_req)();	/* request device specific cmd */
	int	(*ds_geterr)();	/* interpret command errors */
	int	(*ds_wlabel)();	/* write label */
	int	(*ds_cyl)();	/* compute max cylinder */
	int	(*ds_pstats)();	/* print device specific stats */
	int	(*ds_ecc)();	/* current ECC count */
	int	(*ds_glabel)();	/* get label */
	int 	(*ds_format)();	/* format volume if necessary */
	int	ds_basecyl;	/* base of active media area (cylinders) */
	int	ds_base;	/* base of active media area (sectors) */
	int	ds_dcyls;	/* display cylinders */
	int	ds_icyls;	/* increment cylinders */
	int	ds_maxcyl;	/* max cylinder */
};

extern struct	disk_label disk_label;
extern struct	bad_block bad_block;
extern struct	disk_req req;
extern struct	dtype_sw *dsp;
extern struct	disktab disktab, *dt;
extern struct	disk_stats stats;
extern struct 	timeval exec_time;
extern int	*bit_map, lfd, fd, d_size, devblklen, dosdisk;
extern char *	fn;
extern unsigned long dosbase, dossize;
extern unsigned long apple_ufs_base;
extern unsigned long apple_ufs_size;
extern int	apple_block_size;
extern int 	apple_disk;
extern char 	dev[100];

int has_apple_partitions();
void dpanic (int status, const char *msg);
volatile void bomb (int status, char *fmt, ...);
int uses_fdisk();
int write_bitmap();
int do_inquiry(int fd, struct inquiry_reply *irp);
