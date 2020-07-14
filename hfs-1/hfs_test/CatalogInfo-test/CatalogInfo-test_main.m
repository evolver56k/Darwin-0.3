/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/attr.h>
#include <sys/fcntl.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../hfs_glue/vol.h"

//#import <Foundation/Foundation.h>

int main (int argc, const char *argv[])
{
	char nameBuf[256];
	VolumeAttributeInfo vInfo;
	CatalogAttributeInfo info;
	int result;
	long volID;
	u_long savedFinderInfo[8];
	int i;
	uid_t savedOwner;
	gid_t savedGroup;
	mode_t savedMode;
	
//  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	printf("Starting Get/SetCatatlogInfo test...\n");
	
    result = GetVolumeInfo_VDI(1, 0, NULL, 0, 0, 0, &vInfo);
	if (result)
	{
        err(result, "ERROR: Couldn't find any volume");
        return result;
	}
	
	printf("Found volume <%s>, id %d\n", vInfo.name, vInfo.volumeID);
	volID = vInfo.volumeID;
	
	strcpy(nameBuf, "TestFile");
    result = Remove_VDI(volID, 2, nameBuf, 0, 0, 0);
    if (result) {
        if (result != ENOENT) {
            err(errno, "ERROR: Remove_VDI in initial cleanup returned %d; errno = %d", result, errno);
            return result;
        };
    };
	result = Create_VDI(volID, 2, nameBuf, 0, 0, 0, O_TRUNC);
    if (result) {
        err(errno, "ERROR: Create_VDI returned %d; errno = %d", result, errno);
        return result;
    } else {
        printf("Successfully created file 'TestFile'.\n");
    };
	
	/* Get the information on the test file: */
	info.commonBitmap =  ATTR_CMN_MODTIME + ATTR_CMN_CRTIME + ATTR_CMN_FNDRINFO +
							ATTR_CMN_OWNERID + ATTR_CMN_GRPID + ATTR_CMN_ACCESSMASK;
	result = GetCatalogInfo_VDI(volID, 2, nameBuf, 0, 0, 0, &info, kMacOSHFSFormat, NULL, 0);
    if (result) {
        err(errno, "ERROR: GetCatalogInfo returned %d; errno = %d", result, errno);
    } else {
        printf("GetCatalogInfo returned successfully.\n");
    };
	/* Save off the current values of the Finder info: */
	for (i = 0; i < 8; ++i) {
		savedFinderInfo[i] = info.c.finderInfo[i];
	};
	savedOwner = info.c.uid;
	savedGroup = info.c.gid;
	savedMode = info.c.mode;
	printf("owner = %d, group = %d, mode = 0x%04X.\n", info.c.uid, info.c.gid, info.c.mode);
	printf("mod. = 0x%08lX, create = 0x%08lX.\n", info.c.lastModificationTime.tv_sec, info.c.creationTime.tv_sec);
	printf("Finder type = 0x%08lX, creator =  0x%08lX.\n", info.c.finderInfo[0], info.c.finderInfo[1]);
	
	/* Change the Finder filetype and creator and mess with the owner/group/mode: */
	printf("Flipping type, creator, owner, group, and mode...\n");
	info.c.finderInfo[0] = ~info.c.finderInfo[0];
	info.c.finderInfo[1] = ~info.c.finderInfo[1];
	info.c.uid = ~info.c.uid;
	info.c.gid = ~info.c.gid;
	info.c.mode = ~info.c.mode;
	info.commonBitmap =  ATTR_CMN_MODTIME + ATTR_CMN_CRTIME + ATTR_CMN_FNDRINFO +
							ATTR_CMN_OWNERID + ATTR_CMN_GRPID + ATTR_CMN_ACCESSMASK;
	info.fileBitmap = 0;
	info.directoryBitmap = 0;
    result = SetCatalogInfo_VDI(volID, 2, nameBuf, 0, 0, 0, &info);
    if (result) {
        err(errno, "ERROR: SetCatalogInfo returned %d; errno = %d", result, errno);
	} else {
        printf("SetCatalog returned successfully.\n");
    };
	/* Check to see whether the changes took effect: */
    result = GetCatalogInfo_VDI(volID, 2, nameBuf, 0, 0, 0, &info, kMacOSHFSFormat, NULL, 0);
    if (result) {
        err(errno, "ERROR: GetCatalogInfo returned %d; errno = %d", result, errno);
    } else {
        printf("GetCatalogInfo returned successfully.\n");
    };
	printf("owner = %d, group = %d, mode = 0x%04X.\n", info.c.uid, info.c.gid, info.c.mode);
	printf("mod. = 0x%08lX, create = 0x%08lX.\n", info.c.lastModificationTime.tv_sec, info.c.creationTime.tv_sec);
	printf("Finder type = 0x%08lX, creator =  0x%08lX.\n", info.c.finderInfo[0], info.c.finderInfo[1]);
	
	/* Restore the original Finder info, owner, group, and mode: */
	printf("Restoring Finder info, owner, group, and mode...\n");
	for (i = 0; i < 8; ++i) {
		info.c.finderInfo[i] = savedFinderInfo[i];
	};
	info.c.uid = savedOwner;
	info.c.gid = savedGroup;
	info.c.mode = savedMode;
	info.commonBitmap = ATTR_CMN_FNDRINFO + ATTR_CMN_OWNERID + ATTR_CMN_GRPID + ATTR_CMN_ACCESSMASK;
	result = SetCatalogInfo_VDI(volID, 2, nameBuf, 0, 0, 0, &info);
    if (result) {
        err(errno, "ERROR: SetCatalogInfo returned %d; errno = %d", result, errno);
    } else {
        printf("SetCatalogInfo returned successfully.\n");
    };
	
    result = GetCatalogInfo_VDI(volID, 2, nameBuf, 0, 0, 0, &info, kMacOSHFSFormat, NULL, 0);
    if (result) {
        err(errno, "ERROR: GetCatalogInfo returned %d; errno = %d", result, errno);
    } else {
        printf("GetCatalogInfo returned successfully.\n");
    };
	printf("owner = %d, group = %d, mode = 0x%04X.\n", info.c.uid, info.c.gid, info.c.mode);
	if (info.c.uid != savedOwner) {
		printf("ERROR: owner is 0x%08X (should be 0x%08X).\n", info.c.uid, savedOwner);
	}
	if (info.c.gid != savedGroup) {
		printf("ERROR: group is 0x%08X (should be 0x%08X).\n", info.c.gid, savedGroup);
	};
	if (info.c.mode != savedMode) {
		printf("ERROR: mode is 0x%08X (should be 0x%08X).\n", info.c.mode, savedMode);
	};
	printf("mod. = 0x%08lX, create = 0x%08lX.\n", info.c.lastModificationTime.tv_sec, info.c.creationTime.tv_sec);
	for (i = 0; i < 8; ++i) {
		if (info.c.finderInfo[i] != savedFinderInfo[i]) {
			printf("ERROR: finderInfo[%d] is 0x%08lX (should be 0x%08lX).\n", i, info.c.finderInfo[i], savedFinderInfo[i]);
		};
	}

//  [pool release];
	exit(0);       // insure the process exit status is 0
	return 0;      // ...and make main fit the ANSI spec.
}
