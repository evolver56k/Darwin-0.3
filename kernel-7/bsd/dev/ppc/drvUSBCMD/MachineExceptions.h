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
 	File:		MachineExceptions.h
 
 	Contains:	Processor Exception Handling Interfaces.
 
 	Version:	Mac OS 8
 
 	DRI:		Jim Murphy
 
 	Copyright:	© 1993-1998 by Apple Computer, Inc., all rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				MachineExceptions.i
 					Revision:		27
 					Dated:			6/15/98
 					Last change by:	ALX
 					Last comment:	Make VMX stuff public.
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __MACHINEEXCEPTIONS__
#define __MACHINEEXCEPTIONS__

#ifndef __MACTYPES__
//naga#include <MacTypes.h>
#endif



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

/* Some basic declarations used throughout the kernel */
typedef struct OpaqueAreaID* 			AreaID;
//naga#if TARGET_OS_MAC
/* Machine Dependent types for PowerPC: */

struct MachineInformationPowerPC {
	UnsignedWide 					CTR;
	UnsignedWide 					LR;
	UnsignedWide 					PC;
	unsigned long 					CR;
	unsigned long 					XER;
	unsigned long 					MSR;
	unsigned long 					MQ;
	unsigned long 					ExceptKind;
	unsigned long 					DSISR;
	UnsignedWide 					DAR;
	UnsignedWide 					Reserved;
};
typedef struct MachineInformationPowerPC MachineInformationPowerPC;

struct RegisterInformationPowerPC {
	UnsignedWide 					R0;
	UnsignedWide 					R1;
	UnsignedWide 					R2;
	UnsignedWide 					R3;
	UnsignedWide 					R4;
	UnsignedWide 					R5;
	UnsignedWide 					R6;
	UnsignedWide 					R7;
	UnsignedWide 					R8;
	UnsignedWide 					R9;
	UnsignedWide 					R10;
	UnsignedWide 					R11;
	UnsignedWide 					R12;
	UnsignedWide 					R13;
	UnsignedWide 					R14;
	UnsignedWide 					R15;
	UnsignedWide 					R16;
	UnsignedWide 					R17;
	UnsignedWide 					R18;
	UnsignedWide 					R19;
	UnsignedWide 					R20;
	UnsignedWide 					R21;
	UnsignedWide 					R22;
	UnsignedWide 					R23;
	UnsignedWide 					R24;
	UnsignedWide 					R25;
	UnsignedWide 					R26;
	UnsignedWide 					R27;
	UnsignedWide 					R28;
	UnsignedWide 					R29;
	UnsignedWide 					R30;
	UnsignedWide 					R31;
};
typedef struct RegisterInformationPowerPC RegisterInformationPowerPC;

struct FPUInformationPowerPC {
	UnsignedWide 					Registers[32];
	unsigned long 					FPSCR;
	unsigned long 					Reserved;
};
typedef struct FPUInformationPowerPC	FPUInformationPowerPC;

union Vector128 {
#ifdef __VEC__
	vector unsigned long			v;
#endif
	unsigned long 					l[4];
	unsigned short 					s[8];
	unsigned char 					c[16];
};
typedef union Vector128					Vector128;

struct VectorInformationPowerPC {
	Vector128 						Registers[32];
	Vector128 						VSCR;
	UInt32 							VRsave;
};
typedef struct VectorInformationPowerPC	VectorInformationPowerPC;
/* Exception related declarations */

enum {
	kWriteReference				= 0,
	kReadReference				= 1,
	kFetchReference				= 2,
	writeReference				= kWriteReference,				/* Obsolete name*/
	readReference				= kReadReference,				/* Obsolete name*/
	fetchReference				= kFetchReference				/* Obsolete name*/
};


typedef unsigned long 					MemoryReferenceKind;

struct MemoryExceptionInformation {
	AreaID 							theArea;
	LogicalAddress 					theAddress;
	OSStatus 						theError;
	MemoryReferenceKind 			theReference;
};
typedef struct MemoryExceptionInformation MemoryExceptionInformation;

enum {
	kUnknownException			= 0,
	kIllegalInstructionException = 1,
	kTrapException				= 2,
	kAccessException			= 3,
	kUnmappedMemoryException	= 4,
	kExcludedMemoryException	= 5,
	kReadOnlyMemoryException	= 6,
	kUnresolvablePageFaultException = 7,
	kPrivilegeViolationException = 8,
	kTraceException				= 9,
	kInstructionBreakpointException = 10,
	kDataBreakpointException	= 11,
	kIntegerException			= 12,
	kFloatingPointException		= 13,
	kStackOverflowException		= 14,
	kTaskTerminationException	= 15,
	kTaskCreationException		= 16
};

#if OLDROUTINENAMES

enum {
	unknownException			= kUnknownException,			/* Obsolete name*/
	illegalInstructionException	= kIllegalInstructionException,	/* Obsolete name*/
	trapException				= kTrapException,				/* Obsolete name*/
	accessException				= kAccessException,				/* Obsolete name*/
	unmappedMemoryException		= kUnmappedMemoryException,		/* Obsolete name*/
	excludedMemoryException		= kExcludedMemoryException,		/* Obsolete name*/
	readOnlyMemoryException		= kReadOnlyMemoryException,		/* Obsolete name*/
	unresolvablePageFaultException = kUnresolvablePageFaultException, /* Obsolete name*/
	privilegeViolationException	= kPrivilegeViolationException,	/* Obsolete name*/
	traceException				= kTraceException,				/* Obsolete name*/
	instructionBreakpointException = kInstructionBreakpointException, /* Obsolete name*/
	dataBreakpointException		= kDataBreakpointException,		/* Obsolete name*/
	integerException			= kIntegerException,			/* Obsolete name*/
	floatingPointException		= kFloatingPointException,		/* Obsolete name*/
	stackOverflowException		= kStackOverflowException,		/* Obsolete name*/
	terminationException		= kTaskTerminationException,	/* Obsolete name*/
	kTerminationException		= kTaskTerminationException		/* Obsolete name*/
};

#endif  /* OLDROUTINENAMES */


typedef unsigned long 					ExceptionKind;

union ExceptionInfo {
	MemoryExceptionInformation *	memoryInfo;
};
typedef union ExceptionInfo				ExceptionInfo;

struct ExceptionInformationPowerPC {
	ExceptionKind 					theKind;
	MachineInformationPowerPC *		machineState;
	RegisterInformationPowerPC *	registerImage;
	FPUInformationPowerPC *			FPUImage;
	ExceptionInfo 					info;
	VectorInformationPowerPC *		vectorImage;
};
typedef struct ExceptionInformationPowerPC ExceptionInformationPowerPC;

typedef ExceptionInformationPowerPC 	ExceptionInformation;
typedef MachineInformationPowerPC 		MachineInformation;
typedef RegisterInformationPowerPC 		RegisterInformation;
typedef FPUInformationPowerPC 			FPUInformation;
typedef VectorInformationPowerPC 		VectorInformation;
/* 
	Note:	An ExceptionHandler is NOT a UniversalProcPtr.
			It must be a PowerPC function pointer with NO routine descriptor. 
*/
typedef CALLBACK_API_C( OSStatus , ExceptionHandler )(ExceptionInformationPowerPC *theException);
/* Routine for installing per-process exception handlers */
EXTERN_API( ExceptionHandler ) InstallExceptionHandler(ExceptionHandler theHandler);

//#endif  /* TARGET_OS_MAC */



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

#endif /* __MACHINEEXCEPTIONS__ */

