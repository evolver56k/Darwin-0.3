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
 *	Copyright (c) 1988, 1989, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* atstatus.c: 2.0, 1.7; 9/19/89; Copyright 1988-89, Apple Computer, Inc. */

#include <stdlib.h>
#include <unistd.h>

#include <at/appletalk.h>
#include <at/nbp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _AIX
#include <locale.h>
#include <atbin_msg.h>
#endif

#include <at/atp.h>
#include <at/asp_errno.h>
#include <LibcAT/at_proto.h>

/*#define MSGSTR(num,str)		catgets(catd, MS_ATBIN, num,str)*/
#define MSGSTR(num,str)		str
#ifdef _AIX
nl_catd		catd;
#endif
#define DEFAULT_TYPE	"LaserWriter"

main(argc, argv)
int argc;
char **argv;
{
	char		*status;
	char		*namestr = NULL;
	static at_entity_t	entity;
	char		*ch;
	char		*pap_status();
	static at_nbptuple_t	tuple;
	extern	char	*get_chooserPrinter();
#ifdef _AIX
	(void) setlocale(LC_ALL, "");
	catd = catopen(MF_ATBIN, NL_CAT_LOCALE);
#endif

        {
                int error = 0;
                error = checkATStack();
                switch (error) {
                        case NOTLOADED:
                                fprintf(stderr, MSGSTR(M_NOT_LOADED,
                                "The AppleTalk stack is not Loaded\n"));
                                break;
                        case LOADED:
                                fprintf(stderr, MSGSTR(M_NOT_LOADED,
                                "The AppleTalk stack is not Running\n"));
                                break;

                        case RUNNING:
                                error =0;
                                break;
                        default:
                                fprintf(stderr, MSGSTR(M_NOT_LOADED,
                                "Other error with The AppleTalk stack\n"));
                }
                if (error != 0)
                        exit(1);
	}

	switch (argc) {
	case 1 :
		/* read the system chooser file for printer choice */
		if ((namestr = get_chooserPrinter(argv[0])) == NULL)
			exit(1);
		if (nbp_parse_entity(&entity, namestr) < 0) {
			perror(MSGSTR(M_PARSING,"parsing nbp name"));
#ifdef _AIX
			catclose(catd);
#endif
			exit(1);
		}
		break;
	case 2 :
		namestr = argv[1];
		if (nbp_parse_entity(&entity, namestr) < 0) {
			perror(MSGSTR(M_PARSING,"parsing nbp name"));
#ifdef _AIX
			catclose(catd);
#endif
			exit(1);
		}
	
		if ((entity.type.len==1) && (entity.type.str[0]=='=')) {
			if ((ch=strchr(namestr, ':')) && (*(ch++) == '=')) {
				fprintf (stderr,MSGSTR(M_WILD,
					"wildcard not allowed in name string\n"));
#ifdef _AIX
				catclose(catd);
#endif
				exit(1);
			} else {
				entity.type.len = strlen(DEFAULT_TYPE);
				strcpy((char *) entity.type.str, DEFAULT_TYPE);
			}
		}
	
		if (nbp_iswild (&entity)) {
			fprintf (stderr,MSGSTR(M_WILD,
				"wildcard not allowed in name string\n"));
#ifdef _AIX
			catclose(catd);
#endif
			exit(1);
		}

		if ((ch = strchr(namestr, ':')) == NULL) {
			/* use did not specify the "type' */
			strcpy((char *) entity.type.str, DEFAULT_TYPE);
			entity.type.len = strlen(DEFAULT_TYPE);
		}
		break;
	default :
		fprintf(stderr, MSGSTR(M_USAGE1, 
			"usage: %s [object[:type[@zone]]]\n"), argv[0]);
#ifdef _AIX
		catclose(catd);
#endif
		exit(1);
	}

	if (nbp_lookup(&entity, &tuple, 1, NULL) <= 0) {
		switch(errno) {
		case ENETDOWN :
		case ENOENT:
			fprintf(stderr, MSGSTR(M_NET_UNREACH,
				"Network is unreachable, perhaps Appletalk is not running\n"));
			break;
		default :
			fprintf(stderr,MSGSTR(M_S_NOT_FOUND,
				"'%s' not found\n"),namestr);
			break;
		}
#ifdef _AIX
		catclose(catd);
#endif
		exit(1);
	}

	if ((status = pap_status(&tuple)) == NULL) {
		fprintf(stderr, MSGSTR(M_STATUS, "status for '%s' failed\n"),
			namestr);
#ifdef _AIX
		catclose(catd);
#endif
		exit(1);
	}

	printf(MSGSTR(M_DEF_PRINT,"Default printer is:%s:%s@%s\n%s\n"), 
		tuple.enu_entity.object.str, tuple.enu_entity.type.str, 
		entity.zone.str, status);

#ifdef _AIX
	catclose(catd);
#endif
	exit(0);
}
