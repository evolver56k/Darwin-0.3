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
/* Memory addresses used by booter and friends */

/* These are all "virtual" addresses...
 * which are physical addresses plus MEMBASE.
 */

#define MEMBASE 		0x0

#define KERNEL_ADDR 			0x000000
#define KERNEL_LEN				0x3E0000
//#define RLD_MEM_ADDR			0x500000
//#define RLD_MEM_LEN 			0x100000
#define ZALLOC_ADDR 			0xf00000
#define ZALLOC_LEN				0x200000
#define MODULE_ADDR 			0x700000		// to be used for compression..
#define MODULE_LEN				0x080000
#define KSYM_ADDR				0x780000
#define KSYM_LEN				0x080000		// 512k

/* these are physical values */

#define KERNEL_BOOT_ADDR		KERNEL_ADDR 	/* load at 1Mb */

#define ptov(paddr) 	((paddr) - MEMBASE)
#define vtop(vaddr) 	((vaddr) + MEMBASE)

/*
 * Limits to the size of various things...
 */

/* We need a minimum of 8Mb of physical memory. */
#define MIN_EXT_MEM_KB	(7 * 1024)

/* To use compression, we have to decompress the disk image of the kernel
 * into memory.  Therefore, the size of the uncompressed kernel image
 * can't be bigger than half the size of the area set aside for loading it.
 */
#define MAX_KERNEL_IMAGE_SIZE	(KERNEL_LEN / 2)
