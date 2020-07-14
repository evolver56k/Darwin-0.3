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
	File:		ConfigParse.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Craig Keithley

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BG)	Bill Galcher
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB12>	 6/29/98	CJK		change total length so that it's properly byte swapped
	 <USB11>	 6/16/98	CJK		change FindHIDInterfaceByProtocol to FindHIDInterfaceByNumber
	 <USB10>	  6/8/98	CJK		remove unneeded XXGetEndPointDescriptor function
	  <USB9>	 5/19/98	BG		Fix some casting problems.
	  <USB8>	 4/23/98	CJK		added FindHIDInterfaceByProtocol, so that the reworked mouse
									code can find the interface within the configuration descriptor.
	  <USB7>	  4/9/98	CJK		remove include of USBClassDrivers.h, USBHIDModules.h, and
									USBDeviceDefines.h
		 <6>	 3/17/98	CJK		Replace some "};" with just "}" (metrowerks has a problem with
									it).
		 <5>	  3/9/98	CJK		Fix RADAR #2216609 (Duplicate sets of enums)
		 <4>	  3/2/98	CJK		Fix change history (had some old entries from before being
									cloned).
		 <3>	  3/2/98	CJK		Correct semi-colon problem.
		 <2>	  3/2/98	CJK		Add include of USBHIDModules.h. Remove include of HIDEmulation.h
*/

//#include <Types.h>
//#include <Devices.h>
//#include <processes.h>
#include "../driverservices.h"
#include "../USB.h"


#include "MouseModule.h"

OSErr FindHIDInterfaceByNumber(LogicalAddress pConfigDesc, UInt32 ReqInterface, USBInterfaceDescriptorPtr * hInterfaceDesc)
{
UInt32 totalLength;
void * pEndOfDescriptors;
USBInterfaceDescriptorPtr 	pMyIntDesc;
USBDescriptorHeaderPtr		pCurrentDesc;
unsigned long				anAddress, anOffset;

	totalLength = USBToHostWord(((USBConfigurationDescriptorPtr)pConfigDesc)->totalLength);
	pEndOfDescriptors = (Ptr)pConfigDesc + totalLength;					// get the total length and add it to the start of the config space
	pCurrentDesc = (USBDescriptorHeaderPtr)pConfigDesc;					// point the currentdesc to the start of the config space
	
	while (pCurrentDesc < pEndOfDescriptors)							// as long as we haven't exhausted all the descriptors
	{
		if (pCurrentDesc->descriptorType == kUSBInterfaceDesc)			// look at the current descriptor
		{
			pMyIntDesc = (USBInterfaceDescriptorPtr)pCurrentDesc;		// if it's an interface descriptor
			if ((pMyIntDesc->interfaceClass == kUSBHIDInterfaceClass) &&
			    (pMyIntDesc->interfaceNumber == ReqInterface))			// and if it's the interface we want...
			{
				*hInterfaceDesc = pMyIntDesc;							// if it is, then return with hInterfaceDesc set to the
				return noErr;											// current descriptor pointer
			}
		}
		// (Ptr)pCurrentDesc += pCurrentDesc->length;					// Nope, that either wasn't an interface descriptor
		anAddress = (unsigned long) pCurrentDesc;						// Nope, that either wasn't an interface descriptor
		anOffset  = (unsigned long) pCurrentDesc->length;
		anAddress += anOffset;
		pCurrentDesc = (USBDescriptorHeaderPtr) anAddress;
		if (pCurrentDesc->length == 0)
			break;
	}																	// or it was, but not the droid we're looking for.
	*hInterfaceDesc = NULL;
	return kUSBInternalErr;
}




