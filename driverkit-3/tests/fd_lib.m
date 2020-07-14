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
 * fd_lib.m - library routines for floppy tests
 */
 
#import <bsd/fcntl.h>
#import <ansi/stdio.h>
#import <bsd/sys/types.h>
#import <bsd/sys/param.h>
#import <mach/boolean.h>
#import <bsd/dev/fd_extern.h>
#import <bsd/libc.h>
#import <objc/Object.h>
#import "fd_lib.h"
#import <bsd/dev/disk.h>
#import <bsd/dev/FloppyDisk.h>
#import <bsd/dev/FloppyCntPublic.h>
#import <driverkit/IODiskPartition.h>
#import <mach/cthreads.h>

/*
 * Execute a command specified in fdIoReq.
 */
int do_ioc(id fd, 
	fdIoReq_t *fdIoReq, 
	int verbose)
{	
	IOReturn rtn;
	
	fdIoReq->status = FDR_SUCCESS;
	rtn = [fd fdCmdXfr:fdIoReq];
	if(rtn) {
		pr_iortn_text(fd, "fdCmdXfr", rtn);
		rtn = 1;
		goto check_status;
	}
	if(fdIoReq->num_cmd_bytes != fdIoReq->cmd_bytes_xfr) {
		printf("Expected cmd byte count = 0x%x\n",
		 	fdIoReq->num_cmd_bytes);
		printf("received cmd byte count = 0x%x\n",
			fdIoReq->cmd_bytes_xfr);
		rtn = 1;
		goto check_status;
	}
	if(fdIoReq->num_stat_bytes != fdIoReq->stat_bytes_xfr) {
		printf("Expected cmd byte count = 0x%x\n", 
			fdIoReq->num_stat_bytes);
		printf("received cmd byte count = 0x%x\n", 
			fdIoReq->stat_bytes_xfr);
		rtn = 1;
		goto check_status;
	}
	if(fdIoReq->byte_count != fdIoReq->bytes_xfr) {
		printf("Expected byte count = 0x%x\n", fdIoReq->byte_count);
		printf("received byte count = 0x%x\n", fdIoReq->bytes_xfr);
		rtn = 1;
		goto check_status;
	}
	else if(fdIoReq->bytes_xfr && verbose)
		printf("0x%x bytes transferred\n",fdIoReq->bytes_xfr);
check_status:
	if(fdIoReq->status != FDR_SUCCESS) {
		rtn = 1;
		if(verbose) 
			pr_fdstat_text(fd, "fdCmdXfr", fdIoReq->status);
	}
	return(rtn);
}

int seek_com(id fd, 
	int track, 
	struct fd_format_info *finfop, 
	int verbose,
	int density)
{
	fdIoReq_t ioreq;
	struct fd_seek_cmd *cmdp = (struct fd_seek_cmd *)ioreq.cmd_blk;
	
	bzero(&ioreq, sizeof(fdIoReq_t));
	cmdp->opcode = FCCMD_SEEK;
	cmdp->hds = track % finfop->disk_info.tracks_per_cyl;
	cmdp->cyl = track / finfop->disk_info.tracks_per_cyl;
	ioreq.timeout = 2000;
	ioreq.density = density;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = SIZEOF_SEEK_CMD;
	ioreq.num_stat_bytes = sizeof(struct fd_int_stat);
	if(do_ioc(fd, &ioreq, verbose))
		return(1);
	return(0);
}

void pr_fdstat_text(id controller, char *op, int io_stat)
{
	printf("%s: fd I/O status = %s\n", 
		op, IOFindNameForValue(io_stat, fdrValues));
}

void pr_iortn_text(id controller, char *op, IOReturn rtn)
{
	printf("%s: ioReturn status = %s\n", 
		op, [controller stringFromReturn:rtn]);
}


int fd_rw(id fd,
	int block,
	int block_count,
	u_char *addrs,
	boolean_t read_flag,
	int lblocksize,
	boolean_t io_trace)
{
	IOReturn rtn;
	u_int length = lblocksize * block_count;
	u_int actualLength;
	char *read_str;
	
	read_str = read_flag ? "read " : "write";
	if(io_trace)
		printf("......%s(%d @ %d)\n", read_str, block_count, block);
	if(read_flag) {
		rtn = [fd readAt:block
			length:length
			buffer:addrs
			actualLength:&actualLength];
	}
	else {
		rtn = [fd writeAt:block
			length:length
			buffer:addrs
			actualLength:&actualLength];
	}
	if(rtn) {
		pr_iortn_text(fd, read_str, rtn);
		return(1);
	}			
	if(actualLength != length) {
		printf("...%s(0x%x) transferred 0x%x bytes, expected 0x%x\n", 
			read_str, actualLength, length);
		return(1);
	}
	return(0);
} /* fd_rw() */

#ifdef	notdef
int check_mnt_ent(char *device)
{
	/*
	 * device is of the form "sd1", "fd0", etc. Returns 1 if an partition
	 * on device is currently mounted, else returns 0.
	 */
	FILE *mounted;
	struct mntent *mnt;
	char block_device_name[40];
	char raw_device_name[40];
	
	sprintf(block_device_name, "%/dev/%s", device);
	sprintf(raw_device_name, "%/dev/r%s", device);
	mounted = setmntent(MOUNTED, "r");
	if (mounted == NULL) {
		perror("setmntent");
		return(1);
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		if(strncmp(raw_device_name, mnt->mnt_fsname,
		    strlen(raw_device_name)) == 0)
		    	return(1);
		if(strncmp(block_device_name, mnt->mnt_fsname,
		    strlen(block_device_name)) == 0)
		    	return(1);
	}
	endmntent (mounted);
	return(0);
}
#endif	notdef

int get_drive_params(id fd, 
	int *end_sect, 
	int *sectsize,
	int *blocks_per_xfr,
	boolean_t read_only)
{
	struct 	disk_label label;
	int	label_valid = 0;
	IOReturn rtn;
	struct 	fd_format_info 	format_info;
	char 	c[80];
	id	logicalId;
	
	/*
	 * See if there is an IODiskPartition attached, and if it has a valid label.
	 */
	logicalId = [fd logicalDisk];
	if(logicalId != nil) {
		rtn = [fd readLabel:&label];
		if(rtn == IO_R_SUCCESS) {
			if(!read_only)
			{
				printf("Disk Has Valid Label. Continue "
					"(Y/anything)? ");
				gets(c);
				if(c[0] != 'Y')
					return(1);
			}
			label_valid = 1;
		}
	}

	/*
	 * get physical device info
	 */
	rtn = [fd fdGetFormatInfo:&format_info];
	if(rtn) {
		pr_iortn_text(fd, "getFormatInfo", rtn);
		return(1);
	}
	if(!(format_info.flags & FFI_FORMATTED)) {
		printf("Disk Not Formatted; aborting.\n");
		return(1);
	}
	if(label_valid && !read_only) {
		/*
		 * Mark label invalid. We're about to blow it away.
		 */
		rtn = [fd fdSetSectSize:format_info.sectsize_info.sect_size];
		if(rtn) {
			pr_iortn_text(fd, "setSectSize", rtn);
			return(1);
		}
	}
	*sectsize = format_info.sectsize_info.sect_size;
	if(*end_sect == 0)
		*end_sect = format_info.total_sects - 1;
	else if(*end_sect > format_info.total_sects - 1) {
		printf("Disk capacity %d sectors; end_sect %d "
			"specified. ABORTING.\n", 
			format_info.total_sects, end_sect);
		return(1);
	}
	if(*blocks_per_xfr == 0) {
		*blocks_per_xfr = 
			format_info.sectsize_info.sects_per_trk;
	}
	return(0);
}
