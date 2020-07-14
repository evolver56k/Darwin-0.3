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
#include <stdio.h>
#include <stdlib.h>
#include <mach-o/loader.h>


char *progname;
static FILE *machoF;
static FILE *xcoffF;

static struct mach_header mhead;

typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned long UInt32;

#define SWAPS(W)	(W)
#define SWAPL(L)	(L)

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

#define kTextName	".text"
#define kDataName	".data"
#define kBSSName	".bss"

struct header {
	XFileHeader file;
	XOptHeader opt;
	XSection text;
	XSection data;
	XSection BSS;
};

main(int argc, char **argv)
{
    struct header xHead;
    FILE *fp;

    if (argc == 1) {
        read(0,&xHead,sizeof(xHead));
    } else {
        fp = fopen(argv[1],"rb");
        fread(&xHead,sizeof(xHead),1,fp);
        fclose(fp);
    }

    printf("magic number:  0x%x\n",xHead.file.magic);
    printf("nsections:     0x%x\n",xHead.file.nSections);
    printf("time and date: 0x%x\n",xHead.file.timeAndDate);
    printf("symPtr:        0x%x\n",xHead.file.symPtr);
    printf("nSyms:         0x%x\n",xHead.file.nSyms);
    printf("optHeaderSize: 0x%x\n",xHead.file.optHeaderSize);
    printf("flags:         0x%x\n",xHead.file.flags);

    printf("\noptional header:\n");

    printf("magic number:  0x%x\n",xHead.opt.magic);
    printf("version:       0x%x\n",xHead.opt.version);
    printf("text size:     0x%x\n",xHead.opt.textSize);
    printf("data size:     0x%x\n",xHead.opt.dataSize);
    printf("BSS size:      0x%x\n",xHead.opt.BSSSize);
    printf("entryPoint:    0x%x\n",xHead.opt.entryPoint);
    printf("text start:    0x%x\n",xHead.opt.textStart);
    printf("data start:    0x%x\n",xHead.opt.dataStart);
    printf("toc:           0x%x\n",xHead.opt.toc);

    printf("\nsection:\n"); 
    printf("name:          %-8s %-8s %-8s\n",xHead.text.name,xHead.data.name,xHead.BSS.name);
    printf("pAddr:         %-8x %-8x %-8x\n",xHead.text.pAddr,xHead.data.pAddr,xHead.BSS.pAddr);
    printf("vAddr:         %-8x %-8x %-8x\n",xHead.text.vAddr,xHead.data.vAddr,xHead.BSS.vAddr);
    printf("size:          %-8x %-8x %-8x\n",xHead.text.size,xHead.data.size,xHead.BSS.size);
    printf("sectFileOff:   %-8x %-8x %-8x\n",
        xHead.text.sectionFileOffset,xHead.data.sectionFileOffset,xHead.BSS.sectionFileOffset);
    printf("relocFileOff:  %-8x %-8x %-8x\n",
        xHead.text.relocationsFileOffset,xHead.data.relocationsFileOffset,xHead.BSS.relocationsFileOffset);
    printf("lineNFileOff:  %-8x %-8x %-8x\n",
        xHead.text.lineNumbersFileOffset,xHead.data.lineNumbersFileOffset,xHead.BSS.lineNumbersFileOffset);
    printf("nRelocs:       %-8x %-8x %-8x\n",
        xHead.text.nRelocations,xHead.data.nRelocations,xHead.BSS.nRelocations);
    printf("nLineNums:     %-8x %-8x %-8x\n",
        xHead.text.nLineNumbers,xHead.data.nLineNumbers,xHead.BSS.nLineNumbers);
    printf("flags:         %-8x %-8x %-8x\n",xHead.text.flags,xHead.data.flags,xHead.BSS.flags);

}
 
