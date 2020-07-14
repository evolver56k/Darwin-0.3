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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objc/typedstream.h>

static void ReadWriteType (const char *type, const void *in, void *out, int mode)
{
  char c = '!', d = '?';
  char newType[256];
  NXStream *stream = NXOpenMemory (NULL, 0, NX_READWRITE);
  
  strcpy (newType, "c");
  strcat (newType, type);
  strcat (newType, "c");
  
  if (mode == 0)
    NXWriteType (NXOpenTypedStream (stream, NX_WRITEONLY), type, in);
  else if (mode == 1)
    NXWriteTypes (NXOpenTypedStream (stream, NX_WRITEONLY), newType, &c, in, &d);
  else if (mode == 2)
    NXWriteArray (NXOpenTypedStream (stream, NX_WRITEONLY), type, 4, in);
  
  NXSeek (stream, 0, NX_FROMSTART);
  
  if (mode == 0)
    NXReadType (NXOpenTypedStream (stream, NX_READONLY), type, out);
  else if (mode == 1)
    NXReadTypes (NXOpenTypedStream (stream, NX_READONLY), newType, &c, out, &d);
  else if (mode == 2)
    NXReadArray (NXOpenTypedStream (stream, NX_READONLY), type, 4, out);
  
  NXCloseMemory (stream, NX_FREEBUFFER);
  
  if (mode == 1 && (c != '!' || d != '?'))
    fprintf (stderr, "ERROR: Read data does not match written!\n");
}

#define TEST_TYPE(TYPE, INIT)						\
{									\
  TEST_TYPE2 (TYPE, INIT, 0);						\
  TEST_TYPE2 (TYPE, INIT, 1);						\
}

#define TEST_TYPE2(TYPE, INIT, MODE)					\
{									\
  TYPE in = INIT;							\
  TYPE out;								\
  									\
  fprintf (stderr, "testing %s: \"%s\"\n", #TYPE, @encode (TYPE));	\
  									\
  ReadWriteType (@encode (TYPE), &in, &out, MODE);			\
  									\
  if (in != out)							\
    fprintf (stderr, "ERROR: Read data does not match written!\n");	\
}

#define TEST_ARRAY(TYPE, INIT)						\
{									\
  TEST_ARRAY2 (TYPE, INIT, 0);						\
  TEST_ARRAY2 (TYPE, INIT, 1);						\
  TEST_ARRAY2 (TYPE, INIT, 2);						\
}

#define TEST_ARRAY2(TYPE, INIT, MODE)					\
{									\
  TYPE in[4] = {INIT, INIT, INIT, INIT};				\
  TYPE out[4] = {0};							\
  									\
  fprintf (stderr, "testing %s[4]: \"%s\"\n", #TYPE, @encode (TYPE [4])); \
  									\
  if (MODE == 2)							\
    ReadWriteType (@encode (TYPE), in, out, MODE);			\
  else									\
    ReadWriteType (@encode (TYPE [4]), in, out, MODE);			\
  									\
  if (memcmp (in, out, sizeof (in)) != 0)				\
    fprintf (stderr, "ERROR: Read data does not match written\n");	\
}

#define TEST_STRUCT(TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4) \
{									\
  TEST_STRUCT2 (TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4, 0); \
  TEST_STRUCT2 (TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4, 1); \
  TEST_STRUCT2 (TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4, 2); \
  TEST_STRUCT2 (TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4, 3); \
}

#define TEST_STRUCT2(TYPE1, TYPE2, TYPE3, TYPE4, INIT1, INIT2, INIT3, INIT4, MODE) \
{									\
  struct foo {TYPE1 a; TYPE2 b; TYPE3 c; TYPE4 d;};			\
  struct foo in = {INIT1, INIT2, INIT3, INIT4};				\
  struct foo out = {0};							\
  char type[16];							\
  int mode = MODE;							\
  									\
  if (mode == 2 || mode == 3)						\
    {									\
      strcpy (type, @encode (struct foo));				\
      mode -= 2;							\
    }									\
  else									\
    sprintf (type, "{%s%s%s%s}", @encode (TYPE1), @encode (TYPE2),	\
				 @encode (TYPE3), @encode (TYPE4));	\
  									\
  fprintf (stderr, "testing struct: \"%s\"\n", type);			\
  									\
  ReadWriteType (type, &in, &out, mode);				\
  									\
  if (memcmp (&in, &out, sizeof (in)) != 0)				\
    fprintf (stderr, "ERROR: Read data does not match written\n");	\
}

#define TEST_UNION(TYPE1, TYPE2, INIT)					\
{									\
  TEST_UNION2 (TYPE1, TYPE2, INIT, 0);					\
  TEST_UNION2 (TYPE1, TYPE2, INIT, 1);					\
  TEST_UNION2 (TYPE1, TYPE2, INIT, 1);					\
  TEST_UNION2 (TYPE1, TYPE2, INIT, 2);					\
}

#define TEST_UNION2(TYPE1, TYPE2, INIT, MODE)				\
{									\
  union foo {TYPE1 a; TYPE2 b;};					\
  union foo in, out;							\
  char type[16];							\
  int mode = MODE;							\
  									\
  if (mode == 2 || mode == 3)						\
    {									\
      strcpy (type, @encode (union foo));				\
      mode -= 2;							\
    }									\
  else									\
    sprintf (type, "(%s%s)", @encode (TYPE1), @encode (TYPE2));		\
  									\
  fprintf (stderr, "testing union: \"%s\"\n", type);			\
  									\
  in.a = INIT;								\
  									\
  ReadWriteType (type, &in, &out, mode);				\
  									\
  if (memcmp (&in, &out, sizeof (in)) != 0)				\
    fprintf (stderr, "ERROR: Read data does not match written\n");	\
}

void main (void)
{
  TEST_TYPE (char, 'a');
  TEST_TYPE (signed char, 'b');
  TEST_TYPE (unsigned char, 'c');
  TEST_TYPE (short, 32121);
  TEST_TYPE (unsigned short, 54321);
  TEST_TYPE (int, 123);
  TEST_TYPE (unsigned int, 13);
  TEST_TYPE (long, 345);
  TEST_TYPE (unsigned long, 687);
  TEST_TYPE (float, 3.1415792);
  TEST_TYPE (double, 1.412);
  TEST_TYPE (long double, 1.23456);
  TEST_TYPE (SEL, @selector (foo:bar:));
  
  TEST_ARRAY (char, 'a');
  TEST_ARRAY (short, 'a');
  TEST_ARRAY (int, 'a');
  TEST_ARRAY (float, 'a');
  TEST_ARRAY (double, 'a');
  
  TEST_STRUCT (char, char, char, char, 'a', 'b', 'c', 'd');
  TEST_STRUCT (short, short, short, short, 1, 1000, 1, 1000);
  TEST_STRUCT (int, int, int, int, 1, 1000, 1000000, 1000000000);
  TEST_STRUCT (float, float, float, float, 1.0, 2.0, 3.0, 4.0);
  TEST_STRUCT (double, double, double, double, 1.0, 2.0, 3.0, 4.0);
  TEST_STRUCT (char, int, char, int, 'a', 1000, 'b', 1000000000);
  TEST_STRUCT (int, double, int, double, 1, 2.0, 3, 4.0);
  
  TEST_UNION (int, char, 1);
}
