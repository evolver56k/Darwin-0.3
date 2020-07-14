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
	File:		uslDescriptors.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BG)	Bill Galcher
		(DF)	David Ferguson
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB45>	11/13/98	BT		Change to unconfigure device
	 <USB44>	11/13/98	BT		Add unconfigure interface
	 <USB43>	11/11/98	BT		Allow set config to same value for device, add Set config entry
									point
	 <USB42>	11/10/98	BT		Fix abnormal completion when cache config descriptor can not
									find device.
	 <USB41>	11/10/98	BT		Fix early termination from set config with callback return.
	 <USB40>	11/10/98	BT		Fix crash on set config with interface ref.
	 <USB39>	10/29/98	BT		Close old configurations on reset or reconfiguration.
	 <USB38>	10/26/98	BT		fix not finding interface
	 <USB37>	10/12/98	BT		Add power qualification to find next interface
	 <USB36>	 10/6/98	BT		Fix Find next assoc desc with endpoint. Fix usage of getDev in
									various places
	 <USB35>	 9/17/98	BT		1.1 rules for captive devices.
	 <USB34>	 8/25/98	BT		Isoc name changes
	 <USB33>	 8/24/98	BT		Isoc param block definition
	 <USB32>	 8/18/98	BT		Fix small config descriptor error again
	 <USB31>	  7/9/98	BT		Fix previous fixes. Do not call back to dead device
	 <USB30>	  7/9/98	BT		Clean up after delete device and config clash.
	 <USB29>	  7/8/98	BT		Make findconfigdescriptor more paranoid.
	 <USB28>	  7/2/98	BT		Fix new interface ref to not accept interface ref
	 <USB27>	  7/2/98	BT		Fix hang if associated descriptor does not exist.
	 <USB26>	 6/29/98	BT		Fix zeroing paramblock fields after callback in FindNextPipe.
	 <USB25>	 6/24/98	BT		Fix config interface to open all pipes properly.
	 <USB24>	 6/22/98	BT		Add Alt interface to configure interface
	 <USB23>	 6/18/98	BG		Use the correct notation for including <Errors.h> and
									<DriverServices.h>.  I know, this is a nit.
	 <USB22>	  6/9/98	BT		Find next pipe returns max packert size
	 <USB21>	 5/21/98	BT		Note dependancy on dev req in open device. Make config work with
									more than one descriptor
	 <USB20>	 5/20/98	BT		FIx find with interface alt
	 <USB19>	 5/20/98	BT		Fix alt usage in interface functions
	 <USB18>	 5/20/98	BT		Make this top of trunk, (17a2, V2 param b lock stuff)
	 <USB17>	 5/18/98	DF		Get config descriptor actually takes notice of index.
	 <USB16>	 5/18/98	DF		Remove nested comment warning
	 <USB15>	 5/17/98	BT		Add config stuff
	 <USB14>	  5/5/98	BT		Change name to ...immediate
	 <USB13>	 4/16/98	BT		Add include of USBpriv
	 <USB12>	  4/9/98	BT		Use USB.h
		<11>	  4/8/98	BT		Move error codes
		<10>	  4/8/98	BT		More error checking
		 <9>	  4/6/98	BT		Change w names
		 <8>	  4/6/98	BT		Change param block names.
		 <7>	  4/2/98	BT		Add grabbing all config descriptors
		 <6>	 3/19/98	BT		Get endpoint descriptor returns max packet size with right byte
									ordering.
		 <5>	  3/9/98	BT		Eliminate one set of redundant enums
		 <4>	  3/5/98	BT		Add interrupt pipe
		 <3>	 2/23/98	BT		Fix swapped values
		 <2>	  2/4/98	BT		Cope with multiple configs
		 <1>	 1/29/98	BT		first checked in
*/


#include "../USB.h"
#include "../USBpriv.h"

#include "uslpriv.h"
//#include <Errors.h>
#include "../driverservices.h"



//enum{
//	kUSLClientUser = 0,
//	kUSLClientEndPoint

//	};

static Boolean checkZeroBuffer(USBPB *pb)
{
	return( (pb->usbBuffer != 0) || (pb->usbReqCount != 0) || (pb->usbActCount != 0) );
}

/* These allow USL functions to have an internal stage counter. */
/* This drives internal state machines. */

/* In this incarnation some bits in the flags long word are */
/* hijacked for this. If you wanted to revise the PB, an */
/* internal field could be added to do this. See uslPriv.h */
/* abuse of flags */

static void uslClearInternalStage(USBPB *pb)
{
	pb->usbFlags &= ~kUSLStageMask;
}

static int uslFindInternalStageAndIncriment(USBPB *pb)
{
int stage;
	stage = (pb->usbFlags & kUSLStageMask) >>kUSLStageShift;
	stage++;
	pb->usbFlags = (pb->usbFlags & ~kUSLStageMask) | stage << kUSLStageShift;
	return(stage);
}

static void uslCompleteInternalStages(USBPB *pb)
{
usbDevice *dev;

	/* make sure we never hose up a device by leaving it locked */
	dev = getDevicePtr(pb->usbReference);
	if( (dev != nil) && (dev->configLock != 0) )
	{
		dev->configLock = 0;
	}

	uslClearInternalStage(pb);
	pb->usbCompletion = (void *)pb->reserved2;
	(*pb->usbCompletion)(pb);
}

static void uslInitialiseInternalStages(USBPB *pb, void (*fn)(USBPB *pb))
{
	uslClearInternalStage(pb);
	pb->reserved2 = (UInt32) pb->usbCompletion;		/* Save users handler */
	pb->usbCompletion = fn;							/* use our handler */
	pb->usbStatus = noErr;	// so cache conf desc doesn't crapout to start.
	(*fn)(pb);
}


static SInt32 subtractAddress(void *a1, void *a2)
{
	/* Use this to get signed addess compares. */
	/* you get large pos instead of neg using UInts */
	return((SInt32)a1 - (SInt32)a2);

}

static void uslSetInternalStage(USBPB *pb, int stage)
{
	pb->usbFlags = (pb->usbFlags & ~kUSLStageMask) | stage << kUSLStageShift;
}

static void nextInternalStage(USBPB *pb)
{
int stage;
	stage = (pb->usbFlags & kUSLStageMask) >>kUSLStageShift;
	stage ++;
	pb->usbFlags = (pb->usbFlags & ~kUSLStageMask) | stage << kUSLStageShift;
}
#if 0

static void uslMoveStageAlong(USBPB *pb, int n)
{
int stage;
	stage = (pb->usbFlags & kUSLStageMask) >>kUSLStageShift;
	stage += n;
	pb->usbFlags = (pb->usbFlags & ~kUSLStageMask) | stage << kUSLStageShift;
}
#endif

static OSStatus uslReadConfigurationDescriptor(USBPB *pb)
{

//	 --> usbReference		Device
//	 --> usb.cntl.WValue			Configuration value
//	 --> usbBuffer			Configuration Descriptor Structure
//	 --> usbReqCount		size of descriptor
//	<--  usbActCount		size of descriptor returned

	pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);
	pb->usb.cntl.BRequest = kUSBRqGetDescriptor;

	pb->usb.cntl.WValue = (kUSBConfDesc << 8) + (pb->usb.cntl.WValue & 0xff); 
	pb->usb.cntl.WIndex = 0;
			
	return(uslDeviceRequest(pb));
}

static usbDevice *getDev(USBPB *pb)
{
usbDevice *dev;
	dev = getDevicePtr(pb->usbReference);
	if(dev == nil)
	{
		pb->usbStatus = kUSBUnknownDeviceErr;
		(*pb->usbCompletion)(pb);		
	}
	return(dev);
}

/* xxxx This needs to be redone in terms or cache config descr */
static void getFulConfHandler(USBPB *pb)
{
int stage;

//	 --> usbReference		Device
//	 --> usb.cntl.WValue			Configuration number
//	<--  usbBuffer			Configuration Descriptor Structure
//       usbReqCount undefined
//	<--  usbActCount		size of descriptor returned


usbDevice *dev;

	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);			
		return;
	}

	stage = uslFindInternalStageAndIncriment(pb);
	
	do{switch(stage)
	{
		case 1:
			dev = getDev(pb);
			if(dev == nil) break;		
			
			/* use dev scratch to get the root config descritor */
			pb->usbReqCount = sizeof(configHeader);
			pb->usbBuffer = &dev->configScratch;
						
			/* Get the descriptor */
			if(immediateError(uslReadConfigurationDescriptor(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 2:
			/* How long is full descritptor */
			pb->usbReqCount = USBToHostWord(((configHeader *)pb->usbBuffer)->totalLen);

			/* Allocate the full descriptor */
			if(immediateError(uslAllocMem(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 3:
			/* Populate the full descriptor */
			pb->usb.cntl.WValue = pb->usb.cntl.WValue & 0xff; // assumes what readconf does internally
			if(immediateError(uslReadConfigurationDescriptor(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 4:
			uslCompleteInternalStages(pb);			
				
		break;
		
		default:
			USBExpertStatus(0,"USL - get full conf handler, unknown case", 0);
			pb->usbStatus = kUSBInternalErr;
			uslCompleteInternalStages(pb);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */


}

/* xxxx recast interms of cached */
OSStatus USBGetFullConfigurationDescriptor(USBPB *pb)
{
//	 --> usbReference		Device
//	 --> usb.cntl.WValue			Configuration
//	<--  usbBuffer			Configuration Descriptor Structure
//	<--  usbActCount		size of descriptor returned


	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	
	
	pb->usbStatus = noErr;
	uslInitialiseInternalStages(pb, getFulConfHandler);

	return(kUSBPending);
}

static void *nextDescriptor(void *desc)
{
UInt8 *next;
	next = desc;
	next = &next[next[0]];
	return(next);
}

static OSStatus isConfigDescriptor(USBConfigurationDescriptor *conf)
{
	if( (conf->descriptorType != kUSBConfDesc) ||
		(conf->length < 9) ||
		(conf->totalLength < 9) )
	{
		USBExpertStatus(0,"USL - Parse bad config descriptor", 0);
		return(kUSBInternalErr);
	}
	return(noErr);
}

/* xxxx make externally visible ? */
/* xxxx do some sanity checking on the dscriptor?? */
static OSStatus USBFindConfigDescriptorImmedite(USBPB *pb)
{
//	<--> usbBuffer			--> all config descriptors  <-- Config desc
//	 --> usbReqCount		total length of descriptors
//	<--  usbActCount		offset of this descriptor in config descriptor
//	 --> usb.cntl.WValue			Config number
USBConfigurationDescriptor *conf;
void *end;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	
	/* find configuration */
	conf = pb->usbBuffer;
	pb->usbStatus = isConfigDescriptor(conf);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}
	
	end = (void *)(((UInt32)pb->usbBuffer) + pb->usbReqCount);

	while( subtractAddress(end, conf) > 4)
	{
		pb->usbStatus = isConfigDescriptor(conf);
		if(pb->usbStatus != noErr)
		{
			return(pb->usbStatus);
		}

		if(subtractAddress(end, conf) < USBToHostWord(conf->totalLength) )
		{
			return(kUSBNotFound);
		}
		if(conf->configValue == pb->usb.cntl.WValue)
		{
			break;
		}
	
		conf = (void *)(((UInt32)conf) + USBToHostWord(conf->totalLength) );

	}
	if(subtractAddress(end, conf) < (9+9))	// smaller than 1 interface desc and 1 config desc
	{				// BT 18 Aug 98, Rainbow fix. Eliminate the =, so minimal descriptor can work
		return(kUSBNotFound);
	}

	// This check must be done after the one above.
	pb->usbStatus = isConfigDescriptor(conf);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	pb->usbActCount = subtractAddress(conf, pb->usbBuffer);
	pb->usbBuffer = conf;
	return(noErr);

}

/* xxxx make externally visible ? */
/* xxxx do some sanity checking on the dscriptor?? */
static OSStatus USBFindNextConfigDescriptorImmedite(USBPB *pb)
{
//	<--> usbBuffer			--> all config descriptors  <-- Config desc
//	 --> usbReqCount		total length of descriptors
//	<--  usbActCount		offset of this descriptor in config descriptor
//	<--> usb.cntl.WValue			-->Config number  <-- next config number
USBConfigurationDescriptor *conf;
void *end;
UInt32 remaining, confLen;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	
	end = (void *)(((UInt32)pb->usbBuffer) + pb->usbReqCount);
	/* find configuration */
	pb->usbStatus = USBFindConfigDescriptorImmedite(pb);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}
	
	conf = pb->usbBuffer;
	if(conf > end)
	{
		return(kUSBNotFound);
	}
	confLen = USBToHostWord(conf->totalLength);
	remaining = subtractAddress(end, conf) - confLen;
	if(remaining < 9+9)	// a config plus an interface desc again
	{
		return(kUSBNotFound);
	}
	pb->usbBuffer = (void *)( ((UInt32)conf) + confLen );
	pb->usbActCount += confLen;
	conf = pb->usbBuffer;

	pb->usbStatus = isConfigDescriptor(conf);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	pb->usb.cntl.WValue = conf->configValue;
	return(noErr);

}
/* xxxx make externally visible ? */
/* xxxx do some sanity checking on the dscriptor?? */
static OSStatus USBFindInterfaceDescriptorImmedite(USBPB *pb)
{
//	<--> usbBuffer			--> all config descriptors  <-- interface desc
//	 --> usbReqCount		total length of descriptors
//	<--  usbActCount		offset of this descriptor in config descriptor
//	 --> usb.cntl.WIndex			Interface number
//	 --> usb.cntl.WValue			Config number
//	 --> usbOther			alt. Set to 0xff to only find alt zero.
USBConfigurationDescriptor *conf;
USBInterfaceDescriptor *interface;
void *end;
UInt8 matchAlt;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	
	pb->usbStatus = USBFindConfigDescriptorImmedite(pb);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}

	/* we found the configuration descriptor with this number */
	
	conf = pb->usbBuffer;
	
	/* now find the right interface */
	
	end = (void *)(((UInt32)conf) + USBToHostWord(conf->totalLength) - 9 /* interface desc length */ );
	interface = nextDescriptor(conf);

	matchAlt = pb->usbOther;
	if(matchAlt == 0xff)
	{
		matchAlt = 0;
	}

	while(interface <= end)	// BT 18Aug98, add = so minimal descriptors can work.
	{
		if( (interface->descriptorType == kUSBInterfaceDesc) &&
			(interface->interfaceNumber == pb->usb.cntl.WIndex)   &&
			(interface->alternateSetting == matchAlt) 
		)
		{
			pb->usbActCount = subtractAddress(interface, pb->usbBuffer);
			pb->usbBuffer = interface;
			return(noErr);
		}
		
		interface = nextDescriptor(interface);
	}
	
	return(kUSBNotFound);
}



OSStatus USBFindNextEndpointDescriptorImmediate(USBPB *pb)
{

//	<--> usbBuffer			--> interface/endpoint descriptor  <-- endpoint desc
//	<--> usbReqCount		offset of this descriptor in config descriptor
//	<--  usbActcount		length of descriptor found
//	<--> usbDirection		direction/wild card
//	<--> usbClassType			Endpoint type/wild card
//	<--> usbOther			Endpoint number, start at zero
//	<--  usb.cntl.WValue			Max packet size of endpoint

// Note: usbReqCount is used for output here, the value needs to be fed back to the
// next call for this to work. In this sense, treat it like a magic cookie. It should
// be initialised to zero.

USBConfigurationDescriptor *conf;
USBInterfaceDescriptor *interface;
USBEndPointDescriptor *endPt, *end;
UInt8 endPointNum;
UInt8 direction;
UInt8 type;
USBDirection usbDirection;

	if(!checkPBVersion(pb, kUSBAnyDirn))
	{
		return(pb->usbStatus);
	}

	usbDirection = pb->usbFlags & kUSBAnyDirn;

	conf = (void *)((UInt8 *)pb->usbBuffer - pb->usbReqCount);
	if( (conf->descriptorType != kUSBConfDesc)
		|| (pb->usbReqCount == 0) )
	{
		pb->usbStatus = paramErr;
		return(pb->usbStatus);
	}
	end = (void *)(((UInt8 *)conf)+USBToHostWord(conf->totalLength));
	
	interface = (void *)pb->usbBuffer;
	
	if(interface->descriptorType == kUSBInterfaceDesc)
	{
		/* We have an interface descriptor */
		endPt = nextDescriptor(interface);
	}
	else
	{	/* We have an endPt desccriptor */
		endPt = pb->usbBuffer;
		if(endPt->descriptorType != kUSBEndpointDesc)
		{
			pb->usbStatus = paramErr;
			return(pb->usbStatus);
		}
		endPt = nextDescriptor(endPt);
	}

	while(endPt < end)
	{
		if(endPt->descriptorType == kUSBInterfaceDesc)
		{
			break;	/* not found */
		}
		if(endPt->descriptorType == kUSBEndpointDesc)
		{
			endPointNum = endPt->endpointAddress & 0xf;
			direction = endPt->endpointAddress >> 7;
			type = endPt->attributes & 3;

			if(
				( (usbDirection == kUSBAnyDirn) || (usbDirection == direction) ) &&
				( (pb->usbClassType == kUSBAnyType) || (pb->usbClassType == type) ) &&
				( (pb->usbOther == 0) || (pb->usbOther == endPointNum) ) 
			)
			{
				/* This matches */
				pb->usbBuffer = endPt;
				pb->usbReqCount = subtractAddress(endPt, conf);
				pb->usbActCount = endPt->length;
				pb->usbFlags = direction;
				pb->usbClassType = type;	/* Fix this sometime */
				pb->usbOther = endPointNum;
				pb->usb.cntl.WValue = USBToHostWord(endPt->maxPacketSize);
				return(noErr);
			}
		}
		
		endPt = nextDescriptor(endPt);
	}
	
	pb->usbBuffer = conf;
	pb->usbReqCount = 0;
	pb->usbActCount = 0;
	pb->usbFlags = kUSBAnyDirn;
	pb->usbClassType = kUSBAnyType;
	pb->usbOther = 0;
	return(kUSBNotFound);

}

static OSStatus uslFindNextInterfaceDescriptorImmediate(USBPB *pb)
{

//	<--> usbBuffer		--> config/interface descriptor  <-- interface desc
//	<--  usbActcount	length of descriptor found
//	<--> usbReqCount	offset of this descriptor in config descriptor start = 0
//	<--> usbClassType	Class/wild card
//	<--> usbSubclass	subclass/wildcard
//	<--> usbProtocol	Protocol/wildcard
//	<--  usb.cntl.WIndex		InterfaceNumber 
//	<--  usb.cntl.WValue		configuration value - Always return this, even on error
//	<--> usbWOther		alt. Set to Oxff to only find alt zero.


// Note: usbReqCount is used for output here, the value needs to be fed back to the
// next call for this to work. In this sense, treat it like a magic cookie. It should
// be initialised to zero.

USBConfigurationDescriptor *conf;
USBInterfaceDescriptor *interface, *end;
UInt8 confNum;

	conf = (void *)((UInt8 *)pb->usbBuffer - pb->usbReqCount);
	if(conf->descriptorType != kUSBConfDesc)
	{
		pb->usbStatus = paramErr;
		return(pb->usbStatus);
	}
	confNum = conf->configValue;
	end = (void *)(((UInt8 *)conf)+USBToHostWord(conf->totalLength));
	pb->usb.cntl.WValue = confNum;	// always return this
	
	if(pb->usbReqCount == 0)
	{
		/* We have a config descriptor */
		interface = nextDescriptor(conf);
	}
	else
	{	/* We have an interface desccriptor */
		interface = pb->usbBuffer;
		if(interface->descriptorType != kUSBInterfaceDesc)
		{
			pb->usbStatus = paramErr;
			return(pb->usbStatus);
		}
		interface = nextDescriptor(interface);
	}

	while(interface < end)
	{
		if(interface->descriptorType == kUSBInterfaceDesc)
		{
			if(
				( (pb->usbClassType == 0) || (pb->usbClassType == interface->interfaceClass) ) &&
				( (pb->usbSubclass == 0) || (pb->usbSubclass == interface->interfaceSubClass) ) &&
				( (pb->usbProtocol == 0) || (pb->usbProtocol == interface->interfaceProtocol) ) &&
				( (pb->usbOther != 0xff) || (interface->alternateSetting == 0) )
			)
			{
				/* This matches */
				pb->usbBuffer = interface;
				pb->usbReqCount = subtractAddress(interface, conf);
				pb->usbActCount = interface->length;
				pb->usb.cntl.WIndex = interface->interfaceNumber;
				pb->usbClassType = interface->interfaceClass;
				pb->usbSubclass = interface->interfaceSubClass;
				pb->usbProtocol = interface->interfaceProtocol;
				if(pb->usbOther != 0xff)
				{
					pb->usbOther = interface->alternateSetting;
				}
				return(noErr);
			}
		}
		
		interface = nextDescriptor(interface);
	}
	
	pb->usbBuffer = conf;
	pb->usbReqCount = 0;
	pb->usbActCount = 0;

	return(kUSBNotFound);

}

OSStatus USBFindNextInterfaceDescriptorImmediate(USBPB *pb)
{

//	<--> usbBuffer		--> config/interface descriptor  <-- interface desc
//	<--  usbActcount	length of descriptor found
//	<--> usbReqCount	offset of this descriptor in config descriptor start = 0
//	<--> usbClassType	Class/wild card
//	<--> usbSubclass	subclass/wildcard
//	<--> usbProtocol	Protocol/wildcard
//	<--  usb.cntl.WIndex		InterfaceNumber 
//	<--  usb.cntl.WValue		configuration value
//	<--> usbWOther		alt. Set to Oxff to only find alt zero.

// Note: usbReqCount is used for output here, the value needs to be fed back to the
// next call for this to work. In this sense, treat it like a magic cookie. It should
// be initialised to zero.
OSStatus err;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	
	err = uslFindNextInterfaceDescriptorImmediate(pb);
	if(err != noErr)
	{
		pb->usb.cntl.WIndex = 0;
		pb->usb.cntl.WValue = 0;
	}
	return(err);
}

/* xxxx Need a force unit access bit, or clear cahced descriptor function */
/* xxxx need to read configs other than zero */
static void *uslCacheConfigDescriptors(USBPB *pb)
{	/* make confid descriptors are cached. Return pointer when they are */
int stage;	
usbDevice *dev;
static countLoop, getLoop;

#if 0
	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);			
		return(nil);
	}
#endif

	// Don't use getDev here, if it goes wrong it calls back straight to here 
	dev = getDevicePtr(pb->usbReference);
	if(dev == nil)
	{
	pipe *p;
		// Maybe its a pipe ref. This is only used find assoc desc.
		// That should have the descriptors cached, so this is not going to be called too often
		(void)findPipe(pb->usbReference, &p);
		if(p != nil)
		{
			dev = getDevicePtr(p->devIntfRef);
		}
		if(dev == nil)
		{
			pb->usbStatus = kUSBUnknownDeviceErr;
			uslCompleteInternalStages(pb);		
			return(nil);
		}
	}
	
	if(dev->killMe)
	{
		USBExpertStatus(0,"USL - Killing dead device", 0);
		dev->killMe = 0;
#if 1
		dev->configLock = 0;
		uslClearInternalStage(pb);
		// Don't call back this has been hosed already?
#else		
		dev->configLock = 0;
		uslClearInternalStage(pb);
		pb->usbCompletion = (void *)pb->reserved2;
		uslDeleteDevice(pb, dev);
#endif
		return(nil);
	}

	
	if(dev->confValid)
	{	/* We should CRC the descriptor here */
		pb->usbActCount = dev->allConfigLen;
		pb->usbBuffer = dev->allConfigDescriptors;
		return(dev->allConfigDescriptors);
	}		

	/* xxx we need to lock out concurrent attempts to do this */


	do{
	stage = uslFindInternalStageAndIncriment(pb);	
	switch(stage)
	{
		case 1:
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
			pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);
			pb->usbBuffer = &dev->deviceDescriptor;
			
			if(immediateError(uslDeviceRequest(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
			
		break;
		
		case 2:
			/* use dev scratch to get the root config descritor */
			
			/* count length of all descriptors */
			dev->allConfigLen = 0;	
			
			pb->usbReqCount = sizeof(configHeader);
			pb->usbBuffer = &dev->configScratch;
			pb->usb.cntl.WValue = 0;
			countLoop = stage;

			continue;
		break;
		
		case 3:
			/* Get the descriptor */
			if(immediateError(uslReadConfigurationDescriptor(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;

		case 4:

			pb->usb.cntl.WValue &= 0xff;
			pb->usb.cntl.WValue++;
			if(pb->usbStatus == noErr)
			{
				dev->allConfigLen += USBToHostWord(((configHeader *)pb->usbBuffer)->totalLen);
				if(pb->usb.cntl.WValue < dev->deviceDescriptor.numConf) 
				{
					stage = countLoop;
					uslSetInternalStage(pb, stage);
				}
			}
			else
			{
				if(dev->allConfigLen == 0)
				{
					uslCompleteInternalStages(pb);			
					return(nil);
				}
			}
			continue;

		break;
		
		case 5:
			/* How long is full descritptor */
			pb->usbReqCount = dev->allConfigLen;

			/* Allocate the full descriptor */
			if(immediateError(uslAllocMem(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 6:
			if(pb->usbStatus != noErr)
			{
				uslCompleteInternalStages(pb);			
				return(nil);
			}
			dev->allConfigDescriptors = pb->usbBuffer;
			// note pb->usbReqCount still valid from alloc 
			/* Populate the full descriptor */
			getLoop =stage;
			pb->usb.cntl.WValue = 0;

			continue;
		break;
		
		case 7:
			
			if(immediateError(uslReadConfigurationDescriptor(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 8:
			pb->usb.cntl.WValue &= 0xff;
			pb->usb.cntl.WValue++;

			if(pb->usbStatus == noErr)
			{
				pb->usbBuffer = (void *)((UInt32)pb->usbBuffer + USBToHostWord(((configHeader *)pb->usbBuffer)->totalLen));
				pb->usbReqCount = dev->allConfigLen - subtractAddress(pb->usbBuffer, dev->allConfigDescriptors);
				if(pb->usbReqCount > dev->allConfigLen)	// we wrapped
				{
					pb->usbStatus = kUSBInternalErr;
				}
				if( (pb->usbReqCount != 0) && (pb->usb.cntl.WValue < dev->deviceDescriptor.numConf) )
				{
					stage = getLoop;
					uslSetInternalStage(pb, stage);
				}
			}
			else
			{
				if(dev->allConfigLen == 0)
				{
					uslCompleteInternalStages(pb);			
					return(nil);
				}
			}
			continue;
		break;
		
		case 9:
			dev->confValid = true;
			uslClearInternalStage(pb);
			pb->usbActCount = dev->allConfigLen;
			pb->usbBuffer = dev->allConfigDescriptors;
			return(dev->allConfigDescriptors);
		
		break;
		
		default:
			USBExpertStatus(0,"USL - cache conf handler, unknown case", 0);
			pb->usbStatus = kUSBInternalErr;
			uslCompleteInternalStages(pb);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
	return(nil);
}
#if 0
OSStatus USBFindInterface(USBPB *pb);		(Not implemented yet)

	 --> usbReference	device /interface????
    <--  usbBuffer      interface descriptor
    <--  usbActcount    length of descriptor found
	 --  usbReqCount	reserved, set to zero.
	 --> usbClassType	Class
	 --> usbSubclass	subclass
	 --> usbProtocol	Protocol
	<--> usb.cntl.WValue		configuration number   (start at zero)
	<--> usb.cntl.WIndex		InterfaceNumber  (start at zero)
	<--> usbOther		alt // (set to 0xff to find first alt zero only)
#endif

/* xxxx sort out usage of alt */
static void uslFindNextInterface(USBPB *pb)
{
OSStatus err = kUSBNotFound;
void *configDescriptors;
UInt32 totalLen;

	configDescriptors = uslCacheConfigDescriptors(pb);
	if(configDescriptors == nil)
	{
		return;
	}
	totalLen = pb->usbActCount;
	
	pb->usb.cntl.WIndex = pb->wIndexStash;
	pb->usb.cntl.WValue = pb->wValueStash;

	if( (pb->usb.cntl.WIndex == 0) && (pb->usb.cntl.WValue == 0) )
	{
		/* First call to this, init config descriptor */
		pb->usbReqCount = 0;
		pb->usbStatus = noErr;
	}
	else
	{
		pb->usbReqCount = pb->usbActCount;
		pb->usbStatus = USBFindInterfaceDescriptorImmedite(pb);
		pb->usbReqCount = pb->usbActCount;
	}
	while(pb->usbStatus == noErr)
	{
	USBConfigurationDescriptor *c;
		c = (void *)pb->usbBuffer;
		pb->usbStatus = uslFindNextInterfaceDescriptorImmediate(pb);
		if(pb->usbStatus == noErr)
		{
			if( (pb->addrStash == 0) ||
				(c->maxPower <= pb->addrStash) )
			{
					break;
			}
			err = kUSBDevicePowerProblem;
		}
			
		/* Not found in that config desc, try the next */
		pb->usbReqCount = totalLen;
		pb->usbBuffer = configDescriptors;
		pb->usbStatus = USBFindNextConfigDescriptorImmedite(pb);
		pb->usbReqCount = 0;
	}
	
	pb->usbActCount = 0;
	pb->usbBuffer = 0;
//	pb->usbReqCount = 0;
	pb->usbReqCount = pb->addrStash;
	
	if(pb->usbStatus != noErr)
	{
		pb->usb.cntl.WValue = 0;
		pb->usb.cntl.WIndex = 0;
	}

	if(pb->usbStatus == kUSBNotFound)
	{	// If an interface found without sufficient power, say power prob
		pb->usbStatus = err;
	}
	uslCompleteInternalStages(pb);
	return;

}

/* xxxx sort out usage of alt */
OSStatus USBFindNextInterface(USBPB *pb)
{
usbDevice *dev;
 
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	pb->addrStash = pb->usbReqCount;		// reqcount now has the power available
	pb->usbReqCount = 0;
	if(checkZeroBuffer(pb))
	{
		return(paramErr);
	}	
	dev = getDev(pb);
	if(dev == nil)
	{
		return(kUSBUnknownDeviceErr);
	}
	
	pb->wIndexStash = pb->usb.cntl.WIndex;
	pb->wValueStash = pb->usb.cntl.WValue;
	
	pb->usbStatus = noErr;
	uslInitialiseInternalStages(pb, uslFindNextInterface);
	return(kUSBPending);
}

static void uslGetConfigurationDescriptor(USBPB *pb)
{
USBConfigurationDescriptor *configDescriptors;
UInt32 confLen;
kprintf("uslGetConf:ref=0x%x\n",pb->usbReference);
	configDescriptors = uslCacheConfigDescriptors(pb);
	if(configDescriptors == nil)
	{
		return;
	}

/* note, we're relying on WValue being passed through here */

kprintf("uslGetConf after NIL:ref=0x%x\n",pb->usbReference);
	pb->usbReqCount = pb->usbActCount;
	pb->usbStatus = USBFindConfigDescriptorImmedite(pb);
	configDescriptors = pb->usbBuffer;
	confLen = USBToHostWord(configDescriptors->totalLength);
	pb->usbActCount = pb->usbReqCount;
	if(pb->usbActCount > pb->wValueStash)
	{
		pb->usbActCount = pb->wValueStash;
	}
	if(pb->usbActCount > confLen)
	{
		pb->usbActCount = confLen;
	}
	pb->usbReqCount = pb->wValueStash;
	pb->wIndexStash = pb->usb.cntl.WIndex;
	usb_BlockMoveData(pb->usbBuffer, (void *) pb->addrStash, pb->usbActCount);
	pb->usbBuffer = (void *)pb->addrStash;

	uslCompleteInternalStages(pb);
	return;

}

OSStatus USBGetConfigurationDescriptor(USBPB *pb)
{

//	 --> usbReference		Device
//	 --> usb.cntl.WValue			Configuration index
//	 --> usbBuffer			Configuration Descriptor Structure
//	 --> usbReqCount		size of edscriptor
//	<--  usbActCount		size of descriptor returned
usbDevice *dev;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	dev = getDev(pb);
	if(dev == nil)
	{
		return(kUSBUnknownDeviceErr);
	}
	pb->usbStatus = noErr;
	pb->wIndexStash = pb->usb.cntl.WIndex;
	pb->addrStash = (UInt32)pb->usbBuffer;
	pb->wValueStash = pb->usbReqCount;
	if(pb->usbReqCount > 0x7fff)
	{
		pb->wValueStash = 0x7fff;
	}
	uslInitialiseInternalStages(pb, uslGetConfigurationDescriptor);
	return(kUSBPending);
}

static Boolean noMoreDescriptors(USBDescriptorHeader *desc, void *end, Boolean endpoint)
{
	return(
	 (desc >= end) ||	/* run out of descriptors */
	 		/* BT 2Jul98, don't forget ==, caused hang of desc did not exist */
	 (desc->descriptorType == kUSBConfDesc) || /* got to next conf desc */
	 (desc->descriptorType == kUSBInterfaceDesc) || /* got to next interfc desc */
   ( (desc->descriptorType == kUSBEndpointDesc) && endpoint )
   		 /* got to next endpoint desc, for endpoint only*/
   		 );
}

static void uslFindNextAssociatedDescriptor(USBPB *pb)
{
USBConfigurationDescriptor *configDescriptors;
uslInterface *intrfc;
pipe *p;
USBInterfaceRef iRef;
usbDevice *dev;
UInt16 count;
UInt8 *end;
USBDescriptorHeader *desc;
UInt8 descType;

	configDescriptors = uslCacheConfigDescriptors(pb);
	if(configDescriptors == nil)
	{
		return;
	}

	pb->usbFlags = 0;
	end = (UInt8 *)configDescriptors + pb->usbActCount;
	pb->usbReqCount = pb->usbActCount;
	
	pb->usbStatus = findPipe(pb->usbReference, &p);
	if(pb->usbStatus == noErr)
	{
		iRef = p->devIntfRef;
	}
	else
	{
		p = nil;
		iRef = pb->usbReference;
	}
	pb->usbStatus = findInterface(iRef, &intrfc);
	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}

	/* ref was an interface or pipe ref (with interface container). */
	/* p not nil if pipe,  intrfc set */

	dev = getDevicePtr(iRef);
	if(dev == nil)
	{
		pb->usbStatus = kUSBUnknownDeviceErr;
		uslCompleteInternalStages(pb);
		return;
	}
	
	pb->usb.cntl.WIndex = intrfc->interfaceNum;
	pb->usb.cntl.WValue = dev->currentConfiguration;
	descType = pb->usbOther;
	pb->usbOther = intrfc->alt;
	pb->usbStatus = USBFindInterfaceDescriptorImmedite(pb);
	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}
	
	if(p != nil)
	{
		pb->usbReqCount = pb->usbActCount;
		pb->usbFlags = kUSBAnyDirn;
		pb->usbClassType = kUSBAnyType;
		pb->usbOther = p->endPt;
		pb->usbStatus = USBFindNextEndpointDescriptorImmediate(pb);
		if(pb->usbStatus != noErr)
		{
			uslCompleteInternalStages(pb);
			return;
		}
	}
	
	/* we're now pointing to the relevant descriptor to start with */
	pb->usb.cntl.WIndex = pb->wIndexStash;
	
	count = 0;
	desc = pb->usbBuffer;
	while(count < pb->usb.cntl.WIndex)
	{
		desc = nextDescriptor(desc);
		if(noMoreDescriptors(desc, end, (p != nil)))
		{
			pb->usbStatus = kUSBNotFound;
			uslCompleteInternalStages(pb);
			return;
		}
		count++;
	}

	pb->usb.cntl.WIndex++;	/* Next descriptor */
	
	if(descType != 0)
	{
		while(desc->descriptorType != descType)
		{
			desc = nextDescriptor(desc);
			pb->usb.cntl.WIndex++;
			if(noMoreDescriptors(desc, end, (p != nil)) ||
				(pb->usb.cntl.WIndex > 0x7ffe) )	// BT 2Jul98, if something goes wrong stop
			{
				pb->usbStatus = kUSBNotFound;
				uslCompleteInternalStages(pb);
				return;
			}
		}
	}
	
	/* now desc points to the descriptor to return */
	pb->usbOther = desc->descriptorType;
	pb->usbBuffer = (void *)pb->addrStash;
	pb->usbReqCount = pb->wValueStash;
	pb->usbActCount = desc->length;
	
	if(pb->usbActCount > pb->usbReqCount)
	{
		pb->usbActCount = pb->usbReqCount;
	}

	pb->wIndexStash = pb->usb.cntl.WIndex;
	pb->usb.cntl.WValue = 0;
	usb_BlockMoveData(desc, pb->usbBuffer, pb->usbActCount);

	uslCompleteInternalStages(pb);
	return;

}

OSStatus USBFindNextAssociatedDescriptor(USBPB *pb)
{
//	 --> usbReference		Interface/Endpoint
//	<--> usb.cntl.WIndex			Descriptor index (magic cookie) start at zero
//	 --> usbBuffer			Descriptor buffer
//	 --> usbReqCount		size of buffer
//	<--  usbActCount		size of descriptor returned
//	<--> usbOther           Descriptor type (zero matches any)


	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}

/* check interface/endpoint here */

	pb->usbStatus = noErr;
	pb->wIndexStash = pb->usb.cntl.WIndex;
	pb->addrStash = (UInt32)pb->usbBuffer;
	pb->wValueStash = pb->usbReqCount;
	if(pb->usbReqCount > 0x7fff)
	{
		pb->wValueStash = 0x7fff;
	}
	uslInitialiseInternalStages(pb, uslFindNextAssociatedDescriptor);
	return(kUSBPending);
}

#if 0
OSStatus USBOpenDevice(USBPB *pb);

	 --> usbReference	device
	 --> usb.cntl.WValue		configuration number (preserved)
	 --> usbFlags		Open interfaces immediately
	 --  usb.cntl.WIndex      is preserved (unusually)
	<--  usbValue4		number of interfaces in configuration
#endif

/* xxxx Check device power before config */
static void uslSetDeviceConfig(USBPB *pb)
{
//	<--> usbBuffer			--> Config desc
//	 --> usb.cntl.WValue			Config number
int stage;	
usbDevice *dev;
USBConfigurationDescriptor *conf;

	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);			
		return;
	}

	stage = uslFindInternalStageAndIncriment(pb);

	do{switch(stage)
	{
		case 1:
			pb->usbReqCount = 0;
		//	pb->usbBuffer = nil;	// preserve config desc

			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBNone, kUSBStandard, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqSetConfig;
		//	pb->usb.cntl.WValue = confNum;	// input param
			pb->usb.cntl.WIndex = 0;
kprintf("uslSetDeviceConfig:usbRef=0x%x\n",pb->usbReference);
			if(immediateError(uslDeviceRequest(pb)))
			{
				uslCompleteInternalStages(pb);		
			}
		break;
		
		case 2:
			dev = getDev(pb);
			if(dev == nil)
			{
				pb->usbStatus = kUSBUnknownDeviceErr;
			}
			/* We've configured the device, so what? */
			else
			{
				conf = pb->usbBuffer;
				pb->usbActCount = USBToHostWord(conf->totalLength);
				dev->currentConfiguration = pb->wValueStash;
				/* note complete stages now implictly unlcoks device */

			}
			
			/* restore these values, so find interface, getinterface ref works across here */
			pb->usb.cntl.WValue = pb->wValueStash;
			pb->usb.cntl.WIndex = pb->wIndexStash;

			pb->usbReqCount = 0;
			pb->usbActCount = 0;
			pb->usbBuffer = 0;

			uslCompleteInternalStages(pb);		
		break;
		
		default:
			USBExpertStatus(0,"USL - set conf handler, unknown case", 0);
			pb->usbStatus = kUSBInternalErr;
			pb->usbBuffer = (void *)pb->addrStash;	// for dev reqs diverted
			uslCompleteInternalStages(pb);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
	return;
}

/* xxxx Check device power before config */
static void uslOpenDevice2(USBPB *pb)
{
USBConfigurationDescriptor *conf;
usbDevice *dev;

	conf = uslCacheConfigDescriptors(pb);
	if(conf == nil)
	{
		return;
	}
	dev = getDev(pb);
	if(dev == nil)
	{
		return;
	}

	pb->usbReqCount = pb->usbActCount;

	pb->usb.cntl.WValue = pb->wValueStash;
	pb->usbStatus = USBFindConfigDescriptorImmedite(pb);

	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}
	conf = pb->usbBuffer;
	
	if(conf->maxPower > dev->powerAvailable)
	{
		USBExpertStatus(0,"USL - Not enough power. Available:", dev->powerAvailable);		
		USBExpertStatus(0,"USL - Not enough power. needed:", conf->maxPower);		
		
		pb->usbStatus = kUSBDevicePowerProblem;
		USBExpertSetDevicePowerStatus(pb->usbReference, 0, 0, kUSBDevicePower_BusPowerInsufficient, dev->powerAvailable, conf->maxPower);  // TC: <USB67>
		uslCompleteInternalStages(pb);
		return;
	}
	// pb->usb.cntl.WValue already set up. USBFindConfigDescriptorImmedite doesn't change it
	
	pb->usbOther = conf->numInterfaces;	// eventual output 
	pb->usbCompletion = (void *)pb->reserved2;		/* restore users handler */
	uslInitialiseInternalStages(pb, uslSetDeviceConfig);
	return;

}

OSStatus uslOpenDevice(USBPB *pb)
{
usbDevice *dev;

	/* This is called by dev req when a set config is requested */
	/* thats stashed the buffer, we need to restore the buffer */
	/* if stashed */
	dev = getDev(pb);
kprintf("uslOpenDevice:ref=%d,dev=0x%x\n",pb->usbReference,dev);
	if(dev == nil)
	{
		return(kUSBUnknownDeviceErr);
	}

	if(dev->ID != pb->usbReference)
	{
		/* Not a device ref */
		USBExpertStatus(0, "USL - Attempt to set config on interface", pb->usbReference);

		if(dev->currentConfiguration == pb->usb.cntl.WValue)
		{
			USBExpertStatus(0, "USL - setting current config", pb->usb.cntl.WValue);
			kprintf("uslOpenDevice:USL - setting current config\n");
			pb->usbStatus = noErr;
			(*pb->usbCompletion)(pb);
			return(kUSBPending);
		}
		
		return(kUSBUnknownDeviceErr);
	}

	if(!CompareAndSwap(0, 1, &dev->configLock))	/* complete stages now unlocks */
	{
		USBExpertStatus(0, "USL - Attempt to open busy device", dev->usbAddress);
		return(kUSBDeviceBusy);
	}
kprintf("uslOpenDevice:calling uslUnConfigureDevice\n");
	uslUnconfigureDevice(dev);
	
	pb->usbStatus = noErr;
	pb->wValueStash = pb->usb.cntl.WValue;
	pb->wIndexStash  = pb->usb.cntl.WIndex;
kprintf("uslOpenDevice:calling uslInitialiseInternalStages\n");
	uslInitialiseInternalStages(pb, uslOpenDevice2);
	return(kUSBPending);
}

/* xxxx Check device power before config */
/* xxxx sort out config value in usb.cntl.WValue or usbValue4 */
OSStatus USBSetConfiguration(USBPB *pb)
{
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	if(checkZeroBuffer(pb))
	{
		return(paramErr);
	}
	pb->addrStash = 0;	// so crap doesn't get restored 
	return(uslOpenDevice(pb));
}

OSStatus USBOpenDevice(USBPB *pb)
{
	return(USBSetConfiguration(pb));
}
#if 0
OSStatus USBNewInterfaceRef(USBPB *pb)

//	<--> usbReference	 --> device 	<-- interface
// 	 --> usb.cntl.WIndex		InterfaceNumber
#endif

/* xxxx sort out alt usage */
static void uslNewInterfaceRef(USBPB *pb)
{
USBConfigurationDescriptor *conf;
uslInterface *intrfc;
usbDevice *dev;
UInt8 oldOther;

	conf = uslCacheConfigDescriptors(pb);
	if(conf == nil)
	{
		return;
	}
	dev = getDev(pb);
	if(dev == nil)
	{
		return;
	}
	
	pb->usbReqCount = pb->usbActCount;
	pb->usb.cntl.WValue = dev->currentConfiguration;
	//WIndex is a param
	oldOther = pb->usbOther;
	pb->usbOther = 0xff;	// Ignore alt
	pb->usbStatus = USBFindInterfaceDescriptorImmedite(pb);
	pb->usbOther = oldOther;
	
	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}

	/* Specified interface does exist */
	
	intrfc = AllocInterface(pb->usbReference);
kprintf("uslNewInterfaceRef: intrfc=%d\n",intrfc);
	if(intrfc == 0)
	{
		pb->usbStatus = kUSBOutOfMemoryErr;
		uslCompleteInternalStages(pb);
		return;
	}
	
	pb->usbReference = intrfc->ref;
	intrfc->interfaceNum = pb->wIndexStash;
	
	pb->usbStatus = noErr;
	pb->usbReqCount = 0;
	pb->usbActCount = 0;
	pb->usbBuffer = 0;
	
	/* note complete stages now unlocks device */
	uslCompleteInternalStages(pb);
}

#if 0
OSStatus USBNewInterfaceRef(USBPB *pb);


	<--> usbReference	 --> device 	<-- interface
 	 --> usb.cntl.WIndex		InterfaceNumber
	 --> usbFlags
#endif

/* xxxx sort out alt usage */
OSStatus USBNewInterfaceRef(USBPB *pb)
{
usbDevice *dev;
uslInterface *intrfc;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	if(checkZeroBuffer(pb))
	{
		return(paramErr);
	}	
	pb->usbStatus = findInterface(pb->usbReference, &intrfc);
	if(pb->usbStatus == noErr)
	{	// BT 2Jul98, don't allow interface ref here, device ref only.
		pb->usbStatus = kUSBUnknownDeviceErr;
		return(kUSBUnknownDeviceErr);
	}
	dev = getDev(pb);
	if(dev == nil)
	{
		return(kUSBUnknownDeviceErr);
	}
	if(!CompareAndSwap(0, 1, &dev->configLock))
	{
		USBExpertStatus(0, "USL - USBNewInterfaceRef, device busy", dev->usbAddress);
		return(kUSBDeviceBusy);
	}

	pb->usbStatus = noErr;
	pb->wIndexStash = pb->usb.cntl.WIndex;
kprintf("*** USBNewInterfaceRef:ref=0x%x\n",pb->usbReference);
	uslInitialiseInternalStages(pb, uslNewInterfaceRef);
	return(kUSBPending);
}

/* xxxx sort out alt usage */
static void uslConfigureInterface(USBPB *pb)
{
USBConfigurationDescriptor *conf;
uslInterface *intrfc;
usbDevice *dev;
UInt16 pipeCount;

	conf = uslCacheConfigDescriptors(pb);
	if(conf == nil)
	{
		return;
	}
	pb->usbStatus = findInterface(pb->usbReference, &intrfc);
	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}
	dev = getDev(pb);
	if(dev == nil)
	{
		return;
	}
	
	uslCloseInterfacePipes(intrfc->ref, dev);
	
	pb->usb.cntl.WValue = pb->wValueStash;

	pb->usbReqCount = pb->usbActCount;
	pb->usb.cntl.WValue = dev->currentConfiguration;
	pb->usb.cntl.WIndex = intrfc->interfaceNum;
	pb->usbStatus = USBFindInterfaceDescriptorImmedite(pb);

	if(pb->usbStatus != noErr)
	{
		uslCompleteInternalStages(pb);
		return;
	}
	
	intrfc->alt = pb->usbOther; // remember alt
	pb->usbOther = 0; // count of endpoints in interface
	pb->usbReqCount = pb->usbActCount;

	pipeCount = 0;
	do{
		pb->usbFlags = kUSBAnyDirn;
		pb->usbClassType = kUSBAnyType;
		pb->usbOther = 0;	// don't match against endpoint number
		
		pb->usbStatus = USBFindNextEndpointDescriptorImmediate(pb);
		if(pb->usbStatus == noErr)
		{
			pb->usbStatus = uslOpenPipeImmed(intrfc->ref, pb->usbBuffer);
			if(pb->usbStatus == noErr)
			{
				pipeCount++;
			}
			else
			{

				pb->usbReqCount = 0;
				pb->usbActCount = 0;
				pb->usbBuffer = 0;

				uslCompleteInternalStages(pb);
				return;
			}
		}
		
	}while(pb->usbStatus == noErr);
	
	pb->usbStatus = noErr;

	pb->usbReqCount = 0;
	pb->usbActCount = 0;
	pb->usbBuffer = 0;

	pb->usbOther = pipeCount;
	
	uslCompleteInternalStages(pb);
}

/* xxxx sort out alt usage */
OSStatus USBConfigureInterface(USBPB *pb)
{
//	<--> usbReference	interface
//	 --> usbW???		alt
//	 --> usbFlags
//	<--  usbOther		Number of pipes in interface
uslInterface *intrfc;

	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	if(checkZeroBuffer(pb))
	{
		return(paramErr);
	}	

	pb->usbStatus = findInterface(pb->usbReference, &intrfc);
	if(pb->usbStatus != noErr)
	{
		return(pb->usbStatus);
	}
	pb->usbStatus = noErr;
	pb->wValueStash = pb->usb.cntl.WValue;
	uslInitialiseInternalStages(pb, uslConfigureInterface);
	return(kUSBPending);

}

OSStatus uslUnconfigureDeviceByReference(USBDeviceRef ref)
{
usbDevice *dev;

	dev = getDevicePtr(ref);
	if(dev == nil)
	{
		return(kUSBUnknownDeviceErr);
	}
	uslUnconfigureDevice(dev);
	USBAbortPipeByReference(dev->pipe0);
	return(noErr);

}



/* xxxx test this with mor ethan one interface */
OSStatus USBFindNextPipe(USBPB *pb)
{

//	<--> usbReference	 --> interface/pipe	<-- Pipe
//	<--> usbFLags		direction/wild card
//	<--> usbValue1		Endpoint type/wild card
uslInterface *intrfc;
usbDevice *dev;
pipe *p;
USBReference container;
int idx;


	if(!checkPBVersion(pb, kUSBAnyDirn))
	{
		return(pb->usbStatus);
	}
	if(checkZeroBuffer(pb))
	{
		return(paramErr);
	}	

	pb->usbStatus = findInterface(pb->usbReference, &intrfc);
	if(pb->usbStatus == noErr)
	{
		idx = 0;
		dev = getDev(pb);
		if(dev == nil)
		{
			return(kUSBUnknownDeviceErr);
		}
		container = pb->usbReference;
	}
	else
	{
		pb->usbStatus = findPipe(pb->usbReference, &p);
		if(p == nil)
		{
			return(pb->usbStatus);
		}
		container = p->devIntfRef;
		dev = getDevicePtr(container);
		if(dev == nil)
		{
			return(kUSBUnknownPipeErr);
		}

		idx = makeDevPipeIdx(p->endPt, p->direction)+1;
		if(idx > kUSBMaxEndptPerDevice)
		{		
			return(kUSBNotFound);
		}
	}
	
	/* Now have a starting point in devices pipe table */
	
	for(; idx < kUSBMaxEndptPerDevice; idx++)
	{
		if(dev->pipes[idx] != -1)
		{
			p = getPipeByIdx(dev->pipes[idx]);
			if(p == nil)
			{
				return(kUSBInternalErr);
			}
			if( (container == p->devIntfRef) &&
				((pb->usbFlags == kUSBAnyDirn) || (pb->usbFlags == p->direction)) &&
				((pb->usbClassType == kUSBAnyType) || (pb->usbClassType == p->type)) )
			{
				pb->usbStatus = noErr;
				pb->usbReference = p->ref;
				pb->usbFlags = p->direction;
				pb->usbClassType = p->type;
				pb->usb.cntl.WValue = p->maxPacket;

				pb->usbReqCount = 0;
				pb->usbActCount = 0;
				pb->usbBuffer = 0;

				(*pb->usbCompletion)(pb);

				return(kUSBPending);
			}
		}
	}
	pb->usbReqCount = 0;
	pb->usbActCount = 0;
	pb->usbBuffer = 0;
	pb->usb.cntl.WValue = 0;
	return(kUSBNotFound);

}

