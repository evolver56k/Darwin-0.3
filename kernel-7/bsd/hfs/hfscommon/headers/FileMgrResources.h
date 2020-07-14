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
	File:		FileMgrResources.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(djb)	Don Brady
		(DSH)	Deric Horn

	Change History (most recent first):

	   <CS5>	 8/12/97	djb		Add new string constants for Disk Init strings.
	   <CS4>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS3>	  7/8/97	DSH		Added rTextEncodingConverterName to strings for
									InitializeUnicode() call.
	   <CS2>	 6/27/97	DSH		Using "real" ID range allocated to us by Dave Lyons.
	   <CS1>	 6/24/97	DSH		first checked in
*/

//	These are temporary until we get the real ID's from the Allocations Project

//
//	HFS+ resource IDs
//	Our range is	'TEXT' -20573 -> -20575
//					'STR#' -20573 -> -20575

#define	kBaseHFSPlusResourceID		-20573
#define	rEnglishOnlyResD			kBaseHFSPlusResourceID
#define	rLocalizedResID				kBaseHFSPlusResourceID - 1


#define	kFileNameStringListID			-20573
#define	rEnglishReadMeName					 1
#define	rLocalizedReadMeName				 2
#define	rTextEncodingConverterName			 3


#define	kFormatStringListID				-20574
#define	kHFSFormatStringIndex				 1
#define	kHFSPlusFormatStringIndex			 2
#define	kHFS800KAltFormatStrIndex			 3
#define	kHFS1440KAltFormatStrIndex			 4
#define	kHFSAltFormatStrIndex				 5
#define	kHFSPlusAltFormatStrIndex			 6


