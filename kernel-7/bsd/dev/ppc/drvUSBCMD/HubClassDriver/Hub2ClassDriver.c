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
 This file was created 12/1/98 by Adam Wang to make a duplicate hub driver
 for the root hub as well as the keyboard's built-in hub.  This is a duplicate
 of HubClassDriver.c except some name changes simulate the way CFM handles
 global data
*/

#include "../USB.h"
                 
#include "hub.h"
#include "../driverservices.h"
  

enum{	/* States the hub driver can be in */
	kNormal = 0,
	kDead,
	kDeviceZero,
	kDeadDeviceZero,
	kSetAddress,
	kSetAddressFailed
	};

enum{
	kHubIsCompound = 0x040	// Hub decriptor characteristics
	};

enum{
	kMaxPorts = 4,		// Initial allocation of ports, can be expanded
	kInitialRetries = 4
	};



typedef struct{
	UInt16 status;
	UInt16 change;
	} HubStatus;


typedef struct{
	UInt32 (*handler)(USBPB *pb);
	UInt32 bit;
	UInt32 clearFeature;
	}portStatusChangeVector;


static UInt32 defDoNothingChangeHandler(USBPB *pb);
static UInt32 deadConnectionChangeHandler(USBPB *pb);
static UInt32 defOverCrntChangeHandler(USBPB *pb);
static UInt32 defResetChangeHandler(USBPB *pb);
static UInt32 defSuspendChangeHandler(USBPB *pb);
static UInt32 defEndableChangeHandler(USBPB *pb);
static UInt32 defConnectionChangeHandler(USBPB *pb);
static OSStatus HubAreWeFinishedYet(void);
static OSStatus killHub(USBDeviceRef device);

static portStatusChangeVector defaultPortVectors[]={
	{defOverCrntChangeHandler, kHubPortOverCurrent, kUSBHubPortOverCurrentChangeFeature},
	{defResetChangeHandler, kHubPortBeingReset, kUSBHubPortResetChangeFeature},
	{defSuspendChangeHandler, kHubPortSuspend, kUSBHubPortSuspendChangeFeature},
	{defEndableChangeHandler, kHubPortEnabled, kUSBHubPortEnableChangeFeature},
	{defConnectionChangeHandler, kHubPortConnection, kUSBHubPortConnectionChangeFeature},
	};

enum{
	numChangeHandlers = sizeof(defaultPortVectors)/sizeof(portStatusChangeVector),
	

	kVectorContinueImmed = 0,
	kVectorContinueDelayed = 1,
	kVectorfinishedImmed = 2,
	kVectorNotfinished = 3,
	
	kPortWhichVectorShift = 8,
	kPortWhichVector1 = (1<<kPortWhichVectorShift),
	kPortHandlerStageMask = kPortWhichVector1-1,

	kPortVectorStageShift = 16,
	kPortVectorStage1 = (1<<kPortVectorStageShift),
	kPortVectorStageMask = kPortVectorStage1-1
	};

typedef struct{
	USBPB 				pb;
	USBPB				*portRequestPB;
	portStatusChangeVector changeHandler[numChangeHandlers];
	unsigned char 		*errorString;
	USBDeviceDescriptor desc;
	USBHubPortStatus 	portStatus;
	USBDeviceRef 		newDevRef;
	USBDeviceRef 		devZeroRef;
	UInt16				value;
	UInt16				state;
	int 				portIndex;
	int 				onError;
	int 				retries;
	int					delay;
	UInt8				portByte;
	UInt8				portMask;
	}perPort;



static perPort staticPorts[kMaxPorts+1], *ports = staticPorts;
static int numPorts;
static UInt32 busPowerAvail;
static UInt32 powerForCaptive;
static Boolean startExternal;
static Boolean selfPowered;
static Boolean selfPowerGood;
static Boolean busPowered;
static Boolean busPowerGood;
static HubStatus hubStatus;
static UInt16 deviceStatus;
static UInt32 hubSubClass = 1;
static UInt8 intPipeNumber;
static USBDeviceRef hubRef, hubDevRef;
static UInt16 intPipeMaxPacket;
static hubDescriptor hubDesc;
static UInt32 intFrame;
static numCaptive;
static UInt32 errataBits;
#if 1



static void hubAddDeviceHandler(USBPB *pb);
static void doPortResetHandler(USBPB *pb);


	/* Incorporate debugging strings */
#define noteError(s)	pp->errorString = s;

#else

	/* eliminate Error strings */
#define noteError(s)

#endif

/*
	This is copied wholesale from the UIM. 

  This table contains the list of errata that are necessary for known problems with particular hub
  The format is vendorID, product ID, lowest revisionID needing errata, highest rev needing errata, errataBits
  The result of all matches is ORed together, so more than one entry may match.  Typically for a given errata a
  list of hubs revisions that this applies to is supplied.
*/
enum{
	kErrataCaptiveOK = 1
	};

typedef struct {
	UInt16 						vendID;
	UInt16 						deviceID;
	UInt16 						revisionLo;
	UInt16 						revisionHi;
	UInt32 						errata;
}ErrataListEntry;

static ErrataListEntry	errataList[] = {

/* For the Cherry 4 port KB, From Cherry:
We use the bcd_releasenumber-highbyte for hardware- and the lowbyte for
firmwarestatus. We have 2 different for the hardware 03(eprom) and
06(masked microcontroller). Firmwarestatus is 05 today.
So high byte can be 03 or 06 ----  low byte can be 01, 02, 03, 04, 05

Currently we are working on a new mask with the new descriptors. The
firmwarestatus will be higher than 05. 
*/
	{0x046a, 0x003, 0x0301, 0x0305, kErrataCaptiveOK}, // Cherry 4 port KB
	{0x046a, 0x003, 0x0601, 0x0605, kErrataCaptiveOK}  // Cherry 4 port KB
};

#define errataListLength (sizeof(errataList)/sizeof(ErrataListEntry))

static UInt32 GetErrataBits(USBDeviceDescriptor *desc)
{
	UInt32				vendID, deviceID, revisionID;
	ErrataListEntry		*entryPtr;
	UInt32				i, errata = 0;
	
	// get this chips vendID, deviceID, revisionID
	vendID = USBToHostWord(desc->vendor);
	deviceID = USBToHostWord(desc->product);
	revisionID = USBToHostWord(desc->devRel);
	for(i=0, entryPtr = errataList; i<errataListLength; i++, entryPtr++){
		if (vendID == entryPtr->vendID && deviceID == entryPtr->deviceID &&
			revisionID >= entryPtr->revisionLo && revisionID <= entryPtr->revisionHi){
				errata |= entryPtr->errata;  // we match, add this errata to our list
		}
	}
	return(errata);
}

static Boolean bit(long value, long mask)
{
	return( (value & mask) != 0);
}

static Boolean immediateError(OSStatus err)
{
Boolean result;
	result = false;
	if((err != kUSBPending) && (err != noErr))
	{
		//Debugger();
		result = true;
	}
//	return((err != kUSBPending) && (err != noErr) );
	return(result );
}

static void initPortVectors(portStatusChangeVector *changeHandler)
{
int vector;
	for(vector = 0; vector < numChangeHandlers; vector++)
	{
		changeHandler[vector] = defaultPortVectors[vector];
	}
}

static void setDeadPortVectors(portStatusChangeVector *changeHandler)
{
int vector;
	initPortVectors(changeHandler);
	for(vector = 0; vector < numChangeHandlers; vector++)
	{
		changeHandler[vector].handler = defDoNothingChangeHandler;
	}
}


static void setPortVector(portStatusChangeVector *changeHandler, void *routine, UInt32 condition)
{
int vector;
	for(vector = 0; vector < numChangeHandlers; vector++)
	{
		if(condition == changeHandler[vector].bit)
		{
			if(routine == nil)
			{
				changeHandler[vector].handler = defaultPortVectors[vector].handler;
			}
			else
			{
				changeHandler[vector].handler = routine;
			}
		}
	}
}


static void HubFatalError(perPort *pp, OSStatus err, unsigned char *str, UInt32 num)
{
//	Debugger();
	USBExpertFatalError(hubDevRef, err, str, num);

	if(!pp->state != kDead)
	{
		USBExpertStatus(hubDevRef, "******* DEVICE IS DEAD ********", 0);
		if(pp->newDevRef != 0)
		{
			pp->pb.usbReference = pp->newDevRef;	
			pp->pb.usbCompletion = kUSBNoCallBack;
			(void)USBHubDeviceRemoved(&pp->pb);
			USBExpertRemoveDeviceDriver(pp->newDevRef);
		}
	}
	pp->pb.usbRefcon = 0;
	pp->delay = 0;
	pp->state = kDead;
	setDeadPortVectors(pp->changeHandler);
	setPortVector(pp->changeHandler, deadConnectionChangeHandler, kHubPortConnection);
	pp->newDevRef = 0;
	pp->retries = kInitialRetries;
}

static OSStatus HubAreWeFinishedYet(void)
{
enum{
	hubTimeOut = 256
	};
USBPB pb={0,0, sizeof(USBPB), kUSBCurrentPBVersion, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0};
int i;
OSStatus retVal = noErr;
static Boolean finished;
static UInt32 firstFrame=0, askCount=0;

	if(finished)
	{
		return(kUSBNoErr);
	}

	if(firstFrame == 0)
	{
		pb.usbCompletion = kUSBNoCallBack;
		pb.usbReference = hubDevRef;
		USBGetFrameNumberImmediate(&pb);
		firstFrame = pb.usbFrame;
	}
	askCount++;
	
//	USBExpertStatus(hubDevRef, "Hub driver - Are we finished", hubDevRef);
	if(intFrame == 0)
	{
		//USBExpertStatus(hubDevRef, "Hub driver - Not got to int handler yet", 0);
		retVal = kUSBDeviceBusy;
	}

	if(retVal == noErr)
	{
		if(intFrame == 0xffffffff)
		{
			//USBExpertStatus(hubDevRef, "Hub driver - Int already happened", 0);
		}
		else
		{
			pb.usbCompletion = kUSBNoCallBack;
			pb.usbReference = hubDevRef;
			if(USBGetFrameNumberImmediate(&pb) != noErr)
			{
				USBExpertStatus(hubDevRef, "Hub driver - Failed to get frame", pb.usbStatus);
				intFrame = 0xffffffff;
				pb.usbFrame = 0;
			}
			//USBExpertStatus(hubDevRef, "Hub driver - num frames", pb.usbFrame - intFrame);
			if( (pb.usbFrame - intFrame) < hubTimeOut)
			{
				//USBExpertStatus(hubDevRef, "Hub driver - no timeout yet", 0);
				retVal = kUSBDeviceBusy;
			}
		}
	}
	if(retVal == noErr)
	{
		//USBExpertStatus(hubDevRef, "Hub driver - check if any port busy", 0);
		finished = true;
		for(i = 1; i<=numPorts; i++)
		{
			if(ports[i].pb.usbRefcon != 0)
			{
				//USBExpertStatus(hubDevRef, "Hub driver - port busy:", i);
				//USBExpertStatus(hubDevRef, "Hub driver - port ref:", ports[i].pb.usbRefcon);
				finished = false;
			}
		}
	}

	if(retVal != noErr)
	{
		finished = false;
		return(retVal);
	}
	if(finished && (firstFrame != 0xffffffff) )
	{
#if 0
		if(intFrame == 0xffffffff)
		{
			USBExpertStatus(hubDevRef, "Hub driver - Finished Enumeration Int happened", hubDevRef);
		}
		else
		{
			USBExpertStatus(hubDevRef, "Hub driver - Finished Enumeration Timed out", hubDevRef);
		}
#endif
//		USBExpertStatus(hubDevRef, "Hub driver - Ask count", askCount);
		pb.usbCompletion = kUSBNoCallBack;
		pb.usbReference = hubDevRef;
		USBGetFrameNumberImmediate(&pb);
//		USBExpertStatus(hubDevRef, "Hub driver - Ask time", pb.usbFrame - firstFrame);
		firstFrame = 0xffffffff;
	}
	
	return(finished?kUSBNoErr:kUSBDeviceBusy);
}


static void detachDevice(USBPB *pb)
{
perPort *pp;	
	pp = (void *)pb;	/* parameter block has been extended */

	if(pb->usbStatus != noErr)
	{
		if( (pp->onError == 0) && (pp->retries == 0) )
		{
			/* no idea what to do now?? */
			USBExpertFatalError(hubDevRef, pb->usbStatus, pp->errorString, 1);
			
			pb->usbRefcon = 0;
			/* Mark port as errored */
			return;
		}
		else
		{
			USBExpertStatus(hubDevRef, pp->errorString, 2);
			pb->usbRefcon = pp->onError;
			pp->retries--;

			if(pp->retries == 1)
			{
				pp->delay = -3;
			}
			else if(pp->retries == 0)
			{
				pp->delay = -30;
			}
		/* we'll delay comming back to here */
			pb->usbReqCount = 0;
			/* pb->usbFlags = kUSBtaskTime */
//			USBDelay(pb);
		}
		
	}

	pp->onError = 0;
	if(pp->delay != 0)
	{
		pp->delay = -pp->delay;
		if(pp->delay > 0)
		{
			pb->usbReqCount = pp->delay;
			USBDelay(pb);
			return;
		}
	}
	
	
	do{switch(pb->usbRefcon++)
	{
		case 1:
			/* Power off the port */
			
			/* Maybe should only do this if there is power switching */
			
			pb->usbReference = hubRef;
			
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqClearFeature;
			pb->usb.cntl.WValue = kUSBHubPortPowerFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;			
			
			noteError("Hub Driver - Powering off port");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;

		case 2:
			pb->usbReqCount = 100;	// wait 100ms
			USBDelay(pb);
		break;
		
		case 3:
			/* Power on the port */
			
			/* Maybe should only do this if there is power switching */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqSetFeature;
			pb->usb.cntl.WValue = kUSBHubPortPowerFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;			

			noteError("Hub Driver - Powering on port again");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;
		
		case 4:
			pp->state = kDead;
			pb->usbRefcon = 0;			
		break;
		
		default:
			noteError("Hub dead Driver Error - Unused case in detach device");			
			USBExpertFatalError(hubDevRef, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */


}


static Boolean handleError(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */	
	if(pb->usbStatus != noErr)
	{
		if( (pb->usbStatus == kUSBAbortedError) && (pp->state != kDead) &(pp->state != kDeadDeviceZero) )
		{
			/* If the hub watchdog times this out, we may get an aborted error */
			/* We do need to do something about this */
			if(pp->state == kDeviceZero)
			{
				pb->usbStatus = noErr;
				pp->state = kDeadDeviceZero;
				return(false);
			}
			USBExpertFatalError(hubDevRef, hubDevRef, "Hub driver - Processing Aborted - Exiting", pb->usbStatus);
			pb->usbRefcon = 0;
			return(true);
		}
		if( (pp->onError == 0) || (pp->retries == 0) )
		{
			if(pp->state == kDeviceZero)
			{
				pp->state = kDeadDeviceZero;
				return(false);
			}
			else if(pp->state == kSetAddress)
			{
				/* set address failed, need to turn off this device */
				USBExpertStatus(hubDevRef, "Hub driver - set address failed, turning off device", 0);
				pp->retries = kInitialRetries;
				pb->usbRefcon = 1;
				pb->usbStatus = noErr;
				pp->state = kSetAddressFailed;
				pb->usbFlags = 0;
				pp->delay = 0;
				pb->usbCompletion = detachDevice;
				detachDevice(pb);
				return(true);
			}
			
			if(pp->state == kDead)
			{
				USBExpertFatalError(hubDevRef, pb->usbStatus, pp->errorString, 1);
			}
			else
			{
				/* no idea what to do now?? */
				HubFatalError(pp, pb->usbStatus, pp->errorString, 1);
			}
			pb->usbRefcon = 0;
			return(true);
		}
		else
		{
			USBExpertStatus(hubDevRef, pp->errorString, 2);
			pb->usbRefcon = pp->onError;
			pp->retries--;
			
			if(pp->retries == 1)
			{
				pp->delay = -3;
			}
			else if(pp->retries == 0)
			{
				pp->delay = -30;
			}
		}		
	}

	pp->onError = 0;
	
	if(pp->delay != 0)
	{
		pp->delay = -pp->delay;
		if(pp->delay > 0)
		{
			pb->usbReqCount = pp->delay;
			USBDelay(pb);	// Should check for error here
			return(true);
		}
	}
	
	return(false);	/* tell function to carry on */
}

static UInt32 resetChangeHandler(USBPB *pb)
{
perPort *pp;

	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon >> kPortVectorStageShift)
	{		
		case 1:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}
			
			if(bit(pp->portStatus.portFlags, kHubPortBeingReset))
			{
				USBExpertStatus(hubDevRef, "Hub Driver Error - Port not finished resetting, retrying", pb->usbRefcon);
		  // we should never be here, just wait for another status change int
				return(kVectorContinueImmed);
			}
			
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}


			/* Now wait 10 ms after reset */ 
			/* Do fancy things with perPort to shorten this when looping */
			
			pb->usbReqCount = 7;	// This should be ready in 10ms
									// However devices don't have to respond for another 10ms
									// The set address below, won't arrive before 20ms
									// Thats 17 + at least 3ms one each for each of 3 controlls

			//pb->usbReqCount = 10;	// On root hub things are happening too quickly.
									// I'll have to fix that, in the meantime amke it 20

			noteError("Hub Driver - Calling USBDelay");			
			//USBExpertStatus(hubDevRef, "Hub Driver - calling delay", pb->usbReqCount);
			if(immediateError(USBDelay(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				resetChangeHandler(pb);
			}
		break;
		
		case 2:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}

			pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			//pb->usb.cntl.WValue = 8;	// max packet size 
			
			// This should make no difference. For 1.1 max packet size discovery
			pb->usb.cntl.WValue = 64;	// max packet size 

			pb->usbReference = pp->newDevRef;

			noteError("Hub Driver Error - configuring endpoint zero");			
			if(immediateError(USBHubConfigurePipeZero(pb)))
			{
				USBExpertStatus(hubDevRef, "Config pipe zero failed", pb->usbStatus);
				pb->usbRefcon += kPortVectorStage1;
				resetChangeHandler(pb);
			}		
		break;

		case 3:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}
			pb->usbFlags = 0;	// tidy up after above

			/* now do a device request to find out what it is */


			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
			pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
			pb->usb.cntl.WIndex = 0;
			//pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);

			// For 1.1 max packet size discovery, we're relying on numConf being zero here
			pb->usbReqCount = 8;
			pp->desc.numConf = 0;	// This is the flag to read the rest.
			// Do not forget numConf for 1.1 discovery, or take out check below

			/* did we get here because the get descriptor failed? */
			if(pb->usbStatus == kUSBOverRunErr)
			{	/* endpoint zero max packet too large. Read first */
				/* 8 bytes to get the value you need */
				USBExpertStatus(hubDevRef, "Hub driver - overrun error reading device descriptor:", pb->usbActCount);
				pb->usbReqCount = 8;
				pp->desc.numConf = 0;	// make a note to read all of this
			}

			pb->usbBuffer = &pp->desc;
			
			pp->onError = pb->usbRefcon-kPortVectorStage1;	/* do a retry on error */
				/* the get descriptor seems to screw up most often */
			noteError("Hub Driver Error - kUSBRqGetDescriptor (get device descriptor after a SetAddress)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				resetChangeHandler(pb);
			}
		break;
		
		case 4:
			if( (hubDesc.removablePortFlags[pp->portByte] & pp->portMask) != 0)
			{
				pb->usbOther = powerForCaptive;
			}
			else
			{
				pb->usbOther = selfPowerGood?kUSB500mAAvailable:kUSB100mAAvailable;
			}
			

			/* Now address the device */
			
			pb->usb.cntl.WValue = pp->desc.maxPacketSize;
			pp->newDevRef = 0;		// if set address fails, don't try to remove old
			pp->onError = pb->usbRefcon;	/* Go back to start, via next case */
			pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			pb->usbFlags |= kUSBHubPower;
			
			pp->state = kSetAddress;	/* If this fails, detatch */
			noteError("Hub Driver Error - Setting the device address");			
			if(immediateError(USBHubSetAddress(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				resetChangeHandler(pb);
			}

		break;
		
		case 5:
			if( (pp->state == kDeadDeviceZero) || (pb->usbReference == 0) )
			{
				noteError("Hub Driver Error - dead set address");			
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				return(kVectorfinishedImmed);
			}

			pb->usbFlags = 0;
			pp->state = kNormal;	/* no longer need special handling to avoid deadlock */

			if(pb->usbStatus != noErr)
			{
				/* an error setting the addres, so go back and try resetting again */
				pb->usbRefcon = 1;
				pb->usbStatus = noErr;
				pb->usbCompletion = hubAddDeviceHandler;
				pb->usbReqCount = 1;
				setPortVector(pp->changeHandler, 0, kHubPortBeingReset);
				USBDelay(pb);
				break;
			}

			/* refernce is now new device ref, no longer dev zero */

			pp->newDevRef = pb->usbReference;
	
			if(pp->desc.numConf == 0)	// don't have full descriptor, try again */
			{
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
				pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
				pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
				pb->usb.cntl.WIndex = 0;
				pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);
				pb->usbBuffer = &pp->desc;
				
				pp->onError = pb->usbRefcon-kPortVectorStage1;	/* do a retry on error */
					/* the get descriptor seems to screw up most often */
				noteError("Hub Driver Error - kUSBRqGetDescriptor (get device descriptor after a SetAddress)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}
			}
			else
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;
			}
		break;
		
		case 6:

			/* FInally use the data gathered */
			USBExpertInstallDeviceDriver(pb->usbReference, &pp->desc, hubDevRef, pp->portIndex, pb->usbOther);
			/* Call to the expert */
			
			setPortVector(pp->changeHandler, 0, kHubPortBeingReset);
			return(kVectorfinishedImmed);
		break;
		
		default:
			noteError("Hub Driver Error - Unused case in reset change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */

	return(kVectorNotfinished);
}

static void hubAddDeviceHandler(USBPB *pb)
{
perPort *pp;

	if(handleError(pb))
		return;
	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon++)
	{
	
		case 1:
			pb->usbReference = hubRef;
		
			/* Check if the port is suspended */
			if(bit(pp->portStatus.portFlags, kHubPortSuspend))
			{
				/* resume the port */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);
				pb->usb.cntl.BRequest = kUSBRqClearFeature;
				pb->usb.cntl.WValue = kUSBHubPortSuspecdFeature;
				pb->usb.cntl.WIndex = pp->portIndex;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				noteError("Hub Driver - kUSBRqClearFeature (clearing 'port suspend' feature)");
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}		
			}
			else
			{
				continue;
			}

		break;
		
		case 2:
			/* Tell the USB to add the device */
		//	pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			// Speed no longer set in add device 

			noteError("Hub Driver - Device found, calling USBHubAddDevice");			
			USBExpertStatus(hubDevRef, "Hub Driver - Device found, calling USBHubAddDevice. Port", pp->portIndex);
			if(immediateError(USBHubAddDevice(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 3:
			pb->usbFlags = 0;
			/* We're called back when its time to reset */
			pp->state = kDeviceZero;	/* remeber to clean up if fails */
			
			/* remember the ref to the new device (as dev zero) */
			pp->newDevRef = pb->usbReference;
			
			pb->usbReference = hubRef;
			
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqSetFeature;
			pb->usb.cntl.WValue = kUSBHubPortResetFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;				
		
			setPortVector(pp->changeHandler, resetChangeHandler, kHubPortBeingReset);

			noteError("Hub Driver Error - kUSBRqSetFeature (resetting port)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				hubAddDeviceHandler(pb);
			}		
			
		break;

		case 4:
			pb->usbRefcon = 0;	/* this continues via the status change handler */
		break;
		
		default:
			noteError("Hub Driver Error - Unused case in add device");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}

static UInt32 doPortResetChangeHandler(USBPB *pb)
{
perPort *pp;

	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon >> kPortVectorStageShift)
	{		
		case 1:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}
			
			if(bit(pp->portStatus.portFlags, kHubPortBeingReset))
			{
				USBExpertStatus(hubDevRef, "Hub Driver Error - Port not finished resetting, retrying", pb->usbRefcon);
		  // we should never be here, just wait for another status change int
				return(kVectorContinueImmed);
			}
			
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}


			/* Now wait 10 ms after reset */ 
			/* Do fancy things with perPort to shorten this when looping */
			
			pb->usbReqCount = 7;	// This should be ready in 10ms
									// However devices don't have to respond for another 10ms
									// The set address below, won't arrive before 20ms
									// Thats 17 + at least 3ms one each for each of 3 controlls

			//pb->usbReqCount = 10;	// On root hub things are happening too quickly.
									// I'll have to fix that, in the meantime amke it 20

			noteError("Hub Driver - Calling USBDelay");			
			//USBExpertStatus(hubDevRef, "Hub Driver - calling delay", pb->usbReqCount);
			if(immediateError(USBDelay(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				doPortResetChangeHandler(pb);
			}
		break;
		
		case 2:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}

			pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			pb->usb.cntl.WValue = 8;	// max packet size 

			pb->usbReference = pp->devZeroRef;

			noteError("Hub Driver Error - configuring endpoint zero");			
			if(immediateError(USBHubConfigurePipeZero(pb)))
			{
				USBExpertStatus(hubDevRef, "Config pipe zero failed", pb->usbStatus);
				pb->usbRefcon += kPortVectorStage1;
				doPortResetChangeHandler(pb);
			}		
		break;

		case 3:
			if(pp->state == kDeadDeviceZero)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}
			pb->usbFlags = 0;	// tidy up after above

			/* now do a device request to find out what it is */


			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
			pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);

			/* did we get here because the get descriptor failed? */
			if(pb->usbStatus == kUSBOverRunErr)
			{	/* endpoint zero max packet too large. Read first */
				/* 8 bytes to get the value you need */
				USBExpertStatus(hubDevRef, "Hub driver - overrun error reading device descriptor:", pb->usbActCount);
				pb->usbReqCount = 8;
				pp->desc.numConf = 0;	// make a note to read all of this
			}

			pb->usbBuffer = &pp->desc;
			
			pp->onError = pb->usbRefcon-kPortVectorStage1;	/* do a retry on error */
				/* the get descriptor seems to screw up most often */
			noteError("Hub Driver Error - kUSBRqGetDescriptor (get device descriptor after a SetAddress)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				doPortResetChangeHandler(pb);
			}
		break;
		
		case 4:
			if( (hubDesc.removablePortFlags[pp->portByte] & pp->portMask) != 0)
			{
				pb->usbOther = powerForCaptive;
			}
			else
			{
				pb->usbOther = selfPowerGood?kUSB500mAAvailable:kUSB100mAAvailable;
			}
			
			/* Now address the device */
			pb->usbReference = pp->newDevRef;
			pb->usb.cntl.WValue = pp->desc.maxPacketSize;
			pp->devZeroRef = 0;		// if set address fails, don't try to remove old
			pp->onError = pb->usbRefcon;	/* Go back to start, via next case */
			pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			pb->usbFlags |= kUSBHubPower | kUSBHubReaddress;
			
			pp->state = kSetAddress;	/* If this fails, detatch */
			noteError("Hub Driver Error - Setting the device address");			
			if(immediateError(USBHubSetAddress(pb)))
			{
				pb->usbRefcon += kPortVectorStage1;
				doPortResetChangeHandler(pb);
			}

		break;
		
		case 5:
			if( (pp->state == kDeadDeviceZero) || (pb->usbReference == 0) )
			{
				/* If you want to try again with dead devices this is the place to do it */
				noteError("Hub Driver Error - dead set address");			
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				return(kVectorfinishedImmed);
			}

			pb->usbFlags = 0;
			pp->state = kNormal;	/* no longer need special handling to avoid deadlock */

			if(pb->usbStatus != noErr)
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;	// we need to fall forwards to set address
			}

			/* refernce is now new device ref, no longer dev zero */

			pp->devZeroRef = pb->usbReference;
	
			if(pp->desc.numConf == 0)	// don't have full descriptor, try again */
			{
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
				pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
				pb->usb.cntl.WValue = (kUSBDeviceDesc << 8) + 0/*index*/;
				pb->usb.cntl.WIndex = 0;
				pb->usbReqCount = OFFSET(USBDeviceDescriptor, descEnd);
				pb->usbBuffer = &pp->desc;
				
				pp->onError = pb->usbRefcon-kPortVectorStage1;	/* do a retry on error */
					/* the get descriptor seems to screw up most often */
				noteError("Hub Driver Error - kUSBRqGetDescriptor (get device descriptor after a SetAddress)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}
			}
			else
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;
			}
		break;
		
		case 6:

			/* FInally use the data gathered */
			setPortVector(pp->changeHandler, 0, kHubPortBeingReset);
			pp->portRequestPB->usbStatus = pb->usbStatus;
			pb = pp->portRequestPB;
			pp->portRequestPB = nil;	// The port is now free
			
			if( (pb->usbCompletion != nil) || (pb->usbCompletion != (void *)-1) )
			{
				(*pb->usbCompletion)(pb);
			}
			return(kVectorfinishedImmed);
		break;
		
		default:
			noteError("Hub Driver Error - Unused case in port reset change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */

	return(kVectorNotfinished);
}

static void doPortResetHandler(USBPB *pb)
{
perPort *pp;

	if(handleError(pb))
		return;
	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon++)
	{
	
		case 1:
			pb->usbReference = hubRef;
		
			/* Check if the port is suspended */
			if(bit(pp->portStatus.portFlags, kHubPortSuspend))
			{
				/* resume the port */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);
				pb->usb.cntl.BRequest = kUSBRqClearFeature;
				pb->usb.cntl.WValue = kUSBHubPortSuspecdFeature;
				pb->usb.cntl.WIndex = pp->portIndex;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				noteError("Hub Driver - kUSBRqClearFeature (clearing 'port suspend' feature)");
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}		
			}
			else
			{
				continue;
			}

		break;
		
		case 2:
			/* Tell the USB to add the device */
		//	pb->usbFlags = ((pp->portStatus.portFlags & kHubPortSpeed) != 0);	/* 1 if low speed */
			// Speed no longer set in add device 

			noteError("Hub Driver - Resetting device");			
			USBExpertStatus(hubDevRef, "Hub Driver - Waiting to reset device", 0);
			if(immediateError(USBHubAddDevice(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 3:
			pb->usbFlags = 0;
			/* We're called back when its time to reset */
			pp->state = kDeviceZero;	/* remeber to clean up if fails */
			
			/* remember the ref to the new device (as dev zero) */
			pp->devZeroRef = pb->usbReference;
			
			pb->usbReference = hubRef;
			
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqSetFeature;
			pb->usb.cntl.WValue = kUSBHubPortResetFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;				
		
			setPortVector(pp->changeHandler, doPortResetChangeHandler, kHubPortBeingReset);

			noteError("Hub Driver Error - kUSBRqSetFeature (resetting port)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				doPortResetHandler(pb);
			}		
			
		break;

		case 4:
			pb->usbRefcon = 0;	/* this continues via the status change handler */
		break;
		
		default:
			noteError("Hub Driver Error - Unused case in reset device");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}

static void hubStatusChangeHandler(USBPB *pb)
{
perPort *pp;
static void *oldHandler;
static UInt32 oldRefcon;
static USBReference oldRef;

	if(pb->usbCompletion != hubStatusChangeHandler)
	{
		oldHandler = pb->usbCompletion;
		pb->usbCompletion = hubStatusChangeHandler;
		oldRefcon = pb->usbRefcon;
		pb->usbRefcon = 1;
		oldRef = pb->usbReference;
		pb->usbReference = hubRef;
		USBExpertStatus(hubDevRef, "Hub Status change",0);
	}

	pp = (void *)pb;	/* parameter block has been extended */
	if(handleError(pb))
		return;
	
	do{switch(pb->usbRefcon++)
	{
		case 1:
			/* Now get hub status  */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = sizeof(hubStatus);
			pb->usbBuffer = &hubStatus;
			
			noteError("Hub Driver Error - kUSBRqGetStatus (get status before turning on ports)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}

		break;

		case 2:
			hubStatus.status = USBToHostWord(hubStatus.status);
			hubStatus.change = USBToHostWord(hubStatus.change);
			if(bit(hubStatus.change, kHubLocalPowerStatusChange) )
			{
				USBExpertStatus(hubDevRef, "Hub Driver - Local Power Status Change detected",0);
				/* Reset the change enable status */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBDevice);			
				pb->usb.cntl.BRequest = kUSBRqClearFeature;
				pb->usb.cntl.WValue = kUSBHubLocalPowerChangeFeature;
				pb->usb.cntl.WIndex = 0;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				noteError("Hub Driver Error - kUSBRqClearFeature (clear 'hub local power change' feature)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}		
			}
			else
			{
				pb->usbRefcon+=2;	/* Skip the next status */
				continue;	/* Carry on with next case */
			}
		break;

		case 3:
			/* Now get hub status  */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = sizeof(hubStatus);
			pb->usbBuffer = &hubStatus;
			
			noteError("Hub Driver Error - kUSBRqGetStatus (get status before turning on ports)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;

		case 4:
			hubStatus.status = USBToHostWord(hubStatus.status);
			hubStatus.change = USBToHostWord(hubStatus.change);
			continue;
		break;
		
		case 5:
			if(bit(hubStatus.change, kHubOverCurrentIndicatorChange) )
			{
				USBExpertStatus(hubDevRef, "Hub Driver - OverCurrent Indicator Change detected",0);
				/* Reset the change enable status */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBDevice);			
				pb->usb.cntl.BRequest = kUSBRqClearFeature;
				pb->usb.cntl.WValue = kUSBHubOverCurrentChangeFeature;
				pb->usb.cntl.WIndex = 0;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				noteError("Hub Driver Error - kUSBRqClearFeature (clear 'hub local power change' feature)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}		
			}
			else
			{
				pb->usbRefcon+=2;	/* Skip the next status */
				continue;	/* Carry on with next case */
			}
		break;
		
		case 6:
			/* Now get hub status  */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = sizeof(hubStatus);
			pb->usbBuffer = &hubStatus;
			
			noteError("Hub Driver Error - kUSBRqGetStatus (get status before turning on ports)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;

		case 7:
			hubStatus.status = USBToHostWord(hubStatus.status);
			hubStatus.change = USBToHostWord(hubStatus.change);
			continue;
		break;

		case 8:
			pb->usbCompletion = oldHandler;
			pb->usbRefcon = oldRefcon;
			pb->usbReference = oldRef;
		
			pb->usbReqCount = 10;
			USBExpertStatus(hubDevRef, "Hub Driver - calling delay reentering poll loop", 10);
			USBDelay(pb);
		break;
		
		default:
			noteError("Hub Driver Error - Unused case in hub status change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}


static UInt32 defDoNothingChangeHandler(USBPB *pb)
{
	pb = 0;
	return(kVectorContinueImmed);
}

static UInt32 deadConnectionChangeHandler(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */
	if(!bit(pp->portStatus.portFlags, kHubPortConnection))
	{
		USBExpertStatus(hubDevRef, "Hub dead driver - disconnect", 0);
		pp->state = kNormal;
		pp->retries = kInitialRetries;
		initPortVectors(pp->changeHandler);
		pp->delay = 0;
	}
	return(kVectorContinueImmed);
}

static UInt32 defOverCrntChangeHandler(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */
	USBExpertStatus(hubDevRef, "Hub Driver - Over current change notification", 0);
	pp->state = kDead;
	return(kVectorContinueImmed);
}

static UInt32 defResetChangeHandler(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */
	USBExpertStatus(hubDevRef, "Hub Driver - Resett change notification", 0);
	pp->state = kDead;
	return(kVectorContinueImmed);
}

static UInt32 defSuspendChangeHandler(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */
	USBExpertStatus(hubDevRef, "Hub Driver - Resett change notification", 0);
	return(kVectorContinueImmed);
}

static UInt32 defEndableChangeHandler(USBPB *pb)
{
perPort *pp;
	pp = (void *)pb;	/* parameter block has been extended */
	do{switch(pb->usbRefcon >> kPortVectorStageShift)
	{
		case 1:
			if(!bit(pp->portStatus.portFlags, kHubPortEnabled) && 
			   !bit(pp->portStatus.portChangeFlags, kHubPortConnection) )
			{
				/* The hub gave us an enable status change and we're now disabled, strange */
				/* Cosmo does this sometimes, try Re-enabling the port */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
				pb->usb.cntl.BRequest = kUSBRqSetFeature;
				pb->usb.cntl.WValue = kUSBHubPortEnablenFeature;
				pb->usb.cntl.WIndex = pp->portIndex;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				USBExpertStatus(hubDevRef, "Hub driver - re-enabling dead port", pp->portIndex);
				noteError("Hub Driver Error - kUSBRqSetFeature (trying to re-enable port)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}					
			} 
			else
			{
				return(kVectorContinueImmed);
			}
		break;

		case 2:
			return(kVectorContinueImmed);
		break;

		default:
			noteError("Hub Driver Error - Unused case in enable change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
	return(kVectorNotfinished);
}

static UInt32 defConnectionChangeHandler(USBPB *pb)
{
perPort *pp;
OSStatus err;

	pp = (void *)pb;	/* parameter block has been extended */
	do{switch(pb->usbRefcon >> kPortVectorStageShift)
	{
		case 1:
			pb->usbReqCount = 97;	/* Power on wait 100ms, plus at least 3 control requests */
			pb->usbFlags = 0;
			pb->usbStatus = noErr;

			//USBExpertStatus(hubDevRef, "Hub Driver - PS2 calling delay", 100);
			noteError("Hub Driver Error - delaying before adding device");			
			if(immediateError(USBDelay(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 2:
		
			/* If we get to here, there was a connection change */
			/* if we already have a device it must have been disconnected */
			/* at sometime. We should kill it before serviceing a connect event */
			if(pp->newDevRef != 0)
			{	/* already removed, dead device?? */
				pb->usbReference = pp->newDevRef;
				USBExpertStatus(hubDevRef, "Hub driver - Removing dead device:", pb->usbRefcon);
				noteError("Hub Driver Error - removing dead device");
				err = USBHubDeviceRemoved(pb);
				if(immediateError(err))	
				{	/* hub error procedures not needed for this */
					USBExpertFatalError(hubDevRef, pb->usbStatus, pp->errorString, 0);
					break;
				}
				if(err == noErr)
				{	/* completed immediatly */
					pb->usbRefcon += kPortVectorStage1;
					continue;
				}
			}
			else
			{
				pb->usbRefcon += kPortVectorStage1;
				continue;
			}
		break;

		case 3:

			if(pp->newDevRef != 0)
			{	/* this bit transplanted from end of case above */
				USBExpertRemoveDeviceDriver(pp->newDevRef);
				pp->newDevRef = 0;


				pp->delay = 0;
				pp->retries = kInitialRetries;
				/* Disconnection on the port */
			}
			
			// BT 23Jul98 Check port again after delay. Get bounced connections
			/* Do a port status request on current port */
			pb->usbReference = hubRef;
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0; /* Standard type 0 and index 0 */
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = sizeof(USBHubPortStatus);
			pb->usbBuffer = &pp->portStatus;

			noteError("Hub Driver Error - kUSBRqGetStatus (after delay)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 4:
			pp->portStatus.portChangeFlags = USBToHostWord(pp->portStatus.portChangeFlags);
			pp->portStatus.portFlags = USBToHostWord(pp->portStatus.portFlags);

//	USBExpertStatus(hubDevRef, "Hub driver - port status", pp->portStatus.portFlags);
//	USBExpertStatus(hubDevRef, "Hub driver - port change", pp->portStatus.portChangeFlags);
			if(bit(pp->portStatus.portChangeFlags, kHubPortConnection) )
			{
				USBExpertStatus(hubDevRef, "Hub driver - Connection bounce:", pb->usbReference);

				return(kVectorContinueImmed);
			}

			if( bit(pp->portStatus.portFlags, kHubPortConnection) )
			{	/* We have a connection on this port */

				pb->usbCompletion = hubAddDeviceHandler;
				pb->usbRefcon = 1;
				pb->usbStatus = noErr;
				pb->usbFlags = 0;

				hubAddDeviceHandler(pb);
			}
			else
			{
				return(kVectorfinishedImmed);
			}
		break;

		default:
			noteError("Hub Driver Error - Unused case in connection change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
	return(kVectorNotfinished);
}


static void portStatusChangedHandler(USBPB *pb)
{
perPort *pp;
int which, ret;
static getDescLoop;

	pp = (void *)pb;	/* parameter block has been extended */
	
	if(handleError(pb))
		return;
	
	do{switch( (pb->usbRefcon++) & kPortHandlerStageMask)
	{
		case 1:
			
			pb->usbReference = hubRef;
			
			/* Do a port status request on current port */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0; /* Standard type 0 and index 0 */
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = sizeof(USBHubPortStatus);
			pb->usbBuffer = &pp->portStatus;

			pb->usbFlags = 0;	// These were getting set during error processing

			noteError("Hub Driver Error - kUSBRqGetStatus (first after port status change)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 2:
			pp->portStatus.portChangeFlags = USBToHostWord(pp->portStatus.portChangeFlags);
			pp->portStatus.portFlags = USBToHostWord(pp->portStatus.portFlags);
			
			getDescLoop = (pb->usbRefcon & kPortHandlerStageMask);
			continue;
		break;

		case 3:
			pb->usbRefcon &= kPortHandlerStageMask;

			for(which = 0; which<numChangeHandlers; which++)
			{
				if( (pp->portStatus.portChangeFlags & pp->changeHandler[which].bit) != 0 )
				{
					pb->usbRefcon += which*kPortWhichVector1;
					break;
				}
			}
 			if(which >= numChangeHandlers)
 			{
				pb->usbRefcon = 0;	// Handled all changed bits
			}
			else
			{
				continue;
			}	
		break;
		
		case 4:
			which = (pb->usbRefcon & kPortVectorStageMask) >> kPortWhichVectorShift;

			/* Reset the changed status */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqClearFeature;
			pb->usb.cntl.WValue = pp->changeHandler[which].clearFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;				

			noteError("Hub Driver Error - kUSBRqClearFeature (clear port vector bit feature)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 5:			
			pb->usbReference = hubRef;
			
			/* Do a port status request on current port */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0; /* Standard type 0 and index 0 */
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = sizeof(USBHubPortStatus);
			pb->usbBuffer = &pp->portStatus;

			noteError("Hub Driver Error - kUSBRqGetStatus (second after port status change)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 6:
			pp->portStatus.portChangeFlags = USBToHostWord(pp->portStatus.portChangeFlags);
			pp->portStatus.portFlags = USBToHostWord(pp->portStatus.portFlags);
			continue;
			
		case 7:
		
			which = (pb->usbRefcon & kPortVectorStageMask) >> kPortWhichVectorShift;
			pb->usbRefcon--;	// come back to here for completion
			pb->usbRefcon += kPortVectorStage1;	// Step the handlers stage on

			ret = (*pp->changeHandler[which].handler)(pb);

			if( (kVectorContinueImmed == ret) || (kVectorContinueDelayed == ret) )
			{
				pb->usbRefcon = getDescLoop;
				if(kVectorContinueImmed == ret)
				{
					continue;
				}
			}
			else if(kVectorfinishedImmed == ret)
			{
				pb->usbRefcon = 0;
			}
			else if(kVectorNotfinished == ret)
			{
			}
		break;

		default:
			noteError("Hub Driver Error - Unused case in port status change handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}


static void intPipeHandler(USBPB *pb)
{
perPort *pp, *intPort;
static UInt8 buffer[256/8+1];
static numBytes;
static readPipeLoop;
int byteCounter, bitMask, portIndex;

	if(handleError(pb))
		return;
		
	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon++)
	{
		case 1:		
			pb->usbReference = hubRef;
			/* Open the pipes (there's only one) in the interface */
			pb->usb.cntl.WValue = 0;
			pb->usbFlags = 0;

			pb->usbReqCount = 0;
			pb->usbActCount = 0;
			pb->usbBuffer = 0;
			
			pb->usbOther = 0;	// alternative interface
			
			noteError("Hub Driver Error - USBConfigureInterface");			
			if(immediateError(USBConfigureInterface(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 2:
		
			/* Find out when we first ask for the int, so enumeration */
			/* completion can work out its not going to happen. */
			pb->usbFlags = 0;
			if(USBGetFrameNumberImmediate(pb) != noErr)
			{
				USBExpertStatus(hubDevRef, "Hub driver - Frame number error:", pb->usbReference);
				intFrame = 1;
			}
			else
			{
				intFrame = pb->usbFrame;
			}
		
			/* New reads to this pipe will go to the same place we are */
			readPipeLoop = pb->usbRefcon;

			/* Find the pipe ref for the (only) pipe */
			pb->usbFlags = kUSBIn;
			pb->usbClassType = kUSBInterrupt;
			noteError("Hub Driver Error - finding interrupt pipe");			
			if(immediateError(USBFindNextPipe(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 3:
			pb->usbFlags = 0;

			/* reference is now int pipe */			
			numBytes = (numPorts+1)/8+1;
			pb->usbReqCount = numBytes;
			pb->usbBuffer = buffer;
			noteError("Hub Driver Error - Reading interrupt pipe");			
			if(immediateError(USBIntRead(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;
		
		case 4:
			
			/* Now interpret the status returned */
			pb->usbRefcon = readPipeLoop;	/* end up back at the beginning */
			
			bitMask = 2;
			byteCounter = 0;
			for(portIndex = 1; portIndex <= numPorts; portIndex++)
			{
				if( (buffer[byteCounter] & bitMask) != 0 )
				{
					intPort = &ports[portIndex];	// Do it this way so debugger can see pp
					if(intPort->pb.usbRefcon != 0)
					{
						// Maybe want to count failures here 
						USBExpertStatus(hubDevRef, "Hub Driver - Interrupt while hub was busy", portIndex);			
					}
					else
					{
						intFrame = 0xffffffff;	/* note an interrupt happened. */
						intPort->pb.usbStatus = noErr;
						intPort->pb.usbRefcon = 1;
						intPort->pb.usbCompletion = portStatusChangedHandler;
						portStatusChangedHandler(&intPort->pb);
					}
				}
				bitMask <<= 1;
				if(bitMask > 128)
				{
					bitMask = 1;
					byteCounter++;
				}
			}

			if((buffer[0] & 1) != 0)
			{	/* hub status changed */
				intFrame = 0xffffffff;	/* note an interrupt happened. */
				hubStatusChangeHandler(pb);
				break;
			}
			else
			{	
			OSStatus	ret_status;

				if(HubAreWeFinishedYet() == noErr)
				{
					pb->usbReqCount = 255;
				}
				else
				{
					pb->usbReqCount = 20;
				}
				noteError("Hub Driver - Delay after hub status change");			
				//USBExpertStatus(hubDevRef, "Hub Driver - Pipe calling delay", pb->usbReqCount);
				if(immediateError(ret_status = USBDelay(pb)))
				{
kprintf("USB: immediateError delay %d\n", ret_status);
					HubFatalError(pp, pb->usbStatus, pp->errorString, pb->usbReqCount);
				}	
			}
		break;
		default:
			noteError("Hub Driver Error - Unused case in interrupt handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}


static void startPorts(USBPB *pb)
{
perPort *pp;
int currentPort, byte, mask;

	if(handleError(pb))
		return;
		
	pp = (void *)pb;	/* parameter block has been extended */
	
	do{switch(pb->usbRefcon++)
	{
		case 1:
 
 			/* These help with port removable and power flags */
 			byte = 0;
 			mask = 2;	/* First bit is reserved */
 			
 			for(currentPort = 1; currentPort <= numPorts; currentPort++)
			{

				pp = &ports[currentPort];
				pp->pb = *pb;
				pp->pb.pbLength = sizeof(perPort);
				pp->pb.pbVersion = kUSBCurrentPBVersion;

				initPortVectors(pp->changeHandler);

				pp->portIndex = currentPort;
				pp->retries = kInitialRetries;
				pp->delay = 0;
				
				/* Setup port mask for this port, ready for next */
				pp->portByte = byte;
				pp->portMask = mask;
				if( startExternal || ( (hubDesc.removablePortFlags[byte] & mask) != 0)	)
				{	
					startPorts(&pp->pb);	/* Carry on with next case, with new pb */
				}

				mask <<= 1;
				if(mask > 0x80)
				{
					mask = 1;
					byte++;
				}
			}


		break;

		case 2:
			/* Power the port */
			
			/* Maybe should only do this if there is power switching */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqSetFeature;
			pb->usb.cntl.WValue = kUSBHubPortPowerFeature;
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = 0;
			pb->usbBuffer = nil;			

			noteError("Hub Driver - kUSBRqSetFeature (Powering on port)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;

		case 3:
			if( (hubDesc.removablePortFlags[pp->portByte] & pp->portMask) == 0)
			{
				pb->usbRefcon = 0;
				break;	// This is all done by the int handler hopefully.
				/* Tell the world we're finsihed */
			}
		// This for static hub 
		//	Debugger();
			/* wait for the power on good time */
			
			pb->usbReqCount = hubDesc.powerOnToGood * 2;
			/* pb->usbFlags = kUSBtaskTime */

			//USBExpertStatus(hubDevRef, "Hub Driver - Power on calling delay", pb->usbReqCount);
			noteError("Hub Driver - Waiting for port to power on");			
			if(immediateError(USBDelay(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;

		case 4:
			/* We should now be in the disconnected state */
			
			/* Do a port request on current port */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0; /* Standard type 0 and index 0 */
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = sizeof(USBHubPortStatus);
			pb->usbBuffer = &pp->portStatus;

			noteError("Hub Driver - kUSBRqGetStatus (after power on)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 5:
			pp->portStatus.portChangeFlags = USBToHostWord(pp->portStatus.portChangeFlags);
			pp->portStatus.portFlags = USBToHostWord(pp->portStatus.portFlags);
			/* we now have port status */
			if(bit(pp->portStatus.portChangeFlags, kHubPortConnection) )
			{
				/* Reset the change connection status */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBOut, kUSBClass, kUSBOther);			
				pb->usb.cntl.BRequest = kUSBRqClearFeature;
				pb->usb.cntl.WValue = kUSBHubPortConnectionChangeFeature;
				pb->usb.cntl.WIndex = pp->portIndex;
				pb->usbReqCount = 0;
				pb->usbBuffer = nil;				

				noteError("Hub Driver - kUSBRqClearFeature (clear 'port change connection' feature");			
				if(immediateError(USBDeviceRequest(pb)))
				{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}		

			}
			else
			{
				pb->usbRefcon+=2;	/* Skip the next status */
				continue;	/* Carry on with next case */
			}
		break;
	
		case 6:
			/* We should now be in the disconnected state */
			/* Do a port request on current port */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBOther);			
			pb->usb.cntl.BRequest = kUSBRqGetStatus;
			pb->usb.cntl.WValue = 0; /* Standard type 0 and index 0 */
			pb->usb.cntl.WIndex = pp->portIndex;
			pb->usbReqCount = sizeof(USBHubPortStatus);
			pb->usbBuffer = &pp->portStatus;

			noteError("Hub Driver - kUSBRqGetStatus (after change connection)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
		break;

		case 7:
			pp->portStatus.portChangeFlags = USBToHostWord(pp->portStatus.portChangeFlags);
			pp->portStatus.portFlags = USBToHostWord(pp->portStatus.portFlags);
			continue;
		break;

		case 8:
			if( bit(pp->portStatus.portFlags, kHubPortConnection) )
			{	/* We have a connection on this port */

				pb->usbCompletion = hubAddDeviceHandler;
				pb->usbRefcon = 1;
				pb->usbStatus = noErr;
				
				hubAddDeviceHandler(pb);
			}
			else
			{
				pb->usbRefcon = 0;	// this should never happen, but don't freeze port
			}
		// Static hub
		break;

		default:
			noteError("Hub Driver Error - Unused case in start ports");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}


static void hubHandler(USBPB *pb)
{
perPort *pp;
USBDeviceDescriptor *desc;
static USBConfigurationDescriptor conf;
UInt32 powerStatus;

	if(handleError(pb))
		return;
		
	pp = (void *)pb;	/* parameter block has been extended */
kprintf("hubHandler:usbRefcon=%d,ref=0x%x\n",pb->usbRefcon,pb->usbReference);
	do{switch(pb->usbRefcon++)
	{
		case 1:
			hubRef = pb->usbReference;	/* remember who we are */
			hubDevRef = hubRef;			/* remember the dev ref to talk to expt */
			
			desc =  pb->usbBuffer;

			errataBits = GetErrataBits(desc);
			if(errataBits != 0)
			{
				USBExpertStatus(hubDevRef, "Hub Driver - Using errata:", errataBits);
			}

			hubSubClass = desc->deviceSubClass;	/* Old hubs = 1, new hubs = 0 it seems */
			noteError("Hub Driver Error - Device not recognized");			
			if(desc->numConf != 1)
			{
				USBExpertStatus(hubDevRef, "Hub Driver - Device has more than 1 configuration. Using first", numPorts);
			}
			/* get the full config descriptor */
			if(hubSubClass == 1)
			{
				pp->value = 0; /* Hope 0 is the first one */
			}
			else
			{
				pp->value = 1; /* Hope 1 is the first one */
			}
			pp->onError = pb->usbRefcon;	/* Try the clause below again on error */
#if 0
	// This bit tests device reset. You don't normally want it here 
			if(hubSubClass == 1)
			{
				continue;
			}	
			pb->usbFlags = 0;
			noteError("Hub Driver Error - hub reset");			
kprintf("hubHandler:calling USBResetDevice: ref=0x%x\n",pb->usbReference);
			if(immediateError(USBResetDevice(pb)))
			{
				USBExpertStatus(hubDevRef, "Hub driver - Immediate error testing reset", pb->usbStatus);
				continue;
				//HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}		
#else
			continue;
#endif
		break;

		case 2:
			pb->usbClassType = kUSBHubClass;		/* hub class */
			pb->usbSubclass = hubSubClass;		/* hub subclass, as in the device descriptor */
	/* Could make this wild, but devices which have these different are bad */
			pb->usbProtocol = 0;					/* any protocol */
			
			pb->usbReqCount = 0;
			pb->usbActCount = 0;
			pb->usbBuffer = 0;
			
			pb->usb.cntl.WValue = 0;	// first config
			pb->usb.cntl.WIndex = 0;	// first interface
			pb->usbOther = 0xff;// first alt

			pb->usbReqCount = busPowerAvail;
			pp->onError = pb->usbRefcon;
			
			noteError("Hub Driver - Getting interface");			
kprintf("hubHandler:case 2 calling USBFindNextInterface: ref=0x%x\n",pb->usbReference);
			if(immediateError(USBFindNextInterface(pb)))
			{	/* this should return immediatly */
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				break;	/* Nothing else to do now */
			}

		break;

		case 3:
			if(pb->usbStatus == kUSBDevicePowerProblem)
			{
				noteError("Hub Driver - Insufficient power to configure hub");			
				USBExpertSetDevicePowerStatus(hubDevRef, 0, 0, kUSBDevicePower_BusPowerInsufficient, busPowerAvail, kUSB500mAAvailable);  // TC: <USB67>
			}
			
			if(pb->usbStatus != noErr)
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				break;
			}
			pb->usbReqCount = sizeof(conf);
			pb->usbBuffer = &conf;
					
			noteError("Hub Driver - Getting config descriptor");			
kprintf("hubHandler:case 3 calling USBGetConfig: ref=0x%x\n",pb->usbReference);
			if(immediateError(USBGetConfigurationDescriptor(pb)))
			{	/* this should return immediatly */
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				break;	/* Nothing else to do now */
			}

		break;

		case 4:

			/* Work out some power bits */
			
			busPowered = (conf.attributes & kUSBAtrBusPowered) != 0;
			selfPowered = (conf.attributes & kUSBAtrSelfPowered) != 0;
		
			noteError("Hub Driver Error - Illegal device configuration, no power");			
			if( !(busPowered || selfPowered) )
			{	/* Neither slef nor bus powered, its dead!!! */
				HubFatalError(pp, kUSBUnknownDeviceErr, pp->errorString, 0);
				break;	/* Now what ?? */
			}

		/* these got set up to read the descriptor. */
			pb->usbReqCount = 0;
			pb->usbActCount = 0;
			pb->usbBuffer = 0;

			/* for error recovery */
			pp->value = 1-pp->value;
		
			/* There is an interface for us, Configure the device */
			//pb->usb.cntl.WValue = pb->usbValue4;	// have to sort out this
			noteError("Hub Driver Error - Getting full configuration descriptor");			
kprintf("HubClassDriver.c:case 4 SetConfig: ref=0x%x\n",pb->usbReference);
			if(immediateError(USBSetConfiguration(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
kprintf("hubDriver:setconfig complete****\n");
		break;

		case 5:
	
			
			/* find an interface ref to work with */
			noteError("Hub Driver Error - USBNewInterfaceRef");			
kprintf("HubClassDriver.c:case 5 calling USBNewInterfaceRef\n");
			if(immediateError(USBNewInterfaceRef(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;
			
	
	
		case 6:
			/* Now remember the interface ref */
			hubRef = pb->usbReference;

			/* Mess with get hub descriptor */
			if(hubSubClass == 0)
			{
				pp->value = 0; /* Standard type 0 and index 0 */
			}
			else
			{
				pp->value = 0x2900; /* for atmel Standard type 0 and index 0 */
			}
			pp->onError = pb->usbRefcon;	/* Try the clause below again on error */
			continue;
		break;
			
	
		case 7:
			/* Switch the value for retry */
			pp->value = 0x2900 - pp->value;
			pb->usb.cntl.WValue = pp->value;

			/* Start things off, lets find out what the hub is */
			pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBClass, kUSBDevice);			
			pb->usb.cntl.BRequest = kUSBRqGetDescriptor;
			pb->usb.cntl.WIndex = 0;
			pb->usbReqCount = sizeof(hubDescriptor) -1;
			pb->usbBuffer = &hubDesc.length;

			noteError("Hub Driver Error - kUSBRqGetDescriptor (get device descriptor)");			
			if(immediateError(USBDeviceRequest(pb)))
			{
				HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
			}
		break;
			
		case 8:
			/* Hub get descriptor now finished */
			numPorts = hubDesc.numPorts;
			{
				/* Unpack the port flags */
			int numFlags, i;
				numFlags = (numPorts+1)/8+1;
				for(i = 0; i < numFlags; i++)
				{
					hubDesc.pwrCtlPortFlags[i] = hubDesc.removablePortFlags[numFlags+i];
					hubDesc.removablePortFlags[numFlags+i] = 0;
				}
			}
			
			/* Work out some bits for compound devices */
			numCaptive = 0;
		//	if(hubDesc.characteristics & kHubIsCompound)
			{
			int i, portMask = 2, portByte = 0;
				for(i = 1; i<=numPorts; i++)
				{
					if( (hubDesc.removablePortFlags[portByte] & portMask) != 0)
					{
						numCaptive++;
					}
					portMask <<= 1;
					if(portMask > 0x80)
					{
						portMask = 1;
						portByte++;
					}
				}
			}
			
			if(numPorts > kMaxPorts)
			{
				/* Need to allocate more port structs here */
				USBExpertStatus(hubDevRef, "Hub Driver - Allocating more ports", numPorts);
				
				pb->usbReqCount = (numPorts+1)*sizeof(perPort);
				pp->onError = pb->usbRefcon;
				noteError("Hub Driver Error - Allocating memory for ports");			
				USBAllocMem(pb);
			}
			else
			{
				pb->usbRefcon++;
				continue;
			}
		break;
		
		case 9:
			/* new ports allocated */
			if(pb->usbBuffer == nil)
			{
				/* carry on with only pre allocated ports */
				numPorts = kMaxPorts;
				USBExpertStatus(hubDevRef, "Hub Driver Error - Allocating more ports failed", numPorts);
				continue;
			}
			else
			{
				USBExpertStatus(hubDevRef, "Hub Driver - More ports allcated", pb->usbActCount);
				ports = pb->usbBuffer;
				ports[0] = *pp;
				pb = (void *)ports;
				continue;
			}
		break;
	
		case 10:
			if(busPowered)
			{
			UInt32 pHub, pAvailForPorts,pNeededForPorts;
					/* Note hub current in units of 1mA, everything else in units of 2mA */
				pHub = hubDesc.hubCurrent/2;
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* pHub:", pHub);
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* busPowerAvail:", busPowerAvail);
				if(pHub > busPowerAvail)
				{
					/* this is illegal??? */
					USBExpertStatus(hubDevRef, "Hub Driver - Hub needs more power than available", pb->usbActCount);
					USBExpertSetDevicePowerStatus(hubDevRef, 0, 0, kUSBDevicePower_BusPowerInsufficient, busPowerAvail, pHub);  // TC: <USB67>
					break;	/* Thats that. */
				}
				pAvailForPorts = busPowerAvail-pHub;
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* pAvailForPorts:", pAvailForPorts);
				pNeededForPorts = (numPorts-numCaptive)*kUSB100mA;
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* numPorts:", numPorts);
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* numCaptive:", numCaptive);
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* pNeededForPorts:", pNeededForPorts);
				busPowerGood = pAvailForPorts >= pNeededForPorts;
			//USBExpertStatus(hubDevRef, "Hub Driver - *Power* busPowerGood:", busPowerGood);
				if(numCaptive > 0)
				{
					if(busPowerGood)
					{ 
						powerForCaptive = (pAvailForPorts - pNeededForPorts)/numCaptive;
					}
					else
					{
						powerForCaptive = pAvailForPorts/numCaptive;
					}
				}
				
				if( (errataBits & kErrataCaptiveOK) != 0)
				{
					powerForCaptive = kUSB100mAAvailable;
				}
			}

			if(selfPowered)
			{
				/* Now get power status before turning on ports */
				pb->usb.cntl.BMRequestType = USBMakeBMRequestType(kUSBIn, kUSBStandard, kUSBDevice);			
				pb->usb.cntl.BRequest = kUSBRqGetStatus;
				pb->usb.cntl.WValue = 0;
				pb->usb.cntl.WIndex = 0;
				pb->usbReqCount = sizeof(deviceStatus);
				pb->usbBuffer = &deviceStatus;
				
				noteError("Hub Driver Error - kUSBRqGetStatus (get status before turning on ports)");			
				if(immediateError(USBDeviceRequest(pb)))
				{
					HubFatalError(pp, pb->usbStatus, pp->errorString, 0);
				}
			}
			else
			{
				continue;
			}				
		break;

		case 11:
			
			/* Now have hub power status */
			if(selfPowered)
			{
				deviceStatus = USBToHostWord(deviceStatus);
				selfPowerGood = ((deviceStatus & 1) != 0);
			}
 
 //OSStatus USBExpertSetDevicePowerStatus(USBDeviceRef ref, UInt32 powerStatus, UInt32 busPowerAvailable, UInt32 busPowerNeeded )

			if(selfPowered && busPowered)
			{
				/* Duel power hub */
				if(selfPowerGood)
				{
					powerStatus = kUSBDevicePower_PowerOK;
					USBExpertStatus(hubDevRef, "Hub attached - Self/Bus powered, power supply good", 0);
				}
				else
				{
					powerStatus = kUSBDevicePower_SelfPowerInsufficient;
					USBExpertStatus(hubDevRef, "Hub attached - Self/Bus powered, no external power", 0);
				}
			}
			else
			{
				/* Single power hub */
				if(selfPowered)
				{
					if(selfPowerGood)
					{
						powerStatus = kUSBDevicePower_PowerOK;
						USBExpertStatus(hubDevRef, "Externally powered Hub attached - power supply good", 0);
					}
					else
					{
						powerStatus = kUSBDevicePower_SelfPowerInsufficient;
						USBExpertStatus(hubDevRef, "Externally powered Hub attached - no external power", 0);
					}
				}
				else
				{
					powerStatus = kUSBDevicePower_BusPowerInsufficient;
					USBExpertStatus(hubDevRef, "Bus powered Hub attached", 0);
				}
			
			}
 
 
 			startExternal = (busPowerGood || selfPowerGood);
			if( !startExternal )
			{	/* not plugged in or bus powered on a bus powered hub */
				USBExpertStatus(kUSBUnknownDeviceErr, "Hub Driver Error - Insufficient power to turn on ports", 0);
				USBExpertSetDevicePowerStatus(hubDevRef, 0, 0, powerStatus, busPowerAvail, busPowered?kUSB500mAAvailable:0);  // TC: <USB67>
				if(!busPowered)
				{
					/* may be able to turn on compound devices */
					break;	/* Now what ?? */
				}
			}

			//USBExpertStatus(kUSBUnknownDeviceErr, "Hub Starting ports, ext:", startExternal);
			//USBExpertStatus(kUSBUnknownDeviceErr, "Hub compound power:", powerForCaptive);
			pb->usbStatus = noErr;
 			pb->usbCompletion = startPorts;
 			pb->usbRefcon = 1;
 			startPorts(pb);

			pb->usbCompletion = intPipeHandler;
			pb->usbRefcon = 1;
			pb->usbStatus = noErr;
			pp = (void *)pb;
			pp->retries = kInitialRetries;
			intPipeHandler(pb);

 		break;

		default:
			noteError("Hub Driver Error - Unused case in hub handler");			
			HubFatalError(pp, kUSBInternalErr, pp->errorString, pb->usbRefcon);
		break;
	}
	break;	/* only execute once, unless continue used */
	}while(1);	/* so case can be reentered with a continue */
}

#if 0
                          ExtPower
                    Good           off

Bus     Self

 0        0        Illegal config
 
 0        1        Always 100mA per port
 
 1        0        500mA           0 (dead)
 
 1        1        500           100

#endif


static OSStatus killHub(USBDeviceRef device)
{	/* This hub has died, kill its children */
int i;
perPort *pp;
USBPB *pb;

	pp = &ports[0];	
	if(device != hubDevRef)
	{
		HubFatalError(pp, kUSBUnknownDeviceErr, "Hub driver - Kill hub called with wrong ref num", 0);
		return(kUSBUnknownDeviceErr);
	}
	
	
	for(i = 1; i<= numPorts; i++)
	{
		pp = &ports[i];
		pb = &pp->pb;
		pb->usbReference = pp->newDevRef;
		pb->usbCompletion = kUSBNoCallBack;	// we're supposed to be called at task time
		if(pp->newDevRef != 0)
		{
			USBExpertStatus(hubDevRef, "Hub driver - Removing dead child:", pb->usbReference);
			noteError("Hub Driver Error - removing dead child");
			if(immediateError(USBHubDeviceRemoved(pb)))
			{
				USBExpertStatus(hubDevRef, "Hub driver - Failed to remove dead child:", pb->usbReference);
			}

			USBExpertRemoveDeviceDriver(pp->newDevRef);

			pp->newDevRef = 0;
			pb->usbRefcon = 0;

			pp->delay = 0;
			pp->retries = kInitialRetries;
		}
	}

	pp = &ports[0];	
	pb = &pp->pb;
	if(pb->usbStatus == kUSBPending)
	{
		HubFatalError(pp, kUSBPending, "Hub driver - transaction still outstanding", 0);
		return(kUSBAlreadyOpenErr);
	}

	if(staticPorts != ports)
	{	/* we allocated our own */
		USBExpertStatus(hubDevRef, "Hub driver - Deallocating allocated ports", 0);
		pp = &staticPorts[0];
		pb = &pp->pb;	// make sure to use a static port for this.
		pb->usbCompletion = kUSBNoCallBack;
		pb->usbBuffer = ports;

		if(immediateError(USBDeallocMem(pb)))
		{
			USBExpertStatus(hubDevRef, "Hub driver - Failed to deallocate:", pb->usbStatus);
		}		
	}
	


	pb->usbReference = hubRef;
	pb->usbCompletion = kUSBNoCallBack;
	pb->usbFlags = 0;
	USBDisposeInterfaceRef(pb);

	return(noErr);
}

static OSStatus resetPortRequest(USBPB *pb, UInt32 port)
{
	pb = &ports[port].pb;

	pb->usbCompletion = doPortResetHandler;
	pb->usbRefcon = 1;
	pb->usbStatus = noErr;
	pb->usbFlags = 0;

	doPortResetHandler(pb);
	return(kUSBPending);
}

static OSStatus resetPortPowerRequest(USBPB *pb, UInt32 port)
{
	USBExpertStatus(hubDevRef, "Hub driver - port power request, not implimented", port);
pb=0;
	return(kUSBUnknownRequestErr);
}

static OSStatus resetRequest(USBPB *pb, UInt32 port)
{
	if(bit(pb->usbFlags, kUSBPowerReset))
	{
		return(resetPortPowerRequest(pb, port));
	}
	else
	{
		return(resetPortRequest(pb, port));
	}
}



// A.W. this next function is unused anywhere, so I'll just comment it
// out to avoid clashing with original HubClassDriver.c

#ifdef NOT_USED_ANYWHERE
OSStatus HubChildMessage(USBReference deviceRef, USBPB *pb)
{
int port=1;
Boolean busy;
OSStatus err;

//	USBExpertStatus(hubDevRef, "Hub driver - Child message for device:", deviceRef);
	while(port<= numPorts)
	{
		if(ports[port].newDevRef == deviceRef)
		{
			break;
		}
		port++;
	}
	if(port > numPorts)
	{
		return(kUSBUnknownDeviceErr);
	}
	
	busy = !CompareAndSwap(nil, (UInt32)pb, (UInt32 *)&ports[port].portRequestPB);
	if(!busy)
	{
		busy = !CompareAndSwap(0, 1, &ports[port].pb.usbRefcon);
		if(busy)
		{
			ports[port].portRequestPB = nil;	/* port is otherwise busy, so clear request */
		}
	}
	if(busy)
	{
		USBExpertStatus(hubDevRef, "Hub driver - request on busy port:", port);
		return(kUSBDeviceBusy);
	}
	
//	USBExpertStatus(hubDevRef, "Hub driver - Child message for port:", port);
	
	if(pb->usb.hub.Request == kUSBHubPortResetRequest)
	{
		err = resetRequest(pb, port);
	}
	else
	{
		err = kUSBUnknownRequestErr;
	}
	if(err != kUSBPending)
	{
		ports[port].portRequestPB = nil;
		ports[port].pb.usbRefcon = 0;
	}
	return(err);
}
#endif



//A.W. This looks like the one unique function called from uslExpert, so
// this name must be changed for hub 0 or hub 1.  I'll call it "2" for now.

void Hub2DriverEntry(USBDeviceRef device, USBDeviceDescriptor *desc,  UInt32 busPowerAvailable)
{
static Boolean beenThereDoneThat = false;
//VersRec vers;
//OSStatus err;

	if(beenThereDoneThat)
	{
		USBExpertFatalError(device, kUSBInternalErr, "Hub driver called second time", 0);
		return;
	}
//Hot Plug n Play
//naga	beenThereDoneThat = true;
	
//	err = USBServicesGetVersion(1, &vers, sizeof(vers));
//	err = USBServicesGetVersion(2, &vers, sizeof(vers));
	
	ports[0].pb.pbLength = sizeof(perPort);
	ports[0].pb.pbVersion = kUSBCurrentHubPB;
	ports[0].pb.usbStatus = noErr;
	ports[0].pb.usbReference = device;
	ports[0].pb.usbCompletion = hubHandler;
	ports[0].pb.usbRefcon = 1;
	ports[0].pb.usbBuffer = desc;
	
	ports[0].retries = kInitialRetries;
	ports[0].delay = 0;
	ports[0].errorString = "";
	
	busPowerAvail = busPowerAvailable;
kprintf("HubDriverEntry:calling hubHandler:ref=0x%x\n",ports[0].pb.usbReference);
	hubHandler(&ports[0].pb);
}


