/* memory allocation routines with error checking.
   Copyright 1989, 90, 91, 92, 93, 94 Free Software Foundation, Inc.
   
This file is part of the libiberty library.
Libiberty is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Libiberty is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with libiberty; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"

#include "ansidecl.h"
#include "libiberty.h"

#undef USE_EFENCE

#include <stdio.h>

#ifdef __STDC__
#include <stddef.h>
#else
#define size_t unsigned long
#define ptrdiff_t long
#endif

#if VMS
#include <stdlib.h>
#include <unixlib.h>
#else
/* For systems with larger pointers than ints, these must be declared.  */
PTR malloc PARAMS ((size_t));
PTR calloc PARAMS ((size_t, size_t));
PTR realloc PARAMS ((PTR, size_t));
PTR sbrk PARAMS ((ptrdiff_t));
void free (PTR ptr);
#endif

#ifdef USE_EFENCE
#include "efence.h"
#endif

#include <assert.h>
#include <limits.h>

#define MAX_SIZE ULONG_MAX

#undef malloc
#undef realloc
#undef free

/* The program name if set.  */
static const char *name = "";

#if !defined (__CYGWIN__) && defined (__CYGWIN32__)
#define __CYGWIN__ 1
#endif

#undef USE_SBRK

#if ! defined (_WIN32) || defined (__CYGWIN__) || defined (__UWIN__)
#define USE_SBRK 1
#endif

#if defined (__APPLE_CC__)
#undef USE_SBRK
#endif

#if USE_SBRK
/* The initial sbrk, set when the program name is set. Not used for win32
   ports other than cygwin32.  */
static char *first_break = NULL;
#endif /* USE_SBRK */

void
xmalloc_set_program_name (s)
     const char *s;
{
  name = s;
#if USE_SBRK
  /* Win32 ports other than cygwin32 don't have brk() */
  if (first_break == NULL)
    first_break = (char *) sbrk (0);
#endif /* USE_SBRK */
}

#if defined (USE_MMALLOC)

#include <mmalloc.h>

#else /* ! USE_MMALLOC */

PTR
mmalloc (md, size)
     PTR md;
     size_t size;
{
  assert (size < MAX_SIZE);
  return malloc (size);
}

PTR
mcalloc (md, nmemb, size)
     PTR md;
     size_t nmemb;
     size_t size;
{
  assert (nmemb < (MAX_SIZE / size));
  return calloc (nmemb, size);
}

PTR
mrealloc (md, ptr, size)
     PTR md;
     PTR ptr;
     size_t size;
{
  assert (size < MAX_SIZE);
  if (ptr == 0)         /* Guard against old realloc's */
    return malloc (size);
  else
    return realloc (ptr, size);
}

void
mfree (md, ptr)
     PTR md;
     PTR ptr;
{
  free (ptr);
}

#endif  /* USE_MMALLOC */

#if ! defined (USE_EFENCE)

static void nomem (size)
{
#if USE_SBRK
  extern char **environ;
  size_t allocated;
  
  if (first_break != NULL)
    allocated = (char *) sbrk (0) - first_break;
  else
    allocated = (char *) sbrk (0) - (char *) &environ;
  fprintf (stderr,
	   "\n%s%sCan not allocate %lu bytes after allocating %lu bytes\n",
	   name, *name ? ": " : "",
	   (unsigned long) size, (unsigned long) allocated);
#else /* ! USE_SBRK */
  fprintf (stderr,
	   "\n%s%sCan not allocate %lu bytes\n",
	   name, *name ? ": " : "",
	   (unsigned long) size);
#endif /* USE_SBRK */
}

/* Like mmalloc but get error if no storage available, and protect against
   the caller wanting to allocate zero bytes.  Whether to return NULL for
   a zero byte request, or translate the request into a request for one
   byte of zero'd storage, is a religious issue. */

PTR
xmmalloc (md, size)
     PTR md;
     long size;
{
  PTR val;

  assert (size < MAX_SIZE);

  if (size == 0)
    return NULL;
  
  val = mmalloc (md, size);
  if (val == NULL)
    nomem (size);
  
  return val;
}

PTR
xmcalloc (md, nelem, elsize)
     PTR md;
     size_t nelem;
     size_t elsize;
{
  PTR val;

  if (nelem == 0 || elsize == 0)
    return NULL;

  assert (nelem < (MAX_SIZE / elsize));
  val = mcalloc (md, nelem, elsize);

  if (val == NULL)
    nomem (nelem * elsize);
  
  return val;
}

/* Like mrealloc but get error if no storage available.  */

PTR
xmrealloc (md, ptr, size)
     PTR md;
     PTR ptr;
     long size;
{
  PTR val;

  assert (size < MAX_SIZE);
  if (ptr != NULL)
    val = mrealloc (md, ptr, size);
  else
    val = mmalloc (md, size);

  if (val == NULL)
      nomem (size);

  return val;
}

void
xmfree (md, ptr)
     PTR md;
     PTR ptr;
{
  if (ptr == NULL)
    return;
  mfree (md, ptr);
}

#else 

/* Like mmalloc but get error if no storage available, and protect against
   the caller wanting to allocate zero bytes.  Whether to return NULL for
   a zero byte request, or translate the request into a request for one
   byte of zero'd storage, is a religious issue. */

PTR
xmmalloc (md, size)
     PTR md;
     long size;
{
  register PTR val;

  assert (size < MAX_SIZE);
  if (size == 0)
    return NULL;

  val = efence_malloc (size);
  if (val == NULL)
    nomem (size);

  return val;
}

PTR
xmcalloc (md, nelem, esize)
     PTR md;
     size_t nelem;
     size_t elsize;
{
  PTR newmem;

  if (nelem == 0 || elsize == 0)
    return NULL;

  assert (nelem < (MAX_SIZE / elsize));
  newmem = calloc (nelem, elsize);

  if (newmem == NULL)
    nomem (nelem * elsize);
  
  return newmem;
}

/* Like mrealloc but get error if no storage available.  */

PTR
xmrealloc (md, ptr, size)
     PTR md;
     PTR ptr;
     long size;
{
  register PTR val;

  assert (size < MAX_SIZE);
  if (ptr != NULL)
    val = efence_realloc (ptr, size);
  else
    val = efence_malloc (size);

  if (val == NULL)
    nomem (size);

  return val;
}

void
xmfree (md, ptr)
     PTR md;
     PTR ptr;
{
  if (ptr == NULL) 
    return;
  efence_free (ptr);
}

#endif	/* USE_EFENCE */

/* Like malloc but get error if no storage available, and protect against
   the caller wanting to allocate zero bytes.  */

PTR
xmalloc (size)
     size_t size;
{
  assert (size < MAX_SIZE);
  return (xmmalloc ((PTR) NULL, size));
}

PTR
xcalloc (nelem, elsize)
     size_t nelem;
     size_t elsize;
{
  assert ((nelem * elsize) < MAX_SIZE);
  return (xmcalloc ((PTR) NULL, nelem, elsize));
}

/* Like mrealloc but get error if no storage available.  */

PTR
xrealloc (ptr, size)
     PTR ptr;
     size_t size;
{
  assert (size < MAX_SIZE);
  return (xmrealloc ((PTR) NULL, ptr, size));
}

void
xfree (ptr)
     PTR ptr;
{
  xmfree ((PTR) NULL, ptr);
}
