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
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
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
 *	@(#)in_pcb.c	8.4 (Berkeley) 5/24/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#include <kern/kdebug.h>


#if KDEBUG

#define DBG_FNC_PCB_LOOKUP	NETDBG_CODE(DBG_NETTCP, (6 << 8))
#define DBG_FNC_PCB_HLOOKUP	NETDBG_CODE(DBG_NETTCP, ((6 << 8) | 1))

#endif


struct zone   *inpcb_zone;
struct	in_addr zeroin_addr;




void
lport_hash_remove(inp)
        register struct inpcb *inp;
{
	if ((*inp->lport_hash_str->hash_array)[inp->lport_hash_element] == inp)
		(*inp->lport_hash_str->hash_array)[inp->lport_hash_element] = inp->lport_hash_next;

	if (inp->lport_hash_prev) 
	        inp->lport_hash_prev->lport_hash_next = inp->lport_hash_next;

	if (inp->lport_hash_next) 
		inp->lport_hash_next->lport_hash_prev = inp->lport_hash_prev;

	inp->lport_hash_next = 0;
	inp->lport_hash_prev = 0;
	inp->lport_hash_element = -1;
}




void
lport_hash_insert(inp)
	register struct inpcb *inp;

{
        register u_long	h_idx;

	if (inp->lport_hash_element != -1)
	        return;
		    
	h_idx = inp->inp_lport & inp->lport_hash_str->hash_mask;

	if ((*inp->lport_hash_str->hash_array)[h_idx]) {
	        (*inp->lport_hash_str->hash_array)[h_idx]->lport_hash_prev = inp;
		inp->lport_hash_next = (*inp->lport_hash_str->hash_array)[h_idx];
		inp->lport_hash_prev = 0;
		(*inp->lport_hash_str->hash_array)[h_idx] = inp;
	} else {
	        (*inp->lport_hash_str->hash_array)[h_idx] = inp;
		inp->lport_hash_next = 0;
		inp->lport_hash_prev = 0;
	}

	inp->lport_hash_element = h_idx;
}




void
hash_remove(inp)
        register struct inpcb *inp;
{
	if ((*inp->hash_str->hash_array)[inp->hash_element] == inp)
		(*inp->hash_str->hash_array)[inp->hash_element] = inp->hash_next;

	if (inp->hash_prev) 
	        inp->hash_prev->hash_next = inp->hash_next;

	if (inp->hash_next) 
		inp->hash_next->hash_prev = inp->hash_prev;

	inp->hash_next = 0;
	inp->hash_prev = 0;
	inp->hash_element = -1;
}



void
hash_insert(inp)
        register struct inpcb *inp;
{
        register u_long	h_idx;

	if (inp->hash_element != -1)
	        return;

	h_idx = (inp->inp_lport + inp->inp_fport + inp->inp_faddr.s_addr) & inp->hash_str->hash_mask;

	if ((*inp->hash_str->hash_array)[h_idx]) {
	        (*inp->hash_str->hash_array)[h_idx]->hash_prev = inp;
		inp->hash_next = (*inp->hash_str->hash_array)[h_idx];
		inp->hash_prev = 0;
		(*inp->hash_str->hash_array)[h_idx] = inp;
	} else {
	        (*inp->hash_str->hash_array)[h_idx] = inp;
		inp->hash_next = 0;
		inp->hash_prev = 0;
	}
	inp->hash_element = h_idx;
}


struct inpcb *inet_hash1(hash_str, l_addr, l_port, f_addr, f_port)
	struct inpcb_hash_str *hash_str;
        u_long	l_addr;
	u_short	l_port;
	u_long	f_addr;
	u_short	f_port;
{
	return ((*hash_str->hash_array)[(l_port + f_port + f_addr) & hash_str->hash_mask]);
}


struct inpcb *inet_lport_hash1(lport_hash_str, l_port)
	struct inpcb_hash_str *lport_hash_str;
	u_short  l_port;

{
	return ((*lport_hash_str->hash_array)[l_port & lport_hash_str->hash_mask]);
}



int
in_pcballoc(so, head, hash_str, lport_hash_str)
	struct socket *so;
	struct inpcb *head;
	struct inpcb_hash_str *hash_str;
	struct inpcb_hash_str *lport_hash_str;
{
	register struct inpcb *inp;
	caddr_t		      temp;

	if (so->cached_in_sock_layer == 0) {
#if TEMPDEBUG
	    printf("PCBALLOC calling zalloc for socket %x\n", so);
#endif
	    inp = zalloc(inpcb_zone);
	    if (inp == NULL)
		return (ENOBUFS);
	    bzero((caddr_t)inp, sizeof(*inp));
	}
	else {
#if TEMPDEBUG
	    printf("PCBALLOC reusing PCB for socket %x\n", so);
#endif
	    inp = so->so_saved_pcb;
	    temp = inp->inp_saved_ppcb;
	    bzero((caddr_t) inp, sizeof(*inp));
	    inp->inp_saved_ppcb = temp;
	}

	inp->hash_element = -1;
	inp->lport_hash_element = -1;
	inp->inp_head = head;
	inp->hash_str = hash_str;
	inp->lport_hash_str = lport_hash_str;
	inp->inp_socket = so;
	if (head)
	    insque(inp, head);
	so->so_pcb = (caddr_t)inp;
	return (0);
}

int
in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	register struct inpcb *head = inp->inp_head;
	register struct sockaddr_in *sin;
	struct proc *p = current_proc();		/* XXX */
	u_short lport = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);
	int error;
	register int	loop_count = 0;
	struct inpcb	*hash_head;


	if (in_ifaddr == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_lport || inp->inp_laddr.s_addr != INADDR_ANY)
		return (EINVAL);
	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	     (so->so_options & SO_ACCEPTCONN) == 0))
		wild = INPLOOKUP_WILDCARD;
	if (nam) {
		sin = mtod(nam, struct sockaddr_in *);
		if (nam->m_len != sizeof (*sin))
			return (EINVAL);
#ifdef notdef
		/*
		 * We should check the family, but old programs
		 * incorrectly fail to initialize it.
		 */
		if (sin->sin_family != AF_INET)
			return (EAFNOSUPPORT);
#endif
		lport = sin->sin_port;
		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr))) {
			/*
			 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
			 * allow complete duplication of binding if
			 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
			 * and a multicast address is bound on both
			 * new and duplicated sockets.
			 */
			if (so->so_options & SO_REUSEADDR)
				reuseport = SO_REUSEADDR|SO_REUSEPORT;
		} else if (sin->sin_addr.s_addr != INADDR_ANY) {
			sin->sin_port = 0;		/* yech... */
			if (ifa_ifwithaddr((struct sockaddr *)sin) == 0)
				return (EADDRNOTAVAIL);
		}
		if (lport) {
			struct inpcb *t;

			/* GROSS */
			if (ntohs(lport) < IPPORT_RESERVED &&
			    (error = suser(p->p_ucred, &p->p_acflag)))
				return (EACCES);

			hash_head = inet_lport_hash1(inp->lport_hash_str, lport);
			t = lport_hash_in_pcbwild(hash_head,
						  zeroin_addr, 0, sin->sin_addr,
						  lport, wild);

			if (t && (reuseport & t->inp_socket->so_options) == 0)
				return (EADDRINUSE);
		}
		inp->inp_laddr = sin->sin_addr;
	}
	if (lport == 0) {
	    for (;;) {
		if (++loop_count >= (IPPORT_USERRESERVED - IPPORT_RESERVED))
		    return EADDRINUSE;

		if (head->inp_lport++ < IPPORT_RESERVED ||
		    head->inp_lport > IPPORT_USERRESERVED) 
		    head->inp_lport = IPPORT_RESERVED;

		lport = htons(head->inp_lport);

		hash_head = inet_lport_hash1(inp->lport_hash_str, lport);
		if (lport_hash_in_pcbwild(hash_head,
					  zeroin_addr, 0, inp->inp_laddr, lport, wild))
			continue;

		break;
	    }
	}

	inp->inp_lport = lport;
	lport_hash_insert(inp);

	return (0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
in_pcbconnect(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
    struct inpcb	*hash_head;
	struct in_ifaddr *ia;
	struct sockaddr_in *ifaddr;
	register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (in_ifaddr) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use the primary local address.
		 * If the supplied address is INADDR_BROADCAST,
		 * and the primary interface supports broadcast,
		 * choose the broadcast address for that interface.
		 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
#define sintosa(sin)	((struct sockaddr *)(sin))
#define ifatoia(ifa)	((struct in_ifaddr *)(ifa))
		if (sin->sin_addr.s_addr == INADDR_ANY)
		    sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
		else if (sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST &&
		  (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
		    sin->sin_addr = satosin(&in_ifaddr->ia_broadaddr)->sin_addr;
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;

		ia = (struct in_ifaddr *)0;
		/* 
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr !=
			sin->sin_addr.s_addr || 
		    inp->inp_socket->so_options & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0 && /*XXX*/
		    (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
			/* No route yet, so try to acquire one */
			ro->ro_dst.sa_family = AF_INET;
			ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				sin->sin_addr;
			rtalloc(ro);
		}
		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 * unless it is the loopback (in case a route
		 * to our address on another net goes to loopback).
		 */
		if (ro->ro_rt && !(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK))
			ia = ifatoia(ro->ro_rt->rt_ifa);
		if (ia == 0) {
			u_short fport = sin->sin_port;

			sin->sin_port = 0;
			ia = ifatoia(ifa_ifwithdstaddr(sintosa(sin)));
			if (ia == 0)
				ia = ifatoia(ifa_ifwithnet(sintosa(sin)));
			sin->sin_port = fport;
			if (ia == 0)
				ia = in_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		/*
		 * If the destination address is multicast and an outgoing
		 * interface has been set as a multicast option, use the
		 * address of that interface as our source address.
		 */
		if (IN_MULTICAST(ntohl(sin->sin_addr.s_addr)) &&
		    inp->inp_moptions != NULL) {
			struct ip_moptions *imo;
			struct ifnet *ifp;

			imo = inp->inp_moptions;
			if (imo->imo_multicast_ifp != NULL) {
				ifp = imo->imo_multicast_ifp;
				for (ia = in_ifaddr; ia; ia = ia->ia_next)
					if (ia->ia_ifp == ifp)
						break;
				if (ia == 0)
					return (EADDRNOTAVAIL);
			}
		}
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;
	}

	if ( !(inp->inp_flags & INP_NOLOOKUP)) {
		hash_head = inet_hash1(inp->hash_str,
				       inp->inp_laddr.s_addr ? inp->inp_laddr.s_addr : ifaddr->sin_addr.s_addr,
				       inp->inp_lport,
				       sin->sin_addr.s_addr,
				       sin->sin_port);

	        if (hash_in_pcblookup(hash_head,
				      sin->sin_addr,
				      sin->sin_port,
				      inp->inp_laddr.s_addr ? inp->inp_laddr : ifaddr->sin_addr,
				      inp->inp_lport))
			return (EADDRINUSE);
	}


	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		if (inp->inp_lport == 0)
			(void)in_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	hash_insert(inp);
	return (0);
}

int
in_pcbdisconnect(inp)
	struct inpcb *inp;
{

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;

	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
	else {
		if (inp->hash_element >= 0)
			hash_remove(inp);
	}
}

int
in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;

	if (inp->hash_element >= 0)
	        hash_remove(inp);

	if (inp->lport_hash_element >= 0)
		lport_hash_remove(inp);
#if TEMPDEBUG
	if (so->cached_in_sock_layer)
	    printf("PCB_DETACH for cached socket %x\n", so);
	else
	    printf("PCB_DETACH for allocated socket %x\n", so);
#endif
	so->so_pcb = 0;

	if (inp->inp_options)
		(void)m_free(inp->inp_options);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	ip_freemoptions(inp->inp_moptions);
	if ((inp->inp_next) && (inp->inp_prev))
	    remque(inp);

	if (so->cached_in_sock_layer)
  		so->so_saved_pcb = inp;
	else
	        zfree(inpcb_zone, (vm_offset_t) inp);
	sofree(so);
}

int
in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

int
in_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Must be called at splnet.
 */
int
in_pcbnotify(head, dst, fport_arg, laddr, lport_arg, cmd, notify)
	struct inpcb *head;
	struct sockaddr *dst;
	u_int fport_arg, lport_arg;
	struct in_addr laddr;
	int cmd;
	void (*notify) __P((struct inpcb *, int));
{
	extern u_char inetctlerrmap[];
	register struct inpcb *inp, *oinp;
	struct in_addr faddr;
	u_short fport = fport_arg, lport = lport_arg;
	int errno;

	if ((unsigned)cmd > PRC_NCMDS || dst->sa_family != AF_INET)
		return;
	faddr = ((struct sockaddr_in *)dst)->sin_addr;
	if (faddr.s_addr == INADDR_ANY)
		return;

	/*
	 * Redirects go to all references to the destination,
	 * and use in_rtchange to invalidate the route cache.
	 * Dead host indications: notify all references to the destination.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		laddr.s_addr = 0;
		if (cmd != PRC_HOSTDEAD)
			notify = in_rtchange;
	}
	errno = inetctlerrmap[cmd];
	for (inp = head->inp_next; inp != head;) {
		if (inp->inp_faddr.s_addr != faddr.s_addr ||
		    inp->inp_socket == 0 ||
		    (lport && inp->inp_lport != lport) ||
		    (laddr.s_addr && inp->inp_laddr.s_addr != laddr.s_addr) ||
		    (fport && inp->inp_fport != fport)) {
			inp = inp->inp_next;
			continue;
		}
		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify)(oinp, errno);
	}
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
int
in_losing(inp)
	struct inpcb *inp;
{
	register struct rtentry *rt;
	struct rt_addrinfo info;

	if ((rt = inp->inp_route.ro_rt)) {
		inp->inp_route.ro_rt = 0;
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] =
			(struct sockaddr *)&inp->inp_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest(RTM_DELETE, rt_key(rt),
				rt->rt_gateway, rt_mask(rt), rt->rt_flags, 
				(struct rtentry **)0);
		else 
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
			rtfree(rt);
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
in_rtchange(inp, errno)
	register struct inpcb *inp;
	int errno;
{
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
}

struct inpcb *
in_pcblookup(head, faddr, fport_arg, laddr, lport_arg, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;
	u_short fport = fport_arg, lport = lport_arg;
#if KDEBUG
	register int checked = 0;
#endif
	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_START, 0,0,0,0,0);

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
#if KDEBUG
	        checked++;
#endif
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
#if KDEBUG
	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_END, checked,match,0,0,0);
#endif
	return (match);
}



struct inpcb *
hash_in_pcbwild(head, faddr, fport_arg, laddr, lport_arg, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;
	u_short fport = fport_arg, lport = lport_arg;
#if KDEBUG
	register int checked = 0;
#endif

	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_START, 0,0,0,0,-1);

	for (inp = head; inp != 0; inp = inp->hash_next) {
#if KDEBUG
	        checked++;
#endif
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
#if KDEBUG
	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_END, checked,match,0,0,-1);
#endif
	return (match);
}



struct inpcb *
lport_hash_in_pcbwild(head, faddr, fport_arg, laddr, lport_arg, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
	int flags;
{
	register struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;
	u_short fport = fport_arg, lport = lport_arg;
#if KDEBUG
	register int checked = 0;
#endif

	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_START, 0,0,0,0,-1);

	for (inp = head; inp != 0; inp = inp->lport_hash_next) {
#if KDEBUG
	        checked++;
#endif
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
#if KDEBUG
	KERNEL_DEBUG(DBG_FNC_PCB_LOOKUP | DBG_FUNC_END, checked,match,0,0,-1);
#endif
	return (match);
}





struct inpcb *
hash_in_pcblookup(head, faddr, fport_arg, laddr, lport_arg)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
{
	register struct inpcb *inp;
	u_short fport = fport_arg, lport = lport_arg;
#if KDEBUG
	register int checked = 0;
#endif
	KERNEL_DEBUG(DBG_FNC_PCB_HLOOKUP | DBG_FUNC_START, 0,0,0,0,0);

	for (inp = head; inp != 0; inp = inp->hash_next) {
#if KDEBUG
	        checked++;
#endif
		if (inp->inp_lport == lport && inp->inp_laddr.s_addr == laddr.s_addr &&
		    inp->inp_faddr.s_addr == faddr.s_addr && inp->inp_fport == fport)
		        break;
	}
#if KDEBUG
	KERNEL_DEBUG(DBG_FNC_PCB_HLOOKUP | DBG_FUNC_END, checked,inp,0,0,0);
#endif
        return (inp);
}
