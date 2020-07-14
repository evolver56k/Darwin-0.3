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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	File: aurpd.c
 */

/*
 * Kernel process to implement the AURP daemon:
 *  manage tunnels to remote AURP servers across IP networks
 */
#ifdef _AIX
#include <sys/sleep.h>
#endif
#include <sysglue.h>
#ifdef _AIX
#include <sys/atomic_op.h>
#endif
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <at/appletalk.h>
#include <lap.h>
#include <routing_tables.h>
#define _AURP
#define _KERNSYS
#include <at/aurp.h>
#include <at_aurp.h>


#undef  e_sleep_thread
#define M_RCVBUF (64 * 1024)
#define M_SNDBUF (64 * 1024)
struct aurp_global_t aurp_global;
/*
 * Initialize the aurp pipe -
 * -Create, initialize, and start the aurpd kernel process; we need
 *  a process to permit queueing between the socket and the stream,
 *  which is necessary for orderly access to the socket structure.
 * -The user process (aurpd) is there to 'build' the AURP
 *  stream, act as a 'logging agent' (:-}), and hold open the stream
 *  during its use.
 * -Data and AURP packets from the DDP stream will be fed into the
 *  UDP tunnel (AURPsend())
 * -Data and AURP packets from the UDP tunnel will be fed into the
 *  DDP stream (ip_to_atalk(), via the kernel process).
 */
aurpd_start()
{	register int error;
	register struct socket *so;
	int maxbuf;
	extern void aurp_wakeup(caddr_t, int);
/****### TEMPO HACK 9/26: we need more args than that for suser in bsd4.4 
	if (!suser())
		return(EPERM);
*/

	/*
	 * Set up state prior to starting kernel process so we can back out
	 *  (error return) if something goes wrong.
	 */
	bzero((char *)&aurp_global.tunnel, sizeof(aurp_global.tunnel));
	/*lock_alloc(&aurp_global.glock, LOCK_ALLOC_PIN, AURP_EVNT_LOCK, -1);*/
	ATLOCKINIT(aurp_global.glock);
	ATEVENTINIT(aurp_global.event_anchor);

	/* open udp socket */
	if (aurp_global.udp_port == 0)
		aurp_global.udp_port = AURP_SOCKNUM;
	error = socreate(AF_INET, &aurp_global.tunnel, SOCK_DGRAM,
			 IPPROTO_UDP);
	if (error)
	{	dPrintf(D_M_AURP, D_L_FATAL, ("AURP: Can't get socket (%d)\n",
			error));
		return(error);
	}

	so = aurp_global.tunnel;

	if (error = aurp_bindrp(so))
	{	dPrintf(D_M_AURP, D_L_FATAL,
			("AURP: Can't bind to port %d (%d)\n",
			&aurp_global.udp_port, error));
		soclose(so);
		return(error);
	}

#ifdef _AIX
	SOCKET_LOCK(so);
#else
	sblock(&so->so_rcv, M_WAIT);
	sblock(&so->so_snd, M_WAIT);
#endif
	maxbuf = M_RCVBUF;
	setsockopt(so,SOL_SOCKET,SO_RCVBUF,&maxbuf,sizeof(maxbuf));
	maxbuf = M_SNDBUF;
	setsockopt(so,SOL_SOCKET,SO_SNDBUF,&maxbuf,sizeof(maxbuf));
#ifdef _AIX
	so->so_snd.sb_wakeup  = 0;
	so->so_snd.sb_wakearg = 0;
	so->so_rcv.sb_wakeup  = aurp_wakeup;
	so->so_rcv.sb_wakearg = (caddr_t)AE_UDPIP; /* Yuck */
	so->so_rcv.sb_iodone  = 0;
	so->so_rcv.sb_ioarg = 0;
#else
	so->so_upcall = aurp_wakeup;
	so->so_upcallarg = (caddr_t)AE_UDPIP; /* Yuck */
#endif

	so->so_state |= SS_NBIO;
#ifdef _AIX
	so->so_special |=(SP_NOUAREA|SP_EXTPRIV);
#else
	kprintf("aurpd: warning what do we do with so_special?");
#endif
	so->so_rcv.sb_flags |=(SB_SEL|SB_NOINTR);
	so->so_snd.sb_flags |=(SB_SEL|SB_NOINTR);
#ifdef _AIX
	SOCKET_UNLOCK(so);
#else
	sbunlock(&so->so_snd);
	sbunlock(&so->so_rcv);
#endif

	return(0);
}

AURPgetmsg(err)
	int *err;
{	register struct socket *so;
	register int s, events;

	so = aurp_global.tunnel;

	for (;;)
	{	gbuf_t *from, *p_mbuf;
#ifdef _AIX
		int flags = MSG_NONBLOCK;
#else
		int flags = MSG_DONTWAIT;
#endif
		struct uio auio;

		/*
		 * Wait for a package to arrive.  This will be from the
		 * IP side - sowakeup() calls aurp_wakeup()
		 *	     when a packet arrives
		 */

		ATDISABLE(s, aurp_global.glock);
		events = aurp_global.event;
		if (events == 0)
		  {
#ifdef _AIX
		    e_sleep_thread(&aurp_global.event_anchor,
					&aurp_global.glock, LOCK_HANDLER);
#else
		    (void) tsleep(&aurp_global.event_anchor, PSOCK, "AURPgetmsg", 0);
#endif
		    events = aurp_global.event;
		    aurp_global.event = 0;
		  }	
		ATENABLE(s, aurp_global.glock);	 

		if (events & AE_SHUTDOWN)
		  {	
		    aurp_global.shutdown = 1;
		    while (aurp_global.running)
			;
		    /*lock_free(&aurp_global.glock);*/
		    aurp_global.tunnel = 0;
		    aurp_global.event = 0;
		    soclose(so);
		    *err = ESHUTDOWN;
		    return -1;
		  }



		/*
		 * Set up the nominal uio structure -
		 *  give it no iov's, point off to non-existant user space,
		 *  but make sure the 'resid' count means somehting.
		 */

		auio.uio_iov = NULL;
		auio.uio_iovcnt = 0;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_offset = 0;			/* XXX */

		/* Keep up an even flow... */
		for (;;)
		{	register int empty = 0;

/*
 * This should be large enough to encompass a full DDP packet plus
 *  domain header.
 */
#define A_LARGE_SIZE 700
#ifdef _AIX
			flags = MSG_NONBLOCK;
#else
			flags = MSG_DONTWAIT;
#endif
			auio.uio_resid = A_LARGE_SIZE;
			*err = soreceive(so, &from, &auio, &p_mbuf, 0, &flags);
			dPrintf(D_M_AURP, D_L_WARNING,
				("aurpd: soreceive returned %d\n", *err));
			/* soreceive() sets *mp to zero! at start */
			if (p_mbuf)
			        ip_to_atalk(from, p_mbuf);
			else
			  break;
		}
	}

	return -1;
}

/*
 * Wakeup the sleeping giant - we've put a message on his queue(s).
 * The arg indicates what queue has been updated.
 */
void aurp_wakeup(register caddr_t p, int state)
{	register int s;
	register int bit;

	bit = (int) p;
	ATDISABLE(s, aurp_global.glock);
	aurp_global.event |= bit;
	ATENABLE(s, aurp_global.glock);
#ifdef _AIX
	e_wakeup(&aurp_global.event_anchor);
#else
	thread_wakeup(&aurp_global.event_anchor);
#endif
}

/*
 * Try to bind to the specified reserved port.
 * Sort of like sobind(), but no suser() check.
 */
aurp_bindrp(so)
	register struct socket *so;
{
	struct sockaddr_in *sin;
	gbuf_t *m;
	u_short i;
	int error;
	struct ucred *savecred;

	m = (gbuf_t *)gbuf_alloc(sizeof(struct sockaddr_in));
	if (m == NULL) {
		dPrintf(D_M_AURP, D_L_ERROR,
			("aurp_bindrp: couldn't alloc mbuf"));
		return (ENOBUFS);
	}

	sin = (struct sockaddr_in *)gbuf_rptr(m);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htons(aurp_global.src_addr);
	sin->sin_port = htons(aurp_global.udp_port);
	gbuf_len(m) = sizeof(struct sockaddr_in);

#ifdef _AIX
	SOCKET_LOCK(so);
#else
	sblock(&so->so_rcv, M_WAIT);
	sblock(&so->so_snd, M_WAIT);
#endif
	so->so_state |= SS_PRIV;
	error = (*so->so_proto->pr_usrreq)(so, PRU_BIND, 0, m, 0);
#ifdef _AIX
	SOCKET_UNLOCK(so);
#else
	sbunlock(&so->so_snd);
	sbunlock(&so->so_rcv);
#endif

	(void) gbuf_freeb(m);
	return (error);
}

/*
 * receive from UDP
 * fp is the 'source address' mbuf; p_mbuf is the data mbuf.
 * Use the source address to find the 'node number' (index of the address),
 *  and pass that to the next stage.
 */
int ip_to_atalk(register gbuf_t *fp, register gbuf_t *p_mbuf)
{	register aurp_hdr_t *hdr;
	register domain_t *domain;
	unsigned char node;
	register struct sockaddr_in *rem_addr;

	rem_addr = (struct sockaddr_in *)gbuf_rptr(fp);
	/* determine the node where the packet came from */
	for (node=1; node <= dst_addr_cnt; node++) {
		if (aurp_global.dst_addr[node] == *(long *)&rem_addr->sin_addr)
			break;
	}
	if (node > dst_addr_cnt) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrecv: invalid node, %d.%x\n",
			rem_addr->sin_port,
			*(long *)&rem_addr->sin_addr));
		gbuf_freem(fp);
		gbuf_freem(p_mbuf);
		return -1;
	}

	/* validate the domain */
	domain = (domain_t *)gbuf_rptr(p_mbuf);
	if ( (domain->dst_length != IP_LENGTH) ||
	    (domain->dst_authority != IP_AUTHORITY) ||
	    (domain->version != AUD_Version) ||
	    ((domain->type != AUD_Atalk) && (domain->type != AUD_AURP)) ) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrecv: invalid domain, %d.%x\n",
			rem_addr->sin_port,
			*(long *)&rem_addr->sin_addr));
		gbuf_freem(fp);
		gbuf_freem(p_mbuf);
		return -1;
	}

	/* Remove domain header */
     p_mbuf->m_pkthdr.len -= IP_DOMAINSIZE;
	gbuf_rinc(p_mbuf,IP_DOMAINSIZE);
	gbuf_set_type(p_mbuf, MSG_DATA);

	/* forward the packet to the local AppleTalk stack */
	gbuf_freem(fp);
	at_insert(p_mbuf, domain->type, node);
	return 0;
}

/*
 * send to UDP
 * The real work has been done already.	 Here, we just cobble together
 *  a sockaddr for the destination and call sosend().
 */
void
atalk_to_ip(register gbuf_t *m)
{	register domain_t *domain;
	int error;
#ifdef _AIX
	int flags = MSG_NONBLOCK;
#else
	int flags = MSG_DONTWAIT;
#endif
	struct sockaddr_in *rem_addr;
	register gbuf_t *mbp;

	m->m_type = MT_HEADER;
	m->m_pkthdr.len = gbuf_msgsize(m);
	m->m_pkthdr.rcvif = 0;

	/* Cons up a destination sockaddr */
	if ((mbp = (gbuf_t *)gbuf_alloc(sizeof(struct sockaddr_in))) == NULL)
	{
#ifdef _AIX
		(void)fetch_and_add((atomic_p)&aurp_global.no_mbufs, 1);
#endif
		gbuf_freem(m);
		return;
	}

	rem_addr = (struct sockaddr_in *)gbuf_rptr(mbp);
	bzero((char *)rem_addr, sizeof(rem_addr));
	rem_addr->sin_family = PF_INET;
	rem_addr->sin_port = aurp_global.udp_port;
	gbuf_len(mbp) = sizeof (struct sockaddr_in);
	domain = (domain_t *)gbuf_rptr(m);
	*(long *)&rem_addr->sin_addr = domain->dst_address;

#ifdef _AIX
	(void)fetch_and_add((atomic_p)&aurp_global.running, 1);
#endif
	if (aurp_global.shutdown) {
		gbuf_freem(m);
		gbuf_freeb(mbp);
#ifdef _AIX
		(void)fetch_and_add((atomic_p)&aurp_global.running, -1);
#endif
		return;
	}
	error = sosend(aurp_global.tunnel, mbp, NULL, m, NULL, flags);
	if (error)
	{	/*log error*/
	  dPrintf(D_M_AURP, D_L_ERROR, ("AURP: sosend error (%d)\n",
		  error));
	}

	gbuf_freeb(mbp);
#ifdef _AIX	
	(void)fetch_and_add((atomic_p)&aurp_global.running, -1);
#endif
	return;
}


