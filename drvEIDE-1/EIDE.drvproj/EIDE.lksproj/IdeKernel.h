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
 * IdeKernel.h - UNIX front end for kernel IDE Disk driver.
 *
 * HISTORY
 * 27-Jul-94    Rakesh Dubey at NeXT
 *      Created. 
 */

#ifndef _IDEKERNEL_H
#define _IDEKERNEL_H 1

__private_extern__ int ideopen(dev_t dev, int flag, int devtype, struct proc * pp);
__private_extern__ int ideclose(dev_t dev, int flag, int devtype, struct proc * pp);
__private_extern__ int ideread(dev_t dev, struct uio *uiop, int ioflag);
__private_extern__ int idewrite(dev_t dev, struct uio *uiop, int ioflag);
__private_extern__ void idestrategy(register struct buf *bp);
__private_extern__ int ideioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc * pp);
__private_extern__ int idesize(dev_t dev);
__private_extern__ void ide_block_char_majors(int *blockmajor, int *charmajor);

extern IONamedValue iderValues[];

#endif /* _IDEKERNEL_H */
