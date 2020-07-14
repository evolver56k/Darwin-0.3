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
	File:		uslPipe.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB18>	11/16/98	BT		Clear pipe state on open
	 <USB17>	 11/5/98	BT		Fix returning noErr instead of pending.
	 <USB16>	10/22/98	BT		Fix wrong status for deleted devices.
	 <USB15>	10/12/98	BT		Add addressed device request
	 <USB14>	 9/29/98	BT		Use real frame timing
	 <USB13>	 8/31/98	BT		Add isoc pipes
	 <USB12>	 8/25/98	BT		Isoc name changes
	 <USB11>	 8/24/98	BT		Isoc param block definition
	 <USB10>	 8/13/98	BT		Add multibus support
	  <USB9>	 8/13/98	BT		Allow zero length transactions
	  <USB8>	 7/23/98	BT		Cut annoying message
	  <USB7>	  7/7/98	BT		Avoid control reentrancy problem by making sure they're done at
									secondary interrupt time.
	  <USB6>	 5/21/98	BT		Don't corrupt usbBuffer when diverting to opendevice.
	  <USB5>	 5/20/98	BT		Find implicit dev config
	  <USB4>	 5/20/98	BT		Fix pipe devidx field
	  <USB3>	 5/17/98	BT		Device request doesn't squash reqcount, buffer. Add internal
									open pipe for config interface
	  <USB2>	 4/28/98	BT		Add bulk performance monitoring.
	  <USB1>	 4/26/98	BT		first checked in
*/

#include "../USB.h"
#include "../USBpriv.h"

#include "uslpriv.h"
#include "../uimpriv.h"

#include "../driverservices.h"


static pipe pipeZeroZero;	// the default pipe to device zero 

#if 0
static OSStatus uslOpenControlEndpoint(struct USBPB *pb)
{
usbDevice *deviceP;
OSStatus retVal = kUSBInternalErr;
pipe *newPipe;

	do{	/* For error recovery */
	
		/* Validate device */
		deviceP = getDevicePtr(pb->usbReference);
		
		if(deviceP == nil)
		{
			retVal = kUSBUnknownDeviceErr;
			break;
		}
	
#if 0
		/* Validate endpoint */
		if(deviceP->numberOfEndpoints >= endpoint)
		{
			retVal = kUSBUnknownEndpointErr;
			break;
		}
#endif

		newPipe = getPipe(deviceP, pb->usbValue2);
		
		if(newPipe != nil)
		{
			retVal = kUSBAlreadyOpenErr;
			break;
		}

		/* This endpoint has not yet been opened. */
		if(pb->usbValue2 == 0)	/* Endpoint zero is a control endpoint */
		{
			retVal = kUSBUnknownEndpointErr;
			break;
		}


		/* Allocate a new pipe */
		newPipe = AllocPipe(deviceP, pb->usbValue2);
		if(newPipe == nil)
		{
			retVal = kUSBTooManyPipesErr;
			break;
		}
		/* Fill in pertinent information for this pipe instantiation */
		newPipe->deviceIdx = makeDeviceIdx(pb->usbReference);
		newPipe->intrfc = pb->usbReference;
		newPipe->endPt = pb->usbValue2;
		newPipe->devAddress = deviceP->usbAddress;
		newPipe->flags = pb->usbFlags;
		newPipe->state = kUSBActive;

#if 0
		retVal = UIMSetupControlEndpoint(newPipe->addEnd, &newPipe->uimRef, 
					newPipe->maxPacket, controlCallBack, newPipe->iflags, newPipe->addEnd /* refCon */);
#endif

		if(retVal != kUSBNoErr)
		{
			deallocPipe(deviceP, pb->usbValue2);
			break;
		}


		pb->usbReference = newPipe->ref;

		/* Just assume you can have an endpoint zero for now. */
		/* Shouldn't this be already open */

#if 0	
		/* XXXXXXXX In real version should add something here */
		if(pipeH->type != kUSBControlType)
		{
			retVal = kUSBIncorrectTypeErr;
			break;
		}
#endif
	
		
		retVal = kUSBNoErr;

		break;
	}while(0); /* End error checking */
	
	return(retVal);
}
#endif

static OSStatus uslOpenInterruptEndpoint(USBPB *pb)
{
usbDevice *deviceP;
OSStatus retVal = kUSBInternalErr;
pipe *newPipe;

	do{	/* For error recovery */
	
		/* Validate device */
		deviceP = getDevicePtr(pb->usbReference);
		
		if(deviceP == nil)
		{
			retVal = kUSBUnknownDeviceErr;
			break;
		}
	
#if 0
		/* Validate endpoint */
		if(deviceP->numberOfEndpoints >= endpoint)
		{
			retVal = kUSBUnknownEndpointErr;
			break;
		}
#endif

		newPipe = getPipe(deviceP, pb->usbOther, kUSBIn);
		
		if(newPipe != nil)
		{
			retVal = kUSBAlreadyOpenErr;
			break;
		}

		/* This endpoint has not yet been opened. */
		if(pb->usbOther == 0)	/* Int pipe can't be zero */
		{
			retVal = kUSBIncorrectTypeErr;
			break;
		}


		/* Allocate a new pipe */
		newPipe = AllocPipe(deviceP, pb->usbOther, kUSBIn);
		if(newPipe == nil)
		{
			retVal = kUSBTooManyPipesErr;
			break;
		}

		/* Fill in pertinent information for this pipe instantiation */
		newPipe->bus = deviceP->bus;
		newPipe->devIntfRef = pb->usbReference;
		newPipe->endPt = pb->usbOther;
		newPipe->direction = kUSBIn;
		newPipe->devAddress = deviceP->usbAddress;
		newPipe->flags = pb->usbFlags;
		newPipe->type = kUSBInterrupt;
		newPipe->maxPacket = pb->usb.cntl.WValue;
		newPipe->state = kUSBActive;
		pb->usbReference = 0;
		
		retVal = UIMInterruptEDCreate(newPipe->bus,
					newPipe->devAddress, 
					pb->usbOther, 	/* User gives Endpoint number here */
					pb->usb.cntl.WValue, 	/* System fills in Max packet size here */
					deviceP->speed);

		pb->usbStatus = retVal;
		if(retVal != kUSBNoErr)
		{
			deallocPipe(deviceP, pb->usbOther, kUSBIn);
			break;
		}

		pb->usbReference = newPipe->ref;	
		if(pb->usbCompletion != nil)
		{
			(*pb->usbCompletion)(pb);
		}

		break;
	}while(0); /* End error checking */
	
	return(retVal);
}


static OSStatus uslOpenIsocEndpoint(USBPB *pb)
{
usbDevice *deviceP;
OSStatus retVal = kUSBInternalErr;
pipe *newPipe;
USBDirection usbDirection;

	do{	/* For error recovery */
	
		/* Validate device */
		deviceP = getDevicePtr(pb->usbReference);
		
		if(deviceP == nil)
		{
			retVal = kUSBUnknownDeviceErr;
			break;
		}
	
#if 0
		/* Validate endpoint */
		if(deviceP->numberOfEndpoints >= endpoint)
		{
			retVal = kUSBUnknownEndpointErr;
			break;
		}
#endif

		usbDirection = pb->usbFlags & 0xff;
		newPipe = getPipe(deviceP, pb->usbOther, usbDirection);
		
		if(newPipe != nil)
		{
			retVal = kUSBAlreadyOpenErr;
			break;
		}

		/* This endpoint has not yet been opened. */
		if(pb->usbOther == 0)	/* Isoc pipe can't be zero */
		{
			retVal = kUSBIncorrectTypeErr;
			break;
		}


		/* Allocate a new pipe */
		newPipe = AllocPipe(deviceP, pb->usbOther, usbDirection);
		if(newPipe == nil)
		{
			retVal = kUSBTooManyPipesErr;
			break;
		}

		/* Fill in pertinent information for this pipe instantiation */
		newPipe->bus = deviceP->bus;
		newPipe->devIntfRef = pb->usbReference;
		newPipe->endPt = pb->usbOther;
		newPipe->direction = usbDirection;
		newPipe->devAddress = deviceP->usbAddress;
		newPipe->flags = pb->usbFlags;
		newPipe->type = kUSBIsoc;
		newPipe->maxPacket = pb->usb.cntl.WValue;
		newPipe->state = kUSBActive;
		pb->usbReference = 0;
		
		retVal = UIMIsocEDCreate(newPipe->bus,
					newPipe->devAddress, 
					pb->usbOther, 			/* User gives Endpoint number here */
					usbDirection,
					pb->usb.cntl.WValue); 	/* System fills in Max packet size here */

		pb->usbStatus = retVal;
		if(retVal != kUSBNoErr)
		{
			deallocPipe(deviceP, pb->usbOther, usbDirection);
			break;
		}

		pb->usbReference = newPipe->ref;	
		if(pb->usbCompletion != nil)
		{
			(*pb->usbCompletion)(pb);
		}

		break;
	}while(0); /* End error checking */
	
	return(retVal);
}


static OSStatus uslOpenBulkEndpoint(USBPB *pb)
{
usbDevice *deviceP;
OSStatus retVal = kUSBInternalErr;
pipe *newPipe;
USBDirection usbDirection;

	do{	/* For error recovery */
	
		/* Validate device */
		deviceP = getDevicePtr(pb->usbReference);
		
		if(deviceP == nil)
		{
			retVal = kUSBUnknownDeviceErr;
			break;
		}
	
#if 0
		/* Validate endpoint */
		if(deviceP->numberOfEndpoints >= endpoint)
		{
			retVal = kUSBUnknownEndpointErr;
			break;
		}
#endif
		
		usbDirection = pb->usbFlags & 0xff;
		
		newPipe = getPipe(deviceP, pb->usbOther, usbDirection);
		
		if(newPipe != nil)
		{
			retVal = kUSBAlreadyOpenErr;
			break;
		}

		/* This endpoint has not yet been opened. */
		if(pb->usbOther == 0)	/* Not default pipe */
		{
			retVal = kUSBIncorrectTypeErr;
			break;
		}


		/* Allocate a new pipe */
		newPipe = AllocPipe(deviceP, pb->usbOther, usbDirection );
		if(newPipe == nil)
		{
			retVal = kUSBTooManyPipesErr;
			break;
		}
		/* Fill in pertinent information for this pipe instantiation */
		newPipe->bus = deviceP->bus;
		newPipe->devIntfRef = pb->usbReference;
		newPipe->endPt = pb->usbOther;
		newPipe->direction = usbDirection;
		newPipe->devAddress = deviceP->usbAddress;
		newPipe->flags = pb->usbFlags;
		newPipe->type = kUSBBulk;
		newPipe->maxPacket = pb->usb.cntl.WValue;
		newPipe->state = kUSBActive;

		retVal = UIMBulkEDCreate(newPipe->bus,
					newPipe->devAddress, 
					pb->usbOther, 	/* User gives Endpoint number here */
					usbDirection,
					pb->usb.cntl.WValue);

		if(retVal != kUSBNoErr)
		{
			deallocPipe(deviceP, pb->usbOther, usbDirection );
			break;
		}

		pb->usbReference = newPipe->ref;	
		
		retVal = kUSBNoErr;
		if(pb->usbCompletion != nil)
		{
			(*pb->usbCompletion)(pb);
		}

		break;
	}while(0); /* End error checking */
	
	return(retVal);
}

static OSStatus usbSIControlPacket(void *p1, void *addrEp)
{	// p1 is pointer to paramblock
	// second param has addr and endpoint munged together
UInt32 p2;
UInt8 addr, endPt;
OSStatus err;
USBPB *pb;
UInt32 bus;

	p2 = (UInt32)addrEp;
	bus = p2 >> 16;
	addr = (p2>>8)  & 0xff;
	endPt = p2 & 0xf;
	
	err = usbControlPacket(bus, p1, addr, endPt);
	if(err != noErr)
	{
		pb = p1;
		if( (pb->usbCompletion != nil) && (pb->usbCompletion != (void *)-1) )
		{
			pb->usbStatus = err;
			(*pb->usbCompletion)(pb);
		}
	}
	return(noErr);
}

/* We need to make sure these are queued as well */
OSStatus xUSLControlRequest(UInt32 bus, USBPB *pb, pipe *thisPipe)
{
setupPacket *packet;
OSStatus err = kUSBPending;

USBDirection direction;
USBRqType type;
USBRqRecipient recipient;
USBRequest request;
UInt16 transferSize;
//kprintf("xUSLControlRequest:ref=0x%x,device=%d\n",thisPipe->ref,thisPipe->devAddress);
	do{
		direction = pb->usb.cntl.BMRequestType >> 7;
		type = (pb->usb.cntl.BMRequestType >> 5) & 3;
		recipient = pb->usb.cntl.BMRequestType & 31;
		request = pb->usb.cntl.BRequest;

		/* FInd the packet in the param block */
		packet = (void *)&pb->usb.cntl.BMRequestType;
		
		/* Check the direction flag */
 		if(kUSBNone == direction || pb->usbReqCount == 0 || pb->usbBuffer == nil)
		{
 			transferSize = 0;	/* Don't sit on reqcount and buffer */
			direction = kUSBOut;
		} 
		else
		{
			transferSize = pb->usbReqCount;
		}

		/* Put together the bitmap request type byte */
		if(packet->byte[bmRqType] == 0xff)
		{
			err = kUSBRqErr;
			break;
		}
 
 		/* Check the request is a valid one */
 		if( (type == kUSBStandard) &&
 				(request < kUSBRqGetStatus || request > kUSBRqSyncFrame || 
				request == kUSBRqReserved1 || request == kUSBRqReserved2) )
		{
			err = kUSBUnknownRequestErr;
			break;
		}
		
		/* Put together the rest of the packet */
		packet->word[wValue] = HostToUSBWord(pb->usb.cntl.WValue);
		packet->word[wIndex] = HostToUSBWord(pb->usb.cntl.WIndex);
		packet->word[wLength] = HostToUSBWord(transferSize);
		 
		pb->usbStatus = kUSBPending;
		if(CurrentExecutionLevel() == kSecondaryInterruptLevel)
		{
			err = usbControlPacket(bus, pb, thisPipe->devAddress, thisPipe->endPt);
		}
		else
		{
#if 0
		static pipe *rootHub;
			// temp hack.
			// If we're not being called at secondary int, avoid reentrancy prob
			// by arranging to be called at secondary int.
			
			// First try to suppress root hub messages
			if(rootHub == nil)	// assume first seen pipe is root hub
			{
				rootHub = thisPipe;
			}
			if(thisPipe != rootHub)
			{
				USBExpertStatus(-1, "\pUSL - Queueing secondary interrupt Control", thisPipe->devAddress);
			}
#endif			
			// now queue the call back. address and endpoint munged into second parameter
			err = QueueSecondaryInterruptHandler(usbSIControlPacket, nil, pb, 
							(void *)( 
							(bus << 16) +
							((UInt32)thisPipe->devAddress << 8) + 
							((UInt32)thisPipe->endPt) 
							) );
		}
		
		/* Processing now continues in controlCallBack */


 	}while(0);
 	
	if(err == noErr)
	{
		err = kUSBPending;
	}
	else
	{
		pb->usbStatus = err;
	}
	return(err);
}

OSStatus uslDeviceRequest(USBPB *pb)
{
USBReference ref0, ref;
pipe *thisPipe;
OSStatus retVal = noErr;
UInt32 bus=0;

	do{
		ref = pb->usbReference;

		if((pb->usbFlags & kUSBAddressRequest) != 0)
		{
		USBRqRecipient recipient;
			recipient = pb->usb.cntl.BMRequestType & 31;
			if(recipient == kUSBInterface)
			{
			uslInterface *ifc;
				retVal = findInterface(ref, &ifc);
				if(ifc != nil)
				{
					pb->usb.cntl.WIndex = ifc->interfaceNum;
				}
			}
			else if(recipient == kUSBEndpoint)
			{
				thisPipe = GetPipePtr(ref);
				if(thisPipe != 0)
				{
					ref = thisPipe->devIntfRef;
					pb->usb.cntl.WIndex = thisPipe->endPt;
					if(thisPipe->direction == kUSBIn)
					{
						pb->usb.cntl.WIndex |= 0x80;
					}
				}
				else
				{
					retVal = kUSBUnknownPipeErr;
				}
			}
			else
			{
				retVal = paramErr;
			}
			if(retVal != noErr)
			{
				pb->usbStatus = retVal;
				break;
			}
		}
		
		ref0 = getPipeZero(ref);
		if(ref0 == nil)
		{
			if(uslHubValidateDevZero(ref, &bus))
			{
				thisPipe = &pipeZeroZero;
				thisPipe->bus = bus;
			}
			else
			{
				retVal = kUSBUnknownDeviceErr;
				break;
			}
		}
		else
		{
			thisPipe = GetPipePtr(ref0);
	 		if(thisPipe == nil)
			{
				pb->usbStatus = kUSBUnknownPipeErr;
				break;
			}
		}
		retVal = xUSLControlRequest(thisPipe->bus, pb, thisPipe);
		if(retVal != kUSBPending)
		{
			break;
		}
		
	}while(0);
	return(retVal);
}



OSStatus USBDeviceRequest(USBPB *pb)
{
	if(!checkPBVersion(pb, kUSBAddressRequest))
	{
		return(pb->usbStatus);
	}

	/* Short circuit config device requests */
	if( (pb->pbVersion != 1) &&	// requires 0.02 PB
		(pb->usb.cntl.BMRequestType == 0) &&
		(pb->usb.cntl.BRequest == kUSBRqSetConfig) )
	{
		pb->usbReqCount = 0;
		pb->usbActCount = 0;
		
		pb->addrStash = (UInt32)pb->usbBuffer;
		pb->usbBuffer = 0;
		return(uslOpenDevice(pb));
	}

	return(uslDeviceRequest(pb));
}


/* xxxx do checking that params are correct, now we have config desc */

/* xxxx do checking that params are correct, now we have config desc */
static OSStatus uslOpenPipe(USBPB *pb)
{

	switch(pb->usbClassType)
	{
		case kUSBBulk:
			return(uslOpenBulkEndpoint(pb));
		break;
	
		case kUSBInterrupt:
			return(uslOpenInterruptEndpoint(pb));
		break;
	
		case kUSBIsoc:
			return(uslOpenIsocEndpoint(pb));
		break;
	
		case kUSBControl:
			//OSStatus uslOpenControlEndpoint(USBPB *pb);
			return(kUSBIncorrectTypeErr);
		break;
	
		default:
			return(kUSBIncorrectTypeErr);
		break;
	
	}
}

OSStatus USBOpenPipe(USBPB *pb)
{
	if(!checkPBVersion(pb, kUSBAnyDirn))
	{
		return(pb->usbStatus);
	}

	return(uslOpenPipe(pb));

}


OSStatus uslOpenPipeImmed(USBInterfaceRef intrfc, USBEndPointDescriptor *endp)
{
	/* this relies on USBOpenPipe, and the bulk and interrupt opens */
	/* completing immediatly and not caring if the completion is zero */
USBPB pb;

	pb.usbReference = intrfc;
	pb.usbCompletion = nil;
	
	if((endp->endpointAddress & 0x80) == 0)
	{
		pb.usbFlags = kUSBOut;
	}
	else
	{
		pb.usbFlags = kUSBIn;
	}

	pb.usbClassType = endp->attributes&3;	// endpoint type
	pb.usbOther = endp->endpointAddress & 0xf;
	pb.usb.cntl.WValue = USBToHostWord(endp->maxPacketSize);
	return(uslOpenPipe(&pb));
}


void resolvePerformance(USBPB *pb)
{
pipe *thisPipe;
OSStatus err;

	err = findPipe(pb->usbReference, &thisPipe);
	if( (thisPipe != nil) && (thisPipe->monitorPerformance) )
	{
		pb->usbFrame = deltaFrames(pb->usbFrame, thisPipe->bus);
		AddAtomic(pb->usbFrame, (void *)&thisPipe->transferTime);
		AddAtomic(pb->usbActCount, (void *)&thisPipe->bytesTransfered);
		pb->usbFlags = 0;
	}
}

OSStatus USBBulkRead(USBPB *pb)
{
pipe *thisPipe;
OSStatus retVal;
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}

	retVal = kUSBPending;
	pb->usbStatus = retVal;
	do{
		/* Find our pipe structure for this */
		retVal = findPipe(pb->usbReference, &thisPipe);
		if(retVal != noErr)
		{
			break;
		}

		if(thisPipe->monitorPerformance)
		{
			pb->usbFlags = 1;
			pb->usbFrame = deltaFrames(0, thisPipe->bus);
		}

		/* Validate its a bulk out pipe */
 		if( (thisPipe->direction != kUSBIn) || (thisPipe->type != kUSBBulk) )
 		{
			retVal = kUSBIncorrectTypeErr;
			break;
 		}
 		
		/* BT 13Aug98, allow zero length transactions, they're valid. */	

		retVal = usbBulkPacket(thisPipe->bus, pb, thisPipe->devAddress, thisPipe->endPt, kUSBIn);
		/* Processing now continues in bulkCallBack */


 	}while(0);

	if(retVal == noErr)
	{
		retVal = kUSBPending;
	}
	else
	{
		pb->usbStatus = retVal;
	}
	return(retVal);
}

OSStatus USBIntRead(USBPB *pb)
{
pipe *thisPipe;
OSStatus retVal;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}

	retVal = kUSBPending;
	pb->usbStatus = retVal;
	do{
		/* Find our pipe structure for this */
		retVal = findPipe(pb->usbReference, &thisPipe);
		if(retVal != noErr)
		{
			break;
		}

		if(thisPipe->monitorPerformance)
		{
			pb->usbFrame = deltaFrames(0, thisPipe->bus);
		}	

		/* Validate its a bulk out pipe */
 		if( (thisPipe->direction != kUSBIn) || (thisPipe->type != kUSBInterrupt) )
 		{
			retVal = kUSBIncorrectTypeErr;
			break;
 		}
 		
		/* BT 13Aug98, allow zero length transactions, they're valid. */	

kprintf("USBIntRead:calling usbIntPacket\n");
		retVal = usbIntPacket(thisPipe->bus, pb, thisPipe->devAddress, thisPipe->endPt, kUSBIn);
		/* Processing now continues in intCallBack */


 	}while(0);
	if(retVal == noErr)
	{
		retVal = kUSBPending;
	}
	else
	{
		pb->usbStatus = retVal;
	}
	return(retVal);
}

OSStatus USBIsocRead(USBPB *pb)
{
pipe *thisPipe;
OSStatus retVal;

	if(!checkPBVersionIsoc(pb, 0))
	{
		return(pb->usbStatus);
	}

	retVal = kUSBPending;
	pb->usbStatus = retVal;
	do{
		/* Find our pipe structure for this */
		retVal = findPipe(pb->usbReference, &thisPipe);
		if(retVal != noErr)
		{
			break;
		}

		/* Validate its a bulk out pipe */
 		if( (thisPipe->direction != kUSBIn) || (thisPipe->type != kUSBIsoc) )
 		{
			retVal = kUSBIncorrectTypeErr;
			break;
 		}
 		
		retVal = usbIsocPacket(thisPipe->bus, pb, thisPipe->devAddress, thisPipe->endPt, kUSBIn);
		/* Processing now continues in bulkCallBack */


 	}while(0);
	if(retVal == noErr)
	{
		retVal = kUSBPending;
	}
	else
	{
		pb->usbStatus = retVal;
	}
	return(retVal);
}

OSStatus USBBulkWrite(USBPB *pb)
{
pipe *thisPipe;
OSStatus retVal;
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}

	retVal = kUSBPending;
	pb->usbStatus = retVal;
	do{
		/* Find our pipe structure for this */
		retVal = findPipe(pb->usbReference, &thisPipe);
		if(retVal != noErr)
		{
			break;
		}

		if(thisPipe->monitorPerformance)
		{
			pb->usbFlags = 1;
			pb->usbFrame = deltaFrames(0, thisPipe->bus);
		}

		/* Validate its a bulk out pipe */
 		if( (thisPipe->direction != kUSBOut) || (thisPipe->type != kUSBBulk) )
 		{
			retVal = kUSBIncorrectTypeErr;
			break;
 		}
 		
		/* BT 13Aug98, allow zero length transactions, they're valid. */	

		retVal = usbBulkPacket(thisPipe->bus, pb, thisPipe->devAddress, thisPipe->endPt, kUSBOut);
		/* Processing now continues in controlCallBack */


 	}while(0);
	if(retVal == noErr)
	{
		retVal = kUSBPending;
	}
	else
	{
		pb->usbStatus = retVal;
	}
	return(retVal);
}

OSStatus USBIsocWrite(USBPB *pb)
{
pipe *thisPipe;
OSStatus retVal;
	if(!checkPBVersionIsoc(pb, 0))
	{
		return(pb->usbStatus);
	}

	retVal = kUSBPending;
	pb->usbStatus = retVal;
	do{
		/* Find our pipe structure for this */
		retVal = findPipe(pb->usbReference, &thisPipe);
		if(retVal != noErr)
		{
			break;
		}

		/* Validate its a bulk out pipe */
 		if( (thisPipe->direction != kUSBOut) || (thisPipe->type != kUSBIsoc) )
 		{
			retVal = kUSBIncorrectTypeErr;
			break;
 		}
 		
		retVal = usbIsocPacket(thisPipe->bus, pb, thisPipe->devAddress, thisPipe->endPt, kUSBOut);
		/* Processing now continues in controlCallBack */


 	}while(0);
	if(retVal == noErr)
	{
		retVal = kUSBPending;
	}
	else
	{
		pb->usbStatus = retVal;
	}
	return(retVal);
}

OSStatus uslMonitorBulkPerformanceByReference(USBPipeRef ref, UInt32 what, 
					UInt32 *lastRead, UInt32 *bytesTransfered, UInt32 *transferTime)
{
pipe *thisPipe;
OSStatus err = noErr;

	do{
			/* Note this can't be a device ref, they're automatically cleared */
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		
		/* Do something here */
		switch(what)
		{
			case 0:
			/* stop monitoring this pipe */
				if(thisPipe->monitorPerformance)
				{
					thisPipe->monitorPerformance = false;
				}
				else
				{
					err = kUSBNotFound;
				}
			break;
			
			case 1:
			/* Start monitoring pipe */
				if(thisPipe->monitorPerformance)
				{
					err = kUSBAlreadyOpenErr;
				}
				else
				{
					thisPipe->monitorPerformance = true;
					thisPipe->lastRead = deltaFrames(0, thisPipe->bus);
				}
			break;
			
			case 2:
			/* Get stats, don't clear */
				if(thisPipe->monitorPerformance)
				{
					*lastRead = thisPipe->lastRead;
					thisPipe->lastRead = deltaFrames(0, thisPipe->bus);
					*bytesTransfered = thisPipe->bytesTransfered;
					*transferTime = thisPipe->transferTime;
				}
				else
				{
					err = kUSBNotFound;
				}
			break;
			
			case 3:
			/* Get stats clear */
				if(thisPipe->monitorPerformance)
				{
					thisPipe->monitorPerformance = true;
				}
				*lastRead = thisPipe->lastRead;
				thisPipe->lastRead = deltaFrames(0, thisPipe->bus);
				*bytesTransfered = thisPipe->bytesTransfered;
				*transferTime = thisPipe->transferTime;
				
				AddAtomic(-*bytesTransfered, (void *)&thisPipe->bytesTransfered);
				AddAtomic(-*transferTime, (void *)&thisPipe->transferTime);

			break;
			
		}
		
	}while(0);

	return(err);
}
