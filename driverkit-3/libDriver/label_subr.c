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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * label_subr.c - machine-independent disk label routines
 *
 * HISTORY
 * 01-Apr-92    Doug Mitchell at NeXT
 *      Created.
 */
#import <bsd/sys/param.h> 
#import <bsd/dev/disk_label.h>
#import <driverkit/disk_label.h>
#import <driverkit/diskstruct.h>
#import <architecture/byte_order.h>


/*
 * Inlines used to manipulate m68k-style disk label fields.
 */
static inline unsigned int
get_word(const void *src)
{
	return ( NXSwapBigIntToHost(*((unsigned int *)src)));
}

static inline void
put_word(unsigned s, void *dest)
{
	*((unsigned int *)dest) = NXSwapHostIntToBig(s);
}


static inline unsigned short
get_short(const void *src)
{
	return ( NXSwapBigShortToHost(*((unsigned short *)src)));
}

static inline void
put_short(unsigned short s, void *dest)
{
	*((unsigned short *)dest) = NXSwapHostShortToBig(s);
}

/*
 * Machine-independent version of kernel's checksum_16().
 * *wp is a raw m68k-style label.
 */
unsigned short 
checksum16(unsigned short *wp, int num_shorts)
{
	int sum1 = 0;
	int sum2;

	while (num_shorts--){
		sum1 += NXSwapBigShortToHost(*wp);
		wp++;
	}
	sum2 = ((sum1 & 0xffff0000) >> 16) + (sum1 & 0xffff);
	if(sum2 > 65535) {
		sum2 -= 65535;
	}
	return sum2;
}

/*
 * Validate an m68k-style label. Returns NULL if good, else returns pointer
 * to an error string describing what is wrong with the label.
 */
char *
check_label(char *raw_label,			// as it came off disk
	int block_num)	
{
	unsigned short	sum;
	unsigned short 	cksum;
	unsigned short 	size;
	void		*dl_cksump;
	int		 version;
	int 		label_blkno;
	
	version = get_word(raw_label + DISK_LABEL_DL_VERSION);
	if(version == DL_V1 || version == DL_V2) {
		size = SIZEOF_DISK_LABEL_T;
		dl_cksump = raw_label + DISK_LABEL_DL_CHECKSUM;
	} 
	else if(version == DL_V3) {
		size = SIZEOF_DISK_LABEL_T - SIZEOF_DL_UN_T;
		dl_cksump = raw_label + DISK_LABEL_DL_UN;
	} 
	else {
		return "Bad disk label magic number";
	}
	label_blkno = get_word(raw_label + DISK_LABEL_DL_LABEL_BLKNO);
	if(label_blkno != block_num) {
		return "Label in wrong location";
	}
		
	/*
	 * Block number in label has to be 0 for checksum calculation.
	 */
	put_word(0, raw_label + DISK_LABEL_DL_LABEL_BLKNO);
	cksum = get_short(dl_cksump);
	put_short(0, dl_cksump);
	sum = checksum16((unsigned short *)raw_label, size >> 1);
	if (sum != cksum) {
		return "Label checksum error";
	}
	
	/*
	 * Success. Restore block number.
	 */
	put_word(block_num, raw_label + DISK_LABEL_DL_LABEL_BLKNO);
	put_short(cksum, dl_cksump);
	return NULL;
}
