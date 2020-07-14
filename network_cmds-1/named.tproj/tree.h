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
/* tree.h - declare structures used by tree library
 *
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 * vix 27jun86 [broken out of tree.c]
 *
 * $Id: tree.h,v 1.1.1.1 1999/05/02 03:57:47 wsanchez Exp $
 */


#ifndef	_TREE_H_INCLUDED
#define	_TREE_H_INCLUDED


#ifndef __P
# if defined(__STDC__) || defined(__GNUC__)
#  define __P(x) x
# else
#  define __P(x) ()
# endif
#endif

/*
 * tree_t is our package-specific anonymous pointer.
 */
#if defined(__STDC__) || defined(__GNUC__)
typedef	void *tree_t;
#else
typedef	char *tree_t;
#endif


typedef	struct tree_s {
		tree_t		data;
		struct tree_s	*left, *right;
		short		bal;
	}
	tree;


void	tree_init	__P((tree **));
tree_t	tree_srch	__P((tree **, int (*)(), tree_t));
tree_t	tree_add	__P((tree **, int (*)(), tree_t, void (*)()));
int	tree_delete	__P((tree **, int (*)(), tree_t, void (*)()));
int	tree_trav	__P((tree **, int (*)()));
void	tree_mung	__P((tree **, void (*)()));


#endif	/* _TREE_H_INCLUDED */
