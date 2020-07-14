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
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * idemodes.c -- User level utility to print ATA disk parameters
 *             (will work with only 3.3 IDE driver patch and later)
 *
 * HISTORY
 * 14-Aug-1994	 Rakesh Dubey at NeXT
 *	Created.
 *
 */
 
#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <sys/ioctl.h>
#import <errno.h>
#import <libc.h>

/* FIXME */
#import <ata_extern.h>

void print_ide_info(ideIdentifyInfo_t *ideInfo);
void get_info_for_drive(char *name);

char *progname;

int main(int argc, char *argv[])
{
    int i;
    
    if (argc == 1)	{
	fprintf(stderr, "Usages: %s <raw_device>\n", argv[0]);
	exit(0);
    }

    progname = argv[0];
    
    for (i = 1; i < argc; i++)	{
    	get_info_for_drive(argv[i]);
    }
	
    exit(0);
}

void get_info_for_drive(char *name)
{
    int fd, status;
    ideIoReq_t ioReq;
    ideIdentifyInfo_t *ideInfo;

    if ((fd = open(name, O_RDONLY)) < 0)	{
	//fprintf(stderr, "%s: can't open %s\n", progname, name);
	perror(progname);
	exit(1);
    }

    bzero(&ioReq, sizeof(ideIoReq_t));

    ideInfo = (ideIdentifyInfo_t *) malloc(sizeof(ideIdentifyInfo_t));
    if (ideInfo == NULL)	{
	perror(progname);
	exit(2);
    }
    bzero(ideInfo, sizeof(ideIdentifyInfo_t));

    ioReq.cmd = IDE_IDENTIFY_DRIVE;
    ioReq.addr = (void *) ideInfo;

    if ((status = ioctl(fd, IDEDIOCREQ, &ioReq)) < 0)	{
	perror(progname);
	free(ideInfo);
	exit(1);
    }

    close(fd);
    
    if (status != IDER_SUCCESS)	{
	fprintf(stderr, "Command failed: error %d, status 0x%0x\n", 
		status, ioReq.status);
	free(ideInfo);
	exit(3);
    }

    print_ide_info(ideInfo);
    free(ideInfo);

}

char *byte_swap_string(char *s, int len)
{
    int i;
    char *tmp;

    tmp = (char *)malloc(len);

    for (i = 0; i < len/2; i++)	{
	tmp[2*i] = s[2*i+1];
	tmp[2*i+1] = s[2*i];
    }
    strncpy(s, tmp, len);
    s[len-1] = '\0';

    free(tmp);
    return s;
}


void print_ide_info(ideIdentifyInfo_t *ideInfo)
{
    int transfer_mode;
    unsigned int sec_cnt;
    unsigned long capacity;
    unsigned char printall;
    short *data = (short *)ideInfo;
    int i;

    printall = 0;
    
    /* Name */
    printf("Drive Name: %s\n", byte_swap_string(ideInfo->modelNumber, 40));
    printf("Firmware Revision: %s\n",
	byte_swap_string(ideInfo->firmwareRevision, 8));
    printf("Serial Number: %s\n", 
	byte_swap_string(ideInfo->serialNumber, 10));

    /* Physical (hopefully) parameters */
    printf("Cylinders: %d, heads: %d, sectors per track: %d.\n",
	ideInfo->cylinders, ideInfo->heads, ideInfo->sectorsPerTrack);

    /* What this thing can do */
    printf("Capabilities: ");
    if (ideInfo->capabilities & IDE_CAP_LBA_SUPPORTED)	{
	printf("LBA"); 
    }
    
    if (ideInfo->capabilities & IDE_CAP_IORDY_SUPPORTED) {
	printf(", IORDY"); 
    }
    if (ideInfo->capabilities & IDE_CAP_DMA_SUPPORTED)
	printf(", DMA"); 
    printf("\n");

    /* Drive buffer features */
    printf("Drive Buffer: ");
    if (ideInfo->bufferType != 0)	{
	printf("type %d, size %d sectors.\n",
		ideInfo->bufferType, ideInfo->bufferSize);
    }

    /* Transfer mode */
    printf("Data Transfer: ");
    transfer_mode = (ideInfo->pioDataTransferCyleTimingMode &
		IDE_PIO_TIMING_MODE_MASK) >> 8;
    printf("PIO Mode %d", transfer_mode); 

    if (ideInfo->capabilities & IDE_CAP_DMA_SUPPORTED) {
		int i;
	
		/* Single word DMA */
		transfer_mode = (ideInfo->dmaDataTransferCyleTimingMode &
		    IDE_DMA_TIMING_MODE_MASK) >> 8;
	    printf(", SW DMA Mode %d", transfer_mode); 

		/* Multiword DMA */
		for (i = 2; i >= 0; i--) {
			if (ideInfo->mwDma & (1 << i)) {
				printf(", MW DMA Mode %d", i);
				break;
			}
		}
		
		/* UDMA/33 */
		if (ideInfo->fieldValidity & IDE_WORD88_SUPPORTED) {
			for (i = 2; i >= 0; i--) {
				if (ideInfo->UDma & (1 << i)) {
					printf(", UDMA/33 Mode %d", i);
					break;
				}
			}
		}
    }
    printf("\n");
    	
    /* Transfer mode with IOCHRDY */
    if ((ideInfo->fieldValidity & IDE_WORDS64_TO_68_SUPPORTED) &&
    	(ideInfo->capabilities & IDE_CAP_IORDY_SUPPORTED))	{
	printf("Data Transfer (with IORDY): ");
	transfer_mode = ideInfo->fcPioDataTransferCyleTimingMode;
	if (transfer_mode & IDE_FC_PIO_MODE_5_SUPPORTED)
	    printf("PIO Mode 5 ");
	else if (transfer_mode & IDE_FC_PIO_MODE_4_SUPPORTED)
	    printf("PIO Mode 4 ");
	else if (transfer_mode & IDE_FC_PIO_MODE_3_SUPPORTED)
	    printf("PIO Mode 3 ");

	printf("\n");
    }
    
    sec_cnt = ideInfo->multipleSectors & IDE_MULTI_SECTOR_MASK;
    if (sec_cnt > 0)	{
	printf("Multiple Sector: %d sectors per transfer.\n",
	 	sec_cnt); 
    }

    /* Drive capacity */
    if (ideInfo->capabilities & IDE_CAP_LBA_SUPPORTED)	{
    	capacity = ideInfo->userAddressableSectors;
    } else {
	capacity = ideInfo->cylinders * ideInfo->heads * 
		ideInfo->sectorsPerTrack;
    }

    /*
     * Some disks that support LBA have bogus info in userAddressableSectors
     * field. 
     */
    capacity = ideInfo->cylinders * ideInfo->heads * 
	    ideInfo->sectorsPerTrack;
    
    printf("Drive Capacity: %lu sectors of 512 bytes each (total %7.2f MB).\n", 
    	capacity, (float) (capacity*512.0)/(1024*1024)); 

    /*
     * Dump all data returned by IDENTIFY DRIVE command. 
     */
    if (printall)	{
	for (i = 0; i < 256; i++)	{
	    if (i % 8 == 0)
		printf("\n%3d: ", i);
	    printf("%4x ", data[i] & 0x0ffff);
	}
	printf("\n");
    }
    
    return;
}
