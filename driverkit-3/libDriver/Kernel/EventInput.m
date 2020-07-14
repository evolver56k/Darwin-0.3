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
 * EventInput.m -	Event System periodic operations module,
 *			ObjC implementation.
 *
 *			This module is the kernel version.  It will need
 *			to be re-implemented to run in user space.
 *
 *			Periodic services driven out ot this module are almost
 *			entirely cursor related.
 *
 * HISTORY
 * 31-Mar-92    Mike Paquette at NeXT 
 *      Created. 
 * 5  Aug 1993	  Erik Kay at NeXT
 *	minor API cleanup
 */
 
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/kernelDriver.h>
#import <mach/notify.h>
#if	__nrw__
#import	<bsd/sys/reboot.h>	// For disgusting hard poweroff hack...
#endif
#import <bsd/dev/ev_keymap.h>
#import <bsd/dev/evio.h>
#import <kern/queue.h>
#import <bsd/dev/machine/ev_private.h>	/* Per-machine configuration info */
#import <driverkit/EventInput.h>
#if	m88k
#import <machdep/m88k/xpr.h>
#else
#define xpr_ev_cursor(x, a, b, c, d, e)
#endif

#if	DEBUG && __nrw__
/*  
 * Avoid compiler headbutts with mon/types.h..
 *
 * Is this machine dependent?  If so, wrap it and put the 
 * cover function in machine/ev.c
 */
#define _MON_TYPES_BOOLEAN_
#define _MON_TYPES_CPUTYPES_
#define _MON_TYPES_SIZE_
#define	_MON_BIT_MACROS_
#import <mon/mon_service.h>

static void 
break_to_parent()
{
	mon_command_mode();
}
#endif /* DEBUG && __nrw__ */


#define PtInRect(ptp,rp) \
	((ptp)->x >= (rp)->minx && (ptp)->x <  (rp)->maxx && \
	(ptp)->y >= (rp)->miny && (ptp)->y <  (rp)->maxy)

@implementation EventDriver(Input)

//
// Kernel versions of nanosecond callouts.
//
static void EvPeriodicCallout( void *data )
{
	EventDriver *inst = (EventDriver *)data;
	inst->periodicRunPending = NO;
	[inst periodicEvents];
}

//
// Schedule periodicEvents to be run nst nanoseconds.
//
- runPeriodicEvent:(ns_time_t)nst
{

	if ( periodicRunPending == YES )
	    ns_untimeout((func)EvPeriodicCallout,(void *)self);
	ns_abstimeout(	(func)EvPeriodicCallout,
			(void *)self,
			nst,
			CALLOUT_PRI_THREAD );
	periodicRunPending = YES;
	return self;
}

//
// Schedule the next periodic event to be run, based on the current state of
// the event system.  We have to consider things here such as when the last
// periodic event pass ran, if there is currently any mouse delta accumulated,
// and how long it has been since the last event was consumed by an app (for
// driving the wait cursor).
//
// This code should only be run from the periodicEvents method or
// _setCursorPosition.
//
- scheduleNextPeriodicEvent
{
	ns_time_t time_for_next_run;
	ns_time_t current_time;

	IOGetTimestamp(&current_time);
	time_for_next_run = current_time + EV_TICK_TO_NS(10);

	// figure in wait cursor delay periods here
	if (waitFrameTime > current_time && waitFrameTime < time_for_next_run)
		time_for_next_run = waitFrameTime;

	if (	periodicRunPending == NO		// need to schedule
	    ||	nextPeriodicRun > time_for_next_run	// need to reschedule
	    ||	nextPeriodicRun <= thisPeriodicRun )	// clock wrapped around
	{
		nextPeriodicRun = time_for_next_run;
		[self runPeriodicEvent:time_for_next_run];
	}
	
	return self;
}

// Periodic events are driven from this method.
// After taking care of all pending work, the method
// calls scheduleNextPeriodicEvent to compute and set the
// next callout.
//
- (void)periodicEvents
{
	unsigned int	tick;
	EvGlobals	*glob;

	// If eventsOpen is false, then the driver shmem is
	// no longer valid, and it is in the process of shutting down.
	// We should give up without rescheduling.
	[driverLock lock];
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return;
	}
	glob = (EvGlobals *)evg;
	// Increment event time stamp last
	IOGetTimestamp(&thisPeriodicRun);

	// Temporary hack til we wean PS off of VertRetraceClock
	tick = EV_NS_TO_TICK(thisPeriodicRun);
	if ( tick == 0 )
		tick = 1;
	glob->VertRetraceClock = tick;

	// Update cursor position if needed
	if ( needSetCursorPosition == YES )
		[self _setCursorPosition:&pointerLoc atTime:tick];

	// WAITCURSOR ACTION
	if ( ev_try_lock(&glob->waitCursorSema) )
	{
	    if ( ev_try_lock(&glob->cursorSema) )
	    {
		// See if the current context has timed out
		if (   (glob->AALastEventSent != glob->AALastEventConsumed)
		    && ((glob->VertRetraceClock - glob->AALastEventSent >
					glob->waitThreshold)))
		    glob->ctxtTimedOut = TRUE;
		// If wait cursor enabled and context timed out, do waitcursor
		if (glob->waitCursorEnabled && glob->globalWaitCursorEnabled &&
		glob->ctxtTimedOut)
		{
		    /* WAIT CURSOR SHOULD BE ON */
		    if (!glob->waitCursorUp)
			[self showWaitCursor];
		} else
		{
		    /* WAIT CURSOR SHOULD BE OFF */
		    if (glob->waitCursorUp && waitSusTime <= thisPeriodicRun)
			[self hideWaitCursor];
		}
		/* Animate cursor */
		if (glob->waitCursorUp && waitFrameTime <= thisPeriodicRun)
			[self animateWaitCursor];
		ev_unlock(&glob->cursorSema);
		if ((glob->VertRetraceClock > autoDimTime) && (!autoDimmed))
		    [self doAutoDim];
	    }
	    ev_unlock(&glob->waitCursorSema);
	}
	[self scheduleNextPeriodicEvent];
	[driverLock unlock];

	return;
}

//
// Start the cursor system running.  Invoked via setParameterInt:EVIOST from
// the Window Server.
//
// At this point, the WindowServer is up, running, and ready to process events.
// We will attach the keyboard and mouse, if none are available yet.
//
- (void)startCursor
{
    Point p;

    if (screens)	// Should be at least 1!
    {
	[driverLock lock];
	if ( eventsOpen == NO )
	{
	    [driverLock unlock];
	    return;
	}
        p = ((EvGlobals*)evg)->cursorLoc;
	if ( (currentScreen = [self pointToScreen:&p]) < 0 )
	{
	    [driverLock unlock];
	    return;
	}
	cursorPin = ((EvScreen*)evScreen)[currentScreen].bounds;
	cursorPin.maxx--;	// Set the range the cursor is pinned
	cursorPin.maxy--;	// to for this display [closed range]
	[self setBrightness];
	[self showCursor];
	[driverLock unlock];	// We may get events in the attach process
	[self attachDefaultEventSources];
    }
}

//
// Wait Cursor machinery.  The driverLock should be held on entry to
// these methods, and the shared memory area must be set up.
//
- (void)showWaitCursor
{
	xpr_ev_cursor("showWaitCursor\n",1,2,3,4,5);
	((EvGlobals*)evg)->waitCursorUp = YES;
	[self changeCursor:EV_WAITCURSOR];
	// Set animation and sustain absolute times.
	waitFrameTime = waitFrameRate + thisPeriodicRun;
	waitSusTime = waitSustain + thisPeriodicRun;
}

- (void)hideWaitCursor
{
	xpr_ev_cursor("hideWaitCursor\n",1,2,3,4,5);
	((EvGlobals*)evg)->waitCursorUp = NO;
	[self changeCursor:EV_STD_CURSOR];
	waitFrameTime = 0ULL;
	waitSusTime = 0ULL;
}

- (void)animateWaitCursor
{
	xpr_ev_cursor("animateWaitCursor\n",1,2,3,4,5);
	[self changeCursor:((EvGlobals*)evg)->frame + 1];
	// Set the next animation time.
	waitFrameTime = waitFrameRate + thisPeriodicRun;
}

- (void)changeCursor:(int)frame
{
	((EvGlobals*)evg)->frame = (frame > EV_MAXCURSOR) ? EV_WAITCURSOR : frame;
	xpr_ev_cursor("changeCursor %d\n",((EvGlobals*)evg)->frame,2,3,4,5);
	[self moveCursor];
}

//
// Return the screen number in which point p lies.  Return -1 if the point
// lies outside of all registered screens.
//
- (int) pointToScreen:(Point *) p
{
    int i;
    EvScreen *screen = (EvScreen *)evScreen;
    for (i=screens; --i != -1; ) {
	if (screen[i].instance != nil
	&& (p->x >= screen[i].bounds.minx)
	&& (p->x < screen[i].bounds.maxx)
	&& (p->y >= screen[i].bounds.miny)
	&& (p->y < screen[i].bounds.maxy))
	    return i;
    }
    return(-1);	/* Cursor outside of known screen boundary */
}

//
// API used to manipulate screen brightness
//
// On entry to each of these, the driverLock should be set.
//
// Set the current brightness
- setBrightness:(int)b
{
	if ( b < EV_SCREEN_MIN_BRIGHTNESS )
		b = EV_SCREEN_MIN_BRIGHTNESS;
	else if ( b > EV_SCREEN_MAX_BRIGHTNESS )
		b = EV_SCREEN_MAX_BRIGHTNESS;
	if ( b != curBright )
	{
	    curBright = b;
	    if ( autoDimmed == NO )
		return [self setBrightness];
	}
	return self;
}

- (int)brightness
{
	return curBright;
}

// Set the current brightness
- setAutoDimBrightness:(int)b
{
	if ( b < EV_SCREEN_MIN_BRIGHTNESS )
		b = EV_SCREEN_MIN_BRIGHTNESS;
	else if ( b > EV_SCREEN_MAX_BRIGHTNESS )
		b = EV_SCREEN_MAX_BRIGHTNESS;
	if ( b != dimmedBrightness )
	{
	    dimmedBrightness = b;
	    if ( autoDimmed == YES )
		return [self setBrightness];
	}
	return self;
}

- (int)autoDimBrightness
{
	return dimmedBrightness;
}


- (int)currentBrightness;		// Return the current brightness
{
	if ( autoDimmed == YES && dimmedBrightness < curBright )
		return dimmedBrightness;
	else
		return curBright;
}

- doAutoDim
{
	autoDimmed = YES;
	[self setBrightness];
	return self;
}

// Return display brightness to normal
- undoAutoDim
{
	autoDimmed = NO;
	[self setBrightness];
	return self;
}

- forceAutoDimState:(BOOL)dim
{
    	if ( dim == YES )
	{
	    if ( autoDimmed == NO )
	    {
		if ( eventsOpen == YES )
		    autoDimTime = ((EvGlobals*)evg)->VertRetraceClock;
		[self doAutoDim];
	    }
	}
	else
	{
	    if ( autoDimmed == YES )
	    {
		if ( eventsOpen == YES )
		    autoDimTime = ((EvGlobals*)evg)->VertRetraceClock
		    			+ autoDimPeriod;
		[self undoAutoDim];
	    }
	}
	return self;
}

//
// API used to manipulate sound volume/attenuation
//
// Set the current brightness.
- setAudioVolume:(int)v
{
	if ( v < EV_AUDIO_MIN_VOLUME )
		v = EV_AUDIO_MIN_VOLUME;
	else if ( v > EV_AUDIO_MAX_VOLUME )
		v = EV_AUDIO_MAX_VOLUME;
	curVolume = v;
	return self;
}

//
// Volume set programatically, rather than from keyboard
//
- setUserAudioVolume:(int)v
{
	[self setAudioVolume:v];
	// Let sound driver know about the change
	[self evSpecialKeyMsg:	NX_KEYTYPE_SOUND_UP
				direction:NX_KEYDOWN
				flags:0
				level:curVolume];
	
}

- (int)audioVolume
{
	return curVolume;
}

//
// API used to drive event state out to attached screens
//
// On entry to each of these, the driverLock should be set.
//
- setBrightness			// Propagate state out to screens
{
	int i;
	for ( i = 0; i < screens; ++i )
		[self evDispatch:i command:EVLEVEL];

	return self;						
}
- showCursor
{
	return [self evDispatch:currentScreen command:EVSHOW];
}
- hideCursor
{
	return [self evDispatch:currentScreen command:EVHIDE];
}
- moveCursor
{
	return [self evDispatch:currentScreen command:EVMOVE];
}

//
// - attachDefaultEventSources
//	Attach the default event sources.
//	The default sources are machine dependent.
//
- attachDefaultEventSources
{
	id driver;
	id class;
	IOReturn r;
	const char **devClass;

	for ( devClass = defaultEventSources(); *devClass != NULL; ++devClass )
		[self attachEventSource: *devClass];

	return self;
}

- attachEventSource:(const char *)classname
{
	id driver;
	id class;
	if ( (class = objc_getClass(classname)) == nil )
	{
		IOLog( "%s: %s: no such class.\n",
			[self name], classname);
		return nil;
	}
	if ( [class respondsTo:@selector(probe)] == NO )
	{
		IOLog( "%s: %s does not respond to probe.\n",
			[self name], classname);
		return nil;
	}
	if ( (driver = [class probe]) == nil )
	{
		IOLog("%s: probe of %s failed\n",
			[self name], classname  );
		return nil;
	}
	if ( [self registerEventSource: driver] == nil )
	{
		IOLog("%s: becomeOwner of %s failed\n",
			[self name], classname  );
		return nil;
	}
	return driver;
}

//
// - detachEventSources
//	Detach all event sources
//
- detachEventSources
{
	attachedEventSrc *device;

	[eventSrcListLock lock];
	while(!queue_empty(&eventSrcList)) {
		device = (attachedEventSrc *)queue_first(&eventSrcList);
		queue_remove(&eventSrcList,
			device,
			attachedEventSrc *,
			link);
		// Release lock while we tear down dequeued item.
		[eventSrcListLock unlock];

		if ( device->info.eventSrc != nil )
			[device->info.eventSrc relinquishOwnership:self];
		IOFree(device, sizeof (attachedEventSrc));
		[eventSrcListLock lock];
	}
	[eventSrcListLock unlock];
	return self;
}

//
// EventSrcClient implementation
//

//
// A new device instance desires to be added to our list.
// Try to get ownership of the device. If we get it, add it to
// the list.
// 
- registerEventSource:source
{
	attachedEventSrc *device;

	if ( [source becomeOwner:self] != IO_R_SUCCESS )
		return nil;
	device = IOMalloc( sizeof (attachedEventSrc) );
	bzero( device, sizeof (attachedEventSrc) );
	device->info.eventSrc = source;
	// Enter the device on our list.
	[eventSrcListLock lock];
	queue_enter(&eventSrcList,
		device,
		attachedEventSrc *,
		link);
	[eventSrcListLock unlock];
	return self;
}

//
// Called when another client wishes to assume ownership of calling adbDevice.
// This happens when a client attempts a becomeOwner: on a device which is 
// owned by the callee of this method. 
//
// Returns:
//    IO_R_SUCCESS ==> It's OK to give ownership of the device to other client.
//		       In this case, callee no longer owns the device.
//    IO_R_BUSY    ==> Don't transfer ownership.
//
// Note that this call occurs during (i.e., before the return of) a 
// attachDevice: call to the caller of this method.
//
- (IOReturn)relinquishOwnershipRequest	: device;
{
	return (eventsOpen == NO) ? IO_R_SUCCESS : IO_R_BUSY;
}

//
// Method by which a client, who has registered "intent to own" via 
// desireOwnership:client, is notified that the calling device is available.
// Client will typically call becomeOwner: during this call.
//
- (void)canBecomeOwner			: device;
{
	IOReturn drtn;
	drtn = [device becomeOwner: self];
	if(drtn)
	{
		IOLog("%s: becomeOwner of %s failed (%s)\n",
			[self name],
			[device name],
			[self stringFromReturn:drtn]);
	}
}

//
// Process a mouse status change.  The driver should sign extend
// it's deltas and perform any bit flipping needed there.
//
// We take the state as presented and turn it into events.
// 
- relativePointerEvent:(int)buttons deltaX:(int)dx deltaY:(int)dy
{
	ns_time_t ts;
	IOGetTimestamp( &ts );
	return [self relativePointerEvent:buttons
					deltaX:dx
					deltaY:dy
					atTime:ts];
}

- relativePointerEvent:(int)buttons
		deltaX:(int)dx
		deltaY:(int)dy
		atTime:(ns_time_t)ts
{
	unsigned	tick;

	tick = EV_NS_TO_TICK(ts);
	[driverLock lock];
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return self;
	}
	// Fake up pressure changes from button state changes
	if ( (buttons & EV_LB) != (((EvGlobals*)evg)->buttons & EV_LB) )
	{
	    if ( buttons & EV_LB )
		lastPressure = MAXPRESSURE;
	    else
		lastPressure = MINPRESSURE;
	}
	[self _setButtonState:buttons atTime:tick];

	// figure cursor movement
	if ( dx || dy )
	{
	    pointerLoc.x += dx;
	    pointerLoc.y += dy;
	    if ( needSetCursorPosition == NO )
		[self _setCursorPosition:&pointerLoc atTime:tick];
	}
	[driverLock unlock];
	return self;
}

- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity
{
	unsigned int pressure;
	ns_time_t ts;
	IOGetTimestamp( &ts );
	// Fake up pressure changes from button state changes
	[driverLock lock];
	pressure = lastPressure;
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return self;
	}
	if ( (buttons & EV_LB) != (((EvGlobals*)evg)->buttons & EV_LB) )
	{
	    if ( buttons & EV_LB )
		pressure = MAXPRESSURE;
	    else
		pressure = MINPRESSURE;
	}
	[driverLock unlock];
	[self absolutePointerEvent:buttons
				at:newLoc
				inProximity:proximity
				withPressure:pressure
				withAngle:90
				atTime:ts];
	return self;
}

- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity
		withPressure:(int)pressure
{
	ns_time_t ts;
	IOGetTimestamp( &ts );
	[self absolutePointerEvent:buttons
				at:newLoc
				inProximity:proximity
				withPressure:pressure
				withAngle:90
				atTime:ts];
	return self;
}

- absolutePointerEvent:(int)buttons
		at:(Point *)newLoc
		inProximity:(BOOL)proximity
		withPressure:(int)pressure
		withAngle:(int)stylusAngle
		atTime:(ns_time_t)ts
		
{
	NXEventData outData;	/* dummy data */
	unsigned	tick;

	tick = EV_NS_TO_TICK(ts);
	[driverLock lock];
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return self;
	}
	
	lastPressure = pressure; 
	if ( newLoc->x != pointerLoc.x || newLoc->y != pointerLoc.y )
	{
	    pointerLoc = *newLoc;
	    if ( needSetCursorPosition == NO )
		    [self _setCursorPosition:&pointerLoc atTime:tick];
	}
	if ( lastProximity != proximity && proximity == YES )
	{
	    ((EvGlobals*)evg)->eventFlags |= NX_STYLUSPROXIMITYMASK;
	    bzero( (char *)&outData, sizeof outData );
	    [self postEvent:NX_FLAGSCHANGED
			    at:(Point *)&pointerLoc
			    atTime:tick
			    withData:&outData];
	}
	if ( proximity == YES )
	    [self _setButtonState:buttons atTime:tick];
	if ( lastProximity != proximity && proximity == NO )
	{
	    ((EvGlobals*)evg)->eventFlags &= ~NX_STYLUSPROXIMITYMASK;
	    bzero( (char *)&outData, sizeof outData );
	    [self postEvent:NX_FLAGSCHANGED
			    at:(Point *)&pointerLoc
			    atTime:tick
			    withData:&outData];
	}
	lastProximity = proximity;
	[driverLock unlock];
	return self;
}

//
// Process a keyboard state change.
// 
- keyboardEvent:(unsigned)eventType
		flags:(unsigned)flags
		keyCode:(unsigned)key
		charCode:(unsigned)charCode
		charSet:(unsigned)charSet
		originalCharCode:(unsigned)origCharCode
		originalCharSet:(unsigned)origCharSet
		repeat:(BOOL)repeat
		atTime:(ns_time_t)ts
{
	NXEventData	outData;
	unsigned	tick;

	tick = EV_NS_TO_TICK(ts);
	outData.key.repeat = repeat;
	outData.key.keyCode = key;
	outData.key.charSet = charSet;
	outData.key.charCode = charCode;
	outData.key.origCharSet = origCharSet;
	outData.key.origCharCode = origCharCode;

	[driverLock lock];
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return self;
	}
	((EvGlobals*)evg)->eventFlags = (((EvGlobals*)evg)->eventFlags & ~KEYBOARD_FLAGSMASK)
			| (flags & KEYBOARD_FLAGSMASK);

	[self postEvent:eventType
			at:(Point *)&pointerLoc
			atTime:tick
			withData:&outData];

	[driverLock unlock];
	return self;
}


- keyboardSpecialEvent:(unsigned)eventType
		flags:(unsigned)flags
		keyCode:(unsigned)key
		specialty:(unsigned)flavor
		atTime:(ns_time_t)ts
{
	NXEventData	outData;
	unsigned	tick;
	int		level = -1;

	bzero( (void *)&outData, sizeof outData );
	tick = EV_NS_TO_TICK(ts);

	[driverLock lock];
	if ( eventsOpen == NO )
	{
		[driverLock unlock];
		return self;
	}
	// Update flags.
	((EvGlobals*)evg)->eventFlags = (((EvGlobals*)evg)->eventFlags & ~KEYBOARD_FLAGSMASK)
			| (flags & KEYBOARD_FLAGSMASK);
	// Most of these keys don't generate events.  Forcibly undo autodim.
	if ( autoDimmed == YES )
		[self forceAutoDimState:NO];
	if ( eventType == NX_KEYDOWN )
	{
		switch ( flavor )
		{
			case NX_KEYTYPE_SOUND_UP:
			    if ( (flags & SPECIALKEYS_MODIFIER_MASK) == 0 )
				[self setAudioVolume:[self audioVolume] + 1];
			    level = [self audioVolume];
			    break;
			case NX_KEYTYPE_SOUND_DOWN:
			    if ( (flags & SPECIALKEYS_MODIFIER_MASK) == 0 )
				[self setAudioVolume:[self audioVolume] - 1];
			    level = [self audioVolume];
			    break;
			case NX_KEYTYPE_BRIGHTNESS_UP:
#if 	DEBUG && __nrw__
			    if ( (flags & BRIGHT_BREAK_TO_DEBUGGER_MASK)
				== BRIGHT_BREAK_TO_DEBUGGER_MASK )
			    {
				    [driverLock unlock];
				    break_to_parent();
				    return self;
			    }
#endif	DEBUG && __nrw__
			    if ( (flags & SPECIALKEYS_MODIFIER_MASK) == 0 )
				[self setBrightness:[self brightness] + 1];
			    level = [self brightness];
			    break;
			case NX_KEYTYPE_BRIGHTNESS_DOWN:
			    if ( (flags & SPECIALKEYS_MODIFIER_MASK) == 0 )
				[self setBrightness:[self brightness] - 1];
			    level = [self brightness];
			    break;
			case NX_POWER_KEY:
#if 	DEBUG && __nrw__
				if ( (flags & PWR_BREAK_TO_DEBUGGER_MASK)
				    == PWR_BREAK_TO_DEBUGGER_MASK )
				{
				    [driverLock unlock];
				    break_to_parent();
				    return self;
				}
#endif	DEBUG && __nrw__
#if	__nrw__
				// ICK! Do we really need this "feature"?
				if ( (flags & HARD_POWEROFF_MASK)
				    == HARD_POWEROFF_MASK )
				{
				        [driverLock unlock];
					boot(0,RB_NOSYNC|RB_POWERDOWN,NULL);
					return self; // not reached...
				}
#endif
				outData.compound.subType = 1;
				[self postEvent:NX_SYSDEFINED
						at:(Point *)&pointerLoc
						atTime:tick
						withData:&outData];
				break;
		}
	}
#if 0	/* So far, nothing to do on keyup */
	else if ( eventType == NX_KEYUP )
	{
		switch ( flavor )
		{
			case NX_KEYTYPE_SOUND_UP:
				break;
			case NX_KEYTYPE_SOUND_DOWN:
				break;
			case NX_KEYTYPE_BRIGHTNESS_UP:
				break;
			case NX_KEYTYPE_BRIGHTNESS_DOWN:
				break;
			case NX_POWER_KEY:
				break;
		}
	}
#endif
	[driverLock unlock];
	if ( level != -1 )	// An interesting special key event occurred
	{
		[self evSpecialKeyMsg:	flavor
					direction:eventType
					flags:flags
					level:level];
	}
	return self;
}

/*
 * Update current event flags.  Restricted to keyboard flags only, this
 * method is used to silently update the flags state for keys which both
 * generate characters and flag changes.  The specs say we don't generate
 * a flags-changed event for such keys.  This method is also used to clear
 * the keyboard flags on a keyboard subsystem reset.
 */
- updateEventFlags:(unsigned)flags
{
	[driverLock lock];
	if ( eventsOpen )
	    ((EvGlobals*)evg)->eventFlags = (((EvGlobals*)evg)->eventFlags & ~KEYBOARD_FLAGSMASK)
			    | (flags & KEYBOARD_FLAGSMASK);
	[driverLock unlock];
	return self;
}

/*
 * Return current event flags
 */
- (int)eventFlags
{
	int flags = 0;
	[driverLock lock];
	if ( eventsOpen )
		flags = ((EvGlobals*)evg)->eventFlags;
	[driverLock unlock];
	return flags;
}

//
// - _setButtonState:(int)buttons  atTime:(int)t
//	Update the button state.  Generate button events as needed
//
- _setButtonState:(int)buttons atTime:(unsigned)t
{
	EvGlobals *glob = (EvGlobals *)evg;
	if ((glob->buttons & EV_LB) != (buttons & EV_LB))
	{
	    if (buttons & EV_LB)
	    {
		[self postEvent:NX_LMOUSEDOWN
				at:(Point *)&glob->cursorLoc
				atTime:t
				withData:NULL];
		glob->buttons |= EV_LB;
	    }
	    else
	    {
		[self postEvent:NX_LMOUSEUP
				at:(Point *)&glob->cursorLoc
				atTime:t
				withData:NULL];
		glob->buttons &= ~EV_LB;
	    }
	    // After entering initial up/down event, set up
	    // coalescing state so drags will behave correctly
	    glob->dontCoalesce = glob->dontWantCoalesce;
	    if (glob->dontCoalesce)
		glob->eventFlags |= NX_NONCOALSESCEDMASK;
	    else
		glob->eventFlags &= ~NX_NONCOALSESCEDMASK;
	}
    
	if ((glob->buttons & EV_RB) != (buttons & EV_RB)) {
	    if (buttons & EV_RB) {
		[self postEvent:NX_RMOUSEDOWN
				at:(Point *)&glob->cursorLoc
				atTime:t
				withData:NULL];
		glob->buttons |= EV_RB;
	    } else {
		[self postEvent:NX_RMOUSEUP
				at:(Point *)&glob->cursorLoc
				atTime:t
				withData:NULL];
		glob->buttons &= ~EV_RB;
	    }
	}
	return self;
}
//
//  Sets the cursor position (((EvGlobals*)evg)->cursorLoc) to the new
//  location.  The location is clipped against the cursor pin rectangle,
//  mouse moved/dragged events are generated using the given event mask,
//  and a mouse-exited event may be generated. The cursor image is
//  moved.
//  On entry, the driverLock should be set.
//
- setCursorPosition:(Point *)newLoc
{
	if ( eventsOpen == YES )
	{
	    pointerLoc = *newLoc;
	    if ( needSetCursorPosition == NO )
		[self _setCursorPosition:newLoc atTime:EvTickTimeValue()];
	}
}

//
// This mechanism is used to update the cursor position, possibly generating
// messages to registered frame buffer devices and posting drag, tracking, and
// mouse motion events.
//
// On entry, the driverLock should be set.
// This can be called from setCursorPosition:(Point *)newLoc to set the
// position by a _IOSetParameterFromIntArray() call, directly from the absolute or
// relative pointer device routines, or on a timed event callback.
//
- _setCursorPosition:(Point *)newLoc atTime:(unsigned)t
{
	int newScreen = -1;
	EvGlobals *glob = (EvGlobals *)evg;
    
	if (!screens)
	    return self;

	if ( ev_try_lock(&glob->cursorSema) == 0 ) // host using shmem
	{
		needSetCursorPosition = YES;	  // try again later
		[self scheduleNextPeriodicEvent];
		return self;
	}
	// Past here we hold the cursorSema lock.  Make sure the lock is
	// cleared before returning or the system will be wedged.
	
	needSetCursorPosition = NO;	  // We WILL succeed

	/* Check to see if cursor ventured outside of current screen bounds
	    before worrying about which screen it may have gone to. */
    
	if (!PtInRect(newLoc, &((EvScreen*)evScreen)[currentScreen].bounds)) {
	    /* At this point cursor has gone off screen.  Check to see if moved
		to another screen.  If not, just clip it to current screen. */
    
	    if ((newScreen = [self pointToScreen:newLoc]) < 0) {
		/* Pin new cursor position to cursorPin rect */
		newLoc->x = (newLoc->x < cursorPin.minx) ?
		    cursorPin.minx : ((newLoc->x > cursorPin.maxx) ?
		    cursorPin.maxx : newLoc->x);
		newLoc->y = (newLoc->y < cursorPin.miny) ?
		    cursorPin.miny : ((newLoc->y > cursorPin.maxy) ?
		    cursorPin.maxy : newLoc->y);
	    }
	}

	pointerLoc = *newLoc;	// Sync up pointer with clipped cursor
	/* Catch the no-move case */
	if (glob->cursorLoc.x == newLoc->x && glob->cursorLoc.y == newLoc->y)
	{
	    ev_unlock(&glob->cursorSema);
	    return self;
	}    
	glob->cursorLoc = *newLoc;
	/* If newScreen is zero or positive, then cursor crossed screens */
	if (newScreen >= 0) {
	    /* cursor changed screens */
	    [self hideCursor];	/* hide cursor on old screen */
	    currentScreen = newScreen;
	    cursorPin = ((EvScreen*)evScreen)[currentScreen].bounds;
	    cursorPin.maxx--;	/* Make half-open rectangle */
	    cursorPin.maxy--;
	    [self showCursor];
	} else {
	    /* cursor moved on same screen */
	    [self moveCursor];
	}
	
	/* See if anybody wants the mouse moved or dragged events */
	if (glob->movedMask) {
	    if ((glob->movedMask&NX_LMOUSEDRAGGEDMASK)&&(glob->buttons& EV_LB))
		[self postEvent:NX_LMOUSEDRAGGED
				at:newLoc
				atTime:t
				withData:NULL];
	    else
		if ((glob->movedMask&NX_RMOUSEDRAGGEDMASK) &&
		(glob->buttons & EV_RB))
		    [self postEvent:NX_RMOUSEDRAGGED
				    at:newLoc
				    atTime:t
				    withData:NULL];
		else
		    if (glob->movedMask & NX_MOUSEMOVEDMASK)
			[self postEvent:NX_MOUSEMOVED
					at:newLoc
					atTime:t
					withData:NULL];
	}
    
	/* check new cursor position for leaving glob->mouseRect */
	if (glob->mouseRectValid && (!PtInRect(newLoc, &glob->mouseRect)))
	{
	    if (glob->mouseRectValid)
	    {
		[self postEvent:NX_MOUSEEXITED
				at:newLoc
				atTime:t
				withData:NULL];
		glob->mouseRectValid = 0;
	    }
	}
	ev_unlock(&glob->cursorSema);
	return self;
}

@end

