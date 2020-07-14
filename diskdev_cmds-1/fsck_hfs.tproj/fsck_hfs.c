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
        File:           fsck_hfs.c

        Contains:       Top level code for fsck_hfs command line utility.  

        Version:        HFS Plus 1.0

        Copyright:      \xa9 1998-1999 by Apple Computer, Inc., all rights reserved.

        File Ownership:

                DRI:                    Cliff Ritchie    

                Other Contact:          Clark Warner 

                Technology:                     xxx put technology here xxx

        Writers:

                (chw)   Clark Warner 

        Change History (most recent first):
	
	2/9/99  Chw     Added file header 

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__MWERKS__)
#if !__option(mpwc)
#define NO_COMMANDLINE
#include <console.h>
#endif
#endif

#include "ScavDefs.h"
#include "Prototypes.h"

int list		= 0;
int interactive 	= 0;
int automatic		= 0;
int verbose		= 0;
int verify		= 0; 
int force		= 0;
int debugging		= 0;

char device[32] = {sizeof(device)*'\0'};

int main(int argc, char **argv)
{
	SGlob dataArea;
	
	short scavError, termError;
	short userCmd;

	int c;

#ifdef NO_COMMANDLINE
  	argc = ccommand(&argv);
#endif
 
	opterr = 0;

	while ((c = getopt(argc, argv, "lravcfd")) != -1)
		switch (c)
		{
			/* list all filenames */
			case 'l':
				list = 1;
			break;
			
			/* perform interactive repairs */
			case 'r':
				interactive = 1;
			break;
			
			/* perform automatic repairs */
			case 'a':
				automatic = 1;
			break;
			
			/* verbose */
			case 'v':
				verbose = 1;
			break;

			/* verify only */
			case 'c':
				verify = 1;
			break;
			
			/* force check if filesystem is marked valid */
			case 'f':
				force = 1;
			break;
			
			/* debug mode */
			case 'd':
				debugging = 1;
			break;
			
			/* what?? */
			case '?':
				return 16;
			break;
		}
		
	/* last arg must be device */
	if (optind == argc-1)
		strcpy(device, argv[optind]);
	else
		return 16;

	ClearMemory((Ptr) &dataArea, sizeof(SGlob));

	/* do files system check */
	ScavCtrl(&dataArea, Op_IVChk, &scavError, &userCmd);
		
	if ( scavError == noErr )
		ScavCtrl(&dataArea, Op_Verify, &scavError, &userCmd);
		
	if ( scavError == noErr &&
		 (!verify) &&
		 (dataArea.RepLevel != Unrepairable) &&
		 (dataArea.RepLevel != No_RepairNeeded) )
		ScavCtrl(&dataArea, Op_Repair, &scavError, &userCmd);
		
	ScavCtrl(&dataArea, Op_Term, &termError, &userCmd);
	
#ifdef NO_COMMANDLINE
  	{
  		char c;
  		printf("End of Program.  [RetCode=%d]\n",scavError == noErr ? 0 : 8);
  		c = getchar();
  	};
#else
	if (debugging)
  		printf("End of Program.  [RetCode=%d]\n",scavError == noErr ? 0 : 8);
#endif

	return scavError == noErr ? 0 : 8;
}


void WriteMsg( SGlobPtr GPtr, short messageID, short messageType )
{
	char *msg_text;
	
	if (verbose)
	{
		switch (messageID)
		{
			case M_IVChk:
				msg_text = "Checking disk volume.";
				break;
	
			case M_ExtBTChk:
				msg_text = "Checking extent file BTree.";
				break;
	
			case M_ExtFlChk:
				msg_text = "Checking extent file.";
				break;
	
			case M_CatBTChk:
				msg_text = "Checking catalog BTree.";
				break;
	
			case M_CatFlChk:
				msg_text = "Checking catalog file.";
				break;
	
			case M_CatHChk:
				msg_text = "Checking catalog hierarchy.";
				break;
	
			case M_VInfoChk:
				msg_text ="Checking volume information.";
				break;
	
			case M_Missing:
				msg_text = "Checking for missing folders.";
				break;
	
			case M_Repair:
				msg_text = "Repairing volume.";
				break;
	
			case M_LockedVol:
				msg_text = "Checking for locked volume name.";
				break;
	
			case M_Orphaned:
				msg_text = "Checking for orphaned extents.";
				break;
	
			case M_DTDBCheck:
				msg_text = "Checking desktop database.";
				break;
	
			case M_VolumeBitMapChk:
				msg_text = "Checking volume bit map.";
				break;
	
			case M_CheckingHFSVolume:
				msg_text = "Checking \"HFS\" volume structures.";
				break;
	
			case M_CheckingHFSPlusVolume:
				msg_text = "Checking \"HFS Plus\" volume structures.";
				break;
	
			case M_AttrBTChk:
				msg_text = "Checking attributes BTree.";
				break;
	
			case M_RebuildingExtentsBTree:
				msg_text = "Rebuilding Extents BTree.";
				break;
	
			case M_RebuildingCatalogBTree:
				msg_text = "Rebuilding Catalog BTree.";
				break;
	
			case M_RebuildingAttributesBTree:
				msg_text = "Rebuilding Attributes BTree.";
				break;
	
			case M_MountCheckMajorError:
				msg_text = "MountCheck found serious errors.";
				break;
	
			case M_MountCheckMinorError:
				msg_text = "MountCheck found minor errors.";
				break;
		}
		printf("%s\n",msg_text);
	}
}

void WriteError( short MsgID, UInt32 TarID, UInt32 TarBlock )
{
	char *s[] =
	{
		"Invalid PEOF",
		"Invalid LEOF",
		"Invalid directory valence",
		"Invalid CName",
		"Invalid node height",
		"Missing file record for file thread",
		"Invalid allocation block size",
		"Invalid number of allocation blocks",
		"Invalid VBM start block",
		"Invalid allocation block start",
		"Invalid extent entry",
		"Overlapped extent allocation",
		"Invalid BTH length",
		"BT map too short during repair",
		"Invalid root node number",
		"Invalid node type",
		"",
		"Invalid record count",
		"Invalid index key",
		"Invalid index link",
		"Invalid sibling link",
		"Invalid node structure",
		"Overlapped node allocation",
		"Invalid map node linkage",
		"Invalid key length",
		"Keys out of order",
		"Invalid map node",
		"Invalid header node",
		"Exceeded maximum BTree depth",
		"",
		"Invalid catalog record type",
		"Invalid directory record length",
		"Invalid thread record length",
		"Invalid file record length",
		"Missing thread record for root dir",
		"Missing thread record",
		"Missing directory record",
		"Invalid key for thread record",
		"Invalid parent CName in thread record",
		"Invalid catalog record length",
		"Loop in directory hierarchy",
		"Invalid root directory count",
		"Invalid root file count",
		"Invalid volume directory count",
		"Invalid volume file count",
		"Invalid catalog PEOF",
		"Invalid extent file PEOF",
		"Nesting of folders has exceeded the recommended limit of 100",
		"",
		"File thread flag not set in file rec",
		"Missing folder detected",
		"Invalid file name",
		"Invalid file clump size",
		"Invalid BTree Header",
		"Directory name locked",
		"Catalog file entry not found for extent",
		"Custom icon missing",
		"Master Directory Block needs minor repair",
		"Volume Header needs minor repair",
		"Volume Bit Map needs minor repair",
		"Invalid BTree node size",
		"Invalid catalog record type found",
		"",
		"Reserved fields in the catalog record have incorrect data",
		"Invalid file or directory ID found",
		"The version in the VolumeHeader is not compatable with this version of Disk First Aid",
		"Disk full error"
	};
	
	printf("Problem:  %s, %d, %d\n",s[abs(MsgID) - 500], TarID, TarBlock);
}

pascal void SpinCursor(short increment)
{
}
