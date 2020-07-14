/*
 * $Header: /cvs/Darwin/CoreOS/Services/ntp/ntp/include/ieee754io.h,v 1.1.1.1 1999/04/23 01:22:58 wsanchez Exp $
 *
 * $Created: Sun Jul 13 12:22:11 1997 $
 *
 * Copyright (C) 1997 by Frank Kardel
 */
#ifndef IEEE754IO_H
#define IEEE754IO_H

#define IEEE_SINGLE 1
#define IEEE_DOUBLE 2

#define IEEE_MSB 1
#define IEEE_LSB 2

#define IEEE_OK          0	/* conversion ok */
#define IEEE_BADCALL     1	/* bad call parameters */
#define IEEE_NAN         2	/* found an NaN */
#define IEEE_POSINFINITY 3	/* positive infinity */
#define IEEE_NEGINFINITY 4	/* negative infinity */
#define IEEE_POSOVERFLOW 5	/* positive overflow */
#define IEEE_NEGOVERFLOW 6	/* negative overflow */

#define IEEE_OFFSETS     8	/* number of byte positions */
typedef unsigned char offsets_t[IEEE_OFFSETS];

int fetch_ieee754 P((unsigned char **bufp, int size, l_fp *lfpp, offsets_t offsets));
int put_ieee754 P((unsigned char **bufpp, int size, l_fp *lfpp, offsets_t offsets));

#endif
/*
 * $Log: ieee754io.h,v $
 * Revision 1.1.1.1  1999/04/23 01:22:58  wsanchez
 * Import of ntp
 *
 * Revision 1.1.1.1  1998/10/30 22:18:04  wsanchez
 * Import of ntp 4.0.73e13
 *
 * Revision 4.0  1998/04/10 19:50:40  kardel
 * Start 4.0 release version numbering
 *
 * Revision 1.1  1998/04/10 19:27:33  kardel
 * initial NTP VERSION 4 integration of PARSE with GPS166 binary support
 *
 * Revision 1.1  1997/10/06 20:55:37  kardel
 * new parse structure
 *
 */
