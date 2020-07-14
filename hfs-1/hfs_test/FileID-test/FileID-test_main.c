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
/*
    Copyright (c) 1998 Apple Computer, Inc.
    All Rights Reserved.

    THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
    The copyright notice above does not evidence any actual or
    intended publication of such source code.

    About FileID-test_main.c:
    This file test the FileID creation and resolution code for HFS/HFS+.

    To do:
    - Nothing.

    Change History:
    
     4-Jun-1998	Pat Dirks	New Today.
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <c.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../hfs_glue/vol.h"

static char usage[] = "usage: %s [-v volID] [-d dirID]\n";
static char gTestFileName[] = "FileID-TestTarget-XYZZY";

int TestCreateFileID(fsvolid_t volID, u_long dirID) {
	int result;
	u_long newFileID;
	u_long resolvedDirID;
	char resolvedFileName[1024];
	u_long resolvedNameEncoding;
	
	printf("Testing FileID for file '%s' in directory %ld on volume %d...\n", gTestFileName, dirID, volID);
	
    (void)Remove_VDI(volID, dirID, gTestFileName, 0, 0, 0);
	result = Create_VDI(volID, dirID, gTestFileName, 0, 0, 0, 0);
	if (result) {
		err(errno, "ERROR: Couldn't create test file '%s' in directory %ld on volume %d", gTestFileName, dirID, volID);
		return -1;
	};
	
    result = CreateFileID_VDI(volID, dirID, gTestFileName, 0, 0, 0, &newFileID);
	if (result) {
		err(errno, "ERROR: Couldn't create FileID for '%s' in directory %ld on volume %d", gTestFileName, dirID, volID);
		return -1;
	};
	
    result = ResolveFileID_VDI(volID, newFileID, kMacOSHFSFormat, &resolvedDirID, resolvedFileName, sizeof(resolvedFileName), &resolvedNameEncoding);
	if (result) {
		err(errno, "ERROR: Couldn't resolve FileID %ld", newFileID);
		return -1;
	};
	
	if (resolvedDirID != dirID) {
		printf("ERROR: resolved DirID (%ld) doesn't match original DirID (%ld).\n", resolvedDirID, dirID);
		return -1;
	};

    if (strcmp(resolvedFileName, gTestFileName)) {
		printf("ERROR: resolved file name ('%s') doesn't match original name ('%s').\n", resolvedFileName, gTestFileName);
		return -1;
	};
	
	if (resolvedNameEncoding != 0) {
		printf("ERROR: resolved name encoding (0x%08lX) doesn't match original encoding (0).\n", resolvedNameEncoding);
    };
	
	/* Keep the destination directory neat and tidy: */
	(void)Remove_VDI(volID, dirID, gTestFileName, 0, 0, 0);

    return 0;
}


int main (int argc, const char *argv[])
{
    int arg;
    fsvolid_t targetVolID = 0;
    bool targetVolIDSpecified = false;
    u_long targetDirID = 0;
    bool targetDirIDSpecified = false;
    VolumeAttributeInfo volInfo;
    int result;

    for (arg = 1; arg < argc; ++arg) {
        if ((argv[arg][0] == '-') &&					// Starts with a '-'
             (argv[arg][2] == 0)) {						// ... and is exactly two characters long
            switch(argv[arg][1]) {
                case 'D':
                case 'd':
                    targetDirID = (u_long)atoi(argv[++arg]);
                    targetDirIDSpecified = true;
                    break;

                case 'V':
                case 'v':
                    targetVolID = (fsvolid_t)atoi(argv[++arg]);
                    targetVolIDSpecified = true;
                    break;

                default:
                    fprintf(stderr, "%s: unknown option '-%c'.\n", argv[0], argv[arg][1]);
                    fprintf(stderr, usage, argv[0]); 
            };
        };
    };

    if (! targetDirIDSpecified) targetDirID = 2L;

    if (! targetVolIDSpecified) {
		result = GetVolumeInfo_VDI(1, 0, NULL, 0, 0, 0, &volInfo);
		if (result != 0) {
			err(errno, "ERROR: No HFS or HFS+ volumes found");
		exit(1);
        };
        targetVolID = volInfo.volumeID;
    };
    
    printf("Target VolID = %d, target DirID = %ld.\n", targetVolID, targetDirID);
	printf("Starting FileID tests...\n");
	result = TestCreateFileID(targetVolID, targetDirID);
	if (result) {
		exit(1);
	};
	printf("Done.\n");
	
	exit(0);       // insure the process exit status is 0
	return 0;      // ...and make main fit the ANSI spec.
}
