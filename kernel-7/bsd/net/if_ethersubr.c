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

/* Copyright (c) 1997, 1998  Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1989, 1993
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
 *	@(#)if_ethersubr.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#if INET
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>

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

#if defined(ppc)
#include <sys/socketvar.h>
#include <net/if_blue.h>

struct BlueFilter RhapFilter[BFCount];
int BFIx;			/* 0 for Atalk; 1..9 for IP */
#endif /* ppc */

#include <net/ndrv.h>

#if LLC && CCITT
extern struct ifqueue pkintrq;
#endif

#include <h/at_pat.h>
#if NETAT
extern struct ifqueue atalkintrq;
#endif

#include <machine/spl.h>

u_char	etherbroadcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
extern	struct ifnet loif;
#define senderr(e) { error = (e); goto bad;}

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Use trailer local net encapsulation if enough data in first
 * packet leaves a multiple of 512 bytes of data in remainder.
 * Assumes that ifp is actually pointer to arpcom structure.
 */
int
ether_output(ifp, m0, dst, rt0)
	register struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt0;
{
	u_int16_t etype;
	int s, error = 0;
 	u_char edst[6];
	register struct mbuf *m = m0;
	register struct rtentry *rt;
	struct mbuf *mcopy = (struct mbuf *)0;
	register struct ether_header *eh;
	int off, len = m->m_pkthdr.len;
	struct arpcom *ac = (struct arpcom *)ifp;
	int ismulticast = 0;
	extern void kprintf( const char *, ...);
	extern struct mbuf *splitter_input(struct mbuf *, struct ifnet *);

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	ifp->if_lastchange = time;
	if ((rt = rt0)) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			if ((rt0 = rt = rtalloc1(dst, 1)))
				rt->rt_refcnt--;
			else
				senderr(EHOSTUNREACH);
		}
		if (rt->rt_flags & RTF_GATEWAY) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt); rt = rt0;
			lookup: rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
				if ((rt = rt->rt_gwroute) == 0)
					senderr(EHOSTUNREACH);
			}
		}
		if (rt->rt_flags & RTF_REJECT)
			if (rt->rt_rmx.rmx_expire == 0 ||
			    time.tv_sec < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}
	switch (dst->sa_family) {

#if INET
	case AF_INET:
		if (!arpresolve(ac, rt, m, dst, edst))
			return (0);	/* if not yet resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		off = m->m_pkthdr.len - m->m_len;
		etype = ETHERTYPE_IP;
		break;
#endif
#if NS
	case AF_NS:
		etype = ETHERTYPE_NS;
 		bcopy((caddr_t)&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
		    (caddr_t)edst, sizeof (edst));
		if (!bcmp((caddr_t)edst, (caddr_t)&ns_thishost, sizeof(edst)))
			return (looutput(ifp, m, dst, rt));
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		break;
#endif
#if	ISO
	case AF_ISO: {
		int	snpalen;
		struct	llc *l;
		register struct sockaddr_dl *sdl;

		if (rt && (sdl = (struct sockaddr_dl *)rt->rt_gateway) &&
		    sdl->sdl_family == AF_LINK && sdl->sdl_alen > 0) {
			bcopy(LLADDR(sdl), (caddr_t)edst, sizeof(edst));
		} else if (error =
			    iso_snparesolve(ifp, (struct sockaddr_iso *)dst,
					    (char *)edst, &snpalen))
			goto bad; /* Not Resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if (*edst & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX) &&
		    (mcopy = m_copy(m, 0, (int)M_COPYALL))) {
			M_PREPEND(mcopy, sizeof (*eh), M_DONTWAIT);
			if (mcopy) {
				eh = mtod(mcopy, struct ether_header *);
				bcopy((caddr_t)edst,
				      (caddr_t)eh->ether_dhost, sizeof (edst));
				bcopy((caddr_t)ac->ac_enaddr,
				      (caddr_t)eh->ether_shost, sizeof (edst));
			}
		}
		M_PREPEND(m, 3, M_DONTWAIT);
		if (m == NULL)
			return (0);
		etype = m->m_pkthdr.len;
		l = mtod(m, struct llc *);
		l->llc_dsap = l->llc_ssap = LLC_ISO_LSAP;
		l->llc_control = LLC_UI;
		len += 3;
		IFDEBUG(D_ETHER)
			int i;
			printf("unoutput: sending pkt to: ");
			for (i=0; i<6; i++)
				printf("%x ", edst[i] & 0xff);
			printf("\n");
		ENDDEBUG
		} break;
#endif /* ISO */
#if	LLC
/*	case AF_NSAP: */
	case AF_CCITT: {
		register struct sockaddr_dl *sdl = 
			(struct sockaddr_dl *) rt -> rt_gateway;

		if (sdl && sdl->sdl_family == AF_LINK
		    && sdl->sdl_alen > 0) {
			bcopy(LLADDR(sdl), (char *)edst,
				sizeof(edst));
		} else goto bad; /* Not a link interface ? Funny ... */
		if ((ifp->if_flags & IFF_SIMPLEX) && (*edst & 1) &&
		    (mcopy = m_copy(m, 0, (int)M_COPYALL))) {
			M_PREPEND(mcopy, sizeof (*eh), M_DONTWAIT);
			if (mcopy) {
				eh = mtod(mcopy, struct ether_header *);
				bcopy((caddr_t)edst,
				      (caddr_t)eh->ether_dhost, sizeof (edst));
				bcopy((caddr_t)ac->ac_enaddr,
				      (caddr_t)eh->ether_shost, sizeof (edst));
			}
		}
		etype = m->m_pkthdr.len;
#if LLC_DEBUG
		{
			int i;
			register struct llc *l = mtod(m, struct llc *);

			printf("ether_output: sending LLC2 pkt to: ");
			for (i=0; i<6; i++)
				printf("%x ", edst[i] & 0xff);
			printf(" len 0x%x dsap 0x%x ssap 0x%x control 0x%x\n", 
			       etype & 0xff, l->llc_dsap & 0xff, l->llc_ssap &0xff,
			       l->llc_control & 0xff);

		}
#endif /* LLC_DEBUG */
		} break;
#endif /* LLC */	

	case AF_UNSPEC:
		eh = (struct ether_header *)dst->sa_data;
 		bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		/* AF_UNSPEC doesn't swap the byte order of the ether_type. */
		etype = ntohs(eh->ether_type);
		break;

#if NETAT

	case AF_APPLETALK:

		{
#ifdef APPLETALK_DEBUG
			int i;
			register struct llc *l = mtod(m, struct llc *);
#endif

			eh = (struct ether_header *)dst->sa_data;
 			bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		
			etype = m->m_pkthdr.len;
#ifdef APPLETALK_DEBUG
			kprintf("ether_output: sending AppleTalk pkt to: ");
			for (i=0; i<6; i++)
				kprintf("%x ", edst[i] & 0xff);
			kprintf(" len 0x%x dsap 0x%x ssap 0x%x control 0x%x\n", 
			       etype & 0xff, l->llc_dsap & 0xff, l->llc_ssap &0xff,
			       l->llc_control & 0xff);
#endif /* APPLETALK_DEBUG */
		}
		break;

#endif /* NETAT */

	default:
#if 0
		kprintf("%s%d: can't handle af%d\n", ifp->if_name, ifp->if_unit,
			dst->sa_family);
#endif
		senderr(EAFNOSUPPORT);
	}


	if (mcopy)
		(void) looutput(ifp, mcopy, dst, rt);
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct ether_header), M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	eh = mtod(m, struct ether_header *);
	etype = htons(etype);
	bcopy((caddr_t)&etype,(caddr_t)&eh->ether_type,
		sizeof(eh->ether_type));
 	bcopy((caddr_t)edst, (caddr_t)eh->ether_dhost, sizeof (edst));
 	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_shost,
	    sizeof(eh->ether_shost));
	ismulticast = (m->m_flags & M_MCAST);

#if defined(ppc)
	/*
	 * We're already to send.  Let's check for the blue box...
	 */
	if (ifp->if_flags&IFF_SPLITTER)
	{	m->m_flags |= 0x10;
		if ((m = splitter_input(m, ifp)) == NULL)
			return(0);
	}
#endif /* ppc */
	s = splimp();
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		senderr(ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
/*	if ((ifp->if_flags & IFF_OACTIVE) == 0) */
        (*ifp->if_start)(ifp);
	splx(s);
	ifp->if_obytes += len + sizeof (struct ether_header);
	if (ismulticast)
		ifp->if_omcasts++;
	return (error);
bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * Process a received Ethernet packet;
 * the packet is in the mbuf chain m without
 * the ether header, which is provided separately.
 */
void
ether_input(ifp, eh, m)
	struct ifnet *ifp;
	register struct ether_header *eh;
	struct mbuf *m;
{
	register struct ifqueue *inq;
	register struct llc *l;
	u_int16_t etype;
	int s;
	extern void revarpinput(struct mbuf *);

     u_int16_t ptype;


#if defined(ppc)
	extern struct ifnet_blue *blue_if;
	extern struct mbuf *splitter_input(struct mbuf *, struct ifnet *);

	/*
	 * Y-adapter input processing:
	 *  - Don't split if coming from a dummy if
	 *  - If coming from a real if, if splitting enabled,
	 *    then filter the incoming packet
	 */
	if (ifp != (struct ifnet *)blue_if)
	{	/* Is splitter turned on? */
		if (ifp->if_flags&IFF_SPLITTER)
		{	m->m_data -= sizeof(struct ether_header);
			m->m_len += sizeof (struct ether_header);
			m->m_pkthdr.len += sizeof(struct ether_header);
			/*
			 * Check to see if destined for BlueBox or Rhapsody
			 * If NULL return, mbuf's been consumed by the BlueBox.
			 * Otherwise, send on to Rhapsody
			 */
			if ((m = splitter_input(m, ifp)) == NULL)
				return;
			m->m_data += sizeof(struct ether_header);
			m->m_len -= sizeof (struct ether_header);
			m->m_pkthdr.len -= sizeof(struct ether_header);
		}
	} else
	{	/* Get the "real" IF */
		ifp = ((struct ndrv_cb *)(blue_if->ifb_so->so_pcb))->nd_if;
		m->m_pkthdr.rcvif = ifp;
		blue_if->pkts_looped_b2r++;
	}
#endif /* ppc */

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}
#if defined(ppc)
	/*
	 * Here, we're working on the "real" i/f
	 */
#endif

	ifp->if_lastchange = time;
	ifp->if_ibytes += m->m_pkthdr.len + sizeof (*eh);
	if (eh->ether_dhost[0] & 1) {
		if (bcmp((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
		    sizeof(etherbroadcastaddr)) == 0)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;
	}
	if (m->m_flags & (M_BCAST|M_MCAST))
		ifp->if_imcasts++;

     etype = ntohs(eh->ether_type);
     ptype = -1;

	switch (etype) {
#if INET
	case ETHERTYPE_IP:

#if defined(ppc)
	  ptype = mtod(m, struct ip *)->ip_p;
          if (((ifp->if_eflags & IFEF_DVR_REENTRY_OK) == 0) || 
               (ptype != IPPROTO_TCP && ptype != IPPROTO_UDP))
#endif
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		schednetisr(NETISR_ARP);
		inq = &arpintrq;
		break;

	case ETHERTYPE_REVARP:
		revarpinput(m);	/* XXX queue? */
		return;
#endif /* INET */
#if NS
	case ETHERTYPE_NS:
		schednetisr(NETISR_NS);
		inq = &nsintrq;
		break;

#endif /* NS */
	default:
#if ISO || LLC || NETAT
		if (etype > ETHERMTU)
			goto dropanyway;
		l = mtod(m, struct llc *);

		/* Temporary hack: check for AppleTalk and AARP packets */
		/* WARNING we're checking only on the "ether_type" (the 2 bytes
		 * of the SNAP header. This shouldn't be a big deal, 
		 * AppleTalk pat_input is making sure we have the right packets
		 * because it needs to discrimante AARP from EtherTalk packets.
		 */

		if (l->llc_dsap == LLC_SNAP_LSAP &&
			 l->llc_ssap == LLC_SNAP_LSAP &&
			l->llc_un.type_snap.control == 0x03) {
#if NETAT
#ifdef APPLETALK_DEBUG
				printf("ether_input: SNAP Cntrol type=0x%x Src=%s\n",
				 l->llc_un.type_snap.ether_type,
				 ether_sprintf(&eh->ether_shost));
					printf("                                     Dst=%s\n",
					 ether_sprintf(&eh->ether_dhost));
#endif /* APPLETALK_DEBUG */
#endif /* NETAT */
			if ((l->llc_un.type_snap.ether_type == 0x809B) ||
			    (l->llc_un.type_snap.ether_type == 0x80F3)) {

#if NETAT
				/*
				 * note: for AppleTalk we need to pass the enet header of the
				 * packet up stack. To do so, we made sure in that the FULL packet
				 * is copied in the mbuf by the mace driver, and only the m_data and
				 * length have been shifted to make IP and the other guys happy.
				 */

				m->m_data -= sizeof(*eh);
				m->m_len += sizeof(*eh);
				m->m_pkthdr.len += sizeof(*eh);	
#ifdef APPLETALK_DEBUG
				l == (struct llc *)(eh+1);
				if (l->llc_un.type_snap.ether_type == 0x80F3) {
					kprintf("ether_input: RCV AppleTalk type=0x%x Src=%s\n",
					 l->llc_un.type_snap.ether_type,
					 ether_sprintf(&eh->ether_shost));
					kprintf("                                     Dst=%s\n",
					 ether_sprintf(&eh->ether_dhost));
				}
#endif /* APPLETALK_DEBUG */
				schednetisr(NETISR_APPLETALK);
				inq = &atalkintrq ;
#else /* no APPLETALK */
				goto dropanyway;
#endif /* NETAT */
				break;
			}
		}


		switch (l->llc_dsap) {

#if	ISO
		case LLC_ISO_LSAP: 
			switch (l->llc_control) {
			case LLC_UI:
				/* LLC_UI_P forbidden in class 1 service */
				if ((l->llc_dsap == LLC_ISO_LSAP) &&
				    (l->llc_ssap == LLC_ISO_LSAP)) {
					/* LSAP for ISO */
					if (m->m_pkthdr.len > etype)
						m_adj(m, etype - m->m_pkthdr.len);
					m->m_data += 3;		/* XXX */
					m->m_len -= 3;		/* XXX */
					m->m_pkthdr.len -= 3;	/* XXX */
					M_PREPEND(m, sizeof *eh, M_DONTWAIT);
					if (m == 0)
						return;
					*mtod(m, struct ether_header *) = *eh;
					IFDEBUG(D_ETHER)
						printf("clnp packet");
					ENDDEBUG
					schednetisr(NETISR_ISO);
					inq = &clnlintrq;
					break;
				}
				goto dropanyway;
				
			case LLC_XID:
			case LLC_XID_P:
				if(m->m_len < 6)
					goto dropanyway;
				l->llc_window = 0;
				l->llc_fid = 9;
				l->llc_class = 1;
				l->llc_dsap = l->llc_ssap = 0;
				/* Fall through to */
			case LLC_TEST:
			case LLC_TEST_P:
			{
				struct sockaddr sa;
				register struct ether_header *eh2;
				int i;
				u_char c = l->llc_dsap;

				l->llc_dsap = l->llc_ssap;
				l->llc_ssap = c;
				if (m->m_flags & (M_BCAST | M_MCAST))
					bcopy((caddr_t)ac->ac_enaddr,
					      (caddr_t)eh->ether_dhost, 6);
				sa.sa_family = AF_UNSPEC;
				sa.sa_len = sizeof(sa);
				eh2 = (struct ether_header *)sa.sa_data;
				for (i = 0; i < 6; i++) {
					eh2->ether_shost[i] = c = eh->ether_dhost[i];
					eh2->ether_dhost[i] = 
						eh->ether_dhost[i] = eh->ether_shost[i];
					eh->ether_shost[i] = c;
				}
				ifp->if_output(ifp, m, &sa, NULL);
				return;
			}
			default:
				m_freem(m);
				return;
			}
			break;
#endif /* ISO */
#if LLC
		case LLC_X25_LSAP:
		{
			if (m->m_pkthdr.len > etype)
				m_adj(m, etype - m->m_pkthdr.len);
			M_PREPEND(m, sizeof(struct sdl_hdr) , M_DONTWAIT);
			if (m == 0)
				return;
			if ( !sdl_sethdrif(ifp, eh->ether_shost, LLC_X25_LSAP,
					    eh->ether_dhost, LLC_X25_LSAP, 6, 
					    mtod(m, struct sdl_hdr *)))
				panic("ETHER cons addr failure");
			mtod(m, struct sdl_hdr *)->sdlhdr_len = etype;
#if LLC_DEBUG
				printf("llc packet\n");
#endif /* LLC_DEBUG */
			schednetisr(NETISR_CCITT);
			inq = &llcintrq;
			break;
		}
#endif /* LLC */
		dropanyway:
		default:
			m_freem(m);
			return;
		}
#else /* ISO || LLC */
	    m_freem(m);
	    return;
#endif /* ISO || LLC */
	}

	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);

        if ((ifp->if_eflags & IFEF_DVR_REENTRY_OK) && (ptype == IPPROTO_TCP || ptype == IPPROTO_UDP)) {
		extern void ipintr(void);

                s = splnet();
		ipintr();
		splx(s);
	}

}

/*
 * Convert Ethernet address to printable (loggable) representation.
 */
static char digits[] = "0123456789abcdef";
char *
ether_sprintf(ap)
	register u_char *ap;
{
	register i;
	static char etherbuf[18];
	register char *cp = etherbuf;

	for (i = 0; i < 6; i++) {
		*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (etherbuf);
}

/*
 * Perform common duties while attaching to interface list
 */
void
ether_ifattach(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	register struct sockaddr_dl *sdl;
	extern appletalk_hack_start(struct ifnet *);

	ifp->if_type = IFT_ETHER;
	ifp->if_addrlen = 6;
	ifp->if_hdrlen = 14;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) &&
		    sdl->sdl_family == AF_LINK) {
			sdl->sdl_type = IFT_ETHER;
			sdl->sdl_alen = ifp->if_addrlen;
			bcopy((caddr_t)((struct arpcom *)ifp)->ac_enaddr,
			      LLADDR(sdl), ifp->if_addrlen);
			break;
		}
	}

#if NETAT

/*
 * BIG HACK ALERT ### LD 06/02/97
 * This is a way to get the appletalk stack inited. JUST A TEMPO Hack
 */

 appletalk_hack_start(ifp);


#endif /* NETAT */
}

u_char	ether_ipmulticast_min[6] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00 };
u_char	ether_ipmulticast_max[6] = { 0x01, 0x00, 0x5e, 0x7f, 0xff, 0xff };
/*
 * Add an Ethernet multicast address or range of addresses to the list for a
 * given interface.
 */
int
ether_addmulti(ifr, ac)
	struct ifreq *ifr;
	register struct arpcom *ac;
{
	register struct ether_multi *enm;
	struct sockaddr_in *sin;
	u_char addrlo[6];
	u_char addrhi[6];
	int s = splimp();

	switch (ifr->ifr_addr.sa_family) {

	case AF_UNSPEC:
		bcopy(ifr->ifr_addr.sa_data, addrlo, 6);
		bcopy(addrlo, addrhi, 6);
		break;

#if INET
	case AF_INET:
		sin = (struct sockaddr_in *)&(ifr->ifr_addr);
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			/*
			 * An IP address of INADDR_ANY means listen to all
			 * of the Ethernet multicast addresses used for IP.
			 * (This is for the sake of IP multicast routers.)
			 */
			bcopy(ether_ipmulticast_min, addrlo, 6);
			bcopy(ether_ipmulticast_max, addrhi, 6);
		}
		else {
			ETHER_MAP_IP_MULTICAST(&sin->sin_addr, addrlo);
			bcopy(addrlo, addrhi, 6);
		}
		break;
#endif

	default:
		splx(s);
		return (EAFNOSUPPORT);
	}

	/*
	 * Verify that we have valid Ethernet multicast addresses.
	 */
	if ((addrlo[0] & 0x01) != 1 || (addrhi[0] & 0x01) != 1) {
		splx(s);
		return (EINVAL);
	}
	/*
	 * See if the address range is already in the list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ac, enm);
	if (enm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		++enm->enm_refcount;
		splx(s);
		return (0);
	}
	/*
	 * New address or range; malloc a new multicast record
	 * and link it into the interface's multicast list.
	 */
	enm = (struct ether_multi *)_MALLOC(sizeof(*enm), M_IFMADDR, M_WAITOK);
	if (enm == NULL) {
		splx(s);
		return (ENOBUFS);
	}
	bcopy(addrlo, enm->enm_addrlo, 6);
	bcopy(addrhi, enm->enm_addrhi, 6);
	enm->enm_ac = ac;
	enm->enm_refcount = 1;
	enm->enm_next = ac->ac_multiaddrs;
	ac->ac_multiaddrs = enm;
	ac->ac_multicnt++;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return (ENETRESET);
}

/*
 * Delete a multicast address record.
 */
int
ether_delmulti(ifr, ac, ret_mca)
	struct ifreq *ifr;
	register struct arpcom *ac;
	struct ether_addr * ret_mca;
{
	register struct ether_multi *enm;
	register struct ether_multi **p;
	struct sockaddr_in *sin;
	u_char addrlo[6];
	u_char addrhi[6];
	int s = splimp();

	switch (ifr->ifr_addr.sa_family) {

	case AF_UNSPEC:
		bcopy(ifr->ifr_addr.sa_data, addrlo, 6);
		bcopy(addrlo, addrhi, 6);
		break;

#if INET
	case AF_INET:
		sin = (struct sockaddr_in *)&(ifr->ifr_addr);
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			/*
			 * An IP address of INADDR_ANY means stop listening
			 * to the range of Ethernet multicast addresses used
			 * for IP.
			 */
			bcopy(ether_ipmulticast_min, addrlo, 6);
			bcopy(ether_ipmulticast_max, addrhi, 6);
		}
		else {
			ETHER_MAP_IP_MULTICAST(&sin->sin_addr, addrlo);
			bcopy(addrlo, addrhi, 6);
		}
		break;
#endif

	default:
		splx(s);
		return (EAFNOSUPPORT);
	}

	/*
	 * Look up the address in our list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ac, enm);
	if (enm == NULL) {
		splx(s);
		return (ENXIO);
	}
	if (--enm->enm_refcount != 0) {
		/*
		 * Still some claims to this record.
		 */
		splx(s);
		return (0);
	}

	/* save the low and high address of the range before deletion */
	if (ret_mca) {
	   	*ret_mca	= *((struct ether_addr *)addrlo);
	   	*(ret_mca + 1)	= *((struct ether_addr *)addrhi);
	}

	/*
	 * No remaining claims to this record; unlink and free it.
	 */
	for (p = &enm->enm_ac->ac_multiaddrs;
	     *p != enm;
	     p = &(*p)->enm_next)
		continue;
	*p = (*p)->enm_next;
	_FREE(enm, M_IFMADDR);
	ac->ac_multicnt--;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return (ENETRESET);
}

#ifdef sparc
localetheraddr(hint, result)
	struct ether_addr *hint, *result;
{
	static int found = 0;
	static struct ether_addr addr;

	if (!found) {
		found = 1;
		if (hint == NULL)
			return (0);
		addr = *hint;
		printf("Ethernet address = %s\n", ether_sprintf(&addr) );
	}
	if (result != NULL)
		*result = addr;
	return (1);
}
#endif

#if defined(ppc)
/*
 * NB: need to register Atalk addresses from Rhapsody
 */
int
if_register(register struct BlueFilter *f
#ifdef BF_if
	    ,
	    register struct ifnet *ifp
#endif
	    )
{	register int ix;
	register struct BlueFilter *nf;

	if (f->BF_flags & BF_VALID)
		return(EINVAL);
	if (f->BF_flags & BF_IP)
	{	if (BFIx == 0)
			ix = BFIx = 1; /* Starting at 1 for IP */
		else if (BFIx >= BFCount)
			return(0);		/* We'll just deal with it */
		else
			ix = ++BFIx;
	} else if (!(f->BF_flags & BF_ATALK))
		return(EINVAL);
	else			/* AppleTalk */
		ix = 0;

	nf = &RhapFilter[ix];
	*nf = *f;
	nf->BF_flags |= BF_VALID;
#ifdef BF_if
	nf->BF_if = ifp;
#endif
	return(0);
}

/*
 * Y-adapter filter check
 * The rules here:
 *  For Rhap: return 1
 *  For Both: return 0
 *  Not for Rhap: return -1
 *  Multicast/Broadcast => For Both
 *  Atalk address registered
 *   filter matches => For Rhap else Not For Rhap
 *  IP address registered
 *   filter matches => For Rhap else Not For Rhap
 *  For Rhap
 * Note this is *not* a general filter mechanism in that we know
 *  what we *could* be looking for.
 * WARNING: this is a big-endian routine.
 * Note: ARP and AARP packets are implicitly accepted for "both"
 */
int
Filter_check(struct mbuf **m0)
{	register struct BlueFilter *bf;
	register unsigned char *p;
	register unsigned short *s;
	register unsigned long *l;
	int total, flags;
	struct mbuf *m;
	extern struct mbuf *m_pullup(struct mbuf *, int);
	extern void kprintf( const char *, ...);
#define FILTER_LEN 32

	m = *m0;
	flags = m->m_flags;
	if (FILTER_LEN > m->m_pkthdr.len)
		return(1);
	while ((FILTER_LEN > m->m_len) && m->m_next) {
		total = m->m_len + (m->m_next)->m_len;
		if ((m = m_pullup(m, min(FILTER_LEN, total))) == 0)
			return(-1);
	}	 
	*m0 = m;

	p = mtod(m, unsigned char *);	/* Point to destination media addr */
	if (p[0] & 0x01)	/* Multicast/broadcast */
		return(0);
	s = (unsigned short *)p;
	bf = &RhapFilter[BFS_ATALK];
#if 0
	kprintf("!PKT: %x, %x, %x\n", s[6], s[7],	s[8]);
#endif

	if (bf->BF_flags)	/* Filtering Appletalk */
	{
		l = (unsigned long *)&s[8];
#if 0
		kprintf("!AT: %x, %x, %x, %x, %x, %x\n", s[6], s[7],
			*l, s[10], s[13], p[30]);
#endif
		if (s[6] <= ETHERMTU)
		{	if (s[7] == 0xaaaa) /* Could be Atalk */
			{	/* Verify SNAP header */
				if (*l == 0x03080007 && s[10] == 0x809b)
				{	if (s[13] == bf->BF_address &&
					     p[30] == bf->BF_node)
						return(1);
				} else if (*l == 0x03000000 && s[10] == 0x80f3)
					/* AARP pkts aren't net-addressed */
					return(0);
				return(0);
			} else /* Not for us? */
				return(0);
		} /* Fall through */
	} /* Fall through */
	bf++;			/* Look for IP next */
	if (bf->BF_flags)	/* Filtering IP */
	{
		l = (unsigned long *)&s[15];
#if 0
		kprintf("!IP: %x, %x\n", s[6], *l);
#endif
		if (s[6] > ETHERMTU)
		{	if (s[6] == 0x800)	/* Is IP */
			{	/* Verify IP address */
				if (*l == bf->BF_address)
					return(1);
				else	/* Not for us */
					return(0);
			} else if (s[6] == 0x806)
				/* ARP pkts aren't net-addressed */
				return(0);
		}
	}
	return(0);		/* No filters => Accept */
}
#endif /* ppc */
