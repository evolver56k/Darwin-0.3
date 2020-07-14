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
 * Copyright (c) 1997, 1998 Apple Computer, Inc. All Rights Reserved
 * Copyright (c) 1983, 1993
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
 */


#define NEXT 1
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "machine/byte_order.h"
#include "machine/endian.h"

#define	NSIP
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netdb.h>

#define EON
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <sys/protosw.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IFR_NEXT(ifr)	\
    ((struct ifreq *) ((char *) (ifr) + sizeof(*(ifr)) + \
      MAX(0, (int) (ifr)->ifr_addr.sa_len - (int) sizeof((ifr)->ifr_addr))))

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq;
struct	iso_ifreq	iso_ridreq;
struct	iso_aliasreq	iso_addreq;

#define RIDADDR 0
#define ADDR	1
#define MASK_IX	2
#define DSTADDR	3
extern struct sockaddr_in *sintab[];

char	name[30];
int	flags;
int	metric;
int	nsellength = 1;
int	setaddr;
int	setmask;
int     setbroadaddr;
int	autoaddr;
int	autonetmask;
int	setipdst;
int	doalias;
int	clearaddr;
int	newaddr = 1;
int	s;
extern	int errno;

void 	status();
void	setifflags(), setifaddr(), setifdstaddr(), setifnetmask();
void	setifmetric(), setifbroadaddr(), setifipdst();
void	notealias(), setsnpaoffset(), setnsellength(), notrailers();

#define	NEXTARG		0xffffff

struct	cmd {
	char	*c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	void	(*c_func)();
} cmds[] = {
	{ "up",		IFF_UP,		setifflags } ,
	{ "down",	-IFF_UP,	setifflags },
	{ "trailers",	-1,		notrailers },
	{ "-trailers",	1,		notrailers },
	{ "arp",	-IFF_NOARP,	setifflags },
	{ "-arp",	IFF_NOARP,	setifflags },
	{ "debug",	IFF_DEBUG,	setifflags },
	{ "-debug",	-IFF_DEBUG,	setifflags },
	{ "alias",	IFF_UP,		notealias },
	{ "-alias",	-IFF_UP,	notealias },
	{ "delete",	-IFF_UP,	notealias },
#ifdef notdef
#define	EN_SWABIPS	0x1000
	{ "swabips",	EN_SWABIPS,	setifflags },
	{ "-swabips",	-EN_SWABIPS,	setifflags },
#endif
	{ "netmask",	NEXTARG,	setifnetmask },
	{ "metric",	NEXTARG,	setifmetric },
	{ "broadcast",	NEXTARG,	setifbroadaddr },
	{ "ipdst",	NEXTARG,	setifipdst },
	{ "snpaoffset",	NEXTARG,	setsnpaoffset },
	{ "nsellength",	NEXTARG,	setnsellength },
	{ "link0",	IFF_LINK0,	setifflags },
	{ "-link0",	-IFF_LINK0,	setifflags },
	{ "link1",	IFF_LINK1,	setifflags },
	{ "-link1",	-IFF_LINK1,	setifflags },
	{ "link2",	IFF_LINK2,	setifflags },
	{ "-link2",	-IFF_LINK2,	setifflags },
	{ 0,		0,		setifaddr },
	{ 0,		0,		setifdstaddr },
};

/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */
void	in_status(), in_getaddr();
void	xns_status(), xns_getaddr();
void	iso_status(), iso_getaddr();

/* Known address families */
struct afswtch {
	char *af_name;
	short af_af;
	void (*af_status)();
	void (*af_getaddr)();
	int af_difaddr;
	int af_aifaddr;
	caddr_t af_ridreq;
	caddr_t af_addreq;
} afs[] = {
#define C(x) ((caddr_t) &x)
	{ "inet", AF_INET, in_status, in_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ "ns", AF_NS, xns_status, xns_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ "iso", AF_ISO, iso_status, iso_getaddr,
	     SIOCDIFADDR_ISO, SIOCAIFADDR_ISO, C(iso_ridreq), C(iso_addreq) },
	{ 0,	0,	    0,		0 }
};

struct afswtch *afp;	/*the address family being set or asked about*/


void
Perror(cmd, ifr)
	char *cmd;
	struct ifreq *ifr;
{
	extern int errno;

	if (ifr)
		fprintf(stderr, "ifconfig(%s) ", ifr->ifr_name);
	else
		fprintf(stderr, "ifconfig ");
	switch (errno) {

	case ENXIO:
		fprintf(stderr, "%s: no such interface\n", cmd);
		break;

	case EPERM:
		fprintf(stderr, "%s: permission denied\n", cmd);
		break;

	case ETIMEDOUT:
		fprintf(stderr, "%s: timed out\n", cmd);
		break;

	default:
		perror(cmd);
	}
	exit(1);
}
#define MAXADDRS	100
static struct ifreq reqbuf[MAXADDRS];

void
main(argc, argv)
	int argc;
	char *argv[];
{
	int af = AF_INET;
	register struct afswtch *rafp = NULL;
	struct ifreq *ifrp;
	struct ifconf ifc;

	if (argc < 2) {
		fprintf(stderr, "usage: ifconfig interface\n%s%s%s%s%s",
		    "\t[ af [ address [ dest_addr ] ] [ up ] [ down ]",
			    "[ netmask mask ] ]\n",
		    "\t[ metric n ]\n",
		    "\t[ arp | -arp ]\n",
		    "\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ] \n");
		exit(1);
	}
	argc--, argv++;
	strncpy(name, *argv, sizeof(name));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	argc--, argv++;
	if (argc > 0) {
		for (afp = rafp = afs; rafp->af_name; rafp++)
			if (strcmp(rafp->af_name, *argv) == 0) {
				afp = rafp; argc--; argv++;
				break;
			}
		rafp = afp;
		af = ifr.ifr_addr.sa_family = rafp->af_af;
	}
	s = socket(af, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		exit(1);
	}
	/* let "-a" list the configuration of all interfaces */
	if (strcmp(name, "-a") == 0) {
	    ifc.ifc_len = sizeof(reqbuf);
	    ifc.ifc_buf = (caddr_t) reqbuf;

	    if (ioctl(s, SIOCGIFCONF, &ifc) != 0) {
		perror("ioctl(SIOCGIFCONF)");
		exit(1);
	    }
	} else {
	    ifc.ifc_len = sizeof(ifr);
	    ifc.ifc_buf = (caddr_t) &ifr;
	    ifr.ifr_addr.sa_family = af;
	}

	for (ifrp = (struct ifreq *) ifc.ifc_buf;
	     (char *) ifrp < &ifc.ifc_buf[ifc.ifc_len];
	     ifrp = IFR_NEXT(ifrp)) {
	    int save_argc = argc;
	    char **save_argv = argv;
	    unsigned char *p, c;
	    struct ifreq *ifrp2;

	    /* reset state variables */
	    setaddr = autoaddr = autonetmask = setipdst = 0;

	    /* skip duplicate names */
	    for (ifrp2 = (struct ifreq *) ifc.ifc_buf; ifrp2 < ifrp;
		 ifrp2 = IFR_NEXT(ifrp2))
		if (strncmp(ifrp2->ifr_name, ifrp->ifr_name,
			    sizeof(ifrp->ifr_name)) == 0)
		    break;
	    if (ifrp2 < ifrp)
		continue;

	    /*
	     * Adapt to buggy kernel implementation (> 9 of a type)
	     */

	    p = &ifrp->ifr_name[strlen(ifrp->ifr_name)-1];
	    if ((c = *p) > '0'+9)
		sprintf(p, "%d", c-'0');
	    strncpy(name, ifrp->ifr_name, sizeof(name));
	    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
		Perror("ioctl (SIOCGIFFLAGS 1)", &ifr);
		exit(1);
	}
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	flags = ifr.ifr_flags;
	if (ioctl(s, SIOCGIFMETRIC, (caddr_t)&ifr) < 0)
		Perror("ioctl (SIOCGIFMETRIC)", &ifr);
	else
		metric = ifr.ifr_metric;
	if (argc == 0) {
		status();
		/* no need to restore argc */
		continue;
	}
	while (argc > 0) {
		register struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr)
			p++;	/* got src, do dst */
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				if (argv[1] == NULL)
					errx(1, "'%s' requires argument",
					    p->c_name);
				(*p->c_func)(argv[1]);
				argc--, argv++;
			} else
				(*p->c_func)(*argv, p->c_parameter);
		}
		argc--, argv++;
	}
	if (af == AF_ISO) {
		void adjust_nsellength();

		adjust_nsellength();
        }
	if (setipdst && af==AF_NS) {
		struct nsip_req rq;
		int size = sizeof(rq);

		rq.rq_ns = addreq.ifra_addr;
		rq.rq_ip = addreq.ifra_dstaddr;

		if (setsockopt(s, 0, SO_NSIP_ROUTE, &rq, size) < 0)
			Perror("Encapsulation Routing", NULL);
	}

	if (setaddr) {
	    if (autoaddr && af == AF_INET) {
		struct sockaddr_in *autosin;
		unsigned long autosin_s_addr;
		struct	sockaddr_in sin = { AF_INET };

		ifr.ifr_addr = *(struct sockaddr *) &sin;
		if (ioctl(s, SIOCAUTOADDR, (caddr_t)&ifr) < 0) {
		    int	myerrno = errno;
		    /* This should probably be done in the ioctl */
		    if (errno == EINVAL && (flags & IFF_LOOPBACK)
			&& af == AF_INET) {
			/* Set the address to 127.0.0.1 */
			((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr
			    = NXSwapHostLongToBig(INADDR_LOOPBACK);

		    } else {
			Perror("automatic address", &ifr);
			if (myerrno == ENETDOWN ||
			    myerrno == ETIMEDOUT)
			    exit (2);
			exit (1);
		    }
		}
		autosin = (struct sockaddr_in *)&ifr.ifr_addr;
		autosin_s_addr = autosin->sin_addr.s_addr;
		printf("%s: address automatically set to %s\n",
		       name, inet_ntoa(autosin->sin_addr));
		sintab[ADDR]->sin_addr.s_addr = autosin_s_addr;

		/*
		 * apparently, deep in the bowels of the kernel,
		 * the netmask and broadcast address have been reset
		 */
		if (setmask && !autonetmask) {
		    ifr.ifr_addr = *(struct sockaddr *)sintab[MASK_IX];
		    if (ioctl(s, SIOCSIFNETMASK, (caddr_t)&ifr) < 0)
			Perror("ioctl (SIOCSIFNETMASK)", &ifr);
		    printf("%s: netmask reset to %s\n",
			   name, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		    if (!setbroadaddr && (flags & IFF_BROADCAST)) {
			struct	sockaddr_in broadaddr;
			/* Reset the broadcast address based on the new netmask */
			broadaddr.sin_family = AF_INET;
			broadaddr.sin_addr.s_addr = autosin_s_addr | 
			    ( ~(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr));
			ifr.ifr_addr = *(struct sockaddr *)&broadaddr;
			if (ioctl(s, SIOCSIFBRDADDR, (caddr_t)&ifr) < 0)
			    Perror("ioctl (SIOCSIFBRDADDR)", &ifr);
			sintab[DSTADDR]->sin_addr.s_addr = broadaddr.sin_addr.s_addr;
			printf("%s: broadcast address reset to %s\n",
			       name,
			       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		    }
		
		}
	    }
	}

	if (setmask && autonetmask) {
	    /*
	     * If we are broadcasting for the netmask, we do it
	     * after we have set the address.
	     */
#define IN_LOOPBACK_NETMASK	0xff000000UL
	    if (flags & IFF_LOOPBACK) {
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr 
		    = IN_LOOPBACK_NETMASK;
	    }
	    else {
		if (ioctl(s, SIOCAUTONETMASK, (caddr_t)&ifr) < 0) {
		    Perror("ioctl (SIOCAUTONETMASK)", &ifr);
		}
		if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		    Perror("ioctl (SIOCGIFNETMASK)", &ifr);
		}
	    }
	    printf("%s: netmask automatically set to %s\n",
		   name, 
		   inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	    *sintab[MASK_IX] = *((struct sockaddr_in *)&ifr.ifr_addr);
	    if (!setbroadaddr && (flags & IFF_BROADCAST)) { 
		/* recalculate the broadcast address */
		sintab[DSTADDR]->sin_addr.s_addr =sintab[ADDR]->sin_addr.s_addr
			| ~(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
	    }
	}

	if (clearaddr) {
		int ret;
		strncpy(rafp->af_ridreq, name, sizeof ifr.ifr_name);
		if ((ret = ioctl(s, rafp->af_difaddr, rafp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else
				Perror("ioctl (SIOCDIFADDR)", rafp->af_addreq);
		}
	}
	if (newaddr) {
		strncpy(rafp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, rafp->af_aifaddr, rafp->af_addreq) < 0)
			Perror("ioctl (SIOCAIFADDR)", rafp->af_addreq);
	}

	    argc = save_argc;
	    argv = save_argv;
	    /* Go do the next interface in the case of ifconfig -a */
	}
	exit(0);
}

/*ARGSUSED*/
void
setifaddr(addr, param)
	char *addr;
	short param;
{
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr = 1;
	if (strcmp(addr, "-AUTOMATIC-") == 0) {
	    autoaddr = 1;
	} 
	else {
	    if (doalias == 0)
		clearaddr = 1;
	    (*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
	}
}

void
setifnetmask(addr)
	char *addr;
{
	setmask = 1;
	if (strcmp(addr, "-AUTOMATIC-") == 0) {
	    autonetmask = 1;
	} 
	else {
	    (*afp->af_getaddr)(addr, MASK_IX);
	}
}

void
setifbroadaddr(addr)
	char *addr;
{
	(*afp->af_getaddr)(addr, DSTADDR);
	setbroadaddr++;
}


void
setifipdst(addr)
	char *addr;
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
}
#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
void
notealias(addr, param)
	char *addr;
{
	if (setaddr && doalias == 0 && param < 0)
		bcopy((caddr_t)rqtosa(af_addreq),
		      (caddr_t)rqtosa(af_ridreq),
		      rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
	} else
		clearaddr = 0;
}

/*ARGSUSED*/
void
notrailers(vname, value)
	char *vname;
	int value;
{
	printf("Note: trailers are no longer sent, but always received\n");
}

/*ARGSUSED*/
void
setifdstaddr(addr, param)
	char *addr;
	int param;
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifflags(vname, value)
	char *vname;
	short value;
{
 	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
 		Perror("ioctl (SIOCGIFFLAGS 2)", &ifr);
 		exit(1);
 	}
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
 	flags = ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
		Perror(vname, &ifr);
}

void
setifmetric(val)
	char *val;
{
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = atoi(val);
	if (ioctl(s, SIOCSIFMETRIC, (caddr_t)&ifr) < 0)
		Perror("ioctl (set metric)", &ifr);
}

void
setsnpaoffset(val)
	char *val;
{
	iso_addreq.ifra_snpaoffset = atoi(val);
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status()
{
	register struct afswtch *p = afp;
	void printb();

	printf("%s: ", name);
	printb("flags", flags, IFFBITS);
	if (metric)
		printf(" metric %d", metric);
	putchar('\n');
	if ((p = afp) != NULL) {
		(*p->af_status)(1);
	} else for (p = afs; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)(0);
	}
}

void
in_status(force)
	int force;
{
	struct sockaddr_in *sin;
	char *inet_ntoa();
	struct	sockaddr_in	netmask;

	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
		} else
			Perror("ioctl (SIOCGIFADDR)", &ifr);
	}
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	printf("\tinet %s ", inet_ntoa(sin->sin_addr));
	strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			Perror("ioctl (SIOCGIFNETMASK)", &ifr);
		bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
	} else
		netmask.sin_addr =
		    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			else
			    Perror("ioctl (SIOCGIFDSTADDR)", &ifr);
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_dstaddr;
		printf("--> %s ", inet_ntoa(sin->sin_addr));
	}
	printf("netmask 0x%lx ", ntohl(netmask.sin_addr.s_addr));
	if (flags & IFF_BROADCAST) {
		if (ioctl(s, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			else
			    Perror("ioctl (SIOCGIFADDR)", &ifr);
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_addr;
		if (sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
}


void
xns_status(force)
	int force;
{
	struct sockaddr_ns *sns;
	int s;
	s = socket(AF_NS, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		perror("socket");
		exit(1);
	}
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
		} else
			Perror("ioctl (SIOCGIFADDR)", &ifr);
	}
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	sns = (struct sockaddr_ns *)&ifr.ifr_addr;
	printf("\tns %s ", ns_ntoa(sns->sns_addr));
	if (flags & IFF_POINTOPOINT) { /* by W. Nesheim@Cornell */
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    bzero((char *)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			else
			    Perror("ioctl (SIOCGIFDSTADDR)", &ifr);
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sns = (struct sockaddr_ns *)&ifr.ifr_dstaddr;
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}
	putchar('\n');

	close(s);
}

void
iso_status(force)
	int force;
{
	struct sockaddr_iso *siso;
	struct iso_ifreq ifr;
	int s;

	s = socket(AF_ISO, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		Perror("socket", NULL);
		exit(1);
	}
	bzero((caddr_t)&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR_ISO, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			bzero((char *)&ifr.ifr_Addr, sizeof(ifr.ifr_Addr));
		} else {
			Perror("ioctl (SIOCGIFADDR_ISO)", &ifr);
			exit(1);
		}
	}
	strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	siso = &ifr.ifr_Addr;
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK_ISO, (caddr_t)&ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			Perror("ioctl (SIOCGIFNETMASK_ISO)", &ifr);
	} else {
		printf(" netmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR_ISO, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    bzero((char *)&ifr.ifr_Addr, sizeof(ifr.ifr_Addr));
			else
			    Perror("ioctl (SIOCGIFDSTADDR_ISO)", &ifr);
		}
		strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		siso = &ifr.ifr_Addr;
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	putchar('\n');
	close(s);
}

struct	in_addr inet_makeaddr();

#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
SIN(ridreq.ifr_addr), SIN(addreq.ifra_addr),
SIN(addreq.ifra_mask), SIN(addreq.ifra_broadaddr)};

void
in_getaddr(s, which)
	char *s;
{
	register struct sockaddr_in *sin = sintab[which];
	struct hostent *hp;
	struct netent *np;

	sin->sin_len = sizeof(*sin);
	if (which != MASK_IX)
		sin->sin_family = AF_INET;

	if (inet_aton(s, &sin->sin_addr))
		return;
	if (hp = gethostbyname(s))
		bcopy(hp->h_addr, (char *)&sin->sin_addr,
		      hp->h_length);
	else if (np = getnetbyname(s))
		sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
	else
		errx(1, "%s: bad value", s);
	
}

/*
 * Print a value a la the %b format of the kernel's printf
 */
void
printb(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

#define SNS(x) ((struct sockaddr_ns *) &(x))
struct sockaddr_ns *snstab[] = {
SNS(ridreq.ifr_addr), SNS(addreq.ifra_addr),
SNS(addreq.ifra_mask), SNS(addreq.ifra_broadaddr)};

void
xns_getaddr(addr, which)
char *addr;
{
	struct sockaddr_ns *sns = snstab[which];
	struct ns_addr ns_addr();

	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK_IX)
		printf("Attempt to set XNS netmask will be ineffectual\n");
}

#define SISO(x) ((struct sockaddr_iso *) &(x))
struct sockaddr_iso *sisotab[] = {
SISO(iso_ridreq.ifr_Addr), SISO(iso_addreq.ifra_addr),
SISO(iso_addreq.ifra_mask), SISO(iso_addreq.ifra_dstaddr)};

void
iso_getaddr(addr, which)
char *addr;
{
	register struct sockaddr_iso *siso = sisotab[which];
	struct iso_addr *iso_addr();
	siso->siso_addr = *iso_addr(addr);

	if (which == MASK_IX) {
		siso->siso_len = TSEL(siso) - (caddr_t)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

void
setnsellength(val)
	char *val;
{
	nsellength = atoi(val);
	if (nsellength < 0)
		errx(1, "Negative NSEL length is absurd");
	if (afp == 0 || afp->af_af != AF_ISO)
		errx(1, "Setting NSEL length valid only for iso");
}

void
fixnsel(s)
register struct sockaddr_iso *s;
{
	if (s->siso_family == 0)
		return;
	s->siso_tlen = nsellength;
}

void
adjust_nsellength()
{
	fixnsel(sisotab[RIDADDR]);
	fixnsel(sisotab[ADDR]);
	fixnsel(sisotab[DSTADDR]);
}
