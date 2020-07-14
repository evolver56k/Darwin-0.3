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
 *	objc-private.h
 *	Copyright 1988-1996, NeXT Software, Inc.
 */

#if !defined(_OBJC_PRIVATE_H_)
    #define _OBJC_PRIVATE_H_

    #import <objc/objc-api.h>	// for OBJC_EXPORT

    OBJC_EXPORT void checkUniqueness();

    #import "objc-config.h"

    #if defined(KERNEL)
        #import <mach/mach_types.h>
        #import <mach/machine/simple_lock.h>

        #define	mutex_alloc()	simple_lock_alloc()
        #define	mutex_init(x)	simple_lock_init(x)
        #define	mutex_lock(x)	simple_lock(x)
        #define	mutex_try_lock(x)	simple_lock_try(x)
        #define	mutex_unlock(x)	simple_unlock(x)
        #define	mutex_clear(x)
        #define mutex		slock
        #define	mutex_t		simple_lock_t
    #elif defined(NeXT_PDO)
        #define LITERAL_STRING_OBJECTS
        #import <objc/zone.h>	// zome defines over subs
        #import <mach/cthreads_private.h>
        #if defined(WIN32)
	    #import <winnt-pdo.h>
	    #import <ntunix.h>
	#else
            #import <pdo.h>	// for pdo_malloc and pdo_free defines
            #import <sys/time.h>
        #endif 
    #else 
        #import <mach/cthreads.h>
        #import <sys/time.h>
    #endif // KERNEL

    #import <stdlib.h>
    #import <stdarg.h>
    #import <stdio.h>
    #import <string.h>
    #import <ctype.h>

    #import <objc/objc-class.h>
    #import <objc/hashtable2.h>
    #import <objc/objc-runtime.h>

    #import "objc-file.h"	// OS dependent getsect implementatons

    #define END_OF_METHODS_LIST ((struct objc_method_list*)-1)

    struct header_info
    {
      const headerType *	mhdr;
      Module				mod_ptr;
      unsigned int			mod_count;
      void *				frozenTable;
      unsigned long			image_slide;
      unsigned int			objcSize;
    };
    typedef struct header_info	header_info;
    OBJC_EXPORT header_info *_objc_headerVector (const headerType * const *machhdrs);
    OBJC_EXPORT unsigned int _objc_headerCount (void);
    OBJC_EXPORT void _objc_addHeader (const headerType *header, unsigned long vmaddr_slide);
    OBJC_EXPORT void _objc_removeHeader (const headerType *header);
    OBJC_EXPORT SEL __S(_sel_registerName)(const char *str);

    OBJC_EXPORT int _objcModuleCount();
    OBJC_EXPORT const char *_objcModuleNameAtIndex(int i);
    OBJC_EXPORT Class objc_getOrigClass (const char *name);

    extern struct objc_method_list *get_base_method_list(Class cls);

	OBJC_EXPORT uarith_t hashPtrStructKey (const void *info, const void *data);
	OBJC_EXPORT int isEqualPtrStructKey (const void *info, const void *data1, const void *data2);
	OBJC_EXPORT uarith_t hashStrStructKey (const void *info, const void *data);
	OBJC_EXPORT int isEqualStrStructKey (const void *info, const void *data1, const void *data2);
	OBJC_EXPORT int isEqualPrototype (const void *info, const void *data1, const void *data2);
	OBJC_EXPORT uarith_t hashPrototype (const void *info, const void *data);

    // utilities 
    #if defined(__osf__)
        #define _objc_strhash(S) NXStrHash((void*)0,S)
    #else
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
    #endif

    OBJC_EXPORT const char *__S(_nameForHeader) (const headerType*);

    /* initialize */
    OBJC_EXPORT void __S(_sel_init) (const headerType*, const char *, unsigned int, void *);
    void __S(_sel_initsorted) (const headerType *header,
                           void *backrefs,
                           void *relativeTo,
                           unsigned int sectionSize);
    OBJC_EXPORT void _sel_resolve_conflicts(headerType * header, unsigned long slide);
    OBJC_EXPORT void __S(_sel_unloadSelectors)(const char *, const char *);
    OBJC_EXPORT void _class_install_relationships(Class, long);
    OBJC_EXPORT void _objc_add_category(Category, int);
    OBJC_EXPORT void _objc_remove_category(Category, int);
    OBJC_EXPORT void _class_removeProtocols(Class, struct objc_protocol_list *);
    OBJC_EXPORT NXZone *_objc_create_zone(void);
    OBJC_EXPORT void _objc_removeClass(Class);

    /* method lookup */
    OBJC_EXPORT BOOL class_respondsToMethod(Class, SEL);
    OBJC_EXPORT IMP class_lookupMethod(Class, SEL);
    OBJC_EXPORT IMP class_lookupMethodInMethodList(struct objc_method_list *mlist, 
                                          SEL sel);
    OBJC_EXPORT void _objc_insertMethods( struct objc_method_list *mlist, struct objc_method_list ***list );
    OBJC_EXPORT void _objc_removeMethods( struct objc_method_list *mlist, struct objc_method_list ***list );

    /* message dispatcher */
    OBJC_EXPORT Cache _cache_create(Class);
    OBJC_EXPORT IMP _class_lookupMethodAndLoadCache(Class, SEL);
    OBJC_EXPORT id _objc_msgForward (id self, SEL sel, ...);

    /* errors */
    OBJC_EXPORT volatile void __S(_objc_fatal)(const char *message);
    OBJC_EXPORT volatile void _objc_error(id, const char *, va_list);
    OBJC_EXPORT volatile void __objc_error(id, const char *, ...);
    OBJC_EXPORT void _objc_inform(const char *fmt, ...);
    OBJC_EXPORT void _NXLogError(const char *format, ...);

    /* magic */
    OBJC_EXPORT Class _objc_getFreedObjectClass (void);
    OBJC_EXPORT const struct objc_cache emptyCache;
    OBJC_EXPORT void _objc_flush_caches (Class cls);
    
    /* locking */
    #if defined(NeXT_PDO)
        #if defined(WIN32)
            #define MUTEX_TYPE long
            #define OBJC_DECLARE_LOCK(MUTEX) MUTEX_TYPE MUTEX = 0L;
        #elif defined(sparc)
            #define MUTEX_TYPE long
            #define OBJC_DECLARE_LOCK(MUTEX) MUTEX_TYPE MUTEX = 0L;
        #elif defined(__alpha__)
            #define MUTEX_TYPE long
            #define OBJC_DECLARE_LOCK(MUTEX) MUTEX_TYPE MUTEX = 0L;
        #elif defined(hpux)
            typedef struct { int a; int b; int c; int d; } __mutex_struct;
            #define MUTEX_TYPE __mutex_struct
            #define OBJC_DECLARE_LOCK(MUTEX) MUTEX_TYPE MUTEX = { 1, 1, 1, 1 };
        #else // unknown pdo platform
            #define MUTEX_TYPE long
            #define OBJC_DECLARE_LOCK(MUTEX) struct mutex MUTEX = { 0, #MUTEX };
        #endif // WIN32
        OBJC_EXPORT MUTEX_TYPE classLock;
        OBJC_EXPORT MUTEX_TYPE messageLock;
    #else // not PDO
        #define MUTEX_TYPE long
        #define OBJC_DECLARE_LOCK(MUTEX) struct mutex MUTEX = { 0, #MUTEX };
        OBJC_EXPORT struct mutex classLock;
        OBJC_EXPORT struct mutex messageLock;
    #endif // NeXT_PDO

    OBJC_EXPORT int _objc_multithread_mask;

    // _objc_msgNil is actually (unsigned dummy, id, SEL) for i386;
    // currently not implemented for any sparc or hppa platforms
    OBJC_EXPORT void (*_objc_msgNil)(id, SEL);

    typedef struct {
       long addressOffset;
       long selectorOffset;
    } FixupEntry;

    static inline int selEqual( SEL s1, SEL s2 ) {
       OBJC_EXPORT BOOL rocketLaunchingDebug;
       if ( rocketLaunchingDebug )
          checkUniqueness( s1, s2 );
       return (s1 == s2);
    }

    #if defined(KERNEL)
        #if defined(hppa)  /* The sense of kernel locks on the hp are backwards! */
            #define OBJC_DECLARE_LOCK(MUTEX) struct slock MUTEX = { -1,-1,-1,-1 };
        #else 
            #define OBJC_DECLARE_LOCK(MUTEX) struct slock MUTEX = { 0 };
        #endif

        #define OBJC_LOCK(MUTEX)
        #define OBJC_UNLOCK(MUTEX)
        #define OBJC_TRYLOCK(MUTEX)	1
        OBJC_EXPORT int preUniquedSelectors;

    #else /* KERNEL */
    #if defined(FREEZE)
        #define OBJC_LOCK(MUTEX)
        #define OBJC_UNLOCK(MUTEX)
    #else /* FREEZE */
        #if defined(OBJC_COLLECTING_CACHE)
            #define OBJC_LOCK(MUTEX) 	mutex_lock (MUTEX)
            #define OBJC_UNLOCK(MUTEX)	mutex_unlock (MUTEX)
            #define OBJC_TRYLOCK(MUTEX)	mutex_try_lock (MUTEX)
        #elif defined(NeXT_PDO)
            #if !defined(WIN32)
                /* Where are these defined?  NT should probably be using them! */
                OBJC_EXPORT void _objc_private_lock(MUTEX_TYPE*);
                OBJC_EXPORT void _objc_private_unlock(MUTEX_TYPE*);

                /* I don't think this should be commented out for NT, should it? */
                #define OBJC_LOCK(MUTEX)		\
                    do {if (!_objc_multithread_mask)	\
                    _objc_private_lock(MUTEX);} while(0)
                #define OBJC_UNLOCK(MUTEX)		\
                    do {if (!_objc_multithread_mask)	\
                    _objc_private_unlock(MUTEX);} while(0)
            #else
                #define OBJC_LOCK(MUTEX)		\
                    do {if (!_objc_multithread_mask)	\
                    if( *MUTEX == 0 ) *MUTEX = 1;} while(0)
                #define OBJC_UNLOCK(MUTEX)		\
                    do {if (!_objc_multithread_mask)	\
                    *MUTEX = 0;} while(0)
            #endif // WIN32

        #else // not NeXT_PDO
            #define OBJC_LOCK(MUTEX)			\
              do					\
                {					\
                  if (!_objc_multithread_mask)		\
            	mutex_lock (MUTEX);			\
                }					\
              while (0)

            #define OBJC_UNLOCK(MUTEX)			\
              do					\
                {					\
                  if (!_objc_multithread_mask)		\
            	mutex_unlock (MUTEX);			\
                }					\
              while (0)
        #endif /* OBJC_COLLECTING_CACHE */
    #endif /* FREEZE */
#endif /* KERNEL */

#if !defined(SEG_OBJC)
#define SEG_OBJC        "__OBJC"        /* objective-C runtime segment */
#endif

#if defined(NeXT_PDO)
    // GENERIC_OBJ_FILE
    void send_load_message_to_category(Category cat, void *header_addr); 
    void send_load_message_to_class(Class cls, void *header_addr);
#endif

#if !defined(__MACH__)
typedef struct _objcSectionStruct {
    void     **data;                   /* Pointer to array  */
    int      count;                    /* # of elements     */
    int      size;                     /* sizeof an element */
} objcSectionStruct;

typedef struct _objcModHeader {
    char *            name;
    objcSectionStruct Modules;
    objcSectionStruct Classes;
    objcSectionStruct Methods;
    objcSectionStruct Protocols;
    objcSectionStruct StringObjects;
} objcModHeader;
#endif

#endif /* _OBJC_PRIVATE_H_ */
