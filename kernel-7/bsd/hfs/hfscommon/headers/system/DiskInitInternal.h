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
	File:		DiskInitInternal.h

	Contains:	IPI for HFS Disk Initialization

 	Version:	HFS Plus 1.0

	Copyright:	© 1996-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			File Systems

	Writers:

		(DSH)	Deric Horn
		(msd)	Mark Day
		(djb)	Don Brady

	Change History (most recent first):

	  <CS11>	10/29/97	djb		Add clump size factor. Increase kMinHFSPlusVolumeSize to 32 MB.
	  <CS10>	10/23/97	msd		Bug 1685113. Add a createDate parameter to
									InitializeHFSPlusVolume.
	   <CS9>	 9/26/97	DSH		Moved HFSPlusDefaults to DiskInit.i
	   <CS8>	  9/5/97	msd		Add a parameter to InitHFSVolume to indicate alignment of
									allocation blocks.
	   <CS7>	  9/4/97	msd		In HFSPlusDefaults, specify the clump and node sizes of the
									attributes file. Bump kHFSPlusDefaultsVersion to 1.
	   <CS6>	 7/18/97	msd		Include DiskInit.h.
	   <CS5>	  7/2/97	msd		Add prototype for GetMediaInfo driver status call.
	   <CS4>	 6/17/97	msd		Change minimum HFS Plus volume size to 2 MB (so we can test with
									RAM disks). Added version and flags fields to HFSPlusDefaults.
	   <CS3>	 6/11/97	msd		Remove the #pragma load; this should be done by source files,
									and generally not interface files. (Allows the test tools to
									build separately from HFS Plus.)
	   <CS2>	  6/6/97	djb		Change kMinHFSVolumeSize to kMinHFSPlusVolumeSize and make it
									20MB
	   <CS1>	 4/28/97	djb		first checked in
	  <HFS5>	  3/3/97	djb		Add new fields to DriveInfo and HFSPlusDefaults.
	  <HFS4>	12/19/96	djb		Changed kMinHFSVolumeSize so we can make HFS+ floppies for
									debugging.
	  <HFS3>	12/10/96	msd		Check PRAGMA_LOAD_SUPPORTED before loading precompiled headers.
	  <HFS2>	 12/4/96	DSH		Precompiled Headers
	  <HFS1>	 12/4/96	djb		first checked in
*/

#ifndef __DISKINITINTERNAL__
#define __DISKINITINTERNAL__

#include <Types.h>
#include <DiskInit.h>


enum {
	kHFS1440KAltFormatStrID		= -6077,
	kHFS800KAltFormatStrID		= -6076,
	kMacintoshStrID				= -6075,	// Macintosh name StrID
	kHFS720KFormatStrID			= -6074,	// Format name string for 720K HFS
	kHFS720KAltFormatStrID		= -6073		// HFS Alternate Format string for 720K HFS
};

enum {
	kMinHFSPlusVolumeSize	= (32*1024*1024),	// 32 MB
	
	kBytesPerSector			= 512,				// default block size	
	kBitsPerSector			= 4096,				// 512 * 8
	kLog2SectorSize			= 9,				// log2 of 512 when sector size = 512
	
	kHFSPlusDataClumpFactor	= 16,
	kHFSPlusRsrcClumpFactor = 16
};


struct DriveInfo {
	SInt16			driveNumber;
	SInt16 			driverRefNum;
	UInt32			totalSectors;
	UInt32			sectorSize;
	UInt32			sectorOffset;
};
typedef struct DriveInfo DriveInfo;



//
//	The following is a new driver status call that returns a device's actual block
//	size.  It should be moved to some other interface file so third parties can
//	support it.
//
enum { csGetMediaInfo = 128 };
typedef struct GetMediaCapCntrlParam {
	QElem			*qLink;
	short			qType;
	short			ioTrap;
	Ptr				ioCmdAddr;
	IOCompletionUPP	ioCompletion;		/* completion routine addr (0 for synch calls) */
	OSErr			ioResult;			/* result code */
	StringPtr		ioNamePtr;
	short			ioVRefNum;			/* drive number */
	short			ioRefNum;			/* refNum for I/O operation */
	short			csCode;				/* word for control status code */
	struct	{
		unsigned long	numBlks;		// number of blocks on media
		unsigned long	blksize;		// block size in that session
		short			mediaType;		// block size in that session
	}				csParam;			/* operation-defined parameters */
} GetMediaCapCntrlParam;


//
// Exported function prototypes
//

pascal OSErr HFSDiskInitComponent (short whatFunction, void *paramBlock, void *fsdGlobalPtr);

OSErr InitHFSVolume (const DriveInfo	*driveInfo, ConstStr31Param volumeName, HFSDefaults *defaults, UInt32 alignment);

OSErr InitializeHFSPlusVolume (const DriveInfo *driveInfo, ConstStr31Param volumeName, HFSPlusDefaults *defaults, UInt32 createDate);


#endif /* __DISKINITINTERNAL__ */
