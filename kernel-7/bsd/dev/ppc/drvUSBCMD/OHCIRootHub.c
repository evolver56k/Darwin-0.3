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
	File:		OHCIRootHub.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(BT)	Barry Twycross

	Change History (most recent first):

	  <USB8>	 8/12/98	BT		Move root hub into UIM again
	  <USB7>	 7/10/98	TC		Back out previous rev.
	  <USB6>	 6/30/98	BT		Move Root hub sim into UIM
	  <USB5>	 6/11/98	BT		Fix hub descriptor so it knows about overcurrent and power mode
	  <USB4>	 4/23/98	BT		Add reset portsuspend change
	  <USB3>	 4/16/98	BT		Channge to new headers
	  <USB2>	 4/15/98	BT		Add over current change reset
		 <1>	 3/19/98	BT		first checked in
*/
#include "USB.h"
#include "driverservices.h"

#include "OHCIUIM.h"
#include "OHCIRootHub.h"
#include "pci.h"

OHCIUIMDataPtr				pOHCIUIMData;

void IOSync(void);
UInt32 EndianSwap32Bit(UInt32 x);
static void parameterNotUsed(UInt32 notUsed)
{
	notUsed = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// OHCIUIMGetRootHubDescriptor
//
//   This proc returns the root hub descriptor fields.
//   

OSStatus OHCIGetRootDescriptor(
	uslBusRef 					bus,	/* which hub */
	uslRootHubDescriptorPtr 	descr)
{
	OHCIRegistersPtr			pOHCIRegisters;
	UInt32						hcRhDescriptorA;
	UInt32						hcRhDescriptorB;
	UInt32						powerSwitching,
								gangedSwitching,
								compoundDevice,
								overCurrent,
								globalOverCurrent,
								deviceRemovable,
								gangedPower;
	OSStatus					status = noErr;

	parameterNotUsed(bus);
	
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	// Read hcRhDescriptorA registers.
	hcRhDescriptorA = EndianSwap32Bit (pOHCIRegisters->hcRhDescriptorA);
	hcRhDescriptorB = EndianSwap32Bit (pOHCIRegisters->hcRhDescriptorB);


	// Copy data from hcRhDescriptorA masking out reserved bit fields.
	// The hcRhDescriptorA register is the 32 bit byte swap of nbrPorts,
	// hubCharacteristics, and pwrOn2PwrGood.
	// could have problem with porting and overwriting of other bytes.
	hcRhDescriptorA &= kOHCINumPortsMask;
	descr->numPorts = hcRhDescriptorA;


	//Now get NPS Field, Power Switching
	hcRhDescriptorA = EndianSwap32Bit (pOHCIRegisters->hcRhDescriptorA);
	powerSwitching = (hcRhDescriptorA & kOHCIPowerSwitchingMask) >> kOHCIPowerSwitchingOffset;
	descr->powerSwitching =  !((Boolean) powerSwitching);

	//Now Get Ganged Switching
	gangedSwitching = (hcRhDescriptorA & kOHCIGangedSwitchingMask) >> kOHCIGangedSwitchingOffset;
	descr->gangedSwitching =  !(Boolean) gangedSwitching;

/*
	UInt32* myPointer;

	myPointer = (UInt32 *) &(descr->gangedSwitching);

	*myPointer =  (Boolean) gangedSwitching;
*/



	//Is it a Compound device?
	compoundDevice = (hcRhDescriptorA & kOHCICompoundDeviceMask) >> kOHCICompoundDeviceOffset;
	descr->compoundDevice =  (Boolean) compoundDevice;
	
	//Does it allow overcurrent
	overCurrent = (hcRhDescriptorA & kOHCIOverCurrentMask) >> kOHCIOverCurrentOffset;
	descr->overCurrent =  !(Boolean) overCurrent;
	
	//Does it allow Global Over Current?
	globalOverCurrent = (hcRhDescriptorA & kOHCIGlobalOverCurrentMask) >> kOHCIGlobalOverCurrentOffset;
	descr->globalOverCurrent =  !(Boolean) globalOverCurrent;
	
	//Find the device removable and ganged power flags and put into stuffed byte form
	deviceRemovable = (hcRhDescriptorB & kOHCIDeviceRemovableMask) >> kOHCIDeviceRemovableOffset;
	descr->portFlags[0] =  (UInt8) deviceRemovable;
	gangedPower = (hcRhDescriptorB & kOHCIGangedPowerMask) >> kOHCIGangedPowerOffset;
	descr->portFlags[1] =  (UInt8) gangedPower;


	return (status);
}

OSStatus OHCIGetInterruptStatus(
	uslBusRef 					bus,
	UInt32 						*status)
{
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	*status = EndianSwap32Bit (pOHCIRegisters->hcInterruptStatus);
	return(noErr);
}


OSStatus OHCIClearInterruptStatus(
	uslBusRef 					bus,
	UInt32 						status)
{
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit (status);
	return(noErr);
	
}

OSStatus OHCIGetRhStatus(
	uslBusRef 					bus,
	UInt32 						*status)
{
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	*status = EndianSwap32Bit (pOHCIRegisters->hcRhStatus);
	return(noErr);
	
}


OSStatus OHCIRootHubGetPortStatus(
	uslBusRef 					bus,
	short 						port,
	uslRootHubPortStatusPtr 	portStatus)
{
	UInt32						portFlags;
	UInt32						hcRhPortStatus;
	OSStatus					status = noErr;
	
	parameterNotUsed(bus);

	//adjust port number for array use
	port--;

	hcRhPortStatus = EndianSwap32Bit (pOHCIUIMData->pOHCIRegisters->hcRhPortStatus[port]);
//zzz
//	sprintf (debugStr, "OHCIUIMportstatus = %lx", hcRhPortStatus);
//	DebugStr ((ConstStr255Param) c2pstr (debugStr));
//zzz
	portFlags = hcRhPortStatus & kOHCIPortFlagsMask;
	portStatus->portFlags = (UInt16) portFlags;
	portFlags = hcRhPortStatus >> kOHCIportChangeFlagsOffset;
	portStatus->portChangeFlags = (UInt16) portFlags;
	return (status);

}

OSStatus OHCIResetRootHub (
		uslBusRef		bus)
{
	UInt32 				rootHubDescriptorA, savedControl;
	OSStatus			status = noErr;
	OHCIRegistersPtr	pOHCIRegisters;
	UInt16				numDownstreamPorts;
	UInt16				powerOnToGoodTime;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	rootHubDescriptorA = EndianSwap32Bit(pOHCIRegisters->hcRhDescriptorA);
	
	numDownstreamPorts = rootHubDescriptorA & kOHCIHcRhDescriptorA_NDP;
	powerOnToGoodTime = 2 * ((rootHubDescriptorA & kOHCIHcRhDescriptorA_POTPGT) >> kOHCIHcRhDescriptorA_POTPGTPhase);
	// should we wait for this time here?  How about if the ports aren't powered on already?
kprintf("POTPGT val=%d\n",powerOnToGoodTime);	
	DelayForHardware(DurationToAbsolute(powerOnToGoodTime*durationMillisecond));
	// put USB into USB bus-reset state
	savedControl = EndianSwap32Bit(pOHCIRegisters->hcControl) & ~(kOHCIHcControl_HCFS);
	pOHCIRegisters->hcControl = EndianSwap32Bit (pOHCIRegisters->hcControl) & ~(kOHCIHcControl_HCFS);

	DelayForHardware(DurationToAbsolute(10*durationMillisecond));
	
	// then go to the operational state
	pOHCIRegisters->hcControl = EndianSwapImm32Bit((kOHCIFunctionalState_Operational << kOHCIHcControl_HCFSPhase) | savedControl) ;

	DelayForHardware(DurationToAbsolute(10*durationMillisecond));  // spec requires devices be ready within 10mS


	ResetRootHubSimulation();
	
	return (status);
}


OSStatus OHCIRootHubPower(
		uslBusRef 		bus,
		Boolean			on)
{
	UInt32 					value;
	OSStatus				status = noErr;
	OHCIRegistersPtr		pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	value = 0;
	if(on)
	{
		value |= kOHCIBit16;/* power on to all ganged ports */
	}
	else
	{
		value |= kOHCIBit0;	/* turn global power off */
	}
//	value |= kOHCIBit15;/* remote wakeup enable */
//	value |= kOHCIBit17;/* clear over current change indicator */
//	value |= kOHCIBit31;/* remote wakeup disabled */

	pOHCIRegisters->hcRhStatus		= EndianSwap32Bit (value);

	return (status);

}

OSStatus OHCIRootHubResetChangeConnection(
	uslBusRef 		bus,
	short			port)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	value = 0;
	value |= kOHCIBit16;/* clear status change */

	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}


OSStatus OHCIRootHubResetResetChange(
	uslBusRef 		bus,
	short			port)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	value = 0;
	value |= kOHCIBit20;/* clear reset status change */

	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}

OSStatus OHCIRootHubResetSuspendChange(
	uslBusRef 		bus,
	short			port)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	value = 0;
	value |= kOHCIBit18;/* clear suspend status change */

	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}

OSStatus OHCIRootHubResetEnableChange(
	uslBusRef 		bus,
	short			port)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	value = 0;
	value |= kOHCIBit17;/* clear enable status change */

	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}


OSStatus OHCIRootHubResetOverCurrentChange(
	uslBusRef 		bus,
	short			port)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	value = 0;
	value |= kOHCIBit19;/* clear over current status change */

	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}


OSStatus OHCIRootHubResetPort (
	uslBusRef 		bus,
	short			port)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OSStatus					status = noErr;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	//sets Bit 8 in port root hub register
	pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (kOHCIBit4);
        IOSync();
	return (status);

}

OSStatus OHCIRootHubPortEnable(
		uslBusRef 		bus,
		short			port,
		Boolean			on)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

parameterNotUsed(bus);

	value = 0;
	if(on)
	{
		value |= kOHCIBit1;/* enable port */
	}
	else
	{
		value |= kOHCIBit0;	/* disable port */
	}

	pOHCIUIMData->pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);
       IOSync();

	return (status);
}

OSStatus OHCIRootHubPortSuspend(
		uslBusRef 		bus,
		short			port,
		Boolean			on)
{
	UInt32 value;
	OSStatus					status = noErr;
	OHCIRegistersPtr			pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	value = 0;
	if(on)
	{
		value |= kOHCIBit2;/* suspend port */
	}
	else
	{
		value |= kOHCIBit3;	/* resume port */
	}

	pOHCIUIMData->pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}

OSStatus OHCIRootHubPortPower(
	uslBusRef 						bus,
	short							port,
	Boolean							on)
	{
	UInt32 							value= 0;
	OSStatus						status = noErr;
	OHCIRegistersPtr				pOHCIRegisters;

parameterNotUsed(bus);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	if(on)
	{
		value |= kOHCIBit8;/* enable port power */
	}
	else
	{
		value |= kOHCIBit9;	/* disable port power */
	}

	pOHCIUIMData->pOHCIRegisters->hcRhPortStatus[port-1] = EndianSwap32Bit (value);

	return (status);
}


