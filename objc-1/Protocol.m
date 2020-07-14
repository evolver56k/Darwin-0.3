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
/*
	Protocol.h
	Copyright 1991 NeXT, Inc.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include "objc-private.h"
#import "Protocol.h"
#import "maptable.h"
#include <mach-o/ldsyms.h>
#include "objc-runtime.h"
#include <stdlib.h>
#ifndef KERNEL
#include <mach-o/dyld.h>
#endif /* KERNEL */

/* some forward declarations */

static struct objc_method_description *
lookup_method(struct objc_method_description_list *mlist, SEL aSel);

static struct objc_method_description *
lookup_class_method(struct objc_protocol_list *plist, SEL aSel);

static struct objc_method_description *
lookup_instance_method(struct objc_protocol_list *plist, SEL aSel);

@implementation Protocol 

+ _fixup: (Protocol *)protos numElements: (int) nentries
{
  int i;
   
  for (i = 0; i < nentries; i++)
    {
      /* isa has been overloaded by the compiler to indicate version info */

      /* Versions 0 and 1 will never ship; for internal compatibility only.
	  Compensate for new "next" field in struct objc_protocol_list. */
      if ((int) protos[i].isa <= 1 && protos[i].protocol_list)
	(char *) protos[i].protocol_list -= 4;

      /* In version 0 protocols were referenced by name. */
      if ((int) protos[i].isa == 0 && protos[i].protocol_list)
	{
	  _objc_inform ("Unable to install protocols by name...\n");
	  _objc_inform ("Protocol %s must be recompiled.\n",
			protos[i].protocol_name);
	}

      protos[i].isa = self;	// install the class descriptor.    
    }

  return self;
}


- (BOOL) conformsTo: (Protocol *)aProtocolObj
{
  if (!aProtocolObj)
    return NO;

  if (strcmp(aProtocolObj->protocol_name, protocol_name) == 0)
    return YES;
  else if (protocol_list)
    {
    int i;
    
    for (i = 0; i < protocol_list->count; i++)
      {
      Protocol *p = protocol_list->list[i];

      if (strcmp(aProtocolObj->protocol_name, p->protocol_name) == 0)
        return YES;
   
      if ([p conformsTo:aProtocolObj])
	return YES;
      }
    return NO;
    }
  else
    return NO;
}

- (struct objc_method_description *) descriptionForInstanceMethod:(SEL)aSel
{
   struct objc_method_description *m = lookup_method(instance_methods, aSel);

   if (!m && protocol_list)
     m = lookup_instance_method(protocol_list, aSel);

   return m;
}

- (struct objc_method_description *) descriptionForClassMethod:(SEL)aSel
{
   struct objc_method_description *m = lookup_method(class_methods, aSel);

   if (!m && protocol_list)
     m = lookup_class_method(protocol_list, aSel);

   return m;
}

- (const char *)name
{
  return protocol_name;
}

static 
struct objc_method_description *
lookup_method(struct objc_method_description_list *mlist, SEL aSel)
{
   if (mlist)
     {
     int i;
     for (i = 0; i < mlist->count; i++)
       if (mlist->list[i].name == aSel)
         return mlist->list+i;
     }
   return 0;
}

static 
struct objc_method_description *
lookup_instance_method(struct objc_protocol_list *plist, SEL aSel)
{
   int i;
   struct objc_method_description *m = 0;

   for (i = 0; i < plist->count; i++)
     {
     if (plist->list[i]->instance_methods)
       m = lookup_method(plist->list[i]->instance_methods, aSel);
   
     /* depth first search */  
     if (!m && plist->list[i]->protocol_list)
       m = lookup_instance_method(plist->list[i]->protocol_list, aSel);

     if (m)
       return m;
     }
   return 0;
}

static 
struct objc_method_description *
lookup_class_method(struct objc_protocol_list *plist, SEL aSel)
{
   int i;
   struct objc_method_description *m = 0;

   for (i = 0; i < plist->count; i++)
     {
     if (plist->list[i]->class_methods)
       m = lookup_method(plist->list[i]->class_methods, aSel);
   
     /* depth first search */  
     if (!m && plist->list[i]->protocol_list)
       m = lookup_class_method(plist->list[i]->protocol_list, aSel);

     if (m)
       return m;
     }
   return 0;
}

@end
