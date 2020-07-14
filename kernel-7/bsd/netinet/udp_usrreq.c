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

/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)udp_usrreq.c	8.6 (Berkeley) 5/23/95
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/route.h>

#include <kernserv/machine/spl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#if NEXT
#import <kern/kdebug.h>

#if KDEBUG

#define DBG_LAYER_IN_BEG	NETDBG_CODE(DBG_NETUDP, 0)
#define DBG_LAYER_IN_END	NETDBG_CODE(DBG_NETUDP, 2)
#define DBG_LAYER_OUT_BEG	NETDBG_CODE(DBG_NETUDP, 1)
#define DBG_LAYER_OUT_END	NETDBG_CODE(DBG_NETUDP, 3)
#define DBG_FNC_UDP_INPUT	NETDBG_CODE(DBG_NETUDP, (5 << 8))
#define DBG_FNC_UDP_OUTPUT	NETDBG_CODE(DBG_NETUDP, (6 << 8) | 1)

#endif

#endif


#define N_UDP_HASH_ELEMENTS	(2 * 1024)         /* must be power of 2 */
#define UDP_HASH_MASK           (N_UDP_HASH_ELEMENTS - 1)
#define N_UDP_LPORT_HASH_ELEMENTS	(1024)
#define UDP_LPORT_HASH_MASK		(N_UDP_LPORT_HASH_ELEMENTS - 1)


/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
#ifndef	COMPAT_42
int	udpcksum = 1;
#else
int	udpcksum = 0;		/* XXX */
#endif

struct	sockaddr_in udp_in = { sizeof(udp_in), AF_INET };
struct	inpcb udb;
struct	udpstat udpstat;

struct	inpcb	*udp_hash_array[N_UDP_HASH_ELEMENTS];
struct	inpcb	*udp_lport_hash_array[N_UDP_LPORT_HASH_ELEMENTS];

struct  inpcb_hash_str  udp_hash_str = 
		{udp_hash_array, UDP_HASH_MASK, N_UDP_HASH_ELEMENTS};

struct  inpcb_hash_str  udp_lport_hash_str =
		{udp_lport_hash_array, UDP_LPORT_HASH_MASK, N_UDP_LPORT_HASH_ELEMENTS};

static	void udp_detach __P((struct inpcb *));
static	void udp_notify __P((struct inpcb *, int));
static	struct mbuf *udp_saveopt __P((caddr_t, int, int));
static	struct mbuf *udp_savesockopt __P((caddr_t, int, int, int));

#include <strings.h>

static struct mbuf *
udp_insert_ifinfo(struct mbuf * m)
{
    struct ifnet * ifp = m->m_pkthdr.rcvif;

    if (ifp) {
	char 		buf[IFNAMSIZ * 2];
	struct mbuf * 	mp;
	int		len;
	
	bzero(buf, sizeof(buf));
	sprintf(buf, "%s%d", ifp->if_name, ifp->if_unit);
	len = ALIGN(strlen(buf));
	mp = udp_savesockopt((caddr_t)buf, len, SOL_SOCKET, SO_RCVIF);
	return (mp);
    }
    return (NULL);
}

void
udp_init()
{
	int	i;

	udb.inp_next = udb.inp_prev = &udb;

	for (i=0; i < N_UDP_HASH_ELEMENTS; i++) 
	    udp_hash_array[i]    = 0;

	for (i=0; i < N_UDP_LPORT_HASH_ELEMENTS; i++) 
	    udp_lport_hash_array[i]    = 0;
}

void
udp_input(m, iphlen)
	register struct mbuf *m;
	int iphlen;
{
	register struct ip *ip;
	register struct udphdr *uh;
	register struct inpcb *inp;
	struct mbuf *opts = 0;
	int len;
	struct ip save_ip;

	udpstat.udps_ipackets++;

	KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_START, 0,0,0,0,0);

	/*
	 * Strip IP options, if any; should skip this,
	 * make available to user, and use on returned packets,
	 * but we don't yet have a way to check the checksum
	 * with options still present.
	 */
	if (iphlen > sizeof (struct ip)) {
		ip_stripoptions(m, (struct mbuf *)0);
		iphlen = sizeof(struct ip);
	}

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	ip = mtod(m, struct ip *);
	if (m->m_len < iphlen + sizeof(struct udphdr)) {
		if ((m = m_pullup(m, iphlen + sizeof(struct udphdr))) == 0) {
			udpstat.udps_hdrops++;
			KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
			return;
		}
		ip = mtod(m, struct ip *);
	}
	uh = (struct udphdr *)((caddr_t)ip + iphlen);

	KERNEL_DEBUG(DBG_LAYER_IN_BEG, uh->uh_dport, uh->uh_sport,
		     ip->ip_src.s_addr, ip->ip_dst.s_addr, uh->uh_ulen);

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	len = ntohs((u_short)uh->uh_ulen);
	if (ip->ip_len != len) {
		if (len > ip->ip_len) {
			udpstat.udps_badlen++;
			goto bad;
		}
		m_adj(m, len - ip->ip_len);
		/* ip->ip_len = len; */
	}
	/*
	 * Save a copy of the IP header in case we want restore it
	 * for sending an ICMP error message in response.
	 */
	save_ip = *ip;

	/*
	 * Checksum extended UDP header and data.
	 */
	if (uh->uh_sum) {
		((struct ipovly *)ip)->ih_next = 0;
		((struct ipovly *)ip)->ih_prev = 0;
		((struct ipovly *)ip)->ih_x1 = 0;
		((struct ipovly *)ip)->ih_len = uh->uh_ulen;
		if ((uh->uh_sum = in_cksum(m, len + sizeof (struct ip)))) {
			udpstat.udps_badsum++;
			m_freem(m);
			KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
			return;
		}
	}

	if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr)) ||
	    in_broadcast(ip->ip_dst, m->m_pkthdr.rcvif)) {
		struct socket *last;
		/*
		 * Deliver a multicast or broadcast datagram to *all* sockets
		 * for which the local and remote addresses and ports match
		 * those of the incoming datagram.  This allows more than
		 * one process to receive multi/broadcasts on the same port.
		 * (This really ought to be done for unicast datagrams as
		 * well, but that would cause problems with existing
		 * applications that open both address-specific sockets and
		 * a wildcard socket listening to the same port -- they would
		 * end up receiving duplicates of every unicast datagram.
		 * Those applications open the multiple sockets to overcome an
		 * inadequacy of the UDP socket interface, but for backwards
		 * compatibility we avoid the problem here rather than
		 * fixing the interface.  Maybe 4.5BSD will remedy this?)
		 */

		/*
		 * Construct sockaddr format source address.
		 */
		udp_in.sin_port = uh->uh_sport;
		udp_in.sin_addr = ip->ip_src;
		m->m_len -= sizeof (struct udpiphdr);
		m->m_data += sizeof (struct udpiphdr);
		/*
		 * Locate pcb(s) for datagram.
		 * (Algorithm copied from raw_intr().)
		 */
		last = NULL;
		inp = inet_lport_hash1(&udp_lport_hash_str, uh->uh_dport);
		for (; inp != 0; inp = inp->lport_hash_next) {
			if (inp->inp_laddr.s_addr != INADDR_ANY) {
				if (inp->inp_laddr.s_addr !=
				    ip->ip_dst.s_addr)
					continue;
			}
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				if (inp->inp_faddr.s_addr !=
				    ip->ip_src.s_addr ||
				    inp->inp_fport != uh->uh_sport)
					continue;
			}

			if (last != NULL) {
				struct mbuf *n;

				if ((n = m_copy(m, 0, M_COPYALL)) != NULL) {
				    	struct mbuf *  mp = 0;
				    	KERNEL_DEBUG(DBG_LAYER_IN_END, uh->uh_dport, uh->uh_sport,
						     save_ip.ip_src.s_addr, save_ip.ip_dst.s_addr, uh->uh_ulen);

					if (last->so_options & SO_RCVIF)
					    mp = udp_insert_ifinfo(n);

					if (sbappendaddr(&last->so_rcv,
						(struct sockaddr *)&udp_in,
						n, mp) == 0) {
						m_freem(n);
						if (mp)
						    m_freem(mp);
						udpstat.udps_fullsock++;
					} else
						sorwakeup(last);
				}
			}
			last = inp->inp_socket;
			/*
			 * Don't look for additional matches if this one does
			 * not have either the SO_REUSEPORT or SO_REUSEADDR
			 * socket options set.  This heuristic avoids searching
			 * through all pcbs in the common case of a non-shared
			 * port.  It * assumes that an application will never
			 * clear these options after setting them.
			 */
			if ((last->so_options&(SO_REUSEPORT|SO_REUSEADDR) == 0))
				break;
		}

		if (last == NULL) {
			/*
			 * No matching pcb found; discard datagram.
			 * (No need to send an ICMP Port Unreachable
			 * for a broadcast or multicast datgram.)
			 */
			udpstat.udps_noportbcast++;
			goto bad;
		}

		KERNEL_DEBUG(DBG_LAYER_IN_END, uh->uh_dport, uh->uh_sport,
			     save_ip.ip_src.s_addr, save_ip.ip_dst.s_addr, uh->uh_ulen);

		{
		    struct mbuf * mp = 0;

		    if (last->so_options & SO_RCVIF)
			mp = udp_insert_ifinfo(m);
		    if (sbappendaddr(&last->so_rcv, (struct sockaddr *)&udp_in,
				     m, mp) == 0) {
			udpstat.udps_fullsock++;
			if (mp)
			    m_freem(mp);
			goto bad;
		    }
		}
		sorwakeup(last);
		KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
		return;
	}
	/*
	 * Locate pcb for datagram.
	 */

        inp = inet_hash1(&udp_hash_str, ip->ip_dst.s_addr, uh->uh_dport,
			 ip->ip_src.s_addr, uh->uh_sport);

	inp = hash_in_pcblookup(inp, ip->ip_src, uh->uh_sport,
				ip->ip_dst, uh->uh_dport);
	if (inp == 0) {
		inp = inet_hash1(&udp_hash_str, 0, uh->uh_dport,0,0);
	        inp = hash_in_pcbwild(inp,
				      ip->ip_src, uh->uh_sport,
				      ip->ip_dst, uh->uh_dport, INPLOOKUP_WILDCARD);
/*
		if (inp == 0)
			udpstat.udpps_pcbcachemiss++;
		        inp = in_pcblookup(&udb, ip->ip_src, uh->uh_sport,
					   ip->ip_dst, uh->uh_dport, INPLOOKUP_WILDCARD);
*/
	}


	if (inp == 0) {
		udpstat.udps_noport++;
		if (m->m_flags & (M_BCAST | M_MCAST)) {
			udpstat.udps_noportbcast++;
			goto bad;
		}
		*ip = save_ip;
		ip->ip_len += iphlen;
		icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PORT, 0, 0);
		KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
		return;
	}

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	udp_in.sin_port = uh->uh_sport;
	udp_in.sin_addr = ip->ip_src;

	{
	    struct mbuf **mp = &opts;
	    if (inp->inp_flags & INP_CONTROLOPTS) {

		if (inp->inp_flags & INP_RECVDSTADDR) {
			*mp = udp_saveopt((caddr_t) &ip->ip_dst,
			    sizeof(struct in_addr), IP_RECVDSTADDR);
			if (*mp)
				mp = &(*mp)->m_next;
		}
#ifdef notyet
		/* options were tossed above */
		if (inp->inp_flags & INP_RECVOPTS) {
			*mp = udp_saveopt((caddr_t) opts_deleted_above,
			    sizeof(struct in_addr), IP_RECVOPTS);
			if (*mp)
				mp = &(*mp)->m_next;
		}
		/* ip_srcroute doesn't do what we want here, need to fix */
		if (inp->inp_flags & INP_RECVRETOPTS) {
			*mp = udp_saveopt((caddr_t) ip_srcroute(),
			    sizeof(struct in_addr), IP_RECVRETOPTS);
			if (*mp)
				mp = &(*mp)->m_next;
		}
#endif
	    }
	    
	    if (inp->inp_socket->so_options & SO_RCVIF) { 
		/* insert receive interface info */
		*mp = udp_insert_ifinfo(m);
		if (*mp)
		    mp = &(*mp)->m_next;
	    }
	}
	iphlen += sizeof(struct udphdr);
	m->m_len -= iphlen;
	m->m_pkthdr.len -= iphlen;
	m->m_data += iphlen;

	KERNEL_DEBUG(DBG_LAYER_IN_END, uh->uh_dport, uh->uh_sport,
		     save_ip.ip_src.s_addr, save_ip.ip_dst.s_addr, uh->uh_ulen);

	if (sbappendaddr(&inp->inp_socket->so_rcv, (struct sockaddr *)&udp_in,
	    m, opts) == 0) {
		udpstat.udps_fullsock++;
		goto bad;
	}

	sorwakeup(inp->inp_socket);
	
	KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
	return;
bad:
	m_freem(m);
	if (opts)
		m_freem(opts);

	KERNEL_DEBUG(DBG_FNC_UDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
}

/*
 * Create a "control" mbuf containing the specified data
 * with the specified type/level for presentation with a datagram.
 */
struct mbuf *
udp_savesockopt(p, size, level, type)
	caddr_t p;
	register int size;
        int level;
	int type;
{
	register struct cmsghdr *cp;
	struct mbuf *m;

	if ((m = m_get(M_DONTWAIT, MT_CONTROL)) == NULL)
		return ((struct mbuf *) NULL);
	cp = (struct cmsghdr *) mtod(m, struct cmsghdr *);
	bcopy(p, CMSG_DATA(cp), size);
	size += sizeof(*cp);
	m->m_len = size;
	cp->cmsg_len = size;
	cp->cmsg_level = level;
	cp->cmsg_type = type;
	return (m);
}

struct mbuf *
udp_saveopt(p, size, type)
	caddr_t p;
	register int size;
	int type;
{
    return (udp_savesockopt(p, size, IPPROTO_IP, type));
}


/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static void
udp_notify(inp, errno)
	register struct inpcb *inp;
	int errno;
{
	inp->inp_socket->so_error = errno;
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}

void
udp_ctlinput(cmd, sa, ip)
	int cmd;
	struct sockaddr *sa;
	register struct ip *ip;
{
	register struct udphdr *uh;
	extern struct in_addr zeroin_addr;
	extern u_char inetctlerrmap[];

	if (!PRC_IS_REDIRECT(cmd) &&
	    ((unsigned)cmd >= PRC_NCMDS || inetctlerrmap[cmd] == 0))
		return;
	if (ip) {
		uh = (struct udphdr *)((caddr_t)ip + (ip->ip_hl << 2));
		in_pcbnotify(&udb, sa, uh->uh_dport, ip->ip_src, uh->uh_sport,
			cmd, udp_notify);
	} else
		in_pcbnotify(&udb, sa, 0, zeroin_addr, 0, cmd, udp_notify);
}

int
udp_output(inp, m, addr, control)
	register struct inpcb *inp;
	register struct mbuf *m;
	struct mbuf *addr, *control;
{
	register struct udpiphdr *ui;
	register int len = m->m_pkthdr.len;
	struct in_addr laddr;
	int s, error = 0;


	KERNEL_DEBUG(DBG_FNC_UDP_OUTPUT | DBG_FUNC_START, 0,0,0,0,0);

	if (control)
		m_freem(control);		/* XXX */


	KERNEL_DEBUG(DBG_LAYER_OUT_BEG, inp->inp_fport, inp->inp_lport,
		     inp->inp_laddr.s_addr, inp->inp_faddr.s_addr,
		     (htons((u_short)len + sizeof (struct udphdr))));

	if (addr) {
		laddr = inp->inp_laddr;
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			goto release;
		}
		/*
		 * Must block input while temporarily connected.
		 */
		s = splnet();
		error = in_pcbconnect(inp, addr);
		if (error) {
			splx(s);
			goto release;
		}
	} else {
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			goto release;
		}
	}
	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */
	M_PREPEND(m, sizeof(struct udpiphdr), M_DONTWAIT);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)len + sizeof (struct udphdr));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = ui->ui_len;

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum) {
	    if ((ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + len)) == 0)
		ui->ui_sum = 0xffff;
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = inp->inp_ip.ip_ttl;	/* XXX */
	((struct ip *)ui)->ip_tos = inp->inp_ip.ip_tos;	/* XXX */
	udpstat.udps_opackets++;


	KERNEL_DEBUG(DBG_LAYER_OUT_END, ui->ui_dport, ui->ui_sport,
		     ui->ui_src.s_addr, ui->ui_dst.s_addr, ui->ui_ulen);

	error = ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST),
	    inp->inp_moptions);

	if (addr) {
		in_pcbdisconnect(inp);
		inp->inp_laddr = laddr;
		hash_insert(inp);
		splx(s);
	}

	KERNEL_DEBUG(DBG_FNC_UDP_OUTPUT | DBG_FUNC_END, error, 0,0,0,0);
	return (error);

release:
	m_freem(m);
	KERNEL_DEBUG(DBG_FNC_UDP_OUTPUT | DBG_FUNC_END, error, 0,0,0,0);
	return (error);
}

u_long	udp_sendspace = 9216;		/* really max datagram size */
u_long	udp_recvspace = 40 * (1024 + sizeof(struct sockaddr_in));
					/* 40 1K datagrams */

/*ARGSUSED*/
int
udp_usrreq(so, req, m, addr, control)
	struct socket *so;
	int req;
	struct mbuf *m, *addr, *control;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;
	int s;

	if (req == PRU_CONTROL)
		return (in_control(so, (u_long)m, (caddr_t)addr,
			(struct ifnet *)control));
	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	/*
	 * Note: need to block udp_input while changing
	 * the udp pcb queue and/or pcb addresses.
	 */
	switch (req) {

	case PRU_ATTACH:
		if (inp != NULL) {
			error = EINVAL;
			break;
		}
		s = splnet();
		error = in_pcballoc(so, &udb, &udp_hash_str, &udp_lport_hash_str);
		splx(s);
		if (error)
			break;
		error = soreserve(so, udp_sendspace, udp_recvspace);
		if (error)
			break;
		((struct inpcb *) so->so_pcb)->inp_ip.ip_ttl = ip_defttl;
		break;

	case PRU_DETACH:
		udp_detach(inp);
		break;

	case PRU_BIND:
		s = splnet();
		error = in_pcbbind(inp, addr);
		if (error == 0) {
			hash_insert(inp);
		}

		splx(s);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		s = splnet();

		if (inp->hash_element >= 0)
		    hash_remove(inp);

		error = in_pcbconnect(inp, addr);
		splx(s);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		s = splnet();
		in_pcbdisconnect(inp);
		inp->inp_laddr.s_addr = INADDR_ANY;
		hash_insert(inp);
		splx(s);
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:
		return (udp_output(inp, m, addr, control));

	case PRU_ABORT:
		soisdisconnected(so);
		udp_detach(inp);
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, addr);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, addr);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error =  EOPNOTSUPP;
		break;

	case PRU_RCVD:
	case PRU_RCVOOB:
		return (EOPNOTSUPP);	/* do not free mbuf's */

	default:
		panic("udp_usrreq");
	}

release:
	if (control) {
		printf("udp control data unexpectedly retained\n");
		m_freem(control);
	}
	if (m)
		m_freem(m);
	return (error);
}

static void
udp_detach(inp)
	struct inpcb *inp;
{
	int s = splnet();

	in_pcbdetach(inp);
	splx(s);
}

/*
 * Sysctl for udp variables.
 */
int
udp_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case UDPCTL_CHECKSUM:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &udpcksum));
	default:
		return (ENOPROTOOPT);
	}
	/* NOTREACHED */
}
