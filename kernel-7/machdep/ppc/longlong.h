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
 * Routines to optimize some forms of long-long math.
 *
 * Copyright (c) 1992-1993 NeXT, Inc.
 *
 * HISTORY
 *
 *  16Aug93 Mac Gillon at NeXT
 *	hppa version
 *  
 *  23Nov92 Mac Gillon at NeXT
 *      68K version
 *
 *  1Nov92 Brian Pinkerton at NeXT
 *	Created.
 */

static inline void
llp_div_l(unsigned long long *ll, unsigned int divisor, unsigned int *remain)
{

	unsigned long long	copy = *ll;
	
	*ll = copy / divisor;
	*remain = (unsigned int) (copy % divisor);

}


static inline unsigned int
ll_div_l(unsigned long long ll, unsigned int divisor)
{
	return (unsigned int) (ll / divisor);

}
