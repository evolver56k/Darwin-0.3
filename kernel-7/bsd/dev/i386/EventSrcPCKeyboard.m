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
 * EventSrcPCKeyoard.m - PC Keyboard EventSrc subclass implementation
 *
 * HISTORY
 * 10 Sep 1992    Joe Pasqua
 *      Created. 
 * 5  Aug 1993	  Erik Kay at NeXT
 *	minor changes for Event driver api changes
 */

// TO DO:
//
// NOTES:
// * We override methods in the EventSrcExported protocol so that we can
//   intercept messages used to control who owns us. This gives us
//   an opportunity to acquire/release the PCKeyboard device we want to own.
// * In the current system the EventDriver instance is always the owner
//   of this EventSrc. In the future the km driver and the EventDriver might
//   both be potential owners.
// * The device dependent portion of the event flags are set by this module
//   before the flags are passed up to our owner. There are a couple of notes
//   on this. First, the PC keyboard has no key labelled "Command", therefore
//   the NX_NEXT[RL]CMDKEYMASK flag bits will never be set. Conversely, the
//   PC keyboard has both a left and a right Control key. When either key is
//   pressed we set the NX_NEXTCTLKEYMASK mask. The code processing events
//   must determine which one it was by looking at the code that is generated.
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.
//

#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/KeyMap.h>
#import <bsd/dev/evsio.h>
#import <bsd/dev/i386/event.h>
#import <bsd/dev/i386/PCKeymap.c>	// The default keymapping string
#import <bsd/dev/i386/EventSrcPCKeyboard.h> 

// Private variables that are global to the module:
static EventSrcPCKeyboard *instance = (EventSrcPCKeyboard *)0;

// Forward declarations
static void autoRepeatCallout( void *data );

@implementation EventSrcPCKeyboard

//
// BEGIN:	Utility Procedures
//
static void nsecs_to_packed_ns(ns_time_t *nsecs, unsigned int *pnsecs)
{
    _NX_packed_time_t data;
    int i;

    data.tval = *nsecs;	// nsecs to ns_time_t
    for ( i = 0; i < EVS_PACKED_TIME_SIZE; ++i )
	pnsecs[i] = data.itval[i];
}

static void packed_nsecs_to_nsecs(unsigned int *pnsecs, ns_time_t *nsecs)
{
    _NX_packed_time_t data;
    int i;

    for ( i = 0; i < EVS_PACKED_TIME_SIZE; ++i )
	data.itval[i] = pnsecs[i];
    *nsecs = data.tval;
}
//
// END:	Utility Procedures
//


//
// BEGIN:	Implementation of Private EventSrcPCKeyoard methods
//
- resetKeyboard
// Description:	Reset the keymapping to the default value and reconfigure
//		the keyboards.
{
    [deviceLock lock];
    if ( keyMap != nil )
	[keyMap free];
    // Set up default keymapping.
    keyMap = [[KeyMap alloc] initFromKeyMapping:PCDefaultKeymap
	length:sizeof PCDefaultKeymap
	canFree:NO];
    [keyMap setDelegate:self];
    keyRepeat = EV_DEFAULTKEYREPEAT;
    initialKeyRepeat = EV_DEFAULTINITIALREPEAT;
    [deviceLock unlock];
    return self;
}

- scheduleAutoRepeat
// Description:	Schedule a procedure to be called when a timeout has expired
//		so that we can generate a repeated key.
// Preconditions:
// *	deviceLock should be held on entry
{
    if ( calloutPending == YES )
    {
	ns_untimeout( (func)autoRepeatCallout, (void *)self );
	calloutPending = NO;
    }
    if ( downRepeatTime > 0ULL )
    {
	ns_abstimeout(
	    (func)autoRepeatCallout,
	    (void *)self,
	    downRepeatTime,
	    CALLOUT_PRI_THREAD );
	calloutPending = YES;
    }
    return self;
}

- autoRepeat
// Description:	Repeat the currently pressed key and schedule ourselves
//		to be called again after another interval elapses. 
// Preconditions:
// *	Should only be executed on callout thread
// *	deviceLock should be unlocked on entry.
{
    ns_time_t ns;
    
    IOGetTimestamp(&ns);
    [deviceLock lock];
    if ( calloutPending == NO )
    {
	[deviceLock unlock];
	return self;
    }
    calloutPending = NO;
    isRepeat = YES;
    
    if ( downRepeatTime > 0ULL && downRepeatTime <= ns )
    {
	// Device is due to generate a repeat
	lastEventTime = ns;
	[keyMap doKeyboardEvent:codeToRepeat
	    direction:YES
	    keyBits:keyState];
	downRepeatTime += keyRepeat;
    }

    isRepeat = NO;
    [self scheduleAutoRepeat];
    [deviceLock unlock];
    return self;
}

static void autoRepeatCallout( void *data )
// Description:	Callout used to enter Obj-C autoRepeat code from NS timer
//		callout. This proc was set as a callback in scheduleAutoRepeat.
{
    EventSrcPCKeyboard *kbd = (EventSrcPCKeyboard *)data;
    [kbd autoRepeat];
}


- setRepeat:(unsigned)eventType forCode:(unsigned)keyCode
// Description:	Set up or tear dow key repeat operations. The method
//		that locks deviceLock is a bit higher on the call stack.
//		This method is invoked as a side effect of our own
//		invocation of [keyMap doKeyboardEvent].
// Preconditions:
// *	deviceLock should be held upon entry.
{
    if ( isRepeat == NO )  // make sure we're not already repeating
    {
	if (eventType == NX_KEYDOWN)	// Start repeat
	{
	    // Set this key to repeat (push out last key if present)
	    downRepeatTime = initialKeyRepeat + lastEventTime; 
	    codeToRepeat = keyCode;
	    // reschedule key repeat event here
	    [self scheduleAutoRepeat];
	}
	else if (eventType == NX_KEYUP)	// End repeat
	{
	    /* Remove from downKey */
	    if (codeToRepeat == keyCode)
	    {
		downRepeatTime = 0ULL;
		codeToRepeat = -1;
		[self scheduleAutoRepeat];
	    }
	}
    }
    return self;
}

- initKeyboard
// Description:	Perform setup work needed to find and configure our
//		PCKeyboard device. We have to tell it to send events to us.
{
    IOReturn drtn;
    
    drtn = IOGetObjectForDeviceName("PCKeyboard0", &kbdDevice);
    if(drtn) {
	IOLog("initKeyboard: Can't find PCKeyboard0 (%s)\n",
	    [self stringFromReturn:drtn]);
	return nil;
    }

    return self;
}
//
// END:	Implementation of Private EventSrcPCKeyoard methods
//


//
// BEGIN:	Implementation of the KeyMapDelegate protocol
//
- keyboardEvent:(int)eventType
    flags:(unsigned)flags
    keyCode:(unsigned)keyCode
    charCode:(unsigned)charCode
    charSet:(unsigned)charSet
    originalCharCode:(unsigned)origCharCode
    originalCharSet:(unsigned)origCharSet
// Description: We use this notification to set up our keyRepeat timer
//		and to pass along the event to our owner. In the current
//		implementation this is always the EventDriver. In the future,
//		the km driver may be another potential owner. This method
//		will be called while the KeyMap object is processing
//		the key code we've sent it using doKeyboardEvent.
{
    [[self owner] keyboardEvent	:eventType
	flags:(flags | deviceDependentFlags)
	keyCode:keyCode
	charCode:charCode
	charSet:charSet
	originalCharCode:origCharCode
	originalCharSet:origCharSet
	repeat:isRepeat
	atTime:lastEventTime];

    // Set up key repeat operations here.
    return [self setRepeat:eventType forCode:keyCode];
}

- keyboardSpecialEvent:(unsigned)eventType
	flags:(unsigned)flags
	keyCode:(unsigned)keyCode
	specialty:(unsigned)flavor
// Description: See the description for keyboardEvent.
{
    [[self owner] keyboardSpecialEvent:eventType
	flags:(flags | deviceDependentFlags)
	keyCode	:keyCode
	specialty:flavor
	atTime:lastEventTime];

    // Set up key repeat operations here.
    if (flavor != NX_KEYTYPE_CAPS_LOCK)
	return [self setRepeat:eventType forCode:keyCode];
}

- updateEventFlags:(unsigned)flags
// Description:	Process non-event-generating flag changes. Simply pass this
//		along to our owner.
{
    return [[self owner] updateEventFlags:flags];
}

- (unsigned)eventFlags
// Description:	Return global event flags In this world, there is only
//		one keyboard device so device flags == global flags.
{
    return eventFlags;
}

- (unsigned)deviceFlags
// Description: Return per-device event flags. In this world, there is only
//		one keyboard device so device flags == global flags.
{
    return eventFlags;
}

- setDeviceFlags:(unsigned)flags
// Description: Set device event flags. In this world, there is only
//		one keyboard device so device flags == global flags.
{
    eventFlags = flags;
    return self;
}

- (BOOL)alphaLock
// Description: Return current alpha-lock state. This is a state tracking
//		callback used by the KeyMap object.
{
    return alphaLock;
}

- setAlphaLock:(BOOL)val
// Description: Set current alpha-lock state This is a state tracking
//		callback used by the KeyMap object.
{
    alphaLock = val;
    [kbdDevice setAlphaLockFeedback:val];
    return self;
}

- (BOOL)charKeyActive
// Description: Return YES If a character generating key down This is a state
//		tracking callback used by the KeyMap object.
{
    return charKeyActive;
}

- setCharKeyActive:(BOOL)val
// Description: Note whether a char generating key is down. This is a state
//		tracking callback used by the KeyMap object.
{
    charKeyActive = val;
    return self;
}
//
// END:		Implementation of the KeyMapDelegate protocol
//


//
// BEGIN:	Implementation of PCKeyboardImported protocol
//
- (void)dispatchKeyboardEvent:(PCKeyboardEvent *)event
// Description:	This method is the heart of event dispatching. The underlying
//		PCKeyboard object invokes this method with each event. We then
//		get the event xlated and dispatched using a keyMap instance.
//		The event structure passed in by reference should not be freed.
{
    unsigned keyMask;

    lastEventTime = event->timeStamp;
    if (event->keyCode != PC_NULL_KEYCODE)
    {
	// TO DO: The values for the key codes should be in some header file.
	switch (event->keyCode) {
	    case 0x2A: keyMask = NX_NEXTLSHIFTKEYMASK; break;
	    case 0x36: keyMask = NX_NEXTRSHIFTKEYMASK; break;
	    case 0x38: keyMask = NX_NEXTLALTKEYMASK; break;
	    case 0x61: keyMask = NX_NEXTRALTKEYMASK; break;
	    case 0x1D: keyMask = NX_NEXTCTLKEYMASK; break;
	    case 0x60: keyMask = NX_NEXTCTLKEYMASK; break;
	    default: keyMask = 0; break;
	}
	if (event->goingDown)
	    deviceDependentFlags |= keyMask;
	else
	    deviceDependentFlags &= ~keyMask;

	[deviceLock lock];
	[keyMap doKeyboardEvent:event->keyCode
	    direction:event->goingDown
	    keyBits:keyState];
	[deviceLock unlock];

    }
}

- (IOReturn)relinquishOwnershipRequest	: device
{
    IOReturn r;

    [[self ownerLock] lock];
    if ( [self owner] == nil )
    {
	ownDevice = NO;
	r = IO_R_SUCCESS;
    }
    else
	r = IO_R_BUSY;
    
    [[self ownerLock] unlock];
    return r; 
}

- (IOReturn)canBecomeOwner : device
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
    else
    {
	ownDevice = YES;
    }
    return( drtn);
}
//
// END:		Implementation of PCKeyboardImported protocol
//


//
// BEGIN:	Implementation of Exported EventSrcPCKeyoard methods
//
- init
// Description:	Initialize an EventSrcPCKeyboard
{
    [deviceLock lock];
    [super init];
    if ( keyMap != nil )
	[keyMap free];
    // Set up default keymapping
    keyMap = [[KeyMap alloc] initFromKeyMapping:PCDefaultKeymap
	length:sizeof PCDefaultKeymap
	canFree:NO];
    [keyMap setDelegate:self];
    [self initKeyboard];
    keyRepeat = EV_DEFAULTKEYREPEAT;
    initialKeyRepeat = EV_DEFAULTINITIALREPEAT;
    [deviceLock unlock];
    return self;
}

+ probe
// Description:	This is our factory method. It is the IODevice probe
//		routine for psuedo drivers.
{
    if ( instance != nil )
	return instance;

    instance = [self alloc];

    instance->deviceLock = [NXLock new];
    [instance setName:"EventSrcPCKeyboard0"];
    [instance setDeviceKind:"EventSrcPCKeyboard"];
    
    if ( [instance init] == nil )
	[instance free];	// No devices.  Go away
    return instance;
}

- free
// Description:	Go Away. Be careful when freeing the lock.
{
    id lock;
    
    lock = deviceLock;
    [lock lock];
    instance = nil;
    deviceLock = nil;
    if ( keyMap != nil )
	[keyMap free];
    // Release keyboard device, so we won't get any more events
    if ( kbdDevice != nil )
	[kbdDevice relinquishOwnership:self];
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

    if ( strcmp( parameterName, EVSIOCKR ) == 0 )	// Current Key Repeat
    {
	if ( maxCount >= EVSIOCKR_SIZE )
	{
	    *returnedCount = EVSIOCKR_SIZE;
	    [deviceLock lock];
	    nsecs_to_packed_ns(
	        &initialKeyRepeat, &parameterArray[EVSIOCKR_INITIAL]);
	    nsecs_to_packed_ns(
	        &keyRepeat, &parameterArray[EVSIOCKR_BETWEEN]);
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIOCKML ) == 0 )	// Current Keymap len.
    {
	if ( maxCount >= EVSIOCKML_SIZE )
	{
	    *returnedCount = EVSIOCKML_SIZE;
	    [deviceLock lock];
	    if ( keyMap == nil )
		parameterArray[0] = 0;
	    else
		parameterArray[0] = [keyMap keyMappingLength];
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIOINFO ) == 0 )	// Device info
    {
	dp = (NXEventSystemDevice *)&parameterArray[0];
	*returnedCount = 0;
	// No need to lock device since we're not even reading the structure.
	dp->interface = [kbdDevice interfaceId];
	dp->dev_type = NX_EVS_DEVICE_TYPE_KEYBOARD;
	dp->interface_addr = 0;
	dp->id = [kbdDevice handlerId];
	*returnedCount = sizeof(NXEventSystemDevice) / sizeof(int);
	r = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, EVSIOGKEYS ) == 0 )	// Key state
    {
	if( maxCount == EVK_NUNITS) {
	    [deviceLock lock];
	    bcopy( &keyState, parameterArray, EVK_NUNITS * sizeof( int));
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else
    {
	r = [super getIntValues:parameterArray
			forParameter:parameterName
			count:count];
	if (r == IO_R_UNSUPPORTED)
	    r = IO_R_INVALID_ARG;
    }
    return r;
}

- (IOReturn)getCharValues:(unsigned char *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int *)count
{
    IOReturn r = IO_R_INVALID_ARG;
    int cnt;
    unsigned maxCount = *count;
    unsigned *returnedCount = count;

    if ( strcmp( parameterName, EVSIOCKM ) == 0 )	// Current KeyMap
    {
	[deviceLock lock];
	if ( keyMap != nil )
	{
	    cnt = [keyMap keyMappingLength];
	    cnt = (cnt < maxCount) ? cnt : maxCount;
	    *returnedCount = cnt;
	    bcopy( [keyMap keyMapping:(int *)0], parameterArray, cnt );
	    r = IO_R_SUCCESS;
	}
	else
	    r = IO_R_NOT_ATTACHED;
	[deviceLock unlock];
    }
    else
    {
	r = [super getCharValues:parameterArray
			forParameter:parameterName
			count:count];
	if (r == IO_R_UNSUPPORTED)
	    r = IO_R_INVALID_ARG;
    }
    return r;
}
			
- (IOReturn)setIntValues:(unsigned *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
{
    IOReturn r = IO_R_INVALID_ARG;

    if ( strcmp( parameterName, EVSIOSKR ) == 0 )	// Set Key Repeat
    {
	if ( count == EVSIOSKR_SIZE )
	{
	    [deviceLock lock];
	    packed_nsecs_to_nsecs(parameterArray, &keyRepeat);
	    if ( keyRepeat < EV_MINKEYREPEAT )
		    keyRepeat = EV_MINKEYREPEAT;
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIOSIKR ) == 0 )	// Set Init Key Repeat
    {
	if ( count == EVSIOSIKR_SIZE )
	{
	    [deviceLock lock];
	    packed_nsecs_to_nsecs(parameterArray, &initialKeyRepeat);
	    if ( initialKeyRepeat < EV_MINKEYREPEAT )
		    initialKeyRepeat = EV_MINKEYREPEAT;
	    [deviceLock unlock];
	    r = IO_R_SUCCESS;
	}
    }
    else if ( strcmp( parameterName, EVSIORKBD ) == 0 )	// Reset keyboard
    {
	[self resetKeyboard];
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

- (IOReturn)setCharValues:(unsigned char *)parameterArray
	forParameter:(IOParameterName)parameterName
	count:(unsigned int)count
{
    IOReturn r = IO_R_INVALID_ARG;
    unsigned char *map;
    id oldMap;

    if ( strcmp( parameterName, EVSIOSKM ) == 0 )	// Set KeyMap
    {
	map = (unsigned char *)IOMalloc( count );
	bcopy( parameterArray, map, count );
	[deviceLock lock];
	oldMap = keyMap;
	keyMap = [[KeyMap alloc] initFromKeyMapping:map
	    length:count
	    canFree:YES];
	if ( keyMap != nil )
	{
	    if ( oldMap != nil )
		[oldMap free];
	    [keyMap setDelegate:self];
	    r = IO_R_SUCCESS;
	}
	else
	{
	    keyMap = oldMap;
	    r = IO_R_INVALID_ARG;
	} 
	[deviceLock unlock];
    }
    else
    {
	r = [super setCharValues:parameterArray
		   forParameter:parameterName
		   count : count];
	if (r == IO_R_UNSUPPORTED)
	    r = IO_R_INVALID_ARG;
    }
    return r;
}
//
// END:		Implementation of Exported EventSrcPCKeyoard methods
//



//
// BEGIN:	Implementation of EventSrcExported protocol
//
//
- (IOReturn)relinquishOwnership:(id)client
{
	IOReturn r;

	// kill autorepeat task
	downRepeatTime = 0ULL;
	codeToRepeat = -1;
	[self scheduleAutoRepeat];
	// clear modifiers to avoid stuck keys
	[[self owner] updateEventFlags:0];
	deviceDependentFlags = 0;
	eventFlags = 0;

	r = [super relinquishOwnership:client];
	[[self ownerLock] lock];
	// If nobody took over this EventSrc, release attached keyboards
	if ( r == IO_R_SUCCESS && [self owner] == nil )
	{
	    if ( [kbdDevice relinquishOwnership:self] == IO_R_SUCCESS )
		ownDevice = NO;
	}
	[[self ownerLock] unlock];
	return r;
}

- (IOReturn) becomeOwner:(id)client
{
	IOReturn r;

	r = [super becomeOwner:client];
	[[self ownerLock] lock];
	if ( r == IO_R_SUCCESS && ownDevice == NO )
	{
	    if ( [kbdDevice becomeOwner:self] == IO_R_SUCCESS )
	    {
		ownDevice = YES;
	    }
	    else if ( [kbdDevice desireOwnership:self] != IO_R_SUCCESS )
	    {
		[super relinquishOwnership:client];
		r = IO_R_BUSY;
	    }
	}
	[[self ownerLock] unlock];
	return r;
}
//
// END:		Implementation of EventSrcExported protocol
//

@end
