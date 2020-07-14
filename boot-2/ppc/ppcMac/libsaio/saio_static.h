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
 * Definitions of functions used by saio internal clients
 */
 
#import "bitmap.h"
#import "sarld.h"

/* bootstruct.c */
extern int numIDEs(void);
extern void getKernBootStruct(void);

/* gets.c */
extern int Gets(
    char *buf,
    int len,
    int timeout,
    char *prompt,
    char *message
);

/* load.c */
extern int loadStandaloneLinker(
    char *linkerPath,
    sa_rld_t **rld_entry_p
);

void startprog(unsigned int address);

/* disk.c */
extern void devopen(char *name, struct iob *io);
extern int devread(struct iob *io);
extern void devflush(void);

/* ufs_byteorder.c */
extern void byte_swap_superblock(struct fs *sb);
extern void byte_swap_inode_in(struct dinode *dc, struct dinode *ic);
extern void byte_swap_dir_block_in(char *addr, int count);
