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
 * wrclp.m - write/read/compare loop
 */

#import <bsd/sys/types.h> 
#import <driverkit/IOClient.h>
#import <ansi/stdio.h>
#import <mach/kern_return.h>
#import <mach/mach.h>
#import <ansi/stdlib.h>
#import <ansi/stdio.h>
#import <bsd/fcntl.h>
#import "buflib.h"
#import "defaults.h"

void usage(char **argv);
void exit(int exitcode);
void mach_error(char *c, kern_return_t e);
void fill_buf(u_char *buf, register int size, char data_patt);

int main(int argc, char **argv) {

	int 		arg;
	char 		c;
	id 		idClient;
	IOReturn 	rtn;
	char 		*rbuf, *wbuf;
	kern_return_t 	krtn;
	u_int 		bytes_xfr;
	int 		loop_num;
	char		outstr[100];
	
	/*
	 * user-spec'd variables 
	 */
	u_int 		byte_count=BYTE_COUNT_DEFAULT;	/* bytes to read */
	u_int 		block_num=0;		
	char 		*hostname=HOST_DEFAULT;
	char 		*devname=DEVICE_DEFAULT;
	int 		loop_count;
	int		open_each_loop=0;
	int		async = 0;
	
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
		    case 'o':
		    	open_each_loop++;
			break;
		    case 'a':
		    	async++;
			break;
		    default:
		    	usage(argv);
		}
	}
	rtn = [IOClient clientOpen:devname 
		hostName:hostname
		intentions:(IO_INT_READ|IO_INT_WRITE)
		idp:&idClient];
	if(rtn) {
		[IOClient ioError:rtn logString:"clientOpen" outString:outstr];
		printf(outstr);
		exit(1);
	}
	krtn = vm_allocate(task_self(), 
	    	(vm_address_t *)&rbuf, 
	    	byte_count,
	    	TRUE);
	if(krtn) {
	    	mach_error("vm_allocate(rbuf)", krtn);
		exit(1);
	}
	krtn = vm_allocate(task_self(), 
	    	(vm_address_t *)&wbuf, 
	    	byte_count,
	    	TRUE);
	if(krtn) {
	    	mach_error("vm_allocate(wbuf)", krtn);
		exit(1);
	}
	for(loop_num=0; loop_num<loop_count; loop_num++) {
		/*
		 * initialize read/write buffers. Data = loop #.
		 */
		fill_buf((u_char *)wbuf, byte_count, (char)loop_num);
		fill_buf((u_char *)rbuf, byte_count, 0);
		
		if(async) {
			rtn = [idClient ioWriteAsync:block_num
				bytesReq:byte_count 
				buf:wbuf
				queueId:block_num+1];
			if(rtn)
				goto write_error;
			rtn = [idClient ioWriteWait:block_num+1
				bytesXfr:&bytes_xfr];
		}
		else {
			rtn = [idClient ioWrite:block_num
				bytesReq:byte_count
				buf:wbuf
				bytesXfr:&bytes_xfr];
		}
write_error:
		if(rtn != IO_R_SUCCESS) {
			[IOClient ioError:rtn 
				logString: async ? "ioWriteAsync" : "ioWrite"
				outString:outstr];
			printf(outstr);
			exit(1);
		}
		if(async) {
			rtn = [idClient ioReadAsync:block_num
				  bytesReq:byte_count
				  queueId:block_num+1];
			if(rtn)
				goto read_error;
			rtn = [idClient ioReadWait:block_num+1
				buf:rbuf
				bytesXfr:&bytes_xfr];
		}
		else {
			rtn = [idClient ioRead:block_num
				bytesReq:byte_count
				buf:rbuf
				bytesXfr:&bytes_xfr];
		}
read_error:
		if(rtn != IO_R_SUCCESS) {
			[IOClient ioError:rtn 
				logString: async ? "ioReadAsync" : "ioRead"
				outString:outstr];
			printf(outstr);
			exit(1);
		}
		if(bytes_xfr != byte_count) {
			printf("bytes_xfr = %d; expected %d\n", bytes_xfr, 
			byte_count);
		}	
		if(buf_comp(byte_count, (u_char *)wbuf, (u_char *)rbuf))
			exit(1);
		if(loop_num % 100 == 0) {
			printf(".");
			fflush(stdout);
		}
		if(open_each_loop && (loop_num<loop_count)) {
			rtn = [idClient ioClose];
			if(rtn) {
				[IOClient ioError:rtn 
					logString:"ioClose"
					outString:outstr];
				printf(outstr);
				exit(1);
			}
			rtn = [IOClient clientOpen:devname 
				hostName:hostname
				intentions:(IO_INT_READ|IO_INT_WRITE)
				idp:&idClient];
			if(rtn) {
				[IOClient ioError:rtn 
					logString:"clientOpen" 
					outString:outstr];
				printf(outstr);
				exit(1);
			}
		}
	}
	rtn = [idClient ioClose];
	if(rtn) {
		[IOClient ioError:rtn 
			  logString:"ioClose"
			  outString:outstr];
		printf(outstr);
		exit(1);
	}
	printf("\n...ok\n");
	exit(0);

} /* main() */

void usage(char **argv) {
	printf("usage: %s loop_count [options]\n", argv[0]);
	printf("Options:\n");
	printf("\ty=byte_count\n");
	printf("\to(pen_each_lp)\n");
	printf("\tb=block_num\n");
	printf("\th=hostname\n");
	printf("\td=devname\n");
	printf("\ta(async I/O)\n");
	exit(1);
}

void fill_buf(u_char *buf, register int size, char data_patt) {

	register int i;
	
	for(i=0; i<size; i++) 
		*buf++ = data_patt++;
} /* fill_buf() */


