/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IDE driver message logging -- IdeDDM.h
 *
 * HISTORY
 *
 * 1-Feb-1998	Joe Liu at Apple
 *	Added DDM_IDE_DMA.
 *
 * 22-Jul-1994	Rakesh Dubey at NeXT
 *	Created.
 */

#ifndef _IDEDDM_H
#define _IDEDDM_H 1


//#define DDM_DEBUG 		1

#import <driverkit/generalFuncs.h>

#ifdef	DDM_DEBUG
#define DDM_IODEVICE_INDEX 	0
#import <driverkit/debugging.h>

#define IDE_NUM_DDM_BUFS	256

/*
 * Should be different than other user drivers at DDM_IODEVICE_INDEX.
 * c.f. printerdriver/printerTypes.h.
 */
#define DDM_IDE_LOCK	0x00001000	/* lock */
#define DDM_IDE_CTL		0x00002000	/* controller */
#define DDM_IDE_CMD		0x00004000	/* IDE commands */
#define DDM_IDE_DISK	0x00008000	/* disk */
#define DDM_IDE_DMA		0x00000100	/* DMA ISA/PCI */
#define DDM_IDE_LOG		0x00000200	/* Generic log messages */
#define	DDM_IDE_ALL		(DDM_IDE_LOCK|DDM_IDE_CTL|DDM_IDE_DISK)

#define ddm_ide_lock(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_LOCK, x, a, b, c, d, e)
#define ddm_ide_ctl(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_CTL, x, a, b, c, d, e)
#define ddm_ide_cmd(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_CMD, x, a, b, c, d, e)
#define ddm_ide_disk(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_DISK, x, a, b, c, d, e)
#define ddm_ide_dma(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_DMA, x, a, b, c, d, e)
#define ddm_ide_log(x, a, b, c, d, e) \
    IODEBUG(DDM_IODEVICE_INDEX, DDM_IDE_LOG, x, a, b, c, d, e)

#else	DDM_DEBUG
#define ddm_ide_lock(x, a, b, c, d, e)
#define ddm_ide_ctl(x, a, b, c, d, e)
#define ddm_ide_cmd(x, a, b, c, d, e)
#define ddm_ide_disk(x, a, b, c, d, e)
#define ddm_ide_dma(x, a, b, c, d, e)
#define ddm_ide_log(x, a, b, c, d, e)
#endif	DDM_DEBUG

#endif /* _IDEDDM_H */
