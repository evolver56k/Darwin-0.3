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
/* @(#)rstat.c	2.3 88/11/30 4.0 RPCSRC */
/*
 *  Simple program that prints the status of a remote host, in a format
 *  similar to that used by the 'w' command, using the rstat.x service.
 */

#include <stdio.h>
#include <sys/param.h>
#include <rpc/rpc.h>        /* include <sys/time.h> */
#include <rpcsvc/rstat.h>

main(argc, argv)
int argc;
char **argv;
{

    char        *host;
    CLIENT      *rstat_clnt;
    statstime   *host_stat;
    struct tm   *tmp_time;
    struct tm    host_time;
    struct tm    host_uptime;
    char         days_buf[16];
    char         hours_buf[16];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s \"host\"\n", argv[0]);
        exit(1);
    }

    host = argv[1];

    /* client handle to rstat */
    rstat_clnt = clnt_create(host, RSTATPROG, RSTATVERS_TIME, "udp");
    if (rstat_clnt == NULL)
    {
        clnt_pcreateerror(argv[0]);
        exit(1);
    }

    host_stat = rstatproc_stats_3(NULL, rstat_clnt);
    if (host_stat == NULL)
    {
        clnt_perror(rstat_clnt, argv[0]);
        exit(1);
    }

    tmp_time = localtime(&host_stat->curtime.tv_sec);
    host_time = *tmp_time;

    host_stat->curtime.tv_sec -= host_stat->boottime.tv_sec;

    tmp_time = gmtime(&host_stat->curtime.tv_sec);
    host_uptime = *tmp_time;

    if (host_uptime.tm_yday != 0)
        sprintf(days_buf, "%d day%s, ", host_uptime.tm_yday,
            (host_uptime.tm_yday > 1) ? "s" : "");
    else
        days_buf[0] = '\0';

    if (host_uptime.tm_hour != 0)
        sprintf(hours_buf, "%2d:%02d,",
            host_uptime.tm_hour, host_uptime.tm_min);
    else
    if (host_uptime.tm_min != 0)
        sprintf(hours_buf, "%2d mins,", host_uptime.tm_min);
    else
        hours_buf[0] = '\0';

    printf(" %2d:%02d%cm  up %s%s load average: %.2f %.2f %.2f\n",
        (host_time.tm_hour > 12)  ? host_time.tm_hour - 12
                                  : host_time.tm_hour,
        host_time.tm_min,
        (host_time.tm_hour >= 12) ? 'p'
                                  : 'a',
        days_buf,
        hours_buf,
        (double)host_stat->avenrun[0]/FSCALE,
        (double)host_stat->avenrun[1]/FSCALE,
        (double)host_stat->avenrun[2]/FSCALE);

    exit(0);
}
