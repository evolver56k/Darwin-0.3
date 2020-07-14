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
 * kdp.c -- Kernel Debugging Protocol.
 *
 * History:
 *
 * 06 Nov 1997	Herb Ruth [ruth1@apple.com]
 *	Added extern declarations for implicit function declarations.
 *	Added KDP_TEST_HARNESS macro.
 *	Changed dprintf macro to call kprintf() instead of safe_prf() when
 *		KDP_TEST_HARNESS==1
 *
 */

#import <mach/mach_types.h>

#import <bsd/sys/param.h>
#import <sys/systm.h>
#import <bsd/sys/socket.h>
#import <bsd/net/if.h>
#import <bsd/netinet/in.h>

#import <kern/kdp_internal.h>
#import <kern/kdp_private.h>
#import <kern/miniMon.h>

/* 06 Nov 1997 */
#warning extern declarations -- FIXME  XXX

extern void kprintf(const char *format, ...);
extern int copy_for_kdp(vm_offset_t src, vm_offset_t dst, unsigned count, int dir);

#define DO_ALIGN	1	/* align all packet data accesses */

#define KDP_TEST_HARNESS 0
#if KDP_TEST_HARNESS
/*#define dprintf(x) safe_prf x*/
#define dprintf(x) kprintf x
#else
#define dprintf(x)
#endif

static kdp_dispatch_t
    dispatch_table[KDP_TERMINATION - KDP_CONNECT + 1] =
    {
	/* 0 */	kdp_connect,
	/* 1 */	kdp_disconnect,
	/* 2 */	kdp_hostinfo,
	/* 3 */	kdp_regions,
	/* 4 */	kdp_maxbytes,
	/* 5 */	kdp_readmem,
	/* 6 */	kdp_writemem,
	/* 7 */	kdp_readregs,
	/* 8 */	kdp_writeregs,
	/* 9 */	kdp_unknown,
	/* A */	kdp_unknown,
	/* B */	kdp_suspend,
	/* C */	kdp_resumecpus,
	/* D */	kdp_unknown,	/* KDP_EXCEPTION */
	/* E */	kdp_unknown	/* KDP_TERMINATION */
    };
    
kdp_glob_t	kdp;

boolean_t
kdp_packet(
    unsigned char	*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    static unsigned	aligned_pkt[1538/sizeof(unsigned)+1]; // max ether pkt
    kdp_pkt_t		*rd = (kdp_pkt_t *)&aligned_pkt;
    int			plen = *len;
    unsigned int	req;
    boolean_t		ret;
    
#if DO_ALIGN
    bcopy(pkt, rd, sizeof(aligned_pkt));
#else
    rd = (kdp_pkt_t *)pkt;
#endif
    if (plen < sizeof (rd->hdr) || rd->hdr.len != plen) {
	dprintf(("kdp_packet: bad len pkt %d hdr %d\n", plen, rd->hdr.len));

	return (FALSE);
    }
    
    if (rd->hdr.is_reply) {
	dprintf(("kdp_packet: reply recvd req 0x%x seq 0x%x\n",
	    rd->hdr.request, rd->hdr.seq));

	return (FALSE);  
    }
    
    req = rd->hdr.request;
    if (req < KDP_CONNECT || req > KDP_TERMINATION) {
	dprintf(("kdp_packet: bad request 0x%x, len %d, seq 0x%x, key 0x%x\n",
	    rd->hdr.request, rd->hdr.len, rd->hdr.seq, rd->hdr.key));

	return (FALSE);
    }
    
    ret = ((*dispatch_table[req - KDP_CONNECT])(rd, len, reply_port));
#if DO_ALIGN
    bcopy(rd, pkt, *len);
#endif
    return ret;
}

static boolean_t
kdp_unknown(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_pkt_t		*rd = (kdp_pkt_t *)pkt;

    dprintf(("kdp_unknown: request 0x%x, len %d, seq 0x%x, key 0x%x\n",
	rd->hdr.request, rd->hdr.len, rd->hdr.seq, rd->hdr.key));

    return (FALSE);
}

static boolean_t
kdp_connect(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_connect_req_t	*rq = &pkt->connect_req;
    int			plen = *len;
    kdp_connect_reply_t	*rp = &pkt->connect_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    dprintf(("kdp_connect: seq 0x%x, greeting %s\n", rq->hdr.seq, rq->greeting));

    if (kdp.is_conn) {
	if (rq->hdr.seq == kdp.conn_seq)	/* duplicate request */
	    rp->error = KDPERR_NO_ERROR;
	else
	    rp->error = KDPERR_ALREADY_CONNECTED;
    }
    else { 
	kdp.reply_port = rq->req_reply_port;
	kdp.exception_port = rq->exc_note_port;
	kdp.is_conn = TRUE;
	kdp.conn_seq = rq->hdr.seq;
    
	rp->error = KDPERR_NO_ERROR;
    }

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_disconnect(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_disconnect_req_t	*rq = &pkt->disconnect_req;
    int				plen = *len;
    kdp_disconnect_reply_t	*rp = &pkt->disconnect_reply;

    if (plen < sizeof (*rq))
	return (FALSE);
	
    if (!kdp.is_conn)
	return (FALSE);

    dprintf(("kdp_disconnect\n"));
 
    *reply_port = kdp.reply_port;

    kdp.reply_port = kdp.exception_port = 0;
    kdp.is_halted = kdp.is_conn = FALSE;
    kdp.exception_seq = kdp.conn_seq = 0;

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);
    
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_hostinfo(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_hostinfo_req_t	*rq = &pkt->hostinfo_req;
    int			plen = *len;
    kdp_hostinfo_reply_t *rp = &pkt->hostinfo_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    dprintf(("kdp_hostinfo\n"));

    kdp_machine_hostinfo(&rp->hostinfo);
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_suspend(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_suspend_req_t	*rq = &pkt->suspend_req;
    int			plen = *len;
    kdp_suspend_reply_t *rp = &pkt->suspend_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    dprintf(("kdp_suspend\n"));

    kdp.is_halted = TRUE;
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_resumecpus(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_resumecpus_req_t	*rq = &pkt->resumecpus_req;
    int			plen = *len;
    kdp_resumecpus_reply_t 	*rp = &pkt->resumecpus_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    dprintf(("kdp_resumecpus: cpu mask 0x%x\n", rq->cpu_mask));

    kdp.is_halted = FALSE;
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_writemem(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_writemem_req_t	*rq = &pkt->writemem_req;
    int			plen = *len;
    kdp_writemem_reply_t *rp = &pkt->writemem_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    if (rq->nbytes > MAX_KDP_DATA_SIZE)
	rp->error = KDPERR_BAD_NBYTES;
    else {
	dprintf(("kdp_writemem: addr 0x%08x, size %d bytes\n", rq->address, rq->nbytes));
	/* kernacc() ?? */
#if defined(ppc)
	if (copy_for_kdp((vm_offset_t)rq->data, (vm_offset_t)rq->address, rq->nbytes, 1))
		rp->error = KDPERR_BAD_NBYTES;
	else
		rp->error = KDPERR_NO_ERROR;
#else
	copywithin(rq->data, rq->address, rq->nbytes);
	rp->error = KDPERR_NO_ERROR;
#endif
    }

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_readmem(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_readmem_req_t	*rq = &pkt->readmem_req;
    int			plen = *len;
    kdp_readmem_reply_t *rp = &pkt->readmem_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    if (rq->nbytes > MAX_KDP_DATA_SIZE)
	rp->error = KDPERR_BAD_NBYTES;
    else {
	unsigned int	n = rq->nbytes;

	dprintf(("kdp_readmem: addr 0x%08x, size %d bytes\n", rq->address, rq->nbytes));
	/* kernacc() ?? */
#if defined(ppc)
	if (copy_for_kdp((vm_offset_t)rq->address, (vm_offset_t)rp->data, rq->nbytes, 0))
		rp->error = KDPERR_BAD_NBYTES;
	else
		rp->error = KDPERR_NO_ERROR;
#else
	copywithin(rq->address, rp->data, rq->nbytes);
	rp->error = KDPERR_NO_ERROR;
#endif

	rp->hdr.len += n;
    }

    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_maxbytes(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_maxbytes_req_t	*rq = &pkt->maxbytes_req;
    int			plen = *len;
    kdp_maxbytes_reply_t *rp = &pkt->maxbytes_reply;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    dprintf(("kdp_maxbytes\n"));

    rp->max_bytes = MAX_KDP_DATA_SIZE;

    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_regions(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_regions_req_t	*rq = &pkt->regions_req;
    int			plen = *len;
    kdp_regions_reply_t *rp = &pkt->regions_reply;
    kdp_region_t	*r;	

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);

    dprintf(("kdp_regions\n"));

    r = rp->regions;
    rp->nregions = 0;

#if !defined(ppc)
    (vm_offset_t)r->address = VM_MIN_KERNEL_ADDRESS;
    r->nbytes = VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS;
#else
    (vm_offset_t)r->address = VM_MIN_KERNEL_ADDRESS;
    r->nbytes = (VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) + 0x50000000;
#endif
    r->protection = VM_PROT_ALL; r++; rp->nregions++;
    
    rp->hdr.len += rp->nregions * sizeof (kdp_region_t);
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_writeregs(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_writeregs_req_t	*rq = &pkt->writeregs_req;
    int			plen = *len;
    int			size;
    kdp_writeregs_reply_t *rp = &pkt->writeregs_reply;

    if (plen < sizeof (*rq))
	return (FALSE);
    
    size = rq->hdr.len - sizeof(kdp_hdr_t) - sizeof(unsigned int);
    rp->error = kdp_machine_write_regs(rq->cpu, rq->flavor, rq->data, &size);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);
    
    dprintf(("kdp_writeregs\n"));

    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}

static boolean_t
kdp_readregs(
    kdp_pkt_t		*pkt,
    int			*len,
    unsigned short	*reply_port
)
{
    kdp_readregs_req_t	*rq = &pkt->readregs_req;
    int			plen = *len;
    kdp_readregs_reply_t *rp = &pkt->readregs_reply;
    int			size;

    if (plen < sizeof (*rq))
	return (FALSE);

    rp->hdr.is_reply = 1;
    rp->hdr.len = sizeof (*rp);
    
    dprintf(("kdp_readregs\n"));

    rp->error = kdp_machine_read_regs(rq->cpu, rq->flavor, rp->data, &size);
    rp->hdr.len += size;
    
    *reply_port = kdp.reply_port;
    *len = rp->hdr.len;
    
    return (TRUE);
}
