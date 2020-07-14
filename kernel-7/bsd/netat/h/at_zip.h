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
 *	Copyright (c) 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* "@(#)at_zip.h: 2.0, 1.6; 7/5/89; Copyright 1988-89, Apple Computer, Inc." */

typedef struct {
	char		command;
	char		flags;
	at_net		cable_range_start;
	at_net		cable_range_end;
	u_char		data[1];
} at_x_zip_t;

#define	ZIP_X_HDR_SIZE	6

/* flags for ZipNetInfoReply packet */
#define	ZIP_ZONENAME_INVALID	0x80
#define	ZIP_USE_BROADCAST	0x40
#define	ZIP_ONE_ZONE		0x20

#define	ZIP_NETINFO_RETRIES	3
#define	ZIP_TIMER_INT		HZ	/* HZ defined in param.h */

/* ZIP control codes */
#define	ZIP_ONLINE		1
#define ZIP_LATE_ROUTER		2
#define	ZIP_NO_ROUTER		3

#define ZIP_RE_AARP		-1
