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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * Kernel level implementation of IOConfigTable.
 *
 * HISTORY
 * 29-Jan-93    Doug Mitchell at NeXT
 *      Created.
 */

#import <driverkit/IOConfigTable.h>
#import <driverkit/configTableKern.h>
#import <driverkit/configTablePrivate.h>
#import <driverkit/generalFuncs.h>
#if i386
#import <machdep/i386/kernBootStruct.h>
#endif i386
#import <string.h>

/*
 * This really should be static, but it has a prototype in <ansi/string.h>.
 */
char *strstr(const char *s1, const char *s2);

/*
 * The _private ivar is a char *. 
 */
@implementation IOConfigTable

- free
{
	char *configData = (char *)_private;
	
	if(configData) {
		IOFree(configData, strlen(configData)+1);
	}
	return [super free];
}

/*
 * Obtain the system-wide configuration table.
 */
+ newFromSystemConfig
{
#if i386
	KERNBOOTSTRUCT *bootstruct = KERNSTRUCT_ADDR;
	return [self newForConfigData: &bootstruct->config[0]];
#else i386
	/* 
	 * FIXME - where is KERNBOOTSTRUCT?
	 */
	return nil;
#endif i386
}

/*
 * Obtain value for specified string key. Returns null of key not
 * found.
 * The string here must eventually be freed (by the caller) via
 * IOFree(buf, strlen(buf) + 1). This is kinda bogus...
 */
- (const char *)valueForStringKey:(const char *)key
{
	const char *configData = (char *)_private;
	const char *valueEnd;
	const char *valueStart;
	char *out;
	int length;
	
	length = strlen(key);
	{
	    char	quotedkey[length + 3];
	    
	    quotedkey[0] = '"';
	    strcpy(&quotedkey[1], key);
	    quotedkey[length + 1] = '"';
	    quotedkey[length + 2] = 0;
	    
	    valueStart = strstr(configData, quotedkey);
	    if (valueStart == NULL)
	    	return NULL;
		
	    valueStart += (length + 2);	// point past the quoted key
	}
	
	/*
	 * ValueStart points just past the quoted key
	 */
	valueStart = strchr(valueStart, '"');
	valueStart++;
	
	/*
	 * valueStart points to the first character of the desired value.
	 */
	valueEnd = strchr(valueStart, '"');
	if (valueEnd != NULL) {
		length = valueEnd - valueStart;
		out = IOMalloc(length + 1);
		strncpy(out, valueStart, length);
		out[length] = '\0';
		return out;
	}
	else {
		return NULL;
	}
}


+ (void)freeString : (const char *)string
{
	IOFree((char *)string, strlen(string) + 1);
}

- (void)freeString : (const char *)string
{
	[IOConfigTable freeString:string];
}	


@end

@implementation IOConfigTable(KernelPrivate)

/*
 * Create a new instance for specified IOConfigData text. 
 */
+ newForConfigData : (const char *)configData
{
	IOConfigTable *configTable = [[self alloc] init];
	char *data;
	int ssize = strlen(configData) + 1;
	
	if(ssize > IO_CONFIG_DATA_SIZE) {
		ssize = IO_CONFIG_DATA_SIZE;
	}
	data = IOMalloc(ssize);
	bcopy(configData, data, ssize-1);
	data[ssize-1] = 0;
	configTable->_private = data;
	return configTable;
}

@end

/* 
 * Like strchr, but searches for a string. Returns pointer to start of 
 * the found string, else returns NULL.
 */
char *strstr(const char *s1, const char *s2) {
	char c1;
  	const char c2 = *s2;

	while ((c1 = *s1++) != '\0') {
		if (c1 == c2) {
			const char *p1, *p2;

			p1 = s1;
			p2 = &s2[1];
			while (*p1++ == (c1 = *p2++) && c1) {
				continue;
			}
			if (c1 == '\0') {
				return ((char *)s1) - 1;
			}
	     }
      }
      return NULL;
}
