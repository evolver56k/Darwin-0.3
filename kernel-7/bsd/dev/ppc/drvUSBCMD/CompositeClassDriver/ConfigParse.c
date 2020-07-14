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

	  <USB5>	 6/29/98	CJK		change total length so that it's properly byte swapped.
	  <USB4>	  6/5/98	CJK		remove XXGetEndPointDescriptor function (not needed)
	  <USB3>	 5/19/98	BG		Fix some casting problems.
	  <USB2>	  4/9/98	CJK		change to use USB.h
		 <1>	  4/7/98	CJK		first checked in as USBCompositeClassDriver
		 <8>	 3/17/98	CJK		Replace "};" with just "}" (where required by MW).  Also used
									extractprototypes to get the function prototypes out of this
									file and into the header file.
		 <7>	  3/9/98	CJK		Fix RADAR #2216609 (Duplicate sets of enums)
		 <6>	  2/9/98	CJK		remove get hid descriptor function
		 <5>	  2/9/98	CJK		remove HIDEmulation.h include
		 <4>	  2/2/98	CJK		Add get hid descriptor call
		 <3>	 1/26/98	CJK		Change to use structure field names as required by codebert
		 <2>	 1/23/98	CJK		Work on configuration parsing
*/

//#include <Types.h>
//#include <Devices.h>
//#include <processes.h>
#include "../driverservices.h"
#include "../USB.h"

#include "CompositeClassDriver.h"

OSErr GetInterfaceDescriptor(LogicalAddress pConfigDesc, UInt32 ReqInterface, USBInterfaceDescriptorPtr * hInterfaceDesc)
{
UInt32 totalLength;
void * pEndOfDescriptors;
USBInterfaceDescriptorPtr 	pMyIntDesc;
USBDescriptorHeaderPtr		pCurrentDesc;
unsigned long				anAddress, anOffset;

	totalLength = USBToHostWord(((USBConfigurationDescriptorPtr)pConfigDesc)->totalLength);
	pEndOfDescriptors = (Ptr)pConfigDesc + totalLength;				// get the total length and add it to the start of the config space
	pCurrentDesc = (USBDescriptorHeaderPtr)pConfigDesc;				// point the currentdesc to the start of the config space
	
	while (pCurrentDesc < pEndOfDescriptors)						// as long as we haven't exhausted all the descriptors
	{
		if (pCurrentDesc->descriptorType == kUSBInterfaceDesc)		// look at the current descriptor
		{
			pMyIntDesc = (USBInterfaceDescriptorPtr)pCurrentDesc;	// if it's an interface descriptor
			if (pMyIntDesc->interfaceNumber == ReqInterface)		// see if it's the request descriptor
			{
				*hInterfaceDesc = pMyIntDesc;						// if it is, then return with hInterfaceDesc set to the
				return kUSBNoErr;											// current descriptor pointer
			}
		}
		anAddress = (unsigned long) pCurrentDesc;					// Nope, that either wasn't an interface descriptor
		anOffset  = (unsigned long) pCurrentDesc->length;
		anAddress += anOffset;
		pCurrentDesc = (USBDescriptorHeaderPtr) anAddress;
	}																// or it was, but not the droid we're looking for.
	return -1;
}
