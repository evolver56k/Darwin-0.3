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
	File:		USBPriv.i

	Contains:	Private defines & SPI for USB services

	Version:	

	Written by:	Barry Twycross

	Copyright:	(c) 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:               Craig Keithley

		Other Contact:     

		Technology:        I/O

	Writers:

		(DRF)	Dave Falkenburg
		(TC)	Tom Clark
		(GG)	Guillermo Gallegos
		(BT)	Barry Twycross
		(CJK)	Craig Keithley
		(DF)	David Ferguson	


	Change History (most recent first):

		<28>	  9/9/98	BT		Fix size of refcon in Isoc transfers
		<27>	  9/4/98	TC		Add definition for 1.0 version of DriverNotifyProc.
		<26>	  9/3/98	GG		Another Interface tweak.
		<25>	  9/3/98	GG		Fixed Isochronous interface.
		<24>	 8/31/98	GG		Changed Isochronous interest.  Bumped version Number to 3. Added
									Isoc Callback.
		<23>	 8/12/98	BT		Move root hub into UIM again.
		<22>	 8/11/98	BT		Fix semi-colon
		<21>	 8/11/98	BT		Add UIM dispatch table version
		<20>	 7/11/98	TC		Back out <USB18>.
		<19>	 6/30/98	BT		Add hub defs, the hub driver and root hub needs
		<18>	 6/30/98	BT		Move RH sim to UIM
		<17>	 6/15/98	DRF		Added definitions for USB ExpandMem contents.
		<16>	 6/10/98	CJK		add uslGetDeviceRefByID prototype
		<15>	  6/5/98	TC		Move declaration of uxBusRef to USB.i, then change it to
									USBBusRef.
		<14>	  6/4/98	DRF		In UIMInitialize & UIMFinalize, add a Ptr to facilitate
									handing-off global state when performing replacement.
		<13>	  6/3/98	GG		Fixed mistake with comments.
		<12>	  6/3/98	GG		Add interfaces for UIMFrameCount, IsochEndpointCreate, and
									IsochTransfer
		<11>	  6/2/98	DRF		Added params to UIMInitialize & UIMFinalize for replacement.
									Also made a few interfacer tweaks for typedef-ed structs.
		<10>	 5/20/98	DF		add USLPolledProcessDoneQueue prototype
		 <9>	 5/20/98	TC		Include definition USBRegisterExpertNotification.
		 <8>	  5/5/98	CJK		Correct someone's misspelling of my last name.
		 <7>	  5/5/98	GG		Change Buffersize from short to unsigned long in
									uimBulkTransferProc.
		 <6>	 4/29/98	BT		Move common defines to here
		 <5>	 4/23/98	BT		Add reset portsuspend change
		 <4>	 4/15/98	BT		Add over current change reset
		 <3>	 4/11/98	CJK		add back in OHCIPollDoneQueue dispatch table entry.
		 <2>	  4/9/98	DF		Update to for changes made to Neptune 4/8
		 <1>	  4/8/98	DF		first checked in



*/

#pragma once

#include "mactypes.h"
#include "nameregistry.h"
#include "USB.h"

#pragma push

//typedef void (*USBCtlCallbackFn)(struct USBPB *pb);
//typedef void (*USBDevCtlCallbackFn)(struct USBPB *pb);
#define extern 

extern void USBIdleTask(void);

extern OSStatus USBAddBus(void *regEntry,void *UIM,USBBusRef bus);
		
extern OSStatus USBRemoveBus(USBBusRef bus);		

extern OSStatus USBServicesInitialise(void * exProc);

extern void USBServicesFinalise(void);

extern OSStatus USBRegisterExpertNotification (ExpertNotificationProcPtr	pExpertNotify);

extern OSStatus USBServicesGetVersion(UInt32 ID, VersRec *vers, UInt32 versSize);

extern OSStatus USLPolledProcessDoneQueue(void);

extern USBDeviceRef uslGetDeviceRefByID(unsigned short deviceID, USBBusRef bus);


/*
	For casting purposes when we are calling a driver compiled with 1.0 version
	of the driver dispatch table.
*/	
typedef extern OSStatus	(*ONE_OH_USBDDriverNotifyProcPtr) (USBDriverNotification notification, void *pointer);

/* IMPORTANT NOTES!
   This file contains some OHCI specific functions & data structures, which are
   currently required because the USL performs some root hub functions.  When the
   root hub simulation in the UIM is fully functional, the OHCI or UHCI specific
   elements should be removed from this file (and placed in the OHCIUIM.H file)
*/

// Taken out of OHCIUIM.h and uslUIMInterface.c 
enum{
	EDDeleteErr =  -14,
	bandWidthFullErr = -15,
	returnedErr = -16
	};


typedef UInt8 uslBusRef;
// Shouldn't this be in USL.H?

struct uslRootHubDescriptor
{

	short numPorts;
	Boolean powerSwitching;			/* does it have it? */
	Boolean gangedSwitching;		/* if yes, it is ganged */

	Boolean compoundDevice;
	Boolean overCurrent;			/* does it have it? */
	Boolean globalOverCurrent; 		/* if yes is it global */
	UInt8	reserved;				/* for alignment */

	UInt8 	portFlags[32];			/* Device removable and ganged power flags */

}; 

typedef struct uslRootHubDescriptor *uslRootHubDescriptorPtr;

struct uslRootHubPortStatus
{
	UInt16 						portFlags;	/* Port status flags */
	UInt16 						portChangeFlags;	/* Port changed flags */
};

typedef struct uslRootHubPortStatus *uslRootHubPortStatusPtr;


typedef extern OSStatus (*OHCIGetRootDescriptorPtr)(
	uslBusRef 					bus,
	uslRootHubDescriptorPtr 	descr);

typedef extern OSStatus (*OHCIGetInterruptStatusPtr)(
	uslBusRef 					bus,
	UInt32 						*status);

typedef extern OSStatus (*OHCIClearInterruptStatusPtr)(
	uslBusRef 					bus,
	UInt32 						status);

typedef extern OSStatus (*OHCIGetRhStatusPtr)(
	uslBusRef 					bus,
	UInt32 						*status);


typedef extern OSStatus (*OHCIRootHubResetChangeConnectionPtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubResetResetChangePtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubResetSuspendChangePtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubResetEnableChangePtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubResetOverCurrentChangePtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubGetPortStatusPtr)(
	uslBusRef 					bus,
	short 						port,
	uslRootHubPortStatusPtr 	portStatus);

typedef extern void (*OHCIPollRootHubSimPtr)(
	uslBusRef					bus);

typedef extern OSStatus (*OHCIResetRootHubPtr)(
	uslBusRef					bus);

typedef extern OSStatus (*OHCIRootHubPortSuspendPtr)(
	uslBusRef 					bus,
	short						port,
	Boolean						on);

typedef extern OSStatus (*OHCIRootHubPowerPtr)(
	uslBusRef 					bus,
	Boolean						on);

typedef extern OSStatus(* OHCIRootHubResetPortPtr)(
	uslBusRef 					bus,
	short						port);

typedef extern OSStatus (*OHCIRootHubPortEnablePtr)(
	uslBusRef 					bus,
	short						port,
	Boolean						on);

typedef extern OSStatus (*OHCIRootHubPortPowerPtr)(
	uslBusRef 					bus,
	short						port,
	Boolean						on);


//other Functions
typedef extern OSStatus (*OHCIProcessDoneQueuePtr)(void);

typedef extern UInt64 (*UIMGetCurrentFrameNumberPtr)(void);


typedef  extern void (*CallBackFuncPtr) (long lParam, OSStatus status, short s16Param);
typedef extern void (*IsocCallBackFuncPtr) (long lParam, OSStatus status, USBIsocFrame *pFrames);
//typedef CallBackFunc *CallBackFuncPtr;

typedef void *UIMAbortControlEndpointProcPtr;
typedef void *UIMEnableControlEndpointProcPtr;
typedef void *UIMDisableControlEndpointProcPtr;
typedef void *UIMAbortBulkEndpointProcPtr;
typedef void *UIMEnableBulkEndpointProcPtr;
typedef void *UIMDisableBulkEndpointProcPtr;
typedef void *UIMDeleteInterruptEndPointProcPtr;
typedef void *UIMAbortInterruptEndpointProcPtr;
typedef void *UIMEnableInterruptEndpointProcPtr;
typedef void *UIMDisableInterruptEndpointProcPtr;
typedef void *UIMDeleteIsochEndPointProcPtr;
typedef void *UIMAbortIsochEndpointProcPtr;
typedef void *UIMEnableIsochEndpointProcPtr;
typedef void *UIMDisableIsochEndpointProcPtr;

typedef extern OSStatus (*UIMInitializeProcPtr)(RegEntryIDPtr UIMregEntryID, Boolean replacingOld, Ptr savedState);

typedef extern OSStatus (*UIMFinalizeProcPtr)(Boolean beingReplaced,Ptr * savedStatePtr);

typedef extern OSStatus (*UIMCreateControlEndpointProcPtr)(
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed);

typedef extern OSStatus (*UIMCreateBulkEndpointProcPtr)(
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt8						direction,	
	UInt8						maxPacketSize);

typedef extern OSStatus (*UIMCreateControlTransferProcPtr)(
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	short						bufferSize,
	short						direction);

typedef extern OSStatus (*UIMDeleteControlEndPointProcPtr)(
	short 						functionNumber,
	short						endpointNumber);

typedef extern OSStatus (*UIMDeleteBulkEndPointProcPtr)(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

typedef extern OSStatus (*UIMCreateBulkTransferProcPtr)(
	UInt32						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	UInt32						bufferSize,
	short						direction);

typedef extern OSStatus (*UIMClearEndPointStallProcPtr)(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

typedef extern OSStatus (*UIMCreateInterruptEndpointProcPtr)(
	short						functionAddress,
	short						endpointNumber,
	short						speed,
	UInt16 						maxPacketSize,
	short						pollingRate,
	UInt32						reserveBandwidth);

typedef extern OSStatus (*UIMCreateInterruptTransferProcPtr)(
	short						functionNumber,
	short						endpointNumber,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						bufferSize);

typedef extern OSStatus (*UIMAbortEndpointProcPtr)(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

typedef extern OSStatus (*UIMDeleteEndpointProcPtr)(
	short 						functionNumber,
	short						endpointNumber,
	short						direction);

typedef extern OSStatus (*UIMCreateIsochEndpointProcPtr)(
	short						functionAddress,
	short						endpointNumber,
	UInt32						maxPacketSize,						
	UInt8						direction);
	
typedef extern OSStatus (*UIMCreateIsochTransferProcPtr)(
	short						functionAddress,						
	short						endpointNumber,
	UInt32						refcon,
	UInt8						direction,
	IsocCallBackFuncPtr			pIsochHandler,							
	UInt64						frameStart,								
	UInt32						pBufferStart,							
	UInt32						frameCount,									
	USBIsocFrame				*pFrames);								

enum{
	kUIMPluginTableVersion = 3	// BT 11Aug98, bump up with root hub changes
	};

struct UIMPluginDispatchTable
{
	UInt32								pluginVersion;
	UIMInitializeProcPtr				uimInitializeProc;
	UIMFinalizeProcPtr					uimFinalizeProc;
	UIMCreateControlEndpointProcPtr		uimCreateControlEndpointProc;
	UIMDeleteControlEndPointProcPtr		uimDeleteControlEndpointProc;
	UIMCreateControlTransferProcPtr		uimCreateControlTransferProc;
	UIMAbortControlEndpointProcPtr		uimAbortControlEndpointProc;
	UIMEnableControlEndpointProcPtr		uimEnableControlEndpointProc;
	UIMDisableControlEndpointProcPtr	uimDisableControlEndpointProc;
	UIMCreateBulkEndpointProcPtr		uimCreateBulkEndpointProc;
	UIMDeleteBulkEndPointProcPtr		uimDeleteBulkEndpointProc;
	UIMCreateBulkTransferProcPtr		uimCreateBulkTransferProc;
	UIMAbortBulkEndpointProcPtr			uimAbortBulkEndpointProc;
	UIMEnableBulkEndpointProcPtr		uimEnableBulkEndpointProc;
	UIMDisableBulkEndpointProcPtr		uimDisableBulkEndpointProc;
	UIMCreateInterruptEndpointProcPtr	uimCreateInterruptEndpointProc;
	UIMDeleteInterruptEndPointProcPtr	uimDeleteInterruptEndpointProc;
	UIMCreateInterruptTransferProcPtr	uimCreateInterruptTransferProc;
	UIMAbortInterruptEndpointProcPtr	uimAbortInterruptEndpointProc;
	UIMEnableInterruptEndpointProcPtr	uimEnableInterruptEndpointProc;
	UIMDisableInterruptEndpointProcPtr	uimDisableInterruptEndpointProc;
	UIMCreateIsochEndpointProcPtr		uimCreateIsochEndpointProc;
	UIMDeleteIsochEndPointProcPtr		uimDeleteIsochEndpointProc;
	UIMCreateIsochTransferProcPtr		uimCreateIsochTransferProc;
	UIMAbortIsochEndpointProcPtr		uimAbortIsochEndpointProc;
	UIMEnableIsochEndpointProcPtr		uimEnableIsochEndpointProc;
	UIMDisableIsochEndpointProcPtr		uimDisableIsochEndpointProc;
	UIMAbortEndpointProcPtr				uimAbortEndpointProc;
	UIMDeleteEndpointProcPtr			uimDeleteEndpointProc;
/* Need this for error recovery */
	UIMClearEndPointStallProcPtr		uimClearEndPointStallProc;

	OHCIPollRootHubSimPtr				uimPollRootHubSim;
	OHCIResetRootHubPtr					uimResetRootHubProc;
	OHCIProcessDoneQueuePtr				uimProcessDoneQueueProc;
	UIMGetCurrentFrameNumberPtr			uimGetCurrentFrameNumberProc; 

};

typedef struct UIMPluginDispatchTable *UIMPluginDispatchTablePtr;

// extern UIMPluginDispatchTable ThePluginDispatchTable;

// Hub stuff, for root hub



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

union setupPacket{
	UInt8 byte[8];
	UInt16 word[4];
	};

typedef union setupPacket setupPacket;

enum{
	kUSBRqDirnShift = 7,
	kUSBRqDirnMask = 1,
	
	kUSBRqTypeShift = 5,
	kUSBRqTypeMask = 3,
	
	kUSBRqRecipientMask = 0X1F
	};



/***************************************************************************

	Proposal for Product ID in Apple USB devices.
	
	Barry Twycross	25Jan98
	
	
Background.

All USB devices have a device descriptor which tells the system
basic information about the device. Amoung the information contained
therein is a Vendor and a Product code. The Vendor code is assigned
by the USB implimenters forum, for Apple this is 1452 (decimal)
0x5AC (hex).

The product code is left for the implimentor to use as they see fit.
The product code along with the vendor ID is the highest priority way
of matching a device to a driver. A driver can claim to be the best
driver for a particular product by specifying mathcing product/vendor
codes.

There appears to be no one responsible for assigning these codes, so
the USB group is making this suggestion in an attempt to impose order
before chaos reigns.


Implimentation.

The Product code in a USB device is a 16 bit field. It is transmitted
across the USB in Intel byte ordering (swapped from the usual Mac way 
of doing things). This spec uses the logical value of the field. That 
it has to be byte swapped to be decoded sensibly after transmission 
across the bus.

These proposals only apply to devices which have the Apple Vendor ID
code.

Proposal.

It is proposed that the 16bit Product ID value (expressed in hex) is 
decomposed as follows:

  0xRTNN
  
There is a 4 bit 'R' (range) field
There is a 4 bit 'T' (type) field
There is an 8 bit 'NN' field, which finally specifies a product within a type.

Range (R)

This field specifies the party responsible for naming this series of 
devices.

In general it also specifies a device as internal to a system, or an 
external, standalone, peripheral.

  0-7  Is reserved for external devices.
  8-F  is reserved for internal devices.
  
Each of the ranges is further farmed out to a responsible entity to
define the sub fields. Currently assigned are:

  0:  Input devices (Harrold Welch).
  8:  USB team (Barry Twycross)
  

Type (T)

This field specifies the type of device within a range of devices.
The values are specified by the party responsible for the range.

Currently assigned are:

   R=0, T specifies a device with this equivalent ADB ID.
     T = 2:  Keyboard
	 T = 3:  Mouse

   R=8
     T = 0:  Root hub in a CPU.
	 T = 1:  Non-root hub in a CPU


Product (NN)

This field finally specifies which particular device or a type the
product actually is. It is expected the first product will be numbered
1, and further products are numbered sequentially.

**********************************************************************/


//Known assignments:

//These device IDs are currently known to be assigned:

enum{
	  kAppleVendorID = 0x05AC,		// Assigned by USB-if

//                  0xRTNN      	Product
// These are Harold Welch's ADB device equivalents
	  kPrdKBCosmoAnsi = 0x0201,		// Ansi Cosmo keyboard
	  kPrdKBCosmoIso  = 0x0202,		// Iso Cosmo keyboard
	  kPrdKBCosmoJis  = 0x0203,		// Jis Cosmo keyboard
	  kPrdMseRudi     = 0x0301,		// Rudi mouse
// These are the USB root hub simulations
	  kPrdRootHubCMD  = 0x8001,		// CMD chip root hub
	  kPrdRootHubOPTI = 0x8002,		// Opti chip root hub
	  kPrdRootHubLucent = 0x8003	// Lucent chip root hub.
  };


/***************************************************************************

USBExpandMemGlobals: a structure that will eventually be more important,
but for now, we use this to keep track of the connection ID of the USB
FamilyExpertLib so that we can cleanly shut it down from a file-based
version of USB Expert.

****************************************************************************/

struct	USBExpandMemGlobals
{
	UInt32				version;
	CFragConnectionID	expertConnectionID;
};
typedef struct USBExpandMemGlobals *USBExpandMemGlobalsPtr;

enum
	{
	kUSBExpandMemVersionOne	=	0x0100,
	kUSBExpandMemCurVersion	=	kUSBExpandMemVersionOne
	};
	
#pragma pop
#undef extern
