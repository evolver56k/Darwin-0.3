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
/*	NXBundle.m
	Copyright 1990, 1991, NeXT, Inc.
	IPC, November 1990
*/

#ifndef KERNEL

#if defined(WIN32)
    #include <winnt-pdo.h>
    #include <direct.h>
    #include <io.h>
    #include <errno.h>
#endif

#if !defined(NeXT_PDO)
    #import <sys/param.h>
    #import <unistd.h>
#else
    #if !defined(WIN32)
        #import <sys/param.h>
        #import <unistd.h>
    #endif 
    #if defined(hppa)
        #import <symlink.h>
    #endif 
#endif 

#import <objc/NXBundle.h>
#import "NXBundlePrivate.h"
#import <objc/maptable.h>
#import <objc/NXStringTable.h>
#import <objc/objc-load.h>
#import <objc/hashtable2.h>
#import "objc-private.h"

#if !defined(WIN32)
    #import <sys/file.h>

    #if defined(NeXT_PDO) && defined(__svr4__)
	#import <dirent.h>
    #else 
	#import <sys/dir.h>
    #endif
#endif
#import <sys/stat.h>
#import <sys/types.h>
#import <string.h>

#if defined(NeXT_PDO) && (defined(WIN32) || defined(__svr4__))
    #define rindex strrchr
#endif

static void warning(const char *format, ...) {}

/* For the LProj cache */
static id bundleTable = nil;
static id mainBundleStringTableTable = nil;

#define BUNDLE_SUFFIX	"bundle"

/*******	Bundle		********/

@implementation NXBundle

static NXMapTable *classToBundle = NULL;

static NXBundle *mainBundle = nil;
static NXMapTable *pathToBundle = NULL;

- initForDirectory:(const char *)bp {
#ifdef WINNT
    struct _stat statInfo;
#else /* WINNT */
    struct stat statInfo;
#endif /* WINNT */
    id	bb;
    
    if (! pathToBundle) pathToBundle = NXCreateMapTable(NXStrValueMapPrototype, 1);
    bb = NXMapGet(pathToBundle, bp);
    if (bb) {
	[self free];
	return bb;
    } else {
	BOOL allOK = YES;
	
	if (stat(bp, &statInfo) == 0) {
#ifndef WINNT
		/* Windows NT doesn't support filesystem links so there is no readlink function */
	    while (allOK && (statInfo.st_mode & S_IFLNK)) {
		char linkPath[MAXPATHLEN + 1];
		
		allOK = ((readlink(bp, linkPath, sizeof(linkPath)) != -1)
		         && (stat(linkPath, &statInfo) == 0));
	    }
#endif /* WINNT */
	    allOK = allOK
	            && ((statInfo.st_mode & (S_IFDIR | S_IREAD | S_IEXEC))
		        ? YES : NO);
	}
	if (allOK) {
	    _directory = NXCopyStringBuffer(bp);
	    _codeLoaded = NO;
	    NXMapInsert(pathToBundle, _directory, self);
	    return self;
	} else {
	    [self free];
	    return nil;
	}
    }
}

- free
{
    if (_codeLoaded)
	return self;
    else {
	NXMapRemove(pathToBundle, _directory);
	free(_directory);
	return [super free];
    }
}

+ mainBundle {
    if (! mainBundle) {
	char	prog[MAXPATHLEN];
	char	*last;
	char 	**argv;
#if !defined(NeXT_PDO)
        extern char ***_NSGetArgv();
        argv = *_NSGetArgv();
#else
        extern char **NXArgv;
        argv = NXArgv;
#endif

#if defined(NeXT_PDO)
        if (argv[0][0] != '/') { getcwd(prog, MAXPATHLEN); strcat(prog, "/"); }
#else
        if (argv[0][0] != '/') { getwd(prog); strcat(prog, "/"); }
#endif
	else prog[0] = 0;
	strcat(prog, argv[0]);
	last = rindex(prog, '/');
	last[0] = 0;
	mainBundle = [[self alloc] initForDirectory: prog];
	mainBundle->_codeLoaded = YES;
    }
    return mainBundle;
}

+ bundleForClass:class {
    id	bundle;
    if (! classToBundle) return [self mainBundle];
    bundle = NXMapGet(classToBundle, class);
    return (bundle) ? bundle : [self mainBundle];
}
    
- (const char *)directory {
    return _directory;
}

#ifndef NeXT_PDO
static Class globalPrincipalClass = NULL;
static id globalBundle = nil;

static void loadCallback(Class cls, Category cat ) {
    warning("Loading %s '%s'\n", (cat) ? "category of" : "class", cls->name);
    if (cat) return;
    if (! classToBundle) classToBundle = NXCreateMapTable(NXPtrValueMapPrototype, 1);
    NXMapInsert(classToBundle, cls, globalBundle);
    if (! globalPrincipalClass) globalPrincipalClass = cls;
}
#endif NeXT_PDO

static void* bundleErrorStream = 0;

+ (void*) setErrorStream:(void*)stream
{
  void* s = bundleErrorStream;
  bundleErrorStream = stream;
  return s;
}

- (BOOL)ensureLoaded {
    if (! _codeLoaded) {
#ifndef NeXT_PDO
	char 		name[MAXPATHLEN];
	char		path[MAXPATHLEN];
	char		*moduleList[2] = {path, NULL};
	NXStream	*errorStream;
	long		err;
	char		*slash, *dot;

	globalPrincipalClass = NULL;
	globalBundle = self;
	slash = rindex(_directory, '/');
	if (slash) {
	    strcpy(name, slash + 1);
	} else {
	    strcpy(name, _directory);
	}
	dot = rindex(name, '.');
	if (dot)
	    *dot = '\0';
	if ([self getPath: path forResource: name ofType: NULL]) {
	    errorStream = NXOpenMemory(NULL, 0, NX_WRITEONLY);

	    err = objc_loadModules(moduleList, errorStream, loadCallback,
				    NULL, NULL);
	    if (err) {
		char *streamBuf;
		int len, maxLen;
		
		NXGetMemoryBuffer(errorStream, &streamBuf, &len, &maxLen);
		if (!bundleErrorStream)
		  {
		    _NXLogError("Error loading %s", path);
		    if (len) {
		      streamBuf[len] = 0;
		      _NXLogError(streamBuf);
		    }	
		    _NXLogError("\n");
		  }
		else
		  {
		    NXPrintf (bundleErrorStream, "Error loading %s", path);
		    if (len) {
		      streamBuf[len] = 0;
		      NXPrintf (bundleErrorStream, streamBuf);
		    }
		    NXPrintf (bundleErrorStream, "\n");
		  }
	    } else {
		_principalClass = globalPrincipalClass;
		_codeLoaded = YES;
	    }
	    NXCloseMemory(errorStream, NX_FREEBUFFER|NX_TRUNCATEBUFFER);
	    if (err) return NO;
	} else {
	  if (! bundleErrorStream)
	    _NXLogError("Couldn't find %s in bundle for %s", name, _directory);
	  else
	    NXPrintf (bundleErrorStream,
		      "Couldn't find %s in bundle for %s", name, _directory);
	  return NO;
	}
#else
    return NO;
#endif
    }
    return YES;
}

- classNamed:(const char *)className {
    if (![self ensureLoaded]) return nil;
    return objc_getClass((char *)className); 
}

- principalClass {
    if (![self ensureLoaded]) return nil;
    return (id)_principalClass;
}

- setVersion:(int)version {
    _bundleVersion = version;
    return self;
}

- (int)version {
    return _bundleVersion;
}

/*******	International Routines		********/

static BOOL systemHasLanguages = YES;
static const char *const *systemLanguages = NULL;

static const char *const *getDefaultSystemLanguages(void) {
    int count, length;
    const char *language, *separator;
    char **defaultSystemLanguages = NULL;

    if (systemHasLanguages) {
	language = "English";
	if (language && *language) {
	    count = 1;
	    separator = strchr(language, ';');
	    while (separator) {
		count++;
		separator = strchr(separator+1, ';');
	    }
	    defaultSystemLanguages = malloc((count + 1) * sizeof(char *));
	    defaultSystemLanguages[count] = NULL;
	    for (count = 0; language; count++) {
		separator = strchr(language, ';');
		if (separator) {
		    length = separator - language;
		    separator++;
		} else {
		    length = strlen(language);
		}
		while (*language == ' ' || *language == '\t') {
		    language++; length--;
		}
		while (language[length-1] == ' ' || language[length-1] == '\t') length--;
		defaultSystemLanguages[count] = malloc((length + 1) * sizeof(char));
		strncpy(defaultSystemLanguages[count], language, length);
		defaultSystemLanguages[count][length] = '\0';
		language = separator;
	    }
	} else {
	    systemHasLanguages = NO;
	}
    }

    return (const char * const *)defaultSystemLanguages;
}

#ifdef DEBUG
static int ACCESS(const char *path, int way)
{
    int retval = access(path, way);
    _NXLogError("access(%s, R_OK) returns %d\n", path, retval);
    return retval;
}
#else
#define ACCESS access
#endif

#define LINE_BUFFER_SIZE 256

/* The passed-in path is the .lproj directory to look in for the version.  It's not clear what the best way to encode a version into an .lproj directory, but this is probably as good as any...it parses through a file named "version" (if found) looking
 for an integer value starting a line.  If it finds one, that's the version.  If no version file exists in the .lproj directory, then its version is 0. */
 
static int readLprojVersion(char *path) {
    FILE *file;
    int version = 0;
    int pathLength = strlen(path);

    strcat(path, "/version");
    file = fopen(path, "r");
    if (file) {
	char *eof;
	char lineBuffer[LINE_BUFFER_SIZE+1];
	*lineBuffer = '\0';
	do {
	    eof = fgets(lineBuffer, LINE_BUFFER_SIZE, file);
	    version = atoi(lineBuffer);
	} while (!eof && !version);

	fclose(file);
    }
    path[pathLength] = '\0';
    return version;
}

/* Adds the passed extension to the passed name iff the extension is not already on that name. */

static void addExtension(char *name, const char *extension)
{
    if (extension) {
	const char *existingExtension = strrchr(name, '.');
	if (existingExtension && extension[0] != '.') existingExtension++;
	if (!existingExtension || strcmp(existingExtension, extension)) {
	    if (extension[0] != '.' && extension[0]) strcat(name, ".");
	    strcat(name, extension);
	}
    }
}

/* This function takes a string and does a readdir() on the passed directory and adds the names of all the files in the directory to that string.  Returns a potentially newly realloc()'ed version of the string. */

static char *addDirectoryEntriesTo(char *files, const char *directory)
{
#ifndef WINNT
	/* Windows NT doesn't have readdir, this needs to be fixed for NT */
    DIR *dirp;
#if defined(NeXT_PDO) && (defined(__svr4__) || defined(__osf__))
    struct dirent *dp;
#else
    struct direct *dp;
#endif
    int length;

    length = strlen(files);
    if (dirp = opendir(directory)) {
	dp = readdir(dirp); /* Skip . */
	dp = readdir(dirp); /* Skip .. */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	    volatile char *tempFiles;
#if defined(NeXT_PDO) && defined(__svr4__)
	    int namelen = strlen(dp->d_name);
	    tempFiles = realloc(files, length + namelen + 2);
	    files = (char*)tempFiles;
	    strncpy(files + length, dp->d_name, namelen);
	    length += namelen;
#else
	    tempFiles = realloc(files, length + dp->d_namlen + 2);
	    files = (char*)tempFiles;
	    strncpy(files + length, dp->d_name, dp->d_namlen);
	    length += dp->d_namlen;
#endif
	    files[length++] = '/';
	}
	files[length] = '\0';
	closedir(dirp);
    }
#endif /* WINNT */

    return files;
}

/* There's a little bit of an efficiency hack in this function.  The NXMapTable lprojTable is a map table with keys and values both null-terminated strings.  The keys are the path up to and including the .lproj directory.  The value is of the form "ve
rsion/file1/file2/file3/file4".  atoi() is used to suck the version out of the front. */

static BOOL isFileInValidLproj(char *path, const char *name, const char *extension, int bundleVersion)
{
    char *lprojInfo, *slash;
    int length, version = -1;
    char buffer[MAXPATHLEN+1];
    static NXMapTable *lprojTable = NULL;

    if (!lprojTable) lprojTable = NXCreateMapTable(NXStrValueMapPrototype, 0);

    lprojInfo = NXMapGet(lprojTable, path);

    if (!lprojInfo) {
	if (ACCESS(path, R_OK) >= 0) version = readLprojVersion(path);
	sprintf(buffer, "%d/", version);
	lprojInfo = NXCopyStringBuffer(buffer);
	lprojInfo = addDirectoryEntriesTo(lprojInfo, path);
	NXMapInsert(lprojTable, NXCopyStringBuffer(path), lprojInfo);
    } else {
	version = atoi(lprojInfo);
    }

    if (version == bundleVersion && (lprojInfo = strchr(lprojInfo, '/'))) {
	strcpy(buffer, name);
        slash = buffer;
	while (*slash == '/') slash++;
	if ((slash = strchr(buffer, '/'))) {
	    *(slash+1) = '\0';
	} else {
	    addExtension(buffer, extension);
	    strcat(buffer, "/");
	}
	length = strlen(buffer);
	while (*++lprojInfo) {
	    if (!strncmp(lprojInfo, buffer, length)) {
		if (path[strlen(path)-1] != '/') strcat(path, "/");
		strcat(path, name);
		addExtension(path, extension);
		return slash ? (ACCESS(path, R_OK) >= 0) : YES;
	    }
	    lprojInfo = strchr(lprojInfo, '/');
	    if (!lprojInfo) return NO;	/* should never happen */
	}
    }

    return NO;
}

+ (BOOL)getPath:(char *)path forResource:(const char *)name
         ofType:(const char *)ext inDirectory: (const char *)bundlePath
    withVersion: (int)version
{
    const char		*const *cur;
    char		searchPath[MAXPATHLEN+1];

    if (!name) return NO;
    if (!path) path = searchPath;

    if (!systemLanguages) systemLanguages = getDefaultSystemLanguages();

    if (systemLanguages) {
	for (cur = systemLanguages; *cur; cur++) {
	    /* First look for a language in the bundle. */
	    if (bundlePath) {
		sprintf(path, "%s/%s.lproj", bundlePath, *cur);
		if (isFileInValidLproj(path, name, ext, version)) return YES;
                sprintf(path, "%s/Resources/%s.lproj", bundlePath, *cur);
                if (isFileInValidLproj(path, name, ext, version)) return YES;
	    }
	}
    }

    sprintf(path, "%s", bundlePath);
    if (isFileInValidLproj(path, name, ext, version)) return YES;
    sprintf(path, "%s/Resources", bundlePath);
    if (isFileInValidLproj(path, name, ext, version)) return YES;

    return NO;
}

-(BOOL)getPath: (char *)path forResource: (const char *)name
        ofType: (const char *)ext
{
    return [[self class] getPath: path forResource: name ofType: ext
                     inDirectory: _directory withVersion: _bundleVersion];
}

+ setSystemLanguages:(const char *const *)languageList {
    systemLanguages = languageList;
    if (!languageList) systemHasLanguages = YES;
    return self;
}

@end

const char *NXLoadLocalizedStringFromTableInBundle(const char *table, NXBundle *bundle, const char *key, const char *value)
{
    static NXZone *LocalizedStringZone = NULL;

    #define LSZONE (LocalizedStringZone ? LocalizedStringZone : (LocalizedStringZone = NXDefaultMallocZone()))

    const char *tableValue;
    id stringTable, stringTableTable;
    char path[MAXPATHLEN + 1];

    if (!key) return value ? value : "";
    if (!bundle) bundle = [NXBundle mainBundle];
    if (!table) table = "Localizable";

    if (bundle != [NXBundle mainBundle]) {
	stringTableTable = [bundleTable valueForKey:bundle];
    } else {
	stringTableTable = mainBundleStringTableTable;
    }

    if (!(stringTable = [stringTableTable valueForKey:table])) {
	[bundle getPath:path forResource:table ofType:"strings"];
	stringTable = [NXStringTable newFromFile: path];
	if (!stringTableTable) {
	    if (bundle != [NXBundle mainBundle]) {
		if (!bundleTable) bundleTable = [[HashTable allocFromZone:LSZONE] initKeyDesc:"!"];
		if (bundleTable) {
		    stringTableTable = [[HashTable allocFromZone:LSZONE] initKeyDesc:"*"];
		    if (stringTableTable) [bundleTable insertKey:bundle value:stringTableTable];
		}
	    } else {
		mainBundleStringTableTable = stringTableTable = [[HashTable allocFromZone:LSZONE] initKeyDesc:"*"];
	    }
	}
	if (stringTableTable && !stringTable) stringTable = [[NXStringTable allocFromZone:LSZONE] init];
	if (stringTable) [stringTableTable insertKey:table value:stringTable];
    }

    if (!(tableValue = [stringTable valueForStringKey:key])) {
	tableValue = value ? value : key;
	[stringTable insertKey:key value:(void *)tableValue];
    }

    return tableValue;
}
const char *NXLoadLocalStringFromTableInBundle(const char *table, NXBundle *bundle, const char *key, const char *value)
{
  return NXLoadLocalizedStringFromTableInBundle(table, bundle, key, value);
}
@implementation NXBundle (Compatability)

- initForPath: (const char *)path { return [self initForDirectory: path]; }
- (const char *)path { return [self directory]; }
+ forClass: class { return [self bundleForClass: class]; }
+ newFromPath:(const char *)bp { return [[self alloc] initForPath: bp]; }
- initFromPath:(const char *)bp { return [self initForPath: bp]; }
- (const char *)bundlePath { return [self path]; }
- (BOOL)codeLoaded { return _codeLoaded; }
- firstClass { return [self principalClass]; }
- (BOOL)getResourcePath:(char *)path forName:(const char *)name type:(const char *)ext { return [self getPath: path forResource: name ofType: ext]; }
- getClass: (const char *)className { return [self classNamed: className]; }
- classForName: (const char *)className { return [self classNamed: className]; }

@end

@implementation NXBundle (Private)

static void noFree(void *ptr) {}
static void freeObjects(void *ptr)
{
    [(HashTable *)ptr freeObjects];
    [(HashTable *)ptr free];
}

+ (void)_invalidateLprojCache
{
    if (bundleTable) {
	[bundleTable freeKeys: noFree values: freeObjects];
	[bundleTable free];
	bundleTable = nil;
    }
    if (mainBundleStringTableTable) {
	[mainBundleStringTableTable freeObjects];
	[mainBundleStringTableTable free];
	mainBundleStringTableTable = nil;
    }
}

@end
#endif
