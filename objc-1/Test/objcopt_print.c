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

#import <libc.h>
#import <stdio.h>
#import <streams/streams.h>
#import <mach-o/loader.h>

#define SIZEHASHTABLE 	821

typedef struct optHashedSelector 	OHASH, *POHASH;

struct optHashedSelector {
	POHASH		next;
	const char *	key_uid;
};

#define MAP_HASH(x) \
	(x ? (void *) ((int) hashtable + (int) (x) - hashtable_addr) : NULL)
#define MAP_STRING(x) \
	(x ? (void *) ((int) strings + (int) (x) - strings_addr): NULL)
#define UNMAP_HASH(x) \
	(x ? (void *) (hashtable_addr + (int) (x) - (int) hashtable) : NULL)

int main (int argc, char *argv[])
{
  NXStream *stream;
  char *buffer;
  int length, allocLength;
  const struct mach_header *header;
  const struct section *section;
  POHASH *hashtable;
  const char *strings;
  int hashtable_addr, strings_addr, strings_size, hashtable_size;
  unsigned int i;
  unsigned int maxLength = 0;
  unsigned int total = 0;
  unsigned int emptyBuckets = 0;
  
  if (argc != 2)
    {
      fprintf (stderr, "usage: %s filename\n", argv[0]);
      return EXIT_FAILURE;
    }
  
  stream = NXMapFile (argv[1], NX_READONLY);
  
  NXGetMemoryBuffer (stream, &buffer, &length, &allocLength);
  
  header = (const struct mach_header *) buffer;
  
  section = getsectbynamefromheader (header, "__OBJC", "__runtime_setup");
  
  if (!section || section->size == 0)
    {
      fprintf (stderr, "cannot find __runtime_setup section\n");
      exit (EXIT_FAILURE);
    }
  
  hashtable_addr = (int) section->addr;
  hashtable_size = (int) section->size;
  hashtable = (POHASH *) (buffer + section->offset);
  
  printf ("%d hashtable entries from 0x%x to 0x%x\n",
	  SIZEHASHTABLE,
	  hashtable_addr,
	  hashtable_addr + SIZEHASHTABLE * sizeof (POHASH));
  printf ("%d buckets from 0x%x to 0x%x\n",
	  (hashtable_size - SIZEHASHTABLE * sizeof (POHASH)) / sizeof (OHASH),
	  hashtable_addr + SIZEHASHTABLE * sizeof (POHASH),
	  hashtable_addr + hashtable_size);
  
  section = getsectbynamefromheader (header, "__OBJC", "__meth_var_names");
  
  if (!section || section->size == 0)
    section = getsectbynamefromheader (header, "__OBJC", "__selector_strs");
  
  if (!section || section->size == 0)
    {
      fprintf (stderr,
	       "cannot find __meth_var_names or __selector_strs section\n");
      exit (EXIT_FAILURE);
    }
  
  strings_addr = (int) section->addr;
  strings_size = (int) section->size;
  strings = (const char *) (buffer + section->offset);
  
  printf ("%d bytes of strings from 0x%x to 0x%x\n",
	  strings_size,
	  strings_addr,
	  strings_addr + strings_size);
  
  for (i = 0; i < SIZEHASHTABLE; i++)
    {
      POHASH target = MAP_HASH (hashtable[i]);
      int length = 0;
      
//      printf ("hash[%d]:", i);
      
      while (target)
        {
	  POHASH next;
	  const char *string = MAP_STRING (target->key_uid);
	  
	  length++;
	  
	  if (string < strings || strings >= strings + strings_size)
	    {
	      printf ("Bad string pointer 0x%x in entry %d at address 0x%x\n",
		      string, i, UNMAP_HASH (&target->key_uid));
	      target = 0;
	      continue;
	    }
	  if (string != strings && string[-1] != '\0')
	    {
	      const char *start = strrchr (string - 1, '\0') + 1;
	      
	      printf ("bad string %s in entry %d (substring of %s)\n",
		      string, i, start);
	    }
//	  printf ( " %s", string);
	  next = MAP_HASH (target->next);
	  if (next && (next < (POHASH) &hashtable[SIZEHASHTABLE]
	      || next >= (POHASH) ((int) hashtable + hashtable_size)))
	    {
	      printf ("Bad next pointer 0x%x in entry %d at address 0x%x\n",
		      next, i, UNMAP_HASH (&target->next));
	      next = 0;
	    }
	  target = next;
	}
      
//      printf ("\n");
      
      total += length;
      
      if (length == 0)
        emptyBuckets++;
      
      if (length > maxLength)
        maxLength = length;
    }
  
  printf ("%d buckets\n", SIZEHASHTABLE);
  printf ("%d entries\n", total);
  printf ("%f average entries/bucket\n", (float) total / SIZEHASHTABLE);
  printf ("%d maximum entries/bucket\n", maxLength);
  printf ("%d empty buckets\n", emptyBuckets);
  
  return EXIT_SUCCESS;
}

#if 0
Êû
#endif
