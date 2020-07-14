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
/*
 * gparms.m
 */

#import <bsd/sys/types.h> 
#import <driverkit/IODisk.h> 
#import <ansi/stdio.h>
#import <bsd/libc.h>
#import "defaults.h"
#import "buflib.h"
#import <bsd/dev/disk.h>
#import <remote/NXConnection.h>
#import <driverkit/IODisk.h>
#import <driverkit/IODiskRwDistributed.h>
#import <driverkit/IODiskPartition.h>
#import <objc/error.h>
#import <remote/NXProxy.h>

void usage(char **argv);
void exit(int exitcode);

int main(int argc, char **argv) {

	char *hostname=HOST_DEFAULT;
	char *devname=DEVICE_DEFAULT;
	int arg;
	char c;
	id targetId;
	IOReturn rtn;
	unsigned formatted, removable;
	struct disk_label label;
	unsigned blocksize, devsize;
	
	/*
	 * Get standard defaults from environment or defaults.h
	 */
	get_default_t("hostname", &hostname, HOST_DEFAULT);
	get_default_t("devname", &devname, DEVICE_DEFAULT);

	for(arg=1; arg<argc; arg++) {
		c = argv[arg][0];
		switch(c) {
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    case 'd':
		    	devname = &argv[arg][2];
			break;
		    default:
		    	usage(argv);
		}
	}
	
	targetId = [NXConnection connectToName:devname
					 onHost:hostname];
	if(targetId == nil) {
		printf("connectToName:%s failed\n", devname);
		exit(1);
	}
	removable = [targetId isRemovable];
	printf("removable : %s\n", removable ? "TRUE" : "FALSE");
	
	/*
	 * Should do readLabel before getFormatted to fault in possible 
	 * ejected disk.
	 */
	NX_DURING
	    rtn = [targetId readLabel:&label];
	NX_HANDLER
	    if(NXLocalHandler.code == NX_unknownMethodException) {
	    	/*
		 * Not an IODiskPartition. This is OK.
		 */
		rtn = IO_R_NO_LABEL;
	    }
	    else {
	    	printf("Unexpected exception (%d) on readLabel; aborting\n",
			NXLocalHandler.code);
		exit(1);
	    }
	NX_ENDHANDLER
	printf("labelValid: ");
	switch(rtn) {
	    case IO_R_SUCCESS: 
	    	printf("TRUE\n");
		break;
	    case IO_R_NO_LABEL: 
	    	printf("FALSE\n");
		break;
	    default: 
	    	printf("***Unsupported Op***\n");
		break;
	}
	
	formatted = [targetId isFormatted];
	printf("formatted : %s\n", formatted ? "TRUE" : "FALSE");
	blocksize = [targetId blockSize];
	devsize = [targetId diskSize];
	printf("blocksize : %d\n", blocksize);
	printf("devsize   : %d\n", devsize);
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}




