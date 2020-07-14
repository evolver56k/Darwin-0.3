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
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_init.c	2.1  2.1 3/18/91";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "popper.h"

extern int      errno;
#ifdef NeXT
extern int	optind;
#endif /* NeXT */

/* 
 *  init:   Start a Post Office Protocol session
 */

pop_init(p,argcount,argmessage)
POP     *       p;
int             argcount;
char    **      argmessage;
{

    struct sockaddr_in      cs;                 /*  Communication parameters */
    struct hostent      *   ch;                 /*  Client host information */
    int                     errflag = 0;
    int                     c;
    int                     len;
    extern char         *   optarg;
    int                     options = 0;
    int                     sp = 0;             /*  Socket pointer */
    char                *   trace_file_name;
#ifdef NeXT
    const char		*  restricted_hosts[64];
    const char		** rh = restricted_hosts;
#endif /* NeXT */

    /*  Initialize the POP parameter block */
    bzero ((char *)p,(int)sizeof(POP));

    /*  Save my name in a global variable */
    p->myname = argmessage[0];

    /*  Get the name of our host */
    (void)gethostname(p->myhost,MAXHOSTNAMELEN);

    /*  Open the log file */
#ifdef SYSLOG42
    (void)openlog(p->myname,0);
#else
    (void)openlog(p->myname,POP_LOGOPTS,POP_FACILITY);
#endif

    /*  Process command line arguments */
#ifdef NeXT
    while ((c = getopt(argcount,argmessage,"dh:t:")) != EOF)
#else
    while ((c = getopt(argcount,argmessage,"dt:")) != EOF)
#endif /* !NeXT */
        switch (c) {

            /*  Debugging requested */
            case 'd':
                p->debug++;
                options |= SO_DEBUG;
                break;

#ifdef NeXT
	    /* Restricted hosts specified */
	    case 'h':
		*rh++ = optarg;
		break;
#endif NeXT

            /*  Debugging trace file specified */
            case 't':
                p->debug++;
                if ((p->trace = fopen(optarg,"a+")) == NULL) {
                    pop_log(p,POP_PRIORITY,
                        "Unable to open trace file \"%s\", err = %d",
                            optarg,errno);
                    exit(-1);
                }
                trace_file_name = optarg;
                break;

            /*  Unknown option received */
            default:
                errflag++;
        }

    /*  Exit if bad options specified */
    if (errflag) {
        (void)fprintf(stderr,"Usage: %s [-d]\n",argmessage[0]);
        exit(-1);
    }

#ifdef NeXT
    /* Remember restricted users list (if any) */
    p->restricted_users = argmessage[optind] ? &argmessage[optind] : NULL;
#endif /* NeXT */

    /*  Get the address and socket of the client to whom I am speaking */
    len = sizeof(cs);
    if (getpeername(sp,(struct sockaddr *)&cs,&len) < 0){
#ifdef NeXT
	/* Probably using stdio directly -- treat as if a connection
	 * from localhost.
	 */
	bzero(&cs, sizeof(cs));
	cs.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#else
        pop_log(p,POP_PRIORITY,
            "Unable to obtain socket and address of client, err = %d",errno);
        exit(-1);
#endif /* !NeXT */
    }

    /*  Save the dotted decimal form of the client's IP address 
        in the POP parameter block */
    p->ipaddr = inet_ntoa(cs.sin_addr);

    /*  Save the client's port */
    p->ipport = ntohs(cs.sin_port);

    /*  Get the canonical name of the host to whom I am speaking */
    ch = gethostbyaddr((char *) &cs.sin_addr, sizeof(cs.sin_addr), AF_INET);
    if (ch == NULL){
        pop_log(p,POP_PRIORITY,
            "Unable to get canonical name of client, err = %d",errno);
        p->client = p->ipaddr;
    }
    /*  Save the cannonical name of the client host in 
        the POP parameter block */
    else {

#ifndef BIND43
        p->client = ch->h_name;
#else
#       include <arpa/nameser.h>
#       include <resolv.h>

        /*  Distrust distant nameservers */
#ifndef NeXT
        extern struct state     _res;
#endif /* NeXT */
        struct hostent      *   ch_again;
        char            *   *   addrp;

        /*  We already have a fully-qualified name */
        _res.options &= ~RES_DEFNAMES;

        /*  See if the name obtained for the client's IP 
            address returns an address */
        if ((ch_again = gethostbyname(ch->h_name)) == NULL) {
            pop_log(p,POP_PRIORITY,
                "Client at \"%s\" resolves to an unknown host name \"%s\"",
                    p->ipaddr,ch->h_name);
            p->client = p->ipaddr;
        }
        else {
            /*  Save the host name (the previous value was 
                destroyed by gethostbyname) */
            p->client = ch_again->h_name;

            /*  Look for the client's IP address in the list returned 
                for its name */
            for (addrp=ch_again->h_addr_list; *addrp; ++addrp)
                if (bcmp(*addrp,&(cs.sin_addr),sizeof(cs.sin_addr)) == 0) break;

            if (!*addrp) {
                pop_log (p,POP_PRIORITY,
                    "Client address \"%s\" not listed for its host name \"%s\"",
                        p->ipaddr,ch->h_name);
                p->client = p->ipaddr;
            }
        }
#endif BIND43
    }

    /*  Create input file stream for TCP/IP communication */
    if ((p->input = fdopen(sp,"r")) == NULL){
        pop_log(p,POP_PRIORITY,
            "Unable to open communication stream for input, err = %d",errno);
        exit (-1);
    }

    /*  Create output file stream for TCP/IP communication */
    if ((p->output = fdopen(sp,"w")) == NULL){
        pop_log(p,POP_PRIORITY,
            "Unable to open communication stream for output, err = %d",errno);
        exit (-1);
    }

    pop_log(p,POP_PRIORITY,
        "(v%s) Servicing request from \"%s\" at %s\n",
            VERSION,p->client,p->ipaddr);

#ifdef DEBUG
    if (p->trace)
        pop_log(p,POP_PRIORITY,
            "Tracing session and debugging information in file \"%s\"",
                trace_file_name);
    else if (p->debug)
        pop_log(p,POP_PRIORITY,"Debugging turned on");
#endif DEBUG

#ifdef NeXT
    /* Are we allowed to speak to this guy? */
    if (rh > restricted_hosts) {
	while (rh-- > restricted_hosts) {
	    if (strcasecmp(*rh, p->client) == 0)
		break;
	}
	if (rh < restricted_hosts) {
	    pop_msg(p, POP_FAILURE,
		    "Access denied for host \"%s\"", p->client);
	    exit(-1);
	}
    }
#endif /* NeXT */

    return(POP_SUCCESS);
}
