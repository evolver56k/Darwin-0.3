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
 *  fx - DiskObject-type Floppy exerciser
 *
 *	Linked with FloppyDisk and FloppyController objects.
 *
 */

#import <bsd/dev/FloppyDisk.h>
#import <driverkit/return.h>
#import <bsd/dev/FloppyTypes.h>
#import <bsd/dev/nrw/FloppyCnt.h>
#import <architecture/m88k/floppy_channel.h>
#import <bsd/sys/types.h>
#import <bsd/sys/param.h>
#import <bsd/dev/disk.h>
#import <bsd/dev/fd_extern.h>
#import <driverkit/align.h>
#import <driverkit/generalFuncs.h>
#import <bsd/libc.h>
#import <mach/mach.h>
#import "fd_lib.h"
#import "buflib.h"
#import <mach/mach_error.h>
#import <machdep/m88k/xpr.h>
#import <driverkit/volCheck.h>
#import <mach/cthreads.h>

#undef	FAKE_HARDWARE
#define FAKE_HARDWARE	1

void usage(char **argv);

#define MAX_IOSIZE	0x40

struct menu_entry {
	char 	menu_c;
	char	*name;
	void	(*men_fcn)();
};


void fx_read();
void fx_write();
void fx_slc();
void fx_sbyc();
void fx_sbn();
void fx_dump_rb();
void fx_zwb();
void fx_iwb();
void fx_compare();
void fx_readlp();
void print_menu();
void fx_eject();
void fx_motoroff();
void fx_quit();
void fx_seek();
void fx_readid();
void fx_strack();
void fx_recal();
void fx_setformat();
void fx_getformat();
void fx_format();
void fx_cwb();
void fx_sad();
void fx_dumpreg();
void fx_sboff();
int fx_setformatparms();
void fx_setretry();
void fx_getretry();
void fx_sblkccnt();

struct menu_entry me_array[] = {

    {'r', "Read               ", fx_read      }, {'i', "wbuf = Incrementing", fx_iwb       },
    {'w', "Write              ", fx_write     }, {'z', "Zero Write Buffer  ", fx_zwb       },
    {'L', "Read Loop          ", fx_readlp    }, {'1', "Set Data = sect #  ", fx_sad       },
    {'c', "Set Loop Count     ", fx_slc	      }, {'k', "Set Constant Data  ", fx_cwb       },
    {'y', "Set Byte Count     ", fx_sbyc      }, {'d', "Dump Read Buffer   ", fx_dump_rb   },
    {'b', "Set Block Number   ", fx_sbn       }, {'m', "Compare w/r Buffers", fx_compare   },
    {'t', "Set Track          ", fx_strack    }, {'S', "Set Format         ", fx_setformat },
    {'B', "Set Block Count    ", fx_sblkccnt  }, {'g', "Get Format         ", fx_getformat },
    {'C', "Recalibrate        ", fx_recal     }, {'F', "Format Track       ", fx_format    },
    {'o', "Motor Off          ", fx_motoroff  }, {'R', "Read ID            ", fx_readid    },
    {'e', "Eject Disk         ", fx_eject     }, {'D', "Dump Registers     ", fx_dumpreg   },
    {'f', "Set Buffer Offset  ", fx_sboff     }, {'2', "Get Retry Counts   ", fx_getretry  },
    {'s', "Seek               ", fx_seek      }, {'3', "Set Retry Counts   ", fx_setretry  },
    {'x', "Exit Program       ", fx_quit      }, {'h', "Print This menu    ", print_menu   }, 
    {0,    NULL,		       NULL	   },
     
}; /* me_array */

#define SIZEOF_REGS	10

#define SECTSIZE_DEF 	512
#define BUFSIZE		0x8000

id 			fd;
u_char			*wbuffer;
u_char			*rbuffer;
u_char			*wbuf;
u_char			*rbuf;
int 			rtn;
int			loop_count=1;
int			byte_count=SECTSIZE_DEF;
int 			block_count = 1;
int			block_num=0;
int 			loop_count;
int 			track;
struct fd_format_info 	format_info;
boolean_t		use_rawio=TRUE;
int			lblocksize;		/* logical block size */
u_int			fmt_gap_length = -1;
port_name_t		ownerPort;

int main(int argc, char *argv[]) {

	char 		c[80];
	struct		menu_entry *mep;
	char		ok, ch;
	int 		arg;
	id		cntrlId;
	port_name_t	dev_port;
	kern_return_t 	krtn;
	
	for(arg=1; arg<argc; arg++) {
		ch = argv[arg][0];
		switch(ch) {
		    default:
		    	ASSERT(0);
		    	usage(argv);
		}
	}

	/*
	 * Init libraries.
	 */
#ifdef	DDM_DEBUG
	IOInitDDM(1000, "FloppyXpr");
	/* 
	 * Maybe pass this in in argv...
	 */
	IOSetDDMMask(XPR_IODEVICE_INDEX,
		 XPR_FDD | XPR_FC | XPR_DEVICE | XPR_DISK | XPR_ERR | 
		 XPR_VC | XPR_NDMA);
#endif	DDM_DEBUG
	IOInitGeneralFuncs();
	volCheckInit();

	krtn = port_allocate(task_self(), &dev_port);
	if(krtn) {
		mach_error("port_allocate", krtn);
		exit(1);
	}
	cntrlId = [FloppyController probe:0 deviceMaster:PORT_NULL];
	if(cntrlId == nil) {
		printf("FloppyController probe: returned nil; exiting\n");
		exit(1);
	}
	fd = [FloppyDisk probe:cntrlId sender:nil];
	if(cntrlId == nil) {
		printf("FloppyDisk probe: returned nil; exiting\n");
		exit(1);
	}
	
	/*
	 * Set up for well-aligned transfers
	 */
	wbuffer = malloc(BUFSIZE + 2 * FLOPPY_BUFSIZE);
	rbuffer = malloc(BUFSIZE + 2 * FLOPPY_BUFSIZE);
	wbuf = IOAlign(u_char *, wbuffer, FLOPPY_BUFSIZE);
	rbuf = IOAlign(u_char *, rbuffer, FLOPPY_BUFSIZE);
	
	/*
	 * get the physical block size
	 */
	fx_setformatparms();
	print_menu();
	while(1) {
		printf("Enter Selection: ");
		gets(c);
		mep = me_array;
		ok = 0;
		while(mep->menu_c) {
			if(mep->menu_c == c[0]) {
				ok = 1;
				(*mep->men_fcn)();
				break;
			}
			else
				mep++;
		}
		if(!ok)
			printf("***Illegal Selection\n");
	}

} /* main() */

void usage(char **argv) {
	printf("usage: %s\n", argv[0]);
	exit(1);
}

void print_menu() {
	
	struct menu_entry *mep;
	
	printf("\n");
	mep = me_array;
	while(mep->menu_c) {
		printf(" %c: %s        ",mep->menu_c,mep->name);
		mep++;
		if(mep->menu_c) {
			printf(" %c: %s\n",mep->menu_c,mep->name);
			mep++;
		}
		else
			printf("\n\n");
	}

} /* print_menu() */


void fx_read() {

	fd_rw(fd,
		block_num,
		byte_count / lblocksize,
		rbuf,
		TRUE,				/* read */
		lblocksize,
		FALSE);				/* io_trace */
}

void fx_readlp() {

	/* 
	 * read block block_num, length = 1 sector, for loop_count loops 
	 */
	int loop;
	
	for(loop=0; loop<loop_count; loop++) {
		if(fd_rw(fd,
			block_num,
			byte_count / lblocksize,
			rbuf,
			TRUE,			/* read */
			lblocksize,
			FALSE))			/* io_trace */
				return;
	}
	return;
}

void fx_write() {

	fd_rw(fd,
		block_num,
		byte_count / lblocksize,
		wbuf,
		FALSE,				/* read */
		lblocksize,
		FALSE);				/* io_trace */
}


void fx_seek() {
	int rtn;
	int density;
	
	if(format_info.density_info.density == FD_DENS_NONE)
		density = FD_DENS_4;
	else
		density = format_info.density_info.density;
	rtn = seek_com(fd, track, &format_info, TRUE, density);
	if(rtn == FDR_SUCCESS)
		printf("...OK\n");
	return;
}

void fx_readid() {
	struct fd_ioreq ioreq;
	struct fd_readid_cmd *cmdp = (struct fd_readid_cmd *)ioreq.cmd_blk;
	struct fd_rw_stat *statp = (struct fd_rw_stat *)ioreq.stat_blk;
	int density;
	
	if(format_info.density_info.density == FD_DENS_NONE)
		density = FD_DENS_4;
	else
		density = format_info.density_info.density;
	
	bzero(&ioreq, sizeof(struct fd_ioreq));
	cmdp->opcode = FCCMD_READID;
	cmdp->mfm = 1;
	cmdp->hds = track & 1;		/* lsb of track */
	
	ioreq.timeout = 2000;
	ioreq.density = density;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = sizeof(struct fd_readid_cmd);
	ioreq.num_stat_bytes = SIZEOF_RW_STAT;	/* expect a fd_rw_stat */
	if(do_ioc(fd, &ioreq, TRUE))
		return;
	printf("\tcyl    = %d(d)\n", statp->cylinder);
	printf("\thead   = %d(d)\n", statp->head);
	printf("\tsector = %d(d)\n", statp->sector);
	return;
}

void fx_recal() {
	struct fd_ioreq ioreq;
	struct fd_seek_cmd *cmdp = (struct fd_seek_cmd *)ioreq.cmd_blk;
	int rtn;
	int density;
	
	if(format_info.density_info.density == FD_DENS_NONE)
		density = FD_DENS_4;
	else
		density = format_info.density_info.density;
	bzero(&ioreq, sizeof(struct fd_ioreq));
	cmdp->opcode = FCCMD_RECAL;
	ioreq.density = density;
	ioreq.timeout = 2000;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = sizeof(struct fd_recal_cmd);
	ioreq.addrs = 0;
	ioreq.byte_count = 0;
	ioreq.num_stat_bytes = sizeof(struct fd_int_stat);
	rtn = do_ioc(fd, &ioreq, TRUE);
	if(rtn == FDR_SUCCESS)
		printf("...OK\n");
	return;
}


void fx_slc() {

	while(1) {
		printf("Enter loop count (CR = %d(d)): ", loop_count);
		loop_count = get_num(loop_count, DEC);
		if(loop_count < 1)
			printf("Loop Count must be > 0\n");
		else 
			return;
	}
}

void fx_sbyc() {
	
	int sectsize;
	
	printf("Enter byte count (CR = 0x%x): ",byte_count);
	byte_count = get_num(byte_count, HEX);
	sectsize = format_info.sectsize_info.sect_size;
	if(sectsize == 0)
		sectsize = 512;
	block_count = byte_count / format_info.sectsize_info.sect_size;
}

void fx_sbn() {

	printf("Enter block number (CR = %d(d)): ",block_num);
	block_num = get_num(block_num, DEC);
}

void fx_sblkccnt()
{
	int sectsize;
	
	printf("Enter Block count (CR = 0x%x): ", block_count);
	block_count = get_num(byte_count, HEX);
	sectsize = format_info.sectsize_info.sect_size;
	if(sectsize == 0)
		sectsize = 512;
	byte_count = sectsize * block_count;

}

void fx_sboff() {
	char instr[80];
	int offset;
	
	while(1) {
		printf("Enter new offset (0..15): ");
		gets(instr);
		offset = atoi(instr);
		if((offset < 0) || (offset > 15)) 
			printf("try again.\n");
		else
			break;
	}
	rbuf = IOAlign(u_char *, rbuffer, FLOPPY_BUFSIZE) + offset;
	wbuf = IOAlign(u_char *, wbuffer, FLOPPY_BUFSIZE) + offset;
	printf("...rbuf = 0x%x   wbuf = 0x%x\n", rbuf, wbuf);

}
void fx_strack() {
	printf("Enter track (CR = %d(d)): ",track);
	track = get_num(track, DEC);
}

void fx_setretry() {
	int retry;
	IOReturn rtn;

	retry =  [fd innerRetry];
	printf("Inner Retry Count: (CR=%d(d)): ", retry);
	retry = get_num(retry, DEC);
	rtn = [fd fdSetInnerRetry:retry];
	if(rtn) {
		pr_iortn_text(fd, "setInnerRetry", rtn);
		return;
	}

	retry = [fd outerRetry];
	printf("Outer Retry Count: (CR=%d(d)): ", retry);
	retry = get_num(retry, DEC);
	rtn = [fd fdSetOuterRetry:retry];
	if(rtn) {
		pr_iortn_text(fd, "setOuterRetry", rtn);
		return;
	}
	return;
}

void fx_getretry() {
	int retry;
	
	retry = [fd innerRetry];
	printf("Inner Retry Count: %d\n", retry);
	retry = [fd outerRetry];
	printf("Outer Retry Count: %d\n", retry);
	return;
}

void dump_format_info(struct fd_format_info *fip)
{
	printf("\tmedia_id        %d\n",   fip->disk_info.media_id);
	printf("\tdensity         %d\n",   fip->density_info.density);
	printf("\tformatted       ");
	if(fip->flags & FFI_FORMATTED)
		printf("TRUE\n");
	else
		printf("FALSE\n");
	printf("\tlabel valid     ");
	if(fip->flags & FFI_LABELVALID)
		printf("TRUE\n");
	else
		printf("FALSE\n");
	printf("\twrite protect   ");
	if(fip->flags & FFI_WRITEPROTECT)
		printf("TRUE\n");
	else
		printf("FALSE\n");
	printf("\tsect_size       %xH\n",   fip->sectsize_info.sect_size);
	printf("\tsects_per_trk   %d(d)\n", fip->sectsize_info.sects_per_trk);
	printf("\ttracks_per_cyl  %d(d)\n", fip->disk_info.tracks_per_cyl);
	printf("\tnum_cylinders   %d(d)\n", fip->disk_info.num_cylinders);
	printf("\ttotal_sects     %d(d)\n", fip->total_sects);
	printf("\tmfm             ");
	if(fip->density_info.mfm)
		printf("TRUE\n");
	else
		printf("FALSE\n");
	printf("\trw gap length   %d(d)\n", 
		fip->sectsize_info.rw_gap_length);
	printf("\tfmt gap length  %d(d)\n", 
		fip->sectsize_info.fmt_gap_length);
}

void fx_getformat() {

	IOReturn rtn;
	
	rtn = [fd fdGetFormatInfo:&format_info];
	if(rtn) {
		pr_iortn_text(fd, "getFormatInfo", rtn);
		return;
	}
	printf("\nFormat information:\n");
	dump_format_info(&format_info);
	printf("\n");
}

void fx_setformat() {

	char instr[80];
	struct fd_format_info *fip = &format_info;
	struct fd_density_info *dip = &fip->density_info;
	IOReturn rtn;
	
	/*
	 * allow user to set density, sector size, gap length. We assume
	 * that out format_info has been obtained from the driver.
	 */
	printf("Enter density (CR = %d(d)): ", dip->density);
	dip->density = get_num(dip->density, DEC);
	printf("Enter sect_size (CR = %d(d)): ", fip->sectsize_info.sect_size);
	fip->sectsize_info.sect_size = 
		get_num(fip->sectsize_info.sect_size, DEC);

	/*
	 * Since the gap default gap lengths are really a function
	 * of both the density and the sector size, it's impossible
	 * to print out what the default is until both the density
	 * and sector size have been set.  So rather than print a
	 * value for the default, this just says "DEFAULT" and prints
	 * the new gap values after the density and sector size have
	 * been set.
	 */
	printf("Enter rw gap3 length (CR = %d(d)): ",
	    	fip->sectsize_info.rw_gap_length);
	fip->sectsize_info.rw_gap_length = 
		get_num(fip->sectsize_info.rw_gap_length, DEC);
	printf("Enter fmt gap3 length (CR = DEFAULT): ");
	fmt_gap_length = get_num(fmt_gap_length, DEC);
	printf("\n...Send To Driver (y/anything)? ");
	gets(instr);
	if(instr[0] != 'y') {
		return;
	}
	rtn = [fd fdSetDensity:dip->density];
	if(rtn) {
		pr_iortn_text(fd, "fdSetDensity", rtn);
		return;
	}
	rtn = [fd fdSetSectSize:fip->sectsize_info.sect_size];
	if(rtn) {
		pr_iortn_text(fd, "fdSetSectSize", rtn);
		return;
	}
	rtn = [fd fdSetGapLength:fip->sectsize_info.rw_gap_length];
	if(rtn) {
		pr_iortn_text(fd, "fdSetGapLength", rtn);
		return;
	}

	/*
	 * Now get remainder of physical parameters from driver.
	 */
	if(fx_setformatparms() == 0) {
		printf("...Format Parameters Stored by Driver\n");
		printf("Rw gap length  = %d(d)\n",
		    format_info.sectsize_info.rw_gap_length);
		printf("Fmt gap length = %d(d)\n", fmt_gap_length);
		return;
	}
}

void fx_format() {
	struct format_data *fdp, *fdp_align;
	int sector;
	struct fd_ioreq ioreq;
	struct fd_format_cmd *cmdp = (struct fd_format_cmd *)ioreq.cmd_blk;
	int data_size;
	int rtn=0;
	int fmt_cyl;
	int fmt_head;
	
	data_size = sizeof(struct format_data) *
	    format_info.sectsize_info.sects_per_trk;
	fdp = malloc(data_size + (2 * FLOPPY_BUFSIZE));
	if(fdp == 0) {
		printf("Couldn't malloc memory for format data\n");
		return;
	}
	fdp_align = IOAlign(struct format_data *, fdp, FLOPPY_BUFSIZE);
	fmt_cyl  = track / format_info.disk_info.tracks_per_cyl;
	fmt_head = track % format_info.disk_info.tracks_per_cyl;
	for(sector = 1;
	    sector <= format_info.sectsize_info.sects_per_trk;
	    sector++) {
		fdp_align->cylinder = fmt_cyl;
		fdp_align->head = fmt_head;
		fdp_align->sector = sector;
		fdp_align->n = format_info.sectsize_info.n;
		fdp_align++;
	}
	if(seek_com(fd, track, &format_info, TRUE,
	    format_info.density_info.density)) {
		printf("Seek Failed\n");
		free(fdp);
		return;
	}
	usleep(20000);		/* head settling time - 20 ms */
	
	/*
	 * Build a format command 
	 */
	bzero(&ioreq, sizeof (struct fd_ioreq));
	
	ioreq.density = format_info.density_info.density;
	ioreq.timeout = 5000;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = sizeof(struct fd_format_cmd);
	ioreq.addrs = (caddr_t)fdp_align;
	
	/*
	 * Note we can't do a DMA write using the actual byte count we want.
	 * Even worse, we won't really know how many bytes moved! Hardware
	 * feature...
	 */
	ioreq.byte_count = IOAlign(int, data_size, FLOPPY_BUFSIZE);
	ioreq.num_stat_bytes = SIZEOF_RW_STAT;
	ioreq.flags = FD_IOF_DMA_WR;
	
	cmdp->mfm = format_info.density_info.mfm;
	cmdp->opcode =FCCMD_FORMAT;
	cmdp->hds = fmt_head;
	cmdp->n = format_info.sectsize_info.n;
	cmdp->sects_per_trk = format_info.sectsize_info.sects_per_trk;
	cmdp->gap_length = fmt_gap_length;
	cmdp->filler_data = 0x5a;
	rtn = do_ioc(fd, &ioreq, TRUE);
	free(fdp);
	if(rtn == FDR_SUCCESS)
		printf("...OK\n");
}

void fx_dumpreg() {
	struct fd_ioreq ioreq;
	int rtn;
	int i;
	struct fd_82077_regs *regptr = (struct fd_82077_regs *)ioreq.stat_blk;
	int density;
	
	if(format_info.density_info.density == FD_DENS_NONE)
		density = FD_DENS_4;
	else
		density = format_info.density_info.density;
	bzero(&ioreq, sizeof(struct fd_ioreq));
	ioreq.cmd_blk[0] = FCCMD_DUMPREG;
	ioreq.density = density;
	ioreq.timeout = 2000;
	ioreq.command = FDCMD_CMD_XFR;
	ioreq.num_cmd_bytes = 1;
	ioreq.addrs = 0;
	ioreq.byte_count = 0;
	ioreq.num_stat_bytes = SIZEOF_REGS;
	rtn = do_ioc(fd, &ioreq, TRUE);
	if(rtn != FDR_SUCCESS)
		return;
	/*
	 * Print registers 
	 */
	printf("82077 Register Dump (all base 10):\n");
	for(i=0; i<4; i++)
		printf("   pcn%d   : %d\n", i, regptr->pcn[i]);
	printf("   srt    : %d\n", regptr->srt);
	printf("   hut    : %d\n", regptr->hut);
	printf("   hlt    : %d\n", regptr->hlt);
	printf("   nd     : %d\n", regptr->nd);
	printf("   sc_eot : %d\n", regptr->sc_eot);
	printf("   eis    : %d\n", regptr->eis);
	printf("   efifo  : %d\n", regptr->efifo);
	printf("   poll   : %d\n", regptr->poll);
	printf("   fifothr: %d\n", regptr->fifothr);
	printf("   pretrk : %d\n", regptr->pretrk);
	printf("\n");
}
void fx_dump_rb() {	
	printf("Read Buffer:");
	dump_buf(rbuf, BUFSIZE);
}


void fx_zwb() {

	register int i;
	register unsigned char *p;
	
	p = wbuf;
	for(i=0; i<BUFSIZE; i++)
		*p++ = 0;
	printf("...OK\n");
}

void fx_cwb() {

	register int i;
	register int *p;
	int dp=0;
	
	printf("Enter data pattern (CR=0x0): ");
	dp = get_num(0, HEX);
	p = (int *)wbuf;
	for(i=0; i<BUFSIZE/4; i++)
		*p++ = dp;
}

void fx_sad() {

	register int i;
	register int *p;
	int sect_num=0;
	int word_win_sect=0;
	int words_per_sect = format_info.sectsize_info.sect_size / 4;
	
	printf("Enter starting sector # (CR=0x0): ");
	sect_num = get_num(0, HEX);
	p = (int *)wbuf;
	for(i=0; i<BUFSIZE/4; i++) {
		*p++ = sect_num;
		if(++word_win_sect == words_per_sect) {
			sect_num++;
			word_win_sect = 0;
		}
	}
}


void fx_iwb() {

	register int i;
	register unsigned char *p;
	
	p = wbuf;
	for(i=0; i<BUFSIZE; i++)
		*p++ = (char)(i & 0xFF);
	printf("...OK\n");
}

void fx_compare() {
	buf_comp(byte_count, wbuf, rbuf);
}

void fx_eject() 
{
 	IOReturn rtn;
	
	rtn = [fd ejectPhysical];
	if(rtn) 
		pr_iortn_text(fd, "ejectPhysical", rtn);
	else
		printf("...OK\n");
	return;
}

void fx_motoroff() {

	struct fd_ioreq ioreq;
	int density;
	
	if(format_info.density_info.density == FD_DENS_NONE)
		density = FD_DENS_4;
	else
		density = format_info.density_info.density;	
	bzero(&ioreq, sizeof (struct fd_ioreq));
	ioreq.density = density;
	ioreq.timeout = 1000;
	ioreq.command = FDCMD_MOTOR_OFF;
	ioreq.byte_count = 0;
	if(!do_ioc(fd, &ioreq, FALSE))
		printf("...OK\n");
	return;
}

void fx_quit() {
#ifdef	notdef
	if (close(fd) < 0) {
		perror("close");
		exit(1);
	}
#endif	notdef
	exit(0);
}

int fx_setformatparms() {

	IOReturn rtn;
	
	rtn = [fd fdGetFormatInfo:&format_info];
	if(rtn) {
		pr_iortn_text(fd, "fdGetFormatInfo", rtn);
		return(1);
	}
	lblocksize = format_info.sectsize_info.sect_size;
	byte_count = lblocksize;
	fmt_gap_length = format_info.sectsize_info.fmt_gap_length;
	return(0);
}

/* end of fx.m */
