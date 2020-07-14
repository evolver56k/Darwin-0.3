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
/*
 *	objc-dispatch.h
 *	Copyright 1988, NeXT, Inc.
 */

#ifndef _OBJC_DISPATCH_H_
#define _OBJC_DISPATCH_H_

#import "objc.h"

#define GETFRAME(firstArg)	((MSG)&(((MSG*)(&firstArg))[-2/*-3*/]))
#define PREVSELF(firstArg)	(GETFRAME(firstArg)->next->_xxARGS.receiver)
#define PREVSEL(firstArg)	(GETFRAME(firstArg)->next->_xxARGS.cmd)

/* Data-type alignment constants.  Usage is:  while(addr&alignment)addr++ */
#define _A_CHAR		0x0
#define _A_SHORT	0x1
#define _A_INT		0x1
#define _A_LONG		0x1
#define _A_FLOAT	0x1
#define _A_DOUBLE	0x1
#define _A_POINTER	0x1
#define NBITS_CHAR	8
#define NBITS_INT	(sizeof(int)*NBITS_CHAR)

/* Stack Frame layouts */

typedef struct _MSG {			/* Stack frame layout (for messages) */
	struct _MSG *next;		/* Link to next frame */
	/*void *saved_a1;		    GNU uses this for struct returns */
	id (*ret)();			/* Return address from subroutine */
	struct _ARGFRAME {
		id receiver;		/* ID of receiver */
		char *cmd;		/* Message selector */
		int arg;		/* First message argument */
	} _xxARGS;
} *MSG;

typedef struct _MSGSUPER {
	struct _MSG *next;		/* Link to next frame */
	/*void *saved_a1;		    GNU uses this for struct returns */
	id (*ret)();			/* Return address from subroutine */
	struct {
		struct objc_super *caller;
		char *cmd;		/* Message selector */
		int arg;		/* First message argument */
	} _xxARGS;
} *MSGSUPER;

#endif /* _OBJC_DISPATCH_H_ */
