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
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * ata_extern.h -- Externally used data structures and constants for IDE
 *		disk driver
 *
 * KERNEL VERSION
 *
 * HISTORY  
 * 04-Mar-1997	 Scott Vail at NeXT
 *	Changes made to ideDriveInfo_t struct.  No longer stores
 *	capacity, only total sectors.
 *
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_ATA_EXTERN_H
#define _BSD_DEV_ATA_EXTERN_H

#import <sys/types.h>
#import <sys/ioctl.h>
#if (IO_DRIVERKIT_VERSION == 330)
#ifdef	KERNEL
#import <machdep/machine/pmap.h>
#import <vm/vm_map.h>
#endif	KERNEL
#endif
#import <mach/boolean.h>
#import <mach/vm_param.h>

#define MAX_IDE_DRIVES 			2 	/* for a PC AT connector */
#define MAX_IDE_CONTROLLERS 		2 	/* that we support */

/*
 * This should be removed when CDIS stops looking for NIDE_PCAT. 
 */
#define NIDE_PCAT			MAX_IDE_DRIVES	

/*
 * This struct stores the info returned from the BIOS. 
 */
typedef struct _ideDriveInfo {
    unsigned short	type;			/* from CMOS RAM */
    unsigned char	control_byte;	
    unsigned char	interleave;		/* interleave factor */
    unsigned int	total_sectors;		/* total number of sectors */
    unsigned short	cylinders;		/* number of cylinders */
    unsigned char 	heads;			/* total number of heads */
    unsigned char	sectors_per_trk;	/* sectors per track */
    unsigned		bytes_per_sector;	/* bytes per sector */
    unsigned		access_time;		/* average access time */
    unsigned short	precomp;		/* not used anymore */
    unsigned short	landing_zone;
} ideDriveInfo_t;

/*
 * Port addresses for IDE controller registers on PC AT. Don't change the
 * identifiers without changing the #define's in AtapiCnt.h as well. This is
 * because the ATAPI interface uses the same registers but has different
 * names for them. 
 */

typedef struct  _ideRegsAddrs {

    /* Command block registers */
    unsigned data;		/* data register base port (read/write) */
    unsigned features;		/* features register (write only) */
    unsigned error;		/* error register (read only) */
    unsigned sectCnt;		/* sector count (read/write) */
    unsigned sectNum;		/* sector number (read/write) */
    unsigned cylLow;		/* cylinder low (read/write) */
    unsigned cylHigh;		/* cylinder high (read/write) */
    unsigned drHead;		/* drive, head select register (read/write) */
    unsigned status;		/* status register (read only) */
    unsigned command;		/* command register (write only) */

    /* Control Block Registers */
    unsigned deviceControl; 	/* control register */
    unsigned altStatus; 	/* shadow of status register but read does */ 
    				/* not clear a pending interrupt */
    unsigned driveAddress;	/* not used */
} ideRegsAddrs_t; 

/*
 * Register values for IDE controller for PC AT. 
 */

typedef struct _ideRegsVal {
    unsigned short	data;		/* data register  */
    unsigned char	features;	/* features register  */
    unsigned char	error;		/* error register  */
    unsigned char	sectCnt;	/* sector count  */
    unsigned char	sectNum;	/* sector number  */
    unsigned char	cylLow;		/* cylinder low  */
    unsigned char	cylHigh;	/* cylinder high  */
    unsigned char	drHead;		/* drive, head select (write first) */
    unsigned char	status;		/* status register  */
    unsigned char	command;	/* command register  */
} ideRegsVal_t; 

/*
 * Defines for error register. 
 */
#define BAD_BLOCK			0x80
#define ECC_ERROR			0x40
#define ID_NOT_FOUND			0x10
#define CMD_ABORTED			0x04
#define TRK0_NOT_FOUND			0x02
#define DAM_NOT_FOUND			0x01


/*
 * Defines for drive, head select register. 
 */
#define SEL_DRIVE0			0x00
#define	SEL_DRIVE1			0x10

/*
 * Addressing method, LBA or CHS. 
 */
#define ADDRESS_MODE_CHS		0xa0
#define ADDRESS_MODE_LBA		0xe0

/*
 * Defines for the status register. 
 */

#define BUSY				0x80
#define READY				0x40
#define WRITE_FAULT			0x20
#define SEEK_COMPLETE			0x10
#define DREQUEST			0x08
#define ERROR_CORRECTED			0x04	/* really not used */
#define INDEX				0x02
#define ERROR				0x01 	/* Last command failed */

/*
 * Defines for control register. 
 */
#define HEADSEL3_ENABLE			0x08
#define DISK_RESET_ENABLE		0x04
#define DISK_INTERRUPT_ENABLE		0x00
#define DISK_INTERRUPT_DISABLE		0x02

/*
 * End of IDE register related defines. 
 */

/*
 * IDE commands -- as defined in the ATA spec. 
 */

/*
 * Mandatory commands. 
 */
#define 	IDE_READ		0x20
#define 	IDE_WRITE		0x30
#define 	IDE_SEEK		0x70
#define		IDE_RESTORE		0x10
#define		IDE_FORMAT_TRACK	0x50 	/* do not use */
#define 	IDE_READ_VERIFY		0x40
#define		IDE_DIAGNOSE		0x90
#define		IDE_SET_PARAMS		0x91

/*
 * Optional commands. 
 */

#define		IDE_IDENTIFY_DRIVE	0xec
#define		IDE_SET_MULTIPLE	0xc6
#define		IDE_READ_MULTIPLE	0xc4
#define		IDE_WRITE_MULTIPLE	0xc5
#define		IDE_READ_DMA		0xc8
#define		IDE_WRITE_DMA		0xca
#define		IDE_SET_FEATURES	0xef
#define		IDE_IDLE		0xe3
#define		IDE_SLEEP		0xe6

typedef	int ide_return_t;

/*
 * Return values for IDE commmands to the controller. 
 */

#define IDER_SUCCESS		0	/* OK */
#define IDER_TIMEOUT		1	/* cmd timed out */
#define IDER_MEMALLOC		2	/* couldn't allocate memory */
#define IDER_MEMFAIL		3	/* memory transfer error */
#define IDER_REJECT		4	/* bad field in ideIoReq_t */
#define IDER_BADDRV		5	/* drive not present */
#define IDER_CMD_ERROR		6 	/* basic command failure */
#define IDER_VOLUNAVAIL		7	/* Requested Volume not available */
#define IDER_SPURIOUS		8	/* spurious interrupt */
#define IDER_CNTRL_REJECT	9	/* controller has rejected */
#define IDER_RETRY		10	/* controller requests retrying */
#define IDER_DEV_NOT_READY	11	/* DRQ bit not set (status reg) */
#define IDER_DEV_BUSY		12	/* BUSY bit is set */
#define IDER_ERROR			13	/* other IDE error */
#define IDER_ABORT			14  /* ABRT bit is set */


/*
 * I/O request struct. Used in IDEIOCREQ ioctl to specify one command
 * sequence to be executed. 
 */

typedef struct _ideIoReq {

    /*
     * Inputs to driver 
     */
    unsigned 		cmd;	

    /* Starting block for IDE_READ, IDE_WRITE, IDE_READ_VERIFY, IDE_SEEK */
    unsigned 		block;	
    /* Number of blocks for IDE_READ, IDE_SEEK IDE_WRITE, IDE_READ_VERIFY */
    unsigned 		blkcnt;	

    /* Starting address for IDE_READ, IDE_WRITE and IDE_IDENTIFY_DRIVE */
    caddr_t	 	addr;	
									    
    unsigned 		timeout;		/* milliseconds */
    unsigned char	maxSectorsPerIntr; 	/* Only for IDE_SET_MULTIPLE */

    /*
     * Outputs from driver. 
     */

    ide_return_t	status;		/* IDER_SUCCESS, etc. */
    unsigned		blocks_xfered;  /* Blocks actually transfered */
    ideRegsVal_t 	regValues;	/* Valid if status is IDER_CMD_ERROR */
    unsigned 		diagResult; 	/* Result if cmd was IDE_DIAGNOSE */

    /*
     * Used internally by the driver. 
     */
#ifdef KERNEL
	struct vm_map   *map;		/* Map of requestor's task  */
#else
	void		*map;
#endif KERNEL

} ideIoReq_t;

/*
 * Ioctls specific to IDE. 
 */

#define IDEDIOCINFO			_IOR('i',1, struct _ideDriveInfo)

#define IDEDIOCREQ			_IOWR('i',2, struct _ideIoReq)
 
#define MAX_CMOS_IDETYPE		0x00ff
#define MIN_CMOS_IDETYPE		0x0001

#define MAX_BLOCKS_PER_XFER		256
#define IDE_SECTOR_SIZE			512 		/* bytes */
#define IDE_MAX_PHYS_IO			(MAX_BLOCKS_PER_XFER * IDE_SECTOR_SIZE)

/*
 * Structure returned by (optional) IDE_IDENTIFY_DRIVE command. Field
 * definitions from standard X3T9.2 791D Rev4 (17-Mar-93). Some of these
 * definitions have changed in proposed ATA-2 standard. 
 */
#define	IDE_MULTI_SECTOR_MASK		0x00ff

#define	IDE_CAP_LBA_SUPPORTED		0x0200
#define IDE_CAP_DMA_SUPPORTED		0x0100
#define IDE_CAP_IORDY_SUPPORTED		0x0800		/* from ATA-2 */

#define IDE_PIO_TIMING_MODE_MASK 	0xff00
#define IDE_DMA_TIMING_MODE_MASK 	0xff00

#define	IDE_TRANSLATION_VALID		0x0001  

#define	IDE_MULTI_SECTOR_VALID		0x0100
#define IDE_SECTORS_PER_INTERRUPT	0x00ff

#define IDE_SW_DMA_ACTIVE			0xff00
#define IDE_SW_DMA_SUPPORTED		0x00ff
#define IDE_MW_DMA_ACTIVE			0xff00
#define IDE_MW_DMA_SUPPORTED		0x00ff
#define IDE_UDMA_ACTIVE				0xff00
#define IDE_UDMA_SUPPORTED			0x00ff

#define IDE_FC_PIO_MODE_3_SUPPORTED	0x0001
#define IDE_FC_PIO_MODE_4_SUPPORTED	0x0002
#define IDE_FC_PIO_MODE_5_SUPPORTED	0x0004

/*
 * fieldValidity bit definitions.
 */
#define IDE_WORDS54_TO_58_SUPPORTED	0x0001
#define IDE_WORDS64_TO_70_SUPPORTED	0x0002
#define IDE_WORDS64_TO_68_SUPPORTED IDE_WORDS64_TO_70_SUPPORTED
#define IDE_WORD88_SUPPORTED		0x0004		/* UDMA support */

typedef	struct	_ideIdentifyInfo {
    unsigned short	genConfig;
    unsigned short	cylinders;
    unsigned short	reserved_2;
    unsigned short	heads;
    unsigned short	unformattedBytesPerTrack;		/* retired */
    unsigned short	unformattedBytesPerSector;		/* retired */
    unsigned short	sectorsPerTrack;
    unsigned short	vendorSpecific0[3];				/* retired */
    unsigned char	serialNumber[20];
    unsigned short	bufferType;						/* retired */
    unsigned short	bufferSize;						/* retired */
    unsigned short	eccBytes;						/* obsolete */
    unsigned char 	firmwareRevision[8];
    unsigned char	modelNumber[40];
    unsigned short	multipleSectors;
    unsigned short	doubleWordIO;					/* Vendor Unique */
    unsigned short	capabilities;
    
    unsigned short	capabilities2;
    unsigned short	pioDataTransferCyleTimingMode;
    unsigned short	dmaDataTransferCyleTimingMode;	/* retired */
    unsigned short	fieldValidity;
				    
    unsigned short	currentCylinders;
    unsigned short	currentHeads;
    unsigned short	currentSectorsPerTrack;
    unsigned short	capacity[2]; 	
    unsigned short	multiSectorInfo;
    unsigned 		userAddressableSectors;	 		/* LBA mode only */
    unsigned short	swDma;							/* retired */	
    unsigned short	mwDma;	
    unsigned short	fcPioDataTransferCyleTimingMode;
    unsigned short	minMwDMATransferCycleTimePerWord;
    unsigned short	RecommendedMwDMACycleTime;
    unsigned short	MinPIOTransferCycleTimeWithoutFlowControl;
    unsigned short	MinPIOTransferCycleTimeWithIORDY;
	
	/* ATA4/ATA3 additions */
	unsigned short	reserved_69_74[6];
	unsigned short	queueDepth;
	unsigned short	reserved_76_79[4];
	unsigned short	majorVersionNumber;
	unsigned short	minorVersionNumber;
	unsigned short	commandSetSupported[2];			/* word 82-83 */
	unsigned short	commandSetExtension;			/* word 84 */
	unsigned short	commandSetEnabled[2];			/* word 85-86 */
	unsigned short	commandSetDefault;				/* word 87 */
	unsigned short	UDma;							/* word 88 */
	unsigned short	timeForSecurityErase;			/* word 89 */
	unsigned short	timeForEnhancedErase;			/* word 90 */
	unsigned short	currentAPMValue;				/* word 91 */
	unsigned short	reserved_92_126[35];
	unsigned short	removableMediaStatusSupport;	/* word 127 */
	unsigned short	securityStatus;					/* word 128 */
	unsigned short	reserved_129_159[31];
	unsigned short	reserved_160_255[96];
} ideIdentifyInfo_t;

/*
 * Maximum Number of bytes of data that can be transfered via DMA using
 * IDEDIOCREQ ioctl command. 
 */

#ifdef KERNEL
#define IDE_MAX_DMA_SIZE			PAGE_SIZE 
#else KERNEL
#define IDE_MAX_DMA_SIZE			vm_page_size 
#endif KERNEL

/*
 * Register and mask definitions for IDE_SET_FEATURES command. 
 */

#define FEATURE_SET_TRANSFER_MODE			0x03
#define FEATURE_ENABLE_POWER_ON_DEFAULTS	0xcc
#define FEATURE_DISABLE_POWER_ON_DEFAULTS	0x66

#define PIO_FLOW_CONTROL_TM_MASK		0x000f
#define DMA_FLOW_CONTROL_TM_MASK		0x0017

#endif	_BSD_DEV_ATA_EXTERN_H

#endif DRIVER_PRIVATE
