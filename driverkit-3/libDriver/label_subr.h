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
 * label_subr.h - machine-independent disk label routines.
 *
 * HISTORY
 * 01-Apr-92    Doug Mitchell at NeXT
 *      Created.
 */

/*
 * Validate an m68k-style label. Returns 0 if good, else returns pointer
 * to an error string describing what is wrong with the label.
 */
char *check_label(char *raw_label,	// as it came off disk
	int block_num);			// physical block # of start of label

/*
 * Machine-independent version of kernel's checksum_16().
 * *wp is a raw m68k-style label.
 */
unsigned short checksum16(unsigned short *wp, int num_shorts);
