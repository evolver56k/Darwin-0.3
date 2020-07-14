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
	File:		HubClassVersion.h

	Contains:	Version number values for the Hub driver

	Version:	xxx put version here xxx

	Written by:	Craig Keithley

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				

		Other Contact:		

		Technology:			

	Writers:

		(BT)	Barry Twycross
		(CJK)	Craig Keithley

    This file is used in these builds:  

	Change History (most recent first):

	  <USB6>	 5/13/98	CJK		fix version info to get version from packageversion.h if the
									individual version has been overridden
		 <5>	  3/9/98	BT		Fix redundant ;
		 <4>	  3/6/98	CJK		fix version info as it appears in the getinfo dialog
		 <3>	  2/5/98	CJK		Fix defines to be for hub (not compound class device)
		 <2>	  2/5/98	CJK		Add change history
		 <1>	  2/5/98	CJK		First time check in
*/
#ifndef __HUBCLASSVERSION__
#define __HUBCLASSVERSION__

#include	"PackageVersion.h"

#ifdef kOverrideIndividualVersions

#define kHUBStringVersShort		kPKGStringVersShort
#define kHUBStringVersLong		kPKGStringVers1Long

#define kHUBHexMajorVers		kPKGHexMajorVers					
#define kHUBHexMinorVers		kPKGHexMinorVers		
#define kHUBCurrentRelease		kPKGCurrentRelease			
#define kHUBReleaseStage		kPKGReleaseStage							

#else

#define kHUBStringVersShort		"1.0d1"
#define kHUBStringVersLong		"1.0d1, © 1998 Apple Computer, Inc."

#define kHUBHexMajorVers		0x01						// This should never change for the current project.
#define kHUBHexMinorVers		0x00						// This should never change for the current project.
#define kHUBCurrentRelease		kDevelopmentRelease			// Be sure to adjust this when the milestone is reached.
#define kHUBReleaseStage		1							// Be sure to adjust this number every build.

#endif

#endif