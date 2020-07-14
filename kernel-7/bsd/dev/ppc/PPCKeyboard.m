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
 * PPCKeyboard.m - Keyboard driver.
 * 
 *
 * HISTORY
 * 11-Aug-92    Joe Pasqua at NeXT
 *      Created. 
 * 8-April-97   Simon Douglas
 *      Munged into ADB version. Remove mouse interface.
 */
 
// TO DO:
//   Make this an indirect client of ADB. ADB should probe to create multiple
//   event sources.
// Notes:
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.

#define MACH_USER_API	1
#undef	KERNEL_PRIVATE

#import <machkit/NXLock.h>
#import <driverkit/IODevice.h>
#import <driverkit/driverServer.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <bsd/dev/ppc/PPCKeyboardPriv.h>
#import <bsd/dev/ppc/PPCKeyboard.h>
#import <bsd/dev/ppc/PCKeyboardDefs.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/ppc/driverTypes.h>
#import <bsd/dev/ev_types.h>
#import <bsd/dev/event.h>

#import <kern/thread_call.h>

#include <mach_kdb.h>
#include <mach/mach_types.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <machdep/ppc/mach_param.h>
#include <machine/machspl.h>

#include <machdep/ppc/powermac.h>

#include "busses.h"
#include "adb.h"
#import  "IOADBBus.h"

unsigned char	extendCount;

static int adbAsyncAvail = FALSE;			/* can try async ADB */
static int keyboard_led_state;      /* state of caps/num/scroll lock */
static id  adb_driver;              // pointer to adb indirect driver

extern struct tty   cons;

void keyboard_updown(unsigned char key);
void keyboard_adbhandler(int number, unsigned char *buffer, int count, void * ssp);
void keyboard_init(void);
void keyboard_updown(unsigned char key);

io_return_t keyboard_set_led_state(int state);
int     keyboard_get_led_state(void);
void        keyboard_initialize_led_state(void);

boolean_t   keyboard_initted = FALSE;
int     keyboard_adb_addr = -1;

#define	KeyboardIPL	3
#define	SAFE_IPL	(KeyboardIPL + 1)

#define MAX_PENDING_EVENTS 10

#if	KERNOBJC
static PPCKeyboard *_kbdObj;
#endif	KERNOBJC

static PCKeyboardEvent _pendingEvents[MAX_PENDING_EVENTS];
static int _eventIndex;
/* Forward declarations */
static inline PCKeyboardEvent *PPCScancodeToKeyEvent( unsigned char key);
static void EnqueueKey(int keyCode, ns_time_t stamp);


/* Forward declarations */
#define msg_printf null_printf
#define msg1_printf null_printf

#define busywait(x)	delay(x)


//
// BEGIN:	Definitions used to keep track of key state
//
// NOTES:	Key up/down state is tracked in a bit list.  Bits are set
//		for key-down, and cleared for key-up.  The bit vector and
//		macros for it's manipulation are defined here.
#define KBV_BITS_PER_UNIT	32
#define KBV_BITS_MASK		31
#define KBV_BITS_SHIFT		5	// 1<<5 == 32, for cheap divide
#define KBV_NUNITS ((128 + (KBV_BITS_PER_UNIT-1))/KBV_BITS_PER_UNIT)

#define KBV_KEYDOWN(n, bits) \
	(bits)[((n)>>KBV_BITS_SHIFT)] |= (1 << ((n) & KBV_BITS_MASK))

#define KBV_KEYUP(n, bits) \
	(bits)[((n)>>KBV_BITS_SHIFT)] &= ~(1 << ((n) & KBV_BITS_MASK))

#define KBV_IS_KEYDOWN(n, bits) \
	(((bits)[((n)>>KBV_BITS_SHIFT)] & (1 << ((n) & KBV_BITS_MASK))) != 0)

static unsigned long	_kbdBitVector[KBV_NUNITS];
//
// END:		Definitions used to keep track of key state
//


#if	KERNOBJC

@implementation PPCKeyboard
+ (IODeviceStyle) deviceStyle
{
  return IO_IndirectDevice;
}

static Protocol *protocols[] = {
      @protocol(ADBprotocol),
      nil
};

+ (Protocol **)requiredProtocols
{
      return protocols;
}



#endif

//
// BEGIN:	Impl. of internal methods for dealing w/ kbd controller
// NOTE:	The following are utility procedures for dealing with
//		The keyboard controller. They are called from the low
//		level interrupt handler. Because of this, they can not be
//		ObjC methods.
//


static void ADBKeyboardInit(void);

static void
ADBKeyboardInit(void)
{
	extern boolean_t adb_initted; //extern from adb.m
    int              i;
    IOADBDeviceInfo  table[IO_ADB_MAX_DEVICE];

    if (!keyboard_initted) {
		//if early debugging mode, table[] is filled with trash, ADB not initialized
		if (!adb_initted) //set in adb.m if ADB were instantiated
		{
        	keyboard_initted = FALSE;
			return;
		}
    	[adb_driver GetTable: table: &i];
      	for (i = 0; i < ADB_DEVICE_COUNT; i++) {
			if ((table[i].flags & ADB_FLAGS_PRESENT) == 0) 
			{
				continue;
			}
			if (table[i].originalAddress == ADB_DEV_KEYBOARD) {
	  		/* Find the first ADB keyboard device, and use that. */
	  			keyboard_adb_addr = i;
	  			break;  //break out of for loop
			}
      	}
        keyboard_initted = TRUE;
        /* Pickup all keyboard devices */
      	[adb_driver adb_register_handler:ADB_DEV_KEYBOARD: keyboard_adbhandler];
    }
}


/* Called the first time the console is opened, can't be done
 * above since it's too early
 */
void
keyboard_initialize_led_state(void)
{
    keyboard_led_state = keyboard_get_led_state();
}

int
keyboard_get_led_state(void)
{
    unsigned short  lights = 0;
    unsigned char   buffer[8];
    int             length;

    if (keyboard_adb_addr == -1)
        return 0;

    [adb_driver readADBDeviceRegister: keyboard_adb_addr: 2: buffer: &length];

    lights = buffer[0] << 8;
    if (length > 1) {
        lights |= buffer[1];
    }

    return ~lights;
}
    
int
keyboard_set_led_state(int lights)
{
    unsigned char buffer[2];

    if (keyboard_adb_addr == -1)
        return D_NO_SUCH_DEVICE;

    /* Inverse things for ADB */
    lights = ~lights;
    buffer[0] = lights >> 8;
    buffer[1] = lights & 0xFF;

    [adb_driver writeADBDeviceRegister: keyboard_adb_addr: 2: buffer: 2];

    return D_SUCCESS;
}


#import <kern/kdp_internal.h>
#import <mach/exception.h>

void 
keyboard_adbhandler(int number, unsigned char *buffer, int count, void * ssp)
{
ns_time_t	stamp;

    if ( (HasPMU() &&
       (buffer[0] == ADBK_POWER2 || buffer[1] == ADBK_POWER2)) ||
         (!HasPMU() &&
       (buffer[0] == ADBK_POWER && buffer[1] == ADBK_POWER)) )
    {
	if (KBV_IS_KEYDOWN(ADBK_CONTROL, _kbdBitVector))
	{
	    IOGetTimestamp(&stamp);

	    if (KBV_IS_KEYDOWN(ADBK_OPTION, _kbdBitVector))
	    {
		mini_mon("", "Kernel Debugger",ssp);
		EnqueueKey(ADBK_OPTION, stamp);
	    }
	    else {
		if( kdp.is_conn) {
#if 0
		    kdp_raise_exception( EXC_SOFTWARE, 0, 0, ssp);
#else
		    call_kdp();
#endif
		} else {
		    mini_mon("restart", "Restart",ssp);
		}
	    }
	    EnqueueKey(ADBK_CONTROL, stamp);
	    return;
	}
	keyboard_updown(ADBK_POWER);
	keyboard_updown(ADBK_KEYUP(ADBK_POWER));
	return;
    }

    keyboard_updown(buffer[0]);
    if (buffer[1] != 0xff)
        keyboard_updown(buffer[1]);
}


#if	KERNOBJC
void
KeyboardCallout()
{
	[_kbdObj interruptHandler];   
}
#else
void	cDispatchKeyboardEvent( PCKeyboardEvent *event);
#endif	KERNOBJC

void
keyboard_updown(unsigned char key)
{
    PCKeyboardEvent *event;
    if ( (event = (PCKeyboardEvent *)PPCScancodeToKeyEvent(key)) )
    {
	// Add this event to the queue. If there is no room in the queue,
	// we toss the event. TO DO: when we fill the last entry in the
	// queue we should disable keyboard interrupts until the queue gets
	// some free space.
	if (_eventIndex == MAX_PENDING_EVENTS)
	    return;
	_pendingEvents[_eventIndex++] = *event;

        // This call causes an interrupt message to be sent. Note that
	// if a previous message is outstanding, this call does nothing.
	// That is, no new message is queued.

	if( adbAsyncAvail)
	{

#if	KERNOBJC
#if	DRVRKITINTS
	    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
#else
	    thread_call_func((thread_call_func_t)KeyboardCallout, 0, TRUE);
#endif	DRVRKITINTS

#else
	    cDispatchKeyboardEvent( event );
	    _eventIndex = 0;
#endif	DRIVERKIT
 	}
   }
}

//
// END:		Impl. of internal methods for dealing w/ kbd controller
//

//
// BEGIN:	Implementation of internal functions and methods
//
static void EnqueueKey(int keyCode, ns_time_t stamp)
// Description:	Enqueue an event that corresponds to a key coming up
{
    PCKeyboardEvent event;
    
    if (_eventIndex != MAX_PENDING_EVENTS)
    {
	event.keyCode = keyCode;
	event.goingDown = 0;
	event.timeStamp = stamp;
	_pendingEvents[_eventIndex++] = event;
    }
}

static inline
PCKeyboardEvent *PPCScancodeToKeyEvent(
    unsigned char key)
// Description: Take a scan code from the keyboard and package it up into
//		a PCKeyboard event.
{
    static PCKeyboardEvent event;

    event.keyCode = ADBK_KEYVAL(key);
    IOGetTimestamp(&event.timeStamp);
    event.goingDown = ADBK_PRESS(key);

    if (event.goingDown)
    {
        if (KBV_IS_KEYDOWN(event.keyCode, _kbdBitVector))
            return (PCKeyboardEvent *)0;
	else
	    KBV_KEYDOWN(event.keyCode, _kbdBitVector);
    }
    else
        KBV_KEYUP(event.keyCode, _kbdBitVector);

    return (&event);
}

#if	KERNOBJC

- (void)interruptHandler
// Description:	This method is invoked by the I/O Thread when a keyboard
//		interrupt message has been recieved. This method reads
//		the keycode, processes it into a PCKeyboardEvent and
//		sends it to our owner.
{
    int oldIPL, nEvents, i;
    PCKeyboardEvent events[MAX_PENDING_EVENTS];

    if (_owner == nil)
    {
        _eventIndex = 0;  //bug 2228585
        return;		// After all that, no one is interested!
	}

    // Safely copy the events from the pending queue
    oldIPL = spltty();
    if (_eventIndex == 1)
        events[0] = _pendingEvents[0];
    else
	bcopy(_pendingEvents, events, sizeof(PCKeyboardEvent) * _eventIndex);
    nEvents = _eventIndex;
    _eventIndex = 0;
    (void)splx(oldIPL);
    
    for (i = 0; i < nEvents; i++)
    {
	[_owner dispatchKeyboardEvent:&events[i]];
    }
}

#ifdef	DRVRKITINTS

static volatile void kbdThread(PPCKeyboard *_kbdObject)
// Description: This is the function which is executed as the I/O Thread.
//		It waits for messages from driverkit and dispatches them
//		as appropriate.
{
    kern_return_t	result;
    msg_header_t	msg, *msgPtr = &msg;
    
    /* For the time being;it is already init to false */

    //
    // Main loop. Wait for incoming messages, dispatch as appropriate.
    //
    while (TRUE) {
	msgPtr->msg_size = sizeof(msg);
	msgPtr->msg_local_port = [_kbdObject interruptPort];
	
	result = msg_receive(msgPtr, MSG_OPTION_NONE, 0);
	if (result != RCV_SUCCESS) {
	    IOLog("kbdThread: msg_receive() returned %d\n", result);
	    continue;
	}
	
	if (msgPtr->msg_id == IO_DEVICE_INTERRUPT_MSG)
	{
	    [_kbdObject interruptHandler];   
	} else
			IOLog("kbdThread: non intr msg received %d\n", result);
    }
}

#endif	DRVRKITINTS

- (BOOL)kbdInit:(IODeviceDescription *)deviceDescription
// Description:	Initialize the keyboard object. Returns NO on error.
{
    char 	    locationStr[ 24 ];
    IOADBDeviceInfo deviceInfo;

    _owner = nil;
    _desiredOwner = nil;
    _ownerLock = [NXLock new];

#ifdef	DRVRKITINTS
    [self enableAllInterrupts];
    IOForkThread((IOThreadFunc)kbdThread, self);
#else
    _kbdObj = self;
#endif	DRVRKITINTS

    ADBKeyboardInit();
    adbAsyncAvail = TRUE;

    interfaceId = NX_EVS_DEVICE_INTERFACE_ADB;

    [adb_driver getADBInfo: keyboard_adb_addr: &deviceInfo];
    handlerId = 16 + (deviceInfo.handlerID);

    sprintf( locationStr, "handlerID %d", handlerId);
    [self setLocation:locationStr];

    return YES;	
}

//
// END:		Implementation of internal functions and methods
//

//
// BEGIN:	EXPORTED PPCKeyboard methods
//
+ (BOOL)probe:deviceDescription
// Description:	Bring a new instance into existance.
{
    PPCKeyboard *inst;

    //
    // Create an instance and initialize some basic instance variables.
    //
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];

    [inst setUnit: 0];
    [inst setName:"PPCKeyboard0"];
    [inst setDeviceKind:"PPCKeyboard"];

    //
    // Proceed with initialization.
    //
    if ([inst kbdInit:deviceDescription] == NO) {
	IOLog("PPCKeyboard probe: kdbInit failed\n");
	[inst free];
	return NO;
    }
    else
	[inst registerDevice];

    //IODelay(1000000);

    [inst setAlphaLockFeedback:NO];

    return YES;
}

- initFromDeviceDescription:(IODeviceDescription *)deviceDescription
{
    
    adb_driver = [deviceDescription directDevice];
    
    return self;
}

- (BOOL) getHandler:(IOPPCInterruptHandler *)handler
              level:(unsigned int *) ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
#ifdef	DRVRKITINTS
    *handler = KbdIntHandler;
    *ipl = 3;
    *arg = 0xdeadbeef;
    return YES;
#endif	DRVRKITINTS
    return NO;
}

- free
// Description:	Frees an instance.
{
    [_ownerLock free];
    return [super free];
}


- (int)interfaceId
// Description:       return keyboard interface ID.
{
    return interfaceId;
}

- (int)handlerId
// Description:       return keyboard handler ID.
{
    return handlerId;
}

- (void)setAlphaLockFeedback:(BOOL)locked
// Description:	Set the keyboard LEDs to indicate the state of alpha lock
{
    keyboard_set_led_state( locked ? ADBKS_LED_CAPSLOCK : 0 );
}

#endif	KERNOBJC

static PCKeyboardEvent stolenEvent;

PCKeyboardEvent *PPCStealKeyEvent()
// Description:	Call this routine to steal a keyboard event directly
//		from the hardware. Must be called from interrupt level.
{
    ADBKeyboardInit();

    if (_eventIndex == 0) {
	CheckADBPoll();
	if (_eventIndex == 0)
	    return NULL;
    }
    // This sucks.
    stolenEvent = _pendingEvents[0];
    _eventIndex--;
    bcopy( &_pendingEvents[1], &_pendingEvents[0], sizeof(PCKeyboardEvent) * _eventIndex);

    return( &stolenEvent );
}

//
// END:		EXPORTED PPCKeyboard methods
//

#if	KERNOBJC

//
// BEGIN:	Implementation of the PCKeyboardExported protocol
//
- (IOReturn)becomeOwner		: client;
// Description:	Register for event dispatch via device-specific protocol.
//		(See the dispatchKeyboardEvent method)
//		Returns IO_R_SUCCESS if successful, else IO_R_BUSY. The
//		relinquishOwnershipRequest: method may be called on another
//		client during the execution of this method.
{
    IOReturn rtn;
    
    [_ownerLock lock];
    if(_owner != nil) {
	if([_owner respondsTo:@selector(relinquishOwnershipRequest:)])
	{
	    rtn = [_owner relinquishOwnershipRequest:self];
	}
	else {
	    IOLog("%s: owner %s does not respond to "
		    "relinquishOwnershipRequest:\n",
		    [self name], [_owner name]);
	    rtn = IO_R_BUSY;
	}

	if(rtn == IO_R_SUCCESS) {
	    _owner = client;
	}
	else {
	    // NEGOTIATION FAILED
	}
    }
    else {
	_owner = client;
	rtn = IO_R_SUCCESS;
    }

    [_ownerLock unlock];

    if( rtn == IO_R_SUCCESS) {
	// Send the new owner a capslock down

	unsigned char	read_buf[8];
	int		rcvd_length;
	int		s;

	[ adb_driver readADBDeviceRegister: keyboard_adb_addr: 2: read_buf: &rcvd_length];
	if( (rcvd_length == 2) &&
	    (0 == (read_buf[0] & ADBKS_CAPSLOCK_short))) {
	    s = spltty();
	    KBV_KEYUP(ADBK_CAPSLOCK, _kbdBitVector);
	    keyboard_updown(ADBK_CAPSLOCK);
	    splx(s);
	}
    }
    return rtn;
}

- (IOReturn)relinquishOwnership	: client;
// Description:	Relinquish ownership. Returns IO_R_BUSY if caller is not
//		current owner.
{
    IOReturn rtn;
    
    [_ownerLock lock];
    if(_owner == client) {
	rtn = IO_R_SUCCESS;
	_owner = nil;
    }
    else {
	rtn = IO_R_BUSY;
    }
    [_ownerLock unlock];
    if((rtn == IO_R_SUCCESS) && 
	_desiredOwner &&
	(_desiredOwner != client)) {
	    /*
	     * Notify this potential client that it can take over.
	     * We'll most likely be called back during this method,
	     * which is why we released _ownerLock.
	     */
	    if([_desiredOwner respondsTo:@selector(canBecomeOwner:)]) {
		[_desiredOwner canBecomeOwner:self];
	    }
	    else {
		IOLog("%s: desiredOwner does not respond to "
			"canBecomeOwner:\n", [self name]);
	    }
    }
    return rtn;
}


- (IOReturn)desireOwnership	: client;
// Description:	Request notification (via canBecomeOwner:) when
//		relinquishOwnership: is called. This allows one potential
//		client to place itself "next in line" for ownership. The
//		queue is only one deep.
{
    IOReturn rtn;
    
    [_ownerLock lock];
    if(_desiredOwner && (_desiredOwner != client)) {
	rtn = IO_R_BUSY;
    }
    else {
	_desiredOwner = client;
	rtn = IO_R_SUCCESS;
    }
    [_ownerLock unlock];
    return rtn;	
}
//
// END:		Implementation of the PCKeyboardExported protocol
//
@end

#endif	KERNOBJC




