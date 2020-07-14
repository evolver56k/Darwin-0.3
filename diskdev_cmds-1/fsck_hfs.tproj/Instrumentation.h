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
	File:		Instrumentation.h

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(SS)	Steven Swenson

	Change History (most recent first):

		 <1>	  4/7/97	SS		first checked in
*/

/*
 	File:		Instrumentation.h

 	Contains:	Trace and statistics collection interfaces for System 7 clients

 	Version:	1.0

 	Copyright:	© 1996-1997 by Apple Computer, Inc.
 				All rights reserved.

*/
#ifndef __INSTRUMENTATION__
#define __INSTRUMENTATION__

#if TARGET_OS_MAC
#include <Types.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */
#include "HFSDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

/*******************************************************************/
/*                        Types				                       */
/*******************************************************************/
/* Reference to an instrumentation class */
typedef struct InstOpaqueClassRef* InstClassRef;

/* Aliases to the generic instrumentation class for each type of class */
typedef InstClassRef InstPathClassRef;
typedef InstClassRef InstTraceClassRef;
typedef InstClassRef InstHistogramClassRef;
typedef InstClassRef InstSplitHistogramClassRef;
typedef InstClassRef InstMagnitudeClassRef;
typedef InstClassRef InstGrowthClassRef;
typedef InstClassRef InstTallyClassRef;

/* Reference to a data descriptor */
typedef struct InstOpaqueDataDescriptorRef* InstDataDescriptorRef;


/*******************************************************************/
/*            Constant Definitions                                 */
/*******************************************************************/

/* Reference to the root of the class hierarchy */
#define kInstRootClassRef	 ( (InstClassRef) -1)

/* Options used for creating classes */
typedef OptionBits InstClassOptions;

enum {
	kInstDisableClassMask			= 0x00,			/* Create the class disabled */
	kInstEnableClassMask			= 0x01,			/* Create the class enabled */

	kInstSummaryTraceClassMask		= 0x20			/* Create a summary trace class instead of a regular one */
};

/* Options used for logging events*/
typedef OptionBits InstEventOptions;

enum {
	kInstStartEvent					= 1,			/* This is a start event; begin accounting for an arc */
	kInstEndEvent					= 2,			/* This is an end event; stop accounting for an arc */
	kInstMiddleEvent				= 3				/* This is a middle event */
};

/* InstEventTag's are used to associate start, middle and end events */
typedef UInt32 InstEventTag;

enum {
	kInstNoEventTag					= 0
};

/*******************************************************************/
/*                            API                                  */
/*******************************************************************/


/*********** Initialization and Termination routines (68K-only) ***********/
#if GENERATING68K

extern pascal OSStatus		InstInitialize68K( void);

// You do not need to call this if your process calls ExitToShell().
extern pascal OSStatus		InstTerminate68K( void);

#endif	//	GENERATING68K


/*************************** Creation routines ***************************/
/* Create a class that has no statistic or trace data. It is a path node in the class hierarchy */
extern pascal OSStatus InstCreatePathClass( InstPathClassRef parentClass, const char *className, 
											InstClassOptions options, InstPathClassRef *returnPathClass);

/*
 Create a class that can log events to a trace buffer.
*/
extern pascal OSStatus InstCreateTraceClass( InstPathClassRef parentClass, const char *className, 
												OSType component, InstClassOptions options, 
												InstTraceClassRef *returnTraceClass);

/*
 Create a class that contains a histogram
*/
extern pascal OSStatus InstCreateHistogramClass( InstPathClassRef parentClass, const char *className, 
													SInt32 lowerBounds, SInt32 upperBounds, UInt32 bucketWidth, 
													InstClassOptions options, InstHistogramClassRef *returnHistogramClass);

/*
 Create a class that contains a split histogram
 Split histograms are useful when examining a single range with two different
 areas of different resolution.
*/
extern pascal OSStatus InstCreateSplitHistogramClass( InstPathClassRef parentClass, const char *className, 
														SInt32 histogram1LowerBounds, UInt32 histogram1BucketWidth, 
														SInt32 knee, SInt32 histogram2UpperBounds, UInt32 histogram2BucketWidth, 
														InstClassOptions options, InstSplitHistogramClassRef *returnSplitHistogramClass);

/*
 Create a class that has magnitude statistical data
*/
extern pascal OSStatus InstCreateMagnitudeClass( InstPathClassRef parentClass, const char *className, 
												InstClassOptions options, InstMagnitudeClassRef *returnMagnitudeClass);

/*
 Create a class that has growth statistical data
*/
extern pascal OSStatus InstCreateGrowthClass( InstPathClassRef parentClass, const char *className, 
											  InstClassOptions options, InstGrowthClassRef *returnGrowthClass);

/*
 Create a class that keeps tally statistics
*/
extern pascal OSStatus InstCreateTallyClass( InstPathClassRef parentClass, const char *className, 
												UInt16 maxNumberOfValues, InstClassOptions options, 
												InstTallyClassRef *returnTallyClass);

/*
 Create a descriptor to a piece of data
 DataDescriptors must be used when logging trace data so that generic browsers can display the data.
*/
extern pascal OSStatus InstCreateDataDescriptor( const char *formatString, InstDataDescriptorRef *returnDescriptor);

/* Create multiple Data Descriptors.  This is convenient when using static lists of format strings.*/
extern pascal OSStatus InstCreateDataDescriptors( const char **formatStrings, ItemCount numberOfDescriptors, 
													InstDataDescriptorRef *returnDescriptorList);

/* Create a unique event tag.  These can be use to relate trace events.*/
extern pascal InstEventTag InstCreateEventTag( void);

/*************************** Access routines ***************************/

/* Enable spooling for the given class node. */
extern pascal OSStatus InstEnableClass( InstClassRef classRef);

/* Disable spooling for the given class node. */
extern pascal OSStatus InstDisableClass( InstClassRef classRef);

/*************************** Dispose routines ***************************/
extern pascal void InstDisposeClass( InstClassRef theClass);

extern pascal void InstDisposeDataDescriptor( InstDataDescriptorRef theDescriptor);

extern pascal void InstDisposeDataDescriptors( InstDataDescriptorRef *theDescriptorList, ItemCount numberOfDescriptors);

/*************************** Event Logging and Stat Updating routines ***************************/
extern pascal void InstUpdateHistogram( InstHistogramClassRef theHistogramClass, SInt32 value, UInt32 count);

extern pascal void InstUpdateMagnitudeAbsolute( InstMagnitudeClassRef theMagnitudeClass, SInt32 newValue);

extern pascal void InstUpdateMagnitudeDelta( InstMagnitudeClassRef theMagnitudeClass, SInt32 delta);

extern pascal void InstUpdateGrowth( InstGrowthClassRef theGrowthClass, UInt32 increment);

extern pascal void InstUpdateTally( InstTallyClassRef theTallyClass, void *bucketID, UInt32 count);

/* Log a trace event that has no data */
extern pascal void InstLogTraceEvent( InstTraceClassRef theTraceClass, InstEventTag eventTag, InstEventOptions options);

/* Log a trace event with data */
extern pascal void InstLogTraceEventWithData( InstTraceClassRef theTraceClass, InstEventTag eventTag, 
											InstEventOptions options, InstDataDescriptorRef theDescriptor, ...);

/*
 The "dataStructure" should match the dataDescriptor's printf string.
 All fields must be longword multiples.  ie, even char's (%c) or shorts (%d) must
 be longwords in the data structure.
 If the data structure contains a string, the string must be padded to be longword
 aligned.
 Think of this passing in a flattened version of printf varargs
*/
extern pascal void InstLogTraceEventWithDataStructure( InstTraceClassRef theTraceClass, InstEventTag eventTag, 
														InstEventOptions options, InstDataDescriptorRef descriptorRef, 
														const UInt8 *dataStructure, ByteCount structureSize);

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INSTRUMENTATION__ */

