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

/*
	File:		uslUtility.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(DF)	David Ferguson
		(DKF)	David Ferguson
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB33>	10/29/98	BT		Close old configurations on reset or reconfiguration.
	 <USB32>	 10/7/98	BT		Fix callback at task time.
	 <USB31>	 10/7/98	BT		Add 32bit endian functions
	 <USB30>	 9/29/98	BT		Use real frame timing
	 <USB29>	 9/28/98	BT		Add device reset function
	 <USB28>	 8/24/98	BT		Eliminate old checkPB version, add new one for Isoc
	 <USB27>	 8/13/98	BT		Add multibus support
	 <USB26>	 8/12/98	BT		Move root hub into UIM again
	 <USB25>	 7/28/98	BT		Dev zero is valid ref
	 <USB24>	 7/10/98	TC		Essentially take out <USB19>.
	 <USB23>	  7/9/98	BT		Clean up queues when device deleted. Merge in branch
	 <USB22>	  7/2/98	BT		Fix dealloc mem to zero buffer ptr.
	 <USB21>	  7/2/98	BT		Use UIM time for internal timing
	 <USB20>	  7/2/98	BT		Return the right framenumber from USBDelay. Let framecount take
									kNoCallBack.
	 <USB19>	 6/30/98	BT		Move Root hub sim into UIM
	 <USB18>	 6/15/98	DF		Remove compiler build problem.  It didn't like passing a pointer
									to a struct pointer that was declared volatile.
	 <USB17>	 6/14/98	DF		Cancel InterruptTimer when being replaced.
	 <USB16>	  6/5/98	BT		DeallocMem will work immediatlty
	 <USB15>	  6/5/98	BT		Use UIM time
	 <USB14>	 4/30/98	BT		Add real interrupt driven timer
	 <USB13>	 4/23/98	BT		Add hub watchdog function
	 <USB12>	 4/16/98	BT		Eliminate debugger
	 <USB11>	 4/10/98	BT		add USB to host word
	 <USB10>	  4/9/98	BT		Use USB.h
		 <9>	  4/8/98	BT		More error checking
		 <8>	  4/2/98	BT		Error checking for nil call backs. Also Ferg's Use DSL Timing
									Services directly
		 <7>	 3/11/98	BT		Give time to root hub int sim
		 <6>	  3/2/98	BT		Back out timer changes
		 <5>	 2/24/98	BT		Change timer to non-persistant
		 <4>	 2/19/98	BT		Make timer non task time.
		 <3>	 2/16/98	BT		Fix calling processdone queue and timer too often.
	  <USB2>	  2/9/98	DKF		add USBIdleTask for handling events that need file I/O safe
									time.
		 <1>	 1/29/98	BT		first checked in
*/

#include "../driverservices.h"
#include "../USB.h"
#include "../USBpriv.h"
#include "uslpriv.h"
#include "../uimpriv.h"
#import <sys/callout.h>
#import <kernserv/ns_timer.h>


static QHdrPtr delayQueue;
static QHdrPtr notifyQueue;
static QHdrPtr memoryQueue;
static unsigned long notifyFlag = 0;
volatile TimerID curTimerID;

static UInt32 idled;
#define ONE_MILLI_SECOND  1000000ULL     /* nanoseconds/10**6 */
void timerHandler(void *p1);  //naga
static SInt32 interruptPriority;


void uslCleanAQueue(QHdrPtr queue, USBDeviceRef ref)
{	// This is assuming its called at secondary interrupt time.
USBPB *pb, *pbNext;

	if(CurrentExecutionLevel() != kSecondaryInterruptLevel)
	{
		USBExpertStatus(0, "USL - clean Q call at wrong time", ref);
		return;
	}
	
	if(queue == nil)
	{
		return;
	}
	pb = (void *)queue->qHead;
	
	while(pb != nil)
	{
		pbNext = (void *)pb->qlink;
		if(isSameDevice(pb->usbReference, ref))
		{
			USBExpertStatus(0, "USL - killing Q element", ref);
			PBDequeue((void *)pb, queue);	// unqueue it an forget it
		}		
		pb = pbNext;
	}
}

void uslCleanDelayQueue(USBDeviceRef ref)
{	// This is assuming its called at secondary interrupt time.
	uslCleanAQueue(delayQueue, ref);
}


void uslCleanNotifyQueue(USBDeviceRef ref)
{	// This is assuming its called at secondary interrupt time.
	uslCleanAQueue(notifyQueue, ref);
}


void uslCleanMemQueue(USBDeviceRef ref)
{	// This is assuming its called at secondary interrupt time.
	uslCleanAQueue(memoryQueue, ref);
}





void uslInterruptPriority(UInt32 delta)
{
	AddAtomic(delta, &interruptPriority);
	if(interruptPriority < 0)
	{
		USBExpertStatus(0, "USL - interrrupt prority gone negative", interruptPriority);
	}
}


OSStatus USBResetDevice(USBPB *pb)
{
//	 --> usbReference	device/interface/endpoint - which device.
usbDevice *dev;
pipe *p;
OSStatus err;

	if(!checkPBVersion(pb, kUSBPowerReset))
	{
		return(pb->usbStatus);
	}

	dev = getDevicePtr(pb->usbReference);
	if(dev == nil)
	{
		err = findPipe(pb->usbReference, &p);
		if(p != nil)
		{
			dev = getDevicePtr(p->devIntfRef);
		}
	}
	if(dev == nil)
	{
		pb->usbStatus = kUSBUnknownDeviceErr;
		return(pb->usbStatus);
	}

	uslUnconfigureDevice(dev);

	pb->usbStatus = kUSBPending;
	pb->usb.hub.Request = kUSBHubPortResetRequest;
	err = USBExpertNotifyParentMsg(dev->ID, pb);
	if(err != noErr)
	{
		pb->usbStatus = err;
		return(err);
	}
	else
	{
		return(kUSBPending);
	}
}


OSStatus USBGetFrameNumberImmediate(USBPB *pb)
{
//	 --> usbReference	device/interface/endpoint. Specifies which bus
//	 --> usbReqCount	size of buffer (should be 0 or sizeof(UInt64)
//	 --> usbBuffer		nil/point to UInt64 structure for full count
//	<--  usbActCount	size of data returned.
//	<--  usbFrame	low 32 bits of current frame number.
union{
	UInt64 u;
	UnsignedWide s;
	}frame;
Boolean nocallBack = false;
UInt32 bus;

	// Oops, forgot to validate this
	pb->usbStatus = validateRef(pb->usbReference, &bus);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	if(pb->usbCompletion == kUSBNoCallBack)
	{
		nocallBack = true;
		pb->usbCompletion = (void *)-2;
	} 
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	if(nocallBack)
	{
		pb->usbCompletion = kUSBNoCallBack;
	}
	frame.u = UIMGetCurrentFrame(bus);
	pb->usbFrame = frame.s.lo;
	if( (pb->usbBuffer != nil) && (pb->usbReqCount != 0) )
	{
		pb->usbActCount = pb->usbReqCount;
		if(pb->usbActCount > sizeof(frame))
		{
			pb->usbActCount = sizeof(frame);
		}
		usb_BlockMoveData(&frame, pb->usbBuffer, pb->usbActCount);
	}
	return(noErr);

}

UInt32 deltaFrames(UInt32 previous, UInt32 bus)
{
union{
	UInt64 u;
	UnsignedWide s;
	}frame;

	frame.u = UIMGetCurrentFrame(bus);
	if(previous > frame.s.lo)
	{
		previous -= frame.s.lo;
		return(~previous+1);
	}
	else
	{
		return(frame.s.lo - previous);
	}
}


void finaliseNotifications(void)
{
	AbsoluteTime	wastedTime;
	
	PBQueueDelete(notifyQueue);
	PBQueueDelete(memoryQueue);
	while (curTimerID) {
		if (CancelTimer(curTimerID, &wastedTime) == noErr)
			curTimerID = 0;
	}
}

void initialiseNotifications(void)
{
	if( (PBQueueCreate(&notifyQueue) != noErr) ||
		(PBQueueInit(notifyQueue) != noErr) )
	{
		/* Panic */
		USBExpertStatus(0,"USL - failed to initialise notification queue", 0);
	}
	if( (PBQueueCreate(&memoryQueue) != noErr) ||
		(PBQueueInit(memoryQueue) != noErr) )
	{
		/* Panic */
		USBExpertStatus(0,"USL - failed to initialise memory queue", 0);
	}

	idled = 0;
	timerHandler(0);

}

static void taskTimeDoMemory(USBPB *pb)
{
Boolean dealloc;
	dealloc = (pb->usbFlags & (1 << kUSLTTMemDeAllocFlagShift)) != 0;
	pb->usbFlags &= ~(1 << kUSLTTMemDeAllocFlagShift);

	if(dealloc)
	{
		pb->usbStatus = PoolDeallocate(pb->usbBuffer);
		pb->usbBuffer = nil;
	}
	else
	{
		pb->usbBuffer = PoolAllocateResident(pb->usbReqCount, true /*Boolean clear*/);
		if(pb->usbBuffer != nil)
		{
			pb->usbActCount = pb->usbReqCount;
			pb->usbStatus = noErr;
		}
		else
		{
			pb->usbActCount = 0;
			pb->usbStatus = memFullErr;	/* How do you find the right error code???? */
		}
	}
	(pb->usbCompletion)(pb);
}

static void NotifyHandlerProc( )
{
USBPB *pb;

	notifyFlag = false;	/* Queue Processed */
	
	/* Note these don't use interrupt priority, they do not happen at int time */
	while(PBDequeueFirst(notifyQueue, (void *)&pb) == noErr)
	{
		pb->usbStatus = noErr;		/* Call back with right error status */
		(pb->usbCompletion)(pb);
	}
	while(PBDequeueFirst(memoryQueue, (void *)&pb) == noErr)
	{
		taskTimeDoMemory(pb);
	}

}

static void startNotifier(void)
{
	notifyFlag = true;
}

static OSStatus uslTTCallBack(USBPB *pb)
{
	pb->usbStatus = PBEnqueueLast((void *)pb, notifyQueue);
	startNotifier();
	return(pb->usbStatus);	/* safe, this is synchronised */
}

static OSStatus uslTTmem(USBPB *pb)
{
	pb->usbStatus = PBEnqueueLast((void *)pb, memoryQueue);
	startNotifier();
	return(pb->usbStatus);	/* safe, this is synchronised */
}

OSStatus uslAllocMem(USBPB *pb)
{
	pb->usbFlags &= ~(1 << kUSLTTMemDeAllocFlagShift);
	return(uslTTmem(pb));
}

OSStatus USBAllocMem(USBPB *pb)
{
	pb->usbStatus = validateRef(pb->usbReference, nil);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	if(!checkPBVersion(pb, kUSBTaskTimeFlag))
	{
		return(pb->usbStatus);
	}

	return(uslAllocMem(pb));
}

OSStatus uslDeallocMem(USBPB *pb)
{
	pb->usbFlags |= (1 << kUSLTTMemDeAllocFlagShift);
	return(uslTTmem(pb));
}

OSStatus USBDeallocMem(USBPB *pb)
{
Boolean nocallBack = false;

	if(pb->usbCompletion == kUSBNoCallBack)
	{
		nocallBack = true;
		pb->usbCompletion = (void *)-2;
	}
	else
	{
		pb->usbStatus = validateRef(pb->usbReference, nil);
		if(pb->usbStatus != noErr)
		{
			return(pb->usbStatus);
		}
	}
	if(!checkPBVersion(pb, kUSBTaskTimeFlag))
	{
		return(pb->usbStatus);
	}
	if(nocallBack)
	{
	void *buffer;
		if(CurrentExecutionLevel() != kTaskLevel)
		{
			return(kUSBCompletionError);
		}
		pb->usbCompletion = kUSBNoCallBack;
		buffer = pb->usbBuffer;
		pb->usbBuffer = nil;	// BT 2Jun98, don't forget to zero this, its in the spec
		return(PoolDeallocate(buffer));
	}
	return(uslDeallocMem(pb));
}

void initialiseDelays(void)
{
	if( (PBQueueCreate(&delayQueue) != noErr) ||
		(PBQueueInit(delayQueue) != noErr) )
	{
		/* Panic */
		USBExpertStatus(0,"USL - failed to initialise delay queue", 0);
	}
}

void finaliseDelays(void)
{
	PBQueueDelete(delayQueue);
}

static OSStatus uslDelay(USBPB *pb, UInt32 bus)
{
OSStatus err = noErr;

	/* waits the specified number of frames then calls the handler */
	/* length (soon to be usbReqCount) - required frame count to delay */
	if( (delayQueue == nil) || (notifyQueue == nil) )
	{
		pb->usbStatus = kUSBInternalErr;
		return(kUSBInternalErr);
	}

	pb->usbActCount = pb->usbReqCount + deltaFrames(0, bus);
	pb->qType = kUSBDelayQType;
	pb->usbStatus = kUSBPending;

	err = PBEnqueueLast((void *)pb, delayQueue);
	uslInterruptPriority(+1);

	if(err != noErr)
	{
		pb->usbStatus = err;
	}
	else
	{
		err = kUSBPending;
	}
	return(err);
}

OSStatus USBDelay(USBPB *pb)
{
OSStatus err = noErr;
UInt32 bus;

	pb->usbStatus = validateRef(pb->usbReference, &bus);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	/* waits the specified number of frames then calls the handler */
	/* length (soon to be usbReqCount) - required frame count to delay */
	if(delayQueue == nil)
	{
		return(kUSBInternalErr);
	}

	if(!checkPBVersion(pb, kUSBTaskTimeFlag))
	{
		return(pb->usbStatus);
	}
	return(uslDelay(pb, bus));
}

static void processDelayQ( Duration currentDuration)
{
USBPB *pb, *pbNext;
UInt32 bus=0;

	if(delayQueue == nil)
	{
		return;
	}
	/* Walk the que, safe, we're the only thing to take elements off */

	pb = (void *)delayQueue->qHead;
	
	while(pb != nil)
	{
		pbNext = (void *)pb->qlink;
	
		if(pb->usbActCount <= currentDuration)
		{
			if(PBDequeue((void *)pb, delayQueue) == noErr)
			{
				if( ( (pb->usbFlags & kUSBTaskTimeFlag) != 0) &&
					(CurrentExecutionLevel() != kTaskLevel) )
				{
					uslTTCallBack(pb);
				}
				else
				{
					uslInterruptPriority(-1);
					pb->usbStatus = noErr;
					(*pb->usbCompletion)(pb);
				}
			}
		}
		
		pb = pbNext;
	}

}

static Boolean uslInterruptSafeIdle(UInt32 currFrame)
{
static UInt32 nextRootHub, lastRootHub;
static UInt32 lastDone = 0;

	if(currFrame != lastDone)	// don't run more than once per millisecond.
	{
		
		lastDone = currFrame;
		//UIMProcessDoneQueue();	/* Note this is simulated only */
		
		if( (currFrame > nextRootHub) || (currFrame < lastRootHub) )
		{
			UIMPollRootHubSim();
			uslHubWatchDog(currFrame);
			lastRootHub = currFrame;
			nextRootHub = currFrame + 50;
		}
		processDelayQ(currFrame);
		return(true);
	}
	return(false);
}

void USBIdleTask(void)
{
UInt32 currentFrame;
	idled = 0;
	
	currentFrame = UIMGetAFrame();

	if(uslInterruptSafeIdle(currentFrame) && notifyFlag)
	{
		NotifyHandlerProc();
	}
	
}


void timerHandler(void *p1)  //naga it used to be void *p1,void *p2)
{
TimerID		timerID;
Duration currentFrame, delay;
AbsoluteTime expirationTime;
AbsoluteTime currentTime	 = UpTime();

	p1 = 0;
//	p2 = 0;
	
	if(idled != 0)
	{
		//USBExpertStatus(0, "USL - Interrupt task idle", idled);
		currentFrame = UIMGetAFrame();
		uslInterruptSafeIdle(currentFrame);
		if(idled > 18)
		{
			idled = 18;
		}
	}
	
	if(interruptPriority > 0)
	{
		//USBExpertStatus(0, "USL - interruptPriority:", interruptPriority);
		delay = 20 - idled;	// milliseconds
		idled++;
	}
	else
	{
		delay = 100;
	}
	expirationTime = AddAbsoluteToAbsolute(currentTime,
						DurationToAbsolute(delay));
//kprintf("***timerhandler:Setting timerHandler delay=%d\n",delay);
ns_timeout(timerHandler,p1,delay*ONE_MILLI_SECOND,CALLOUT_PRI_SOFTINT0);
//	SetInterruptTimer(&expirationTime, timerHandler, nil, &timerID);
	curTimerID = timerID;
	return(0);
}






/* ************* Endianisms ************* */

UInt16 HostToUSBWord(UInt16 value)
{
	return( (value << 8) | (value >> 8) );
}

/* This is identical to above, but can't make defining it language independant */
UInt16 USBToHostWord(UInt16 value)
{
	return( (value << 8) | (value >> 8) );
}

UInt32 HostToUSBLong(UInt32 value)
{
	return(
		(((UInt32) value) >> 24)				|		
		((((UInt32) value) >> 8) & 0xFF00)		|		
		((((UInt32) value) << 8) & 0xFF0000)	|		
		(((UInt32) value) << 24)
		);

}

UInt32 USBToHostLong(UInt32 value)
{
	return(
		(((UInt32) value) >> 24)				|		
		((((UInt32) value) >> 8) & 0xFF00)		|		
		((((UInt32) value) << 8) & 0xFF0000)	|		
		(((UInt32) value) << 24)
		);

}


Boolean immediateError(OSStatus err)
{
	return((err != kUSBPending) && (err != noErr) );
}


