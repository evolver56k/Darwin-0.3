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
 	File:		ConditionalMacros.h
 
 	Contains:	Set up for compiler independent conditionals
 
 	Version:	Universal Interface Files 3.0dx
 
 	DRI:		Nick Kledzik
 
 	Copyright:	© 1984-1996 by Apple Computer, Inc.
 				All rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Alan Mimms
 				With Interfacer:	2.0d13   (PowerPC native)
 				From:				ConditionalMacros.i
 					Revision:		48
 					Dated:			7/26/96
 					Last change by:	ngk
 					Last comment:	#1371833, update way GENERATINGPOWERPC is set up for pascal
 
 	Bugs:		Report bugs to Radar component ÒSystem InterfacesÓ, ÒLatestÓ
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __CONDITIONALMACROS__
#define __CONDITIONALMACROS__

/*
***************************************************************************************************
	UNIVERSAL_INTERFACES_VERSION
	
		0x0300 => version 3.0
		0x0210 => version 2.1
		This conditional did not exist prior to version 2.1
***************************************************************************************************
*/
#define UNIVERSAL_INTERFACES_VERSION 0x0300
/*
***************************************************************************************************
	GENERATINGPOWERPC		- Compiler is generating PowerPC instructions
	GENERATING68K			- Compiler is generating 68k family instructions

		Invariant:
			GENERATINGPOWERPC != GENERATING68K
***************************************************************************************************
*/

#ifdef __GNUC__
#define pascal
#define GCC_PACKED	__attribute__ ((packed))
#else
#define GCC_PACKED
#endif

#ifdef GENERATINGPOWERPC
#ifndef GENERATING68K
#define GENERATING68K !GENERATINGPOWERPC
#endif
#endif
#ifdef GENERATING68K
#ifndef GENERATINGPOWERPC
#define GENERATINGPOWERPC !GENERATING68K
#endif
#endif
#ifndef GENERATINGPOWERPC
#if defined(powerc) || defined(__powerc)
#define GENERATINGPOWERPC 1
#else
#define GENERATINGPOWERPC 0
#endif
#endif
#ifndef GENERATING68K
#if GENERATINGPOWERPC
#define GENERATING68K 0
#else
#define GENERATING68K 1
#endif
#endif
/*
***************************************************************************************************
	GENERATING68881			- Compiler is generating mc68881 floating point instructions
	
		Invariant:
			GENERATING68881 => GENERATING68K
***************************************************************************************************
*/
#if GENERATING68K
#if defined(applec) || defined(__SC__)
#ifdef mc68881
#define GENERATING68881 1
#endif
#else
#ifdef __MWERKS__
#if __MC68881__
#define GENERATING68881 1
#endif
#endif
#endif
#endif
#ifndef GENERATING68881
#define GENERATING68881 0
#endif
/*
***************************************************************************************************
	GENERATINGCFM			- Code being generated assumes CFM calling conventions
	CFMSYSTEMCALLS			- No A-traps.  Systems calls are made using CFM and UPP's

		Invariants:
			GENERATINGPOWERPC => GENERATINGCFM
			GENERATINGPOWERPC => CFMSYSTEMCALLS
			CFMSYSTEMCALLS => GENERATINGCFM
***************************************************************************************************
*/
#if GENERATINGPOWERPC || defined(__CFM68K__)
#define GENERATINGCFM 1
#define CFMSYSTEMCALLS 1
#else
#define GENERATINGCFM 0
#define CFMSYSTEMCALLS 0
#endif
/*
***************************************************************************************************
	One or none of the following BUILDING_Å conditionals is expected to be set during 
	compilation (e.g. MrC -d BUILDING_FOR_SYSTEM7), the others should be left undefined.
	If none is set, BUILDING_FOR_SYSTEM7_AND_SYSTEM8 is used.
	
		BUILDING_FOR_SYSTEM7				- Code is intended to run on System 7.x machine or earlier .
		BUILDING_FOR_SYSTEM7_AND_SYSTEM8	- Code is intended to run on System 7 or Copland.
		BUILDING_FOR_SYSTEM8				- Code is intended to run on Copland only.
		BUILDING_PREEMPTIVE_CODE			- Code is intended to run as Copland server or driver.
		
	The following conditionals are set up based on which of the BUILDING_Å flag (above) was specified.
	They are used in throughout the interface files to conditionalize declarations.
	
		FOR_SYSTEM7_ONLY					- In System 7. Not in Copland.
		FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED	- In System 7. Works in Copland, but there is a better way.
		FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE	- In System 7. In Copland, but only for cooperative tasks.
		FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE	- In System 7. In Copland.
		FOR_SYSTEM8_COOPERATIVE				- Not in System 7.  In Copland, but only for cooperative tasks.
		FOR_SYSTEM8_PREEMPTIVE				- Not in System 7.  In Copland.
		
		FOR_OPAQUE_SYSTEM_DATA_STRUCTURES	- Always true for system 8, but can be set by developer to
											  true or false for System 7.  When true, the contents of 
											  many system data structures are removed from the interfaces.
											  In the future, the Mac OS will have fewer data structures 
											  shared between applications and the system.  The problem
											  with shared data is 1) the system has to poll the data
											  to detect changes made by an application, 2) it prevents
											  data structures from being changed in the future.
											  Procedural interface will be used instead.
											  
		FOR_PTR_BASED_AE					- This is a temporary fix for Copland DR1.  It is needed to
											  distinguish between pointer based and handle based AppleEvents.
											  If you are in the case of BUILDING_FOR_SYSTEM7_AND_SYSTEM8
											  and want to use new pointer base AppleEvents, you will need to
											  -d FOR_PTR_BASED_AE on your compiler command line.
		
***************************************************************************************************
*/
#if 0
/* extra if statement is to work around a bug in PPCAsm 1.2a2 */
#elif defined(BUILDING_FOR_SYSTEM7)
#ifndef FOR_OPAQUE_SYSTEM_DATA_STRUCTURES
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 0
#endif
#define FOR_PTR_BASED_AE 0
#define FOR_SYSTEM7_ONLY 1
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 1
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 0
#define FOR_SYSTEM8_PREEMPTIVE 0
#elif defined(BUILDING_FOR_SYSTEM7_AND_SYSTEM8)
#ifndef FOR_OPAQUE_SYSTEM_DATA_STRUCTURES
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 1
#endif
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 0
#endif
#define FOR_SYSTEM7_ONLY 0
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 1
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM8_PREEMPTIVE 1
#elif defined(BUILDING_FOR_SYSTEM7_AND_SYSTEM)
/* xlc has a limit of 31 chars for command line defines, so redefine above clipped to 31 chars */
#ifndef FOR_OPAQUE_SYSTEM_DATA_STRUCTURES
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 1
#endif
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 0
#endif
#define FOR_SYSTEM7_ONLY 0
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 1
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM8_PREEMPTIVE 1
#elif defined(BUILDING_FOR_SYSTEM8)
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 1
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 1
#endif
#if !FOR_PTR_BASED_AE
#error FOR_PTR_BASED_AE must be 1
#endif
#define FOR_SYSTEM7_ONLY 0
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 0
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM8_PREEMPTIVE 1
#elif defined(BUILDING_PREEMPTIVE_CODE)
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 1
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 1
#endif
#if !FOR_PTR_BASED_AE
#error FOR_PTR_BASED_AE must be 1
#endif
#define FOR_SYSTEM7_ONLY 0
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 0
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 0
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 0
#define FOR_SYSTEM8_PREEMPTIVE 1
#else
/* default is BUILDING_FOR_SYSTEM7_AND_SYSTEM8  */
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 1
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 0
#endif
#define FOR_SYSTEM7_ONLY 0
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 1
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM8_PREEMPTIVE 1
#endif
/*
***************************************************************************************************

	OLDROUTINENAMES			- "Old" names for Macintosh system calls are allowed in source code.
							  (e.g. DisposPtr instead of DisposePtr). The names of system routine
							  are now more sensitive to change because CFM binds by name.  In the 
							  past, system routine names were compiled out to just an A-Trap.  
							  Macros have been added that each map an old name to its new name.  
							  This allows old routine names to be used in existing source files,
							  but the macros only work if OLDROUTINENAMES is true.  This support
							  will be removed in the near future.  Thus, all source code should 
							  be changed to use the new names! You can set OLDROUTINENAMES to false
							  to see if your code has any old names left in it.
	
	OLDROUTINELOCATIONS     - "Old" location of Macintosh system calls are used.  For example, c2pstr 
							  has been moved from Strings to TextUtils.  It is conditionalized in
							  Strings with OLDROUTINELOCATIONS and in TextUtils with !OLDROUTINELOCATIONS.
							  This allows developers to upgrade to newer interface files without suddenly
							  all their code not compiling becuase of "incorrect" includes.  But, it
							  allows the slow migration of system calls to more understandable file
							  locations.  OLDROUTINELOCATIONS currently defaults to true, but eventually
							  will default to false.

***************************************************************************************************
*/
#ifndef OLDROUTINENAMES
#define OLDROUTINENAMES 0
#endif
#ifndef OLDROUTINELOCATIONS
#define OLDROUTINELOCATIONS 0
#endif
/*
***************************************************************************************************
	C specific conditionals

	CGLUESUPPORTED			- Interface library will support "C glue" functions (function names
							  are: all lowercase, use C strings instead of pascal strings, use 
							  Point* instead of Point).

	PRAGMA_ALIGN_SUPPORTED	- Compiler supports "#pragma align=..." directives. The only compilers that
							  can get by without supporting the pragma are old classic 68K compilers
							  that will only be used to compile older structs that have 68K alignment
							  anyways.  
	
	PRAGMA_IMPORT_SUPPORTED	- Compiler supports "#pragma import on/off" directives.  These directives
							  were introduced with the SC compiler which supports CFM 68K.  The directive
							  is used to tell the compiler which functions will be called through a 
							  transition vector (instead of a simple PC-relative offset).  This allows 
							  the compiler to generate better code.  Since System Software functions are
							  implemented as shared libraries and called through transition vectors,
							  all System Software functions are declared with "#pragma import on".
							  
		Invariants:
			PRAGMA_IMPORT_SUPPORTED => CFMSYSTEMCALLS
			GENERATINGPOWERPC => PRAGMA_ALIGN_SUPPORTED
***************************************************************************************************
*/
#ifndef CGLUESUPPORTED
#ifdef THINK_C
#define CGLUESUPPORTED 0
#else
#define CGLUESUPPORTED 1
#endif
#endif
/*
	All PowerPC compilers support pragma align
	For 68K, only Metrowerks and SC 8.0 support pragma align 
*/
#ifndef PRAGMA_ALIGN_SUPPORTED
	#if GENERATINGPOWERPC || defined(__MWERKS__) || ( defined(__SC__) && (__SC__ >= 0x0800) )
		#define  PRAGMA_ALIGN_SUPPORTED 1
	#else
		#define  PRAGMA_ALIGN_SUPPORTED 0
	#endif
#endif
/* pragma import on/off is supported by: Metowerks CW7 and later, SC 8.0 and later, MrC 2.0 and later */
/* pragma import reset is supported by: Metrowerks and MrC 2.0.2 and later, but not yet by SC */
#ifndef PRAGMA_IMPORT_SUPPORTED
	#if ( defined(__MWERKS__) && (__MWERKS__ >= 0x0700) ) || ( defined(__SC__) && (__SC__ >= 0x0800) )
		#define  PRAGMA_IMPORT_SUPPORTED 1
	#elif defined(__MRC__) && ((__MRC__ & 0x0F00) == 0x0200) /* MrC 1.0 used 0x0704 and 0x0800 */
		#define  PRAGMA_IMPORT_SUPPORTED 1
	#else
		#define  PRAGMA_IMPORT_SUPPORTED 0
	#endif
#endif
/*
***************************************************************************************************
	The following macros isolate the use of inlines from the routine prototypes.
	A routine prototype will always be followed by on of these inline macros with
	a list of the opcodes to be inlined.  On the 68K side, the appropriate inline
	code will be generated.  On platforms that use code fragments, the macros are
	essentially NOPs.
***************************************************************************************************
*/
#if CFMSYSTEMCALLS
#define ONEWORDINLINE(trapNum)
#define TWOWORDINLINE(w1,w2)
#define THREEWORDINLINE(w1,w2,w3)
#define FOURWORDINLINE(w1,w2,w3,w4)
#define FIVEWORDINLINE(w1,w2,w3,w4,w5)
#define SIXWORDINLINE(w1,w2,w3,w4,w5,w6)
#define SEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7)
#define EIGHTWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8)
#define NINEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9)
#define TENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10)
#define ELEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11)
#define TWELVEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12)
#else
#define ONEWORDINLINE(trapNum) = trapNum
#define TWOWORDINLINE(w1,w2) = {w1,w2}
#define THREEWORDINLINE(w1,w2,w3) = {w1,w2,w3}
#define FOURWORDINLINE(w1,w2,w3,w4)  = {w1,w2,w3,w4}
#define FIVEWORDINLINE(w1,w2,w3,w4,w5) = {w1,w2,w3,w4,w5}
#define SIXWORDINLINE(w1,w2,w3,w4,w5,w6)	 = {w1,w2,w3,w4,w5,w6}
#define SEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7) 	 = {w1,w2,w3,w4,w5,w6,w7}
#define EIGHTWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8) 	 = {w1,w2,w3,w4,w5,w6,w7,w8}
#define NINEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9}
#define TENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10)  = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10}
#define ELEVENWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11}
#define TWELVEWORDINLINE(w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12) 	 = {w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,w12}
#endif

#endif /* __CONDITIONALMACROS__ */

