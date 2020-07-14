/*
 * $Header: /cvs/Darwin/CoreOS/Services/ntp/ntp/libntp/gpstolfp.c,v 1.1.1.1 1999/04/23 01:23:10 wsanchez Exp $
 *
 * $Created: Sun Jun 28 16:30:38 1998 $
 *
 * Copyright (C) 1998 by Frank Kardel
 */
#include "ntp_fp.h"

#define GPSORIGIN	(unsigned)(2524953600)  /* NTP origin - GPS origin in seconds */
#define SECSPERWEEK	(unsigned)(604800)	/* seconds per week - GPS tells us about weeks */
#define GPSWRAP		930	/* assume week count less than this in the previous epoch */

void
gpstolfp(
	 int weeks,
	 int days,
	 unsigned long  seconds,
	 l_fp * lfp
	 )
{
  if (weeks < GPSWRAP)
    {
      weeks += 1024;
    }

  lfp->l_ui = weeks * SECSPERWEEK + days * 86400 + seconds + GPSORIGIN; /* convert to NTP time */
  lfp->l_uf = 0;
}

/*
 * $Log: gpstolfp.c,v $
 * Revision 1.1.1.1  1999/04/23 01:23:10  wsanchez
 * Import of ntp
 *
 * Revision 1.1.1.1  1998/10/30 22:18:16  wsanchez
 * Import of ntp 4.0.73e13
 *
 * Revision 4.1  1998/06/28 16:47:15  kardel
 * added gpstolfp() function
 *
 */
