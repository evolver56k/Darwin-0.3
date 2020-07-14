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


/* 	Copyright (c) 1997 Apple Computer, Inc.  All rights reserved. 
 *
 * kdebugprivate.h -   private kernel_debug definitions
 *
 */


/**********************************************************************/

typedef struct {
tvalspec_t	timestamp;
unsigned int	arg1;
unsigned int	arg2;
unsigned int	arg3;
unsigned int	arg4;
unsigned int	arg5;
unsigned int	debugid;
} kd_buf;

/* Debug Flags */
#define	KDBG_INIT	1
#define	KDBG_NOWRAP	2
#define	KDBG_FREERUN	4
#define	KDBG_WRAPPED	8
#define	KDBG_USERFLAGS	(KDBG_FREERUN|KDBG_NOWRAP|KDBG_INIT)

typedef struct {
	unsigned int	type;
	unsigned int	value1;
	unsigned int	value2;
	
} kd_regtype;

#define	KDBG_CLASSTYPE		0x10000
#define	KDBG_SUBCLSTYPE		0x20000
#define	KDBG_RANGETYPE		0x40000
#define	KDBG_TYPENONE		0x80000
#define KDBG_CKTYPES		0xF0000
#define	KDBG_RANGECHECK		0x100000

#define	KDBG_BUFINIT	0x80000000
/* Maximum number of buffer entries is 64k */

#define	KDBG_MAXBUFSIZE	(64*1024)

/* Control operations */
#define	KDBG_EFLAGS	1
#define	KDBG_DFLAGS	2
#define KDBG_ENABLE	3
#define KDBG_SETNUMBUF	4
#define KDBG_GETNUMBUF	5
#define KDBG_SETUP	6
#define KDBG_REMOVE	7
#define	KDBG_SETREGCODE	8
#define	KDBG_GETREGCODE	9
#define	KDBG_READTRACE	10

#define KDBGREGCALSS	1
#define KDBGREGSUBCALSS	2
#define KDBGREGRANGE	3
#define KDBGREGNONE	4
/**********************************************************************/

