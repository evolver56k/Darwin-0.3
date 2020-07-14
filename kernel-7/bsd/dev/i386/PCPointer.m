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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * PCPointer.m - Generic PCPointer device class.
 * 
 *
 * HISTORY
 * 01-Dec-92	Joe Pasqua at NeXT
 *      Created. 
 */
 
// TO DO:
//
// Notes:
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.
//

#undef KERNEL_BUILD
#undef _KERNEL_PRIVATE
#define MACH_USER_API	1

#import	<objc/Object.h>
#import <driverkit/driverServer.h>
#import <driverkit/generalFuncs.h>
#import <bsd/dev/i386/PCPointer.h>
#import <bsd/dev/i386/PCPointerDefs.h>
#import <bsd/dev/i386/EventSrcPCPointer.h>

static id activePointerDevice;

@implementation PCPointer

- (int)getResolution
{
    // Subclasses must implement this method for proper operation
    return 50;	// This is a standard sort of value.
}

- (BOOL)setEventTarget:eventTarget
{
    if ( [eventTarget conformsTo:@protocol(PCPointerTarget)] )
    {
	target = eventTarget;
	return TRUE;
    }
    else
    {
	IOLog( "PCPointer setEventTarget: new target [%s] does not "
	    "implement PCPointerTarget protocol.\n",
	    object_getClassName(eventTarget) );
	return FALSE;
    }
}

PCPatoi(char *p)
{
	int n = 0;
	int f = 0;

	for(;;p++) {
		switch(*p) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
		case '+':
			p++;
		}
		break;
	}
	while(*p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
	return(f? -n: n);
}

- (BOOL)mouseInit:deviceDescription
{
    return NO;
}

+ (id) activePointerDevice
{
    return activePointerDevice;
}

+ (BOOL)probe:deviceDescription
{
    PCPointer *inst;
    //char nameBuf[20];
    static IOObjectNumber nextUnit;	// Initial value is 0

    inst = [[self alloc] initFromDeviceDescription:deviceDescription];
    inst->target = nil;	// No one is the target of mouse events yet

    // Initialize the specific rodent in question.
    if ([inst mouseInit:deviceDescription] == NO) {
	IOLog("PCPointer probe: mouseInit failure\n");
	[inst free];
	return NO;
    }
    else {
	//sprintf(nameBuf, "PCPointer%d", nextUnit);
	[inst setUnit:nextUnit++];
	//[inst setName:nameBuf];
	// [self setDeviceKind:"SpecificType"]; was done by the subclass
	[inst registerDevice];
	activePointerDevice = inst;
    }
    
    return YES;
}

- (IOReturn)getIntValues:(unsigned *)parameterArray
    forParameter:(IOParameterName)parameterName count:(unsigned *)count
{
    if (strcmp(parameterName, RESOLUTION) == 0) {
        parameterArray[0] = resolution;
        return IO_R_SUCCESS;
    } else if (strcmp(parameterName, INVERTED) == 0) {
	parameterArray[0] = inverted;
	return IO_R_SUCCESS;
    } else {
	return IO_R_UNSUPPORTED;
    }
}

- (IOReturn)setIntValues:(unsigned *)parameterArray
    forParameter:(IOParameterName)parameterName count:(unsigned)count
{
    if (strcmp(parameterName, RESOLUTION) == 0) {
	resolution = parameterArray[0];
	[target setResolution:[self getResolution]];
	return IO_R_SUCCESS;
    } else if (strcmp(parameterName, INVERTED) == 0) {
	inverted = parameterArray[0];
	[target setInverted:inverted];
	return IO_R_SUCCESS;
    } else {
	return IO_R_UNSUPPORTED;
    }
}

- (BOOL) getInverted
{
    return inverted;
}
 
@end
