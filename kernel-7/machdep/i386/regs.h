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
 * Intel386 Family:	General processor registers.
 *
 * HISTORY
 *
 * 30 March 1992 ? at NeXT
 *	Created.
 */
 
#import <architecture/i386/sel.h>

/*
 * Format of saved registers on
 * the kernel stack.
 */

typedef struct regs {
    sel_t		gs;
    unsigned int		:0;
    sel_t		fs;
    unsigned int		:0;
    sel_t		es;
    unsigned int		:0;
    sel_t		ds;
    unsigned int		:0;
    unsigned int	edi;
    unsigned int	esi;
    unsigned int	ebp;
    unsigned int	xxx;	/* actually except_frame_t * */
    unsigned int	ebx;
    unsigned int	edx;
    unsigned int	ecx;
    unsigned int	eax;
} regs_t;
