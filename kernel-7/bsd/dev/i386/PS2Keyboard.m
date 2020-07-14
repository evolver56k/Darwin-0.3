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
 * PS2Keyboard.m - Keyboard driver.
 * 
 *
 * HISTORY
 * 11-Aug-92    Joe Pasqua at NeXT
 *      Created. 
 */
 
// TO DO:
// * We still have kdreboot in here. Make a real public method and get the
//   rest of the system to use it.
// Notes:
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.

#define MACH_USER_API	1
#undef	KERNEL_PRIVATE

#import <machkit/NXLock.h>
#import <driverkit/driverServer.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <bsd/dev/i386/PS2KeyboardPriv.h>
#import <bsd/dev/i386/PS2Keyboard.h>
#import <bsd/dev/i386/PS2Mouse.h>
#import <bsd/dev/i386/PCKeyboardDefs.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/driverTypes.h>
#import <bsd/dev/ev_types.h>

#define	KeyboardIPL	3
#define	SAFE_IPL	(KeyboardIPL + 1)
#define	MAX_PENDING_EVENTS	5
#define	KEYCODE_NUMLOCK		0x45
#define	KEYCODE_LALT		0x38
#define	KEYCODE_RALT		0x61
#define	KEYCODE_PAUSE		0x6F

static PS2Keyboard *_kbdObj;
static PCKeyboardEvent _pendingEvents[MAX_PENDING_EVENTS];
static int _eventIndex;
static BOOL _manualDataHandling;

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


@implementation PS2Keyboard

//
// BEGIN:	Impl. of internal methods for dealing w/ kbd controller
// NOTE:	The following are utility procedures for dealing with
//		The keyboard controller. They are called from the low
//		level interrupt handler. Because of this, they can not be
//		ObjC methods.
//
static void SendCmd(unsigned char cmd)
// Description:	Sends a command to the keyboard controller
{
    // Wait for room in the buffer
    while (inb(K_STATUS) & K_IBUF_FUL)
	continue;
    outb(K_CMD, cmd);	// Send the command
    return;
}

static void SendData(unsigned char data)
// Description:	Sends data to the keyboard itself
{
    // Wait for room in the keyboard's input buffer
    while (inb(K_STATUS) & K_IBUF_FUL)
	continue;
    outb(K_RDWR, data);	// Send the data
    _kbdObj->lastSent = data;
}

static void DoResend()
// Description:	Resends the last data sent to the keyboard
{
    if (_kbdObj->pendingAck == NOT_WAITING) 
	IOLog("PS2Keyboard/DoResend: Unexpected RESEND from keyboard\n");
    else
        SendData(_kbdObj->lastSent);
}

static unsigned char GetData()
// Description:	Read data from the keyboard
{
    // Spin until there is something ready to read
    while ((inb(K_STATUS) & (K_OBUF_FUL | M_OBUF_FUL)) == 0);
    IODelay(K_DATA_DELAY);
    return(inb(K_RDWR));	// Read the data
}

static void BeginSettingLEDs(unsigned char val)
// Description:	Begin the process of setting the keyboard LEDs. First
//		you tell the keyboard that you want to set the LEDs. It
//		will respond with an ACK. When we get the ACK, HandleAck
//		will call CompleteSettingLEDs which sends the LED values
//		to the keyboard.
{
    if (_kbdObj->pendingAck != NOT_WAITING) {
	IOLog("PS2Keyboard/BeginSettingLEDs: Currently awaiting an ACK\n");
	return;
    }

    _kbdObj->pendingAck = SET_LEDS;
    _kbdObj->pendingLEDVal = val;
    SendData(K_CMD_LEDS);
}

static void CompleteSettingLEDs()
// Description:	See BeginSettingLEDs().
{
    SendData(_kbdObj->pendingLEDVal);	// Send the actual value
}

static void HandleAck()
// Description:	Respond to an ACK from the keyboard based on our state.
{
    switch (_kbdObj->pendingAck) {
	case SET_LEDS:
	    CompleteSettingLEDs();
	    _kbdObj->pendingAck = DATA_ACK;
	    break;
	case DATA_ACK:
	    _kbdObj->pendingAck = NOT_WAITING;
	    break;
	case NOT_WAITING:
	    IOLog("PS2Keyboard/HandleAck: Unexpected ACK from keyboard\n");
	    break;
	default:
	    IOLog("PS2Keyboard/HandleAck: pendingAck has impossible value\n");
	    break;
    }
}

static void InitKbdController()
// Description: Prepare the keyboard controller for use. This involves
//		setting certain enable bits in the controller.
{
    unsigned char cmdByte;		// keyboard command byte

    _manualDataHandling = YES;	// We'll be reading data manually

    /* get rid of any garbage in output buffer */
    if (inb(K_STATUS) & (K_OBUF_FUL | M_OBUF_FUL)) {
	    IODelay(K_DATA_DELAY);
	    (void)inb(K_RDWR);
    }
    
    SendCmd(KC_CMD_READ);	// ask for the ctlr command byte
    cmdByte = GetData();
    cmdByte |= K_CB_TRANSLATE;	// translate incoming scan codes to scan code 1
    cmdByte &= ~K_CB_DISBLE;	// Enable kbd dev
    cmdByte |= K_CB_ENBLIRQ;	// Enable kbd IRQ
    cmdByte |= M_CB_DISBLE;	// Disable mouse dev
    cmdByte &= ~M_CB_ENBLIRQ;	// Disable mouse IRQ
    SendCmd(KC_CMD_WRITE);	// Write new ctlr command byte
    SendData(cmdByte);
    _manualDataHandling = NO;	// We're done reading data manually
}
//
// END:		Impl. of internal methods for dealing w/ kbd controller
//


//
// BEGIN:	Implementation of internal functions and methods
//
static void EnqueueAltUp(int keyCode, ns_time_t stamp)
// Description:	Enqueue an event that corresponds to an ALT key coming up
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

static BOOL SpecialKeys(PCKeyboardEvent *event, void *state)
// Description:	Test whether we have encountered a special key sequence.
//		If we have a special sequence, then we invoke mini_mon
//		appropriately and return YES. Otherwise we return NO.
{
    if (event->keyCode == KEYCODE_NUMLOCK && event->goingDown)
    {
	if (KBV_IS_KEYDOWN(KEYCODE_RALT, _kbdBitVector))
	{
	    if (KBV_IS_KEYDOWN(KEYCODE_LALT, _kbdBitVector))
	    {
		mini_mon("", "Mini-Monitor", state);
		EnqueueAltUp(KEYCODE_LALT, event->timeStamp);
	    }
	    else
		mini_mon("restart", "Restart", state);
	    EnqueueAltUp(KEYCODE_RALT , event->timeStamp);
	    return YES;
	}
    }
    
    return NO;
}

static inline
PCKeyboardEvent *ScancodeToKeyEvent(
    unsigned char key, unsigned char *extendCount)
// Description: Take a scan code from the keyboard and package it up into
//		a PCKeyboard event. If this is the first part of an extended
//		sequence then return a NIL pointer.
{
    static PCKeyboardEvent event;

    if (key == K_EXTEND) {
	// This introduces an extended key sequence. Note that fact
	// and then return. Next time we get a key we'll finish the
	// sequence.
	*extendCount = 1;
	return ((PCKeyboardEvent *) 0);
    }

    if (key == K_PAUSE) {
	// This introduces an extended key sequence for the pause key.
	// Note that fact and then return. We'll eat the rest of the
	// key sequence and return a non-zero keycode at the end.
	// The sequence is K_PAUSE 1D 5 K_PAUSE 9D C5. Note the 2nd
	// occurance of the K_PAUSE keycode.
	if (*extendCount == 0)
	{
	    *extendCount = 5;
	    return ((PCKeyboardEvent *) 0);
	}
	// Else this is the 2nd K_PAUSE in the sequence, chuck it...
    }


    if (*extendCount == 0) 
	event.keyCode = key & ~K_UP;
    else
    {
        (*extendCount)--;
	if (*extendCount != 0)
	    return ((PCKeyboardEvent *) 0);
	// Convert certain extended codes on the PC keyboard
	// into single scancodes
	switch (key & ~K_UP) {
	    case 0x1D: event.keyCode = 0x60; break;     // ctrl
	    case 0x38: event.keyCode = 0x61; break;	// alt
            case 0x1C: event.keyCode = 0x62; break;     // enter
            case 0x35: event.keyCode = 0x63; break;     // /
            case 0x48: event.keyCode = 0x64; break;     // up arrow
            case 0x50: event.keyCode = 0x65; break;     // down arrow
            case 0x4B: event.keyCode = 0x66; break;     // left arrow
            case 0x4D: event.keyCode = 0x67; break;     // right arrow
            case 0x52: event.keyCode = 0x68; break;     // insert
            case 0x53: event.keyCode = 0x69; break;     // delete
            case 0x49: event.keyCode = 0x6A; break;     // page up
            case 0x51: event.keyCode = 0x6B; break;     // page down
            case 0x47: event.keyCode = 0x6C; break;     // home
            case 0x4F: event.keyCode = 0x6D; break;     // end
	    case 0x37: event.keyCode = 0x6E; break;	// PrintScreen
	    case 0x45: event.keyCode = KEYCODE_PAUSE; break;	// Pause
	    case 0x2A: // This is a header or trailer for PrintScreen
	    default: return ((PCKeyboardEvent *) 0);
        }
    }

    if (event.keyCode == 0)
	return (PCKeyboardEvent *) 0;

    IOGetTimestamp(&event.timeStamp);
    event.goingDown = !(key & K_UP);
    
    // Deal with the Pause key in a special way. It only generates a
    // down code and it does not repeat. It always looks like a key up
    // to this code.
    if (event.keyCode == KEYCODE_PAUSE)
        event.goingDown = !(KBV_IS_KEYDOWN(event.keyCode, _kbdBitVector));
    
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

static void KbdIntHandler(void *identity, void *state, unsigned int arg)
// Description:	This is the low level interupt handler that gets called when
//		a keyboard interrupt occurs. It reads the key from the
//		controller, turns it into an event and adds it to a queue
//		of events for the high level interrupt handler to process.
//		It then causes an interrupt message to be sent. The
//		interruptHandler method will eventually receive that message
//		and send all queued events to the higher levels.
// NOTE:	This code also watches for "special" key sequences which are
//		meant to allow reboots and to invoke the debugger. This must
//		be done at interrupt level so that we avoid having a huge
//		amount of machinery between the request for a reboot/debugger
//		and the completion of that request. Furthermore, we need to
//		have a thread_saved_state_t in order to invoke the debugger.
{
    unsigned char key;
    PCKeyboardEvent *event;

    // Sometimes when tweaking the controller the code wants to poll
    // for data coming back from the controller. In these cases we just
    // ignore the interrupt telling us that there is data available. Someone
    // else will poll and read the data.  
    if (_manualDataHandling == YES)
        return;

    // If there's nothing there, ignore the interrupt
    if (!((unsigned char)inb(K_STATUS) & K_OBUF_FUL))
	return;

    IODelay(K_DATA_DELAY);    
    key = (unsigned char)inb(K_RDWR);	// Read data from keyboard

    if (key == K_ACKSC) {
	HandleAck();	// Respond to ack of previous request
	return;
    }
    else if (key == K_RESEND) {
	DoResend();	// Resend previous data
	return;
    }

    if ( (event = ScancodeToKeyEvent(key, &_kbdObj->extendCount)) )
    {
        if (SpecialKeys(event, state) == NO)
	{
	    // Add this event to the queue. If there is no room in the queue,
	    // we toss the event. TO DO: when we fill the last entry in the
	    // queue we should disable keyboard interrupts until the queue gets
	    // some free space.
	    if (_eventIndex == MAX_PENDING_EVENTS)
		return;
	    _pendingEvents[_eventIndex++] = *event;
	}
	
        // This call causes an interrupt message to be sent. Note that
	// if a previous message is outstanding, this call does nothing.
	// That is, no new message is queued.
    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
    }
}

- (void)interruptHandler
// Description:	This method is invoked by the I/O Thread when a keyboard
//		interrupt message has been recieved. This method reads
//		the keycode, processes it into a PCKeyboardEvent and
//		sends it to our owner.
{
    int oldIPL, nEvents, i;
    PCKeyboardEvent events[MAX_PENDING_EVENTS];

    if (_owner == nil)
        return;		// After all that, no one is interested!

    // Safely copy the events from the pending queue
    oldIPL = splx(SAFE_IPL);
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


static volatile void kbdThread(PS2Keyboard *_kbdObject)
// Description: This is the function which is executed as the I/O Thread.
//		It waits for messages from driverkit and dispatches them
//		as appropriate.
{
    kern_return_t	result;
    msg_header_t	msg, *msgPtr = &msg;
    port_t		kbdPort = [_kbdObject interruptPort];
    
    [_kbdObject setAlphaLockFeedback:FALSE];	// Set state of keys

    //
    // Main loop. Wait for incoming messages, dispatch as appropriate.
    //
    while (TRUE) {
	msgPtr->msg_size = sizeof(msg);
	msgPtr->msg_local_port = _kbdObject->portSet;
	
	result = msg_receive(msgPtr, MSG_OPTION_NONE, 0);
	if (result != RCV_SUCCESS) {
	    IOLog("kbdThread: msg_receive() returned %d\n", result);
	    continue;
	}
        if (msgPtr->msg_id == IO_DEVICE_INTERRUPT_MSG)
	{
	    if (msgPtr->msg_local_port ==  kbdPort)
	        [_kbdObject interruptHandler];   
	    else if (msgPtr->msg_local_port ==  _kbdObject->mouseIntPort)
	        [_kbdObject->mouseObject interruptHandler];   
	    else
		IOLog("kbdThread: Bogus msg_local_port\n");
	}
    }
}

- (BOOL)kbdInit:deviceDescription
// Description:	Initialize the keyboard object. Returns NO on error.
{
    kern_return_t krtn;
    IOReturn drtn;
    IOThread thread;
    id configTable;
    char *keyStr;

    pendingAck = NOT_WAITING;	// Set up initial state
    lastSent = 0;
    pendingLEDVal = 0;
    extendCount = 0;
    mouseObject = 0;
    _owner = nil;
    _desiredOwner = nil;
    _ownerLock = [NXLock new];

    // Obtain keyboard interface and hander IDs from config table.
    configTable = [[self deviceDescription] configTable];
    if (configTable == nil) {
	IOLog("PS2Keyboard kbdInit: no configuration table\n");
	return NO;
    }
    keyStr = (char *)[configTable valueForStringKey:"Interface"];
    if (keyStr == NULL) {
	IOLog("PS2Keyboard kbdInit: no Interface ID; use default\n");
	interfaceId = NX_EVS_DEVICE_INTERFACE_ACE;
    } else
	interfaceId = PCPatoi(keyStr);
    keyStr = (char *)[configTable valueForStringKey:"Handler ID"];
    if (keyStr == NULL) {
	IOLog("PS2Keyboard kbdInit: no Handler ID; use default\n");
	handlerId = 0;
    } else
	handlerId = PCPatoi(keyStr);

    // Initialize the controller itself.
    InitKbdController();

    // We create this portSet to contain the interrupt port for
    // the keyboard and the interrupt port for the mouse. When the
    // mouse object is created, it will call us and provide it's
    // interruptPort to add to the portSet.
    [self enableAllInterrupts];
    krtn = port_set_allocate(task_self(), &portSet);
    if(krtn) {
	IOLog("kbdInit: port_set_allocate returned %d\n", krtn);
	return NO;
    }
    krtn = port_set_add(task_self(), portSet, [self interruptPort]);
    if(krtn) {
	IOLog("kbdInit: port_set_add returned %d\n", krtn);
	return(-1);
    }

    thread = IOForkThread((IOThreadFunc)kbdThread, self);
    (void) IOSetThreadPolicy(thread, POLICY_FIXEDPRI);
    (void) IOSetThreadPriority(thread, 28);	/* XXX */
    
    return YES;	
}
//
// END:		Implementation of internal functions and methods
//


//
// BEGIN:	EXPORTED PS2Keyboard methods
//
+ (BOOL)probe:deviceDescription
// Description:	Bring a new instance into existance.
{
    PS2Keyboard *inst;
    
    //
    // Create an instance and initialize some basic instance variables.
    //
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];

    [inst setUnit: 0];
    [inst setName:"PCKeyboard0"];
    [inst setDeviceKind:"PS2Keyboard"];

    //
    // Proceed with initialization.
    //
    if ([inst kbdInit:deviceDescription] == NO) {
	IOLog("PS2Keyboard probe: kdbInit failed\n");
	[inst free];
	return NO;
    }
    else
	[inst registerDevice];

    
    _kbdObj = inst;
    return YES;
}

- (BOOL) getHandler:(IOEISAInterruptHandler *)handler
              level:(unsigned int *) ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
    *handler = KbdIntHandler;
    *ipl = 3;
    *arg = 0xdeadbeef;
    return YES;
}

- free
// Description:	Frees an instance.
{
    [_ownerLock free];
    port_set_deallocate(task_self(), portSet);
    return [super free];
}

- (int)attachMouse:mouseObj withPort:(port_t)mousePort
// Description:	Add the given port to the port set that we use to listen
//		for interrupt messages. When we get a message on the mousePort,
//		we dispatch it to mouseObj. This routine also initializes the
//		mouse related parts of the controller.
{
    unsigned char cmdByte;		// keyboard command byte
    kern_return_t krtn;
    unsigned char i;
    
    mouseObject = mouseObj;
    mouseIntPort = mousePort;
    krtn = port_set_add(task_self(), portSet, mouseIntPort);
    if (krtn) {
	IOLog("attachMouse: port_set_add returned %d\n", krtn);
	return(-1);
    }

    _manualDataHandling = YES;	// We'll be reading data manually
    /* get rid of any garbage in output buffer */
    if (inb(K_STATUS) & (K_OBUF_FUL | M_OBUF_FUL)) {
	IODelay(K_DATA_DELAY);
	(void)inb(K_RDWR);
    }
    
    SendCmd(KC_CMD_READ);	// ask for the ctlr command byte
    cmdByte = GetData();
    cmdByte &= ~(K_CB_DISBLE | M_CB_DISBLE);  // enable mouse & kbd devs
    cmdByte |= (K_CB_ENBLIRQ | M_CB_ENBLIRQ); // enable mouse & kbd IRQs
    SendCmd(KC_CMD_WRITE);	// Write new ctlr command byte
    SendData(cmdByte);

    SendCmd(KC_CMD_MOUSE);	// Send next data to mouse
    SendData(M_CMD_SAMPLING);	// Set mouse sampling rate
    SendCmd(KC_CMD_MOUSE);	// Send next data to mouse
    SendData(5);		// Set sampling index to 100 reports/second
    (void)GetData();		// Eat the ack

    SendCmd(KC_CMD_MOUSE);	// Send next data to mouse
    SendData(M_CMD_SETRES);	// Set mouse resolution
    SendCmd(KC_CMD_MOUSE);	// Send next data to mouse
    SendData(0x1);		// Set resolution index to 2 counts/mm
    (void)GetData();		// Eat the ack

    SendCmd(KC_CMD_MOUSE);	// Send next data to mouse
    SendData(M_CMD_POLL);	// Enable mouse data transmission
    (void)GetData();			// Eat the ack
    
    _manualDataHandling = NO;	// We're done reading data manually

    return(0);
}

- (void)detachMouse
// Description:	Don't listen on the mouse port any longer.
{
	if (mouseObject == 0)
		return;
	// TO DO: delete the mouseIntPort from the portSet;
	return;
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
	BeginSettingLEDs(locked ? K_LED_CAPSLK : 0);
}

PCKeyboardEvent *StealKeyEvent()
// Description:	Call this routine to steal a keyboard event directly
//		from the hardware. Must be called from interrupt level.
{
    unsigned char key;
    PCKeyboardEvent *event;

    // Toss mouse events.
    // FIX: This is going to confuse the mouse driver no end!!!
    if ((unsigned char)inb(K_STATUS) & M_OBUF_FUL) {
	IODelay(K_DATA_DELAY);
	(void)inb(K_RDWR);
    }

    // If there's nothing there, just return
    if (!((unsigned char)inb(K_STATUS) & K_OBUF_FUL))
	return (PCKeyboardEvent *)0;
    
    IODelay(K_DATA_DELAY);
    key = (unsigned char)inb(K_RDWR);	// Read data from keyboard

    if (key == K_ACKSC) {
	HandleAck();	// Respond to ack of previous request
	return (PCKeyboardEvent *)0;
    }
    else if (key == K_RESEND) {
	DoResend();	// Resend previous data
	return (PCKeyboardEvent *)0;
    }

    event = ScancodeToKeyEvent(key, &_kbdObj->extendCount);

    return event;
}
//
// END:		EXPORTED PS2Keyboard methods
//

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

kdreboot()
// Description:	Sends a "magic" sequence to the keyboard controller
//		which causes it to send a signal back to the system which
//		causes the system to reboot.
// TO DO:	Export this in the interface
{
    // Wait for room in the buffer
    while (inb(K_STATUS) & K_IBUF_FUL)
	continue;
    outb(K_CMD, KC_REBOOT);	// Send the command
    return;
}

