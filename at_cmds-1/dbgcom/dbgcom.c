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

/* dbgcom.c

	routines common to setdbgi.c & router.c

	revision history
	----------------
	09-08-94	0.01 jjs Created
	09-12-94	0.02 jjs minor


*/


#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include <at/appletalk.h>
#include <at/asp_errno.h>
#include <at/atp.h>

#include <h/atlog.h>
#include <h/debug.h>
#include <h/lap.h>
#include <LibcAT/at_proto.h>

dbgBits_t       dbgBits;

/* Path information for access of LAP special files
*/
#define LAP_DIR			"/dev/appletalk/lap"
#define CONTROL_FILE		"control"
#define AT_INTERFACE	"ethertalk0"


#define MIN_MAJOR			0		/* minimum required kernel version */
#define MIN_MINOR			6

int
kernelDbg(cmd, dbgBits)
	int 		cmd;
	dbgBits_t	*dbgBits;
{

	dbgBits_t	dbgBitsLocal;
	int 		if_id;
	int 		size;
	FILE 		*pF=0;
	int			status;

	if ((if_id = openCtrlFile(NULL, "kernelDbg",MIN_MAJOR,MIN_MINOR)) < 0) {
		printf("could not open control file to set kernel bits\n");
		if_id=0;
	}

    size = sizeof(dbgBits_t);

	switch (cmd)
	{
	case DBG_GET:
    	size = 0;
		if (!if_id || (status = at_send_to_dev(if_id, LAP_IOC_GET_DBG, (char *) dbgBits, &size))) {
			printf("error getting DBG info from kernel, trying file\n");
			if (!(pF = fopen(DEBUG_STAT_FILE, "r"))) {
				printf("error opening debug file %s\n", DEBUG_STAT_FILE);
				goto error;
			}
			if (fread(dbgBits, sizeof(*dbgBits), 1, pF) != 1) {
				printf("error reading DBG info file: %s\n", DEBUG_STAT_FILE);
				goto error;
			}
			printf("***WARNING dbg status is from file \n"); 
		}
		break;

	case DBG_SET_FROM_FILE:
		if (!if_id)
			goto error;
		if (!(pF = fopen(DEBUG_STAT_FILE, "r"))) {
			/*printf("error opening debug file %s\n", DEBUG_STAT_FILE);*/
			goto error;
		}
		if (fread(&dbgBitsLocal, sizeof(dbgBitsLocal), 1, pF) != 1) {
			printf("error reading DBG info file: %s\n", DEBUG_STAT_FILE);
			goto error;
		}
		if (status = at_send_to_dev(if_id, LAP_IOC_SET_DBG, (char *) &dbgBitsLocal, &size)) {
			printf("error setting DBG info in kernel\n");
			goto error;	
		}
		break;

	case DBG_SET:
		if (!(pF = fopen(DEBUG_STAT_FILE, "w"))) {
			printf("error opening debug file %s\n", DEBUG_STAT_FILE);
			goto error;
		}
		if (fwrite(dbgBits, sizeof(dbgBits_t), 1, pF) != 1) {
			printf("error writing DBG info file: %s\n", DEBUG_STAT_FILE);
			goto error;
		}
		printf("file updated\n");
		if (!if_id)
			goto error;
		if (status = at_send_to_dev(if_id, LAP_IOC_SET_DBG, (char *) dbgBits, &size)) {
			printf("error setting DBG info in kernel\n");
			goto error;	
		}
		printf("kernel updated\n");
		break;

	default:
		printf("kernelDbg: unknown command\n");
		goto error;
	}		

	if (if_id) close(if_id);
	if (pF) fclose(pF);
	return(0);
error:
	if (!if_id) {
		printf("error opening ctrl file, is stack loaded??\n");
	}
	else
		close(if_id);
	if (pF) fclose(pF);
	return(-1);
}
