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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * dma_buf_internal.h   -- DMA buffer handling internals.
 *
 * HISTORY
 *
 * 26 August 1992 ? at NeXT
 *	Created.
 */

typedef union dma_buf_region {
    unsigned char		data[0];
    union dma_buf_region	*next;
} dma_buf_region_t;
 
typedef struct dma_buf_type {
    dma_buf_region_t *		(*create_fcn)();
    dma_buf_region_t *		free_head;
    int				free_count;
    int				total_count;
} dma_buf_type_t;

static dma_buf_region_t		*dma_buf_sm_create(
							void);
static dma_buf_region_t		*dma_buf_lg_create(
							void);
