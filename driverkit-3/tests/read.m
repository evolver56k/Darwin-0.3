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
 * Read once.
 */

#import <bsd/sys/types.h> 
#import <ansi/stdio.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import <bsd/libc.h>
#import "buflib.h"
#import "defaults.h"
#import <remote/NXConnection.h>
#import <driverkit/IODisk.h>
#import <driverkit/IODiskRwDistributed.h>

void usage(char **argv);
void exit(int exitcode);
void mach_error(char *c, kern_return_t e);

int main(int argc, char **argv) {

	int 		arg;
	char 		c;
	id 		targetId;
	IOReturn 	rtn;
	u_int 		bytes_xfr;
	const char	*outstr;
	IOData		*rdata;
	
	/*
	 * user-spec'd variables 
	 */
	u_char		dump_data = 0;		/* dump read data to stdout */
	u_int 		byte_count=BYTE_COUNT_DEFAULT;	/* bytes to read */
	u_int 		block_num=0;		
	char 		*hostname=HOST_DEFAULT;
	char 		*devname=DEVICE_DEFAULT;
	char		do_unaligned=0;		/* force non-page-aligned read
						 * buffer */
		
	/*
	 * FIXME - isn't there a better way to ensure that IOData gets 
	 * linked into this executable? We need it to be linked so that
	 * DO decode can instantiate it.
	 */
	targetId = [IOData alloc];
	[targetId free];
	
	/*
	 * Get standard defaults from environment or defaults.h
	 */
	get_default("byte_count", &byte_count, BYTE_COUNT_DEFAULT);
	get_default_t("hostname", &hostname, HOST_DEFAULT);
	get_default_t("devname", &devname, DEVICE_DEFAULT);

	for(arg=1; arg<argc; arg++) {
		c = argv[arg][0];
		switch(c) {
		    case 'y':
			byte_count = atoi(&argv[arg][2]);
		    	break;
		    case 'h':
		    	hostname = &argv[arg][2];
			break;
		    case 'd':
		    	devname = &argv[arg][2];
			break;
		    case 'b':
		    	block_num = atoi(&argv[arg][2]);
			break;
		    case 'p':
		    	dump_data++;
			break;
		    case 'u':
		    	do_unaligned++;
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

	rtn = [targetId readAt:block_num
			length:byte_count
			data:&rdata
			actualLength:&bytes_xfr];
	if(rtn != IO_R_SUCCESS) {
		outstr = [targetId stringFromReturn:rtn];
		printf("read: %s\n", outstr);
		exit(1);
	}
	if(bytes_xfr != byte_count) {
		printf("bytes_xfr = %d; expected %d\n", bytes_xfr, byte_count);
	}	
	if(dump_data) 
		dump_buf((u_char *)[rdata data], bytes_xfr);
	else 
		printf("...ok\n");
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s [y=byte_count] [p (dump data)] [u(naligned buffer)] [b=block_num] [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}

