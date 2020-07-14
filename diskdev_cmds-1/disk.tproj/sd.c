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
/*	@(#)sd.c	1.0	08/28/87	(c) 1987 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 4-Aug-92 Matt Watson at NeXT
 *	Added 486 specifics
 * 16-Sep-91	Garth Snyder at NeXT
 *	Added support for variable blocksize filesystems
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added scsi_req support.
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
#include <bsd/sys/file.h>
#include <sys/time.h>
#include "disk.h"
#include <errno.h>
#import <libc.h>
#import <limits.h>
#import <stdio.h>
#import <unistd.h>
#include <bsd/dev/scsireg.h>

int sd_conf()
{
	/* eventually do mode select stuff here */
	return 0;
}

int sd_init()
{
	return 0;
}

int sd_wlabel()
{
	register struct disk_label *l = &disk_label;

	if (ioctl (lfd, DKIOCSLABEL, l) < 0)
		dpanic (S_MEDIA, "can't write label -- disk unusable!");
	return 0;
}

int sd_glabel (l, bb, rtn)
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

int sd_cyl (l)
	struct disk_label *l;
{
	dsp->ds_maxcyl = l->dl_ncyl;
	return 0;
}

int sd_geterr()
{
	return 0;
}

int sd_req (int cmd, int start, char *buf, int count)
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
	
} /* sd_req() */


int sd_pstats()
{
	return 0;
}

int sd_ecc()
{
	return (0);
}

#define SD_FORMAT_SIZE_MAX	6000000

int sd_format(int fd, char *devname, int force) 
{
	int rtn;
	int data;
	char cmd_str[80];
	
	/*
	 * 'force' false means we're just doing an init; in that case, we only
	 * do a format for small removable disks.
	 */
	if(!force) {
		struct inquiry_reply ir;		
		u_int lastlba, blklen;
		int size, num;
		
		/*
		 * Removable?
		 */
		if(rtn = do_inquiry(fd, &ir))
			return(rtn);
		if(!ir.ir_removable || (ir.ir_devicetype != DEVTYPE_DISK)) 
			return(0);
		/*
		 * Small enough?
		 */
		if (ioctl(fd, DKIOCBLKSIZE, &size) < 0) {
			perror("ioctl(DKIOCBLKSIZE)");
			return(1);
		}
		if (ioctl(fd, DKIOCNUMBLKS, &num) < 0) {
			perror("ioctl(DKIOCNUMBLKS)");
			return(1);
		}
		blklen = size;
		lastlba = num - 1;
		if((blklen * (lastlba + 1)) > SD_FORMAT_SIZE_MAX)
			return(0);
	}
	/*
	 * Format the disk - use partition a
	 */
	sprintf(cmd_str, SDFORM_PATH " %sa n\n", devname);
	if(system(cmd_str)) 
		return(1);		/* bomb on error */
	/*
	 * Set volume to formatted state
	 */
	data = 1;
	ioctl(fd, DKIOCSFORMAT, &data);
	return(0);
}

