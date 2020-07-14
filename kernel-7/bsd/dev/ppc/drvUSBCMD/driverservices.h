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
 	File:		DriverServices.h
 
 	Contains:	Driver Services Interfaces.
 
 	Version:	Technology:	PowerSurge 1.0.2
 				Release:	Universal Interfaces 3.1
 
 	Copyright:	© 1985-1998 by Apple Computer, Inc., all rights reserved
 
 	Bugs?:		Please include the the file and version information (from above) with
 				the problem description.  Developers belonging to one of the Apple
 				developer programs can submit bug reports to:
 
 					devsupport@apple.com
 
*/
#ifndef __DRIVERSERVICES__
#define __DRIVERSERVICES__

#ifndef __CONDITIONALMACROS__
#include "conditionalmacros.h"
#endif
#ifndef __MACTYPES__
#include "mactypes.h"
#endif
#ifndef __ERRORS__
#include "errors.h"
#endif
#include "OSUtils.h"
#include "MachineExceptions.h"
/*naga
#ifndef __MACHINEEXCEPTIONS__
#include "MachineExceptions.h
#endif
#ifndef __DEVICES__
#include <Devices.h>
#endif
#ifndef __DRIVERSYNCHRONIZATION__
#include <DriverSynchronization.h>
#endif
*/


#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=power
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

/******************************************************************
 *
 * 		Previously in Kernel.h
 *
 ******************************************************************/
/* Kernel basics*/
//naga
typedef int AddressSpaceID;
typedef int IOCommandID;
typedef struct OpaqueIOPreparationID* 	IOPreparationID;
typedef struct OpaqueSoftwareInterruptID*  SoftwareInterruptID;
typedef struct OpaqueTaskID* 			TaskID;
typedef struct OpaqueTimerID* 			TimerID;
/* OrderedItem stuff*/
typedef OSType 							OrderedItemService;
typedef OSType 							OrderedItemSignature;

enum {
	kMatchAnyOrderedItemService	= FOUR_CHAR_CODE('****'),
	kMatchAnyOrderedItemSignature = FOUR_CHAR_CODE('****'),
	kDoNotMatchAnyOrderedItemService = FOUR_CHAR_CODE('----'),
	kDoNotMatchAnyOrderedItemSignature = FOUR_CHAR_CODE('----')
};


struct OrderedItemName {
	OrderedItemService 				service;
	OrderedItemSignature 			signature;
};
typedef struct OrderedItemName			OrderedItemName;
typedef OrderedItemName *				OrderedItemNamePtr;

typedef OptionBits 						OrderedItemOptions;

enum {
	kOrderedItemIsRightBefore	= 0x00000001,
	kOrderedItemIsRightAfter	= 0x00000002
};


struct OrderRequirements {
	OrderedItemOptions 				options;
	OrderedItemName 				itemBefore;
	OrderedItemName 				itemAfter;
};
typedef struct OrderRequirements		OrderRequirements;
typedef OrderRequirements *				OrderRequirementsPtr;
/* Tasking*/

typedef UInt32 							ExecutionLevel;

enum {
	kTaskLevel					= 0,
	kSoftwareInterruptLevel		= 1,
	kAcceptFunctionLevel		= 2,
	kKernelLevel				= 3,
	kSIHAcceptFunctionLevel		= 4,
	kSecondaryInterruptLevel	= 5,
	kHardwareInterruptLevel		= 6
};

typedef CALLBACK_API_C( void , SoftwareInterruptHandler )(void *p1, void *p2);
typedef CALLBACK_API_C( OSStatus , SecondaryInterruptHandler2 )(void *p1, void *p2);
#define kCurrentAddressSpaceID ((AddressSpaceID) -1)

/* Memory System basics*/

struct LogicalAddressRange {
	LogicalAddress 					address;
	ByteCount 						count;
};
typedef struct LogicalAddressRange		LogicalAddressRange;
typedef LogicalAddressRange *			LogicalAddressRangePtr;

struct PhysicalAddressRange {
	PhysicalAddress 				address;
	ByteCount 						count;
};
typedef struct PhysicalAddressRange		PhysicalAddressRange;
typedef PhysicalAddressRange *			PhysicalAddressRangePtr;
/* For PrepareMemoryForIO and CheckpointIO*/

typedef OptionBits 						IOPreparationOptions;

enum {
	kIOMultipleRanges			= 0x00000001,
	kIOLogicalRanges			= 0x00000002,
	kIOMinimalLogicalMapping	= 0x00000004,
	kIOShareMappingTables		= 0x00000008,
	kIOIsInput					= 0x00000010,
	kIOIsOutput					= 0x00000020,
	kIOCoherentDataPath			= 0x00000040,
	kIOTransferIsLogical		= 0x00000080,
	kIOClientIsUserMode			= 0x00000080
};

typedef OptionBits 						IOPreparationState;

enum {
	kIOStateDone				= 0x00000001
};


enum {
	kInvalidPageAddress			= (-1)
};


struct AddressRange {
	void *							base;
	ByteCount 						length;
};
typedef struct AddressRange				AddressRange;
/* C's treatment of arrays and array pointers is atypical*/

typedef LogicalAddress *				LogicalMappingTablePtr;
typedef PhysicalAddress *				PhysicalMappingTablePtr;
typedef AddressRange *					AddressRangeTablePtr;

struct MultipleAddressRange {
	ItemCount 						entryCount;
	AddressRangeTablePtr 			rangeTable;
};
typedef struct MultipleAddressRange		MultipleAddressRange;
/*
   Separate C definition so that union has a name.  A future version of the interfacer
   tool will allow a name (that gets thrown out in Pascal and Asm).
*/

struct IOPreparationTable {
	IOPreparationOptions 			options;
	IOPreparationState 				state;
	IOPreparationID 				preparationID;
	AddressSpaceID 					addressSpace;
	ByteCount 						granularity;
	ByteCount 						firstPrepared;
	ByteCount 						lengthPrepared;
	ItemCount 						mappingEntryCount;
	LogicalMappingTablePtr 			logicalMapping;
	PhysicalMappingTablePtr 		physicalMapping;
	union {
		AddressRange 					range;
		MultipleAddressRange 			multipleRanges;
	} 								rangeInfo;
};
typedef struct IOPreparationTable		IOPreparationTable;

typedef OptionBits 						IOCheckpointOptions;

enum {
	kNextIOIsInput				= 0x00000001,
	kNextIOIsOutput				= 0x00000002,
	kMoreIOTransfers			= 0x00000004
};

/* For SetProcessorCacheMode*/

typedef UInt32 							ProcessorCacheMode;

enum {
	kProcessorCacheModeDefault	= 0,
	kProcessorCacheModeInhibited = 1,
	kProcessorCacheModeWriteThrough = 2,
	kProcessorCacheModeCopyBack	= 3
};

/* For GetPageInformation*/


enum {
	kPageInformationVersion		= 1
};

typedef UInt32 							PageStateInformation;

enum {
	kPageIsProtected			= 0x00000001,
	kPageIsProtectedPrivileged	= 0x00000002,
	kPageIsModified				= 0x00000004,
	kPageIsReferenced			= 0x00000008,
	kPageIsLockedResident		= 0x00000010,
	kPageIsInMemory				= 0x00000020,
	kPageIsShared				= 0x00000040,
	kPageIsWriteThroughCached	= 0x00000080,
	kPageIsCopyBackCached		= 0x00000100,
	kPageIsLocked				= kPageIsLockedResident,		/* Deprecated*/
	kPageIsResident				= kPageIsInMemory				/* Deprecated*/
};

struct PageInformation {
	AreaID 							area;
	ItemCount 						count;
	PageStateInformation 			information[1];
};
typedef struct PageInformation			PageInformation;
typedef PageInformation *				PageInformationPtr;

/*  Tasks  */
EXTERN_API_C( ExecutionLevel )
CurrentExecutionLevel			(void);

EXTERN_API_C( TaskID )
CurrentTaskID					(void);

EXTERN_API_C( OSStatus )
DelayFor						(Duration 				delayDuration);

EXTERN_API_C( Boolean )
InPrivilegedMode				(void);


/*  Software Interrupts  */
EXTERN_API_C( OSStatus )
CreateSoftwareInterrupt			(SoftwareInterruptHandler  handler,
								 TaskID 				task,
								 void *					p1,
								 Boolean 				persistent,
								 SoftwareInterruptID *	theSoftwareInterrupt);


EXTERN_API_C( OSStatus )
SendSoftwareInterrupt			(SoftwareInterruptID 	theSoftwareInterrupt,
								 void *					p2);

EXTERN_API_C( OSStatus )
DeleteSoftwareInterrupt			(SoftwareInterruptID 	theSoftwareInterrupt);

//naga#if TARGET_OS_MAC
/*  Secondary Interrupts  */
EXTERN_API_C( OSStatus )
CallSecondaryInterruptHandler2	(SecondaryInterruptHandler2  theHandler,
								 ExceptionHandler 		exceptionHandler,
								 void *					p1,
								 void *					p2);

EXTERN_API_C( OSStatus )
QueueSecondaryInterruptHandler	(SecondaryInterruptHandler2  theHandler,
								 ExceptionHandler 		exceptionHandler,
								 void *					p1,
								 void *					p2);

//#endif  /* TARGET_OS_MAC */

/*  Timers  */
EXTERN_API_C( OSStatus )
SetInterruptTimer				(const AbsoluteTime *	expirationTime,
								 SecondaryInterruptHandler2  handler,
								 void *					p1,
								 TimerID *				theTimer);

EXTERN_API_C( OSStatus )
CancelTimer						(TimerID 				theTimer,
								 AbsoluteTime *			timeRemaining);


/*  I/O related Operations  */
EXTERN_API_C( OSStatus )
PrepareMemoryForIO				(IOPreparationTable *	theIOPreparationTable);

EXTERN_API_C( OSStatus )
CheckpointIO					(IOPreparationID 		theIOPreparation,
								 IOCheckpointOptions 	options);


/*  Memory Operations  */
EXTERN_API_C( OSStatus )
GetPageInformation				(AddressSpaceID 		addressSpace,
								 ConstLogicalAddress 	base,
								 ItemCount 				requestedPages,
								 PBVersion 				version,
								 PageInformation *		thePageInfo);

/*  Processor Cache Related  */
EXTERN_API_C( OSStatus )
SetProcessorCacheMode			(AddressSpaceID 		addressSpace,
								 ConstLogicalAddress 	base,
								 ByteCount 				length,
								 ProcessorCacheMode 	cacheMode);

/******************************************************************
 *
 * 		Was in DriverSupport.h or DriverServices.h
 *
 ******************************************************************/
#define	kAAPLDeviceLogicalAddress "AAPL,address"

typedef LogicalAddress *				DeviceLogicalAddressPtr;

enum {
	durationMicrosecond			= -1L,							/* Microseconds are negative*/
	durationMillisecond			= 1L,							/* Milliseconds are positive*/
	durationSecond				= 1000L,						/* 1000 * durationMillisecond*/
	durationMinute				= 60000L,						/* 60 * durationSecond,*/
	durationHour				= 3600000L,						/* 60 * durationMinute,*/
	durationDay					= 86400000L,					/* 24 * durationHour,*/
	durationNoWait				= 0L,							/* don't block*/
	durationForever				= 0x7FFFFFFF					/* no time limit*/
};


enum {
	k8BitAccess					= 0,							/* access as 8 bit*/
	k16BitAccess				= 1,							/* access as 16 bit*/
	k32BitAccess				= 2								/* access as 32 bit*/
};

typedef UnsignedWide 					Nanoseconds;

EXTERN_API_C( OSErr )
IOCommandIsComplete				(IOCommandID 			theID,
								 OSErr 					theResult);
/*naga
EXTERN_API_C( OSErr )
GetIOCommandInfo				(IOCommandID 			theID,
								 IOCommandContents *	theContents,
								 IOCommandCode *		theCommand,
								 IOCommandKind *		theKind);
EXTERN_API_C( void )
UpdateDeviceActivity			(RegEntryID *			deviceEntry);
naga */

EXTERN_API_C( void )
BlockCopy						(const void *			srcPtr,
								 void *					destPtr,
								 Size 					byteCount);

EXTERN_API_C( LogicalAddress )
PoolAllocateResident			(ByteCount 				byteSize,
								 Boolean 				clear);

EXTERN_API_C( OSStatus )
PoolDeallocate					(LogicalAddress 		address);


EXTERN_API_C( ByteCount )
GetDataCacheLineSize			(void);

EXTERN_API_C( OSStatus )
FlushProcessorCache				(AddressSpaceID 		spaceID,
								 LogicalAddress 		base,
								 ByteCount 				length);

EXTERN_API_C( LogicalAddress )
MemAllocatePhysicallyContiguous	(ByteCount 				byteSize,
								 Boolean 				clear);

EXTERN_API_C( OSStatus )
MemDeallocatePhysicallyContiguous (LogicalAddress 		address);


EXTERN_API_C( AbsoluteTime )
UpTime							(void);

EXTERN_API_C( void )
GetTimeBaseInfo					(UInt32 *				minAbsoluteTimeDelta,
								 UInt32 *				theAbsoluteTimeToNanosecondNumerator,
								 UInt32 *				theAbsoluteTimeToNanosecondDenominator,
								 UInt32 *				theProcessorToAbsoluteTimeNumerator,
								 UInt32 *				theProcessorToAbsoluteTimeDenominator);


EXTERN_API_C( Nanoseconds )
AbsoluteToNanoseconds			(AbsoluteTime 			absoluteTime);

EXTERN_API_C( Duration )
AbsoluteToDuration				(AbsoluteTime 			absoluteTime);

EXTERN_API_C( AbsoluteTime )
NanosecondsToAbsolute			(Nanoseconds 			nanoseconds);

EXTERN_API_C( AbsoluteTime )
DurationToAbsolute				(Duration 				duration);

EXTERN_API_C( AbsoluteTime )
AddAbsoluteToAbsolute			(AbsoluteTime 			absoluteTime1,
								 AbsoluteTime 			absoluteTime2);

EXTERN_API_C( AbsoluteTime )
SubAbsoluteFromAbsolute			(AbsoluteTime 			leftAbsoluteTime,
								 AbsoluteTime 			rightAbsoluteTime);

EXTERN_API_C( AbsoluteTime )
AddNanosecondsToAbsolute		(Nanoseconds 			nanoseconds,
								 AbsoluteTime 			absoluteTime);

EXTERN_API_C( AbsoluteTime )
AddDurationToAbsolute			(Duration 				duration,
								 AbsoluteTime 			absoluteTime);

EXTERN_API_C( AbsoluteTime )
SubNanosecondsFromAbsolute		(Nanoseconds 			nanoseconds,
								 AbsoluteTime 			absoluteTime);

EXTERN_API_C( AbsoluteTime )
SubDurationFromAbsolute			(Duration 				duration,
								 AbsoluteTime 			absoluteTime);

EXTERN_API_C( Nanoseconds )
AbsoluteDeltaToNanoseconds		(AbsoluteTime 			leftAbsoluteTime,
								 AbsoluteTime 			rightAbsoluteTime);

EXTERN_API_C( Duration )
AbsoluteDeltaToDuration			(AbsoluteTime 			leftAbsoluteTime,
								 AbsoluteTime 			rightAbsoluteTime);

EXTERN_API_C( Nanoseconds )
DurationToNanoseconds			(Duration 				theDuration);

EXTERN_API_C( Duration )
NanosecondsToDuration			(Nanoseconds 			theNanoseconds);

EXTERN_API_C( OSErr )
PBQueueInit						(QHdrPtr 				qHeader);

EXTERN_API_C( OSErr )
PBQueueCreate					(QHdrPtr *				qHeader);

EXTERN_API_C( OSErr )
PBQueueDelete					(QHdrPtr 				qHeader);

EXTERN_API_C( void )
PBEnqueue						(QElemPtr 				qElement,
								 QHdrPtr 				qHeader);

EXTERN_API_C( OSErr )
PBEnqueueLast					(QElemPtr 				qElement,
								 QHdrPtr 				qHeader);

EXTERN_API_C( OSErr )
PBDequeue						(QElemPtr 				qElement,
								 QHdrPtr 				qHeader);

EXTERN_API_C( OSErr )
PBDequeueFirst					(QHdrPtr 				qHeader,
								 QElemPtr *				theFirstqElem);

EXTERN_API_C( OSErr )
PBDequeueLast					(QHdrPtr 				qHeader,
								 QElemPtr *				theLastqElem);

EXTERN_API_C( char *)
CStrCopy						(char *					dst,
								 const char *			src);

EXTERN_API_C( StringPtr )
PStrCopy						(StringPtr 				dst,
								 ConstStr255Param 		src);

EXTERN_API_C( char *)
CStrNCopy						(char *					dst,
								 const char *			src,
								 UInt32 				max);

EXTERN_API_C( StringPtr )
PStrNCopy						(StringPtr 				dst,
								 ConstStr255Param 		src,
								 UInt32 				max);

EXTERN_API_C( char *)
CStrCat							(char *					dst,
								 const char *			src);

EXTERN_API_C( StringPtr )
PStrCat							(StringPtr 				dst,
								 ConstStr255Param 		src);

EXTERN_API_C( char *)
CStrNCat						(char *					dst,
								 const char *			src,
								 UInt32 				max);

EXTERN_API_C( StringPtr )
PStrNCat						(StringPtr 				dst,
								 ConstStr255Param 		src,
								 UInt32 				max);

EXTERN_API_C( void )
PStrToCStr						(char *					dst,
								 ConstStr255Param 		src);

EXTERN_API_C( void )
CStrToPStr						(Str255 				dst,
								 const char *			src);

EXTERN_API_C( SInt16 )
CStrCmp							(const char *			s1,
								 const char *			s2);

EXTERN_API_C( SInt16 )
PStrCmp							(ConstStr255Param 		str1,
								 ConstStr255Param 		str2);

EXTERN_API_C( SInt16 )
CStrNCmp						(const char *			s1,
								 const char *			s2,
								 UInt32 				max);

EXTERN_API_C( SInt16 )
PStrNCmp						(ConstStr255Param 		str1,
								 ConstStr255Param 		str2,
								 UInt32 				max);

EXTERN_API_C( UInt32 )
CStrLen							(const char *			src);

EXTERN_API_C( UInt32 )
PStrLen							(ConstStr255Param 		src);

EXTERN_API_C( OSStatus )
DeviceProbe						(void *					theSrc,
								 void *					theDest,
								 UInt32 				AccessType);

EXTERN_API_C( OSStatus )
DelayForHardware				(AbsoluteTime 			absoluteTime);



/******************************************************************
 *
 * 		Was in Interrupts.h 
 *
 ******************************************************************/
/*  Interrupt types  */
typedef struct OpaqueInterruptSetID* 	InterruptSetID;
typedef long 							InterruptMemberNumber;

struct InterruptSetMember {
	InterruptSetID 					setID;
	InterruptMemberNumber 			member;
};
typedef struct InterruptSetMember		InterruptSetMember;

enum {
	kISTChipInterruptSource		= 0,
	kISTOutputDMAInterruptSource = 1,
	kISTInputDMAInterruptSource	= 2,
	kISTPropertyMemberCount		= 3
};


typedef InterruptSetMember 				ISTProperty[3];
#define kISTPropertyName	"driver-ist" 

typedef long 							InterruptReturnValue;

enum {
	kFirstMemberNumber			= 1,
	kIsrIsComplete				= 0,
	kIsrIsNotComplete			= -1,
	kMemberNumberParent			= -2
};

typedef Boolean 						InterruptSourceState;

enum {
	kSourceWasEnabled			= true,
	kSourceWasDisabled			= false
};


typedef CALLBACK_API_C( InterruptMemberNumber , InterruptHandler )(InterruptSetMember ISTmember, void *refCon, UInt32 theIntCount);
typedef CALLBACK_API_C( void , InterruptEnabler )(InterruptSetMember ISTmember, void *refCon);
typedef CALLBACK_API_C( InterruptSourceState , InterruptDisabler )(InterruptSetMember ISTmember, void *refCon);

enum {
	kReturnToParentWhenComplete	= 0x00000001,
	kReturnToParentWhenNotComplete = 0x00000002
};

typedef OptionBits 						InterruptSetOptions;
/*  Interrupt Services  */
EXTERN_API_C( OSStatus )
CreateInterruptSet				(InterruptSetID 		parentSet,
								 InterruptMemberNumber 	parentMember,
								 InterruptMemberNumber 	setSize,
								 InterruptSetID *		setID,
								 InterruptSetOptions 	options);


EXTERN_API_C( OSStatus )
InstallInterruptFunctions		(InterruptSetID 		setID,
								 InterruptMemberNumber 	member,
								 void *					refCon,
								 InterruptHandler 		handlerFunction,
								 InterruptEnabler 		enableFunction,
								 InterruptDisabler 		disableFunction);


EXTERN_API_C( OSStatus )
GetInterruptFunctions			(InterruptSetID 		setID,
								 InterruptMemberNumber 	member,
								 void **				refCon,
								 InterruptHandler *		handlerFunction,
								 InterruptEnabler *		enableFunction,
								 InterruptDisabler *	disableFunction);

EXTERN_API_C( OSStatus )
ChangeInterruptSetOptions		(InterruptSetID 		setID,
								 InterruptSetOptions 	options);

EXTERN_API_C( OSStatus )
GetInterruptSetOptions			(InterruptSetID 		setID,
								 InterruptSetOptions *	options);

void usb_BlockMoveData(void *src, void *tgt, UInt32 size); //naga added
OSStatus CompareAndSwap(UInt32,UInt32,UInt32 *);


#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERSERVICES__ */

