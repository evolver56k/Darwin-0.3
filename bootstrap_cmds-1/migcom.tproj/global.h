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
/*	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/global.h,v 1.1.1.1 1999/04/15 18:30:36 wsanchez Exp $	*/
/*
 * HISTORY
 *  4-Sep-91  Gregg Kellogg (gk) at NeXT
 *	Added SendTime to operate like WaitTime.
 *
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 03-Jan-89  Trey Matteson (trey@next.com)
 *	Removed init_<sysname> function
 *
 * 17-Sep-87  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Added GenSymTab
 *
 * 16-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added CamelotPrefix
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_GLOBAL_H
#define	_GLOBAL_H

#define	EXPORT_BOOLEAN
#include <mach/boolean.h>
#include <sys/types.h>
#include "string.h"

extern boolean_t BeQuiet;	/* no warning messages */
extern boolean_t BeVerbose;	/* summarize types, routines */
extern boolean_t UseMsgRPC;
extern boolean_t GenSymTab;
#if	NeXT
extern boolean_t GenHandler;	/* Generate a handler interface */
#endif	NeXT

extern boolean_t IsKernel;
extern boolean_t IsCamelot;

extern string_t RCSId;

extern string_t SubsystemName;
extern u_int SubsystemBase;

extern string_t MsgType;
extern string_t WaitTime;
#if	NeXT
extern string_t SendTime;
#endif	NeXT
extern string_t ErrorProc;
extern string_t ServerPrefix;
extern string_t UserPrefix;
extern char CamelotPrefix[4];

extern int yylineno;
extern string_t yyinname;

extern void init_global();

extern string_t HeaderFileName;
extern string_t UserFileName;
extern string_t ServerFileName;
#if	NeXT
extern string_t ServerHeaderFileName;
#endif	NeXT

extern identifier_t SetMsgTypeName;
extern identifier_t ReplyPortName;
extern identifier_t ReplyPortIsOursName;
extern identifier_t MsgTypeVarName;
extern identifier_t DeallocPortRoutineName;
extern identifier_t AllocPortRoutineName;
extern identifier_t ServerProcName;

extern void more_global();

extern char NewCDecl[];
extern char LintLib[];

#endif	_GLOBAL_H
