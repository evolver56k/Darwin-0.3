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
 * Rudimentary testing of the HFS filesystem file locking code.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
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

#define IMMUTABLE UF_IMMUTABLE

static char gTargetFileName[] = "LOCKING-TEST-FILE";
static char gVolfsName[] = "/.vol";

int main (int argc, const char *argv[])
{
    int result;
    fsvolid_t targetVolID = 0;
    int volIndex;
    VolumeAttributeInfo volInfo;
    CatalogAttributeInfo catInfo;
    char targetPathName[PATH_MAX];
    struct stat statBuf;

    for (volIndex = 1; volIndex < 16; ++volIndex) {
        result = GetVolumeInfo_VDI(volIndex, 0, NULL, 0, 0, 0, &volInfo);
        if (result == 0) {
            printf("Lock-test: Found volume '%s'...\n", volInfo.name);
            targetVolID = volInfo.volumeID;
            break;
        };
    };
    if (targetVolID == 0) {
        printf("Lock-test: No suitable volumes found on-line.\n");
        exit(1);
    };

    if (GetCatalogInfo_VDI(targetVolID, 2, gTargetFileName, 0, 0, 0, &catInfo, kMacOSHFSFormat, NULL, 0) != 0) {
        result = Create_VDI(targetVolID, 2, gTargetFileName, 0, 0, 0, 0);
        if (result != 0) {
            err(result,
                "Lock-test: error from Create_VDI(VolID = %ld, DirID = %ld, Name = '%s')",
                targetVolID,
                2,
                gTargetFileName);
        };
    };

    /* Show the initial stat of the file's flags: */
    sprintf(targetPathName, "%s/%d/2/%s", gVolfsName, targetVolID, gTargetFileName);
    result = stat(targetPathName, &statBuf);
    if (result != 0) {
        err(result, "Lock-test: error from stat('%s')", targetPathName);
    };
    printf("Lock-test: stat(): st_flags = 0x%lX.\n", (u_long)statBuf.st_flags);

    /* Lock the file and verify the change: */
    printf("Lock-test: locking file...\n");
    result = chflags(targetPathName, (u_long)IMMUTABLE);
    if (result != 0) {
        err(result, "Lock-test: error from chflags('%s', 0x%lX)", targetPathName, (u_long)IMMUTABLE);
    };
    result = stat(targetPathName, &statBuf);
    if (result != 0) {
        err(result, "Lock-test: error from stat('%s')", targetPathName);
    };
    printf("Lock-test: stat(): st_flags = 0x%lX.\n", (u_long)statBuf.st_flags);

    /* Unlock the file and verify the change: */
    printf("Lock-test: unlocking file...\n");
    result = chflags(targetPathName, 0);
    if (result != 0) {
        err(result, "Lock-test: error from chflags('%s',0)", targetPathName);
    };
    result = stat(targetPathName, &statBuf);
    if (result != 0) {
        err(result, "Lock-test: error from stat('%s')", targetPathName);
    };
    printf("Lock-test: stat(): st_flags = 0x%lX.\n", (u_long)statBuf.st_flags);

    exit(0);       // insure the process exit status is 0
    return 0;      // ...and make main fit the ANSI spec.
}
