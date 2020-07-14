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
	File:		UCStringComparePriv.h

	Contains:	xxx put contents here xxx

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(djb)	Don Brady
		(msd)	Mark Day

	Change History (most recent first):

	   <CS1>	 4/24/97	djb		first checked in
	  <HFS1>	 2/13/97	msd		first checked in
*/

/* File: UCStringComparePriv.h */

#if TARGET_OS_MAC
#include <Types.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

typedef SInt32 UCCollateResult;

typedef OptionBits UCCollateOptions;
enum {
	kUCCollateComposeSensitiveBit	= 0,
	kUCCollateCaseSensitiveBit= 1
};
enum {
	kUCCollateComposeSensitiveMask = 1L << kUCCollateComposeSensitiveBit,
	kUCCollateCaseSensitiveMask	= 1L << kUCCollateCaseSensitiveBit
};

/* Return values follow standard C library conventions */

/* strcmp */
extern UCCollateResult UCSimpleCompareStrings (
	const UniChar str1[], ItemCount length1,
	const UniChar str2[], ItemCount length2,
	UCCollateOptions options );

extern Boolean UCSimpleEqualStrings (
	const UniChar str1[], ItemCount length1,
	const UniChar str2[], ItemCount length2,
	UCCollateOptions options );

/* strstr */
extern Boolean UCSimpleFindString (
	const UniChar src[], ItemCount srcLength,
	const UniChar sub[], ItemCount subLength,
	UCCollateOptions options );
