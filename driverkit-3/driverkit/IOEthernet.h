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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Device independent abstract superclass for Ethernet.
 *
 * HISTORY
 *
 * 25 September 1992 David E. Bohman at NeXT
 *	Created.
 */

#ifdef	KERNEL

#import <sys/socket.h>
#import <net/if.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/IONetwork.h>
#import <kernserv/ns_timer.h>
#import <kernserv/queue.h>
#import	<netinet/in.h>
#import <net/etherdefs.h>
#import <machine/label_t.h>


@interface IOEthernet:IODirectDevice<IONetworkDeviceMethods>
{
@private
    BOOL		_isRunning;
    BOOL		_promiscEnabled;
    id			_driverCmd;
    ns_time_t		_absTimeout;
    queue_head_t	_multicastQueue; 		// queue of multicast addresses
    IONetwork		*_netif;
    enet_addr_t		_ethernetAddress;
    int             	_net_buf_type;
    struct arpcom *	en_arpcom;
    int			_IOEthernet_reserved[ 8 ];
}

- initFromDeviceDescription:(IODeviceDescription *)devDesc;
- free;

- (BOOL)isRunning;
- (void)setRunning:(BOOL)running;

- (unsigned int)relativeTimeout;

- (void)setRelativeTimeout:(unsigned int)timeout;

- (void)clearTimeout;

- (BOOL)isUnwantedMulticastPacket:(ether_header_t *)header; 

- (void)performLoopback:(netbuf_t)pkt;

- (id)getDriverCmd;
- (struct ifnet *)allocateIfnet;

@end

@interface IOEthernet(DriverInterface)

- (BOOL)resetAndEnable:(BOOL)enable;
- (IONetwork *)attachToNetworkWithAddress:(enet_addr_t)addrs;

- (void)transmit:(netbuf_t)pkt;

- (BOOL)enablePromiscuousMode;	 
- (void)disablePromiscuousMode;	 
- (BOOL)enableMulticastMode;	 
- (void)disableMulticastMode;	 

- (void)addMulticastAddress:(enet_addr_t *)addr;
- (void)removeMulticastAddress:(enet_addr_t *)addr;

@end

#endif
