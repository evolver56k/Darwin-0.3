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
/*	NXBundle.m
	Copyright 1990, 1991, NeXT, Inc.
	IPC, November 1990
*/

#ifndef KERNEL
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB


#import "NXBundle.h"
#import "NXBundlePrivate.h"
#import "maptable.h"
#import "NXStringTable.h"
#import "objc-load.h"
#import "hashtable.h"
#import "objc-private.h"

#import <sys/file.h>
#import <sys/dir.h>
#import <sys/stat.h>
#import <sys/types.h>

#import <strings.h>

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
    struct stat statInfo;
    id	bb;
    
    if (! pathToBundle) pathToBundle = NXCreateMapTable(NXStrValueMapPrototype, 1);
    bb = NXMapGet(pathToBundle, bp);
    if (bb) {
	[self free];
	return bb;
    } else {
	BOOL allOK = YES;
	
	if (stat(bp, &statInfo) == 0) {
	    while (allOK && (statInfo.st_mode & S_IFLNK)) {
		char linkPath[MAXPATHLEN + 1];
		
		allOK = ((readlink(bp, linkPath, sizeof(linkPath)) != -1)
		         && (stat(linkPath, &statInfo) == 0));
	    }
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
#if defined(__DYNAMIC__)
        extern char ***_NSGetArgv();
        argv = *_NSGetArgv();
#else
		extern char **NXArgv;
        argv = NXArgv;
#endif
        if (argv[0][0] != '/') { getwd(prog); strcat(prog, "/"); }
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

static Class globalPrincipalClass = NULL;
static id globalBundle = nil;

static void loadCallback(Class cls, Category cat ) {
    warning("Loading %s '%s'\n", (cat) ? "category of" : "class", cls->name);
    if (cat) return;
    if (! classToBundle) classToBundle = NXCreateMapTable(NXPtrValueMapPrototype, 1);
    NXMapInsert(classToBundle, cls, globalBundle);
    if (! globalPrincipalClass) globalPrincipalClass = cls;
}

static NXStream* bundleErrorStream = 0;

+ (NXStream*) setErrorStream:(NXStream*)stream
{
  NXStream* s = bundleErrorStream;
  bundleErrorStream = stream;
  return s;
}

- (BOOL)ensureLoaded {
    if (! _codeLoaded) {
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

    return defaultSystemLanguages;
}

#ifdef DEBUG
static int ACCESS(const char *path, int way)
{
    int retval = access(path, way);
    printf("access(%s, R_OK) returns %d\n", path, retval);
    return retval;
}
#else
#define ACCESS access
#endif

#define LINE_BUFFER_SIZE 256

/* The passed-in path is the .lproj directory to look in for the version.  It's not clear what the best way to encode a version into an .lproj directory, but this is probably as good as any...it parses through a file named "version" (if found) looking for an integer value starting a line.  If it finds one, that's the version.  If no version file exists in the .lproj directory, then its version is 0. */
 
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
    }
    path[pathLength] = '\0';
    fclose(file);

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
    DIR *dirp;
    struct direct *dp;
    int length;

    length = strlen(files);
    if ((dirp = opendir(directory))) {
	dp = readdir(dirp); /* Skip . */
	dp = readdir(dirp); /* Skip .. */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	    files = realloc(files, length + dp->d_namlen + 2);
	    strncpy(files + length, dp->d_name, dp->d_namlen);
	    length += dp->d_namlen;
	    files[length++] = '/';
	}
	files[length] = '\0';
	closedir(dirp);
    }

    return files;
}

/* There's a little bit of an efficiency hack in this function.  The NXMapTable lprojTable is a map table with keys and values both null-terminated strings.  The keys are the path up to and including the .lproj directory.  The value is of the form "version/file1/file2/file3/file4".  atoi() is used to suck the version out of the front. */

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

/* Returns a path to the appropriate shadow .lproj in /NextLibrary/Languages. */

static BOOL getPathToNextLibraryLanguages(char *path, const char *realPath, const char *language)
{
    const char *s, *t;
    char scratch[ MAXPATHLEN+1 ];

    s = strchr(realPath, '.');
    while (s) {
	if (!strncmp(s, ".app", 4)) {
	    char *p;
	    strcpy(scratch, realPath);
	    p = scratch + (s - realPath);
	    t = p+4;
	    if (*t == '/') t++;
	    *p = '\0';
	    s = strrchr(scratch, '/');
	    if (s) {
		if (*t) {
		    sprintf(path, "/NextLibrary/Languages/%s%s.lproj/%s", language, s, t);
		} else {
		    sprintf(path, "/NextLibrary/Languages/%s%s.lproj", language, s);
		}
	    }
	    return YES;
	} else {
	    s = strchr(s+1, '.');
	}
    }
    if (!s && !strncmp(realPath, "/usr/lib/", 9)) {
	s = realPath+8;
	while (*s == '/') s++;
	if (*s) {
	    s = strchr(s, '/');
	    if (s) {
		char *tmp;
		
		strcpy(scratch, realPath);
		tmp = scratch + (s - realPath);
		*tmp++ = '\0';
		s = tmp;
	    }
	    if (s && *s) {
		sprintf(path, "/NextLibrary/Languages/%s%s.lproj/%s", language, scratch+8, s);
	    } else {
		sprintf(path, "/NextLibrary/Languages/%s%s.lproj", language, realPath+8);
	    }
	    return YES;
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
	    }
	    /* Then look in /NextLibrary/Languages. */
	    if (getPathToNextLibraryLanguages(path, bundlePath, *cur)) {
		if (isFileInValidLproj(path, name, ext, version)) return YES;
	    }
	}
    }

    sprintf(path, "%s", bundlePath);
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
