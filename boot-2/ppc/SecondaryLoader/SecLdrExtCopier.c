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
#include <Types.h>
#include <Files.h>
#include <Devices.h>
#include <SCSI.h>
#include <Memory.h>
#include <Resources.h>
#include <Errors.h>
#include <string.h>
#include <stdlib.h>
#include <stat.h>
#include <errno.h>
#include <stdio.h>

#include "SCSIIO.h"

enum {kTargetID = 2};
enum {kMaxPartitionMap = 64};

static Partition map[kMaxPartitionMap];
int gNPartitionMapBlocks;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// A convention:  Everywhere we use a partition "index" herein, it is an index into to the
// gPartitionMapP array, which is zero origin.  Adding ONE to one of these "index" values
// gives the block number of the disk block that partition map element lives in.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

static const UInt8 kExtensionPartitionType[] = "Apple_MacOSPrepExtension";
static const UInt8 kExtensionPartitionName[] = "SecondaryLoaderExtension";

static const char kExtensionFileName[] = "SecondaryLdrExt";


static void preparePartitionBuffer (Partition *partP, UInt32 status,
									const UInt8 *partitionType, const UInt8 *partitionName, const UInt8 *processorType);
static int useSpaceFromPartition (int partitionMapIndex);
static void markLoaderPartitionValid (int partitionMapIndex);


static void fatal (char *msg, int st)
{
	printf ("%s: %d\n", msg, st);
	exit (999);
}


static char toUpper (const UInt8 c)
{
	if (c >= 'a' && c <= 'z')
		return c - ('a' - 'A');
	else
		return c;
}


// Compare two NUL-terminated C strings for equality in a case-insensitive manner.
// Returns true if the strings ARE equal, false otherwise.
static Boolean partitionStringIsEqual (const UInt8 *string1, const UInt8 *string2)
{
	while (*string1 && *string2) {
		if (toUpper (*string1++) != toUpper (*string2++)) return false;
	}

	return (*string1 == *string2);
}


void main ()
{
	OSErr st;
	int k;
	int extensionIndex = -1;
	Handle loaderH;
	FILE *f;
	UInt32 oldSize;
	UInt32 newSize;
	struct stat statbuf;

	// Read partition map
	st = ReadSCSI (kTargetID, map, 1, kMaxPartitionMap);
	if (st != noErr) fatal ("Can't read partition map", st);
	gNPartitionMapBlocks = map[0].pmMapBlkCnt;

	// Walk through the partition map looking for the partition map's partition map entry,
	// any existing kLoaderPartitionType entry, and finally for Apple_Free entries.
	// Remember the index of any and all of these that are found.
	for (k = 0; k < gNPartitionMapBlocks; ++k) {

		if (partitionStringIsEqual ((const UInt8 *) map[k].pmParType, kExtensionPartitionType))
		{
			extensionIndex = k;
			break;								// It's a fine place to put the loader
		}
	}

	if (extensionIndex == -1) fatal ("No existing \"Apple_MacOSPrepExtension\" partition found", 0);

	// Get the Self PEF Loader resource and append the contents of the extension to it
	loaderH = Get1Resource ('SLdr', 0);
	if (loaderH == nil) fatal ("Error reading Self PEF Loader (SLdr=0)", ResError ());
	DetachResource (loaderH);							// Detach this so we can screw around with it
	oldSize = GetHandleSize (loaderH);					// Remember how big it WAS
	HUnlock (loaderH);
	HNoPurge (loaderH);

	f = fopen (kExtensionFileName, "rb");
	if (f == nil) fatal ("Can't open SecondaryLdrExt file", errno);
	if (stat (kExtensionFileName, &statbuf) != 0) fatal ("stat failed on SecondaryLdrExt file", errno);

	newSize = oldSize + statbuf.st_size;		// This is size of DATA FORK, right?

	SetHandleSize (loaderH, newSize);
	st = MemError ();									// Make DAMNED sure
	if (st != noErr) fatal ("Can't get enough memory for Self PEF Loader + extension image", st);
	HLock (loaderH);

	if (fread (*loaderH + oldSize, 1, statbuf.st_size, f) != statbuf.st_size)
		fatal ("Read of extension image failed", errno);
	
	fclose (f);

	// Write the loader into its new partition
	st = WriteSCSI (kTargetID, *loaderH,
					map[extensionIndex].pmPyPartStart, (newSize + kBlockSize - 1) / kBlockSize);
	if (st != noErr) fatal ("Error writing extension image into its partition", st);
	fclose (f);
}
