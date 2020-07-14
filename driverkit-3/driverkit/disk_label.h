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
 *	File:	disk_label.h - Convert between m68k and machine-dependent
 *		disk_label_t.
 *
 * HISTORY
 * 28-Mar-92  Doug Mitchell at NeXT
 *	Created.
 */
 
#import <bsd/dev/disk_label.h>

/*
 * Convert from native (m68k) disk structs to target. Used when reading
 * from disk.
 */
extern void get_partition(const char *source, partition_t *dest);
extern void get_disktab(const char *source, disktab_t *dest);
extern void get_dl_un(const char *source, dl_un_t *dest);
extern void get_disk_label(const char *source, disk_label_t *dest);


/*
 * Convert from target disk structs to native (m68k). Used when writing
 * to disk.
 */
extern void put_partition(const partition_t *source, char *dest);
extern void put_disktab(const disktab_t *source, char *dest);
extern void put_dl_un(const dl_un_t *source, char *dest);
extern void put_disk_label(const disk_label_t *source, char *dest);

