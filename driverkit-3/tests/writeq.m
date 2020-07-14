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
 * writeq.c - Write to using queued I/O
 */

#import <bsd/sys/types.h> 
#import <driverkit/IOClient.h>
#import <ansi/stdio.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import "defaults.h"
#import "buflib.h"
#import <bsd/sys/signal.h>
#import <mach/mach_error.h>

void usage(char **argv);
void exit(int exitcode);
void fill_buf(u_char *p, int size, char d);
void sigint(int foo);

int write_requests = 0;
int write_replies = 0;

int main(int argc, char **argv) {

	int 		arg;
	char 		c;
	id 		idClient;
	IOReturn 	rtn;
	char 		*wbuf;			/* write buffer */
	char 		*wwbuf;			/* working wrtite buffer */
	kern_return_t 	krtn;
	u_int 		bytes_xfr;
	int 		io_num;
	IOErrorString	outstr;
	
	/*
	 * user-spec'd variables 
	 */
	u_int 		byte_count=BYTE_COUNT_DEFAULT;	/* bytes to write */
	u_int 		block_num=0;		
	char 		*hostname=HOST_DEFAULT;
	char 		*devname=DEVICE_DEFAULT;
	char		data_patt = 'z';
	int		num_ios;
	int 		loop = 0;
	int		loop_num = 0;
	
	if(argc < 2)
		usage(argv);
	num_ios = atoi(argv[1]);
	
	/*
	 * Get standard defaults from environment of defaults.h
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
		intentions:IO_INT_WRITE
		idp:&idClient];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"clientOpen"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
	krtn = vm_allocate(task_self(), 
	    	(vm_address_t *)&wbuf, 
	    	byte_count * num_ios,
	    	TRUE);
	if(krtn) {
	    	mach_error("vm_allocate", krtn);
		exit(1);
	}
	fill_buf((u_char *)wbuf, (int)byte_count * num_ios, data_patt);
	
	do {
	 
		/* 
		 * fire off num_ios queued write requests
		 */
		wwbuf = wbuf;
		for(io_num=1; io_num<=num_ios; io_num++) {
		
			rtn = [idClient ioWriteAsync:block_num+io_num
					bytesReq:byte_count
					buf:wwbuf
					queueId:io_num];
			if(rtn != IO_R_SUCCESS) {
				[IOClient ioError:rtn 
					    logString:"ioWriteAsync"
					    outString:outstr];
				printf(outstr);
				exit(1);
			}
			write_requests++;
			wwbuf += byte_count;
		}
	
		/* 
		 * now wait for the I/O completes, in reverse order
		 */
		    
		for(io_num=num_ios; io_num; io_num--) {
			rtn = [idClient ioWriteWait:io_num
					bytesXfr:&bytes_xfr];
			if(rtn != IO_R_SUCCESS) {
				[IOClient ioError:rtn 
					    logString:"ioWriteWait"
					    outString:outstr];
				printf(outstr);
				exit(1);
			}
			write_replies++;
			if(bytes_xfr != byte_count) {
				printf("bytes_xfr = %d; expected %d\n", 
					bytes_xfr, byte_count);
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
	else
		printf("...ok\n");
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s num_ios [l(oop] [y=byte_count] [b=block_num] [p={z,1,i,d}] [h=hostname] [d=devname]\n", argv[0]);
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

void sigint(int foo)
{
	printf("\nAborting.\n");
	printf("%d write requests\n", write_requests);
	printf("%d write replies\n", write_replies);
	exit(1);
}


