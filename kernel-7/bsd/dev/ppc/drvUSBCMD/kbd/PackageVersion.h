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

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		

		Technology:			

	Writers:

		(TC)	Tom Clark
		(DF)	David Ferguson
		(CJK)	Craig Keithley

    This file is used in these builds:  

	Change History (most recent first):

	 <USB53>	10/28/98	CJK		change version to 1.1a5
	 <USB52>	10/22/98	CJK		change version to 1.1a4
	 <USB51>	10/16/98	TC		Change version to 1.1a3
	 <USB50>	 10/5/98	TC		Change version to 1.1d4
	 <USB49>	 9/18/98	TC		Change version to 1.1d3.  Added 1997 to copyright string.
	 <USB48>	 9/14/98	TC		Change version to 1.1d2
	 <USB47>	 8/27/98	DF		OK, now make the change to more than the checkin comment
	 <USB46>	 8/27/98	DF		change version to GM 1.1d1
	 <USB45>	 8/27/98	DF		change version to GM 1.0.1
	 <USB44>	 8/24/98	DF		change revision to 1.0.1b5
	 <USB43>	 8/13/98	CJK		change version to 1.0.1b4
	 <USB42>	 7/30/98	CJK		change version to 1.0.1b2
	 <USB41>	 7/28/98	CJK		change version to 1.0.1b1
	 <USB40>	 7/15/98	DF		We're GM with 1.0!!!!
	 <USB39>	 7/10/98	CJK		change version to 1.0b8
	 <USB38>	 7/10/98	CJK		change version to 1.0b8
	 <USB37>	  7/1/98	DF		change version to 1.0b7
	 <USB36>	 6/22/98	DF		change version to 1.0b6
	 <USB35>	 6/15/98	CJK		change version to 1.0b5
	 <USB34>	  6/8/98	CJK		change version to 1.0b4
	 <USB33>	  6/2/98	CJK		change version to 1.0b3
	 <USB32>	 5/18/98	CJK		change version to 1.0b2
	 <USB31>	 5/18/98	CJK		change version to 1.0a12 (should have been a12/BC1)
	 <USB30>	 5/13/98	CJK		change from decimal 11 to hex 11
	 <USB29>	 5/13/98	CJK		change version to 1.0a11+
	 <USB28>	 5/11/98	CJK		change version to 1.0a11
	 <USB27>	 4/27/98	CJK		change version to 1.0a9
	 <USB26>	 4/23/98	CJK		change version to 1.0a8+
	 <USB25>	 4/20/98	CJK		change version to 1.0a8
	 <USB24>	 4/20/98	CJK		change version to 1.0a8e1
	 <USB23>	 4/14/98	CJK		change version to 1.0a7e2
	 <USB22>	  4/9/98	CJK		change version to 1.0a7e1
	 <USB21>	  4/9/98	CJK		change copyright date (bad clone, bad bad clone)
		<20>	  4/6/98	CJK		change version to 1.0a6+
		<19>	  4/3/98	CJK		change version to 1.0a6
		<18>	 3/27/98	CJK		change version to 1.0a5
		<17>	 3/24/98	CJK		change version to 1.0a4+
		<16>	 3/20/98	CJK		change version to 1.0a4
		<15>	 3/18/98	CJK		change version to 1.0a4e1
		<14>	 3/12/98	CJK		change version to 1.0a3
		<13>	  3/6/98	CJK		change version to 1.0a2
		<12>	  3/6/98	CJK		change version to 1.0a2e2
		<11>	  3/6/98	CJK		change version to 1.0a2e1
		<10>	  3/5/98	CJK		Work on version string formatting
		 <9>	  3/4/98	CJK		change version to 1.0a1c1
		 <8>	  3/3/98	CJK		change version to 1.0d5ac1
		 <7>	  3/2/98	CJK		change version to 1.0d5e2
		 <6>	 2/27/98	CJK		change version to 1.0d5e1
		 <5>	 2/24/98	CJK		change version to 1.0d4+
		 <4>	 2/19/98	CJK		change version to 1.0d4
		 <3>	 2/17/98	CJK		change version to 1.0d3+
		 <2>	 2/12/98	CJK		change version to 1.0d3
*/
#ifndef __PACKAGEVERSION__
#define __PACKAGEVERSION__

#define kIsFinalCandidate		0x00

#define	kOverrideIndividualVersions	0xff

#define kDevelopmentRelease		0x20
#define kAlphaRelease 			0x40
#define kBetaRelease			0x60
#define kFinalRelease			0x80

#define kPKGStringVersShort		"1.1a5"
#define kPKGStringVers1Long		"1.1a5, ©Copyright 1997-1998 Apple Computer, Inc."
#define kPKGStringVers2Long		"USB Manager 1.1a5"

#define kPKGHexMajorVers		0x01						// This should never change for the current project.
#define kPKGHexMinorVers		0x10						// This should never change for the current project.

#define kPKGCurrentRelease		kAlphaRelease				// Be sure to adjust this when the milestone is reached.
#define kPKGReleaseStage		0x05						// Be sure to adjust this number every build.
	
#endif