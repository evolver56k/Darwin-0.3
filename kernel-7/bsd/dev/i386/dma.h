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
 * Dummy file for now.
 *
 * HISTORY
 *
 * 9 July 1992 ? at NeXT
 *	Created.
 */

/*	@(#)dma.h	1.0	08/12/87	(c) 1987 NeXT	*/

/* 
 **********************************************************************
 * HISTORY
 *
 * 26-Sep-89  John Seamons (jks) at NeXT
 *	Made dd_csr in struct dma_dev an int (and changed the corresponding
 *	register #defines) because this register responds as a 32-bit device
 *	and therefore needs to be accessed this way to work on the 68040.
 *
 * 12-Apr-88  John Seamons (jks) at NeXT
 *	Changes to support DMA chip 313.
 *
 * 12-Aug-87  Mike DeMoney (mike) at NeXT
 *	Created.
 *
 **********************************************************************
 */ 

/*
 * dma.h -- description of NeXT dma structures
 */
 
#if	KERNEL_PRIVATE

#ifndef	_I386_DEV_DMA_
#define	_I386_DEV_DMA_

#define	DMA_BEGINALIGNMENT	1 
#define	DMA_ENDALIGNMENT	1

#define	DMA_BEGINALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_BEGINALIGNMENT-1) \
		&~(DMA_BEGINALIGNMENT-1)))

#define	DMA_ENDALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_ENDALIGNMENT-1) \
		&~(DMA_ENDALIGNMENT-1)))
#define	DMA_BEGINALIGNED(addr)	(((unsigned)(addr)&(DMA_BEGINALIGNMENT-1))==0)
#define	DMA_ENDALIGNED(addr)	(((unsigned)(addr)&(DMA_ENDALIGNMENT-1))==0)
#define	DMA_LONGALIGNED(addr)	(((unsigned)(addr)&(DMA_LONGALIGNMENT-1))==0)

#endif	/* _I386_DEV_DMA_ */

#endif
