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
 	File:		ConditionalMacros.h
 
 	Contains:	Set up for compiler independent conditionals
 
 	Version:	Universal Interface Files 3.0dx
 
 	DRI:		Nick Kledzik
 
 	Copyright:	© 1993-1997 by Apple Computer, Inc., all rights reserved
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file may contain unreleased API's
 
 	BuildInfo:	Built by:			Scott Roberts
 				With Interfacer:	3.0d2   (PowerPC native)
 				From:				ConditionalMacros.i
 					Revision:		75
 					Dated:			6/11/97
 					Last change by:	ngk
 					Last comment:	Restore TYPE_LONGLONG for MrC - changed type for AbsoluteTime in
 
 	Bugs:		Report bugs to Radar component "System Interfaces", "Latest"
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __CONDITIONALMACROS__
#define __CONDITIONALMACROS__

/****************************************************************************************************
	UNIVERSAL_INTERFACES_VERSION
	
		0x0300 => version 3.0
		0x0210 => version 2.1
		This conditional did not exist prior to version 2.1
****************************************************************************************************/
#define UNIVERSAL_INTERFACES_VERSION 0x0300

/****************************************************************************************************

	TARGET_CPU_‰	
	These conditionals specify which microprocessor instruction set is being
	generated.  At most one of these is true, the rest are false.

		TARGET_CPU_PPC			- Compiler is generating PowerPC instructions
		TARGET_CPU_68K			- Compiler is generating 680x0 instructions
		TARGET_CPU_X86			- Compiler is generating x86 instructions
		TARGET_CPU_MIPS			- Compiler is generating MIPS instructions
		TARGET_CPU_SPARC		- Compiler is generating Sparc instructions
		TARGET_CPU_ALPHA		- Compiler is generating Dec Alpha instructions


	TARGET_OS_‰	
	These conditionals specify in which Operating System the generated code will
	run. At most one of the these is true, the rest are false.

		TARGET_OS_MAC			- Generate code will run under Mac OS
		TARGET_OS_WIN32			- Generate code will run under 32-bit Windows
		TARGET_OS_UNIX			- Generate code will run under some unix 
		TARGET_OS_RHAPSODY		- Generate code will run under Rhapsody 


	TARGET_RT_‰	
	These conditionals specify in which runtime the generated code will
	run. This is needed when the OS and CPU support more than one runtime
	(e.g. MacOS on 68K supports CFM68K and Classic 68k).

		TARGET_RT_LITTLE_ENDIAN	- Generated code uses little endian format for integers
		TARGET_RT_BIG_ENDIAN	- Generated code uses big endian format for integers 	
		TARGET_RT_MAC_CFM		- TARGET_OS_MAC is true and CFM68K or PowerPC CFM being used	
		TARGET_RT_MAC_68881		- TARGET_OS_MAC is true and 68881 floating point instructions used	


	PRAGMA_‰
	These conditionals specify whether the compiler supports particular #pragma's
	
		PRAGMA_IMPORT 			- Compiler supports: #pragma import on/off/reset
		PRAGMA_ONCE  			- Compiler supports: #pragma once
		PRAGMA_STRUCT_ALIGN  	- Compiler supports: #pragma options align=mac68k/power/reset
		PRAGMA_STRUCT_PACK		- Compiler supports: #pragma pack(n)
		PRAGMA_STRUCT_PACKPUSH	- Compiler supports: #pragma pack(push, n)/pack(pop)
		PRAGMA_ENUM_PACK 		- Compiler supports: #pragma options(!pack_enums)
		PRAGMA_ENUM_ALWAYSINT 	- Compiler supports: #pragma enumsalwaysint on/off/reset
		PRAGMA_ENUM_OPTIONS		- Compiler supports: #pragma options enum=int/small/reset

	FOUR_CHAR_CODE
	This conditional does the proper byte swapping to assue that a four character code (e.g. 'TEXT')
	is compiled down to the correct value on all compilers.

		FOUR_CHAR_CODE('abcd')	- Convert a four-char-code to the correct 32-bit value

	TYPE_‰
	These conditionals specify whether the compiler supports particular types.

		TYPE_LONGLONG			- Compiler supports "long long" 64-bit integers
		TYPE_BOOL				- Compiler supports "bool"
		TYPE_EXTENDED			- Compiler supports "extended" 80/96 bit floating point

****************************************************************************************************/

#if defined(__MRC__)
	/*
		MrC[pp] compiler from Apple Computer, Inc.
	*/
	#define TARGET_CPU_PPC  			1
	#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			1
	#define TARGET_RT_MAC_68881			0
	#if (__MRC__  > 0x0200) && (__MRC__ < 0x0700)
		#define PRAGMA_IMPORT			1
	#else
		#define PRAGMA_IMPORT			0
	#endif
	#define PRAGMA_STRUCT_ALIGN			1
	#define PRAGMA_ONCE					1
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
		
	#if (__MRC__  > 0x0300) && (__MRC__ < 0x0700)
		#if __option(longlong)
			#define TYPE_LONGLONG		1
		#else
			#define TYPE_LONGLONG		0
		#endif
		#if __option(bool)
			#define TYPE_BOOL			1
		#else
			#define TYPE_BOOL			0
		#endif
	#else
		#define TYPE_LONGLONG			0
		#define TYPE_BOOL				0
	#endif
	#define TYPE_EXTENDED				0
	


#elif defined(__SC__) && (defined(MPW_CPLUS) || defined(MPW_C))
	/*
		SC[pp] compiler from Apple Computer, Inc.
	*/
	#define TARGET_CPU_PPC  			0
	#define TARGET_CPU_68K  			1
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#if defined(__CFM68K__)
		#define TARGET_RT_MAC_CFM		1
	#else
		#define TARGET_RT_MAC_CFM		0
	#endif
	#if defined(mc68881)
		#define TARGET_RT_MAC_68881		1
	#else
		#define TARGET_RT_MAC_68881		0
	#endif
	#if TARGET_RT_MAC_CFM 
		#define PRAGMA_IMPORT			1
		#if (__SC__ <= 0x0810)
			/* old versions of SC don't support ³#pragma import reset² */
			#define PRAGMA_IMPORT_OFF	1
		#endif
	#else
		#define PRAGMA_IMPORT			0
	#endif
	#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			1
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
	
	#define TYPE_LONGLONG				0
	#define TYPE_EXTENDED				1
	#if (__SC__  > 0x0810)
		#if __option(bool)
			#define TYPE_BOOL			1
		#else
			#define TYPE_BOOL			0
		#endif
	#else
		#define TYPE_BOOL				0
	#endif
		


#elif defined(x__MWERKS__)
	/*
		CodeWarrior compiler from Metrowerks, Inc.
	*/
	#if (__MWERKS__ < 0x0900) || macintosh
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				0
		#define TARGET_OS_UNIX				1
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0		
		#define TARGET_CPU_ALPHA			0
		#define TARGET_RT_LITTLE_ENDIAN		0
		#define TARGET_RT_BIG_ENDIAN		1
		#if powerc
			#define TARGET_CPU_PPC  		1
			#define TARGET_CPU_68K  		0
			#define TARGET_RT_MAC_CFM		1
			#define TARGET_RT_MAC_68881		0
		#else
			#define TARGET_CPU_PPC  		0
			#define TARGET_CPU_68K  		1
			#if defined(__CFM68K__)
				#define TARGET_RT_MAC_CFM	1
			#else
				#define TARGET_RT_MAC_CFM	0
			#endif
			#if __MC68881__
				#define TARGET_RT_MAC_68881	1
			#else
				#define TARGET_RT_MAC_68881	0
			#endif
		#endif
	#elif (__MWERKS__ >= 0x0900) && __INTEL__
		#define TARGET_CPU_PPC  			0
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			1
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0		
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				1
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		1
		#define TARGET_RT_BIG_ENDIAN		0
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
	#else	
		#error unknown Metrowerks compiler
	#endif


	#if (__MWERKS__ >= 0x0700)
		#define PRAGMA_IMPORT				TARGET_RT_MAC_CFM
	#else
		#define PRAGMA_IMPORT				0
	#endif
	#define PRAGMA_STRUCT_ALIGN				1
	#define PRAGMA_ONCE						1
	#define PRAGMA_STRUCT_PACK				0
	#define PRAGMA_STRUCT_PACKPUSH			0
	#define PRAGMA_ENUM_PACK				0
	#define PRAGMA_ENUM_ALWAYSINT			1
	#define PRAGMA_ENUM_OPTIONS				0
	#define FOUR_CHAR_CODE(x)				(x)
	
	#if (__MWERKS__ >= 0x1100)
		#if __option(longlong)
			#define TYPE_LONGLONG		1
			#define _LONG_LONG			1		/* temporary */
		#else
			#define TYPE_LONGLONG		0
		#endif
	#else
		#define TYPE_LONGLONG			0
	#endif
	#if (__MWERKS__ >= 0x1000)
		#if __option(bool)
			#define TYPE_BOOL			1
		#else
			#define TYPE_BOOL			0
		#endif
	#else
		#define TYPE_BOOL				0
	#endif

	#if TARGET_OS_MAC && TARGET_CPU_68K
		typedef long double extended;
		#define TYPE_EXTENDED			1
	#else
		#define TYPE_EXTENDED			0
	#endif



#elif defined(SYMANTEC_CPLUS) || defined(SYMANTEC_C)
	/*
		C and C++ compiler from Symantec, Inc.
	*/
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#if powerc
		#define TARGET_CPU_PPC  		1
		#define TARGET_CPU_68K  		0
		#define TARGET_RT_MAC_CFM		1
		#define TARGET_RT_MAC_68881		0
	#else
		#define TARGET_CPU_PPC  		0
		#define TARGET_CPU_68K  		1
		#if defined(__CFM68K)
			#define TARGET_RT_MAC_CFM	1
		#else
			#define TARGET_RT_MAC_CFM	0
		#endif
		#if mc68881
			#define TARGET_RT_MAC_68881	1
		#else
			#define TARGET_RT_MAC_68881	0
		#endif
	#endif
	#define PRAGMA_IMPORT				0
	#define PRAGMA_ONCE					1
	#define PRAGMA_STRUCT_ALIGN			1
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			1
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
	
	#if __useAppleExts__
		#define TYPE_EXTENDED			1
	#else
		#define TYPE_EXTENDED			0
	#endif
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0
	


#elif defined(THINK_C)
	/*
		THINK C compiler from Symantec, Inc.		<< WARNING: Unsupported Compiler >>
	*/
	#define TARGET_CPU_PPC  			0
	#define TARGET_CPU_68K  			1
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			0
	#if defined(mc68881)
		#define TARGET_RT_MAC_68881		1
	#else
		#define TARGET_RT_MAC_68881		0
	#endif
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_ONCE					1
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			1
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
	
	#define TYPE_EXTENDED				1
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0
	


#elif defined(__PPCC__)
	/*
		PPCC compiler from Apple Computer, Inc.		<< WARNING: Unsupported Compiler >>
	*/
	#define TARGET_CPU_PPC  			1
	#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			1
	#define TARGET_RT_MAC_68881			0
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			1
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
	
	#define TYPE_EXTENDED				0
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0


#elif defined(applec) && !defined(__SC__)
	/*
		MPW C compiler from Apple Computer, Inc.	<< WARNING: Unsupported Compiler >>
	*/
	#define TARGET_CPU_PPC  			0
	#define TARGET_CPU_68K  			1
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			0
	#if defined(mc68881)
		#define TARGET_RT_MAC_68881		1
	#else
		#define TARGET_RT_MAC_68881		0
	#endif
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	/* Note: MPW C 3.2 had a bug where MACRO('xx   ') would cause 'xx  ' to be misevaluated */
	#define FOUR_CHAR_CODE				

	#define TYPE_EXTENDED				1
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0



#elif defined(__GNUC__)
	/*
		gC from Free Software Foundation, Inc.
	*/
#if 0

	#if #cpu(powerpc)
		#define TARGET_CPU_PPC  		1
		#define TARGET_CPU_68K  		0
		#define TARGET_CPU_X86  		0
		#define TARGET_CPU_MIPS 		0
		#define TARGET_CPU_SPARC		0	
		#define TARGET_CPU_ALPHA		0
		#define TARGET_RT_MAC_CFM		1
		#define TARGET_RT_MAC_68881		0
	#elif #cpu(m68k)
		#define TARGET_CPU_PPC  		0
		#define TARGET_CPU_68K  		1
		#define TARGET_CPU_X86  		0
		#define TARGET_CPU_MIPS 		0
		#define TARGET_CPU_SPARC		0	
		#define TARGET_CPU_ALPHA		0
		#define TARGET_RT_MAC_CFM		0
		#define TARGET_RT_MAC_68881		0
	#else
		#error unsupported GNU C compiler
	#endif
	
	#if #system(macos)
		#define TARGET_OS_MAC			1
		#define TARGET_OS_WIN32			0
		#define TARGET_OS_UNIX			0
	#elif #system(unix)
		#define TARGET_OS_MAC			0
		#define TARGET_OS_WIN32			0
		#define TARGET_OS_UNIX			1
	#else
		#error unsupported GNU C compiler
	#endif
#endif

#define TARGET_CPU_PPC  		1
#define TARGET_CPU_68K  		0
#define TARGET_CPU_X86  		0
#define TARGET_CPU_MIPS 		0
#define TARGET_CPU_SPARC		0	
#define TARGET_CPU_ALPHA		0
#define TARGET_RT_MAC_CFM		0
#define TARGET_RT_MAC_68881		0

#define TARGET_OS_MAC			0
#define TARGET_OS_WIN32			0
#define TARGET_OS_UNIX			0
#define TARGET_OS_RHAPSODY		1
	
#define DEBUG_BUILD				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			1
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define DWORD				unsigned long
	#define FOUR_CHAR_CODE(x) x
//	#define FOUR_CHAR_CODE(x)		(((DWORD) ((x) & 0x000000FF)) << 24) \
//                                                        | (((DWORD) ((x) & 0x0000FF00)) << 8) \
//                                                        | (((DWORD) ((x) & 0x00FF0000)) >> 8) \
//                                                        | (((DWORD) ((x) & 0xFF000000)) >> 24)

	#define TYPE_EXTENDED				0
	#ifdef _LONG_LONG
		#define TYPE_LONGLONG			1
	#else
		#define TYPE_LONGLONG			0
	#endif
	#define TYPE_BOOL					0



#elif defined(__xlc) || defined(__xlC) || defined(__xlC__) || defined(__XLC121__)
	/*
		xlc and xlC on RS/6000 from IBM, Inc.
	*/
	#define TARGET_CPU_PPC  			1
	#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				1	
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				0
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			1
	#define TARGET_RT_MAC_68881			0
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			1
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			0
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			1
	#define FOUR_CHAR_CODE(x)			(x)

	#define TYPE_EXTENDED				0
	#ifdef _LONG_LONG
		#define TYPE_LONGLONG			1
	#else
		#define TYPE_LONGLONG			0
	#endif
	#define TYPE_BOOL					0


#elif defined(_MSC_VER) && !defined(__MWERKS__) 
	/*
		Visual C++ from Microsoft, Inc.
	*/
	#if defined(_M_M68K)	/* Visual C++ with Macintosh 68K target */
		#define TARGET_CPU_PPC  			0
		#define TARGET_CPU_68K  			1
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				1
		#define TARGET_OS_WIN32				0
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		0
		#define TARGET_RT_BIG_ENDIAN		1
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			1
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)			(x)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#define TYPE_BOOL					0
	#elif defined(_M_MPPC)	/* Visual C++ with Macintosh PowerPC target */
		#define TARGET_CPU_PPC  			1
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				1
		#define TARGET_OS_WIN32				0
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		0
		#define TARGET_RT_BIG_ENDIAN		1
		#define TARGET_RT_MAC_CFM			1
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			1
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)			(x)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#define TYPE_BOOL					0

	#elif defined(_M_IX86)	/* Visual C++ with Intel x86 target */
		#define TARGET_CPU_PPC  			0
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			1
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				1
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		1
		#define TARGET_RT_BIG_ENDIAN		0
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			0
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)		(((DWORD) ((x) & 0x000000FF)) << 24) \
									| (((DWORD) ((x) & 0x0000FF00)) << 8) \
									| (((DWORD) ((x) & 0x00FF0000)) >> 8) \
									| (((DWORD) ((x) & 0xFF000000)) >> 24)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#if defined(__cplusplus) && (_MSC_VER >= 1100)
			#define TYPE_BOOL				1
		#else
			#define TYPE_BOOL				0
		#endif

		#elif defined(_M_ALPHA)		/* Visual C++ with Dec Alpha target */
		#define TARGET_CPU_PPC  			0
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			1
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				1
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		1
		#define TARGET_RT_BIG_ENDIAN		0
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			0
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)		(((DWORD) ((x) & 0x000000FF)) << 24) \
									| (((DWORD) ((x) & 0x0000FF00)) << 8) \
									| (((DWORD) ((x) & 0x00FF0000)) >> 8) \
									| (((DWORD) ((x) & 0xFF000000)) >> 24)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#define TYPE_BOOL					0

	#elif defined(_M_PPC)	/* Visual C++ for Windows NT on PowerPC target */
		#define TARGET_CPU_PPC  			1
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			0
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				1
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		1
		#define TARGET_RT_BIG_ENDIAN		0
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			0
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)		(((DWORD) ((x) & 0x000000FF)) << 24) \
									| (((DWORD) ((x) & 0x0000FF00)) << 8) \
									| (((DWORD) ((x) & 0x00FF0000)) >> 8) \
									| (((DWORD) ((x) & 0xFF000000)) >> 24)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#define TYPE_BOOL					0

		#elif defined(_M_MRX000)	/* Visual C++ for Windows NT on MIPS target */
		#define TARGET_CPU_PPC  			0
		#define TARGET_CPU_68K  			0
		#define TARGET_CPU_X86  			0
		#define TARGET_CPU_MIPS 			1
		#define TARGET_CPU_SPARC			0
		#define TARGET_CPU_ALPHA			0
		#define TARGET_OS_MAC				0
		#define TARGET_OS_WIN32				1
		#define TARGET_OS_UNIX				0
		#define TARGET_RT_LITTLE_ENDIAN		1
		#define TARGET_RT_BIG_ENDIAN		0
		#define TARGET_RT_MAC_CFM			0
		#define TARGET_RT_MAC_68881			0
		#define PRAGMA_IMPORT				0
		#define PRAGMA_STRUCT_ALIGN			0
		#define PRAGMA_ONCE		 			0
		#define PRAGMA_STRUCT_PACK			1
		#define PRAGMA_STRUCT_PACKPUSH		1
		#define PRAGMA_ENUM_PACK			0
		#define PRAGMA_ENUM_ALWAYSINT		0
		#define PRAGMA_ENUM_OPTIONS			0
		#define FOUR_CHAR_CODE(x)		(((DWORD) ((x) & 0x000000FF)) << 24) \
									| (((DWORD) ((x) & 0x0000FF00)) << 8) \
									| (((DWORD) ((x) & 0x00FF0000)) >> 8) \
									| (((DWORD) ((x) & 0xFF000000)) >> 24)
		#define TYPE_EXTENDED				0
		#define TYPE_LONGLONG				0
		#define TYPE_BOOL					0
	#endif


#elif defined(__MOTO__)
	/*
		mcc from Motorola, Inc. ??
	*/



#elif defined(_MIPS_ISA)
	/*
		MIPSpro compiler from Silicon Graphics Inc.
	*/
	#define TARGET_CPU_PPC  			0
	#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			1
	#define TARGET_CPU_SPARC			0		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				0
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				1
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			0
	#define TARGET_RT_MAC_68881			0
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			1
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)			(x)
	#define TYPE_EXTENDED				0
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0


#elif defined(__sparc)
	/*
		SPARCompiler compiler from Sun Microsystems Inc.
	*/
	#define TARGET_CPU_PPC  			0
	#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			0
	#define TARGET_CPU_MIPS 			0
	#define TARGET_CPU_SPARC			1		
	#define TARGET_CPU_ALPHA			0
	#define TARGET_OS_MAC				0
	#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				1
	#define TARGET_RT_LITTLE_ENDIAN		0
	#define TARGET_RT_BIG_ENDIAN		1
	#define TARGET_RT_MAC_CFM			0
	#define TARGET_RT_MAC_68881			0
	#define PRAGMA_IMPORT				0
	#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_ONCE					0
	#define PRAGMA_STRUCT_PACK			1
	#define PRAGMA_STRUCT_PACKPUSH		0
	#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		0
	#define PRAGMA_ENUM_OPTIONS			0
	#define FOUR_CHAR_CODE(x)		(((unsigned long) ((x) & 0x000000FF)) << 24) \
									| (((unsigned long) ((x) & 0x0000FF00)) << 8) \
									| (((unsigned long) ((x) & 0x00FF0000)) >> 8) \
									| (((unsigned long) ((x) & 0xFF000000)) >> 24)
	#define TYPE_EXTENDED				0
	#define TYPE_LONGLONG				0
	#define TYPE_BOOL					0


#else
	/*
		Unknown compiler, perhaps set up from the command line (e.g. -d TARGET_CPU_MIPS , etc.)
	*/
	#if defined(TARGET_CPU_PPC) && TARGET_CPU_PPC
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_SPARC 0
		#define TARGET_CPU_ALPHA 0
	#elif defined(TARGET_CPU_68K) && TARGET_CPU_68K
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_SPARC 0
		#define TARGET_CPU_ALPHA 0
	#elif defined(TARGET_CPU_X86) && TARGET_CPU_X86
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_SPARC 0
		#define TARGET_CPU_ALPHA 0
	#elif defined(TARGET_CPU_MIPS) && TARGET_CPU_MIPS
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_SPARC 0
		#define TARGET_CPU_ALPHA 0
	#elif defined(TARGET_CPU_SPARC) && TARGET_CPU_SPARC
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_ALPHA 0
	#elif defined(TARGET_CPU_ALPHA) && TARGET_CPU_ALPHA
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_SPARC 0
	#else
		#error ConditionalMacros.h: unknown compiler
		#define TARGET_CPU_PPC   0
		#define TARGET_CPU_68K   0
		#define TARGET_CPU_X86   0
		#define TARGET_CPU_MIPS  0
		#define TARGET_CPU_SPARC 0
		#define TARGET_CPU_ALPHA 0
	#endif
	
	#if defined(TARGET_OS_RHAPSODY) && TARGET_OS_RHAPSODY
		#define TARGET_OS_WIN32  0
		#define TARGET_OS_MAC    0
		#define TARGET_OS_UNIX   1
	#endif

	#if defined(TARGET_OS_MAC) && TARGET_OS_MAC
		#define TARGET_OS_WIN32  0
		#define TARGET_OS_UNIX   0
	#elif defined(TARGET_OS_WIN32) && TARGET_OS_WIN32
		#define TARGET_OS_MAC    0
		#define TARGET_OS_UNIX   0
	#elif defined(TARGET_OS_UNIX) && TARGET_OS_UNIX
		#define TARGET_OS_MAC    0
		#define TARGET_OS_WIN32  0
	#elif TARGET_CPU_PPC || TARGET_CPU_68K
		#define TARGET_OS_MAC    1
		#define TARGET_OS_WIN32  0
		#define TARGET_OS_UNIX   0
	#else
		#error ConditionalMacros.h: unknown target OS
	#endif

	
	#if !defined(TARGET_RT_BIG_ENDIAN) && !defined(TARGET_RT_LITTLE_ENDIAN)
		#if TARGET_OS_MAC
			#define TARGET_RT_LITTLE_ENDIAN		0
			#define TARGET_RT_BIG_ENDIAN		1
		#elif TARGET_OS_WIN32
			#define TARGET_RT_LITTLE_ENDIAN		1
			#define TARGET_RT_BIG_ENDIAN		0
		#endif
	#endif
		
	#if defined(TARGET_RT_BIG_ENDIAN) && !defined(TARGET_RT_LITTLE_ENDIAN)
		#define TARGET_RT_LITTLE_ENDIAN		!TARGET_RT_BIG_ENDIAN
	#elif !defined(TARGET_RT_BIG_ENDIAN) && defined(TARGET_RT_LITTLE_ENDIAN)
		#define TARGET_RT_BIG_ENDIAN		!TARGET_RT_LITTLE_ENDIAN
	#endif
	
	#if !defined(TARGET_RT_BIG_ENDIAN) && !defined(TARGET_RT_LITTLE_ENDIAN)
		#error unknown endianess of target processor
	#endif

	#ifndef TARGET_RT_MAC_CFM
	#define TARGET_RT_MAC_CFM			TARGET_CPU_PPC
	#endif
	#ifndef TARGET_RT_MAC_68881
	#define TARGET_RT_MAC_68881			0
	#endif

		
	#ifndef PRAGMA_IMPORT
	#define PRAGMA_IMPORT				0
	#endif
	#ifndef PRAGMA_STRUCT_ALIGN
	#define PRAGMA_STRUCT_ALIGN			0
	#endif
	#ifndef PRAGMA_ONCE
	#define PRAGMA_ONCE					0
	#endif
	#ifndef PRAGMA_STRUCT_PACK
	#define PRAGMA_STRUCT_PACK			0
	#endif
	#ifndef PRAGMA_STRUCT_PACKPUSH
	#define PRAGMA_STRUCT_PACKPUSH		0
	#endif
	#ifndef PRAGMA_ENUM_PACK
	#define PRAGMA_ENUM_PACK			0
	#endif
	#ifndef PRAGMA_ENUM_ALWAYSINT
	#define PRAGMA_ENUM_ALWAYSINT		0
	#endif
	#ifndef PRAGMA_ENUM_OPTIONS
	#define PRAGMA_ENUM_OPTIONS			0
	#endif
	#ifndef FOUR_CHAR_CODE
	#define FOUR_CHAR_CODE(x)			(x)
	#endif

	#ifndef TYPE_EXTENDED
	#define TYPE_EXTENDED				0
	#endif
	#ifndef TYPE_LONGLONG
	#define TYPE_LONGLONG				0
	#endif
	#ifndef TYPE_BOOL
	#define TYPE_BOOL					0
	#endif
#endif




/****************************************************************************************************

	EXTERN_API‰	
	These conditionals are used to specify the calling convention of a function.
	Mac classic 68K and Win32 each have two possible calling conventions:

		EXTERN_API				- Classic 68K pascal, Win32 __cdecl
		EXTERN_API_C			- Classic 68K C,      Win32 __cdecl
		EXTERN_API_STDCALL		- Classic 68K pascal, Win32 __stdcall
		EXTERN_API_C_STDCALL	- Classic 68K C,      Win32 __stdcall


	CALLBACK_API‰	
	These conditionals are used to specify the calling convention of a function pointer.
	Mac classic 68K and Win32 each have two possible calling conventions:

		CALLBACK_API			- Classic 68K pascal, Win32 __stdcall
		CALLBACK_API_C			- Classic 68K C,      Win32 __stdcall
		CALLBACK_API_STDCALL	- Classic 68K pascal, Win32 __cdecl
		CALLBACK_API_C_STDCALL	- Classic 68K C,      Win32 __cdecl

****************************************************************************************************/
#if TARGET_OS_MAC
	/*
		Mac OS
	*/
	#define EXTERN_API(_type)						extern pascal _type		/* Note: all Mac OS compilers must be able to handle the pascal keyword */
	#define EXTERN_API_C(_type)						extern _type
	#define EXTERN_API_STDCALL(_type)				extern pascal _type
	#define EXTERN_API_C_STDCALL(_type)				extern _type
	#define CALLBACK_API(_type, _name)				pascal _type (*_name)
	#define CALLBACK_API_C(_type, _name)			_type (*_name)
	#define CALLBACK_API_STDCALL(_type, _name)		pascal _type (*_name)
	#define CALLBACK_API_C_STDCALL(_type, _name)	_type (*_name)	
#elif TARGET_OS_WIN32
	/*
		Win32 
	*/
	#define EXTERN_API(_type)						__declspec(dllimport) _type __cdecl
	#define EXTERN_API_C(_type)						__declspec(dllimport) _type __cdecl
	#define EXTERN_API_STDCALL(_type)				__declspec(dllimport) _type __stdcall
	#define EXTERN_API_C_STDCALL(_type)				__declspec(dllimport) _type __stdcall
	#define CALLBACK_API(_type, _name)				_type (* __cdecl _name)
	#define CALLBACK_API_C(_type, _name)			_type (* __cdecl _name)
	#define CALLBACK_API_STDCALL(_type, _name)		_type (* __stdcall _name)
	#define CALLBACK_API_C_STDCALL(_type, _name)	_type (* __stdcall _name)	
#else
	/*
		Unix or anything else
	*/
	#define EXTERN_API(_type)						extern _type
	#define EXTERN_API_C(_type)						extern _type
	#define EXTERN_API_STDCALL(_type)				extern _type
	#define EXTERN_API_C_STDCALL(_type)				extern _type
	#define CALLBACK_API(_type, _name)				_type (*_name)
	#define CALLBACK_API_C(_type, _name)			_type (*_name)
	#define CALLBACK_API_STDCALL(_type, _name)		_type (*_name)
	#define CALLBACK_API_C_STDCALL(_type, _name)	_type (*_name)	
#endif


/*  Backwards compatibility for clients expecting Universal Interfacew 3.0d7 (Interfacer 2.d014) style */
#define EXTERN_API_1	EXTERN_API_STDCALL
#define EXTERN_API_2	EXTERN_API_C_STDCALL
#define EXTERN_API_3	EXTERN_API
#define EXTERN_API_4	EXTERN_API_C
#define CALLBACK_API_1	CALLBACK_API_STDCALL
#define CALLBACK_API_2	CALLBACK_API_C_STDCALL
#define CALLBACK_API_3	CALLBACK_API
#define CALLBACK_API_4	CALLBACK_API_C

/* Remove pascal support from Rhapsody */
#ifdef TARGET_OS_RHAPSODY
#define pascal
#endif

/****************************************************************************************************
	Backward compatibility for clients expecting 2.x version on ConditionalMacros.h

	GENERATINGPOWERPC		- Compiler is generating PowerPC instructions
	GENERATING68K			- Compiler is generating 68k family instructions
	GENERATING68881			- Compiler is generating mc68881 floating point instructions
	GENERATINGCFM			- Code being generated assumes CFM calling conventions
	CFMSYSTEMCALLS			- No A-traps.  Systems calls are made using CFM and UPP's
	PRAGMA_ALIGN_SUPPORTED	- Compiler supports: #pragma options align=mac68k/power/reset
	PRAGMA_IMPORT_SUPPORTED	- Compiler supports: #pragma import on/off/reset

****************************************************************************************************/
#define GENERATINGPOWERPC TARGET_CPU_PPC
#define GENERATING68K TARGET_CPU_68K
#define GENERATING68881 TARGET_RT_MAC_68881
#define GENERATINGCFM TARGET_RT_MAC_CFM
#define CFMSYSTEMCALLS TARGET_RT_MAC_CFM
/*
   The SuperMario build sets PRAGMA_ALIGN_SUPPORTED in some cases to work around old MPW C compiler 
   using NameRegistry.  Once SM switches to Interfacer 2.0d14 the #ifndef will no longer be needed.
*/
#ifndef PRAGMA_ALIGN_SUPPORTED
	#define PRAGMA_ALIGN_SUPPORTED PRAGMA_STRUCT_ALIGN
#endif
#define PRAGMA_IMPORT_SUPPORTED PRAGMA_IMPORT
/*
	NOTE: The FOR_‰ conditionals were developed to produce integerated
		  interface files for System 7 and Copland.  Now that Copland
		  is canceled, all FOR_ conditionals have been removed from
		  the interface files.  But, just in case you someone got an 
		  interface file that uses them, the following sets the FOR_‰
		  conditionals to a consistent, usable state.
*/
#define FOR_OPAQUE_SYSTEM_DATA_STRUCTURES 0
#ifndef FOR_PTR_BASED_AE
#define FOR_PTR_BASED_AE 0
#endif  /*  ! defined(FOR_PTR_BASED_AE)  */

#define FOR_SYSTEM7_ONLY 1
#define FOR_SYSTEM7_AND_SYSTEM8_DEPRECATED 1
#define FOR_SYSTEM7_AND_SYSTEM8_COOPERATIVE 1
#define FOR_SYSTEM7_AND_SYSTEM8_PREEMPTIVE 1
#define FOR_SYSTEM8_COOPERATIVE 0
#define FOR_SYSTEM8_PREEMPTIVE 0



/****************************************************************************************************

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
							  This allows developers to upgrade to newer interface files without 
							  having to change the includes in their source code.  But, it allows
							  the slow migration of system calls to more understandable file locations.  
							  OLDROUTINELOCATIONS currently defaults to true, but eventually will 
							  default to false.

****************************************************************************************************/
#ifndef OLDROUTINENAMES
#define OLDROUTINENAMES 0
#endif  /*  ! defined(OLDROUTINENAMES)  */

#ifndef OLDROUTINELOCATIONS
#define OLDROUTINELOCATIONS 0
#endif  /*  ! defined(OLDROUTINELOCATIONS)  */



/****************************************************************************************************
	C specific conditionals

	CGLUESUPPORTED			- Interface library will support "C glue" functions (function names
							  are: all lowercase, use C strings instead of pascal strings, use 
							  Point* instead of Point).

****************************************************************************************************/
#ifndef CGLUESUPPORTED
#ifdef THINK_C
#define CGLUESUPPORTED 0
#else
#define CGLUESUPPORTED 1
#endif  /*  defined(THINK_C)  */

#endif  /*  ! defined(CGLUESUPPORTED)  */



/****************************************************************************************************
	The following macros isolate the use of 68K inlines in function prototypes.
	On the Mac OS under the Classic 68K runtime, function prototypes were followed
	by a list of 68K opcodes which the compiler inserted in the generated code instead
	of a JSR.  Under Classic 68K on the Mac OS, this macro will put the opcodes
	in the right syntax.  For all other OS's and runtimes the macro suppress the opcodes.
	Example:
	
		EXTERN_P void DrawPicture(PicHandle myPicture, const Rect *dstRect)
 			ONEWORDINLINE(0xA8F6);
	
****************************************************************************************************/

#if TARGET_OS_MAC && !TARGET_RT_MAC_CFM
	#define ONEWORDINLINE(w1) = w1
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
#else
	#define ONEWORDINLINE(w1)
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
#endif





#endif /* __CONDITIONALMACROS__ */

