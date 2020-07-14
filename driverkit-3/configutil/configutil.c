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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * configutil - simple Config server tool.
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h>
#import <driverkit/userConfigServer.h>
#import <mach/mach.h>
#import <servers/netname.h>
#import <bsd/libc.h>
#import <mach/mach_error.h>

static void usage(char **argv);


typedef enum { ACT_REGISTER, 
	ACT_DELETE, 
	ACT_RESCAN, 
	ACT_CONFIG,
	ACT_DELETE_BY_DEV,
	ACT_NONE
} action_t;

int main(int argc, char **argv)
{
	port_name_t configPort;
	action_t action = ACT_NONE;
	int optarg = -1;
	int arg;
	kern_return_t krtn;
	char *hostname = "";
	IOConfigReturn crtn;
	IODeviceType dev_type;
	IOSlotId slot_id;
	
	if(argc < 2)
		usage(argv);
	switch(argv[1][0]) {
	    case 'r':
		action = ACT_REGISTER;
		optarg = 2;
		break;
		
	    case 'd':
	    	action = ACT_DELETE;
		optarg = 2;
		break;
		
	    case 's':
	    	action = ACT_RESCAN;
		optarg = 2;
		break;

	    case 'c':
	    	action = ACT_CONFIG;
		if(argc < 4)
			usage(argv);
		slot_id = atoh(argv[2]);
		dev_type = atoh(argv[3]);
		optarg = 4;
		break;
				
	    case 't':
	    	action = ACT_DELETE_BY_DEV;
		if(argc < 4)
			usage(argv);
		slot_id = atoh(argv[2]);
		dev_type = atoh(argv[3]);
		optarg = 4;
		break;
				
	    default:
	    	usage(argv);
	}
	
	for(arg=optarg; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    default:
		    	usage(argv);
		}
	}
	
	/*
	 * Connect to the server.
	 */
	if((krtn = netname_look_up(name_server_port, 
	    hostname,
	    CONFIG_SERVER_NAME,
	    &configPort)) != KERN_SUCCESS) {
		printf("Couldn't connect to Config server\n");
		mach_error("netname_loop_up", krtn);
		exit(1);
	}
	
	
	/* 
	 * What do we want to do...?
	 */
	switch(action) {
	    case ACT_REGISTER:
	    	printf("...unsupported\n");
		break;
	    case ACT_DELETE:
	    	printf("...unsupported\n");
		break;
	    case ACT_RESCAN:
	    	crtn = IORescanDriver(configPort);
		if(crtn)
			printf("IORescanDriver() returned %d\n", crtn);
		else
			printf("...IORescanDriver executed\n");
		break;
		
	    case ACT_CONFIG:
	    	crtn = IOConfigDevice(configPort, 
			slot_id,
			dev_type);
		if(crtn)
			printf("IOConfigDevice() returned %d\n", crtn);
		else
			printf("...IOConfigDevice executed\n");
		break;
	    case ACT_DELETE_BY_DEV:
	    	crtn = IODeleteDeviceByType(configPort, 
			slot_id,
			dev_type);
		if(crtn)
			printf("IODeleteDeviceByType() returned %d\n", crtn);
		else
			printf("...IODeleteDeviceByType executed\n");
		break;
	    default:
	    	printf("bogus bogality\n");
		exit(1);
	}
	
	return(0);
}

static void usage(char **argv)
{
	printf("Usage: \n");
	printf("\t%s r(egister) [h=hostname]\n", argv[0]);
	printf("\t%s d(elete) [h=hostname]\n", argv[0]);
	printf("\t%s s(can) [h=hostname]\n", argv[0]);
	printf("\t%s c(onfig) slot_id dev_type [h=hostname]\n", argv[0]);
	printf("\t%s t(delete by dev_type) slot_id dev_type [h=hostname]\n",
		argv[0]);
	exit(1);
}

