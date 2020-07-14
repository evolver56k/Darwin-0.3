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
/* fdform - Floppy format utility
 *
 *	usage:   fdform device [b=blocksize] [d=density] [g=gap3_length]
 *
 *		blocksize must be 512 or 1024.
 *		density is in Kbytes. Current legal values are 720, 1440,
 *			and 2880. Default is max density allowable for media.
 *
 */

#import <fcntl.h>
#import <stdio.h>
#import <unistd.h>

#ifdef	m68k
#include <kern/lock.h>
#endif	m68k

#import <sys/types.h>
#import <mach/mach.h>
#import <bsd/dev/scsireg.h>
/*
 * m68k is worst case as far as alignment restrictions...
 */
#ifdef	m68k
#import <bsd/dev/m68k/dma.h>
#endif	m68k
#import <sys/param.h>
#import <signal.h>
#import <bsd/dev/fd_extern.h>
#import <libc.h>
#import <stdlib.h>
#import <sys/file.h>

void usage(char **argv);
int format_disk(int fd, int live_fd, struct fd_format_info *finfop);
int format_track(int fd, int cylinder, int head, 
	struct fd_format_info *finfop);
int recalibrate(int fd, struct fd_format_info *fip);
int seek_com(int fd, int track, struct fd_format_info *finfop, int density);
int do_ioc(int fd, fd_ioreq_t fdiop);
int fd_rw(int fd,
	int block,
	int block_count,
	u_char *addrs,
	boolean_t read_flag,
	struct fd_format_info *finfop);
void format_abort();

#ifndef	TRUE
#define TRUE	(1)
#endif	TRUE
#ifndef	FALSE
#define FALSE	(0)
#endif	FALSE

u_char 			*vfy_buf;
struct format_data 	*fdp;
int			cyls_to_format = 0;
int 			fmt_gap3_length = -1;
int 			fd;
int			live_fd;
struct fd_format_info 	format_info;

int
main(int argc, char *argv[]) {
	int 			arg;
	char 			ch;
	int 			rtn;
	kern_return_t 		krtn;
	int			format_data_size;
	struct fd_sectsize_info *ssip;
	int 			density = FD_DENS_NONE;
	int			blocksize = 512;
	
	if(argc<2) 
		usage(argv);
	fd = open(argv[1], O_RDWR, 0);
	if(fd <= 0) {
		printf("Opening %s:\n", argv[1]);
		perror("open");
		exit(1);
	}
	
	/*
	 * Open the live partition for read/verify.
	 */
	argv[1][strlen(argv[1]) - 1] = 'b';
	live_fd = open(argv[1], O_RDONLY, 0);
	if(live_fd <= 0) {
		printf("Opening %s:\n", argv[1]);
		perror("open");
		exit(1);
	}
	for(arg=2; arg<argc; arg++) {
		ch = argv[arg][0];
		switch(ch) {
		    case 'd':
		    	density = atoi(&argv[arg][2]);
			switch(density) {
			    case 720:
			    	density = FD_DENS_1;
				break;
			    case 1440:
			    	density = FD_DENS_2;
				break;
			    case 2880:
			    	density = FD_DENS_4;
				break;
			    default:
			    	usage(argv);
			}
			break;
		    case 'b':
		    	blocksize = atoi(&argv[arg][2]);
			if((blocksize != 512) && (blocksize != 1024))
				usage(argv);
			break;
			
		    case 'g':
		    	fmt_gap3_length = atoi(&argv[arg][2]);
			break;
			
		    case 'n':
		    	cyls_to_format = atoi(&argv[arg][2]);
			break;
			
		    default:
		    	usage(argv);
		}
	}
	/*
	 * find out what kind of disk is installed, then ensure that we
	 * haven't been asked to format a bogus density for this disk.
	 */
	if(ioctl(fd, FDIOCGFORM, &format_info)) {
		perror("ioctl(FDIOCGFORM)");
		return(1);
	}
	if(density > format_info.disk_info.max_density) {
		printf("\nMaximum Legal Density for this disk is ");
		switch(format_info.disk_info.max_density) {
		    case FD_DENS_1:
		    	printf("1 (720 KBytes formatted)\n");
			exit(1);
		    case FD_DENS_2:
		    	printf("2 (1.44 MByte formatted)\n");
			exit(1);
		    case FD_DENS_4:
		    	printf("4 (2.88 MByte formatted)\n");
			exit(1);
		}
	}
	if(density == FD_DENS_NONE) 
		density = format_info.disk_info.max_density;

	printf("Formatting disk %s: \n", argv[1]);
	printf("    blocksize   = 0x%x\n", blocksize);
	printf("    density     = ");
	switch(density) {
	    case FD_DENS_1:
	    	printf("720 KBytes\n");
		break;
	    case FD_DENS_2:
		printf("1.44 MByte\n");
		break;
	    case FD_DENS_4:
		printf("2.88 MByte\n");
		break;
	}	
	/*
	 * If user hasn't specified gap length, use default provided by driver
	 */
	if(fmt_gap3_length < 0)
		fmt_gap3_length = format_info.sectsize_info.fmt_gap_length;
	printf("    gap3 length = %d(d)\n", fmt_gap3_length);

	/*
	 * Generate a new format_info. Have the driver calculate physical 
	 * parameters based on sector size and density If we abort the 
	 * format for any reason, we'll mark the disk unformatted.
	 */
	if(ioctl(fd, FDIOCSDENS, &density)) {
		perror("ioctl(FDIOCSDENS)");
		return(1);
	}
	if(ioctl(fd, FDIOCSSIZE, &blocksize)) {
		perror("ioctl(FDIOCSSIZE)");
		return(1);
	}

	/*
	 * This returns all the current parameters, based on what we just
	 * told the driver.
	 */
	if(ioctl(fd, FDIOCGFORM, &format_info)) {
		perror("ioctl(FDIOCGFORM)");
		format_abort();
	}
	
	/*
	 * Get page-aligned buffers for all the reading and writing we'll 
	 * be doing.
	 */
	ssip = &format_info.sectsize_info;
	krtn = vm_allocate(task_self(), 
			(vm_address_t *)&vfy_buf,
			ssip->sects_per_trk * ssip->sect_size,
			TRUE);
	if(krtn) {
		printf("\n...Couldn't allocate track buffer\n");
		format_abort();
	}
	format_data_size = sizeof(struct format_data) * ssip->sects_per_trk;
	krtn = vm_allocate(task_self(), 
			(vm_address_t *)&fdp,
			format_data_size,
			TRUE);
	if(krtn) {
		printf("\n...Couldn't allocate Format data buffer\n");
		format_abort();
	}

	signal(SIGINT, format_abort);
	if(rtn = format_disk(fd, live_fd, &format_info))
		format_abort();
	
	printf("\n..Format Complete\n");
	exit(0);
}

void usage(char **argv) {
	printf("usage: %s device [b=blocksize)] [d=density] [g=gap3_length] [n=num_cylinders]\n", argv[0]);
	printf("       blocksize = 512 or 1024\n");
	printf("       density = 720, 1440, 2880\n");
	exit(1);
}

void format_abort()
{
	int arg;
	
	/*
	 * ctl C or error abort; mark disk as unformatted.
	 */
	arg = FD_DENS_NONE;
	if(ioctl(fd, FDIOCSDENS, &arg)) {	/* unformatted */
		perror("ioctl(FDIOCSDENS)");
		exit(1);
	}
	printf("\n..Format Aborted\n");
	exit(1);

}
int format_disk(int fd, int live_fd, struct fd_format_info *finfop)
{
	int rtn;
	int cylinder;
	int head;
	int vfy_block=0;
	struct fd_sectsize_info *ssip = &finfop->sectsize_info;
	
	if(cyls_to_format == 0)
		cyls_to_format = format_info.disk_info.num_cylinders;
		
	if(rtn = recalibrate(fd, &format_info)) {
		return(1);
	}
	for(cylinder=0; cylinder<cyls_to_format; cylinder++) {
		for(head=0; 
		    head<format_info.disk_info.tracks_per_cyl; 
		    head++) {
			if(rtn = format_track(fd, cylinder, head, finfop)) {
				printf("\n...Format track FAILED\n");
				printf("  cyl %d  head %d\n", cylinder, head);
				return(1);
			}
			/*
			 * Read the whole track. No data verify; just rely on
			 * CRC.
			 */
			if(fd_rw(live_fd,
			    vfy_block,
			    ssip->sects_per_trk,
			    vfy_buf,
			    TRUE,
			    finfop))
				return(1);
			vfy_block += ssip->sects_per_trk;
		}
		printf(".");   
	        fflush(stdout);
	}
	return(0);
}
 
/*
 * Format one track.
 */
int format_track(int fd, int cylinder, int head, struct fd_format_info *finfop)
{
	struct format_data *fdp_work;
	int sector;
	struct fd_ioreq ioreq;
	struct fd_format_cmd *cmdp = (struct fd_format_cmd *)ioreq.cmd_blk;
	int data_size;
	int rtn=0;
	struct fd_sectsize_info *ssip = &finfop->sectsize_info;

	data_size = sizeof(struct format_data) * ssip->sects_per_trk;

	/*
	 * Generate the data we'll DMA during the format track command.
	 * This consists if the headers for each sector on the cylinder.
	 */
	fdp_work = fdp;
	for(sector=1; sector<=ssip->sects_per_trk; sector++) {
		fdp_work->cylinder = cylinder;
		fdp_work->head = head;
		fdp_work->sector = sector;
		fdp_work->n = ssip->n;
		fdp_work++;
	}
	if(seek_com(fd, cylinder * finfop->disk_info.tracks_per_cyl + head, 
	    finfop, finfop->density_info.density)) {
		return(1);
	}
	usleep(20000);		/* head settling time - 20 ms (fixme) */
	
	/*
	 * Build a format command 
	 */
	bzero(&ioreq, sizeof (struct fd_ioreq));
	
	ioreq.density = finfop->density_info.density;
	ioreq.timeout = 5000;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = sizeof(struct fd_format_cmd);
	ioreq.addrs = (caddr_t)fdp;
#if	m68k
	ioreq.byte_count = DMA_ENDALIGN(int, data_size);
#else	m68k
	ioreq.byte_count = data_size;
#endif	m68k
	ioreq.num_stat_bytes = SIZEOF_RW_STAT;
	ioreq.flags = FD_IOF_DMA_WR;
	
	cmdp->mfm           = finfop->density_info.mfm;
	cmdp->opcode        = FCCMD_FORMAT;
	cmdp->hds           = head;
	cmdp->n             = ssip->n;
	cmdp->sects_per_trk = ssip->sects_per_trk;
	cmdp->gap_length    = fmt_gap3_length;
	cmdp->filler_data   = 0x5a;
	rtn = do_ioc(fd, &ioreq);
	if(rtn) {
		printf("\n...Format (cylinder %d head %d) Failed\n", 
			cylinder, head);
	}
	return(rtn);
} /* format_track() */

int recalibrate(int fd, struct fd_format_info *fip) {
	struct fd_ioreq ioreq;
	struct fd_seek_cmd *cmdp = (struct fd_seek_cmd *)ioreq.cmd_blk;
	int rtn=0;
	
	bzero(&ioreq, sizeof(struct fd_ioreq));
	cmdp->opcode = FCCMD_RECAL;
	ioreq.density = fip->density_info.density;
	ioreq.timeout = 2000;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = sizeof(struct fd_recal_cmd);
	ioreq.addrs = 0;
	ioreq.byte_count = 0;
	ioreq.num_stat_bytes = sizeof(struct fd_int_stat);
	rtn = do_ioc(fd, &ioreq);
	if(rtn) {
		printf("\n...Recalibrate Failed\n");
	}
	return(rtn);
}

int do_ioc(int fd, fd_ioreq_t fdiop)
{	
	int rtn=0;
	
	fdiop->status = FDR_SUCCESS;
	if (ioctl(fd, FDIOCREQ, fdiop) < 0) {
		perror("ioctl(FDIOCREQ)");
		rtn = 1;
		goto check_status;
	}
	if(fdiop->num_cmd_bytes != fdiop->cmd_bytes_xfr) {
		printf("\n...Expected cmd byte count = 0x%x\n",
		 	fdiop->num_cmd_bytes);
		printf("   received cmd byte count = 0x%x\n",
			fdiop->cmd_bytes_xfr);
		rtn = 1;
		goto check_status;
	}
	if(fdiop->num_stat_bytes != fdiop->stat_bytes_xfr) {
		printf("\n...Expected cmd byte count = 0x%x\n", 
			fdiop->num_stat_bytes);
		printf("   received cmd byte count = 0x%x\n", 
			fdiop->stat_bytes_xfr);
		rtn = 1;
		goto check_status;
	}
	if(fdiop->byte_count != fdiop->bytes_xfr) {
		printf("\n...Expected byte count = 0x%x\n", fdiop->byte_count);
		printf("   received byte count = 0x%x\n", fdiop->bytes_xfr);
		rtn = 1;
		goto check_status;
	}
check_status:
	if(fdiop->status != FDR_SUCCESS) {
		rtn = 1;
		printf("\n...Unexpected status: %x\n", fdiop->status);
	}
	return(rtn);
}

int seek_com(int fd, int track, struct fd_format_info *finfop, int density)
{
	struct fd_ioreq ioreq;
	struct fd_seek_cmd *cmdp = (struct fd_seek_cmd *)ioreq.cmd_blk;
	int rtn = 0;
	
	bzero(&ioreq, sizeof(struct fd_ioreq));
	cmdp->opcode = FCCMD_SEEK;
	cmdp->hds = track % finfop->disk_info.tracks_per_cyl;
	cmdp->cyl = track / finfop->disk_info.tracks_per_cyl;
	ioreq.timeout = 2000;
	ioreq.density = density;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = SIZEOF_SEEK_CMD;
	ioreq.num_stat_bytes = sizeof(struct fd_int_stat);
	rtn = do_ioc(fd, &ioreq);
	if(rtn) {
		printf("\n...Seek (track %d) failed\n", track);
	}
	return(rtn);
}

#define	USE_LIVE_IO	1

#if	USE_LIVE_IO

int fd_rw(int fd,
	int block,
	int block_count,
	u_char *addrs,
	boolean_t read_flag,
	struct fd_format_info *finfop)
{
	int 		rtn;
	char 		*read_str;
	int 		byte_count;
	int		offset;
		
	read_str = read_flag ? "read " : "write";
	offset = block * finfop->sectsize_info.sect_size;
	byte_count = block_count * finfop->sectsize_info.sect_size;
	rtn = lseek(fd, offset, L_SET);
	if(rtn != offset) {
		printf("Live Partition Seek Seek error on %s\n", read_str);
		return(1);
	}
	if(read_flag) {
		rtn = read(fd, addrs, byte_count);
	}
	else {
		rtn = write(fd, addrs, byte_count);
	}
	if(rtn != byte_count) {
		printf("\n");
		if(rtn <= 0) {
			perror(read_str);
		}
		else {
			printf("Short %s (exp %d, recd %d)\n",
				read_str, byte_count, rtn);
		}
		printf("block %d block_count %d\n", block, block_count);
		return(1);
	}
	return(0);
} /* fd_rw() */


#else	USE_LIVE_IO

int fd_rw(int fd,
	int block,
	int block_count,
	u_char *addrs,
	boolean_t read_flag,
	struct fd_format_info *finfop)
{
	int rtn;
	char *read_str;
	int byte_count;
	struct fd_rawio rawio;
	
	read_str = read_flag ? "read " : "write";
		
	rawio.sector = block;
	rawio.sector_count = block_count;
	rawio.dma_addrs = (caddr_t)addrs;
	rawio.read = read_flag;
	rawio.sects_xfr = rawio.status = -1;
	
	rtn = ioctl(fd, FDIOCRRW, &rawio);
	if(rtn) {
		if(read_flag)
			perror("ioctl(FDIOCRW, read)");
		else
			perror("ioctl(FDIOCRW, write)");
		return(1);
			
	}
	if(rawio.status != FDR_SUCCESS) {
		printf("\n...%s: rawio.status = %d(d)\n",
			read_str, rawio.status);
		return(1);
	}
	if(rawio.sects_xfr != block_count) {
		printf("\n...ioctl(FDIOCRW, %s) moved %d(d) blocks, "
			"expected %d(d) blocks\n", 
			read_str, rawio.sects_xfr, block_count);
		return(1);
	}
	return(0);
} /* fd_rw() */

#endif	USE_LIVE_IO





