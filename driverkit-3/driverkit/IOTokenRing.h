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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOTokenRing.h - TokenRing device-independent superclass
 *
 *	The TokenRing superclass contains the device-independent functions
 *	that are common to most token-ring drivers.  The hardware-specific
 *	driver should be a subclass of IOTokenRing.
 *
 * HISTORY
 * 25-Jan-93 
 *	created
 *      
 */

#ifdef KERNEL

#import <driverkit/IODirectDevice.h>
#import <driverkit/IONetwork.h>
#import <kernserv/ns_timer.h>
#import <net/tokendefs.h>


@interface IOTokenRing:IODirectDevice<IONetworkDeviceMethods>
{
@private

    /* 
     * Interface flags 
     */
	
    struct _trFlags {
	unsigned int        _isRunning:1;		// operational or not
	unsigned int        _isEarlyTokenEnabled:1;	// early token on/off
	unsigned int        _shouldAutoRecover:1;	// auto recovery on/off
	unsigned int        _shouldAttachIP:1;		// tr to ip iface
	unsigned int        _shouldAttachNullSap:1;	// null sap iface
	unsigned int        _doesIPSourceRouting:1;	// ip 802.5 src rtng
	unsigned int        RESERVED:10;
    }	_flags;

    /* 
     * Interface parameters 
     */
	
    unsigned 	_ringSpeed;		// ring speed (4 or 16)
    unsigned	_ipTokenPriority;	// IP token priority
    unsigned	_ipMtu;			// IP max tran unit for tring
    unsigned	_maxInfoFieldSize;	// MAC-layer info field max
    
    /*
     * Hardware addresses
     */
	
    token_addr_t	_nodeAddress; 		// our Node address
    token_addr_t	_groupAddress;		// not supported
    token_addr_t 	_functionalAddress;	// not supported
	    
    /*
     * Other instance variables
     */

    IONetwork	*_netif;
    id		_driverCmd;		 
    ns_time_t 	_absTimeout;	
    id		_hwLock;	// NXSpinLock; protects access to hdw
    int		_IOTokenRing_reserved[4];
}

- initFromDeviceDescription:(IODeviceDescription *)devDesc;
- free;

- (BOOL)isRunning;
- (void)setRunning:(BOOL)running;

- (unsigned int)relativeTimeout;
- (void)setRelativeTimeout:(unsigned int)timeout;
- (void)clearTimeout;

- (BOOL)earlyTokenEnabled;			// defined in Instance.table
- (BOOL)shouldAutoRecover;			// defined in Instance.table
- (unsigned)ringSpeed;				// defined in Instance.table

- (unsigned)maxInfoFieldSize;			// based on ring speed, but may
- (void)setMaxInfoFieldSize:(unsigned)size;	// also be hardware dependent

/*
 * Hardware Addresses
 */

- (token_addr_t)nodeAddress;				// our Node address

@end

@interface IOTokenRing(DriverInterface)

- (BOOL)resetAndEnable:(BOOL)enable;
- (IONetwork *)attachToNetworkWithAddress:(token_addr_t)addrs;

/*
 * NOTE:  Use of the transmit: method may be undesirable in cases
 *        where it forces buffer copying, or precludes a subclass
 *	  buffer management scheme.  In those cases it is preferable
 *	  to override the outputPacket:address: method (and perhaps the 
 *	  allocateNetbuf method).  If the outputPacket:address: method is 
 *	  overridden, then no transmit method is required.	 
 */
 
- (void)transmit:(netbuf_t)pkt; 


@end

#endif
