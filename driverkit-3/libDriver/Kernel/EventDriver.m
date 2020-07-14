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
 * EventDriver.m - Event System module, ObjC implementation.
 *
 *		The EventDriver is a pseudo-device driver.
 *
 * HISTORY
 * 31-Mar-92    Mike Paquette at NeXT 
 *      Created. 
 * 4  Aug 1993	  Erik Kay at NeXT
 *	minor API cleanup
 */

#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/kernelDriver.h>
#import <mach/notify.h>
#import <bsd/dev/evio.h>
#import <kern/queue.h>
#import <bsd/dev/machine/ev_private.h>	/* Per-machine configuration info */
#import <driverkit/EventDriver.h>
#import <driverkit/EventInput.h>
#import <bsd/dev/evsio.h>

#define EVSRC_PRINT	0
#if	EVSRC_PRINT
#undef	xpr_evsrc
#define xpr_evsrc(x,a,b,c,d,e) printf(x,a,b,c,d,e)
#endif	EVSRC_PRINT

static EventDriver *evInstance = (EventDriver *)nil;
static volatile void EventListener(EventDriver *inst);

typedef void * kern_port_t;
#define KERN_PORT_NULL	((kern_port_t) 0)

/*
 * Template for evIoOpMsg.
 */
static evIoOpMsg opMsgTemplate = {

        {   						// header 
                0,					// msg_unused 
                1,					// msg_simple 
                sizeof(evIoOpMsg), 			// msg_size 
                MSG_TYPE_NORMAL,			// msg_type 
                PORT_NULL, 				// msg_local_port 
                PORT_NULL,				// msg_remote_port - TO
                                                        // BE FILLED IN 
                (int)EV_IO_OP_MSG_ID 			// msg_id 
        },
        { 						// type 
                MSG_TYPE_UNSTRUCTURED,			// msg_type_name 
                sizeof(evIoOpBuf) * 8,			// msg_type_size 
                1,					// msg_type_number 
                1, 					// msg_type_inline 
                0, 					// msg_type_longform 
                0,					// msg_type_deallocate 
                0					// msg_type_unused
        },
        { 0 }						/* evIoOpBuf - TO BE 
							 * FILLED IN */
};

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

@implementation EventDriver: IODevice

/* Probe routine for a pseudo-device driver. */
+ (BOOL)probe : deviceDescription
{	
	if ( evInstance != nil )
		return YES;

	evInstance = [self alloc];
	/*
	 * Take care of private stuff...
	 */
	evInstance->devicePort = PORT_NULL;
	[evInstance setUnit:0];
	[evInstance setName:"event0"];
	[evInstance setDeviceKind:"event"];
	
	return ([evInstance init] ? YES : NO);
}

+ (IODeviceStyle)deviceStyle
{
	return IO_PseudoDevice;
}

/* subclass specific methods */

/* Return the current instance of the EventDriver, or nil if none. */
+ instance
{
	return (id)evInstance;
}

/*
 * Perform reusable initialization actions here.
 */
- init
{
	kern_return_t krtn;
	IOReturn drtn;
	IOThread thread;
#ifdef KERNEL
	extern kern_port_t ev_port_list[];
#endif
	
	driverLock = [NXLock new];	// Event driver data protection lock
	eventSrcListLock = [NXLock new];
	kickConsumerLock = [NXLock new];

	/*
	 * Set up the ports we'll be using.
	 */
	krtn = port_allocate(task_self(), &ev_port);
	if(krtn) {
		xpr_err("Ev init: port_allocate returned %d\n", krtn, 2,3,4,5);
		return nil;
	}

	krtn = port_allocate(task_self(), &evs_port);
	if(krtn) {
		xpr_err("Ev init: port_allocate returned %d\n", krtn, 2,3,4,5);
		return nil;
	}
	krtn = port_allocate(task_self(), &notify_port);
	if(krtn) {
		xpr_err("Ev init: port_allocate returned %d\n", krtn, 2,3,4,5);
		return nil;
	}
#ifdef	KERNEL
	/*
	 * Get a kern_port_t version of same for use with 
	 * msg_send_from_kernel().
	 */
	ev_port_list[0] = (kern_port_t)IOGetKernPort(ev_port);
	ev_port_list[1] = (kern_port_t)IOGetKernPort(evs_port);
	notify_kern_port = IOGetKernPort(notify_port);
#endif	KERNEL
	krtn = port_set_allocate(task_self(), &ev_port_set);
	if(krtn) {
		xpr_err("adbInit: port_set_allocate returned %d\n", 
			krtn, 2,3,4,5);
		return nil;
	}
	krtn = port_set_add(task_self(), ev_port_set, ev_port);
	if(krtn) {
		xpr_err("Ev init: port_set_add returned %d\n", krtn, 2,3,4,5);
		return nil;
	}
	krtn = port_set_add(task_self(), ev_port_set, evs_port);
	if(krtn) {
		xpr_err("Ev init: port_set_add returned %d\n", krtn, 2,3,4,5);
		return nil;
	}
	krtn = port_set_add(task_self(), ev_port_set, notify_port);
	if(krtn) {
		xpr_err("Ev init: port_set_add returned %d\n", krtn, 2,3,4,5);
		return nil;
	}
	
	/*
	 * Initialize the eventSrc list.
	 */
	queue_init(&eventSrcList);

	/*
	 * Have IODevice do its thing.
	 */
	[super init];

	/* A few details to be set up... */
	pointerLoc.x = INIT_CURSOR_X;
	pointerLoc.y = INIT_CURSOR_Y;

	/*
	 * Start up the I/O thread and wait for it to finish 
	 * initialization via an AIO_PING command.
	 */
	thread = IOForkThread((IOThreadFunc)EventListener, self);
	(void) IOSetThreadPolicy(thread, POLICY_FIXEDPRI);
	(void) IOSetThreadPriority(thread, 28);	/* XXX */

	if ( ! hasRegistered )
	{
		[self registerDevice];
		hasRegistered = YES;
	}
	return self;
}

/*
 * Free locally allocated resources, and then ourselves.
 */
- free
{
	/* Initiates a normal close if open */
	[self evClose:ev_port token:eventPort];

	/*
	 * Destroy the ports and port set listenerThread is using.
	 * This will cause it to return from msg_receive() with an
	 * error.  It should take this as a clue to exit.
	 */
	port_deallocate(task_self(), ev_port);
	port_deallocate(task_self(), evs_port);
	port_deallocate(task_self(), notify_port);
	port_set_deallocate(task_self(), ev_port_set);

	/* Release locally allocated resources */
	[eventSrcListLock free];
	[driverLock free];
	return [super free];
}

/*
 * Open the driver for business.  This call must be made before
 * any other calls to the Event driver.  We can only be opened by
 * one user at a time.
 */
- (IOReturn)evOpen:(port_t)dev_port token:(port_t)event_port
{
	IOReturn r = IO_R_SUCCESS;

	if ( dev_port != ev_port )
		return IO_R_INVALID_ARG;

	[driverLock lock];
	
	if ( evOpenCalled == YES )
	{
		r = IO_R_BUSY;
		goto done;
	}
	evOpenCalled = YES;

	if (!evInitialized)
	{
	    evInitialized = YES;
	    curBright = EV_SCREEN_MAX_BRIGHTNESS; // FIXME: Set from NVRAM?
	    curVolume = EV_AUDIO_MAX_VOLUME / 2; // FIXME: Set from NVRAM?
	    // Put code here that is to run on the first open ONLY.
	}

	[self setEventPort:event_port];
	// Init local state
	// IODDMMasks[XPR_EVENTDRIVER_INDEX] |= XPR_EVSRC;
	// IODDMMasks[XPR_IODEVICE_INDEX] |= XPR_ADB;
done:
	[driverLock unlock];
	return r;
}

- (IOReturn)evClose:(port_t)dev_port token:(port_t)event_port
{
	[driverLock lock];
	if ( evOpenCalled == NO || event_port != eventPort )
	{
		[driverLock unlock];
		return IO_R_INVALID_ARG;
	}
	// Early close actions here
	[self forceAutoDimState:NO];
	[self hideCursor];

	[driverLock unlock];

	// Release the input devices.
	[self detachEventSources];

	// Tear down the shared memory area if set up
	if ( eventsOpen == YES )
	    [self unmapEventShmem:eventPort];

	[driverLock lock];
	// Clear screens registry and related data
	if ( evScreen != (void *)0 )
	{
	    IOFree( (void *)evScreen, evScreenSize );
	    evScreen = (void *)0;
	    evScreenSize = 0;
	    screens = 0;
	    lastShmemPtr = (void *)0;
	}
	// Remove port notification for the eventPort and clear the port out
	[self setEventPort:PORT_NULL];

	// Clear local state to shutdown
	evOpenCalled = NO;
	[driverLock unlock];

	return IO_R_SUCCESS;
}

- (IOReturn)evFrameBufferDevicePort:(port_t)event_port
		  unitName:(IOString)name
		  unitClass:(IOString)class
		  unitPort:(port_t *)port
{
	id instance;
	IOReturn r;

	*port = PORT_NULL;
	if ( evOpenCalled == NO || event_port != eventPort )
		return IO_R_INVALID_ARG;

	if ( (r = IOGetObjectForDeviceName( name, &instance )) != IO_R_SUCCESS )
	{	// Not checked in yet.  Lookup class and force a probe
		if ( (instance = objc_getClass(class)) == nil )
		    return r;
		if ( [instance respondsTo:@selector(probe)] == NO )
		    return r;
		if ( (instance = [instance probe]) == nil )
		    return r;
	}

	if ( [instance respondsTo:@selector(devicePort)] == NO )
		return IO_R_PRIVILEGE;

	*port = [instance devicePort];
	return IO_R_SUCCESS;
}

/*
 * General get/set parameter methods for use with the
 * event system.  These replace the old evs ioctl calls.
 *
 *	We could wind up not needing any of these, in which case we should
 *	toss them out and let inheritance take care of the messages.
 */
- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out
{
	IOReturn r = IO_R_INVALID_ARG;
	IOReturn retval;
	attachedEventSrc *device;
	id	srcInstance;
	ns_time_t	nst;
	unsigned 	maxCount = *count;
	unsigned	returnedCount = 0;
	
	// Report the ID for the last left/right button down event
	if (   strcmp( parameterName, EVIOEVNUM ) == 0 )
	{
	    if ( maxCount >= EVIOEVNUM_SIZE )
	    {
		returnedCount = EVIOEVNUM_SIZE;
		[driverLock lock];
		parameterArray[EVIOEVNUM_LEFT] = leftENum;
		parameterArray[EVIOEVNUM_RIGHT] = rightENum;
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if ( strcmp( parameterName, EVIOSHMEMSIZE ) == 0 )
	{
	    if ( maxCount >= EVIOSHMEMSIZE_SIZE )
	    {
		returnedCount = EVIOSHMEMSIZE_SIZE;
		parameterArray[0] = shmem_size;
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOCWINFO ) == 0 )
	{
	    if ( maxCount >= EVSIOCWINFO_SIZE )
	    {
		returnedCount = EVSIOCWINFO_SIZE;
		[driverLock lock];
		if ( eventsOpen == YES )
		    nst = EV_TICK_TO_NS(((EvGlobals*)evg)->waitThreshold);
		else
		    nst = 0ULL;
		nsecs_to_packed_ns(&nst,&parameterArray[EVSIOCWINFO_THRESH]);
		nsecs_to_packed_ns(&waitSustain,
				&parameterArray[EVSIOCWINFO_SUSTAIN]);
		nsecs_to_packed_ns(&waitFrameRate,
				&parameterArray[EVSIOCWINFO_FINTERVAL]);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIO_DCTLINFO ) == 0 )
	{
	    if ( maxCount >= EVSIO_DCTLINFO_SIZE )
	    {
		returnedCount = EVSIO_DCTLINFO_SIZE;
		[driverLock lock];
		parameterArray[EVSIO_DCTLINFO_BRIGHT] =
			[self brightness];
		// EVSIO_DCTLINFO_ATTEN obsolete once libc-67 is released
		parameterArray[EVSIO_DCTLINFO_ATTEN] = curVolume;
		parameterArray[EVSIO_DCTLINFO_AUTODIMBRIGHT] =
			[self autoDimBrightness];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOCCT ) == 0 )
	{
	    if ( maxCount >= EVSIOCCT_SIZE )
	    {
		returnedCount = EVSIOCCT_SIZE;
		[driverLock lock];
		nst = EV_TICK_TO_NS(clickTimeThresh);
		nsecs_to_packed_ns(&nst,&parameterArray[0]);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOCADT ) == 0 )
	{
	    if ( maxCount >= EVSIOCADT_SIZE )
	    {
		returnedCount = EVSIOCADT_SIZE;
		[driverLock lock];
		nst = EV_TICK_TO_NS(autoDimPeriod);
		nsecs_to_packed_ns(&nst,&parameterArray[0]);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOGDADT ) == 0 )
	{
	    if ( maxCount >= EVSIOGDADT_SIZE )
	    {
		returnedCount = EVSIOGDADT_SIZE;
		[driverLock lock];
		if ( eventsOpen == YES )
		{
		    if ( autoDimmed )
			nst = EV_TICK_TO_NS(0);
		    else
			nst = EV_TICK_TO_NS(autoDimTime -
					((EvGlobals*)evg)->VertRetraceClock);
		}
		else
		    nst = EV_TICK_TO_NS(autoDimPeriod);
		nsecs_to_packed_ns(&nst,&parameterArray[0]);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	// added april 7, 1994 EK - to fix bug 41768
	else if (   strcmp( parameterName, EVSIOIDLE ) == 0 )
	{
	    if (maxCount >= EVSIOIDLE_SIZE)
	    {
		returnedCount = EVSIOIDLE_SIZE;
		[driverLock lock];
		if (eventsOpen == YES)
		{
		    if (autoDimmed)
			nst = EV_TICK_TO_NS(((EvGlobals*)evg)->VertRetraceClock 
					- (autoDimTime - autoDimPeriod));
		    else
			nst = EV_TICK_TO_NS(autoDimPeriod - (autoDimTime -
					((EvGlobals*)evg)->VertRetraceClock));
		}
		else
		    nst = EV_TICK_TO_NS(0); // user is active
		nsecs_to_packed_ns(&nst,&parameterArray[0]);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOCCS ) == 0 )
	{
	    if ( maxCount >= EVSIOCCS_SIZE )
	    {
		returnedCount = EVSIOCCS_SIZE;
		[driverLock lock];
		parameterArray[EVSIOCCS_X] = clickSpaceThresh.x;
		parameterArray[EVSIOCCS_Y] = clickSpaceThresh.y;
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOCADS ) == 0 )
	{
	    if ( maxCount >= EVSIOCADS_SIZE )
	    {
		returnedCount = EVSIOCADS_SIZE;
		[driverLock lock];
		parameterArray[0] = autoDimmed;
		[driverLock unlock];
		r = IO_R_SUCCESS;
	    }
	}
	else if (   strcmp( parameterName, EVSIOINFO ) == 0 )
	{
	    NXEventSystemDevice dp;
	    unsigned int cnt;

	    [eventSrcListLock lock];
	    device = (attachedEventSrc *)queue_first(&eventSrcList);
	    while( ! queue_end(&eventSrcList, (queue_t)device)
		  && maxCount >= (sizeof(NXEventSystemDevice) / sizeof(int)) )
	    {
		srcInstance = device->info.eventSrc;
		device = (attachedEventSrc *)device->link.next;
		cnt = 0;
		retval = [srcInstance 
			    getIntValues:&parameterArray[returnedCount]
			   forParameter : parameterName
			          count : &cnt];
		if ( retval == IO_R_SUCCESS )
		{
			maxCount -= cnt;
			returnedCount += cnt;
		}
	    }
	    [eventSrcListLock unlock];
	    r = IO_R_SUCCESS;

	}
	else
	{
	    // Try sending the operation out to the attached
	    // event sources.
	    returnedCount = *count;
	    [eventSrcListLock lock];
	    device = (attachedEventSrc *)queue_first(&eventSrcList);
	    while(!queue_end(&eventSrcList, (queue_t)device))
	    {
		srcInstance = device->info.eventSrc;
		device = (attachedEventSrc *)device->link.next;
		retval = [srcInstance getIntValues:parameterArray
				    forParameter:parameterName
				    count:&returnedCount];
		if ( retval != IO_R_INVALID_ARG )
		{
		    r = retval;
		    break;
		}
	    }
	    [eventSrcListLock unlock];

	    if ( r == IO_R_INVALID_ARG )
	    {
		r = [super getIntValues:parameterArray
			   forParameter : parameterName
			          count : &returnedCount];
	    }
	}
	*count = returnedCount;
	return r;
}

- (IOReturn)getCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count
{
	IOReturn r = IO_R_INVALID_ARG;
	IOReturn retval;
	attachedEventSrc *device;
	id	srcInstance;
	unsigned maxCount = *count;
	unsigned returnedCount = 0;
	
	if ( 0 )
	{
	}
	else
	{
		// Try sending the operation out to the attached
		// event sources.
		returnedCount = *count;
		[eventSrcListLock lock];
		device = (attachedEventSrc *)queue_first(&eventSrcList);
		while(!queue_end(&eventSrcList, (queue_t)device))
		{
			srcInstance = device->info.eventSrc;
			device = (attachedEventSrc *)device->link.next;
			retval = [srcInstance getCharValues:parameterArray
					    forParameter:parameterName
					    count:&returnedCount];
			if ( retval != IO_R_INVALID_ARG )
			{
				r = retval;
				break;
			}
		}
		[eventSrcListLock unlock];

		if ( r == IO_R_INVALID_ARG )
		{
			r = [super getCharValues:parameterArray
			   forParameter : parameterName
			          count : &returnedCount];
		}
	}
	*count = returnedCount;
	return r;
}

		
- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;
{
	IOReturn r = IO_R_INVALID_ARG;
	IOReturn retval;
	attachedEventSrc *device;
	id	srcInstance;
	Point		p;
	_NX_packed_event_t event;
	ns_time_t nst;

	if ( strcmp( parameterName, EVIOSETSCREEN ) == 0 )
	{
		if ( count == EVIOSETSCREEN_SIZE )
		    r = [self evSetScreen:parameterArray];
	}
	else if ( strcmp( parameterName, EVIOST ) == 0 )
	{
		[self startCursor];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVIOSM ) == 0 )
	{
		if ( count == EVIOSM_SIZE )
		{
			p.x = parameterArray[EVIOSM_LOC_X];
			p.y = parameterArray[EVIOSM_LOC_Y];
			[driverLock lock];
			[self setCursorPosition:&p];
			[driverLock unlock];
			r = IO_R_SUCCESS;
		}
	}
	else if ( strcmp( parameterName, EVSIOSWT ) == 0 )
	{
		packed_nsecs_to_nsecs(parameterArray, &nst);		
		[driverLock lock];
		if ( eventsOpen )
			((EvGlobals*)evg)->waitThreshold = EV_NS_TO_TICK(nst);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSWS ) == 0 )
	{
		[driverLock lock];
		packed_nsecs_to_nsecs(parameterArray, &waitSustain);		
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSWFI ) == 0 )
	{
		[driverLock lock];
		packed_nsecs_to_nsecs(parameterArray, &waitFrameRate);		
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSB ) == 0 )
	{
		[driverLock lock];
		[self setBrightness:parameterArray[0]];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSA ) == 0 )
	{	// Obsolete once libc-67 is released
		[driverLock lock];
		[self setUserAudioVolume:parameterArray[0]];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSADB ) == 0 )
	{
		[driverLock lock];
		[self setAutoDimBrightness:parameterArray[0]];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSCT ) == 0 )
	{
		packed_nsecs_to_nsecs(parameterArray, &nst);		
		[driverLock lock];
		clickTimeThresh = EV_NS_TO_TICK(nst);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSCS ) == 0 )
	{
		[driverLock lock];
		clickSpaceThresh.x = parameterArray[EVSIOSCS_X];
		clickSpaceThresh.y = parameterArray[EVSIOSCS_Y];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSADT ) == 0 )
	{
		packed_nsecs_to_nsecs(parameterArray, &nst);		
		[driverLock lock];
		autoDimTime = autoDimTime - autoDimPeriod + EV_NS_TO_TICK(nst);
		autoDimPeriod = EV_NS_TO_TICK(nst);
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIOSADS ) == 0 )
	{
		[driverLock lock];
		[self forceAutoDimState:parameterArray[0]];
		[driverLock unlock];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIORMS ) == 0 )
	{
		[self _resetMouseParameters];
		r = IO_R_SUCCESS;
	}
	else if ( strcmp( parameterName, EVSIORKBD ) == 0 )
	{
		[self _resetKeyboardParameters];
		r = IO_R_SUCCESS;
	}
	else if (  strcmp( parameterName, EVIOLLPE ) == 0 
		|| strcmp( parameterName, EVIOPTRLLPE ) == 0 )
	{
		if ( count == EVIOLLPE_SIZE )
		{
			p.x = parameterArray[EVIOLLPE_LOC_X];
			p.y = parameterArray[EVIOLLPE_LOC_Y];
			event.idata[0] = parameterArray[EVIOLLPE_DATA0];
			event.idata[1] = parameterArray[EVIOLLPE_DATA1];
			event.idata[2] = parameterArray[EVIOLLPE_DATA2];
			[driverLock lock];
			if ( strcmp( parameterName, EVIOPTRLLPE ) == 0 )
				[self setCursorPosition:&p];
			[self postEvent:parameterArray[EVIOLLPE_TYPE]
				at:&p
				atTime:EvTickTimeValue()
				withData:&event.data];		
			[driverLock unlock];
			r = IO_R_SUCCESS;
		}
	}
	else
	{
		// Try sending the operation out to the attached
		// event sources.
		[eventSrcListLock lock];
		device = (attachedEventSrc *)queue_first(&eventSrcList);
		while(!queue_end(&eventSrcList, (queue_t)device))
		{
			srcInstance = device->info.eventSrc;
			device = (attachedEventSrc *)device->link.next;
			retval = [srcInstance setIntValues:parameterArray
					    forParameter:parameterName
					    count:count];
			if ( retval != IO_R_INVALID_ARG )
				r = retval;
		}
		[eventSrcListLock unlock];

		// Nobody wants it?  Kick the message upstairs.
		if ( r == IO_R_INVALID_ARG )
		{
			r = [super setIntValues:parameterArray
					    forParameter:parameterName
					    count:count];
		}
	}
	return r;
}

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;
{
	IOReturn r = IO_R_INVALID_ARG;
	IOReturn retval;
	attachedEventSrc *device;
	id	srcInstance;

	if ( 0 )
	{
	}
	else
	{
		// Try sending the operation out to the attached
		// event sources.
		[eventSrcListLock lock];
		device = (attachedEventSrc *)queue_first(&eventSrcList);
		while(!queue_end(&eventSrcList, (queue_t)device))
		{
			srcInstance = device->info.eventSrc;
			device = (attachedEventSrc *)device->link.next;
			retval = [srcInstance setCharValues:parameterArray
						forParameter:parameterName
					        count:count];
			if ( retval != IO_R_INVALID_ARG )
				r = retval;
		}
		[eventSrcListLock unlock];

		if ( r == IO_R_INVALID_ARG )
		{
			r = [super setCharValues: parameterArray
					forParameter:parameterName
				        count:count];
		}
	}
	return r;
}

//
// Reset instance variables to their default state for mice/pointers
//
- _resetMouseParameters
{
	attachedEventSrc *device;
	id	srcInstance;
	unsigned int parameterArray[EVSIORMS_SIZE];

	[driverLock lock];
	if ( eventsOpen == NO )
	{
	    [driverLock unlock];
	    return self;
	}
	clickTimeThresh = EV_DCLICKTIME;
	clickSpaceThresh.x = clickSpaceThresh.y = EV_DCLICKSPACE;
	clickTime = -EV_DCLICKTIME;
	clickLoc.x = clickLoc.y = -EV_DCLICKSPACE;
	clickState = 1;
	autoDimTime = ((EvGlobals*)evg)->VertRetraceClock + DAUTODIMPERIOD;
	autoDimPeriod = DAUTODIMPERIOD;
	dimmedBrightness = DDIMBRIGHTNESS;

	[driverLock unlock];
	// Go down the Event Src list looking for devices which respond to
	// a EVSIORMS message.  Ping these.
	[eventSrcListLock lock];
	device = (attachedEventSrc *)queue_first(&eventSrcList);
	while(!queue_end(&eventSrcList, (queue_t)device))
	{
		srcInstance = device->info.eventSrc;
		device = (attachedEventSrc *)device->link.next;
		[srcInstance setIntValues:parameterArray
				    forParameter:EVSIORMS
				    count:EVSIORMS_SIZE];
	}
	[eventSrcListLock unlock];

	return self;
}

- _resetKeyboardParameters
{
	attachedEventSrc *device;
	id	srcInstance;
	unsigned int parameterArray[EVSIORKBD_SIZE];

	// Go down the Event Src list looking for devices which respond to
	// a EVSIORKBD message.  Ping these.
	[eventSrcListLock lock];
	device = (attachedEventSrc *)queue_first(&eventSrcList);
	while(!queue_end(&eventSrcList, (queue_t)device))
	{
		srcInstance = device->info.eventSrc;
		device = (attachedEventSrc *)device->link.next;
		[srcInstance setIntValues:parameterArray
				    forParameter:EVSIORKBD
				    count:EVSIORKBD_SIZE];
	}
	[eventSrcListLock unlock];
	return self;
}

/*
 * Methods exported by the EventDriver.
 *
 *	The screenRegister protocol is used by frame buffer drivers to register
 *	themselves with the Event Driver.  These methods are called in response
 *	to a registerSelf or unregisterSelf message received from the Event
 *	Driver.
 */
/* @protocol screenRegister */

- (int) registerScreen:	(id)instance
		bounds:(Bounds *)bp
		shmem:(void **)addr
		size:(int *)size
{
    EvScreen *esp;

    if ( eventsOpen == NO )
    {
	*addr = (void *)0;
	*size = 0;
	return -1;
    }
    if ( lastShmemPtr == (void *)0 )
	lastShmemPtr = evs;
    
    /* shmemSize and bounds already set */
    esp = &((EvScreen*)evScreen)[screens];
    esp->instance = instance;
    /* If this driver wants private shmem, then set its shmemPtr */
    if (esp->shmemSize)
	esp->shmemPtr = lastShmemPtr;
    lastShmemPtr += esp->shmemSize;
    /* Fill in parameters for the requesting instance */
    *addr = esp->shmemPtr;
    *size = esp->shmemSize;
    bcopy( (char *)&esp->bounds, (char *)bp, sizeof (Bounds) );
    return(SCREENTOKEN + screens++);
}


- (void) unregisterScreen:(int)index
{
    int i;

    index -= SCREENTOKEN;

    [driverLock lock];
    if ( eventsOpen == NO || index < 0 || index >= screens )
    {
	[driverLock unlock];
	return;
    }
    [self hideCursor];
    // clear the state for the screen
    ((EvScreen*)evScreen)[index].instance = nil;
    // Put the cursor someplace reasonable if it was on the destroyed screen
    if ( currentScreen == index )	// Uh oh...
    {
	for ( i = screens; --i != -1; )	// Pick a new currentScreen
	{
	    if ( ((EvScreen*)evScreen)[i].instance != nil )
	    {
		currentScreen = i;
		break;
	    }
	}
	// This will jump the cursor back on screen
	[self setCursorPosition:(Point *)&((EvGlobals*)evg)->cursorLoc];
    }
    else
	[self showCursor];
    [driverLock unlock];
    return;
}

/* @end screenRegister */

/* Private methods specific to this driver */	 

#if KERNEL
//
// Allocate a private array of EvScreen structures based on the screen count
// passed in by PostScript and copy in each screen's bounds and shmemSize.
// Also calculate the total size of shared memory and return it to PostScript.
// PostScript will later call mapEventShmem to map in the page(s) at which
// point the ev driver will fill out the EvOffsets structure partly
// based on each driver's shmemSize.  NOTE: Can't access evg pointer yet!
//

- (IOReturn)evSetScreen:(unsigned int *)parameterArray
{
    int i = parameterArray[EVIOSETSCREEN_INDEX];
    EvScreen * screen;

    if ( evOpenCalled == NO )	// Try to screen out cruft...
	return IO_R_PRIVILEGE;

    if (!evScreen) {
	/* if first time through, allocate screen array */
	evScreenSize = sizeof(EvScreen)
			* parameterArray[EVIOSETSCREEN_TOTALSCREENS];
	evScreen = (void *) IOMalloc(evScreenSize);
	bzero(evScreen, evScreenSize);
	/* The following initial shmem size can change in the kernel if
	 * more space is required.  This lets the kernel shmem structure
	 * expand if needed without breaking PostScript.
	 */
	shmem_size = sizeof(EvGlobals) + sizeof(EvOffsets);
	// Set up screen registration variables
	lastShmemPtr = (void *)0;
	screens = 0;
	workSpace.minx = workSpace.miny = workSpace.maxx = workSpace.maxy = 0;
    }
    if ( i < 0 || i >= (evScreenSize / sizeof(EvScreen)) )	// Sanity check
	return IO_R_INVALID_ARG;
    screen = &((EvScreen*)evScreen)[i];
    screen->bounds.minx = parameterArray[EVIOSETSCREEN_MINX];
    screen->bounds.maxx = parameterArray[EVIOSETSCREEN_MAXX];
    screen->bounds.miny = parameterArray[EVIOSETSCREEN_MINY];
    screen->bounds.maxy = parameterArray[EVIOSETSCREEN_MAXY];
    screen->shmemSize = parameterArray[EVIOSETSCREEN_SHMEMSIZE];
    shmem_size += parameterArray[EVIOSETSCREEN_SHMEMSIZE];
    // Update our idea of workSpace bounds
    if ( screen->bounds.minx < workSpace.minx )
	workSpace.minx = screen->bounds.minx;
    if ( screen->bounds.miny < workSpace.miny )
	workSpace.miny = screen->bounds.miny;
    if ( screen->bounds.maxx < workSpace.maxx )
	workSpace.maxx = screen->bounds.maxx;
    if ( screen->bounds.maxy < workSpace.maxy )
	workSpace.maxy = screen->bounds.maxy;
    return IO_R_SUCCESS;
}

/* Member of EventClient protocol 
 *
 * Absolute position input devices and some specialized output devices
 * may need to know the bounding rectangle for all attached displays.
 * The following method returns a Bounds* for the workspace.  Please note
 * that the bounds are kept as signed values, and that on a multi-display
 * system the minx and miny values may very well be negative.
 */
- (Bounds *)workspaceBounds
{
	return &workSpace;
}

/*
 * Set up the shared memory area between the Window Server and the kernel.
 *
 *	Obtain page aligned wired kernel memory for 'size' bytes using
 *	kmem_alloc().
 *	Find a similar sized region in the Window Server task VM map using
 *	vm_map_find().  This function will find an appropriately sized region,
 *	create a memory object, and insert it in the VM map.
 *	For each physical page in the kernel's wired memory we got from
 *	kmem_alloc(), enter that page at the appropriate location in the page
 *	map for the Window Server, in the address range we allocated using
 *	vm_map_find().  
 */
- (IOReturn) mapEventShmem : (port_t) event_port
		   task : (port_t)task			// in
		   size : (vm_size_t)size		// in
		   at : (vm_offset_t *)addr		// out
{
	vm_offset_t	off;
	vm_offset_t	phys_addr;
	IOReturn	krtn;
	void *		task_map;
	vm_offset_t	task_addr;
	
	if ( event_port != eventPort || evOpenCalled == NO )
	    return IO_R_PRIVILEGE;
	if ( task == PORT_NULL || size == 0 )	// malformed request
	    return IO_R_INVALID_ARG;
	if ( owner_task != PORT_NULL || owner != NULL )
	    return IO_R_INVALID_ARG;	// Mapping set up already

	krtn = createEventShmem(task,size,&task_map,&task_addr,&shmem_addr);
	if ( krtn != KERN_SUCCESS )
	{
	    IOLog("%s: createEventShmem fails (%d).\n",[self name],krtn);
	    return krtn;
	}

	[driverLock lock];
	shmem_size = size;
	owner_task = task;
	owner_addr = task_addr;
	*addr = task_addr;
	owner = task_map;
	[self initShmem];
	[driverLock unlock];

	[self _resetMouseParameters];
	[self _resetKeyboardParameters];
	// Start the cursor control callouts
	[driverLock lock];
	[self scheduleNextPeriodicEvent];
	[driverLock unlock];

	return IO_R_SUCCESS;
}

// 
// Unmap the shared memory area and release the wired memory.
//
- (IOReturn) unmapEventShmem : (port_t)event_port;
{
	vm_offset_t off;
	IOReturn r;

	// Since the shared memory area is being torn down, set eventsOpen
	// to NO to keep the cursor thread from futzing with the shared area.
	// We need to implement a lock to guard the shmem, and acquire the
	// lock before tearing the area down.
	xpr_ev_shmemlock("unmapEventShmem: will lock %x\n",
			driverLock, 3, 4, 5, 6);
	[driverLock lock];
	xpr_ev_shmemlock("unmapEventShmem: did lock %x\n",
			driverLock, 3, 4, 5, 6);

	if (event_port != eventPort || evOpenCalled == NO || eventsOpen == NO)
	{
	    [driverLock unlock];
	    return IO_R_PRIVILEGE;
	}
	eventsOpen = NO;

	r=destroyEventShmem(owner_task,owner,shmem_size,owner_addr,shmem_addr);
	if ( r != KERN_SUCCESS )
	{
	    IOLog("%s: destroyEventShmem fails (%d).\n", [self name], r);
	}
	shmem_addr = owner_addr = (vm_offset_t)0;
	shmem_size = 0;
	owner = NULL;
	owner_task = PORT_NULL;
	[driverLock unlock];
	xpr_ev_shmemlock("unmapEventShmem: did unlock %x\n",
			driverLock, 3, 4, 5, 6);
	return r;
}
#endif

// Initialize the shared memory area.
//
// On entry, the driverLock should be set.
- initShmem
{
	int		i;
	EvOffsets	*eop;
	EvGlobals	*glob;

	pointerLoc.x = INIT_CURSOR_X;
	pointerLoc.y = INIT_CURSOR_Y;

	/* top of sharedMem is EvOffsets structure */
	eop = (EvOffsets *) shmem_addr;
	
	/* fill in EvOffsets structure */
	eop->evGlobalsOffset = sizeof(EvOffsets);
	eop->evShmemOffset = eop->evGlobalsOffset + sizeof(EvGlobals);
    
	/* find pointers to start of globals and private shmem region */
	glob = (EvGlobals *) ((char *)shmem_addr + eop->evGlobalsOffset);
	evs = (void *)((char *)shmem_addr + eop->evShmemOffset);
    
	/* Set default wait cursor parameters */
	glob->waitCursorEnabled = TRUE;
	glob->globalWaitCursorEnabled = TRUE;
	glob->waitThreshold = EV_NS_TO_TICK(DefaultWCThreshold);
	waitFrameRate = DefaultWCFrameRate;
	waitSustain = DefaultWCSustain;	
	waitSusTime = 0ULL;
	waitFrameTime = 0ULL;

	/* Set up low-level queues */
	lleqSize = LLEQSIZE;
	for (i=lleqSize; --i != -1; ) {
	    glob->lleq[i].event.type = 0;
	    glob->lleq[i].event.time = 0;
	    glob->lleq[i].event.flags = 0;
	    ev_init_lock(&glob->lleq[i].sema);
	    glob->lleq[i].next = i+1;
	}
	glob->LLELast = 0;
	glob->lleq[lleqSize-1].next = 0;
	glob->LLEHead =
	    glob->lleq[glob->LLELast].next;
	glob->LLETail =
	    glob->lleq[glob->LLELast].next;
	glob->buttons = 0;
	glob->eNum = INITEVENTNUM;
	glob->eventFlags = 0;
	glob->VertRetraceClock = EvTickTimeValue();
	glob->cursorLoc = pointerLoc;
	glob->dontCoalesce = 0;
	glob->dontWantCoalesce = 0;
	glob->wantPressure = 0;
	glob->wantPrecision = 0;
	glob->mouseRectValid = 0;
	glob->movedMask = 0;
	ev_init_lock( &glob->cursorSema );
	ev_init_lock( &glob->waitCursorSema );
	evg = (void *)glob;
	// Set eventsOpen last to avoid race conditions.
	eventsOpen = YES;
	
	return self;
}

//
// Set the event port.  The event port is both an ownership token
// and a live port we hold send rights on.  The port is owned by our client,
// the WindowServer.  We arrange to be notified on a port death so that
// we can tear down any active resources set up during this session.
// An argument of PORT_NULL will cause us to forget any port death
// notification that's set up.
//
// The driverLock should be held on entry.
//
- setEventPort:(port_t)port
{
	static struct _eventMsg init_msg =
			{ { 0, 1, sizeof(msg_header_t)+sizeof(msg_type_t), 
			   MSG_TYPE_NORMAL, (port_t)0, (port_t)0, 0 },
		          { MSG_TYPE_UNSTRUCTURED, 0, 0, 1, 0, 0 } };
	if ( port == PORT_NULL )
	{
		event_kern_port = (port_t)KERN_PORT_NULL;
	}
	else if ( port != eventPort )	// Set up a new notification
	{
		event_kern_port = IOGetKernPort(port);
		port_request_notification((kern_port_t)event_kern_port, 
			(kern_port_t)notify_kern_port);
	}
	if ( eventMsg == NULL )
		eventMsg = IOMalloc( sizeof (struct _eventMsg) );
	eventPort = port;
	// Initialize the events available message.
	*((struct _eventMsg *)eventMsg) = init_msg;

	((struct _eventMsg *)eventMsg)->h.msg_remote_port = port;
	return self;
}

//
// Set the port to be used for a special key notification.  This could be more
// robust about letting ports be set...
//
- (IOReturn)	setSpecialKeyPort	: (port_t)dev_port
				keyFlavor	: (int)special_key
				keyPort		: (port_t)key_port
{
	if ( dev_port != ev_port )
	    return IO_R_PRIVILEGE;
	
	if ( special_key >= 0 && special_key < NX_NUM_SCANNED_SPECIALKEYS )
		specialKeyPort[special_key] = key_port;
	return IO_R_SUCCESS;
}

- (port_t)specialKeyPort: (int)special_key
{
	if ( special_key >= 0 && special_key < NX_NUM_SCANNED_SPECIALKEYS )
		return specialKeyPort[special_key];
	return PORT_NULL;
}

//	Return ports used for Mach interface
- (port_t)ev_port
{ 
	return ev_port;
}

- (port_t)evs_port
{
	return evs_port;
}

//
// Dispatch mechanism for special key press.  If a port has been registered,
// a message is built to be sent out to that port notifying that the key has
// changed state.  A level in the range 0-64 is provided for convenience.
//
- evSpecialKeyMsg:	(unsigned)key
			direction:(unsigned)dir
			flags:(unsigned)f
			level:(unsigned)l
{
	port_t dst_port;
	struct evioSpecialKeyMsg *msg;
	static const struct evioSpecialKeyMsg init_msg =
			{ { 0, 1, sizeof (struct evioSpecialKeyMsg), 
			   MSG_TYPE_NORMAL, (port_t)0, (port_t)0,
			  EV_SPECIAL_KEY_MSG_ID },
		          { MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE, FALSE },
			  0,	/* key */
		          { MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE, FALSE },
			  0,	/* direction */
		          { MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE, FALSE },
			  0,	/* flags */
		          { MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE, FALSE },
			  0	/* level */
			};

	if ( (dst_port = [self specialKeyPort:key]) == PORT_NULL )
		return self;
	msg = (struct evioSpecialKeyMsg *) IOMalloc(
				sizeof (struct evioSpecialKeyMsg) );
	if ( msg == NULL )
		return self;
	
	// Initialize the message.
	bcopy( &init_msg, msg, sizeof (struct evioSpecialKeyMsg) );
	msg->Head.msg_remote_port = dst_port;
	msg->key = key;
	msg->direction = dir;
	msg->flags = f;
	msg->level = l;

	// Send the message out from the I/O thread.
	[self sendIOThreadAsyncMsg	:@selector(_performSpecialKeyMsg:)
			to		:self
			with		:(void *)msg];

	return self;
}

/*
 * This is run in the I/O thread, to perform the actual message send operation.
 */
- _performSpecialKeyMsg:(id)data
{
	kern_return_t r;
	struct evioSpecialKeyMsg *msg;

	msg = (struct evioSpecialKeyMsg *)data;
	xpr_ev_post("_performSpecialKeyMsg 0x%x\n", msg,2,3,4,5);

	r = msg_send( &msg->Head, SEND_TIMEOUT, 0 );	/* Don't block */
	xpr_ev_post("_performSpecialKeyMsg: msg_send() == %d\n",
		r,2,3,4,5);
	if ( r != SEND_SUCCESS )
	{
		IOLog("%s: _performSpecialKeyMsg msg_send returned %d\n",
			[self name], r);
	}
	if ( r == SEND_INVALID_PORT )	/* Invalidate the port */
	{
		[self setSpecialKeyPort		: ev_port
				keyFlavor	: msg->key
				keyPort		: PORT_NULL];
	}
	IOFree( (void *)msg, sizeof (struct evioSpecialKeyMsg) );
	return self;
}

//
// Dispatch state to screens registered with the Event Driver
// Pending state changes for a device may be coalesced.
//
//
// On entry, the  driverLock should be set.
//
- evDispatch:(int)screen command:(EvCmd)evcmd
{
    Point p;
    EvScreen *esp = &((EvScreen*)evScreen)[screen];

    if ( eventsOpen == NO )
	return self;

    p = ((EvGlobals*)evg)->cursorLoc;	// Copy from shmem.
    if ( esp->instance != nil )
    {
	switch ( evcmd )
	{
	    case EVMOVE:
		[esp->instance	moveCursor:&p
				frame:((EvGlobals*)evg)->frame
				token:(screen + SCREENTOKEN)];
		break;

	    case EVSHOW:
		[esp->instance	showCursor:&p
				frame:((EvGlobals*)evg)->frame
				token:(screen + SCREENTOKEN)];
		break;

	    case EVHIDE:
		[esp->instance hideCursor:(screen + SCREENTOKEN)];
		break;

	    case EVLEVEL:
		[esp->instance	setBrightness:[self currentBrightness]
				token:(screen + SCREENTOKEN)];
	}
    }
    return self;
}

//
// Helper functions for postEvent
//
static inline int myAbs(int a) { return(a > 0 ? a : -a); }

static inline short UniqueEventNum(EventDriver * instance)
{
   EvGlobals *evg = (EvGlobals *)instance->evg;
    while (++evg->eNum == NULLEVENTNUM)
	; /* sic */
    return(evg->eNum);
}

// postEvent 
//
// This routine actually places events in the event queue which is in
// the EvGlobals structure.  It is called from all parts of the ev
// driver.
//
// On entry, the driverLock should be set.
//

- postEvent:(int)what
	at:(Point *)location
	atTime:(unsigned)theClock
	withData:(NXEventData *)myData
{
    EvGlobals *glob = (EvGlobals *)evg;
    NXEQElement	*theHead = (NXEQElement *) &glob->lleq[glob->LLEHead];
    NXEQElement	*theLast = (NXEQElement *) &glob->lleq[glob->LLELast];
    NXEQElement	*theTail = (NXEQElement *) &glob->lleq[glob->LLETail];
    int		wereEvents;

    /* Some events affect screen dimming */
    if (EventCodeMask(what) & NX_UNDIMMASK) {
	autoDimTime = theClock + autoDimPeriod;
    	if (autoDimmed)
	    [self undoAutoDim];
    }
    // Update the PS VertRetraceClock off of the timestamp if it looks sane
    if (   theClock > glob->VertRetraceClock
	&& theClock < (glob->VertRetraceClock + (20 * EV_TICK_TIME)) )
	glob->VertRetraceClock = theClock;

    wereEvents = EventsInQueue();

    xpr_ev_post("postEvent: what %d, X %d Y %d Q %d, needKick %d\n",
		what,location->x,location->y,
		EventsInQueue(), needToKickEventConsumer);

    if ((!glob->dontCoalesce)	/* Coalescing enabled */
    && (theHead != theTail)
    && (theLast->event.type == what)
    && (EventCodeMask(what) & COALESCEEVENTMASK)
    && ev_try_lock(&theLast->sema)) {
    /* coalesce events */
	theLast->event.location.x = location->x;
	theLast->event.location.y = location->y;
	theLast->event.time = theClock;
	if (myData != NULL)
	    theLast->event.data = *myData;
	ev_unlock(&theLast->sema);
    } else if (theTail->next != glob->LLEHead) {
	/* store event in tail */
	theTail->event.type = what;
	theTail->event.location.x = location->x;
	theTail->event.location.y = location->y;
	theTail->event.flags = glob->eventFlags;
	theTail->event.time = theClock;
	theTail->event.window = 0;
	if (myData != NULL)
	    theTail->event.data = *myData;
	switch(what) {
	case NX_LMOUSEDOWN:
	    theTail->event.data.mouse.eventNum =
		leftENum = UniqueEventNum(self);
	    break;
	case NX_RMOUSEDOWN:
	    theTail->event.data.mouse.eventNum =
		rightENum = UniqueEventNum(self);
	    break;
	case NX_LMOUSEUP:
	    theTail->event.data.mouse.eventNum = leftENum;
	    leftENum = NULLEVENTNUM;
	    break;
	case NX_RMOUSEUP:
	    theTail->event.data.mouse.eventNum = rightENum;
	    rightENum = NULLEVENTNUM;
	    break;
	}
	if (EventCodeMask(what) & PRESSUREEVENTMASK) {
	    theTail->event.data.mouse.pressure = lastPressure;
	}
	if (EventCodeMask(what) & MOUSEEVENTMASK) { /* Click state */
	    if (((theClock - clickTime) <= clickTimeThresh)
	    && (myAbs(location->x - clickLoc.x) <= clickSpaceThresh.x)
	    && (myAbs(location->y - clickLoc.y) <= clickSpaceThresh.y)) {
		theTail->event.data.mouse.click = 
		    ((what == NX_LMOUSEDOWN)||(what == NX_RMOUSEDOWN)) ?
		    (clickTime=theClock,++clickState) : clickState ;
	    } else if ((what == NX_LMOUSEDOWN)||(what == NX_RMOUSEDOWN)) {
		clickLoc = *location;
		clickTime = theClock;
		clickState = 1;
		theTail->event.data.mouse.click = clickState;
	    } else
		theTail->event.data.mouse.click = 0;
	}
#if PMON
	pmon_log_event(PMON_SOURCE_EV,
		       KP_EV_POST_EVENT,
		       what,
		       glob->eventFlags,
		       theClock);
#endif
	glob->LLETail = theTail->next;
	glob->LLELast = theLast->next;
	if ( ! wereEvents )	// Events available, so wake event consumer
	    [self kickEventConsumer];
    }
    else
    {
	/*
	 * if queue is full, ignore event, too hard to take care of all cases 
	 */
	IOLog("%s: postEvent LLEventQueue overflow.\n", [self name]);
	[self kickEventConsumer];
#if PMON
	pmon_log_event( PMON_SOURCE_EV,
			KP_EV_QUEUE_FULL,
			what,
			glob->eventFlags,
			theClock);
#endif
    }
}

/*
 * - kickEventConsumer
 *
 * 	Try to send a message out to let the event consumer know that
 *	there are now events available for consumption.
 */
- kickEventConsumer
{
	[kickConsumerLock lock];
	xpr_ev_post("kickEventConsumer (need == %d)\n",
		needToKickEventConsumer,2,3,4,5);
	if ( needToKickEventConsumer == YES )
	{
		[kickConsumerLock unlock];
		return self;		// Request is already pending
	}
	needToKickEventConsumer = YES;	// Posting a request now
	[kickConsumerLock unlock];
	[self sendIOThreadAsyncMsg	:@selector(_performKickEventConsumer:)
			to		:self
			with		:(void *)0];

	return self;
}

/*
 * This is run in the I/O thread, to perform the actual message send operation.
 * Note that we perform a non-blocking send.  The Event port in the event
 * consumer has a queue depth of 1 message.  Once the consumer picks up that
 * message, it runs until the event queue is exhausted before trying to read
 * another message.  If a message is pending,there is no need to enqueue a
 * second one.  This also keeps us from blocking the I/O thread in a msg_send
 * which could result in a deadlock if the consumer were to make a call into
 * the event driver.
 */
- _performKickEventConsumer:(id)data
{
	kern_return_t r;

	xpr_ev_post("_performKickEventConsumer\n", 1,2,3,4,5);
	[kickConsumerLock lock];
	needToKickEventConsumer = NO;	// Request received and processed
	[kickConsumerLock unlock];

	r = msg_send( (msg_header_t *)eventMsg, SEND_TIMEOUT, 0 );
	xpr_ev_post("_performKickEventConsumer: msg_send() == %d\n",
		r,2,3,4,5);
	switch ( r )
	{
	    case SEND_TIMED_OUT:	/* Already has a message posted */
	    case SEND_SUCCESS:		/* Message is posted */
		break;
	    default:			/* Log the error */
		IOLog("%s: _performKickEventConsumer msg_send returned %d\n",
			[self name], r);
		break;
	}
	return self;
}

/* 
 * Event sources may need to use an I/O thread from time to time.
 * Rather than have each instance running it's own thread, we provide
 * a callback mechanism to let all the instances share a common Event I/O
 * thread running in the IOTask space, and managed by the Event Driver.
 * Returns self, or nil on error.
 */
- (IOReturn)sendIOThreadMsg:	(SEL)selector	// Selector to call back on
	to		:	(id)instance	// Instance to call back
	with		:	(id)data;	// Data to pass back
{
	evIoOpParams params;

	params.callback.instance = instance;
	params.callback.selector = selector;
	params.callback.data = data;

	return [self	_threadOpCommon : EVENT_LISTENER_CALLBACK
			opParams : (void *)&params
			async : NO];
}

- sendIOThreadAsyncMsg:		(SEL)selector	// Selector to call back on
	to		:	(id)instance	// Instance to call back
	with		:	(id)data;	// Data to pass back
{
	evIoOpParams params;

	params.callback.instance = instance;
	params.callback.selector = selector;
	params.callback.data = data;

	[self	_threadOpCommon : EVENT_LISTENER_CALLBACK
		opParams : (void *)&params
		async : YES];
	return self;
}

/*
 * This routine is run within the I/O thread, on demand from the
 * sendIOThreadMsg::: methods above.  We attempt to dispatch a message
 * to the specified selector and instance.
 */
- (IOReturn) _doPerformInIOThread:(void *)data
{
	IOReturn ret;
	evCallback *msg = data;
	if ( [msg->instance respondsTo:msg->selector] )
	{
	    [msg->instance perform:msg->selector with:msg->data];
	    ret = IO_R_SUCCESS;
	}
	else
	{
	    IOLog("%s: _doPerformInIOThread: [%s] does not respond to SEL [%s]\n",
			[self name],
			object_getClassName(msg->instance),
			sel_getName(msg->selector) );
	    ret = IO_R_IPC_FAILURE;
	}
	return ret;
}

/*
 * Common thread dispatch code used to drive
 * I/O operations.  The operation to be performed is encoded
 * in a Mach message, which we send to our I/O thread.  This
 * thread then performs the requested operations, returning a
 * status
 */
- (IOReturn)_threadOpCommon : (int)op
			     opParams : (void *)data
			     async : (BOOL)async
{
	evIoOpParams *	opParams = data;
	evIoOpMsg opMsg;
	kern_return_t krtn;
	IOReturn rtn;
#ifdef KERNEL
	extern kern_port_t ev_port_list[];
#endif
	
	xpr_ev_post("_threadOpCommon: op %d\n",op , 2,3,4,5);
		
	/*
	 * First, an evIoOpMsg.
	 */
	opMsg = opMsgTemplate;
	opMsg.header.msg_local_port = PORT_NULL;
	
	/*
	 * Next, the evIoOpBuf.
	 */
	opMsg.opBuf.op = op;
	if ( async == NO )
	{
	    opMsg.opBuf.cmdLock = [NXConditionLock alloc];
	    [opMsg.opBuf.cmdLock initWith:CMD_INPROGRESS];
	    opMsg.opBuf.status = &rtn;
	    rtn = IO_R_INVALID_ARG;
	}
	opMsg.opBuf.params = *opParams;
	
	/*
	 * Go for it. Send the message to the I/O thread and wait for
	 * I/O complete. We use msg_send_from_kernel becuase this is called
	 * from exported methods; we could be in a user task.
	 */
#ifdef	KERNEL
	opMsg.header.msg_remote_port = (port_t)ev_port_list[0];
	krtn = msg_send_from_kernel(&opMsg.header, MSG_OPTION_NONE, 0);
#else	KERNEL
	opMsg.header.msg_remote_port = ev_port;
	krtn = msg_send(&opMsg.header, MSG_OPTION_NONE, 0);
#endif	KERNEL
	if(krtn) {
		IOLog("%s: _threadOpCommon msg_send returned %d\n",
			[self name], krtn);
		rtn = IO_R_IPC_FAILURE;
		goto out;
	}
	if ( async == NO )
	{
	    [opMsg.opBuf.cmdLock lockWhen:CMD_DONE];	// Wait for completion
	}
	else
	    rtn = IO_R_SUCCESS;
out:
	if ( async == NO )
	    [opMsg.opBuf.cmdLock free];
	xpr_ev_post("_threadOpCommon: status %s\n", 
		[self stringFromReturn:rtn], 2,3,4,5);
	return rtn;
}


/*
 * The following methods are executed from the I/O thread only.
 */

/*
 * Service incoming client evIoOp.
 */
- (void)_ioOpHandler	: (void *)data
{
	IOReturn ret;
	evIoOpBuf *opBuf = data;

	xpr_ev_post("_ioOpHandler: op %d\n",opBuf->op, 2,3,4,5);

	if ( opBuf->status != NULL )
	    *opBuf->status = IO_R_SUCCESS;
	switch(opBuf->op) {
		 
	    case EVENT_LISTENER_PING:
	    	/*
		 * This is just used for the init thread to know when we've
		 * come this far.
		 */
		break;
		
	    case EVENT_LISTENER_EXIT:
	    	/* 
		 * First I/O complete this request, then terminate.
		 */
		[opBuf->cmdLock unlockWith:CMD_DONE];
	    	IOExitThread();

	    case EVENT_LISTENER_CALLBACK:
		ret = [self _doPerformInIOThread:&(opBuf->params.callback)];
		if ( opBuf->status != NULL )
		    *opBuf->status = ret;
		break;

	    default:
	    	IOPanic("EventDriver: Bogus opBuf.op");
	}
	
	/*
	 * Release caller for synchronous operations.
	 */
	if ( opBuf->cmdLock != nil )
	    [opBuf->cmdLock unlockWith:CMD_DONE];
}

/*
 * Listen for and dispatch messages for the ev and evs clients
 * Messages received on these ports are used to control input devices, the
 * display, and the cursor.
 *
 * These ports replace the old /dev/ev0 and /dev/evs0 devices.
 * Programs which use the published event status driver API in
 * Release 3.0 will continue to work.  Programs which directly manipulated
 * /dev/ev0 and /dev/evs0 are out of luck.
 *
 * Message buffers:  The reply buffer is allocated when an RPC is generated,
 * and is freed after we have sent the reply out.  While this may seem
 * expensive, we normally only have between 4 and 12 RPCs in a session, with
 * almost all occuring at WindowServer startup and login time.  IOMalloc()
 * is fast enough to prevent any detectable delay.  The design opts to reduce
 * wired memory at the cost of a few extra lines of code.
 */
static volatile void EventListener(EventDriver *inst)
{
	msg_header_t	*in;
	int		in_size;
	msg_header_t	*out;
	int		out_size;
	msg_return_t	r;
	boolean_t	result;
	boolean_t	ok = TRUE;
	EvInMsg		in_msg;			// Big enough for most input
	extern boolean_t Event_server();
	extern boolean_t EventStatus_server();
	
	in_size = sizeof in_msg;
	in = &in_msg.hdr;
	out_size = 0;
	out = (msg_header_t *)NULL;		// Dynamically alloc as needed

	while ( ok )
	{
	    in->msg_local_port = inst->ev_port_set;
	    in->msg_size = in_size;
	    
	    r = msg_receive( in, RCV_LARGE, 0 );
	    switch( r )
	    {
		case RCV_SUCCESS:
			break;

		case RCV_TOO_LARGE:
			/* If we already grew it, free grown buffer */
		        if ( in_size > (sizeof in_msg) )
			    IOFree( (void *)in, in_size );
			/* Get a new buffer of an appropriate size. */
			in_size = in->msg_size;
			in = (msg_header_t *)IOMalloc( in_size );
			xpr_ev_post(
				"EventListener: msg ID %d insize %d: 0x%x\n",
				in->msg_id, in_size,in,4,5);
			continue;

		case RCV_INVALID_PORT:	/* Driver is being freed */
			ok = FALSE;
			continue;

		default:
			IOLog("%s: error on msg_receive (%d)\n",
				[inst name], r);
			continue;
	    }
	    xpr_ev_post("EventListener: msg ID %d size %d\n",
			in->msg_id, in->msg_size,3,4,5);
	    /*
	     * We have arranged for notification when the WindowServer
	     * dies.  The eventPort, owned by the WindowServer, has been
	     * checked in for port death notification.
	     * In the event of it's death, we assume that the WindowServer
	     * has met an unfortunate fate, and invoke our device close
	     * actions.
	     */
	    if ( in->msg_local_port == inst->notify_port )
	    {
		notification_t *nmsg = (notification_t *)in;
	
		if ( nmsg->notify_port == inst->event_kern_port &&
		     inst->event_kern_port!= (port_t)KERN_PORT_NULL )
		{
		    [inst evClose:inst->ev_port token:inst->eventPort];
#ifdef DEBUG
		    IOLog("%s: client token invalidated\n", [inst name]);
#endif
		}
		continue;
	    }
	    /*
	     * We got a request.  If it's on the privileged ev_port, 
	     * test the message to see if it's a control message for us.
	     * If it's not a control message, try passing it to the
	     * Event_server.
	     */
	    result = FALSE;
	    switch( in->msg_id )
	    {
		case EV_IO_OP_MSG_ID:
		    if ( in->msg_local_port == inst->ev_port )
		    {
			[inst _ioOpHandler:&((evIoOpMsg *)in)->opBuf];
			result = TRUE;
		    }
		    break;

		default:
		    /* A real RPC needs a reply buffer.  Make one. */
		    if ( out == (msg_header_t *)NULL )
		    {
			    out_size = sizeof (EvOutMsg);
			    out = (msg_header_t *)IOMalloc( out_size );
			    xpr_ev_post(
			      "EventListener: msg ID %d replysize %d: 0x%x\n",
					in->msg_id, out_size,out,4,5);
		    }
		    result = Event_server( in, out );
		    break;
	    }

	    if ( result == FALSE )
	    {
		IOLog("%s: invalid message ID %d\n",
			[inst name], in->msg_id );
	    }
	    else	/* result == TRUE */
	    {
		if(in->msg_remote_port!=PORT_NULL && out!=(msg_header_t*)NULL)
		{
		    /*
		     * We were passed a reply port.  Set up and send a
		     * reply message.  If this fails, it's because of
		     * a MiG error.  No big deal. Just log it and try to go on.
		     */
		    if ( out->msg_size > out_size ) /* Mem smasher? panic? */
			IOLog("%s: reply msg overflow (%d > %d)\n",
				[inst name], out->msg_size, out_size);
		    r = msg_send(out, MSG_OPTION_NONE, 0);
		    xpr_ev_post("EventListener: msg ID %d reply stat %d\n",
				in->msg_id, r,3,4,5);
		    if ( r != SEND_SUCCESS )
			IOLog("%s: error on msg_send (%d)\n",
				[inst name], r);
		}
	    }
	    /*
	     * Done with messages.  Free the reply buffer, and shrink the
	     * input buffer as needed.
	     */
	    if ( out != (msg_header_t *)NULL )
	    {
		IOFree( (void *)out, out_size );
		out = (msg_header_t *)NULL;
		out_size = 0;
	    }
	    if ( in_size > (sizeof in_msg) )
	    {
		IOFree( (void *)in, in_size );
		in_size = sizeof in_msg;
		in = &in_msg.hdr;
	    }
	}

	(volatile void) IOExitThread();
}

@end

