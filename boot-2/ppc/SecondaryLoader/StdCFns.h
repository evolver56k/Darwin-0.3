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
	File:		StdCFns.h

	Contains:	

	Copyright:	© 1996 by Apple Computer, Inc., all rights reserved.

	Version:	Maxwell

	File Ownership:

		DRI:				Alan Mimms

		Other Contact:		Stanford Au

		Technology:			MacOS

	Writers:

		(ABM)	Alan Mimms

	Change History (most recent first):

		 <2>	10/11/96	ABM		Add min, max and abs.
		 <1>	  3/5/96	ABM		First checked in.
		<1+>	  3/1/96	ABM		Merge into tertiary loader
*/

#ifndef STDCFNS_H
#define STDCFNS_H	1

#include <stdarg.h>

#if defined(__MWERKS__) || defined(__GNUC__)
#if !defined(_SIZE_T) && !defined(_T_SIZE_) && !defined(_T_SIZE)
typedef long size_t;
#endif
#endif

extern size_t strlen (const char *str);
extern char *strcat (char *str1, const char *str2);
extern char *strncat (char *str1, const char *str2, size_t count);
extern char *strcpy (char *destStr, const char *srcStr);
extern char *strncpy (char *destStr, const char *srcStr, size_t length);
extern char *strchr (const char *str, int chr);
extern int strcmp (const char *str1, const char *str2);

/* memcmp and memcpy are builtin in GNUC */
#ifndef __GNUC__
extern int memcmp (const void *s1, const void *s2, size_t n);
extern void *memcpy (void *str1, const void *str2, size_t count);
#endif

extern void *memset (void *str, int c, size_t count);
extern int strncmp (const char *str1, const char *str2, size_t length);

extern void exit (int code);

extern int printf (char const *fmt, ...);
extern int sprintf (char *str, char const *fmt, ...);
extern int vsprintf(char *str, char const *fmt, va_list args);
extern int strcmpCaseInsensitive (const char *s1, const char *s2);
extern long StrToInt (char *string, char **ptr, int base);

extern int min (int a, int b);
extern int max (int a, int b);
extern int abs (int a);

#endif

