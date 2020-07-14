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
 *	objc-private.h
 *	Copyright 1988, NeXT, Inc.
 */

#ifndef _OBJC_PRIVATE_H_
#define _OBJC_PRIVATE_H_

#import "objc-config.h"

#ifdef KERNEL
#import	<mach/mach_types.h>
#import	<mach/machine/simple_lock.h>

#define	mutex_alloc()	simple_lock_alloc()
#define	mutex_init(x)	simple_lock_init(x)
#define	mutex_lock(x)	simple_lock(x)
#define	mutex_try_lock(x)	simple_lock_try(x)
#define	mutex_unlock(x)	simple_unlock(x)
#define	mutex_clear(x)
#define mutex		slock
#define	mutex_t		simple_lock_t
#else /* KERNEL */
#import <mach/cthreads.h>
#endif /* KERNEL */

#import <stdlib.h>
#import <stdarg.h>
#import <stdio.h>
#import <string.h>
#import <ctype.h>

#import "objc.h"
#import "objc-runtime.h"
#import "hashtable.h"

#import <sys/time.h>

struct header_info
{
  const struct mach_header *mhdr;
  Module mod_ptr;
  unsigned int mod_count;
  void *frozenTable;
  unsigned long image_slide;
  unsigned int objcSize;
};
extern struct header_info *_objc_headerVector (const struct mach_header * const *machhdrs);
extern unsigned int _objc_headerCount (void);
extern void _objc_addHeader (const struct mach_header *header, long vmaddr_slide);
extern void _objc_removeHeader (const struct mach_header *header);
extern SEL __S(_sel_registerName)(const char *str);

/* utilities */
static inline unsigned int _objc_strhash (const unsigned char *s)
{
  unsigned int hash = 0;

  /* Unroll the loop. */
  while (1)
    { 
      if (*s == '\0') 
	break;
      hash ^= *s++;
      if (*s == '\0') 
	break;
      hash ^= *s++ << 8;
      if (*s == '\0') 
	break;
      hash ^= *s++ << 16;
      if (*s == '\0') 
	break;
      hash ^= *s++ << 24;
    }
  
  return hash;
}

extern const char *__S(_nameForHeader) (const struct mach_header *);

/* debug */
extern int gettime(void);
extern void print_time(const char *, int, struct timeval *);

/* initialize */
extern void __S(_sel_init) (const struct mach_header *,
		       const char *, unsigned int, void *);
extern void __S(_sel_unloadSelectors)(const char *, const char *);
extern void _class_install_relationships(Class, long);
extern void _objc_add_category(Category, int);
extern void _objc_remove_category(Category, int);
extern void _class_removeProtocols(Class, struct objc_protocol_list *);
extern NXZone *_objc_create_zone(void);
extern void _objc_removeClass(Class);

/* method lookup */
extern BOOL class_respondsToMethod(Class, SEL);
extern IMP class_lookupMethod(Class, SEL);
extern IMP class_lookupMethodInMethodList(struct objc_method_list *mlist, 
                                          SEL sel);

/* message dispatcher */
extern Cache _cache_create(Class);
extern IMP _class_lookupMethodAndLoadCache(Class, SEL);
extern id _objc_msgForward (id self, SEL sel, ...);

/* errors */
volatile void __S(_objc_fatal)(const char *message);
extern volatile void _objc_error(id, const char *, va_list);
extern volatile void __objc_error(id, const char *, ...);
extern void _objc_inform(const char *fmt, ...);
extern void _NXLogError(const char *format, ...);

/* magic */
extern Class _objc_getFreedObjectClass (void);
extern Class _objc_getNonExistentClass (void);
extern const struct objc_cache emptyCache;
extern void _objc_flush_caches (Class cls);

/* locking */
extern int _objc_multithread_mask;
extern struct mutex classLock;
extern struct mutex messageLock;

#ifdef KERNEL
#ifdef hppa  /* The sense of kernel locks on the hp are backwards! */
#define OBJC_DECLARE_LOCK(MUTEX) struct slock MUTEX = { -1,-1,-1,-1 }
#elif defined(ppc)
#define OBJC_DECLARE_LOCK(MUTEX) struct slock MUTEX = { 0, 0, 0, 0 }
#else
#define OBJC_DECLARE_LOCK(MUTEX) struct slock MUTEX = { 0 }
#endif hppa
#define OBJC_LOCK(MUTEX)
#define OBJC_UNLOCK(MUTEX)
#define OBJC_TRYLOCK(MUTEX)	1

#else /* KERNEL */

#define OBJC_DECLARE_LOCK(MUTEX) struct mutex MUTEX = { 0, #MUTEX }

#ifdef FREEZE
#define OBJC_LOCK(MUTEX)
#define OBJC_UNLOCK(MUTEX)
#else /* FREEZE */
#ifdef OBJC_COLLECTING_CACHE
#define OBJC_LOCK(MUTEX) 	mutex_lock (MUTEX)
#define OBJC_UNLOCK(MUTEX)	mutex_unlock (MUTEX)
#define OBJC_TRYLOCK(MUTEX)	mutex_try_lock (MUTEX)
#else
#define OBJC_LOCK(MUTEX)		\
  do					\
    {					\
      if (!_objc_multithread_mask)	\
	mutex_lock (MUTEX);		\
    }					\
  while (0)

#define OBJC_UNLOCK(MUTEX)		\
  do					\
    {					\
      if (!_objc_multithread_mask)	\
	mutex_unlock (MUTEX);		\
    }					\
  while (0)

#endif /* OBJC_COLLECTING_CACHE */
#endif /* FREEZE */
#endif /* KERNEL */

#endif /* _OBJC_PRIVATE_H_ */
