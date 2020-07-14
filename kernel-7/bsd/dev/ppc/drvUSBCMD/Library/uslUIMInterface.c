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
	File:		uslUIMInterface.c

	Contains:	Interface between UIM and USL

	Version:	Neptune 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DRF)	Dave Falkenburg
		(bwm)	bruce merritt
		(TC)	Tom Clark
		(CSS)	Chas Spillar
		(GG)	Guillermo Gallegos
		(DF)	David Ferguson
		(DKF)	David Ferguson
		(CJK)	Craig Keithley
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB81>	11/13/98	BT		Delete hub device when removing bus.
	 <USB80>	11/11/98	DRF		(CJK) Make sure to check ALL UIMs when polling before returning.
									Allows us to type in Macsbug on the 3rd or 4th USB port.
	 <USB79>	10/22/98	BT		Fix wrong status for deleted devices.
	 <USB78>	10/21/98	DF		make probeEndpoint reentrant
	 <USB77>	 10/5/98	BT		Add root hub to probeendpoint
	 <USB76>	 9/29/98	bwm		BT: Fix UInt64 problems.
	 <USB75>	 9/29/98	BT		Use real frame timing
	 <USB74>	 9/10/98	DF		When removing a bus, clear all references to that bus.
	 <USB73>	  9/9/98	BT		Fix Isoc
	 <USB72>	  9/9/98	BT		Use correct number of frames calcing Isoc act count
	 <USB71>	  9/3/98	BT		Return immediate errors, fix param order
	 <USB70>	  9/3/98	GG		Changed Isoc interface to conform to dispatchtable interface.
	 <USB69>	  9/1/98	BT		FIx passing wrong parameter to UIM. Supermario compiler didn't
									like it.
	 <USB68>	 8/31/98	BT		Add isoc pipes
	 <USB67>	 8/25/98	BT		Isoc name changes
	 <USB66>	 8/24/98	BT		Cope without #define for reserved4
	 <USB65>	 8/13/98	BT		Add multibus support
	 <USB64>	 8/12/98	BT		Move root hub into UIM again.
	 <USB63>	 8/12/98	DF		(for BT) Finally fix fix
	 <USB62+>	 8/12/98	DF		(for BT) Finally fix fix
	 <USB62>	 8/11/98	BT		Issue bad UIM version status message
	 <USB61>	 8/11/98	BT		Fix check
	 <USB60>	 8/11/98	BT		Check UIM version
	 <USB59>	  8/7/98	GG		Tweaked interface to USL to be consistent with direction
									parameter.
	 <USB58>	 7/23/98	BT		Don't corrupt flags if control failes
	 <USB57>	 7/10/98	TC		Back out previous revision - it was breaking Yosemite.
	 <USB56>	 6/30/98	BT		Move Root hub sim into UIM
	 <USB55>	 6/24/98	DF		fix problem that kept the UIM's suspend change reset proc from
									being called.
	 <USB54>	 6/14/98	DF		Bad integration, left out the Check_UIM change.
	 <USB53>	 6/14/98	DF		Do a little more in USBRemoveBus, eliminate UIM moved checks
	 <USB52>	  6/5/98	GG		Changed uxbusref to USBBusRef.  Changed GetFrameNumberImmediate
									to return a UInt64 zero.
	 <USB51>	  6/5/98	CSS		Forgot to include Math64.h.
	 <USB50>	  6/5/98	CSS		Update to use U64Max() instead of -1 for the maximum 64 bit
									value.
	 <USB49>	  6/5/98	BT		Use UIM time
	 <USB48>	 5/20/98	BT		Eliminate PB0
	 <USB47>	  5/5/98	GG		Change Buffersize from short to unsigned long in BulkTransfer.
	 <USB46>	  5/4/98	BT		Fix pipe stall errors
	 <USB45>	 4/30/98	BT		Add real interrupt driven timer
	 <USB44>	 4/29/98	BT		Move common defines to USBpriv
	 <USB43>	 4/28/98	BT		Add bulk performance monitoring.
	 <USB42>	 4/26/98	BT		Add pipe state control
	 <USB41>	 4/23/98	BT		Add reset portsuspend change
	 <USB40>	 4/22/98	DF		Make USBAddBus return gracefully if more than one bus is
									attached.
	 <USB39>	 4/20/98	BT		Add abort pipe
	 <USB38>	 4/16/98	BT		Fix unused variable
	 <USB37>	 4/15/98	BT		Add over current change reset
	 <USB36>	 4/14/98	DF		Add UIM polling for debugger support
	 <USB35>	 4/14/98	BT		Use EDDelete
	 <USB34>	  4/9/98	BT		Use USB.h
		<33>	  4/7/98	BT		Fix act count in control transactions
		<32>	  4/6/98	BT		Change w names
		<31>	  4/6/98	BT		New param block names
		<30>	  4/2/98	BT		eliminate obsolete function
		<29>	 3/24/98	BT		Swap parameter order in open Int ED
		<28>	 3/18/98	BT		Add reset enable change to root hub.
		<27>	 3/18/98	BT		Add clear port enable change feature (Disabled waiting UIM
									support)
		<26>	 3/11/98	BT		Int simulation for root hub. More debugs for moving UIM.
		<25>	  3/5/98	BT		track down changing UIM
		<24>	  3/5/98	BT		Add interrupt pipe
		<23>	 2/23/98	BT		Fix error recovery
		<22>	 2/19/98	BT		Cut out call to process done Queue in UIM.
		<21>	 2/16/98	BT		Fix calling processdone queue and timer too often.
	 <USB20>	 2/11/98	DKF		Add back in ProcessDoneQueue if we're at task time
		<19>	  2/8/98	BT		Take out process done q in send control.
		<18>	  2/4/98	BT		Fix uslControlPacket to have errors. Add mnore support for
									usbProbe
		<17>	  2/2/98	BT		Add bulk stuff
		<16>	 1/29/98	BT		Use symbols for the flags fields used
		<15>	 1/26/98	BT		Mangle names after design review, finish up
		<14>	 1/26/98	BT		Mangle names after design review
		<13>	 1/26/98	BT		Hack in clear enpoint stall
		<12>	 1/21/98	BT		Change hardware status codes
		<11>	 1/20/98	BT		Unwrapping root hub
		<10>	 1/19/98	BT		More root hub sim
		 <9>	 1/15/98	BT		Do root hub simulation.
		 <8>	 1/15/98	CJK		Change include of USL.h to USBServicesLib.h
		 <7>	 1/14/98	BT		Implimenting Interrupt transactions
		 <6>	 1/13/98	BT		Change uslPacket to uslControlPacket
		 <5>	12/22/97	BT		StartRootHub moving to USL
		 <4>	12/19/97	BT		TAking out dependancy on UIM
		 <3>	12/19/97	BT		UIM now a Shared lib
*/

#include "../USB.h"
#include "../USBpriv.h"
//#include <Math64.h"

#include "../uimpriv.h"
#include "uslpriv.h"
//#include "../MonaUSBPriv.h"



enum{
	kSetupSent  = 0x01,
	kDataSent	= 0x02,
	kStatusSent = 0x04,
	kSetupBack  = 0x10,
	kDataBack	= 0x20,
	kStatusBack = 0x40
	};

enum{
	kMaxBus = 16
	};
static struct UIMPluginDispatchTable *UIMPlugIn[kMaxBus], *pending[kMaxBus];
static USBBusRef UIMBus[kMaxBus];
static USBDeviceRef rootHubRefs[kMaxBus];


void UIMSetRootHubRef(UInt32 bus, USBDeviceRef ref)
{
	if(UIMPlugIn[bus] == nil)
	{
		USBExpertStatus(0,"USL - Bad bus passed to root hub ref", 0);
	}
	else
	{
		rootHubRefs[bus] = ref;
	}
}

#define CHECK_UIM 0
#if CHECK_UIM
static void checkUIMMoved(UInt32 bus, void ***UIM,void  **UIM1,void  **current)
{

static void *UIMp;

	if(UIMPlugIn[bus] != nil)
	{
		if(UIMp == nil)
		{
			UIMp = (void *)UIMPlugIn[bus];
		}
		else
		{
			if(UIMp != (void *)UIMPlugIn[bus])
			{
				//DebugStr("UIM Pointer changed, tell barry, you're hosed");
				USBExpertStatus(0,"USL - UIM Pointer changed, tell barry, you're hosed", 0);
			}
		}

		if(*UIM == nil)
		{
			*UIM = (void *)*current;
			*UIM1 = **UIM;
		}
		else
		{
			if(*UIM != (void *)*current)
			{
				//DebugStr("UIM moved, tell barry, you're hosed");
				USBExpertStatus(0,"USL - UIM moved, tell barry, you're hosed", 0);
			}
			
			if(*UIM1 != **UIM)
			{
				//DebugStr("UIM 1 moved, tell barry, you're hosed");
				USBExpertStatus(0,"USL - UIM 1 moved, tell barry, you're hosed", 0);
			}
		}
	}
	else
	{
		SysDebugStr("Bad bus");
		USBExpertStatus(0,"USL - bad bus passed to UIM interface", bus);
	}
}
#endif

OSStatus USBAddBus(	
		void *regEntry,
		void *UIM,	/* GG to specifiy */
		USBBusRef inBus)					/* from TC */
{

UInt32 bus=
#if CHECK_UIM
			5
#else
			0
#endif
			 ;

	regEntry = 0;	/* not used */

printf("USBAddBus:Adding bus=%d\n",bus);
	while( (UIMPlugIn[bus] != nil) || (pending[bus] != nil) )
	{
		if(bus++ >= kMaxBus)
		{
			return(-1);
		}
	}
	USBExpertStatus(-1, "USL - Adding bus index:", bus);
	pending[bus] = UIM;
	if(pending[bus]->pluginVersion != kUIMPluginTableVersion)
	{
		USBExpertStatus(-1, "UIM is wrong versions, bus not added", pending[bus]->pluginVersion);
		pending[bus] = nil;
		return(kUSBPBVersionError);
	}


	UIMBus[bus] = inBus;

printf("USBAddBus:starting roothub\n");
	StartRootHub(bus);
printf("USBAddBus:roothub complete\n");
	return(noErr);
}

OSStatus UIMResetRootHub(UInt32 bus)
{
	if(pending[bus] == nil)
	{
		SysDebugStr("Bad bus for reset");
		USBExpertStatus(0,"USL - bad bus passed to reset", bus);	
	}
	
	if(pending[bus] != nil)
	{
	OSStatus ret;
		ret = (*pending[bus]->uimResetRootHubProc)(UIMBus[bus]);
		if(ret != noErr)
		{
			return(ret);
		}
		UIMPlugIn[bus] = pending[bus];
		pending[bus] = nil;
	}
	return(-1);
}

OSStatus USBRemoveBus(
		USBBusRef inBus)					/* from TC */
{
UInt32 bus = 0;
USBPB pb;

	while(inBus != UIMBus[bus])
	{
		if(bus++ >= kMaxBus)
		{
			return(kUSBNotFound);
		}
	}
	USBExpertStatus(-1, "USL - Removing bus index:", bus);

	pb.pbLength = sizeof(pb);
	pb.pbVersion = kUSBCurrentHubPB;
	pb.usbStatus = noErr;
	pb.usbFlags = 0;
	pb.usbReference = rootHubRefs[bus];
	pb.usbCompletion = kUSBNoCallBack;

	if(immediateError(USBHubDeviceRemoved(&pb)))
	{
		USBExpertStatus(pb.usbReference, "USL - remove bus, Failed to remove root hub", pb.usbReference);
	}
	USBExpertRemoveDeviceDriver(rootHubRefs[bus]);
	rootHubRefs[bus] = 0;

	UIMPlugIn[bus] = nil;
	UIMBus[bus] = 0;
	pending[bus] = nil;
	
	return(noErr);
}

void UIMPollRootHubSim(void)
{
#if CHECK_UIM
static void **UIM, *UIM1;
#endif
UInt32 bus = 0;

	for(bus = 0; bus<kMaxBus; bus++)
	{
		if(UIMPlugIn[bus] != nil)
		{
#if CHECK_UIM
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimPollRootHubSim);
#endif

			(*UIMPlugIn[bus]->uimPollRootHubSim)(UIMBus[bus]);
		}
	}
}

OSStatus USLPolledProcessDoneQueue(void)
{
#if CHECK_UIM
static void **UIM, *UIM1;
#endif
UInt32 bus = 0;
OSStatus myErr;

	for(bus = 0; bus<kMaxBus; bus++)
	{
		if(UIMPlugIn[bus] != nil)
		{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimProcessDoneQueueProc);
#endif
			myErr = (*UIMPlugIn[bus]->uimProcessDoneQueueProc)();
		}
	}
	return(noErr);
}


	

UInt64 UIMGetCurrentFrame(UInt32 bus)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimGetCurrentFrameNumberProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimGetCurrentFrameNumberProc)();
	}
	return(U64SetU(0));
}

UInt32 UIMGetAFrame(void)
{
union{
	UInt64 u;
	UnsignedWide s;
	}frame;
static UInt32 bus = 0;

	if(UIMPlugIn[bus] != nil)
	{
		frame.u = (*UIMPlugIn[bus]->uimGetCurrentFrameNumberProc)();
		return(frame.s.lo);
	}

	for(bus = 0; bus<kMaxBus; bus++)
	{
		if(UIMPlugIn[bus] != nil)
		{
			frame.u = (*UIMPlugIn[bus]->uimGetCurrentFrameNumberProc)();
			return(frame.s.lo);
		}
	}
	return(0);
}


OSStatus UIMControlEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateControlEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateControlEndpointProc)(
				functionNumber, 
				endpointNumber,
				maxPacketSize, 
				speed);
	}
	return(-1);
}

OSStatus UIMInterruptEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateInterruptEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateInterruptEndpointProc)(
				functionNumber, 
				endpointNumber,
				speed,
				maxPacketSize, 
				8,	// Polling rate
				0);	// Reserve bandwidth?? 
	}
	return(-1);
}

OSStatus UIMIsocEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt8						direction,	
	UInt16						maxPacketSize)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateIsochEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateIsochEndpointProc)(
				functionNumber, 
				endpointNumber,
				maxPacketSize, 
				direction); 
	}
	return(-1);
}


OSStatus UIMBulkEDCreate(
	UInt32 bus,
	UInt8 						functionNumber,
	UInt8						endpointNumber,
	UInt8						direction,	
	UInt8						maxPacketSize)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateBulkEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateBulkEndpointProc)(
				functionNumber, 
				endpointNumber,
				direction,
				maxPacketSize);
	}
	return(-1);
}

OSStatus UIMControlTransfer(
	UInt32 bus,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	short						bufferSize,
	short						direction)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateControlTransferProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateControlTransferProc)(
		refcon,
		handler,
		CBP,
		bufferRounding,
		functionNumber,
		endpointNumber,
		bufferSize,
		direction);
	}
	return(-1);
}

#if 0
#include "TextUtils.h"
static void AddNum(UInt8 *s1, UInt32 num)
{
UInt8 number[20];
	NumToString(num, number);
	AddString(s1, number);
}

static void AddString(UInt8 *s1, UInt8 *s2)
{
int count;
	count = 255-*s1;
	if(count == 0)
	{	
		return;
	}
	if(count > *s2)
	{
		count = *s2;
	}
	usb_BlockMoveData(&s2[1], &s1[*s1]+1, s2[0]);
	*s1+=*s2;
}

static void DebugNum(UInt8 *str, UInt32 num, Boolean stop)
{
UInt8 strBuf[256];

	AddString(strBuf, str);
	AddString(strBuf, " : ");
	AddNum(strBuf, num);
	AddString(strBuf, " 100;g");
	DebugStr(strBuf);
	if(stop)
	{
		SysDebugStr("This seems bad, tell barry about it, cmd-g to continue");	
	}
}
#endif
OSStatus UIMControlEDDelete(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimDeleteControlEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimDeleteControlEndpointProc)(functionNumber, endpointNumber);
	}
	else
	{
		USBExpertStatus(0,"USL -control ED delete failed", bus);		
	}
	return(-1);
}

OSStatus UIMEDDelete(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimDeleteEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimDeleteEndpointProc)(functionNumber, endpointNumber, direction);
	}
	return(-1);
}

OSStatus UIMBulkTransfer(
	UInt32 bus,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	UInt32						bufferSize,
	short						direction)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateBulkTransferProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateBulkTransferProc)(
		refcon,
		handler,
		CBP,
		bufferRounding,
		functionNumber,
		endpointNumber,
		bufferSize,
				direction);
	}
	return(-1);
}

OSStatus UIMIntTransfer(
	UInt32 bus,
	short						functionNumber,
	short						endpointNumber,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						bufferSize)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateInterruptTransferProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimCreateInterruptTransferProc)(
		functionNumber,
		endpointNumber,
		refcon,
		handler,
		CBP,
		bufferRounding,
		bufferSize);
	}
	return(-1);
}

OSStatus UIMIsocTransfer(
	UInt32 bus,
	short						functionNumber,
	short						endpointNumber,
	short						direction,
	UInt32 				 		frameStart, 
	UInt32 						refcon,
	IsocCallBackFuncPtr			handler,
	void 						*buffer,
	UInt32 						numFrames,
	USBIsocFrame 				*frames)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimCreateIsochTransferProc);
#endif
union{
	UInt64 u;
	UnsignedWide s;
	}startFrame;

	if(UIMPlugIn[bus] != nil)
	{
		startFrame.u = UIMGetCurrentFrame(bus);
		startFrame.s.lo = frameStart; /* this needs to be more sophisticated */
	
		return(*UIMPlugIn[bus]->uimCreateIsochTransferProc)(
		functionNumber,
		endpointNumber,
		refcon,
		direction,
		handler,
		startFrame.u,
		(UInt32)buffer,
		numFrames,
		frames);
	}
	return(-1);
}
/*
typedef CALLBACK_API_C( OSStatus , UIMCreateIsochTransferProcPtr )(
short functionAddress, 
short endpointNumber, 
UInt8 direction, 
short refcon, 
UInt32 pIsochHandler, 
UInt64 frameStart, 
UInt32 pBufferStart, 
UInt32 frameCount, 
USBIsocFrame *pFrames, 
UInt32 pStatus);
*/
OSStatus UIMClearEndPointStall(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimClearEndPointStallProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimClearEndPointStallProc)(functionNumber, endpointNumber, direction);
	}
	return(-1);
}

OSStatus UIMAbortEndpoint(
	UInt32 bus,
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
#if CHECK_UIM
static void **UIM, *UIM1;
	checkUIMMoved(bus, &UIM, &UIM1, (void *)&UIMPlugIn[bus]->uimAbortEndpointProc);
#endif

	if(UIMPlugIn[bus] != nil)
	{
		return(*UIMPlugIn[bus]->uimAbortEndpointProc)(
				functionNumber, 
				endpointNumber,
				direction);
	}
	return(-1);
}


static OSStatus doUSLStatus(OSStatus status, USBReference ref)
{
	if(status != noErr)
	{
		if(status == EDDeleteErr)
		{
			status = kUSBAbortedError;
		}
		else if(status == bandWidthFullErr)
		{
			status = kUSBNoBandwidthError;
		}
		else if(status == returnedErr)
		{
			status = kUSBAbortedError;
		}
		else
		{
			if( (status > 0) && (status < 16))
			{
				/* As ferg points out all host contoller errors stall the pipe */
				uslSetPipeStall(ref);			
			}
			status = kUSBLinkErr+status;
#if 0
			if( (status == kUSBCRCErr) ||
			 	(status == kUSBBitstufErr) ||
			 	(status == kUSBDataToggleErr) ||
			 	(status == kUSBEndpointStallErr) ||
			 	(status == kUSBNotRespondingErr) ||
			 	(status == kUSBPIDCheckErr) ||
			 	(status == kUSBWrongPIDErr) )
			{
				uslSetPipeStall(ref);
			}
#endif
		}
	}
	return(status);
}


static void intPktHandler(long refCon, OSStatus status, short shortfall)
{
USBPB *pb;

	if(refCon == 0)
	{
		return;
	}

	pb = (void *)refCon;

	status = doUSLStatus(status, pb->usbReference);

	pb->usbStatus = status;
	pb->usbActCount = pb->usbReqCount - shortfall;
	if (pb->usbCompletion && (pb->usbCompletion != kUSBNoCallBack))
		(*pb->usbCompletion)(pb);
}

static void bulkPktHandler(long refCon, OSStatus status, short shortfall)
{
USBPB *pb;

	if(refCon == 0)
	{
		return;
	}

	pb = (void *)refCon;

	status = doUSLStatus(status, pb->usbReference);

	pb->usbStatus = status;
	if(shortfall > pb->usbReqCount)
	{
		pb->usbActCount = 0;
	}
	else
	{
		pb->usbActCount = pb->usbReqCount - shortfall;
	}

	if(pb->usbFlags)
	{
		resolvePerformance(pb);
	}

	if (pb->usbCompletion && (pb->usbCompletion != kUSBNoCallBack))
		(*pb->usbCompletion)(pb);
}

static void isocPktHandler(long refCon, OSStatus status, USBIsocFrame *pFrames)
{
USBPB *pb;
OSStatus aggregate = noErr;
UInt32 actCount = 0, frame;

	if(refCon == 0)
	{
		return;
	}

	pb = (void *)refCon;

	actCount = 0;
	for(frame = 0; frame < pb->usb.isoc.NumFrames; frame++)
	{
		actCount += pFrames[frame].frActCount;
		if(pFrames[frame].frStatus != noErr)
		{
			aggregate = pFrames[frame].frStatus;
		}
	}
	pb->usbActCount = actCount;
	
	if(status == noErr)
	{
		pb->usbStatus = aggregate;
	}
	else
	{
		pb->usbStatus = status;
	}
	if(pb->usbStatus != noErr)
	{
		pb->usbStatus = kUSBLinkErr+pb->usbStatus;
	}

	if ( (pb->usbCompletion != nil) && (pb->usbCompletion != kUSBNoCallBack))
	{
		(*pb->usbCompletion)(pb);
	}
}

static void ctlPktHandler(long refCon, OSStatus status, short shortfall)
{
UInt8 sent, back, todo;
USBPB *pb;
Boolean callBackTIme;
UInt32 bus;

	if(refCon == 0)
	{
		return;
	}


	pb = (void *)(refCon & ~1);
	callBackTIme = ((refCon & 1) != 0);

	sent = (pb->reserved1 &0xf) << 4;
	back = pb->reserved1 &0xf0;
	todo = sent ^ back;	/* thats xor */
	
	if((todo & kSetupBack) != 0)
	{
		pb->reserved1 |= kSetupBack;

	}
	else if((todo & kDataBack) != 0)
	{	/* This is the data transport phase, so this is the interesting one */
		pb->reserved1 |= kDataBack;
		pb->usbActCount = pb->usbReqCount - shortfall;	
	}
	else if((todo & kStatusBack) != 0)
	{
		pb->reserved1 |= kStatusBack;
		
	}
	else
	{
		USBExpertStatus(-1, "Spare transactions, This seems to be harmless", 0);
	}

	back = pb->reserved1 &0xf0;
	todo = sent ^ back;	/* thats xor */

	if( (status != noErr) && (status != returnedErr) )
	{
	UInt8 addr, endpt;
		addr = ( (pb->usbFlags) &kUSLAddrMask) >> kUSLAddrShift;
		endpt = ( (pb->usbFlags &kUSLEndpMask) >> kUSLEndpShift);
		bus = ( (pb->usbFlags &kUSLBusMask) >> kUSLBusShift);

		pb->usbStatus = doUSLStatus(status, pb->usbReference);
		if(endpt == 0)
		{
			UIMClearEndPointStall(bus, addr, endpt, 0);
		}
		
	}


	if( callBackTIme || (todo == 0) )
	{
		if( !callBackTIme || (todo != 0) )	/* One but not the other */
		{
			USBExpertStatus(-1, "UIM Interface odd completion", 0);
		}
		pb->usbFlags &= ~(kUSLAddrMask|kUSLEndpMask|kUSLBusMask);

		if(status == noErr)
		{
			if(pb->usbStatus != kUSBPending)
			{
				USBExpertStatus(-1, "UIM Interface odd status", 0);
			}
			pb->usbStatus = noErr;
		}
		
		pb->usb.cntl.WIndex = USBToHostWord(pb->usb.cntl.WIndex);
		pb->usb.cntl.WValue = USBToHostWord(pb->usb.cntl.WValue);
		if (pb->usbCompletion && (pb->usbCompletion != kUSBNoCallBack))
			(*pb->usbCompletion)(pb); 
	}
}

OSStatus usbIsocPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction)
{
OSStatus err;

	pb->usbStatus = noErr;
	
	err = UIMIsocTransfer(bus,
						address /* address */,
						endpoint /* endpoint */,
						direction /* in or out */,
						pb->usbFrame /* starting frame */,
						(UInt32)pb /* refcon */, 
						isocPktHandler,
						pb->usbBuffer /* buffer */, 
						pb->usb.isoc.NumFrames,
						pb->usb.isoc.FrameList
						);

#if 0
	if(err != noErr)
	{
		isocPktHandler((UInt32)pb, err, pb->usb.isoc.FrameList);
	}
#else
	return(err);
#endif

}

OSStatus usbIntPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction)
{
OSStatus err;
	direction = 0;	/* Not used */

	pb->usbStatus = noErr;
kprintf("usbIntPacket:Calling UIMIntTransfer:addr=%d,ep=%d,reqcount=%d\n",address,endpoint,pb->usbReqCount);
	err = UIMIntTransfer(bus,
						address,
						endpoint,
						(UInt32)pb /* refcon */, 
						intPktHandler,
						(UInt32)pb->usbBuffer /* buffer */, 
						true /* short packets OK */,
						pb->usbReqCount /* buffer size */);
kprintf("usbIntPacket:UIMTransfer returned err=%d\n",err);

	return(err);
}


OSStatus usbBulkPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint, UInt8 direction)
{
OSStatus err;

	pb->usbStatus = noErr;
	
	err = UIMBulkTransfer(bus,
						(UInt32)pb /* refcon */, 
						bulkPktHandler,
						(UInt32)pb->usbBuffer /* buffer */, 
						true /* short packets OK */,
						address /* address */,
						endpoint /* endpoint */,
						pb->usbReqCount /* buffer size */,
						direction /* in or out */);

	return(err);
}

#define kUSBSetup kUSBNone

OSStatus usbControlPacket(UInt32 bus, USBPB *pb, UInt8 address, UInt8 endpoint)
{
OSStatus err;
short dirn;
	

	if( ((UInt32)pb &1) != 0)
	{
		USBExpertStatus(-1, "Missaligned PB", 0);
	}

	/* Remeber address and endpoint here for later */
	pb->usbFlags |= (address << kUSLAddrShift)+(endpoint << kUSLEndpShift)+(bus << kUSLBusShift);

	do{
	pb->reserved1 = 0;	/* stage pointer */
	pb->usbStatus = kUSBPending;
	pb->usbActCount = 0;
	
	pb->reserved1 |= kSetupSent;
//kprintf("usbControlPacket:addr=%d,ep=%d\n",address,endpoint);
	err = UIMControlTransfer(bus,
							(UInt32)pb/* refcon */,
							ctlPktHandler,
							(UInt32)&pb->usb.cntl.BMRequestType /* packet */, 
							true /* short packets OK */,
							address /* address */,
							endpoint /* endpoint */,
							8 /* packet size */,
							kUSBSetup /* setup */);
//kprintf("usbControlPacket:After first call refcon(pb)=0x%x,err=%d\n",pb,err);
	if(err != noErr)
	{
		pb->usbFlags  = 0;
		USBExpertStatus(-1, "Control packet 1 error", err);
		break;
	}
	
 	dirn = ((pb->usb.cntl.BMRequestType & 0x80) == 0)?kUSBOut:kUSBIn;
	if(pb->usb.cntl.reserved4 != 0)
	{
		pb->reserved1 |= kDataSent;
		err = UIMControlTransfer(bus,
							(UInt32)pb /* refcon */, 
							ctlPktHandler,
							(UInt32)pb->usbBuffer /* buffer */, 
							true /* short packets OK */,
							address /* address */,
							endpoint /* endpoint */,
							pb->usbReqCount /* buffer size */,
							dirn /* in or out */);
		if(err != noErr)
		{
			USBExpertStatus(-1, "Control packet 2 error", err);
			break;
		}
//kprintf("usbControlPacket:After second(data phase)call refcon(pb)=0x%x,err=%d\n",pb,err);

	}

 	dirn = kUSBOut+kUSBIn - dirn;

	pb->reserved1 |= kStatusSent;
	err = UIMControlTransfer(bus,
						(UInt32)pb | 1 /* refcon, with marker */, 
						ctlPktHandler,
						0 /* buffer, for no data transfer */, 
						true /* short packets OK */,
						address /* address */,
						endpoint /* endpoint */,
						0 /* buffer size */,
						dirn /* in or out */);

//kprintf("usbControlPacket:After third(status phase)call refcon(pb)=0x%x,err=%d\n",(UInt32)pb | 1,err);
	if(err != noErr)
	{
		USBExpertStatus(-1, "Control packet 3 error", err);
	}

	}while(0);

	return(err);
}

OSStatus uslProbeEndpoint(USBPB *pb, UInt8 address, UInt8 endpoint);

OSStatus uslProbeEndpoint(USBPB *pb, UInt8 address, UInt8 endpoint)
{
OSStatus err;
UInt32	 bus;

	if	(address > 127)
	{	
		if(rootHubRefs[255-address] == 0)
		{
			return(kUSBUnknownDeviceErr);
		}
		address = rootHubRefs[255-address] & 0x7f;
	}
	
	if ((bus = uslDeviceAddrToBus(address)) >= kMaxBus)
	{
		return(kUSBUnknownDeviceErr);
	}
	
	pb->usbStatus = kUSBPending;
	err = usbControlPacket(bus, pb, address, endpoint);
	
	return(err);
}





