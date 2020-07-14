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
#ifndef lint
static char sccsid[] =	"@(#)tcp.c	1.2 91/11/20 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * make tcp calls
	 */
#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>

extern int debug;
/*
 *  routine taken from new_calltcp.c;
 *  no caching is done!
 *  continueously calling if timeout;
 *  in case of error, print put error msg; this msg usually is to be
 *  thrown away
 */
int
call_tcp(host, prognum, versnum, procnum, inproc, in, outproc, out, tot )
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int tot;
{
	struct sockaddr_in server_addr;
	struct in_addr *get_addr_cache();
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval  tottimeout;
	register CLIENT *client;
	int socket = RPC_ANYSOCK;

	if ((hp = gethostbyname(host)) == NULL) {
		if (debug)
			printf( "RPC_UNKNOWNHOST\n");
		return ((int) RPC_UNKNOWNHOST);
	}
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;

	tottimeout.tv_usec = 0;
	tottimeout.tv_sec = tot;
	if ((client = clnttcp_create(&server_addr, prognum, versnum, &socket,
		0, 0)) == NULL) {
		clnt_pcreateerror("clnttcp_create");   /* RPC_PMAPFAILURE or RPC_SYSTEMERROR */
		return ((int) rpc_createerr.cf_stat);  /* return (svr_not_avail); */
	}
again:
	clnt_stat = clnt_call(client, procnum, inproc, in, outproc, out,
			tottimeout);
	if (clnt_stat != RPC_SUCCESS)  {
		if (clnt_stat == RPC_TIMEDOUT) {
			if (tot != 0) {
				if (debug)
					printf("call_tcp timeout, retry\n");
				goto again;
			}
			/* if tot == 0, no reply is expected */
		}
		else {
			if (debug) {
				clnt_perrno(clnt_stat);
				fprintf(stderr, "\n");
			}
		}
	}
	/* should do cacheing, rather than always destroy */
	(void) close(socket);
	clnt_destroy(client);
	return (int) clnt_stat;
}
