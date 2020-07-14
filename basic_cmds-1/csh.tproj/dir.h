/* 
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * Copyright (c) 1980, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 *
 *	@(#)dir.h	8.1 (Berkeley) 5/31/93
 */

/*
 * Structure for entries in directory stack.
 */
struct directory {
    struct directory *di_next;	/* next in loop */
    struct directory *di_prev;	/* prev in loop */
    unsigned short *di_count;	/* refcount of processes */
    Char   *di_name;		/* actual name */
};
extern struct directory *dcwd;		/* the one we are in now */
