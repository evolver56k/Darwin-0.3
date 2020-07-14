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
 * Intel386 Family:	Machine dependent exception codes.
 *
 * HISTORY
 *
 * 13 April 1992 ? at NeXT
 *	Created.
 */
 
/*
 * EXC_BAD_ACCESS
 */
#define EXC_I386_PAGE_FAULT		14

/*
 * EXC_BAD_INSTRUCTION
 */
#define EXC_I386_INVALID_OPCODE		6
#define EXC_I386_SEGMENT_NOTPRESENT	11
#define EXC_I386_STACK_EXCEPTION	12
#define EXC_I386_GENERAL_PROTECTION	13

/*
 * EXC_ARITHMETIC
 */
#define EXC_I386_ZERO_DIVIDE		0
#define EXC_I386_EXTENSION_FAULT	16

/*
 * EXC_EMULATION
 */
#define EXC_I386_NOEXTENSION		7

/*
 * EXC_SOFTWARE
 */
#define EXC_I386_OVERFLOW		4
#define EXC_I386_BOUNDS_CHECK		5
#define EXC_I386_ALIGNMENT_CHECK	17

/*
 * EXC_BREAKPOINT
 */
#define EXC_I386_DEBUG			1
#define EXC_I386_BREAKPOINT		3
