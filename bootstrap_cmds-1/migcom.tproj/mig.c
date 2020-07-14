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
/**********************************************************
 *  ABSRACT:
 *	Main control program for mig.
 *	Mig parses an interface definitions module for a mach server.
 *	It generates three c modules: subsystem.h, 
 *	subsystemUser.c and subsystemServer.c for the user
 *	and server sides of the ipc-message passing interface.
 *
 *	Switches are;
 *		-[v,Q]  verbose or not quiet:  prints out type
 *			and routine information as mig runs.
 *		-[V,q]  not verbose or quiet : don't print 
 *			information during compilation
 *			(this is the default)
 *		-[r,R]  do or don't use msg_rpc calls instead of 
 *			msg_send, msg_receive pairs. Default is -r
 *		-[s,S]	generate symbol table or not:  generate a
 *			table of rpc-name, number, routine triplets
 *			as an external data structure -- main use is
 *			for protection system's specification of rights
 *			and for protection dispatch code.  Default is -s.
 *		-[p,P]	Pad message elements to two bytes or four bytes.
 *			Default is four bytes.
 *		-i	Put each user routine in its own file.  The
 *			file is named <routine-name>.c.
 *		-user <name>
 *			Name the user-side file <name>
 *		-server <name>
 *			Name the server-side file <name>
 *		-header <name>
 *			Name the user-side header file <name>
 *		-sheader <name>
 *			NAme the server-side header file <name>
 *
 *  DESIGN:
 *	Mig uses a lexxer module created by lex from lexxer.l and
 *	a parser module created by yacc from parser.y to parse an
 *	interface definitions module for a mach server.
 *	The parser module calls routines in statement.c
 *	and routines.c to build a list of statement structures.
 *	The most interesting statements are the routine definitions
 *	which contain information about the name, type, characteristics
 *	of the routine, an argument list containing information for
 *	each argument type, and a list of special arguments. The
 *	argument type structures are build by routines in type.c
 *	Once parsing is completed, the three code generation modules:
 *	header.c user.c and server.c are called sequentially. These
 *	do some code generation directly and also call the routines
 *	in utils.c for common (parameterized) code generation.
 *	
 * HISTORY:
 * 17-Oct-90  Gregg Kellogg (gk) at NeXT
 *	Add -p and -P arguments to specify element padding (for compatibility).
 *
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 *  8-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Added -user, -server, and -header switches to name output files.
 *	Added -i switch to write individual files for user routines.
 *
 * 17-Aug-87  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Added -s,-S switches for generating a SymTab
 *
 *  3-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed -t,-T switch as code is now the same for
 *	multi and single threaded use.
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 ****************************************************************/

#include "error.h"
#include "lexxer.h"
#include "global.h"
#include "write.h"

extern int yyparse();
#if	NeXT
static FILE *myfopen();
#endif	NeXT

boolean_t	GenIndividualUser = FALSE;
#if	NeXT
boolean_t	GenHandler = FALSE;
unsigned int NeXT_pad = 4;
#endif	NeXT

static void
parseArgs(argc, argv)
    int argc;
    char *argv[];
{
    while (--argc > 0)
	if ((++argv)[0][0] == '-') 
	{
	    switch (argv[0][1]) 
	    {
	      case 'q':
		BeQuiet = TRUE;
		break;
	      case 'Q':
		BeQuiet = FALSE;
		break;
	      case 'v':
		BeVerbose = TRUE;
		break;
	      case 'V':
		BeVerbose = FALSE;
		break;
	      case 'r':
		UseMsgRPC = TRUE;
		break;
	      case 'R':
		UseMsgRPC = FALSE;
		break;
	      case 's':
		if (!strcmp(argv[0], "-server"))
		{
		    --argc; ++argv;
		    if (argc == 0)
			fatal("missing name for -server option");
		    ServerFileName = strmake(argv[0]);
		}
#if	NeXT
		else if (streql(argv[0], "-sheader"))
		  {
		    --argc; ++argv;
		    if (argc == 0)
		      fatal ("missing name for -sheader option");
		    ServerHeaderFileName = strmake(argv[0]);
		  }
#endif	NeXT
		else
		    GenSymTab = TRUE;
		break;
	      case 'S':
	        GenSymTab = FALSE;
		break;
#if	NeXT
	      case 'p':
		NeXT_pad = 2;
		break;
	      case 'P':
		NeXT_pad = 4;
		break;
#endif	NeXT
	      case 'i':
		GenIndividualUser = TRUE;
		break;
	      case 'u':
		if (!strcmp(argv[0], "-user"))
		{
		    --argc; ++argv;
		    if (argc == 0)
			fatal("missing name for -user option");
		    UserFileName = strmake(argv[0]);
		}
		else
		    fatal("unknown flag: '%s'", argv[0]);
		break;
	      case 'h':
		if (!strcmp(argv[0], "-header"))
		{
		    --argc; ++argv;
		    if (argc == 0)
			fatal("missing name for -header option");
		    HeaderFileName = strmake(argv[0]);
		}
#if	NeXT
		else if (!strcmp(argv[0], "-handler"))
		{
		    --argc; ++argv;
		    if (argc == 0)
			fatal("missing name for -handler option");
		    ServerFileName = strmake(argv[0]);
		    GenHandler = TRUE;
		}
#endif	NeXT
		else
		    fatal("unknown flag: '%s'", argv[0]);
		break;
	      default:
		fatal("unknown flag: '%s'", argv[0]);
		/*NOTREACHED*/
	    }
	}
	else
	    fatal("bad argument: '%s'", *argv);
}

void
main(argc, argv)
    int argc;
    char *argv[];
{
    FILE *h, *server, *user;
#if	NeXT
    FILE *sheader;
#endif	NeXT

    set_program_name("mig");
    parseArgs(argc, argv);
    init_global();
    init_type();

    LookNormal();
    (void) yyparse();

    if (errors > 0)
	exit(1);

    more_global();

    h = myfopen(HeaderFileName, "w");
    if (!GenIndividualUser)
	user = myfopen(UserFileName, "w");
    server = myfopen(ServerFileName, "w");
#if	NeXT
    if (ServerHeaderFileName != strNULL)
	sheader = myfopen(ServerHeaderFileName, "w");
#endif	NeXT

    if (BeVerbose)
    {
	printf("Writing %s ... ", HeaderFileName);
	fflush(stdout);
    }
#if	NeXT
    WriteHeader(h, stats, 0);
#else	NeXT
    WriteHeader(h, stats);
#endif	NeXT
    fclose(h);

#if	NeXT
    if (ServerHeaderFileName != strNULL)
      {
	if (BeVerbose)
	  {
	    printf ("done.\nWriting %s ...", ServerHeaderFileName);
	    fflush (stdout);
	  }
	WriteHeader(sheader, stats, 1);
	fclose(sheader);
     }
#endif	NeXT

    if (GenIndividualUser)
    {
	if (BeVerbose)
	{
	    printf("done.\nWriting individual user files ... ");
	    fflush(stdout);
	}
	WriteUserIndividual(stats);
    }
    else
    {
	if (BeVerbose)
	{
	    printf("done.\nWriting %s ... ", UserFileName);
	    fflush(stdout);
	}
	WriteUser(user, stats);
	fclose(user);
    }
    if (BeVerbose)
    {
	printf("done.\nWriting %s ... ", ServerFileName);
	fflush(stdout);
    }
#if	NeXT
    if (GenHandler)
	WriteHandler(server, stats);
    else
#endif	NeXT
    WriteServer(server, stats);
    fclose(server);
    if (BeVerbose)
	printf("done.\n");

    exit(0);
}

#if	NeXT
static FILE *
myfopen(name, mode)
    char *name;
    char *mode;
{
    char *realname;
    FILE *file;

    if (name == strNULL)
	realname = "/dev/null";
    else {
	realname = name;
	(void) unlink(name);
    }

    file = fopen(realname, mode);
    if (file == NULL)
	fatal("fopen(%s): %s", realname, unix_error_string(errno));

    return file;
}
#endif	NeXT
