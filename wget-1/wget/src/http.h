/* Declarations for HTTP support.
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


/* $Id: http.h,v 1.1.1.1 1999/04/23 02:05:56 wsanchez Exp $ */

#ifndef HTTP_H
#define HTTP_H

/* Header HTTP definitions */
#define CONTLEN_H "Content-length:"
#define CONTRANGE_H "Content-Range:"
#define ACCEPTRANGES_H "Accept-Ranges:"
#define CONTTYPE_H "Content-type:"
#define LOCATION_H "Location:"
#define LASTMODIFIED_H "Last-Modified:"
#define TEXTHTML_S "text/html"
#define HTTP_ACCEPT "*/*"

/* Some status code validation macros: */
#define H_20X(x)        (((x) >= 200) && ((x) < 300))
#define H_PARTIAL(x)    ((x) == HTTP_PARTIAL_CONTENTS)
#define H_REDIRECTED(x) (((x) == HTTP_MOVED_PERMANENTLY) || ((x) == HTTP_MOVED_TEMPORARILY))

/* HTTP/1.0 status codes from RFC1945, given for reference. */
/* Successful 2xx. */
#define HTTP_OK                200
#define HTTP_CREATED           201
#define HTTP_ACCEPTED          202
#define HTTP_NO_CONTENT        204
#define HTTP_PARTIAL_CONTENTS  206

/* Redirection 3xx. */
#define HTTP_MULTIPLE_CHOICES  300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_MOVED_TEMPORARILY 302
#define HTTP_NOT_MODIFIED      304

/* Client error 4xx. */
#define HTTP_BAD_REQUEST       400
#define HTTP_UNAUTHORIZED      401
#define HTTP_FORBIDDEN         403
#define HTTP_NOT_FOUND         404

/* Server errors 5xx. */
#define HTTP_INTERNAL          500
#define HTTP_NOT_IMPLEMENTED   501
#define HTTP_BAD_GATEWAY       502
#define HTTP_UNAVAILABLE       503

/* Typedefs: */
typedef struct {
   long len;                    /* Received length. */
   long contlen;                /* Expected length. */
   long restval;                /* The restart value. */
   int res;                     /* The result of last read. */
   char *newloc;                /* New location (redirection). */
   char *remote_time;           /* Remote time-stamp string. */
   char *error;                 /* Textual HTTP error. */
   int statcode;		/* Status code. */
   long dltime;                 /* Time of the download. */
} http_stat_t;

/* A macro to free the elements of hstat. */
#define FREEHSTAT(x)                            \
do                                              \
{                                               \
   if (x.newloc)                                \
      free(x.newloc);                           \
   if (x.remote_time)                           \
      free(x.remote_time);                      \
   if (x.error)                                 \
      free(x.error);                            \
   x.newloc = x.remote_time = x.error = NULL;   \
} while (0)

/* Function declarations */
uerr_t fetch_next_header PARAMS((int, char **));

int hskip_lws PARAMS((const char *));
int hparsestatline PARAMS((const char *, const char **));
long hgetlen PARAMS((const char *));
long hgetrange PARAMS((const char *));
char *hgettype PARAMS((const char *));
char *hgetlocation PARAMS((const char *));
char *hgetmodified PARAMS((const char *));
int haccepts_none PARAMS((const char *));

uerr_t gethttp PARAMS((urlinfo *, http_stat_t *, int *));
uerr_t http_loop PARAMS((urlinfo *, char **, int *));

char *base64_encode_line PARAMS((const char *));
time_t mktime_from_utc PARAMS((struct tm *));
time_t http_atotm PARAMS((char *));

#endif /* HTTP_H */
