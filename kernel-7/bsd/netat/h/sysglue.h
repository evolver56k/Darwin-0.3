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
 * Title:	sysglue.h - AppleTalk protocol to streams interface
 *
 * Facility:	AppleTalk Protocol Execution Environment
 *
 * Author:	Gregory Burns, Creation Date: Jun-3-1988
 *
 ******************************************************************************
 *                                                                            *
 *        Copyright (c) 1988 Apple Computer, Inc.                             *
 *                                                                            *
 *        The information contained herein is subject to change without       *
 *        notice and  should not be  construed as a commitment by Apple       *
 *        Computer, Inc. Apple Computer, Inc. assumes no responsibility       *
 *        for any errors that may appear.                                     *
 *                                                                            *
 *        Confidential and Proprietary to Apple Computer, Inc.                *
 *                                                                            *
 ******************************************************************************
 *
 * History:
 * X01-001	Gregory Burns	3-Jun-1988
 *	 	Initial Creation.
 *
 */

#ifndef __SYSGLUE__
#define __SYSGLUE__

#include <h/debug.h>
#include <h/at-config.h>
#include <h/localglue.h> 

/* Some basics ... */

#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif

#ifdef _KERNEL
#define SYS_HZ HZ /* Number of clock (SYS_SETTIMER) ticks per second */
#else
#define OPEN   open
#define CLOSE  close
#define READ   read
#define WRITE  write
#define READV  readv
#define WRITEV writev
#define IOCTL  ioctl
#define SELECT select
#define ERRNO  errno
#endif

#ifdef RHAPSODY
#ifndef	I_STR
#define	I_STR	666
#endif
#else
#ifdef LITTLE_ENDIAN
#else
#define htonl(x) (x)
#define htons(x) (x)
#define ntohl(x) (x)
#define ntohs(x) (x)
#endif
#endif

#endif /* __SYSGLUE__ */
