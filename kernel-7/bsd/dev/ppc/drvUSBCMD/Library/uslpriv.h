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
	File:		uslpriv.h

	Contains:	USB Services Library private include.

	Version:	Neptune 1.0
	
	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DF)	David Ferguson
		(TC)	Tom Clark
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB52>	10/29/98	BT		Close old configurations on reset or reconfiguration.
	 <USB51>	10/22/98	BT		Fix wrong status for deleted devices.
	 <USB50>	10/21/98	DF		Add uslDeviceAddrToBus function
	 <USB49>	 9/29/98	BT		Use real frame timing
	 <USB48>	 9/28/98	BT		Add device reset function
	 <USB47>	 9/17/98	BT		1.1 rules for captive devices.
	 <USB46>	 8/31/98	BT		Add isoc pipes
	 <USB45>	 8/24/98	BT		Isoc param block definition
	 <USB44>	 8/13/98	BT		Add multibus support
	 <USB43>	  7/9/98	BT		Clean up after delete device and config clash. Merge in branch
	 <USB42>	 6/30/98	BT		Some defs moved to usb.i
	 <USB41>	 6/22/98	BT		Add Alt interface to configure interface
	 <USB40>	 6/14/98	DF		Add RemoveRootHub function prototype
	 <USB39>	  6/5/98	BT		Add pb param to deletedvice
	 <USB38>	  6/5/98	TC		Change uxBusRef to USBBusRef.
	 <USB37>	 5/21/98	BT		Add uslOpenDevice
	 <USB36>	 5/20/98	BT		Lots of stuff for V2 PB s
	 <USB35>	 5/17/98	BT		Add config stuff
	 <USB34>	 5/12/98	BT		New interface stuff
	 <USB33>	  5/8/98	BT		Add open interface for Tom
	 <USB32>	 4/30/98	BT		Add real interrupt driven timer
	 <USB31>	 4/28/98	BT		Add bulk performance monitoring.
	 <USB30>	 4/26/98	BT		Make find pipe global
	 <USB29>	 4/24/98	BT		Add pipe functions
	 <USB28>	 4/23/98	BT		Add hub watchdog function
	 <USB27>	 4/21/98	BT		Allow requests to device zero, pipe zero.
	 <USB26>	 4/16/98	BT		Remove duplicated defn
	 <USB25>	 4/14/98	BT		Note restrictions on device structure. uslDeleteDevice makes
									assumptions
	 <USB24>	 4/14/98	BT		Impliment device remove
	 <USB23>	 4/11/98	CJK		Now that USB.i has been re-built, the problem of the qtypes is
									gone...
	 <USB22>	 4/11/98	CJK		investigate qtype problem
	 <USB21>	 4/10/98	BT		Re add q type defines
	 <USB20>	  4/9/98	CJK		trim down to only those defines not yet present in USBPriv.h
	 <USB19>	  4/9/98	BT		Move defines from public
		<18>	  4/8/98	BT		Move error codes to public file
		<17>	  4/8/98	BT		Add error codes (should be in USL.h)
		<16>	  4/2/98	BT		Start on config/interface/endpoint stuff. First caching config
									descriptors.
		<15>	 3/19/98	BT		Add USLprivate.h
		<14>	 3/19/98	BT		Split USBServices.h into public and private
		<13>	  3/5/98	BT		Add int transactions
		<12>	  2/4/98	BT		Fix uslControlPacket to have errors. Add more support for
									usbProbe
		<11>	  2/2/98	BT		Add bulk stuff
		<10>	 1/29/98	BT		Carve up the flags
		 <9>	 1/26/98	BT		Mangle names after design review
		 <8>	 1/26/98	BT		Make expert notify public
		 <7>	 1/19/98	BT		More root hub sim
		 <6>	 1/15/98	BT		Add extra defines for bMRequestType
		 <5>	 1/13/98	BT		Change uslPacket to uslControlPacket, get ready for Int
									transactions.
		 <4>	12/22/97	BT		StartRootHub moving to USL
		 <3>	12/18/97	BT		Emulate expert notification proc.
*/

/* Need to autoinclude all files here */
/* Must conditionalise this */

#ifndef __USLPRIVH__
#define __USLPRIVH__
#include "../driverservices.h"
/* ************* Constants ************* */

#if 0
enum{
	kUSBBus = 0
	};
#endif

enum{
	kUSBDeviceIDShift = 7,
	kUSBMaxDevices = 128,
	kUSBMaxDevice = kUSBMaxDevices-1,
	kUSBDeviceIDMask = 0x7f,

	kUSBMaxPipeIDShift =7,
	kUSBMaxPipes = (1<<kUSBMaxPipeIDShift),
	kUSBPipeIDMask = (kUSBMaxPipes-1),
	
	kUSBMaxEndptPerDevice = 30,
	
	kUSBEndPtShift = 7,
	kUSBDeviceMask = ((1 << kUSBEndPtShift) -1),
	
	kUSBNoPipeIdx = -1,
	
	kUSBHubQType = 21,
	kUSBDelayQType = 22
	
	};

enum{
	kUSBIdleStalled = kUSBStalled+1
	};



/* Flag long word of PB. Used internally 

Should probably make this a seperate reserved field 
	b0    Task Time, wow a real flag
	b1
	b2
	b3
	b4
	b5
	b6
	b7
	b8
	b9
	b10
	b11    der/alloc - 1 bit is this an alloc or dealloc of task time memory
	b12    bus    - bus 4 bit internal bus number
	b13    bus
	b14    bus
	b15    bus
	b16    stage - 4 bit internal stage counter
	b17    stage
	b18    stage
	b19    stage
	b20    endp - 5 bits of endpoint (if we need direction).
	b21    endp
	b22    endp
	b23    endp
	b24    endp
	b25    addr - 7 bits of function addr
	b26    addr
	b27    addr
	b28    addr
	b29    addr
	b30    addr
	b31    addr

*/
enum{
	kUSLTTMemDeAllocFlagShift = 11,
	kUSLBusShift = 12,
	kUSLBusMask = 0xf000,
	kUSLStageShift = 16,
	kUSLStageMask = 0xf0000,
	kUSLEndpShift = 20,
	kUSLEndpMask =  0x1f00000,
	kUSLAddrShift = 25,
	kUSLAddrMask = 0xfe000000
	};


/* ************* types ************* */


#define addrStash reserved6
#define wIndexStash reserved7
#define wValueStash reserved8

typedef short pipesIdx;
typedef short InterfacesIdx;
typedef short packetIdx;
typedef short transIndex;

typedef struct{
	USBPipeRef ref;
	USBReference devIntfRef;	/* device or interface ref of container */
	
	UInt32 bus;
	
	long maxPacket;
	USBFlags flags;	
	USBPipeState state;
	
	UInt8 devAddress;
	UInt8 endPt;
	UInt8 direction;
	UInt8 type;

	/* For perforance monitoring */
	UInt32 lastRead;
	UInt32 bytesTransfered;
	UInt32 transferTime;
	Boolean monitorPerformance;
	
	}pipe;

typedef struct{
	USBInterfaceRef ref;
	USBDeviceRef device;
	UInt32 interfaceNum;
	UInt32 alt;
	}uslInterface;

/* This one is the beginnings of a config descriptor, so we can extract total length */
typedef struct{
	UInt8 len;
	UInt8 type;
	UInt16 totalLen;
	} configHeader;

typedef struct{
	/* uslDeleteDevice assumes that this is the first entry in this struct */
	USBDeviceRef ID;

	/* it assumes this is the second. Beware if you change the order here */
	void *allConfigDescriptors;
	UInt32 allConfigLen;
	UInt32 powerAvailable;
	UInt32 configLock;
	UInt32 killMe;
	
	UInt32 bus;
	
	pipesIdx pipe0;
	pipesIdx pipes[kUSBMaxEndptPerDevice];

	configHeader configScratch;
	USBDeviceDescriptor deviceDescriptor;

	UInt8 currentConfiguration;
	UInt8 usbAddress;
	Boolean confValid;
	Boolean speed;
	}usbDevice;
	

#define OFFSET(ty, fl)  ((UInt32)  &(((ty *)0)->fl))

void uslInterruptPriority(UInt32);

Boolean uslIsInterfaceRef(USBInterfaceRef interf);
OSStatus findInterface(USBInterfaceRef InterfaceRef, uslInterface **p);
OSStatus uslOpenPipeImmed(USBInterfaceRef intrfc, USBEndPointDescriptor *endp);
usbDevice *getDevicePtrFromIdx(unsigned deviceIndex);
int makeDevPipeIdx( UInt8 endpoint, UInt8 direction);
pipe *getPipeByIdx(pipesIdx idx);
OSStatus uslOpenDevice(USBPB *pb);

//UInt32 uslDeviceToBus(USBDeviceRef ref);
Boolean uslHubValidateDevZero(USBDeviceRef ref, UInt32 *bus);
void uslHubAddDevice(USBPB *pb);
void UIMSetRootHubRef(UInt32 bus, USBDeviceRef ref);


Boolean checkPBVersion(USBPB *pb, UInt32 flags);
Boolean checkPBVersionIsoc(USBPB *pb, UInt32 flags);

OSStatus USBExpertNotifyParentMsg(USBReference reference, void *pointer);
USBDeviceRef MakeDevRef(UInt32 bus, UInt16 addr);
USBDeviceRef addNewDevice(UInt32 bus, UInt16 addr, UInt8 speed, UInt8 maxPacket, UInt32 power);
UInt32 uslDeviceAddrToBus(UInt16 addr);
UInt8 getNewAddress(void);
usbDevice *getDevicePtr(USBDeviceRef device);
OSStatus findPipe(USBPipeRef pipeRef, pipe **p);
USBPipeRef getPipeZero(USBDeviceRef device);
long recoverPipeIdx(USBPipeRef pipeRef);
pipe *getPipe(usbDevice *device, UInt8 endpoint, UInt8 direction);
pipe *AllocPipe(usbDevice *device, UInt8 endpoint, UInt8 direction);
unsigned makeDeviceIdx(USBDeviceRef device);
void deallocPipe(usbDevice *device, UInt8 endpoint, UInt8 direction);
pipe *GetPipePtr(USBPipeRef pipeRef);
void uslSetPipeStall(USBPipeRef ref);
uslInterface *AllocInterface(USBDeviceRef device);

OSStatus validateRef(USBReference ref, UInt32 *bus);
void uslCleanHubQueue(USBDeviceRef ref);
void uslCleanDelayQueue(USBDeviceRef ref);
void uslCleanNotifyQueue(USBDeviceRef ref);
void uslCleanMemQueue(USBDeviceRef ref);
void uslCleanAQueue(QHdrPtr queue, USBDeviceRef ref);
Boolean isSameDevice(USBReference ref, USBDeviceRef devRef);





OSStatus uslClosePipe(pipesIdx pipe);
void uslCloseNonDefaultPipes(usbDevice *deviceP);
void uslUnconfigureDevice(usbDevice *device);
void uslCloseInterfacePipes(USBInterfaceRef iRef, usbDevice *deviceP);
OSStatus uslDeleteDevice(USBPB *pb, usbDevice *device);
UInt32 deltaFrames(UInt32 previous, UInt32 bus);
UInt32 UIMGetAFrame(void);
void uslHubWatchDog(Duration currentMilliSec);

OSStatus usbControlPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint);
OSStatus usbIntPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction);
OSStatus usbIsocPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction);
OSStatus usbBulkPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction);
OSStatus xUSLControlRequest(UInt32 bus, USBPB *pb, pipe *thisPipe);

Boolean immediateError(OSStatus err);
OSStatus StartRootHub(UInt32 bus);

void initialiseNotifications(void);
void initialiseDelays(void);
void initialiseTimer(void);
void initialiseHubs(void);

void finaliseNotifications(void);
void finaliseDelays(void);
void finaliseHubs(void);

void SetExpertFunction(void *exProc);

void resolvePerformance(USBPB *pb);
OSStatus uslMonitorBulkPerformanceByReference(USBPipeRef ref, UInt32 what, 
					UInt32 *lastRead, UInt32 *bytesTransfered, UInt32 *transferTime);


/* these are internal versions of USB... with less error checking */
OSStatus uslDeviceRequest(struct USBPB *pb);

OSStatus uslAllocMem(USBPB *pb);
OSStatus uslDeallocMem(USBPB *pb);


#endif /*__USLPRIVH__ */
