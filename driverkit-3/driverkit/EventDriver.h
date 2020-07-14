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
 * EventDriver.h - Exported Interface Event Driver object.
 *
 *		The EventDriver is a pseudo-device driver.
 *
 * HISTORY
 * 19 Mar 1992    Mike Paquette at NeXT
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	API cleanup
 */

#ifndef	_EVENT_DRIVER_
#define _EVENT_DRIVER_

#import <driverkit/IODevice.h>
#import <kernserv/queue.h>
#import <driverkit/eventProtocols.h>
#import <bsd/dev/ev_types.h>
#import <bsd/dev/ev_keymap.h>		/* For NX_NUM_SCANNED_SPECIALKEYS */

@interface EventDriver: IODevice <IOScreenRegistration, IOEventThread, IOWorkspaceBounds>
{
@private

	BOOL		hasRegistered;	// Has device been registered yet?
	port_t		devicePort;
	id		driverLock;

	// Ports on which we hold send rights
	port_t	eventPort;		// Send msg here when event queue
					// goes non-empty
	port_t	specialKeyPort[NX_NUM_SCANNED_SPECIALKEYS]; // Special key msgs

	// Ports owned by the event driver
	port_t		ev_port;	// Privileged request port
	port_t		evs_port;	// event status port
	port_t		notify_port;	// client port death notification

	// kern_port_t versions of previous ports. Typed as port_t's to allow
	// MACH_USER_API compilation.
	port_t		notify_kern_port;	// from notify_port
	port_t		event_kern_port;	// from eventPort

	port_set_name_t	ev_port_set;	// Set holding above ports

	void		*eventMsg;	// Msg to be sent to Window Server.

	// Shared memory area information
	port_t		owner_task;	// task port object for owner
	void 		*owner;		// Task which shares mem with driver
	vm_offset_t	owner_addr;	// Address as mapped into owner task
	vm_offset_t	shmem_addr;	// kernel address of shared memory
	vm_size_t	shmem_size;	// size of shared memory

	// Pointers to structures which occupy the shared memory area.
	volatile void	*evs;		// Pointer to private driver shmem
	volatile void	*evg;		// Pointer to EvGlobals (shmem)
	// Internal variables related to the shared memory area
	int		lleqSize;	// # of entries in low-level queue
	
	// Event sources list
	id		eventSrcListLock;
	queue_head_t	eventSrcList;

	// Screens list
	vm_size_t	evScreenSize;	// Byte size of evScreen array
	void		*evScreen;	// array of screens known to driver
	volatile void	*lastShmemPtr;	// Pointer used to index thru shmem
					// while assigning shared areas to
					// drivers.
	int		screens;	// running total of allocated screens
	int		currentScreen;	// Current active screen
	Bounds		cursorPin;	// Range to which cursor is pinned
					// while on this screen.
	Bounds		workSpace;	// Bounds of full workspace.
	// Event Status state - This includes things like event timestamps,
	// time til screen dim, and related things manipulated through the
	// Event Status API.
	//
	// Note: In this implementation, a tick is defined as 1/64th of
	// one second.  Vertical blanking interrupts are not used.
	//
	unsigned autoDimPeriod;	// How long since last user action before
				// we autodim screen?  User preference item,
				// set by InitMouse and evsioctl
	unsigned autoDimTime;	// Time value when we will autodim screen,
				// if autoDimmed is 0.
				// Set in LLEventPost.
	Point	pointerLoc;	// Current pointing device location
				// The value leads evg->cursorLoc.
	Point	clickLoc;	// location of last mouse click
	Point clickSpaceThresh;	// max mouse delta to be a doubleclick
	int	clickState;	// Current click state
	unsigned clickTime;	// Timestamps used to determine doubleclicks
	unsigned clickTimeThresh;
	unsigned char lastPressure;	// last pressure seen
	BOOL	lastProximity;	// last proximity state seen

	int	curVolume;	// Value of volume setting.
	int	dimmedBrightness;// Value of screen brightness when autoDim
				// has turned on.
	int	curBright;	// The current brightness is cached here while
				// the driver is open.  This number is always
				// the user-specified brightness level; if the
				// screen is autodimmed, the actual brightness
				// level in the monitor will be less.
	BOOL evOpenCalled;	// Has the driver been opened?
	BOOL evInitialized;	// Has the first-open-only initialization run?
	BOOL eventsOpen;	// Boolean: has evmmap been called yet?
	BOOL autoDimmed;	// Is screen currently autodimmed?
	ns_time_t waitSustain;	// Sustain time before removing cursor
	ns_time_t waitSusTime;	// Sustain counter
	ns_time_t waitFrameRate;// Ticks per wait cursor frame
	ns_time_t waitFrameTime;// Wait cursor frame timer

	short leftENum;		// Unique id for last left down event
	short rightENum;	// Unique id for last right down event
	
	// The periodic event mechanism timestamps and state
	// are recorded here.
	ns_time_t	thisPeriodicRun;
	ns_time_t	nextPeriodicRun;
	BOOL		periodicRunPending;

	// Flags used in scheduling periodic event callbacks
	BOOL		needSetCursorPosition;
	BOOL		needToKickEventConsumer;
	id		kickConsumerLock;
}

/* subclass specific factory methods */
+ instance;			/* Return the current instance of the */
				/* EventDriver, or nil if none. */

/* Methods from superclass which we override */
+ (BOOL)probe : deviceDescription;
+ (IODeviceStyle)deviceStyle;
- init;
- free;
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out
			
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

#if KERNEL
- (IOReturn) mapEventShmem : (port_t)event_port
		   task : (port_t)task			// in
		   size : (vm_size_t)size		// in
		   at : (vm_offset_t *)addr;		// out
- (IOReturn) unmapEventShmem : (port_t)event_port;
#endif
- (IOReturn)evOpen:(port_t)dev_port token:(port_t)event_port;
- (IOReturn)evClose:(port_t)dev_port token:(port_t)event_port;
- (IOReturn)evFrameBufferDevicePort:(port_t)event_port
		  unitName:(IOString)name
		  unitClass:(IOString)class
		  unitPort:(port_t *)port;
- (IOReturn)evSetScreen:(unsigned int *)parameterArray;
- initShmem;			// Initialize the shared memory area
- setEventPort:(port_t)port;	// Set the port for event available notify msg
// Set the port for the special key keypress msg
- (IOReturn)	setSpecialKeyPort	: (port_t)dev_port
				keyFlavor	: (int)special_key
				keyPort		: (port_t)key_port;
- (port_t)specialKeyPort: (int)special_key;

//	Return ports used for Mach interface
- (port_t)ev_port;
- (port_t)evs_port;

// Dispatch mechanism for special key press
- evSpecialKeyMsg:	(unsigned)key
			direction:(unsigned)dir
			flags:(unsigned)f
			level:(unsigned)l;

// Dispatch mechanisms for screen state changes
- evDispatch:(int)screen command:(EvCmd)cmd;

// Dispatch low level events through shared memory to the WindowServer
- postEvent:(int)what
	at:(Point *)location
	atTime:(unsigned)theClock
	withData:(NXEventData *)myData;

// Message the event consumer to process posted events
- kickEventConsumer;
- _performKickEventConsumer:(id)data;
- (IOReturn)_threadOpCommon : (int)op
			     opParams : (void *)opParams
			     async : (BOOL)async;
/* Resets */
- _resetMouseParameters;
- _resetKeyboardParameters;

/* I/O thread methods */
- (void)_ioOpHandler	: (void *)opBuf;
- (IOReturn)_doPerformInIOThread:(void *)data;

@end

@interface Object (prober)
- probe; //! this is in here to prevent a warning that we get when sending
	 //! a method to a class that has been looked up
@end;

#endif	_EVENT_DRIVER_

