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
 * Ethernet object exerciser.
 */
 
#define dbg_prn(a,b,c,d,e,f)		\
{					\
	if(verbose) {			\
		printf(a,b,c,d,e,f);	\
	}				\
}

/*
 * Hmmm...we need KERNEL defined to get netif.h...
 * ...and if.h really should import sys/socket.h.
 */
#import <bsd/sys/socket.h>		
#define KERNEL	1
#import <net/netif.h>
#undef	KERNEL
#import <Enet/Ethernet.h>
#import <driverkit/KernDevUxpr.h>
#import <driverkit/generalFuncs.h>
#import <kernserv/queue.h>
#import <machkit/NXLock.h>
#import <libc.h>

#define MIN_SIZE 	100		// minimum packet size
#define MAX_SIZE	500		// maximum packet size
#define INCREMENT	20		// delta packet size
#define THROTTLE	10		// max # packets outstanding
#define LOOP_COUNT	1

/*
 * This is how we enqueue incoming buffers.
 */
typedef struct in_buf in_buf_t;
struct in_buf {
	netbuf_t nb;
	in_buf_t *next;
};

/*
 * Static functions.
 */
static void usage(char **argv);
static int runTest(netif_t nif,
	int min_packet_size,
	int max_packet_size,
	int increment,
	int throttle);
static int check_new_bufs(int min_size, int increment, int packets_recd);
static void etherTimeout(void *flag);
static netbuf_t wait_for_packets(netif_t nif);

/*
 * Static variables.
 */
in_buf_t 	*in_buf_next, *in_buf_last;
id 		in_buf_lock;			// NXLock - protects
						//   in_buf
enetAddress_t	destAdrs = { {0,1,2,3,4,5} };
#ifdef	notdef
id		buf_wait_lock;			// NXConditionLock - used to 
						//   wait for incoming buf
#endif	notdef

/*
 * User-specified parameters.
 */
int min_size  = MIN_SIZE;
int max_size  = MAX_SIZE;
int increment = INCREMENT;
int throttle  = THROTTLE;
int verbose = 0;
int loop_count = LOOP_COUNT;
						
int main(int argc, char **argv)
{
	id	etherId;
	netif_t	nif;
	char 	s[100];
	int 	arg;
	int	loop_num;
	
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'v':
		    	verbose++;
			break;
		    case 'n':
		    	min_size = atoi(&argv[arg][2]);
			break;
		    case 'x':
		    	max_size = atoi(&argv[arg][2]);
			break;
		    case 'i':
		    	increment = atoi(&argv[arg][2]);
			break;
		    case 't':
		    	throttle = atoi(&argv[arg][2]);
			break;
		    case 'l':
		    	loop_count = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
		}
	}

#ifdef	DDM_DEBUG
	IOInitDDM(1000, "EnetXpr");
	IOSetDDMMask(XPR_IODEVICE_INDEX,
		 XPR_NET | XPR_ENTX | XPR_ENRX | XPR_ENCOM | XPR_NDMA);
#endif	DDM_DEBUG
	IOInitGeneralFuncs();
	etherId = [Ethernet probe:0 deviceMaster:PORT_NULL];
	if(etherId == nil) {
		printf("Driver Probe failed; exiting.\n");
		exit(1);
	}
	nif = [etherId getNetif];
#ifdef	notdef
	buf_wait_lock = [NXConditionLock new];
#endif	notdef
	in_buf_lock   = [NXLock new];
	loop_num = 0;
	do {
		printf("...Loop %d\n", loop_num);
		if(runTest(nif, min_size, max_size, increment, throttle))
			break;
		loop_num++;
	} while ((loop_count == 0) || (loop_num < loop_count));
	printf("Hit <CR> to quit: ");
	gets(s);
	exit(0);
}

static void usage(char **argv)
{
	printf("Usage: %s [options]\n");
	printf("   Options:\n");
	printf("\tn=min_size   (default = %d)\n", MIN_SIZE);
	printf("\tx=max_size   (default = %d)\n", MAX_SIZE);
	printf("\ti=increment  (default = %d)\n", INCREMENT);
	printf("\tt=throttle   (default = %d)\n", THROTTLE);
	printf("\tl=loop count (default = %d; 0 means forever)\n", LOOP_COUNT); 
	printf("\tv=verbose mode\n");
	exit(1);
}
/*
 * Main test loop. Returns non-zero on error.
 */
static int runTest(netif_t nif,
	int min_size,
	int max_size,
	int increment,
	int throttle)
{
/*
 *	packets_recd = 0;
 * 	for(size=min to max) {
 *		getbuf a netbuf from driver;
 *		if(no bufs available or throttle exceeded) {
 *			wait for some rx buffers to arrive;
 *		}
 *		else {
 *			fill with some data pattern;
 *			if_output() the netbuf to the driver;
 *			packets_sent++;
 *		}
 *		if any new netbufs on in_buf queue {
 *			for each one {
 *				verify proper data;
 *				packets_recd++;
 *				nb_free() it;
 *			}
 *		}
 *      }
 *	wait a reasonable time for packets_sent == packets_recd, checking 
 *		all incoming data;
 */
 
 	int size;
	int packets_sent = 0;
	int packets_recd = 0;
	netbuf_t nb;
	int rtn;
	int current_size;
	int timeout_flag;
	int packets_out = 0;
	
 	packets_recd = 0;
	size = min_size;
	do {
		nb = if_getbuf(nif);
		if((nb == NULL) || (packets_out >= throttle)) {
			
			/*
			 * nb == NULL means that the driver has no more 
			 * buffers; exceeding the throttle means we should
			 * back off and let 'DMA' complete.
			 *
			 * We'll just have to wait for some network activity.
			 */
			nb = wait_for_packets(nif);
			if(nb == NULL) {
				
				/*
				 * Still no Tx bufs available, but some 
				 * Rx bufs showed up.
				 */
				goto handle_input;
			}
			/* 
			 * Else drop thru and send using nb.
			 */
		}

		/*
		 * nb_write() some data here eventually. For now, 
		 * just set the size to min_size + 
		 * (increment * packet_num).
		 */
		dbg_prn("sending  packet num %d size %d\n", 
			packets_sent, size, 3,4,5);
		current_size = min_size + (increment * packets_sent); 
		nb_shrink_bot(nb, nb_size(nb) - current_size);
		rtn = if_output(nif, nb, &destAdrs);
		if(rtn) {
			printf("Error on if_output (rtn = %d)\n", rtn);
			return -1;
		}
		packets_sent++;
		packets_out++;
		size += increment;
		
		/*
		 * Check for incoming packets.
		 */
handle_input:
		rtn = check_new_bufs(min_size, increment, packets_recd);
		if(rtn < 0) 
			return rtn;
		packets_recd += rtn;
		packets_out -= rtn;
		
	} while (size <= max_size);
	
	/*
	 * In a reasonable time, we should see all of the packets. 
	 */
	if(packets_recd != packets_sent) {
		dbg_prn("Exit main loop; waiting for incoming packets\n",
			1,2,3,4,5);
		timeout_flag = 0;
		IOScheduleFunc((IOThreadFunc)etherTimeout, &timeout_flag, 2);
		do {
			rtn = check_new_bufs(min_size, increment,
				packets_recd);
			if(rtn < 0) 
				return rtn;
			packets_recd += rtn;
		} while (!timeout_flag && (packets_recd != packets_sent));
		if(timeout_flag) {
			printf("%d packets sent, %d received: FATAL\n",
				packets_sent, packets_recd);
			return -1;
		}
		else {
			IOUnscheduleFunc((IOThreadFunc)etherTimeout,
				&timeout_flag);
		}
	}
	return 0;
}

/*
 * Timeout function, called out via IOScheduleFunc. Sets the specified flag to 1. 
 */
static void etherTimeout(void *flag)
{
	*((int *)flag) = 1;
#ifdef	notdef
	[buf_wait_lock lock];
	[buf_wait_lock unlockWith:TRUE];
#endif	notdef
}

/*
 * Wait for some packets to arrive or for if_getbuf() to succeed.
 * if_getbuf() is attempted every 50 ms (actually, with current libIO, 
 * every 1 second...). 
 *
 * If if_getbuf() succeeds, this returns a netbuf_t; else (in_buf non-empty)
 * returns NULL.
 */
static netbuf_t wait_for_packets(netif_t nif)
{
	netbuf_t nb;
	
	dbg_prn("...waiting for available packets\n", 1,2,3,4,5);
	while(1) {
	
		/*
		 * Any incoming bufs?
		 */
		[in_buf_lock lock];
		if(in_buf_next != NULL) {
			[in_buf_lock unlock];
			return NULL;
		}
		[in_buf_lock unlock];
		
		/*
		 * Any outgoing bufs?
		 */
		nb = if_getbuf(nif);
		if(nb) {
			return(nb);
		}	  
		
		/*
		 * Go to sleep. 
		 */
		/* IOSleep(50); */
		cthread_yield();
	}
}

/*
 * Check out any new netbufs in in_buf queue. Returns -1 on error, else 
 * return the number of new packets received and checked.
 *
 * One entry, packets_recd is the number of packets already seen.
 */
static int check_new_bufs(int min_size, int increment, int packets_recd)
{
	in_buf_t *ib;
	int new_packets = 0;
	int current_size;
	int error = 0;
	int current_packet = packets_recd;
	int recd_size;
	
	[in_buf_lock lock];
	ib = in_buf_next;
	if(ib) {
		in_buf_next = ib->next;
	}
	[in_buf_lock unlock];
	while(ib) {
			
		/*
		 * eventually check out data here...For now, just check size.
		 */
		current_size = min_size + (increment * (current_packet));
		recd_size = nb_size(ib->nb);
		dbg_prn("received packet num %d size %d\n", 
			current_packet, recd_size,3,4,5);
		if(recd_size != current_size) {
			printf("***bad size for in_packet %d\n", 
				current_packet);
			printf("   Expected %d   actual %d\n",
				current_size, recd_size);
			error++;
		}
		current_packet++;
		new_packets++;		
		
		/*
		 * Free both the netbuf and the in_buf.
		 */
		nb_free(ib->nb);
		IOFree(ib, sizeof(*ib));
		
		/*
		 * Get next in_buf.
		 */
		[in_buf_lock lock];
		ib = in_buf_next;
		if(ib) {
			in_buf_next = ib->next;
		}
		[in_buf_lock unlock];

	}
	if(error)
		return -1;
	else
		return new_packets;
}

/*
 * Provided for handling incoming packets. These should map one-to-one with the
 * packets we send via if_output(). This will actually be called from the 
 * driver's enetThread.
 *
 * All we do is save the incoming netbuf in in_buf. The main thread
 * does data and sequence checking.
 */
int if_handle_input(netif_t netif, 
	netbuf_t nb, 
	void *extra)
{
	in_buf_t *ib = IOMalloc(sizeof(in_buf_t));

	ib->nb = nb;
	ib->next = NULL;
	[in_buf_lock lock];
	if(in_buf_next == NULL) {
		in_buf_next = in_buf_last = ib;
	}
	else {
		in_buf_last->next = ib;
		in_buf_last = ib;
	}
	[in_buf_lock unlock];
#ifdef	notdef
	[buf_wait_lock lock];
	[buf_wait_lock unlockWith:TRUE];
#endif	notdef
	return 0;
}

