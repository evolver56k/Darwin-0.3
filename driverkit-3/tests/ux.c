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
 * ux - user-level XPR utility.
 *
 * HISTORY
 * 22-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h>
#import <driverkit/debuggingMsg.h>
#import <mach/mach.h>
#import <servers/netname.h>
#import <bsd/libc.h>
#import <mach/mach_error.h>

static void usage(char **argv);
static int ux_clear(port_name_t serverPort, 
	port_name_t localPort);
static int ux_setmask(port_name_t serverPort, 
	port_name_t localPort, 
	u_int bitmask);
static int ux_dump(port_name_t serverPort, 
	port_name_t localPort, 
	int numbufs);
static int ux_simplemsg(port_name_t serverPort, 
	port_name_t localPort,
	int msg_id, 
	u_int maskValue,
	u_int index, 
	char *op);

/*
 * Template for creating IODDMMsg's.
 */
static IODDMMsg msgTemplate = {

	{						// header 
		0,					// msg_unused 
		1,					// msg_simple 
		sizeof(IODDMMsg),			// msg_size 
		MSG_TYPE_NORMAL,			// msg_type 
		PORT_NULL,				// msg_local_port 
		PORT_NULL,				// msg_remote_port - TO
							//   BE FILLED IN 
		0					// msg_id - TO BE 
							//   FILLED IN
	},
	{						// argType 
		MSG_TYPE_INTEGER_32,			// msg_type_name 
		sizeof(int) * 8,			// msg_type_size 
		5,					// msg_type_number 
		1,					// msg_type_inline 
		0,					// msg_type_longform 
		0,					// msg_type_deallocate 
		0					// msg_type_unused
	},
	0,						// index
	0,						// maskValue
	0,						// status
	0,						// timestampHighInt
	0,						// timestampLowInt
	0,						// cpuNumber
	{						// stringType 
		MSG_TYPE_CHAR,				// msg_type_name 
		1,					// msg_type_size 
		IO_DDM_STRING_LENGTH,			// msg_type_number 
		1,					// msg_type_inline 
		0,					// msg_type_longform 
		0,					// msg_type_deallocate 
		0					// msg_type_unused
	},
	0						// the string 
};

typedef enum { ACT_CLEAR, ACT_DUMP, ACT_SETMASK, ACT_NONE } action_t;

int main(int argc, char **argv)
{
	port_name_t serverPort, localPort;
	action_t action = ACT_NONE;
	int numbufs = -1;
	int optarg = -1;
	char *hostname = "";
	u_int bitmask = 0;
	int rtn;
	netname_name_t nn_portName, nn_hostName;
	kern_return_t krtn;
	
	if(argc < 3)
		usage(argv);
	switch(argv[2][0]) {
	    case 'd':
		action = ACT_DUMP;
		optarg = 3;
		if(argc > 3) {
			switch(argv[3][0]) {
			    case 'b':
			    	numbufs = atoi(&argv[3][2]);
				optarg++;
				break;
			    default:
			    	break;
			}
		}
		break;
	    case 'c':
	    	optarg = 3;
	    	action = ACT_CLEAR;
		break;
		
	    case 's':
	    	optarg = 4;
	    	action = ACT_SETMASK;
		bitmask = atoi(argv[3]);
		break;
		
	    default:
	    	usage(argv);
	}
	
	while(optarg < argc) {
		switch(argv[optarg][0]) {
		    case 'h':
		    	hostname = argv[optarg];
			break;
		    default:
		    	usage(argv);
		}
	}
	
	/*
	 * Connect to the server.
	 */
	strcpy(nn_portName, argv[1]);
	strcpy(nn_hostName, hostname);
	if(netname_look_up(name_server_port, 
	    nn_hostName, 
	    nn_portName,
	    &serverPort) != KERN_SUCCESS) {
		printf("Couldn't connect to device %s\n", argv[1]);
		exit(1);
	}

	krtn = port_allocate(task_self(), &localPort);
	if(krtn) {
		mach_error("port_allocate", krtn);
		exit(1);
	}
	
	/*
	 * Lock the xpr state.
	 */
	if(ux_simplemsg(serverPort, 
	    localPort, 
	    IO_LOCK_DDM_MSG, 
	    0, 0,
	    "Lock"))
		exit(1);
		
	/* 
	 * What do we want to do...?
	 */
	switch(action) {
	    case ACT_CLEAR:
	    	rtn = ux_clear(serverPort, localPort);
		break;
	    case ACT_SETMASK:
	    	rtn = ux_setmask(serverPort, localPort, bitmask);
		break;
	    case ACT_DUMP:
	    	rtn = ux_dump(serverPort, localPort, numbufs);
		break;
	    default:
	    	printf("bogus bogality\n");
		exit(1);
	}
	
	/*
         * Unlock the xpr state.
	 */
	ux_simplemsg(serverPort, 
		localPort, 
		IO_UNLOCK_DDM_MSG, 
		0, 0,
		"Unlock");
	return(rtn);
}

static void usage(char **argv)
{
	printf("Usage: \n");
	printf("\t%s server_name d(ump) [b=buffer_count] [h=hostname]\n",
		argv[0]);
	printf("\t%s server_name s(etmask) bitmask [h=hostname]\n", argv[0]);
	printf("\t%s server_name c(lear) [h=hostname]\n", argv[0]);
	exit(1);
}

static int ux_simplemsg(port_name_t serverPort, 
	port_name_t localPort,
	int msg_id, 
	u_int maskValue,
	u_int index, 
	char *op)
{
	IODDMMsg msg;
	kern_return_t krtn;
	
	msg = msgTemplate;
	msg.header.msg_remote_port = serverPort;
	msg.header.msg_local_port  = localPort;
	msg.header.msg_id = msg_id;
	msg.index = index;
	msg.maskValue = maskValue;
	krtn = msg_rpc(&msg.header, 
		MSG_OPTION_NONE, 
		sizeof(msg),
		0,
		0);
	if(krtn) {
		printf("%s: msg_rpc failure\n", op);
		mach_error("msg_rpc", krtn);
		return(1);
	}
	return(0);
}

/*
 * All of these functions assume that the xpr buffer has already been locked.
 */
static int ux_clear(port_name_t serverPort, port_name_t localPort)
{
	return(ux_simplemsg(serverPort, 
		localPort, 
		IO_CLEAR_DDM_MSG, 
		0, 0,
		"Clear")); 
}

static int ux_setmask(port_name_t serverPort, 
	port_name_t localPort,
	u_int bitmask)
{
	return(ux_simplemsg(serverPort, 
		localPort, 
		IO_SET_DDM_MASK_MSG, 
		bitmask, 
		0,		// index
		"Set Mask")); 
}

static int ux_dump(port_name_t serverPort, 
	port_name_t localPort,
	int numbufs)
{
	int offset;
	IODDMMsg msg;
	kern_return_t krtn;
	unsigned long long timestamp;
	
	msg = msgTemplate;
	
	/*
	 * Careful with this loop - numbufs == -1 means "run forever".
	 */
	for(offset=0; offset!=numbufs; offset++) {
		msg.header.msg_remote_port = serverPort;
		msg.header.msg_local_port  = localPort;
		msg.header.msg_id = IO_GET_DDM_ENTRY_MSG;
		msg.index = offset;
		krtn = msg_rpc(&msg.header, 
			MSG_OPTION_NONE, 
			sizeof(msg),
			0,
			0);
		if(krtn) {
			mach_error("ux_dump: msg_rpc failure", krtn);
			return(1);
		}
		switch(msg.status) {
		    case IO_DDM_SUCCESS:
		    	/*
			 * Display one string.
			 */
			timestamp = IONsTimeFromDDMMsg(&msg);
		    	printf("%10u c:%d %s", (unsigned)timestamp,
				msg.cpuNumber, msg.string);
			break;
			
		    case IO_NO_DDM_BUFFER:
		    	/*
			 * This is OK, it means we got to the end of the 
			 * buffer.
			 */
			printf("\n");
			return(0);
		
		    default:
		    	printf("ux_dump: Bogus xpr status (%d)\n", msg.status);
			return(1);
		}
	}
	return(0);
}
