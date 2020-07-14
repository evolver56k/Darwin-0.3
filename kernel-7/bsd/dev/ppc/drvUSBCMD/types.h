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
 	File:		Types.h
 
 	Contains:	Basic Macintosh data types.
 
 	Version:	System 7.5
 
 	DRI:		Nick Kledzik
 
 	Copyright:	© 1985-1998 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				Types.i
 					Revision:		114
 					Dated:			8/28/98
 					Last change by:	dle
 					Last comment:	Add temporary support for Carbon as well
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
/*
	NOTE
	
	The file "Types.h" has been renamed to "MacTypes.h" to prevent a collision
	with the "Types.h" available on other platforms.  MacOS only developers may 
	continue to use #include <Types.h>.  Developers doing cross-platform work where 
	Types.h also exists should change their sources to use #include <MacTypes.h>
*/

#ifndef __CONDITIONALMACROS__
#include <ConditionalMacros.h>
#endif

#if TARGET_OS_MAC
#ifndef __MACTYPES__
#include <MacTypes.h>
#endif
#else
/*
	If you get here, your development environment is messed up.
	This file is for MacOS development only.
*/
#if forCarbon
#include <MacTypes.h>
#else
#error This file (Types.h) is for developing MacOS software only!
#endif
#endif  /* TARGET_OS_MAC */

