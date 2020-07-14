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
 *	Copyright (c) 1996 Apple Computer, Inc.
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *	Created April 8, 1996 by Tuyen Nguyen
 *	Modified, May, 1996, to move routing/tunneling into kernel.
 *		aurpd now functions as user process to configure/update
 *		kernel process.
 *	Modified, March 17, 1997 by Tuyen Nguyen for Rhapsody.
 */
#ifndef lint
static char sccsid[] = "@(#)aurpd.c; Copyright 1996, Apple Computer, Inc.";
#endif  /* lint */

/*
 * Exit codes:
 *	0 - AOK
 *	-1 - Not Super-User
 *	-3 - Bad config file
 *	-4 - Fork failed
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <h/sysglue.h>
#include <at/appletalk.h>
#include <at/aurp.h>
#ifdef _AIX
#include <locale.h>
#include <atsbin_msg.h>
#endif

/* */
#define AURP_CFGFILENAME  "/etc/aurp_tunnel.cfg"

/*#define MSGSTR(num,str)   catgets(catd, MS_SBIN, num,str)*/
#define MSGSTR(num,str)		str

int  AURPgetcfg();
void AURPdown();
#ifdef _AIX
extern char *sys_errlist [];
nl_catd catd;
#endif
char *usage = "Usage: %s [-dqv] [-l log] [config_file]\n";
char *progname;
#ifdef DEBUG
char *opt_string = "Ddl:qv";
#else
char *opt_string = "dl:qv";
#endif
FILE *logfp;
char *logfn;
short udp_port;
unsigned char opt_error = 1;	/* Print helpful message on error */
unsigned char dst_addr_cnt;
unsigned char net_export;
unsigned char net_hide;
unsigned char net_access_cnt;
unsigned char debug;
unsigned short net_access[AURP_MAXNETACCESS];
char dst_active[256];
long dst_addr[256];
long src_addr;

main(argc, argv)
	int	argc;
	char *argv[];
{	int rc;
	char *filename = AURP_CFGFILENAME;
	register int errflag, c;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;
	extern void AURP();
#ifdef _AIX
	(void) setlocale(LC_ALL, "");
	catd = catopen(MF_ATSBIN, NL_CAT_LOCALE);
#endif

	/*
	 * find out what this program is called and other miscellaneous stuff
	 */
	progname = strrchr(argv[0], '/') ? strrchr(argv[0], '/') + 1 : argv[0];

	errflag = 0;
	while ((c = getopt(argc, argv, opt_string)) != EOF)
		switch(c)
		{
#ifdef DEBUG
			case 'D':
				debug++;
				break;
#endif
			case 'd':
				AURPdown();
#ifdef _AIX
				catclose(catd);
#endif
				exit(0);
			case 'l':
				logfn = optarg;
				break;
			case 'q':
				opt_error = 0;
				break;
			case '?':
			default:
				errflag++;
				break;
		}

	if (errflag)
	{	fprintf(stderr, MSGSTR(M_USAGE, usage), progname);
		return -1;
	}

	/*
	 * Make sure that program is invoked by a super-user, before we
	 *  do anything substantive.
	 */
	if (getuid()) {
		fprintf(stderr, MSGSTR(M_MUST_BE_SU,
			"%s: Permission denied; must be super-user.\n"), progname);
#ifdef _AIX
		catclose(catd);
#endif
		exit(-1);
	}

	if (optind < argc)
		filename = argv[optind];

	if (AURPgetcfg(filename)) {
#ifdef _AIX
		catclose(catd);
#endif
		exit(-3);
	}

	/* run the aurp daemon in the background, disassociated from tty */
#ifdef DEBUG
	/* if we're not debugging and the user wants to */
	if (!debug)
	{
#endif
	if ((rc = fork()) == -1) {
		perror("fork");
#ifdef _AIX
		catclose(catd);
#endif
		exit(-4);
	}
	if (rc) {		/* In parent */
#ifdef _AIX
		catclose(catd);
#endif
		exit(0);
	}
#ifdef DEBUG
	}
#endif

	setpgid(0, 0);
	AURP();
#ifdef _AIX
	catclose(catd);
#endif
	exit(0);
}

void AURP()
{
	int at_descr, ic_cmd, ic_len;
	ioccmd_t ioc;

	/* create the AURP Appletalk socket */
	if ((at_descr = at_open_dev(ATPROTO_AURP)) == -1) {
		perror("at_open_dev");
		return;
	}

	/* configure local/remote addresses */
	if (dst_addr_cnt) {
		ic_cmd = AUC_NETLIST;
		ic_len = (dst_addr_cnt+1) * sizeof(unsigned long);
		if (at_send_to_dev(at_descr, ic_cmd, dst_addr, &ic_len) == -1) {
			perror("NETLIST");
			close(at_descr);
			return;
		}
	}
	/* configure network hiding or exporting */
	if (net_access_cnt) {
		ic_cmd = net_export ? AUC_EXPNET : AUC_HIDENET;
		ic_len = net_access_cnt * sizeof(unsigned short);
		if (at_send_to_dev(at_descr, ic_cmd, net_access, &ic_len) == -1) {
			perror("HIDE/EXPORT");
			close(at_descr);
			return;
		}
	}

	if (udp_port != AURP_SOCKNUM)
	{	ic_cmd = AUC_UDPPORT;
		ic_len = sizeof(udp_port);
		if (at_send_to_dev(at_descr, ic_cmd, &udp_port, &ic_len) == -1) {
			perror("UDPPORT");
			close(at_descr);
			return;
		}
	}

	/* configure the tunnel - this starts the ball rolling */
	ic_cmd = AUC_CFGTNL;
	ic_len = sizeof(dst_addr_cnt);
	if (at_send_to_dev(at_descr, ic_cmd, &dst_addr_cnt, &ic_len) == -1) {
		perror("AURP CONFIG");
		close(at_descr);
		return;
	}

	logfp = stdout;
	if (logfn) {
		/* log data to the specified file */
		if ((logfp = fopen(logfn, "w")) == 0) {
			perror(logfn);
			close(at_descr);
			return;
		}
	}

	/* wait for shutdown */
	errno = aurpd(at_descr);

	close(at_descr);
}

int AURPgetcfg(filename)
	char *filename;
{
	FILE *fp;
	int line, len;
	char cfgbuf[128];

	/* open the cfg file */
	errno = 0;
	if ((fp = fopen(filename, "r")) == 0) {
		perror(filename);
		return -1;
	}

	/* get the cfg info */
	udp_port = -1;
	dst_addr[0] = -1;
	for (line=1; !feof(fp); line++) {
		register char *p, *q;

		/* fgets() NUL-terminates our string, but leaves the NL */
		if (fgets(cfgbuf, sizeof(cfgbuf), fp) == 0)
		{	if (errno)
				perror(filename);
			break;
		}
		if (cfgbuf[0] == '#')
			continue;
		if ((p = strtok(cfgbuf, "\t \n")) == NULL)	/* Keyword */
			continue;
		q = strtok(NULL, "\t \n");	/* value */

		if (strpfx(p, "port") == 0)
		{	if (udp_port == -1)
				udp_port = (short)atoi(q);
		} else if (strpfx(p, "local") == 0)
		{	if (dst_addr[0] == -1)
				dst_addr[0] = inet_addr(q);
		} else if (strpfx(p, "remote") == 0)
		{	dst_addr[++dst_addr_cnt] = inet_addr(q);
		}
		else if (strpfx(p, "hide") == 0)
		{	if (net_export) {
				fprintf(stderr,
					MSGSTR(M_HIDE, "%s: Hiding/Export conflict\n"),
					progname);
				return -1;
			}
			net_hide = 1;
			net_access[net_access_cnt++] =
				(unsigned short)atoi(q);
		} else if (strpfx(p, "export") == 0)
		{	if (net_hide) {
				fprintf(stderr,
					MSGSTR(M_EXP, "%s: Export/Hiding conflict\n"),
					progname);
				return -1;
			}
			net_export = 1;
			net_access[net_access_cnt++] =
				(unsigned short)atoi(q);
		} else
		{	if (opt_error)
				fprintf(stderr,
					MSGSTR(M_KEYWORD,
					       "%s: unknown keyword (`%s\'), line %d\n"),
					filename, p, line);
			continue;
		}
	}

	if (dst_addr_cnt == 0) {
		if (opt_error)
			fprintf(stderr,
				MSGSTR(M_REMOTE,
				       "%s: Invalid format (%s): no `remote\' keyword\n"),
				progname, filename);
		return -1;
	}

	if (src_addr == -1) {
		fprintf(stderr,
			MSGSTR(M_LOCAL,
			       "%s: Invalid format (%s): no `local\' keyword\n"),
			progname, filename);
		return(-1);
	}

	if (udp_port == -1)
		udp_port = AURP_SOCKNUM;

	return 0;
}

void AURPdown()
{
	int at_descr, ic_cmd;
	ioccmd_t ioc;

	if ((at_descr = at_open_dev(ATPROTO_AURP)) == -1)
		perror("at_open_dev");
	else {
		ic_cmd = AUC_SHTDOWN;
		if (at_send_to_dev(at_descr, ic_cmd, 0, 0) == -1)
			perror("SHUTDOWN");
		close(at_descr);
	}
}

/* see if pat is a prefix of str */
strpfx(pat, str)
register char *pat, *str;
{
        if (pat == NULL || str == NULL)
                return(0);
        if (*pat == '\0' || *str == '\0')
                return(0);
        while (*pat && *str && *pat == *str)
        {       pat++;
                str++;
        }
        /* If *pat is nul, pat is a prefix; otherwise, not */
        return(*pat);
}
