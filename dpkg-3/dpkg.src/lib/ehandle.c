/*
 * libdpkg - Debian packaging suite library routines
 * ehandle.c - error handling
 *
 * Copyright (C) 1994, 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 * Copyright (C) 1997, 1998 Klee Dienes <klee@alum.mit.edu>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ehandle.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <setjmp.h>
#include <assert.h>

#include "varbuf.h"
#include "dpkg.h"
#include "dpkg-db.h"
#include "libdpkg-int.h"
#include "config.h"

#define NCALLS 2

extern const char *errmsg;
extern char errmsgbuf[];

struct cleanupentry {
  struct cleanupentry *next;
  struct {
    int mask;
    void (*call) (int argc, void **argv);
  } calls[NCALLS];
  int cpmask, cpvalue;
  int argc;
  void *argv[1];
};         

struct errorcontext {
  struct errorcontext *next;
  jmp_buf *jbufp;
  struct cleanupentry *cleanups;
  printerror_func *printerror;
  const char *contextstring;
};

/* points to errmsgbuf or malloc'd.  also used by ehandle.c */

static const char *errmsg;

/* 6x255 for inserted strings (%.255s &c in fmt; also %s with limited length arg)
   1x255 for constant text
   1x255 for strerror()
   same again just in case. */

static char errmsgbuf[4096];

static struct errorcontext *volatile econtext = NULL;

static struct {
  struct cleanupentry ce;
  void *args[20];
} emergency;

void set_error_display
(printerror_func *printerror, const char *contextstring)
{
  assert (econtext != NULL);
  econtext->printerror = printerror;
  econtext->contextstring = contextstring;
}

void push_error_handler
(jmp_buf *jbufp, printerror_func *printerror, const char *context)
{
  struct errorcontext *necp;
  necp = malloc (sizeof (struct errorcontext));
  if (! necp) {
    int e = errno;
    strcpy (errmsgbuf, (_("out of memory pushing error handler: ")));
    strcat (errmsgbuf, strerror (e));
    errmsg = errmsgbuf;
    if (econtext) {
      longjmp (*econtext->jbufp, 1);
    }
    fprintf (stderr, "%s: %s\n", thisname, errmsgbuf);
    exit(EXIT_FAILURE);
  }
  necp->next = econtext;
  necp->jbufp = jbufp;
  necp->cleanups = 0;
  necp->printerror = printerror;
  necp->contextstring = context;
  econtext = necp;
  onerr_abort = 0;
}

static void print_error_cleanup (const char *msg, const char *context)
{
  fprintf (stderr, _("%s: error while cleaning up:\n %s\n"), thisname, msg);
}

/* run all cleanup functions for a given errorcontext */

static void run_cleanups (struct errorcontext *econ, int flagsetin)
{
  static volatile int preventrecurse = 0;
  struct cleanupentry *volatile cep;
  struct cleanupentry *ncep;
  struct errorcontext recurserr, *oldecontext;
  jmp_buf recurejbuf;
  volatile int i, flagset;

  onerr_abort++;
  preventrecurse++;

  if (econ->printerror) {
    econ->printerror (errmsg, econ->contextstring);
  }
     
  if (preventrecurse > 3) {
    fprintf (stderr, _("dpkg: too many nested errors during error recovery !!\n"));
    flagset = 0;
  } else {
    flagset = flagsetin;
  }
  cep = econ->cleanups;
  oldecontext = econtext;
  while (cep) {
    for (i=0; i<NCALLS; i++) {
      if (cep->calls[i].call && cep->calls[i].mask & flagset) {
        if (setjmp (recurejbuf)) {
          run_cleanups(&recurserr, ehflag_bombout | ehflag_recursiveerror);
        } else {
          recurserr.jbufp = &recurejbuf;
          recurserr.cleanups = 0;
          recurserr.next = 0;
          recurserr.printerror = print_error_cleanup;
          recurserr.contextstring = 0;
          econtext = &recurserr;
          cep->calls[i].call (cep->argc,cep->argv);
        }
        econtext = oldecontext;
      }
    }
    ncep = cep->next;
    if (cep != &emergency.ce) {
      free (cep);
    }
    cep= ncep;
  }

  preventrecurse--;
  onerr_abort--;
}

void error_unwind (int flagset)
{
  struct cleanupentry *cep;
  struct errorcontext *tecp;

  tecp = econtext;
  cep = tecp->cleanups;
  econtext = tecp->next;
  run_cleanups (tecp, flagset);
  free (tecp);
}

void push_checkpoint (int mask, int value)
{
  struct cleanupentry *cep;
  int i;
  
  cep = m_malloc (sizeof (struct cleanupentry) + sizeof (char*));
  for (i = 0; i < NCALLS; i++) {
    cep->calls[i].call = NULL;
    cep->calls[i].mask = NULL;
  }
  cep->cpmask = mask;
  cep->cpvalue = value;
  cep->argc= 0;
  cep->argv[0]= 0;
  cep->next = econtext->cleanups;
  econtext->cleanups= cep;
}

void push_cleanup 
(void (*call1) (int argc, void **argv), int mask1,
 void (*call2) (int argc, void **argv), int mask2,
 int nargs, ...) 
{
  struct cleanupentry *cep;
  void **args;
  va_list al;

  onerr_abort++;
  
  cep = malloc (sizeof (struct cleanupentry) + sizeof (char*) * (nargs + 1));
  if (cep == NULL) {
    if (nargs > sizeof (emergency.args) / sizeof(void*)) {
      ohshit (_("unable to allocate memory for new cleanup entry (memory exhausted)"));
    }
    cep = &emergency.ce;
  }
  cep->calls[0].call = call1;
  cep->calls[0].mask = mask1;
  cep->calls[1].call = call2;
  cep->calls[1].mask = mask2;
  cep->cpmask = ~0;
  cep->cpvalue = 0;
  cep->argc = nargs;
  
  va_start (al, nargs);
  args = cep->argv;
  while (nargs-- >0) {
    *args++= va_arg (al, void*);
  }
  *args++ = 0;
  va_end (al);

  cep->next = econtext->cleanups;
  econtext->cleanups = cep;
  if (cep == &emergency.ce) {
    ohshite ("unable to allocate memory for new cleanup entry (memory exhausted)");
  }

  onerr_abort--;
}

void pop_cleanup (int flagset)
{
  struct cleanupentry *cep;
  int i;

  cep = econtext->cleanups;
  econtext->cleanups= cep->next;

  for (i = 0; i < NCALLS; i++) {
    if (cep->calls[i].call && (cep->calls[i].mask & flagset)) {
      cep->calls[i].call (cep->argc, cep->argv);
    }
    flagset &= cep->cpmask;
    flagset |= cep->cpvalue;
  }
  
  if (cep != &emergency.ce) {
    free (cep);
  }
}

void ohshit (const char *fmt, ...)
{
  va_list al;
  va_start (al,fmt);
  vsprintf (errmsgbuf, fmt, al);
  va_end (al);
  errmsg = errmsgbuf;
  longjmp (*econtext->jbufp, 1);
}

void ohshits (const char *s)
{
  errmsg = s;
  longjmp (*econtext->jbufp, 1);
}

void ohshitvb (struct varbuf *vb)
{
  char *m;
  varbufaddc (vb, 0);
  m = m_malloc (strlen (vb->buf));
  strcpy (m, vb->buf);
  errmsg = m;
  longjmp (*econtext->jbufp, 1);
}

void ohshitv (const char *fmt, va_list al)
{
  vsprintf (errmsgbuf, fmt, al);
  errmsg = errmsgbuf;
  longjmp (*econtext->jbufp, 1);
}

void ohshite (const char *fmt, ...)
{
  int e;
  va_list al;

  e = errno;
  va_start (al, fmt);
  vsprintf (errmsgbuf, fmt, al);
  va_end (al);

  strcat (errmsgbuf, ": ");
  strcat (errmsgbuf, strerror (e));
  errmsg = errmsgbuf; 
  longjmp (*econtext->jbufp, 1);
}

void badusage (const char *fmt, ...)
{
  va_list al;
  va_start (al,fmt);
  vsprintf (errmsgbuf, fmt, al);
  va_end (al);
  strcat (errmsgbuf, "\n\n");
  strcat (errmsgbuf, G_(printforhelp));
  errmsg = errmsgbuf; 
  longjmp (*econtext->jbufp, 1);
}

void werr (const char *fn) 
{
  ohshite (_("error writing to output stream `%.250s'"), fn);
}

void do_internerr (const char *string, int line, const char *file)
{
  fprintf (stderr, _("%s:%d: internal error `%s'\n"), file, line, string);
  abort ();
}
