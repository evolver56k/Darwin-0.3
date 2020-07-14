/* Declarations for basic FTP support.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


/* $Id: ftp-basic.h,v 1.1.1.1 1999/04/23 02:05:55 wsanchez Exp $ */

#ifndef FTP_BASIC_H
#define FTP_BASIC_H

char *ftp_request PARAMS((const char *, const char *));
uerr_t ftp_response PARAMS((int, char **));
uerr_t ftp_login PARAMS((int, const char *, const char *));
uerr_t ftp_port PARAMS((int));
uerr_t ftp_pasv PARAMS((int, unsigned char *));
uerr_t ftp_type PARAMS((int, int));
uerr_t ftp_cwd PARAMS((int, const char *));
uerr_t ftp_retr PARAMS((int, const char *));
uerr_t ftp_rest PARAMS((int, long));
uerr_t ftp_list PARAMS((int, const char *));
long ftp_expected_bytes PARAMS((const char *));

#endif /* FTP_BASIC_H */
