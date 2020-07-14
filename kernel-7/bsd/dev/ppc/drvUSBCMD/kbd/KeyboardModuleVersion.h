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
	File:		KeyboardModuleVersion.h

	Contains:	Version number values for the Keyboard HID Module

	Version:	xxx put version here xxx

	Written by:	Craig Keithley

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		

		Technology:			

	Writers:

		(CJK)	Craig Keithley

    This file is used in these builds:  

	Change History (most recent first):

	  <USB7>	 5/13/98	CJK		fix version info to get version from packageversion.h if the
									individual version has been overridden
	  <USB6>	 5/11/98	CJK		remove semicolon after include statement
	  <USB5>	  4/9/98	CJK		change copyright date (bad clone, bad bad clone)
		 <4>	  3/6/98	CJK		change so that the override of the version occurs in this file,
									and not in the .r file.
		 <3>	  3/5/98	CJK		change defines to indicate that they're for the keyboard hid
									module.
		 <2>	 2/10/98	CJK		Correct change history (to reflect the keyboard module changes)
		 <1>	 2/10/98	CJK		First time check in.  Cloned from Mouse HID Module
*/
#ifndef __KBDMODULEVERSION__
#define __KBDMODULEVERSION__

#include	"PackageVersion.h"

#ifdef kOverrideIndividualVersions

#define kKBDStringVersShort		kPKGStringVersShort
#define kKBDStringVersLong		kPKGStringVers1Long

#define kKBDHexMajorVers		kPKGHexMajorVers					
#define kKBDHexMinorVers		kPKGHexMinorVers		
#define kKBDCurrentRelease		kPKGCurrentRelease			
#define kKBDReleaseStage		kPKGReleaseStage							

#else

#define kKBDStringVersShort		"1.0d1"
#define kKBDStringVersLong		"1.0d1, © 1998 Apple Computer, Inc."

#define kKBDHexMajorVers		0x01						// This should never change for the current project.
#define kKBDHexMinorVers		0x00						// This should never change for the current project.
#define kKBDCurrentRelease		kDevelopmentRelease			// Be sure to adjust this when the milestone is reached.
#define kKBDReleaseStage		1							// Be sure to adjust this number every build.

#endif

#endif
