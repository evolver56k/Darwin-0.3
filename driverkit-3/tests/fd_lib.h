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
 * Floppy test library interface.
 */
#import <objc/Object.h>
#import <driverkit/return.h>
#import <bsd/dev/FloppyDisk.h>

#ifndef	TRUE
#define TRUE		(1)
#endif	TRUE
#ifndef	FALSE
#define FALSE		(0)
#endif	FALSE
#define FPORCH		96		/* should get from label.. */
#define BPORCH		0
#ifndef	DEV_BSIZE
#define DEV_BSIZE	1024
#endif	DEV_BSIZE

#define SECTS_PER_TRACK_720	9
#define SECTS_PER_TRACK_1440	18
#define SECTS_PER_TRACK_2880	36

int do_ioc(id fd, fdIoReq_t *fdIoReq, int verbose);
int seek_com(id fd, 
	int track, 
	struct fd_format_info *finfop, 
	int verbose,
	int density);
void pr_fdstat_text(id controller, char *op, int io_stat);
void pr_iortn_text(id controller, char *op, IOReturn rtn);
int fd_rw(id fd,
	int block,
	int block_count,
	u_char *addrs,
	boolean_t read,
	int lblocksize,
	boolean_t io_trace);
int check_mnt_ent(char *device);
int get_drive_params(id fd, 
	int *end_sect, 
	int *sectsize,
	int *blocks_per_xfr,
	boolean_t read_only);




