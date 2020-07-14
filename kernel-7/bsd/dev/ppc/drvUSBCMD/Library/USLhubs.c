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
	File:		USLhubs.c

	Contains:	Hub specific functions for the USL (mainly add and subtract devices)

	Version:	Nepturn 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DF)	David Ferguson
		(CJK)	Craig Keithley
		(BT)	Barry Twycross
		
	Change History (most recent first):

	 <USB40>	 11/4/98	BT		Comment inobvious zering of refCon
	 <USB39>	10/29/98	BT		Close old configurations on reset or reconfiguration.
	 <USB38>	 10/7/98	BT		Fix hub not killing children
	 <USB37>	 10/3/98	BT		Add more port reset stuff
	 <USB36>	 9/29/98	BT		Use real frame timing
	 <USB35>	 9/28/98	BT		Add device reset function
	 <USB34>	 9/17/98	BT		1.1 rules for captive devices.
	 <USB33>	 8/25/98	BT		Isoc name changes
	 <USB32>	 8/24/98	BT		Cope without #define for reserved4
	 <USB31>	 8/13/98	BT		Add multibus support
	 <USB30>	 7/10/98	BT		DO secondary interrupt right, set address fail returns ref 0.
	 <USB29>	  7/9/98	BT		Fix previous fixes. Secondary interrupt does not queue secondary
									interrupt
	 <USB28>	  7/9/98	BT		Clean up queues when device deleted
	 <USB27>	 6/15/98	BT		Cope with 1.1 hubs speed detection
	 <USB26>	 6/14/98	DF		Add RemoveRootHub function
	 <USB25>	  6/5/98	BT		Pass pb to delete device
	 <USB24>	 5/20/98	BT		Version 2 PB new names
	 <USB23>	 4/29/98	BT		FIx negative timeout
	 <USB22>	 4/26/98	BT		FIx timeout adding dead device
	 <USB21>	 4/23/98	BT		Add watchdog
	 <USB20>	 4/21/98	BT		Allow requests to device zero, pipe zero.
	 <USB19>	 4/16/98	BT		Eliminate debugger
	 <USB18>	 4/14/98	BT		Impliment device remove
	 <USB17>	 4/10/98	BT		Add device remove
	 <USB16>	  4/9/98	CJK		remove uimpriv.h and uslpriv.h
	 <USB15>	  4/9/98	BT		Use USB.h
		<14>	  4/8/98	BT		More error checking
		<13>	  4/6/98	BT		Change w names
		<12>	  4/6/98	BT		New param block names
		<11>	  4/2/98	BT		Eliminate obsolete delay function
		<10>	 2/23/98	BT		Only add device if set address was sucessful
		 <9>	  2/4/98	BT		Fix uslControlPacket to have errors. Add more support for
									usbProbe
		 <8>	 1/26/98	BT		Mangle names after design review
		 <7>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <6>	 1/13/98	BT		Change uslPacket to uslControlPacket
		 <5>	12/19/97	BT		UIM now a Shared lib
		 <4>	12/18/97	BT		Fix header dependancies and add expert call back
*/

#include "../USB.h"
#include "../USBpriv.h"
                
#include "../uimpriv.h"
#include "uslpriv.h"   
                
#include "../driverservices.h"


static QHdrPtr hubAddQueue;
static unsigned long hubAddFlag = 0;
static USBDeviceRef deviceZero;
static UInt32 busZero;
static Duration lastAdd;
static USBPB timeout={0,0, sizeof(USBPB), kUSBCurrentPBVersion};

void initialiseHubs(void)
{
	if( (PBQueueCreate(&hubAddQueue) != noErr) ||
		(PBQueueInit(hubAddQueue) != noErr) )
	{
		/* Panic */
		USBExpertStatus(0,"USL - failed to initialise hub queue", 0);
	}
}

void finaliseHubs(void)
{
	// really need to count the busses!
	//RemoveRootHub(0);	// This should only happen if bus is removed????
	//PBQueueDelete(hubAddQueue);
}

Boolean uslHubValidateDevZero(USBDeviceRef ref, UInt32 *bus)
{
	if(bus != nil)
	{
		*bus = busZero;
	}
	return(ref == deviceZero);
}

OSStatus USBHubConfigurePipeZero(USBPB *pb)
{
UInt32 bus=0;
	if(!checkPBVersion(pb, 1))
	{
		return(pb->usbStatus);
	}
	if(pb->pbVersion == 2)	// v1.00+ only
	{
		pb->usbStatus = kUSBPBVersionError;
		return(pb->usbStatus);
	}
	if(!uslHubValidateDevZero(pb->usbReference, &bus))
	{
		pb->usbStatus = kUSBUnknownDeviceErr;
		return(pb->usbStatus);		
	}

kprintf("USBHubConfigurePipeZero:calling UIMControlEDCreate\n");
	pb->usbStatus = UIMControlEDCreate(bus, 0, 0, pb->usb.cntl.WValue, pb->usbFlags);
	(*pb->usbCompletion)(pb);	
	return(kUSBPending);
}


void uslCleanHubQueue(USBDeviceRef ref)
{	// This is assuming its called at secondary interrupt time.
	uslCleanAQueue(hubAddQueue, ref);
}

static void processAddQ(void)
{
OSStatus err;
USBPB *pb;
	if(CompareAndSwap(0, 1, &hubAddFlag))
	{	/* We now have the queue*/
		if(PBDequeueFirst(hubAddQueue, (void *)&pb) == noErr)
		{

			// Could validate ref here.
			
			busZero = pb->usbOther;
			lastAdd = deltaFrames(0, busZero);
			if(pb->pbVersion == 2)
			{
kprintf("processAddQ:calling UIMControlEDCreate\n");
				err = UIMControlEDCreate(busZero, 0, 0, 8, pb->usbFlags);
			/* Error checking ??? */
			}
			
			deviceZero = MakeDevRef(busZero, 0);
			pb->usbReference = deviceZero;
			pb->usbStatus = noErr;
			(*pb->usbCompletion)(pb);
		}
	}
}

/* Takes bus in reference */

void uslHubAddDevice(USBPB *pb)
{
	pb->qType = kUSBHubQType;
	pb->usbStatus = kUSBPending;
	PBEnqueueLast((void *)pb, hubAddQueue);
	processAddQ();
}

OSStatus USBHubAddDevice(USBPB *pb)
{
UInt32 bus;

	pb->usbStatus = validateRef(pb->usbReference, &bus);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	if(!checkPBVersion(pb, 1))
	{
		return(pb->usbStatus);
	}

	pb->usbOther = bus;
	uslHubAddDevice(pb);
	
	return(kUSBPending);
}

/* Takes bus in reference */

static void setAddressHandler(USBPB *pb)
{
UInt32 bus;
UInt32 power;

	bus = busZero;

//	UIMEDDelete(0, 0, 0);
kprintf("setAddressHandler: calling UIMControlEDDelete\n");
	UIMControlEDDelete(bus, 0, 0);
	deviceZero = 0;
	lastAdd = 0;
	
	if(pb->reserved2 != nil)
	{	/* real user call back */
		if(pb->usbStatus == noErr)
		{
			power = ( (pb->usbFlags & kUSBHubPower) != 0)?pb->usbOther:kUSB500mAAvailable;
			if((pb->usbFlags & kUSBHubReaddress) == 0)
			{
				/* Note, this is relying on having usbValue swapped by uiminterface */
				pb->usbReference = addNewDevice(bus, pb->usb.cntl.WValue,  pb->usbFlags&1, pb->wValueStash, power);
			}
		}
		else
		{
			pb->usbReference = 0;
			USBExpertStatus(0, "USL - Error setting address:", pb->usbReference);
		}
	
		pb->usbCompletion = (void *)pb->reserved2;
		(*pb->usbCompletion)(pb);
	}
	else
	{	/* timeout call back */
	
		/* This really is refcon, not reference, this is not a bug. */
		/* This is the call back from the watchdog timer */
		pb->usbRefcon = 0;
	}	
	CompareAndSwap(1, 0, &hubAddFlag);	/* Unlock the queue */
	
	if(hubAddQueue->qHead != nil)
	{
		processAddQ();
	}
}



static void uslHubSetAddress(USBPB *pb)
{
OSStatus err;
	pb->reserved2 = (UInt32) pb->usbCompletion;
	pb->usbCompletion = setAddressHandler;
	pb->wValueStash = pb->usb.cntl.WValue;

	/* Note this does not have to faff around with */
	/* pbVersion etc its writing a packet directly */
	pb->usb.cntl.BMRequestType = 0;	/* bmRqTyp = 0, bRq = 5 */
	pb->usb.cntl.BRequest = kUSBRqSetAddress;	/* bmRqTyp = 0, bRq = 5 */

	if(pb->usbFlags & kUSBHubReaddress)
	{
	usbDevice *d;
		d = getDevicePtr(pb->usbReference);
		if(d == nil)
		{
			pb->usbStatus = kUSBUnknownDeviceErr;
			setAddressHandler(pb);
			return;
		}
		pb->usb.cntl.WValue = HostToUSBWord(d->usbAddress);
	}
	else
	{
		pb->usb.cntl.WValue = HostToUSBWord(getNewAddress());
	}
	pb->usb.cntl.WIndex = 0;
	pb->usb.cntl.reserved4 = 0;					/* zero length */
	
	pb->usbStatus = kUSBPending;
	err = usbControlPacket(busZero, pb, 0, 0);

	if(err != noErr)
	{	/* Even if error, we need to go through handler */
		pb->usbStatus = err;
		setAddressHandler(pb);
	}
}

OSStatus USBHubSetAddress(USBPB *pb)
{
	if(!checkPBVersion(pb, 1|kUSBHubPower|kUSBHubReaddress))
	{
		return(pb->usbStatus);
	}
	
	if(pb->usbFlags & kUSBHubReaddress)
	{
		if(getDevicePtr(pb->usbReference) == nil)
		{
			pb->usbStatus = kUSBUnknownDeviceErr;
			return(pb->usbStatus);
		}
	}
	else
	{	
		if(!uslHubValidateDevZero(pb->usbReference, nil))
		{
			pb->usbReference = 0;
			pb->usbStatus = paramErr;
			return(pb->usbStatus);
		}
	}

	uslHubSetAddress(pb);
	
	return(kUSBPending);
}

static OSStatus secondaryCleanInternalQueues(void *p1, void *p2)
{	// Any new queues should put an entry in here 
USBDeviceRef ref;
	p2=0;
	ref = (USBDeviceRef)p1;
	
	uslCleanHubQueue(ref);
	uslCleanDelayQueue(ref);
	uslCleanNotifyQueue(ref);
	uslCleanMemQueue(ref);
	
	return(noErr);	// no one's listening
}

static OSStatus uslCleanInternalQueues(USBDeviceRef ref)
{
	return(CallSecondaryInterruptHandler2(secondaryCleanInternalQueues, nil, (void *)ref, 0));
}

OSStatus USBHubDeviceRemoved(USBPB *pb)
{
OSStatus err = kUSBPending;
usbDevice *deviceP;
OSStatus retVal = noErr;
Boolean noCallBack;

	noCallBack = (pb->usbCompletion == kUSBNoCallBack);
	if(noCallBack)
	{
		pb->usbCompletion = (void *)-2;
	}
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	if(noCallBack)
	{
		pb->usbCompletion = kUSBNoCallBack;
	}

	deviceP = getDevicePtr(pb->usbReference);
	if(deviceP == nil)
	{
		pb->usbStatus = kUSBUnknownDeviceErr;
		return(kUSBUnknownDeviceErr);
	}
	
	// Must do this before pipes are closed.
	retVal = uslCleanInternalQueues(deviceP->ID);
	if(retVal != noErr)	// if this returns an erro we're hosed.
	{
		return(retVal);
	}

	uslClosePipe(deviceP->pipe0);
	
	uslCloseNonDefaultPipes(deviceP);
	
	return(uslDeleteDevice(pb, deviceP));
}

void uslHubWatchDog(Duration currentMilliSec)
{
USBReference currDevZero;
UInt32 bus;

	if((hubAddQueue->qHead != nil) && (lastAdd != 0) )
	{
		currDevZero = deviceZero;
		bus = deviceZero;
		if(lastAdd < currentMilliSec - 1000)
		{	/* Candidate for removal */
		
			if(CompareAndSwap(currDevZero, 0, (void *)&deviceZero))
			{	/* if we're here, deviceZero didn't change since we checked the time */
				/* We've also just diabled that device zero */
				if(timeout.usbRefcon != 0)
				{
					USBExpertStatus(0, "USL hubs - force set address timed out", 0);
					/* timeout in use already */
kprintf("uslHubWatchDog: calling UIMControlEDDelete\n");
					UIMControlEDDelete(bus, 0, 0);
					timeout.usbRefcon = 0;
				}
				else
				{
					USBExpertStatus(0, "USL hubs - timing out add", 0);
					timeout.usbRefcon = 1;
					lastAdd = currentMilliSec + 1000;
					uslHubSetAddress(&timeout);
				}
			}
		
		}
	}
}



