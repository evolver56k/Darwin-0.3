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
	HashTable.m
  	Copyright 1988-1996 NeXT Software, Inc.
	Written by Bertrand Serlet, Dec 88
	Responsibility: Bertrand Serlet
*/

#if defined(WIN32)
    #include <winnt-pdo.h>
#endif

#import <stdlib.h>
#import <stdio.h>
#import <string.h>

#if defined(NeXT_PDO)
    #import <pdo.h>
#endif

#import <objc/objc.h>
#import <objc/HashTable.h>
#import <objc/NXStringTable.h>
#import "objc-private.h"

typedef struct {const void *key; void *value;} Pair;
typedef	Pair	*Pairs;
typedef struct	{unsigned count; Pairs pairs;} Bucket;
typedef	Bucket	*Buckets;

#define INITCAPA	1	/* _nbBuckets has to be a 2**N-1, and at least 1 */
#define PAIRSSIZE(count) ((count) * sizeof(Pair))
#define HASHSTR(str) (_objc_strhash (str))
#define HASHINT(x) ((((uarith_t) x) >> ARITH_SHIFT) ^ ((uarith_t) x))

static uarith_t log2 (uarith_t x) { return (x<2) ? 0 : log2 (x>>1)+1; };

static uarith_t exp2m1 (uarith_t x) { return (1 << x) - 1; };

static uarith_t hash (const char *keyDesc, const void *aKey, uarith_t mod) {
    switch (keyDesc[0]) {
    	case '@': return [(id) aKey hash] % mod;
	case '%':
	case '*': return aKey ? HASHSTR (aKey) % mod : 0;
	default: return HASHINT (aKey) % mod;
    }
};  
    
static unsigned isEqual (const char *keyDesc, const void *key1, const void *key2) {
    if (key1 == key2) return YES;
    switch (keyDesc[0]) {
    	case '@': return [(id) key1 isEqual: (id) key2];
	case '%':
	case '*':
		if (! key1) return (strlen (key2) == 0);
		if (! key2) return (strlen (key1) == 0);
		if (((char *) key1)[0] != ((char *) key2)[0]) return NO;
		return (strcmp (key1, key2) == 0);
	default: return NO;
	};
    };
    
static void freeObject (void *item) {
    [(id) item free];
    };
    
static void noFree (void *item) {
    };
    
@implementation  HashTable

+ initialize
{
    [self setVersion: 1];
    return self;
}

- _initBare: (const char *) aKeyDesc : (const char *) aValueDesc : (unsigned) capacity
/* used for efficient implementation of rehashing: does create a semi ok table */
{
    count = 0; _nbBuckets = capacity;
    keyDesc = (aKeyDesc) ? aKeyDesc : "@"; 
    valueDesc = (aValueDesc) ? aValueDesc : "@";
    _buckets = nil;
    return self;
}

+ _newBare: (const char *) aKeyDesc : (const char *) aValueDesc : (unsigned) capacity
/* used for efficient implementation of rehashing: does create a semi ok table */
{
    return [[self allocFromZone: NXDefaultMallocZone()] 
	        _initBare:aKeyDesc:aValueDesc:capacity];
}

- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc
{
    return [self initKeyDesc: aKeyDesc valueDesc: aValueDesc capacity: INITCAPA];
}

- initKeyDesc: (const char *) aKeyDesc
{
    return [self initKeyDesc: aKeyDesc valueDesc: NULL];
}

- init
{
    return [self initKeyDesc: NULL];
}
- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc capacity: (unsigned) aCapacity
{
    [self _initBare: aKeyDesc: aValueDesc: exp2m1 (log2 (aCapacity)+1)];
    _buckets = NXZoneCalloc ([self zone], _nbBuckets, sizeof(Bucket));
    return self;
}

+ newKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc capacity: (unsigned) aCapacity
{
    return [[self allocFromZone: NXDefaultMallocZone()]
	initKeyDesc:aKeyDesc valueDesc:aValueDesc capacity:aCapacity];
}

+ newKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc
{
    return [self newKeyDesc: aKeyDesc valueDesc: aValueDesc capacity: INITCAPA];
}

+ newKeyDesc: (const char *) aKeyDesc
{
    return [self newKeyDesc: aKeyDesc valueDesc: NULL];
}

+ new
{
    return [self newKeyDesc: NULL];
}

- free {
    [self freeKeys: noFree values: noFree];
    free(_buckets);
    return [super free];
    }

- freeObjects {
    return [self 
	freeKeys: (keyDesc[0] == '@') ? freeObject : noFree 
	values: (valueDesc[0] == '@') ? freeObject : noFree 
	];
    }

- freeKeys: (void (*) (void *)) keyFunc values: (void (*) (void *)) valueFunc {
    unsigned	i = _nbBuckets;
    Buckets	buckets = (Buckets) _buckets;
    while (i--) {
	if (buckets->count) {
	    unsigned	j = buckets->count;
	    Pairs	pairs = buckets->pairs;
	    while (j--) {
		(*keyFunc) ((void *) pairs->key);
		(*valueFunc) (pairs->value);
		pairs++;
		};
	    free(buckets->pairs);
	    buckets->count = 0;
	    buckets->pairs = (Pairs) nil;
	    };
	buckets++;
	};
    count = 0;
    return self;
    };
    
- empty
{
    unsigned	i = _nbBuckets;
    Buckets	buckets = (Buckets) _buckets;
    while (i--) {
	if (buckets->count) free(buckets->pairs);
	buckets->count = 0;
	buckets->pairs = NULL;
	buckets++;
	};
    count = 0;
    return self;
}


- copyFromZone:(NXZone *)zone
/* we enumerate the table instead of blind copy to create a hash table of the right size */
{
    HashTable	*new = [[[self class] allocFromZone: zone] 
	                                   initKeyDesc: keyDesc
							 		    valueDesc: valueDesc
									     capacity: count];
    NXHashState	state = [self initState];
    const void	*key;
    void	*value;
    while ([self nextState: &state key: &key value: &value])
    	[new insertKey: key value: value];
    return new;
}

- (unsigned) count
{
    return count;
}

- (BOOL) isKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs;
    if (j == 0) return NO;
    pairs = bucket.pairs;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) return YES; 
	pairs++;
	};
    return NO;
}

- (void *) valueForKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs;
    if (j == 0) return nil;
    pairs = bucket.pairs;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) return pairs->value; 
	pairs++;
	};
    return nil;
}

- (void *) _insertKeyNoRehash: (const void *) aKey value: (void *) aValue
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs = bucket.pairs;
    Pairs	new;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) {
		void	*old = pairs->value;
		pairs->key = aKey;
		pairs->value = aValue;
		return old;
		};
	pairs++;
	};
    /* we enlarge this bucket; this could be optimized by using realloc */
    new = (Pairs) NXZoneMalloc ([self zone], PAIRSSIZE(bucket.count+1));
    if (bucket.count) bcopy ((const char*)bucket.pairs, (char*)(new+1), PAIRSSIZE(bucket.count));
    new->key = aKey; new->value = aValue;
    if (bucket.count) free (bucket.pairs);
    buckets[index].count++; buckets[index].pairs = new; count++; 
    return nil;
}

- (void *) insertKey: (const void *) aKey value: (void *) aValue
{
    void	*result = [self _insertKeyNoRehash: aKey value: aValue];
    if (result) return result; /* it was a replace */
    if (count > _nbBuckets) {
    	/* Rehash: we create a pseudo table pointing really to the old guys,
	extend self, copy the old pairs, and free the pseudo table */
	HashTable	* old = [[HashTable allocFromZone: [self zone]]
	                     _initBare: keyDesc: valueDesc: _nbBuckets];
        NXHashState	state;
        const void	*key;
	void		*value;
	
	old->count = count; old->_buckets = _buckets;
	_nbBuckets += _nbBuckets + 1;
	count = 0;
	_buckets = NXZoneCalloc ([self zone], _nbBuckets, sizeof(Bucket));
	state = [old initState];
	while ([old nextState: &state key: &key value: &value])
	    [self insertKey: key value: value];
#if !defined(KERNEL)
	if (old->count != count)
	    _NXLogError("*** HashTable: count differs after rehashing; probably indicates a broken invariant: there are x and y such as isEqual(x, y) is TRUE but hash(x) != hash (y)\n");
#endif
	[old free];
	};
    return nil;
}

- (void *) removeKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs = bucket.pairs;
    Pairs	new;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) {
		/* we shrink this bucket; this could be optimized by using realloc */
		void	*oldValue = pairs->value;
		new = (bucket.count-1) ? (Pairs) NXZoneMalloc (
		    [self zone], PAIRSSIZE(bucket.count-1)) : 0;
		if (bucket.count-1 != j)
			bcopy ((const char*)bucket.pairs, (char*)new, PAIRSSIZE(bucket.count-j-1));
		if (j)
			bcopy ((const char*)(bucket.pairs+bucket.count-j), (char*)(new+bucket.count-j-1),PAIRSSIZE (j));
	        free (bucket.pairs); 
		count--; buckets[index].count--; buckets[index].pairs = new;
		return oldValue;
		};
	pairs++;
	};
    return nil;
}

- (NXHashState) initState {
    NXHashState	state;
    state.i = _nbBuckets;
    state.j = 0;
    return state;
    };
    
- (BOOL) nextState: (NXHashState *) aState key: (const void **) aKey value: (void **) aValue
{
    Buckets	buckets = (Buckets) _buckets;
    Pair	pair;
    while (aState->j == 0) {
	if (aState->i == 0)
	  {
	    *aKey = NULL; *aValue = NULL;
	    return NO;
	  }
	aState->i--; aState->j = buckets[aState->i].count;
	}
    aState->j--;
    pair = buckets[aState->i].pairs[aState->j];
    *aKey = pair.key; *aValue = pair.value;
    return YES;
}

- (void) printForDebugger:(void *)stream
{
}

/* Obsolete (for binary compatibility only). */

- _debugPrint: (void *) stream
{
  [self printForDebugger: stream];
  return self;
}

@end
