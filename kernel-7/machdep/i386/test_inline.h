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
 * i486 Test register inlines.
 *
 * HISTORY
 *
 * 1 September 1992 ? at NeXT
 *	Created.
 */

#import <machdep/i386/test.h>

static inline
void
set_tr6(
    tr6_t	value
)
{
    asm volatile("
    	mov %0,%%tr6"
	    :
	    : "r" (value));
}

static inline
tr7_t
tr7(
    void
)
{
    tr7_t	value;
    
    asm volatile("
    	mov %%tr7,%0"
	    : "=r" (value));
	    
    return (value);
}

static inline
void
set_tr7(
    tr7_t	value
)
{
    asm volatile("
    	mov %0,%%tr7"
	    :
	    : "r" (value));
}
