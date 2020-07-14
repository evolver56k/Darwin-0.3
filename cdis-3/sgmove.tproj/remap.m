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
#import <stdio.h>
#import <libc.h>
#import <fcntl.h>
#import <dev/scsireg.h>

void error(char *);

int main(int argc, char **argv)
{
    int fd;
    int sc_num;
    int sg_num;
    
    char path[256];

    if(argc < 3) {
        error(argv[0]);
        exit(-1);
    }
    
    sscanf(argv[1],"sg%i",&sg_num);
    sscanf(argv[2],"sc%i",&sc_num);
    
    sprintf(path,"/dev/sg%i",sg_num);
    fd = open(path,O_RDWR,O_NDELAY);

    if(ioctl(fd, SGIOCCNTR, &sc_num) < 0) {
        perror("SGIOCCNTR");
        return(0);
    }
    close(fd);

    printf("sg%i now connected to sc%i\n",sg_num,sc_num);
    
    return(1);
}

void error(char *progname)
{
    fprintf(stderr, "Usage: %s sg[num] sc[num]\n", progname);
    return;
} 

