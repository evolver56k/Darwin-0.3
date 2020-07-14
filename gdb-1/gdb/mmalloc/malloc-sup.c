/* Support for mmalloc using system malloc()
   Copyright 1992 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com

This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "mmprivate.h"

/* Cache the pagesize for the current host machine.  Note that if the host
   does not readily provide a getpagesize() function, we need to emulate it
   elsewhere, not clutter up this file with lots of kluges to try to figure
   it out. */

static size_t pagesize;
extern int getpagesize PARAMS ((void));

#define PAGE_ALIGN(addr) (caddr_t) (((long)(addr) + pagesize - 1) & \
				    ~(pagesize - 1))

/*  Get core for the memory region specified by MDP, using SIZE as the
    amount to either add to or subtract from the existing region.  Works
    like sbrk(), but using malloc(). */

static PTR
mmalloc_malloc_morecore (mdp, size)
  struct mdesc *mdp;
  int size;
{
  size_t current_size = 0;
  size_t new_size = 0;

  if (pagesize == 0)
    {
      pagesize = getpagesize ();
    }

  if (size == 0)
    {
      /* Just return the current "break" value. */
      return mdp -> breakval;
    }

  /* If we are deallocating memory and the amount requested would
     cause us to try to deallocate back past the base of the malloc buffer,
     then do nothing, and return NULL.  Otherwise, deallocate
     the memory and return the old break value. */
  
  current_size = mdp -> breakval - mdp -> base;

  if ((size < 0) && ((-size) > current_size))
    {
      abort ();
    }

  new_size = current_size + size;
  
  if (mdp -> base)
    {
      mdp -> base = realloc (mdp->base, new_size);
      if (! mdp -> base)
	{
	  return NULL;
	}
      mdp -> top = mdp -> base + new_size;
      mdp -> breakval = mdp -> base + current_size;
    }
  else
    {
      mdp -> base = malloc (new_size);
      if (! mdp -> base)
	{
	  return NULL;
	}
      mdp -> breakval = mdp -> base;
      mdp -> top = mdp -> breakval + new_size;
    }

  return mdp -> breakval;
}

struct mdesc *
mmalloc_malloc_create ()
{
  struct mdesc *ret = NULL;

  ret = (struct mdesc *) malloc (sizeof (struct mdesc));
  memset ((char *) ret, 0, sizeof (struct mdesc));
  ret -> morecore = mmalloc_malloc_morecore;
  ret -> base = 0;
  ret -> breakval = ret -> top = ret->base;
  ret -> fd = -1;

  return ret;
}
