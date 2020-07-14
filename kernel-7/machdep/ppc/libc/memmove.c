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
 * Copyright (c) 1991,1993,1998 Apple Computer, Inc. All rights reserved.
 * 
 *	File:	machdep/ppc/libc/memmove.c
 *	History:
 *
 */

#include <ppc/bcopy.h>

#if	ORG_BCOPY
#if USE_FAST_BCOPY
extern void fast_bcopy(const char *src, char *dst, unsigned int ulen);
#endif
#endif	/* ORG_BCOPY */

#if	ORG_BCOPY
void bcopy(const char *src, char *dst, unsigned int ulen)
#else	/* ORG_BCOPY */
void tjm_bcopy(const char *src, char *dst, unsigned int ulen)
#endif	/* ORG_BCOPY */
{
	if (ulen < 0)
		return;
		
	if (src > dst || dst > src + ulen) {
#if USE_FAST_BCOPY
		fast_bcopy(src, dst, ulen);
#else
		while (ulen-- > 0) 
			*dst++ = *src++;
#endif /* USE_FAST_BCOPY */
	}
	else {
		dst += (ulen - 1);
		src += (ulen - 1);
		while (ulen-- > 0)
			*dst-- = *src--;
	}
	return;
}

void ovbcopy(const char *src, char *dst, unsigned int ulen)
{
	bcopy(src, dst, ulen);
}

#if USE_FAST_BCOPY
#else
void *memcpy(void *dst, const void *src, unsigned int ulen)
{
	bcopy(src, dst, ulen);
	return dst;
}
#endif

void *memmove(void *dst, const void *src, unsigned int ulen)
{
	bcopy(src, dst, ulen);
	return dst;
}


