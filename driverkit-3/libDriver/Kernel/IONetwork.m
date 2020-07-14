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
 * Network interface class.
 *
 * HISTORY
 *
 * 1-Nov-93	John Immordino (jimmord) at NeXT
 * 	Added API to allow read access to counters and to increment counters
 * 	by an arbitrary number.  The latter is useful for hardware which
 * 	reports statistics in bursts. 
 *
 * 11 December 1992 David E. Bohman at NeXT
 *	Created.
 */
#import	<sys/types.h>
#import <sys/socket.h>
#import <sys/param.h>
#import <driverkit/IONetwork.h>
#import <sys/mbuf.h>
#import <net/if.h>
#import <netinet/in.h>
#import	<netinet/if_ether.h>
#import <sys/kernel.h>


static const char IFNAME_NULL[] = "null";

@implementation IONetwork

- initForNetworkDevice:device
	name:(const char *)name
	unit:(unsigned int)unit
	type:(const char *)type
	maxTransferUnit:(unsigned int)mtu
	flags:(unsigned int)flags
{
 	struct ifnet *ifp;
	int reuse;

   [super init];

	ifp = [device allocateIfnet];
	if(ifp == (struct ifnet *)NULL) return (0);
	bzero((caddr_t)ifp, sizeof(struct ifnet));

	ifp->if_name = (char *)name;
	ifp->if_type = (char *)type;
	ifp->if_unit = unit;
	ifp->if_mtu = mtu;
	ifp->if_flags = flags;
	ifp->if_private = device;
	
#if defined(i386) && 0							// XXX fix later
		/*************************************************
	 	 * This code sets the hostid global variable for *
		 * the i386 achitecure only.  Refer to Tracker   *
		 * #35253 - JI					 *
		 *************************************************/
		 
		 /*
		  * Do this only if hostid hasn't been set already.
		  */
		 if (hostid == 0) {
		    unsigned char hardwareAddr[6];	
		    
		    /*
			* Get the hardware address for this interface.
			*/
		    if (if_control((netif_t)ifp, IFCONTROL_GETADDR,
			(void *)&hardwareAddr) == 0) {
			
			/* 
			    * Xor the first 2 bytes into the 
			    * middle 2 bytes, then set hostid.
			    */	
			hardwareAddr[2] ^= hardwareAddr[0];
			hardwareAddr[3] ^= hardwareAddr[1];
			hostid = NXSwapHostLongToBig(*(long *)&hardwareAddr[2]);
		    }
		 }
#endif i386

    _ifp = ifp;
    _device = device;		    

    return self;
}

- free
{
    return [super free];
}

- (int)handleInputPacket:(netbuf_t)pkt extra:(void *)extra
{
    if (_device == nil)
		return -1;

    return [_device inputPacket:pkt extra:(void *)extra];
}

- (unsigned)inputPackets
{
	return (_ifp->if_ipackets);
}

- (void)incrementInputPackets
{
	_ifp->if_ipackets++;

}

- (void)incrementInputPacketsBy:(unsigned)increment
{
	_ifp->if_ipackets += increment;
}

- (unsigned)inputErrors
{
	return (_ifp->if_ierrors);
}

- (void)incrementInputErrors
{
	_ifp->if_ierrors++;
}

- (void)incrementInputErrorsBy:(unsigned)increment
{
	_ifp->if_ierrors += increment;
}

- (unsigned)outputPackets
{
	return (_ifp->if_opackets);
}

- (void)incrementOutputPackets
{
	_ifp->if_opackets++;
}

- (void)incrementOutputPacketsBy:(unsigned)increment
{
	_ifp->if_opackets += increment;
}

- (unsigned)outputErrors
{
	return (_ifp->if_oerrors);
}

- (void)incrementOutputErrors
{
	_ifp->if_oerrors++;
}

- (void)incrementOutputErrorsBy:(unsigned)increment
{
	_ifp->if_oerrors += increment;
}

- (unsigned)collisions
{
	return (_ifp->if_collisions);
}

- (void)incrementCollisions
{
	_ifp->if_collisions++;
}

- (void)incrementCollisionsBy:(unsigned)increment
{
	_ifp->if_collisions += increment;
}

- (struct ifnet *)getIONetworkIfnet
{
	return(_ifp);
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    strcpy( classes, IOClassNetwork);
    return( self);
}

@end

