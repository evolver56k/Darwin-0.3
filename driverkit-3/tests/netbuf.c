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
 * netbuf.m - netbuf stub for testing NetDriver objects.
 *
 * HISTORY
 * 24-Jul-91    Doug Mitchell at NeXT
 *      Created. 
 */

/*
 * Hmmm...we need KERNEL defined to get netif.h...
 */
#import <bsd/sys/socket.h>		
#define KERNEL	1
#import <net/netif.h>
#undef	KERNEL
#import <driverkit/generalFuncs.h>
#import <libc.h>

/*
 * Our private mbuf equivalent.
 */
typedef struct {
	void		*start_data;		// starting DMA data at
						// nb_alloc time
	int		start_length;		// starting size at 
						// nb_alloc time
	void 		*data;			// current DMA data
	int		length;			// amount of data in *data
	void		(*free_fcn)(void *);	// "free" function
	void 		*free_arg;		// "free argument
} stub_netbuf_t;

/*
 * Private functions.
 */
static stub_netbuf_t *stub_netbuf_alloc();
static void stub_netbuf_free(stub_netbuf_t *snb);


char *nb_map(netbuf_t nb)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;

	return (char *)snb->data;
}

/*
 * Alloc some data as well as a stub_netbuf_t.
 */
netbuf_t nb_alloc(unsigned size)
{
	char *data;
	stub_netbuf_t *snb;

	data = IOMalloc(size);
	snb = stub_netbuf_alloc();
	snb->data = snb->start_data = data;
	snb->length = snb->start_length = size;
	snb->free_fcn = NULL;
	return (netbuf_t)snb;
}

netbuf_t nb_alloc_wrapper(void *data, 
	unsigned size,
	void freefunc(void *), void *freefunc_arg)
{
	stub_netbuf_t *snb;

	snb = stub_netbuf_alloc();
	snb->data = snb->start_data = data;
	snb->length = snb->start_length = size;
	snb->free_fcn = freefunc;
	snb->free_arg = freefunc_arg;
	return (netbuf_t)snb;
}

void nb_free(netbuf_t nb)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	/*
	 * First free the data, whether we allocated it or the driver did.
	 */
	if(snb->free_fcn) {
		(*snb->free_fcn)(snb->free_arg);
	}
	else {
		IOFree(snb->data, snb->length);
	}
	
	/*
	 * now free the wrapper.
	 */
	stub_netbuf_free(snb);
}

void nb_free_wrapper(netbuf_t nb)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;

	stub_netbuf_free(snb);
}

unsigned nb_size(netbuf_t nb)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	return (unsigned)snb->length;
}

int nb_shrink_top(netbuf_t nb, unsigned size)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	if((snb->length - (int)size) < 0) {
		IOLog("Illegal nb_shrink_top\n");
		return -1;
	}
	snb->length -= size;
	snb->data   += size;
	return 0;
}

int
nb_grow_top(netbuf_t nb, unsigned size)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	if(((char *)snb->data - size) < (char *)snb->start_data) {
		IOLog("Illegal nb_grow_top (buf underflow)\n");
		return -1;
	}
	if((snb->length + size) > snb->start_length) {
		IOLog("Illegal nb_grow_top (buf overflow)\n");
		return -1;
	}
	snb->length += size;
	snb->data   -= size;
	return 0;
}

int
nb_shrink_bot(netbuf_t nb, unsigned size)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	if((snb->length - (int)size) < 0) {
		IOLog("Illegal nb_shrink_bot\n");
		return -1;
	}
	snb->length -= size;
	return 0;
}


int
nb_grow_bot(netbuf_t nb, unsigned size)
{
	stub_netbuf_t *snb = (stub_netbuf_t *)nb;
	
	if((snb->length + size) > snb->start_length) {
		IOLog("Illegal nb_grow_bot (buf overflow)\n");
		return -1;
	}
	snb->length += size;
	return 0;
}

/*
 * Private functions.
 */
static stub_netbuf_t *stub_netbuf_alloc()
{
	stub_netbuf_t *snb;
	
	snb = (stub_netbuf_t *)IOMalloc(sizeof(*snb));
	bzero(snb, sizeof(*snb));
	return snb;
}

static void stub_netbuf_free(stub_netbuf_t *snb)
{
	IOFree(snb, sizeof(*snb));
}
