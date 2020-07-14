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
	File:		usl.c

 	Contains:	USB Services Library source code.
 
 	Version:	Neptune 1.0
 
	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DF)	David Ferguson
		(TC)	Tom Clark
		(BG)	Bill Galcher
		(DKF)	David Ferguson
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB76>	11/25/98	TC		Add calls to USBExpertStatusLevel.
	 <USB75>	11/16/98	BT		Mark pipe stalled on allocate to check open pipe is clearing it
									properly.
	 <USB74>	 11/8/98	TC		We need to assign a unique DeviceRef for UIM's. So when creating
									DeviceRefs, don't return one that matches kUIMRefID.
	 <USB73>	 11/6/98	TC		Formalize parameter names for uslRefToBus.
	 <USB72>	 11/6/98	BT		ref to bus function
	 <USB71>	10/29/98	BT		Close old configurations on reset or reconfiguration.
	 <USB70>	10/21/98	DF		Add uslDeviceAddrToBus function
	 <USB69>	 10/7/98	BT		Fix hub not killing children
	 <USB68>	 9/17/98	BT		1.1 rules for captive devices.
	 <USB67>	 8/24/98	BT		Eliminate old checkPB version, add new one for Isoc
	 <USB66>	 8/13/98	BT		Add multibus support
	 <USB65>	 7/10/98	BT		Device zero has a valid ref, delete device deletes mem even
									after trashing device and ref.
	 <USB64>	  7/9/98	BT		Fix previous fixes. Device refs can also be validated
	 <USB63>	  7/9/98	BT		Clean up after delete device and config clash.
	 <USB62>	 6/13/98	BT		Eliminate V1 param blocks
	 <USB61>	 6/10/98	CJK		add uslGetDeviceRefByID function (thanks Barry!)
	 <USB60>	  6/5/98	BT		Delete mem properly in delete device
	 <USB59>	  6/5/98	TC		Change uxBusRef to USBBusRef.
	 <USB58>	  6/3/98	BT		Make dispose interface ref able to ignore call back
	 <USB57>	 5/21/98	BT		Don't initialise redundant firstConf field in dev structure
	 <USB56>	 5/20/98	BT		Add check for V2 param block function
	 <USB55>	 5/19/98	BG		Fixed ZeroMem() be a little easier for MrC to swallow.
	 <USB54>	 5/17/98	BT		Impliment new interface ref correctly, add some functions to
									support config stuff
	 <USB53>	 5/12/98	BT		New interface handling
	 <USB52>	  5/8/98	BT		Add open interface for Tom
	 <USB51>	 4/26/98	BT		Add pipe state control
	 <USB50>	 4/24/98	BT		Add clear endpoint stall
	 <USB49>	 4/24/98	CJK		change string for status message
	 <USB48>	 4/21/98	BT		Allow requests to device zero, pipe zero.
	 <USB47>	 4/20/98	BT		Add abort pipe
	 <USB46>	 4/16/98	BT		Add back UIMpriv prototypes
	 <USB45>	 4/15/98	BT		Remove redundant diagnostic
	 <USB44>	 4/15/98	BT		Fix wrong flags check.
	 <USB43>	 4/14/98	BT		Impliment device remove
	 <USB42>	  4/9/98	CJK		remove uimpriv.h
	 <USB41>	  4/9/98	BT		Use USB.h
		<40>	  4/8/98	BT		More error checking
		<39>	  4/6/98	BT		Change w names
		<38>	  4/6/98	BT		New param block names
		<37>	  4/2/98	BT		Start on config/interface/endpoint stuff. First caching config
									descriptors.
		<36>	 3/24/98	BT		Record device speed in device structure. (Fixes low speed int
									pipe problems.)
		<35>	 3/24/98	BT		Fix recording of Maxpacketsize
		<34>	 3/19/98	BT		Stop double inititialisation
		<33>	 3/19/98	BT		Open bulk takes max poacket size in usbValue.
		<32>	 3/18/98	BT		Add remove device.
		<31>	  3/5/98	BT		Add interrupt pipe
	 <USB30>	 2/17/98	DKF		Fix a typo in the error codes
		<29>	 2/16/98	BT		Direct notifications to expert
		<28>	  2/9/98	BT		Add add interface stuff
		<27>	  2/8/98	BT		Power Allocation stuff
		<26>	  2/5/98	BT		Add status notification stuff
		<25>	  2/4/98	BT		Add ref/port info to expert notify]
		<24>	  2/4/98	BT		More USBprobe support
		<23>	  2/3/98	BT		Fix uslControlPacket to have errors. Add mnore support for
									usbProbe. Also add USBExpertNotify
		<22>	  2/2/98	BT		Add bulk stuff
		<21>	 1/29/98	BT		Utility stuff split off
		<20>	 1/26/98	CJK		Change to use USBDeviceDescriptor (instead of just
									deviceDescriptor)
		<19>	 1/26/98	BT		Mangle names after design review, finish up
		<18>	 1/26/98	BT		Mangle names after design review
		<17>	 1/26/98	BT		Make expert notify public
		<16>	 1/26/98	BT		Fix status in USB delay function
		<15>	 1/23/98	BT		Add USB delay function
		<14>	 1/20/98	BT		Increase number of timers
		<13>	 1/19/98	BT		Fix race condition
		<12>	 1/15/98	BT		Use new USBClassDrivers.h
		<11>	 1/14/98	BT		Remove outdated expert stuff
		<10>	 1/13/98	BT		User timer services for task time, starting implimenting Int
									transactions (not finished)
		 <9>	12/23/97	BT		Remove obsolete include
		 <8>	12/19/97	BT		Add add and remove bus.
		 <7>	12/19/97	BT		UIM now a Shared lib
		 <6>	12/18/97	BT		Changed expert proc declaratiopn in USLInitialise
		 <5>	12/18/97	BT		Fix header dependancies, add expert call back
		 <4>	12/17/97	BT		Fix header
		 <3>	12/17/97	BT		First checking to BBS
*/



/* ************* Includes ************* */
#include "../USB.h"
#include "../USBpriv.h"
#include "../driverservices.h"

#include "uslpriv.h"
#include "../uimpriv.h" // This does need to be included. It contains prototypes
                                           // Stupid MPW just doesn't tell
/*****************************************************************************

	Data Structures
	
There is an array of devices. 

Each device entry has information specific to it. It also holds and array of
indexes of pipes (open endpoints) for the device.

Each open pipe holds information specific to it.

Devices are refrenced by device IDs, which are currently an index in tot the
devices table in the lower bits and random number in the high bits. This 
knowledge is kept to a few functions and could be changed.

The pipe refs are similarly indexes in to the pipes array.

******************************************************************************/

static pipe AllPipes[kUSBMaxPipes];
static usbDevice devices[kUSBMaxDevices];

enum{
	kUSBInterfaceShift = 8,
	kUSBMaxInterface = 1 << kUSBInterfaceShift,
	kUSBIntefaceMask = kUSBMaxInterface-1
	};
static uslInterface interfaces[kUSBMaxInterface];

//static usbDevice *devices;

static Boolean initialised;


/* ************* CallBack prototyopes. ************* */
/* These need to be referenced before the properplace for their definition */
static void controlCallBack(
		UInt32	refcon,				/* General Purpose reference supplied by call */
		OSStatus status, 			/* normal or error condition */
		short shortFall); 			/* how much less than requested transferred */




/* ************* Actual Code *************** */

/* This doesn't work without length bytes in the strings */

OSStatus USBServicesGetVersion(UInt32 ID, VersRec *vers, UInt32 versSize)
{
#if 0
static VersRec ID1=
#if kOverrideIndividualVersions
	{
	{kPKGHexMajorVers, kPKGHexMinorVers, kPKGCurrentRelease, kPKGReleaseStage},
	0,					/* Country code */
	"\p"kPKGStringVersShort, "\p"kPKGStringVers1Long
	
	},
#else
	{
	{kSRVHexMajorVers, kSRVHexMinorVers, kSRVCurrentRelease, kSRVReleaseStage},
	0,					/* Country code */
	kMNGRStringVersShort kMNGRStringVers1Long
	
	},
#endif
	ID2 = {
	{kPKGHexMajorVers, kPKGHexMinorVers, kPKGCurrentRelease, kPKGReleaseStage},
	0,					/* Country code */
	kPKGStringVersShort kPKGStringVers1Long
	
	},
	*theVers;

	if( (ID == 0) || (ID > 2) )
	{
		return(paramErr);
	}
	if(ID == 1)
	{
		theVers = &ID1;
	}
	else
	{
		theVers = &ID2;
	}
	if(versSize > sizeof(VersRec) )
	{
		versSize = sizeof(VersRec);
	}
	usb_BlockMoveData(theVers, vers, versSize);
#endif
	ID = 0;
	vers = 0;
	versSize = 0;
	return(noErr);
}
/* ******** Interfaces ********** */

/* This impliments the arbitrary high bits as a counter. */
static USBInterfaceRef MakeInterfaceRef(UInt16 addr)
{
static long strangeCounter = 0x980508;
long shiftedCounter;

	strangeCounter++;
	shiftedCounter = strangeCounter << kUSBInterfaceShift;
	if(shiftedCounter == 0)
	{
		shiftedCounter = ++strangeCounter << kUSBInterfaceShift;
	}
	return(addr | shiftedCounter | 0x80000000);	// high bit is one
}

/* Given a interfaceRef find the pointer to its structure */
	
OSStatus findInterface(USBInterfaceRef InterfaceRef, uslInterface **p)
{
InterfacesIdx idx;
uslInterface *theInterface;

	idx = InterfaceRef & kUSBIntefaceMask;
	theInterface = & interfaces[idx];
	*p = theInterface;
	if(theInterface->ref == InterfaceRef)
	{
#if 0
		if(theInterface->state != kUSBActive)
		{
			if(theInterface->state == kUSBIdle)
			{
				return(kUSBInterfaceIdleError);
			}
			else
			{
				return(kUSBInterfaceStalledError);
			}			
		}
#endif
		return(noErr);
	}
	*p = nil;
	return(kUSBUnknownInterfaceErr);
}


/* This probably is not the best method for Interfaces */
static InterfacesIdx AllocInterfaceIdx(void)
{
static InterfacesIdx current;
InterfacesIdx new;
unsigned counted;

/* Allocates a Interface. This implimentation has a pointer to */
/* the next Interface to be allocated, which is incrimented each */
/* time. If that Interface is in use, the list is search linearly */
/* from that point for the next free Interface */
/* Same as packet allocator */

/* Do we need to use atomicTestAndSet for this? */
/* Not if transactions are queued */

	new = current;
	counted = 0;

	while(counted < kUSBMaxInterface)
	{
		if(interfaces[new].ref == 0)	/* Zero not a valid Interface ref */
		{
			break;
		}
		counted++;
		new++;
		if(new > kUSBMaxInterface)
		{
			new = 0;
		}
		
	}
	if(counted < kUSBMaxInterface)
	{
		if(counted > 0)
		{
			/* maybe keep track of these at some time */
		}
		interfaces[new].ref = 0;
	}
	else
	{
		/* maybe keep track of these at some time */
		new = -1;
	}

	current++;
	if(current > kUSBMaxInterface)
	{
		current = 0;
	}
	return(new);
}

/* Allocate a new Interface structure for the given device and endpoint */
uslInterface *AllocInterface(USBDeviceRef device)
{
long newIdx;

	newIdx = AllocInterfaceIdx();
	if(newIdx == -1)
	{
		return(nil);
	}
	interfaces[newIdx].ref = MakeInterfaceRef(newIdx);
	interfaces[newIdx].device = device;
	return(&interfaces[newIdx]);
}


static void deallocAllDeviceInterfaces(USBDeviceRef ID)
{
int i;
	for(i = 0; i<kUSBMaxInterface ; i++)
	{
		if(interfaces[i].device == ID)
		{
			USBExpertStatusLevel(4, interfaces[i].ref, "USL - Delete undeleted interface", 0);
			
			interfaces[i].device = 0;
			interfaces[i].interfaceNum = 0;
			interfaces[i].alt = 0;
			interfaces[i].ref = 0;
		}
	}


}

/* ************* Device Table maintainance internal functions *************/
unsigned makeDeviceIdx(USBDeviceRef device)
{
	return(device & kUSBDeviceIDMask);
}

Boolean uslIsInterfaceRef(USBInterfaceRef interf)
{
	return((interf & 0x80000000) != 0);
}

usbDevice *getDevicePtrFromIdx(unsigned deviceIndex)
{
	if(deviceIndex > kUSBMaxDevices)
	{
		return(nil);
	}
	return(&devices[deviceIndex]);
}

USBDeviceRef uslGetDeviceRefByID(unsigned short deviceID, USBBusRef bus);

USBDeviceRef uslGetDeviceRefByID(unsigned short deviceID, USBBusRef bus)
{
usbDevice *device;
	if(bus != 1)
	{
		return(kUSBUnknownDeviceErr);
	}
	device = getDevicePtrFromIdx(deviceID);
	if(device == nil)
	{
		return(kUSBUnknownDeviceErr);
	}
	return(device->ID);

}

usbDevice *getDevicePtr(USBDeviceRef device)
{
unsigned deviceIndex;

	if( (device & 0x80000000) != 0)
	{
	OSStatus err;
	uslInterface *intrfc;
	
		err = findInterface(device, &intrfc);
		if(intrfc == nil)
		{
			return(nil);
		}
		device = intrfc->device;
	}

	deviceIndex = makeDeviceIdx(device);
	if(devices[deviceIndex].ID == device)
	{
		return(&devices[deviceIndex]);
	}
	return(nil);
}

OSStatus uslRefToBus(USBReference ref, USBBusRef *bus)
{
usbDevice *dev;
	dev = getDevicePtr(ref);
	if(dev != nil)
	{
		if(bus != nil)
		{
			*bus = dev->bus;
		}
		return(noErr);
	}
	else
	{
		return(kUSBUnknownDeviceErr);
	}

}


/* This one is a prober only hack function */
UInt32 uslDeviceAddrToBus(UInt16 addr)
{
unsigned deviceIndex;

	for(deviceIndex = 0; deviceIndex < kUSBMaxDevices; deviceIndex++)
	{
		if ((devices[deviceIndex].ID != 0) && (devices[deviceIndex].usbAddress == addr))
		{
			return(devices[deviceIndex].bus);
		}
	}
	return (0xffff);  // indicate that we couldn't find a bus
}



static void initialiseOneDevice(usbDevice *device)
{	/* Shared by delete device */
	device->ID = 0;	
}

static void initialiseDevices(void)
{
unsigned deviceIndex;

	for(deviceIndex = 0; deviceIndex < kUSBMaxDevices; deviceIndex++)
	{
		initialiseOneDevice(&devices[deviceIndex]);
	}

}

UInt8 getNewAddress(void)
{
static int addr = -1;
int counted = 1;
	if(addr == -1)
	{
	AbsoluteTime t;
		t = UpTime();
		//naga addr = t.lo & 0x7f;
		addr = t & 0x7f;
	}
	do{
		addr++;
		if(addr > 127)
		{
			addr = 1;
		}
		if(devices[addr].ID == 0)
		{
			break;
		}
	}while(counted++ <= 127);
	
	if(counted > 127)
	{
		addr = 0;
	}
	
	USBExpertStatusLevel(4, 0, "USL - New device:", addr); // Paging Mr. Twycross
kprintf("****getNewAddress returning addr=%d***\n",addr);
	return(addr);
}


/* This impliments the arbitrary high bits as a counter. */
USBDeviceRef MakeDevRef(UInt32 bus, UInt16 addr)
{
static long strangeCounter = 0x090263;
long shiftedCounter;

	strangeCounter++;
	shiftedCounter = strangeCounter << kUSBDeviceIDShift;
	if(shiftedCounter == 0)
	{
		shiftedCounter = ++strangeCounter << kUSBDeviceIDShift;
	}

	bus = 0; // currenty not used.
	return( (addr + shiftedCounter) & 0x7fffffff);	// High bit is zero
}

/* ************* Pipe Table Maintainance internal functions ************* */

/* So that stale pipeRefs can not be reused, the pipe ref is */
/* the index into the pipe table (the interesting part) */
/* and an arbitrary value stuffing the high bits. */
/* A pipe ref is only accepted if it matches the pipeRef */
/* stored in the pipe structure. */

/* This impliments the arbitrary high bits as a counter. */
static USBPipeRef MakePipeRef(long pipeIdx)
{
static long strangeCounter = 0x230259;
long shiftedCounter;

	strangeCounter++;
	shiftedCounter = strangeCounter << kUSBMaxPipeIDShift;
	if(shiftedCounter == 0)
	{
		shiftedCounter = ++strangeCounter << kUSBMaxPipeIDShift;
	}
	return(pipeIdx + shiftedCounter);
}

/* Given a pipeRef find the pointer to its structure */
	
OSStatus findPipe(USBPipeRef pipeRef, pipe **p)
{
pipesIdx idx;
pipe *thePipe;

	idx = pipeRef & kUSBPipeIDMask;
	thePipe = & AllPipes[idx];
	*p = thePipe;
	if(thePipe->ref == pipeRef)
	{
		if(thePipe->state != kUSBActive)
		{
			if(thePipe->state == kUSBIdle)
			{
				return(kUSBPipeIdleError);
			}
			else
			{
				return(kUSBPipeStalledError);
			}			
		}
		return(noErr);
	}
	*p = nil;	// BT some things rely on this being nil if no pipe, oops.
	return(kUSBUnknownPipeErr);
}


/* This probably is not the best method for pipes */
static pipesIdx AllocPipeIdx(void)
{
static pipesIdx current;
pipesIdx new;
unsigned counted;

/* Allocates a pipe. This implimentation has a pointer to */
/* the next pipe to be allocated, which is incrimented each */
/* time. If that pipe is in use, the list is search linearly */
/* from that point for the next free pipe */
/* Same as packet allocator */

/* Do we need to use atomicTestAndSet for this? */
/* Not if transactions are queued */

	new = current;
	counted = 0;

	while(counted < kUSBMaxPipes)
	{
		if(AllPipes[new].ref == 0)	/* Zero not a valid pipe ref */
		{
			break;
		}
		counted++;
		new++;
		if(new > kUSBMaxPipes)
		{
			new = 0;
		}
		
	}
	if(counted < kUSBMaxPipes)
	{
		if(counted > 0)
		{
			/* maybe keep track of these at some time */
		}
		AllPipes[new].ref = 0;
		AllPipes[new].state = kUSBStalled;	// This is just here to tell me an open pipe forgot to set state
	}
	else
	{
		/* maybe keep track of these at some time */
		new = -1;
	}

	current++;
	if(current > kUSBMaxPipes)
	{
		current = 0;
	}
	return(new);
}

int makeDevPipeIdx( UInt8 endpoint, UInt8 direction)
{
int devPipeIdx;
	devPipeIdx = endpoint-1;
	if(direction != kUSBOut)
	{
		devPipeIdx += kUSBMaxEndptPerDevice/2;
	}
	return(devPipeIdx);
}

/* Given a device and an endpoint, wheres the pipe? */
pipe *getPipe(usbDevice *device, UInt8 endpoint, UInt8 direction)
{
pipesIdx index;
	index = device->pipes[makeDevPipeIdx(endpoint, direction)];
	if(index == -1)
	{
		return(nil);
	}
	else
	{
		return(&AllPipes[index]);
	}
}

pipe *getPipeByIdx(pipesIdx idx)
{
	if(idx > kUSBMaxPipes)
	{
		return(nil);
	}
	
	return(&AllPipes[idx]);

}

/* Allocate a new pipe structure for the given device and endpoint */
pipe *AllocPipe(usbDevice *device, UInt8 endpoint, UInt8 direction)
{
long newIdx;

	newIdx = AllocPipeIdx();
	if(newIdx == -1)
	{
		return(nil);
	}
	device->pipes[makeDevPipeIdx(endpoint, direction)] = newIdx;
	AllPipes[newIdx].ref = MakePipeRef(newIdx);
	return(&AllPipes[newIdx]);
}


/* XXXXXXXX More to be done */
void deallocPipe(usbDevice *device, UInt8 endpoint, UInt8 direction)
{
pipesIdx index;
int devPipeIdx;

	devPipeIdx = makeDevPipeIdx(endpoint, direction);
	index = device->pipes[devPipeIdx];
	AllPipes[index].ref = 0;
	AllPipes[index].state = kUSBStalled;	// This is just here to tell me an open pipe forgot to set state
	device->pipes[devPipeIdx] = -1;
}


OSStatus uslClosePipe(pipesIdx pipe)
{
OSStatus err;
	if(pipe == -1)
	{
		return(noErr);
	}
	if(AllPipes[pipe].type == kUSBControl)
	{
kprintf("uslClosePipe:calling UIMControlEDDelete\n");
		err = UIMControlEDDelete(AllPipes[pipe].bus, AllPipes[pipe].devAddress, AllPipes[pipe].endPt);
	}
	else
	{
		err = UIMEDDelete(AllPipes[pipe].bus, AllPipes[pipe].devAddress, AllPipes[pipe].endPt, AllPipes[pipe].direction);
	}

	AllPipes[pipe].ref = 0;
	AllPipes[pipe].state = kUSBStalled;	// This is just here to tell me an open pipe forgot to set state
	return(noErr);
}

void uslCloseNonDefaultPipes(usbDevice *deviceP)
{
int i;
	for(i = 0; i< kUSBMaxEndptPerDevice; i++)
	{
		uslClosePipe(deviceP->pipes[i]);
		deviceP->pipes[i] = -1;
	}
}

void uslCloseInterfacePipes(USBInterfaceRef iRef, usbDevice *deviceP)
{
int i;
	for(i = 0; i< kUSBMaxEndptPerDevice; i++)
	{
		if(AllPipes[deviceP->pipes[i]].devIntfRef == iRef)
		{
			uslClosePipe(deviceP->pipes[i]);
			deviceP->pipes[i] = -1;
		}
	}
}


OSStatus validateRef(USBReference ref, UInt32 *bus)
{
OSStatus err;
usbDevice *dev;
pipe *p;

	dev = getDevicePtr(ref);
	if(dev != nil)
	{
		if(bus != nil)
		{
			*bus = dev->bus;
		}
		return(noErr);
	}
	err = findPipe(ref, &p);
	if(p != nil)
	{
		if(bus != nil)
		{
			*bus = p->bus;
		}
		return(noErr);
	}
	if(uslHubValidateDevZero(ref, bus))
	{
		return(noErr);
	}
	USBExpertStatusLevel(2, ref, "USL - Invalid ref passed to USL", 0);

	return(kUSBUnknownDeviceErr);
}

Boolean isSameDevice(USBReference ref, USBDeviceRef devRef)
{	// true if the ref refers to the device
usbDevice *dev;
OSStatus err;
pipe *p;

	dev = getDevicePtr(ref);
	if(dev == nil)
	{
		err = findPipe(ref, &p);
		if(p == nil)
		{
			return(false);
		}
		
		dev = getDevicePtr(p->devIntfRef);
	}
	if(dev != nil)
	{
		if(dev->ID == devRef)
		{
			return(true);
		}
	}
	return(false);
}

static void ZeroMem(void *addr, UInt32 len)
{
UInt8	*byteAddr;

	if(len == 0)
		return;

	byteAddr = (UInt8 *) addr;
	while(len--)
	{
		*byteAddr++ = 0;
	}
}


void uslUnconfigureDevice(usbDevice *device)
{
	uslCloseNonDefaultPipes(device);
	deallocAllDeviceInterfaces(device->ID);
	device->currentConfiguration = 0;
}

OSStatus uslDeleteDevice(USBPB *pb, usbDevice *device)
{	/* All pipes are closed, kill off the device */
OSStatus retVal = noErr;
void *allConfDesc;

	if(device->configLock != 0)
	{
		USBExpertStatusLevel(2, device->ID, "USL - Device busy when deleting: Address ==", device->usbAddress);
		device->killMe = true;
		if(device->configLock != 0)
		{	// Make sure it hasn't been asynchronously unlocked			
			return(noErr);
		}
		USBExpertStatusLevel(2, device->ID, "USL - Device no longer busy: Address ==", device->usbAddress);
	}

	USBExpertStatusLevel(4, 0, "USL - Deleting device: Address ==", device->usbAddress);

	allConfDesc = device->allConfigDescriptors;
	
	deallocAllDeviceInterfaces(device->ID);
	
	/* Zero out the structure, not strictly necessary, but prudent */
	/* This bit relies on allConfigDescriptors being the second field after ID */
	ZeroMem(&device->allConfigDescriptors, sizeof(usbDevice)-OFFSET(usbDevice, allConfigDescriptors));
	
	initialiseOneDevice(device);

	if(allConfDesc != nil)
	{
		if(CurrentExecutionLevel() != kTaskLevel)
		{
			if(pb->usbCompletion == kUSBNoCallBack)
			{
				retVal = kUSBCompletionError;
			}
			else
			{
				pb->usbBuffer = allConfDesc;
				retVal = kUSBPending;
				uslDeallocMem(pb);
			}
		}
		else
		{
			PoolDeallocate(device->allConfigDescriptors);
		}
	}

	return(retVal);
}


/* Given a pipeRef, whats the pipe index? */
long recoverPipeIdx(USBPipeRef pipeRef)
{
	if(pipeRef == 0)
	{
		return(-1);
	}
	
	return(pipeRef & kUSBPipeIDMask);
}


/* GIven a pipeRef, find the pipe structure */
pipe *GetPipePtr(USBPipeRef pipeRef)
{
pipe *retPipe = nil;
long pipeIdx;
	do{
		pipeIdx = recoverPipeIdx(pipeRef);
		if(pipeIdx == -1)
		{
			break;
		}
		
		retPipe = &AllPipes[pipeIdx];
		
		if(pipeRef != retPipe->ref)
		{
			retPipe = nil;
			break;
		}
		
	}while(0);

	return(retPipe);
}

/* Mark all pipes unused */
static void initialisePipes(void)
{
long index, index2;

	/* Mark all pipes close */
	for(index = 0; index < kUSBMaxDevices; index++)
	{
		for(index2 = 0; index2 < kUSBMaxEndptPerDevice; index2++)
		{
			devices[index].pipes[index2] = -1;
		}
	}
	
	
}
#define  kUIMRefID 0x061563
/* This needs to take account of bus in some manner */
USBDeviceRef addNewDevice(UInt32 bus, UInt16 addr, UInt8 speed, UInt8 maxPacket, UInt32 power)
{
USBDeviceRef new;
usbDevice *dev;
long pipeIdx;
pipe *pipe0;

	new = 0;
	
	do{	/* for error checking */

	if( (addr == 0) || (addr > kUSBMaxDevice) )
	{
		break;
	}

	if(UIMControlEDCreate(bus, addr, 0, maxPacket, speed) != noErr)
	{
		break;
	}


	dev = &devices[addr];
	if(dev->ID != 0)
	{
		break;
	}
	
	do
	{
		new = MakeDevRef(bus, addr);
	}while (new == kUIMRefID);
	
	dev->bus = bus;
	dev->ID = new;
	dev->usbAddress = addr;
	dev->speed = speed;
	dev->powerAvailable = power;	
	USBExpertStatusLevel(4, dev->ID, "USL - New device power:", power);

	pipeIdx = AllocPipeIdx();
	if(pipeIdx == -1)
	{
		break;
	}	
	dev->pipe0 = pipeIdx;
	pipe0 = &AllPipes[pipeIdx];
	
/* this is repeated with open control endpoint. Should be consolodated */
	pipe0->bus = bus;
	pipe0->devIntfRef = new;
	pipe0->endPt = 0;
	pipe0->ref = MakePipeRef(pipeIdx);
	pipe0->devAddress = addr;
	pipe0->type = kUSBControl;
	pipe0->state = kUSBActive;
	for(pipeIdx = 0; pipeIdx < kUSBMaxEndptPerDevice; pipeIdx++)
	{
		dev->pipes[pipeIdx] = -1;
	}
	
	}while(0);
	
	return(new);
}



/* ************* Functions dealing with transactions ************* */

/* Each pipe has a number of possible transactions outstanding */
/* on it at one time. The pipe structure has 2 tables one of the */
/* UIM transaction ref for these and another parallel one which */
/* has the index of the packet its using */



/* ************* Actual Functions ************* */





USBPipeRef getPipeZero(USBDeviceRef device)
{
usbDevice *deviceP;
pipesIdx pipe0Idx;
USBPipeRef retVal = 0;
	
	/* Validate device */
	deviceP = getDevicePtr(device);
	
	if(deviceP == nil)
	{
		return(0);
	}
	pipe0Idx = deviceP->pipe0;
	if( (pipe0Idx < 0) || (pipe0Idx > kUSBMaxPipes) )
	{
		return(0);
	}
	return(AllPipes[pipe0Idx].ref);		
}


OSStatus USBServicesInitialise(void *exProc)
{	
kprintf("***USBServicesInitialize:calling severel inits***\n");
	if(initialised)
	{
		return(kUSBAlreadyOpenErr);
	}
	SetExpertFunction(exProc);
	
	initialiseDevices();
	initialisePipes();
	initialiseHubs();
	initialiseDelays();
	initialiseNotifications();

	initialised = true;
	
	return(noErr);
}

void USBServicesFinalise(void)
{
	if(!initialised)
	{
		return;//(paramErr);
	}
	finaliseHubs();
	finaliseDelays();
	finaliseNotifications();
	
	initialised = false;
}


Boolean checkPBVersionIsoc(USBPB *pb, UInt32 flags)
{
	if(	( (pb->pbVersion == 0x109) || (pb->pbVersion == 0x109) )&& 
		(pb->pbLength >= sizeof(USBPB)) &&
		( (pb->usbCompletion != nil) && (pb->usbCompletion != (void *)-1) ) &&
		((pb->usbFlags & ~flags) == 0) )
	{
		pb->usbStatus = noErr;
		return(true);
	}
	else
	{
		if((pb->pbVersion != 0x109) && (pb->pbVersion != 0x109) )
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - Bad param block version", pb->pbVersion);
			pb->usbStatus = kUSBPBVersionError;
		}
		else if(pb->pbLength < sizeof(USBPB)) 
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - small param block version", pb->pbLength);
			pb->usbStatus = kUSBPBLengthError;
		}
		else if( (pb->usbCompletion == nil) || (pb->usbCompletion == (void *)-1) )
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - bad completion function", (UInt32)pb->usbCompletion);
			pb->usbStatus = kUSBCompletionError;
		}
		else if((pb->usbFlags & ~flags) != 0)
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - bad flags", pb->usbFlags & ~flags);
			pb->usbStatus = kUSBFlagsError;
		}
		else
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - What??", 0);
			pb->usbStatus = paramErr;
		}
		return(false);
	}
}

Boolean checkPBVersion(USBPB *pb, UInt32 flags)
{
	if(	( (pb->pbVersion == 2) || (pb->pbVersion == 0x100) || (pb->pbVersion == 0x109))&& 
		(pb->pbLength >= sizeof(USBPB)) &&
		( (pb->usbCompletion != nil) && (pb->usbCompletion != (void *)-1) ) &&
		((pb->usbFlags & ~flags) == 0) )
	{
		pb->usbStatus = noErr;
		return(true);
	}
	else
	{
		if((pb->pbVersion != 2) && (pb->pbVersion != 0x100)  && (pb->pbVersion != 0x109) )
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - Bad param block version", pb->pbVersion);
			pb->usbStatus = kUSBPBVersionError;
		}
		else if(pb->pbLength < sizeof(USBPB)) 
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - small param block version", pb->pbLength);
			pb->usbStatus = kUSBPBLengthError;
		}
		else if( (pb->usbCompletion == nil) || (pb->usbCompletion == (void *)-1) )
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - bad completion function", (UInt32)pb->usbCompletion);
			pb->usbStatus = kUSBCompletionError;
		}
		else if((pb->usbFlags & ~flags) != 0)
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - bad flags", pb->usbFlags & ~flags);
			pb->usbStatus = kUSBFlagsError;
		}
		else
		{
			USBExpertStatusLevel(2, pb->usbReference, "USL error - What??", 0);
			pb->usbStatus = paramErr;
		}
		return(false);
	}
}



UInt8 USBMakeBMRequestType(USBDirection direction, USBRqType type, USBRqRecipient recipient)
{
UInt8 rq = 0xff;
	do{
		if(direction == kUSBNone)
		{
			direction = kUSBOut;
		}
		if(direction != kUSBOut && direction != kUSBIn)
		{
			break;
		}
		if(type < kUSBStandard || type > kUSBVendor)
		{
			break;
		}
		if(recipient < kUSBDevice || recipient > kUSBOther)
		{
			break;
		}
	
		rq = recipient + (type << kUSBRqTypeShift) + (direction << kUSBRqDirnShift);
	
	}while(0);
	return(rq);
}



OSStatus USBDisposeInterfaceRef(USBPB *pb)
{
//	 --> usbReference	 --> interface
OSStatus err;
uslInterface *intfrc;
Boolean nocallBack = false;
	
	if(pb->usbCompletion == kUSBNoCallBack)
	{
		nocallBack = true;
		pb->usbCompletion = (void *)-2;
	}
	
	if(!checkPBVersion(pb, 0))
	{
		return(pb->usbStatus);
	}
	err = findInterface(pb->usbReference, &intfrc);
	if(intfrc == nil)
	{
		USBExpertStatusLevel(4, pb->usbReference, "USL - Interface to dispose not found", 0);
		return(kUSBUnknownInterfaceErr);
	}
	intfrc->device = 0;
	intfrc->interfaceNum = 0;
	intfrc->alt = 0;
	intfrc->ref = 0;
	
	if(nocallBack)
	{
		pb->usbCompletion = kUSBNoCallBack;
	}
	else
	{
		(*pb->usbCompletion)(pb);
	}
	return(kUSBPending);
}

