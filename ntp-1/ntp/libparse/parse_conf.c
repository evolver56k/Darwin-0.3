/*
 * $Header: /cvs/Darwin/CoreOS/Services/ntp/ntp/libparse/parse_conf.c,v 1.1.1.2 1999/04/23 01:23:22 wsanchez Exp $
 *  
 * $Id: parse_conf.c,v 1.1.1.2 1999/04/23 01:23:22 wsanchez Exp $
 *
 * Parser configuration module for reference clocks
 *
 * STREAM define switches between two personalities of the module
 * if STREAM is defined this module can be used with dcf77sync.c as
 * a STREAMS kernel module. In this case the time stamps will be
 * a struct timeval.
 * when STREAM is not defined NTP time stamps will be used.
 *
 * Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998 by Frank Kardel
 * Friedrich-Alexander Universität Erlangen-Nürnberg, Germany
 *                                    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && defined(CLOCK_PARSE)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_calendar.h"

#include "parse.h"

#ifdef CLOCK_SCHMID
extern clockformat_t clock_schmid;
#endif

#ifdef CLOCK_DCF7000
extern clockformat_t clock_dcf7000;
#endif

#ifdef CLOCK_MEINBERG
extern clockformat_t clock_meinberg[];
#endif

#ifdef CLOCK_RAWDCF
extern clockformat_t clock_rawdcf;
#endif

#ifdef CLOCK_TRIMTAIP
extern clockformat_t clock_trimtaip;
#endif

#ifdef CLOCK_TRIMTSIP
extern clockformat_t clock_trimtsip;
#endif

#ifdef CLOCK_RCC8000
extern clockformat_t clock_rcc8000;
#endif

#ifdef CLOCK_HOPF6021
extern clockformat_t clock_hopf6021;
#endif

#ifdef CLOCK_COMPUTIME
extern clockformat_t clock_computime;
#endif

/*
 * format definitions
 */
clockformat_t *clockformats[] =
{
#ifdef CLOCK_MEINBERG
	&clock_meinberg[0],
	&clock_meinberg[1],
	&clock_meinberg[2],
#endif
#ifdef CLOCK_DCF7000
	&clock_dcf7000,
#endif
#ifdef CLOCK_SCHMID
	&clock_schmid,
#endif
#ifdef CLOCK_RAWDCF
	&clock_rawdcf,
#endif
#ifdef CLOCK_TRIMTAIP
	&clock_trimtaip,
#endif
#if defined(CLOCK_TRIMTSIP) && !defined(PARSESTREAM)
	&clock_trimtsip,
#endif
#ifdef CLOCK_RCC8000
	&clock_rcc8000,
#endif
#ifdef CLOCK_HOPF6021
	&clock_hopf6021,
#endif
#ifdef CLOCK_COMPUTIME
	&clock_computime,
#endif
	0};

unsigned short nformats = sizeof(clockformats) / sizeof(clockformats[0]) - 1;

#else /* not (REFCLOCK && CLOCK_PARSE) */
int parse_conf_bs;
#endif /* not (REFCLOCK && CLOCK_PARSE) */

/*
 * History:
 *
 * $Log: parse_conf.c,v $
 * Revision 1.1.1.2  1999/04/23 01:23:22  wsanchez
 * Import of ntp
 *
 * Revision 1.1.1.2  1998/10/30 22:18:18  wsanchez
 * Import of ntp 4.0.73e13
 *
 * Revision 4.2  1998/06/12 09:13:48  kardel
 * conditional compile macros fixed
 *
 * Revision 4.1  1998/05/24 09:40:49  kardel
 * adjustments of log messages
 *
 *
 * from V3 3.24 log info deleted 1998/04/11 kardel
 */
