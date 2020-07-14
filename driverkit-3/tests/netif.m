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
 * netif.m - netif stub for testing NetDriver objects.
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
#import <driverkit/IONetDevice.h>
#import <net/if.h>
#import <bsd/sys/errno.h>
#import <bsd/sys/ioctl.h>
#import <driverkit/generalFuncs.h>
#import <libc.h>

/*
 * Horrible hack; these are kenel globals.
 */
const char IFCONTROL_SETFLAGS[] = "setflags";
const char IFCONTROL_SETADDR[] = "setaddr";
const char IFCONTROL_GETADDR[] = "getaddr";
const char IFCONTROL_AUTOADDR[] = "autoaddr";
const char IFCONTROL_UNIXIOCTL[] = "unix-ioctl";

/*
 * This emulates all the calls to netif.c which a typical driver will make
 * except for if_handle_input(); the client test program must implement
 * that function to provide dispatch of incoming buffers.
 *
 * The client test program can define any arbitrary 'netbuf' struct desired.
 * This module does not manipulate netbufs.
 * We happen to use ifnet for internal representation for convenience - a lot
 * of this code is just cloned from the kernel's netif.c.
 */

int if_output(netif_t netif, netbuf_t packet, void *addr)
{
	struct ifnet *ifp = (struct ifnet *)netif;

	if (ifp->if_output == NULL) {
		return (ENXIO);
	} else {
		return (ifp->if_output(netif, packet, addr));
	}
}

int if_init(netif_t netif)
{
	struct ifnet *ifp = (struct ifnet *)netif;

	if (ifp->if_init == NULL) {
		return (ENXIO);
	} else {
		return (ifp->if_init(netif));
	}
}

int if_control(netif_t netif, const char *command, void *data)
{
	struct ifnet *ifp = (struct ifnet *)netif;

	if (ifp->if_control == NULL) {
		return (ENXIO);
	} else {
		return (ifp->if_control(netif, command, data));
	}
}

int if_ioctl(netif_t netif, unsigned command, void *data)
{
	struct ifnet *ifp = (struct ifnet *)netif;
	struct ifreq *ifr = (struct ifreq *)data;
	if_ioctl_t ioctl_stuff;

	if (ifp->if_control == NULL) {
		return (ENXIO);
	} 
	switch (command) {
	case SIOCAUTOADDR:
		return (ifp->if_control(netif, IFCONTROL_AUTOADDR,
					(void *)&ifr->ifr_ifru));
					
		
	case SIOCSIFADDR:
		/*
		 * XXX: IP calls this incorrectly with an ifaddr
		 * struct instead of an ifreq struct
		 */
		return (ifp->if_control(netif, IFCONTROL_SETADDR,
					(void *)data));
		
	case SIOCGIFADDR:
		return (ifp->if_control(netif, IFCONTROL_GETADDR,
					(void *)&ifr->ifr_ifru));
		
	case SIOCSIFFLAGS:
		return (ifp->if_control(netif, IFCONTROL_SETFLAGS,
					(void *)&ifr->ifr_ifru));
		
	default:
		ioctl_stuff.ioctl_command = command;
		ioctl_stuff.ioctl_data = data;
		return (ifp->if_control(netif, IFCONTROL_UNIXIOCTL, 
					(void *)&ioctl_stuff));
	}
}

netbuf_t if_getbuf(netif_t netif)
{
	struct ifnet *ifp = (struct ifnet *)netif;

	if (ifp->if_getbuf == NULL) {
		return (NULL);		/* should never happen */
	} else {
		return (ifp->if_getbuf(netif));
	}
}

netif_t if_attach(if_init_func_t init_func, 
			 if_input_func_t input_func,
			 if_output_func_t output_func,
			 if_getbuf_func_t getbuf_func,
			 if_control_func_t control_func,
			 const char *name,
			 unsigned unit,
			 const char *type,
			 unsigned mtu,
			 unsigned flags,
			 netif_class_t class,
			 void *private)
{
	struct ifnet *ifp;

	ifp = (struct ifnet *)IOMalloc(sizeof(struct ifnet));
	bzero((void *)ifp, sizeof(struct ifnet));
	ifp->if_name = (char *)name;
	ifp->if_type = (char *)type;
	ifp->if_unit = unit;
	ifp->if_mtu = mtu;
	ifp->if_flags = flags;
	ifp->if_snd.ifq_maxlen = /* ifqmaxlen */ IFQ_MAXLEN;
	ifp->if_init = init_func;
	ifp->if_output = output_func;
	ifp->if_control = control_func;
	ifp->if_input = input_func;
	ifp->if_getbuf = getbuf_func;
	ifp->if_private = private;
	ifp->if_class = class;
	
#ifdef	notdef
	/* we don't need this. */
	for (ifpp = &ifnet; 
	     *ifpp != NULL && (*ifpp)->if_class >= class;
	     ifpp = &(*ifpp)->if_next) {
	}
	ifp->if_next = *ifpp;
	*ifpp = ifp;	
	if (ifp->if_class == NETIFCLASS_REAL) {
		pingvirtuals((netif_t)ifp);
	}
#endif	notdef
	return ((netif_t)ifp);
}

void if_registervirtual(if_attach_func_t attach_func, void *private)
{
	return;		// ??
}
		  
void *
if_private(
	   netif_t netif
	   )
{
	return (((struct ifnet *)netif)->if_private);
}

unsigned
if_unit(
	netif_t netif
	)
{
	return (((struct ifnet *)netif)->if_unit);
}

const char *
if_name(
	netif_t netif
	)
{
	return (((struct ifnet *)netif)->if_name);
}

const char *
if_type(
	netif_t netif
	)
{
	return (((struct ifnet *)netif)->if_type);
}

unsigned
if_mtu(
       netif_t netif
       )
{
	return (((struct ifnet *)netif)->if_mtu);
}

unsigned
if_flags(
	 netif_t netif
	 )
{
	return (((struct ifnet *)netif)->if_flags);
}

unsigned
if_opackets(
	    netif_t netif
	    )
{
	return (((struct ifnet *)netif)->if_opackets);
}

unsigned
if_ipackets(
	    netif_t netif
	    )
{
	return (((struct ifnet *)netif)->if_ipackets);
}

unsigned
if_oerrors(
	   netif_t netif
	   )
{
	return (((struct ifnet *)netif)->if_oerrors);
}

unsigned
if_ierrors(
	   netif_t netif
	   )
{
	return (((struct ifnet *)netif)->if_ierrors);
}

unsigned
if_collisions(
	      netif_t netif
	      )
{
	return (((struct ifnet *)netif)->if_collisions);
}

void
if_flags_set(
	     netif_t netif,
	     unsigned flags
	     )
{
	((struct ifnet *)netif)->if_flags = flags;
}

void
if_opackets_set(
		netif_t netif,
		unsigned opackets
		)
{
	((struct ifnet *)netif)->if_opackets = opackets;
}

void
if_ipackets_set(
		netif_t netif,
		unsigned ipackets
		)
{
	((struct ifnet *)netif)->if_ipackets = ipackets;
}

void
if_oerrors_set(
		netif_t netif,
		unsigned oerrors
		)
{
	((struct ifnet *)netif)->if_oerrors = oerrors;
}

void
if_ierrors_set(
		netif_t netif,
		unsigned ierrors
		)
{
	((struct ifnet *)netif)->if_oerrors = ierrors;
}

void
if_collisions_set(
		netif_t netif,
		unsigned collisions
		)
{
	((struct ifnet *)netif)->if_collisions = collisions;
}
