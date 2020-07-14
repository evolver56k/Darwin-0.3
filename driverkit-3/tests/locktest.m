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
 * locktest.c - open device with specified access and locking, wait for user
 * action before closing.
 */

#import <driverkit/IODevice.h>
#if	IODEVICE_LOCKS

#import <bsd/sys/types.h> 
#import <driverkit/IOClient.h>
#import <ansi/stdio.h>
#import <bsd/libc.h>
#import "defaults.h"
#import "buflib.h"

void usage(char **argv);
void exit(int exitcode);
char *gets(char *s);

int main(int argc, char **argv) {

	char *hostname=HOST_DEFAULT;
	char *devname=DEVICE_DEFAULT;
	int arg;
	char c[40];
	id idClient;
	IOReturn rtn;
	int intentions;
	char outstr[100];
	
	if(argc < 2)
		usage(argv);
	
	/*
	 * Get standard defaults from environment or defaults.h
	 */
	get_default_t("hostname", &hostname, HOST_DEFAULT);
	get_default_t("devname", &devname, DEVICE_DEFAULT);
	
	switch(argv[1][0]) {
	    case 'r':	
	    	intentions = IO_INT_READ;
		break;
	    case 'w':	
	    	intentions = IO_INT_WRITE;
		break;
	    case 'a':	
	    	intentions = IO_INT_READ|IO_INT_WRITE;
		break;
	    default:
	    	usage(argv);
	}
	for(arg=2; arg<argc; arg++) {
		char c;
		
		c = argv[arg][0];
		switch(c) {
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    case 'd':
		    	devname = &argv[arg][2];
			break;
		    case 'l':
			switch(argv[arg][2]) {
			    case 'r':
			    	intentions |= IO_INT_LOCK_READ;
			    	break;
			    case 'w':
			    	intentions |= IO_INT_LOCK_WRITE;
			    	break;
			    case 'a':
			    	intentions |= 
				   (IO_INT_LOCK_READ | IO_INT_LOCK_WRITE);
			    	break;
			    default:
			    	usage(argv);
			}
			break;
		    case 'W':
		    	intentions |= IO_INT_WAIT;
			break;
		    default:
		    	usage(argv);
		}
	}
	rtn = [IOClient clientOpen:devname 
		hostName:hostname
		intentions:intentions
		idp:&idClient];
	if(rtn) {
		[IOClient ioError:rtn 
			logString:"ioOpen"
			outString:outstr];
		printf(outstr);
		exit(1);
	}
	printf("\n...Device open; Hit CR to close: ");
	gets(c);
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
	printf("usage: %s r|w|a [l=r|w|a] [W(ait)] [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}

#endif	IODEVICE_LOCKS



