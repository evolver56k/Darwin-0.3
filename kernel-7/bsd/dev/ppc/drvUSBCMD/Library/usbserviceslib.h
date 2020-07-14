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
	File:		USBServicesLib.h

	Contains:	USB Services Library public include.

	Version:	Neptune 1.0

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Barry Twycross

		Other Contact:		xxx put other contact here xxx

		Technology:			USB

	Writers:

		(DF)	David Ferguson
		(DKF)	David Ferguson
		(CJK)	Craig Keithley
		(TC)	Tom Clark
		(BT)	Barry Twycross

	Change History (most recent first):

	 <USB36>	  4/8/98	DF		Replace contents with include of the correct file
		<35>	  4/8/98	BT		Add error codes
		<34>	  4/6/98	BT		Fix spelling
		<33>	  4/6/98	BT		New param block names
		<32>	  4/2/98	BT		Avoid C++ keywords (class)
		<31>	 3/31/98	TC		Add error #-5150: kUSBBadDispatchTable.
		<30>	 3/24/98	BT		Eliminate include of private defines
		<29>	 3/19/98	BT		Split UIM into UIM and root hub parts
		<28>	 3/18/98	BT		Add remove device.
		<27>	 3/11/98	BT		Fix bad typedef
		<26>	 3/11/98	CJK		Add report & physical descriptor enums
		<25>	 3/10/98	CJK		Add HID Descriptor struct
		<24>	  3/9/98	BT		Eliminate one set of redundant enums
		<23>	  3/9/98	CJK		Fix RADAR #2216609 (Duplicate sets of enums).  Fix problem with
									USB_CONSTANT16 (wasn't getting rid of high order byte before
									shifting contents to the left).
		<22>	 2/27/98	BT		Add swapped USB constants
	 <USB21>	 2/17/98	DKF		Fix a typo in the error codes
		<20>	 2/10/98	BT		Incorporate Ferg's changes
		<19>	  2/9/98	BT		Add add interface stuff
		<18>	  2/8/98	BT		More Power allocation stuff
		<17>	  2/8/98	BT		Add version function
		<16>	  2/5/98	BT		Add status notification stuff
		<15>	  2/4/98	BT		Add ref/port info to expert notify]
		<14>	  2/4/98	BT		Fix endpoint desc
		<13>	  2/3/98	BT		Add expert notify function
		<12>	  2/2/98	BT		Add bulk stuff
		<11>	 1/29/98	BT		Fix illegal struct decls MPW doesn't catch, add class driver
									functionality
		<10>	 1/26/98	CJK		change device descriptor to be prefixed with USB.
		 <9>	 1/26/98	BT		Mangle names after design review, finsih up
		 <8>	 1/26/98	BT		Mangle names after design review
		 <7>	 1/26/98	BT		Make expert notify public
		 <6>	 1/23/98	BT		Add USB delay function
		 <5>	 1/21/98	BT		Change hardware status codes
		 <4>	 1/21/98	TC		Correct misspelling of InterfaceDescriptor struct typedef. Add
									ConfigurationDescriptor definition.
		 <3>	 1/15/98	TC		Add InterfaceDescriptor definition.
		 <2>	 1/15/98	CJK		Change file header to indicate name is USBServicesLib.h
		 <1>	 1/15/98	CJK		Created USBServicesLib.h files (From USL.H, which is now obsolete)
		<10>	 1/14/98	BT		Remove pre expert expert stuff
		 <9>	  1/8/98	TC		add struct pointer to devicedescriptor typedef.
		 <8>	12/19/97	BT		Add add and remove bus.
		 <7>	12/18/97	BT		Change defs of expert notify proc
		 <6>	12/18/97	BT		Emulate expert notification proc.
		 <5>	12/18/97	BT		Tidy up header
		 <4>	12/18/97	BT		Moved device descriptor in here.
*/

#include "../USB.h"
