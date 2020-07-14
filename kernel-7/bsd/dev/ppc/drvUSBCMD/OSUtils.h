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
 	File:		OSUtils.h
 
 	Contains:	OS Utilities Interfaces.
 
 	Version:	Mac OS 8
 
 	DRI:		Jerry Godes
 
 	Copyright:	© 1985-1998 by Apple Computer, Inc., all rights reserved
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Naga Pappireddi
 				With Interfacer:	3.0d9 (PowerPC native)
 				From:				OSUtils.i
 					Revision:		63
 					Dated:			6/19/98
 					Last change by:	ngk
 					Last comment:	[2244553]  Add glueIgnore to Read/WriteLocation because they are
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __OSUTILS__
#define __OSUTILS__

#ifndef __MACTYPES__
//#include <MacTypes.h>
#endif
#ifndef __MIXEDMODE__
//#include <MixedMode.h>
#endif
/*  HandToHand and other memory utilties were moved to MacMemory.h */
#ifndef __MACMEMORY__
//#include <MacMemory.h>
#endif
/*  GetTrapAddress and other trap table utilties were moved to Patches.h */
#ifndef __PATCHES__
//#include <Patches.h>
#endif
/*  Date and Time utilties were moved to DateTimeUtils.h */
#ifndef __DATETIMEUTILS__
//#include <DateTimeUtils.h>
#endif
#if !TARGET_OS_MAC
#ifndef __ENDIAN__
//#include <Endian.h>
#endif
#endif  /* !TARGET_OS_MAC */



#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif
/*
#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif
*/

enum {
	useFree						= 0,
	useATalk					= 1,
	useAsync					= 2,
	useExtClk					= 3,							/*Externally clocked*/
	useMIDI						= 4
};


enum {
	false32b					= 0,							/*24 bit addressing error*/
	true32b						= 1								/*32 bit addressing error*/
};


enum {
																/* result types for RelString Call */
	sortsBefore					= -1,							/*first string < second string*/
	sortsEqual					= 0,							/*first string = second string*/
	sortsAfter					= 1								/*first string > second string*/
};


enum {
	dummyType					= 0,
	vType						= 1,
	ioQType						= 2,
	drvQType					= 3,
	evType						= 4,
	fsQType						= 5,
	sIQType						= 6,
	dtQType						= 7,
	nmType						= 8
};

typedef SignedByte 						QTypes;

struct SysParmType {
	UInt8 							valid;
	UInt8 							aTalkA;
	UInt8 							aTalkB;
	UInt8 							config;
	short 							portA;
	short 							portB;
	long 							alarm;
	short 							font;
	short 							kbdPrint;
	short 							volClik;
	short 							misc;
};
typedef struct SysParmType				SysParmType;

typedef SysParmType *					SysPPtr;

struct QElem {
	struct QElem *					qLink;
	short 							qType;
	short 							qData[1];
};
typedef struct QElem					QElem;
typedef QElem *							QElemPtr;
#if TARGET_OS_MAC

struct QHdr {
	short 							qFlags;
	QElemPtr 						qHead;
	QElemPtr 						qTail;
};
typedef struct QHdr						QHdr;
typedef QHdr *							QHdrPtr;
#else
/*
   QuickTime 3.0:
   this version of QHdr contains the Mutex necessary for
   non-mac non-interrupt code
*/

struct QHdr {
	short 							qFlags;
	short 							pad;
	long 							MutexID;
	QElemPtr 						qHead;
	QElemPtr 						qTail;
};
typedef struct QHdr						QHdr;
typedef QHdr *							QHdrPtr;
#endif  /* TARGET_OS_MAC */



typedef CALLBACK_API( void , DeferredTaskProcPtr )(long dtParam);
/*
	WARNING: DeferredTaskProcPtr uses register based parameters under classic 68k
			 and cannot be written in a high-level language without 
			 the help of mixed mode or assembly glue.
*/
//nagatypedef REGISTER_UPP_TYPE(DeferredTaskProcPtr) 					DeferredTaskUPP;
enum { uppDeferredTaskProcInfo = 0x0000B802 }; 					/* register no_return_value Func(4_bytes:A1) */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(DeferredTaskUPP)
NewDeferredTaskProc			   (DeferredTaskProcPtr		userRoutine);
EXTERN_API(void)
CallDeferredTaskProc		   (DeferredTaskUPP			userRoutine,
								long					dtParam);
#else
#define NewDeferredTaskProc(userRoutine) 						(DeferredTaskUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppDeferredTaskProcInfo, GetCurrentArchitecture())
#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	#pragma parameter CallDeferredTaskProc(__A0, __A1)
	void CallDeferredTaskProc(DeferredTaskUPP routine, long dtParam) = 0x4E90;
#else
	#define CallDeferredTaskProc(userRoutine, dtParam) 			CALL_ONE_PARAMETER_UPP((userRoutine), uppDeferredTaskProcInfo, (dtParam))
#endif
#endif

struct DeferredTask {
	QElemPtr 						qLink;
	short 							qType;
	short 							dtFlags;
	//naga DeferredTaskUPP 				dtAddr;
	long 							dtParam;
	long 							dtReserved;
};
typedef struct DeferredTask				DeferredTask;
typedef DeferredTask *					DeferredTaskPtr;
#if TARGET_OS_MAC

struct MachineLocation {
	Fract 							latitude;
	Fract 							longitude;
	union {
		SInt8 							dlsDelta;				/* signed byte; daylight savings delta */
		long 							gmtDelta;				/* use low 24-bits only */
	} 								u;
};
typedef struct MachineLocation			MachineLocation;
#else
/*
	QuickTime 3.0:
	Alignment of MachineLocation is weird. The union above used for delta
	tends not to work on non-Mac compilers.
*/
//naga
typedef long BigEndianLong;
struct MachineLocation {
	Fract 							latitude;
	Fract 							longitude;
	BigEndianLong 					delta;						/* high byte is daylight savings delta, low 24-bits is GMT delta */
};
typedef struct MachineLocation			MachineLocation;
#endif  /* TARGET_OS_MAC */

EXTERN_API( Boolean )
IsMetric						(void)														THREEWORDINLINE(0x3F3C, 0x0004, 0xA9ED);

EXTERN_API( SysPPtr )
GetSysPPtr						(void)														THREEWORDINLINE(0x2EBC, 0x0000, 0x01F8);


/*
	NOTE: SysBeep() has been moved to Sound.h.  
		  We could not automatically #include Sound.h in this file
		  because Sound.h indirectly #include's OSUtils.h which
		  would make a circular include.
*/
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 DTInstall(__A0)
																							#endif
EXTERN_API( OSErr )
DTInstall						(DeferredTaskPtr 		dtTaskPtr)							ONEWORDINLINE(0xA082);



#if TARGET_CPU_PPC || !TARGET_OS_MAC
#define GetMMUMode() ((SInt8)true32b)
#define SwapMMUMode(x) (*(SInt8*)(x) = true32b)
#else
EXTERN_API( SInt8 )
GetMMUMode						(void)														TWOWORDINLINE(0x1EB8, 0x0CB2);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter SwapMMUMode(__A0)
																							#endif
EXTERN_API( void )
SwapMMUMode						(SInt8 *				mode)								THREEWORDINLINE(0x1010, 0xA05D, 0x1080);

#endif
																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter Delay(__A0, __A1)
																							#endif
EXTERN_API( void )
Delay							(unsigned long 			numTicks,
								 unsigned long *		finalTicks)							TWOWORDINLINE(0xA03B, 0x2280);


EXTERN_API( OSErr )
WriteParam						(void);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter Enqueue(__A0, __A1)
																							#endif
EXTERN_API( void )
Enqueue							(QElemPtr 				qElement,
								 QHdrPtr 				qHeader)							ONEWORDINLINE(0xA96F);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 Dequeue(__A0, __A1)
																							#endif
EXTERN_API( OSErr )
Dequeue							(QElemPtr 				qElement,
								 QHdrPtr 				qHeader)							ONEWORDINLINE(0xA96E);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 SetCurrentA5
																							#endif
EXTERN_API( long )
SetCurrentA5					(void)														THREEWORDINLINE(0x200D, 0x2A78, 0x0904);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 SetA5(__D0)
																							#endif
EXTERN_API( long )
SetA5							(long 					newA5)								ONEWORDINLINE(0xC18D);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 InitUtil
																							#endif
EXTERN_API( OSErr )
InitUtil						(void)														ONEWORDINLINE(0xA03F);


#if TARGET_CPU_PPC
EXTERN_API( void )
MakeDataExecutable				(void *					baseAddress,
								 unsigned long 			length);

#endif  /* TARGET_CPU_PPC */

#if TARGET_CPU_68K
EXTERN_API( Boolean )
SwapInstructionCache			(Boolean 				cacheEnable);

EXTERN_API( void )
FlushInstructionCache			(void)														TWOWORDINLINE(0x7001, 0xA098);

EXTERN_API( Boolean )
SwapDataCache					(Boolean 				cacheEnable);

EXTERN_API( void )
FlushDataCache					(void)														TWOWORDINLINE(0x7003, 0xA098);

EXTERN_API( void )
FlushCodeCache					(void)														ONEWORDINLINE(0xA0BD);

#endif  /* TARGET_CPU_68K */

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 FlushCodeCacheRange(__A0, __A1)
																							#endif
EXTERN_API( OSErr )
FlushCodeCacheRange				(void *					address,
								 unsigned long 			count)								TWOWORDINLINE(0x7009, 0xA098);

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter ReadLocation(__A0)
																							#endif
EXTERN_API( void )
ReadLocation					(MachineLocation *		loc)								FOURWORDINLINE(0x203C, 0x000C, 0x00E4, 0xA051);


																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter WriteLocation(__A0)
																							#endif
EXTERN_API( void )
WriteLocation					(const MachineLocation * loc)								FOURWORDINLINE(0x203C, 0x000C, 0x00E4, 0xA052);



#if OLDROUTINENAMES
#define IUMetric() IsMetric()
#endif  /* OLDROUTINENAMES */

/*
	NOTE: SysEnvirons is obsolete.  You should be using Gestalt.
*/
/* Environs Equates */

enum {
	curSysEnvVers				= 2								/*Updated to equal latest SysEnvirons version*/
};


struct SysEnvRec {
	short 							environsVersion;
	short 							machineType;
	short 							systemVersion;
	short 							processor;
	Boolean 						hasFPU;
	Boolean 						hasColorQD;
	short 							keyBoardType;
	short 							atDrvrVersNum;
	short 							sysVRefNum;
};
typedef struct SysEnvRec				SysEnvRec;

enum {
																/* Machine Types */
	envMac						= -1,
	envXL						= -2,
	envMachUnknown				= 0,
	env512KE					= 1,
	envMacPlus					= 2,
	envSE						= 3,
	envMacII					= 4,
	envMacIIx					= 5,
	envMacIIcx					= 6,
	envSE30						= 7,
	envPortable					= 8,
	envMacIIci					= 9,
	envMacIIfx					= 11
};


enum {
																/* CPU types */
	envCPUUnknown				= 0,
	env68000					= 1,
	env68010					= 2,
	env68020					= 3,
	env68030					= 4,
	env68040					= 5
};


enum {
																/* Keyboard types */
	envUnknownKbd				= 0,
	envMacKbd					= 1,
	envMacAndPad				= 2,
	envMacPlusKbd				= 3,
	envAExtendKbd				= 4,
	envStandADBKbd				= 5,
	envPrtblADBKbd				= 6,
	envPrtblISOKbd				= 7,
	envStdISOADBKbd				= 8,
	envExtISOADBKbd				= 9
};

																							#if TARGET_OS_MAC && TARGET_CPU_68K && !TARGET_RT_MAC_CFM
																							#pragma parameter __D0 SysEnvirons(__D0, __A0)
																							#endif
EXTERN_API( OSErr )
SysEnvirons						(short 					versionRequested,
								 SysEnvRec *			theWorld)							ONEWORDINLINE(0xA090);






/*naga
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
*/
#endif /* __OSUTILS__ */

