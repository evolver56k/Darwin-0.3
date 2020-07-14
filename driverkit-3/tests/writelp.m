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
 * write.c - Write to stub server 'n' times
 */

#import <bsd/sys/types.h> 
#import <remote/NXConnection.h>
#import <driverkit/IODisk.h>
#import <driverkit/IODiskRwDistributed.h>
#import <ansi/stdio.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import "defaults.h"
#import "buflib.h"

void usage(char **argv);
void exit(int exitcode);
void fill_buf(u_char *p, int size, char d);
void mach_error(char *c, kern_return_t e);

int main(int argc, char **argv) {

	int 		arg;
	char 		c;
	id 		targetId;
	IOReturn 	rtn;
	IOData 		*wdata;
	u_int 		bytes_xfr;
	int		loop_num;
	
	/*
	 * user-spec'd variables 
	 */
	u_int 		byte_count=BYTE_COUNT_DEFAULT;	/* bytes to write */
	u_int 		block_num=0;		
	char 		*hostname=HOST_DEFAULT;
	char 		*devname=DEVICE_DEFAULT;
	char		data_patt = 'z';
	int 		loop_count;
	int		quiet = 0;
	
	if(argc < 2)
		usage(argv);
	loop_count = atoi(argv[1]);

	/*
	 * Get standard defaults from environment or defaults.h
	 */
	get_default("byte_count", &byte_count, BYTE_COUNT_DEFAULT);
	get_default_t("hostname", &hostname, HOST_DEFAULT);
	get_default_t("devname", &devname, DEVICE_DEFAULT);

	for(arg=2; arg<argc; arg++) {
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
		    	data_patt = argv[arg][2];
			break;
		    case 'q':
		    	quiet++;
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
	wdata = [[IOData alloc] initWithSize:byte_count];
	[wdata setFreeFlag:NO];
	fill_buf((u_char *)[wdata data], (int)byte_count, data_patt);
	for(loop_num=0; loop_num<loop_count; loop_num++) {
		rtn = [targetId writeAt:block_num
				length:byte_count
				data:wdata
				actualLength:&bytes_xfr];
		if(rtn != IO_R_SUCCESS) {
			printf("write: %s\n", [targetId stringFromReturn:rtn]);
			exit(1);
		}
		if(bytes_xfr != byte_count) {
			printf("bytes_xfr = %d; expected %d\n", 
				bytes_xfr, byte_count);
		}	
		if(!quiet && (loop_num % 100 == 0)) {
			printf(".");
			fflush(stdout);
		}
	}
	printf("...ok\n");
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s  loop_count [y=byte_count] [q(uiet)] [b=block_num] [p={z,1,i,d}] [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}

void fill_buf(u_char *buf, int size, char data_patt) {

	int i;
	
	printf("\n");
	for(i=0; i<size; i++) {
		switch(data_patt) {
		    case 'z':
			*buf++ = 0;
			break;
		    case '1':
		    	*buf++ = 0xff;
			break;
		    case 'i':
		    	*buf++ = (u_char)i;
			break;
		    case 'd':
		    	*buf++ = 0xff - (u_char)i;
			break;
		    default:
		    	printf("bogus data_patt (%c) in fill_buf()\n", 
				data_patt);
			exit(1);
		}
	}
} /* fill_buf() */




