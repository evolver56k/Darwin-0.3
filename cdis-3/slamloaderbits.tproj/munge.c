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
//
// CopySecondaryLoader
//
//
// Derived from code written by Alan Mimms under the MacOS for Copland and Rhapsody.
// Ported by Eryk Vershen <eryk@apple.com> in July 1997.
//

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

// Defines
#define BlockMoveData(s,d,l)	memcpy(d,s,l)

// Constants

// Typedefs
typedef unsigned long UInt32;
typedef unsigned short UInt16;
typedef unsigned char UInt8;

typedef struct {
	/* File header */
	UInt16 magic;
#define kFileMagic			0x1DF
	UInt16 nSections;
	UInt32 timeAndDate;
	UInt32 symPtr;
	UInt32 nSyms;
	UInt16 optHeaderSize;
	UInt16 flags;
} XFileHeader;

typedef struct {
	/* Optional header */
	UInt16 magic;
#define kOptHeaderMagic		0x10B
	UInt16 version;
	UInt32 textSize;
	UInt32 dataSize;
	UInt32 BSSSize;
	UInt32 entryPoint;
	UInt32 textStart;
	UInt32 dataStart;
	UInt32 toc;
	UInt16 snEntry;
	UInt16 snText;
	UInt16 snData;
	UInt16 snTOC;
	UInt16 snLoader;
	UInt16 snBSS;
	UInt8 filler[28];
} XOptHeader;

typedef struct {
	char name[8];
	UInt32 pAddr;
	UInt32 vAddr;
	UInt32 size;
	UInt32 sectionFileOffset;
	UInt32 relocationsFileOffset;
	UInt32 lineNumbersFileOffset;
	UInt16 nRelocations;
	UInt16 nLineNumbers;
	UInt32 flags;
} XSection;

enum SectionNumbers {
	kTextSN = 1,
	kDataSN,
	kBSSSN
};

const char kTextName[] = ".text";
const char kDataName[] = ".data";
const char kBSSName[] = ".bss";

// Variables
char *program;
extern int errno;
extern int sys_nerr;
extern const char * const sys_errlist[];

char *device_arg;
int partition_arg;
char *loader_arg;

UInt32 lowestAddress = ~0ul;
UInt32 highestAddress = 0ul;

/*
 * error ( format_string, item )
 *
 *	Print a message on standard error.
 */
error(value, fmt, x1, x2)
int value;
char *fmt;
{
    fprintf(stderr, "%s: ", program);
    fprintf(stderr, fmt, x1, x2);

    if (value > 0 && value < sys_nerr) {
	fprintf(stderr, "  (%s)\n", sys_errlist[value]);
    } else {
	fprintf(stderr, "\n");
    }
}

/*
 * fatal ( value, format_string, item )
 *
 *	Print a message on standard error and exit with value.
 *	Values in the range of system error numbers will add
 *	the perror(3) message.
 */
fatal(value, fmt, x1, x2)
int value;
char *fmt;
{
    fprintf(stderr, "%s: ", program);
    fprintf(stderr, fmt, x1, x2);

    if (value > 0 && value < sys_nerr) {
	fprintf(stderr, "  (%s)\n", sys_errlist[value]);
    } else {
	fprintf(stderr, "\n");
    }

    exit(value);
}

/*
 * usage ( kind )
 *
 *	Bad usage is a fatal error.
 */
usage(kind)
char *kind;
{
    error(-1, "(usage: %s)", kind);
    printf("%s [-p #] [-f loader] device\n", program);
    printf("\t-p         number of partition to load\n");
    printf("\t-f         XCOFF file containing loader\n");
    printf("\tdevice     disk containing partition\n");
}

void
accumulateSectionSpans (XSection *sectionP)
{
    if (sectionP->vAddr < lowestAddress) {
	lowestAddress = sectionP->vAddr;
    }
    if (sectionP->vAddr + sectionP->size  > highestAddress) {
	highestAddress = sectionP->vAddr + sectionP->size;
    }
}


char *
convertXCOFFImage (void *xImageP, UInt32 *entryP, UInt32 *loadBaseP, UInt32 *loadSizeP)
{
	XFileHeader *fileP = (XFileHeader *) xImageP;
	XOptHeader *optP = (XOptHeader *) (fileP + 1);
	XSection *sectionsP = (XSection *) (optP + 1);
	XSection *sectionP;
	char * partImageH;
	UInt32 partImageSize;
	const UInt32 kPageSize = 4096;

	if (fileP->magic != kFileMagic || optP->magic != kOptHeaderMagic) {
	    fatal(-1, "Bad SecondaryLoader XCOFF");
	}

	accumulateSectionSpans (&sectionsP[optP->snText - 1]);
	accumulateSectionSpans (&sectionsP[optP->snData - 1]);
	accumulateSectionSpans (&sectionsP[optP->snBSS  - 1]);
	
	// Round to page multiples (OF 1.0.5 bug)
	lowestAddress &= -kPageSize;
	highestAddress = (highestAddress + kPageSize - 1) & -kPageSize;
	partImageSize = highestAddress - lowestAddress;

	partImageH = (char *)malloc(partImageSize);
	if (partImageH == 0) {
	    fatal("Can't allocate memory for SecondaryLoader image", partImageSize);
	}
	
	// Copy TEXT section into partition image area
	sectionP = &sectionsP[optP->snText - 1];
	BlockMoveData ((UInt8 *) xImageP + sectionP->sectionFileOffset,
					partImageH + sectionP->vAddr - lowestAddress,
					sectionP->size);

	// Copy DATA section into partition image area
	sectionP = &sectionsP[optP->snData - 1];
	BlockMoveData ((UInt8 *) xImageP + sectionP->sectionFileOffset,
					partImageH + sectionP->vAddr - lowestAddress,
					sectionP->size);

	// Zero BSS section in partition image area
	sectionP = &sectionsP[optP->snBSS - 1];
	memset (partImageH + sectionP->vAddr - lowestAddress, 0, sectionP->size);

	*loadBaseP = lowestAddress;
	*loadSizeP = partImageSize;
	*entryP = *(UInt32 *) (partImageH + optP->entryPoint - lowestAddress);		// Dereference transition vector[0]
	return partImageH;
}

char *
getSecondaryLoaderImage(char *filename)
{
    int fd;
    struct stat buf;
    unsigned size;
    char *image;
    
    if ((fd = open(filename, O_RDONLY, 0)) < 0) {
	fatal(errno, "Couldn't open file '%s' for reading", filename);
    }

    if (fstat(fd, &buf) < 0) {
	fatal(errno, "Couldn't stat file '%s'", filename);
    }

    size = (unsigned) buf.st_size;
    if ((image = (char *)malloc(size)) == 0) {
	fatal(errno, "Couldn't malloc image for '%s'", filename);
    }

    if (read(fd, image, size) < 0) {
	fatal(errno, "Couldn't read '%s'", filename);
    }

    close(fd);

    return image;
}

void
doit(char *device_arg, int partition_arg, char *loader_arg)
{
    UInt32 partitionEntry;
    UInt32 partitionBase;
    UInt32 partitionSize;
    char *raw_image;
    char *cooked_image;
    char command[512];
    int i;

    raw_image = getSecondaryLoaderImage(loader_arg);
    cooked_image = convertXCOFFImage(raw_image,
	    &partitionEntry, &partitionBase, &partitionSize);
    // set partition bootable by calling pdisk
    sprintf(command, "pdisk %s -makeBootable %d %d %d %d %d",
	    device_arg, partition_arg, 0, partitionSize,
	    partitionBase, partitionEntry);
    system(command);
    // cat out the cooked_image up to partitionSize
    for (i = 0; i < partitionSize; i++) {
	putchar(cooked_image[i]);
    }
}

int
main(argc, argv)
int argc;
char **argv;
{
    register int i;
    register char *s;
    char *option_error;
    int err;
    char message[100];
    int flag = 1;

    if ((program = strrchr(argv[0], '/')) != (char *)NULL) {
	program++;
    } else {
	program = argv[0];
    }

    for (i = 1; i < argc; i++) {
	s = argv[i];
	if (s[0] == '-' && s[1] != 0) {
	    /* options */
	    for (s += 1 ; *s; s++) {
		switch (*s) {

		case 'p':
		    i++;
		    if (i < argc && *argv[i] != '-') {
			partition_arg = atoi(argv[i]);
		    } else {
			usage("missing partition number");
		    }
		    break;

		case 'f':
		    i++;
		    if (i < argc && *argv[i] != '-') {
			loader_arg = argv[i];
		    } else {
			usage("missing loader filename");
		    }
		    break;

		default:
		    strcpy(message, "no such option as --");
		    message[strlen(message)-1] = *s;
		    usage(message);
		}
	    }
	} else {
	    /* Other arguments */
	    device_arg = s;
	    flag = 0;
	}
    }

    if (flag) {
	usage("no device argument");
    } else {
	doit(device_arg, partition_arg, loader_arg);
    }

    return 0;
}
