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
/*	@(#)hd.c	1.0	10/27/92	(c) 1992 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 27-Oct-92 Matt Watson at NeXT
 *	IDE module -- created from sd.c
 *
 **********************************************************************
 */
 
#include <errno.h>
#import <libc.h>
#import <limits.h>
#import <stdio.h>
#import <unistd.h>
#include <bsd/dev/disk.h>
#include <bsd/dev/ata_extern.h>
#include <bsd/sys/file.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include "disk.h"

#ifndef	BFD_PART_TYPE
#define BFD_PART_TYPE	"bfd"		/* from <mon> eventually */
#endif	BFD_PART_TYPE

#define	FPORCH_HARD	160		/* front porch size in K */
#define	BPORCH		0
#define RPM_HARD	3600
#define	CYLPGRP		16
#define	BYTPERINO	4096

#define	MINFREE_LARGE_THRESH	(150*1024) 	/* 150MB */
#define	MINFREE_LARGE	10		/* % minfree for large drives */
#define	MINFREE_SMALL	5		/* % minfree for small drives */

static struct disktab hddt;

#ifdef i386
extern int useAllSectors;
extern int biosAccessibleBlocks;
#endif

int hd_conf()
{
	/* eventually do mode select stuff here */
	return 0;
}

int hd_init()
{
	return 0;
}

int hd_wlabel()
{
	register struct disk_label *l = &disk_label;

	if (ioctl (lfd, DKIOCSLABEL, l) < 0)
		dpanic (S_MEDIA, "can't write label -- disk unusable!");
	return 0;
}

int hd_glabel (l, bb, rtn)
	register struct disk_label *l;
	struct bad_block *bb;
	int rtn;
{
	extern int errno;

	if (ioctl (lfd, DKIOCGLABEL, l) < 0) {
		if (rtn)
			return (-1);
		if (errno == ENXIO)
			bomb (S_NEVER, "no label on disk");
		dpanic (S_NEVER, "get label");
	}
	return (0);
}

int hd_cyl (l)
	struct disk_label *l;
{
	dsp->ds_maxcyl = l->dl_ncyl;
	return 0;
}

int hd_geterr()
{
	return 0;
}

int hd_req (int cmd, int start, char *buf, int count)
{
	off_t rtn;
	off_t offset = 0;
	int byte_count;
	int sect_count;
	char *rwbuf = NULL;
	int freebuf = 0;
	
	switch (cmd) {
	case CMD_READ:
	case CMD_WRITE:
		rwbuf = (char *)buf;
		offset = (off_t)start * devblklen;
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
	return((int)rtn);
	
} /* hd_req() */


int hd_pstats()
{
	return 0;
}

int hd_ecc()
{
	return (0);
}

int hd_format(int fd, char *devname, int force) 
{
	int data;
	char cmd_str[80];
	
#if 0 /* No such command */
#define FORMAT_CMD		"/usr/sbin/hdform"
	sprintf(cmd_str, "exec %s %sa n\n", FORMAT_CMD, devname);
	if(system(cmd_str)) 
		return(1);		/* bomb on error */
#endif
	/*
	 * Set volume to formatted state
	 */
	data = 1;
	ioctl(fd, DKIOCSFORMAT, &data);
	return(0);
}

struct disktab *
hd_inferdisktab(int fd, int partition_size, int bfd_size)
{
	extern struct dtype_sw *drive_type(), *dsp;
	extern int dosdisk, force_blocksize;
	extern unsigned long dossize, dosbase;
	ideDriveInfo_t info;
	int ncyl, ntracks, nsectors, nblocks;
	char namebuf[512];
	int min_free;
	u_int blklen, lastlba;
	int sfactor;
#if i386
	int tbiosAccessibleBlocks = biosAccessibleBlocks;
#endif
	if (ioctl(fd, IDEDIOCINFO, &info) < 0)
		return NULL;

	blklen = info.bytes_per_sector;
	lastlba = info.total_sectors - 1;
      
	strcpy(hddt.d_type, "fixed_rw_ide");
	dsp = drive_type(hddt.d_type);
	dosdisk = uses_fdisk();
	apple_disk = has_apple_partitions();
	if (apple_disk && apple_block_size != blklen) {
	    fprintf(stderr, "apple block size %d != device block length %d\n",
		    apple_block_size, blklen);
	    return (NULL);
	}

	if (force_blocksize && (blklen < DEV_BSIZE)) {
		sfactor = DEV_BSIZE / blklen;
		if (apple_disk)
		    nblocks = apple_ufs_size / sfactor;
		else
		    nblocks = dosdisk ? dossize  : (lastlba + 1) / sfactor;
#if i386
		tbiosAccessibleBlocks /= sfactor;
#endif
	} else {
	    sfactor = 1;
	    if (apple_disk)
		nblocks = apple_ufs_size;
	    else
		nblocks = dosdisk ? dossize : lastlba + 1;
	}
		
#ifdef i386
	if (blklen == 512 && !useAllSectors && !dosdisk && (nblocks > tbiosAccessibleBlocks))
	{
		printf("Only using BIOS-accessible sectors\n");
		nblocks = tbiosAccessibleBlocks;
	}
#endif i386

	ntracks  = info.heads;
	nsectors = info.sectors_per_trk;
	ncyl     = info.cylinders;
	
	if((partition_size + bfd_size) > nblocks) {
		fprintf(stderr, "Requested partition size larger than disk\n");
		return NULL;
	}

	sprintf(namebuf, "Type %d-%d", info.type, blklen);
	strcpy(hddt.d_name, namebuf);

	hddt.d_secsize = (force_blocksize && (blklen < DEV_BSIZE)) ? 
	    DEV_BSIZE : blklen;
	hddt.d_ntracks = ntracks;
	hddt.d_nsectors = nsectors;
	hddt.d_ncylinders = ncyl;
	hddt.d_rpm            = RPM_HARD;
	hddt.d_front          = FPORCH_HARD * 1024 / hddt.d_secsize;
	if (apple_disk) {
	    hddt.d_boot0_blkno[0] = 32 * 1024 / hddt.d_secsize 
		+ apple_ufs_base;
	    hddt.d_boot0_blkno[1] = 96 * 1024 / hddt.d_secsize 
		+ apple_ufs_base;
	}
	else {
	    hddt.d_boot0_blkno[0] = 32 * 1024 / hddt.d_secsize + dosbase;
	    hddt.d_boot0_blkno[1] = 96 * 1024 / hddt.d_secsize + dosbase;
	}
	if(((unsigned long long)nblocks * hddt.d_secsize)/1024 > MINFREE_LARGE_THRESH)
		min_free = MINFREE_LARGE;
	else 
		min_free = MINFREE_SMALL;
	hddt.d_back = BPORCH;
	hddt.d_ngroups = hddt.d_ag_size = 0;
	hddt.d_ag_alts = hddt.d_ag_off = 0;
	strcpy(hddt.d_bootfile, "mach_kernel");
	gethostname(hddt.d_hostname, MAXHNLEN);
	hddt.d_hostname[MAXHNLEN-1] = '\0';
	hddt.d_rootpartition = 'a';
	hddt.d_rwpartition = 'b';

	{
		struct partition *pp;
		int i = 1;
		int bfd_base = 0;

		pp = &hddt.d_partitions[0];
		if (apple_disk)
		    pp->p_base = apple_ufs_base / sfactor;
		else
		    pp->p_base = dosdisk ? dosbase : 0;
		pp->p_size = partition_size ? partition_size :
				nblocks - hddt.d_front - BPORCH - bfd_size;
		pp->p_bsize = BLKSIZE;
		pp->p_fsize = MAX(hddt.d_secsize, DEV_BSIZE);
		pp->p_cpg = CYLPGRP;
		pp->p_density = BYTPERINO;
		pp->p_minfree = min_free;
		pp->p_newfs = 1;
		pp->p_mountpt[0] = '\0';
		pp->p_automnt = 1;
		pp->p_opt = min_free < 10 ? 's' : 't';
		strcpy(pp->p_type, "4.4BSD");

		if (partition_size) {
			pp = &hddt.d_partitions[i++];
			pp->p_base = partition_size + hddt.d_partitions[0].p_base;
			pp->p_size = nblocks - partition_size -
				hddt.d_front - BPORCH - bfd_size;
			pp->p_bsize = BLKSIZE;
			pp->p_fsize = (hddt.d_secsize < 1024) ? 1024 : hddt.d_secsize;
			pp->p_cpg = CYLPGRP;
			pp->p_density = BYTPERINO;
			pp->p_minfree = min_free;
			pp->p_newfs = 1;
			pp->p_mountpt[0] = '\0';
			pp->p_automnt = 1;
			pp->p_opt = min_free < 10 ? 's' : 't';
			strcpy(pp->p_type, "4.4BSD");
			bfd_base = pp->p_base + pp->p_size;
		}
		else {
			bfd_base = hddt.d_partitions[0].p_base + 
			  	   hddt.d_partitions[0].p_size;
		}

		for ( ; i < NPART; i++) {
			pp = &hddt.d_partitions[i];
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
		
		if(bfd_size) {
			/*
			 * Put bfd in partition 6 by convention; nobody
			 * depends on this. Only base and size matter here;
			 * ufs parameters are ignored.
			 */
			pp = &hddt.d_partitions[6];
			pp->p_base = bfd_base;
			pp->p_size = bfd_size;
			
			pp->p_bsize = -1;
			pp->p_fsize = -1;
			pp->p_newfs = 0;
			strcpy(pp->p_type, BFD_PART_TYPE);
		}
	}

	return &hddt;
}
