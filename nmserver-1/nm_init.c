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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#define NM_INIT_DEBUG	0

#include <mach/mach.h>
#include <mach/cthreads.h>
#include <mach/boolean.h>

#include "datagram.h"
#include "dispatcher.h"
#include "ipc.h"
#include "keyman.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_init.h"
#include "nn.h"
#include "portcheck.h"
#include "portops.h"
#include "portrec.h"
#include "portsearch.h"
#include "srr.h"
#include "timer.h"
#include "uid.h"

#ifndef WIN32
#include <netinet/in.h>
#endif

port_set_name_t nm_port_set;

extern boolean_t tcp_init (void);
extern boolean_t transport_init (void);
extern boolean_t lock_queue_init (void);


/*
 * nm_init
 *	initialises all the modules of the network server.
 *
 * Returns:
 *	TRUE or FALSE depending on whether initialisation was successful or not.
 *
 */
EXPORT boolean_t nm_init()
{
    boolean_t	success = TRUE;

    if (port_set_allocate(task_self(), &nm_port_set) != KERN_SUCCESS) {
	panic("port_set init failed.");
	success = FALSE;
    }

    /*
     * Initialise the utility modules.
     */
    if (!(mem_init())) {
	panic("mem_init failed.");
	success = FALSE;
    }
    if (!(lock_queue_init())) {
	panic("lock_queue_init failed.");
	success = FALSE;
    }
    if (!(uid_init())) {
	panic("uid_init failed.");
	success = FALSE;
    }
    if (!(timer_init())) {
	panic("timer_init failed.");
	success = FALSE;
    }
#if defined(NeXT_PDO) && !defined(WIN32)
    if (!(network_init())) {
#else
    if (param.conf_network && !(network_init())) {
#endif
	ERROR((msg,"Autoconf: network_init failed: no network"));
	param.conf_network = FALSE;
    }
    if (!(disp_init())) {
	panic("disp_init failed.");
	success = FALSE;
    }
    if (!(transport_init())) {
	panic("transport_init failed.");
	success = FALSE;
    }
    if (!(pr_init())) {
	panic("pr_init fails.");
	success = FALSE;
    }

#if !defined(NeXT_PDO) || defined(WIN32)
    if (param.conf_network) {
#endif
	    /*
	     * Initialise the transport protocols about which we know.
	     */
	    if (!(datagram_init())) {
		panic("datagram_init failed.");
		success = FALSE;
	    }
	    if (!(srr_init())) {
		panic("srr_init failed.");
		success = FALSE;
	    }

	    /*
	     * Start the IPC protocol(s).
	     */
	    tr_default_entry = TR_NOOP_ENTRY;

	    if (!(tcp_init())) {
		ERROR((msg,"Autoconf: tcp_init failed."));
	    } else {
		    tr_default_entry = TR_TCP_ENTRY;
	    }

	    /*
	     * Make sure there is at least one valid IPC protocol.
	     */
	    if (tr_default_entry == TR_NOOP_ENTRY) {
		    panic("Network enabled, but no valid IPC transport protocol.");
	    }

#if !defined(NeXT_PDO) || defined(WIN32)
    } else {
	    tr_default_entry = TR_NOOP_ENTRY;
    }
#endif


    /*
     * Initialise the higher level modules.
     */
    if (!(netname_init())) {
	panic("netname_init fails.");
	success = FALSE;
    }
    if (!(ipc_init())) {
	panic("ipc_init failed.");
	success = FALSE;
    }
    if (!(pc_init())) {
	panic("pc_init fails.");
	success = FALSE;
    }
    if (!(po_init())) {
	panic("po_init fails.");
	success = FALSE;
    }
    if (!(ps_init())) {
	panic("po_init fails.");
	success = FALSE;
    }
    return success;

}

#ifndef NeXT_PDO
/*
 *  This routine is called whenever we receive a USR2 signal.  This allows
 *  us to start up nmserver without having any network interface enabled,
 *  do some work (such as kern_load a token ring driver), initialize the
 *  network ifs, and finally tell nmserver that it should take a look at
 *  the network again.  We'll only start up the various network service
 *  routines once, but network_init() may be called multiple times.
 *  Note that some routines may stash the current IP address away (my_host_id)
 *  and not notice any changes.  This may cause a problem for ephemeral IP
 *  addresses such as those given to temporary SLIP or ISDN links.
 *  --Lennart Lovstrand, Thu Nov 19 17:00:12 1992
 */
void reinit_network()
{
    /* Try to (re)initialize the network, but only start the network
     * dependent services if they haven't already been started.
     */      
    if (network_init() && !param.conf_network) {
	param.conf_network = TRUE;

	/*
	 * Initialise the transport protocols about which we know.
	 */
	if (!(datagram_init())) {
	    panic("datagram_init failed.");
	}
	if (!(srr_init())) {
	    panic("srr_init failed.");
	}

	/*
	 * Start the IPC protocol(s).
	 */
	tr_default_entry = TR_NOOP_ENTRY;

	if (!(tcp_init())) {
	    ERROR((msg,"Autoconf: tcp_init failed."));
	} else {
		tr_default_entry = TR_TCP_ENTRY;
	}

	/*
	 * Make sure there is at least one valid IPC protocol.
	 */
	if (tr_default_entry == TR_NOOP_ENTRY) {
		panic("Network enabled, but no valid IPC transport protocol.");
	}

    }
}
#endif !NeXT_PDO
