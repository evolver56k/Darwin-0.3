/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef KERNEL
#ifdef SHLIB
#include "shlib.h"
#endif

#include <libc.h>
#include <stdlib.h>
#include <mach-o/loader.h>

static void call_constructors (void);
static void call_destructors (void);
static void call_routines_for_section (const struct mach_header **headers,
				       const char *segment_name,
				       const char *section_name);

/* Perform C++ initialization.  This only works for the MH_EXECUTE and
   MH_FVMLIB formats, since the headers must be mapped.
   
   crt0.o uses the trick of declaring a common symbol with the same name
   as this function to call this routine if it is linked in for some other
   reason, but without forcing it to be linked in itself. */

void _cplus_init (void)
{
  /* Each module which contains constructors or destructors should
     contain an undefined reference to these symbols to ensure that
     this initialization routine is linked in. */
  asm (".globl .constructors_used");
  asm (".constructors_used = 0");
  asm (".globl .destructors_used");
  asm (".destructors_used = 0");
  
  atexit (&call_destructors);
  call_constructors ();
}


/* Call the appropriate contructor for each static object in the program. */

static void call_constructors (void)
{
  const struct mach_header **headers = getmachheaders ();

  call_routines_for_section (headers, "__TEXT", "__constructor");
  free (headers);
}


/* Call the appropriate destructor for each static object in the program. */

static void call_destructors (void)
{
  const struct mach_header **headers = getmachheaders ();

  call_routines_for_section (getmachheaders (), "__TEXT", "__destructor");
  free (headers);
}


/* Call the init functions in the specified section of each header.
   The section is assumed to contain a sequential list of function pointers.
   The functions are called with no arguments, and no return value is
   expected. */

static void call_routines_for_section (const struct mach_header **headers,
				       const char *segment_name,
				       const char *section_name)
{
  unsigned int i;
  
  if (headers == NULL)
    return;
  
  for (i = 0; headers[i] != NULL; i++)
    {
      const struct section *section;
      void (**init_routines) (void);
      unsigned int n, j;
      
      section = getsectbynamefromheader (headers[i],
					 segment_name,
					 section_name);
      
      if (section == NULL)
        continue;
      
      init_routines = (void (**) (void)) section->addr;
      n = section->size / sizeof (init_routines[0]);
      
      for (j = 0; j < n; j++)
	(*init_routines[j]) ();
    }
}

#endif /* KERNEL */