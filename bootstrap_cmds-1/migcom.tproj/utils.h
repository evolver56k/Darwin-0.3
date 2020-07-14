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
/* 	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/utils.h,v 1.1.1.1 1999/04/15 18:30:38 wsanchez Exp $	*/
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_UTILS_H
#define	_UTILS_H

/* stuff used by more than one of header.c, user.c, server.c */

extern void WriteImport(/* FILE *file, string_t filename */);
extern void WriteRCSDecl(/* FILE *file, identifier_t name, string_t rcs */);
extern void WriteBogusDefines(/* FILE *file */);

extern void WriteList(/* FILE *file, argument_t *args,
			 void (*func)(FILE *file, argument_t *arg),
			 u_int mask, char *between, char *after */);

#if	NeXT
extern void WriteListSkipFirst(/* FILE *file, argument_t *args,
			 void (*func)(FILE *file, argument_t *arg),
			 u_int mask, char *between, char *after */);
#endif	NeXT

extern void WriteReverseList(/* FILE *file, argument_t *args,
				void (*func)(FILE *file, argument_t *arg),
				u_int mask, char *between, char *after */);

/* good as arguments to WriteList */
extern void WriteNameDecl(/* FILE *file, argument_t *arg */);
extern void WriteVarDecl(/* FILE *file, argument_t *arg */);
extern void WriteServerVarDecl();
extern void WriteTypeDecl(/* FILE *file, argument_t *arg */);
extern void WriteCheckDecl(/* FILE *file, argument_t *arg */);

extern char *ReturnTypeStr(/* routine_t *rt */);

extern char *FetchUserType(/* ipc_type_t *it */);
extern char *FetchServerType(/* ipc_type_t *it */);
extern void WriteFieldDeclPrim(/* FILE *file, argument_t *arg,
				  char *(*tfunc)(ipc_type_t *it) */);

extern void WriteStructDecl(/* FILE *file, argument_t *args,
			       void (*func)(FILE *file, argument_t *arg),
			       u_int mask, char *name */);

extern void WriteStaticDecl(/* FILE *file, ipc_type_t *it,
			       boolean_t dealloc, boolean_t longform,
			       identifier_t name */);

extern void WriteCopyType(/* FILE *file, ipc_type_t *it,
			     char *left, char *right, ... */);

extern void WritePackMsgType(/* FILE *file, ipc_type_t *it,
				boolean_t dealloc, boolean_t longform,
				char *left, char *right, ... */);

#endif	_UTILS_H
