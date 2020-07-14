/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
// Copyright 1988-1996 NeXT Software, Inc.

#if !defined(KERNEL) && !defined(NeXT_PDO)

#include <stdlib.h>
#include <mach-o/loader.h>
#include <crt_externs.h>

/* prototype coming soon to <mach-o/getsect.h> */
extern const struct section *getsectbynamefromheader(
    const struct mach_header *mhp,
    const char *segname,
    const char *sectname);

static void call_constructors (void);
static void call_destructors (void);
static void call_routines_for_section (const struct mach_header *header,
				       const char *segment_name,
				       const char *section_name);

/* Perform C++ initialization for static code generation.
   
   crt0.o uses the trick of declaring a common symbol with the same name
   as this function to call this routine if it is linked in for some other
   reason, but without forcing it to be linked in itself. */

void _cplus_init (void)
{
  /* Each module which contains constructors or destructors should
     contain an undefined reference to these symbols to ensure that
     this initialization routine is linked in.  Again this is for
     static code generation. */
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
  call_routines_for_section (_NSGetMachExecuteHeader(), "__TEXT", "__constructor");
}


/* Call the appropriate destructor for each static object in the program. */

static void call_destructors (void)
{
  call_routines_for_section (_NSGetMachExecuteHeader(), "__TEXT", "__destructor");
}


/* Call the init functions in the specified section of each header.
   The section is assumed to contain a sequential list of function pointers.
   The functions are called with no arguments, and no return value is
   expected.  Again this is for static code generation. */

static void call_routines_for_section (const struct mach_header *header,
				       const char *segment_name,
				       const char *section_name)
{
  unsigned int i;
  const struct section *section;
  void (**init_routines) (void);
  unsigned int n, j;
  
      section = getsectbynamefromheader (header,
					 segment_name,
					 section_name);
      
      if (section == NULL)
        return;
      
      init_routines = (void (**) (void)) section->addr;
      n = section->size / sizeof (init_routines[0]);
      
      for (j = 0; j < n; j++)
	(*init_routines[j]) ();
}

#endif /* not KERNEL && not NeXT_PDO */
