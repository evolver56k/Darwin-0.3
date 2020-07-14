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
 * Rudimentary testing of the HFS filesystem attribute implementation
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
static char volfsName[] = "/.vol";

int TestFolder(fsvolid_t targetVolID, u_long targetDirID, char *targetFileName);
int TestFile(fsvolid_t targetVolID, u_long targetDirID, char *targetFileName);



int main (int argc, const char *argv[])
{
    int arg;
	DIR *volfsDirectory;
    struct dirent *volume;
    fsvolid_t targetVolID = 0;
    bool targetVolIDSpecified = false;
    u_long targetDirID = 0;
    bool targetDirIDSpecified = false;
    char targetFileName[MAXNAMLEN];
    bool targetFileNameSpecified = false;
    int result;
	
    targetFileName[0] = 0;								// Null out by default

    for (arg = 1; arg < argc; ++arg) {
        if ((argv[arg][0] == '-') &&					// Starts with a '-'
             (argv[arg][2] == 0)) {						// ... and is exactly two characters long
            switch(argv[arg][1]) {
                case 'D':
                case 'd':
                    targetDirID = (u_long)atoi(argv[++arg]);
                    targetDirIDSpecified = true;
                    break;

                case 'F':
                case 'f':
                    strncpy(targetFileName, argv[++arg], MAXNAMLEN-1);
                    targetFileName[MAXNAMLEN-1] = 0;					// Ensure proper termination...
                    targetFileNameSpecified = true;
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

    if (targetVolIDSpecified) {
        result = TestFolder(targetVolID, targetDirID, targetFileName);
        if (result != 0) {
            err(result,
                "%s: error from TestFolder(VolID = %ld, DirID = %ld, Name = '%s')",
                argv[0],
                targetVolID,
                targetDirID,
                targetFileName);
            return result;
        };
    } else {
        volfsDirectory = opendir(volfsName);
        if (volfsDirectory == NULL) {
            err(errno, "error trying to open %s", volfsName);
        };
        printf("Looking through %s...\n",volfsName);
        while ((volume = readdir(volfsDirectory)) != NULL) {
            if (volume->d_name[0] == '.') continue;

            targetVolID = atoi(volume->d_name);
            printf("volid = %d...\n", targetVolID);
            result = TestFolder(targetVolID, targetDirID, targetFileName);
            if (result != 0) {
                err(result,
                    "%s: error from TestFolder(VolID = %ld, DirID = %ld, Name = '%s')",
                    argv[0],
                    targetVolID,
                    targetDirID,
                    targetFileName);
                return result;
            };
            printf("Done.\n");
            closedir(volfsDirectory);
        };
    };

	exit(0);       // insure the process exit status is 0
	return 0;      // ...and make main fit the ANSI spec.
}



int TestFolder(fsvolid_t targetVolID, u_long targetDirID, char *targetFileName) {
    char dirname[1] = {0};
    DIR *directory;
    struct dirent *catalogEntry;
    int result = 0;

    if (targetFileName[0] != 0) {
        result = TestFile(targetVolID, targetDirID, targetFileName);
    } else {
        directory = OpenDir_VDI(targetVolID, targetDirID, dirname, 0, 0, kDontTranslateSeparators);
        if (directory == NULL) {
            err(errno, "error trying to open directory %ld on volume %ld", targetDirID, targetVolID);
            result = -1;
        } else {
            printf("Looking through directory %ld on volid %d...\n",targetDirID, targetVolID);
            while ((catalogEntry = readdir(directory)) != NULL) {
                if (catalogEntry->d_name[0] == '.') continue;

                result = TestFolder(targetVolID, targetDirID, catalogEntry->d_name);
                if (result != 0) break;
            };
        };
        closedir(directory);
    };

    return result;
}



int TestFile(fsvolid_t targetVolID, u_long targetDirID, char *targetFileName) {
    int result;
    CatalogAttributeInfo catInfo;
    char finderType[5],finderCreator[5];

    result = GetCatalogInfo_VDI(targetVolID, targetDirID, targetFileName, 0, kTextEncodingUTF8,
								kDontTranslateSeparators, &catInfo, kMacOSHFSFormat, NULL, 0);
    if (result != 0) {
        err(errno, "error retrieving info for VolID %ld, DirID %ld, Name '%s'", targetVolID, targetDirID, targetFileName);
        return errno;
    };
    printf("%d: <%ld>'%s': Obj. Type = %d",
           targetVolID, targetDirID, targetFileName, catInfo.c.objectType);
    switch (catInfo.c.objectType) {
        case VDIR:
            printf(", DirID = %ld.\n", catInfo.c.objectID);
            break;

        case VREG:
        case VCPLX:
            memcpy(finderType, &catInfo.c.finderInfo[0], 4);
            finderType[4] = 0;
            memcpy(finderCreator, &catInfo.c.finderInfo[1], 4);
            finderCreator[4] = 0;
            printf(", Type = '%4s', Creator = '%4s'\n", finderType, finderCreator);
            break;

       default:
           printf(" (unknown object type).\n");
    };

    return 0;
}
