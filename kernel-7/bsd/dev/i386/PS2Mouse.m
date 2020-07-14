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
 * PS2Mouse.m - PS/2 style mouse driver.
 * 
 *
 * HISTORY
 * 14-Aug-92    Joe Pasqua at NeXT
 *      Created. 
 * 19-Aug-93	Paul Frantz
 *	Moved setting the event timestamp from interruptHandler method to
 *	MouseIntHandler() - it was getting excessively delayed when the system is busy, 
 */
 
// TO DO:
//
// Notes:
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.
//

#define MACH_USER_API	1
#undef	KERNEL_PRIVATE

#import <driverkit/driverServer.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <bsd/dev/i386/PS2Mouse.h>
#import <bsd/dev/i386/PCPointerDefs.h>
#import <bsd/dev/i386/PS2Keyboard.h>
#import <bsd/dev/i386/PS2KeyboardPriv.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/driverTypes.h>
#import <bsd/dev/i386/EventSrcPCPointer.h>

// Device information
#define	MOUSE_SEQUENCE_LENGTH	3

static int indexInSequence;
static PCPointerEvent event, pendingEvent, summedEvent;

static int seqBeingProcessed, seqInProgress;

extern unsigned PS2MouseResArray[];

@implementation PS2Mouse

//
// BEGIN:	Implementation of internal support methods
//

static void MouseIntHandler(void *identity, void *state, unsigned int arg)
{
    unsigned char data;
    static ns_time_t stamp;
    
    if (!((unsigned char)inb(K_STATUS) & M_OBUF_FUL))
	return;
    IODelay(K_DATA_DELAY);
    data = (unsigned char)inb(K_RDWR);

#define	RESYNC_THRESHOLD	(250 * 1000 * 1000)	// .25 seconds
    {
	ns_time_t newStamp;
	
	IOGetTimestamp(&newStamp);
	if ((indexInSequence != 0) && (newStamp - stamp > RESYNC_THRESHOLD))
	    indexInSequence = 0;
	stamp = newStamp;
    }
    
    if (seqBeingProcessed)
    {
	seqInProgress = 1;
        pendingEvent.data.buf[indexInSequence++] = data;
	if (indexInSequence == MOUSE_SEQUENCE_LENGTH)
	{
	    summedEvent.data.buf[1] += pendingEvent.data.buf[1];
	    summedEvent.data.buf[2] += pendingEvent.data.buf[2];
	    indexInSequence = 0;
	    seqInProgress = 0;
	}
	return;
    }
    if (seqInProgress)
    {
        pendingEvent.data.buf[indexInSequence++] = data;
	if (indexInSequence == MOUSE_SEQUENCE_LENGTH)
	{
	    event.data.buf[0] = pendingEvent.data.buf[0];
	    event.data.buf[1] =
	        summedEvent.data.buf[1] + pendingEvent.data.buf[1];
	    event.data.buf[2] =
		summedEvent.data.buf[2] + pendingEvent.data.buf[2];
    	    event.timeStamp = stamp;
	    summedEvent.data.buf[1] = summedEvent.data.buf[2] = 0;
	    indexInSequence = 0;
	    seqBeingProcessed = 1;
	    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);	    
	}
	return;
    }
    
    event.data.buf[indexInSequence] = data;

    if (++indexInSequence < MOUSE_SEQUENCE_LENGTH)
    	return;

    indexInSequence = 0;
    seqBeingProcessed = 1;
    
    event.data.buf[1] += summedEvent.data.buf[1];
    event.data.buf[2] += summedEvent.data.buf[2];
    event.timeStamp = stamp;
    summedEvent.data.buf[1] = summedEvent.data.buf[2] = 0;
    
    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
}

- (BOOL)mouseInit:deviceDescription
// Description:	Initialize the mouse object.
{
    id		configTable;
    char	*val;
    IOReturn	drtn;

    [self setName:"PS2Mouse"];
    [self setDeviceKind:"PS2Mouse"];

    configTable = [[self deviceDescription] configTable];
    if (configTable == nil) {
    	IOLog("PS2Mouse mouseInit: no configuration table\n");
        return NO;
    }

// check if the mouse should be inverted or not

    val = (char *)[configTable valueForStringKey:INVERTED];
    if (val && (val[0] == 'y' || val[0] == 'Y'))
	inverted = YES;
    else
	inverted = NO;
    
// check if there's a non-default resolution

    val = (char *)[configTable valueForStringKey:RESOLUTION];
    if (val == NULL) {
        resolution = DEFAULTRES;
    	IOLog("SerialMouse mouseInit: no resolution in config table.  Default is %d\n", resolution);
   } else {
    	resolution = PCPatoi(val);
    }

    [self enableAllInterrupts];
    seqBeingProcessed = seqInProgress = 0;
    summedEvent.data.buf[1] = summedEvent.data.buf[2] = 0;
    
    drtn = IOGetObjectForDeviceName("PCKeyboard0", &kbdDevice);
    if (drtn) {
	IOLog("initPointer: Can't find PCKeyboard0 (%s)\n",
	    [self stringFromReturn:drtn]);
	return NO;
    }
    [kbdDevice attachMouse:self withPort:[self interruptPort]];

    return YES;	
}

//
// END:		Implementation of internal support methods
//

//
// BEGIN:	Implementation of EXPORTED methods
//
- free
{
    [kbdDevice detachMouse];
    return [super free];
}


- (BOOL) getHandler:(IOEISAInterruptHandler *)handler
              level:(unsigned int *) ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
    *handler = MouseIntHandler;
    *ipl = 3;
    *arg = 0xdeadbeef;
    return YES;
}


- (void)interruptHandler
{
    if (target != nil)
    {
	[target dispatchPointerEvent:&event];
    }
    seqBeingProcessed = 0;	// Finished w/ event posted by MouseIntHandler
}


- (int)getResolution
{
    return resolution;
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

//
// END:		Implementation of EXPORTED methods
//
@end
