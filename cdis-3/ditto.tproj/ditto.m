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
// 7-28-1997 pmb - Added autorelease pool

#import <libc.h>
#import <errno.h>
#import <IndexingKit/unixtree.h>
#import "ProgressWriter.h"

static int vflag = 0;
static int Vflag = 0;
static int bflag = 0;
static int tflag = 0;
static int Pflag = 0;

static const char usage[] =
    "Usage: ditto [-vV] [-arch <archName> ...] [-lang <langName> ...] [-bomThin] [-bom <bom>] [-outBom <outBom>] [-inodeMap <inodeMap>] <src> [<src>...] <dst>\n"
    "    The -v (verbose) option prints a line of output as each src is copied.\n"
    "    The -V (very verbose) option prints a line of output for every file copied.\n"
    "    Architecture(s) for thinning can be specified by using the -arch option.\n"
    "    An archName should be one of m68k, i386, hppa, etc.\n"
    "    Languages(s) can be specified by using the -lang option.\n"
    "    A langName should be one of English, French, German, etc.\n"
    "    The -bomThin option uses the fatness of objects in the bom to control thinning.\n"
    "    [The -bomThin option is disabled if any -arch options are given or if no bom.]\n"
    "    If a bom is specified then only objects contained in bom are copied.\n"
    "    If an inodeMap is specified then copied files and symlinks are logged in the map.\n"
    "    The srcs and dst must exist and be directories or symlinks to directories.\n";
    
#define MAX_ARCH_NAMES	16
#define MAX_LANG_NAMES	16

static char *
_GetTempFilename( char *name, int pos )
{
    struct timeval time;
    int num, firstNum;
    char numBuf[10];

    /* get 6 digits of reasonable randomness, seed independent */
    gettimeofday( &time, NULL );
    num = firstNum = ((time.tv_usec>>8) * 439821) % (1000*1000);
    do {
        sprintf( numBuf, "%06d", num );
        bcopy( numBuf, name+pos, 6 );
        if( access( name, F_OK ) == -1 )
            break;              /* we found a good one */
        if( --num < 0 )
            num = 999999;
    } while( num != firstNum );
    return name;
}

int
main(int argc, char* argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    char		*maskBomName = NULL;
    char		*outBomName = NULL;
    char		*inodeMapName = NULL;
    char		*archNames[MAX_ARCH_NAMES];
    char		*langNames[MAX_LANG_NAMES];
    char		*variantNames[MAX_ARCH_NAMES + MAX_LANG_NAMES];
    int			numArchs = 0;
    int			numLangs = 0;
    int			numVariants = 0;
    InodeMap		*inodeMap = NULL;
    UnixBTree		*maskBom = NULL;
    UnixBTree		*outBom = NULL;
    UnixBTree		*copyBom = NULL;
    UnixCopier		*copier = nil;
    ProgressWriter	*writer;
    struct stat		srcStat;
    char		srcPath[MAXPATHLEN];
    char		origPath[MAXPATHLEN];
    char		outBomNameBuf[MAXPATHLEN];
    char		deleteOutBom = NO;
    // @@@@ fix for broken mmap
    char		noOutBomHack = NO;
    int			exitVal = 0;
    int			i, j;
    
    // bail if not root
    if (geteuid()) {
	fprintf(stderr, "You must be logged in as root to run ditto.\n");
	exit(1);
    }

    // bail if not enough arguments
    if (argc < 3) {
	fprintf(stderr, usage);
	exit(1);
    }
    
    // collect options
    for (argc--, argv++; *argv && **argv == '-'; argc--, argv++) {
        if ((*argv)[1] == 'T') {
	    tflag++;
        } else if ((*argv)[1] == 'P') {
            Pflag++;
        } else if ((*argv)[1] == 'v') {
            vflag++;
        } else if ((*argv)[1] == 'V') {
	    vflag++;
	    Vflag++;
	} else if (!strcmp(*argv + 1, "arch")) {
	    argc--, argv++;
	    if (!*argv) {
		fprintf(stderr, usage);
		exit(1);
	    } else {
		if (numArchs == MAX_ARCH_NAMES) {
		    fprintf(stderr, "ERROR - maximum number (%d) of archs exceeded\n", MAX_ARCH_NAMES);
		    exit(1);
		}
	        archNames[numArchs++] = *argv;
	        variantNames[numVariants++] = *argv;
	    }
	} else if (!strcmp(*argv + 1, "lang")) {
	    argc--, argv++;
	    if (!*argv) {
		fprintf(stderr, usage);
		exit(1);
	    } else {
		if (numLangs == MAX_LANG_NAMES) {
		    fprintf(stderr, "ERROR - maximum number (%d) of languages exceeded\n", MAX_LANG_NAMES);
		    exit(1);
		}
	        langNames[numLangs++] = *argv;
	        variantNames[numVariants++] = *argv;
	    }
	} else if (!strcmp(*argv + 1, "bom")) {
	    argc--, argv++;
	    if (!*argv || maskBomName) {
		fprintf(stderr, usage);
		exit(1);
	    }
	    maskBomName = *argv;
	} else if (!strcmp(*argv + 1, "outBom")) {
	    argc--, argv++;
	    if (!*argv || outBomName) {
		fprintf(stderr, usage);
		exit(1);
	    }
	    outBomName = *argv;
	} else if (!strcmp(*argv + 1, "inodeMap")) {
	    argc--, argv++;
	    if (!*argv || inodeMapName) {
		fprintf(stderr, usage);
		exit(1);
	    }
	    inodeMapName = *argv;
        } else if (!strcmp(*argv + 1, "bomThin")) {
            bflag++;
        // @@@@ fix for broken mmap
        } else if (!strcmp(*argv + 1, "noOutBomHack")) {
            noOutBomHack = YES;
            fprintf(stderr, "**** WARNING **** Input bom will not be stripped into a thin bom\n");
	} else {
	    fprintf(stderr, "unrecognized flag %s\n", *argv);
	    exit(1);
	}
    }
    archNames[numArchs] = NULL;
    langNames[numLangs] = NULL;
    variantNames[numVariants] = NULL;
    
    // out of arguments?
    if (argc < 2) {
	fprintf(stderr, usage);
	exit(1);
    }
    
    // open mask bom
    if (maskBomName) {
	if (stat(maskBomName, &srcStat) == -1) {
	    perror(maskBomName);
	    exit(1);
	}
	maskBom = [[UnixBTree alloc] initFromFile:maskBomName forWriting:NO];
	if (!maskBom) {
	    fprintf(stderr, "**** ERROR **** Can't open bom %s\n", maskBomName);
	    exit(1);
	} else if (![maskBom rootObject]) {
		fprintf(stderr, "**** WARNING **** bom %s contains no files, no files copied\n", maskBomName);
                [pool release];
		exit(0);
	}
	copyBom = maskBom;
    }

    // @@@@ fix for broken mmap "!noOutBom"
    // open out bom
    if (!noOutBomHack && maskBom && (outBomName || numVariants)) {
        if (!outBomName) {
	    strcpy(outBomNameBuf, "/private/tmp/ditto.XXXXXX.bom");
	    _GetTempFilename(outBomNameBuf, 19);
	    outBomName = outBomNameBuf;
	    deleteOutBom = YES;
	}
        (void) unlink(outBomName);
	if (vflag) {
	    printf("%sCreating stripped mask bom ... ", Vflag ? ">>> " : "");
	}
	outBom = [[UnixBTree alloc]
		    initWithFile:outBomName
		    fromBom:maskBom
		    keepArchs:(numArchs) ? archNames : NULL
		    keepLangs:(numLangs) ? langNames : NULL];
	if (!outBom) {
	    fprintf(stderr, "Can't open bom %s\n", outBomName);
	    exit(1);
	} else if (![outBom rootObject]) {
	    fprintf(stderr, "**** WARNING **** bom %s contains no files, no files copied\n", outBomName);
            [pool release];
	    exit(0);
	}
	if (vflag) {
	    printf("done\n");
	}
	copyBom = outBom;
    }
    
    // open inode map
    if (inodeMapName) {
	if (stat(inodeMapName, &srcStat) == -1) {
	    perror(inodeMapName);
	    exit(1);
	}
	inodeMap = [[InodeMap alloc] initFromFile:inodeMapName forWriting:YES];
	if (!inodeMap) {
	    fprintf(stderr, "**** ERROR **** Can't open inode map %s\n", inodeMapName);
            [pool release];
	    exit(0);
	}
    }
    
    // squirrel away current path
    if (!getwd(origPath)) {
        fprintf(stderr, "%s\n", origPath);
	exit(1);
    }
    
    writer = [[ProgressWriter alloc] initWithBom:copyBom useThermometer:tflag usefbshow:Pflag];
    copier = [[UnixCopier alloc]
		initWithBom:copyBom
		archNames:(copyBom) ? NULL : archNames
		delegate:writer
		inodeMap:inodeMap];
    if (!copier) {
	fprintf(stderr, "Can't create UnixCopier object\n");
	exit(1);
    }
            
    for (i = 0; i < argc-1; i++ ) {
    
	if (stat(argv[i], &srcStat) == -1) {
	    if (errno == ENOENT) {
	        fprintf(stderr, "\n**** WARNING **** %s not found\n\n", argv[i]);
		continue;
	    } else {
	        fprintf(stderr, "\n**** ERROR **** ");
		perror(argv[i]);
		fprintf(stderr, "\n");
		continue;
	    }
	} else if ((srcStat.st_mode & S_IFMT) != S_IFDIR) {
	    fprintf(stderr, "\n**** ERROR **** %s is not a directory\n\n", argv[i]);
	    continue;
	} else {
	    if (chdir(argv[i]) < 0) {
		perror(argv[i]);
		exit(1);
	    }
	    if (!getwd(srcPath)) {
		fprintf(stderr, "%s\n", srcPath);
		exit(1);
	    }
	    if (chdir(origPath) < 0) {
		perror(origPath);
		exit(1);
	    }
	}
    
	if (vflag) {
	    printf("%s%sCopying %s ", (Vflag && i) ? "\n" : "", (Vflag) ? ">>> " : "", argv[i]);
	    if (numVariants == 1) {
	        printf("[%s]", variantNames[0]);
	    } else if (numVariants > 1) {
	        printf("[%s", variantNames[0]);
		for (j = 1; j < numVariants; j++) {
		    printf(",%s", variantNames[j]);
		}
	        printf("]");
	    }
	    printf("\n");
	    fflush(stdout);
	}
	
	if (![copier copyFromDir:srcPath
	                   toDir:argv[argc-1]
	              filesInBom:copyBom ? YES : NO
	            thinUsingBom:((bflag || numArchs) && copyBom) ? YES : NO
	          thinUsingArchs:(numArchs && !copyBom) ? YES : NO
	           sendStartMsgs:(Vflag || tflag) ? YES : NO
	          sendFinishMsgs:(Vflag || tflag || Pflag) ? YES : NO
		  updateInodeMap:inodeMap ? YES : NO]) {
	    exitVal++;
	}
	
    }
    
    [writer release];
    if (outBom) {
//	[outBom release];
//	[outBom dealloc];
        [outBom free];
        if (deleteOutBom) {
	    (void) unlink(outBomName);
	}
    }
    
    if (inodeMap) {
        [inodeMap flush];
	[inodeMap free];
    }
		
    [pool release];
    exit(exitVal);
}
