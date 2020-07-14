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
/*
 Copyright (c) 1987-99 Apple Computer, Inc.
 All Rights Reserved.

 About hfs.util.m:
 Contains code to implement hfs utility used by the WorkSpace to mount HFS.

 To do:
 look for "PPD" for unresolved issues
 look for "XXX" for unresolved issues

 Change History:
     5-Jan-1999 Don Brady       Write hfs.label names in UTF-8.
    10-Dec-1998 Pat Dirks       Changed to try built-in hfs filesystem first.
     3-Sep-1998	Don Brady	Disable the daylight savings time stuff.
    28-Aug-1998 chw		Fixed parse args and verify args to indicate that the
			        flags (fixed or removable) are required in the probe case.
    22-Jun-1998	Pat Dirks	Changed HFSToUFSStr table to switch ":" and "/".
    13-Jan-1998 jwc 		first cut (derived from old NextStep macfs.util code and cdrom.util code).
 */


/* ************************************** I N C L U D E S ***************************************** */

#import <kernserv/loadable_fs.h>
#import <servers/netname.h>
#import <sys/types.h>
#import <sys/wait.h>
#import <bsd/dev/disk.h>
#import <bsd/fcntl.h>
#import <bsd/sys/errno.h>

#import <mach/mach_init.h> // name_server_port

#import "kl_com.h"
#import "hfs_KS.h"

#import "MacOSTypes.h"
#import "HFSVolumes.h"
#import "HFSBtreesPriv.h"

#import <bsd/stdio.h> // printf
#import <bsd/unistd.h> //
#import <bsd/string.h> // strcat
#import <bsd/stdlib.h> // malloc, system
#import <bsd/sys/stat.h> //
#import <bsd/sys/time.h> // gettimeofday
#import <bsd/sys/mount.h> // struct statfs

/* **************************************** L O C A L S ******************************************* */

#define DEVICE_PREFIX 				"/dev/"
#define DEVICE_SUFFIX				"_hfs_a"
#define	HFS_BLOCK_SIZE				512

#define HFS_FS_NAME					"hfs"
#define HFS_FS_NAME_NAME			"HFS"

#define	MOUNT_COMMAND				"/sbin/mount"
#define	UMOUNT_COMMAND				"/sbin/umount"

#define HFS_MOUNT_TYPE				"hfs"
#define HFS_LOADABLE_FILE_NAME		"hfs_fs_reloc"
#define HFS_LOADABLE_SERVER_NAME	"hfs_fs"

#define VOLFS_PATH					"/usr/filesystems/vol.fs"
#define VOLFS_MOUNT_TYPE			"volfs"
#define VOLFS_LOADABLE_FILE_NAME	"volfs_reloc"
#define VOLFS_LOADABLE_SERVER_NAME	"volfs"
#define VOLFS_MOUNT_COMMAND			"/sbin/mount_volfs"
#define VOLFS_MOUNT_POINT			"/.vol"

/* BEWARE OCTAL CONSTANT */
#define VOLFS_MOUNT_POINT_PERMS		(0755)

typedef unsigned long	UCS4;
typedef unsigned short	UTF16;
typedef unsigned char	UTF8;

typedef enum {
	ok, 				/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted		/* insuff. room in target for conversion */
} ConversionResult;

/* ************************************ G L O B A L S *************************************** */


/* XXX - bek - 5/28/98 - Remove this after Atlas1F.  It should be in kernserv/loadable_fs.h. */
#ifndef FSUR_MOUNT_HIDDEN
#define FSUR_MOUNT_HIDDEN (-9)
#endif

boolean_t	wait_for_kl = FALSE;


/* ************************************ P R O T O T Y P E S *************************************** */

static void		DoDisplayUsage( const char * argv[] );
static void		DoFileSystemFile( char * theFileNameSuffixPtr, char * theContentsPtr );
static int		DoMount( char * theDeviceNamePtr, const char * theMountPointPtr, boolean_t isLocked );
static int 		DoProbe( char * theDeviceNamePtr );
static int 		DoUnmount( const char * theMountPointPtr );
static int		ParseArgs( int argc, const char * argv[], const char ** actionPtr, const char ** mountPointPtr, boolean_t * isEjectablePtr, boolean_t * isLockedPtr );

static int GetEmbeddedHFSPlusVol(HFSMasterDirectoryBlock * hfsMasterDirectoryBlockPtr, off_t * startOffsetPtr);
static int GetNameFromHFSPlusVolumeStartingAt(int fd, off_t hfsPlusVolumeOffset, char * name_o);
static int GetBTreeNodeInfo(int fd, off_t btreeOffset, UInt32 *nodeSize, UInt32 *firstLeafNode);
static off_t CalcFirstLeafNodeOffset(off_t fileOffset, UInt32 blockSize, UInt32 extentCount,
									 HFSPlusExtentDescriptor *extentList);
static int GetCatalogOverflowExtents(int fd, off_t hfsPlusVolumeOffset, HFSPlusVolumeHeader *volHdrPtr,
									 HFSPlusExtentDescriptor **catalogExtents, UInt32 *catalogExtCount);



static ssize_t readAt( int fd, void * buf, off_t offset, ssize_t length );
static int PrepareHFS( char * deviceNamePtr, const char * mountPointPtr );
static int PrepareVolFS( void );
static int AttemptMount(char *deviceNamePtr, const char *mountPointPtr, boolean_t isLocked);

static void ConvertMacRomanToUnicode (ConstStr255Param pascalString, int *unicodeChars,
									  UniChar* unicodeString);

static void ConvertUnicodeToUTF8 (int srcCount, const UniChar* srcStr, int dstMaxBytes,
					 			  UTF8* dstStr);

static ConversionResult	ConvertUTF16toUTF8 (UTF16** sourceStart, const UTF16* sourceEnd,
											UTF8** targetStart, const UTF8* targetEnd);

/* ******************************************** main ************************************************
Purpose -
This our main entry point to this utility.  We get called by the WorkSpace.  See DoVerifyArgs
for detail info on input arguments.
Input -
argc - the number of arguments in argv.
argv - array of arguments.
Output -
returns FSUR_IO_SUCCESS if OK else one of the other FSUR_xyz errors in loadable_fs.h.
*************************************************************************************************** */

int main (int argc, const char *argv[])
{
    const char			*	actionPtr = NULL;
    const char			*	mountPointPtr = NULL;
    int						result = FSUR_IO_SUCCESS;
    char					deviceName[256];
    boolean_t				isLocked = 0, isEjectable = 0;	/* reasonable assumptions */

#if DEBUG
    {
		int		i;

		printf("\nhfs.util - entering with argc of %d \n", argc );

		for ( i = 0; i < argc; i++ )
		{
			printf("argv[%d]: '%s'\n", i, argv[i] );
		}
	}
#endif // DEBUG

    /* Verify our arguments */
    if ( (result = ParseArgs( argc, argv, & actionPtr, & mountPointPtr, & isEjectable, & isLocked )) != 0 ) {
        goto AllDone;
    }

    /*
    -- Build our device name (full path), should end up with something like:
    -- "/dev/fd0_hfs_a" or "/dev/hd0_hfs_a" or "/dev/sd2_hfs_a"
    */

    // Assume it is of the form "sd3_hfs_a" already.
    strcpy( & deviceName[0], DEVICE_PREFIX );	// "/dev"
    strcat( & deviceName[0], argv[2] );			// "sd2" or "hd0" or "fd0"
                                                // or: "sd2_hfs_a" or "hd0_hfs_a" or "fd0_hfs_a"
/* XXX - bek - 4/28/98 - bek - This can be removed when the Workspace stops passing "sd2" and passes "sd3_hfs_a" instead. */
#if 1
    if ( strlen( argv[2] ) <= 4 )
    {
        strcat( & deviceName[0], DEVICE_SUFFIX );	// "_hfs_a"
    }
#endif
    
#if DEBUG
    printf("actionPtr = '%s', mountPointPtr = '%s', isEjectable = %d, isLocked = %d\n",
           actionPtr ? actionPtr : "(NULL)",
           mountPointPtr ? mountPointPtr : "(NULL)",
           isEjectable, isLocked );
    printf("deviceName = '%s'\n", deviceName);
#endif

    /* call the appropriate routine to handle the given action argument after becoming root */

    result = seteuid( 0 );
    if ( result ) {
#if DEBUG
        printf("hfs.util: ERROR: seteuid(0): %s\n",strerror(errno));
#endif
        result = FSUR_INVAL;
        goto AllDone;
    }

    result = setegid( 0 );	// PPD - is this necessary?

#if DEBUG
    if ( result ) {
        printf("hfs.util: ERROR: setegid: %s\n",strerror(errno));
    }
#endif

#if DEBUG
    printf("Entering the switch with action = %s\n",actionPtr);
#endif
    switch( * actionPtr )
      {
        case FSUC_PROBE:
            result = DoProbe( & deviceName[0] );
            break;

        case FSUC_MOUNT:
        case FSUC_MOUNT_FORCE:
            result = DoMount( & deviceName[0], mountPointPtr, isLocked );
            break;

        case FSUC_UNMOUNT:
            result = DoUnmount( mountPointPtr );
            break;

        default:
            /* should never get here since DoVerifyArgs should handle this situation */
            DoDisplayUsage( argv );
            result = FSUR_INVAL;
            break;
      }

AllDone:
        if ( wait_for_kl ) {
            kl_com_wait();
        }

#if DEBUG
    printf("hfs.util: EXIT: %d = ", result );
    switch (result)
      {
        case FSUR_LOADERR:
            printf("FSUR_LOADERR\n");
            break;
        case FSUR_INVAL:
            printf("FSUR_INVAL\n");
            break;
        case FSUR_IO_SUCCESS:
            printf("FSUR_IO_SUCCESS\n");
            break;
        case FSUR_IO_FAIL:
            printf("FSUR_IO_FAIL\n");
            break;
        case FSUR_RECOGNIZED:
            printf("FSUR_RECOGNIZED\n");
            break;
        case FSUR_MOUNT_HIDDEN:
            printf("FSUR_MOUNT_HIDDEN\n");
            break;
        case FSUR_UNRECOGNIZED:
            printf("FSUR_UNRECOGNIZED\n");
            break;
        default:
            printf("default\n");
            break;
      }
#endif //

    exit(result);

    return result;      // ...and make main fit the ANSI spec.
}



/* ******************************************* DoMount **********************************************
Purpose -
This routine will fire off a system command to mount the given device at the given mountpoint.
NOTE - Workspace will make sure the mountpoint exists and will remove it at Unmount time.
Input -
deviceNamePtr - pointer to the device name (full path, like /dev/sd2_hfs_a).
mountPointPtr - pointer to the mount point.
isLocked - a flag
Output -
returns FSUR_IO_SUCCESS everything is cool else one of several other FSUR_xyz error codes.
*************************************************************************************************** */

static int DoMount( char * deviceNamePtr, const char * mountPointPtr, boolean_t isLocked )
{
    int		result = FSUR_IO_FAIL;

    dprintf(("DoMount('%s','%s')\n",deviceNamePtr,mountPointPtr));

    if ( mountPointPtr == NULL || *mountPointPtr == '\0' ) {
        result = FSUR_IO_FAIL;
        goto Return;
    }
    
    /* Make sure the volfs server is loaded and mounted. */
    
    result = PrepareVolFS();
    if ( result != FSUR_IO_SUCCESS ) {
    	goto Return;
    }
    
    result = AttemptMount(deviceNamePtr, mountPointPtr, isLocked);
    if(result) {
      /* Make sure the HFS filesystem is loaded. */

        result = PrepareHFS(deviceNamePtr, mountPointPtr);
      if (result != FSUR_IO_SUCCESS ) {
	goto Return;
      };
      
      result = AttemptMount(deviceNamePtr, mountPointPtr, isLocked);
   }
    else {
      result = FSUR_IO_SUCCESS;
    }

Return:
        return result;

} /* DoMount */


/* ****************************************** DoUnmount *********************************************
Purpose -
    This routine will fire off a system command to unmount the given device.
Input -
    theDeviceNamePtr - pointer to the device name (full path, like /dev/sd2_hfs_a).
Output -
    returns FSUR_IO_SUCCESS everything is cool else FSUR_IO_FAIL.
*************************************************************************************************** */

static int DoUnmount( const char * theMountPointPtr )
{
    int		result;
    int		mountflags = 0; /* for future stuff */

    result = unmount(theMountPointPtr, mountflags);

    if ( result != 0 ) {
        result = FSUR_IO_FAIL;
        dprintf(("hfs.util: ERROR: DoUnmount('%s') returned %d\n", theMountPointPtr, result));
    } else {
        result = FSUR_IO_SUCCESS;
    }

    return result;

} /* DoUnmount */


/* ******************************************* DoProbe **********************************************
Purpose -
    This routine will open the given raw device and check to make sure there is media that looks
    like an HFS.
Input -
    theDeviceNamePtr - pointer to the device name (full path, like /dev/sd2_hfs_a).
Output -
    returns FSUR_MOUNT_HIDDEN (previously FSUR_RECOGNIZED) if we can handle the media else one of the FSUR_xyz error codes.
*************************************************************************************************** */

static int DoProbe( char *deviceNamePtr )
{
    int					result = FSUR_UNRECOGNIZED;

    int 				fd = 0;

    char 			*	bufPtr;
    HFSMasterDirectoryBlock 	*	mdbPtr;
    HFSPlusVolumeHeader		*	volHdrPtr;

	int charCount;
	u_char volnameUTF8[NAME_MAX];
	UniChar volnameUnicode[64];

    dprintf(("ENTER: DoProbe('%s')\n",deviceNamePtr));

    bufPtr = (char *)malloc(HFS_BLOCK_SIZE);
    if ( ! bufPtr ) {
        printf("hfs.util: ERROR: malloc failed\n");
        result = FSUR_UNRECOGNIZED;
        goto Return;
    }

    mdbPtr = (HFSMasterDirectoryBlock *) bufPtr;
    volHdrPtr = (HFSPlusVolumeHeader *) bufPtr;

    fd = open( deviceNamePtr, O_RDONLY | O_NDELAY, 0 );
    if( fd <= 0 ) {
        dprintf(("hfs.util: ERROR: DoProbe: open('%s'): %s\n",deviceNamePtr,strerror(errno)));
        result = FSUR_IO_FAIL;
        goto Return;
    }

    /* Read the HFS Master Directory Block from block 2. */

    result = readAt( fd, bufPtr, (off_t)(2 * HFS_BLOCK_SIZE), HFS_BLOCK_SIZE );
    if ( FSUR_IO_FAIL == result )
      {
        goto Return;
      }

	/* get the volume name from the MDB (HFS) or Catalog (HFS Plus) */

    if (mdbPtr->drSigWord == kHFSSigWord  &&  mdbPtr->drEmbedSigWord != kHFSPlusSigWord) {

		/* HFS volume, so use name in MDB */
		ConvertMacRomanToUnicode(mdbPtr->drVN, &charCount, volnameUnicode);
		ConvertUnicodeToUTF8(charCount, volnameUnicode, sizeof(volnameUTF8), volnameUTF8);
 
   } else if (volHdrPtr->signature == kHFSPlusSigWord  ||
		(mdbPtr->drSigWord == kHFSSigWord  &&  mdbPtr->drEmbedSigWord == kHFSPlusSigWord)) {
		off_t startOffset;

		if (volHdrPtr->signature == kHFSPlusSigWord) {
			startOffset = 0;
		} else {/* embedded volume, first find offset */
          	result = GetEmbeddedHFSPlusVol(mdbPtr, &startOffset);
          	if ( result != FSUR_IO_SUCCESS )
				goto Return;
		}

        result = GetNameFromHFSPlusVolumeStartingAt( fd, startOffset, volnameUTF8 );

	} else {
	/* This is not one that we recognize so return an FSUR_UNRECOGNIZED error */
        result = FSUR_UNRECOGNIZED;
	}

	if (FSUR_IO_SUCCESS == result) {
		DoFileSystemFile( FS_NAME_SUFFIX, HFS_FS_NAME_NAME );
		DoFileSystemFile( FS_LABEL_SUFFIX, volnameUTF8 );
		result = FSUR_MOUNT_HIDDEN;
	}

Return:

        if ( bufPtr ) {
            free( bufPtr );
        }

    if ( fd > 0 ) {
        close( fd );
    }

#if DEBUG
    printf("DoProbe: returns ");
    switch (result)
      {
        case FSUR_IO_FAIL:
            printf("FSUR_IO_FAIL\n");
            break;
        case FSUR_RECOGNIZED:
            printf("FSUR_RECOGNIZED\n");
            break;
        case FSUR_MOUNT_HIDDEN:
            printf("FSUR_MOUNT_HIDDEN\n");
            break;
        case FSUR_UNRECOGNIZED:
            printf("FSUR_UNRECOGNIZED\n");
            break;
        default:
            printf("default\n");
            break;
      }
#endif

    return result;

} /* DoProbe */


/* **************************************** DoVerifyArgs ********************************************
Purpose -
    This routine will make sure the arguments passed in to us are cool.
    Here is how this utility is used:

usage: hfs.util actionArg deviceArg [mountPointArg] [flagsArg]
actionArg:
        -p (Probe for mounting)
       	-P (Probe for initializing - not supported)
       	-m (Mount)
        -r (Repair - not supported)
        -u (Unmount)
        -M (Force Mount)
        -i (Initialize - not supported)

deviceArg:
        sd2 (for example)

mountPointArg:
        /foo/bar/ (required for Mount and Force Mount actions)

flagsArg:
        (these are ignored for CDROMs)
        either "readonly" OR "writable"
        either "removable" OR "fixed"

examples:
	hfs.util -p sd2 removable writable
	hfs.util -p sd2 removable readonly
	hfs.util -m sd2 /my/hfs

Input -
    argc - the number of arguments in argv.
    argv - array of arguments.
Output -
    returns FSUR_INVAL if we find a bad argument else 0.
*************************************************************************************************** */

static int ParseArgs( int argc, const char *argv[], const char ** actionPtr, const char ** mountPointPtr, boolean_t * isEjectablePtr, boolean_t * isLockedPtr )
{
    int			result = FSUR_INVAL;
    int			deviceLength;
    int			index;

    /* Must have at least 3 arguments and the action argument must start with a '-' */
    if ( (argc < 3) || (argv[1][0] != '-') ) {
        DoDisplayUsage( argv );
        goto Return;
    }

    /* we only support actions Probe, Mount, Force Mount, and Unmount */

    * actionPtr = & argv[1][1];

    switch ( argv[1][1] )
      {
        case FSUC_PROBE:
        /* action Probe and requires 5 arguments (need the flags) */
	     if ( argc < 5 ) {
             	DoDisplayUsage( argv );
             	goto Return;
	     } else {
            	index = 3;
	     }
            break;
        case FSUC_UNMOUNT:
            * mountPointPtr = argv[3];
            index = 0; /* No isEjectable/isLocked flags for unmount. */
            break;
        case FSUC_MOUNT:
        case FSUC_MOUNT_FORCE:
            /* action Mount and ForceMount require 6 arguments (need the mountpoint and the flags) */
            if ( argc < 6 ) {
                DoDisplayUsage( argv );
                goto Return;
            } else {
                * mountPointPtr = argv[3];
                index = 4;
            }
            break;
        default:
            DoDisplayUsage( argv );
            goto Return;
            break;
      }

    /* Make sure device (argv[2]) is something reasonable (we expect something like "sd2", maybe "sd2_hfs_a" in a perfect universe) */
    deviceLength = strlen( argv[2] );
    if ( deviceLength < 3 || deviceLength > 10 ) {
        DoDisplayUsage( argv );
        goto Return;
    }

    if ( index ) {
        /* Flags: removable/fixed. */
        if ( 0 == strcmp(argv[index],"removable") ) {
            * isEjectablePtr = 1;
        } else if ( 0 == strcmp(argv[index],"fixed") ) {
            * isEjectablePtr = 0;
        } else {
            printf("hfs.util: ERROR: unrecognized flag (removable/fixed) argv[%d]='%s'\n",index,argv[index]);
        }

        /* Flags: readonly/writable. */
        if ( 0 == strcmp(argv[index+1],"readonly") ) {
            * isLockedPtr = 1;
        } else if ( 0 == strcmp(argv[index+1],"writable") ) {
            * isLockedPtr = 0;
        } else {
            printf("hfs.util: ERROR: unrecognized flag (readonly/writable) argv[%d]='%s'\n",index,argv[index+1]);
        }
    }

    result = 0;

Return:
        return result;

} /* DoVerifyArgs */


/* *************************************** DoDisplayUsage ********************************************
Purpose -
    This routine will do a printf of the correct usage for this utility.
Input -
    argv - array of arguments.
Output -
    NA.
*************************************************************************************************** */

static void DoDisplayUsage( const char *argv[] )
{
    printf("usage: %s action_arg device_arg [mount_point_arg] [Flags] \n", argv[0]);
    printf("action_arg:\n");
    printf("       -%c (Probe for mounting)\n", FSUC_PROBE);
    printf("       -%c (Mount)\n", FSUC_MOUNT);
    printf("       -%c (Unmount)\n", FSUC_UNMOUNT);
    printf("       -%c (Force Mount)\n", FSUC_MOUNT_FORCE);
    printf("device_arg:\n");
    printf("       device we are acting upon (for example, 'sd2')\n");
    printf("mount_point_arg:\n");
    printf("       required for Mount and Force Mount \n");
    printf("Flags:\n");
    printf("       required for Mount, Force Mount and Probe\n");
    printf("       indicates removable or fixed (for example 'fixed')\n");
    printf("       indicates readonly or writable (for example 'readonly')\n");
    printf("Examples:\n");
    printf("       %s -p sd2 fixed writable\n", argv[0]);
    printf("       %s -m sd2 /my/hfs removable readonly\n", argv[0]);

    return;

} /* DoDisplayUsage */


/* ************************************** DoFileSystemFile *******************************************
Purpose -
    This routine will create a file system info file that is used by WorkSpace.  After creating the
    file it will write whatever theContentsPtr points to the new file.
    We end up with a file something like:
    /usr/filesystems/hfs.fs/hfs.name
    when our file system name is "hfs" and theFileNameSuffixPtr points to ".name"
Input -
    theFileNameSuffixPtr - pointer to a suffix we add to the file name we're creating.
    theContentsPtr - pointer to C string to write into the file.
Output -
    NA.
*************************************************************************************************** */

static void DoFileSystemFile( char *fileNameSuffixPtr, char *contentsPtr )
{
    int    		fd;
    char   		fileName[MAXPATHLEN];

    dprintf(("ENTER: DoFileSystemFile('%s','%s')\n",fileNameSuffixPtr,contentsPtr));

    /* Remove any trailing white space */
    if ( strlen(contentsPtr) ) {
        char    	*myPtr;

        myPtr = contentsPtr + strlen( contentsPtr ) - 1;
        while ( *myPtr == ' ' && myPtr >= contentsPtr ) {
            *myPtr = 0x00;
            myPtr--;
        }
    }

    sprintf( & fileName[0], "%s/%s%s/%s", FS_DIR_LOCATION, HFS_FS_NAME, FS_DIR_SUFFIX, HFS_FS_NAME );
    strcat( & fileName[0], fileNameSuffixPtr );
    unlink( & fileName[0] );		/* erase existing string */

    if ( strlen( fileNameSuffixPtr ) ) {
        int oldMask = umask(0);

        dprintf(("DoFileSystemFile: fileName = '%s'\n",fileName));

        fd = open( & fileName[0], O_CREAT | O_TRUNC | O_WRONLY, 0644 );
        umask( oldMask );
        if ( fd > 0 ) {
            write( fd, contentsPtr, strlen( contentsPtr ) );
            close( fd );
        } else {
            perror( fileName );
        }
    }

    return;

} /* DoFileSystemFile */


/*
 --	GetEmbeddedHFSPlusVol
 --
 --	In: hfsMasterDirectoryBlockPtr
 --	Out: startOffsetPtr - the disk offset at which the HFS+ volume starts (that is, 2 blocks before the volume header)
 --
 */

static int GetEmbeddedHFSPlusVol (HFSMasterDirectoryBlock * hfsMasterDirectoryBlockPtr, off_t * startOffsetPtr)
{
    int		result = FSUR_IO_SUCCESS;
    UInt32	allocationBlockSize, firstAllocationBlock, startBlock, blockCount;

    if ( hfsMasterDirectoryBlockPtr->drSigWord != kHFSSigWord )
      {
        result = FSUR_UNRECOGNIZED;
        goto Return;
      }

    allocationBlockSize = hfsMasterDirectoryBlockPtr->drAlBlkSiz;
    firstAllocationBlock = hfsMasterDirectoryBlockPtr->drAlBlSt;

    dprintf(("GetEmbeddedHFSPlusVol: allocationBlockSize = $%x, firstAllocationBlock = $%x\n", (int32_t)allocationBlockSize, (int32_t) firstAllocationBlock));

    if ( hfsMasterDirectoryBlockPtr->drEmbedSigWord != kHFSPlusSigWord )
      {
        result = FSUR_UNRECOGNIZED;
        goto Return;
      }

    startBlock = hfsMasterDirectoryBlockPtr->drEmbedExtent.startBlock;
    blockCount = hfsMasterDirectoryBlockPtr->drEmbedExtent.blockCount;

    dprintf(("GetEmbeddedHFSPlusVol: startBlock = $%x, blockCount = $%x\n", (int32_t)startBlock, (int32_t) blockCount));

    if ( startOffsetPtr )
        * startOffsetPtr = ( (u_int64_t)startBlock * (u_int64_t)allocationBlockSize ) + ( (u_int64_t)firstAllocationBlock * (u_int64_t)HFS_BLOCK_SIZE );

Return:
        return result;

} // GetEmbeddedHFSPlusVol


/*
 --	GetNameFromHFSPlusVolumeStartingAt
 --
 --	Caller's responsibility to allocate and release memory for the converted string.
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL
 */

static int GetNameFromHFSPlusVolumeStartingAt(int fd, off_t hfsPlusVolumeOffset, char * name_o)
{
    int					result = FSUR_IO_SUCCESS;
    UInt32				blockSize;
    char			*	bufPtr = NULL;
    HFSPlusVolumeHeader		*	volHdrPtr;
    off_t				offset;
    BTNodeDescriptor		*	bTreeNodeDescriptorPtr;
    UInt32				catalogNodeSize;
	UInt32 leafNode;
	UInt32 catalogExtCount;
	HFSPlusExtentDescriptor *catalogExtents = NULL;

    dprintf(("GetNameFromHFSPlusVolumeStartingAt(%d, $%016qx, ...)\n", fd, hfsPlusVolumeOffset));

    volHdrPtr = (HFSPlusVolumeHeader *) malloc(HFS_BLOCK_SIZE);
    if ( ! volHdrPtr ) {
        printf("hfs.util: ERROR: malloc failed\n");
        result = FSUR_IO_FAIL;
        goto Return;
    }

    // Read the volume header.  (This is a little redundant for a pure, unwrapped HFS+ volume.)

    result = readAt( fd, volHdrPtr, hfsPlusVolumeOffset + (off_t)(2*HFS_BLOCK_SIZE), HFS_BLOCK_SIZE );
    if ( result == FSUR_IO_FAIL )
      {
        printf("hfs.util: ERROR: readAt failed\n");
        goto Return; // return FSUR_IO_FAIL
      }

    dprintf(("GetNameFromHFSPlusVolumeStartingAt (2): volHdrPtr->signature = $%04x\n", volHdrPtr->signature));

    /* Verify that it is an HFS+ volume. */

    if ( volHdrPtr->signature != kHFSPlusSigWord )
      {
        result = FSUR_IO_FAIL;
        printf("hfs.util: ERROR: volHdrPtr->signature != kHFSPlusSigWord\n");
        goto Return;
      }

    blockSize = volHdrPtr->blockSize;
    catalogExtents = (HFSPlusExtentDescriptor *) malloc(sizeof(HFSPlusExtentRecord));
    if ( ! catalogExtents ) {
        printf("hfs.util: ERROR: malloc failed\n");
        result = FSUR_IO_FAIL;
        goto Return;
    }
	bcopy(volHdrPtr->catalogFile.extents, catalogExtents, sizeof(HFSPlusExtentRecord));
	catalogExtCount = kHFSPlusExtentDensity;

	/* if there are overflow catalog extents, then go get them */
	if (catalogExtents[7].blockCount != 0) {
		result = GetCatalogOverflowExtents(fd, hfsPlusVolumeOffset, volHdrPtr, &catalogExtents, &catalogExtCount);
		if (result != FSUR_IO_SUCCESS)
			goto Return;
	}

    offset = (off_t)catalogExtents[0].startBlock * (off_t)blockSize;
    dprintf(("GetNameFromHFSPlusVolumeStartingAt: Read the header node of the catalog B-Tree offset = $%016qx\n", offset));
    
	/* Read the header node of the catalog B-Tree */

	result = GetBTreeNodeInfo(fd, hfsPlusVolumeOffset + offset, &catalogNodeSize, &leafNode);
	if (result != FSUR_IO_SUCCESS)
        goto Return;

	/* Calculate the starting block of the first leaf node */

	offset = CalcFirstLeafNodeOffset((leafNode * catalogNodeSize), blockSize, catalogExtCount, catalogExtents);

    if ( offset == 0 ) {
		result = FSUR_IO_FAIL;
        printf("hfs.util: ERROR: can't find leaf block\n");
        goto Return;
	}

	/* Read the first leaf node of the catalog b-tree */

    bufPtr = (char *)malloc(catalogNodeSize);
    if ( ! bufPtr ) {
        printf("hfs.util: ERROR: malloc failed\n");
        result = FSUR_IO_FAIL;
        goto Return;
    }

    bTreeNodeDescriptorPtr = (BTNodeDescriptor *)bufPtr;

    result = readAt( fd, bufPtr, hfsPlusVolumeOffset + offset, catalogNodeSize );
    if ( result == FSUR_IO_FAIL )
      {
        printf("hfs.util: ERROR: readAt (first leaf) failed\n");
        goto Return; // return FSUR_IO_FAIL
      }

    dprintf(("fLink = $%x, bLink = $%x, type = $%x\n",
             (u_int32_t)bTreeNodeDescriptorPtr->fLink, (u_int32_t)bTreeNodeDescriptorPtr->bLink, (u_int32_t)bTreeNodeDescriptorPtr->type ));
    dprintf(("height = $%x, numRecords = $%x, reserved = $%x\n",
             (u_int32_t)bTreeNodeDescriptorPtr->height, (u_int32_t)bTreeNodeDescriptorPtr->numRecords, (u_int32_t)bTreeNodeDescriptorPtr->reserved ));

    {
        UInt16			*	v;
        char			*	p;
        HFSPlusCatalogKey	*	k;

        if (  bTreeNodeDescriptorPtr->numRecords < 1 )
          {
            result = FSUR_IO_FAIL;
			printf("hfs.util: ERROR: bTreeNodeDescriptorPtr->numRecords < 1\n");
            goto Return;
          }

	// Get the offset (in bytes) of the first record from the list of offsets at the end of the node.

        p = bufPtr + catalogNodeSize - sizeof(UInt16); // pointer arithmetic in bytes
        v = (UInt16 *)p;

	// Get a pointer to the first record.

        p = bufPtr + *v; // pointer arithmetic in bytes
        k = (HFSPlusCatalogKey *)p;

	// There should be only one record whose parent is the root parent.  It should be the first record.

        if ( k->parentID != kHFSRootParentID )
          {
            result = FSUR_IO_FAIL;
			printf("hfs.util: ERROR: k->parentID != kHFSRootParentID\n");
            goto Return;
          }

	// Extract the name of the root.
		ConvertUnicodeToUTF8(k->nodeName.length, k->nodeName.unicode, NAME_MAX, name_o);

    }

    result = FSUR_IO_SUCCESS;

Return:
	if (volHdrPtr)
		free((char*) volHdrPtr);

	if (catalogExtents)
		free((char*) catalogExtents);
		
	if (bufPtr)
		free((char*)bufPtr);

    return result;

} /* GetNameFromHFSPlusVolumeStartingAt */



/*
 --	
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL
 --
 */
static int
GetBTreeNodeInfo(int fd, off_t btreeOffset, UInt32 *nodeSize, UInt32 *firstLeafNode)
{
	int result;
	HeaderRec * bTreeHeaderPtr = NULL;

	bTreeHeaderPtr = (HeaderRec *) malloc(HFS_BLOCK_SIZE);
	if ( bTreeHeaderPtr == NULL ) {
        printf("hfs.util: ERROR: malloc failed\n");
		return (FSUR_IO_FAIL);
	}
    
	/* Read the b-tree header node */

	result = readAt( fd, bTreeHeaderPtr, btreeOffset, HFS_BLOCK_SIZE );
	if ( result == FSUR_IO_FAIL ) {
		printf("hfs.util: ERROR: readAt (header node) failed\n");
		goto free;
	}

	if ( bTreeHeaderPtr->node.type != kHeaderNode ) {
		result = FSUR_IO_FAIL;
		printf("hfs.util: ERROR: bTreeHeaderPtr->node.type != kHeaderNode\n");
		goto free;
	}

	*nodeSize = bTreeHeaderPtr->nodeSize;

	if (bTreeHeaderPtr->leafRecords == 0)
		*firstLeafNode = 0;
	else
		*firstLeafNode = bTreeHeaderPtr->firstLeafNode;

free:;
	free((char*) bTreeHeaderPtr);

	return result;

} /* GetBTreeNodeInfo */


/*
 --
 --
 --	Returns: byte offset to first leaf node 
 --
 */
static off_t
CalcFirstLeafNodeOffset(off_t fileOffset, UInt32 blockSize, UInt32 extentCount,
						HFSPlusExtentDescriptor *extentList) {
	off_t offset = 0;
	int i;
	u_long extblks;
	u_long leafblk;

	/* Find this block in the list of extents */
 
	leafblk = fileOffset / blockSize;
	extblks = 0;

	for (i = 0; i < extentCount; ++i) {
		if (extentList[i].startBlock == 0 || extentList[i].blockCount == 0)
			break; /* done when we reach empty extents */

		extblks += extentList [i].blockCount;

		if (extblks > leafblk) {
			offset = (off_t) extentList[i].startBlock * (off_t) blockSize;
			offset += fileOffset - (off_t) ((extblks - extentList[i].blockCount) * blockSize);
			break;
		}
	}

	return offset;

} /* CalcFirstLeafNodeOffset */


/*
 --
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL
 --
 */
static int
GetCatalogOverflowExtents(int fd, off_t hfsPlusVolumeOffset, HFSPlusVolumeHeader *volHdrPtr,
						  HFSPlusExtentDescriptor **catalogExtents, UInt32 *catalogExtCount)
{
	off_t offset;
	UInt32 nodeSize;
	UInt32 firstLeafNode;
    BTNodeDescriptor * bTreeNodeDescriptorPtr;
	HFSPlusExtentDescriptor * extents;
	size_t listsize;
    char *	bufPtr = NULL;
	int i;
	int result;

	listsize = *catalogExtCount * sizeof(HFSPlusExtentDescriptor);
	extents = *catalogExtents;
    offset = (off_t)volHdrPtr->extentsFile.extents[0].startBlock * (off_t)volHdrPtr->blockSize;

	/* Read the header node of the extents B-Tree */

	result = GetBTreeNodeInfo(fd, hfsPlusVolumeOffset + offset, &nodeSize, &firstLeafNode);
	if (result != FSUR_IO_SUCCESS || firstLeafNode == 0)
        goto Return;

	/* Calculate the starting block of the first leaf node */

	offset = CalcFirstLeafNodeOffset((firstLeafNode * nodeSize), volHdrPtr->blockSize, kHFSPlusExtentDensity,
						&volHdrPtr->extentsFile.extents[0]);

	if ( offset == 0 ) {
		result = FSUR_IO_FAIL;
		printf("hfs.util: ERROR: can't find extents b-tree leaf block\n");
		goto Return;
	}

	/* Read the first leaf node of the extents b-tree */

    bufPtr = (char *)malloc(nodeSize);
    if ( ! bufPtr ) {
        printf("hfs.util: ERROR: malloc failed\n");
        result = FSUR_IO_FAIL;
        goto Return;
    }

    bTreeNodeDescriptorPtr = (BTNodeDescriptor *)bufPtr;

    result = readAt( fd, bufPtr, hfsPlusVolumeOffset + offset, nodeSize );
    if ( result == FSUR_IO_FAIL ) {
        printf("hfs.util: ERROR: readAt (first leaf) failed\n");
        goto Return;
	}

	if ( bTreeNodeDescriptorPtr->type != kLeafNode ) {
		result = FSUR_IO_FAIL;
		goto Return;
	}

	for (i = 1; i <= bTreeNodeDescriptorPtr->numRecords; ++i) {
        UInt16			*	v;
        char			*	p;
        HFSPlusExtentKey	*	k;

		/* Get the offset (in bytes) of the record from the list of offsets at the end of the node */

        p = bufPtr + nodeSize - (sizeof(UInt16) * i); /* pointer arithmetic in bytes */
        v = (UInt16 *)p;

		/* Get a pointer to the record */

        p = bufPtr + *v; /* pointer arithmetic in bytes */
        k = (HFSPlusExtentKey *)p;

		if ( k->fileID != kHFSCatalogFileID )
			break;

		/* grow list and copy additional extents */
		listsize += sizeof(HFSPlusExtentRecord);
		extents = (HFSPlusExtentDescriptor *) realloc(extents, listsize);
		bcopy(p + k->keyLength + sizeof(UInt16), &extents[*catalogExtCount], sizeof(HFSPlusExtentRecord));

		*catalogExtCount += kHFSPlusExtentDensity;
	}

	*catalogExtents = extents;

Return:;
	if (bufPtr)
		free(bufPtr);

	return (result);
}


/*
 --	readAt = lseek() + read()
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL
 --
 */

static ssize_t readAt( int fd, void * bufPtr, off_t offset, ssize_t length )
{
    off_t		lseekResult;
    ssize_t		readResult;
    int			result = FSUR_IO_SUCCESS;

#if DEBUG
	printf("hfs.util: readAt(%d, $%08x, $%016qx, $%08x)\n", fd, (u_int32_t)bufPtr, offset, length);
#endif

    lseekResult = lseek( fd, offset, SEEK_SET );
    if ( lseekResult != offset ) {
        dprintf(("hfs.util: readAt: lseek(%d,$%qx,SEEK_SET): $%qx, errno=%d: %s\n",fd,offset,lseekResult,errno,strerror(errno)));
        result = FSUR_IO_FAIL;
        goto Return;
    }

    readResult = read( fd, bufPtr, length );
    if ( readResult != length ) {
        dprintf(("hfs.util: readAt: read(%d,$%x,$%x): $%x, errno=%d: %s\n",fd,(u_int32_t)bufPtr,length,readResult,errno,strerror(errno)));
        result = FSUR_IO_FAIL;
        goto Return;
    }

Return:
        return result;

} /* readAt */


/*
 --	PrepareHFS
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL, FSUR_LOADERR
 --
 */

int PrepareHFS( char * deviceNamePtr, const char * mountPointPtr )
{
    int						result;
    char					path[MAXPATHLEN];

    dprintf(("hfs.util: PrepareHFS: ENTER\n"));

    getwd(path);
    strcat(path, "/");
    strcat(path, HFS_LOADABLE_FILE_NAME);

	dprintf(("hfs.util: PrepareHFS: hfs path = '%s'\n",path));

    result = kl_com_add(path, HFS_LOADABLE_SERVER_NAME);
    if ( result ) {
        dprintf(("hfs.util: ERROR: PrepareHFS:('%s','%s'): kl_com_add('%s','%s') returned %d\n",deviceNamePtr,mountPointPtr,path,HFS_LOADABLE_SERVER_NAME,result));
        result = FSUR_LOADERR;
        goto Return;
    }

    wait_for_kl = TRUE;

    result = kl_com_load(HFS_LOADABLE_SERVER_NAME);
    if ( result ) {
        dprintf(("hfs.util: ERROR: PrepareHFS('%s','%s'): kl_com_load('%s') returned %d\n",deviceNamePtr,mountPointPtr,VOLFS_LOADABLE_SERVER_NAME,result));
        result = FSUR_LOADERR;
        goto Return;
    }

Return:

    dprintf(("hfs.util: PrepareVolFS: EXIT\n"));

    return result;

}


/*
 --	PrepareVolFS
 --
 --	Returns: FSUR_IO_SUCCESS, FSUR_IO_FAIL, FSUR_LOADERR
 --
 */

int PrepareVolFS( void )
{
    int						result;
    struct statfs		*	mntBufPtr;
    int						count;
    int						i;
    int						savedUmask;
    int						mkdirResult;
    int						execlResult;
    int						klResult;
    char					path[MAXPATHLEN];
    union wait  			status;
    int    					pid;

    dprintf(("hfs.util: PrepareVolFS: ENTER\n"));

    /* If the volfs is mounted, then bail. */

    count = getmntinfo( & mntBufPtr, 0 );

    for (i = 0; i < count; i++) {
        if ( ( 0 == strcmp(VOLFS_MOUNT_TYPE,mntBufPtr[i].f_fstypename ) ) && ( 0 == strcmp(VOLFS_MOUNT_POINT,mntBufPtr[i].f_mntonname ) ) ) {
            result = FSUR_IO_SUCCESS;
            goto Return;
        }
    }

    /* If we get here, then the volfs is not mounted on "/.vol" */

    /* Make sure "/.vol" exists. */

    savedUmask = umask(0);
    dprintf(("hfs.util: mkdir('%s',$%x)\n", VOLFS_MOUNT_POINT, VOLFS_MOUNT_POINT_PERMS));
    mkdirResult = mkdir(VOLFS_MOUNT_POINT, VOLFS_MOUNT_POINT_PERMS);
    if ( -1 == mkdirResult ) {
        if ( errno != EEXIST ) {
            printf("hfs.util: ERROR: PrepareVolFS: mkdir('%s',$%x): %s\n", VOLFS_MOUNT_POINT, VOLFS_MOUNT_POINT_PERMS, strerror(errno));
            result = FSUR_IO_FAIL;
            goto Return;
        }
    }
    (void) umask(savedUmask);

    /* Load the volfs kernel server. */

    sprintf(path, "%s/%s", VOLFS_PATH, VOLFS_LOADABLE_FILE_NAME);

    dprintf(("hfs.util: PrepareVolFS: volfs path = '%s'\n",path));

    klResult = kl_com_add(path, VOLFS_LOADABLE_SERVER_NAME);
    if ( klResult ) {
        dprintf(("hfs.util: ERROR: PrepareVolFS: kl_com_add('%s','%s') returned %d\n",path,VOLFS_LOADABLE_SERVER_NAME,klResult));
        result = FSUR_LOADERR;
        goto Return;
    }

    wait_for_kl = TRUE;

    klResult = kl_com_load(VOLFS_LOADABLE_SERVER_NAME);
    if ( klResult ) {
        dprintf(("hfs.util: ERROR: PrepareVolFS: kl_com_load('%s') returned %d\n",HFS_LOADABLE_SERVER_NAME,klResult));
        result = FSUR_LOADERR;
        goto Return;
    }

    /* Construct the volfs mount system command */
    dprintf(("hfs.util: execl('%s')\n", VOLFS_MOUNT_COMMAND));

    pid = fork();
    if (pid == 0) {
        execlResult = execl(VOLFS_MOUNT_COMMAND, VOLFS_MOUNT_COMMAND, VOLFS_MOUNT_POINT, NULL);

        /* IF WE ARE HERE, WE WERE UNSUCCESFULL */
        dprintf(("hfs.util: ERROR: PrepareVolFS: execl('%s') returned %d\n", VOLFS_MOUNT_COMMAND, execlResult));
        result = FSUR_IO_FAIL;
        goto Return;
    }

    if (pid == -1) {
        result = FSUR_IO_FAIL;
        goto Return;
    }

    /* Success! */
    if ((wait4(pid, (int *)&status, 0, NULL) == pid) && (WIFEXITED(status))) {
        result = status.w_retcode;
    }
    else {
        result = -1;
    }

    if(result) {
        result = FSUR_IO_FAIL;
    }
    else {
        result = FSUR_IO_SUCCESS;
    }

Return:

    dprintf(("hfs.util: PrepareVolFS: EXIT\n"));

    return result;

} /* PrepareVolFS */

static int AttemptMount(char *deviceNamePtr, const char *mountPointPtr, boolean_t isLocked) {
    int pid;
    char readonlystr[4]="-r", readwritestr[4]="-w", *isLockedstr;
    int execlResult;
    int result = FSUR_IO_FAIL;
    union wait status;

    pid = fork();
    if (pid == 0) {
        isLockedstr = ( isLocked ) ? readonlystr :readwritestr;
        dprintf(("hfs.util: %s(%s %s %s %s %s %s)\n", MOUNT_COMMAND, isLockedstr, "-o -x", "-t", HFS_MOUNT_TYPE, deviceNamePtr, mountPointPtr));
        execlResult = execl(MOUNT_COMMAND, MOUNT_COMMAND, isLockedstr,"-o","-x","-t", HFS_MOUNT_TYPE, deviceNamePtr, mountPointPtr, NULL);

        /* IF WE ARE HERE, WE WERE UNSUCCESFULL */
        dprintf(("hfs.util: ERROR: AttemptMount: execl('%s' '%s') returned %d\n", MOUNT_COMMAND, mountPointPtr, execlResult));
        result = FSUR_IO_FAIL;
        goto Return;
    }

    if (pid == -1) {
        result = FSUR_IO_FAIL;
        goto Return;
    }

    /* Success! */
    if ((wait4(pid, (int *)&status, 0, NULL) == pid) && (WIFEXITED(status))) {
        result = status.w_retcode;
    }
    else {
        result = -1;
    }

Return:
        return result;
}


UniChar	gHiBitBaseUnicode[128] =
{
	/* 0x80 */	0x0041, 0x0041, 0x0043, 0x0045, 0x004e, 0x004f, 0x0055, 0x0061, 
	/* 0x88 */	0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0063, 0x0065, 0x0065, 
	/* 0x90 */	0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069, 0x006e, 0x006f, 
	/* 0x98 */	0x006f, 0x006f, 0x006f, 0x006f, 0x0075, 0x0075, 0x0075, 0x0075, 
	/* 0xa0 */	0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
	/* 0xa8 */	0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
	/* 0xb0 */	0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
	/* 0xb8 */	0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
	/* 0xc0 */	0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
	/* 0xc8 */	0x00bb, 0x2026, 0x00a0, 0x0041, 0x0041, 0x004f, 0x0152, 0x0153, 
	/* 0xd0 */	0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
	/* 0xd8 */	0x0079, 0x0059, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02, 
	/* 0xe0 */	0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x0041, 0x0045, 0x0041, 
	/* 0xe8 */	0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004f, 0x004f, 
	/* 0xf0 */	0xf8ff, 0x004f, 0x0055, 0x0055, 0x0055, 0x0131, 0x02c6, 0x02dc, 
	/* 0xf8 */	0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
};

UniChar	gHiBitCombUnicode[128] =
{
	/* 0x80 */	0x0308, 0x030a, 0x0327, 0x0301, 0x0303, 0x0308, 0x0308, 0x0301, 
	/* 0x88 */	0x0300, 0x0302, 0x0308, 0x0303, 0x030a, 0x0327, 0x0301, 0x0300, 
	/* 0x90 */	0x0302, 0x0308, 0x0301, 0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 
	/* 0x98 */	0x0300, 0x0302, 0x0308, 0x0303, 0x0301, 0x0300, 0x0302, 0x0308, 
	/* 0xa0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	/* 0xa8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xb8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xc8 */	0x0000, 0x0000, 0x0000, 0x0300, 0x0303, 0x0303, 0x0000, 0x0000, 
	/* 0xd0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xd8 */	0x0308, 0x0308, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	/* 0xe0 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0302, 0x0302, 0x0301, 
	/* 0xe8 */	0x0308, 0x0300, 0x0301, 0x0302, 0x0308, 0x0300, 0x0301, 0x0302, 
	/* 0xf0 */	0x0000, 0x0300, 0x0301, 0x0302, 0x0300, 0x0000, 0x0000, 0x0000, 
	/* 0xf8 */	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};


static void
ConvertMacRomanToUnicode ( ConstStr255Param pascalString, int *unicodeChars,
						   UniCharArrayPtr unicodeString )
{
	const UInt8		*p;
	UniChar			*u;
	UInt16			pascalChars;
	UInt8			c;


	p = pascalString;
	u = unicodeString;

	*unicodeChars = pascalChars = *(p++);	// pick up length byte

	while (pascalChars--) {
		c = *(p++);

		if ( (SInt8) c >= 0 ) {		// make sure its seven bit ascii
			*(u++) = (UniChar) c;		//  pad high byte with zero
		}
		else {	/* its a hi bit character */
			UniChar uc;

			c &= 0x7F;
			*(u++) = uc = gHiBitBaseUnicode[c];
			
			/*
			 * if the unicode character (uc) is an alpha char
			 * then we have an additional combining character
			 */
			if ((uc <= (UniChar) 'z') && (uc >= (UniChar) 'A')) {
				*(u++) = gHiBitCombUnicode[c];
				++(*unicodeChars);
			}
		}
	}
}


static void
ConvertUnicodeToUTF8(int srcCount, const UniChar* srcStr, int dstMaxBytes, UTF8* dstStr)
{
	UTF16* sourceStart;
	UTF16* sourceEnd;
	UTF8* targetStart;
	UTF8* targetEnd;
	int outputLength;

	sourceStart = (UTF16*) srcStr;
	sourceEnd = (UTF16*) (srcStr + srcCount);
	targetStart = (UTF8*) dstStr;
	targetEnd = targetStart + dstMaxBytes - 1;	/* - 1 for NULL termination space */
	
	(void) ConvertUTF16toUTF8 (&sourceStart, sourceEnd, &targetStart, targetEnd);
	
	outputLength = targetStart - dstStr;
		
	dstStr[outputLength] = '\0';	/* add null termination */
}

/* ================================================================ */

#define halfShift				10
#define halfBase				0x0010000UL
#define kSurrogateHighStart		0xD800UL
#define kSurrogateHighEnd		0xDBFFUL
#define kSurrogateLowStart		0xDC00UL
#define kSurrogateLowEnd		0xDFFFUL

#define kReplacementCharacter	0x0000FFFDUL
#define kMaximumUCS4			0x7FFFFFFFUL

/* ================================================================ */

UTF8 firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

/*
 * Colons vs. Slash
 *
 * The VFS layer uses a "/" as a pathname separator but HFS disks
 * use a ":".  So when converting into UTF-8, "/" characters need
 * to be changed to ":" so that a "/" in a filename is not returned 
 * through the VFS layer.
 *
 * We do not need to worry about full-width slash or colons since
 * their respective representations outside of Unicode are never
 * the 7-bit versions (0x2f or 0x3a).
 */


/* ================================================================ */


static ConversionResult
ConvertUTF16toUTF8 (UTF16** sourceStart, const UTF16* sourceEnd, UTF8** targetStart,
					const UTF8* targetEnd)
{
	ConversionResult result = ok;
	register UTF16* source = *sourceStart;
	register UTF8* target = *targetStart;
	while (source < sourceEnd) {
		register UCS4 ch;
		register unsigned short bytesToWrite = 0;
		register const UCS4 byteMask = 0xBF;
		register const UCS4 byteMark = 0x80; 
		register const UCS4 slash = '/'; 
		ch = *source++;
		if (ch == slash) {
			ch = ':';	/* VFS doesn't like slash */
		} else if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
				&& source < sourceEnd) {
			register UCS4 ch2 = *source;
			if (ch2 >= kSurrogateLowStart && ch2 <= kSurrogateLowEnd) {
				ch = ((ch - kSurrogateHighStart) << halfShift)
					+ (ch2 - kSurrogateLowStart) + halfBase;
				++source;
			};
		};
		if (ch < 0x80) {				bytesToWrite = 1;
		} else if (ch < 0x800) {		bytesToWrite = 2;
		} else if (ch < 0x10000) {		bytesToWrite = 3;
		} else if (ch < 0x200000) {		bytesToWrite = 4;
		} else if (ch < 0x4000000) {	bytesToWrite = 5;
		} else if (ch <= kMaximumUCS4){	bytesToWrite = 6;
		} else {						bytesToWrite = 2;
										ch = kReplacementCharacter;
		}; /* I wish there were a smart way to avoid this conditional */
		
		target += bytesToWrite;
		if (target > targetEnd) {
			target -= bytesToWrite; result = targetExhausted; break;
		};
		switch (bytesToWrite) {	/* note: code falls through cases! */
			case 6:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 5:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 4:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 3:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 2:	*--target = (ch | byteMark) & byteMask; ch >>= 6;
			case 1:	*--target =  ch | firstByteMark[bytesToWrite];
		};
		target += bytesToWrite;
	};
	*sourceStart = source;
	*targetStart = target;
	return result;
};





