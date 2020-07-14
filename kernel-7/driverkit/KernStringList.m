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
 * A List object for C-strings.
 *
 * Its sole purpose is to make whitespace-separated lists of words
 * easier to deal with, since we use them a lot in driverkit string tables.
 *
 *
 */

#import "KernStringList.h"

#define isspace(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n'))

static void *my_malloc(unsigned len)
{
#if KERNEL
    extern void *IOMalloc(int);
    return IOMalloc(len);
#else
    extern void *malloc(unsigned);
    void *ptr = malloc(len);
    char *p = (char *)ptr;
    
    while (len--)
	*p++ = 'X';
    return ptr;
#endif
}

static void my_free(void *ptr, unsigned len)
{
#if KERNEL
    extern void IOFree(void *, int);
    IOFree(ptr, len);
#else
    extern void free(void *);
    free(ptr);
#endif
}

@implementation KernStringList

- init
{
    return [self initWithWhitespaceDelimitedString:NULL];
}


- initWithWhitespaceDelimitedString:(const char *)str
{
    char *sp;
    int index;
    unsigned len;
    extern int strncpy(char *,const char *, int);
    
    [super init];

    while (*str && isspace(*str))
	str++;

    sp = (char *)str;
    while (*sp) {
	while (*sp && isspace(*sp))
	    sp++;
	if (*sp)
	    count++;
	while (*sp && !isspace(*sp))
	    sp++;
    }
    
    strings = (char **)my_malloc(sizeof(char *) * count);

    for (index = 0; str && *str; index++) {
	char *s;
	
	for (sp = (char *)str; *sp && !isspace(*sp); sp++)
	    continue;
	len = sp - str + 1;
	s = strings[index] = (char *)my_malloc(len);
	strncpy(s, str, len - 1);
	s[len - 1] = '\0';
	for (str = sp; *str && isspace(*str); str++)
	    continue;
    }
    return self;
}

- free
{
    int i;
    
    for (i=0; i<count; i++) {
	my_free((char *)strings[i], strlen(strings[i])+1);
    }
    my_free(strings, sizeof(char *) * count);
    return [super free];
}

- (unsigned)count
{
    return count;
}

- (const char *)stringAt:(unsigned)index
{
    if (index >= 0 && index < count)
	return strings[index];
    return NULL;
}

- (const char *)lastString
{
    return strings[count - 1];
}


@end
