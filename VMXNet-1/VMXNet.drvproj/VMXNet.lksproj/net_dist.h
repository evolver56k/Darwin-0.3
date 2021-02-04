/* **********************************************************
 * Copyright 2004 VMware, Inc.  All rights reserved. 
 * -- VMware Confidential
 * **********************************************************/

/*
 * net_dist.h --
 *
 *      Networking headers.
 */

#ifndef _NET_DIST_H_
#define _NET_DIST_H_

typedef uint32 Net_PortID;
/*
 * Set this to anything, and that value will never be assigned as a 
 * PortID for a valid port, but will be assigned in most error cases.
 */
#define NET_INVALID_PORT_ID      0

#endif
