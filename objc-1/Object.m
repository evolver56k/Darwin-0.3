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
	Object.m
	Copyright 1988, 1989 NeXT, Inc.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"
#import <stdarg.h> 
#import <string.h> 

#import "objc-runtime.h"	
#import "typedstream.h"
#import "Protocol.h"

extern id (*_cvtToId)(const char *);
extern id (*_poseAs)();

#define ISMETA(cls)		((cls)->info & CLS_META) 

// Error Messages
static const char
	_errNoMem[] = "failed -- out of memory(%s, %u)",
	_errReAllocNil[] = "reallocating nil object",
	_errReAllocFreed[] = "reallocating freed object",
	_errReAllocTooSmall[] = "(%s, %u) requested size too small",
	_errShouldHaveImp[] = "should have implemented the '%s' method.",
	_errShouldNotImp[] = "should NOT have implemented the '%s' method.",
	_errLeftUndone[] = "method '%s' not implemented",
	_errBadSel[] = "method %s given invalid selector %s",
	_errDoesntRecognize[] = "does not recognize selector %c%s";

#import "Object.h"

@implementation Object 


+ initialize
{
	return self; 
}

- awake 
{
	return self; 
}

+ poseAs: aFactory
{ 
	return (*_poseAs)(self, aFactory); 
}

+ new
{
	id newObject = (*_alloc)((Class)self, 0);
	Class metaClass = ((Class) self)->isa;
	if (metaClass->version > 1)
	    return [newObject init];
	else
	    return newObject;
}

+ alloc
{
	return (*_zoneAlloc)((Class)self, 0, NXDefaultMallocZone()); 
}

+ allocFromZone:(NXZone *) zone
{
	return (*_zoneAlloc)((Class)self, 0, zone); 
}

- init
{
    return self;
}

- (const char *)name
{
	return isa->name; 
}

+ (const char *)name
{
	return ((Class)self)->name; 
}

- (unsigned)hash
{
	return ((unsigned)self) >> 2; 
}

- (BOOL)isEqual:anObject
{
	return anObject == self; 
}

- free 
{ 
	return (*_dealloc)(self); 
}

+ free
{
	return nil; 
}

- self
{
	return self; 
}

- class
{
	return (id)isa; 
}

+ class 
{
	return self;
}

- (NXZone *)zone
{
	NXZone *zone = NXZoneFromPtr(self);
	return zone ? zone : NXDefaultMallocZone();
}

+ superclass 
{ 
	return ((Class)self)->super_class; 
}

- superclass 
{ 
	return isa->super_class; 
}

+ (int) version
{
	Class	class = (Class) self;
	return class->version;
}

+ setVersion: (int) aVersion
{
	Class	class = (Class) self;
	class->version = aVersion;
	return self;
}

- (BOOL)isKindOf:aClass
{
	register Class cls;
	for (cls = isa; cls; cls = cls->super_class) 
		if (cls == (Class)aClass)
			return YES;
	return NO;
}

- (BOOL)isMemberOf:aClass
{
	return isa == (Class)aClass;
}

- (BOOL)isKindOfClassNamed:(const char *)aClassName
{
	register Class cls;
	for (cls = isa; cls; cls = cls->super_class) 
		if (strcmp(aClassName, cls->name) == 0)
			return YES;
	return NO;
}

- (BOOL)isMemberOfClassNamed:(const char *)aClassName 
{
	return strcmp(aClassName, isa->name) == 0;
}

+ (BOOL)instancesRespondTo:(SEL)aSelector 
{
	return class_respondsToMethod((Class)self, aSelector);
}

- (BOOL)respondsTo:(SEL)aSelector 
{
	return class_respondsToMethod(isa, aSelector);
}

- copy 
{
	return [self copyFromZone: [self zone]];
}

- copyFromZone:(NXZone *)zone
{
	return (*_zoneCopy)(self, 0, zone); 
}

- (IMP)methodFor:(SEL)aSelector 
{
	return class_lookupMethod(isa, aSelector);
}

+ (IMP)instanceMethodFor:(SEL)aSelector 
{
	return class_lookupMethod(self, aSelector);
}

- perform:(SEL)aSelector 
{ 
	if (aSelector)
		return objc_msgSend(self, aSelector); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- perform:(SEL)aSelector with:anObject 
{
	if (aSelector)
		return objc_msgSend(self, aSelector, anObject); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- perform:(SEL)aSelector with:obj1 with:obj2 
{
	if (aSelector)
		return objc_msgSend(self, aSelector, obj1, obj2); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- subclassResponsibility:(SEL)aSelector 
{
	return [self error:_errShouldHaveImp, sel_getName(aSelector)];
}

- notImplemented:(SEL)aSelector
{
	return [self error:_errLeftUndone, sel_getName(aSelector)];
}

- doesNotRecognize:(SEL)aMessage
{
	return [self error:_errDoesntRecognize, 
		ISMETA (isa) ? '+' : '-', SELNAME(aMessage)];
}

- error:(const char *)aCStr, ... 
{
	va_list ap;
	va_start(ap,aCStr); 
	(*_error)(self, aCStr, ap); 
	_objc_error (self, aCStr, ap);	/* In case (*_error)() returns. */
	va_end(ap);
        return nil;
}

- (void) printForDebugger:(NXStream *)stream
{
	NXPrintf (stream, "<%s: 0x%x>", object_getClassName (self), self);
	NXFlush (stream);
}

- write:(NXTypedStream *) stream 
{
	return self;
}

- read:(NXTypedStream *) stream 
{
	return self;
}

- forward: (SEL) sel : (marg_list) args 
{
    return [self doesNotRecognize: sel];
}

/* this method is not part of the published API */

- (unsigned)methodArgSize:(SEL)sel 
{
    Method	method = class_getInstanceMethod((Class)isa, sel);
    if (! method) return 0;
    return method_getSizeOfArguments(method);
}

- performv: (SEL) sel : (marg_list) args 
{
    unsigned	size;
#if hppa && 0
    void *ret;
   
    // Save ret0 so methods that return a struct might work.
    asm("copy %%r28, %0": "=r"(ret): );
#endif hppa

    if (! self) return nil;
    size = [self methodArgSize: sel];
    if (! size) return [self doesNotRecognize: sel];

#if hppa && 0
    // Unfortunately, it looks like the compiler puts something else in
    // r28 right after this instruction, so this is all for naught.
    asm("copy %0, %%r28": : "r"(ret));
#endif hppa

    return objc_msgSendv (self, sel, size, args); 
}

/* Testing protocol conformance */

- (BOOL) conformsTo: (Protocol *)aProtocolObj
{
  return [isa conformsTo:aProtocolObj];
}

+ (BOOL) conformsTo: (Protocol *)aProtocolObj
{
  Class class;

  for (class = self; class; class = class->super_class)
    {
      if (class->isa->version >= 3)
        {
	  struct objc_protocol_list *protocols = class->protocols;

	  while (protocols)
	    {
	      int i;

	      for (i = 0; i < protocols->count; i++)
		{
		  Protocol *p = protocols->list[i];
    
		  if ([p conformsTo:aProtocolObj])
		    return YES;
		}

	      if (class->isa->version <= 4)
	        break;

	      protocols = protocols->next;
	    }
	}
    }
  return NO;
}

#if hppa && 0
#define OBJC_REMOTE_SIGN_BUG 1
#endif

#if OBJC_REMOTE_SIGN_BUG
/*
 * The problem being worked around here is the one where NextStep DO 
 * (actually ObjC) can't handle minus signs in method descriptor strings sent
 * by HP clients.  We remove them here, but this code returns the wrong 
 * answer for local clients.
 * 
 * The bug needs to get fixed in NextStep. This workaround is really ugly.
 */
static char *
stripsigns(char *str)
{
    char *dst;
    char *src;

    if (str) {
	dst = src = str;
	while (*src) {
	    if (*src == '-')  {
		src++;	/* skip minus sign */
	    } else {
	    	*dst++ = *src++;	/* copy */
	    }
	}
	*dst = 0;
    }
    return (str);
}
#endif /* OBJC_REMOTE_SIGN_BUG */

/* Looking up information for a method */

- (struct objc_method_description *) descriptionForMethod:(SEL)aSelector
{
  Class cls;
  struct objc_method_description *m;

  /* Look in the protocols first. */
  for (cls = isa; cls; cls = cls->super_class)
    {
      if (cls->isa->version >= 3)
        {
	  struct objc_protocol_list *protocols = cls->protocols;
  
	  while (protocols)
	    {
	      int i;

	      for (i = 0; i < protocols->count; i++)
		{
		  Protocol *p = protocols->list[i];

		  if (ISMETA (cls))
		    m = [p descriptionForClassMethod:aSelector];
		  else
		    m = [p descriptionForInstanceMethod:aSelector];

		  if (m) {
#if OBJC_REMOTE_SIGN_BUG
#warning "Working around OBJC_REMOTE_SIGN_BUG"
		      m->types = stripsigns(m->types);
#endif /* OBJC_REMOTE_SIGN_BUG */
		      return m;
		  }
		}
  
	      if (cls->isa->version <= 4)
		break;
  
	      protocols = protocols->next;
	    }
	}
    }

  /* Then try the class implementations. */
  for (cls = isa; cls; cls = cls->super_class)
    {
      struct objc_method_list *methods;

      for (methods = cls->methods; methods; methods = methods->method_next)
	{
	  int i;
  
	  for (i = 0; i < methods->method_count; i++)
	    if (methods->method_list[i].method_name == aSelector) {
	      m = (struct objc_method_description *)
		      &methods->method_list[i];
#if OBJC_REMOTE_SIGN_BUG
	      m->types = stripsigns(m->types);
#endif /* OBJC_REMOTE_SIGN_BUG */
	      return (m);
	    }
	}
    }

  return 0;
}

+ (struct objc_method_description *) descriptionForInstanceMethod:(SEL)aSelector
{
  Class cls;

  /* Look in the protocols first. */
  for (cls = self; cls; cls = cls->super_class)
    {
      if (cls->isa->version >= 3)
        {
	  struct objc_protocol_list *protocols = cls->protocols;
  
	  while (protocols)
	    {
	      int i;

	      for (i = 0; i < protocols->count; i++)
		{
		  Protocol *p = protocols->list[i];
		  struct objc_method_description *m;

		  if ((m = [p descriptionForInstanceMethod:aSelector]))
		    return m;
		}
  
	      if (cls->isa->version <= 4)
		break;
  
	      protocols = protocols->next;
	    }
	}
    }

  /* Then try the class implementations. */
  for (cls = self; cls; cls = cls->super_class)
    {
      struct objc_method_list *methods;

      for (methods = cls->methods; methods; methods = methods->method_next)
	{
	  int i;
  
	  for (i = 0; i < methods->method_count; i++)
	    if (methods->method_list[i].method_name == aSelector)
	      return (struct objc_method_description *)
		      &methods->method_list[i];
	}
    }

  return 0;
}


/* Obsolete methods (for binary compatibility only). */

+ superClass
{
	return [self superclass];
}

- superClass
{
	return [self superclass];
}

- (BOOL)isKindOfGivenName:(const char *)aClassName
{
	return [self isKindOfClassNamed: aClassName];
}

- (BOOL)isMemberOfGivenName:(const char *)aClassName 
{
	return [self isMemberOfClassNamed: aClassName];
}

- (struct objc_method_description *) methodDescFor:(SEL)aSelector
{
  return [self descriptionForMethod: aSelector];
}

+ (struct objc_method_description *) instanceMethodDescFor:(SEL)aSelector
{
  return [self descriptionForInstanceMethod: aSelector];
}

- findClass:(const char *)aClassName
{
	return (*_cvtToId)(aClassName);
}

- shouldNotImplement:(SEL)aSelector
{
	return [self error:_errShouldNotImp, sel_getName(aSelector)];
}

@end

/* System Primitive...declared as `private externs' in shared library */

id _internal_object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	id obj;
	register unsigned siz;

	if (anObject == nil)
		return nil;

	obj = (*_zoneAlloc)(anObject->isa, nBytes, zone);
	siz = anObject->isa->instance_size + nBytes;
	bcopy(anObject, obj, siz);
	return obj;
}

id _internal_object_copy(Object *anObject, unsigned nBytes) 
{
    NXZone *zone = NXZoneFromPtr(anObject);
    return _internal_object_copyFromZone(anObject, 
					 nBytes,
					 zone ? zone : NXDefaultMallocZone());
}

id _internal_object_dispose(Object *anObject) 
{
	if (anObject==nil) return nil;
	anObject->isa = _objc_getFreedObjectClass (); 
	free(anObject);
	return nil;
}

id _internal_object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	Object *newObject; 
	Class tmp;

	if (anObject == nil)
		__objc_error(nil, _errReAllocNil, 0);

	if (anObject->isa == _objc_getFreedObjectClass ())
		__objc_error(anObject, _errReAllocFreed, 0);

	if (nBytes < ((Class)anObject->isa)->instance_size)
		__objc_error(anObject, _errReAllocTooSmall, 
				object_getClassName(anObject), nBytes);

	// Make sure not to modify space that has been declared free
	tmp = anObject->isa; 
	anObject->isa = _objc_getFreedObjectClass ();
	newObject = (id)NXZoneRealloc(zone, anObject, nBytes);
	if (newObject) {
		newObject->isa = tmp;
		return newObject;
	}
	else
            {
		__objc_error(anObject, _errNoMem, 
				object_getClassName(anObject), nBytes);
                return nil;
            }
}

id _internal_object_realloc(Object *anObject, unsigned nBytes) 
{
    NXZone *zone = NXZoneFromPtr(anObject);
    return _internal_object_reallocFromZone(anObject,
					    nBytes,
					    zone ? zone : NXDefaultMallocZone());
}

/* Functional Interface to system primitives */

id object_copy(Object *anObject, unsigned nBytes) 
{
	return (*_copy)(anObject, nBytes); 
}

id object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneCopy)(anObject, nBytes, zone); 
}

id object_dispose(Object *anObject) 
{
	return (*_dealloc)(anObject); 
}

id object_realloc(Object *anObject, unsigned nBytes) 
{
	return (*_realloc)(anObject, nBytes); 
}

id object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneRealloc)(anObject, nBytes, zone); 
}

Ivar object_setInstanceVariable(id obj, const char *name, void *value)
{
	Ivar ivar = 0;

	if (obj && name) {
		void **ivaridx;

		if ((ivar = class_getInstanceVariable(((Object*)obj)->isa, name))) {
		       ivaridx = (void **)((char *)obj + ivar->ivar_offset);
		       *ivaridx = value;
		}
	}
	return ivar;
}

Ivar object_getInstanceVariable(id obj, const char *name, void **value)
{
	Ivar ivar = 0;

	if (obj && name) {
		void **ivaridx;

		if ((ivar = class_getInstanceVariable(((Object*)obj)->isa, name))) {
		       ivaridx = (void **)((char *)obj + ivar->ivar_offset);
		       *value = *ivaridx;
		} else
		       *value = 0;
	}
	return ivar;
}
