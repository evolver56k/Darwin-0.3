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
/*     fastcp.c
       Original - Bertrand Jan 92
       Updated for use in app_makefiles - Mike Monegan Apr 92
       Ported to NT - Mike Monegan May 95
       Ported to Solaris & HPUX - Mike Monegan Jan 96
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <objc/hashtable.h>
#include <mach/mach.h>
#ifdef NeXT
#include <libc.h>
#include <sys/dir.h>
#include <bsd/errno.h>
typedef struct direct DIRENT;
#endif

#if defined(sun) || defined(hpux)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
typedef struct dirent DIRENT;

extern const char * sys_errlist[];                        // Sun & HP Bug
#ifdef sun
extern int utimes(const char *file, struct timeval *tvp); // Sun Bug
#endif
#endif

#define IS_DIRECTORY(st_mode)	(((st_mode) & S_IFMT) == S_IFDIR)
#define IS_LINK(st_mode)	(((st_mode) & S_IFMT) == S_IFLNK)
#define IS_REGULAR(st_mode)	(((st_mode) & S_IFMT) == S_IFREG)

#define PROGRAM_NAME "fastcp"

static void fastcp1(const char *source, const char *dest);
static void copy_destmissing(const char *source, struct stat *sstat, const char *dest);
static void blast(const char *dest);

extern void initializeCopyPublicity();
extern void finalizeCopyPublicity();
extern void publicizeCopy(const char* fileName, int tentative);
extern void commitTentativePublicity();
extern void abortTentativePublicity();


/* implementations */

static void check(int ret, const char *format, ...) {
    va_list	args;
    if (!ret) return;
    printf("%s: ", PROGRAM_NAME);
    va_start(args, format);
    vprintf(format, args);
#ifdef DEBUG
    printf(" : %s\n", sys_errlist[errno]);
#else
    printf("\n");
#endif
    exit(ret);
}

static void catdirfile(char *dirfile, const char *dir, const char *file) {
    strcpy(dirfile, dir); strcat(dirfile, "/"); strcat(dirfile, file);
}

static void checkdest(const char *dest) {
    struct stat	dstat;
    check(stat(dest, &dstat), "Destination '%s' does not exist.", dest);
    if (!IS_DIRECTORY(dstat.st_mode)) {
	printf("%s: Destination '%s' is not a directory\n", PROGRAM_NAME, dest);
	exit(-3);
    }
    check(access(dest, W_OK), "Destination '%s' is not writable directory.", dest);
}

static void settimes(const char *path, time_t mtime) {
#ifdef hpux
   struct utimbuf  times;
   times.actime = mtime;
   times.modtime = mtime;
   check(utime(path, &times), "Can't set time on '%s'.", path);
#else
    struct timeval  tv[2];
    tv[0].tv_sec = time(NULL);
    tv[1].tv_sec = mtime;
    tv[0].tv_usec = tv[1].tv_usec = 0;
    check(utimes(path, tv), "Can't set time on '%s'.", path);
#endif    
}

static void copy_dir_destmissing(const char *source, struct stat *sstat, const char *dest) {
    /* we know source is a directory */
    /* we know dest does not exist, but its parent is a directory */
    DIRENT	*dp;
    DIR		*dirp;
    check(mkdir(dest, 0777), "Can't create '%s'.", dest);
    dirp = opendir(source);
    check(!dirp, "Can't open directory '%s'.", source);
    dp = readdir(dirp); /* Skip . */
    dp = readdir(dirp); /* Skip .. */
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	char	subsource[MAXPATHLEN+1];
	char	subdest[MAXPATHLEN+1];
	struct stat	cstat;
	catdirfile(subsource, source, dp->d_name);
	catdirfile(subdest, dest, dp->d_name);
	check(stat(subsource, &cstat), "'%s' does not exist", subsource);
	check(access(subsource, R_OK), "Cannot read '%s'", subsource);
	copy_destmissing(subsource, &cstat, subdest);
    }
    closedir(dirp);
    settimes(dest, sstat->st_mtime);
    check(chmod(dest, sstat->st_mode), "Cannot protect '%s'.", dest);
}

static void copy_file_destmissing(const char *source, struct stat *sstat, const char *dest) {
    int		sfd;
    int		dfd;
    
#ifdef TRY_TO_LINK
    if (!link(source, dest)) return;
#endif
    (void)unlink(dest);
    sfd = open(source, O_RDONLY, 0666);
    if (sfd < 0) {
	printf("%s: cannot open '%s' : %s\n", 
	        PROGRAM_NAME, source, sys_errlist[errno]);
	exit(-4);
    }
    dfd = open(dest, O_TRUNC | O_CREAT | O_WRONLY, sstat->st_mode & 0777);
    if (dfd < 0) {
	printf("%s: cannot create '%s' : %s\n", 
		PROGRAM_NAME, dest, sys_errlist[errno]);
	exit(-5);
    }
    if (sstat->st_size) {
	vm_offset_t	addr = 0;
	check(map_fd(sfd, 0, &addr, 1, sstat->st_size), "cannot map file '%s'.", source);
	if (write(dfd, (char *)addr, sstat->st_size) != sstat->st_size) {
	    printf("%s: Error writing %s\n", PROGRAM_NAME, dest);
	    exit(-6);
	}
	vm_deallocate(task_self(), addr, sstat->st_size);
    }
    check(close(sfd), "Error closing %s", source);
    check(close(dfd), "Error closing %s", dest);
    settimes(dest, sstat->st_mtime);
}

static void copy_destmissing(const char *source, struct stat *sstat, const char *dest) {
    /* we know dest does not exist, but its parent is a directory */
    if (IS_LINK(sstat->st_mode)) {
	check(stat(source, sstat), "'%s' does not exist.", source);
    }
    commitTentativePublicity();
    if (IS_DIRECTORY(sstat->st_mode)) {
	return copy_dir_destmissing(source, sstat, dest);
    }
    return copy_file_destmissing(source, sstat, dest);
}

static void blast_stated(const char *dest, struct stat *dstat) {
    if (IS_DIRECTORY(dstat->st_mode)) {
       DIRENT	*dp;
	DIR	*dirp;
	chmod(dest, 0777); /* We chmod dest in order to blast it */
	dirp = opendir(dest);
	check(!dirp, "Can't open directory '%s'.", dest);
	dp = readdir(dirp); /* Skip . */
	dp = readdir(dirp); /* Skip .. */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	    char	subdest[MAXPATHLEN+1];
	    catdirfile(subdest, dest, dp->d_name);
	    blast(subdest);
	}
	closedir(dirp);
	check(rmdir(dest), "Cannot remove directory '%s'.", dest);
    } else {
	check(unlink(dest), "Cannot unlink '%s'.", dest);
    }
}

static void blast(const char *dest) {
    struct stat	dstat;
    /* we save the time of a stat by just going for it */
    if (!unlink(dest)) return;
    check(stat(dest, &dstat), "'%s' does not exist.", dest);
    blast_stated(dest, &dstat);
}

static void merge_dir_dir(const char *source, struct stat *sstat, const char *dest, struct stat *dstat) {
    /* we first put all names into a hashtable */
    DIRENT	*dp;
    DIR		*dirp;
    char	*names = malloc(sstat->st_size); /* this will be big enough for all names */
    char	*name = names;
    NXHashTable		*table = NXCreateHashTable(NXStrPrototype, 0, NULL);
    NXHashState	state;
    struct stat	cstat;

    dirp = opendir(source);
    check(!dirp, "Can't open directory '%s'.", source);
    dp = readdir(dirp); /* Skip . */
    dp = readdir(dirp); /* Skip .. */
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	strcpy(name, dp->d_name);
	NXHashInsert(table, name);
#ifdef __svr4__
        name += strlen(dp->d_name) + 1;
#else        
        name += dp->d_namlen + 1;
#endif        
    }
    closedir(dirp);
    chmod(dest, 0777); /* We chmod dest in order to fill it */
    dirp = opendir(dest);
    check(!dirp, "Can't open directory '%s'.", dest);
    dp = readdir(dirp); /* Skip . */
    dp = readdir(dirp); /* Skip .. */
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	char	subsource[MAXPATHLEN+1];
	char	subdest[MAXPATHLEN+1];
	catdirfile(subdest, dest, dp->d_name);
	if (NXHashGet(table, dp->d_name)) {
	    /* recurse! */
	    catdirfile(subsource, source, dp->d_name);
	    fastcp1(subsource, subdest);

/*	    check(stat(subsource, &cstat), "Cannot access '%s'", subsource);
	    check(access(subsource, R_OK), "Cannot access '%s'", subsource);
	    copy_destmissing(subsource, &cstat, subdest);
*/
	    NXHashRemove(table, dp->d_name); /* done ! */
	} else {
	    /* blast */
	    blast(subdest);
	}
    }
    closedir(dirp);
    /* in table only the ones that are new now */
    state = NXInitHashState(table);
    while (NXNextHashState(table, &state, (void **)&name)) {
	char	subsource[MAXPATHLEN+1];
	char	subdest[MAXPATHLEN+1];
	catdirfile(subdest, dest, name);
	catdirfile(subsource, source, name);
	check(stat(subsource, &cstat), "'%s' does not exist.", subsource);
	check(access(subsource, R_OK), "'%s' does not exist.", subsource);
	copy_destmissing(subsource, &cstat, subdest);
    }
    NXFreeHashTable(table);
    free(names);
    settimes(dest, sstat->st_mtime);
    check(chmod(dest, sstat->st_mode), "Cannot protect '%s'.", dest);
}

static void fastcp1(const char *source, const char *dest) {
    struct stat	sstat;
    struct stat	dstat;
    check(stat(source, &sstat), "'%s' does not exist.", source);
    check(access(source, R_OK), "'%s' does not exist.", source);
    if (lstat(dest, &dstat)) {  /* Note that we want to copy over a link */
	/* it's ok if dest does not exist */
        publicizeCopy(source,FALSE);
    	return copy_destmissing(source, &sstat, dest);
    }
    if (IS_DIRECTORY(sstat.st_mode) && IS_DIRECTORY(dstat.st_mode)) {
        publicizeCopy(source,TRUE);
    	return merge_dir_dir(source, &sstat, dest, &dstat);
    }
    if (IS_REGULAR(sstat.st_mode) && IS_REGULAR(dstat.st_mode) && (sstat.st_mtime == dstat.st_mtime) && (sstat.st_size == dstat.st_size)) {
	/* same date, same size, regular files, just trust */
	if (sstat.st_mode != dstat.st_mode) {
	    check(chmod(dest, sstat.st_mode), "Cannot protect '%s'.", dest);
	}
	return;
    }

    publicizeCopy(source,FALSE);
    /* links or special files */
    blast_stated(dest, &dstat);
    copy_destmissing(source, &sstat, dest);
}

void main(int argc, char *argv[]) {
    unsigned	index = 1;
    char	wd[MAXPATHLEN+1];
    char	dest[MAXPATHLEN+1];

    if (argc < 2) {
	printf("usage: %s <source>1 ... <source>n <dest>\n", PROGRAM_NAME);
	printf("(recursively copies source files (or directories) to dest directory,\n");
#ifdef TRY_TO_LINK
	printf("when source and dest are on the same device, hard links are created\n");
#endif
	printf("but does nothing when type, time, and size of a file match.)\n");
	exit(-1);
    }
#if defined(sun) || defined(hpux)
    if (!getcwd(wd, MAXPATHLEN+1)) {
#else
    if (!getwd(wd)) {
#endif       
	printf("%s: Can't find wd.\n", PROGRAM_NAME);
	exit(-2);
    }
    if (argv[argc-1][0] == '/') {
	strcpy(dest, argv[argc-1]);
    } else {
	catdirfile(dest, wd, argv[argc-1]);
    }
    checkdest(dest);
    initializeCopyPublicity();
    while (index < argc - 1) {
	char	source[MAXPATHLEN+1];
	char	dd[MAXPATHLEN+1];
	char	*file;
	if (argv[index][0] == '/') {
	    strcpy(source, argv[index]);
	} else {
	    catdirfile(source, wd, argv[index]);
	}
	file = strrchr(source, '/');
	catdirfile(dd, dest, file);
	fastcp1(source, dd);
	abortTentativePublicity();/* Don't let tentative publicity persist */
	index++;
    }
    finalizeCopyPublicity();
    exit(0);
}
