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
 * simple printer object test.
 */
 
#import <Printer/printer.h>
#import <remote/NXConnection.h>
#import <remote/NXProxy.h>
#import <objc/error.h>
#import <sys/printf.h>

static void usage(char **argv)
{
	printf("usage: %s action [n=printername] [l(oop)]\n", argv[0]);
	printf("action:\n");
	printf("   p   power on\n");
	printf("   P   power off\n");
	printf("   s   get status\n");
	exit(1);
}

int main(int argc, char **argv)
{
	id printer;
	char *printername = "np0";
	char action;
	IOReturn rtn;
	npStatus status;
	int arg;	
	int loop = 0;
	
	if(argc < 2) {
		usage(argv);
	}
	for(arg=2; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'n':
		   	printername = &argv[arg][2];
			break;
		    case 'l':
		    	loop = 1;
			break;
		    default:
		    	usage(argv);
		}
	}
	action = argv[1][0];
	NX_DURING {
		printer = [NXConnection connectToName:printername];
	} NX_HANDLER {
		printf("connectToName raised %d\n", NXLocalHandler.code);
		NX_VALRETURN(1);
	} NX_ENDHANDLER;
	
	if(printer == nil) {
		printf("connectToName returned nil\n");
		exit(1);
	}
	
	/* 
	 * Set a long timeout for printer's (an NXProxy) NXConnection.
	 * FIXME - isn't there a more straightforward was of doing this?
	 */
	[[printer connectionForProxy] setInTimeout:0x7fffffff];
	[[printer connectionForProxy] setOutTimeout:0x7fffffff];
	
	NX_DURING {
	        do {
			switch(action) {
			    case 'p':
				rtn = [printer powerOn];
				break;
			    case 'P':
				rtn = [printer powerOff];
				break;
			    case 's':
				rtn = [printer getStatus:&status];
				if(rtn == IO_R_SUCCESS) {
					printf("status.flags = 0x%x\n", 
						status.flags);
					printf("status.retrans = %d\n",
						status.retrans);
				}
				break;
				
			    default:
				usage(argv);
			}
		} while ((rtn == IO_R_SUCCESS) && loop);
	} NX_HANDLER {
		printf("I/O raised %d\n", NXLocalHandler.code);
		NX_VALRETURN(1);
	} NX_ENDHANDLER;
	
	if(rtn) {
		printf("...cmd returned %d\n", rtn);
		exit(1);
	}
	else {
		printf("...complete\n");
	}
	exit(0);
