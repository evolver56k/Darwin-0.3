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
 * EventSrcPCPointer.m - PC Pointer EventSrc subclass implementation
 *
 * HISTORY
 * 28 Aug 1992    Joe Pasqua
 *      Created. 
 * 11 April 1997  Simon Douglas
 *      ADB version for PPC.
 */

// TO DO:
// * VBL sync'ing
// * Real Mac scaling & acceleration & move to device
// * Better FixDiv/Mul
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
#import <bsd/dev/ppc/EventSrcPCPointer.h> 

#define MACSCALE	1

#if MACSCALE
#define MAXMAG 128
static int scaleValues[MAXMAG];
static int fractX,fractY;
#endif

#define	PC_PTR_DFLT_DELTA_T	2

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

@implementation EventSrcPCPointer: IOEventSource

int
FixMul( int a, int b)
{
    union {
	    long long 	result;
	    int		word[2];
    } big;

    big.result = ((long long) a) * ((long long) b) * 65536;
    return( big.word[0] );
}

int
FixDiv( int a, int b)
{
    return( (((long long) a) * 65536) / ((long long) b));
}


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
    int dx, dy;
    int absDx, absDy;
    unsigned delta, velocity;
    int index;

    dx = *dxp;
    dy = *dyp;
    absDx = (dx < 0) ? -dx : dx;
    absDy = (dy < 0) ? -dy : dy;

#if MACSCALE

#if 1
    if( absDx > absDy)
	delta = (absDx + (absDy / 2));
    else
	delta = (absDy + (absDx / 2));
#else
    delta = absDx + absDy;
#endif

    // scale
    if( delta > (MAXMAG - 1))
	delta = MAXMAG - 1;
    dx = FixMul( dx << 16, scaleValues[delta]);
    dy = FixMul( dy << 16, scaleValues[delta]);

    // if no direction changes add fract parts
    if( (dx ^ fractX) >= 0)
	dx += fractX;
    if( (dy ^ fractY) >= 0)
	dy += fractY;

    *dxp = dx / 65536;
    *dyp = dy / 65536;

    // get fractional part with sign extend
    if( dx >= 0)
	fractX = dx & 0xffff;
    else
	fractX = dx | 0xffff0000;
    if( dy >= 0)
	fractY = dy & 0xffff;
    else
	fractY = dy | 0xffff0000;

#else

    // Take the sum of the X and Y distances as the delta to compute
    // the velocity. (Manhatten distance)
    delta = absDx + absDy;
    if ( dt == 0 )
	    dt = PC_PTR_DFLT_DELTA_T;
    //
    // Velocity in IPS = (delta in Dots * ticks/sec)/(res in DPI * dT)
    //
    // This is a bit heavyweight, and could probably be approximated
    // with considerable savings.
    //
    velocity =  (delta * EV_TICKS_PER_SEC) / (res * dt);

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
	*dxp = dx * pointerScaling.scaleFactors[index];
	*dyp = dy * pointerScaling.scaleFactors[index];
    }

#endif

    return self;
}

IOReturn ReadNVRAM( unsigned int offset, unsigned int length, unsigned char * buffer );

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

#if MACSCALE
// a pale imitation of CrsrDev.a
    {
	static int deviceSpeed[] = { 0x000000, 0x00713b, 0x010000, 0x044ec5, 0x0c0000, 0x16ec4f, 0x1d3b14, 0x227627, 0x7ffffff };
	static int cursorSpeed[] = { 0x000000, 0x006000, 0x010000, 0x108000, 0x5f0000, 0x8b0000, 0x948000, 0x960000, 0x0960000 };

	int	slope,j = 0;
	int	x;
	int     devSpeed, crsrSpeed;
	int     lastCrsrSpeed, nextCrsrSpeed;
	int     lastDeviceSpeed, nextDeviceSpeed;
	int	accl = 0x008000;

#if 0
	static int pram2Fixed[] =  { 0x000000, 0x002000, 0x005000, 0x008000, 0x00b000, 0x00e000, 0x010000, 0x010000 };
	unsigned char abyte;
	if( 0 == ReadNVRAM( 0x1300 + 8, 1, &abyte )) {
	    abyte = 7 & (abyte >> 3);
	    accl = pram2Fixed[ abyte ];
	}
#else
	int nxMax = (*(scaleData + (2 * numScalings) - 1)) - 1;
	if( nxMax > 18)
	    nxMax = 18;
	accl = FixDiv( nxMax << 16, 18 << 16 );
#endif
kprintf( "Mouse acceleration = 0x%x\n", accl);

	// scale for device speed
	devSpeed = FixDiv( 90 << 16, resolution << 16 );			// no vbl sync, so 90 autopolls /s
	// scale for cursor speed
	crsrSpeed = FixDiv( 72 << 16, resolution << 16 );			// screen is 72 dpi

	nextCrsrSpeed = 0;
	nextDeviceSpeed = 0;

	// Precalculate fixed point scales. Not as accurate as MacOS, but no FixDiv() in handler
	for ( i = 1; i < MAXMAG; i++ )
	{
	    x = FixMul( i << 16, devSpeed );
	    if( x > deviceSpeed[j]) {
		lastCrsrSpeed = nextCrsrSpeed;
		lastDeviceSpeed = nextDeviceSpeed;
		j++;
		nextDeviceSpeed = deviceSpeed[j];
		// Interpolate by accl between y=x and y=acclTable(x) to get nextCrsrSpeed
		{
		    int factorCursor;
		    int factorDevice;
		    if( cursorSpeed[j] < nextDeviceSpeed) {
			factorDevice = accl;
			factorCursor = (0x10000 - accl);
		    } else {
			factorCursor = accl;
			factorDevice = (0x10000 - accl);
		    }
		    nextCrsrSpeed = FixMul( factorCursor, cursorSpeed[j] ) + FixMul( factorDevice, nextDeviceSpeed);
		}
		slope = FixDiv( nextCrsrSpeed - lastCrsrSpeed, nextDeviceSpeed - lastDeviceSpeed );
	    }
    	    scaleValues[i] = FixDiv( FixMul( crsrSpeed, FixMul( slope, x - lastDeviceSpeed ) + lastCrsrSpeed), x);
	}
	scaleValues[0] = scaleValues[1];
	fractX = fractY = 0;
    }
#endif

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
- (void)dispatchPointerEvent:(PointerEvent *)event
// Description:	This method is the heart of event dispatching. The underlying
//		PCPointer object invokes this method with each event.
//		The event structure passed in by reference should not be freed.
{
    int buttons;
    int dx;
    int dy;
    unsigned tick;
    unsigned delta_t;
    int menuButton;

    [deviceLock lock];

    menuButton = (event->buttonCount > 1);
    buttons = 0;

    if (event->b0 == 0)
        buttons |= EV_LB;

    if( menuButton) {
	if ((event->b1 || event->b2 || event->b3) == 0)		// either down
	    buttons |= EV_RB;
    }

    dx = event->dx;
    dy = event->dy;

    // Convert the nanosecond time into a tick time since boot.
    tick = EV_NS_TO_TICK(event->timeStamp);

    // Perform pointer acceleration computations
    if ( lastPointerEvent == 0 )
	delta_t = PC_PTR_DFLT_DELTA_T;
    else
	delta_t = tick - lastPointerEvent;

    [self scalePointerInX:&dx
	andY:&dy
	over:delta_t
	atRes:resolution];
    lastPointerEvent = tick;

    // Perform button tying and mapping.  This
    // stuff applies to relative posn devices (mice) only.
    if ( buttonMode == NX_OneButton )
    {
	if ( (buttons & (EV_LB|EV_RB)) != 0 )
	    buttons = EV_LB;
    }
    else if ( menuButton && (buttonMode == NX_LeftButton) )	// Menus on left button. Swap!
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

    if ( deviceLock == nil )
	    deviceLock = [NXLock new];
    [deviceLock lock];
    [super init];
    pointerDevice = nil;
    buttonMode = NX_OneButton;

    rtn = [self initPointer];
    resolution = [pointerDevice getResolution];

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
//
// END:		Implementation of Exported EventSrcPCPointer methods
//
@end
