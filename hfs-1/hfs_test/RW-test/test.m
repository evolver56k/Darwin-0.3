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
 * Rudimentary testing of the HFS filesystem implementation
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <c.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#import <Foundation/Foundation.h>

#define DEFAULTSTARTINGOFFSET 0
#define DEFAULTTRANSFERSIZE 512
#define NUMPRINTCOLUMNS 16

static char usage[] = "usage: %s [-v volID] [-d dirID] [-f fileName] [-o offset] [-s size]\n";
static char volfsName[] = "/.vol";
static char defaultTargetFileName[] = "README";



void PrintBuffer(char *bufferPtr, long startingOffset, long bufferLength) {
	long bufferAddress = startingOffset - (startingOffset % NUMPRINTCOLUMNS);
	long startingFill = startingOffset - bufferAddress;
	int column;
    char *cptr;
	
//	printf("PrintBuffer: startingOffset = %d, bufferLength = %d\n", startingOffset, bufferLength);
    while (bufferLength > 0) {
//		printf("PrintBuffer: bufferPtr = %p, bufferLength = %d.\n", bufferPtr, bufferLength);
		printf("0x%04X: ", (short)(bufferAddress & 0x0000FFFF));

        cptr = bufferPtr;
		for (column = 0; column < NUMPRINTCOLUMNS; ++column) {
            if ((column < startingFill) || ((column - startingFill) >= bufferLength)) {
				printf("   ");
			} else {
                printf("%02X ", (unsigned char)(*(cptr++)));
            };
		};
		
		printf("  ");
		
        cptr = bufferPtr;
		for (column = 0; column < NUMPRINTCOLUMNS; ++column) {
            if ((column < startingFill) || ((column - startingFill) >= bufferLength)) {
                printf(" ");
            } else {
                if ((*cptr < 0x20) || (*cptr >0x7E)) {
                    printf(".");
                } else {
                    printf("%c", *cptr);
                };
                ++cptr;
            };
		};
		
		printf("\n");
		
        bufferPtr += NUMPRINTCOLUMNS - startingFill;
        bufferLength -= NUMPRINTCOLUMNS - startingFill;
        bufferAddress += NUMPRINTCOLUMNS;
		startingFill = 0;
	};
}



void DumpFile(char *fileName, off_t startingOffset, long transferSize) {
	int fd;
	char *readBuffer;
	int bytesRead;
	
	readBuffer = malloc(transferSize);
	if (readBuffer == NULL) {
        err(errno, "ERROR: error trying to allocation a %d-byte I/O buffer", (int)transferSize);
	};
	
	fd = open(fileName, O_RDONLY, 0);
	if (fd < 0) {
        err(errno, "ERROR: error trying to open %s", fileName);
	};
    lseek(fd, startingOffset, SEEK_SET);
	bytesRead = read(fd, readBuffer, transferSize);
	if (bytesRead <= 0) {
        err(errno, "ERROR: error trying to read %d bytes from %s", (int)transferSize, fileName);
	} else {
        printf("Dumpfile: successfully read %d of %d bytes...\n", (int)bytesRead, (int)transferSize);
    };
    PrintBuffer(readBuffer, startingOffset, bytesRead);
	close(fd);
}



void TestFS(long volID, long DirID, char *fileName, off_t startingOffset, long transferSize) {
   	char pathName[PATH_MAX];
   	char name[MAXNAMLEN];
   	
   	strcpy(pathName, volfsName);
   	sprintf(name, "/%d/", (int)volID);
   	strcat(pathName, name);
   	sprintf(name, "%d/", (int)DirID);
   	strcat(pathName, name);
   	strcat(pathName, fileName);
   	DumpFile(pathName, startingOffset, transferSize);
}



int main (int argc, const char *argv[])
{
    int arg;
	DIR *volfsDirectory;
    struct dirent *volume;
    long targetVolID = 0;
    long targetDirID = 0;
    char targetFileName[MAXNAMLEN];
    bool targetFileNameSpecified = false;
    off_t startingOffset = DEFAULTSTARTINGOFFSET;
    long transferSize = DEFAULTTRANSFERSIZE;
	
//	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    for (arg = 1; arg < argc; ++arg) {
        if ((argv[arg][0] == '-') &&					// Starts with a '-'
             (argv[arg][2] == 0)) {						// ... and is exactly two characters long
            switch(argv[arg][1]) {
                case 'D':
                case 'd':
                    targetDirID = atoi(argv[++arg]);
                    break;

                case 'F':
                case 'f':
                    strncpy(targetFileName, argv[++arg], MAXNAMLEN-1);
                    targetFileName[MAXNAMLEN-1] = 0;									// Ensure proper termination...
                    targetFileNameSpecified = true;
                    break;

				case 'O':
				case 'o':
					startingOffset = atoi(argv[++arg]);
					break;
				
				case 'S':
				case 's':
					transferSize = atoi(argv[++arg]);
					break;
				
                case 'V':
                case 'v':
                    targetVolID = atoi(argv[++arg]);
                    break;

                default:
                    fprintf(stderr, "%s: unknown option '-%c'.\n", argv[0], argv[arg][1]);
                    fprintf(stderr, usage, argv[0]); 
            };
        };
    };

    if (targetVolID == 0) {
        volfsDirectory = opendir(volfsName);
        if (volfsDirectory == NULL) {
            err(errno, "ERROR: error trying to open %s", volfsName);
        };
        printf("Looking through %s...\n",volfsName);
        while ((volume = readdir(volfsDirectory)) != NULL) {
            if (volume->d_name[0] == '.') continue;

            targetVolID = atoi(volume->d_name);
            printf("targetVolID = %d: name = '%s'\n", (int)targetVolID, volume->d_name);
            break;
        };
        closedir(volfsDirectory);
    };

    if (targetDirID == 0) {
        targetDirID = 2;
    };

    if (targetFileNameSpecified == false) {
        strcpy(targetFileName, defaultTargetFileName);
    };

    fprintf(stderr, "%s: targetVolID = %d, dirID = %d, fileName = %s.\n",
            argv[0],
            (int)targetVolID,
            (int)targetDirID,
            targetFileName);
    TestFS(targetVolID, targetDirID, targetFileName, startingOffset, transferSize);

    printf("Done.\n");
    
//	[pool release];
	exit(0);       // insure the process exit status is 0
	return 0;      // ...and make main fit the ANSI spec.
}
