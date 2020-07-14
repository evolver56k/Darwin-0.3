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
 * i486 Test register declarations.
 *
 * HISTORY
 *
 * 1 September 1992 ? at NeXT
 *	Created.
 */

typedef struct _tr6 {
    unsigned int	c	:1,
#define i486_TR6_C_WRITE	0
#define i486_TR6_C_LOOKUP	1
    				:4,
			w	:2,
#define i486_TR6_B_MATCH_NONE	0
#define i486_TR6_B_MATCH_CLR	1
#define i486_TR6_B_MATCH_SET	2
#define i486_TR6_B_MATCH_ANY	3
			u	:2,
			d	:2,
			v	:1,
			linear	:20;
} tr6_t;

typedef struct _tr7 {
    unsigned int		:2,
    			rep	:2,
			pl	:1,
				:2,
			lru	:3,
			pwt	:1,
			pcd	:1,
			phys	:20;
} tr7_t;
