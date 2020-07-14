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
/* objc-config.h created by kthorup on Fri 24-Mar-1995 */

#ifndef KERNEL
#ifndef SHLIB
#define RUNTIME_DYLD 1
#endif
#endif

/* Turn on support for class refs. */
#define OBJC_CLASS_REFS

#if defined(hppa) || defined (i386) || defined (m68k)
#if !defined(KERNEL) && !defined(SHLIB)
#define OBJC_COLLECTING_CACHE
#endif
#endif

#ifdef FREEZE
#define __S(x) __objcopt ## x
#else
#define __S(x) x
#endif

