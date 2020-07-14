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
/* pickdisk -- lists disks connected to the system	*/
/* Matt Watson										*/
/* 18 Jan 1993										*/
// teflonified -pmb- 970423
// added code to check for -mach2id switch (actaully, anything starting with '-')
// compares 3rd arg to machdev, will retun list of matches, with bus and scsi id
// "pickdisk -mach2id /dev/rsd0h" will return "/dev/rsd0a: scsi ext 1"
// "pickdisk -mach2id /dev"	  will return a list of all drives.
// "pickdisk -mach2id /dev/rsd"   will return a list of all scsi drives.
// "pickdisk -mach2id /dev/rhd"   will return a list of all ide drives.

#import <bsd/dev/disk.h>
#import <bsd/dev/ata_extern.h>
#import <bsd/dev/scsireg.h>
#import <bsd/sys/types.h>
#import <ctype.h>
#import <errno.h>
#import <libc.h>
#import <stdio.h>
#import <sys/file.h>

#define kIDE -1

#define RAWDEV "/dev/r"
#define IDE "hd%d%c"
#define SCSI "sd%d%c"
#define MIN_SIZE 200
#define MEG 1048576
#define BLOCK_SIZE 512
#if defined(i386)
#define NO_DISK "\tMac OS X Server cannot be installed on any disks.\n" \
"\tYou must have a 512 byte/sector disk with at " \
"least 250MB of free space.\n"
#else
#define NO_DISK "\tMac OS X Server cannot be installed on any disks.\n" \
"\tYou must have a disk with at least 250MB of free space.\n"
#endif
     
#define VENDOR_SIZE sizeof(((inquiry_reply_t *)NULL)->ir_vendorid)
#define PRODUCT_SIZE sizeof(((inquiry_reply_t *)NULL)->ir_productid)

static int match = 0;
static int showBoot = 0;

void fixup(char *dest, char *source, int len) {
    char *cp;
    strncpy(dest, source, len);
    cp = dest + strlen(dest);
    while (isspace(*(--cp)));
    *(++cp) = '\0';
}

int DriveName(int fd, int count, int target, inquiry_reply_t *reply, char* mach_dev, int scsibus, char* ideDev) {

    static char driveStr[12];
    static int scsinum = 0;
    int scsidevice = 0, BigEnough = 0;
    long blocksize, num;
    double size;

if (!reply) {
    ideDriveInfo_t info;
    if (ioctl(fd, IDEDIOCINFO, &info) < 0) {
        perror("ioctl(IDEDIOCINFO)");
        return 0;
    }
    blocksize = info.bytes_per_sector;
    num = info.total_sectors;
    size = (double) blocksize * (double) num / MEG;
    BigEnough = (size >= MIN_SIZE);

    if (BigEnough)
    {
        if (mach_dev)
        {
            if (!strncmp(ideDev, mach_dev, strlen(mach_dev)-1))
            {
                printf("%s: ", ideDev);
                printf("ide bus ");
                printf("%d\n", target);	// ide id
            }
         }
         else
         {
            if (!match)
            {
                if (!showBoot) {
                    printf("%d.  ", count);
                }
                printf("IDE Disk %d (Type %d) - %ld MB\n",
                        target, info.type, (long) size);
                if (showBoot) {
                    return 1;
                }
            }
         }
     }
} else
{
    if (reply->ir_devicetype == DEVTYPE_DISK) {

        sprintf(driveStr, RAWDEV SCSI, scsinum, 'h');
        if ((scsidevice = open(driveStr, O_RDONLY, 0)) < 0) {
            perror("open(scsidevice)");
            return 0;
        }
        sprintf(driveStr, RAWDEV SCSI, scsinum, 'a');
        if (ioctl(scsidevice, DKIOCBLKSIZE, &blocksize) < 0) {
            perror("ioctl(DKIOCBLKSIZE)");
            return 0;
        }
#if defined(i386)
        if (blocksize != BLOCK_SIZE) {
            return 0;
        }
#endif
        if (ioctl(scsidevice, DKIOCNUMBLKS, &num) < 0) {
            perror("ioctl(DKIOCNUMBLKS)");
            return 0;
        }
        close(scsidevice);
        size = (double) blocksize * (double) num / MEG;
        BigEnough = (size >= MIN_SIZE);
        if (BigEnough) {
            char vendor[VENDOR_SIZE + 1] = {0};
            char product[PRODUCT_SIZE + 1] = {0};
            fixup(vendor, reply->ir_vendorid, VENDOR_SIZE);
            fixup(product, reply->ir_productid, PRODUCT_SIZE);
            if (mach_dev)
            {
                if (!strncmp(driveStr, mach_dev, strlen(mach_dev)-1))
                {
                    printf("%s: ", driveStr);
                    if (scsibus == 0)
                        printf("scsi ext ");
                    else
                        printf("scsi int ");
                    printf("%d\n", target);	// scsi id
                }
            }
            else
            {
                if (!match) {
                    if (!showBoot) {
                        printf("%d.  ", count);
                    }
#if defined(i386)
                    printf("SCSI Disk at scsi bus %d id %d - %ld MB - (%s %s)\n",
			scsibus, target, (long) size, vendor, product);
#else
                    printf("SCSI Disk at %s scsi bus %d id %d - %ld MB - (%s %s)\n",
                           scsibus?"internal":"external", scsibus, target, (long) size, vendor, product);
#endif                    
                    if (showBoot) {
                        return 1;
                    }
                }
            }
        }
    }
    scsinum++;
}
return BigEnough;
}

int do_inquiry(int device, int target, inquiry_reply_t *reply) {
    int tries = 5;
    int successful;
    scsi_req_t	request = {0};
    esense_reply_t *esense = &(request.sr_esense);
    scsi_adr_t sa;

    sa.sa_lun = 0;
    sa.sa_target = target;
    if (ioctl(device, SGIOCSTL, &sa) < 0) {
        fprintf(stderr, "For target %d ", target);
        perror("ioctl(SGIOCSTL)");
        return(0);
    }
    request.sr_cdb.cdb_opcode = C6OP_INQUIRY;
    request.sr_dma_dir = SR_DMA_RD;
    request.sr_addr = (caddr_t)reply;
    request.sr_dma_max = sizeof(inquiry_reply_t);
    request.sr_cdb.cdb_c6.c6_len = sizeof(inquiry_reply_t);
    request.sr_ioto = 30;
    do {
        successful = 0;
        if (ioctl(device, SGIOCREQ, &request) < 0) {
            perror("ioctl(SGIOCREQ)");
            exit(-1);
        }
        if (request.sr_io_status != SR_IOST_SELTO) {
            if ((request.sr_io_status != SR_IOST_GOOD) ||
                (request.sr_scsi_status != STAT_GOOD)) {
                fprintf(stderr,
                        "io_status = %d, scsi_status = %d, sense key = %d\n",
                        request.sr_io_status, request.sr_scsi_status,
                        esense->er_sensekey);
                tries--;
            } else {
                successful = 1;
                break;
            }
        } else break;
    } while (tries);

    if (!tries) {
        fprintf(stderr, "Too many tries...\n");
        exit(-1);
    }

    return successful;
}

int main(int argc, char **argv) {

    int target, device, scsicount = 0, count = 1, x=0;
    char scsi_generic[10];
    char *mach_dev = 0L;
    int num_targets;
    
    if (argc > 3) {
        usage:
        fprintf(stderr, "usage: %s [disknumber]\nor\n       %s -r [mach_device]\n", argv[0], argv[0]);
        exit(-1);
    }

    if (argc==3) {
	mach_dev = argv[2];
        showBoot = 0;
        match = 0;
    }
    else
	if (argc == 2) {
            if (isdigit(*argv[1])) {
                match = atoi(argv[1]);
                if (match < 0) goto usage;
                if (match == 0) {
                    showBoot = 1;
                }
            } else goto usage;
        }
    else
        match = 0;

//=================================================================
// Check IDE.
//=================================================================
    for (target = 0; target < MAX_IDE_DRIVES; target++) {
        char deviceStr[12];
        sprintf(deviceStr, RAWDEV IDE, target, 'h');
        if ((device = open(deviceStr, O_RDONLY | O_NDELAY, 0)) >= 0) {
            sprintf(deviceStr, RAWDEV IDE, target, 'a');
            if (DriveName(device, count, target, NULL, mach_dev, kIDE, deviceStr)) {
                if (match == count++) {
                    printf(IDE, target, 'a');
                    close(device);
                    exit(0);
                }
                if (showBoot) {
                    exit(1);
                }
            }
            close(device);
        }
    }

//=================================================================
// Check SCSI. Shoulda broken this out into a function.
//=================================================================

    for (x=0; x<4; x++)
    {
        sprintf(scsi_generic, "/dev/sg%d", x);
        if ((device = open(scsi_generic, O_RDWR, 0)) >= 0)
        {
            if (ioctl(device, SGIOCENAS) < 0) {
                perror("ioctl(SGIOCENAS)");
            }

            if (ioctl(device, SGIOCNUMTARGS, &num_targets) < 0) {
                perror("ioctl(SGIOCNUMTARGS)");
                num_targets = SCSI_NTARGETS;
            }

            for (target = 0; target < num_targets; target++) {
                inquiry_reply_t reply = {0};
                if (do_inquiry(device, target, &reply) && (
                    reply.ir_devicetype == DEVTYPE_DISK ||
                    reply.ir_devicetype == DEVTYPE_WORM ||
                    reply.ir_devicetype == DEVTYPE_OPTICAL ||
                    reply.ir_devicetype == DEVTYPE_CDROM )) {
                    if (DriveName(device, count, target, &reply, mach_dev, x, "scsi" )) {
                        if (match == count++) {
                            printf(SCSI, scsicount, 'a');
                            close(device);
                            exit(0);
                        }
                        if (showBoot) {
                            exit(1);
                        }
                    }
                    scsicount++;
                }
            }
            close(device);
        }
    }
    

//=================================================================
    if (count == 1) {
        printf(NO_DISK);
        exit(-1);
    }
    return (count - 1);
}

