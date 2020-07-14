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

#import "objc-config.h"

#if defined (m68k)
#if defined(OBJC_COLLECTING_CACHE)
#include "objc-msg-m68k-nolock.s"
#elif  defined(OBJC_COPY_CACHE)
#include "objc-msg-m68k-copycache-lock.s"
#else
#include "objc-msg-m68k-lock.s"
#endif

#elif defined (__i386__)
#if defined(OBJC_COLLECTING_CACHE)
#include "objc-msg-i386-nolock.s"
#elif  defined(OBJC_COPY_CACHE)
#include "objc-msg-i386-copycache-lock.s"
#else
#include "objc-msg-i386-lock.s"
#endif

#elif defined (__ppc__)
#include "objc-msg-ppc.s"

#elif defined (hppa)
#if defined(OBJC_COLLECTING_CACHE)
#include "objc-msg-hppa-nolock.s"
#elif defined(OBJC_COPY_CACHE)
#include "objc-msg-hppa-copycache-lock.s"
#else
#include "objc-msg-hppa-lock.s"
#endif

#elif defined (sparc)
#include "objc-msg-sparc.s"
#else
#error Architecture not supported
#endif
