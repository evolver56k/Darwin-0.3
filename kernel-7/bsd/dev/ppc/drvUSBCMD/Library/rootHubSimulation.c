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
	File:		rootHubSimulation.c

	Contains:	Packet level simulation of root hub functions

	Version:	Neptune 1.0

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DF)	David Ferguson
		(DKF)	David Ferguson
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB27>	 6/14/98	DF		change implemented spelling.
	 <USB26>	 5/20/98	BT		Fix alt settings field of interface descriptor
	 <USB25>	 5/17/98	BT		Make get conf descriptor more forgiving. Fix hub endpoint desc
	 <USB24>	 5/14/98	DF		Add status messages for unimplemented root hub stuff, also use
									info from the UIM when getting the hub status.
	 <USB23>	 4/23/98	BT		Fix reset portsuspend change feature
	 <USB22>	 4/20/98	BT		Add device status
	 <USB21>	 4/16/98	BT		Eliminate debugger
	 <USB20>	 4/15/98	BT		More root hub capabilities
	 <USB19>	  4/9/98	BT		Use USB.h
		<18>	  4/2/98	BT		Do OPTI chip gang switching, fix sim stall error number
		<17>	 3/19/98	BT		Split UIM into UIM and root hub parts
		<16>	 3/18/98	BT		Add port enable and clear port enable change features
		<15>	 3/11/98	BT		interrupt transaction simulation
		<14>	  3/9/98	BT		Eliminate one set of redundant descriptors
		<13>	 2/27/98	BT		Add string descriptors
		<12>	 2/25/98	BT		Add Apple product codes.
		<11>	 2/24/98	BT		Change Apple vendor ID.
		<10>	 2/23/98	BT		Simulate root hub to not corrupt reqCount
	  <USB9>	  2/9/98	DKF		Byte swap configuration descriptor totalLength field
		 <8>	  2/8/98	BT		More Power allocation stuff
		 <7>	  2/4/98	BT		Cope with multiple configs
		 <6>	 1/30/98	BT		Put Apple vendor ID in root hub device descriptor
		 <5>	 1/26/98	CJK		Change to use USBDeviceDescriptor (instead of just
									devicedescriptor)
		 <4>	 1/26/98	BT		Mangle names after design review
		 <3>	 1/20/98	BT		Endian swap port status, and other fixes
		 <2>	 1/20/98	BT		More root hub simulation
		 <1>	 1/19/98	BT		first checked in
*/

#include <USB.h>
#include <USBpriv.h>
#include "OHCIUIM.h"
#include "OHCIRootHub.h"
#include "uimpriv.h"
#include "uslpriv.h"
#include "hub.h"
#include <DriverServices.h>

static UInt16 configuration;

static Boolean SimulateSetConfiguration(UInt16 value)
{
	if((value == 1) || (value == 0) )
	{
		configuration = value;
		return(false);
	}
	return(true);
}

static Boolean SimulateGetDeviceDescriptor(UInt8 *buf, UInt32 *bufLen, UInt32 descIndex)
{
UInt32 length;
static USBDeviceDescriptor desc ={
	18,					// UInt8 length;
	kUSBDeviceDesc,		// UInt8 descType;
	USB_CONSTANT16(kUSBRel10),			// UInt16 usbRel;
	kUSBHubClass,		// UInt8 class;
	kUSBHubSubClass,	// UInt8 subClass;
	0,					// UInt8 protocol;
	8,					// UInt8 maxPacketSize;
	USB_CONSTANT16(kAppleVendorID),		// UInt16 vendor; 	Apple = 1452 (decimal, swapped)
	USB_CONSTANT16(kPrdRootHubCMD),		// UInt16 product;	0x8001
	USB_CONSTANT16(0x0100),				// UInt16 devRel;	1.00
	0,					// UInt8 manuIdx;
	0,					// UInt8 prodIdx;
	0,					// UInt8 serialIdx;
	1,					// UInt8 numConf;
	0					// UInt16 end;
	};

	if(descIndex != 0)
	{
		return(true);
	}

	length = sizeof(desc);
	if(*bufLen < length)
	{
		length = *bufLen;
	}
	usb_BlockMoveData(&desc, buf, length);
	*bufLen -= length;
	
	return(false);
}

static Boolean SimulateGetConfDescriptor(UInt8 *buf, UInt32 *bufLen, UInt32 descIndex)
{
UInt32 length;
#define totalLen  (9+9+7)
static USBConfigurationDescriptor confDesc ={
	9,							//UInt8 length;
	kUSBConfDesc,				//UInt8 descriptorType;
	USB_CONSTANT16(totalLen),	//UInt16 totalLength;
	1,							//UInt8 numInterfaces;
	1,							//UInt8 configValue;
	0,							//UInt8 configStrIndex;
	0x60,						//UInt8 attributes; self powered, supports remote wkup
	0,							//UInt8 maxPower;
	};
static USBInterfaceDescriptor intfDesc ={
	9,						//UInt8 length;
	kUSBInterfaceDesc,		//UInt8 descriptorType;
	1,						//UInt8 interfaceNumber;
	0,						//UInt8 alternateSetting;
	1,						//UInt8 numEndpoints;
	kUSBHubClass,			//UInt8 interfaceClass;
	kUSBHubSubClass,		//UInt8 interfaceSubClass;
	1,						//UInt8 interfaceProtocol;
	0						//UInt8 interfaceStrIndex;
	};
static USBEndPointDescriptor endptDesc ={
	7,						//UInt8 length;
	kUSBEndpointDesc,		//UInt8 descriptorType;
	0x81,					//UInt8	 endpointAddress; In, 1
	kUSBInterrupt,			//UInt8 attributes;
	USB_CONSTANT16(8),		//UInt16 maxPacketSize;
	255,					//UInt8 interval;
	};
UInt8 confBuf[totalLen];
	
	if(descIndex != 0)
	{
		//return(true);	// temp don't make this fail while debugging root hub
	}
	if(totalLen != confDesc.length+intfDesc.length+endptDesc.length)
	{
		return(true);
	}
	usb_BlockMoveData(&confDesc, &confBuf[0], confDesc.length);
	usb_BlockMoveData(&intfDesc, &confBuf[confDesc.length], intfDesc.length);
	usb_BlockMoveData(&endptDesc, &confBuf[confDesc.length+intfDesc.length], endptDesc.length);
	
	length = totalLen;
	if(*bufLen < length)
	{
		length = *bufLen;
	}
	usb_BlockMoveData(&confBuf, buf, length);
	*bufLen -= length;
	
	return(false);
}

static Boolean SimulateGetADeviceDescriptor(setupPacket *packet, UInt8 *buf, UInt32 *bufLen)
{
UInt8 descType, descIndex;
	descType = USBToHostWord(packet->word[wValue]) >> 8;
	descIndex = USBToHostWord(packet->word[wValue]) & 0xff;
	if( (packet->word[wIndex] != 0) || (descType != kUSBStringDesc) )
	{
		/* This is not a string descriptor and its is not a string descriptor */
		if(descType == kUSBDeviceDesc)
		{
			return(SimulateGetDeviceDescriptor(buf, bufLen, descIndex));
		}
		else if(descType == kUSBConfDesc)
		{
			return(SimulateGetConfDescriptor(buf, bufLen, descIndex));
		}
		else
		{
			return(true);	/* Unknown dscriptor */
		}
	}
	else if(descType == kUSBStringDesc)
	{
		/* cope with string descriptor */
		return(true);
	}

	/* Non zero index with non string descriptor */
	return(true);
 
}

static Boolean SimulateGetHubDescriptor(setupPacket *packet, UInt8 *buf, UInt32 *bufLen)
{
uslRootHubDescriptor 	descr;
int bytesOfFlags, copy, length;

	if(packet->word[wValue] == 0)
	{
	
		if(UIMGetRootDescriptor(&descr) != noErr)
		{
			return(true);
		}
		
		length = *bufLen;
		
		if(length > 0)
		{
			/* Descriptor length */
			bytesOfFlags = (descr.numPorts+1)/8+1;
			buf[0] = bytesOfFlags*2 + 7;
			length--;
		}
		if(length > 0)
		{
			/* Descriptor type */
			buf[1] = kUSBHubDescriptorType;
			length--;
		}
		if(length > 0)
		{
			buf[2] = descr.numPorts;
			length--;
		}
		if(length > 0)
		{
			if(descr.powerSwitching)
			{
				buf[3] = !descr.gangedSwitching;
			}
			else
			{
				buf[3] = 2;
			}
			if(descr.compoundDevice)
			{
				buf[3] |= 4;
			}
			if(descr.overCurrent)
			{
				buf[3] = (!descr.globalOverCurrent) << 3;
			}
			else
			{
				buf[3] |= 0x10;
			}
			length--;
		}
		if(length > 0)
		{
			/* Flags high byte */			
			buf[4] = 0;
			length--;
		}
		if(length > 0)
		{
			/* power on to good */
			buf[5] = 50;	/* 100 ms? */
			length--;
		}
		if(length > 0)
		{
			/* Hub controller current */
			buf[6] = 0;	/* no cuurrent from bus */
			length--;
		}
		if(length > 0)
		{
			copy = length;
			if(copy > bytesOfFlags)
			{
				copy = bytesOfFlags;
			}
			usb_BlockMoveData(&descr.portFlags[0], &buf[7], copy);
			length -= copy;
		}
		if(length > 0)
		{
		int copy2;
			copy2 = length;
			if(copy2 > bytesOfFlags)
			{
				copy2 = bytesOfFlags;
			}
			usb_BlockMoveData(&descr.portFlags[copy], &buf[7+copy], copy2);
			length -= copy2;
		}
		
		*bufLen -= length;
		return(false);
	}
	
	return(true);


}
static Boolean SimulateClearPortFeature(setupPacket *packet)
{
UInt16 feature, port;
OSStatus err = -1;
	feature = USBToHostWord(packet->word[wValue]);
	port = USBToHostWord(packet->word[wIndex]);
	
	switch(feature)
	{
		case kUSBHubPortConnectionChangeFeature :
			err =UIMRootHubResetChangeConnection(port);
		break;
		
		case kUSBHubPortSuspendChangeFeature :
			err =UIMRootHubResetChangeSuspend(port);
		break;
		
		case kUSBHubPortEnablenFeature :
			err =UIMRootHubPortEnable(port, false);
		break;
		
		case kUSBHubPortPowerFeature :
			err =UIMRootHubPortPower(port, false);
			// Now need to check if all ports are switched off and 
			// gang off if in gang mode
		break;

		case kUSBHubPortResetChangeFeature :
			err =UIMRootHubResetResetChange(port);
		break;

		case kUSBHubPortEnableChangeFeature :
			err =UIMRootHubResetEnableChange(port);
		break;
		
		case kUSBHubPortOverCurrentChangeFeature :
			err =UIMRootHubResetOverCurrentChange(port);
		break;
		

	defualt:
		//DebugStr("\pUnknown port clear");
		USBExpertStatus(0,"\pUSL - Unknown port clear in root hub simulation", 0);
		break;
	}
	return(err != noErr);
}

static Boolean SimulateGetHubStatus(setupPacket *packet, UInt8 *buf, UInt32 *bufLen)
{
OSStatus err = -1;
static struct{
	UInt16 status;
	UInt16 change;
	}hubStatus = {0,0};	/* Change this is overcurrent is implimeneted */
UInt32	rhStatus;

	if( (packet->word[wValue] == 0) && (packet->word[wIndex] == 0) )
	{
		if ((err = UIMGetRhStatus(&rhStatus)) == noErr){
		
			// wouldn't it be handy to have HostToUSBLong
			hubStatus.status = HostToUSBWord(rhStatus & 0x0FFFF);
			hubStatus.change = HostToUSBWord((rhStatus >> 16) & 0x0FFFF);
			
			if(*bufLen > sizeof(hubStatus))
			{
				*bufLen = sizeof(hubStatus);
			}
			usb_BlockMoveData(&hubStatus, buf, *bufLen);
			err = noErr;
		}
	}
	return(err != noErr);
}


static Boolean SimulateGetPortStatus(setupPacket *packet, UInt8 *buf, UInt32 *bufLen)
{
uslRootHubPortStatus stat;
OSStatus err = -1;
UInt32 length;

	if(packet->word[wValue] == 0)
	{
		err = UIMRootHubGetPortStatus(USBToHostWord(packet->word[wIndex]), &stat);
		stat.portChangeFlags = USBToHostWord(stat.portChangeFlags);
		stat.portFlags = USBToHostWord(stat.portFlags);
		if(err == noErr)
		{
			length = *bufLen;
			if(length > sizeof(stat))
			{
				length = sizeof(stat);
			}
			usb_BlockMoveData(&stat, buf, length);
			*bufLen -= length;
		}
	}
	return(err != noErr);
}
static Boolean SimulateSetPortFeature(setupPacket *packet)
{
UInt16 feature, port;
OSStatus err = -1;
	feature = USBToHostWord(packet->word[wValue]);
	port = USBToHostWord(packet->word[wIndex]);
	switch(feature)
	{
		case kUSBHubPortSuspecdFeature :
			err = UIMRootHubPortSuspend(port, true);
		break;
		
		case kUSBHubPortResetFeature :
			err = UIMRootHubResetPort(port);
		break;
		
		case kUSBHubPortEnablenFeature :
			err = UIMRootHubPortEnable(port, true);
		break;
		
		case kUSBHubPortPowerFeature :
			err = UIMRootHubPortPower(port, true);
			if(err == noErr)
			{
				err = UIMRootHubPower(true);
			}
		break;
	
	default:
		//	DebugStr("\pUnknown port set");
		USBExpertStatus(0,"\pUSL - Unknown port set in root hub simulation", 0);
		break;
	}
	return(err != noErr);
}

static Boolean SimulateHubClearFeature(setupPacket *packet)
{
OSStatus err = -1;
	if(packet->word[wValue] == kUSBHubLocalPowerChangeFeature)
	{	
		USBExpertStatus(0,"\pRoot hub simulation - unimplemented Clear Power Change Feature", 0);
		// err = UIMRootHubLPSChange(false);	// not implemented yet
	}
	if(packet->word[wValue] == kUSBHubOverCurrentChangeFeature)
	{	
		USBExpertStatus(0,"\pRoot hub simulation - unimplemented Clear Overcurrent Change Feature", 0);
		// err = UIMRootHubOCChange(false);	// not implemented yet
	}	
	return(err != noErr);
}
static Boolean SimulateHubSetFeature(setupPacket *packet)
{
OSStatus err = -1;
	if(packet->word[wValue] == kUSBHubLocalPowerChangeFeature)
	{	
		USBExpertStatus(0,"\pRoot hub simulation - unimplemented Set Power Change Feature", 0);
		// err = UIMRootHubLPSChange(true);	// not implemented yet
	}
	if(packet->word[wValue] == kUSBHubOverCurrentChangeFeature)
	{	
		USBExpertStatus(0,"\pRoot hub simulation - unimplemented Set Overcurrent Change Feature", 0);
		// err = UIMRootHubOCChange(true);	// not implemented yet
	}	
	return(err != noErr);
}

typedef struct{
	UInt8 *buf;
	UInt32 bufLen;
	CallBackFuncPtr handler;
	UInt32 refCon;
	}intTrans;
#define kMaxOutstandingTrans 4

static intTrans outstanding[kMaxOutstandingTrans];

static UInt32 intLock;


static void rootHubStatusChangeHandler(void)
{	/* Only called if the RHSC bit is set at int. */
UInt16 statusChanged;	/* only have 15 ports in OHCI */
UInt32 status, statusBit;
uslRootHubDescriptor 	descr;
static numPorts;
int index, move;
uslRootHubPortStatus stat;
intTrans last;
	
	last = outstanding[0];
	if(last.handler == nil)
	{
		return;
	}
	
	if(CompareAndSwap(0, 1, &intLock))
	{
		for(index = 1; index < kMaxOutstandingTrans ; index++)
		{
			outstanding[index-1] = outstanding[index];
			if(outstanding[index].handler == nil)
			{
				break;
			}
		}
	
		statusChanged = 0;
		statusBit = 1;
		if(UIMGetRhStatus(&status) == noErr)
		{
			if( (status & kOHCIHcRhStatus_Change ) != 0)
			{
				statusChanged |= statusBit;	/* Hub status change bit */
			}
			
			if(numPorts == 0)
			{
				if(UIMGetRootDescriptor(&descr) == noErr)
				{
					numPorts = descr.numPorts;
				}
				else
				{
					//DebugStr("\pRoot hub int simulation hosed. No ports, tell Barry");
					USBExpertStatus(0,"\pUSL - Root hub int simulation hosed. No ports, tell Barry", 0);
				}
			}

			for(index = 1; index <= numPorts; index++)
			{
				statusBit <<= 1;	/* Next bit */
				
				if(UIMRootHubGetPortStatus(index, &stat) == noErr)
				{
					if(stat.portChangeFlags != 0)
					{
						statusChanged |= statusBit;	/* Hub status change bit */
					}
				}
				
			}
			
		}
		
		move = last.bufLen;
		if(move > sizeof(statusChanged))
		{
			move = sizeof(statusChanged);
		}
		if(numPorts < 8)
		{
			move = 1;
		}
		statusChanged = HostToUSBWord(statusChanged);
		usb_BlockMoveData(&statusChanged, last.buf, move);
		CompareAndSwap(1, 0, &intLock);	/* Unlock the queue */
		(*last.handler)(last.refCon, noErr, last.bufLen - move);
	}
}




void pollRootHubSim(void)
{
UInt32 intrStatus;

	if(outstanding[0].handler != nil)
	{
		if( (UIMGetInterruptStatus(&intrStatus) == noErr) &&
		    ((intrStatus & kOHCIHcInterrupt_RHSC) != 0) )
		{
			UIMClearInterruptStatus(kOHCIHcInterrupt_RHSC);
			rootHubStatusChangeHandler();
		}
	}
}

void SimulateRootHubInt(UInt8 endpoint, UInt8 *buf, UInt32 bufLen,
						CallBackFuncPtr handler, UInt32 refCon)
{
int index;

	if(endpoint != 1)
	{
		(*handler)(refCon, -1, bufLen);
		return;
	}

	if(!CompareAndSwap(0, 1, &intLock))
	{	/* Busy */
		(*handler)(refCon, -1, bufLen);
		return;
	}

	for(index = 0; index < kMaxOutstandingTrans; index++)
	{
		if(outstanding[0].handler == nil)
		{
			/* found free trans */
			outstanding[0].buf = buf;
			outstanding[0].bufLen = bufLen;
			outstanding[0].handler = handler;
			outstanding[0].refCon = refCon;
			CompareAndSwap(1, 0, &intLock);	/* Unlock the queue */
			return;
		}
	}

	CompareAndSwap(1, 0, &intLock);	/* Unlock the queue */
	(*handler)(refCon, -1, bufLen);	/* too many trans */
	
}

void SimulateRootHub(setupPacket *packet, UInt8 endpoint, UInt8 *buf, UInt32 inBufLen,
						CallBackFuncPtr handler, UInt32 refCon)
{
UInt8 dirn, type, recipient;
Boolean stall;
UInt32 *bufLen;
	if(endpoint != 0)
	{
		return;
	}
	bufLen = &inBufLen;	/* Barry, was stamping on param block field */

#if 0
typedef union{
	UInt8 byte[8];
	UInt16 word[4];
	}setupPacket;

enum{
/* Byte offsets */
	bmRqType = 0,
	bUsedFlag = bmRqType,
	bRequest,
/* Word offsets */
	wValue =1,	
	wIndex,
	wLength
	};
#endif

	stall = false;
	dirn = (packet->byte[bmRqType] >> kUSBRqDirnShift) & kUSBRqDirnMask;
	type = (packet->byte[bmRqType] >> kUSBRqTypeShift) & kUSBRqTypeMask;
	recipient = packet->byte[bmRqType] & kUSBRqRecipientMask;

	if(type == kUSBStandard)
	{

#if 0
Standard Requests

bmRequestType bRequest wValue wIndex wLength Data
00000000B		CLEAR_FEATURE	device
00000001B						Interface
00000010B						Endpoint

10000000B GET_CONFIGURATION Zero Zero One Configuration
10000000B GET_DESCRIPTOR Descriptor
10000001B GET_INTERFACE Zero Interface One Alternate

10000000B		GET_STATUS		device
10000001B						Interface
10000010B						Endpoint

00000000B SET_ADDRESS Device
00000000B SET_CONFIGURATION Configuration
00000000B SET_DESCRIPTOR Descriptor

00000000B		SET_FEATURE		device
00000001B						Interface
00000010B						Endpoint

00000001B SET_INTERFACE Alternate
10000010B SYNCH_FRAME Zero Endpoint Two Frame Number#endif
#endif
		switch(packet->byte[bRequest])
		{
			case kUSBRqClearFeature:
				if(packet->byte[bmRqType] == 0x00)
				{
					/* Clear Device Feature */
					stall = SimulateHubClearFeature(packet);
				}
				else if(packet->byte[bmRqType] == 0x01)
				{
					/* Clear Interface Feature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 2");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 2", 0);
				}
				else if(packet->byte[bmRqType] == 0x02)
				{
					/* Clear Endpoint Feature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 3");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 3", 0);
				}
				else
				{
					stall = true;
				}
			break;
		
			case kUSBRqGetConfig:
				if(packet->byte[bmRqType] == 0x80)
				{
					/* Get configuration */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 4");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 4", 0);
				}
				else
				{
					stall = true;
				}
			break;
		
			case kUSBRqGetDescriptor:
				if(packet->byte[bmRqType] == 0x80)
				{
					/* Get descriptor */
					stall = SimulateGetADeviceDescriptor(packet, buf, bufLen);
				}
				else
				{
					stall = true;
				}
			
			break;
		
			case kUSBRqGetInterface:
				if(packet->byte[bmRqType] == 0x81)
				{
					/* Get interface */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 6");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 6", 0);
				}
				else
				{
					stall = true;
				}
			break;
		
			case kUSBRqGetStatus:
				if(packet->byte[bmRqType] == 0x80)
				{
				UInt16 status;
				int copy;
					/* Get Device Status */
					stall = false;
					status = USB_CONSTANT16(1);		// self powered
					copy = sizeof(status);
					if(copy > *bufLen)
					{
						copy = *bufLen;
					}
					usb_BlockMoveData(&status, buf, copy);
					*bufLen -= copy;
					
				}
				else if(packet->byte[bmRqType] == 0x81)
				{
					/* Get Interface Status */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 8");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 8", 0);
				}
				else if(packet->byte[bmRqType] == 0x82)
				{
					/* Get Endpoint Status */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 9");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 9", 0);
				}
				else
				{
					stall = true;
				}
			break;
		
			case kUSBRqSetAddress:
				if(packet->byte[bmRqType] == 0x00)
				{
					/* Set address */
					stall = UIMSetOurAddress(USBToHostWord(packet->word[wValue]));
				}
				else
				{
					stall = true;
				}			
			break;
		
			case kUSBRqSetConfig:
				if(packet->byte[bmRqType] == 0x00)
				{
					/* Set configuration */
					stall = SimulateSetConfiguration(USBToHostWord(packet->word[wValue]));
				}
				else
				{
					stall = true;
				}			
			break;
		
			case kUSBRqSetDescriptor:
				if(packet->byte[bmRqType] == 0x00)
				{
					/* Set descriptor */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 12");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 12", 0);
				}
				else
				{
					stall = true;
				}			
			break;
		
			case kUSBRqSetFeature:
				if(packet->byte[bmRqType] == 0x00)
				{
					/* Set Device Feature */
					stall = SimulateHubSetFeature(packet);
				}
				else if(packet->byte[bmRqType] == 0x01)
				{
					/* Set Interface Feature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 14");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 14", 0);
				}
				else if(packet->byte[bmRqType] == 0x02)
				{
					/* Set Endpoint Feature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 15");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 15", 0);
				}
				else
				{
					stall = true;
				}
			break;
		
			case kUSBRqSetInterface:
				if(packet->byte[bmRqType] == 0x01)
				{
					/* Set interface */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 16");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 16", 0);
				}
				else
				{
					stall = true;
				}			
			break;
		
			case kUSBRqSyncFrame:
				stall = true;
			break;

			default:
				stall = true;
			break;

		}
	}
	else if(type == kUSBClass)
	{

#if 0
Class-specific Requests

ClearHubFeature  0010 0000B CLEAR_FEATURE Feature
ClearPortFeature 0010 0011B CLEAR_FEATURE Feature
GetBusState      1010 0011B GET_STATE Zero Port One Per Port
GetHubDescriptor 1010 0000B GET_DESCRIPTOR Descriptor
GetHubStatus     1010 0000B GET_STATUS Zero Zero Four Hub Status
GetPortStatus    1010 0011B GET_STATUS Zero Port Four Port Status
SetHubDescriptor 0010 0000B SET_DESCRIPTOR Descriptor
SetHubFeature    0010 0000B SET_FEATURE Feature
SetPortFeature   0010 0011B SET_FEATURE Feature
#endif		
		switch(packet->byte[bRequest])
		{
			case kUSBRqClearFeature:
				if(packet->byte[bmRqType] == 0x20)
				{
					/* ClearHubFeature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 17");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 17", 0);
				}
				else if(packet->byte[bmRqType] == 0x23)
				{
					stall = SimulateClearPortFeature(packet);
				}
				else
				{
					stall = true;
				}
			
			break;
				
			case kUSBRqReserved1 /*GET_STATE*/:
				if(packet->byte[bmRqType] == 0xA3)
				{
					/* GetBusState */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 18");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 18", 0);
				}
				else
				{
					stall = true;
				}
			break;
				
			case kUSBRqGetDescriptor:
				if(packet->byte[bmRqType] == 0xA0)
				{
					/* GetHubDescriptor */
					SimulateGetHubDescriptor(packet, buf, bufLen);
				}
				else
				{
					stall = true;
				}
			
			break;
				
			case kUSBRqGetStatus:
				if(packet->byte[bmRqType] == 0xA0)
				{
					/* GetHubStatus */
					stall = SimulateGetHubStatus(packet, buf, bufLen);
				}
				else if(packet->byte[bmRqType] == 0xA3)
				{
					stall = SimulateGetPortStatus(packet, buf, bufLen);
				}
				else
				{
					stall = true;
				}
			
			break;
				
			case kUSBRqSetDescriptor:
				if(packet->byte[bmRqType] == 0x20)
				{
					/* SetHubDescriptor */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 20");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 20", 0);
				}
				else
				{
					stall = true;
				}
			
			break;
				
			case kUSBRqSetFeature :
				if(packet->byte[bmRqType] == 0x20)
				{
					/* SetHubFeature */
					stall = true;
					//DebugStr("\pRoot hub not implemented yet 21");
					USBExpertStatus(0,"\pUSL - Root hub not implemented yet 21", 0);
				}
				else if(packet->byte[bmRqType] == 0x23)
				{
					stall = SimulateSetPortFeature(packet);
				}
				else
				{
					stall = true;
				}
			break;

			default:
				stall = true;
			break;
		}

	}
	else
	{
		stall = true;
	}
	
	if(stall)
	{
		//DebugStr("\pSim Stall");
		USBExpertStatus(0,"\pUSL - Root hub simulated stall", 0);
	}
	(*handler)(refCon, stall?4:noErr, *bufLen);
}

void ResetRootHubSimulation(void)
{
	int	index;
	
	for(index = 0; index < kMaxOutstandingTrans; index++)
	{
		outstanding[0].handler = nil; // clear out any outstanding transactions
	}
	intLock = 0;
}

