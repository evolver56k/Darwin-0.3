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
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <assert.h>
#include <c.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
//#import <Foundation/Foundation.h>
#include "../../hfs_glue/vol.h"

#define DEBUG 1

#define RANDSEEDRANGE 1000

#define MINBLOCKSIZE 1
#define MAXBLOCKSIZE 16384L
#define VERIFYBLOCKSIZE 512

#define MINBLOCKCOUNT 1
#define MAXBLOCKCOUNT 15

#define DATAPATTERN(BYTEPOSITION) (((BYTEPOSITION) + ((BYTEPOSITION) / 256) + ((BYTEPOSITION) / 65536L)) & 0x000000FF)

#define DEFAULTDIRID 2

#define DEFAULTSTARTINGOFFSET 0
#define DEFAULTTRANSFERSIZE 512
#define NUMPRINTCOLUMNS 16

static char usage[] = "usage: %s [-v volID] [-d dirID] [-f fileName] [-b blocksize] [-n blockcount] [-m maxblocksize] [-i rng-seed]\n";
static char volfsName[] = "/.vol";
static char defaultTargetFileName[] = "ScatterTest";

char gIOBuffer[MAXBLOCKSIZE];


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



int WriteBlock(int dataFile, u_long blockNumber, size_t blockSize) {
    int i;
	off_t bytePosition;
    off_t newPosition;
    ssize_t bytesWritten;

	bytePosition = (off_t)blockNumber * (off_t)blockSize;
#if DEBUG
    fprintf(stderr, "WriteBlock: writing %ld-byte block %ld (byte position 0x%08qX):\n", blockSize, blockNumber, bytePosition);
#endif
	for (i = 0; i < blockSize; ++i, ++bytePosition) {
        gIOBuffer[i] = DATAPATTERN(bytePosition);
	};
#if DEBUG
//  PrintBuffer(gIOBuffer, 0, blockSize);
#endif
    bytePosition = (off_t)blockNumber * (off_t)blockSize;
    newPosition = lseek(dataFile, bytePosition, SEEK_SET);
    if (newPosition != bytePosition) {
        err(errno, "ERROR: WriteBlock: error trying to seek to offset 0x%08qX on fd %d (newPosition = 0x%08qX).", bytePosition, dataFile, newPosition);
        return errno > 0 ? errno : EIO;
    };
    bytesWritten = write(dataFile, gIOBuffer, blockSize);
    if ((bytesWritten != blockSize) || (bytesWritten == -1)) {
        return (errno != 0) ? errno : EIO;
    };

    return 0;
};



int VerifyBlock(int dataFile, u_long blockNumber, size_t blockSize, off_t fileSize) {
	int i;
	off_t bytePosition;
    off_t newPosition;
	size_t bytesInBlock;
	ssize_t bytesRead;
	
    bytePosition = (off_t)blockNumber * (off_t)blockSize;
	bytesInBlock = (size_t)((bytePosition + blockSize <= fileSize) ? blockSize : fileSize - bytePosition);
#if DEBUG
//  fprintf(stderr, "VerifyBlock: verifying block %ld, %ld bytes starting at 0x%08qX...\n", blockNumber, bytesInBlock, bytePosition);
#endif
    newPosition = lseek(dataFile, bytePosition, SEEK_SET);
    if (newPosition != bytePosition) {
        err(errno, "ERROR: VerifyBlock: error trying to seek to offset 0x%08qX on fd %d (newPosition = 0x%08qX).",
            bytePosition, dataFile, newPosition);
        return errno > 0 ? errno : EIO;
    };
    bytesRead = read(dataFile, gIOBuffer, bytesInBlock);
    if (bytesRead == -1) {
        err(errno, "ERROR: VerifyBlock: error trying to read block %ld, %d bytes on fd %d at 0x%08qX of 0x%08qX.",
            blockNumber, bytesInBlock, dataFile, bytePosition, fileSize);
        return errno > 0 ? errno : EIO;
    };
	if (bytesRead != bytesInBlock) {
		fprintf(stderr, "VerifyBlock: blockNumber = %ld, blockSize = %ld, bytesRead = %ld (should be %ld for file size %qd).\n",
					blockNumber, blockSize, (unsigned long)bytesRead, bytesInBlock, fileSize);
		return EIO;
	};
	
	/* Verify that the data returned is, in fact, correct for the given byte position: */
    for (i = 0; i < bytesInBlock; ++i, ++bytePosition) {
        if ((unsigned char)gIOBuffer[i] != (unsigned char)DATAPATTERN(bytePosition)) {
			fprintf(stderr, "VerifyBlock: byte 0x%08qX read back as 0x%02X, should be 0x%02X.\n",
                            bytePosition, (unsigned char)gIOBuffer[i], (unsigned char)DATAPATTERN(bytePosition));
			return EIO;
		};
	};
	
	return 0;
};



int main (int argc, const char *argv[])
{
    int i,j,t;
    int arg;
	DIR *volfsDirectory;
    struct dirent *volume;
    long targetVolID = 0;
    long targetDirID = 0;
    long blockSize = 0;
    long blockCount = 0;
    unsigned long maxblocksize = MAXBLOCKSIZE;
    off_t fileSize;
    struct timeval currentTime;
    unsigned long randomSeed;
    char targetFileName[MAXNAMLEN];
    bool targetFileNameSpecified = false;
	long volID;
	int dataFile;
	long targetBlock[MAXBLOCKCOUNT];
	int result = 0;
		
//  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	gettimeofday(&currentTime, NULL);
	randomSeed = currentTime.tv_sec % RANDSEEDRANGE;
	
    for (arg = 1; arg < argc; ++arg) {
        if ((argv[arg][0] == '-') &&					// Starts with a '-'
             (argv[arg][2] == 0)) {						// ... and is exactly two characters long
            switch(argv[arg][1]) {
                case 'B':
                case 'b':
                	blockSize = atoi(argv[++arg]);
                    break;
                
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
				
				case 'I':
				case 'i':
					randomSeed = atoi(argv[++arg]);
                    break;
				
                case 'M':
                case 'm':
                    maxblocksize = atoi(argv[++arg]);
                    maxblocksize = MIN(maxblocksize, MAXBLOCKSIZE);
                    break;

				case 'N':
				case 'n':
					blockCount = atoi(argv[++arg]);
                    break;
					
                case 'V':
                case 'v':
                    targetVolID = atoi(argv[++arg]);
                    break;

                default:
                    fprintf(stderr, "%s: unknown option '-%c'.\n", argv[0], argv[arg][1]);
                    fprintf(stderr, usage, argv[0]);
                    break;
            };
        };
    };

	fprintf(stderr, "%s: initial sequence number is %ld.\n", argv[0], randomSeed);
	srandom(randomSeed);
	
    if (targetVolID == 0) {
        volfsDirectory = opendir(volfsName);
        if (volfsDirectory == NULL) {
            err(errno, "ERROR: error trying to open '%s'", volfsName);
            };
//  fprintf(stderr, "Looking through '%s'...\n",volfsName);
        while ((volume = readdir(volfsDirectory)) != NULL) {
            if (volume->d_name[0] == '.') continue;

            volID = atoi(volume->d_name);
            printf("volid = %d: name = '%s'\n", (int)volID, volume->d_name);
            targetVolID = volID;
            break;
            };
//  printf("Done.\n");
        closedir(volfsDirectory);
    };

    if (targetDirID == 0) targetDirID = DEFAULTDIRID;

    if (targetFileNameSpecified == false) {
        strcpy(targetFileName, defaultTargetFileName);
    };
    fprintf(stderr, "%s: volID = %d, dirID = %d, fileName = '%s'.\n",
            argv[0],
            (int)targetVolID,
            (int)targetDirID,
            targetFileName);
	
    (void)Remove_VDI(targetVolID, targetDirID, targetFileName, 0, 0, 0);
    result = Create_VDI(targetVolID, targetDirID, targetFileName, 0, 0, 0, S_IRUSR | S_IWUSR);
    if (result) {
        err(errno, "%s: ERROR: error trying to open test data file '%s'", argv[0], targetFileName);
        return errno;
    };
    dataFile = OpenFork_VDI(targetVolID, targetDirID, targetFileName, 0, 0, kHFSDataForkName, 0);
    if (dataFile == -1) {
        err(errno, "%s: ERROR: error re-trying to open test data file '%s'", argv[0], targetFileName);
        return errno;
    };

    if (blockSize == 0) blockSize = MINBLOCKSIZE + (random() % (maxblocksize - MINBLOCKSIZE + 1));
	if (blockCount == 0) blockCount = MINBLOCKCOUNT + (random() % (MAXBLOCKCOUNT - MINBLOCKCOUNT + 1));
	fileSize = blockCount * blockSize;
    fprintf(stderr, "%s: testing %ld blocks of size %ld (total = %qd)...\n",
            argv[0],
    		blockCount,
    		blockSize,
    		fileSize);
	
	/* Prepare a random order to write the blocks:
	   Put all the number 0..blockCount in a row, then shuffle their order,
	   making sure each block is still targeted exactly once:
	 */
	for (i = 0; i < blockCount; ++i) {
		targetBlock[i] = i;
	};
	for (i = blockCount-1; i > 0; --i) {
		j = random() % (i + 1);
		t = targetBlock[i];
		targetBlock[i] = targetBlock[j];
		targetBlock[j] = t;
	};
#if 0
	fprintf(stderr, "%s: writing blocks ", argv[0]);
	for (i = 0; i < blockCount; ++i) {
		fprintf(stderr, "%ld", targetBlock[i]);
		if (i < blockCount - 1) {
			fprintf(stderr, ", ");
		};
	};
	fprintf(stderr, "...\n");
#endif
	
	/* Write the blocks in the chosen order: */
	for (i = 0; i < blockCount; ++i) {
		result = WriteBlock(dataFile, targetBlock[i], blockSize);
		if (result) {
            err(result, "%s: ERROR: error writing block %2d.", argv[0], targetBlock[i]);
	        return (result);
		};
	};

    (void)close(dataFile);
	
    dataFile = OpenFork_VDI(targetVolID, targetDirID, targetFileName, 0, 0, kHFSDataForkName, 0);
    if (dataFile == -1) {
        err(errno, "%s: ERROR: error trying to re-open test data file '%s'", argv[0], targetFileName);
        return errno;
    };
	blockCount = (fileSize + (VERIFYBLOCKSIZE - 1)) / VERIFYBLOCKSIZE;

    fprintf(stderr, "%s: Verifying data just written...\n", argv[0]);

	for (i = 0; i < blockCount - 1; ++i) {
		result = VerifyBlock(dataFile, i, VERIFYBLOCKSIZE, fileSize);
        if (result) {
            err(result, "%s: ERROR: error verifying block %d.", argv[0], i);
            return (result);
        };
	};
	
    (void)close(dataFile);

//  [pool release];
    exit(0);       // ensure the process exit status is returned
    return 0;      // ...and make main fit the ANSI spec.
}
