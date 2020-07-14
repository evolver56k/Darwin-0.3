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
	
	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
	The copyright notice above does not evidence any actual or
	intended publication of such source code.
	
	About VDI.c:
	Contains glue code for the Rhapsody Volume/Directory ID (VDI) interface.
	
	To do:
    Look for "PPD" for unresolved issues
	
	Change History:
	 2-Feb-1999	Pat Dirks	Fixed DoUnpackSearchResults to factor in name mangling effect on name length.
    26-Jan-1999 Don Brady	Need to toggle path separators for resolved dir names.
     8-Jan-1999 Don Brady	Add name-mangling for returned filenames.
     5-Jan-1999 Pat Dirks	Added code in OpenFork_VDI to implement kNoMapFS option
     17-Dec-1998 Pat Dirks	Removed setattrlist permissions workaround in SetCatalogInfo_VDI (#2293114).
     9-Dec-1998 Don Brady	Pass packed attribute names to kernel in UTF-8.
     3-Dec-1998 Pat Dirks	Added support for kVolumeReadOnly attribute in GetVolumeInfo_VDI.
    30-Nov-1998	Don Brady	Fix buffer parameters for ConvertUTF8ToCString calls.

     3-Dec-1998 Pat Dirks   Added code to copy flags in DoUnPackAttributeBuffer and set kObjectLocked in attributes.
    20-Nov-1998	Don Brady	Add support for UTF-8 names.
    18-Nov-1998	Pat Dirks	Added stub routines with old names to allow debugging with Workspace.
    17-Nov-1998 Pat Dirks	Changed to add _VDI to all routines, add unicodeCharCount argument, change
                            ResolveFileID_VDI to allow null originalEncodingFormat specification.
     2-Sep-1998	Don Brady	Fix RemoveDir_VDI to handle empty names (only DirID specified) for radar #2261951.
    27-Aug-1998	Pat Dirks	Added error checking code to SetCatalogInfo to check for attempts to set any fileBitmap
                            attributes or directoryBitmap attributes (#2265841)
    27-Jul-1998	Don Brady	Add kOpenReadOnly option to OpenFork_VDI (radar #2258546).
    23-Jul-1998	Don Brady	Remove some debugging printf calls.
    13-Jul-1998	Don Brady	Fix SearchCatalog semantics and clean up interaction with searchfs (radar #2251855).
	04-Jul-1998	chw			Added fstat calls to ForkReserveSpace to calculate actual bytes allocated
	 7-Jul-1998	Pat Dirks	Changed to use bitmaps on vol. info calls and use ATTR_VOL_NAME instead of ATTR_CMN_OBJNAME.
    15-Jul-1998	Pat Dirks	Changed GetCatalogInfo to stop clobbering user-specified bitmap fields.
	04-Jul-1998	chw			Added fstat calls to ForkReserveSpace to calculate actual bytes allocated
	23-Jun-1998	Pat Dirks	Added PathTranslationRequired and ToggleSeparators to do ":" and "/" translation.
	22-Jun-1998 Pat Dirks	Fixed CreateSymlink_VDI to check for return EINVAL if ANY parameter is zero.
	 9-Jun-1998	Pat Dirks	Fix DoPackSearchInfo to pass parent object id as type fsobj_id_t (radar #2249248).
	 9-Jun-1998	Pat Dirks	Fixed DoUnpackSearchResults to copy name into user's buffer.
	 4-Jun-1998	Pat Dirks	Added CreateFileID.
	28-May-1998	Pat Dirks	Changed to make GetCatalogInfo/SearchCatalog return more uniform.
                            Remove bogus preflight assertions (radar #2237709). When invalid parameters
                            are passed in, set errno and cthread_set_errno_self to EINVAL.
    11-May-1998	Don Brady	Create_VDI now has HFS/MacOS semantics...(radar #2220488).
	23-Apr-1998	Pat Dirks	Fixed DoPackCommonAttributes to side-effect length argument and pack mode_t
							properly (as a 4-int u_long).
    23-Apr-1998	Don Brady	Fix VDIR type check. Conditionally removed ForkReserveSpace (not part of CR1).
	4/17/98	DSH	   			SearchCatalog() matches buffer was being refilled rather than added to.
	4/14/98	DSH	   			Added SearchCatalog() and related routines to call searchfs system trap.
	4/8/98	chw	   			Changed arguments to useraccess to match kernel routine

     7-Apr-1998	Pat Dirks	Changed CheckUserAccess_VDI to UserAccess_VDI to better match
                            name of underlying system call.
	 3/20/1998	Pat Dirks	Changed to leave off fork specification in DoBuildVolFSPath
	 						until we're actually ready to interpret it.  Fixed order of
                            arguments in some memcpy() calls.
    03/17/1998  Pat Dirks	Changed to use 'fsvolid_t' for volID type throughout.
    01/05/1998  jwc			Birth of a file.
*/


// ************************************** I N C L U D E S *************************************

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/syslimits.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/attr.h>
#include <mach/cthreads.h>

#include "volvar.h"
#include "vdi_translation.h"

#define LOCALNAMEBUFFERSIZE 32

#define INCLUDEOLDROUTINESTUBS 0

#define MAXHFSNAMESIZE 32

// *********************************** P R O T O T Y P E S ***********************************

static int GetVolumeIDFromIndex(long volumeIndex, fsvolid_t *volID);
static void DoBuildVolFSPath ( 	char *thePathPtr, fsvolid_t theVolID, u_long theDirID, const char *theNamePtr, u_long theScriptCode, const char *theForkNamePtr );
static int PathTranslationRequired( const char * pathNamePtr );
static char * ToggleSeparators( const char * SourcePathNamePtr, u_long SourcePathEncoding, u_long TargetPathEncoding, char * TargetPathNamePtr );
static void ToggleSeparatorsInPlace(char * pathNamePtr);
static void DoPackVolumeInfoBuffer ( VolumeAttributeInfoPtr theInfoPtr, VolInfoChangeBuf* theBufferPtr );
static void DoUnPackAttributeBuffer( struct catalogAttrBlock *getAttrBuf, CatalogAttributeInfoPtr theCatInfoPtr );
static void DoPackCommonAttributes(	struct attrlist *alist, struct CommonAttributeInfo *setcommattrbuf, void** attrbufptrptr );
static void DoSetFileNameScript ( u_long theScriptCode );
static void DoSetFileSystemOptions ( u_long theOptions );
static void DoUnpackVolumeInfoBuffer ( VolInfoReturnBuf* theBufferPtr, VolumeAttributeInfoPtr theInfoPtr );
static int DoValidateForkName (	const char *theForkNamePtr, u_long theScriptCode );
static int DoValidateOptions ( 	u_long theOptions );
static int DoValidateScriptCode ( u_long theScriptCode );
static void DoPackSearchInfo( struct attrlist *alist, CatalogAttributeInfo *searchInfo, char *name,
							  u_long nameEncoding, void** attrbufptrptr, void** varbufptrptr );
static void DoUnPackSearchResults( void *packedBuffer, void **targetBuffer, u_long *targetBufferSize, u_long numPackedRecords, u_long *numTargetRecords );
static	size_t	VDIAttributeBlockSize( struct attrlist *attrlist );
static int HasNonASCIIChars(const char *string);


/*
 *	typedefs and macros to mark a fs as hfs standard
 */
#define HFSFSMNT_HFSSTDMASK 0x40000000	/* Set to determine if a volume is hfs standard */
#define ISHFSSTD(volID)		((u_long)(volID) & HFSFSMNT_HFSSTDMASK)	/* To determine if a volume is hfs */

/*
 ***************************************** Create_VDI *****************************************
	Purpose -
		This routine implements the outter most layer of creating a file using a volume ID,
		Directory ID, and file name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target file will live.
		theDirID - the unique directory ID where the target file will live.
		theFileNamePtr - pointer to the name of the target file.
		nameEncoding - script the given name is in.
		theOptions - call options
		theMode - UNIX mode for the file (see chmod)
	Output -
		NA. 
	Result - 
		file descriptor (always > 0) or -1 on error. PPD - is this correct?
 */

int Create_VDI(	fsvolid_t theVolID,
                u_long theDirID,
                const char *theFileNamePtr,
                u_long unicodeCharacterCount,
                u_long nameEncoding,
                u_long theOptions,
                mode_t theMode )
{
	int				myError;
	int				myFD = 0;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];

	/* Check input parms */
    if ( theFileNamePtr == NULL || theDirID == 0 || theVolID == 0 || unicodeCharacterCount != 0)
		goto InvalidArgument;
		
    if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;

    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theFileNamePtr)) == 0) {
        namePtr = theFileNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theFileNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the creat request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Create the file */
    myFD = open( &myPath[0], O_CREAT | O_EXCL | O_RDWR, theMode );
	if (myFD == -1) {
        myError = errno;
    } else {
		myError = close(myFD);
		if (myError != 0) myError = errno;
	}
    goto StdExit;

InvalidArgument:
	myError = EINVAL;

ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(EINVAL);
    return myError;

} /* Create_VDI */


#if 0
/*
 ********************************** FSChangeNotify_VDI ********************************
	Purpose -
		This routine implements the outter most layer of system call to get notification
		when a directory changes using a volume ID and directory ID.  Some housekeeping is 
		done then a path is built for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target directory lives.
		theDirID - the unique directory ID of the target.
		theOptions - call options.
		theNotifyPort - the message port to send notification to.
	Output -
		theLastModDatePtr - current modification date of the target directory. 
	Result - 
		PPD - what goes here?
 */

int FSChangeNotify_VDI(	fsvolid_t theVolID,
                        u_long theDirID,
                        const char *theFileNamePtr,
                        u_long unicodeCharacterCount,
                        u_long nameEncoding,
                        u_long theOptions,
                        struct timespec *theLastModDatePtr )
{
	// PPD - not sure what to do here?

    errno = EINVAL;
    cthread_set_errno_self(EINVAL);
	return -1;

} /* FSChangeNotify_VDI */


/*
 *********************************** CancelChangeNotify_VDI **********************************
	Purpose -
		This routine implements the outter most layer of system call to cancel notification
		when a directory changes using a volume ID and directory ID.  Some housekeeping is 
		done then a path is built for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target directory lives.
		theDirID - the unique directory ID of the target.
		theOptions - call options.
		theNotifyPort - the message port we're cancelling notification to.
	Output -
		NA. 
	Result - 
		PPD - what goes here?
 */

int CancelChangeNotify_VDI(	fsvolid_t theVolID,
                            u_long theDirID,
                            const char *theFileNamePtr,
                            u_long unicodeCharacterCount,
                            u_long nameEncoding,
                            u_long theOptions )
{
	// PPD - not sure what to do here? 

    errno = EINVAL;
    cthread_set_errno_self(EINVAL);
	return -1;

} /* CancelChangeNotify_VDI */
#endif


/*
 ***************************************** Remove_VDI ****************************************
	Purpose -
		This routine implements the outter most layer of removing a file using a volume ID,
		directory ID, and file name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target file lives.
		theDirID - the unique directory ID where the target file lives.
		theFileNamePtr - pointer to the name of the target file.
		nameEncoding - script the given name is in.
		theOptions - call options
	Output -
		NA. 
	Result - 
		whatever UNIX unlink command returns. PPD - is this correct?
 */

int Remove_VDI(	fsvolid_t theVolID,
                u_long theDirID,
                const char *theFileNamePtr,
                u_long unicodeCharacterCount,
                u_long nameEncoding,
                u_long theOptions )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];

	/* Check input parms */
    if ( theFileNamePtr == NULL || unicodeCharacterCount != 0 || theDirID == 0 || theVolID == 0 )
		goto InvalidArgument;
		
    if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theFileNamePtr)) == 0) {
        namePtr = theFileNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theFileNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the unlink request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Remove the file */
    myError = unlink( &myPath[0] );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* Remove_VDI */


/*
 ************************************* UserAccess_VDI ***********************************
	Purpose -
		This routine implements the outter most layer of checking the access of a specific
		user using a volume ID, directory ID, and file name.  Some housekeeping is done then 
		a path is built for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target lives.
		theDirID - the unique directory ID where the target lives.
		theNamePtr - pointer to the name of the target.
        nameEncoding - script the given name is in.
		theOptions - call options.
		theUserID - uid of the user we're checking on.
		theAccessRequired - PPD -  what is this?
	Output -
		NA. 
	Result - 
		whatever useraccess returns.  (0 if access requested is allowed, otherwise EACES or EPERM)
	Note - 
		The caller must be root
 */

int UserAccess_VDI( fsvolid_t theVolID,
                    u_long theDirID,
                    const char *theNamePtr,
                    u_long unicodeCharacterCount,
                    u_long nameEncoding,
                    u_long theOptions,
                    uid_t theUserID,
                    gid_t *theGroupList,
                    int nGroups,
                    u_long theAccessRequired )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];

	/* Check input parms */
    if ( theDirID == 0 || theVolID == 0 || unicodeCharacterCount != 0 )
		goto InvalidArgument;
		
    if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
	
	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theNamePtr)) == 0) {
        namePtr = theNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the unlink request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Check Access */
    myError = checkuseraccess( &myPath[0], theUserID, theGroupList,nGroups,theAccessRequired );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* UserAccess_VDI */


/*
 *************************************** RemoveDir_VDI ***************************************
	Purpose -
		This routine implements the outter most layer of removing a directory using a volume 
		ID, directory ID, and directory name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target directory lives.
		theDirID - the unique directory ID where the target directory lives.
		theNamePtr - pointer to the name of the target directory.
        nameEncoding - script the given name is in.
		theOptions - call options
	Output -
		NA. 
	Result - 
		whatever UNIX rmdir command returns. PPD - is this correct?
 */

int RemoveDir_VDI( fsvolid_t theVolID,
                   u_long theDirID,
                   const char *theNamePtr,
                   u_long unicodeCharacterCount,
                   u_long nameEncoding,
                   u_long theOptions )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];

	/* Check input parms */
    if ( unicodeCharacterCount != 0 || theDirID == 0 || theVolID == 0 )
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
	
	/* If the name is empty then go look it up using ResolveFileID.
	   Otherwise see if the incoming pathname contains colons or slashes that need
	   translation and construct a translated pathname if any are encountered.
	 */
	if ((theNamePtr == NULL) || (theNamePtr[0] == 0)) {
		u_long dirID;
		u_long textEncoding;

		nameBufferPtr = malloc(NAME_MAX);
		if (nameBufferPtr == NULL) {
			myError = ENOMEM;
			goto ErrorExit;
		};
		allocatedNameBuffer = 1;

        myError = ResolveFileID_VDI(theVolID, theDirID, kMacOSHFSFormat, &dirID, nameBufferPtr, NAME_MAX, &textEncoding);
		if (myError != 0) {
			myError = errno;
			goto StdExit;
		}
		if ((theOptions & kDontTranslateSeparators) == 0) {
			ToggleSeparatorsInPlace(nameBufferPtr);
		}
		/* XXX nameEncoding = textEncoding; */
		theDirID = dirID;
        namePtr = nameBufferPtr;

    } else if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theNamePtr)) == 0) {
        namePtr = theNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the rmdir request down to the appropriate file system plug in */
	DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Remove the directory */
    myError = rmdir( &myPath[0] );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* RemoveDir_VDI */


/*
 ***************************************** OpenDir_VDI ***************************************
	Purpose -
		This routine implements the outter most layer of opening a directory using a volume 
		ID, directory ID, and directory name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target directory lives.
		theDirID - the unique directory ID where the target directory lives.
		theNamePtr - pointer to the name of the target directory.
		nameEncoding - script the given name is in.
		theOptions - call options
	Output -
		NA. 
	Result - 
		DIR * or NULL on error. PPD - is this correct?
 */

DIR * OpenDir_VDI( fsvolid_t theVolID,
                   u_long theDirID,
                   const char *theNamePtr,
                   u_long unicodeCharacterCount,
                   u_long nameEncoding,
                   u_long theOptions )
{
	DIR *			myDirPtr = NULL;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];
    int				myError;

	/* Check input parms */
    if ( theDirID == 0 || theVolID == 0 || unicodeCharacterCount != 0 )
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;
	
	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theNamePtr)) == 0) {
        namePtr = theNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path */
	/* then pass the opendir request down to the appropriate file system plug in */
	DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Open the directory */
    myDirPtr = opendir( &myPath[0] );
	if (myDirPtr == NULL) {
		myError = errno;
		goto ErrorExit;
	};
	myError = 0;
	goto StdExit;

InvalidArgument:
	myError = EINVAL;

ErrorExit:
	myDirPtr = NULL;

StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);
    
    return myDirPtr;

} /* OpenDir_VDI */


#if 0
/*
 *************************************** CreateLink_VDI **************************************
	Purpose -
		This routine implements the outter most layer of creating a hard link using a volume 
		ID, directory ID, and file name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target lives (and new link).
		theNewDirID - the unique directory ID where the new link will live.
		theNewNamePtr - pointer to the name of the link.
		nameEncoding - script the given name is in.
		theTargetDirID - the unique directory ID where the target lives.
		theTargetNamePtr - pointer to the name of the target.
		theOptions - call options
	Output -
		NA
	Result - 
		whatever link system command returns. PPD - is this correct?
 */

int CreateLink_VDI( 	fsvolid_t theVolID,
                     u_long theNewDirID,
                     const char *theNewNamePtr,
                     u_long unicodeCharacterCount,
                     u_long nameEncoding,
                     u_long theTargetDirID,
                     const char *theTargetNamePtr,
                     u_long theOptions )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNewNameBuffer = 0;
	int				allocatedTargetNameBuffer = 0;
	char *			newNameBufferPtr = NULL;
    const char *	newNamePtr;
	char *			targetNameBufferPtr = NULL;
    const char *	targetNamePtr;
	char			myNewPath[PATH_MAX];
	char			myTargetPath[PATH_MAX];

	/* Check input parms */
	if ( theVolID == 0 || theNewDirID == 0 || theNewNamePtr == NULL ||
      theTargetDirID == 0 || theTargetNamePtr == NULL || unicodeCharacterCount != 0)
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
	
	/* See if either of the incoming pathnames contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theNewNamePtr)) == 0) {
        newNamePtr = theNewNamePtr;
	} else {
		newNameBufferPtr = malloc(requiredPathBufferSize);
		if (newNameBufferPtr == NULL) {
			myError = ENOMEM;
			goto ErrorExit;
		};
		allocatedNewNameBuffer = 1;
        ToggleSeparators( theNewNamePtr, nameEncoding, nameEncoding, newNameBufferPtr );
        newNamePtr = newNameBufferPtr;
	};
	
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theTargetNamePtr)) == 0) {
        targetNamePtr = theTargetNamePtr;
	} else {
		targetNameBufferPtr = malloc(requiredPathBufferSize);
		if (targetNameBufferPtr == NULL) {
			myError = ENOMEM;
			goto ErrorExit;
		};
		allocatedTargetNameBuffer = 1;
        ToggleSeparators( theTargetNamePtr, nameEncoding, nameEncoding, targetNameBufferPtr );
        targetNamePtr = targetNameBufferPtr;
	};
	
	/* Build VolFS paths - the volume file system layer will convert these to a UNIX path */
	/* pass the link request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myNewPath[0], theVolID, theNewDirID, newNamePtr, nameEncoding, 0 );
    DoBuildVolFSPath( &myTargetPath[0], theVolID, theTargetDirID, targetNamePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Create the file */
    myError = link( &myTargetPath[0], &myNewPath[0] );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNewNameBuffer) free(newNameBufferPtr);
	if (allocatedTargetNameBuffer) free(targetNameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* CreateLink_VDI */
#endif


#if 0
/*
 ************************************** CreateSymlink_VDI ************************************
	Purpose -
		This routine implements the outter most layer of creating a symbolic link using a 
		volume ID, directory ID, and file name.  Some housekeeping is done then a path is 
		built for use in the Volume File System layer.  
	Input - 
		theNewVolID - the unique volume ID where the new symlink will live.
		theNewDirID - the unique directory ID where the new symlink will live.
		theNewNamePtr - pointer to the name of the new symlink.
		nameEncoding - script the given name is in.
		theTargetVolID - the unique volume ID where the target lives.
		theTargetDirID - the unique directory ID where the target lives.
		theTargetNamePtr - pointer to the name of the target.
		theOptions - call options
	Output -
		NA
	Result - 
		whatever symlink system command returns. PPD - is this correct?
 */

int CreateSymlink_VDI( fsvolid_t theNewVolID,
                       u_long theNewDirID,
                       const char *theNewNamePtr,
                       u_long unicodeCharacterCount,
                       u_long nameEncoding,
                       fsvolid_t theTargetVolID,
                       u_long theTargetDirID,
                       const char *theTargetNamePtr,
                       u_long theOptions )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNewNameBuffer = 0;
	int				allocatedTargetNameBuffer = 0;
	char *			newNameBufferPtr = NULL;
    const char *	newNamePtr;
	char *			targetNameBufferPtr = NULL;
    const char *	targetNamePtr;
	char			myNewPath[PATH_MAX];
	char			myTargetPath[PATH_MAX];

	/* Check input parms */
	if ( theNewVolID == 0 || theNewDirID == 0 || theNewNamePtr == NULL ||
		 theTargetVolID == 0 || theTargetDirID == 0 || theTargetNamePtr == NULL )
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
	
	/* See if either of the incoming pathnames contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theNewNamePtr)) == 0) {
        newNamePtr = theNewNamePtr;
	} else {
		newNameBufferPtr = malloc(requiredPathBufferSize);
		if (newNameBufferPtr == NULL) {
			myError = ENOMEM;
			goto ErrorExit;
		};
		allocatedNewNameBuffer = 1;
		ToggleSeparators( theNewNamePtr, nameEncoding, newNameBufferPtr, nameEncoding );
        newNamePtr = newNameBufferPtr;
	};
	
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theTargetNamePtr)) == 0) {
        targetNamePtr = theTargetNamePtr;
	} else {
		targetNameBufferPtr = malloc(requiredPathBufferSize);
		if (targetNameBufferPtr == NULL) {
			myError = ENOMEM;
			goto ErrorExit;
		};
		allocatedTargetNameBuffer = 1;
        ToggleSeparators( theTargetNamePtr, nameEncoding, nameEncoding, targetNameBufferPtr );
        targetNamePtr = targetNameBufferPtr;
	};
	
	/* Build VolFS paths - the volume file system layer will convert these to a UNIX path */
	/* pass the link request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myNewPath[0], theNewVolID, theNewDirID, newNamePtr, nameEncoding, 0 );
    DoBuildVolFSPath( &myTargetPath[0], theTargetVolID, theTargetDirID, targetNamePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Create the symlink */
    myError = symlink( &myTargetPath[0], &myNewPath[0] );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNewNameBuffer) free(newNameBufferPtr);
	if (allocatedTargetNameBuffer) free(targetNameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(EINVAL);

    return myError;

} /* CreateSymlink_VDI */
#endif


/*
 ***************************************** MakeDir_VDI ***************************************
	Purpose -
		This routine implements the outter most layer of creating a directory using a volume ID,
		directory ID, and file name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target directory will live.
		theDirID - the unique directory ID where the target directory will live.
		theDirNamePtr - pointer to the name of the target directory.
		nameEncoding - script the given name is in.
		theOptions - call options
		theMode - UNIX mode for the directory (see chmod)
	Output -
		NA. 
	Result - 
		whatever UNIX mkdir command returns.  PPD - is this correct?
 */

int MakeDir_VDI( fsvolid_t theVolID,
                 u_long theDirID,
                 const char *theDirNamePtr,
                 u_long unicodeCharacterCount,
                 u_long nameEncoding,
                 u_long theOptions,
                 mode_t theMode )
{
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];
    int				myError;

	/* Check input parms */
    if ( theDirNamePtr == NULL || theDirID == 0 || theVolID == 0 || unicodeCharacterCount != 0)
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;
	
	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theDirNamePtr)) == 0) {
        namePtr = theDirNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theDirNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the mkdir request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, 0 );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Make the directory */
	myError = mkdir( &myPath[0], theMode );
	if (myError != 0) myError = errno;
	goto StdExit;

InvalidArgument:
	myError = EINVAL;

ErrorExit:	
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* MakeDir_VDI */


/*
 ***************************************** Rename_VDI ****************************************
	Purpose -
		This routine implements the outter most layer of renameing a file system object using 
		a volume ID, directory ID, and name.  Some housekeeping is done then a path is 
		built for use in the Volume File System layer.  
	Input - 
		theVolID - the unique volume ID where the target lives.
		theOldDirID - the unique directory ID where the target lives.
		theOldNamePtr - pointer to the name of the target.
		nameEncoding - script the given name is in.
		theNewDirID - the unique directory ID where the new object will live.
		theNewNamePtr - pointer to the name of the new object.
		theOptions - call options
	Output -
		NA
	Result - 
		whatever rename system command returns. PPD - is this correct?
 */

int Rename_VDI(	fsvolid_t theVolID,
                u_long theOldDirID,
                const char *theOldNamePtr,
                u_long oldUnicodeCharacterCount,
                u_long nameEncoding,
                u_long theNewDirID,
                const char *theNewNamePtr,
                u_long newUnicodeCharacterCount,
                u_long theOptions )
{
    int				myError;
    int				requiredPathBufferSize;
    int				allocatedOldNameBuffer = 0;
	char			localOldNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			oldNameBufferPtr = NULL;
    const char *	oldNamePtr;
	int				allocatedNewNameBuffer = 0;
	char			localNewNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			newNameBufferPtr = NULL;
    const char *	newNamePtr;
    char			myNewPath[PATH_MAX];
    char			myOldPath[PATH_MAX];

    /* Check input parms */
    if ( (theVolID == 0) || (theNewDirID == 0) || (theNewNamePtr == NULL) || (newUnicodeCharacterCount != 0) ||
         (theOldDirID == 0) || (theOldNamePtr == NULL) || (oldUnicodeCharacterCount != 0) )
        {
        goto InvalidArgument;
        };

    if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;

    if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;

	/* See if either of the incoming pathnames contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theOldNamePtr)) == 0) {
        oldNamePtr = theOldNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			oldNameBufferPtr = localOldNameBuffer;
		} else {
			oldNameBufferPtr = malloc(requiredPathBufferSize);
			if (oldNameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedOldNameBuffer = 1;
		};
        ToggleSeparators( theOldNamePtr, nameEncoding, nameEncoding, oldNameBufferPtr );
        oldNamePtr = oldNameBufferPtr;
	};

    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theNewNamePtr)) == 0) {
        newNamePtr = theNewNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			newNameBufferPtr = localNewNameBuffer;
		} else {
			newNameBufferPtr = malloc(requiredPathBufferSize);
			if (newNameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNewNameBuffer = 1;
		};
        ToggleSeparators( theNewNamePtr, nameEncoding, nameEncoding, newNameBufferPtr );
        newNamePtr = newNameBufferPtr;
	};

    /* Build VolFS paths - the volume file system layer will convert these to a UNIX path */
    /* pass the link request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myNewPath[0], theVolID, theNewDirID, newNamePtr, nameEncoding, 0 );
    DoBuildVolFSPath( &myOldPath[0], theVolID, theOldDirID, oldNamePtr, nameEncoding, 0 );

    /* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
    DoSetFileSystemOptions( theOptions & kOSOptionsMask );

    /* rename the file system object */
    myError = rename( myOldPath, myNewPath );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedOldNameBuffer) free(oldNameBufferPtr);
	if (allocatedNewNameBuffer) free(newNameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* Rename_VDI */


/*
 ***************************************** OpenFork_VDI ***************************************
	Purpose -
		This routine implements the outter most layer of opening a file using a volume ID,
		Directory ID, file name, and fork name.  Some housekeeping is done then a path is built
		for use in the Volume File System layer.  After we build the path we do an open with
		said path which will route the open request to the appropriate file system plug in.  
	Input - 
		theVolID - the unique volume ID where the target file lives.
		theDirID - the unique directory ID where the target file lives.
		theNamePtr - pointer to the name of the target file.
		nameEncoding - script the given name is in.
		theForkNamePtr - pointer to the name of the fork to open.
		theOptions - call options 
	Output -
		NA. 
	Result - 
		file descriptor (always > 0) or -1 on error.
 */

int OpenFork_VDI( fsvolid_t theVolID,
                  u_long theDirID,
                  const char *theNamePtr,
                  u_long unicodeCharacterCount,
                  u_long nameEncoding,
                  const char *theForkNamePtr,
                  u_long theOptions )
{
	int				myFD = 0;
    int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];
    int				myError;
    int				flags;

	/* Check input parms */
    if ( (theForkNamePtr == NULL) || (theNamePtr == NULL) || (unicodeCharacterCount != 0) ||
      	 (theDirID == 0) || (theVolID == 0) )
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;
		
	if ( DoValidateForkName( theForkNamePtr, nameEncoding ) != 0 )
        goto InvalidArgument;
	
	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theNamePtr)) == 0) {
        namePtr = theNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};

	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the open request down to the appropriate file system plug in */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, theForkNamePtr );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	flags = (theOptions & kOpenReadOnly) ? (O_RDONLY) : (O_RDWR);
	if (theOptions & kNoMapFS) flags |= O_NO_MFS;

	/* Turn off mapfs for resource forks */
    if (! strcmp(theForkNamePtr, "rsrc"))
        flags |= O_NO_MFS;

	/* Open the file */
	myFD = open( &myPath[0], flags, 0 );
	if ( myFD == -1 )
	{
		myError = errno;
		/*
		 * isofs doesn't know about data/rsrc forks.
		 * to allow data forks to open, we need to retry
		 * without the "/data" suffix.
		 */
		if (myError == ENOTDIR && strcmp( theForkNamePtr, kHFSDataForkName ) == 0) {
			int pathlen = strlen(myPath);

			if (pathlen > 5) {
				myPath[pathlen - 5] = '\0';		/* strip "/data" off the end of path */
				myFD = open( &myPath[0], flags, 0 );	/* try again */

				if ( myFD == -1 ) {
					myError = errno;
					goto ErrorExit;
				} else {
					myError = 0;
					goto StdExit;
				}
			}
		}
		goto ErrorExit;
	}
    myError = 0;
	goto StdExit;

InvalidArgument:
	myError = EINVAL;

ErrorExit:
    myFD = -1;

StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

	return myFD;

} /* OpenFork_VDI */


/*
 ************************************** ExchangeFiles_VDI *************************************
	Purpose -
		This routine implements the outter most layer of exchanging data in a pair of files 
		using a volume ID, Directory ID, and file name.  Some housekeeping is done then a path 
		is built for use in the Volume File System layer.  After we build a path for each file 
		we do an exchangedata system call with said path which will route the exchangedata 
		request to the appropriate file system plug in.  
	Input - 
		theVolID - the unique volume ID where the target file lives.
		theDirID1 - the unique directory ID where the target file lives.
		theName1Ptr - pointer to the name of the target file.
		nameEncoding - script the given name is in.
		theDirID2 - the unique directory ID where the target file lives.
		theName2Ptr - pointer to the name of the target file.
		theOptions - call options 
	Output -
		NA. 
	Result - 
		returns whatever exchangedata returns or PPD - what????.
 */

int	ExchangeFiles_VDI( 	fsvolid_t theVolID,
                        u_long theDirID1,
                        const char *theName1Ptr,
                        u_long unicodeCharacterCount1,
                        u_long nameEncoding,
                        u_long theDirID2,
                        const char *theName2Ptr,
                        u_long unicodeCharacterCount2,
                        u_long theOptions )
{
	int				myError;
    int				requiredPathBufferSize;
	char			localName1Buffer[LOCALNAMEBUFFERSIZE];
	char *			name1BufferPtr = NULL;
    const char *	name1Ptr;
	int				allocatedName1Buffer = 0;
	char			localName2Buffer[LOCALNAMEBUFFERSIZE];
	char *			name2BufferPtr = NULL;
    const char *	name2Ptr;
	int				allocatedName2Buffer = 0;
	char			myPath1[PATH_MAX];	
	char			myPath2[PATH_MAX];	

	/* Check input parms */
	if ( (theName1Ptr == NULL) || (theName2Ptr == NULL) ||
         (unicodeCharacterCount1 != 0) || (unicodeCharacterCount2 != 0) ||
      	 (theDirID1 == 0) || (theDirID2 == 0) || (theVolID == 0) )
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
	
	/* See if either of the incoming pathnames contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theName1Ptr)) == 0) {
        name1Ptr = theName1Ptr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			name1BufferPtr = localName1Buffer;
		} else {
			name1BufferPtr = malloc(requiredPathBufferSize);
			if (name1BufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
            allocatedName1Buffer = 1;
		};
        ToggleSeparators( theName1Ptr, nameEncoding, nameEncoding, name1BufferPtr );
        name1Ptr = name1BufferPtr;
	};

    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theName2Ptr)) == 0) {
        name2Ptr = theName2Ptr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			name2BufferPtr = localName2Buffer;
		} else {
			name2BufferPtr = malloc(requiredPathBufferSize);
			if (name2BufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedName2Buffer = 1;
		};
        ToggleSeparators( theName2Ptr, nameEncoding, nameEncoding, name2BufferPtr );
        name2Ptr = name2BufferPtr;
	};

	/* Build VolFS path - the volume file system layer will convert this to a UNIX path then */
	/* pass the open request down to the appropriate file system plug in */
	DoBuildVolFSPath( &myPath1[0], theVolID, theDirID1, name1Ptr, nameEncoding, NULL );
	DoBuildVolFSPath( &myPath2[0], theVolID, theDirID2, name2Ptr, nameEncoding, NULL );
					  
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* exchange the data */
    myError = exchangedata( &myPath1[0], &myPath2[0] );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedName1Buffer) free(name1BufferPtr);
	if (allocatedName2Buffer) free(name2BufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* ExchangeFiles_VDI */



/*
 *************************************** GetCatalogInfo_VDI ***********************************
	Purpose -
		This routine implements the outter most layer of the code that returns info about a
		a file or directory.  
	Input - 
		theVolID - the unique volume ID where the target file or directory lives.
		theDirID - the unique directory ID where the target file or directory lives.
		theItemNamePtr - pointer to the name of the target file or directory.
		nameEncoding - script the given name is in.
		theOptions - call options 
	Output -
		theCatInfoPtr - info about a file or directory 
	Result - 
		PPD - what do we return here????
 */

int GetCatalogInfo_VDI(	fsvolid_t theVolID,
                        u_long theDirID,
                        const char *theItemNamePtr,
                        u_long unicodeCharacterCount,
                        u_long nameEncoding,
                        u_long theOptions,
                        CatalogAttributeInfoPtr theCatInfoPtr,
                        FSTextEncodingFormat nameEncodingFormat,
                        void* nameBuffer,
                        size_t nameBufferSize )
{
    int				myError;
    struct attrlist	myAttrList;
	int				requiredPathBufferSize;
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
    char			myPath[PATH_MAX];
    GetCatAttributeBuffer attrReturnBuffer;

    /* Check input parms */
    if ( (theVolID == 0) ||
         (theDirID == 0) ||
         (unicodeCharacterCount != 0) ||
         (theCatInfoPtr == NULL) ||
         ((theOptions & kReturnObjectName) && (nameEncodingFormat != kMacOSHFSFormat)) )
      {
        goto InvalidArgument;
      }

    if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;

    if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;

	nameBufferPtr = malloc(NAME_MAX+1);
	if (nameBufferPtr == NULL) {
		myError = ENOMEM;
		goto ErrorExit;
	};

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theItemNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theItemNamePtr)) == 0) {
        namePtr = theItemNamePtr;
	} else {
		if (requiredPathBufferSize > (NAME_MAX+1)) {
			myError = ENAMETOOLONG;
			goto ErrorExit;
		}

        ToggleSeparators( theItemNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
    };
	
    /* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
    /* the appropriate file system */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, NULL );

    /* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
    DoSetFileSystemOptions( theOptions & kOSOptionsMask );

    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= ATTR_GCI_COMMONBITMAP;
    if (theOptions & kReturnObjectName) {
    	myAttrList.commonattr |= ATTR_CMN_NAME;
    };
    myAttrList.volattr 		= 0;
    myAttrList.dirattr 		= ATTR_GCI_DIRECTORYBITMAP;
    myAttrList.fileattr 	= ATTR_GCI_FILEBITMAP;
    myAttrList.forkattr 	= 0;

try_again:
    myError = getattrlist( myPath, &myAttrList, &attrReturnBuffer, sizeof(attrReturnBuffer) );
	if (myError != 0) {
		myError = errno;
		/*
		 * On HFS Plus volumes we must retry with additional encodings in
		 * case the supplied text encoding was inappropriate...
		 */
		if (!ISHFSSTD(theVolID) && (myError == ENOENT) && (namePtr != NULL) ) {
			if (nameEncoding == kTextEncodingMacRoman) {
				if (HasNonASCIIChars(namePtr))
					nameEncoding = kTextEncodingMacJapanese;
				else
					goto ErrorExit;	/* plain ASCII was already tried */
			} else if (nameEncoding == kTextEncodingMacJapanese) {
				nameEncoding = kTextEncodingMacRoman;
			} else {
				goto ErrorExit;
			}
    		DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, NULL );
			namePtr = NULL;	/* only retry once */
			goto try_again;
		}
		goto ErrorExit;
	};

    if (theOptions & kReturnObjectName) {
		DoUnPackAttributeBuffer( &attrReturnBuffer.na.ca, theCatInfoPtr);
		/* Copy the translated name string into the user's buffer: */
		ToggleSeparators( GET_ATTR_REF_ADDR(attrReturnBuffer.na.objectName),
                              attrReturnBuffer.na.ca.c.nameEncoding,
                              attrReturnBuffer.na.ca.c.nameEncoding,
							  nameBufferPtr );
			
		if (ISHFSSTD(theVolID) && (attrReturnBuffer.na.ca.c.nameEncoding != kTextEncodingMacRoman)) {
#if DEBUG
				printf("GetCatalogInfo: bad text encoding %ld\n", attrReturnBuffer.na.ca.c.nameEncoding);
#endif
				attrReturnBuffer.na.ca.c.nameEncoding = kTextEncodingMacRoman;
		}
		ConvertUTF8ToCString(nameBufferPtr,	/* always in UTF-8 */
								attrReturnBuffer.na.ca.c.nameEncoding,	/* original encoding */
								theCatInfoPtr->c.objectID,
						 		nameBufferSize,
						 		(char *)nameBuffer);
		//	ToggleSeparatorsInPlace((char *)nameBuffer);
	} else {
		DoUnPackAttributeBuffer( &attrReturnBuffer.a.ca, theCatInfoPtr);
	};

	/*
	 * BandAid fix for ISO 9660 CDs...
	 * The name is not always filled in so we check
	 * if its missing and go get it using readdir.
	 *
	 * This should eventually be fixed inside ISO implemenation!
	 */
	if ((theOptions & kReturnObjectName) &&
		(((char*)nameBuffer)[0] == '\0') &&
		(theCatInfoPtr->c.parentDirID != 0)) {

		DIR *directory;
		struct dirent *entry;
		u_long targetFileID = theCatInfoPtr->c.objectID;

		directory = OpenDir_VDI(theVolID, theCatInfoPtr->c.parentDirID, NULL, 0, 0, 0);
		if (directory != NULL) {
            while ((entry = readdir(directory)) != NULL) {
				if (entry->d_fileno == targetFileID) {
					strncpy(nameBuffer, entry->d_name, nameBufferSize);
					break;
				}
            };
        	closedir(directory);
		}
	}

    myError = 0;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;

ErrorExit:
StdExit:
	if (nameBufferPtr != NULL)
		free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* GetCatalogInfo */

#if INCLUDEOLDROUTINESTUBS
int GetCatalogInfo(	fsvolid_t theVolID,
                      u_long theDirID,
                      const char *theItemNamePtr,
                      u_long nameEncoding,
                      u_long theOptions,
                      CatalogAttributeInfoPtr theCatInfoPtr,
                      void* nameBuffer,
                      size_t nameBufferSize ) {
   return GetCatalogInfo_VDI(theVolID,
                             theDirID,
                             theItemNamePtr,
                             0,
                             nameEncoding,
                             theOptions,
                             theCatInfoPtr,
                             kMacOSHFSFormat,
                             nameBuffer,
                             nameBufferSize);
}
#endif

/*
 *************************************** SetCatalogInfo_VDI ***********************************
	Purpose -
		This routine implements the outter most layer of the code that sets info about a
		a file or directory.  
	Input - 
		theVolID - the unique volume ID where the target file or directory lives.
		theDirID - the unique directory ID where the target file or directory lives.
		theItemNamePtr - pointer to the name of the target file or directory.
		nameEncoding - script the given name is in.
		theOptions - call options
        newInfoPtr - info about a file or directory that we're setting
	Output -
		NA 
	Result - 
		PPD - what do we return here????
 */

int SetCatalogInfo_VDI(	fsvolid_t theVolID,
                        u_long theDirID,
                        const char *theItemNamePtr,
                        u_long unicodeCharacterCount,
                        u_long nameEncoding,
                        u_long theOptions,
                        CatalogAttributeInfoPtr newInfoPtr )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
	char			myPath[PATH_MAX];
    struct attrlist	myAttrList;
    void 			*myBuffPtr,*currBuff;
    struct SetCommAttrBuf mySetCommAttrBuf;
    u_long			attrBufSize;

    /* Check input parms */
    if ( (theVolID == 0) || (theDirID == 0) || (unicodeCharacterCount != 0) || (newInfoPtr == NULL) ||
         (newInfoPtr->fileBitmap != 0) ||
         (newInfoPtr->directoryBitmap != 0) )
    {
        goto InvalidArgument;
    }
        
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theItemNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theItemNamePtr)) == 0) {
        namePtr = theItemNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theItemNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
	/* the appropriate file system */
	DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, NULL );

	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );

    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= newInfoPtr->commonBitmap;
    myAttrList.volattr 		= 0;
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;
	
    myBuffPtr = currBuff = &mySetCommAttrBuf;
    DoPackCommonAttributes(	&myAttrList, &newInfoPtr->c, &currBuff);
    attrBufSize = (u_long)currBuff - (u_long)(myBuffPtr);

    myError = setattrlist( &myPath[0], &myAttrList, myBuffPtr, attrBufSize );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* SetCatalogInfo_VDI */

#if INCLUDEOLDROUTINESTUBS
int SetCatalogInfo(	fsvolid_t theVolID,
                       u_long theDirID,
                       const char *theItemNamePtr,
                       u_long nameEncoding,
                       u_long theOptions,
                       CatalogAttributeInfoPtr newInfoPtr ) {
    return SetCatalogInfo_VDI(theVolID,
                              theDirID,
                              theItemNamePtr,
                              0,
                              nameEncoding,
                              theOptions,
                              newInfoPtr);
}
#endif


/*
*************************************** ForkReserveSpace_VDI *********************************
       Purpose -
               This routine implements the outter most layer of the code that prepares for a future
               allocation of disk space.  Some housekeeping is done then we make a fcntl call to do
               the real work.
       Input -
               theFD - file descriptor of target file.
               theReservationSize - size we want to reserve.
               theOptions - call options
       Output -
               theSpaceAvailablePtr - amount of space that is available
       Result -
               PPD - what do we return here????
*/


int	ForkReserveSpace_VDI(	int theFD,
                                               off_t theReservationSize,
                                               u_long theOptions,
                                               off_t *theSpaceAvailablePtr )
{
       int				myError;
       int				myCmd;
       fstore_t				alloc_struct;

       /* Set the space acquired Size to 0 right away */

       *theSpaceAvailablePtr = 0;

       /* Check input parms */
       if ( theSpaceAvailablePtr == NULL || theFD == 0 || theReservationSize == 0 )
               goto InvalidArgument;

       if ( DoValidateOptions( theOptions ) != 0 )
               goto InvalidArgument;

       /* set up the alloc structure for our fcmd */

       alloc_struct.fst_posmode = F_PEOFPOSMODE;
       alloc_struct.fst_offset = 0;
       alloc_struct.fst_length = theReservationSize;

       alloc_struct.fst_flags = 0;
	
       if (theOptions & kContiguous)
		{
      		 alloc_struct.fst_flags |= F_ALLOCATECONTIG;
		}

       if (theOptions &kAllOrNothing)
		{
	        alloc_struct.fst_flags |= F_ALLOCATEALL;    
	        }
       
       /* reserve some space */
       myCmd = F_PREALLOCATE;

	/* Note that fcntl may return an error but still set the
	  bytesalloc field in the allco_struct. */ 

   	myError = (fcntl( theFD, myCmd, (int) &alloc_struct));

	if (myError != 0) {
        myError = errno;
	}

        *theSpaceAvailablePtr = alloc_struct.fst_bytesalloc;

	goto StdExit;


InvalidArgument:
       myError = EINVAL;

/* ErrorExit: */
StdExit:
   errno = myError;
   cthread_set_errno_self(myError);

   return myError;

} /* ForkReserveSpace_VDI */

#if INCLUDEOLDROUTINESTUBS
int	ForkReserveSpace(	int theFD,
                        off_t theReservationSize,
                        u_long theOptions,
                        off_t *theSpaceAvailablePtr ) {
    return ForkReserveSpace_VDI(theFD, theReservationSize, theOptions, theSpaceAvailablePtr);
}
#endif


#if 0
/*
 ***************************************** SetFileInfo_VDI ****************************************
	Purpose -
		This routine implements the outter most layer of the code that sets info about a
		a file.  
	Input - 
		theVolID - the unique volume ID where the target file lives.
		theDirID - the unique directory ID where the target file lives.
		theFileNamePtr - pointer to the name of the target file.
		nameEncoding - script the given name is in.
		theOptions - call options 
		theForkNamePtr - pointer to the name of the fork to update.
		theInfoPtr - info about a file that we're updating
	Output -
		NA 
	Result - 
		PPD - what do we return here????
 */

int SetFileInfo_VDI( fsvolid_t theVolID,
                     u_long theDirID,
                     const char *theFileNamePtr,
                     u_long unicodeCharacterCount,
                     u_long nameEncoding,
                     u_long theOptions,
                     const char *theForkNamePtr,
                     CatalogAttributeInfoPtr theInfoPtr )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
    struct attrlist	myAttrList;
	char			myPath[PATH_MAX];
    void 			*myBuffPtr,*currBuff;
    struct SetCommAttrBuf mySetCommAttrBuf;
    u_long			attrBufSize;

	/* Check input parms */
    if ( theFileNamePtr == NULL || theInfoPtr == NULL || unicodeCharacterCount != 0
		 theVolID == 0 || theDirID == 0 )
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theOptions & kDontTranslateSeparators) || (requiredPathBufferSize = PathTranslationRequired(theFileNamePtr)) == 0) {
        namePtr = theFileNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theFileNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
	/* the appropriate file system */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, theForkNamePtr );

	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= theInfoPtr->commonBitmap;
    myAttrList.volattr 		= 0;
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= theInfoPtr->fileBitmap;
    myAttrList.forkattr 	= 0;
	
    myBuffPtr = currBuff = &mySetCommAttrBuf;
    DoPackCommonAttributes(	&myAttrList, &theInfoPtr->c, &currBuff);
    attrBufSize = (u_long)currBuff - (u_long)(&mySetCommAttrBuf);

    myError = setattrlist( &myPath[0], &myAttrList, myBuffPtr, attrBufSize );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(EINVAL);

    return myError;

} /* SetFileInfo_VDI */


/*
 *************************************** SetDirectoryInfo_VDI *************************************
	Purpose -
		This routine implements the outter most layer of the code that sets info about a
		a directory.  
	Input - 
		theVolID - the unique volume ID where the target directory lives.
		theDirID - the unique directory ID where the target directory lives.
		theDirectoryNamePtr - pointer to the name of the target directory.
		nameEncoding - script the given name is in.
		theOptions - call options 
		theInfoPtr - info about a file that we're updating
	Output -
		NA 
	Result - 
		PPD - what do we return here????
 */

int SetDirectoryInfo_VDI(fsvolid_t theVolID,
                         u_long theDirID,
                         const char *theDirectoryNamePtr,
                         u_long unicodeCharacterCount,
                         u_long nameEncoding,
                         u_long theOptions,
                         CatalogAttributeInfoPtr theInfoPtr )
{
	int				myError;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char *			nameBufferPtr = NULL;
    const char *	namePtr;
    struct attrlist	myAttrList;
	char			myPath[PATH_MAX];
    void 			*myBuffPtr,*currBuff;
    struct SetCommAttrBuf mySetCommAttrBuf;
    u_long			attrBufSize;

	/* Check input parms */
    if (theInfoPtr == NULL || theVolID == 0 || theDirID == 0 || unicodeCharacterCount != 0)
	{
		goto InvalidArgument;
	}
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;
		
	if ( DoValidateScriptCode( nameEncoding ) != 0 )
		goto InvalidArgument;

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theDirectoryNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theDirectoryNamePtr)) == 0) {
        namePtr = theDirectoryNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theDirectoryNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
	/* the appropriate file system */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, NULL );
	
	/* Set the script code and options for this call */
	DoSetFileNameScript( nameEncoding );
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved	= 0;
    myAttrList.commonattr	= theInfoPtr->commonBitmap;
    myAttrList.volattr 	= 0;
    myAttrList.dirattr 	= theInfoPtr->directoryBitmap;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;

    myBuffPtr = currBuff = &mySetCommAttrBuf;
    DoPackCommonAttributes(	&myAttrList, &theInfoPtr->c, &currBuff);
    attrBufSize = (u_long)currBuff - (u_long)(&mySetCommAttrBuf);

    myError = setattrlist( &myPath[0], &myAttrList, myBuffPtr, attrBufSize );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(EINVAL);

    return myError;

} /* SetDirectoryInfo_VDI */

#endif


/*
 **************************************** GetVolumeInfo_VDI ***********************************
	Purpose -
		This routine implements the outter most layer of the code that returns info about a
		volume.  
	Input - 
		PPD - how are the input parms used here????  If index is specified is name and ID
		valid?  What is the parm precedence???
		theVolumeIndex - index into our list of volumes.
		theVolID - the unique volume ID of the target volume.
		theVolumeNamePtr - pointer to the name of the target volume.
		nameEncoding - script the given name is in.
		theOptions - call options 
	Output -
		theInfoPtr - info about target volume 
	Result - 
		PPD - what do we return here????
 */

int GetVolumeInfo_VDI( long theVolumeIndex,
                       fsvolid_t theVolID,
                       const char *theVolumeNamePtr,
                       u_long unicodeCharacterCount,
                       u_long nameEncoding,
                       u_long theOptions,
                       VolumeAttributeInfoPtr theInfoPtr )
{
    int				myError;
    struct attrlist	myAttrList;
    char			myPath[PATH_MAX];
    VolInfoReturnBuf *myBuffer = NULL;
    fsvolid_t		volumeID;

    /* Check input parms */
    if ( theInfoPtr == NULL || (unicodeCharacterCount != 0) || (theVolID == 0 && theVolumeIndex <= 0) )
        goto InvalidArgument;

    if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;

	if (theVolumeIndex > 0)		/* by index takes precedence */
		{
		myError = GetVolumeIDFromIndex(theVolumeIndex, &volumeID);

		if (myError != 0) goto ErrorExit;

        assert(volumeID != 0);
		/* XXX could also use UTF-8 as theNameEncoding, faster? */
        nameEncoding = kTextEncodingMacRoman;	/* ascii numbers are always MacRoman */
		}
	else
		{
		volumeID = theVolID;

		if ( DoValidateScriptCode( nameEncoding ) != 0 )
			goto InvalidArgument;
		}

    /* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
    DoSetFileSystemOptions( theOptions & kOSOptionsMask );

    /* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
    /* the appropriate file system */
    DoBuildVolFSPath( &myPath[0], volumeID, 0, NULL, 0, NULL );

    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= ATTR_GCI_COMMONBITMAP;
    myAttrList.volattr 		= ATTR_GVI_VOLUMEBITMAP;
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;

    myBuffer = malloc (sizeof(*myBuffer));
    if (myBuffer == NULL) {
        myError = ENOMEM;
        goto ErrorExit;
    };

    myError = getattrlist( &myPath[0], &myAttrList, myBuffer, sizeof(*myBuffer) );
	if (myError != 0) {
		myError = errno;
        goto ErrorExit;
	};

    DoUnpackVolumeInfoBuffer(myBuffer, theInfoPtr );

    myError = 0;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
    if (myBuffer) free (myBuffer);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* GetVolumeInfo_VDI */

#if INCLUDEOLDROUTINESTUBS
int GetVolumeInfo( long theVolumeIndex,
                      fsvolid_t theVolID,
                      const char *theVolumeNamePtr,
                      u_long nameEncoding,
                      u_long theOptions,
                      VolumeAttributeInfoPtr theInfoPtr ) {
    return GetVolumeInfo_VDI(theVolumeIndex,
                             theVolID,
                             theVolumeNamePtr,
                             0,
                             nameEncoding,
                             theOptions,
                             theInfoPtr);
}
#endif

/*
 **************************************** SetVolumeInfo_VDI ***********************************
	Purpose -
		This routine implements the outter most layer of the code that updates info about a
		volume.  
	Input - 
		theVolID - the unique volume ID of the target volume.
		theOptions - call options 
		theInfoPtr - info about target volume 
	Output -
		NA 
	Result - 
		PPD - what do we return here????
 */

int SetVolumeInfo_VDI( fsvolid_t theVolID,
                       u_long theOptions,
                       VolumeAttributeInfoPtr theInfoPtr )
{
	int				myError;
    struct attrlist	myAttrList;
    char			myPath[PATH_MAX];
    VolInfoChangeBuf myBuffer;

	/* Check input parms */
	if ( theInfoPtr == NULL || theVolID == 0 )
		goto InvalidArgument;
		
	if ( DoValidateOptions( theOptions ) != 0 )
		goto InvalidArgument;

	/* Check the attributes marked for change: XXX PPD someday this code should require SOME bits set... */
	if ((theInfoPtr->commonBitmap & ~ATTR_SVI_COMMONBITMAP) ||
		(theInfoPtr->volumeBitmap & ~(ATTR_VOL_INFO | ATTR_VOL_NAME))) {
		goto InvalidArgument;
	};
	
	/* If the name is being changed the encoding MUST be specified with it: */
	if ((theInfoPtr->volumeBitmap & ATTR_VOL_NAME) && !(theInfoPtr->commonBitmap & ATTR_CMN_SCRIPT)) {
		goto InvalidArgument;
	};
	
	/* Set the options for this call */
	DoSetFileSystemOptions( theOptions & kOSOptionsMask );
	
	/* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
	/* the appropriate file system */
	DoBuildVolFSPath( &myPath[0], theVolID, 0, NULL, 0, NULL );
	
    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    /* For now, supply a default attribute set if none is specified at all: */
    if ((theInfoPtr->commonBitmap == 0) && (theInfoPtr->volumeBitmap == 0)) {
	    myAttrList.commonattr	= ATTR_SVI_COMMONBITMAP;	/* Common bits to be changed */
	    myAttrList.volattr 		= 0;						/* No volume-specific fields are set by default */
	} else {
	    myAttrList.commonattr	= theInfoPtr->commonBitmap;	/* Common bits to be changed */
	    myAttrList.volattr 		= theInfoPtr->volumeBitmap | ATTR_VOL_INFO;	/* Volume-specific fields to be changed */
	};
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;

    DoPackVolumeInfoBuffer( theInfoPtr, &myBuffer );
		
    myError = setattrlist( &myPath[0], &myAttrList, &myBuffer, sizeof(VolInfoReturnBuf) );
	if (myError != 0) myError = errno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
/* ErrorExit: */
StdExit:    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* SetVolumeInfo_VDI */

#if INCLUDEOLDROUTINESTUBS
int SetVolumeInfo( fsvolid_t theVolID,
                   u_long theOptions,
                   VolumeAttributeInfoPtr theInfoPtr ) {
    return SetVolumeInfo_VDI(theVolID, theOptions, theInfoPtr);
}
#endif


#if 0
/*
 ************************************** GetNamedAttribute_VDI *********************************
	Purpose -
		This routine ?????? - PPD - what does this routine do???????.  
	Input - 
		theVolID - the unique volume ID of the target volume.
		theItemID - PPD - ???? 
		theOptions - call options 
		theAttributeNamePtr - pointer to name of attribute we want. 
		theAttributeNameLength - length of attribute name. 
		theBufferSize - size of buffer theBufferPtr points to. 
	Output -
		theBufferPtr - PPD - ????? 
		theSizeReturnedPtr - PPD - ????? 
	Result - 
		PPD - what do we return here????
 */

int GetNamedAttribute_VDI( fsvolid_t theVolID,
                           u_long theItemID,
                           u_long theOptions,
                           const void *theAttributeNamePtr,
                           size_t theAttributeNameLength,
                           void *theBufferPtr,
                           size_t theBufferSize,
                           size_t *theSizeReturnedPtr )
{
    errno = EINVAL;
    cthread_set_errno_self(EINVAL);
	return -1;

} /* GetNamedAttribute_VDI */
#endif


#if 0
/*
 ************************************** SetNamedAttribute_VDI *********************************
	Purpose -
		This routine ?????? - PPD - what does this routine do???????.  
	Input - 
		theVolID - the unique volume ID of the target volume.
		theItemID - ????? 
		theOptions - call options 
		theAttributeNamePtr - pointer to name of attribute we want. 
		theAttributeNameLength - length of attribute name. 
		theBufferPtr - ????? 
		theBufferSize - size of buffer theBufferPtr points to. 
	Output -
		NA
	Result - 
		PPD - what do we return here????
 */

int SetNamedAttribute_VDI( fsvolid_t theVolID,
                           u_long theItemID,
                           u_long theOptions,
                           const void *theAttributeNamePtr,
                           size_t theAttributeNameLength,
                           void *theBufferPtr,
                           size_t theBufferSize )
{
    errno = EINVAL;
    cthread_set_errno_self(EINVAL);
	return -1;

} /* SetNamedAttribute_VDI */
#endif


/*
 ***************************************** SearchCatalog_VDI **********************************
	Purpose -
		VDI entry point to the searchfs() system call. 
	Input - 
		theVolID	- the unique volume ID of the target volume.
		theOptions	- call options 
		searchPB	- search parameters, buffers, etc.
	Output -
		searchPB->buffer			- contains an array of searchPB->numFoundMatches
									  catalogInfo structures.
		searchPB->numFoundMatches	- Number of matches in buffer
		searchPB->state				- private state of last element searched
	Result - 
		0			- search is complete
		EAGAIN		- search has not yet completed and must be called again to continue
 		ENOBUFS		- buffer supplied is too small for at least one result entry
 */

#define	kTempBufferSize		(32 * 1024)		//	32K buffer

#define kMaxSearchInfoEntrySize		(sizeof(SearchInfoReturn) + MAXHFSNAMESIZE)


int SearchCatalog_VDI( fsvolid_t volumeID, u_long options, SearchParam *searchPB )
{
	struct fssearchblock fsSearchBlock;
	struct attrlist		returnAttrList;				//	We coerce it to the same as GetCatInfo for VDI layer
	struct searchstate	lastState;
	char				myPath[PATH_MAX];
	void				*attrptr;
	void				*varptr;
	void				*matchesBuffer;
	long				matchesBufferSize;
	u_long				fixedblocksize;
	u_long				matchesFound;
	u_long				unpackedRecords;
	u_long				nameEncoding;
	int					myError;

    fsSearchBlock.searchparams1 = NULL;
    fsSearchBlock.returnbuffer = NULL;
    if ( (volumeID == 0) ||
         (searchPB == NULL) ||
         (searchPB->unicodeCharacterCount != 0) ||
         (searchPB->nameEncodingFormat != kMacOSHFSFormat) ||
         (searchPB->maxMatches <= 0) ||
         (searchPB->buffer == NULL) ) {
        goto InvalidArgument;
    };

	//	Validate input parameters
	if ( options & ~SRCHFS_VALIDOPTIONSMASK ) {
        goto InvalidArgument;
    };

	if ( DoValidateScriptCode( searchPB->targetNameEncoding ) != 0 ) {
        goto InvalidArgument;
    };

	/*
	 * On HFS volumes ignore the nameEncoding field (unless its already utf8)
	 * and always interpret the name (byte stream) as if its MacRoman.
	 */
	nameEncoding = searchPB->targetNameEncoding;
	if ( (nameEncoding != kTextEncodingUTF8) && ISHFSSTD(volumeID) )
		nameEncoding = kTextEncodingMacRoman;
	
    /* The two catalog attribute blocks had better include valid copies of the same fields
       (except possibly the ATTR_CMN_NAME bit, which is ignored in the second block)...
     */
    if ( ((searchPB->searchInfo2->commonBitmap & ~ATTR_CMN_NAME) != (searchPB->searchInfo1->commonBitmap & ~ATTR_CMN_NAME)) ||
         (searchPB->searchInfo2->directoryBitmap != searchPB->searchInfo1->directoryBitmap) ||
         (searchPB->searchInfo2->fileBitmap != searchPB->searchInfo1->fileBitmap) ) {
        goto InvalidArgument;
    };

	searchPB->numFoundMatches = 0;
	matchesBuffer = searchPB->buffer;
	matchesBufferSize = searchPB->bufferSize;

	if ( matchesBufferSize < kMaxSearchInfoEntrySize ) {
		myError = ENOBUFS;
		goto ErrorExit;
	};

	/* No need to translate any pathnames: the target is specified by volID alone. */
	
	//	Build VolFS path - the volume file system layer will convert this to a UNIX path in the appropriate file system */
    DoBuildVolFSPath( myPath, volumeID, 0, NULL, nameEncoding, NULL );
	
	//	Set the script code and options for this call
    DoSetFileNameScript( nameEncoding );

	fsSearchBlock.searchattrs.bitmapcount	= ATTR_BIT_MAP_COUNT;
	fsSearchBlock.searchattrs.reserved		= 0;
	fsSearchBlock.searchattrs.commonattr	= searchPB->searchInfo1->commonBitmap;
	fsSearchBlock.searchattrs.volattr 		= 0;
    fsSearchBlock.searchattrs.dirattr 		= searchPB->searchInfo1->directoryBitmap;
    fsSearchBlock.searchattrs.fileattr 		= searchPB->searchInfo1->fileBitmap;
	fsSearchBlock.searchattrs.forkattr 		= 0;
	fsSearchBlock.searchparams1				= NULL;
	fsSearchBlock.returnbuffer				= NULL;

	//	Set up the returnAttrList, same as GetCatInfo()
	fsSearchBlock.returnattrs				= &returnAttrList;
	returnAttrList.bitmapcount				= ATTR_BIT_MAP_COUNT;
	returnAttrList.reserved					= 0;
	returnAttrList.commonattr				= ATTR_GCI_COMMONBITMAP | ATTR_CMN_NAME;
	returnAttrList.volattr 					= 0;
	returnAttrList.dirattr 					= ATTR_GCI_DIRECTORYBITMAP;
	returnAttrList.fileattr 				= ATTR_GCI_FILEBITMAP;
	returnAttrList.forkattr 				= 0;
	
	//	Prepare for packing
	fsSearchBlock.sizeofsearchparams1 = fixedblocksize = VDIAttributeBlockSize( &fsSearchBlock.searchattrs );
	if ( fsSearchBlock.searchattrs.commonattr & ATTR_CMN_NAME )
		fsSearchBlock.sizeofsearchparams1 += NAME_MAX;
	
	//
	//	Pack up searchPB->searchInfo1
	//
	fsSearchBlock.sizeofsearchparams2	= fsSearchBlock.sizeofsearchparams1;
	fsSearchBlock.searchparams1	= malloc( fsSearchBlock.sizeofsearchparams1 + fsSearchBlock.sizeofsearchparams2 );
	
	attrptr	= fsSearchBlock.searchparams1;
	varptr	= fsSearchBlock.searchparams1 + fixedblocksize;										//	Point to variable-length storage

	*((u_long *)attrptr)++	= 0;																//	Reserve space for length field
	
    DoPackSearchInfo( &fsSearchBlock.searchattrs, searchPB->searchInfo1, searchPB->targetNameString,
					nameEncoding, &attrptr, &varptr);	//	Pack it up!

	*((u_long *)fsSearchBlock.searchparams1)	= (varptr - fsSearchBlock.searchparams1);		//	Store length of fixed + var block
	
	//
	//	Pack up searchPB->searchInfo2
	//
	fsSearchBlock.searchparams2	= fsSearchBlock.searchparams1 + fsSearchBlock.sizeofsearchparams1;
	
	attrptr	= fsSearchBlock.searchparams2;
	varptr	= fsSearchBlock.searchparams2 + fixedblocksize;										//	Point to variable-length storage

	*((u_long *)attrptr)++	= 0;																//	Reserve space for length field
	
	DoPackSearchInfo( &fsSearchBlock.searchattrs, searchPB->searchInfo2, NULL, 0, &attrptr, &varptr );	//	Pack it up!

	*((u_long *)fsSearchBlock.searchparams2)	= (varptr - fsSearchBlock.searchparams2);		//	Store length of fixed + var block

	//
	//	Allocate a 32K buffer we use to transfer packed data to the users buffer
	//
	fsSearchBlock.returnbuffer = malloc( kTempBufferSize );									
	if ( fsSearchBlock.returnbuffer == NULL ) {
        myError = errno;
        goto ErrorExit;
    };

	//
	//	Call the System Trap
	//

	fsSearchBlock.maxmatches = MIN(searchPB->maxMatches, (matchesBufferSize / kMaxSearchInfoEntrySize));
	fsSearchBlock.timelimit.tv_sec	= searchPB->timeLimit;		// XXX still need to implement this : = startTime + searchPB->timeLimit - currentTime;

	do {
		matchesFound = 0;
		fsSearchBlock.returnbuffersize = MIN(matchesBufferSize, kTempBufferSize);
	
		/*
		 * save a copy of current state (in case we need to try again)
		 */
		bcopy(&searchPB->state, &lastState, sizeof(struct searchstate));

        myError = searchfs( myPath, &fsSearchBlock, &matchesFound, nameEncoding, options, &searchPB->state );
		if ( myError == -1 ) myError = errno;					//	system trap only returns 0 or -1, so check global

		if ( (matchesFound > 0) && ((myError == 0) || (myError == EAGAIN)) ) {
			/*
			 * copy the packed info back into the users buffer
			 */
			DoUnPackSearchResults( fsSearchBlock.returnbuffer, &matchesBuffer, &matchesBufferSize, matchesFound, &unpackedRecords );

			/*
			 * if we did not successfully unpacked all the matches,
			 * then we need to try again using the same search
			 * state but with maxmatches constrained to the amount
			 * of records we can successfully unpack.
			 */
			if (unpackedRecords < matchesFound) {
				bcopy(&lastState, &searchPB->state, sizeof(struct searchstate));
				fsSearchBlock.maxmatches = unpackedRecords;
				continue;
			}
			
			searchPB->numFoundMatches += unpackedRecords;

			if (searchPB->timeLimit > 0)	/* if caller set _any_ time limit then yield */
				break;

			if (searchPB->numFoundMatches >= searchPB->maxMatches)
				break;
				
			if (matchesBufferSize < kMaxSearchInfoEntrySize)	/* enough for another pass? */
				break;
		}

		options &= ~ SRCHFS_START;				//	clear the SRCHFS_START bit for continuing where we left off
		fsSearchBlock.maxmatches = MIN ( searchPB->maxMatches - searchPB->numFoundMatches,
										 matchesBufferSize / kMaxSearchInfoEntrySize);

	} while ( myError == EAGAIN );

    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if ( fsSearchBlock.searchparams1 != NULL ) free ( fsSearchBlock.searchparams1 );
	if ( fsSearchBlock.returnbuffer != NULL ) free ( fsSearchBlock.returnbuffer );
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;
}	//	SearchCatalog_VDI

#if INCLUDEOLDROUTINESTUBS
int SearchCatalog( fsvolid_t volumeID, u_long options, SearchParam *searchPB ) {
    return SearchCatalog_VDI(volumeID, options, searchPB);
};
#endif


#if 0
/*
 *************************************** GetDirEntryAttr *************************************
	Purpose -
		This routine ?????? - PPD - what does this routine do???????.  
	Input - 
		don't know yet.
	Output -
		don't know yet.
	Result - 
		don't know yet.
 */

int GetDirEntryAttr( void )
{
    errno = EINVAL;
    cthread_set_errno_self(EINVAL);
	return -1;

} /* GetDirEntryAttr */
#endif

/*
 ***************************************** CreateFileID_VDI **************************************
	Purpose -
		This routine creates a FileID for the given object and return it to the caller.  
	Input - 
		theVolID - the unique volume ID of the target volume.
		theDirID - DirID for parent directory
		theNamePtr - name of target file
		nameEncoding - ????? 
		theOptions - unused
		theFileIDPtr - pointer to resulting fileID

	Output -
		theFileIDPtr - place to put the FileID of the specified object

	Result - 
		Error code or zero if successfully completed.
 */

int CreateFileID_VDI ( fsvolid_t theVolID,
                       u_long theDirID,
                       const char *theNamePtr,
                       u_long unicodeCharacterCount,
                       u_long nameEncoding,
                       u_long theOptions,
                       u_long *theFileIDPtr )
{
    int				myError;
    struct attrlist	myAttrList;
	int				requiredPathBufferSize;
	int				allocatedNameBuffer = 0;
	char			localNameBuffer[LOCALNAMEBUFFERSIZE];
	char			*nameBufferPtr = NULL;
    const char *	namePtr;
    char			myPath[PATH_MAX];
    CreateFileIDReturnBuf attrReturnBuffer;

	*theFileIDPtr = 0;				/* Just in case of errors... */
	
    /* Check input parms */
    if ( theVolID == 0 || theDirID == 0 || unicodeCharacterCount != 0 || theFileIDPtr == NULL )
      {
        goto InvalidArgument;
      }

    if ( DoValidateOptions( theOptions ) != 0 )
        goto InvalidArgument;

    if ( DoValidateScriptCode( nameEncoding ) != 0 )
        goto InvalidArgument;

	/* See if the incoming pathname contains colons or slashes that need translation
	   and construct a translated pathname if any are encountered.
	 */
    if ((theNamePtr == NULL) ||
        (theOptions & kDontTranslateSeparators) ||
        (requiredPathBufferSize = PathTranslationRequired(theNamePtr)) == 0) {
        namePtr = theNamePtr;
	} else {
		if (requiredPathBufferSize <= LOCALNAMEBUFFERSIZE) {
			nameBufferPtr = localNameBuffer;
		} else {
			nameBufferPtr = malloc(requiredPathBufferSize);
			if (nameBufferPtr == NULL) {
				myError = ENOMEM;
				goto ErrorExit;
			};
			allocatedNameBuffer = 1;
		};
        ToggleSeparators( theNamePtr, nameEncoding, nameEncoding, nameBufferPtr );
        namePtr = nameBufferPtr;
	};
	
    /* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
    /* the appropriate file system */
    DoBuildVolFSPath( &myPath[0], theVolID, theDirID, namePtr, nameEncoding, NULL );

    /* Set the script code and options for this call */
    DoSetFileNameScript( nameEncoding );
    DoSetFileSystemOptions( theOptions & kOSOptionsMask );

    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= ATTR_CMN_OBJPERMANENTID;
    myAttrList.volattr 		= 0;
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;

    myError = getattrlist( myPath, &myAttrList, &attrReturnBuffer, sizeof(attrReturnBuffer) );
	if (myError != 0) myError = errno;

    if (myError) goto ErrorExit;

	/* Copy the resulting FileID out to the caller: */
    assert(attrReturnBuffer.permanentID.fid_generation == 0);
    *theFileIDPtr = attrReturnBuffer.permanentID.fid_objno;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
	if (allocatedNameBuffer) free(nameBufferPtr);
    
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* CreateFileID_VDI */



/*
 ***************************************** ResolveFileID_VDI **********************************
	Purpose -
		This routine converts a vol/file id to a path.  
	Input - 
		theVolID - the unique volume ID of the target volume.
		theFileID - file ID we want to resolve. 
		nameEncoding - ????? 
	Output -
		theDirIDPtr - place to put the directory ID of the resolved file's parent 
		theFileNamePtr - place to put the name of the resolved file.
		theOrigScriptCodePtr - place to put the original script code of the file name. 
	Result - 
		PPD - what do we return here????
 */

int	ResolveFileID_VDI( fsvolid_t theVolID,
                       u_long theFileID,
                       FSTextEncodingFormat nameEncodingFormat,
                       u_long *theDirIDPtr,
                       void* nameBuffer,
                       size_t nameBufferSize,
                       u_long *theOrigNameEncodingPtr )
{
    int				myError;
    FileIDResolveBuf myAttrBuffer;
    char			myPath[PATH_MAX];
    char			tmpname[NAME_MAX];
    struct attrlist	myAttrList;

    /* Check input parms */
    if ( (nameEncodingFormat != kMacOSHFSFormat) ||
         (theVolID == 0) || (theFileID == 0) || (theDirIDPtr == NULL) ||
         (nameBuffer == NULL) )
        {
        goto InvalidArgument;
        }

    /* Build VolFS path - the volume file system layer will convert this to a UNIX path in */
    /* the appropriate file system */
    // PPD - how do we resolve file IDs here??
    DoBuildVolFSPath( &myPath[0], theVolID, theFileID, NULL, 0, NULL );

    myAttrList.bitmapcount	= ATTR_BIT_MAP_COUNT;
    myAttrList.reserved		= 0;
    myAttrList.commonattr	= ATTR_CMN_NAME | ATTR_CMN_PAROBJID | ATTR_CMN_SCRIPT;
    myAttrList.volattr 		= 0;
    myAttrList.dirattr 		= 0;
    myAttrList.fileattr 	= 0;
    myAttrList.forkattr 	= 0;

    myError = getattrlist( &myPath[0], &myAttrList, &myAttrBuffer, sizeof(myAttrBuffer) );
	if (myError != 0) {
		myError = errno;
        goto ErrorExit;
    };

	*theDirIDPtr = myAttrBuffer.parentDirID.fid_objno;

    assert( GET_ATTR_REF_ADDR(myAttrBuffer.name) == myAttrBuffer.nameBuf );
//  assert( ((daddr_t)(&myAttrBuffer.name) + myAttrBuffer.name.attr_dataoffset) == &myAttrBuffer.nameBuf );
	
	/* Translate any colons in the returned name to slashes: */
    ToggleSeparators( myAttrBuffer.nameBuf, myAttrBuffer.nameEncoding, myAttrBuffer.nameEncoding, tmpname);
    if (theOrigNameEncodingPtr) *theOrigNameEncodingPtr = myAttrBuffer.nameEncoding;

	if (ISHFSSTD(theVolID) && (myAttrBuffer.nameEncoding != kTextEncodingMacRoman))
		myAttrBuffer.nameEncoding = kTextEncodingMacRoman;

	ConvertUTF8ToCString(tmpname, /* always in UTF-8 */
						 myAttrBuffer.nameEncoding,	/* XXX should come from client */
						 theFileID,
						 nameBufferSize,
						 (char *)nameBuffer);
//	ToggleSeparatorsInPlace();

	/*
	 * BandAid fix for ISO 9660 CDs...
	 * The name is not always filled in so we check
	 * if its missing and go get it using readdir.
	 *
	 * This should eventually be fixed inside ISO implemenation!
	 */

	if ((((char*)nameBuffer)[0] == '\0') && (*theDirIDPtr != 0)) {
		DIR *directory;
		struct dirent *entry;

		directory = OpenDir_VDI(theVolID, *theDirIDPtr, NULL, 0, 0, 0);
		if (directory != NULL) {
            while ((entry = readdir(directory)) != NULL) {
				if (entry->d_fileno == theFileID) {
					strncpy(nameBuffer, entry->d_name, nameBufferSize);
					break;
				}
             };
        	closedir(directory);
		}
	}

    myError = 0;
    goto StdExit;

InvalidArgument:
	myError = EINVAL;
	
ErrorExit:
StdExit:
    errno = myError;
    cthread_set_errno_self(myError);

    return myError;

} /* ResolveFileID */



#if INCLUDEOLDROUTINESTUBS
int	ResolveFileID( fsvolid_t theVolID,
                      u_long theFileID,
                      u_long *theDirIDPtr,
                      void* nameBuffer,
                      size_t nameBufferSize,
                      u_long *theOrigNameEncodingPtr ) {
    return ResolveFileID_VDI(theVolID,
                             theFileID,
                             kMacOSHFSFormat,
                             theDirIDPtr,
                             nameBuffer,
                             nameBufferSize,
                             theOrigNameEncodingPtr);
};
#endif

/******************************************************************************************************/
/*                                                                                                    */
/*********************************** I N T E R N A L   R O U T I N E S ********************************/
/*                                                                                                    */
/******************************************************************************************************/



/*
 *********************************** GetVolumeIDFromIndex ********************************
	Purpose -
 	Derive a volume id from volfs from an index.   
	Input - 
		volumeIndex - which volume wanted
	Output -
		volID - pointer to the volID 
	Result - 
 */
static int GetVolumeIDFromIndex(long volumeIndex, fsvolid_t *volID)
{
	struct dirent	*dp;
	DIR				*dfd;
    long			index;
    int				result;
    
    *volID = 0;		/* in case we fail */

	dfd = opendir(VOLFS_PREFIX);
	if (dfd == NULL)
		{
        printf(" GetVolumeIDFromIndex: Could not open %s for reading!\n", VOLFS_PREFIX);
		return (errno);
		}
	
	index = 0;
	while ((dp = readdir(dfd)) != NULL)
		{
		if (dp->d_name[0] == '.')
			continue;	/* skip self (.) and parent (..) or anything starting with a dot */
		
		index++;	/* count all non-dot entries */

		if (index == volumeIndex)
			{
			char	*check_ptr;
			*volID = strtol(dp->d_name, &check_ptr, 10);
			
			if ((check_ptr - dp->d_name) != strlen(dp->d_name))	/* make sure strtol used all the characters! */
				printf(" GetVolumeIDFromIndex: name '%s' is not a number?!\n", dp->d_name);

			break;
			}
		}
	
	if (dp == NULL)
	{
        result = ENOENT;	/* ran out of entries! */
		errno = ENOENT;
		cthread_set_errno_self(ENOENT);
	}
	else
		result = 0;

	(void) closedir(dfd);
	
	return result;
}


/*
 *********************************** DoUnpackVolumeInfoBuffer ********************************
	Purpose -
 	This routine will move the contents from a VolInfoReturnBuf, to a getVInfoBufPtr.  
	Input - 
		theBufferPtr - pointer to buffer we're to extract info from
	Output -
		theInfoPtr - pointer to buffer we're to place extracted info. 
	Result - 
		NA.
 */


static void DoUnpackVolumeInfoBuffer(	VolInfoReturnBuf* theBufferPtr,
										VolumeAttributeInfoPtr theInfoPtr )
{
	if ( theBufferPtr == NULL || theInfoPtr == NULL )
		return;
    
    theInfoPtr->volumeID 		= theBufferPtr->c.fsid.val[0];			
    theInfoPtr->nameEncoding 	= theBufferPtr->c.nameEncoding;
    theInfoPtr->creationDate 	= theBufferPtr->c.creationTime;
    theInfoPtr->lastModDate 	= theBufferPtr->c.lastModificationTime;
    theInfoPtr->lastBackupDate 	= theBufferPtr->c.lastBackupTime;
    theInfoPtr->attributes 		= 0;
    theInfoPtr->numAllocBlocks 	= (u_long)(theBufferPtr->v.size / theBufferPtr->v.minallocation);
    theInfoPtr->allocBlockSize 	= (u_long)theBufferPtr->v.minallocation;
    theInfoPtr->nextItemID 		= 0;			// Next Dir or file ID
    theInfoPtr->numUnusedBlocks = (u_long)(theBufferPtr->v.spacefree / theBufferPtr->v.minallocation);
    theInfoPtr->signature 		= theBufferPtr->v.signature;
    theInfoPtr->numFiles 		= theBufferPtr->v.filecount;		// Total files on volume
    theInfoPtr->numDirectories 	= theBufferPtr->v.dircount;			// Total directories on volume
    
    memcpy (theInfoPtr->finderInfo, theBufferPtr->c.finderInfo, sizeof(theInfoPtr->finderInfo));
	
	if (theInfoPtr->signature == (u_long) 0x4244 && theInfoPtr->nameEncoding != kTextEncodingMacRoman) {
#if DEBUG
		printf("GetVolumeInfo: bad text encoding %ld\n", theInfoPtr->nameEncoding);
#endif
		theInfoPtr->nameEncoding = kTextEncodingMacRoman;
	}

	ConvertUTF8ToCString(GET_ATTR_REF_ADDR(theBufferPtr->v.volumename),	/* always in UTF-8 */
						 theInfoPtr->nameEncoding,						/* into what it was originally */
						 2,
						 theBufferPtr->v.volumename.attr_length + 1,
						 theInfoPtr->name);
#if 1
    ToggleSeparatorsInPlace(theInfoPtr->name);
#endif
    theInfoPtr->mountFlags      = theBufferPtr->v.mountflags;
    if (theBufferPtr->v.mountflags & MNT_RDONLY) {
        theInfoPtr->attributes |= kVolumeReadOnly;
    };
	return;
	
} /* DoUnpackVolumeInfoBuffer */


/*
 ************************************ DoPackVolumeInfoBuffer **********************************
	Purpose -
 		This routine will copy a getVInfoBufPtr to a VolInfoReturnBuf.  
	Input - 
		theInfoPtr - pointer to catalog info for volume attributes we're updating. 
	Output -
		theBufferPtr - pointer to buffer we're to fill attributes into
	Result - 
		NA.
 */

static void DoPackVolumeInfoBuffer(	VolumeAttributeInfoPtr theInfoPtr,
                                    VolInfoChangeBuf* theBufferPtr )
{
	attrgroup_t a;
	void *attrbufptr;
	
	/* PPD - do something here */
	if ( theBufferPtr == NULL || theInfoPtr == NULL )
		return;
	
	a = theInfoPtr->commonBitmap;
	attrbufptr = theBufferPtr;
	
	if (a & ATTR_CMN_SCRIPT) {
		*(text_encoding_t *)attrbufptr = theInfoPtr->nameEncoding;
		++((text_encoding_t *)attrbufptr);
	};
    if (a & ATTR_CMN_CRTIME) {
        *(struct timespec *)attrbufptr = theInfoPtr->creationDate;
        ++((struct timespec *)attrbufptr);
    };
    if (a & ATTR_CMN_MODTIME) {
        *(struct timespec *)attrbufptr = theInfoPtr->lastModDate;
        ++((struct timespec *)attrbufptr);
    };
    if (a & ATTR_CMN_BKUPTIME) {
        *(struct timespec *)attrbufptr = theInfoPtr->lastBackupDate;
        ++((struct timespec *)attrbufptr);
    };
    if (a & ATTR_CMN_FNDRINFO) {
        bcopy (theInfoPtr->finderInfo, attrbufptr, sizeof(theInfoPtr->finderInfo));
        attrbufptr += sizeof(theInfoPtr->finderInfo);
    };
    
	if (theInfoPtr->volumeBitmap & ATTR_VOL_NAME) {
        struct attrreference *volumeNameRef;
		
        volumeNameRef = (struct attrreference *)attrbufptr;
		++((struct attrreference *)attrbufptr);
		/* XXX force nameEncoding for hfs disks */
		ConvertCStringToUTF8(theInfoPtr->name, theInfoPtr->nameEncoding,
							 sizeof(theBufferPtr->volumeName), (char*)attrbufptr);
		ToggleSeparatorsInPlace((char*)attrbufptr);
	    volumeNameRef->attr_dataoffset = ((long)attrbufptr) - ((long)(volumeNameRef));
	    volumeNameRef->attr_length = strlen((char*)attrbufptr) + 1;
	};

	
	return;
	
} /* DoPackVolumeInfoBuffer */


/*
************************************* DoUnPackAttributeBuffer *********************************
  Purpose -
      This routine will copy commonAttributeInfo to the given attribute buffer.
  Input -
      theInfoPtr - pointer to catalog info common attributes we're updating.
  Output -
attrbufptr - pointer to buffer we're to fill common attributes into
  Result -
      NA.
*/

static void DoUnPackAttributeBuffer( struct catalogAttrBlock *getAttrBuf, CatalogAttributeInfoPtr theCatInfoPtr )
{

    assert(getAttrBuf != NULL);
    assert(theCatInfoPtr != NULL);

    theCatInfoPtr->c.objectType = getAttrBuf->c.objectType;
    theCatInfoPtr->c.device = getAttrBuf->c.device;
    theCatInfoPtr->c.nameEncoding = getAttrBuf->c.nameEncoding;
    theCatInfoPtr->c.objectID = getAttrBuf->c.objectID.fid_objno;	
    theCatInfoPtr->c.instance = getAttrBuf->c.objectID.fid_generation;	
    theCatInfoPtr->c.parentDirID = getAttrBuf->c.parentDirID.fid_objno;
    theCatInfoPtr->c.uid = getAttrBuf->c.uid;
    theCatInfoPtr->c.gid = getAttrBuf->c.gid;
    theCatInfoPtr->c.mode = (mode_t)getAttrBuf->c.mode;
    theCatInfoPtr->c.flags = getAttrBuf->c.flags;
    theCatInfoPtr->c.nlink = 1;
    memcpy(theCatInfoPtr->c.finderInfo, getAttrBuf->c.finderInfo, sizeof(theCatInfoPtr->c.finderInfo));
    theCatInfoPtr->c.creationTime = getAttrBuf->c.creationTime;
    theCatInfoPtr->c.lastModificationTime = getAttrBuf->c.lastModificationTime;
    theCatInfoPtr->c.lastBackupTime = getAttrBuf->c.lastBackupTime;
    theCatInfoPtr->c.lastChangeTime = getAttrBuf->c.lastChangeTime;
    if ( theCatInfoPtr->c.objectType == VDIR)   {
        theCatInfoPtr->d.numEntries = getAttrBuf->u.d.numEntries;
    }
    else {
        theCatInfoPtr->f.totalSize = getAttrBuf->u.f.totalSize;		/* size of all forks, in blocks */
        theCatInfoPtr->f.blockSize = getAttrBuf->u.f.blockSize;		/* Optimal file I/O blocksize */
        theCatInfoPtr->f.numForks = 2 /* getAttrBuf->u.f.numForks */;	/* Number of forks in the file */
        theCatInfoPtr->f.attributes = 0;								/* See IM:Files 2-100 */
        if ((getAttrBuf->c.flags & (SF_IMMUTABLE | UF_IMMUTABLE))) {
            theCatInfoPtr->f.attributes |= kObjectLocked;
        };
        theCatInfoPtr->f.dataLogicalLength = getAttrBuf->u.f.dataLogicalLength;
        theCatInfoPtr->f.dataPhysicalLength = getAttrBuf->u.f.dataPhysicalLength;
        theCatInfoPtr->f.resourceLogicalLength = getAttrBuf->u.f.resourceLogicalLength;
        theCatInfoPtr->f.resourcePhysicalLength = getAttrBuf->u.f.resourcePhysicalLength;
//    	theCatInfoPtr->f.deviceType = getAttrBuf->f.physicalDevice;
    }
}


//
//	DoPackSearchInfo()
//	Given a struct SearchInfo coerce into packable structures, and call packing routines.
//
static void DoPackSearchInfo( struct attrlist *alist, CatalogAttributeInfo *searchInfo, char *namePtr,
							  u_long nameEncoding, void** attrbufptrptr, void** varbufptrptr )
{
	attrgroup_t		a;
	u_long			attributeLength;
	void			*attributeBuffer;
	void			*variableBuffer;
	
	assert( alist != NULL );
	assert( searchInfo != NULL );
	assert( attrbufptrptr != NULL );

	attributeBuffer	= *attrbufptrptr;
	variableBuffer	= *varbufptrptr;
	
	//
	//	Pack common attributes
	//
	a				= alist->commonattr;

	if ( a != 0 )
	{
        if ( a & ATTR_CMN_NAME )
        {
            ((struct attrreference *)attributeBuffer)->attr_dataoffset	= variableBuffer - attributeBuffer;	//	offset from current position
        	if (namePtr) {
				ConvertCStringToUTF8(namePtr, nameEncoding, NAME_MAX, (char*)variableBuffer);
				ToggleSeparatorsInPlace((char*)variableBuffer);
            	attributeLength = strlen((char*)variableBuffer) + 1;
            	variableBuffer += attributeLength + ((4 - (attributeLength & 3)) & 3);				//	Advance beyond the space just allocated and round up to the next 4-byte boundary
            } else {
            	attributeLength = 0;
            };
            ((struct attrreference *)attributeBuffer)->attr_length		= attributeLength;
            ++( (struct attrreference *) attributeBuffer );
        }
    
		if ( a & ATTR_CMN_PAROBJID )
		{
			((fsobj_id_t *)attributeBuffer)->fid_objno = searchInfo->c.parentDirID;
			((fsobj_id_t *)attributeBuffer)->fid_generation = 0;
			++((fsobj_id_t *)attributeBuffer);
		}
		
        if ( a & ATTR_CMN_CRTIME )		*((struct timespec *)attributeBuffer)++	= searchInfo->c.creationTime;
        if ( a & ATTR_CMN_MODTIME )		*((struct timespec *)attributeBuffer)++	= searchInfo->c.lastModificationTime;
        if ( a & ATTR_CMN_BKUPTIME )	*((struct timespec *)attributeBuffer)++	= searchInfo->c.lastBackupTime;

		if ( a & ATTR_CMN_FNDRINFO )
		{
            bcopy( searchInfo->c.finderInfo, attributeBuffer, sizeof(searchInfo->c.finderInfo) );
            attributeBuffer += ( sizeof(searchInfo->c.finderInfo) );
		}
	}

	//
	//	Pack Directory attributes
	//
	a				= alist->dirattr;

	if ( a != 0 )
	{
        if ( a & ATTR_DIR_ENTRYCOUNT )	*((u_long *)attributeBuffer)++	= searchInfo->d.numEntries;
	}
	
	//
	//	Pack File attributes
	//
	a				= alist->fileattr;

	if ( a != 0 )
	{
		if ( a & ATTR_FILE_DATALENGTH )		*((off_t *)attributeBuffer)++ = searchInfo->f.dataLogicalLength;
		if ( a & ATTR_FILE_DATAALLOCSIZE )	*((off_t *)attributeBuffer)++ = searchInfo->f.dataPhysicalLength;
		if ( a & ATTR_FILE_RSRCLENGTH )		*((off_t *)attributeBuffer)++ = searchInfo->f.resourceLogicalLength;
		if ( a & ATTR_FILE_RSRCALLOCSIZE )	*((off_t *)attributeBuffer)++ = searchInfo->f.resourcePhysicalLength;
	}
	
	*attrbufptrptr	= attributeBuffer;
	*varbufptrptr	= variableBuffer;
}


//
//	DoUnPackSearchResults()
//	Given a packed buffer, unpack it into the described buffer
//	Since we know the format of the packed array, there is no reason to pass in attrlist
//
static	void	DoUnPackSearchResults( void *packedBuffer, void **targetBuffer, u_long *targetBufferSize, u_long numPackedRecords, u_long *numTargetRecords )
{
	SearchInfoReturnPtr	matchedEntry;
    NamedGetCatAttrBuf	*packedAttrBuffer;
	u_long				i;
	u_long				blocksize;
	u_long				numRecords = 0;
	u_long				spaceUsed = 0;
	
    packedAttrBuffer	= (NamedGetCatAttrBuf *) packedBuffer;						//	This attribute record
	matchedEntry		= (SearchInfoReturnPtr) *targetBuffer;						//	This cat info struct in users return buffer
	
	for ( i = 0 ; i < numPackedRecords ; i++ ) {
		blocksize = sizeof(SearchInfoReturn) + MAXHFSNAMESIZE;
		
		/*
		 * Make sure there's enough room in the user's buffer to return
		 * all the matches we've already captured in the search buffer...
		 */
		if (blocksize > (*targetBufferSize - spaceUsed)) break;

		DoUnPackAttributeBuffer( &packedAttrBuffer->ca, &matchedEntry->ci );							//	copy common attributes
		matchedEntry->ci.commonBitmap = ATTR_GCI_COMMONBITMAP | ATTR_CMN_NAME;
		if (matchedEntry->ci.c.objectType == VDIR) {
            matchedEntry->ci.directoryBitmap = ATTR_GCI_DIRECTORYBITMAP;
            matchedEntry->ci.fileBitmap = 0;
		} else {
            matchedEntry->ci.directoryBitmap = 0;
            matchedEntry->ci.fileBitmap = ATTR_GCI_FILEBITMAP;
		};
        matchedEntry->name = ((char *)&matchedEntry->name) + sizeof(matchedEntry->name);
		ConvertUTF8ToCString(GET_ATTR_REF_ADDR(packedAttrBuffer->objectName),	/* always in UTF-8 */
						 packedAttrBuffer->ca.c.nameEncoding,					/* into what it was originally */
						 packedAttrBuffer->ca.c.objectID.fid_objno,
						 MAXHFSNAMESIZE,	/* XXX should be passed by client [and blocksize computation adjusted accordingly]! */
						 matchedEntry->name);
		ToggleSeparatorsInPlace(matchedEntry->name);

		/*
		 * Adjust size for actual name length (after conversion from UTF-8)
		 * The length of the name after conversion will always be less than
		 * or equal to the 32-byte worst case we prepared for:
		 */
		blocksize -= (MAXHFSNAMESIZE) - (strlen(matchedEntry->name) + 1);
		blocksize += ((4 - (blocksize & 3)) & 3);								/* Round up to nearest longword boundary */
		matchedEntry->size = blocksize;

		numRecords++;
		spaceUsed += blocksize;
		(char *)matchedEntry += blocksize;
        packedAttrBuffer	= (NamedGetCatAttrBuf*) ( ((char*) packedAttrBuffer) + packedAttrBuffer->size);	//	start of next attribute record
	}

	/*
	 * if we successfully unpacked them all, then bump the
	 * user's buffer.  Otherwise SearchCatalog will try
	 * again with maxmatches set to numRecords
	 */
	if (numRecords == numPackedRecords) {
		(char *)(*targetBuffer) += spaceUsed;
		*targetBufferSize	-= spaceUsed;
	}

	*numTargetRecords = numRecords;
}



/************************************** DoPackCommonAttributes *********************************
  Purpose -
	This routine will copy commonAttributeInfo to the given attribute buffer.
  Input -
	theInfoPtr - pointer to catalog info common attributes we're updating.
  Output -
	attrbufptr - pointer to buffer we're to fill common attributes into
  Result -
      NA.
*/

static void DoPackCommonAttributes(	struct attrlist *alist, struct CommonAttributeInfo *setcommattrbuf, void** attrbufptrptr )
{
   attrgroup_t a;
   void* attrbufptr;

   assert(alist != NULL);
   assert(setcommattrbuf != NULL);
   assert(attrbufptrptr != NULL);

   a = alist->commonattr;
   attrbufptr = *attrbufptrptr;

   if (a & ATTR_CMN_CRTIME) {
       *(struct timespec *)attrbufptr = setcommattrbuf->creationTime;
       ++((struct timespec *)attrbufptr);
   };
   if (a & ATTR_CMN_MODTIME) {
       *(struct timespec *)attrbufptr = setcommattrbuf->lastModificationTime;
       ++((struct timespec *)attrbufptr);
   };
   if (a & ATTR_CMN_CHGTIME) {
       *(struct timespec *)attrbufptr = setcommattrbuf->lastChangeTime;
       ++((struct timespec *)attrbufptr);
   };
   if (a & ATTR_CMN_BKUPTIME) {
       *(struct timespec *)attrbufptr = setcommattrbuf->lastBackupTime;
       ++((struct timespec *)attrbufptr);
   };
   if (a & ATTR_CMN_FNDRINFO) {
       bcopy (setcommattrbuf->finderInfo, attrbufptr, sizeof(setcommattrbuf->finderInfo));
       attrbufptr += sizeof(setcommattrbuf->finderInfo);
   };
   if (a & ATTR_CMN_OWNERID) {
       *(uid_t *)attrbufptr = setcommattrbuf->uid;
       ++((uid_t *)attrbufptr);
   };
   if (a & ATTR_CMN_GRPID) {
       *(gid_t *)attrbufptr = setcommattrbuf->gid;
       ++((gid_t *)attrbufptr);
   };
   if (a & ATTR_CMN_ACCESSMASK) {
       *(u_long *)attrbufptr = (u_long)setcommattrbuf->mode;
       ++((u_long *)attrbufptr);
   };
   if (a & ATTR_CMN_FLAGS) {
       *(u_long *)attrbufptr = (u_long)setcommattrbuf->flags;
       ++((u_long *)attrbufptr);
   };

   *attrbufptrptr = attrbufptr;
}


/*
 *************************************** DoValidateForkName ***********************************
	Purpose -
		This routine will make sure the given fork name is valid.  
	Input - 
		theForkNamePtr - pointer to the name of a fork (can be NULL???)
		nameEncoding - script the given name is in.
	Output -
		NA. 
	Result - 
		0 when theForkNamePtr is valid or -1 on error.
 */

static int DoValidateForkName(	const char *theForkNamePtr,
								u_long nameEncoding )
{
	/* PPD - is null theForkNamePtr OK??? */
	if ( theForkNamePtr == NULL )
		return 0;
		
	/* PPD - need to take into account the nameEncoding */
	if ( strcmp( theForkNamePtr, kHFSResourceForkName ) == 0 )
		return 0;
		
	if ( strcmp( theForkNamePtr, kHFSDataForkName ) == 0 )
		return 0;
		
	return -1;
	
} /* DoValidateForkName */


/*
 ************************************* DoValidateScriptCode ***********************************
	Purpose -
		This routine will make sure the given text encoding value is valid.  
	Input - 
		nameEncoding - text encoding we're to verify.
	Output -
		NA. 
	Result - 
		0 when nameEncoding is valid or -1 on error.
 */

static int DoValidateScriptCode( u_long nameEncoding )
{
	int result;

	switch (nameEncoding) {
	/* non-Unicode encodings */
	case kTextEncodingMacRoman:
	case kTextEncodingMacJapanese:
//	case kTextEncodingShiftJIS:		/* plain Shift-JIS */
//	case kTextEncodingNextStepLatin:

	/* Unicode encoding (as 16-bit or UTF-8) */
	case kTextEncodingUnicodeV2_0:
	case kTextEncodingUTF8:
		result = 0;
		break;

	default:
        printf("WARNING: invalid text encoding (%ld) provided on VDI call.\n", nameEncoding);
		result = -1;
	}

    return result;
} /* DoValidateScriptCode */


/*
 ************************************** DoValidateOptions *************************************
	Purpose -
		This routine will make sure the given file system options are valid.  
	Input - 
		theOptions - options we're to verify.
	Output -
		NA. 
	Result - 
		0 when theOptions are valid or -1 on error.
 */

static int DoValidateOptions( u_long theOptions )
{
return ( (theOptions & ~(kReturnObjectName | kDontResolveAliases | kDontTranslateSeparators | kContiguous | kAllOrNothing | kOpenReadOnly | kNoMapFS)) == 0 ) ? 0 : -1;
	
} /* DoValidateOptions */


/*
 ************************************* DoSetFileNameScript ************************************
	Purpose -
		This routine will set the file name script for processing down in the bowels of the 
		target file system.  
	Input - 
		nameEncoding - script code we want to use.
	Output -
		NA. 
	Result - 
		NA.
 */

static void DoSetFileNameScript( u_long nameEncoding )
{
	/* PPD - Do something here */
	
	if ( nameEncoding == 0 )
		return;
	return;				
	
} /* DoSetFileNameScript */


/*
 ************************************ DoSetFileSystemOptions **********************************
	Purpose -
		This routine will set the file system options for processing down in the bowels of the 
		target file system.  
	Input - 
		theOptions - the options we want to use.
	Output -
		NA. 
	Result - 
		NA.
 */

static void DoSetFileSystemOptions( u_long theOptions )
{
	if ( theOptions == 0 ) return;
		
	/* PPD - Do something here */
	
	return;				
	
} /* DoSetFileSystemOptions */


/*
 *************************************** DoBuildVolFSPath *************************************
	Purpose -
		This routine will xxxxx.  
	Input - 
		theVolID - xxxxx.
		theDirID - xxxxx.
		theNamePtr - xxxxx.
		nameEncoding - what encoding theNamePtr is in.
		theForkNamePtr - xxxxx.
	Output -
		thePathPtr - xxxx. 
	Result - 
		NA.
 */

static void DoBuildVolFSPath( 	char *thePathPtr,
								fsvolid_t theVolID,
								u_long theDirID,
								const char *theNamePtr,
								u_long nameEncoding,
								const char *theForkNamePtr )
{
	assert( thePathPtr != NULL && theVolID != 0 );

	/* PPD - do something with nameEncoding!!!! */
	
	if ( theDirID != 0 )
		sprintf(thePathPtr, "%s/%d/%ld", VOLFS_PREFIX, theVolID, theDirID);
	else
		sprintf(thePathPtr, "%s/%d", VOLFS_PREFIX, theVolID);

    if ( theNamePtr && (theNamePtr[0] != 0) ) {		/* XXX PPD Different treatment for theNamePtr[0] == 0? */
		char utf8name[256];

		/*
		 * On HFS volumes ignore the nameEncoding field (unless its already utf8)
		 * and always interpret the name (byte stream) as if its MacRoman.
		 */
		if ( (nameEncoding != kTextEncodingUTF8) && ISHFSSTD(theVolID) )
			nameEncoding = kTextEncodingMacRoman;

		ConvertCStringToUTF8(theNamePtr, nameEncoding, sizeof(utf8name), utf8name);
			
        /* Only when a real name is specified is it appended (possibly along with a fork name */
        strcat(thePathPtr, "/");
  		strcat(thePathPtr, utf8name);

        if ( theForkNamePtr ) {
            strcat(thePathPtr, "/");
            strcat(thePathPtr, theForkNamePtr);
        };
    } else {
        if (theDirID == 0) {
            strcat(thePathPtr, "/");
            strcat(thePathPtr, "@");	/* use '@' to indicate the file system root */
        } else {
            /* No name and DirID != 0; no more work required. */
        }
    };	
} /* DoBuildVolFSPath */



/*
 ************************************ PathTranslationRequired **********************************
	Purpose -
		This routine will return the number of bytes to allocate for a translated pathname
		if the pathname is longer than a specified maximum length or contains characters
		that require translation.
	Input - 
		pathNamePtr		- Pointer to pathname string to be examined
	Output -
		None.
	Result - 
		The length of the pathname to be translated or zero if no translation is needed
 */
static int PathTranslationRequired( const char * pathNamePtr ) {
	int pathNameLength = 0;
	char translationRequired = 0;
	char c;
	
	do {
		c = *pathNamePtr++;
		if ((c == ':') || (c == '/')) {
			translationRequired = 1;
		};
		++pathNameLength;
	} while (c);
	
	return translationRequired ? pathNameLength : 0; 
}


 
/*
 *************************************** ToggleSeparators *************************************
	Purpose -
		This routine will convert a UNIX(tm)-style pathname with slashes as separators and
		embedded colons to indicate slashes in filenames to an HFS-style pathname and vice-versa.

	Input - 
		SourcePathNamePtr	- Pointer to UFS pathname string to be translated
		SourcePathEncoding	- The representation of the input pathname
		TargetPathEncoding	- The desired representation of the generated pathname

	Output -
		TargetPathNamePtr	- Pointer to HFS pathname string to be generated

	Result -
		TargetPathNamePtr	- The pointer to the freshly translated pathname string
 */
 
static char * ToggleSeparators( const char * SourcePathNamePtr,
                                u_long SourcePathEncoding,
                                u_long TargetPathEncoding,
                                char * TargetPathNamePtr ) {
	char *translatedPathNamePtr = TargetPathNamePtr;
	char c;
	
	assert(SourcePathEncoding == TargetPathEncoding);

	do {
      c = *SourcePathNamePtr++;
	  switch (c) {
		case ':':
		  *translatedPathNamePtr++ = '/';
		  break;
  	  
		case '/':
		  *translatedPathNamePtr++ = ':';
		  break;
  	  
		default:
		  *translatedPathNamePtr++ = c;
	  };
	} while (c);

	return TargetPathNamePtr;
} /* ToggleSeparators */


/*
 *************************************** ToggleSeparators *************************************
	Purpose -
		This routine will convert a BSD-style pathname with slashes as separators and
		embedded colons to indicate slashes in filenames to an HFS-style pathname and
		vice-versa.
 */
static void ToggleSeparatorsInPlace(char * pathNamePtr) {
	char *p = pathNamePtr;
	char c;

	while ((c = *p)) {
		if (c == ':')
			*p = '/';
		else if (c == '/')
			*p = ':';
		++p;
	};

} /* ToggleSeparatorsInPlace */



//	This routine is an exact duplicate of the system routine AttributeBlockSize()
//	located in .../hfs/hfs_fs/hfs_vfsutils.c.
//	Both should be kept in sync with one another.
static	size_t VDIAttributeBlockSize  (struct attrlist *attrlist)
{
	int size = sizeof(u_long);	/* size field is always exists */
	attrgroup_t a;
	
#if ((ATTR_CMN_NAME			| ATTR_CMN_DEVID			| ATTR_CMN_FSID 			| ATTR_CMN_OBJTYPE 		| \
	  ATTR_CMN_OBJTAG		| ATTR_CMN_OBJID			| ATTR_CMN_OBJPERMANENTID	| ATTR_CMN_PAROBJID		| \
	  ATTR_CMN_SCRIPT		| ATTR_CMN_CRTIME			| ATTR_CMN_MODTIME			| ATTR_CMN_CHGTIME		| \
	  ATTR_CMN_ACCTIME		| ATTR_CMN_BKUPTIME			| ATTR_CMN_FNDRINFO			| ATTR_CMN_OWNERID		| \
	  ATTR_CMN_GRPID		| ATTR_CMN_ACCESSMASK		| ATTR_CMN_NAMEDATTRCOUNT	| ATTR_CMN_NAMEDATTRLIST| \
	  ATTR_CMN_FLAGS) != ATTR_CMN_VALIDMASK)
#error AttributeBlockSize: Missing bits in common mask computation!
#endif
	ASSERT((attrlist->commonattr & ~ATTR_CMN_VALIDMASK) == 0);

#if ((ATTR_VOL_FSTYPE		| ATTR_VOL_SIGNATURE		| ATTR_VOL_SIZE				| ATTR_VOL_SPACEFREE 	| \
	  ATTR_VOL_SPACEAVAIL	| ATTR_VOL_MINALLOCATION	| ATTR_VOL_ALLOCATIONCLUMP	| ATTR_VOL_IOBLOCKSIZE	| \
	  ATTR_VOL_OBJCOUNT		| ATTR_VOL_FILECOUNT		| ATTR_VOL_DIRCOUNT			| ATTR_VOL_MAXOBJCOUNT	| \
	  ATTR_VOL_MOUNTPOINT	| ATTR_VOL_NAME				| ATTR_VOL_MOUNTFLAGS		| ATTR_VOL_INFO) != ATTR_VOL_VALIDMASK)
#error AttributeBlockSize: Missing bits in volume mask computation!
#endif
	ASSERT((attrlist->volattr & ~ATTR_VOL_VALIDMASK) == 0);

#if ((ATTR_DIR_LINKCOUNT | ATTR_DIR_ENTRYCOUNT) != ATTR_DIR_VALIDMASK)
#error AttributeBlockSize: Missing bits in directory mask computation!
#endif
	ASSERT((attrlist->dirattr & ~ATTR_DIR_VALIDMASK) == 0);
#if ((ATTR_FILE_LINKCOUNT	| ATTR_FILE_TOTALSIZE		| ATTR_FILE_ALLOCSIZE 		| ATTR_FILE_IOBLOCKSIZE 	| \
	  ATTR_FILE_CLUMPSIZE	| ATTR_FILE_DEVTYPE			| ATTR_FILE_FILETYPE		| ATTR_FILE_FORKCOUNT		| \
	  ATTR_FILE_FORKLIST	| ATTR_FILE_DATALENGTH		| ATTR_FILE_DATAALLOCSIZE	| ATTR_FILE_DATAEXTENTS		| \
	  ATTR_FILE_RSRCLENGTH	| ATTR_FILE_RSRCALLOCSIZE	| ATTR_FILE_RSRCEXTENTS) != ATTR_FILE_VALIDMASK)
#error AttributeBlockSize: Missing bits in file mask computation!
#endif
	ASSERT((attrlist->fileattr & ~ATTR_FILE_VALIDMASK) == 0);

#if ((ATTR_FORK_TOTALSIZE | ATTR_FORK_ALLOCSIZE) != ATTR_FORK_VALIDMASK)
#error AttributeBlockSize: Missing bits in fork mask computation!
#endif
	ASSERT((attrlist->forkattr & ~ATTR_FORK_VALIDMASK) == 0);

	if ((a = attrlist->commonattr) != 0) {
        if (a & ATTR_CMN_NAME) size += sizeof(struct attrreference);
		if (a & ATTR_CMN_DEVID) size += sizeof(dev_t);
		if (a & ATTR_CMN_FSID) size += sizeof(fsid_t);
		if (a & ATTR_CMN_OBJTYPE) size += sizeof(fsobj_type_t);
		if (a & ATTR_CMN_OBJTAG) size += sizeof(fsobj_tag_t);
		if (a & ATTR_CMN_OBJID) size += sizeof(fsobj_id_t);
        if (a & ATTR_CMN_OBJPERMANENTID) size += sizeof(fsobj_id_t);
		if (a & ATTR_CMN_PAROBJID) size += sizeof(fsobj_id_t);
		if (a & ATTR_CMN_SCRIPT) size += sizeof(text_encoding_t);
		if (a & ATTR_CMN_CRTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_MODTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_CHGTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_ACCTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_BKUPTIME) size += sizeof(struct timespec);
		if (a & ATTR_CMN_FNDRINFO) size += 32 * sizeof(char);
		if (a & ATTR_CMN_OWNERID) size += sizeof(uid_t);
		if (a & ATTR_CMN_GRPID) size += sizeof(gid_t);
		if (a & ATTR_CMN_ACCESSMASK) size += sizeof(u_long);
		if (a & ATTR_CMN_NAMEDATTRCOUNT) size += sizeof(u_long);
		if (a & ATTR_CMN_NAMEDATTRLIST) size += sizeof(struct attrreference);
		if (a & ATTR_CMN_FLAGS) size += sizeof(u_long);
	};
	if ((a = attrlist->volattr) != 0) {
		if (a & ATTR_VOL_FSTYPE) size += sizeof(u_long);
		if (a & ATTR_VOL_SIGNATURE) size += sizeof(u_long);
		if (a & ATTR_VOL_SIZE) size += sizeof(off_t);
		if (a & ATTR_VOL_SPACEFREE) size += sizeof(off_t);
		if (a & ATTR_VOL_SPACEAVAIL) size += sizeof(off_t);
		if (a & ATTR_VOL_MINALLOCATION) size += sizeof(off_t);
		if (a & ATTR_VOL_ALLOCATIONCLUMP) size += sizeof(off_t);
		if (a & ATTR_VOL_IOBLOCKSIZE) size += sizeof(size_t);
		if (a & ATTR_VOL_OBJCOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_FILECOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_DIRCOUNT) size += sizeof(u_long);
		if (a & ATTR_VOL_MAXOBJCOUNT) size += sizeof(u_long);
        if (a & ATTR_VOL_MOUNTPOINT) size += sizeof(struct attrreference);
        if (a & ATTR_VOL_NAME) size += sizeof(struct attrreference);
        if (a & ATTR_VOL_MOUNTFLAGS) size += sizeof(u_long);
	};
	if ((a = attrlist->dirattr) != 0) {
		if (a & ATTR_DIR_LINKCOUNT) size += sizeof(u_long);
		if (a & ATTR_DIR_ENTRYCOUNT) size += sizeof(u_long);
	};
	if ((a = attrlist->fileattr) != 0) {
		if (a & ATTR_FILE_LINKCOUNT) size += sizeof(u_long);
		if (a & ATTR_FILE_TOTALSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_ALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_IOBLOCKSIZE) size += sizeof(size_t);
		if (a & ATTR_FILE_CLUMPSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_DEVTYPE) size += sizeof(u_long);
		if (a & ATTR_FILE_FILETYPE) size += sizeof(u_long);
		if (a & ATTR_FILE_FORKCOUNT) size += sizeof(u_long);
		if (a & ATTR_FILE_FORKLIST) size += sizeof(struct attrreference);
		if (a & ATTR_FILE_DATALENGTH) size += sizeof(off_t);
		if (a & ATTR_FILE_DATAALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_DATAEXTENTS) size += sizeof(extentrecord);
		if (a & ATTR_FILE_RSRCLENGTH) size += sizeof(off_t);
		if (a & ATTR_FILE_RSRCALLOCSIZE) size += sizeof(off_t);
		if (a & ATTR_FILE_RSRCEXTENTS) size += sizeof(extentrecord);
	};
	if ((a = attrlist->forkattr) != 0) {
		if (a & ATTR_FORK_TOTALSIZE) size += sizeof(off_t);
		if (a & ATTR_FORK_ALLOCSIZE) size += sizeof(off_t);
	};

	return size;
}


static int
HasNonASCIIChars(const char *string)
{
	char c;
	
	do {
		c = *string++;
		if (c < 0)
			return (1);
	} while (c);

	return (0);
}

