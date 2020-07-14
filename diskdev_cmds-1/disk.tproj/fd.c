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
/*	@(#)fd.c	2.0	03/05/90	(c) 1990 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 16-Sep-91	Garth Snyder at NeXT
 *	Added support for variable device blocksizes
 * 05-Mar-89	Doug Mitchell at NeXT
 *	Created.
 *
 **********************************************************************
 */
 
#include <sys/types.h>
#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include <sys/ioctl.h>
#include <bsd/dev/disk.h>
#include <sys/time.h>
#include "disk.h"
#include <errno.h>
#import <libc.h>
#import <stdio.h>
#import <unistd.h>
#include <bsd/dev/fd_extern.h>
#include <bsd/dev/scsireg.h>
#import <sys/file.h>

extern int density;
extern int force_blocksize;

#ifndef	TRUE
#define TRUE	(1)
#endif	TRUE
#ifndef	FALSE
#define FALSE	(0)
#endif	FALSE

int fd_conf()
{
	return 0;
}

int fd_init()
{
	return 0;
}

int fd_wlabel()
{
	register struct disk_label *l = &disk_label;

	if (ioctl (fd, DKIOCSLABEL, l) < 0)
		dpanic (S_MEDIA, "can't write label -- disk unusable!");
	return 0;
}

int fd_glabel (l, bb, rtn)
	register struct disk_label *l;
	struct bad_block *bb;
	int rtn;
{
	extern int errno;

	if (ioctl (fd, DKIOCGLABEL, l) < 0) {
		if (rtn)
			return (-1);
		if (errno == ENXIO)
			bomb (S_NEVER, "no label on disk");
		dpanic (S_NEVER, "get label");
	}
	return (0);
}

int fd_cyl (l)
	struct disk_label *l;
{
	dsp->ds_maxcyl = l->dl_ncyl;
	return 0;
}

int fd_geterr()
{
	return 0;
}

int fd_req (int cmd, int start, char *buf, int count)
{
	off_t rtn;
	off_t offset;
	int byte_count;
	int sect_count;
	char *rwbuf = NULL;
	int freebuf = 0;
	
	switch (cmd) {
	case CMD_READ:
	case CMD_WRITE:
		rwbuf = (char *)buf;
		offset = start * devblklen;
		rtn = lseek(lfd, offset, L_SET);
		if(rtn != offset) {
			printf("...lseek returned %qd; expected %qd\n", 
				rtn, offset);
			rtn = -1;
			break;
		}
		
		/*
		 * Round to sector size if necessary.
		 */
		sect_count = howmany(count, devblklen);
		byte_count = sect_count * devblklen;
		if(byte_count > count) {
			rwbuf = (char *)malloc(byte_count);
			freebuf = 1;
		} 
		
		if(cmd == CMD_READ) {
			rtn = read(lfd, rwbuf, byte_count);
			if (freebuf)
				bcopy(rwbuf, (char *)buf, count);
		}
		else {
			if (freebuf)
				bcopy((char *)buf, rwbuf, count);
			rtn = write(lfd, rwbuf, byte_count);
		}
		if(freebuf) {
			free(rwbuf); 
		}
		if(rtn != byte_count) {
			printf("...r/w returned %qd; expected %d\n", 
				rtn, byte_count);
			rtn = -1;
			break;
		}
		rtn = 0;
		break;
		
	default:
		rtn = -1;
		break;
	}
	return(rtn);
	
} /* fd_req() */


int fd_pstats()
{
	return 0;
}

int fd_ecc()
{
	return (0);
}

static struct disktab fd_dt;

#define FPORCH	96
#define BPORCH	0
#define	CYLPGRP	32
#define	BYTPERINO 2048		/* Floppies need more inodes 'cuz they small */
#define	MINFREE	0

#define	NELEM(x)		(sizeof(x)/sizeof(x[0]))
#define	LAST_ELEM(x)		((x)[NELEM(x)-1])

#define FD_SECTSIZE_DEF		512
#define FD_NSECTS_DEF		36
#define FD_NCYLS_DEF		80
#define FD_TOTALSECT_DEF	5760

static int fd_formatted = 0;	/* disk was formatted in this exec of 'disk' */
 
int fd_format(int fd, char *devname, int force)  
{
	/*
	 * Format disk if we haven't already done so (we might have already 
	 * done this in fd_inferdisktab()...).
	 * Returns an errno. Assumes global density from command line (0 means
	 * format at default density).
	 */
	char command_str[80];
	int rtn;
	
	if(fd_formatted) {
		fd_formatted = 0;	/* for interactive use */
		return(0);
	}
	if(density)
		sprintf(command_str, FDFORM_PATH " %sa d=%d\n",
			devname, density);
	else
		sprintf(command_str, FDFORM_PATH " %sa\n", devname);
	rtn = system(command_str);
	if(rtn == 0)
		fd_formatted = 1;
	return(rtn);
}

struct disktab *
fd_inferdisktab(int fd, char *devname, int init_flag, int density)
{
	struct drive_info di;
	int sfactor, ncyl, ntracks, nsectors, nblocks;
	char namebuf[512];
	struct fd_format_info format_info;
	
	/*
	 * Get physical disk parameters. We'll format the disk if init_flag
	 * is true (i.e., we're either doing a Format or an init command).
	 * Density of 0 means use default density for given media_id.
	 *
	 * Note we format here instead of in the call to (*dsp->ds_format)()
	 * in init() to set up the proper disktab entry.
	 *
	 * If init_flag is false and the disk is unformatted, we'll wing it
	 * and generate just the disktab entries we need for other things.
	 */
	if(init_flag) {
		if(fd_format(fd, devname, TRUE))
			return NULL;
	}
	if(ioctl(fd, FDIOCGFORM, &format_info) < 0) 
		return NULL;
	/*
	 * Generate bogus format info is not formatted at this point. 
	 */
	if (ioctl(fd, DKIOCINFO, &di) < 0)
		return NULL;
	if(!(format_info.flags & FFI_FORMATTED)) {
		format_info.sectsize_info.sect_size = FD_SECTSIZE_DEF;
		format_info.disk_info.tracks_per_cyl = 2;
		format_info.sectsize_info.sects_per_trk = FD_NSECTS_DEF;
		format_info.total_sects = FD_TOTALSECT_DEF;
		format_info.disk_info.num_cylinders = FD_NCYLS_DEF;
	}

	/*
	 * One way or another, we have reasonable format info.
	 */
	
	if (force_blocksize && 
	    (format_info.sectsize_info.sect_size < DEV_BSIZE)) 
	{
	    sfactor = DEV_BSIZE/(format_info.sectsize_info.sect_size);
	    if ((sfactor != 1) && (sfactor != 2) && (sfactor != 4))
		    return NULL;
	} else {
	    sfactor = 1;  /* The blocksize is what it is */
	}

	ntracks  = format_info.disk_info.tracks_per_cyl;
	nsectors = format_info.sectsize_info.sects_per_trk/sfactor;
	nblocks  = format_info.total_sects/sfactor;
	ncyl     = format_info.disk_info.num_cylinders;

	sprintf(namebuf, "%.*s-%d", MAXDNMLEN, di.di_name, 
		format_info.sectsize_info.sect_size);
	if (strlen(namebuf) < MAXDNMLEN)
		strcpy(fd_dt.d_name, namebuf);
	else {
		strncpy(fd_dt.d_name, di.di_name, MAXDNMLEN);
		LAST_ELEM(fd_dt.d_name) = '\0';
	}
	strcpy(fd_dt.d_type, "removable_rw_floppy");
	fd_dt.d_secsize = (force_blocksize && 
	    (format_info.sectsize_info.sect_size < DEV_BSIZE)) ? DEV_BSIZE : 
	    format_info.sectsize_info.sect_size;
	fd_dt.d_ntracks = ntracks;
	fd_dt.d_nsectors = nsectors;
	fd_dt.d_ncylinders = ncyl;
	fd_dt.d_rpm = 300;
	fd_dt.d_front = FPORCH * 1024 / fd_dt.d_secsize;
	fd_dt.d_back = BPORCH;
	fd_dt.d_ngroups = fd_dt.d_ag_size = 0;
	fd_dt.d_ag_alts = fd_dt.d_ag_off = 0;
	fd_dt.d_boot0_blkno[0] = 32 * 1024 / fd_dt.d_secsize;
	fd_dt.d_boot0_blkno[1] = -1;		/* only one boot block */
#if m68k
	strcpy(fd_dt.d_bootfile, "fdmach");
#else
	strcpy(fd_dt.d_bootfile, "mach_kernel");
#endif m68k
	gethostname(fd_dt.d_hostname, MAXHNLEN);
	fd_dt.d_hostname[MAXHNLEN-1] = '\0';
	fd_dt.d_rootpartition = 'a';
	fd_dt.d_rwpartition = 'b';

	{
		struct partition *pp;
		int i;

		pp = &fd_dt.d_partitions[0];
		pp->p_base = 0;
		pp->p_size = nblocks - fd_dt.d_front - fd_dt.d_back;
		pp->p_bsize = BLKSIZE;
		pp->p_fsize = MAX(fd_dt.d_secsize, 1024);
		pp->p_cpg = CYLPGRP;
		pp->p_density = BYTPERINO;
		pp->p_minfree = MINFREE;
		pp->p_newfs = 1;
		pp->p_mountpt[0] = '\0';
		pp->p_automnt = 1;
		pp->p_opt = 't';
		strcpy(pp->p_type, "4.4BSD");

		for (i = 1; i < NPART; i++) {
			pp = &fd_dt.d_partitions[i];
			pp->p_base = -1;
			pp->p_size = -1;
			pp->p_bsize = -1;
			pp->p_fsize = -1;
			pp->p_cpg = -1;
			pp->p_density = -1;
			pp->p_minfree = -1;
			pp->p_newfs = 0;
			pp->p_mountpt[0] = '\0';
			pp->p_automnt = 0;
			pp->p_opt = '\0';
			pp->p_type[0] = '\0';
		}
	}

	return &fd_dt;
	
} /* fd_inferdisktab() */

