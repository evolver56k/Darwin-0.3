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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 * disk.h
 * 	Apple partition map definitions and data structures
 *
 * DPME = Disk Partition Map Entry
 *
 * Each entry is (and shall remain) 512 bytes long.
 * For more information see:
 *	"Inside Macintosh: Devices" pages 3-12 to 3-15.
 *	"Inside Macintosh - Volume V" pages V-576 to V-582
 *	"Inside Macintosh - Volume IV" page IV-292
 *
 * HISTORY
 *
 * 11 Jun 97	Dieter Siegmund at Apple.
 *	Taken from Eryk Vershen's dpme.h.
 */
/*
 * Copyright 1996 by Apple Computer, Inc.
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

#define	BLOCK0_SIGNATURE	0x4552	/* Signature value.         */
#define	DPISTRLEN	32
#define	DPME_SIGNATURE	0x504D

#define RHAPSODY_PART_TYPE "Apple_Rhapsody_UFS"	/* native partition type */

typedef	unsigned char	u8;
typedef	unsigned short	u16;
typedef	unsigned long	u32;

// Physical block zero of the disk has this format
struct Block0 {
    u16 	sbSig;		/* unique value for SCSI block 0 */
    u16 	sbBlkSize;	/* block size of device */
    u32 	sbBlkCount;	/* number of blocks on device */
    u16 	sbDevType;	/* device type */
    u16 	sbDevId;	/* device id */
    u32 	sbData;		/* not used */
    u16 	sbDrvrCount;	/* driver descriptor count */
    u16 	sbMap[247];	/* descriptor map */
};
typedef struct Block0 Block0;

struct DDMap {
    u32 	ddBlock;	/* 1st driver's starting block */
    u16 	ddSize;		/* size of 1st driver (512-byte blks) */
    u16 	ddType;		/* system type (1 for Mac+) */
};
typedef struct DDMap DDMap;

// Each partition map entry (blocks 1 through n) has this format
struct dpme {
    u16     dpme_signature          ;
    u16     dpme_reserved_1         ;
    u32     dpme_map_entries        ;
    u32     dpme_pblock_start       ;
    u32     dpme_pblocks            ;
    char    dpme_name[DPISTRLEN]    ;  /* name of partition */
    char    dpme_type[DPISTRLEN]    ;  /* type of partition */
    u32     dpme_lblock_start       ;
    u32     dpme_lblocks            ;
    u32     dpme_flags;
#if 0
    u32     dpme_reserved_2    : 23 ;  /* Bit 9 through 31.        */
    u32     dpme_os_specific_1 :  1 ;  /* Bit 8.                   */
    u32     dpme_os_specific_2 :  1 ;  /* Bit 7.                   */
    u32     dpme_os_pic_code   :  1 ;  /* Bit 6.                   */
    u32     dpme_writable      :  1 ;  /* Bit 5.                   */
    u32     dpme_readable      :  1 ;  /* Bit 4.                   */
    u32     dpme_bootable      :  1 ;  /* Bit 3.                   */
    u32     dpme_in_use        :  1 ;  /* Bit 2.                   */
    u32     dpme_allocated     :  1 ;  /* Bit 1.                   */
    u32     dpme_valid         :  1 ;  /* Bit 0.                   */
#endif
    u32     dpme_boot_block         ;
    u32     dpme_boot_bytes         ;
    u8     *dpme_load_addr          ;
    u8     *dpme_load_addr_2        ;
    u8     *dpme_goto_addr          ;
    u8     *dpme_goto_addr_2        ;
    u32     dpme_checksum           ;
    char    dpme_process_id[16]     ;
    u32     dpme_boot_args[32]      ;
    u32     dpme_reserved_3[62]     ;
};
typedef struct dpme DPME;

