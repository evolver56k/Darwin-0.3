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
	File:		CompositeClassVersion.h

	Contains:	Version number values for the Composite Class Driver

	Version:	xxx put version here xxx

	Written by:	Craig Keithley

	Copyright:	©1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		

		Technology:			

	Writers:

		(CJK)	Craig Keithley

    This file is used in these builds:  

	Change History (most recent first):

	  <USB3>	 5/13/98	CJK		fix version info to get version from packageversion.h if the
									individual version has been overridden
	  <USB2>	 5/11/98	CJK		remove semicolon after include statement
		 <1>	  4/7/98	CJK		first checked in
		 <3>	  3/6/98	CJK		change to use the override individual versions flag to determine
									whether the version info comes from the packageversion file or
									from this file.
		 <2>	  3/5/98	CJK		Work on version string formatting (so that the correct strings
									appear when doing a get info).
*/
#ifndef __COMPOSITEVERSION__
#define __COMPOSITEVERSION__

//naga#include	"PackageVersion.h"

#ifdef kOverrideIndividualVersions

#define kCMPStringVersShort		kPKGStringVersShort
#define kCMPStringVersLong		kPKGStringVers1Long

#define kCMPHexMajorVers		kPKGHexMajorVers					
#define kCMPHexMinorVers		kPKGHexMinorVers		
#define kCMPCurrentRelease		kPKGCurrentRelease			
#define kCMPReleaseStage		kPKGReleaseStage						

#else

#define kCMPStringVersShort		"1.0d1"
#define kCMPStringVersLong		"1.0d1, © 1998 Apple Computer, Inc."

#define kCMPHexMajorVers		0x01						// This should never change for the current project.
#define kCMPHexMinorVers		0x00						// This should never change for the current project.
#define kCMPCurrentRelease		kDevelopmentRelease			// Be sure to adjust this when the milestone is reached.
#define kCMPReleaseStage		1							// Be sure to adjust this number every build.

#endif

#endif
