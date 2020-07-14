/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * ++Copyright++ 1985, 1989
 * -
 * Copyright (c) 1985, 1989
 *    The Regents of the University of California.  All rights reserved.
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
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
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
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#ifndef lint
static char sccsid[] = "@(#)skip.c	5.12 (Berkeley) 3/21/91";
static char rcsid[] = "$Id: skip.c,v 1.1.1.1 1999/05/02 03:57:56 wsanchez Exp $";
#endif /* not lint */

/*
 *******************************************************************************
 *
 *  skip.c --
 *
 *	Routines to skip over portions of a query buffer.
 *
 *	Note: this file has been submitted for inclusion in
 *	BIND resolver library. When this has been done, this file
 *	is no longer necessary (assuming there haven't been any
 *	changes).
 *
 *	Adapted from 4.3BSD BIND res_debug.c
 *
 *******************************************************************************
 */

#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <stdio.h>
#include "../named.tproj/portability.h"

char *res_skip_rr();


/*
 *******************************************************************************
 *
 *  res_skip --
 *
 * 	Skip the contents of a query.
 *
 * 	Interpretation of numFieldsToSkip argument:
 *            res_skip returns pointer to:
 *    	1 ->  start of question records.
 *    	2 ->  start of authoritative answer records.
 *    	3 ->  start of additional records.
 *    	4 ->  first byte after end of additional records.
 *
 *   Results:
 *	(address)	- success operation.
 *  	NULL 		- a resource record had an incorrect format.
 *
 *******************************************************************************
 */

char *
res_skip(msg, numFieldsToSkip, eom)
	char *msg;
	int numFieldsToSkip;
	char *eom;
{
	register char *cp;
	register HEADER *hp;
	register int tmp;
	register int n;

	/*
	 * Skip the header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + HFIXEDSZ;

	/*
	 * skip question records.
	 */
	if (n = ntohs(hp->qdcount) ) {
		while (--n >= 0 && cp < eom) {
			tmp = dn_skipname((u_char *)cp, (u_char *)eom);
			if (tmp == -1) return(NULL);
			cp += tmp;
			cp += INT16SZ;	/* type 	*/
			cp += INT16SZ;	/* class 	*/
		}
	}
	if (--numFieldsToSkip <= 0) return(cp);

	/*
	 * skip authoritative answer records
	 */
	if (n = ntohs(hp->ancount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}
	if (--numFieldsToSkip == 0) return(cp);

	/*
	 * skip name server records
	 */
	if (n = ntohs(hp->nscount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}
	if (--numFieldsToSkip == 0) return(cp);

	/*
	 * skip additional records
	 */
	if (n = ntohs(hp->arcount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}

	return(cp);
}


/*
 *******************************************************************************
 *
 *  res_skip_rr --
 *
 * 	Skip over resource record fields.
 *
 *   Results:
 *	(address)	- success operation.
 *  	NULL 		- a resource record had an incorrect format.
 *******************************************************************************
 */

char *
res_skip_rr(cp, eom)
	char *cp;
	char *eom;
{
	int tmp;
	int dlen;

	if ((tmp = dn_skipname((u_char *)cp, (u_char *)eom)) == -1)
		return (NULL);			/* compression error */
	cp += tmp;
	if ((cp + RRFIXEDSZ) > eom)
		return (NULL);
	cp += INT16SZ;	/* 	type 	*/
	cp += INT16SZ;	/* 	class 	*/
	cp += INT32SZ;	/* 	ttl 	*/
	dlen = _getshort((u_char*)cp);
	cp += INT16SZ;	/* 	dlen 	*/
	cp += dlen;
	if (cp > eom)
		return (NULL);
	return (cp);
}
