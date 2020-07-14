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
	File:		rootHubDriver.c

	Contains:	Temp root hub stuff

	Version:	0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(TC)	Tom Clark
		(DF)	David Ferguson
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB29>	10/21/98	DF		Pass bus number when adding a root hub
	 <USB28>	 8/25/98	BT		Isoc name changes
	 <USB27>	 8/13/98	BT		Add multibus support
	 <USB26>	 8/12/98	BT		Move root hub back into UIM again. (This file only comment
									changed).
	 <USB25>	 7/10/98	TC		Back out previous revision - it was breaking Yosemite.
	 <USB24>	 6/30/98	BT		Move Root hub sim into UIM
	 <USB23>	 6/14/98	DF		Add RemoveRootHub function
	 <USB22>	  6/5/98	TC		Change uxBusRef to USBBusRef.
	 <USB21>	 5/20/98	BT		Fix V2 pb compatability
	 <USB20>	 5/12/98	BT		New interface stuff
	 <USB19>	  4/9/98	BT		Use USB.h
	 <USB18>	  4/9/98	DF		Fix to work with MasterInterfaces
		<17>	  4/6/98	BT		Change w names
		<16>	  4/6/98	BT		New param block names
		<15>	  2/8/98	BT		Power allocation stuff
		<14>	  2/5/98	BT		Add status notification stuff
		<13>	  2/4/98	BT		Add ref/port info to expert notify]
		<12>	  2/4/98	BT		Clean up after TOms changes
		<11>	 1/26/98	CJK		Change to use USBDeviceDescriptor (instead of just
									devicedescriptor)
		<10>	 1/26/98	BT		Mangle names after design review
		 <9>	 1/26/98	BT		Make expert notify public
		 <8>	 1/15/98	BT		Now obsolete this, make this root stub which just boots real hub
									driver, which uses hub simulation.
		 <7>	 1/15/98	BT		Better error checking
		 <6>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <5>	12/23/97	BT		Remove obsolete include
		 <4>	12/19/97	BT		UIM now a Shared lib
		 <3>	12/18/97	BT		Add expert call back function
*/


#include "../USB.h"
#include "../USBpriv.h"

#include "../uimpriv.h" /* for root hub stuff */
#include "../Library/uslpriv.h" /* for uslDelay */
#include "hub.h"


typedef struct{
	USBPB 				pb;
	UInt32				bus;
	UInt8 *				errorString;
	USBDeviceRef 		hubRef;
	USBDeviceDescriptor desc;
	}rootHub;

#if 1

	/* Incorporate debugging strings */
#define noteError(s)	rh->errorString = s;

#else

	/* eliminate Error strings */
#define noteError(s)

#endif



static Boolean immediateError(OSStatus err)
{
	return((err != kUSBPending) && (err != noErr) );
}


static void rootHubAddDeviceHanlder(USBPB *pb)
{
rootHub *rh;

	rh = (void *)pb;	/* parameter block has been extended */
	if(pb->usbStatus != noErr)
	{
		/* no idea what to do now?? */
		USBExpertFatalError(rh->hubRef, pb->usbStatus, rh->errorString, 1);
		
		return;
	}
	

	do{switch(pb->usbRefcon++)
	{
		case 1:
			rh->hubRef = 0;	/* we use zero to add */
			rh->bus = pb->usbReference;	/* remember who we are */
			pb->usbReference = rh->hubRef;
		
			/* Tell the USL to add the device */
			pb->usbFlags = 0;	/* 1 if low speed, hub never low speed */
			
			noteError("Root hub Error Calling Add Device");
			pb->usbOther = rh->bus;
			uslHubAddDevice(pb);

		break;

		case 2:
			/* We're called back when its time to reset */
			pb->usbFlags = 0;	/* 1 if low speed, hub never low speed */
			pb->usb.cntl.WValue = 8;	/* max packet size, ?? irrelevant ?? */
		
			UIMResetRootHub(rh->bus);		// ***** Do we need this?????

			/* Now address the device */
			noteError("Resetting root hub");
			if(immediateError(USBHubSetAddress(pb)))
			{
				USBExpertFatalError(rh->hubRef, pb->usbStatus, rh->errorString, 0);
			}
			
			
		break;
		case 3:
			/* refernce is now new device ref, no longer dev zero */
			
			/* now do a device request to find out what it is */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);
			pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
			pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);
			pb->usbBuffer = &rh->desc;
			
				/* the get descriptor seems to screw up most often */
			noteError("Getting root hub descriptor");
			if(immediateError(USBDeviceRequest(pb)))
			{
				USBExpertFatalError(rh->hubRef, pb->usbStatus, rh->errorString, 0);
			}
		break;
		
		case 4:
			/* FInally use the data gathered */
			/* The root hub has at least the 500mA available to it. */
			USBExpertInstallDeviceDriver(pb->usbReference, &rh->desc, 0, rh->bus, kUSB500mAAvailable);
			/* Call to the expert */
			
			UIMSetRootHubRef(pb->usbOther, pb->usbReference);
			PoolDeallocate(rh);
		break;
		default:
			noteError("Root Hub Driver Internal Error unused case in hub handler");			
			USBExpertFatalError(rh->hubRef, kUSBInternalErr, rh->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}

OSStatus StartRootHub(UInt32 bus)
{	
rootHub *deviceAdd;

	deviceAdd = PoolAllocateResident(sizeof(rootHub), true);
	if(deviceAdd == nil)
	{
		USBExpertFatalError(bus, kUSBOutOfMemoryErr, "USL - Root hub could not allocate param block", 0);	
	}

	
	deviceAdd->pb.pbVersion = kUSBCurrentPBVersion;
	deviceAdd->pb.pbLength = sizeof(rootHub);
	deviceAdd->pb.usbStatus = noErr;
	deviceAdd->pb.usbReference = bus;
	deviceAdd->pb.usbCompletion = rootHubAddDeviceHanlder;
	deviceAdd->pb.usbRefcon = 1;
	deviceAdd->pb.usbBuffer = 0;
	rootHubAddDeviceHanlder(&deviceAdd->pb);
	return(kUSBPending);
}

