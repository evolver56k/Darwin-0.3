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
/* Copyright (c) 1992 by NeXT Computer, Inc.
 *
 *	File:	i386/disk_label.c - trivial native (i386) implementation
 *		of disk_label conversion API.
 *
 * HISTORY
 * 01-Apr-92  Doug Mitchell at NeXT
 *	Created.
 */
 
#import <bsd/dev/disk_label.h>
#import <driverkit/diskstruct.h>
#import <bsd/string.h>
#import <architecture/byte_order.h>

static inline void get_char(const char *src, char *dest)
{
	*dest = *src;
}

static inline void get_short(const char *src, char *dest)
{
	dest[0] = src[0];
	dest[1] = src[1];
}

static inline void get_word(const char *src, char *dest)
{
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = src[3];
}

static inline void put_char(const char *src, char *dest)
{
	*dest = *src;
}

static inline void put_short(const char *src, char *dest)
{
	dest[0] = src[0];
	dest[1] = src[1];
}

static inline void put_word(const char *src, char *dest)
{
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = src[3];
}


/*
 * Convert from native (m68k) disk structs to m88k. Used when reading
 * from disk.
 */
void get_partition(const char *source, partition_t *dest)
{
	get_word(source + PARTITION_P_BASE, (char *)&dest->p_base);
	get_word(source + PARTITION_P_SIZE, (char *)&dest->p_size);
	get_short(source + PARTITION_P_BSIZE, (char *)&dest->p_bsize);
	get_short(source + PARTITION_P_FSIZE, (char *)&dest->p_fsize);
	get_char(source + PARTITION_P_OPT, (char *)&dest->p_opt);
	get_short(source + PARTITION_P_CPG, (char *)&dest->p_cpg);
	get_short(source + PARTITION_P_DENSITY, (char *)&dest->p_density);
	get_char(source + PARTITION_P_MINFREE, (char *)&dest->p_minfree);
	get_char(source + PARTITION_P_NEWFS, (char *)&dest->p_newfs);
	bcopy(source + PARTITION_P_MOUNTPT, dest->p_mountpt, MAXMPTLEN);
	get_char(source + PARTITION_P_AUTOMNT, (char *)&dest->p_automnt);
	bcopy(source + PARTITION_P_TYPE, dest->p_type, MAXFSTLEN);
}

void get_disktab(const char *source, disktab_t *dest)
{
	int part_num;
	int i;
	unsigned char *bblk_src, *bblk_dst;
	
	bcopy(source + DISKTAB_D_NAME, dest->d_name, MAXDNMLEN);
	bcopy(source + DISKTAB_D_TYPE, dest->d_type, MAXTYPLEN);
	get_word(source + DISKTAB_D_SECSIZE, (char *)&dest->d_secsize);
	get_word(source + DISKTAB_D_NTRACKS, (char *)&dest->d_ntracks);
	get_word(source + DISKTAB_D_NSECTORS, (char *)&dest->d_nsectors);
	get_word(source + DISKTAB_D_NCYLINDERS, (char *)&dest->d_ncylinders);
	get_word(source + DISKTAB_D_RPM, (char *)&dest->d_rpm);
	get_short(source + DISKTAB_D_FRONT, (char *)&dest->d_front);
	get_short(source + DISKTAB_D_BACK, (char *)&dest->d_back);
	get_short(source + DISKTAB_D_NGROUPS, (char *)&dest->d_ngroups);
	get_short(source + DISKTAB_D_AG_SIZE, (char *)&dest->d_ag_size);
	get_short(source + DISKTAB_D_AG_ALTS, (char *)&dest->d_ag_alts);
	get_short(source + DISKTAB_D_AG_OFF, (char *)&dest->d_ag_off);
	if (NBOOTS){
		bblk_src = (unsigned char *)(source + DISKTAB_D_BOOT0_BLKNO);
		bblk_dst = (unsigned char *)&dest->d_boot0_blkno;
		for (i = 0; i < (NBOOTS) ; i++){
			get_word(bblk_src, bblk_dst);
			bblk_src += sizeof(int);
			bblk_dst += sizeof(int);
		}
	}
	bcopy(source + DISKTAB_D_BOOTFILE, dest->d_bootfile, MAXBFLEN);
	bcopy(source + DISKTAB_D_HOSTNAME, dest->d_hostname, MAXHNLEN);
	get_char(source + DISKTAB_D_ROOTPARTITION, 
		 (char *)&dest->d_rootpartition);
	get_char(source + DISKTAB_D_RWPARTITION, (char *)&dest->d_rwpartition);
	for(part_num=0; part_num<NPART; part_num++) {
		get_partition(source + DISKTAB_D_PARTITIONS + 
			(part_num * SIZEOF_PARTITION_T),
			&dest->d_partitions[part_num]);
	}
}

void get_dl_un(const char *source, dl_un_t *dest)
{
	int i;
	
	for(i=0; i<NBAD; i++) {
		get_word(source, (char *)&dest->DL_bad[i]);
		source += sizeof(int);
	}
}

void get_disk_label(const char *source, disk_label_t *dest)
{
	get_word(source + DISK_LABEL_DL_VERSION, (char *)&dest->dl_version);
	get_word(source + DISK_LABEL_DL_LABEL_BLKNO, 
		(char *)&dest->dl_label_blkno);
	get_word(source + DISK_LABEL_DL_SIZE, (char *)&dest->dl_size);
	bcopy(source + DISK_LABEL_DL_LABEL, dest->dl_label, MAXLBLLEN);
	get_word(source + DISK_LABEL_DL_FLAGS, (char *)&dest->dl_flags);
	get_word(source + DISK_LABEL_DL_TAG, (char *)&dest->dl_tag);
	get_disktab(source + DISK_LABEL_DL_DT, &dest->dl_dt);
	get_dl_un(source + DISK_LABEL_DL_UN, &dest->dl_un);
	get_short(source + DISK_LABEL_DL_CHECKSUM, (char *)&dest->dl_checksum);
	get_short(source + DISK_LABEL_DL_UN, (char *)&dest->dl_v3_checksum);
}

/*
 * Convert from m88k disk structs to native (m68k). Used when writing
 * to disk.
 */
void put_partition(const partition_t *source, char *dest)
{
	put_word((const char *)&source->p_base, dest + PARTITION_P_BASE);
	put_word((const char *)&source->p_size, dest + PARTITION_P_SIZE);
	put_short((const char *)&source->p_bsize, dest + PARTITION_P_BSIZE);
	put_short((const char *)&source->p_fsize, dest + PARTITION_P_FSIZE);
	put_char((const char *)&source->p_opt, dest + PARTITION_P_OPT);
	put_short((const char *)&source->p_cpg, dest + PARTITION_P_CPG);
	put_short((const char *)&source->p_density, dest + 
		PARTITION_P_DENSITY);
	put_char((const char *)&source->p_minfree, dest + PARTITION_P_MINFREE);
	put_char((const char *)&source->p_newfs, dest + PARTITION_P_NEWFS);
	bcopy(source->p_mountpt, dest + PARTITION_P_MOUNTPT, MAXMPTLEN);
	put_char((const char *)&source->p_automnt, dest + PARTITION_P_AUTOMNT);
	bcopy(source->p_type, dest + PARTITION_P_TYPE, MAXFSTLEN);
}

void put_disktab(const disktab_t *source, char *dest)
{
	int part_num;
	unsigned char *bblk_src, *bblk_dst;
	int i;
	
	bcopy(source->d_name, dest + DISKTAB_D_NAME, MAXDNMLEN);
	bcopy(source->d_type, dest + DISKTAB_D_TYPE, MAXTYPLEN);
	put_word((const char *)&source->d_secsize, dest + DISKTAB_D_SECSIZE);
	put_word((const char *)&source->d_ntracks, dest + DISKTAB_D_NTRACKS);
	put_word((const char *)&source->d_nsectors, dest + DISKTAB_D_NSECTORS);
	put_word((const char *)&source->d_ncylinders, dest + 
		DISKTAB_D_NCYLINDERS);
	put_word((const char *)&source->d_rpm, dest + DISKTAB_D_RPM);
	put_short((const char *)&source->d_front, dest + DISKTAB_D_FRONT);
	put_short((const char *)&source->d_back, dest + DISKTAB_D_BACK);
	put_short((const char *)&source->d_ngroups, dest + DISKTAB_D_NGROUPS);
	put_short((const char *)&source->d_ag_size, dest + DISKTAB_D_AG_SIZE);
	put_short((const char *)&source->d_ag_alts, dest + DISKTAB_D_AG_ALTS);
	put_short((const char *)&source->d_ag_off, dest + DISKTAB_D_AG_OFF);
	if (NBOOTS){
		bblk_dst = (unsigned char *)(dest + DISKTAB_D_BOOT0_BLKNO);
		bblk_src = (unsigned char *)&source->d_boot0_blkno;
		for (i = 0; i < (NBOOTS) ; i++){
			put_word(bblk_src, bblk_dst);
			bblk_src += sizeof(int);
			bblk_dst += sizeof(int);
		}
	}
	bcopy(source->d_bootfile, dest + DISKTAB_D_BOOTFILE, MAXBFLEN);
	bcopy(source->d_hostname, dest + DISKTAB_D_HOSTNAME, MAXHNLEN);
	put_char((const char *)&source->d_rootpartition,
		 dest + DISKTAB_D_ROOTPARTITION);
	put_char((const char *)&source->d_rwpartition, 
		dest + DISKTAB_D_RWPARTITION);
	for(part_num=0; part_num<NPART; part_num++) {
		put_partition(&source->d_partitions[part_num],
			dest + DISKTAB_D_PARTITIONS + 
			(part_num * SIZEOF_PARTITION_T));
	}
}

/*
 * Warning - this doesn't copy checksum properly. Caller must do that.
 */
void put_dl_un(const dl_un_t *source, char *dest) 
{
	int i;
	
	for(i=0; i<NBAD; i++) {
		put_word((const char *)&source->DL_bad[i], dest);
		dest += sizeof(int);
	}
}

void put_disk_label(const disk_label_t *source, char *dest)
{
	put_word((const char *)&source->dl_version, 
		dest + DISK_LABEL_DL_VERSION);
	put_word((const char *)&source->dl_label_blkno,
		dest + DISK_LABEL_DL_LABEL_BLKNO);
	put_word((const char *)&source->dl_size, dest + DISK_LABEL_DL_SIZE);
	bcopy(source->dl_label, dest + DISK_LABEL_DL_LABEL, MAXLBLLEN);
	put_word((const char *)&source->dl_flags, dest + DISK_LABEL_DL_FLAGS);
	put_word((const char *)&source->dl_tag, dest + DISK_LABEL_DL_TAG);
	put_disktab(&source->dl_dt, dest + DISK_LABEL_DL_DT);
	put_dl_un(&source->dl_un, dest + DISK_LABEL_DL_UN);
	put_short((const char *)&source->dl_checksum, 
		dest + DISK_LABEL_DL_CHECKSUM);
	put_short((const char *)&source->dl_v3_checksum, 
		dest + DISK_LABEL_DL_UN);
}
