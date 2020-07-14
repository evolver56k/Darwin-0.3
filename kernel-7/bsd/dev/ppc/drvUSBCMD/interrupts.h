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
 	File:		Interrupts.h
 
 	Contains:	Interface to the Interrupt Manager.
 
 	Version:	Technology:	PowerSurge 1.0.2.
 				Package:	Universal Interfaces 2.1.2 on ETO #20
 
 	Copyright:	© 1984-1995 by Apple Computer, Inc.
 				All rights reserved.
 
 	Bugs?:		If you find a problem with this file, use the Apple Bug Reporter
 				stack.  Include the file and version information (from above)
 				in the problem description and send to:
 					Internet:	apple.bugs@applelink.apple.com
 					AppleLink:	APPLE.BUGS
 
*/

#ifndef __INTERRUPTS__
#define __INTERRUPTS__


#ifndef __TYPES__
//naga#include "types.h"
#endif
/*	#include <ConditionalMacros.h>								*/

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

//naga typedef KernelID InterruptSetID;
typedef int InterruptSetID;

typedef long InterruptMemberNumber;

struct InterruptSetMember {
	InterruptSetID					setID;
	InterruptMemberNumber			member;
};
typedef struct InterruptSetMember InterruptSetMember;


enum {
	kISTChipInterruptSource		= 0,
	kISTOutputDMAInterruptSource = 1,
	kISTInputDMAInterruptSource	= 2,
	kISTPropertyMemberCount		= 3
};

typedef InterruptSetMember ISTProperty[kISTPropertyMemberCount];

#define kISTPropertyName	"driver-ist" 

typedef long InterruptReturnValue;


enum {
	kFirstMemberNumber			= 1,
	kIsrIsComplete				= 0,
	kIsrIsNotComplete			= -1,
	kMemberNumberParent			= -2
};

typedef Boolean InterruptSourceState;


enum {
	kSourceWasEnabled			= true,
	kSourceWasDisabled			= false
};

typedef InterruptMemberNumber (*InterruptHandler)(InterruptSetMember ISTmember, void *refCon, UInt32 theIntCount);
typedef void (*InterruptEnabler)(InterruptSetMember ISTmember, void *refCon);
typedef InterruptSourceState (*InterruptDisabler)(InterruptSetMember ISTmember, void *refCon);

enum {
	kReturnToParentWhenComplete	= 0x00000001,
	kReturnToParentWhenNotComplete = 0x00000002
};

typedef OptionBits InterruptSetOptions;

/*  Interrupt Services  */
extern OSStatus CreateInterruptSet(InterruptSetID parentSet, InterruptMemberNumber parentMember, InterruptMemberNumber setSize, InterruptSetID *setID, InterruptSetOptions options);
extern OSStatus InstallInterruptFunctions(InterruptSetID setID, InterruptMemberNumber member, void *refCon, InterruptHandler handlerFunction, InterruptEnabler enableFunction, InterruptDisabler disableFunction);
extern OSStatus GetInterruptFunctions(InterruptSetID setID, InterruptMemberNumber member, void **refCon, InterruptHandler *handlerFunction, InterruptEnabler *enableFunction, InterruptDisabler *disableFunction);
extern OSStatus ChangeInterruptSetOptions(InterruptSetID setID, InterruptSetOptions options);
extern OSStatus GetInterruptSetOptions(InterruptSetID setID, InterruptSetOptions *options);

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INTERRUPTS__ */
