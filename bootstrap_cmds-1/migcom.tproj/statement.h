/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/statement.h,v 1.1.1.1 1999/04/15 18:30:37 wsanchez Exp $	*/
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_STATEMENT_H
#define	_STATEMENT_H

#include "routine.h"

typedef enum statement_kind
{
    skRoutine,
    skImport,
    skUImport,
    skSImport,
    skRCSDecl,
} statement_kind_t;

typedef struct statement
{
    statement_kind_t stKind;
    struct statement *stNext;
    union
    {
	/* when stKind == skRoutine */
	routine_t *_stRoutine;
	/* when stKind == skImport, skUImport, skSImport */
	string_t _stFileName;
    } data;
} statement_t;

#define	stRoutine	data._stRoutine
#define	stFileName	data._stFileName

#define stNULL		((statement_t *) 0)

/* stNext will be initialized to put the statement in the list */
extern statement_t *stAlloc();

/* list of statements, in order they occur in the .defs file */
extern statement_t *stats;

#endif	_STATEMENT_H
