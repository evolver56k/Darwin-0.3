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

/* Copyright 1997 Apple Computer, Inc.
 *
 * History :
 * 18-Sep-1997  Umesh Vaishampayan (umeshv@apple.com)
 *	Created.
 *
 */

#if defined(GPROF)

/*
 * The compiler generates calls to this function and passes address
 * of caller of the function [ from which mcount is called ] as the 
 * first parameter.
 */

        .text
		.align 4
		.globl mcount
mcount:
        mflr r0
        stw r0,8(r1)
        stwu r1,-64(r1)
        mr r4, r0
        bl	_mcount			; Call the C routine
        addi r1,r1,64
        lwz r0,8(r1)
        mtlr r0
        blr

#endif /* GPROF */

