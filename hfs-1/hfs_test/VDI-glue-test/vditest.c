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
/*	@(#)vditest.c
*
*	(c) 1998 Apple Computer, Inc.  All Rights Reserved
*
*	HISTORY
*	15-Jul-1998	Don Brady		More updates to ForkReserveSpace tests.
*	13-Jul-1998	Don Brady		Updated SearchCatalog test to match actual searchfs semantics.
*	04-Jul-1998	CHW			Updated ForkReserveSpace test to include some error cases.
*	25-jun-1988	CHW			Added ForkReserveSpace test
*	11-may-1998	Don Brady		Add duplicate file Create_VDI test.
*	21-apr-1998	Don Brady		Use time() function for SearchCatalogTest dates.
*
*/


#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/attr.h>
#include <sys/fcntl.h>

#include "../../hfs_glue/vol.h"

#define DEFAULTOPENMODE 0
#define DEFAULTCREATEMODE (DEFFILEMODE)


static void Simple_VDI_Test(fsvolid_t volID);
static void printresult(int result);
static void printcatinfo(const CatalogAttributeInfo *objCatalogInfo);
static void printvolinfo(const VolumeAttributeInfo *vip);
static void printtime(const time_t * time);
static void printobject(const SearchInfoReturn * obj);

static	int	SearchCatalogTest(fsvolid_t volID);
static	int	ExchangeFileIDTest(fsvolid_t volID, u_long dirID);
static	int	ForkReserveSpaceTest(fsvolid_t volID, u_long dirID);
static void VolumeAttributeTest(fsvolid_t volID);


int VDITestMain(int argc, const char *argv[])
{
    long				volumeIndex;
    VolumeAttributeInfo	volInfo;
    int					result;

	printf("Begin of VDITest\n");

	/* for each volume mounted, call Simple_VDI_Test */

    volumeIndex = 1;
    while (volumeIndex <= 16) {		/* just look at the first 16 for now */
    
    	result = GetVolumeInfo_VDI(volumeIndex, 0, NULL, 0, 0, 0, &volInfo);
    	if (result != 0 && errno == ENOENT)
    		break;	/* all done */

		volumeIndex++;	/* we found one */

	   	if (result != 0)
    	{
        	printf("  GetVolumeInfo_VDI failed for volume index %ld (result = %d, errno = %d)\n", volumeIndex-1, result, errno);

    		if (result != 0 && errno == EOPNOTSUPP)
    			continue;	/* skip this volume */
		}

        printf(" VDITestMain: found volume '%s' (id = %d)\n", volInfo.name, volInfo.volumeID);
		printvolinfo(&volInfo);

		Simple_VDI_Test(volInfo.volumeID);
	}

    if (volumeIndex == 1)
        printf(" VDITestMain: cound not find any mounted volumes!\n");

    printf("End of VDITest\n\n");

	return 0;
}


static void Simple_VDI_Test(fsvolid_t volID)
{
    int					result = -1;
    u_long				options = 0;
    u_long				dirID;
	CatalogAttributeInfo objCatalogInfo;
    char				filename1[] = "VDI-testfile";
    char				filename2[] = "VDI-testfilerenamed";
    char				foldername[] = "VDI-testdir";

    dirID = 2;	/* just use root directory */

	VolumeAttributeTest(volID);

    printf("  cleaning up old test files...");
    (void) RemoveDir_VDI(volID, dirID, foldername, 0, kTextEncodingMacRoman, options);
    (void) Remove_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, options);
    (void) Remove_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, options);
    printf("  done.\n");

    printf("  calling MakeDir_VDI (volID = %d, dirID = %ld, name = %s)...", volID, dirID, foldername);
    result = MakeDir_VDI(volID, dirID, foldername, 0, kTextEncodingMacRoman, options, ACCESSPERMS);
    printresult(result);

    if (result == 0) {
		char objName[255] = {0};

        printf("  calling GetCatalogInfo (volID = %d, dirID = %ld, name = '%s')...", volID, dirID, foldername);
        result = GetCatalogInfo_VDI(volID, dirID, foldername, 0, kTextEncodingMacRoman, kReturnObjectName, &objCatalogInfo, kMacOSHFSFormat, objName, sizeof(objName));
    	printresult(result);
        printf("  %s's directoryID = %ld...\n", foldername, objCatalogInfo.c.objectID);
    	if (result == 0) {
            printf("  calling GetCatalogInfo (volID = %d, objID = %ld, name = NULL)...", volID, objCatalogInfo.c.objectID);
            result = GetCatalogInfo_VDI(volID,
                                        objCatalogInfo.c.objectID,
                                        NULL,
                                        0,
                                        kTextEncodingMacRoman,
                                        kReturnObjectName,
                                        &objCatalogInfo,
                                        kMacOSHFSFormat,
                                        objName,
                                        sizeof(objName));
            printf("   got back name '%s'\n", objName);
		}
		
		//	SearchCatalog
	//	(void) SearchCatalogTest( volID );

        printf("  calling RemoveDir_VDI (volID = %d, dirID = %ld, name = %s)...", volID, dirID, foldername);
        result = RemoveDir_VDI(volID, dirID, foldername, 0, kTextEncodingMacRoman, options);
        printresult(result);
	}

    printf("  calling Create_VDI (volID = %d, dirID = %ld, name = %s)...", volID, dirID, filename1);
    result = Create_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, options, DEFAULTCREATEMODE);
    printresult(result);

    printf("  calling Create_VDI to create duplicate...\n");
    result = Create_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, options, DEFAULTCREATEMODE);
    if ((result != EEXIST) && ((result == 0) || (errno != EEXIST)))
		printf(" Expected errno to be EEXISTS (17), instead got result %d (errno = %d)\n", result, errno);

	if (result < 0) {
		printf(" VDI is not happy so I'm giving up!\n");
	} else {
		char * filename;

        printf("  calling Rename_VDI (volID = %d, dirID = %ld, '%s', '%s')...", volID, dirID, filename1, filename2);
        result = Rename_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, dirID, filename2, 0, options);
        printresult(result);

		if (result != 0)
            filename = filename1;
		else
            filename = filename2;

		objCatalogInfo.commonBitmap	= 0;
		objCatalogInfo.directoryBitmap	= 0;
		objCatalogInfo.fileBitmap = 0;

        printf("  calling GetCatalogInfo (volID = %d, dirID = %ld, name = %s)...", volID, dirID, filename);
        result = GetCatalogInfo_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, options, &objCatalogInfo, kMacOSHFSFormat, NULL, 0);
        printresult(result);
        if (result == 0)
			printcatinfo(&objCatalogInfo);

        if (result == 0) {
            objCatalogInfo.commonBitmap	= ATTR_CMN_BKUPTIME | ATTR_CMN_MODTIME;
        	objCatalogInfo.directoryBitmap	= 0;
        	objCatalogInfo.fileBitmap		= 0;

            objCatalogInfo.c.lastModificationTime.tv_sec += 15;
            objCatalogInfo.c.lastBackupTime.tv_sec = objCatalogInfo.c.lastModificationTime.tv_sec;

			printf("  calling SetCatalogInfo_VDI (volID = %d, dirID = %ld, name = %s)...", volID, dirID, filename);
            result = SetCatalogInfo_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, options, &objCatalogInfo);
            printresult(result);

			if (result == 0) {
                objCatalogInfo.commonBitmap	= 0;
                objCatalogInfo.directoryBitmap	= 0;
                objCatalogInfo.fileBitmap		= 0;

                objCatalogInfo.c.lastModificationTime.tv_sec = 0;
                objCatalogInfo.c.lastBackupTime.tv_sec = 0;

                printf("  calling GetCatalogInfo (volID = %d, dirID = %ld, name = %s)...", volID, dirID, filename);
                result = GetCatalogInfo_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, options, &objCatalogInfo, kMacOSHFSFormat, NULL, 0);
                printresult(result);
                if (result == 0)
                    printcatinfo(&objCatalogInfo);
			}
		}

        printf("  calling Remove_VDI (volID = %d, dirID = %ld, name = %s)...", volID, dirID, filename);
        result = Remove_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, options);
        printresult(result);
		
        ExchangeFileIDTest(volID, dirID);
        ForkReserveSpaceTest(volID,dirID);
	}
	printf("\n");

}


#define SEARCHTESTFILENAME "Search-Test-XYZZY"
static	int	SearchCatalogTest(fsvolid_t volID)
{
	int					result;
	int					i;
	SearchParam			searchPB;
    CatalogAttributeInfo searchInfo1;
    CatalogAttributeInfo searchInfo2;
    char				targetName[512];
	SearchInfoReturn	*foundObjectInfo;
	u_long				options		= SRCHFS_START | SRCHFS_MATCHDIRS | SRCHFS_MATCHFILES;
	u_long				matches;

	printf( "\n*** Begin SearchCatalog_VDI Test ***\n" );

    printf("Cleaning up old test files...\n");
    (void) Remove_VDI(volID, 2, SEARCHTESTFILENAME, 0, kTextEncodingMacRoman, 0);
    printf("Done.\n");

    memset( &searchPB, 0, sizeof(SearchParam) );

    searchPB.searchInfo1						= &searchInfo1;
    searchPB.searchInfo2						= &searchInfo2;
    searchPB.targetNameString					= targetName;
    searchPB.unicodeCharacterCount				= 0;
    searchPB.targetNameEncoding					= kTextEncodingMacRoman;
	searchPB.bufferSize							= 75 * 1024;	//	75K buffer
	searchPB.buffer								= malloc( searchPB.bufferSize );
	searchPB.maxMatches							= 10;
    searchPB.nameEncodingFormat					= kMacOSHFSFormat;
	searchPB.timeLimit							= 100;
    searchInfo1.commonBitmap					= ATTR_CMN_CRTIME;
    searchInfo1.fileBitmap						= 0;
    searchInfo1.directoryBitmap					= 0;
    searchInfo2.commonBitmap					= ATTR_CMN_CRTIME;
    searchInfo2.fileBitmap						= 0;
    searchInfo2.directoryBitmap					= 0;

    result = Create_VDI(volID, 2, SEARCHTESTFILENAME, 0, kTextEncodingMacRoman, 0, DEFAULTCREATEMODE);
    if (result) {
        err(errno, "creating file '%s' for SearchCatalog_VDI test (result = %d, errno = %d)", SEARCHTESTFILENAME, result, errno);
    };
	//
	//	Search By Creation Date
	//
    time((time_t*) &searchInfo2.c.creationTime.tv_sec);		// Get the current time
    searchInfo1.c.creationTime.tv_sec = searchInfo2.c.creationTime.tv_sec - 30;
	searchInfo2.c.creationTime.tv_sec += 30;

    printf("\tSearchCatalog_VDI by create date between ");
	printtime((time_t*) &searchInfo1.c.creationTime);
	printf(" and ");
	printtime((time_t*) &searchInfo2.c.creationTime);
	printf("\n");

	do 
	{
		result = SearchCatalog_VDI( volID, options, &searchPB );
		options &= ~SRCHFS_START;							//	continue search

		printf( "\tresult=%d\tnumFoundMatches=%ld\n\tSearchCatalog_VDI Results:\n", result, searchPB.numFoundMatches );
	
	    foundObjectInfo = (SearchInfoReturnPtr)searchPB.buffer;
	
		for ( i=0 ; i < searchPB.numFoundMatches ; i++ )
		{
			printobject(foundObjectInfo);
	        foundObjectInfo = (SearchInfoReturnPtr)((char *)foundObjectInfo + foundObjectInfo->size);
		}

	} while ( result == EAGAIN );
	
	
	//
	//	Search By Partial Name
	//
	options										= SRCHFS_START | SRCHFS_MATCHPARTIALNAMES | SRCHFS_MATCHDIRS | SRCHFS_MATCHFILES;
	searchPB.numFoundMatches					= 0;
	searchInfo2.commonBitmap = searchInfo1.commonBitmap = ATTR_CMN_NAME;
    strcpy( targetName, "DeepTest" );			//	Search for all objects containing the word "Test"

    printf("\n\tSearchCatalog_VDI by partial name \"%s\"\n", targetName );

	do 
	{
		result = SearchCatalog_VDI( volID, options, &searchPB );
		options &= ~SRCHFS_START;							//	continue search

		printf( "\tresult=%d\tnumFoundMatches=%ld\n\tSearchCatalog_VDI Results:\n", result, searchPB.numFoundMatches );
	
	    foundObjectInfo = searchPB.buffer;
	
		for ( i=0 ; i < searchPB.numFoundMatches ; i++ )
		{
			printobject(foundObjectInfo);
	        foundObjectInfo = (SearchInfoReturnPtr)((char *)foundObjectInfo + foundObjectInfo->size);
		}

	} while ( result == EAGAIN );
	
		
	//
	//	Search By ParID & partial name & only files
	//
	options										= SRCHFS_START | SRCHFS_MATCHPARTIALNAMES | SRCHFS_MATCHFILES;
	searchPB.numFoundMatches					= 0;
    searchInfo2.commonBitmap = searchInfo1.commonBitmap = ATTR_CMN_NAME | ATTR_CMN_PAROBJID;
    strcpy( targetName, "test" );								//	Search for all objects containing the word "Test"
	searchInfo1.c.parentDirID					= 2;			//	root directory
	searchInfo2.c.parentDirID					= 2;			//	root directory
	searchPB.bufferSize							= 75 * 1024;	//	75K buffer

    printf("\n\tSearchCatalog_VDI for files with partial name \"%s\", parID=%ld\n", targetName, searchInfo1.c.parentDirID );

	do 
	{
		result = SearchCatalog_VDI( volID, options, &searchPB );
		options &= ~SRCHFS_START;							//	continue search

		printf( "\tresult=%d\tnumFoundMatches=%ld\n\tSearchCatalog_VDI Results:\n", result, searchPB.numFoundMatches );
	
	    foundObjectInfo = searchPB.buffer;
	
		for ( i=0 ; i < searchPB.numFoundMatches ; i++ )
		{
			printobject(foundObjectInfo);
	        foundObjectInfo = (SearchInfoReturnPtr)((char *)foundObjectInfo + foundObjectInfo->size);
		}

	} while ( result == EAGAIN );
	

	//
	//	Search By partial name to read more matches than will fit in buffer
	//
	searchPB.bufferSize							= 5 * 1024;	//	5K buffer
	searchPB.maxMatches							= 1000;		//	Ask for 1000 matches in our 5K buffer
	options										= SRCHFS_START | SRCHFS_MATCHPARTIALNAMES | SRCHFS_MATCHDIRS | SRCHFS_MATCHFILES;
    searchInfo2.commonBitmap = searchInfo1.commonBitmap	= ATTR_CMN_NAME;
    strcpy( targetName, "e" );								//	Search for all objects containing the word "e"
	matches = 0;

    printf("\n\tSearchCatalog_VDI for files with partial name \"%s\" with %dK buffer\n\tSearchCatalog_VDI Results:\n", targetName, ((int)searchPB.bufferSize/1024) );

	do 
	{
		result = SearchCatalog_VDI( volID, options, &searchPB );
		options &= ~SRCHFS_START;							//	continue search

		if (result != EAGAIN && result != 0)
			break;

	
	    foundObjectInfo = searchPB.buffer;
	
		for ( i=0 ; i < searchPB.numFoundMatches ; i++ )
		{
			printobject(foundObjectInfo);
	        foundObjectInfo = (SearchInfoReturnPtr)((char *)foundObjectInfo + foundObjectInfo->size);
		}
		matches += searchPB.numFoundMatches;

	} while ( result == EAGAIN );
	
	printf( "\tresult=%d\tnumFoundMatches=%ld\n\n", result, matches );
		

    printf( "*** End SearchCatalog_VDI Test ***\n\n" );
    
	free( searchPB.buffer );

    result = Remove_VDI(volID, 2, SEARCHTESTFILENAME, 0, kTextEncodingMacRoman, 0);
    if (result) {
        err(errno, "deleting file '",SEARCHTESTFILENAME,"' for SearchCatalog_VDI test (result = %d, errno = %d)", result, errno);
    };

    return( result );
}


static	int	ExchangeFileIDTest(fsvolid_t volID, u_long dirID)
{
    int					result = -1;
    char				filename1[] = "VDI_Exchange_xyz";
    char				filename2[] = "VDI_Exchange_b";
    char				teststr1[] = "sometexttotest";
    char				teststr2[] = "othertexttocompare";
    char				instr1[32],instr2[32];
    int					fd1, fd2;
    FILE				*file1 = NULL;
    FILE				*file2 = NULL;


    printf( "\n*** Begin ExchangeFileID Test: volID = %d, dirID = %ld ***\n", volID, dirID );

    printf("  cleaning up old test files...");
    (void) Remove_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, 0);
    (void) Remove_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, 0);
    printf("  done.\n");

    printf("creating %s...\n", filename1);
    result = Create_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, 0, DEFAULTCREATEMODE);
    if (result != 0) {
        err (errno, "ERROR: Could not create file %s", filename1);
        goto Err_Exit;
    }
    fd1 = OpenFork_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, kHFSDataForkName, DEFAULTOPENMODE);
    if (fd1 == -1) {
        err (errno, "ERROR: Opening file %s", filename1);
        goto Err_Exit1;
    };
    file1 = fdopen(fd1, "w+");
    if (file1 == NULL) {
        err (errno, "ERROR: fopen-ing file %s", filename1);
        goto Err_Exit1;
    };

    printf("creating %s...\n", filename2);
    result = Create_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, 0, DEFAULTCREATEMODE);
    if (result) {
        err (errno, "ERROR: Could not create file %s", filename2);
        goto Err_Exit1;
    }
    fd2 = OpenFork_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, kHFSDataForkName, DEFAULTOPENMODE);
    if (fd2 == -1) {
        err (errno, "ERROR: Opening file %s", filename2);
        goto Err_Exit1;
    };
    file2 = fdopen(fd2,"w+");
    if (file2 == NULL) {
        err (errno, "ERROR: fopen-ing file %s", filename2);
        goto Err_Exit1;
    };

    printf("writing test data...\n");
    result = fprintf(file1, teststr1, "%s");
    if (result<0) {
        err(errno, "writing test data to file1");
        goto Err_Exit2;
    };
    result = fprintf(file2, teststr2, "%s");
    if (result<0) {
        err(errno, "writing test data to file2");
        goto Err_Exit2;
    };

    result = fclose(file1);
    if (result) {
        err(errno, "closing file1 after writing initial data");
        goto Err_Exit2;
    };
    result = fclose(file2);
    if (result) {
        err(errno, "closing file2 after writing initial data");
        goto Err_Exit2;
    };

    printf("exchanging file contents...\n");
    result = ExchangeFiles_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, dirID, filename2, 0, 0);
    if (result<0) {
        printf ("The actual exchange failed!\n");
        goto Err_Exit2;
    }
	
    printf("re-opening test files...\n");
    fd1 = OpenFork_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, kHFSDataForkName, DEFAULTOPENMODE);
    if (fd1 == -1) {
        err (errno, "ERROR: Opening file %s", filename1);
        goto Err_Exit1;
    };
    file1 = fdopen(fd1, "r");
    if (file1 == NULL) {
        err (errno, "ERROR: fopen-ing file %s", filename1);
        goto Err_Exit1;
    };

    fd2 = OpenFork_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, kHFSDataForkName, DEFAULTOPENMODE);
    if (fd2 == -1) {
        err (errno, "ERROR: Opening file %s", filename2);
        goto Err_Exit1;
    };
    file2 = fdopen(fd2,"r");
    if (file2 == NULL) {
        err (errno, "ERROR: fopen-ing file %s", filename2);
        goto Err_Exit1;
    };

    printf("verifying results...\n");
    result = (int)fscanf (file1, "%32s", instr1);
    if (result<0) {
        printf ("Couldn't get the string of file1 (result = %d)\n", result);
        goto Err_Exit2;
    }
    result = (int)fscanf (file2, "%32s", instr2);
    if (result<0) {
        printf ("Couldn't get the string of file2 (result = %d)\n", result);
        goto Err_Exit2;
    }

    result = strcmp (instr1, teststr2);
    if (result!=0) {
        printf ("File 1 string did not compare teststr:%s, filestr %s\n", teststr2, instr1);
        result = -1;
        goto Err_Exit2;
    }

    result = strcmp (instr2, teststr1);
    if (result!=0) {
        printf ("File 2 string did not compare teststr:%s, filestr %s\n", teststr1, instr2);
        result = -1;
        goto Err_Exit2;
    }

    printf( "\n!!! Exchange was succesfull !!!\n" );

    (void)fclose(file1);
    (void)fclose(file2);

Err_Exit2:;
    if (file2) fclose(file2);
    (void)Remove_VDI(volID, dirID, filename2, 0, kTextEncodingMacRoman, 0);

Err_Exit1:;
    if (file1) fclose(file1);
    (void)Remove_VDI(volID, dirID, filename1, 0, kTextEncodingMacRoman, 0);

Err_Exit:;
    printresult(result);
    printf( "\n*** End ExchangeFileID Test ***\n" );
    
    return( result );
}

#define TESTFORKSIZE 4095
#define BIGFORKSIZE  0x8FFFFFFF
#define INVALIDFORKSIZE -2

static	int	ForkReserveSpaceTest(fsvolid_t volID, u_long dirID)
{
    int					result = -1;
    char				filename[] = "VDI_ForkReserveSpace";
    off_t 				theSpaceAvailable=0;
    off_t				requestSize;
    off_t				originalPEOF;
    int					fd;
	CatalogAttributeInfo objInfo;
    VolumeAttributeInfo volInfo;


    printf( "\n*** Begin Fork Reserve Space Test: volID = %d, dirID = %ld ***\n", volID, dirID );

    printf("\tcleaning up old test file...");
    (void) Remove_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, 0);
    printf("  done.\n");

    printf("\tcreating file \"%s\"...\n", filename);
    result = Create_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, 0, DEFAULTCREATEMODE);
    if (result != 0) {
        err (errno, "\tERROR: Could not create file %s", filename);
        goto Err_Exit;
    };

    fd = OpenFork_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, kHFSDataForkName, DEFAULTOPENMODE);
    if (fd == -1) {
        err (errno, "\tERROR: Opening file %s", filename);
        goto Err_Exit;
    };

    printf("\tCalling reserve space...\n");
    result = ForkReserveSpace_VDI(fd, TESTFORKSIZE, 0, &theSpaceAvailable);
         if (result != 0) {
        err(errno, "Reserving space in data fork of file");
        goto Err_Exit1;
    };

         result = GetCatalogInfo_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, 0, &objInfo, kMacOSHFSFormat, NULL, 0);
	if (result != 0) {
		err (errno, "\tERROR: Could not get info on file file %s", filename);
		goto Err_Exit;
	};

	if (objInfo.f.dataPhysicalLength < TESTFORKSIZE) {
		printf("\tfile's dataPhysicalLength of %qd is too small\n", objInfo.f.dataPhysicalLength);
		goto Err_Exit;
	}

	if (objInfo.f.dataPhysicalLength != theSpaceAvailable) {
		printf("\tfile's dataPhysicalLength of %qd does not match bytes allocated\n", objInfo.f.dataPhysicalLength);
		goto Err_Exit;
	}

    printf ("\tBytes Allocated = %qd, dataPhysicalLength = %qd\n\n",theSpaceAvailable, objInfo.f.dataPhysicalLength);
	
	originalPEOF = objInfo.f.dataPhysicalLength;
    theSpaceAvailable = 0;

    printf("\tChecking error cases on Fork Reserve Space...\n\n");

    /* Before calling big fork size,  lseek out a ways */
    
   
     result = ForkReserveSpace_VDI(fd, INVALIDFORKSIZE, 0, &theSpaceAvailable);
              if (result == 0) {
               printf("\tShould have gotten an error from ForkReserveSpace_VDI but did not\n");
                goto Err_Exit1;
            };

     printf("\tGot error as expected.  Error should be 22: Invalid Argument. \n\tError was %d: %s \n",result,strerror(result));
     printf ("\tBytes Allocated = %qd\n\n",theSpaceAvailable);

     result = lseek (fd,4096,SEEK_CUR);

     result = ForkReserveSpace_VDI(fd, BIGFORKSIZE, 0, &theSpaceAvailable);
             if (result == 0) {
                 printf("\tShould have gotten an error from ForkReserveSpace_VDI but did not\n");
               goto Err_Exit1;
           };

	printf("\tGot error as expected.  Error should be 27: File too large. \n\tError was %d: %s \n",result,strerror(result));
	printf ("\tBytes Allocated = %qd\n\n", theSpaceAvailable);

	/*
	 * Get the latest volume info...
	 */
	result = GetVolumeInfo_VDI(0, volID, NULL, 0, 0, 0, &volInfo);
	if (result != 0) {
		err (result, "\tERROR: Could not get info on volume (%ld)", volID);
		goto Err_Exit;
	};
	
	requestSize = (off_t) ((u_int64_t)(volInfo.numUnusedBlocks + 10) * (u_int64_t)volInfo.allocBlockSize);
	
	result = ForkReserveSpace_VDI(fd, requestSize, 0, &theSpaceAvailable);

    (void) GetVolumeInfo_VDI(0, volID, NULL, 0, 0, 0, &volInfo);

	if (result == 0) {
		printf("\tShould have gotten an error (28) from ForkReserveSpace_VDI but did not\n");
		printf ("\tBytes Requested = %qd, bytes allocated = %qd, free space = %qd\n",
				requestSize, theSpaceAvailable, (u_int64_t)volInfo.numUnusedBlocks * (u_int64_t)volInfo.allocBlockSize);
		goto Err_Exit1;
	};

	printf("\tGot error as expected.  Error should be 28: No space left. \n\tError was %d: %s \n",result,strerror(result));

	if (volInfo.numUnusedBlocks != 0) {
		printf ("\tDid not allocate all available space, %qd bytes still available\n", (u_int64_t)volInfo.numUnusedBlocks * (u_int64_t)volInfo.allocBlockSize);
		printf ("\tBytes Requested = %qd, bytes allocated = %qd\n", requestSize, theSpaceAvailable);
		goto Err_Exit1;
	}

    result = GetCatalogInfo_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, 0, &objInfo, kMacOSHFSFormat, NULL, 0);
	if (result != 0) {
		err (result, "\tERROR: Could not get info on file file %s", filename);
		goto Err_Exit;
	};

	if ((objInfo.f.dataPhysicalLength - originalPEOF) != theSpaceAvailable) {
		printf("\tBytes added to file's dataPhysicalLength (%qd) do not match bytes allocated\n", objInfo.f.dataPhysicalLength - originalPEOF);
		goto Err_Exit;
	}

    printf ("\tBytes Allocated = %qd, dataPhysicalLength = %qd\n",theSpaceAvailable, objInfo.f.dataPhysicalLength);

	printf( "\n\t!!! Fork Reserve Space Test was succesfull !!!\n" );

Err_Exit1: ;

    result = close(fd);
    if (result) {
        err(errno, "closing file after reserving space");
        goto Err_Exit;
    };

Err_Exit: ;

    (void) Remove_VDI(volID, dirID, filename, 0, kTextEncodingMacRoman, 0);

    printf( "\n*** End ForkReserveSpace_VDI Test ***\n" );
    
    return( result );
}


static void VolumeAttributeTest(fsvolid_t volID) {
    int result;
    VolumeAttributeInfo volInfo;
    char volumeName[32] = { 0 };
    char savedVolumeName[32] = { 0 };
    char newVolumeName[] = "XYZZY";

    printf("VolumeAttributeTest: calling GetVolumeInfo_VDI, volID = %d... ", volID);
    result = GetVolumeInfo_VDI(0, volID, volumeName, 0, 0, 0, &volInfo);
    printf("result = %d.\n", result);
    if (volumeName[0]) {
        printf("HEY!  Volume name returned = '%s'?\n", volumeName);
    };
    printvolinfo(&volInfo);

    printf("VolumeAttributeTest: changing volume name to '%s'...", newVolumeName);
    strncpy(savedVolumeName, volInfo.name, sizeof(savedVolumeName));
    strncpy(volInfo.name, newVolumeName, sizeof(volInfo.name));
    volInfo.commonBitmap = ATTR_CMN_SCRIPT;
    volInfo.volumeBitmap = ATTR_VOL_NAME;
    result = SetVolumeInfo_VDI(volID, 0, &volInfo);
    printf("result = %d.\n", result);

    printf("VolumeAttributeTest: retrieving new volume info... ");
    result = GetVolumeInfo_VDI(0, volID, volumeName, 0, 0, 0, &volInfo);
    printf("result = %d.\n", result);
    if (volumeName[0]) {
        printf("HEY!  Volume name returned = '%s'?\n", volumeName);
    };
    printvolinfo(&volInfo);

    printf("VolumeAttributeTest: restoring original volume name ('%s')... ", savedVolumeName);
    strncpy(volInfo.name, savedVolumeName, sizeof(volInfo.name));
    volInfo.commonBitmap = ATTR_CMN_SCRIPT;
    volInfo.volumeBitmap = ATTR_VOL_NAME;
    result = SetVolumeInfo_VDI(volID, 0, &volInfo);
    printf("result = %d.\n", result);
};

static void printresult(int result)
{
	if (result == -1)
		printf(" result %d (errno = %d)\n", result, errno);
	else
    	printf(" result %d\n", result);
}

static void printcatinfo(const CatalogAttributeInfo *objCatalogInfo)
{
//	printf("    commonBitmap       = 0x%010lx\n", objCatalogInfo->commonBitmap);
//	printf("    directoryBitmap    = 0x%010lx\n", objCatalogInfo->directoryBitmap);
//	printf("    fileBitmap         = 0x%010lx\n", objCatalogInfo->fileBitmap);

    printf("    c.objectType       = %d\n", objCatalogInfo->c.objectType);
//	printf("    c.scriptCode       = %ld\n", objCatalogInfo->c.scriptCode);
    printf("    c.objectID         = %ld\n", objCatalogInfo->c.objectID);
//	printf("    c.instance         = %ld\n", objCatalogInfo->c.instance);
    printf("    c.parentDirID      = %ld\n", objCatalogInfo->c.parentDirID);		
    printf("    c.creationTime     = ");
    printtime((time_t*) &objCatalogInfo->c.creationTime);
    printf("\n");
    printf("    c.lastModTime      = ");
    printtime((time_t*) &objCatalogInfo->c.lastModificationTime);
    printf("\n");
    printf("    c.lastChangeTime   = ");
    printtime((time_t*) &objCatalogInfo->c.lastChangeTime);
    printf("\n");
    printf("    c.lastBackupTime   = ");
    printtime((time_t*) &objCatalogInfo->c.lastBackupTime);
    printf("\n");

//	printf("    fd.fInfo.fdType    = %d\n", objCatalogInfo->fd.u.f.finderInfo.fdType);
//	printf("    fd.fInfo.fdCreator = %d\n", objCatalogInfo->fd.u.f.finderInfo.fdCreator);

    printf("    f.totalSize        = %ld\n", (u_long) objCatalogInfo->f.totalSize);
    printf("    f.blockSize        = %ld\n", objCatalogInfo->f.blockSize);
    printf("    f.attributes       = 0x%08lx\n", objCatalogInfo->f.attributes);
    printf("    f.dataEOF          = %ld\n", (u_long) objCatalogInfo->f.dataLogicalLength);
    printf("    f.resourceEOF      = %ld\n", (u_long) objCatalogInfo->f.resourceLogicalLength);
}


static void printvolinfo(const VolumeAttributeInfo *vip)
{
    printf("    name            = '%s'\n", vip->name);
    printf("    signature       = 0x%08lx\n", vip->signature);
    printf("    attributes      = 0x%08lx\n", vip->attributes);
    printf("    numFiles        = %ld\n", vip->numFiles);
    printf("    numDirectories  = %ld\n", vip->numDirectories);
    printf("    creationDate    = ");
    printtime((time_t*) &vip->creationDate);
    printf("\n");
    printf("    lastModDate     = ");
    printtime((time_t*) &vip->lastModDate);
    printf("\n");
    printf("    lastBackupDate  = ");
    printtime((time_t*) &vip->lastBackupDate);
    printf("\n");
    printf("    allocBlockSize  = %ld\n", vip->allocBlockSize);
    printf("    numAllocBlocks  = %ld\n", (u_long) vip->numAllocBlocks);
    printf("    numUnusedBlocks = %ld\n", (u_long) vip->numUnusedBlocks);
}


static void printtime(const time_t * time)
{
	struct tm * ltm;
	ltm = localtime(time);
	
	printf("%02d/%02d/%02d %02d:%02d:%02d %s", ltm->tm_mon+1, ltm->tm_mday, ltm->tm_year, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, ltm->tm_zone);
}


static void printobject(const SearchInfoReturn * obj)
{
	printf("\t\t");
	printtime((time_t*) &obj->ci.c.creationTime);
	printf("  parID = %4ld  \"%s\"\n", obj->ci.c.parentDirID, obj->name );
}
