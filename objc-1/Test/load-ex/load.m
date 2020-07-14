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

#import "load.h"
#import <streams/streams.h>
#import <mach-o/loader.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <objc/objc-load.h>

static void load_module(Class cls, Category cat)
{
	if (cat) 
	  printf("load_module(): loading Category `%s' for Class `%s'\n",
			cat->category_name, cat->class_name);
	else
	  printf("load_module(): loading Class `%s'\n",[cls name]);
}

static void unload_module(Class cls, Category cat)
{
	if (cat)
	  printf("unload_module(): unloading Category `%s' for Class `%s'\n",
			cat->category_name, cat->class_name);
	else
	  printf("unload_module(): unloading Class `%s'\n",[cls name]);
}

static char **get_module_list()
{
        static char foo[256], tmpfoo[256];
	char *tmp, **mlist;
	int nmods = 0, i;

	fprintf(stdout,"Enter list of object modules to be loaded.\n");
	fprintf(stdout,"module list = ");
       	fgets(foo, 256, stdin);
	if (foo[strlen(foo)-1] == '\n')
		foo[strlen(foo)-1] = '\0';

	/* make a copy, strtok() is destructive */
	strcpy(tmpfoo, foo);

	tmp = strtok(foo, " ");
	while (tmp) {
		nmods++;
		tmp = strtok(0, " ");
	}
	mlist = malloc((nmods + 1) * sizeof(char *));

	tmp = strtok(tmpfoo, " ");
	i = 0;
	while (i < nmods) {
		mlist[i++] = tmp;
		tmp = strtok(0, " ");
	}
	mlist[i] = 0;
	return mlist;
}

static void handle_error(NXStream *stream)
{
	/* something went wrong */
	char *errorstring;
	int len, max;

	NXGetMemoryBuffer(stream, &errorstring, &len, &max);	    

	/* all error messages contain exactly 1 `\n' at the end */
	printf("%s", errorstring);
}

void main (void)
{
	while (1) {
		NXStream *stream;
		char **modlist;
		struct mach_header *mh;

		modlist = get_module_list();
		stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);

		if (strcmp(modlist[0], "unload") == 0) {
			if (objc_unloadModules(stream, unload_module))
				handle_error(stream);
		} else {
			if (objc_loadModules(modlist, stream, load_module, 
						&mh, "weeb")) 
		  		handle_error(stream);
		}
		NXCloseMemory(stream, NX_FREEBUFFER);
	}
}

