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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 *	machdep/ppc/ast.h: Definitions for ppc ast mechanism.
 *
 *
 * HISTORY
 *
 * 4-APR-1997  Umesh Vaishampayan
 *	Created.
 */

#ifndef	_MACHDEP_PPC_AST_H_
#define _MACHDEP_PPC_AST_H_

#include <kern/ast.h>

/* Need to change this is kern/ast.h defines more bits */

/* should be 0x3F ??? */
#define AST_ALL	(	AST_UNIX	|	\
			AST_NETIPC	|	\
			AST_NETWORK	|	\
			AST_BLOCK	|	\
			AST_TERMINATE	|	\
			AST_HALT	)

#endif /* _MACHDEP_PPC_AST_H_ */
