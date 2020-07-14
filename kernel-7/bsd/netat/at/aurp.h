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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *		Created April 23, 1996, by Justin C. Walker
 *
 *	File: aurp.h
 */

/*
 * AURP device ioctl (I_STR) 'subcommands'
 */
#define AUC_CFGTNL	0	/* Configure Tunnels */
#define AUC_SHTDOWN	1	/* Shutdown AURP */
#define AUC_EXPNET	2	/* Configure exported networks */
#define AUC_HIDENET	3	/* Configure hidden netowrks */
#define AUC_UDPPORT	4	/* UPD Port number */
#define AUC_NETLIST	5	/* List of remote endpoints */
#define AUC_TIMER	6	/* Configured update interval timer */
#define AUC_ADDNET      7       /* Add remote endpoints */
#define AUC_ACCEPTALL   8       /* Accept all nets */
#define AUC_MAX         9       /* Maximun numbers of access nets */

/* Default AURP-over-UDP port */
#define AURP_SOCKNUM      387
#define AURP_MAXNETACCESS 64
