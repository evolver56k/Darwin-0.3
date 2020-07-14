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
 * IOStubKernLoad.m - kernload module for IOStub.
 *
 * HISTORY
 * 25-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#undef	MACH_USER_API
#define KERNEL_PRIVATE	1

#define STUB_BLOCK_DEVICE	7
#define STUB_RAW_DEVICE		16

#import <kernserv/kern_server_types.h>
#import <mach/message.h>
#import "IOStub.h"
#import "IOStubPrivate.h"
#import "IOStubUnix.h"
#import <bsd/sys/conf.h>
#import <bsd/dev/ldd.h>

kern_server_t stub_var;

struct cdevsw stub_saved_cdevsw;
struct bdevsw stub_saved_bdevsw;

extern int nulldev();
extern int nodev();
extern int seltrue();
#define	nullstr 0

/*
 * these get copied into bdevsw/cdevsw at load time.
 */
struct bdevsw stub_bdevsw =  { (PFI)stub_open,	
			       (PFI)stub_close,	
			       (PFI)stub_strategy,	
			       nodev,
			       nodev,	
			       0 };
struct cdevsw stub_cdevsw =  { (PFI)stub_open,
			       (PFI)stub_close,	
			       (PFI)stub_read,	
			       (PFI)stub_write,
			       (PFI)stub_ioctl,	
			       nodev,		
			       nulldev,	
			       seltrue,	
			       nodev,		
			       nodev,
			       nodev };

void stub_announce(int unit);
void stub_port_gone(port_name_t port);
void stub_terminate(int unit);
void stub_server(msg_header_t *in_p, int unit);

/* 
 * we're just placed into memory here. Actual device initialization
 * and instantiation is done in stub_server().
 */
void stub_announce(int unit) 
{
	IOLog("stub%d: IOStub device Loaded\n", unit);
} /* stub_announce() */

void stub_port_gone(port_name_t port) 
{
	/* no-op, we don't really care about any ports. */
	
} /* stub_port_gone() */

void stub_terminate(int unit) 
{
	int i;
	
	xpr_stub("stub_terminate\n", 1,2,3,4,5);
	
	/* 
	 * remove ourself from devsw's.
	 */
	bdevsw[STUB_BLOCK_DEVICE] = stub_saved_bdevsw;
	cdevsw[STUB_RAW_DEVICE]   = stub_saved_cdevsw;
	for(i=0; i<NUM_IOSTUBS; i++) {
		if(stub_object[i].stub_id != nil) {
			[stub_object[i].stub_id free];
			stub_object[i].stub_id = nil;
			IOFree(stub_object[i].physbuf, sizeof(struct buf));
		}
	}	
	
	IOLog("stub%d: Stub Device Unloaded\n", unit);
} /* stub_terminate() */

/*
 * Initialize and instantiate. This is invoked upon receipt of any message.
 *
 * FIXME: this could share common code with stub_probe() when we're no longer
 * a loadable module.
 */
static int stub_is_initialized = 0;

void stub_server(msg_header_t *in_p, int unit)
{
	int Unit;

	if(stub_is_initialized) {
		IOLog("stub_server already initialized\n");
		goto done;
	}
	
#if	m68k
	/*
	 * First initialize the libraries we'll use.
	 */
	IOlibIOInit();		
#endif	m68k
#ifdef	XPR_DEBUG
#if	m68k
	xprInit(500);
#endif	m68k
	xprSetBitmask(XPR_IODEVICE_INDEX, XPR_DEVICE | XPR_STUB);
#endif	XPR_DEBUG
	xpr_stub("stub_server\n", 1,2,3,4,5);
	
	for(Unit=0; Unit<NUM_IOSTUBS; Unit++) {
		stub_object[Unit].stub_id = [IOStub stubProbe:Unit];
		if(stub_object[Unit].stub_id != nil) {
			stub_object[Unit].physbuf = 
				IOMalloc(sizeof(struct buf));
			stub_object[Unit].physbuf->b_flags = 0;
		}
	}
	
	/* 
	 * place ourself in cdevsw and bdevsw.
	 */
	stub_saved_bdevsw = bdevsw[STUB_BLOCK_DEVICE];
	stub_saved_cdevsw = cdevsw[STUB_RAW_DEVICE];
	bdevsw[STUB_BLOCK_DEVICE] = stub_bdevsw;
	cdevsw[STUB_RAW_DEVICE]   = stub_cdevsw;

	IOLog("IOStub device Installed\n");

	/*
	 * return message to loader.
	 */
done:
	msg_send(in_p, 
		MSG_OPTION_NONE,
		0);	
} /* stub_server() */


/* end of stub_ks.m */
