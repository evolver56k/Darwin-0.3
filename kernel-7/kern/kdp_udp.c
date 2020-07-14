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
 * Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved.
 *
 * kdp_udp.c -- Kernel Debugging Protocol UDP implementation.
 *
 * History:
 * 10 Nov 1997	Herb Ruth [ruth1@apple.com]
 *	Added extern declarations for implicit function
 *	declarations.
 *
 */

#import <mach/boolean.h>
#import <mach/exception.h>

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/socket.h>
#import <sys/reboot.h>

#import <net/if.h>
#import <net/route.h>

#import <netinet/in.h>
#import <netinet/in_systm.h>
#import <netinet/in_var.h>
#import <net/etherdefs.h>
#import <netinet/ip.h>
#import <netinet/ip_var.h>
#import <netinet/in_pcb.h>
#import <netinet/udp.h>
#import <netinet/udp_var.h>

#import <kern/kdp_internal.h>
#import <kern/miniMon.h>
#import <kern/kdp_en_debugger.h>

/* 10 Nov 1997 */
#warning extern declarations -- FIXME  XXX
extern void kprintf(const char *format, ...);


#define DO_ALIGN	1	/* align all packet data accesses */

#if NOTYET
#import <bsd/dev/km.h>	// not yet
#else
extern int kmtrygetc(void);
#endif

/*	@(#)udp_usrreq.c	2.2 88/05/23 4.0NFSSRC SMI;	from UCB 7.1 6/5/86	*/

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
#define UDP_TTL	60 /* deflt time to live for UDP packets */
int		udp_ttl=UDP_TTL;
static unsigned char	exception_seq;

static struct {
    unsigned char	data[ETHERMAXPACKET];
    unsigned int	off, len;
    boolean_t		input;
} pkt, saved_reply;

struct {
    struct {
	struct in_addr		in;
	struct ether_addr	ea;
    } loc;
    struct {
	struct in_addr		in;
	struct ether_addr	ea;
    } rmt;
} adr;

static char
*exception_message[] = {
    "Unknown",
    "Memory access",		/* EXC_BAD_ACCESS */
    "Failed instruction",	/* EXC_BAD_INSTRUCTION */
    "Arithmetic",		/* EXC_ARITHMETIC */
    "Emulation",		/* EXC_EMULATION */
    "Software",			/* EXC_SOFTWARE */
    "Breakpoint"		/* EXC_BREAKPOINT */
};

static kdp_send_t kdp_en_send_pkt = 0;
static kdp_receive_t kdp_en_recv_pkt = 0;

void
kdp_register_send_receive(kdp_send_t send, kdp_receive_t receive)
{
    if (kdp_en_send_pkt == 0) { /* the first to register is the debugger */
	kdp_en_send_pkt = send;
	kdp_en_recv_pkt = receive;
    }
}

static
void
kdp_handler(
    void	*
);


static inline
void
enaddr_copy(
    void	*src,
    void	*dst
)
{
    bcopy((caddr_t)src, (caddr_t)dst, sizeof(struct ether_addr));
}

static inline
unsigned short
ip_sum(
    unsigned char	*c,
    unsigned int	hlen
)
{
    unsigned int	high, low, sum;
    
    high = low = 0;
    while (hlen-- > 0) {
	low += c[1] + c[3];
	high += c[0] + c[2];
	
	c += sizeof (int);
    }
    
    sum = (high << 8) + low;
    sum = (sum >> 16) + (sum & 65535);
    
    return (sum > 65535 ? sum - 65535 : sum);
}

static
void
kdp_reply(
    unsigned short		reply_port
)
{
    struct udpiphdr		aligned_ui, *ui = &aligned_ui;
    struct ip			aligned_ip, *ip = &aligned_ip;
    struct in_addr		tmp_ipaddr;
    struct ether_addr		tmp_enaddr;
    struct ether_header		*eh;
    
    if (!pkt.input)
	kdp_panic("kdp_reply");
	
    pkt.off -= sizeof (struct udpiphdr);

#if DO_ALIGN    
    bcopy(&pkt.data[pkt.off], ui, sizeof(*ui));
#else
    ui = (struct udpiphdr *)&pkt.data[pkt.off];
#endif
    ui->ui_next = ui->ui_prev = 0;
    ui->ui_x1 = 0;
    ui->ui_pr = IPPROTO_UDP;
    ui->ui_len = htons((u_short)pkt.len + sizeof (struct udphdr));
    tmp_ipaddr = ui->ui_src;
    ui->ui_src = ui->ui_dst;
    ui->ui_dst = tmp_ipaddr;
    ui->ui_sport = htons(KDP_REMOTE_PORT);
    ui->ui_dport = reply_port;
    ui->ui_ulen = ui->ui_len;
    ui->ui_sum = 0;
#if DO_ALIGN
    bcopy(ui, &pkt.data[pkt.off], sizeof(*ui));
    
    bcopy(&pkt.data[pkt.off], ip, sizeof(*ip));
#else
    ip = (struct ip *)&pkt.data[pkt.off];
#endif
    ip->ip_len = htons(sizeof (struct udpiphdr) + pkt.len);
    ip->ip_v = IPVERSION;
    ip->ip_id = htons(ip_id++);
    ip->ip_hl = sizeof (struct ip) >> 2;
    ip->ip_ttl = udp_ttl;
    ip->ip_sum = 0;
    ip->ip_sum = htons(~ip_sum((unsigned char *)ip, ip->ip_hl));
#if DO_ALIGN
    bcopy(ip, &pkt.data[pkt.off], sizeof(*ip));
#endif
    
    pkt.len += sizeof (struct udpiphdr);
    
    pkt.off -= sizeof (struct ether_header);
    
    eh = (struct ether_header *)&pkt.data[pkt.off];
    enaddr_copy(eh->ether_shost, &tmp_enaddr);
    enaddr_copy(eh->ether_dhost, eh->ether_shost);
    enaddr_copy(&tmp_enaddr, eh->ether_dhost);
    eh->ether_type = htons(ETHERTYPE_IP);
    
    pkt.len += sizeof (struct ether_header);
    
    // save reply for possible retransmission
    bcopy(&pkt, &saved_reply, sizeof(pkt));

    (*kdp_en_send_pkt)(&pkt.data[pkt.off], pkt.len);

    // increment expected sequence number
    exception_seq++;
}

static
void
kdp_send(
    unsigned short		remote_port
)
{
    struct udpiphdr		aligned_ui, *ui = &aligned_ui;
    struct ip			aligned_ip, *ip = &aligned_ip;
    struct ether_header		*eh;
    
    if (pkt.input)
	kdp_panic("kdp_send");

    pkt.off -= sizeof (struct udpiphdr);

#if DO_ALIGN
    bcopy(&pkt.data[pkt.off], ui, sizeof(*ui));
#else
    ui = (struct udpiphdr *)&pkt.data[pkt.off];
#endif
    ui->ui_next = ui->ui_prev = 0;
    ui->ui_x1 = 0;
    ui->ui_pr = IPPROTO_UDP;
    ui->ui_len = htons((u_short)pkt.len + sizeof (struct udphdr));
    ui->ui_src = adr.loc.in;
    ui->ui_dst = adr.rmt.in;
    ui->ui_sport = htons(KDP_REMOTE_PORT);
    ui->ui_dport = remote_port;
    ui->ui_ulen = ui->ui_len;
    ui->ui_sum = 0;
#if DO_ALIGN
    bcopy(ui, &pkt.data[pkt.off], sizeof(*ui));
    
    bcopy(&pkt.data[pkt.off], ip, sizeof(*ip));
#else
    ip = (struct ip *)&pkt.data[pkt.off];
#endif
    ip->ip_len = htons(sizeof (struct udpiphdr) + pkt.len);
    ip->ip_v = IPVERSION;
    ip->ip_id = htons(ip_id++);
    ip->ip_hl = sizeof (struct ip) >> 2;
    ip->ip_ttl = udp_ttl;
    ip->ip_sum = 0;
    ip->ip_sum = htons(~ip_sum((unsigned char *)ip, ip->ip_hl));
#if DO_ALIGN
    bcopy(ip, &pkt.data[pkt.off], sizeof(*ip));
#endif
    
    pkt.len += sizeof (struct udpiphdr);
    
    pkt.off -= sizeof (struct ether_header);
    
    eh = (struct ether_header *)&pkt.data[pkt.off];
    enaddr_copy(&adr.loc.ea, eh->ether_shost);
    enaddr_copy(&adr.rmt.ea, eh->ether_dhost);
    eh->ether_type = htons(ETHERTYPE_IP);
    
    pkt.len += sizeof (struct ether_header);
    
    (*kdp_en_send_pkt)(&pkt.data[pkt.off], pkt.len);
}

static
void
kdp_poll(
    void
)
{
    struct ether_header		*eh;
    struct udpiphdr		aligned_ui, *ui = &aligned_ui;
    struct ip			aligned_ip, *ip = &aligned_ip;

    if (pkt.input)
	kdp_panic("kdp_poll");
 
    if (!kdp_en_recv_pkt || !kdp_en_send_pkt)
	kdp_panic("kdp_poll: no debugger device\n");

    pkt.off = 0;
    (*kdp_en_recv_pkt)(pkt.data, &pkt.len, 3/* ms */);
    
    if (pkt.len == 0)
	return;
    
    if (pkt.len < (sizeof (struct ether_header) + sizeof (struct udpiphdr)))
    	return;
	
    eh = (struct ether_header *)&pkt.data[pkt.off];
    pkt.off += sizeof (struct ether_header);
    if (ntohs(eh->ether_type) != ETHERTYPE_IP) {
	return;
    }

#if DO_ALIGN
    bcopy(&pkt.data[pkt.off], ui, sizeof(*ui));
    bcopy(&pkt.data[pkt.off], ip, sizeof(*ip));
#else
    ui = (struct udpiphdr *)&pkt.data[pkt.off];
    ip = (struct ip *)&pkt.data[pkt.off];
#endif

    pkt.off += sizeof (struct udpiphdr);
    if (ui->ui_pr != IPPROTO_UDP) {
	return;
    }
 
    if (ip->ip_hl > (sizeof (struct ip) >> 2)) {
	return;
    }

    if (ntohs(ui->ui_dport) != KDP_REMOTE_PORT) {
	return;
    }
	
    if (!kdp.is_conn) {
	enaddr_copy(eh->ether_dhost, &adr.loc.ea);
	adr.loc.in = ui->ui_dst;

	enaddr_copy(eh->ether_shost, &adr.rmt.ea);
	adr.rmt.in = ui->ui_src;
    }

    /*
     * Calculate kdp packet length.
     */
    pkt.len = ntohs((u_short)ui->ui_ulen) - sizeof (struct udphdr);
    pkt.input = TRUE;
}

static
void
kdp_handler(
    void			*saved_state
)
{
    unsigned short		reply_port;
    kdp_hdr_t			aligned_hdr, *hdr = &aligned_hdr;


    kdp.saved_state = saved_state;  // see comment in kdp_raise_exception

    do {
	while (!pkt.input)
	    kdp_poll();
	    		
#if DO_ALIGN
	bcopy(&pkt.data[pkt.off], hdr, sizeof(*hdr));
#else
	hdr = (kdp_hdr_t *)&pkt.data[pkt.off];
#endif

	// ignore replies -- we're not expecting them anyway.
	if (hdr->is_reply) {
	    goto again;
	}
	
	// check for retransmitted request
	if (hdr->seq == (exception_seq - 1)) {
	    /* retransmit last reply */
	    (*kdp_en_send_pkt)(&saved_reply.data[saved_reply.off],
			    saved_reply.len);
	    goto again;
	} else if (hdr->seq != exception_seq) {
	    safe_prf("kdp: bad sequence %d (want %d)\n",
			hdr->seq, exception_seq);
	    goto again;
	}
	
	if (kdp_packet(&pkt.data[pkt.off], &pkt.len, &reply_port)) {
	    kdp_reply(reply_port);
	}

again:
	pkt.input = FALSE;
    } while (kdp.is_halted);
}

static
void
kdp_connection_wait(
    void
)
{
    unsigned short	reply_port;

    safe_prf("Waiting for remote debugger connection.\n");
    safe_prf("(Type 'c' to continue or 'r' to reboot)\n");

    exception_seq = 0;
    do {
	kdp_hdr_t aligned_hdr, *hdr = &aligned_hdr;
	
	while (!pkt.input) {
	    int c;
	    c = kmtrygetc();
	    switch(c) {
		case 'c':
		    safe_prf("Continuing...\n");
		    return;
		case 'r':
		    safe_prf("Rebooting...\n");
		    kdp_reboot();
		    break;
		default:
		    break;
	    }
	    kdp_poll();
	}

	// check for sequence number of 0
#if DO_ALIGN
	bcopy(&pkt.data[pkt.off], hdr, sizeof(*hdr));
#else
	hdr = (kdp_hdr_t *)&pkt.data[pkt.off];
#endif
	if ((hdr->request == KDP_CONNECT) &&
		!hdr->is_reply && (hdr->seq == exception_seq)) {
	    if (kdp_packet(&pkt.data[pkt.off], &pkt.len, &reply_port))
		kdp_reply(reply_port);
	  }
	    
	pkt.input = FALSE;
    } while (!kdp.is_conn);
    
    safe_prf("Connected to remote debugger.\n");
}

static
void
kdp_send_exception(
    unsigned int		exception,
    unsigned int		code,
    unsigned int		subcode
)
{
    unsigned short		remote_port;
    unsigned int		timeout_count;

    timeout_count = 300;	// should be about 30 seconds
    do {
	pkt.off = sizeof (struct ether_header) + sizeof (struct udpiphdr);
	kdp_exception(&pkt.data[pkt.off], &pkt.len, &remote_port,
			exception, code, subcode);
    
	kdp_send(remote_port);
    
	kdp_poll();
	
	if (pkt.input)
	    kdp_exception_ack(&pkt.data[pkt.off], pkt.len);
	    
	pkt.input = FALSE;
	if (kdp.exception_ack_needed)
	    kdp_us_spin(100000);	// 1/10 sec

    } while (kdp.exception_ack_needed && timeout_count--);
    
    if (kdp.exception_ack_needed) {
	// give up & disconnect
	safe_prf("kdp: exception ack timeout\n");
	kdp_reset();
    }
}

int
kdp_processing_packet(void)
{
	return pkt.input;
}

void
kdp_raise_exception(
    unsigned int		exception,
    unsigned int		code,
    unsigned int		subcode,
    void			*saved_state
)
{
    int			s = kdp_intr_disbl();
    int			index;

    if (saved_state == 0)
	safe_prf("kdp_raise_exception: saved_state is NULL!\n");

    index = exception;
    if (exception != EXC_BREAKPOINT) {
	if (exception > EXC_BREAKPOINT || exception < EXC_BAD_ACCESS) {
	    index = 0;
	    kprintf("kdp_raise_exception: unknown exception value (%d), changing to zero.\n", exception);
	}
	kprintf("kdp_raise_exception: %s exception (exception = %x, code = %x, subcode = %x)\n",
		exception_message[index],
		exception,
		code,
		subcode);
	safe_prf("%s exception (%x,%x,%x)\n",
		exception_message[index],
		exception, code, subcode);
    }

    kdp_flush_cache();
    
	/* XXX WMG it seems that sometimes it doesn't work to let kdp_handler
	 * do this. I think the client and the host can get out of sync.
	 */
	kdp.saved_state = saved_state;

    if (pkt.input)
	kdp_panic("kdp_raise_exception");

    if (!kdp.is_conn)
	kdp_connection_wait();
    else
	kdp_send_exception(exception, code, subcode);
	
    if (kdp.is_conn) {
	kdp.is_halted = TRUE;		/* XXX */
    
	kdp_handler(saved_state);
	
	if (!kdp.is_conn)
	    safe_prf("Remote debugger disconnected.\n");
    }
    
    kdp_flush_cache();
    kdp_intr_enbl(s);
}

void
kdp_reset(void)
{
    kdp.reply_port = kdp.exception_port = 0;
    kdp.is_halted = kdp.is_conn = FALSE;
    kdp.exception_seq = kdp.conn_seq = 0;
}

