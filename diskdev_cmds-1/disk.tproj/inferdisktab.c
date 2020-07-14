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
/*	inferdisktab.c
 *
 * History
 * -------
 * 5-Sep-97	Dieter Siegmund at Apple
 *  Bump the size of the max partition size to 1024G from 2G,
 *  and treat all SCSI media as dumb hard disks if we can't get
 *  mode sense data.  This allows us to init Jazz drives
 *  and gives a more reasonable layout for Zip as well.
 * 13-Aug-97 Martin Minow at Apple
 *  Zero out the mode sense reply buffer to prevent returning
 *  unknown garbage if the SCSI device rejects the (optional)
 *  mode sense command.
 * 4-Aug-92 Matt Watson at NeXT
 *	Added 486 specifics
 * 16-Sep-91	Garth Snyder at NeXT
 *	Added support for variable blocksize filesystems
 * 02-Feb-90	Mike DeMoney at NeXT
 *	Created
 */
 
#import "disk.h"
#import <libc.h>
#import <bsd/dev/i386/disk.h>
#import <unistd.h>
#include <sys/param.h>
#include <sys/file.h>
#include <bsd/dev/disk.h>
#include <bsd/dev/scsireg.h>
#include <bsd/dev/fd_extern.h>

#ifndef	BFD_PART_TYPE
#define BFD_PART_TYPE	"bfd"		/* from <mon> eventually */
#endif	BFD_PART_TYPE
#ifndef DEVTYPE_RWOPTICAL
#define	DEVTYPE_RWOPTICAL	0x07
#endif

#define	FPORCH_HARD	160		/* front porch size in K */
#define FPORCH_FLOPPY	96		/* floppy only has one boot block */
#define	BPORCH		0

#define RPM_HARD	3600
#define RPM_FLOPPY	300

#define	CYLPGRP		16
#define	BYTPERINO	4096
#define BYTPERINO_FLPY	2048		/* Need more inodes 'cuz they small */
#define DEV_BSIZE	1024

/* MAX_PARTITION_SIZE: value expressed in terms of 1K blocks */
#define MAX_PARTITION_SIZE     		1073741824ULL  	/* 1024 Gig */

#define SCSI_TIMEOUT	15		/* command timeout in seconds */

/*
 * Disk capacities above MINFREE_THRESH use MINFREE_LARGE,
 * capacities under MINFREE_THRESH use MINFREE_SMALL.
 *
 * MINFREE_THRESH is given in Kbytes to avoid overflows with
 * large disks, so take capacity and divide by 1024.
 */
#define	MINFREE_LARGE_THRESH	(150*1024) 	/* 150MB */
#define MINFREE_FLOPPY_THRESH	(6*1024) 	/* 6 MB */
#define	MINFREE_LARGE	10		/* % minfree for large drives */
#define	MINFREE_SMALL	5		/* % minfree for small drives */
#define MINFREE_FLOPPY	0		/* % minfree for floppy disks */

/*
 * Constants for faking hard disk entries
 */
#define NUM_HD_HEADS 	4		/* number of tracks/cylinder */
#define NUM_HD_SECT	32		/* sectors/track (DEV_BSIZE) */

#define	NELEM(x)		(sizeof(x)/sizeof(x[0]))
#define	LAST_ELEM(x)		((x)[NELEM(x)-1])

#include <stdio.h>

static struct disktab idt;

#define	MS_DASD		3		/* Direct access device mode page */
#define	MS_RDG		4		/* Rigid geometry mode page */

#ifdef i386
struct mode_sense_cmd {
	u_char	msc_opcode;
	u_char	msc_mbz1:5,
		msc_lun:3;
	u_char	msc_page:6,
		msc_pcf:2;
	u_char	msc_mbz2;
	u_char	msc_len;
	u_char	msc_ctrl;
};

struct param_list_header {
	u_char	plh_len;
	u_char	plh_medium;
	u_char	plh_reserved:7,
		plh_wp:1;
	u_char	plh_blkdesclen;
};

struct block_descriptor {
	u_char	bd_density;
	u_char	bd_nblkmsb;
	u_char	bd_nblkinb;
	u_char	bd_nblklsb;

	u_char	bd_reserved;
	u_char	bd_blklenmsb;
	u_char	bd_blkleninb;
	u_char	bd_blklenlsb;
};

struct device_format_params {
	u_char	dfp_pagecode:6,
		dfp_reserved:1,
		dfp_savable:1;
	u_char	dfp_pagelen;
	u_char	dfp_trkszonemsb;
	u_char	dfp_trkszonelsb;
	u_char	dfp_altsecszonemsb;
	u_char	dfp_altsecszonelsb;
	u_char	dfp_alttrkszonemsb;
	u_char	dfp_alttrkszonelsb;
	u_char	dfp_alttrksvolmsb;
	u_char	dfp_alttrksvollsb;
	u_char	dfp_sectorsmsb;
	u_char	dfp_sectorslsb;
	u_char	dfp_bytessectormsb;
	u_char	dfp_bytessectorlsb;
	u_char	dfp_interleavemsb;
	u_char	dfp_interleavelsb;
	u_char	dfp_trkskewmsb;
	u_char	dfp_trkskewlsb;
	u_char	dfp_cylskewmsb;
	u_char	dfp_cylskewlsb;
	u_char	dfp_reserved2:4,
		dfp_surf:1,
		dfp_rmb:1,
		dfp_hsec:1,
		dfp_ssec:1;
	u_char	dfp_reserved3;
	u_char	dfp_reserved4;
	u_char	dfp_reserved5;
};

struct rigid_drive_params {
	u_char	rdp_pagecode:6,
		rdp_reserved:1,
		rdp_savable:1;
	u_char	rdp_pagelen;

	u_char	rdp_maxcylmsb;
	u_char	rdp_maxcylinb;
	u_char	rdp_maxcyllsb;

	u_char	rdp_maxheads;

	u_char	rdp_wpstartmsb;
	u_char	rdp_wpstartinb;
	u_char	rdp_wpstartlsb;

	u_char	rdp_rwcstartmsb;
	u_char	rdp_rwcstartinb;
	u_char	rdp_rwcstartlsb;

	u_char	rdp_stepratemsb;
	u_char	rdp_stepratelsb;

	u_char	rdp_landcylmsb;
	u_char	rdp_landcylinb;
	u_char	rdp_landcyllsb;

	u_char	rdp_reserved2;
	u_char	rdp_reserved3;
	u_char	rdp_reserved4;
};
#else i386
struct mode_sense_cmd {
	u_int	msc_opcode:8,
		msc_lun:3,
		msc_mbz1:5,
		msc_pcf:2,
		msc_page:6,
		msc_mbz2:8;
	u_char	msc_len;
	u_char	msc_ctrl;
};

struct param_list_header {
	u_char	plh_len;
	u_char	plh_medium;
	u_char	plh_wp:1,
		plh_reserved:7;
	u_char	plh_blkdesclen;
};

struct block_descriptor {
	u_int	bd_density:8,
		bd_nblk:24;
	u_int	bd_reserved:8,
		bd_blklen:24;
};

struct device_format_params {
	u_char	dfp_savable:1,
		dfp_reserved:1,
		dfp_pagecode:6;
	u_char	dfp_pagelen;
	u_short	dfp_trkszone;
	u_short	dfp_altsecszone;
	u_short	dfp_alttrkszone;
	u_short	dfp_alttrksvol;
	u_short	dfp_sectors;
	u_short	dfp_bytessector;
	u_short	dfp_interleave;
	u_short	dfp_trkskew;
	u_short	dfp_cylskew;
	u_char	dfp_ssec:1,
		dfp_hsec:1,
		dfp_rmb:1,
		dfp_surf:1,
		dfp_reserved2:4;
	u_char	dfp_reserved3;
	u_char	dfp_reserved4;
	u_char	dfp_reserved5;
};

struct rigid_drive_params {
	u_char	rdp_savable:1,
		rdp_reserved:1,
		rdp_pagecode:6;
	u_char	rdp_pagelen;

	u_char	rdp_maxcylmsb;
	u_char	rdp_maxcylinb;
	u_char	rdp_maxcyllsb;

	u_char	rdp_maxheads;

	u_char	rdp_wpstartmsb;
	u_char	rdp_wpstartinb;
	u_char	rdp_wpstartlsb;

	u_char	rdp_rwcstartmsb;
	u_char	rdp_rwcstartinb;
	u_char	rdp_rwcstartlsb;

	u_char	rdp_stepratemsb;
	u_char	rdp_stepratelsb;

	u_char	rdp_landcylmsb;
	u_char	rdp_landcylinb;
	u_char	rdp_landcyllsb;

	u_char	rdp_reserved2;
	u_char	rdp_reserved3;
	u_char	rdp_reserved4;
};
#endif i386

#define	THREE_BYTE(x)	\
		(((x##msb)<<16)|((x##inb)<<8)|(x##lsb))

#define	TWO_BYTE(x)	\
		(((x##msb)<<8)|(x##lsb))

#ifdef i386
#define SWAPIT(x) TWO_BYTE(x)
#else
#define SWAPIT(x) x
#endif i386

struct mode_sense_reply {
	struct param_list_header msr_plh;
	struct block_descriptor msr_bd;
	union {
		struct device_format_params u_msr_dfp;
		struct rigid_drive_params u_msr_rdp;
	}u;
};

#define msr_dfp	u.u_msr_dfp
#define msr_rdp	u.u_msr_rdp

char *progname;

extern int	force_blocksize;	/* Make blksize >= DEV_BSIZE? */

#ifdef i386
extern int useAllSectors;
extern int biosAccessibleBlocks;
#endif

int
do_inquiry(int fd, struct inquiry_reply *irp)
{
	struct scsi_req sr;
	struct cdb_6 *c6p;

	bzero((char *)&sr, sizeof(sr));

	c6p = (struct cdb_6 *)&sr.sr_cdb;
	c6p->c6_opcode = C6OP_INQUIRY;
	c6p->c6_len = sizeof(*irp);

	sr.sr_addr = (char *)irp;
	sr.sr_dma_max = sizeof(*irp);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;

	return ioctl(fd, SDIOCSRQ, &sr);
}

static int
do_modesense(int fd, struct mode_sense_reply *msrp, int page)
{
	struct scsi_req sr;
	struct mode_sense_cmd *mscp;
	int		status;		/* 97.08.14 Need error status from ioctl */

	bzero((char *)&sr, sizeof(sr));

	mscp = (struct mode_sense_cmd *)&sr.sr_cdb;
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = page;
	mscp->msc_len = sizeof(*msrp);

	sr.sr_addr = (char *)msrp;
	sr.sr_dma_max = sizeof(*msrp);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;

	status = ioctl(fd, SDIOCSRQ, &sr);
	if (status != 0) {
		/*
	 	 * 97.08.13 MM: make sure that the reply buffer is zero
		 * in case the mode sense fails - drive geometry is
		 * optional for SCSI disks.
		 */
		// perror("mode sense failed");
		bzero((char *) msrp, sizeof (struct mode_sense_reply));
	}
	/* Ignore mode sense errors */
	return 0;
}

struct disktab *
sd_inferdisktab(int fd, int partition_size, int bfd_size)
{
	extern struct dtype_sw *drive_type(), *dsp;
	extern int dosdisk;
	struct inquiry_reply ir;
	struct mode_sense_reply dasd, rd;
	struct drive_info di;
	int sfactor, ncyl, ntracks, nsectors, nblocks;
	char namebuf[512];
	int min_free, size, num;
	u_int blklen, lastlba;

	if (do_inquiry(fd, &ir) < 0)
		return NULL;

	if (ioctl(fd, DKIOCBLKSIZE, &size) < 0) {
		perror("ioctl(DKIOCBLKSIZE)");
		return NULL;
	}
	if (ioctl(fd, DKIOCNUMBLKS, &num) < 0) {
		perror("ioctl(DKIOCNUMBLKS)");
		return NULL;
	}

	blklen = size;
	lastlba = num - 1;
		
	if (ioctl(fd, DKIOCINFO, &di) < 0)
		return NULL;

	bzero(&dasd, sizeof(struct mode_sense_reply));
	if (do_modesense(fd, &dasd, MS_DASD) < 0)
		return NULL;
	
	strcpy(idt.d_type, ir.ir_removable ? "removable" : "fixed");
	strcat(idt.d_type, "_rw_scsi");
	dsp = drive_type(idt.d_type);
	if (blklen == DISK_BLK0SZ) {
	    dosdisk = uses_fdisk();
	}
	else {
	    dosdisk = 0;
	}
	if (force_blocksize && (blklen < DEV_BSIZE)) {
	    sfactor = DEV_BSIZE / blklen;
	    if ((sfactor != 1) && (sfactor != 2) && (sfactor != 4))
		    return NULL;
	} else {
	    sfactor = 1;
	}
      
	apple_disk = has_apple_partitions();

	if(dasd.u.u_msr_dfp.dfp_pagelen == 0) {
		/*
		 * Page not supported. Fake it. 
		 */
fake_floppy:
		if (apple_disk) {
		    nblocks = apple_ufs_size / sfactor 
			/ (blklen / apple_block_size);
		}
		else if (force_blocksize && (blklen < DEV_BSIZE)) {
		    nblocks = dosdisk ? (dossize / sfactor)
			: ((lastlba + 1) / sfactor);
		} 
		else {
		    nblocks = dosdisk ? dossize : lastlba + 1;
		}

		/*
		 * Dumb hard disk.
		 */
		ntracks  = NUM_HD_HEADS;
		nsectors = NUM_HD_SECT;
		ncyl     = nblocks / (ntracks * nsectors);
#if 0
		if(ir.ir_removable) {
			/*
			 * Treat it like a floppy.
			 */
			ncyl     = NUM_FD_CYL;
			ntracks  = NUM_FD_HEADS;
			nsectors = nblocks / (ncyl * ntracks);
		}
#endif
		goto gen_entry;		
	}

	bzero(&rd, sizeof(struct mode_sense_reply));
	if (do_modesense(fd, &rd, MS_RDG) < 0)
		return NULL;
	if(rd.u.u_msr_rdp.rdp_pagelen == 0)
		goto fake_floppy;

	if (ir.ir_devicetype != DEVTYPE_DISK
	    && ir.ir_devicetype != DEVTYPE_RWOPTICAL)
		return NULL;

	if (SWAPIT(dasd.msr_dfp.dfp_sectors) == 0) {
		int wedgesize;
		u_int tempsect;

		/*
		 * Bummer -- this is likely zone-sectored....
		 * Best to choose nsectors as max sectors in
		 * any zone.  For now, we just add 1 and hope.
		 */
		wedgesize = THREE_BYTE(rd.msr_rdp.rdp_maxcyl)
		  * rd.msr_rdp.rdp_maxheads;
		tempsect = 1 + ((lastlba + 1 + (wedgesize / 2)) / wedgesize);
#ifdef i386
		dasd.msr_dfp.dfp_sectorsmsb = (tempsect >> 8);
		dasd.msr_dfp.dfp_sectorslsb = (tempsect & 0xFF);
#else
		dasd.msr_dfp.dfp_sectors = tempsect;
#endif i386
		fprintf(stderr,
		    "Zone sectored device, guessing sectors at %d\n",
		    SWAPIT(dasd.msr_dfp.dfp_sectors));
	}

	ncyl = THREE_BYTE(rd.msr_rdp.rdp_maxcyl);
	ntracks = rd.msr_rdp.rdp_maxheads;
	nsectors = (SWAPIT(dasd.msr_dfp.dfp_sectors) + sfactor - 1)/sfactor;

        if (apple_disk)
	    nblocks = apple_ufs_size / sfactor / (blklen / apple_block_size);
        else
	    nblocks = dosdisk ? dossize : (lastlba + 1)/sfactor;
#ifdef i386
	if (blklen == 512 && !useAllSectors && !dosdisk && (nblocks > (biosAccessibleBlocks / sfactor)))
	{
		printf("Only using BIOS-accessible sectors\n");
		nblocks = biosAccessibleBlocks / sfactor;
	}
#endif i386
	
	if (SWAPIT(dasd.msr_dfp.dfp_trkszone) == 1)
		nsectors -= SWAPIT(dasd.msr_dfp.dfp_altsecszone)/sfactor;
gen_entry:
	
	if((partition_size + bfd_size) > nblocks) {
		fprintf(stderr, "Requested partition size larger than disk\n");
		return NULL;
	}

	sprintf(namebuf, "%.*s-%d", MAXDNMLEN, di.di_name, blklen);
	if (strlen(namebuf) < MAXDNMLEN)
		strcpy(idt.d_name, namebuf);
	else {
		strncpy(idt.d_name, di.di_name, MAXDNMLEN);
		LAST_ELEM(idt.d_name) = '\0';
	}
	idt.d_secsize = (force_blocksize && (blklen < DEV_BSIZE)) ? 
	    DEV_BSIZE : blklen;
	idt.d_ntracks = ntracks;
	idt.d_nsectors = nsectors;
	idt.d_ncylinders = ncyl;
	if(((unsigned long long)nblocks * idt.d_secsize)/1024 > MINFREE_FLOPPY_THRESH) {
	        unsigned long base;
		base = apple_disk ? 
		    (apple_ufs_base / (blklen / apple_block_size)) : dosbase;

	        idt.d_rpm            = RPM_HARD;
		idt.d_front          = FPORCH_HARD * 1024 / idt.d_secsize;
		idt.d_boot0_blkno[0] = 32 * 1024 / idt.d_secsize + base;
		idt.d_boot0_blkno[1] = 96 * 1024 / idt.d_secsize + base;
		if(((unsigned long long)nblocks * idt.d_secsize)/1024 > MINFREE_LARGE_THRESH)
			min_free = MINFREE_LARGE;
		else 
			min_free = MINFREE_SMALL;
	}
	else {
		idt.d_rpm            = RPM_FLOPPY;
		idt.d_front          = FPORCH_FLOPPY * 1024 / idt.d_secsize;
		idt.d_boot0_blkno[0] = 32 * 1024 / idt.d_secsize;
		idt.d_boot0_blkno[1] = -1;	/* floppy has 1 boot block */
		min_free = MINFREE_FLOPPY;
	}
	idt.d_back = BPORCH;
	idt.d_ngroups = idt.d_ag_size = 0;
	idt.d_ag_alts = idt.d_ag_off = 0;
#ifdef m68k
	strcpy(idt.d_bootfile, "sdmach");
#else
	strcpy(idt.d_bootfile, "mach_kernel");
#endif m68k
	gethostname(idt.d_hostname, MAXHNLEN);
	idt.d_hostname[MAXHNLEN-1] = '\0';
	idt.d_rootpartition = 'a';
	idt.d_rwpartition = 'b';

        if (((unsigned long long)nblocks * idt.d_secsize)/1024 < MAX_PARTITION_SIZE) {
		struct partition *pp;
		int i = 1;
		int bfd_base = 0;

		pp = &idt.d_partitions[0];
		
		if (apple_disk)
		    pp->p_base = apple_ufs_base / sfactor / (blklen / apple_block_size);
		else
		    pp->p_base = dosdisk ? dosbase : 0;
		pp->p_size = partition_size ? partition_size :
				nblocks - idt.d_front - BPORCH - bfd_size;
		pp->p_bsize = BLKSIZE;
		pp->p_fsize = MAX(idt.d_secsize, DEV_BSIZE);
		pp->p_cpg = CYLPGRP;
		pp->p_density = (((unsigned long long)nblocks * idt.d_secsize)/1024 <= 
		    MINFREE_FLOPPY_THRESH) ? BYTPERINO_FLPY : BYTPERINO;
		pp->p_minfree = min_free;
		pp->p_newfs = 1;
		pp->p_mountpt[0] = '\0';
		pp->p_automnt = 1;
		pp->p_opt = min_free < 10 ? 's' : 't';
		strcpy(pp->p_type, "4.4BSD");

		if (partition_size) {
			pp = &idt.d_partitions[i++];
			pp->p_base = partition_size + idt.d_partitions[0].p_base;
			pp->p_size = nblocks - partition_size -
				idt.d_front - BPORCH - bfd_size;
			pp->p_bsize = BLKSIZE;
			pp->p_fsize = (idt.d_secsize < 1024) ? 1024 : idt.d_secsize;
			pp->p_cpg = CYLPGRP;
			pp->p_density = (((unsigned long long)nblocks * idt.d_secsize)/1024 <= 
			    MINFREE_FLOPPY_THRESH) ? BYTPERINO_FLPY : 
			    BYTPERINO;
			pp->p_minfree = min_free;
			pp->p_newfs = 1;
			pp->p_mountpt[0] = '\0';
			pp->p_automnt = 1;
			pp->p_opt = min_free < 10 ? 's' : 't';
			strcpy(pp->p_type, "4.4BSD");
			bfd_base = pp->p_base + pp->p_size;
		}
		else {
			bfd_base = idt.d_partitions[0].p_base + 
			  	   idt.d_partitions[0].p_size;
		}

		for ( ; i < NPART; i++) {
			pp = &idt.d_partitions[i];
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
			pp = &idt.d_partitions[6];
			pp->p_base = bfd_base;
			pp->p_size = bfd_size;
			
			pp->p_bsize = -1;
			pp->p_fsize = -1;
			pp->p_newfs = 0;
			strcpy(pp->p_type, BFD_PART_TYPE);
		}

        } else {            /* Disk is larger than maximum partition size */

            struct partition *pp;
            int blocks_remaining, p_size;
            int i, npart, divisor, bfd_base = 0;
            
            blocks_remaining = nblocks - idt.d_front - BPORCH - bfd_size;
            npart = bfd_size ? NPART - 1 : NPART;
            for (i = 0; i < npart; i++) {
                if (partition_size) {
                    p_size = partition_size;
                } else {
                    if (blocks_remaining <= MAX_PARTITION_SIZE)
                        p_size = blocks_remaining;
                    else {
                        divisor = (blocks_remaining + MAX_PARTITION_SIZE) /
			    MAX_PARTITION_SIZE;	// round up
                        p_size = blocks_remaining / divisor;
                    }
                }
                partition_size = 0;
                blocks_remaining -= p_size;
                
                pp = &idt.d_partitions[i];
                if (p_size > 0) {
                    if (i == 0) {
			if (apple_disk)
			    pp->p_base = apple_ufs_base / sfactor 
				/ (blklen / apple_block_size);
			else
			    pp->p_base = dosdisk ? dosbase : 0;
		    }
                    else
                        pp->p_base = idt.d_partitions[i-1].p_base + idt.d_partitions[i-1].p_size;
                    pp->p_size = p_size;
                    pp->p_bsize = BLKSIZE;
                    pp->p_fsize = MAX(idt.d_secsize, DEV_BSIZE);
                    pp->p_cpg = CYLPGRP;
                    pp->p_density = (((unsigned long long)p_size * idt.d_secsize)/1024 <=
                        MINFREE_FLOPPY_THRESH) ? BYTPERINO_FLPY :
                        BYTPERINO;
                    pp->p_minfree = min_free;
                    pp->p_newfs = 1;
                    pp->p_mountpt[0] = '\0';
                    pp->p_automnt = 1;
                    pp->p_opt = min_free < 10 ? 's' : 't';
                    strcpy(pp->p_type, "4.4BSD");
                    bfd_base = pp->p_base + pp->p_size;
                } else {
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
            if(bfd_size) {
                    /*
                     * Put bfd in partition 6 by convention; nobody
                     * depends on this. Only base and size matter here;
                     * ufs parameters are ignored.
                     */
                    pp = &idt.d_partitions[6];
                    pp->p_base = bfd_base;
                    pp->p_size = bfd_size;

                    pp->p_bsize = -1;
                    pp->p_fsize = -1;
                    pp->p_newfs = 0;
                    strcpy(pp->p_type, BFD_PART_TYPE);
            }
        }


	return &idt;
}


