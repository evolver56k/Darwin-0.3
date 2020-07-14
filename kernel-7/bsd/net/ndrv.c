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

/* Copyright (c) 1997, 1998 Apple Computer, Inc. All Rights Reserved */
/*
 *	@(#)ndrv.c	1.1 (Rhapsody) 6/10/97
 * Justin Walker, 970604
 *   AF_NDRV support
 * 980130 - Cleanup, reorg, performance improvemements
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include "if_blue.h"
#include "ndrv.h"

#if INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#endif
#include <netinet/if_ether.h>

#if NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if ISO
#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <netiso/iso_snpac.h>
#endif

#if LLC
#include <netccitt/dll.h>
#include <netccitt/llc_var.h>
#endif

#include <machine/spl.h>

extern void ndrv_ctlinput(), ndrv_init(), ndrv_drain();
extern int ndrv_ctloutput(), ndrv_usrreq(), ndrv_output(), ndrv_sysctl();

int ndrv_attach(struct socket *, int);
int ndrv_detach(struct ndrv_cb *);
int ndrv_bind(struct socket *, struct mbuf *);
int ndrv_disconnect(struct ndrv_cb *);

extern struct domain ndrvdomain; /* That silly forward reference stuff */

struct protosw ndrvsw[] =
{	{	SOCK_RAW, &ndrvdomain, 0, PR_ATOMIC|PR_ADDR,
		0, ndrv_output, ndrv_ctlinput, ndrv_ctloutput,
		ndrv_usrreq, ndrv_init, NULL, NULL,
		ndrv_drain, ndrv_sysctl
	}
};

void ndrv_dinit(void);

struct domain ndrvdomain =
{	AF_NDRV, "NetDriver", NULL, NULL, NULL,
	ndrvsw, &ndrvsw[sizeof(ndrvsw)/sizeof(ndrvsw[0])],
	NULL, NULL, 0, 0
};

unsigned long  ndrv_sendspace = NDRVSNDQ;
unsigned long  ndrv_recvspace = NDRVRCVQ;
struct ndrv_cb ndrvl;		/* Head of controlblock list */

#define BLUEQMAXLEN	50
int blueqmaxlen = BLUEQMAXLEN;	/* Default */

/* Domain init function for AF_NDRV - noop */
void
ndrv_dinit(void)
{
}

/*
 * Protocol init function for NDRV protocol
 * Init the control block list.
 */
void
ndrv_init()
{	ndrvl.nd_next = ndrvl.nd_prev = &ndrvl;
	blueq.ifq_maxlen = blueqmaxlen;
}

/*
 * Protocol output - Called from, e.g., the Blue Box
 * Just use the driver.  If we're splitting, loop it back.
 */
int
ndrv_output(register struct mbuf *m,
	    register struct socket *so)
{	register struct ndrv_cb *np = sotondrvcb(so);
	register struct ifnet *ifp = np->nd_if;
	int s, error;
	extern struct ifnet_blue *blue_if;
	extern void kprintf(const char *, ...);
	extern int Filter_check(struct mbuf **);
#define senderr(e) { error = (e); goto bad;}

#if 0
	kprintf("NDRV output: %x, %x, %x\n", m, so, np);
#endif

	/*
	 * No header is a format error
	 */
	if ((m->m_flags&M_PKTHDR) == 0)
		return(ENOBUFS); /* EINVAL??? */

	/* If we're splitting,  */
	if (ifp->if_flags&IFF_SPLITTER) /* Splitter is turned on */
	{	register struct mbuf *m0;
		struct mbuf *m1;
		register int rv;
		extern struct mbuf *m_dup(struct mbuf *, int);
#if 0
		kprintf("NDRV_OUTPUT: m0 = %x\n", m0);
#endif
		
		m1 = m;
		rv = Filter_check(&m1);
		m = m1;
		/*
		 * -1 => Not For Rhapsody
		 * 0 => For Both
		 * 1 => For Rhapsody
		 */
		if (rv >= 0)
		{	register struct ether_header *eh;

			if (rv == 0)
			{	if ((m0 = m_dup(m, M_WAIT)) == NULL)
					((struct ifnet_blue *)ifp->if_Y)->no_bufs2++;
			} else
			{	m0 = m;
				m = NULL;
			}
			/* Hack alert! */
			eh = mtod(m0, struct ether_header *);
			m0->m_data += sizeof(struct ether_header);
			m0->m_len -= sizeof (struct ether_header);
			m0->m_pkthdr.len -= sizeof(struct ether_header);
			m0->m_flags |= 0x80;
			ether_input((struct ifnet *)blue_if, eh, m0);
			if (!m)
				return(0);
		}
	}

	/*
	 * Output to real device.  Here if rv==0.
	 * Cribbed from ether_output() XXX
	 *
	 * Can't do multicast accounting because we don't know
	 *  (a) if our interface does multicast; and
	 *  (b) what a multicast address looks like
	 */
	s = splimp();
	/*
	 * Queue message on interface, and start output
	 *  Can't check for already active, since that's a race.
	 */
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		senderr(ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
        (*ifp->if_start)(ifp);
	splx(s);
	ifp->if_obytes += m->m_len; /* MP alert! */
	blue_if->pkts_out++;
	return (0);
bad:
	if (m)
		m_freem(m);
	return (error);
}

/* The meat of the protocol handler */
int
ndrv_usrreq(register struct socket *so,
	    register int req,
	    register struct mbuf *m,
	    register struct mbuf *nam,
	    register struct mbuf *control)
{	register struct ndrv_cb *np = sotondrvcb(so);
	register int error = 0;
	int len;
	extern int splitter_ctl(struct socket *, int, caddr_t, struct ifnet *);

#if 0
	kprintf("NDRV usrreq: %x, %x, %x\n", so, req, np);
#endif
	if (req == PRU_CONTROL)
		return (splitter_ctl(so, (int)m, (caddr_t)nam,
				     (struct ifnet *)control));
	if (control && control->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (req != PRU_ATTACH && np == 0) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

		/*
		 * Allocate a control block and fill in the
		 * necessary info to allow packets to be routed to
		 * the appropriate device driver routine.
		 */
		case PRU_ATTACH:
			error = ndrv_attach(so, (int)nam);
			break;

		/*
		 * Destroy state just before socket deallocation.
		 * Flush data or not depending on the options.
		 */
		case PRU_DETACH:
			ndrv_detach((struct ndrv_cb *)np);
			break;

		/*
		 * If a socket isn't bound to a single address,
		 * the ndrv input routine will hand it anything
		 * within that protocol family (assuming there's
		 * nothing else around it should go to).
		 *
		 * Don't expect this to be used.
		 */
		case PRU_CONNECT:
			if (np->nd_faddr) {
				error = EISCONN;
				break;
			}
			nam = m_copym(nam, 0, M_COPYALL, M_WAIT);
			np->nd_faddr = mtod(nam, struct sockaddr_ndrv *);
			soisconnected(so);
			break;

		/*
		 * This is the "driver open" hook - we 'bind' to the
		 *  named driver.
		 */
		case PRU_BIND:
			if (np->nd_laddr) {
				error = EINVAL;			/* XXX */
				break;
			}
			error = ndrv_bind(so, nam);
			break;

		case PRU_CONNECT2:
			error = EOPNOTSUPP;
			goto release;

		case PRU_DISCONNECT:
			if (np->nd_faddr == 0) {
				error = ENOTCONN;
				break;
			}
			ndrv_disconnect((struct ndrv_cb *)np);
			soisdisconnected(so);
			break;

		/*
		 * Mark the connection as being incapable of further input.
		 */
		case PRU_SHUTDOWN:
			socantsendmore(so);
			break;

		/*
		 * Ship a packet out.  The ndrv output will pass it
		 *  to the appropriate driver.  The really tricky part
		 *  is the destination address...
		 */
		case PRU_SEND:
			error = ndrv_output(m, so);
			m = NULL;
			break;

		case PRU_ABORT:
			ndrv_disconnect((struct ndrv_cb *)np);
			sofree(so);
			soisdisconnected(so);
			break;

		case PRU_SENSE:
			/*
			 * stat: don't bother with a blocksize.
			 */
			return (0);

		/*
		 * Not supported.
		 */
		case PRU_RCVOOB:
		case PRU_RCVD:
			return(EOPNOTSUPP);

		case PRU_LISTEN:
		case PRU_ACCEPT:
		case PRU_SENDOOB:
		case PRU_SLOWTIMO:
		case PRU_FASTTIMO:
		case PRU_PROTORCV:
		case PRU_PROTOSEND:
			error = EOPNOTSUPP;
			break;

		case PRU_SOCKADDR:
			if (np->nd_laddr == 0) {
				error = EINVAL;
				break;
			}
			len = np->nd_laddr->snd_len;
			bcopy((caddr_t)np->nd_laddr, mtod(nam, caddr_t),
			      (unsigned)len);
			nam->m_len = len;
			break;

		case PRU_PEERADDR:
			if (np->nd_faddr == 0) {
				error = ENOTCONN;
				break;
			}
			len = np->nd_faddr->snd_len;
			bcopy((caddr_t)np->nd_faddr, mtod(nam, caddr_t),
			      (unsigned)len);
			nam->m_len = len;
			break;

		default:
			panic("ndrv_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}

/* Control input */
void
ndrv_ctlinput()
{}

/* Control output */
int
ndrv_ctloutput()
{
	return(0);
}

/* Drain the queues */
void
ndrv_drain()
{}

/* Sysctl hook for NDRV */
int
ndrv_sysctl()
{
	return(0);
}

/*
 * Allocate an ndrv control block and some buffer space for the socket
 */
int
ndrv_attach(register struct socket *so,
	    register int proto)
{	int error;
	register struct ndrv_cb *np = sotondrvcb(so);

#if 0
	kprintf("NDRV attach: %x, %x, %x\n", so, proto, np);
#endif
	MALLOC(np, struct ndrv_cb *, sizeof(*np), M_PCB, M_WAITOK);
#if 0
	kprintf("NDRV attach: %x, %x, %x\n", so, proto, np);
#endif
	if ((so->so_pcb = (caddr_t)np))
		bzero(np, sizeof(*np));
	else
		return(ENOBUFS);
	if ((error = soreserve(so, ndrv_sendspace, ndrv_recvspace)))
		return(error);
	np->nd_signature = NDRV_SIGNATURE;
	np->nd_socket = so;
	np->nd_proto.sp_family = so->so_proto->pr_domain->dom_family;
	np->nd_proto.sp_protocol = proto;
	insque((queue_t)np, (queue_t)&ndrvl);
	return(0);
}

int
ndrv_detach(register struct ndrv_cb *np)
{	register struct socket *so = np->nd_socket;
	extern struct ifnet_blue *blue_if;
	extern void splitter_close(struct ndrv_cb *);

#if 0
	kprintf("NDRV detach: %x, %x\n", so, np);
#endif
	if (blue_if)
		splitter_close(np); /* 'np' is freed within */
	so->so_pcb = 0;
	sofree(so);
	return(0);
}

int
ndrv_disconnect(register struct ndrv_cb *np)
{
#if 0
	kprintf("NDRV disconnect: %x\n", np);
#endif
	if (np->nd_faddr)
	{	m_freem(dtom(np->nd_faddr));
		np->nd_faddr = 0;
	}
	if (np->nd_socket->so_state & SS_NOFDREF)
		ndrv_detach(np);
	return(0);
}

/*
 * Here's where we latch onto the driver and make it ours.
 */
int
ndrv_bind(register struct socket *so,
	  register struct mbuf *nam)
{	register char *dname;
	register struct sockaddr_ndrv *sa;
	register struct ndrv_cb *np;
	register struct ifnet *ifp;
	extern int name_cmp(struct ifnet *, char *);

	if (ifnet == 0)
		return(EADDRNOTAVAIL); /* Quick sanity check */
	np = sotondrvcb(so);
	/* I think we just latch onto a copy here; the caller frees */
	nam = m_copym(nam, 0, M_COPYALL, M_WAITOK);
	sa = mtod(nam, struct sockaddr_ndrv *);
	np->nd_laddr = sa;
	dname = sa->snd_name;
	if (dname == NULL)
		return(EINVAL);
#if 0
	kprintf("NDRV bind: %x, %x, %s\n", so, np, dname);
#endif
	/* Track down the driver and its ifnet structure.
	 * There's no internal call for this so we have to dup the code
	 *  in if.c/ifconf()
	 */
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (name_cmp(ifp, dname) == 0)
			break;
	if (ifp == NULL)
		return(EADDRNOTAVAIL);
	/*
	 * Now, at this point, we should force open the driver and somehow
	 *  register ourselves to receive packets (a la the bpf).
	 * However, we have this groaty hack in place that makes it
	 *  not necessary for blue box purposes (the 'splitter' trick).
	 * If we want this to be a full-fledged AF, we have to force the
	 *  open and implement a filter mechanism.
	 */
	np->nd_if = ifp;
	return(0);
}

/*
 * Try to compare a device name (q) with one of the funky ifnet
 *  device names (ifp).
 */
int name_cmp(register struct ifnet *ifp, register char *q)
{	register char *r;
	register int len;
	char buf[IFNAMSIZ];
	static char *sprint_d();

	r = buf;
	len = strlen(ifp->if_name);
	strncpy(r, ifp->if_name, IFNAMSIZ);
	r += len;
	(void)sprint_d(ifp->if_unit, r, r-buf);
#if 0
	kprintf("Comparing %s, %s\n", buf, q);
#endif
	return(strncmp(buf, q, IFNAMSIZ));
}

/* Hackery - return a string version of a decimal number */
static char *
sprint_d(n, buf, buflen)
        u_int n;
        char *buf;
        int buflen;
{
        register char *cp = buf + buflen - 1;

        *cp = 0;
        do {
                cp--;
                *cp = "0123456789"[n % 10];
                n /= 10;
        } while (n != 0);
        return (cp);
}

/*
 * When closing, dump any enqueued mbufs.
 */
void
ndrv_flushq(register struct ifqueue *q)
{	register struct mbuf *m;
	register int s;
	for (;;)
	{	s = splimp();
		IF_DEQUEUE(q, m);
		if (m == NULL)
			break;
		IF_DROP(q);
		splx(s);
		if (m)
			m_freem(m);
	}
	splx(s);
}
