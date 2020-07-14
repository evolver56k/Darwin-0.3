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
 * execute open:, wait for user action to close.
 */

#import <bsd/sys/types.h> 
#import <driverkit/IOClient.h>
#import <ansi/stdio.h>
#import <bsd/libc.h>
#import "defaults.h"
#import "buflib.h"

void usage(char **argv);
void exit(int exitcode);

int main(int argc, char **argv) {

	char *hostname=HOST_DEFAULT;
	char *devname=DEVICE_DEFAULT;
	int arg;
	char c;
	id idClient;
	IOReturn rtn;
	char outstr[100];
	
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
	
	rtn = [IOClient clientOpen:devname 
		hostName:hostname
		intentions:IO_INT_READ
		idp:&idClient];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"clientOpen"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
	printf("...%s open. Hit CR to close: ", devname);
	gets(outstr);
	rtn = [idClient ioClose];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"ioClose"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}




