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
 * slabel.m
 */

#import <bsd/sys/types.h> 
#import <remote/NXConnection.h>
#import <driverkit/IODiskPartition.h>
#import <ansi/stdio.h>
#import <bsd/libc.h>
#import "defaults.h"
#import "buflib.h"

void usage(char **argv);
void exit(int exitcode);

char *label_name = (char *)0;
int label_version = 0;

int main(int argc, char **argv) {

	char *hostname=HOST_DEFAULT;
	char *devname=DEVICE_DEFAULT;
	int arg;
	char c;
	id targetId;
	IOReturn rtn;
	char outstr[100];
	struct disk_label label;
	
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
		    case 'n':
		    	label_name = &argv[arg][2];
			break;
		    case 'v':
		    	label_version = atoi(&argv[arg][2]);
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
	
	/*
	 * First read the existing label.
	 */
	rtn = [targetId readLabel:&label];
	if(rtn) {
		sprintf(outstr, "Error on readLabel: %s\n",
			[targetId stringFromReturn:rtn]);
		printf(outstr);
		exit(1);
	}

	/*
	 * Anything to change?
	 */
	if(label_name)
		strncpy(label.dl_label, label_name, MAXLBLLEN);
	if(label_version)
		label.dl_version = label_version;
		
	/*
	 * write new label.
	 */
	rtn = [targetId writeLabel:&label];
	if(rtn) {
		sprintf(outstr, "Error on writeLabel: %s\n",
			[targetId stringFromReturn:rtn]);
		printf(outstr);
		exit(1);
	}

	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\th=hostname\n");
	printf("\td=devname\n");
	printf("\tn=new_label_name\n");
	printf("\tv=new_version_num\n");
	exit(1);
}




