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
 * execute query:
 */

#import <bsd/sys/types.h> 
#import <ansi/stdio.h>
#import <bsd/libc.h>
#import "defaults.h"
#import "buflib.h"
#import <remote/NXConnection.h>
#import <driverkit/IODisk.h>

void usage(char **argv);
void exit(int exitcode);

int main(int argc, char **argv) {

	char *hostname=HOST_DEFAULT;
	char *devname=DEVICE_DEFAULT;
	int arg;
	char c;
	id targetId;
	u_int queryData;
	u_int blockSize;
	u_int devSize;
	
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
	
#if	IOSERVER
	rtn = [DiskClient clientOpen:devname 
		hostName:hostname
		intentions:IO_INT_READ
		idp:&targetId];
	if(rtn) {
		[DiskClient ioError:rtn 
			  logString:"clientOpen"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
#else	IOSERVER
	/*
	 * RO way...
	 */
	targetId = [NXConnection connectToName:devname
					 onHost:hostname];
	if(targetId == nil) {
		printf("connectToName:%s failed\n", devname);
		exit(1);
	}
#endif	IOSERVER
	queryData = [targetId queryFlags];
	printf("   queryData = 0x%X\n", queryData);
	if(queryData & DQF_READABLE)
		printf("\tDQF_READABLE\n");
	if(queryData & DQF_WRITEABLE)
		printf("\tDQF_WRITABLE\n");
	if(queryData & DQF_RAND_ACC)
		printf("\tDQF_RAND_ACC\n");
        if(queryData & DQF_CAN_QUEUE)
		printf("\tDQF_CAN_QUEUE\n");
#if	IODEVICE_LOCKS
        if(queryData & DQF_CAN_RD_LOCK)
		printf("\tDQF_CAN_RD_LOCK\n");
        if(queryData & DQF_CAN_WRT_LOCK)
		printf("\tDQF_CAN_WRT_LOCK\n");
        if(queryData & DQF_IS_RD_LOCK)
 		printf("\tDQF_IS_RD_LOCK\n");
        if(queryData & DQF_IS_WRT_LOCK)
		printf("\tDQF_IS_WRT_LOCK\n");
        if(queryData & DQF_EXCLUSIVE)
		printf("\tDQF_EXCLUSIVE\n");
#endif	IODEVICE_LOCKS

	blockSize = [targetId blockSize];
	devSize   = [targetId diskSize];
	printf("   block_size = 0x%X   dev_size = %d\n", 
		blockSize, devSize);
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}




