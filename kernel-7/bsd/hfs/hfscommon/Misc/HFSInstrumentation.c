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
	File:		HFSInstrumentation.c

	Contains:	Code for instrumenting the File Manager

	Version:	HFS Plus 1.0

	Written by:	Mark Day

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Mark Day

		Other Contacts:		Don Brady, Deric Horn

		Technology:			xxx put technology here xxx

	Writers:

		(DSH)	Deric Horn
		(djb)	Don Brady
		(msd)	Mark Day

	Change History (most recent first):

	   <CS5>	 7/21/97	djb		Add STLogStartTime and STLogEndTime routines for summary traces.
	   <CS4>	 7/16/97	DSH		FilesInternal.i renamed FileMgrInternal.i to avoid name
									collision
	   <CS3>	  5/9/97	djb		Include LowMemPriv.h
	   <CS2>	  5/7/97	djb		Add support for summary traces.
	   <CS1>	 4/24/97	djb		first checked in
	  <HFS5>	 2/27/97	msd		By default, disable the HFS instrumentation nodes. They can be
									turned on at runtime with the instrumentation viewer.
	  <HFS4>	 2/26/97	msd		Add nodes for HFS:Extents, and add data descriptor for an
									extent.
	  <HFS3>	  2/6/97	msd		Add a format descriptor for to help with logging FSSpec's.
	  <HFS2>	 1/21/97	msd		Create the HFS and VSM nodes in the instrumentation tree.
	  <HFS1>	 1/17/97	msd		first checked in
		<0>		 1/16/97	msd		First written.

*/

#if ( PRAGMA_LOAD_SUPPORTED )
        #pragma	load	PrecompiledHeaders
#else
        #if TARGET_OS_MAC
	#include	<Gestalt.h>
	#include	<LowMemPriv.h>
	#include	<Types.h>
        #else
	#include "../../hfs.h"
        #include "../headers/system/MacOSStubs.h"
        #endif 	/* TARGET_OS_MAC */
#endif	/* PRAGMA_LOAD_SUPPORTED */

#include	"../headers/FileMgrInternal.h"
#include	"../headers/system/HFSInstrumentation.h"


void GetTimeBase(UInt64 *timeBase);



void InitHFSInstrumentation(void);


void InitHFSInstrumentation(void)
{
#if TARGET_OS_MAC
	OSStatus				err;
	InstTraceClassRef		catSearch;
	FSVarsRec				*fsVars;
	InstPathClassRef		pathClass;
	InstDataDescriptorRef	specDescriptor;
	InstDataDescriptorRef	extentDescriptor;
	
        fsVars = (FSVarsRec*) LMGetFSMVars();
	
	err = InstInitialize68K();
	if (err != noErr) DebugStr("\pError from InstInitialize68K");
	
	err = InstCreatePathClass( kInstRootClassRef, "HFS", kInstEnableClassMask, &pathClass);
	if (err != noErr) DebugStr("\pError from InstCreatePathClass");
	
	if ( HFSInstrumentation )
	{
		err = InstCreatePathClass( kInstRootClassRef, "HFS:VSM", kInstEnableClassMask, &pathClass);
		if (err != noErr) DebugStr("\pError from InstCreatePathClass");
		err = InstCreatePathClass( kInstRootClassRef, "HFS:Extents", kInstEnableClassMask, &pathClass);
		if (err != noErr) DebugStr("\pError from InstCreatePathClass");
												
		err = InstCreateTraceClass(kInstRootClassRef, "HFS:CatSearch", 'srch', kInstEnableClassMask, &catSearch);
		if (err != noErr) DebugStr("\pError from InstCreateTraceClass");
		
		fsVars->later[0] = (UInt32) catSearch;
		
		err = InstCreateDataDescriptor( "[vol: %d, parID: %08lX, name: %s]", &specDescriptor);
		if (err != noErr) DebugStr("\pError from InstCreateDataDescriptor");
	
		fsVars->later[1] = (UInt32) specDescriptor;
		
		err = InstCreateDataDescriptor( "fileID: %8lX, start: %8lX, num: %8lX, FABN: %8lX", &extentDescriptor);
		if (err != noErr) DebugStr("\pError from InstCreateDataDescriptor");
	
		fsVars->later[2] = (UInt32) extentDescriptor;
	}
#endif
}


void STLogStartTime(UInt32 selector)
{
#if TARGET_OS_MAC
	FSVarsRec *fsVars;

	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( fsVars->gNativeCPUtype >= gestaltCPU601 )
	{
		CallProfile *p;

		p = &fsVars->gCallProfile[selector];
		
		if (p->refCount++ == 0)
			GetTimeBase(&p->startBase);		// record start time for first call (recursive calls are ignored)
	}
#else
#pragma unused(selector)
#endif 	/* TARGET_OS_MAC */
}


void STLogEndTime(UInt32 selector, OSErr error)
{
#if TARGET_OS_MAC
	FSVarsRec	*fsVars;

	fsVars = (FSVarsRec*) LMGetFSMVars();

	if ( fsVars->gNativeCPUtype >= gestaltCPU601 )
	{
		CallProfile *p;
		UInt64		timebase;		// in nanoseconds
		UInt32		elaspedTime;

		p = &fsVars->gCallProfile[selector];
		p->refCount--;
		
		if ( p->refCount != 0 )
			return;		// this is a recursive call, so skip
		
		GetTimeBase(&timebase);
		timebase = U64Subtract(timebase, p->startBase);
		

		// check if gTimeBaseFactor has been set up yet...
		if ( fsVars->gTimeBaseFactor == 0 )
		{
			long	response;
			OSErr	result;
			
			result = Gestalt( gestaltNativeCPUtype, &response );		
			if (result)  return;		// gTimeBaseFactor not ready yet so bail
	
			if ( fsVars->gNativeCPUtype == gestaltCPU601 )
			{
				fsVars->gTimeBaseFactor = (UInt32) 1000;		// convert from nanosec to µsec
			}
			else // 603 or 604 (ie has real TimeBase register)
			{
				result = Gestalt( gestaltBusClkSpeed, &response );
				if (result)  return;	// gTimeBaseFactor not ready yet so bail
	
				fsVars->gTimeBaseFactor = (UInt32) response / (UInt32) 4000000;		// calc µsec conversion factor
			}
		}
	
		if ( timebase.hi == 0 )
			elaspedTime = timebase.lo / fsVars->gTimeBaseFactor;
		else
			return;		// this time is too long (we're probally in the debugger!)

		
		if ( elaspedTime > p->maxTime )
			 p->maxTime = elaspedTime;
	
		if ( elaspedTime < p->minTime || p->minTime == 0 )
			 p->minTime = elaspedTime;
			 
		p->callCount++;
		
		if ( error != 0 )
			p->errCount++;
		
		timebase.lo = elaspedTime;
		timebase.hi = 0;
		p->totalTime = U64Add(p->totalTime, timebase);
	}
#else
#pragma unused(selector,error)
#endif 	/* TARGET_OS_MAC */
}
