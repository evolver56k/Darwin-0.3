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
/*
	Protocol.h
	Copyright 1991-1996 NeXT Software, Inc.
*/

#if defined(WIN32)
    #include <winnt-pdo.h>
#endif

#include "objc-private.h"
#import <objc/Protocol.h>
#import "maptable.h"

#include <objc/objc-runtime.h>
#include <stdlib.h>

#if defined(KERNEL)
    extern const struct section *getsectbynamefromheader(
        const struct mach_header *mhp,
        const char *segname,
        const char *sectname);
#elif defined(__MACH__) 
    #include <mach-o/dyld.h>
    #include <mach-o/ldsyms.h>
#endif 

/* some forward declarations */

static struct objc_method_description *
lookup_method(struct objc_method_description_list *mlist, SEL aSel);

static struct objc_method_description *
lookup_class_method(struct objc_protocol_list *plist, SEL aSel);

static struct objc_method_description *
lookup_instance_method(struct objc_protocol_list *plist, SEL aSel);

@implementation Protocol 

/* Version 3 classes appear to have been expunged from NeXT,
   but version 4 classes may still be present.  We should eliminate
   this function entirely once all classes at NeXT are version 5. */

static void fixup_classes()
{
   NXHashTable *class_hash = objc_getClasses();
   NXHashState state;
   Class clsObject;

  OBJC_LOCK (&classLock);

  state = NXInitHashState(class_hash);

  while (NXNextHashState(class_hash, &state, (void **)&clsObject)) 
    {
      /* Versions 3 and 4 will never ship; for internal compatibility only.
	  Compensate for new "next" field in struct objc_protocol_list. */
      if (clsObject->isa->version >= 3 && clsObject->isa->version <= 4
          && clsObject->protocols)
	{
	  (char *) clsObject->protocols -= 4;
	  (char *) clsObject->isa->protocols -= 4;
	}

      /* version 3 will never be shipped...for internal compatibility */
      if ((clsObject->isa->version == 3) && clsObject->protocols)
        {
	  _objc_inform ("Unable to install protocols by name...\n");
	  _objc_inform ("Class %s must be recompiled.\n",
			clsObject->name);
          clsObject->protocols = 0;
          clsObject->isa->protocols = 0;
	}
    }

  OBJC_UNLOCK (&classLock);
}

+ _fixup: (OBJC_PROTOCOL_PTR)protos numElements: (int) nentries
{
  int i;
  for (i = 0; i < nentries; i++)
    {
      /* isa has been overloaded by the compiler to indicate version info */

      /* Versions 0 and 1 will never ship; for internal compatibility only.
	  Compensate for new "next" field in struct objc_protocol_list. */
      if ((((arith_t) protos[i] OBJC_PROTOCOL_DEREF isa == 0) || ((arith_t) protos[i] OBJC_PROTOCOL_DEREF isa == 1)) &&
         protos[i] OBJC_PROTOCOL_DEREF protocol_list) {
	(char *) protos[i] OBJC_PROTOCOL_DEREF protocol_list -= 4;
	}
      /* In version 0 protocols were referenced by name. */
      if ((arith_t) protos[i] OBJC_PROTOCOL_DEREF isa == 0 && protos[i] OBJC_PROTOCOL_DEREF protocol_list)
	{
	  _objc_inform ("Unable to install protocols by name...\n");
	  _objc_inform ("Protocol %s must be recompiled.\n",
			protos[i] OBJC_PROTOCOL_DEREF protocol_name);
	}
      protos[i] OBJC_PROTOCOL_DEREF isa = self;	// install the class descriptor.    
    }

  return self;
}

+ load
{
  OBJC_PROTOCOL_PTR p;
  int size;
  headerType **hp;
  headerType **hdrs;
  hdrs = _getObjcHeaders();

  for (hp = hdrs; *hp; hp++) 
    {
      p = (OBJC_PROTOCOL_PTR)_getObjcProtocols((headerType*)*hp, &size);
      if (p && size) { [self _fixup:p numElements: size]; }
    }
  fixup_classes();
  free (hdrs);

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

- (BOOL)isEqual:other
{
    return [other isKindOf:[Protocol class]] && [self conformsTo: other] && [other conformsTo: self];
}

- (unsigned int)hash
{
    return 23;
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
