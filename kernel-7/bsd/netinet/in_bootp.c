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

#if NEXT
/*
 * Copyright (c) 1988, by NeXT Inc.
 *
 **********************************************************************
 * HISTORY
 * Tue Mar 18 11:49:14 PST 1997 Dieter Siegmund (dieter) at NeXT
 *	Ported from 4.3 to 4.4Lite (Teflon)
 *
 * Mon Jan 10 15:13:33 PST 1994 Matt Watson (mwatson) at NeXT
 *	Allow "Sexy Net Init"(tm) while netbooting
 *	Function prototype cleanup
 *
 * 09-Apr-90  Bradley Taylor (btaylor) at NeXT
 *	Changes because of new network API (netbuf and netif)
 *
 * 25-May-89  Peter King (king) at NeXT
 *	Added support for "Sexy Net Init" -- NeXT vendor protocol 1.
 *
 * 15-Aug-88  Peter King (king) at NeXT, Inc.
 *	Created.
 **********************************************************************
 */ 

#if	m68k
#else

#import <machine/spl.h>
#import <machine/label_t.h>
#import <sys/param.h>
#import <sys/types.h>
#import <mach/boolean.h>
#import <sys/kernel.h>
#import <sys/errno.h>
#import <sys/file.h>
#import <sys/uio.h>
#import <sys/ioctl.h>
#import <sys/time.h>
#import <sys/mbuf.h>
#import <sys/vnode.h>
#import <sys/socket.h>
#import <sys/socketvar.h>
#import <net/if.h>
#import <net/route.h>
#import <netinet/in.h>
#import <netinet/in_systm.h>
//#import <netinet/in_pcb.h>
#import <netinet/if_ether.h>
#import <netinet/ip.h>
#import <netinet/ip_var.h>
#import <netinet/udp.h>
#import <netinet/udp_var.h>
#import <netinet/ip_icmp.h>
#import <netinet/bootp.h>
#import <dev/kmreg_com.h>

#define	CONSOLE	"/dev/console"

#define	BOOTP_DEBUG 0
#if	BOOTP_DEBUG
#define	dprintf(x) printf x;
#else	BOOTP_DEBUG
#define	dprintf(x)
#endif	BOOTP_DEBUG

static int in_bootp_initnet(volatile struct ifnet *ifp, struct ifreq *ifr,
	struct socket **sop);
			
static struct bootp_packet * in_bootp_buildpacket(volatile struct ifnet *ifp,
	struct sockaddr_in *sin, u_char my_enaddr[6]);

struct mbuf * in_bootp_bptombuf(struct bootp_packet *bp);
	
static void in_bootp_timeout(struct socket **socketflag);
	
static int in_bootp_promisctimeout(boolean_t *ignoreflag);
	
static int in_bootp_getpacket(struct socket *so, caddr_t pp, int psize);

static int in_bootp_sendrequest(volatile struct ifnet *ifp,
	struct socket *so, struct bootp_packet *bp_s, struct bootp *bp_r,
	u_char my_enaddr[6], struct vnode **console_vpp);

static int in_bootp_openconsole(struct vnode **vpp);

static void in_bootp_closeconsole(struct vnode *vp);

static int in_bootp_processreply(struct vnode *vp, struct bootp_packet *bp_s,
	struct bootp *bp_r);

static int in_bootp_setaddress(volatile struct ifnet *ifp, struct ifreq *ifr,
	struct socket *so, struct in_addr *addr);

static struct ifreq in_bootp_makeifreq(volatile struct ifnet *ifp,
	struct sockaddr_in *sin);

static void kmgets(char *cp, char *lp, boolean_t silent);
	
static boolean_t alertShowing = FALSE;

/*
 * Routine: in_bootp
 * Function:
 *	Use the BOOTP protocol to resolve what our IP address should be
 *	on a particular interface.
 * Note:
 *	The ifreq structure returns the newly configured address.  If there
 *	is an error, it can be full of bogus information.
 *
 *	This is called from driver level ioctl routines to assure that
 *	the interface is a 10Meg Ethernet.
 */
int
in_bootp(
	 volatile struct ifnet	*ifp,
	 struct sockaddr_in *sin,
	 u_char my_enaddr[6]
	 )
{
	struct mbuf		*m;
	struct socket		*so;
	register int		error;
	struct bootp_packet	*bp_s = NULL;
	struct bootp		*bp_r = NULL;
	struct vnode		*vp = NULL;
	struct nextvend		*nv_s;
	struct nextvend		*nv_r;
	struct ifreq		ifr;
	struct proc *		procp = current_proc();	/* XXX */

	dprintf(("in_bootp() entered\n"));

	ifr = in_bootp_makeifreq(ifp, sin);
	if (error = in_bootp_initnet(ifp, &ifr, &so)) {
		if (error == -1) {
			/*
			 * This means the interface was already
			 * autoconfigured.  Reset the address to make
			 * sure the netmask is right.
			 */
			error = ifioctl(so, SIOCSIFADDR, (caddr_t)&ifr,
					procp);
			if (error == 0) {
				bcopy(&ifr.ifr_addr, sin, sizeof(*sin));
			}
			soclose(so);
			return (error);
		}
		dprintf(("in_bootp() in_bootp_initnet failed: %d\n", error));
		goto errout;
	}

	/* Build a packet */
	bp_s = in_bootp_buildpacket(ifp, 
				    sin,
				    my_enaddr);
	nv_s = (struct nextvend *)&bp_s->bp_bootp.bp_vend;

	/* Set up place to receive BOOTP response */
	bp_r = (struct bootp *)kalloc(sizeof (*bp_r));
	nv_r = (struct nextvend *)&bp_r->bp_vend;

	while (TRUE) {
		/* Send the request */
	       dprintf(("in_bootp() trying to send request\n"));
		if (error = in_bootp_sendrequest(ifp, so, bp_s, bp_r, 
						 my_enaddr, &vp)) {
			goto errout;
		}
		if (nv_r->nv_opcode == BPOP_OK) {
			break;
		}
	       dprintf(("in_bootp() trying to open console\n"));
		if (vp == NULL && (error = in_bootp_openconsole(&vp))) {
			goto errout;
		}
	       dprintf(("in_bootp() trying to process reply\n"));
		if (error = in_bootp_processreply(vp, bp_s, bp_r)) {
			goto errout;
		}
	}

	/* It's for us. */
	dprintf(("in_bootp() got a response!\n"));
	if (vp) {
		(void) in_bootp_closeconsole(vp);
		vp = NULL;
	}

	error = in_bootp_setaddress(ifp, &ifr, so,
				    (struct in_addr *)&bp_r->bp_yiaddr);
	if (error == 0) {
		bcopy(&ifr.ifr_addr, sin, sizeof(*sin));
	}

errout:	
	if (error && so) {
		/* Make sure the interface is shut down */
		(void) ifioctl(so, SIOCDIFADDR, (caddr_t) &ifr, procp);
		ifr.ifr_flags = ifp->if_flags & ~(IFF_UP);
		(void) ifioctl(so, SIOCSIFFLAGS, (caddr_t) &ifr, procp);
	}
	ifp->if_eflags &= ~(IFEF_AUTOCONF);
	if (so) {
		soclose(so);
	}
	if (bp_s) {
		kfree((caddr_t)bp_s, sizeof (*bp_s));
	}
	if (bp_r) {
		kfree((caddr_t)bp_r, sizeof (*bp_r));
	}
	if (!error) {
		(void) in_bootp_closeconsole(vp);
	}
	dprintf(("in_bootp() returning: %d\n", error));

	return (error);
}

/*
 * Routine: in_bootp_initnet
 * Function:
 *	Set up the interface to handle BOOTP packets.  Return the interface
 *	address in ifr.
 * Returns:
 *	-1:	interface previously autoconfigured
 *	0:	success
 *	>0:	unix error 
 */
int
in_bootp_initnet(ifp, ifr, sop)
	register volatile struct ifnet	*ifp;
	register struct ifreq		*ifr;
	struct socket			**sop;
{
	struct sockaddr_in	lsin;
	struct sockaddr_in	*sin;
	struct mbuf		*m;
	int	error;
	struct proc *		procp = current_proc();	/* XXX */
	
	dprintf(("  in_bootp_initnet() entered\n"));

	/*
	 * Get a socket (wouldn't it be nice to make system calls from
	 * within the kernel?)
	 */
	dprintf(("  in_bootp_initnet() socreate..."));
	if (error = socreate(AF_INET, sop, SOCK_DGRAM, 0)) {
		*sop = NULL;
		dprintf(("failed: %d\n", error));
		return (error);
	}
	dprintf(("ok\n"));

	/*
	 * Bring the interface up if neccessary.
	 */
	if ((ifp->if_flags & IFF_UP) == 0) {
		/* Turn off trailers here, otherwise the server ARP reply
		   will cause the booting client the issue its own ARP
		   reply enabling trailers.
		*/
		ifr->ifr_flags = ifp->if_flags | IFF_UP | IFF_NOTRAILERS;
		dprintf(("  in_bootp_initnet() SIOCSIFFLAGS..."));
		if (error = ifioctl(*sop, SIOCSIFFLAGS, (caddr_t)ifr, procp)) {
			dprintf(("failed: %d\n", error));
			return (error);
		}
		dprintf(("ok\n"));
	} 
	else {
		if (ifp->if_eflags & IFEF_AUTOCONF_DONE) {
		/* If the interface was previously autoconfigured, return */
			dprintf(("  in_bootp_initnet() already done:"));
			if ((error = ifioctl(*sop, SIOCGIFADDR,
					     (caddr_t)ifr, procp))) {
				dprintf((" %d\n", error));
				return (error);
			}
			ifp->if_eflags &= ~(IFEF_AUTOCONF);
			dprintf((" -1\n"));
			return(-1);
		    }
	}

	/*
	 * Assign the all-zeroes address to the interface.
	 */
	bzero((caddr_t)&lsin, sizeof(lsin));
	lsin.sin_family = AF_INET;
	ifr->ifr_addr = *(struct sockaddr *)&lsin;

	dprintf(("  in_bootp_initnet() SIOCSIFADDR..."));
	if (error = ifioctl(*sop, SIOCSIFADDR, (caddr_t)ifr, procp)) {
		dprintf(("failed: %d\n",
			 error));
		return (error);
	}
	dprintf(("ok\n"));

	/*
	 * Now set up the socket to receive the BOOTP response, i.e. bind
	 * a name to it.
	 */
	dprintf(("  in_bootp_initnet() m_get..."));
	m = m_get(M_WAIT, MT_SONAME);
	if (m == NULL) {
		error = ENOBUFS;
		dprintf(("failed: %d\n", error));
		return (error);
	}
	dprintf(("ok\n"));

	m->m_len = sizeof (struct sockaddr_in);
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(IPPORT_BOOTPC);
	sin->sin_addr.s_addr = INADDR_ANY;

	dprintf(("  in_bootp_initnet() sobind..."));
	error = sobind(*sop, m);
	m_freem(m);
	if (error) {
		dprintf(("failed: %d\n", error));
		return (error);
	}
	dprintf(("ok\n"));

	/* We will be doing non-blocking IO on this socket */
	(*sop)->so_state |= SS_NBIO;
	
	dprintf(("  in_bootp_initnet() exiting normally\n"));
	return (0);
}

/*
 * Routine: in_bootp_buildpacket
 * Function:
 *	Put together a BOOTP request packet.  RFC951.
 */
struct bootp_packet *
in_bootp_buildpacket(
		     volatile struct ifnet *ifp,	/* Interface */
		     struct sockaddr_in	*sin, 		/* Local IP address */
		     u_char my_enaddr[6]		/* Local EN address */
		     )
{
	struct bootp_packet	*bp_s;
	struct nextvend		*nv;

	dprintf(("  in_bootp_buildpacket() entered\n"));

	bp_s = (struct bootp_packet *)kalloc(sizeof (*bp_s));
	nv = (struct nextvend *)&bp_s->bp_bootp.bp_vend;

	bzero(bp_s, sizeof (*bp_s));
	bp_s->bp_ip.ip_v = IPVERSION;
	bp_s->bp_ip.ip_hl = sizeof (struct ip) >> 2;
	bp_s->bp_ip.ip_id = htons(ip_id++);
	bp_s->bp_ip.ip_ttl = MAXTTL;
	bp_s->bp_ip.ip_p = IPPROTO_UDP;
	bp_s->bp_ip.ip_src.s_addr = sin->sin_addr.s_addr;
	bp_s->bp_ip.ip_dst.s_addr = INADDR_BROADCAST;
	bp_s->bp_udp.uh_sport = htons(IPPORT_BOOTPC);
	bp_s->bp_udp.uh_dport = htons(IPPORT_BOOTPS);
	bp_s->bp_udp.uh_sum = 0;
	bp_s->bp_bootp.bp_op = BOOTREQUEST;
	bp_s->bp_bootp.bp_htype = ARPHRD_ETHER;
	bp_s->bp_bootp.bp_hlen = 6;
	bp_s->bp_bootp.bp_ciaddr.s_addr = 0;
	bcopy((caddr_t)my_enaddr, bp_s->bp_bootp.bp_chaddr, 6);
	bcopy(VM_NEXT, &nv->nv_magic, 4);

	nv->nv_version = 1;
	nv->nv_opcode = BPOP_OK;
	bp_s->bp_udp.uh_ulen = htons(sizeof (bp_s->bp_udp) +
				     sizeof (bp_s->bp_bootp));
	bp_s->bp_ip.ip_len = htons(sizeof (struct ip) +
				   ntohs(bp_s->bp_udp.uh_ulen));
	bp_s->bp_ip.ip_sum = 0;

	dprintf(("  in_bootp_buildpacket() exiting\n"));

	return (bp_s);
}

/*
 * Routine: in_bootp_bptombuf
 * Function:
 *	Put the outgoing BOOTP packet into an mbuf chain.
 */
struct mbuf *
in_bootp_bptombuf(bp)
	struct bootp_packet	*bp;
{
	register struct ip	*ip;
	struct mbuf		*m;
	int			resid = sizeof (*bp);
	caddr_t			cp = (caddr_t) bp;

	m = (struct mbuf *)m_devget(bp, sizeof(*bp), 0, 0, 0);

	if (m == 0) {
	    printf("in_bootp_bptombuf: m_devget failed\n");
	    return 0;
	}
	m->m_flags |= M_BCAST;

	/* Compute the checksum */
	ip = mtod(m, struct ip *);
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m, sizeof (struct ip));
	return (m);
}

/*
 * Routine: in_bootp_timeout
 * Function:
 *	Wakeup the process waiting to receive something on a socket.
 *	Boy wouldn't it be nice if we could do system calls from the
 *	kernel.  Then I could do a select!
 */
void
in_bootp_timeout(socketflag)
	struct socket 		**socketflag;
{
	struct socket *so =	*socketflag;

	dprintf(("\n---->in_bootp_timeout()\n"));

	*socketflag = NULL;
	sowakeup(so, &so->so_rcv);
}

/*
 * Routine: in_bootp_promisctimeout
 * Function:
 *	Flag that it is ok to accept a response from a promiscuous BOOTP
 *	server.
 */
int
in_bootp_promisctimeout(ignoreflag)
	boolean_t	*ignoreflag;
{
	dprintf(("\n---->in_bootp_promisctimeout()\n"));

	*ignoreflag = FALSE;
	return 0;
}
 
/*
 * Routine: in_bootp_getpacket
 * Function:
 *	Wait for an incoming packet on the socket.
 */
int
in_bootp_getpacket(so, pp, psize)
	struct socket		*so;
	caddr_t			pp;
	int			psize;
{
	struct	iovec		aiov;
	struct	uio		auio;
	int			rcvflg;

	aiov.iov_base = pp;
	aiov.iov_len = psize;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_offset = 0;
	auio.uio_resid = psize;
	auio.uio_rw = UIO_READ;
	rcvflg = MSG_WAITALL;

	return (soreceive(so, 0, &auio, 0, 0, &rcvflg));
}

#define INITIAL_DEADLINE	5
/*
 * Routine: in_bootp_sendrequest
 * Function:
 *	Do the actual work of sending a BOOTP request and getting the
 *	response.
 */
int
in_bootp_sendrequest(
	volatile struct ifnet	*ifp,
	struct socket		*so,
	struct bootp_packet	*bp_s,
	struct bootp		*bp_r,
	u_char			my_enaddr[6],
	struct vnode		**console_vpp
) {
	struct mbuf		*m;
	register int		error;
	int			xid;
	struct timeval		tv;
	int			timeo;
	int			timecount;
	unsigned long		totaltime;
	unsigned long		deadline;
	struct socket		*timeflag;
	label_t			jmpbuf;
	struct sockaddr_in	lsin;
	struct nextvend		*nv_s =
		(struct nextvend *) &bp_s->bp_bootp.bp_vend;
	struct nextvend		*nv_r = (struct nextvend *)&bp_r->bp_vend;
	boolean_t		promisc_rcvd = FALSE;
	boolean_t		promisc_ignore;

	dprintf(("  in_bootp_sendrequest() entered\n"));

	/* Create a transaction ID */
	microtime(&tv);
	xid = my_enaddr[5] ^ tv.tv_sec;
	bp_s->bp_bootp.bp_xid = xid;

	/* Address to send to */
	lsin.sin_family = AF_INET;
	lsin.sin_port = htons(IPPORT_BOOTPS);
	lsin.sin_addr.s_addr = INADDR_BROADCAST;

	/*
 	 * timeout/retry policy:
	 * Before a server has responded we retransmit at a random time
	 * in a binary window which is doubled each transmission.  This
	 * avoids flooding the net after, say, a power failure when all
	 * machines are trying to reboot simultaneously.
	 */
	timeo = 1;
	timecount = 0;
	totaltime = 0;
	deadline = INITIAL_DEADLINE;

	while (TRUE) {
		if (timecount == 0) {
			/* Put the outgoing packet in an mbuf */
			m = in_bootp_bptombuf(bp_s);
			dprintf(("  in_bootp_sendrequest() if_output "));
			if (error = (*ifp->if_output)(ifp, m,
					      (struct sockaddr *)&lsin, 0)) {
				dprintf(("failed: %d\n", error));
				goto errout;
			}
			dprintf(("ok\n"));
		}

		/*
		 * Sleep for a second and then check for the abort
		 * character again.
		 */
		timeflag = so;
		timeout(in_bootp_timeout, &timeflag, hz);
keepwaiting:
		dprintf(("  in_bootp_sendrequest(): in_bootp_getpacket..."));
		while (((error = in_bootp_getpacket(so, (caddr_t)bp_r, sizeof (*bp_r)))
			== EWOULDBLOCK) && (timeflag == so)) {
			/* dprintf(("waiting (spl = 0x%x)...", curspl())); */
			sbwait(&so->so_rcv);
		}
		if (error && (error != EWOULDBLOCK)) {
			dprintf(("failed: %d\n", error));
			untimeout(in_bootp_timeout, &timeflag);
			goto errout;
		}
		if (timeflag == NULL) {
			dprintf(("  in_bootp_sendrequest(): "
				 "in_bootp_getpacket...timed out\n"));
			if ((deadline > INITIAL_DEADLINE) && (timecount & 1))
			    printf(".");
			timecount++;
			totaltime++;
			if (timecount == timeo) {
				if (timeo < 64) {
					timeo <<= 1;
				}
				timecount = 0;
			}
			if (totaltime == deadline) {
			        register c = 0;

				if (error=in_bootp_openconsole(console_vpp)) {
					goto errout;
				}
				printf("\nServer %snot responding.\n"
				       "Continue without network? (y/n) ",
				       (deadline == INITIAL_DEADLINE) ? "is " : "still ");
				while (1) {
				    c = kmgetc_silent(0) & 0177;
				    if (c == 'y' || c == 'Y') {
					printf("%c\n", c);
					psignal(current_proc(), SIGINT);
					error = EINTR;
					in_bootp_closeconsole(*console_vpp);
					goto errout;
				    }
				    else if (c == 'n' || c == 'N') {
					break;
				    }
				}
				printf("%c\nWaiting for response.", c);
				deadline += 30;
			}

			continue;
		}

		dprintf(("received packet!\n"));

		/* We have a packet.  Check it out. */
		if (bp_r->bp_xid != xid ||
		    bp_r->bp_op != BOOTREPLY ||
		    bcmp(bp_r->bp_chaddr, my_enaddr, 6) != 0) {
			dprintf(("  in_bootp_sendrequest() not for us\n"));
			goto keepwaiting;
		}

		/*
		 * It's for us.  If it is not an authoritative answer,
		 * wait for a while (10 secs) to see if there is an
		 * authoritative server out there.
		 */
		if (nv_s->nv_opcode == BPOP_OK && nv_r->nv_opcode != BPOP_OK) {
			if (!promisc_rcvd) {
				promisc_rcvd = TRUE;
				promisc_ignore = TRUE;
				timeout(in_bootp_promisctimeout,
					&promisc_ignore, 10 * hz);
				dprintf(("  in_bootp_sendrequest() "
					 "ignoring 1st promisc response\n"));
				goto keepwaiting;
			}
			if (promisc_ignore == TRUE) {
				dprintf(("  in_bootp_sendrequest() "
					 "ignoring promisc response\n"));
				goto keepwaiting;
			}
		}


		/* OK, it's for us */
		dprintf(("  in_bootp_sendrequest() it's for us!\n"));

		untimeout(in_bootp_timeout, &timeflag);

		if (alertShowing) {
			printf("Network responded!\n");
		}
		error = 0;
		goto errout;
	}

	/* We aborted.  Return appropriate error */
	error = ETIMEDOUT;

errout:
	dprintf(("  in_bootp_sendrequest() returning %d\n", error));

	untimeout(in_bootp_promisctimeout, &promisc_ignore);
	return (error);
}

/*
 * Routine: in_bootp_openconsole
 * Function:
 *	Pop up a console window so that we can do some user I/O to
 *	answer the server's questions.
 */
int
in_bootp_openconsole(vpp)
	struct vnode	**vpp;
{
	int	ret = 0;
	struct proc *		procp = current_proc();	/* XXX */

	if (alertShowing == FALSE) {
		ret = alert(60, 8, "Configuring Network", "%L", 0, 0, 0, 0, 0, 0, 0);
		alertShowing = TRUE;
	}
	dprintf(("in_bootp_openconsole() returning: %d\n", ret));
	return(ret);
}

/*
 * Routine: in_bootp_closeconsole
 * Function:
 *	Restore and close and the console window.
 */
void
in_bootp_closeconsole(vp)
	struct vnode *vp;
{
	if (alertShowing)
	    alert_done();
	alertShowing = FALSE;
}

/*
 * Routine: in_bootp_processreply
 * Function:
 *	The heart of "Sexy Net Init".  This routine handles requests from
 *	the BOOTP server to interact with the user.
 */
int
in_bootp_processreply(vp, bp_s, bp_r)
	struct vnode	*vp;
	struct bootp_packet	*bp_s;
	struct bootp		*bp_r;
{
	struct nextvend		*nv_s =
		(struct nextvend *) &bp_s->bp_bootp.bp_vend;
	struct nextvend		*nv_r = (struct nextvend *)&bp_r->bp_vend;
	int			resid;
	int			error;
	int			flush = FREAD;
	boolean_t		noecho = FALSE;
	char			*cp;

	/* Truncate text if needed */
	if (strlen(nv_r->nv_text) > NVMAXTEXT) {
		nv_r->nv_null = '\0';
	}

	/* Write out the server message */
	printf("%s", nv_r->nv_text);

	if (error = kmioctl(0, TIOCFLUSH, &flush, 0)) {
		goto errout;
	}
	switch (nv_r->nv_opcode) {
	    case BPOP_QUERY_NE:
		noecho = TRUE;
		/* Fall through */

	    case BPOP_QUERY:
		error = 0;
		/* Read in the user response */
		kmgets(nv_s->nv_text, nv_s->nv_text, noecho);

		/* If we are not echoing, move the cursor to the next line */
		if (noecho) {
			printf("\n");
		}

		/* Get rid of the CR or LF if either exists. */
		for (cp = (char *)nv_s->nv_text; *cp; cp++) {
			if (*cp == '\n' || *cp == '\r') {
				*cp = '\0';
				break;
			}
		}

		/* Set the destination, opcode and xid */
		bp_s->bp_ip.ip_dst = *(struct in_addr *)&bp_r->bp_siaddr;
		nv_s->nv_opcode = nv_r->nv_opcode;
		nv_s->nv_xid = nv_r->nv_xid;
		break;

	    case BPOP_ERROR:
		/* Reset the destination, opcode and xid */
		bp_s->bp_ip.ip_dst.s_addr = INADDR_BROADCAST;
		nv_s->nv_opcode = BPOP_OK;
		nv_s->nv_xid = 0;
		break;
	}
errout:
	return  (error);
}

/*
 * Routine: in_bootp_setaddress
 * Function:
 *	Set the address of the interface.  Leave the new address in ifr
 *	to be returned to the user.
 */
int
in_bootp_setaddress(ifp, ifr, so, addr)
	register volatile struct ifnet	*ifp;
	register struct ifreq		*ifr;
	struct socket			*so;
	struct in_addr			*addr;
{
	struct sockaddr_in		*sin;
	int				error;
	struct proc *			procp = current_proc();	/* XXX */

	dprintf(("  in_bootp_setaddress() entered\n"));

	ifr->ifr_flags = ifp->if_flags & ~(IFF_UP);
	if (error = ifioctl(so, SIOCSIFFLAGS, (caddr_t) ifr, procp)) {
		dprintf(("  in_bootp_setaddress() SIOCSIFFLAGS failed: %d\n",
			 error));
		return (error);
	}	

	/* Clear the netmask first */
	sin = (struct sockaddr_in *)&(ifr->ifr_addr);
	bzero((caddr_t)sin, sizeof(struct sockaddr_in));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	if (error = ifioctl(so, SIOCSIFNETMASK, (caddr_t)ifr, procp)) {
		dprintf(("  in_bootp_setaddress() SIOCSIFNETMASK failed: %d\n",
			 error));
		return (error);
	}

	/* Now set the new address */
	sin->sin_addr.s_addr = addr->s_addr;
	if (error = ifioctl(so, SIOCSIFADDR, (caddr_t)ifr, procp)) {
		dprintf(("  in_bootp_setaddress() SIOCSIFADDR failed: %d\n",
			 error));
		return (error);
	}

	/* Flag that we have autoconfigured */
	ifp->if_eflags |= IFEF_AUTOCONF_DONE;

	dprintf(("  in_bootp_setaddress() exiting\n"));

	return (0);
}

/*
 * Routine: in_bootp_makeifreq
 * Function:
 *	Make a valid ifreq struct from interface pointer and an address
 */
struct ifreq
in_bootp_makeifreq(
		   volatile struct ifnet *ifp,
		   struct sockaddr_in *sin
		   )
{
	struct ifreq ifr;
	char *p;
	
	strcpy(ifr.ifr_name, ifp->if_name);
	for (p = ifr.ifr_name; *p; p++) {
	}
	*p++ = ifp->if_unit + '0';
	*p = 0;
	bcopy(sin, &ifr.ifr_addr, sizeof(*sin));
	return (ifr);
}

/*
 * Routine: kmgets (mostly stolen from swapgeneric.{c,m}
 * Function:
 *	Allows printing echoed or non-echoed text.
 */
void
kmgets(cp, lp, silent)
	char *cp, *lp;
	boolean_t silent;
{
	register c = 0;

	for (;;) {
		if (silent) {
			c = kmgetc_silent(0) & 0177;
		} else {
			c = kmgetc(0) & 0177;
		}
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\177':
			if (lp == cp) {
				cnputc('\b');
				continue;
			}
			cnputc('\b');
			cnputc('\b');
			/* fall into ... */
		case '\b':
			if (lp == cp) {
				cnputc('\b');
				continue;
			}
			cnputc(' ');
			cnputc('\b');
			lp--;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}

#endif
#endif NEXT
