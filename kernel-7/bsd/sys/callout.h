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
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 * 
 */

#ifndef	__CALLOUT__
#define __CALLOUT__

typedef int (*func)(void *);

#define CALLOUT_PRI_SOFTINT0	0
#define CALLOUT_PRI_SOFTINT1	1
#define CALLOUT_PRI_RETRACE	2
#define CALLOUT_PRI_DSP		3
#define CALLOUT_PRI_THREAD	4	/* run in a thread */
#define CALLOUT_PRI_NOW		5	/* must be last */
#define N_CALLOUT_PRI		6

#endif /* __CALLOUT__ */
