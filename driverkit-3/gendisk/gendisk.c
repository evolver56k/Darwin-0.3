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
 *	File:	gendisk.c - Generate offsets for disk-resident structs.
 *
 * HISTORY
 * 28-Mar-92  Doug Mitchell at NeXT
 *	Created.
 *
 * NeXT disks are native to the m68k architecture. When writing or reading
 * structs to or from disks on other architectures, m68k alignment and 
 * byte ordering must be preserved. This program generates raw byte offsets
 * from an m68k point of view for the following types:
 *
 * 	disk_label_t
 *	disktab_t
 *	partition_t
 *	
 * This program must run on an m68k machine.
 */

#import <stdio.h>
#import <ctype.h>
#import <libc.h>
#import <bsd/dev/disk.h>

#ifndef	MACRO_BEGIN
# define		MACRO_BEGIN	do {
#endif	MACRO_BEGIN

#ifndef	MACRO_END
# define		MACRO_END	} while (0)
#endif	MACRO_END

#ifndef	MACRO_RETURN
# define		MACRO_RETURN	if (1) return
#endif	MACRO_RETURN

#define	NAME_LEN	30

#define	PRINT_OFFSET(ptr_type, field)					\
MACRO_BEGIN								\
	print_define(#ptr_type, #field);				\
	printf("%#010x\n", (& (((ptr_type)0)->field)));			\
MACRO_END

#define	PRINT_BIT_FIELD(reg_type, field)				\
MACRO_BEGIN								\
	reg_type __reg;							\
	__reg.field = (typeof (__reg.field)) -1;			\
	print_define(#reg_type, #field);				\
	printf("%#010x\n", CONTENTS(__reg));				\
MACRO_END

#define	PRINT_FIELD_INFO(reg_type, field)				\
MACRO_BEGIN								\
	reg_type __reg;							\
	CONTENTS(__reg) = 0;						\
	__reg.field = -1;						\
	print_define(#reg_type, #field "_OFF");				\
	printf("%d\n", bit_num(#reg_type, #field, CONTENTS(__reg)));	\
	print_define(#reg_type, #field "_WIDTH");			\
	printf("%d\n", field_width(#reg_type, #field, CONTENTS(__reg)));\
MACRO_END

#define	PRINT_ENUM(item)						\
MACRO_BEGIN								\
	print_define("", #item);					\
	printf("%#010x\n", item);					\
MACRO_END

#define	PRINT_DEFINE(macro)						\
MACRO_BEGIN								\
	print_define("", #macro);					\
	printf("%s\n", STRINGIFY(macro));				\
MACRO_END

#define	PRINT_CONSTANT(macro)						\
MACRO_BEGIN								\
	print_define("", #macro);					\
	printf("%#010x\n", macro);					\
MACRO_END

#define	PRINT_REGADDR(macro)						\
MACRO_BEGIN								\
	print_define("", #macro);					\
	printf("%#010x\n", &macro);					\
MACRO_END

#define	PRINT_REG_PAIR(struct_ptr, name0, name1)			\
MACRO_BEGIN								\
	print_define(#struct_ptr, #name0 "_" #name1);			\
	printf("%#010x\n", (& (((struct_ptr)0)->name0##_##name1)));	\
MACRO_END

#define	PRINT_BIT_POS(reg_type, field)					\
MACRO_BEGIN								\
	reg_type __reg;							\
	CONTENTS(__reg) = 0;						\
	__reg.field = 1;						\
	print_define(#reg_type, #field "_BIT");				\
	printf("%d\n", bit_num(#reg_type, #field, CONTENTS(__reg)));	\
MACRO_END

#define	PRINT_L2_SIZE(type)						\
MACRO_BEGIN								\
	print_define("L2_SIZE", #type);					\
	printf("%d\n", log2(sizeof(type), #type));			\
MACRO_END

#define	PRINT_L2_CONSTANT(macro)					\
MACRO_BEGIN								\
	print_define("L2", #macro);					\
	printf("%d\n", log2(macro, #macro));				\
MACRO_END

#define	PRINT_SIZEOF(type)						\
MACRO_BEGIN								\
	print_define("SIZEOF", #type);					\
	printf("%d\n", sizeof(type), #type);				\
MACRO_END


char *progname;

unsigned bit_num(char *reg_type, char *field, unsigned bits)
{
	unsigned bit;
	unsigned mask;
	
	for (bit = 0, mask = 0x1;
	  (mask & bits) == 0 && mask;
	  mask <<= 1, bit += 1)
		continue;
	if (mask)
		return bit;
	fprintf(stderr, "%s: Bad BIT_POS for %s.%s\n", progname,
	  reg_type, field);
	exit(1);
	return 0;
}

unsigned field_width(char *reg_type, char *field, unsigned bits)
{
	unsigned width;

	while (bits && (bits & 0x1) == 0)
		bits >>= 1;
	for (width = 0; (bits & 0x1) == 1; bits >>= 1, width += 1)
		continue;
	if (bits == 0 && width)
		return width;
	fprintf(stderr, "%s: Bad BIT_FIELD for %s.%s\n", progname,
	  reg_type, field);
	exit(1);
	return 0;
}

unsigned log2(unsigned val, char *type)
{
	unsigned l2 = 0;

	if (val == 0) {
		fprintf(stderr, "log2: sizeof(%s) is zero!\n", type);
		exit(1);
	}
	while ((val & 0x1) == 0) {
		l2 += 1;
		val >>= 1;
	}
	if (val != 0x1) {
		fprintf(stderr, "log2: sizeof(%s) is not power of two!\n",
		  type);
		exit(1);
	}
	return l2;
}

const char *skip_white(const char *cp)
{
	while (*cp && isspace(*cp))
		cp += 1;
	return cp;
}

const char *strip_prefix(const char *cp, const char *prefix)
{
	int len;

	cp = skip_white(cp);
	len = strlen(prefix);
	if (strncmp(cp, prefix, len) && isspace(*(cp+len)))
		cp += len;
	return cp;
}

void print_define(char *type_name, char *field)
{
	const char *cp;
	int col = 0;
	
	printf("#define\t");
	if (type_name != NULL && *type_name != '\0') {
		cp = strip_prefix(type_name, "struct");
		cp = strip_prefix(cp, "enum");
		cp = skip_white(cp);
		for (; *cp; cp++) {
			if (isspace(*cp))
				break;
			if (*cp == '*')
				break;
			if (strncmp(cp, "_t", 2) == 0 && !isalnum(cp[2]))
				break;
			putchar(isalpha(*cp) ? toupper(*cp) : *cp);
			col += 1;
				
		}
		putchar('_');
		col += 1;
	}
	for (cp = field; *cp; cp++) {
		if (*cp == '.')
			putchar('_');
		else
			putchar(isalpha(*cp) ? toupper(*cp) : *cp);
		col += 1;
	}
	do {
		putchar(' ');
		col += 1;
	} while (col < NAME_LEN);
}

typedef enum {
	MAJOR, MINOR
} cmt_level_t;

void comment(cmt_level_t level, const char *cmt)
{
	switch (level) {
	case MAJOR:
		printf("\n\n");
		printf("//\n");
		printf("// %s\n", cmt);
		printf("//\n");
		break;
	case MINOR:
		printf("\n");
		printf("// %s\n", cmt);
		printf("\n");
		break;
	default:
		fprintf(stderr, "%s: Bad comment level\n", progname);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	progname = argv[0];
	
	printf("// diskstruct.h -- generated by gendisk.c\n");
	printf("// DON'T EDIT THIS -- change gendisk.c\n");
	
	comment(MAJOR, "Structure Offsets");
	
	comment(MINOR, "disk_label_t offsets");
	PRINT_OFFSET(disk_label_t *, dl_version);
	PRINT_OFFSET(disk_label_t *, dl_label_blkno);
	PRINT_OFFSET(disk_label_t *, dl_size);
	PRINT_OFFSET(disk_label_t *, dl_label);
	PRINT_OFFSET(disk_label_t *, dl_flags);
	PRINT_OFFSET(disk_label_t *, dl_tag);
	PRINT_OFFSET(disk_label_t *, dl_dt);
	PRINT_OFFSET(disk_label_t *, dl_un);
	PRINT_OFFSET(disk_label_t *, dl_checksum);
	PRINT_SIZEOF(disk_label_t);
	PRINT_SIZEOF(dl_un_t);
	
	comment(MINOR, "disktab_t offsets");
	PRINT_OFFSET(disktab_t *, d_name);
	PRINT_OFFSET(disktab_t *, d_type);
	PRINT_OFFSET(disktab_t *, d_secsize);
	PRINT_OFFSET(disktab_t *, d_ntracks);
	PRINT_OFFSET(disktab_t *, d_nsectors);
	PRINT_OFFSET(disktab_t *, d_ncylinders);
	PRINT_OFFSET(disktab_t *, d_rpm);
	PRINT_OFFSET(disktab_t *, d_front);
	PRINT_OFFSET(disktab_t *, d_back);
	PRINT_OFFSET(disktab_t *, d_ngroups);
	PRINT_OFFSET(disktab_t *, d_ag_size);
	PRINT_OFFSET(disktab_t *, d_ag_alts);
	PRINT_OFFSET(disktab_t *, d_ag_off);
	PRINT_OFFSET(disktab_t *, d_boot0_blkno);
	PRINT_OFFSET(disktab_t *, d_bootfile);
	PRINT_OFFSET(disktab_t *, d_hostname);
	PRINT_OFFSET(disktab_t *, d_rootpartition);
	PRINT_OFFSET(disktab_t *, d_rwpartition);
	PRINT_OFFSET(disktab_t *, d_partitions);
	PRINT_SIZEOF(disktab_t);
	
	comment(MINOR, "partition_t offsets");
	PRINT_OFFSET(partition_t *, p_base);
	PRINT_OFFSET(partition_t *, p_size);
	PRINT_OFFSET(partition_t *, p_bsize);
	PRINT_OFFSET(partition_t *, p_fsize);
	PRINT_OFFSET(partition_t *, p_opt);
	PRINT_OFFSET(partition_t *, p_cpg);
	PRINT_OFFSET(partition_t *, p_density);
	PRINT_OFFSET(partition_t *, p_minfree);
	PRINT_OFFSET(partition_t *, p_newfs);
	PRINT_OFFSET(partition_t *, p_mountpt);
	PRINT_OFFSET(partition_t *, p_automnt);
	PRINT_OFFSET(partition_t *, p_type);
	PRINT_SIZEOF(partition_t);
	
	return 0;
}
