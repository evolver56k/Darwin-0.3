/*
 * $Header: /cvs/Darwin/CoreOS/Services/ntp/ntp/include/ascii.h,v 1.1.1.1 1999/04/23 01:22:57 wsanchez Exp $
 *
 * $Created: Sun Jul 20 11:42:53 1997 $
 *
 * Copyright (C) 1997 by Frank Kardel
 */
#ifndef ASCII_H
#define ASCII_H

/*
 * just name the common ASCII control codes
 */
#define NUL	  0
#define SOH	  1
#define STX	  2
#define ETX	  3
#define EOT	  4
#define ENQ	  5
#define ACK	  6
#define BEL	  7
#define BS	  8
#define HT	  9
#define NL	 10
#define VT	 11
#define NP	 12
#define CR	 13
#define SO	 14
#define SI	 15
#define DLE	 16
#define DC1	 17
#define DC2	 18
#define DC3	 19
#define DC4	 20
#define NAK	 21
#define SYN	 22
#define ETB	 23
#define CAN	 24
#define EM	 25
#define SUB	 26
#define ESC	 27
#define FS	 28
#define GS	 29
#define RS	 30
#define US	 31
#define SP	 32
#define DEL	127

#endif
/*
 * $Log: ascii.h,v $
 * Revision 1.1.1.1  1999/04/23 01:22:57  wsanchez
 * Import of ntp
 *
 * Revision 1.1.1.1  1998/10/30 22:18:04  wsanchez
 * Import of ntp 4.0.73e13
 *
 * Revision 4.0  1998/04/10 19:50:38  kardel
 * Start 4.0 release version numbering
 *
 * Revision 1.1  1998/04/10 19:27:32  kardel
 * initial NTP VERSION 4 integration of PARSE with GPS166 binary support
 *
 * Revision 1.1  1997/10/06 20:55:38  kardel
 * new parse structure
 *
 */
