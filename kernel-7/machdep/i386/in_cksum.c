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
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)in_cksum.c	7.1 (Berkeley) 3/29/88
 */

/* HISTORY
 * 26-May-94 Curtis Galloway at NeXT
 *	Grabbed the m68k code and added a Pentium-optimized checksummer.
 *	(See RFC 1107 for the checksum algorithm.)
 *
 * 06-Dec-92 Mac Gillon (mgillon) at NeXT
 *	Lifted the '020 code from 4.4. The compiler generates crummy
 *      code for the portable version. 
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif


#if	DEBUG_OC

static inline int
oc_cksum(
    unsigned char *buf,
    int len,
    unsigned long oldsum
)
{
    unsigned short *p = (unsigned short *)buf;
    
    for (; len>1; len -=2)
	oldsum += *p++;
#if __BIG_ENDIAN__
    if (len > 0)
	oldsum += (*(unsigned char *)p) << 8;
#else
    if (len > 0)
	oldsum += *(unsigned char *)p;
#endif
    oldsum = (oldsum >> 16) + (oldsum & 0xffff);
    oldsum = (oldsum >> 16) + (oldsum & 0xffff);
    return oldsum;	
}

#else	DEBUG_OC

static inline int
oc_cksum(
    unsigned char *buf,
    int len,
    unsigned long oldsum
)
{
    unsigned short sum;
    	
    asm("
	testb	$1, %%ecx
	jne	5f			// one or three extra bytes
	testb	$2, %%ecx
	jne	7f			// two extra bytes
0:
        adc     $0,%%eax		//add carry if extra bytes
        shr     $3,%%ecx		//we'll do two dwords per loop
        jnc     1f			//is there an odd dword in buffer?
        add     (%%esi),%%eax		//checksum the odd dword
        adc     $0,%%eax		//add carry if needed
        add     $4,%%esi		//point to the next dword
1:
	test	%%ecx, %%ecx
	jz	4f
        mov     (%%esi),%%edx		//preload the first dword
        mov     4(%%esi),%%ebx		//preload the second dword
        dec     %%ecx			//we'll do 1 checksum outside the loop
        jz      3f			//only 1 checksum to do
        add     $8, %%esi		//point to the next dword

2:
        add     %%edx,%%eax		//cycle 1 U-pipe
        mov     (%%esi),%%edx		//cycle 1 V-pipe
        adc     %%ebx,%%eax		//cycle 2 U-pipe
        mov     4(%%esi),%%ebx		//cycle 2 V-pipe
        adc     $0,%%eax		//cycle 3 U-pipe
        add     $8,%%esi		//cycle 3 V-pipe
        dec     %%ecx			//cycle 4 U-pipe
        jnz     2b			//cycle 4 V-pipe

3:
        add     %%edx,%%eax		//checksum the last two dwords
        adc     %%ebx,%%eax
        adc     $0,%%eax
4:
        mov     %%eax,%%edx		//compress the 32-bit checksum
        shr     $16,%%edx		//into a 16-bit checksum
        add     %%dx,%%ax
        adc     $0,%%eax
	jmp	8f
	
5:
	testb	$2, %%ecx
	je	6f			// do one byte
	
	movzwl	-3(%%esi,%%ecx),%%ebx
	add	%%ebx,%%eax		// pick up two bytes and fall through

6:
	movzbl	-1(%%esi,%%ecx),%%ebx
	adc	%%ebx,%%eax
	jmp	0b
	
7:
	movzwl	-2(%%esi,%%ecx),%%ebx
	add	%%ebx, %%eax
	jmp	0b

8:	
    " : "=a" (sum) : "c" (len), SIREG (buf), "a" (oldsum) :
    "eax", "ebx", "ecx", "edx", "esi");

    return sum;
}

#endif	DEBUG_OC



/*
 * Checksum routine for the Internet Protocol family.
 *
 * This isn't as bad as it looks.  For ip headers the "while" isn't
 * executed and we just drop through to the return statement at the
 * end.  For the usual tcp or udp packet (a single header mbuf
 * chained onto a cluster of data, we make exactly one trip through
 * the while (for the header mbuf) and never do the hairy code
 * inside the "if".  If fact, if m_copydata & sb_compact are doing
 * their job, we should never do the hairy code inside the "if".
 */
in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register int sum = 0;
	register int i;

	while (len > m->m_len) {
		sum = oc_cksum(mtod(m, u_char *), i = m->m_len, sum);
		m = m->m_next;
		len -= i;
		if (i & 1) {
			/*
			 * ouch - we ended on an odd byte with more
			 * to do.  This xfer is obviously not interested
			 * in performance so finish things slowly.
			 */
			register u_char *cp;

			while (len > m->m_len) {
				cp = mtod(m, u_char *);
				if (i & 1) {
					i = m->m_len - 1;
					--len;
#if __BIG_ENDIAN__
					sum += *cp++;
#else
					sum += (*cp++) << 8;
#endif
				} else
					i = m->m_len;

				sum = oc_cksum(cp, i, sum);
				m = m->m_next;
				len -= i;
			}
			if (i & 1) {
				cp =  mtod(m, u_char *);
#if __BIG_ENDIAN__
				sum += *cp++;
#else
				sum += (*cp++) << 8;
#endif
				return (0xffff & ~oc_cksum(cp, len - 1, sum));
			}
		}
	}
	return (0xffff & ~oc_cksum(mtod(m, u_char *), len, sum));
}


/*
 * Checksum routine for Internet Protocol family headers (Portable Version).
 *
 * This routine is very heavily used in the network
 * code and should be modified for each CPU to be as fast as possible.
 */

#ifdef notdef

#define ADDCARRY(x)  (x > 65535 ? x -= 65535 : x)
#define REDUCE {l_util.l = sum; sum = l_util.s[0] + l_util.s[1]; ADDCARRY(sum);}

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_short *w;
	register int sum = 0;
	register int mlen = 0;
	int byte_swapped = 0;

	union {
		char	c[2];
		u_short	s;
	} s_util;
	union {
		u_short s[2];
		long	l;
	} l_util;

	for (;m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		w = mtod(m, u_short *);
		if (mlen == -1) {
			/*
			 * The first byte of this mbuf is the continuation
			 * of a word spanning between this mbuf and the
			 * last mbuf.
			 *
			 * s_util.c[0] is already saved when scanning previous 
			 * mbuf.
			 */
			s_util.c[1] = *(char *)w;
			sum += s_util.s;
			w = (u_short *)((char *)w + 1);
			mlen = m->m_len - 1;
			len--;
		} else
			mlen = m->m_len;
		if (len < mlen)
			mlen = len;
		len -= mlen;
		/*
		 * Force to even boundary.
		 */
		if ((1 & (int) w) && (mlen > 0)) {
			REDUCE;
			sum <<= 8;
			s_util.c[0] = *(u_char *)w;
			w = (u_short *)((char *)w + 1);
			mlen--;
			byte_swapped = 1;
		}
		/*
		 * Unroll the loop to make overhead from
		 * branches &c small.
		 */
		while ((mlen -= 32) >= 0) {
			sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
			sum += w[4]; sum += w[5]; sum += w[6]; sum += w[7];
			sum += w[8]; sum += w[9]; sum += w[10]; sum += w[11];
			sum += w[12]; sum += w[13]; sum += w[14]; sum += w[15];
			w += 16;
		}
		mlen += 32;
		while ((mlen -= 8) >= 0) {
			sum += w[0]; sum += w[1]; sum += w[2]; sum += w[3];
			w += 4;
		}
		mlen += 8;
		if (mlen == 0 && byte_swapped == 0)
			continue;
		REDUCE;
		while ((mlen -= 2) >= 0) {
			sum += *w++;
		}
		if (byte_swapped) {
			REDUCE;
			sum <<= 8;
			byte_swapped = 0;
			if (mlen == -1) {
				s_util.c[1] = *(char *)w;
				sum += s_util.s;
				mlen = 0;
			} else
				mlen = -1;
		} else if (mlen == -1)
			s_util.c[0] = *(char *)w;
	}
	if (len)
		printf("cksum: out of data\n");
	if (mlen == -1) {
		/* The last mbuf has odd # of bytes. Follow the
		   standard (the odd byte may be shifted left by 8 bits
		   or not as determined by endian-ness of the machine) */
		s_util.c[1] = 0;
		sum += s_util.s;
	}
	REDUCE;
	return (~sum & 0xffff);
}
#endif
