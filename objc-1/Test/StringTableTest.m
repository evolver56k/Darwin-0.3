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

#include <libc.h>
#include <stdio.h>
#include <stdlib.h>
#include <objc/NXStringTable.h>

/* Tests to make sure that the lexer in NXStringTable.m is identical to
   the compiler's lexer. */

const char s1[] = "hello world";
const char s2[] = "\'\"\?\\\a\b\f\n\r\t\v";
const char s3[] = "\1234q\123q\12q\1q";
const char s4[] = "\xffq\xfq";
const char s5[] = "\q\8q";	/* error cases */

int main (void)
{
  NXStringTable *table = [[NXStringTable alloc] init];
  
  [table readFromFile: "test.strings"];
  
  if ([table valueForStringKey: s1] == NULL ||
      [table valueForStringKey: s2] == NULL ||
      [table valueForStringKey: s3] == NULL ||
      [table valueForStringKey: s4] == NULL ||
      [table valueForStringKey: s5] == NULL)
    {
      fprintf (stderr, "failed\n");
      return EXIT_FAILURE;
    }
  
  [table writeToFile: "test2.strings"];
  
  [table readFromFile: "test2.strings"];
  
  if ([table valueForStringKey: s1] == NULL ||
      [table valueForStringKey: s2] == NULL ||
      [table valueForStringKey: s3] == NULL ||
      [table valueForStringKey: s4] == NULL ||
      [table valueForStringKey: s5] == NULL)
    {
      fprintf (stderr, "failed\n");
      return EXIT_FAILURE;
    }
  
  fprintf (stderr, "passed\n");
  return EXIT_SUCCESS;
}
