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

#import <objc/Object.h>
#import <objc/objc-runtime.h>
#import <stdio.h>

@interface Forwarder : Object
{
  id delegate;
}

- initWithDelegate: anObject;
- forward: (SEL) sel : (marg_list) args;
@end

@implementation Forwarder

- initWithDelegate: anObject
{
  [super init];
  delegate = anObject;
  return self;
}

- forward: (SEL) sel : (marg_list) args
{
  Method method = class_getInstanceMethod ([delegate class], sel);
  unsigned int size = method_getSizeOfArguments (method);
  id result;
  static int dummy = 0;
  
  fprintf (stderr, "[Forwarder forward: \"%s\" : %u]\n",
	   sel_getName (sel), size);
  
  result = objc_msgSendv (delegate, sel, size, args);
  
  fprintf (stderr, "objc_msgSendv() returned\n");
  
  /* This computation causes d1 to get trashed. */
  dummy = dummy / 2 + size + 3;
  return result;
}

@end

struct foo
{
  int one;
  int two;
  int three;
  int four;
};

@interface Delegate : Object
{
  int x;
  float f;
  double d;
  struct foo s;
}

- (int) x;
- (void) setX: (int) anInt;
- (float) f;
- (void) setF: (float) aFloat;
- (double) d;
- (void) setD: (double) aDouble;
- (struct foo) s;
- (void) setS: (struct foo) aStruct;
@end

@implementation Delegate

- (int) x
{
  fprintf (stderr, "[Delegate x]\n");
  
  return x;
}

- (void) setX: (int) anInt
{
  fprintf (stderr, "[Delegate setX: %d]\n", anInt);
  
  x = anInt;
}

- (float) f
{
  fprintf (stderr, "[Delegate f]\n");
  
  return f;
}

- (void) setF: (float) aFloat
{
  fprintf (stderr, "[Delegate setF: %f]\n", aFloat);
  
  f = aFloat;
}

- (double) d
{
  fprintf (stderr, "[Delegate d]\n");
  
  return d;
}

- (void) setD: (double) aDouble
{
  fprintf (stderr, "[Delegate setD: %f]\n", aDouble);
  
  d = aDouble;
}

- (struct foo) s
{
  fprintf (stderr, "[Delegate s]\n");
  
  return s;
}

- (void) setS: (struct foo) aStruct
{
  fprintf (stderr, "[Delegate setS: {%d, %d, %d, %d}]\n",
	   aStruct.one, aStruct.two, aStruct.three, aStruct.four);
  
  s = aStruct;
}

@end

void main (void)
{
  id forwarder = [[Forwarder alloc] initWithDelegate: [[Delegate alloc] init]];
  struct foo s = {1, 2, 3, 4}, t;
  
  fprintf (stderr, "About to forward setX: 13\n");
  [forwarder setX: 13];
  fprintf (stderr, "Forwarding succeeded!\n");
  if ([forwarder x] == 13)
    fprintf (stderr, "passed for int\n");
  else
    fprintf (stderr, "failed for int\n");
  
  [forwarder setF: 13.0];
  if ([forwarder f] == 13.0)
    fprintf (stderr, "passed for float\n");
  else
    fprintf (stderr, "failed for float\n");
  
  [forwarder setD: 13.0];
  if ([forwarder d] == 13.0)
    fprintf (stderr, "passed for double\n");
  else
    fprintf (stderr, "failed for double\n");
  
  [forwarder setS: s];
  t = [forwarder s];
  if (memcmp (&t, &s, sizeof (struct foo)) == 0)
    fprintf (stderr, "passed for struct\n");
  else
    fprintf (stderr, "failed for struct\n");
  
  fprintf (stderr, "done\n");
}
