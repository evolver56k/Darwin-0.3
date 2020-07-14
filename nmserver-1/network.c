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


#include <stdio.h>
#include <errno.h>

#if defined(WIN32)
#include <winnt-pdo.h>
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/file.h>
#include <netdb.h>

#if defined(NeXT)
#include <libc.h>
#include <machdep/machine/features.h>
#else
#include <unistd.h>
#endif

#ifdef __svr4__
#include <sys/sockio.h>
#endif

#endif WIN32

#include <mach/mach.h>

#include "netmsg.h"
#include "network.h"
#include <servers/nm_defs.h>
#include "nm_extra.h"
#include "uid.h"
#include "access_list.h"

#if !defined(BSD) || BSD < 199506
#define IFR_NEXT(ifr)	((ifr) + 1)
#else
/* BSD 4.4 has variably sized ifreq's depending on the embedded
 * sockaddr's sa_len field.
 */
#define IFR_NEXT(ifr)	\
    ((struct ifreq *) ((char *) (ifr) + sizeof(*(ifr)) + \
      MAX(0, (int) (ifr)->ifr_addr.sa_len - (int) sizeof((ifr)->ifr_addr))))
#endif

char		my_host_name[HOST_NAME_SIZE];
netaddr_t	my_host_id;
netaddr_t	broadcast_address;
short		last_ip_id;

#ifdef WIN32
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif


/*
 * init_network
 *	Initialises some things for the network-level interface.
 *
 * Returns:
 *	TRUE or FALSE.
 *
 * Side effects:
 *	Sets up my_host_name, my_host_id, last_ip_id and broadcast_address.
 *
 */
PUBLIC boolean_t network_init()
{
#ifdef NeXT_PDO
	struct hostent		*hp;
#else
	int			s;
	ip_addr_t		temp_address;
	char			buf[1000];
	struct ifconf		ifc;
        struct ifreq		*ifr = NULL;
	int			n;
	boolean_t		found_bcast = FALSE;
	FILE			*nm_fp;
#endif

	if (nevernet_flag) RETURN(FALSE);
	ERROR((msg, "network_init"));

#ifdef WIN32
	{
		WORD ver = MAKEWORD(1,1);
		WSADATA data;
		if (WSAStartup(ver, &data)) {
			ERROR((msg, "WINSOCK initialization failure = %d",
			       WSAGetLastError()));
			RETURN(FALSE);
		}
	}
#endif WIN32

	/*
	 * Make sure all important variables get a sensible value no
	 * matter what happens.
	 */
	my_host_id = INADDR_ANY;
	broadcast_address = INADDR_BROADCAST;
	last_ip_id = (short)uid_get_new_uid();

	/*
	 * Find the local IP address.
	 */
	if ((gethostname(my_host_name, HOST_NAME_SIZE)) != 0) {
#ifndef WIN32
            panic("network_init.gethostname");
#else
            /* On Windows, return FALSE which will record lack of network. */
            RETURN(FALSE);
#endif
        }

#ifdef NeXT_PDO
	hp = gethostbyname(my_host_name);

	if (hp == 0) {
		ERROR((msg,"network_init.gethostbyname fails errno=%d",ERRNO));
		RETURN(FALSE);
	} else {
		my_host_id = *(netaddr_t *)(hp->h_addr);
	}
#else
	/*
	 * Do not call gethostbyname() to get our own address, because
	 * it may invoke services that are not running yet.
	 */
#endif


#ifndef NeXT_PDO
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    ERROR((msg,"network_init.socket fails, errno=%d", ERRNO));
		RETURN(FALSE);
	}

	/* Get a list of all available interfaces and their addresses.
	 */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t) buf;
	if ((n = ioctl(s, SIOCGIFCONF, (char *) &ifc)) < 0 ) {
	    ERROR((msg, "network_init.ioctl(SIOCGIFCONF) fails, errno=%d", 
		   ERRNO));
		close(s);
		RETURN(FALSE);
	}

	/* Find the first non-loopback Internet interface that is up.
	 */
	my_host_id = 0;
	ifr = ifc.ifc_req;
	for (n = 0; n < ifc.ifc_len / sizeof(struct ifreq);
	     n++, ifr = IFR_NEXT(ifr)) {
	    struct ifreq ifreq2;

	    /* Not interested in non-Internet interfaces
	     */
	    if (ifr->ifr_addr.sa_family != AF_INET)
			continue;

	    /* Get the interface's flags.
	     */
            memmove(&ifreq2.ifr_name, &ifr->ifr_name, IFNAMSIZ);
	    if (ioctl(s, SIOCGIFFLAGS, &ifreq2) < 0) {
		ERROR((msg, "network_init.ioctl(SIOCGIFFLAGS) fails, errno=%d",
									ERRNO));
		continue;
		}

	    /* Ignore it if it isn't up.
		 */
	    if ((ifreq2.ifr_flags & IFF_UP) == 0)
		continue;

	    /* Got a good one; remember it's address.
	     */
	    my_host_id =
		((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;

	    /* Stop looking if it is a "real" interface.
	     */
	    if ((ifreq2.ifr_flags & IFF_LOOPBACK) == 0)
			break;
		}

	if (my_host_id == 0) {
		ERROR((msg,
	"Could not find a working network interface - disabling the network"));
		close(s);
		RETURN(FALSE);
	}
#endif !NeXT_PDO

#ifndef NeXT_PDO

#ifdef SIOCGIFBRDADDR
	/*
	 * Find the socket broadcast address.
	 */
	if (ioctl(s, SIOCGIFBRDADDR, (char *)ifr) < 0) {
		ERROR((msg,"Cannot get the socket broadcast address for interface %s: errno=%d", ifr->ifr_name, ERRNO));
	} else {
		found_bcast = TRUE;
		broadcast_address =
		    ((struct sockaddr_in *) &ifr->ifr_broadaddr)->
			sin_addr.s_addr;
	}
#endif SIOCGIFBRDADDR

	close(s);

	/*
	 * Decide on the broadcast address.
	 */
	nm_fp = fopen("nmbroadcast","r");
	if (nm_fp == NULL) {
		nm_fp = fopen("/etc/nmbroadcast","r");
	}
	if (nm_fp != NULL) {
		int	a,b,c,d;
		int	ret;

		ret = fscanf(nm_fp,"%d.%d.%d.%d\n",&a,&b,&c,&d);
		temp_address.ia_bytes.ia_net_owner = a;
		temp_address.ia_bytes.ia_net_node_type = b;
		temp_address.ia_bytes.ia_host_high = c;
		temp_address.ia_bytes.ia_host_low = d;
		fclose(nm_fp);
		if (ret != 4) {
			ERROR((msg,"** Invalid format for nmbroadcast file."));
			RETURN(FALSE);
		}
		broadcast_address = temp_address.ia_netaddr;
		found_bcast = TRUE;
	}

	if (found_bcast == FALSE) {
		/*
		 * Use the CMU default.
		 */
		ERROR((msg,
  "Warning: could not find a useful broadcast address, using 255.255.255.255"));
		temp_address.ia_bytes.ia_net_owner = 255;
		temp_address.ia_bytes.ia_net_node_type = 255;
		temp_address.ia_bytes.ia_host_high = 255;
		temp_address.ia_bytes.ia_host_low = 255;
		broadcast_address = temp_address.ia_netaddr;
	}

	access_init();

#endif !NeXT_PDO

	{
		char buf [BUFSIZ];
		sprintf (buf, "Broadcast address: 0x%x", (int)broadcast_address);
	}
	RETURN(TRUE);
}




/*
 * udp_checksum
 *	Calculate a UDP checksum.
 *
 * Parameters:
 *	base	: pointer to data to be checksummed.
 *	length	: number of bytes to be checksummed.
 *
 * Returns:
 *	The checksum.
 *
 */

/*
 * Checksum routine for Internet Protocol family headers (NON-VAX Version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 * This particular version is a quick hack which needs to be rewritten.
 */

/*
 * FIXME ??? 8/12/93 pfrantz@NeXT
 * This routine appears to give an incorrect checksum on m68k when the
 * length is odd.  Looks OK on i386.  We probably have never seen the
 * problem because we only happen to send even length datagram and 
 * SRR packets.  The same routine is used for send and receive, so
 * problems would only show up between unlike machines.
 *
 * AOF - applied Paul's idea Wed Nov  9 14:29:44 PST 1994
 */

int udp_checksum(base, length)
	register unsigned short *base;
	register int length;
{
	register int sum = 0;

	while (length >= 2) {
		sum += *base++;
		if (sum > 0xffff)
			sum -= 0xffff;
		length -= 2;
	}
	if (length == 1) {
		sum += (*base) & ntohs(0xff00);
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	return sum ^ 0xffff;
}
