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
 * Copyright (c) 1994 Christos Zoulas
 * All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: sprayd.c,v 1.1.1.1 1999/05/02 03:58:21 wsanchez Exp $
 */

#ifndef lint
static char rcsid[] = "$Id: sprayd.c,v 1.1.1.1 1999/05/02 03:58:21 wsanchez Exp $";
#endif /* not lint */

#include <stdio.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <syslog.h>
#include <rpcsvc/spray.h>

static void spray_service __P((struct svc_req *, SVCXPRT *));

static int from_inetd = 1;

#define TIMEOUT 120

static void
cleanup()
{
	(void) pmap_unset(SPRAYPROG, SPRAYVERS);
	exit(0);
}


int
main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	int sock = 0;
	int proto = 0;
	struct sockaddr_in from;
	int fromlen;
	
	/*
	 * See if inetd started us
	 */
	if (getsockname(0, (struct sockaddr *)&from, &fromlen) < 0) {
		from_inetd = 0;
		sock = RPC_ANYSOCK;
		proto = IPPROTO_UDP;
	}
	
	if (!from_inetd) {
		daemon(0, 0);

		(void) pmap_unset(SPRAYPROG, SPRAYVERS);

		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
	}

	openlog("rpc.sprayd", LOG_CONS|LOG_PID, LOG_DAEMON);
	
	transp = svcudp_create(sock);
	if (transp == NULL) {
		syslog(LOG_ERR, "cannot create udp service.");
		return 1;
	}
	if (!svc_register(transp, SPRAYPROG, SPRAYVERS, spray_service, proto)) {
		syslog(LOG_ERR,
		    "unable to register (SPRAYPROG, SPRAYVERS, %s).",
		    proto ? "udp" : "(inetd)");
		return 1;
	}

	alarm(TIMEOUT);
	svc_run();
	syslog(LOG_ERR, "svc_run returned");
	return 1;
}


static void
spray_service(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	static spraycumul scum;
	static struct timeval clear;

	switch (rqstp->rq_proc) {
	case SPRAYPROC_CLEAR:
		scum.counter = 0;
		(void) gettimeofday(&clear, 0);
		/*FALLTHROUGH*/

	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, (char *)NULL)) {
			svcerr_systemerr(transp);
			syslog(LOG_ERR, "bad svc_sendreply");
		}
		return;

	case SPRAYPROC_SPRAY:
		scum.counter++;
		return;

	case SPRAYPROC_GET:
		(void) gettimeofday((struct timeval *)&scum.clock, 0);
		if (scum.clock.usec < clear.tv_usec) {
			scum.clock.sec--;
			scum.clock.usec += 1000000;
		}
		scum.clock.sec -= clear.tv_sec;
		scum.clock.usec -= clear.tv_usec;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_spraycumul, (caddr_t)&scum)) {
		svcerr_systemerr(transp);
		syslog(LOG_ERR, "bad svc_sendreply");
	}
}
