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
	File:		FileMgrInit.c

	Contains:	Initialization code for HFS Plus

	Version:	HFS Plus 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):
	   <Rhap>	  4/3/98	djb		Clean up usage of FSVar based globals.

	   <CS9>	10/13/97	djb		Set initial default text encoding to kTextEncodingUndefined.
	   <CS8>	 10/1/97	djb		Call InitCatalogCache to allocate catalog iterator cache.
	   <CS7>	 9/16/97	msd		Fix bug #1679792 by tail patching WriteXParam to do a
									ReadLocation.
	   <CS6>	 7/28/97	msd		Allocate a 32K attributes buffer in PostInitFS.
	   <CS5>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS4>	  7/8/97	DSH		Loading PrecompiledHeaders from define passed in on C line
	   <CS3>	 6/17/97	msd		In PostInitFS, set up the offset to GMT.
	   <CS2>	  5/7/97	djb		Turn on the gestaltDTMgrSupportsFSM bit.
	   <CS1>	 4/25/97	djb		first checked in

	  <HFS2>	 4/10/97	msd		Remove enums gestaltFSNoMFSVols and
									gestaltFSSupportsHFSPlusVols, since they are now in Gestalt.i.
	  <HFS1>	 3/31/97	djb		first checked in
*/

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
	#include	<Types.h>
	#include	<Gestalt.h>
	#include	<OSUtils.h>
	#include	<LowMemPriv.h>
	#include <Patches.h>
	#else
	#include "../../hfs.h"
        #include "../headers/system/MacOSStubs.h"
	#endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#include "../headers/FileMgrInternal.h"

OSErr PostInitFS(void);

pascal OSErr FSAttrGestaltFunction( OSType selector, long *response );
extern long WriteXPRamPatch(void);
void UpdateGMTDelta(void);



OSErr PostInitFS(void)
{
#if TARGET_OS_MAC
	SelectorFunctionUPP	oldGestaltFunction;
#endif
	OSErr				result;
	FSVarsRec			*fsVars;
	
	fsVars = (FSVarsRec *) LMGetFSMVars();


	fsVars->gDefaultBaseEncoding = kTextEncodingUndefined;
	
	//
	// Allocate Catalog Iterator cache...
	//
	result = InitCatalogCache();
	ReturnIfError(result);

	//
	//	Allocate a 32K buffer used by CatSearch, attributes, ExchangeFiles.
	//€€ constant should really be in FileMgrInternal.i
	//
#if TARGET_OS_MAC
	fsVars->gAttributesBuffer		= NewPtrSys(32768);
	if (fsVars->gAttributesBuffer == NULL)
	{
	  #if DEBUG_BUILD
		DebugStr("\pFATAL: No memory for File Manager");
	  #endif

		return MemError();
	}
	fsVars->gAttributesBufferSize	= 32768;
#endif

	//
	//	Update the GMT offset
	//
	UpdateGMTDelta();
	
#if TARGET_OS_MAC
	//
	//	Patch the WriteXPRam trap so we can update the GMT offset when it changes.
	//
	fsVars->oldWriteXPRam = GetOSTrapAddress(_WriteXPRam);
	SetOSTrapAddress(WriteXPRamPatch, _WriteXPRam);
	
	//
	//	Fix up the gestalt value we return (for new flags)
	//
	result = ReplaceGestalt(gestaltFSAttr, FSAttrGestaltFunction, &oldGestaltFunction);
#else
#pragma unused(oldGestaltFunction)
#endif 	/* TARGET_OS_MAC */
	
	return result;
}


	
#if TARGET_OS_MAC

pascal OSErr FSAttrGestaltFunction( OSType selector, long *response )
{
	#pragma unused (selector)

	*response = (1 << gestaltFullExtFSDispatching)
			  | (1 << gestaltHasFSSpecCalls)
			  | (1 << gestaltHasFileSystemManager)
			  | (1 << gestaltFSMDoesDynamicLoad)
			  | (1 << gestaltFSSupports4GBVols)
			  | (1 << gestaltFSSupports2TBVols)
			  | (1 << gestaltHasExtendedDiskInit)
			  | (1 << gestaltDTMgrSupportsFSM)

			  | (1 << gestaltFSNoMFSVols)
			  | (1 << gestaltFSSupportsHFSPlusVols);

	return noErr;
}
#endif 	/* TARGET_OS_MAC */



void UpdateGMTDelta(void)
{
/* ### */
#if TARGET_OS_MAC
	long				gmtDelta;
	FSVarsRec			*fsVars = (FSVarsRec *) LMGetFSMVars();
	MachineLocation		location;

	//
	//	Determine the offset from GMT to local time (local - GMT)
	//
	ReadLocation(&location);
	gmtDelta = location.u.gmtDelta & 0x00FFFFFF;		// value is low 24 bits
	if (gmtDelta & 0x00800000)							// negative?
		gmtDelta |= 0xFF000000;							// if so, sign extend
	
	fsVars->offsetToUTC = gmtDelta;
#endif 	/* TARGET_OS_MAC */
}
