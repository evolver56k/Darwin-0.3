/*
 * $Header: /cvs/Darwin/CoreOS/Services/ntp/ntp/include/binio.h,v 1.1.1.1 1999/04/23 01:22:57 wsanchez Exp $
 *
 * $Created: Sun Jul 20 13:03:05 1997 $
 *
 * Copyright (C) 1997-1998 by Frank Kardel
 */
#ifndef BINIO_H
#define BINIO_H

#include "ntp_stdlib.h"

long get_lsb_short P((unsigned char **));
void put_lsb_short P((unsigned char **, long));
long get_lsb_long P((unsigned char **));
void put_lsb_long P((unsigned char **, long));

long get_msb_short P((unsigned char **));
void put_msb_short P((unsigned char **, long));
long get_msb_long P((unsigned char **));
void put_msb_long P((unsigned char **, long));

#endif
/*
 * $Log: binio.h,v $
 * Revision 1.1.1.1  1999/04/23 01:22:57  wsanchez
 * Import of ntp
 *
 * Revision 1.1.1.1  1998/10/30 22:18:04  wsanchez
 * Import of ntp 4.0.73e13
 *
 * Revision 4.2  1998/06/28 16:52:15  kardel
 * added binio MSB prototypes for {get,put}_msb_{short,long}
 *
 * Revision 4.1  1998/06/12 15:07:40  kardel
 * fixed prototyping
 *
 * Revision 4.0  1998/04/10 19:50:38  kardel
 * Start 4.0 release version numbering
 *
 * Revision 1.1  1998/04/10 19:27:32  kardel
 * initial NTP VERSION 4 integration of PARSE with GPS166 binary support
 *
 * Revision 1.1  1997/10/06 20:55:37  kardel
 * new parse structure
 *
 */
