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

// #ifndef DEBUG
// #define DEBUG 1
// #endif
#define NORMALIZE 1

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * EventSrcPCPointer.m - PC Pointer EventSrc subclass implementation
 *
 * HISTORY
 * 28 Aug 1992    Joe Pasqua
 *      Created. 
 * 5  Aug 1993	  Erik Kay at NeXT
 *	minor changes for Event driver api changes
 *
 * 19 Aug 1993	Paul Frantz
 *	Increased precision of velocity calculation by doing time calculations
 *	in units of 2**16 ns (about 65us) instead of ticks.
 *	The raw input is scaled by the resolution setting now, not just the velocity calculation.
 *
 */

// TO DO:
//
// NOTES:
// * The EventSrcExported protocol is implemented completely by our superclass.
//   At this point there is no need for this subclass to override anything.
// * In the current system the EventDriver instance is always the owner
//   of this EventSrc.
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.
//

#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <bsd/dev/evsio.h>
#import <bsd/dev/i386/EventSrcPCPointer.h> 

// Scaled ticks per second at 1 scaled_tick = 2**16 ns
#define SCALED_TICKS_PER_SEC	15259

// "Infinite" interval is 0xffff scaled ticks = 4.2 sec.
#define MAX_EVENT_DELTA		0xffff

// Reference DPI (to match black hardware characteristics)
#define REFERENCE_RESOLUTION 	72

// Default pointer acceleration or scaling table
static const unsigned int dfltPointerScaling[] =
{
	2,	2,	// Threshold and scaling pairs
	3,	3,
	4,	5,
	5,	7,
	6,	10
};

static EventSrcPCPointer *instance = (EventSrcPCPointer *)0;

#ifdef DEBUG
#define LOG_LENGTH 1000
static unsigned short velocityLog[LOG_LENGTH];
static unsigned short deltaTimestamp[LOG_LENGTH];
static unsigned short delta_log[LOG_LENGTH];
static int velocityLogP, maxVelocity;
#endif

@implementation EventSrcPCPointer: IOEventSource


//
// BEGIN:	Implementation of Private EventSrcPCPointer methods
//
- scalePointerInX:(int *)dxp
    andY:(int *)dyp
    over:(unsigned)dt
    atRes:(unsigned)res
// Description:	Perform pointer acceleration computations here.
//		Given the resolution, dx, dy, and time, compute the velocity
//		of the pointer over a Manhatten distance in inches/second.
//		Using this velocity, do a lookup in the pointerScaling table
//		to select a scaling factor. Scale dx and dy up as appropriate.
// Preconditions:
// *	deviceLock should be held on entry
{
    int dx, dy, scale;
    int absDx, absDy;
    unsigned delta, velocity;
    int index, tmp;

    dx = *dxp;
    dy = *dyp;
    absDx = (dx < 0) ? -dx : dx;
    absDy = (dy < 0) ? -dy : dy;

    // Take the sum of the X and Y distances as the delta to compute
    // the velocity. (Manhatten distance)
    delta = absDx + absDy;
    if ( dt == 0 )
	    dt++;
    //
    // Velocity in IPS = (delta in Dots * scaled_ticks/sec)/(res in DPI * dT)
    //
    // This is a bit heavyweight, and could probably be approximated
    // with considerable savings.
    //
    velocity =  (delta * SCALED_TICKS_PER_SEC) / (res *  dt);

#ifdef DEBUG
    delta_log[velocityLogP] = delta;
    velocityLog[velocityLogP++] = velocity;
    if (velocityLogP >= LOG_LENGTH)
    	velocityLogP = 0;
	
    if (velocity > maxVelocity)
    	maxVelocity = velocity;
#endif

#ifdef NORMALIZE
    scale = resScaling;
#else
    scale = 256;
#endif

    // Look through the scalings for the first threshold greater than
    // or equal to our velocity, and back off by one.
    if ( velocity > pointerScaling.scaleThresholds[0] )
    {
	for ( index = 1; index < pointerScaling.numScaleLevels; ++index )
	{
	    if ( velocity <= pointerScaling.scaleThresholds[index] )
		break;
	}
	--index;	// Back off to get index covering our velocity.
	scale *= pointerScaling.scaleFactors[index];
    }
    // Don't let errors accumulate (With normalization, dx and dy can be <1 most of the time)
    // The 128 causes rounding instead of truncation
    if (dx >= 0) {
	tmp = (dx * scale) + 128 + dxRemainder;
	*dxp = tmp >> 8;
	dxRemainder = (tmp & 0xff) - 128;
    } else {
	tmp = (absDx * scale)  + 128 - dxRemainder;
	*dxp = -(tmp >> 8);
	dxRemainder = -((tmp & 0xff) - 128);
    }  

    if (dy >= 0) {
	tmp = (dy * scale) + 128 + dyRemainder;
	*dyp = tmp >> 8;
	dyRemainder = (tmp & 0xff) - 128;
    } else {
	tmp = (absDy * scale) + 128 - dyRemainder;
	*dyp = -(tmp >> 8);
	dyRemainder = -((tmp & 0xff) - 128);
    }  

    return self;
}

- setPointerScaling:(unsigned)numScalings data:(unsigned const *)scaleData
// Description:	Set the pointer scaling factors and thresholds from the int 
//		array and count passed in here. There should be 2*numScaling
//		ints in the scaleData array, packed with a threshold followed
//		by a scaling factor.
// Preconditions:
// *	deviceLock must not be held on entry
{
    int i;
    if ( numScalings > NX_MAXMOUSESCALINGS )
	numScalings = NX_MAXMOUSESCALINGS;	// Clamp range

    [deviceLock lock];
    pointerScaling.numScaleLevels = numScalings;
    for ( i = 0; i < numScalings; ++i )
    {
	pointerScaling.scaleThresholds[i] = *scaleData++;
	pointerScaling.scaleFactors[i] = *scaleData++;
    }
    [deviceLock unlock];
    return self;
}

- pointerScaling:(unsigned *)numScalings data:(unsigned *)scaleData
// Description:	Return the pointer scaling factors and thresholds using the int 
//		array and count passed in here. On entry, *numScalings should
//		reflect the max number of scaling entries scaleData can hold.
{
    int i;

    [deviceLock lock];
    if ( *numScalings > pointerScaling.numScaleLevels )
	*numScalings = pointerScaling.numScaleLevels;	// Clamp range

    for ( i = 0; i < *numScalings; ++i )
    {
	*scaleData++ = pointerScaling.scaleThresholds[i];
	*scaleData++ = pointerScaling.scaleFactors[i];
    }
    [deviceLock unlock];
    return self;
}

- initPointer
// Description:	Perform setup work needed to find and configure our
//		PCPointer device. We have to tell it to send events to us.
// Preconditions:
// *	deviceLock must be held on entry
{
    if ((pointerDevice = [PCPointer activePointerDevice]) == nil) {
	IOLog("initPointer: Can't find active pointer device\n");
	return nil;
    }
    if ( [pointerDevice respondsTo:@selector(setEventTarget:)] )
	[pointerDevice setEventTarget: self];
    else {
	IOLog("initPointer: PCPointer0 does not respond to setEventTarget:\n");
	return nil;
    }
    [pointerDevice setEventTarget: self];
    return self;
}

- resetPointer
{
    [deviceLock lock];
    buttonMode = NX_OneButton;
    [deviceLock unlock];
    // setPointerScaling will try to acquire deviceLock, so release it first.
    [self setPointerScaling:
	((sizeof dfltPointerScaling/sizeof dfltPointerScaling[0])/2)
	data:dfltPointerScaling];
    return self;
}
//
// END:		Implementation of Private EventSrcPCPointer methods
//


//
// BEGIN:	Implementation of PCPointerTarget protocol
//
- (void)dispatchPointerEvent:(PCPointerEvent *)event
// Description:	This method is the heart of event dispatching. The underlying
//		PCPointer object invokes this method with each event.
//		The event structure passed in by reference should not be freed.
{
    int buttons;
    int dx;
    int dy;
    unsigned scaledDeltaT;
    ns_time_t longDelta;

    [deviceLock lock];

    buttons = 0;
    if (event->data.values.leftButton)
        buttons |= EV_LB;
    if (event->data.values.rightButton)
        buttons |= EV_RB;
	
    if (inverted) {
	dx = -event->data.values.dx;
	dy = event->data.values.dy;
    } else {
	dx = event->data.values.dx;
	dy = -event->data.values.dy;
    }

    // Perform pointer acceleration computations

    // Compute delta_t in units of (2**16 * 1ns) = 65us to keep precision
    // Clamp it to 16 bits (about 4 seconds) to avoid overflow
    longDelta = (((event->timeStamp) - lastTimestamp) >> 16);
    if (longDelta > MAX_EVENT_DELTA)
    	scaledDeltaT = MAX_EVENT_DELTA;
    else
    	scaledDeltaT = longDelta;
    
    lastTimestamp = (event->timeStamp);

#ifdef DEBUG
    deltaTimestamp[velocityLogP] = scaledDeltaT;
#endif
    [self scalePointerInX:&dx
	andY:&dy
	over:scaledDeltaT
	atRes:resolution];

    // Perform button tying and mapping.  This
    // stuff applies to relative posn devices (mice) only.
    if ( buttonMode == NX_OneButton )
    {
	if ( (buttons & (EV_LB|EV_RB)) != 0 )
	    buttons = EV_LB;
    }
    else if ( buttonMode == NX_LeftButton )	// Menus on left button. Swap!
    {
	int temp = 0;
	if ( buttons & EV_LB )
	    temp = EV_RB;
	if ( buttons & EV_RB )
	    temp |= EV_LB;
	buttons = temp;
    }
    [deviceLock unlock];

    [[self owner] relativePointerEvent:buttons
	deltaX:dx
	deltaY:dy
	atTime:event->timeStamp];
}
//
// END:		Implementation of PCPointerTarget protocol
//


//
// BEGIN:	Implementation of Exported EventSrcPCPointer methods
//
- init
// Description:	Basic initialization stuff.
// Preconditions:
// *	deviceLock must not be held on entry
{
    id rtn;
    
#ifdef DEBUG
    for (velocityLogP=0; velocityLogP < LOG_LENGTH; velocityLogP++)
    	velocityLog[velocityLogP] = 0;
    velocityLogP = 0;
    maxVelocity = 0;
#endif

    lastTimestamp = 0;
    dxRemainder = dyRemainder = 0;
    if ( deviceLock == nil )
	    deviceLock = [NXLock new];
    [deviceLock lock];
    [super init];
    pointerDevice = nil;
    buttonMode = NX_OneButton;

    rtn = [self initPointer];
    resolution = [pointerDevice getResolution];
    if (resolution == 0)
    	resolution = REFERENCE_RESOLUTION;
    resScaling = (256 * REFERENCE_RESOLUTION) / resolution;
    inverted = [pointerDevice getInverted];
    
    [deviceLock unlock];
    // setPointerScaling will try to acquire deviceLock, so release it first.
    [self setPointerScaling:
	((sizeof dfltPointerScaling/sizeof dfltPointerScaling[0])/2)
	data:dfltPointerScaling];
	
    return rtn;
}

+ probe
// Description:	This is our factory method. It is the IODevice probe
//		routine for psuedo drivers.
{
    if ( instance != nil )
	return instance;

    instance = [self alloc];

    [instance setName:"EventSrcPCPointer0"];
    [instance setDeviceKind:"EventSrcPCPointer"];
    
    if ( [instance init] == nil )
	[instance free];	// Zaps 'instance' on the way out

    return instance;
}

- free
// Description:	Go Away. Be careful when freeing the lock.
{
    id lock;

    [deviceLock lock];
    instance = nil;
    lock = deviceLock;
    deviceLock = nil;
    // Release pointer device, so we won't get any more events
    if ( pointerDevice != nil )
	[pointerDevice setEventTarget:nil];
    [lock unlock];
    [lock free];
    return [super free];
}


- (IOReturn)getIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
{
    IOReturn r = IO_R_INVALID_ARG;
    NXEventSystemDevice *dp;
    unsigned maxCount = *count;
    unsigned *returnedCount = count;
    
    if ( strcmp( parameterName, EVSIOCMS ) == 0 )	// Pointer Scaling
    {
	parameterArray[EVSIOSMS_NSCALINGS] = (maxCount - 1)/2;
	[self pointerScaling:&parameterArray[EVSIOSMS_NSCALINGS]
	    data:&parameterArray[EVSIOSMS_DATA]];
	*returnedCount = (parameterArray[EVSIOSMS_NSCALINGS] * 2) + 1;
	r = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, EVSIOCMH ) == 0 )	// Pointer Handedness
    {
	if ( maxCount >= EVSIOCMH_SIZE )
	{
	    *returnedCount = EVSIOCMH_SIZE;
	    [deviceLock lock];
	    parameterArray[0] = buttonMode;
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIOINFO ) == 0 )	// Device info
    {
	dp = (NXEventSystemDevice *) &parameterArray[0];
	*returnedCount = 0;
	// No need to lock device since we're not even reading the structure.
	dp->interface = NX_EVS_DEVICE_INTERFACE_SERIAL_ACE;
	dp->dev_type = NX_EVS_DEVICE_TYPE_MOUSE;
	dp->interface_addr = 0;
	dp->id = 0;
	*returnedCount = sizeof (NXEventSystemDevice) / sizeof (int);
	r = IO_R_SUCCESS;
    }
    else
    {
	r = [super getIntValues:parameterArray
		   forParameter: parameterName
		   count : count];
	if (r == IO_R_UNSUPPORTED)
	    r = IO_R_INVALID_ARG;
    }
    return r;
}

- (IOReturn)getCharValues:(unsigned char *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
{
    return IO_R_INVALID_ARG;
}

- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
{
    IOReturn r = IO_R_INVALID_ARG;
    int cnt;

    if ( strcmp( parameterName, EVSIOSMS ) == 0 )	// Pointer Scaling
    {
	cnt = (parameterArray[EVSIOSMS_NSCALINGS] * 2) + 1;
	if ( count <= EVSIOSMS_SIZE && cnt <= count )
	{
	    [self setPointerScaling:parameterArray[EVSIOSMS_NSCALINGS]
		data:&parameterArray[EVSIOSMS_DATA]];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIOSMH ) == 0 )	// Pointer Handedness
    {
	if ( count == EVSIOSMH_SIZE )
	{
	    [deviceLock lock];
	    buttonMode = parameterArray[0];
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIORMS ) == 0 )
    {
	r = IO_R_SUCCESS;
    }
    else
    {
	r = [super setIntValues:parameterArray
		   forParameter:parameterName
		   count : count];
	if (r == IO_R_UNSUPPORTED)
	    r = IO_R_INVALID_ARG;
    }
    return r;
}

- (void)setResolution:(unsigned) res
{
    resolution = res;
    if (resolution == 0)
    	resolution = REFERENCE_RESOLUTION;
    resScaling = (256 * REFERENCE_RESOLUTION) / resolution;

}

- (void)setInverted:(BOOL)flag
{
    inverted = flag;
}

//
// END:		Implementation of Exported EventSrcPCPointer methods
//
@end
