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
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 */


struct	inpcb_hash_str {
	struct	inpcb		*(*hash_array)[];
	u_long			hash_mask;
	u_long			n_elements;
};


/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct inpcb {
	struct	inpcb *inp_next,*inp_prev;
					/* pointers to other pcb's */
	struct	inpcb *inp_head;	/* pointer back to chain of inpcb's
					   for this protocol */
	struct	in_addr inp_faddr;	/* foreign host table entry */
	u_short	inp_fport;		/* foreign port */
	struct	in_addr inp_laddr;	/* local host table entry */
	u_short	inp_lport;		/* local port */
	struct	socket *inp_socket;	/* back pointer to socket */
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	caddr_t inp_saved_ppcb;		/* place to save pointer while cached */
	struct	route inp_route;	/* placeholder for routing entry */
	int	inp_flags;		/* generic IP/datagram flags */
	struct	ip inp_ip;		/* header prototype; should have more */
	struct	mbuf *inp_options;	/* IP options */
	struct	ip_moptions *inp_moptions; /* IP multicast options */
	struct  inpcb_hash_str *hash_str;
	struct  inpcb_hash_str *lport_hash_str;
	struct	inpcb *hash_next;
	struct	inpcb *hash_prev;
	int	hash_element;
	struct	inpcb *lport_hash_next;
	struct	inpcb *lport_hash_prev;
	int	lport_hash_element;
};

/* flags in inp_flags: */
#define	INP_RECVOPTS		0x01	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x02	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x04	/* receive IP dst address */
#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR)
#define	INP_HDRINCL		0x08	/* user supplies entire IP header */
#define INP_NOLOOKUP            0x10    /* don't need to re-issue lookup in 'in_pcbconnect' */
                                        /* we've already failed to find this address in tcp_input */ 

#define	INPLOOKUP_WILDCARD	1
#define	INPLOOKUP_SETLOCAL	2

#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)

#ifdef _KERNEL
int	 in_losing __P((struct inpcb *));
int	 in_pcballoc __P((struct socket *, struct inpcb *, 
			  struct inpcb_hash_str *, struct inpcb_hash_str *));
int	 in_pcbbind __P((struct inpcb *, struct mbuf *));
int	 in_pcbconnect __P((struct inpcb *, struct mbuf *));
int	 in_pcbdetach __P((struct inpcb *));
int	 in_pcbdisconnect __P((struct inpcb *));
struct inpcb *
	 in_pcblookup __P((struct inpcb *,
	    struct in_addr, u_int, struct in_addr, u_int, int));
int	 in_pcbnotify __P((struct inpcb *, struct sockaddr *,
	    u_int, struct in_addr, u_int, int, void (*)(struct inpcb *, int)));
void	 in_rtchange __P((struct inpcb *, int));
int	 in_setpeeraddr __P((struct inpcb *, struct mbuf *));
int	 in_setsockaddr __P((struct inpcb *, struct mbuf *));
void	 lport_hash_remove __P((struct inpcb *));
void	 lport_hash_insert __P((struct inpcb *));
void	 hash_remove __P((struct inpcb *));
void	 hash_insert __P((struct inpcb *));


struct inpcb *inet_hash1 __P((struct inpcb_hash_str *, u_long, u_short,	u_long, u_short));
struct inpcb *inet_lport_hash1 __P((struct inpcb_hash_str *, u_short));

struct inpcb *
hash_in_pcbwild __P((struct inpcb *, struct in_addr, u_int, struct in_addr, u_int, int));

struct inpcb *
lport_hash_in_pcbwild __P((struct inpcb *, struct in_addr, u_int, struct in_addr, u_int, int));

struct inpcb *
	 hash_in_pcblookup __P((struct inpcb *,
	    struct in_addr, u_int, struct in_addr, u_int));

#endif
