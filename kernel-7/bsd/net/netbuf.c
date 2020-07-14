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
 * Copyright (C) 1990 by NeXT, Inc., All Rights Reserved
 *
 */

/* 
 * Network Buffer handling
 * These should be used instead of mbufs. Currently, they are just
 * wrappers around mbufs, but we hope to flush mbufs one day. Third parties
 * must use this API, or risk breakage after an OS upgrade.
 *
 * HISTORY
 * 09-Apr-90  Bradley Taylor (btaylor) at NeXT, Inc.
 *	Created.
 */

#import <sys/types.h>
#import <sys/param.h>
#import <sys/mbuf.h>
#import <kernserv/machine/spl.h>
#import <net/netbuf.h>

const char
	IFCONTROL_SETFLAGS[] = "setflags",	
	IFCONTROL_SETADDR[] = "setaddr",
	IFCONTROL_GETADDR[] = "getaddr",
	IFCONTROL_AUTOADDR[] = "autoaddr",
	IFCONTROL_UNIXIOCTL[] = "unix-ioctl",
	IFCONTROL_RCVPROMISCUOUS[] = "promiscuous-on",
	IFCONTROL_RCVPROMISCOFF[] = "promiscuous-off",
	IFCONTROL_ADDMULTICAST[] = "add-multicast",
	IFCONTROL_RMVMULTICAST[] = "rmv-multicast";

static void
nb_alloc_free(
	      void *orig_data
	      )
{
	int orig_size = *(unsigned *)orig_data;

        if ( orig_size <= MCLBYTES ) {
		MCLFREE( orig_data );
	} else {
		kfree(orig_data, orig_size);
	}
}

void
nb_delayed_free(void *data, unsigned size, void *orig_data)
{
	int orig_size = *(unsigned *)orig_data;

        if ( orig_size <= MCLBYTES ) {
		MCLFREE( orig_data );
	} else {
		kfree(orig_data, orig_size);
        }
}



netbuf_t
nb_alloc(
	 unsigned data_size
	 )
{
	void *data;
	netbuf_t nb;
	unsigned *orig_data;
	unsigned orig_size;

	orig_size = data_size + 0x20;		/* Add cache-line size to request */

        if ( orig_size <= MCLBYTES ) {
	        MCLALLOC( orig_data, M_DONTWAIT );
        } else { 
		orig_data = (unsigned *)kalloc(orig_size);
        }
	if (orig_data == NULL) {
		return (NULL);
	}
	orig_data[0] = orig_size;
	data = (void *)&orig_data[1];
        data = (void *)(((unsigned int)data + 0x1F) & ~0x1F);	/* Align to cache-line boundary */
	nb = nb_alloc_wrapper(data, data_size, 
			      nb_delayed_free, 
			      (void *)orig_data);
	if (nb == NULL) {
		nb_alloc_free((void *)orig_data);
		return (NULL);
	}

	return (nb);
}
struct mbuf *
mclgetx(fun, arg, addr, len, wait)
	int (*fun)(), arg, len, wait;
	caddr_t addr;
{
	register struct mbuf *m;

	MGETHDR(m, wait, MT_DATA);
	if (m == 0)
		return (0);
//	m->m_off = (int)addr - (int)m;
//	m->m_len = len;
//	m->m_cltype = MCL_LOANED;
//	m->m_clfun = fun;
//	m->m_clarg = arg;
//	m->m_clswp = NULL;
	m->m_ext.ext_buf = addr;
	m->m_ext.ext_free = fun;
	m->m_ext.ext_size = len;
	m->m_ext.ext_arg = arg;
	m->m_ext.ext_refs.forward = m->m_ext.ext_refs.backward =
			&m->m_ext.ext_refs;
	m->m_data = addr;
	m->m_flags |= M_EXT;
	m->m_pkthdr.len = len;
	m->m_len = len;

	return (m);
}

netbuf_t
nb_alloc_wrapper(
		 void *data,
		 unsigned data_size,
		 void data_free(void *arg),
		 void *data_free_arg
		 )
{
	struct mbuf *m;
	m = mclgetx(data_free, data_free_arg, data, data_size, M_DONTWAIT);
	if (m == NULL) {
		return (NULL);
	}
	return ((netbuf_t)m);
}

char *
nb_map(netbuf_t nb)
{
	return (mtod(((struct mbuf *)nb), char *));
}

void
nb_free(netbuf_t nb)
{
	m_freem((struct mbuf *)nb);
}

unsigned
nb_size(netbuf_t nb)
{
	return (((struct mbuf *)nb)->m_len);
}

int
nb_read(netbuf_t nb, unsigned offset, unsigned size, void *target)
{
	struct mbuf *m = (struct mbuf *)nb;
	void *data;
	
	if (offset + size > m->m_len) {
		return (-1);
	}
	data = mtod(m, void *);
	bcopy(data + offset, target, size);
	return (0);
}

int
nb_write(netbuf_t nb, unsigned offset, unsigned size, void *source)
{
	struct mbuf *m = (struct mbuf *)nb;
	void *data;
	
	if (offset + size > m->m_len) {
		return (-1);
	}
	data = mtod(m, void *);
	bcopy(source, data + offset, size);
	return (0);
}
/* Remember to look at the PPP and slip interfaces, which bypassses the
 * m_data field XXXX : (AR, CCG 6/30/95)
 */
int
nb_shrink_top(netbuf_t nb, unsigned size)
{
	struct mbuf *m = (struct mbuf *)nb;
	
	m->m_len -= size;
	m->m_data += size;
	return (0); /* XXX should error check */
}

int
nb_grow_top(netbuf_t nb, unsigned size)
{
	struct mbuf *m = (struct mbuf *)nb;
	
	m->m_len += size;
	m->m_data -= size;
	return (0); /* XXX should error check */
}

int
nb_shrink_bot(netbuf_t nb, unsigned size)
{
	struct mbuf *m = (struct mbuf *)nb;
	
	m->m_len -= size;
	return (0); /* XXX should error check */
}


int
nb_grow_bot(netbuf_t nb, unsigned size)
{
	struct mbuf *m = (struct mbuf *)nb;
	
	m->m_len += size;
	return (0); /* XXX should error check */
}

int
nb_aligned_copy(netbuf_t nb, unsigned aligned)
{
	struct mbuf *m = (struct mbuf *)nb;
	unsigned char *buf_p, *buf_m;
	
	buf_p = buf_m = mtod(m, unsigned char *);
	buf_m -= aligned;
	if (buf_p != buf_m)
	  memmove(buf_m, buf_p,  m->m_len);
	m->m_data = buf_m;
	return(0);
}
