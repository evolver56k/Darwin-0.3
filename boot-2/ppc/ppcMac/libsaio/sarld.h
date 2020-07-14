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
 * These two variables are set in sa_rld() and used in layout_segments()
 * as the place to put the output in memory.
 */
extern char 		*sa_rld_output_addr;
extern unsigned long sa_rld_output_size;

/*
 * sa_rld() loads the specified object in memory against the specified
 * base file in memory.  The output is placed in memory starting at
 * the value of the parameter workmem_addr and the size of the memory
 * used for the output returned indirectly through workmem_size.
 * Initially *workmem_size is the size of the working memory.
 */
typedef int sa_rld_t(
char			   *basefile_name,	/* base file name */
struct mach_header *basefile_addr,	/* mach header of the base file */

char			   *object_name,	/* name of the object to load */
char			   *object_addr,	/* addr of the object in memory to load */
unsigned long		object_size,	/* size of the object in memory to load */

char			   *workmem_addr,	/* address of working memory */
unsigned long	   *workmem_size,	/* size of working memory (in/out) */

char			   *error_buf_addr, /* address of error message buffer */
unsigned long		error_buf_size, /* size of error message buffer */

char			   *malloc_addr,	/* address to use for initializing malloc */
unsigned long		malloc_len);	/* length to use for same */

extern sa_rld_t sa_rld;

