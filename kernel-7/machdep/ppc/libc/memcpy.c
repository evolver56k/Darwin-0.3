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

/* Copyright (c) 1997 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   libc/ppc/memcpy.c
 *      Author: Eryk Vershen, Apple Computer, Inc.
 *
 *      This file contains machine dependent code for block copies
 *      on PPC-based products.
 *
 * HISTORY
 */

void *memcpy(void *, const void *, unsigned long);

#if 0
void *memcpy(void *dst, const void *src, unsigned long ulen)
{
    char *odst = dst;
    int i = ulen;

    while (i > 0) {
	*((char*)dst)++ = *((char*)src)++;
	i--;
    }

    return odst;
}
#endif
