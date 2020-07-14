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
	File:		PackageVersion.h

	Contains:	Version number values for the entire package

	Version:	xxx put version here xxx

	Written by:	Craig Keithley

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		

		Technology:			

	Writers:

		(CJK)	Craig Keithley

    This file is used in these builds:  

	Change History (most recent first):

	  <USB5>	 5/13/98	CJK		fix version info to get version from packageversion.h if the
									individual version has been overridden
	  <USB4>	 5/11/98	CJK		remove semicolon after include statement
		 <3>	  3/6/98	CJK		change so that the check for the override of the version occurs
									here, and not in the .r file.
		 <2>	  3/5/98	CJK		Fix version string problems
*/
#ifndef __MOUSEMODULEVERSION__
#define __MOUSEMODULEVERSION__

#include	"../kbd/PackageVersion.h"

#ifdef kOverrideIndividualVersions

#define kMouseStringVersShort		kPKGStringVersShort
#define kMouseStringVersLong		kPKGStringVers1Long

#define kMouseHexMajorVers			kPKGHexMajorVers					
#define kMouseHexMinorVers			kPKGHexMinorVers		
#define kMouseCurrentRelease		kPKGCurrentRelease			
#define kMouseReleaseStage			kPKGReleaseStage						

#else

#define kMouseStringVersShort		"1.0d1"
#define kMouseStringVersLong		"1.0d1, © 1998 Apple Computer, Inc."

#define kMouseHexMajorVers			0x01						// This should never change for the current project.
#define kMouseHexMinorVers			0x00						// This should never change for the current project.
#define kMouseCurrentRelease		kDevelopmentRelease			// Be sure to adjust this when the milestone is reached.
#define kMouseReleaseStage			1							// Be sure to adjust this number every build.

#endif

#endif
