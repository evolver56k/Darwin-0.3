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
 * readq.m - Read using queued I/O
 */

#import <bsd/sys/types.h> 
#import <driverkit/IOClient.h>
#import <ansi/stdio.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import <bsd/libc.h>
#import "buflib.h"
#import "defaults.h"
#import <bsd/sys/signal.h>
#import <mach/mach_error.h>

void usage(char **argv);
void exit(int exitcode);
void sigint(int foo);

int read_requests = 0;
int read_replies = 0;

int main(int argc, char **argv) {

	int 		arg;
	char 		c;
	id 		idClient;
	IOReturn 	rtn;
	char 		*rbuf, *rrbuf;
	kern_return_t 	krtn;
	u_int 		bytes_xfr;
	int		i;
	int 		io_num;
	IOErrorString	outstr;
	int		loop_num = 0;
	
	/*
	 * user-spec'd variables 
	 */
	u_char		dump_data = 0;		/* dump read data to stdout */
	u_int 		byte_count=BYTE_COUNT_DEFAULT;	/* bytes to read */
	u_int 		block_num=0;		
	char 		*hostname=HOST_DEFAULT;
	char 		*devname=DEVICE_DEFAULT;
	int		num_ios;
	int 		loop = 0;
	
	if(argc < 2)
		usage(argv);
	num_ios = atoi(argv[1]);

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
		    	dump_data++;
			break;
		    case 'l':
		    	loop++;
			break;
		    default:
		    	usage(argv);
		}
	}
	signal(SIGINT, sigint);
	
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
	krtn = vm_allocate(task_self(), 
	    	(vm_address_t *)&rbuf, 
	    	byte_count * num_ios,
	    	TRUE);
	if(krtn) {
	    	mach_error("vm_allocate", krtn);
		exit(1);
	}
	for(i=0; i<byte_count * num_ios; i++)
		rbuf[i] = 0x5a;
		
	do {
	
		/* 
		 * fire off num_ios queued read requests.
		 */
		for(io_num=1; io_num<=num_ios; io_num++) {
		
			rtn = [idClient ioReadAsync:block_num+io_num
					bytesReq:byte_count
					queueId:io_num];
			if(rtn != IO_R_SUCCESS) {
				[IOClient ioError:rtn 
					    logString:"ioReadAsync"
					    outString:outstr];
				printf(outstr);
				exit(1);
			}
			read_requests++;
		}
		
		/* 
		 * now wait for the data, in reverse order. 
		 */
		rrbuf = rbuf + num_ios * byte_count;
		for(io_num=num_ios; io_num; io_num--) {
			rrbuf -= byte_count;
			rtn = [idClient ioReadWait:io_num
					buf:rrbuf
					bytesXfr:&bytes_xfr];
			if(rtn != IO_R_SUCCESS) {
				[IOClient ioError:rtn 
					    logString:"ioReadWait"
					    outString:outstr];
				printf(outstr);
				exit(1);
			}
			read_replies++;
			if(bytes_xfr != byte_count) {
				printf("bytes_xfr = %d; expected %d\n", 
					bytes_xfr, byte_count);
			}	
			if(dump_data) {
				printf("Read data, block %d\n", 
					block_num+io_num);
				dump_buf((u_char *)rrbuf, bytes_xfr);
				printf("\n");
			}
		}
		if(++loop_num % 100 == 0) {
			printf(".");
			fflush(stdout);
		}
	} while(loop);
	rtn = [idClient ioClose];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"ioClose"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
	else if(!dump_data)
		printf("...ok\n");
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s num_ios [l(oop)] [y=byte_count] [p (dump data)] [b=block_num] [h=hostname] [d=devname]\n", argv[0]);
	exit(1);
}

void sigint(int foo)
{
	printf("\nAborting.\n");
	printf("%d read requests\n", read_requests);
	printf("%d read replies\n", read_replies);
	exit(1);
}
