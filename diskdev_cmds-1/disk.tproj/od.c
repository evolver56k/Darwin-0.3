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
/*	@(#)od.c	1.0	08/28/87	(c) 1987 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added exec_time mechanism for scsi_req support.
 *
 **********************************************************************
 */
#ifdef m68k


#include <sys/types.h>
#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include <sys/ioctl.h>
#include <bsd/dev/disk.h>
#include <bsd/dev/m68k/odvar.h>
#include "disk.h"
#include <errno.h>
#import <libc.h>
#import <stdio.h>

int od_conf()
{
	return 0;
}

int od_init()
{
	register int size;
	register struct disk_label *l = &disk_label;

	/* reset the bitmap to untested for every half track */
	if (bit_map)
		free (bit_map);
	if (l->dl_version == DL_V1)
		size = (d_size / l->dl_nsect) >> 1;
	else
		size = d_size >> 2;
	bit_map = (int*) malloc (size);
	bzero (bit_map, size);
	write_bitmap();
	return 0;
}

int od_wlabel (bb)
	struct bad_block *bb;
{
	register struct disk_label *l = &disk_label;

	if (ioctl (fd, DKIOCSLABEL, l) < 0)
		dpanic (S_MEDIA, "can't write label -- disk unusable!");
	if (bb && ioctl (fd, DKIOCSBBT, bb) < 0)
		dpanic (S_MEDIA, "can't write bad block table!");
	return 0;
}

int od_glabel (l, bb, rtn)
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
	if (bb && ioctl (fd, DKIOCGBBT, bb) < 0) {
		if (rtn)
			return (-1);
		dpanic (S_NEVER, "get bbt");
	}
	return (0);
}

int od_geterr()
{
	register struct dr_errmap *de = (struct dr_errmap*) &req.dr_errblk;

	if (de->de_err == E_ECC)
		return (ERR_ECC);
	return (ERR_UNKNOWN);
}

int od_cyl (l)
	struct disk_label *l;
{
	/* undo pseudo-cylinders (-10 to stay away from bitmap) */
	dsp->ds_maxcyl = l->dl_ncyl * l->dl_ntrack - 10;
	return 0;
}

int od_req (int cmd, int p1, int p2, int p3, int p4, int p5, int p6)
{
	register struct disk_req *dr = &req;
	register struct dr_cmdmap *dc = (struct dr_cmdmap*) dr->dr_cmdblk;
	int rtn;
	
	bzero (dr, sizeof (*dr));
	bzero (dc, sizeof (*dc));

	switch (cmd) {
		case CMD_SEEK:
			dc->dc_cmd = OMD_SEEK;
			dc->dc_blkno = p1;
			dc->dc_wait = p2;
			break;

		case CMD_READ:
			dc->dc_cmd = OMD_READ;
			goto rwv;

		case CMD_WRITE:
			dc->dc_cmd = OMD_WRITE;
			goto rwv;

		case CMD_ERASE:
			dc->dc_cmd = OMD_ERASE;
			goto rwv;

		case CMD_VERIFY:
			dc->dc_cmd = OMD_VERIFY;
rwv:
			dc->dc_blkno = (p1 * dt->d_secsize) / devblklen;
			dr->dr_addr = (caddr_t) p2;
			dr->dr_bcount = p3;
			if (p4 == SPEC_RETRY) {
				dc->dc_flags = DRF_SPEC_RETRY;
				dc->dc_retry = p5;
				dc->dc_rtz = p6;
			}
			break;

		case CMD_EJECT:
			dc->dc_cmd = OMD_EJECT;
			break;

		case CMD_RESPIN:
			dc->dc_cmd = OMD_RESPIN;
			break;
	}
	rtn = ioctl (fd, DKIOCREQ, dr);
	exec_time = dr->dr_exec_time;
	return(rtn);
}

int od_pstats() {
	register struct od_stats *s = (struct od_stats*) stats.s_stats;

	printf ("\t%7d verify retries\n", s->s_vfy_retry);
	printf ("\t%7d verify failures\n", s->s_vfy_failed);
	return 0;
}

int od_ecc()
{
	register struct dr_errmap *de = (struct dr_errmap*) &req.dr_errblk;

	return (de->de_ecc_count);
}

int od_format(int fd, char *devname, int force)
{
	if(force) {
		printf("disk: can not format optical disk!\n");
		return(EINVAL);
	}
	else
		return(0);	/* always formatted */
}

#endif m68k
