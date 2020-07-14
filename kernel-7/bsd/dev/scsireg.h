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

/*	@(#)scsireg.h	1.0	10/23/89	(c) 1989 NeXT	*/
/*
 * scsireg.h -- generic SCSI definitions
 * KERNEL VERSION
 *
 * HISTORY
 * 15-Jan-98	Martin Minow at Apple
 *		Radar 2178255: Fix 12-byte command length field
 * 13-Jun-95	dmitch at NeXT
 *	Added SCSI-3 types.
 * 10-Feb-93   	dmitch
 *	Added generic endian/alignment #ifdef's.
 * 17-May-90	dmitch
 *	Added SDIOCGETCAP
 * 30-Apr-90	dmitch
 *	Added C6S_SS_START, C6S_SS_STOP, C6S_SS_EJECT, SR_IOST_VOLNA
 * 30-Oct-89	dmitch
 *	Deleted private #defines (--> scsivar.h)
 * 25-Sep-89	dmitch at NeXT
 *	Added scsi_req, support for sg and st drivers
 * 10-Sept-87  	Mike DeMoney (mike) at NeXT
 *	Created.
 */

#ifndef _BSD_DEV_SCSIREG_
#define	_BSD_DEV_SCSIREG_

#import <sys/ioctl.h>
#import <sys/time.h>
#import <kern/queue.h>
#import <sys/types.h>

/*
 * status byte definitions
 */
#define	STAT_GOOD		0x00	/* cmd successfully completed */
#define	STAT_CHECK		0x02	/* abnormal condition occurred */
#define	STAT_CONDMET		0x04	/* condition met / good */
#define	STAT_BUSY		0x08	/* target busy */
#define	STAT_INTMGOOD		0x10	/* intermediate / good */
#define	STAT_INTMCONDMET	0x14	/* intermediate / cond met / good */
#define	STAT_RESERVED		0x18	/* reservation conflict */

#define	STAT_MASK		0x1e	/* clears vendor unique bits */


/*
 * SCSI command descriptor blocks
 * (Any device level driver doing fancy things will probably define
 * these locally and cast a pointer on top of the sd_cdb.  We define
 * them here to reserve the appropriate space, driver level routines
 * can use them if they want.)
 *
 * 6 byte command descriptor block, random device
 */
typedef struct cdb_6 {
#if	__BIG_ENDIAN__
	u_int	c6_opcode:8,		/* device command */
		c6_lun:3,		/* logical unit */
		c6_lba:21;		/* logical block number */
	u_char	c6_len;			/* transfer length */
	u_char	c6_ctrl;		/* control byte */

#elif	__LITTLE_ENDIAN__
	u_char	c6_opcode;
	u_char	c6_lba2		:5,
		c6_lun		:3;
	u_char	c6_lba1;
	u_char	c6_lba0;
	u_char	c6_len;
	u_char	c6_ctrl;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} cdb_6_t;

#define CDB_6_MAX_LENGTH        (255)           /* max block count */
#define CDB_6_MAX_LBA           ((1 << 21) - 1) /* max block address */

/*
 * 6 byte command descriptor block, sequential device
 */
typedef struct cdb_6s {
#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__

	u_char	c6s_opcode:8;		/* device command */
	u_char  c6s_lun:3,		/* logical unit */
		c6s_spare:3,		/* reserved */
		c6s_opt:2;		/* bits 1..0 - space type, fixed, 
					 *    etc. */ 
	/*
	 * Careful, natural alignment...
	 */
	u_char 	c6s_len[3]; 		/* transfer length */
	u_char	c6s_ctrl;		/* control byte */

#else	/* __NATURAL_ALIGNMENT__ */


	u_char	c6s_opcode:8;		/* device command */
	u_char  c6s_lun:3,		/* logical unit */
		c6s_spare:3,		/* reserved */
		c6s_opt:2;		/* bits 1..0 - space type, fixed, 
					 *    etc. */ 
	u_int	c6s_len:24,		/* transfer length */
		c6s_ctrl:8;		/* control byte */
#endif	/* __NATURAL_ALIGNMENT__ */


#elif	__LITTLE_ENDIAN__
	u_char	c6s_opcode;
	u_char	c6s_opt		:2,
		c6s_spare	:3,
		c6s_lun		:3;
	u_char	c6s_len2;
	u_char	c6s_len1;
	u_char	c6s_len0;
	u_char	c6s_ctrl;

#else	
#error	SCSI command / data structures are compiler sensitive
#endif
} cdb_6s_t;


/*
 * 10 byte command descriptor block
 * This definition is machine dependent due to an int on a short boundary.
 */
typedef struct cdb_10 {
#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__
        u_char  c10_opcode;             /* device command */
        u_char  c10_lun:3,              /* logical unit */
                c10_dp0:1,              /* disable page out */
                c10_fua:1,              /* force unit access */
                c10_mbz1:2,             /* reserved: must be zero */
                c10_reladr:1;           /* addr relative to prev 
		       			 * linked cmd */
	/*
	 * Careful, this can't be an int due to natural alignment...
	 */
        u_char	c10_lba[4];             /* logical block number */
        u_char	c10_mbz2:8;  		/* reserved: must be zero */
	u_char 	c10_len[2]; 		/* transfer length */
	u_char	c10_ctrl;		/* control byte */

#else	/* __NATURAL_ALIGNMENT__ */

	u_char	c10_opcode;		/* device command */
	u_char	c10_lun:3,		/* logical unit */
		c10_dp0:1,		/* disable page out (cache control) */
		c10_fua:1,		/* force unit access (cache control) */
		c10_mbz1:2,		/* reserved: must be zero */
		c10_reladr:1;		/* addr relative to prev linked cmd */
	u_int	c10_lba;		/* logical block number */
	u_int	c10_mbz2:8,		/* reserved: must be zero */
		c10_len:16,		/* transfer length */
		c10_ctrl:8;		/* control byte */
		
#endif	/* __NATURAL_ALIGNMENT__ */
#elif	  __LITTLE_ENDIAN__

	u_char	c10_opcode;
	u_char	c10_reladr	:1,
		c10_mbz1	:2,
		c10_fua		:1,
		c10_dp0		:1,
		c10_lun		:3;
	u_char	c10_lba3;
	u_char	c10_lba2;
	u_char	c10_lba1;
	u_char	c10_lba0;
	u_char	c10_mbz2;
	u_char	c10_len1;
	u_char	c10_len0;
	u_char	c10_ctrl;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} cdb_10_t;

/*
 * 12 byte command descriptor block
 * This definition is machine dependent due to an int on a short boundary.
 */
typedef struct cdb_12 {
#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__

        u_char	c12_opcode;		/* device command */
        u_char	c12_lun:3,		/* logical unit */
		c12_dp0:1,              /* disable page out */
		c12_fua:1,              /* force unit access */
		c12_mbz1:2,             /* reserved: must be zero */
		c12_reladr:1;           /* addr relative to prev 
					 * linked cmd */
        u_char	c12_lba[4];             /* logical block number */
/* Radar 2178255: need 4-byte length field */
        u_char	c12_len[4];		/* transfer length */
	u_char	c12_mbz2;		/* Reserved: must be zero */
        u_char	c12_ctrl:8;             /* control byte */

#else	/* __NATURAL_ALIGNMENT__ */

	u_char	c12_opcode;		/* device command */
	u_char	c12_lun:3,		/* logical unit */
		c12_dp0:1,		/* disable page out (cache control) */
		c12_fua:1,		/* force unit access (cache control) */
		c12_mbz1:2,		/* reserved: must be zero */
		c12_reladr:1;		/* addr relative to prev linked cmd */
	u_int	c12_lba;		/* logical block number */
/* Radar 2178255: need 4-byte length field */
        u_char	c12_len[4];		/* transfer length */
	u_char	c12_mbz2;		/* Reserved: must be zero */
	u_char	c12_ctrl:8;		/* control byte */

#endif	/* __NATURAL_ALIGNMENT__ */
#elif	 __LITTLE_ENDIAN__ 

	u_char	c12_opcode;
	u_char	c12_reladr	:1,
		c12_mbz1	:2,
		c12_fua		:1,
		c12_dp0		:1,
		c12_lun		:3;
	u_char	c12_lba3;
	u_char	c12_lba2;
	u_char	c12_lba1;
	u_char	c12_lba0;
/* Radar 2178255: need 4-byte length field */
	u_char	c12_len3;
	u_char	c12_len2;
	u_char	c12_len1;
	u_char	c12_len0;
	u_char	c12_mbz2;
	u_char	c12_ctrl;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} cdb_12_t;

/*
 * 16 byte command descriptor block (SCSI-3 only)
 * This definition is machine dependent due to an int on a short boundary.
 */
typedef struct cdb_16 {
#if	__BIG_ENDIAN__
#if	__NATURAL_ALIGNMENT__

        u_char	c16_opcode;		/* device command */
        u_char	c16_mbz1;               /* reserved: must be zero */
        u_char	c16_lba[4];             /* logical block number */
	u_char	c16_addl0;
	u_char	c16_addl1;
	u_char	c16_addl2;
	u_char	c16_addl3;
        u_char	c16_len[4];		/* transfer length */
        u_char	c16_mbz2;               /* reserved: must be zero */
        u_char	c16_ctrl;             	/* control byte */

#else	/* __NATURAL_ALIGNMENT__ */

	u_char	c16_opcode;		/* device command */
        u_char	c16_mbz1;               /* reserved: must be zero */
	u_int	c16_lba;		/* logical block number */
	u_char	c16_addl0;
	u_char	c16_addl1;
	u_char	c16_addl2;
	u_char	c16_addl3;
	u_int	c16_len;		/* transfer length */
        u_char	c16_mbz2;               /* reserved: must be zero */
        u_char	c16_ctrl;             	/* control byte */

#endif	/* __NATURAL_ALIGNMENT__ */
#elif	__LITTLE_ENDIAN__

	u_char	c16_opcode;
        u_char	c16_mbz1;               /* reserved: must be zero */
	u_char	c16_lba3;
	u_char	c16_lba2;
	u_char	c16_lba1;
	u_char	c16_lba0;
	u_char	c16_addl0;
	u_char	c16_addl1;
	u_char	c16_addl2;
	u_char	c16_addl3;
	u_char	c16_len3;
	u_char	c16_len2;
	u_char	c16_len1;
	u_char	c16_len0;
        u_char	c16_mbz2;               /* reserved: must be zero */
        u_char	c16_ctrl;             	/* control byte */

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} cdb_16_t;


typedef union cdb {
	struct	cdb_6	cdb_c6;
	struct  cdb_6s  cdb_c6s;
	struct	cdb_10	cdb_c10;
	struct	cdb_12	cdb_c12;
} cdb_t;

/*
 * SCSI-3 CDB union.
 */
typedef union scsi3_cdb {
	struct	cdb_6	cdb_c6;
	struct  cdb_6s  cdb_c6s;
	struct	cdb_10	cdb_c10;
	struct	cdb_12	cdb_c12;
	struct	cdb_16	cdb_c16;
} scsi3_cdb_t;


#define	cdb_opcode	cdb_c6.c6_opcode	/* all opcodes in same place */

/*
 * control byte values
 */
#define	CTRL_LINKFLAG		0x03	/* link and flag bits */
#define	CTRL_LINK		0x01	/* link only */
#define	CTRL_NOLINK		0x00	/* no command linking */

/*
 * six byte cdb opcodes
 * (Optional commands should only be used by formatters)
 */
#define	C6OP_TESTRDY		0x00	/* test unit ready */
#define C6OP_REWIND		0x01	/* rewind */
#define	C6OP_REQSENSE		0x03	/* request sense */
#define	C6OP_FORMAT		0x04	/* format unit */
#define	C6OP_REASSIGNBLK	0x07	/* OPT: reassign block */
#define	C6OP_READ		0x08	/* read data */
#define	C6OP_WRITE		0x0a	/* write data */
#define	C6OP_SEEK		0x0b	/* seek */
#define C6OP_READREV		0x0F	/* read reverse */
#define C6OP_WRTFM		0x10	/* write filemarks */
#define C6OP_SPACE		0x11	/* space records/filemarks */
#define	C6OP_INQUIRY		0x12	/* get device specific info */
#define C6OP_VERIFY		0x13	/* sequential verify */
#define	C6OP_MODESELECT		0x15	/* OPT: set device parameters */
#define	C6OP_MODESENSE		0x1a	/* OPT: get device parameters */
#define	C6OP_STARTSTOP		0x1b	/* OPT: start or stop device */
#define	C6OP_SENDDIAG		0x1d	/* send diagnostic */
#define	C60P_PREVENTALLOW	0x1e	/* prevent/allow medium removal */

/*
 * ten byte cdb opcodes
 */
#define	C10OP_READCAPACITY	0x25	/* read capacity */
#define	C10OP_READEXTENDED	0x28	/* read extended */
#define	C10OP_WRITEEXTENDED	0x2a	/* write extended */
#define	C10OP_READDEFECTDATA	0x37	/* OPT: read media defect info */


/*
 *	c6s_opt - options for 6-byte sequential device commands 
 */
 
#define C6OPT_FIXED		0x01	/* fixed block transfer */
#define C6OPT_LONG		0x01	/* 1 = erase to EOT */
#define C6OPT_IMMED		0x01	/* immediate (for rewind, retension) */
#define C6OPT_BYTECMP		0x02	/* byte compare for C6OP_VERIFY */
#define C6OPT_SIL		0x02	/* suppress illegal length (Exabyte) */
#define C6OPT_SPACE_LB		0x00	/* space logical blocks */
#define C6OPT_SPACE_FM		0x01	/* space filemarks */
#define C6OPT_SPACE_SFM		0x02	/* space sequential filemarks */
#define C6OPT_SPACE_PEOD	0x03	/* space to physical end of data */	

/*	
 *	other 6-byte sequential command constants
 */
 
#define C6S_MAXLEN		0xFFFFFF	
#define C6S_RETEN		0x02	/* byte 4 of load/unload - retension */
#define C6S_LOAD		0x01	/* byte 4 of load/unload - load */

/*
 * these go in the c6_len fields of start/stop command
 */
#define C6S_SS_START		0x01	/* start unit */
#define C6S_SS_STOP		0x00	/* stop unit */
#define C6S_SS_EJECT		0x02	/* eject disk */

/*
 * extended sense data
 * returned by C6OP_REQSENSE
 */
typedef struct esense_reply {
#if	__BIG_ENDIAN__

	u_char	er_ibvalid:1,		/* information bytes valid */
		er_class:3,		/* error class */
		er_code:4;		/* error code */
	u_char	er_segment;		/* segment number for copy cmd */
	u_char	er_filemark:1,		/* file mark */
		er_endofmedium:1,	/* end-of-medium */
		er_badlen:1,		/* incorrect length */
		er_rsvd2:1,		/* reserved */
		er_sensekey:4;		/* sense key */
	u_char	er_infomsb;		/* MSB of information byte */
	u_int	er_info:24,		/* bits 23 - 0 of info "byte" */
		er_addsenselen:8;	/* additional sense length */
	u_int	er_rsvd8;		/* copy status (unused) */
	u_char	er_addsensecode;	/* additional sense code */
	
	/* the following are used for tape only as of 27-Feb-89 */
	
	u_char	er_qualifier;		/* sense code qualifier */
	u_char  er_rsvd_e;
	u_char  er_rsvd_f;
	u_int   er_err_count:24,	/* three bytes of data error counter */
		er_stat_13:8;		/* byte 0x13 - discrete status bits */
	u_char  er_stat_14;		/* byte 0x14 - discrete status bits */
	u_char  er_stat_15;		/* byte 0x15 - discrete status bits */
	
#if	__NATURAL_ALIGNMENT__

	u_char	er_rsvd_16;
	u_char	er_remaining[3];	/* bytes 0x17..0x19 - remaining tape */

#else	/* __NATURAL_ALIGNMENT__ */

	u_int   er_rsvd_16:8,
		er_remaining:24;	/* bytes 0x17..0x19 - remaining tape */

#endif	/* __NATURAL_ALIGNMENT__ */

#elif	__LITTLE_ENDIAN__

	u_char	er_code		:4,
		er_class	:3,
		er_ibvalid	:1;
	u_char	er_segment;
	u_char	er_sensekey	:4,
		er_rsvd2	:1,
		er_badlen	:1,
		er_endofmedium	:1,
		er_filemark	:1;
	u_char	er_info3;
	u_char	er_info2;
	u_char	er_info1;
	u_char	er_info0;
	u_char	er_addsenselen;
	u_char	er_rsvd8[4];
	u_char	er_addsensecode;

	u_char	er_qualifier;
	u_char	er_rsvd_e;
	u_char	er_rsvd_f;
	u_char	er_err_count2;
	u_char	er_err_count1;
	u_char	er_err_count0;
	u_char	er_stat_13;
	u_char	er_stat_14;
	u_char	er_stat_15;
	u_char	er_rsvd_16;
	u_char	er_remaining2;
	u_char	er_remaining1;
	u_char	er_remaining0;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
		
	/* technically, there can be additional bytes of sense info
	 * here, but we don't check them, so we don't define them
	 */
} esense_reply_t;

/*
 * sense keys
 */
#define	SENSE_NOSENSE		0x0	/* no error to report */
#define	SENSE_RECOVERED		0x1	/* recovered error */
#define	SENSE_NOTREADY		0x2	/* target not ready */
#define	SENSE_MEDIA		0x3	/* media flaw */
#define	SENSE_HARDWARE		0x4	/* hardware failure */
#define	SENSE_ILLEGALREQUEST	0x5	/* illegal request */
#define	SENSE_UNITATTENTION	0x6	/* drive attention */
#define	SENSE_DATAPROTECT	0x7	/* drive access protected */
#define	SENSE_ABORTEDCOMMAND	0xb	/* target aborted command */
#define	SENSE_VOLUMEOVERFLOW	0xd	/* eom, some data not transfered */
#define	SENSE_MISCOMPARE	0xe	/* source/media data mismatch */

/*
 * inquiry data
 */
typedef struct inquiry_reply {

#if	__BIG_ENDIAN__

	u_char	ir_qual:3,		/* qualifier */
		ir_devicetype:5;	/* device type, see below */
	u_char	ir_removable:1,		/* removable media */
		ir_typequalifier:7;	/* device type qualifier */
	u_char	ir_isoversion:2,	/* ISO  version number */
		ir_ecmaversion:3,	/* ECMA version number */
		ir_ansiversion:3;	/* ANSI version number */
	u_char	ir_aerc:1,		/* Async event reporting capability */
		ir_trmtsk:1,		/* Terminate Task */
		ir_normaca:1,		/* Normal ACA supported */
		ir_zero2:1,		/* reserved */
		ir_rspdatafmt:4;	/* response data format */
	u_char	ir_addlistlen;		/* additional list length */
	u_char	ir_zero3;		/* reserved */
	u_char	ir_zero4:2,		/* reserved */
		ir_port:1,		/* port A/B */
		ir_dualPort:1,		/* dual port supported */
		ir_mchngr:1,		/* medium changer */
		ir_ackqreqq:1,		/* Q cable (SIP only) */
		ir_addr32:1,		/* 32-bit addressing (SIP only) */
		ir_addr16:1;		/* 16-bit addressing (SIP only) */
	u_char	ir_reladr:1,		/* relative addressing */
		ir_wbus32:1,		/* 32-bit wide data transfers */
		ir_wbus16:1,		/* 16-bit wide data transfers */
		ir_sync:1,		/* synchronous data transfers */
		ir_linked:1,		/* linked commands */
		ir_trandis:1,		/* transfer disable */
		ir_cmdque:1,		/* tagged command queuing */
		ir_sftre:1;		/* soft reset */
	char	ir_vendorid[8];		/* vendor name in ascii */
	char	ir_productid[16];	/* product name in ascii */
	char	ir_revision[4];		/* revision level info in ascii */
	char	ir_misc[28];		/* misc info */
	char	ir_endofid[1];		/* just a handle for end of id info */

#elif	__LITTLE_ENDIAN__

	u_char	ir_devicetype	:5,
		ir_qual		:3;
	u_char	ir_typequalifier:7,
		ir_removable	:1;
	u_char	ir_ansiversion	:3,
		ir_ecmaversion	:3,
		ir_isoversion	:2;
	u_char	ir_rspdatafmt	:4,
		ir_zero2:1,		/* reserved */
		ir_normaca:1,		/* Normal ACA supported */
		ir_trmtsk:1,		/* Terminate Task */
		ir_aerc:1;		/* Async event reporting capability */
	u_char	ir_addlistlen;
	u_char	ir_zero3;
	u_char	ir_addr16:1,		/* 16-bit addressing (SIP only) */
		ir_addr32:1,		/* 32-bit addressing (SIP only) */
		ir_ackqreqq:1,		/* Q cable (SIP only) */
		ir_mchngr:1,		/* medium changer */
		ir_dualPort:1,		/* dual port supported */
		ir_port:1,		/* port A/B */
		ir_zero4:2;		/* reserved */
	u_char	ir_sftre:1,
		ir_cmdque:1,
		ir_trandis:1,		/* transfer disable */
		ir_linked:1,
		ir_sync:1,
		ir_wbus16:1,
		ir_wbus32:1,
		ir_reladr:1;
	u_char	ir_vendorid[8];
	u_char	ir_productid[16];
	u_char	ir_revision[4];
	u_char	ir_misc[28];
	u_char	ir_endofid[1];

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} inquiry_reply_t;

#define	DEVQUAL_OK		0x00	/* device is connected to this lun */
#define	DEVQUAL_MIA		0x01	/* device not connected to lun */
#define	DEVQUAL_RSVD		0x02	/* reserved */
#define	DEVQUAL_NODEV		0x03	/* target doesn't support dev on lun */
#define	DEVQUAL_VUMASK		0x04	/* 1XXb is vendor specific */


#define	DEVTYPE_DISK		0x00	/* read/write disks */
#define	DEVTYPE_TAPE		0x01	/* tapes and other sequential devices*/
#define	DEVTYPE_PRINTER		0x02	/* printers */
#define	DEVTYPE_PROCESSOR	0x03	/* cpu's */
#define	DEVTYPE_WORM		0x04	/* write-once optical disks */
#define	DEVTYPE_CDROM		0x05	/* cd rom's, etc */
#define DEVTYPE_SCANNER		0x06
#define DEVTYPE_OPTICAL		0x07	/* other optical storage */
#define DEVTYPE_CHANGER		0x08	/* jukebox */
#define DEVTYPE_COMM		0x09	/* communication device */
#define DEVTYPE_GRAPH_A		0x0a	/* ASC IT8 graphics */
#define DEVTYPE_GRAPH_B		0x0b	/* ASC IT8 graphics */
#define DEVTYPE_RAID		0x0c	/* RAID controller */
#define DEVTYPE_NOTPRESENT      0x1f    /* logical unit not present */

/*
 * read capacity reply
 */
typedef struct capacity_reply {

#if	__BIG_ENDIAN__

	u_int	cr_lastlba;		/* last logical block address */
	u_int	cr_blklen;		/* block length */

#elif	__LITTLE_ENDIAN__

	u_char	cr_lastlba3;
	u_char	cr_lastlba2;
	u_char	cr_lastlba1;
	u_char	cr_lastlba0;

	u_char	cr_blklen3;
	u_char	cr_blklen2;
	u_char	cr_blklen1;
	u_char	cr_blklen0;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} capacity_reply_t;

/*
 * Standard Mode Select/Mode Sense data structures
 */
 
typedef struct mode_sel_hdr {

#if	__BIG_ENDIAN__

	u_char 		msh_sd_length_0;	/* byte 0 - length (mode sense
						 *    only)  */
	u_char		msh_med_type;		/* medium type - random access
						 *   devices only */
	u_char		msh_wp:1,		/* byte 2 bit 7 - write protect
						 *   mode sense only) */
			msh_bufmode:3,		/* buffered mode - sequential
						 *   access devices only */
			msh_speed:4;		/* speed - sequential access
						 *   devices only */
	u_char		msh_bd_length;		/* block descriptor length */

#elif	__LITTLE_ENDIAN__

	u_char		msh_sd_length_0;
	u_char		msh_med_type;
	u_char		msh_speed	:4,
			msh_bufmode	:3,
			msh_wp		:1;
	u_char		msh_bd_length;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} mode_sel_hdr_t;

typedef struct mode_sel_bd {				/* block descriptor */

#if	__BIG_ENDIAN__

	u_int		msbd_density:8,
			msbd_numblocks:24;
	u_int		msbd_rsvd_0:8,		/* byte 4 - reserved */
			msbd_blocklength:24;

#elif	__LITTLE_ENDIAN__

	u_char		msbd_density;
	u_char		msbd_numblocks2;
	u_char		msbd_numblocks1;
	u_char		msbd_numblocks0;
	u_char		msbd_rsvd_0;
	u_char		msbd_blocklength2;
	u_char		msbd_blocklength1;
	u_char		msbd_blocklength0;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
} mode_sel_bd_t;

#define MODSEL_DATA_LEN	0x30

typedef struct mode_sel_data {

	/* transferred to/from target during mode select/mode sense */
	struct mode_sel_hdr msd_header;
	struct mode_sel_bd  msd_blockdescript;
	u_char msd_vudata[MODSEL_DATA_LEN];	/* for vendor unique data */
} mode_sel_data_t;

/* 
 * struct for MTIOCMODSEL/ MTIOCMODSEN
 */
typedef struct modesel_parms {
	struct mode_sel_data    msp_data;
	int			msp_bcount;	/* # of bytes to DMA */
} modesel_parms_t;

/*
 * Day-to-day constants in the SCSI world
 */
#define	SCSI_NTARGETS	8		/* 0 - 7 for target numbers */
#define	SCSI_NLUNS	8		/* 0 - 7 luns for each target */

/*
 * For SCSI-3 Wide.
 */
#define	SCSI3_NTARGETS	32

/*
 * Defect list header
 * Used by FORMAT and REASSIGN BLOCK commands
 */
struct defect_header {

#if	__BIG_ENDIAN__

	u_char	dh_mbz1;
	u_char	dh_fov:1,		/* format options valid */
		dh_dpry:1,		/* disable primary */
		dh_dcrt:1,		/* disable certification */
		dh_stpf:1,		/* stop format */
		dh_mbz2:4;
	u_short	dh_len;			/* items in defect list */

#elif	__LITTLE_ENDIAN__

	u_char	dh_mbz1;
	u_char	dh_mbz2		:4,
		dh_stpf		:1,
		dh_dcrt		:1,
		dh_dpry		:1,
		dh_fov		:1;
	u_char	dh_len1;
	u_char	dh_len0;

#else
#error	SCSI command / data structures are compiler sensitive
#endif
};

/*
 * Status for scsi_req (see below).
 */
typedef enum {
	
	SR_IOST_GOOD	= 0,		/* successful */
	SR_IOST_SELTO	= 1,		/* selection timeout */
	SR_IOST_CHKSV	= 2,		/* check status, sr_esense */
					/*    valid */
	SR_IOST_CHKSNV	= 3,		/* check status, sr_esense */
					/*    not valid */
	SR_IOST_DMAOR	= 4,		/* target attempted to move */
					/*    more than sr_dma_max */
					/*    bytes */
	SR_IOST_IOTO	= 5,		/* sr_ioto exceeded */
	SR_IOST_BV	= 6,		/* SCSI Bus violation */
	SR_IOST_CMDREJ	= 7,		/* command reject (by 
					 *    driver) */
	SR_IOST_MEMALL	= 8,		/* memory allocation failure */
	SR_IOST_MEMF	= 9,		/* memory fault */
	SR_IOST_PERM	= 10,		/* not super user */
	SR_IOST_NOPEN	= 11,		/* device not open */
	SR_IOST_TABT	= 12,		/* target aborted command */
	ST_IOST_BADST	= 13,		/* bad SCSI status byte  */
					/*  (other than check status)*/
#define	SR_IOST_BADST	ST_IOST_BADST
	ST_IOST_INT	= 14,		/* internal driver error */
#define	SR_IOST_INT	ST_IOST_INT
	SR_IOST_BCOUNT	= 15,		/* unexpected byte count */
					/* seen on SCSI bus */ 
	SR_IOST_VOLNA	= 16,		/* desired volume not available */
	SR_IOST_WP	= 17,		/* Media Write Protected */
	SR_IOST_ALIGN	= 18,		/* DMA alignment error */
	SR_IOST_IPCFAIL = 19,		/* Mach IPC failure */
	SR_IOST_RESET	= 20,		/* bus was reset during 
					 * processing of command */
	SR_IOST_PARITY	= 21,		/* SCSI Bus Parity Error */
	SR_IOST_HW	= 22,		/* Gross Hardware Failure */
	SR_IOST_DMA	= 23,		/* DMA error */
	SR_IOST_INVALID	= 100,		/* should never be seen */
} sc_status_t;

/*
 * DMA Direction.
 */
typedef enum {
	SR_DMA_RD = 0,			/* DMA from device to host */
	SR_DMA_WR = 1,			/* DMA from host to device */
} sc_dma_dir_t;

/*
 * SCSI Request used by sg driver via SGIOCREQ and internally in st driver
 */

typedef struct scsi_req {

	/*** inputs ***/
	
	cdb_t			sr_cdb;		/* command descriptor block - 
						 * one of four formats */
	sc_dma_dir_t		sr_dma_dir;	/* DMA direction */
	caddr_t			sr_addr;	/* memory addr for data 
						 * transfers */
	int			sr_dma_max;	/* maximum number of bytes to
						 * transfer */
	int			sr_ioto;	/* I/O timeout in seconds */
						 
	/*** outputs ***/
	
	int			sr_io_status;	/* driver status */
	u_char			sr_scsi_status;	/* SCSI status byte */
	esense_reply_t	 	sr_esense;	/* extended sense in case of
						 * check status */
	int			sr_dma_xfr;	/* actual number of bytes 
						 * transferred by DMA */
	struct	timeval		sr_exec_time;	/* execution time in 
						 * microseconds */
						 
#if	m68k

	/*** for driver's internal use ***/
	
	u_char			sr_flags;
	queue_chain_t		sr_io_q;	/* for linking onto sgdp->
						 *    sdg_io_q */
#else	/* m68k */

	u_char			sr_cdb_length;	/* length of CDB bytes 
						 *    (optional) */

	/*
	 * Flags to disable disconnect, command queueing, synchronous 
	 * transfer negotiation. Add one bit to allow chk. cond. to be 
	 * ignored. 
	 */

	u_char			sr_discon_disable:1,
				sr_cmd_queue_disable:1,
				sr_sync_disable:1,
				sr_ignore_chkcond:1,	/* to disable issuing 
							   of chk.cond. cmd  - 
							   specifically used 
							   for MTIOCSRQ 
							   requests. */
				
				sr_pad1:4;	

	u_char			sr_pad2;
				    
	u_char			sr_flags;	/* driver private */
	queue_chain_t		sr_io_q;
	
#endif	/* m68k */

} scsi_req_t;

/*
 * SCSI Request, with capability of 16-byte CDB
 */

typedef struct scsi3_req {

	/*** inputs ***/
	
	scsi3_cdb_t		s3r_cdb;	/* command descriptor block - 						 
						 * one of five formats */
	sc_dma_dir_t		s3r_dma_dir;	/* DMA direction */
	caddr_t			s3r_addr;	/* memory addr for data 
						 * transfers */
	int			s3r_dma_max;	/* maximum number of bytes to
						 * transfer */
	int			s3r_ioto;	/* I/O timeout in seconds */
						 
	/*** outputs ***/
	
	int			s3r_io_status;	/* driver status */
	u_char			s3r_scsi_status; /* SCSI status byte */
	esense_reply_t	 	s3r_esense;	/* extended sense in case of
						 * check status */
	int			s3r_dma_xfr;	/* actual number of bytes 
						 * transferred by DMA */
	struct	timeval		s3r_exec_time;	/* execution time in 
						 * microseconds */
						 
#if	m68k

	/*** for driver's internal use ***/
	
	u_char			s3r_flags;
	queue_chain_t		s3r_io_q;	/* for linking onto sgdp->
						 *    sdg_io_q */
#else	/* m68k */

	u_char			s3r_cdb_length;	/* length of CDB bytes 
						 *    (optional) */

	/*
	 * Flags to disable disconnect, command queueing, synchronous transfer
	 * negotiation.
	 */
	u_char			s3r_discon_disable:1,
				s3r_cmd_queue_disable:1,
				s3r_sync_disable:1,
				s3r_pad1:5;
	u_char			s3r_pad2;
				    
	u_char			s3r_flags;	/* driver private */
	queue_chain_t		s3r_io_q;
	
#endif	/* m68k */

} scsi3_req_t;

/*
 * SCSI-2 address specifier.
 */
typedef struct scsi_adr {

	u_char			sa_target;
	u_char			sa_lun;
	
} scsi_adr_t;

/*
 * SCSI-3 address specifier.
 */
typedef struct scsi3_adr {

	unsigned long long	s3a_target;
	unsigned long long	s3a_lun;
	
} scsi3_adr_t;


/*
 *	Generic SCSI ioctl requests
 */
 
#define SGIOCSTL	_IOW ('s', 0, struct scsi_adr)	/* set target/lun */
#define	SGIOCREQ	_IOWR('s', 1, struct scsi_req) 	/* cmd request */
#define SGIOCENAS	_IO(  's', 2)		 	/* enable autosense */
#define SGIOCDAS	_IO(  's', 3)			/* disable autosense */
#define SGIOCRST	_IO(  's', 4)			/* reset SCSI bus */
#define SGIOCCNTR       _IOW( 's', 6, int)              /* select controller */
#define SGIOCGAS	_IOR( 's', 7, int)		/* get autosense */
#define SGIOCMAXDMA	_IOR( 's', 8, int)		/* max DMA size */
#define SGIOCNUMTARGS	_IOR( 's', 9, int)		/* # of targets/bus */

/*
 *	ioctl requests specific to SCSI disks
 */
#define	SDIOCSRQ	_IOWR('s', 1, struct scsi_req) 	/* cmd request using */
							/* struct scsi_req */

#define SDIOCGETCAP	_IOR  ('s', 5, struct capacity_reply)
							/* Get Read 
							 * Capacity info */

/* 
 *	ioctl requests specific to SCSI tapes 
 */
 
#define	MTIOCFIXBLK	_IOW('m', 5, int )	/* set fixed block mode */
#define MTIOCVARBLK 	_IO('m',  6)		/* set variable block mode */
#define MTIOCMODSEL	_IOW('m', 7, struct modesel_parms)	
						/* mode select */
#define MTIOCMODSEN	_IOWR('m',8, struct modesel_parms)	
						/* mode sense */
#define MTIOCINILL	_IO('m',  9)		/* inhibit illegal length */
						/*    errors */
#define MTIOCALILL	_IO('m',  10)		/* allow illegal length */
						/*    errors */
#define	MTIOCSRQ	_IOWR('m', 11, struct scsi_req) 	
						/* cmd request using 
						 * struct scsi_req */

/*
 * SCSI-3 ioctl requests
 *
 * Generic SCSI:
 */
#define SGIOCSTL3	_IOW ('s', 12, struct scsi3_adr) /* set target/lun */
#define SGIOCGTL3	_IOR ('s', 13, struct scsi3_adr) /* get target/lun */
#define	SGIOCREQ3	_IOWR('s', 14, struct scsi3_req) /* cmd request */

/*
 * Disk
 */
#define	SDIOCSRQ3	_IOWR('s', 15, struct scsi3_req) /* cmd request */

/*
 * Tape
 */
#define	MTIOCSRQ3	_IOWR('m', 15, struct scsi3_req) /* cmd request */

#endif /* _BSD_DEV_SCSIREG_ */

