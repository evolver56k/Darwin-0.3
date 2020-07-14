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
** rootdev.c
** returns the name of the root device --
** /dev/sd0a (e.g) if hard disk (EXIT_SUCCESS)
** otherwise EXIT_FAILURE
** 
*/

#import <libc.h>
#import <stdio.h>
#import <stdlib.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/dir.h>

#define ROOT "/"
#define PRIVATE_DEV "/private/dev"
#define CWD "."

int main(int argc, char *argv[]) {
   struct stat statstr;
   DIR *devdir;
   struct direct *devp;
   dev_t rootdev;
   
   stat(ROOT, &statstr);
   rootdev = statstr.st_dev;
   chdir(PRIVATE_DEV);
   devdir = opendir(CWD);
   while ((devp = readdir(devdir)) != NULL) {
      stat(devp->d_name, &statstr);
      if(((statstr.st_mode & S_IFMT) == S_IFBLK) &&
	 (statstr.st_rdev == rootdev)) {
	 printf("/dev/%s", devp->d_name);
	 closedir(devdir);
	 exit(EXIT_SUCCESS);
      }
   }
   closedir(devdir);
   exit(EXIT_FAILURE);
}
