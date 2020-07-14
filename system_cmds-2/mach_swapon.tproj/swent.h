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
 * swent.h - Header file for swaptab entries.
 *
 * Copyright (c) 1989 by NeXT, Inc.
 *
 **********************************************************************
 * HISTORY
 * 28-Feb-89  Peter King (king) at NeXT
 *	Created.
 **********************************************************************
 */

/*
 * Swaptab entry
 */
struct swapent {
	char		*sw_file;
	unsigned int	sw_prefer : 1;
	unsigned int	sw_noauto : 1;
	unsigned int 	sw_nocompress : 1;
	unsigned int	sw_lowat;
	unsigned int	sw_hiwat;
};
typedef struct swapent *swapent_t;
#define SWAPENT_NULL (swapent_t)0

/*
 * Swapent related routines:
 *
 * swent_start	- Sets up to read through swaptab entries.
 * swent_get	- Gets next swaptab entry, returns NULL when done.
 * swent_rele	- Release swaptab entry returned by swent_get().
 * swent_end	- Cleans up after reading through swaptab entries.
 * swent_parseopts - Parses options in string "str" into "sw".
 */
int		swent_start(char *);
swapent_t	swent_get();
void		swent_rele(swapent_t);
void		swent_end();
void		swent_parseopts(char *str, swapent_t sw);

