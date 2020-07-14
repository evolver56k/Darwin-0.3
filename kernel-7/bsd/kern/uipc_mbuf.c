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
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
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
 *	@(#)uipc_mbuf.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <net/netisr.h>

struct mbuf 	*mfree;		/* mbuf free list */
struct	mbuf *mfreelater;	/* mbuf deallocation list */
extern vm_map_t mb_map;		/* special map */
int		m_want;		/* sleepers on mbufs */
extern int	nmbclusters;	/* max number of mapped clusters */
short		*mclrefcnt; 	/* mapped cluster reference counts */
union mcluster 	*mclfree;	/* mapped cluster free list */
int		max_linkhdr;	/* largest link-level header */
int		max_protohdr;	/* largest protocol header */
int		max_hdr;	/* largest link+protocol header */
int		max_datalen;	/* MHLEN - max_hdr */
struct mbstat 	mbstat;		/* statistics */
union mcluster 	*mbutl;		/* first mapped cluster address */
union 	mcluster embutl;	/* virtual address of mclusters */

static int 	nclpp;		/* # clusters per physical page */
static char	mbfail[] = "mbuf not mapped";

static int m_howmany();

/* The number of cluster mbufs that are allocated, to start. */
#define MINCL	max(16, 2)

void
mbinit()
{
	int s,m;

	if (nclpp)
		return;
	s = splimp();
	nclpp = round_page(MCLBYTES) / MCLBYTES;	/* see mbufgc() */
	if (nclpp < 1) nclpp = 1;
	MBUF_LOCKINIT();
//	NETISR_LOCKINIT();
	if (nmbclusters == 0)
		nmbclusters = NMBCLUSTERS;
	MALLOC(mclrefcnt, short *, nmbclusters * sizeof (short),
					M_TEMP, M_WAITOK);
	if (mclrefcnt == 0)
		panic("mbinit");
	for (m = 0; m < nmbclusters; m++)
		mclrefcnt[m] = -1;
	MBUF_LOCK();
	if (m_clalloc(max(4096/CLBYTES, 1) * 10, M_DONTWAIT) == 0)
		goto bad;
	MBUF_UNLOCK();
	splx(s);
	return;
bad:
		panic("mbinit");
}

/*
 * Allocate some number of mbuf clusters
 * and place on cluster free list.
 * Must be called at splimp.
 */
/* ARGSUSED */
m_clalloc(ncl, nowait)
	register int ncl;
	int nowait;
{
	register union mcluster *mcl;
	register int i;
	vm_size_t size;
	static char doing_alloc;

	if (doing_alloc || (i = m_howmany()) <= 0)
		goto out;
	doing_alloc = 1;
	MBUF_UNLOCK();

	if (ncl < i) ncl = i;
	size = round_page(ncl * MCLBYTES);
	mcl = (union mcluster *)kmem_mb_alloc(mb_map, size);
	if (mcl == 0 && ncl > 1) {
		size = round_page(MCLBYTES); /* Try for 1 if failed */
		mcl = (union mcluster *)kmem_mb_alloc(mb_map, size);
	}
	MBUF_LOCK();
	doing_alloc = 0;
	if (mcl) {
		ncl = size / MCLBYTES;
		for (i = 0; i < ncl; i++) {
			if (++mclrefcnt[mtocl(mcl)] != 0)
				panic("m_clalloc already there");
			mcl->mcl_next = mclfree;
			mclfree = mcl++;
		}
		mbstat.m_clfree += ncl;
		mbstat.m_clusters += ncl;
		return (ncl);
	} /* else ... */
out:
	if (mclfree)
		return 1;
	mbstat.m_drops++;
	return 0;
}

/*
 * Add more free mbufs by cutting up a cluster.
 */
m_expand(canwait)
	int canwait;
{
	register caddr_t mcl;

	if (mbstat.m_clfree < (mbstat.m_clusters >> 4))
	  /* 1/16th of the total number of cluster mbufs allocated is
	     reserved for large packets.  The number reserved must
	     always be < 1/2, or future allocation will be prevented.
	  */
        	return 0;

	MCLALLOC(mcl, canwait);
	if (mcl) {
		register struct mbuf *m = (struct mbuf *)mcl;
		register int i = NMBPCL;
		int s = splimp();
		MBUF_LOCK();
		mbstat.m_mtypes[MT_FREE] += i;
		mbstat.m_mbufs += i;
		while (i--) {
			m->m_type = MT_FREE;
			m->m_next = mfree;
			mfree = m++;
		}
		i = m_want;
		m_want = 0;
		MBUF_UNLOCK();
		splx(s);
		if (i) wakeup((caddr_t)&mfree);
		return 1;
	}
	return 0;
}

/*
 * When MGET failes, ask protocols to free space when short of memory,
 * then re-attempt to allocate an mbuf.
 */
struct mbuf *
m_retry(canwait, type)
	int canwait, type;
{
#define	m_retry(h, t)	0
	register struct mbuf *m;
	int wait, s;

	for (;;) {
		(void) m_expand(canwait);
		MGET(m, XXX, type);
		if (m || canwait == M_DONTWAIT)
			break;
		s = splimp();
		MBUF_LOCK();
		wait = m_want++;
		if (wait == 0)
			mbstat.m_drain++;
		else {
			assert_wait((caddr_t)&mfree, FALSE);
			mbstat.m_wait++;
		}
		MBUF_UNLOCK();
		if (wait == 0) {
			splx(s);
			m_reclaim();
		} else {
			/* Sleep with a small timeout as insurance */
			(void) tsleep((caddr_t)0, PZERO-1, "m_retry", hz);
			splx(s);
		}
	}
	return (m);
#undef	m_retry
}

/*
 * As above; retry an MGETHDR.
 */
struct mbuf *
m_retryhdr(canwait, type)
	int canwait, type;
{
	register struct mbuf *m;

	if (m = m_retry(canwait, type)) {
		m->m_flags |= M_PKTHDR;
		m->m_data = m->m_pktdat;
	}
	return (m);
}

m_reclaim()
{
	register struct domain *dp;
	register struct protosw *pr;
	int s = splimp();

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_drain)
				(*pr->pr_drain)();
	splx(s);
	mbstat.m_drain++;
}

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(nowait, type)
	int nowait, type;
{
	register struct mbuf *m;

	MGET(m, nowait, type);
	return (m);
}

struct mbuf *
m_gethdr(nowait, type)
	int nowait, type;
{
	register struct mbuf *m;

	MGETHDR(m, nowait, type);
	return (m);
}

struct mbuf *
m_getclr(nowait, type)
	int nowait, type;
{
	register struct mbuf *m;

	MGET(m, nowait, type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

struct mbuf *
m_free(m)
	struct mbuf *m;
{
	struct mbuf *n = m->m_next;
	int i, s;

	if (m->m_type == MT_FREE)
		panic("freeing free mbuf");
	s = splimp();
	MBUF_LOCK();
	if (m->m_flags & M_EXT) {
		if (MCLHASREFERENCE(m)) {
			remque((queue_t)&m->m_ext.ext_refs);
		} else if (m->m_ext.ext_free == NULL) {
				union mcluster *mcl= (union mcluster *)m->m_ext.ext_buf;
				if (MCLUNREF(mcl)) {
				mcl->mcl_next = mclfree;
				mclfree = mcl;
				++mbstat.m_clfree;
			} else	/* sanity check - not referenced this way */
				panic("m_free m_ext cluster not free");
		} else {
			(*(m->m_ext.ext_free))(m->m_ext.ext_buf,
			    m->m_ext.ext_size, m->m_ext.ext_arg);
		}
	}
	mbstat.m_mtypes[m->m_type]--;
	(void) MCLUNREF(m);
	m->m_type = MT_FREE;
	mbstat.m_mtypes[m->m_type]++;
	m->m_flags = 0;
	m->m_next = mfree;
	m->m_len = 0;
	mfree = m;
	i = m_want;
	m_want = 0;
	MBUF_UNLOCK();
	splx(s);
	if (i) wakeup((caddr_t)&mfree);
	return (n);
}

void
m_freem(m)
	register struct mbuf *m;
{
	while (m)
		m = m_free(m);
}

/*
 * Mbuffer utility routines.
 */
/*
 * Compute the amount of space available
 * before the current start of data in an mbuf.
 */
m_leadingspace(m)
register struct mbuf *m;
{
	if (m->m_flags & M_EXT) {
		if (MCLHASREFERENCE(m))
			return(0);
		return (m->m_data - m->m_ext.ext_buf);
	}
	if (m->m_flags & M_PKTHDR)
		return (m->m_data - m->m_pktdat);
	return (m->m_data - m->m_dat);
}

/*
 * Compute the amount of space available
 * after the end of data in an mbuf.
 */
m_trailingspace(m)
register struct mbuf *m;
{
	if (m->m_flags & M_EXT) {
		if (MCLHASREFERENCE(m))
			return(0);
		return (m->m_ext.ext_buf + m->m_ext.ext_size -
			(m->m_data + m->m_len));
	}
	return (&m->m_dat[MLEN] - (m->m_data + m->m_len));
}

/*
 * Lesser-used path for M_PREPEND:
 * allocate new mbuf to prepend to chain,
 * copy junk along.
 */
struct mbuf *
m_prepend(m, len, how)
	register struct mbuf *m;
	int len, how;
{
	struct mbuf *mn;

	MGET(mn, how, m->m_type);
	if (mn == (struct mbuf *)NULL) {
		m_freem(m);
		return ((struct mbuf *)NULL);
	}
	if (m->m_flags & M_PKTHDR) {
		M_COPY_PKTHDR(mn, m);
		m->m_flags &= ~M_PKTHDR;
	}
	mn->m_next = m;
	m = mn;
	if (len < MHLEN)
		MH_ALIGN(m, len);
	m->m_len = len;
	return (m);
}

/*
 * Make a copy of an mbuf chain starting "off0" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * The wait parameter is a choice of M_WAIT/M_DONTWAIT from caller.
 */
int MCFail;

struct mbuf *
m_copym(m, off0, len, wait)
	register struct mbuf *m;
	int off0, wait;
	register int len;
{
	register struct mbuf *n, **np;
	register int off = off0;
	struct mbuf *top;
	int copyhdr = 0;

	if (off < 0 || len < 0)
		panic("m_copym");
	if (off == 0 && m->m_flags & M_PKTHDR)
		copyhdr = 1;
	while (off > 0) {
		if (m == 0)
			panic("m_copym");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copym");
			break;
		}
		MGET(n, wait, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		if (copyhdr) {
			M_COPY_PKTHDR(n, m);
			if (len == M_COPYALL)
				n->m_pkthdr.len -= off0;
			else
				n->m_pkthdr.len = len;
			copyhdr = 0;
		}
		if (len == M_COPYALL) {
		    if (min(len, (m->m_len - off)) == len) {
			printf("m->m_len %d - off %d = %d, %d\n", 
			       m->m_len, off, m->m_len - off,
			       min(len, (m->m_len - off)));
		    }
		}
		n->m_len = min(len, (m->m_len - off));
		if (n->m_len == M_COPYALL) {
		    printf("n->m_len == M_COPYALL, fixing\n");
		    n->m_len = MHLEN;
		}
		if (m->m_flags & M_EXT) {
			int s = splimp();
			MBUF_LOCK();
			n->m_ext = m->m_ext;
			insque((queue_t)&n->m_ext.ext_refs, (queue_t)&m->m_ext.ext_refs);
 			MBUF_UNLOCK();
			splx(s);
			n->m_data = m->m_data + off;
			n->m_flags |= M_EXT;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	if (top == 0)
		MCFail++;
	return (top);
nospace:
	m_freem(top);
	MCFail++;
	return (0);
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
m_copydata(m, off, len, cp)
	register struct mbuf *m;
	register int off;
	register int len;
	caddr_t cp;
{
	register unsigned count;

	if (off < 0 || len < 0)
		panic("m_copydata");
	while (off > 0) {
		if (m == 0)
			panic("m_copydata");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		if (m == 0)
			panic("m_copydata");
		count = min(m->m_len - off, len);
		bcopy(mtod(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

/*
 * Concatenate mbuf chain n to m.
 * Both chains must be of the same type (e.g. MT_DATA).
 * Any m_pkthdr is not updated.
 */
m_cat(m, n)
	register struct mbuf *m, *n;
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_flags & M_EXT ||
		    m->m_data + m->m_len + n->m_len >= &m->m_dat[MLEN]) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

void
m_adj(mp, req_len)
	struct mbuf *mp;
	int req_len;
{
	register int len = req_len;
	register struct mbuf *m;
	register count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		/*
		 * Trim from head.
		 */
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_data += len;
				len = 0;
			}
		}
		m = mp;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= (req_len - len);
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			m = mp;
			if (m->m_flags & M_PKTHDR)
				m->m_pkthdr.len -= len;
			return;
		}
		count -= len;
		if (count < 0)
			count = 0;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		m = mp;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len = count;
		for (; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}

/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to max_protohdr-len extra bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
int MPFail;

struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	int len;
{
	register struct mbuf *m;
	register int count;
	int space;

	/*
	 * If first mbuf has no cluster, and has room for len bytes
	 * without shifting current data, pullup into it,
	 * otherwise allocate a new mbuf to prepend to the chain.
	 */
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data + len < &n->m_dat[MLEN] && n->m_next) {
		if (n->m_len >= len)
			return (n);
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MHLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR) {
			M_COPY_PKTHDR(m, n);
			n->m_flags &= ~M_PKTHDR;
		}
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	MPFail++;
	return (0);
}

/*
 * Partition an mbuf chain in two pieces, returning the tail --
 * all but the first len0 bytes.  In case of failure, it returns NULL and
 * attempts to restore the chain to its original state.
 */
struct mbuf *
m_split(m0, len0, wait)
	register struct mbuf *m0;
	int len0, wait;
{
	register struct mbuf *m, *n;
	unsigned len = len0, remain;

	for (m = m0; m && len > m->m_len; m = m->m_next)
		len -= m->m_len;
	if (m == 0)
		return (0);
	remain = m->m_len - len;
	if (m0->m_flags & M_PKTHDR) {
		MGETHDR(n, wait, m0->m_type);
		if (n == 0)
			return (0);
		n->m_pkthdr.rcvif = m0->m_pkthdr.rcvif;
		n->m_pkthdr.len = m0->m_pkthdr.len - len0;
		m0->m_pkthdr.len = len0;
		if (m->m_flags & M_EXT)
			goto extpacket;
		if (remain > MHLEN) {
			/* m can't be the lead packet */
			MH_ALIGN(n, 0);
			n->m_next = m_split(m, len, wait);
			if (n->m_next == 0) {
				(void) m_free(n);
				return (0);
			} else
				return (n);
		} else
			MH_ALIGN(n, remain);
	} else if (remain == 0) {
		n = m->m_next;
		m->m_next = 0;
		return (n);
	} else {
		MGET(n, wait, m->m_type);
		if (n == 0)
			return (0);
		M_ALIGN(n, remain);
	}
extpacket:
	if (m->m_flags & M_EXT) {
		n->m_flags |= M_EXT;
		n->m_ext = m->m_ext;
		mclrefcnt[mtocl(m->m_ext.ext_buf)]++;
		m->m_ext.ext_size = 0; /* For Accounting XXXXXX danger */
		n->m_data = m->m_data + len;
	} else {
		bcopy(mtod(m, caddr_t) + len, mtod(n, caddr_t), remain);
	}
	n->m_len = remain;
	m->m_len = len;
	n->m_next = m->m_next;
	m->m_next = 0;
	return (n);
}
/*
 * Routine to copy from device local memory into mbufs.
 */
struct mbuf *
m_devget(buf, totlen, off0, ifp, copy)
	char *buf;
	int totlen, off0;
	struct ifnet *ifp;
	void (*copy)();
{
	register struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	register int off = off0, len;
	register char *cp;
	char *epkt;

	cp = buf;
	epkt = cp + totlen;
	if (off) {
		/*
		 * If 'off' is non-zero, packet is trailer-encapsulated,
		 * so we have to skip the type and length fields.
		 */
		cp += off + 2 * sizeof(u_int16_t);
		totlen -= 2 * sizeof(u_int16_t);
	}
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (0);
			}
			m->m_len = MLEN;
		}
		len = min(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = len = min(len, MCLBYTES);
			else {
			  	/* give up when it's out of cluster mbufs */
			        if (top)
				   m_freem(top);
				m_freem(m);
				return (0);
			}
		} else {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		if (copy)
			copy(cp, mtod(m, caddr_t), (unsigned)len);
		else
			bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return (top);
}

/*
 * Cluster freelist allocation check. Mbuf lock/splimp must be held.
 * Ensure hysteresis between hi/lo.
 */
static int
m_howmany()
{
	register int i;

	/* Under minimum */
	if (mbstat.m_clusters < MINCL)
		return (MINCL - mbstat.m_clusters);
	/* Too few (free < 1/2 total) and not over maximum */
	if (mbstat.m_clusters < nmbclusters &&
	    (i = ((mbstat.m_clusters >> 1) - mbstat.m_clfree)) > 0)
		return i;
	return 0;
}

