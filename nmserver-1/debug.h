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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_DEBUG_
#define	_DEBUG_

#include 	<mach/mach.h>
#include	<mach/cthreads.h>
#include 	<servers/nm_defs.h>

#include	<stdio.h>

#ifdef __svr4__
#include	<string.h>
#else
#include	<strings.h>
#endif

#ifndef WIN32
#include	<syslog.h>
#endif

#include	"config.h"
#include	"ls_defs.h"

extern	stat_t		nmstat;
extern	debug_t		debug;
extern	param_t		param;
extern	int		debug_flag;
extern	int		local_flag;
extern	int		secure_flag;
extern	int		nevernet_flag;

#if	NM_STATISTICS
#define INCSTAT(field) nmstat.field++
#else	NM_STATISTICS
#define INCSTAT(field)
#endif	NM_STATISTICS

#define INCPORTSTAT(pr_ptr,field)

/*
 * The panic() routine is to be called when an unrecoverable error occurs. 
 * It will cause the server to dump core (iff properly configured).
 */
#define panic(message) { char* dummy = (char*)1; *dummy = message[0]; }	


/*
 * ERROR prints a message on stderr and puts it into the log.
 */
#ifdef WIN32

#undef ERROR		/* Already defined in wingdi.h */

#define	ERROR(args) {			\
	char msg[200];			\
	(void)sprintf args;		\
	(void)fprintf(stderr,msg);	\
	(void)fprintf(stderr,"\n");	\
	(void)fflush(stderr);		\
}
#else WIN32
#define	ERROR(args) {			\
	char msg[200];			\
	(void)sprintf args;		\
	(void)fprintf(stderr,msg);	\
	(void)fprintf(stderr,"\n");	\
	(void)fflush(stderr);		\
	if (param.syslog)		\
		syslog(LOG_ERR,msg);	\
}
#endif WIN32


#define	LOGCHECK

//#ifdef DEBUG
//#define debug(x) { if (debug_flag) printf x ; fflush(stdout); }
//#else
//#define debug(x)
//#endif

#endif	_DEBUG_

