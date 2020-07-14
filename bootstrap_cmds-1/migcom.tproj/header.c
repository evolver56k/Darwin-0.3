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
/********************************************************
 *  Abstract:
 *	routines to write pieces of the Header module.
 *	exports WriteHeader which directs the writing of
 *	the Header module
 *
 * $Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/header.c,v 1.1.1.1 1999/04/15 18:30:37 wsanchez Exp $ 
 *
 ********************************************************
 * HISTORY
 * 03-Jul-97  Daniel Wade (danielw) at Apple
 *      Generated code is now ANSI C compliant
 *
 * 03-Sep-91  Gregg Kellogg (gk) at NeXT
 *	Added changes to write out a server header file.  Basic Mechanism
 *	from Michael I Bushnell <mib@gnu.ai.mit.edu>, server prototype by me.
 *
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 03-Jan-89  Trey Matteson (trey@next.com)
 *	Removed init_<sysname> function
 *
 *  8-Jul-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Conditionally defined mig_external to be extern and then defined
 *	all functions  with the storage class mig_external.
 *	Mig_external can be changed
 *	when the generated code is compiled.
 *
 * 18-Jan-88  David Detlefs (dld) at Carnegie-Mellon University
 *	Modified to produce C++ compatible code via #ifdefs.
 *	All changes have to do with argument declarations.
 *
 *  3-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Revision to make multi-threaded use work. Removed definitions for
 * 	alloc_reply_port and init_msg_type as these routines are 
 * 	no longer generated.
 *
 * 30-Jul-87  Mary Thompson (mrt) at Carnegie-Mellon University
 * 	Made changes to generate conditional code for C++ parameter lists
 *
 * 29-Jul-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed WriteRoutine to produce conditional argument
 *	lists for C++
 *
 *  8-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed the KERNEL include from ../h to sys/
 *	Removed extern from WriteHeader to make hi-c happy
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 *******************************************************/

#include "write.h"
#include "utils.h"
#include "global.h"

static int writing_server_header;

#if	NeXT
#define syPrefix  (writing_server_header ? ServerPrefix : UserPrefix)
#define akbArg  (writing_server_header ? akbServerArg : akbUserArg)
#define WriteOneVarDecl \
  (writing_server_header ? WriteServerVarDecl : WriteVarDecl)
#define rtName(rt) (writing_server_header ? rt->rtServerName : rt->rtUserName)
#endif	NeXT

static void
WriteIncludes(file)
    FILE *file;
{
    fprintf(file, "#include <mach/kern_return.h>\n");
#if	NeXT
#else	NeXT
    fprintf(file, "#if\t%s || %s\n", NewCDecl, LintLib);
#endif	NeXT
    fprintf(file, "#include <mach/port.h>\n");
    fprintf(file, "#include <mach/message.h>\n");
#if	NeXT
#else	NeXT
    fprintf(file, "#endif\n");
#endif	NeXT
    fprintf(file, "\n");
}

static void
WriteExternalDecls(file)
    FILE *file;
{
}

static void
WriteProlog(file)
    FILE *file;
{
#if	NeXT
    if (writing_server_header && GenHandler) {
	fprintf(file, "#ifndef\t_%s_handler\n", SubsystemName);
	fprintf(file, "#define\t_%s_handler\n", SubsystemName);
    } else {
	fprintf(file, "#ifndef\t_%s%s\n", syPrefix, SubsystemName);
	fprintf(file, "#define\t_%s%s\n", syPrefix, SubsystemName);
    }
#else	NeXT
    fprintf(file, "#ifndef\t_%s\n", SubsystemName);
    fprintf(file, "#define\t_%s\n", SubsystemName);
#endif	NeXT
    fprintf(file, "\n");
    fprintf(file, "/* Module %s */\n", SubsystemName);
    fprintf(file, "\n");

    WriteIncludes(file);

    fprintf(file, "#ifndef\tmig_external\n");
    fprintf(file, "#define mig_external extern\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    WriteExternalDecls(file);
}

#if	NeXT
static void
WriteHandlerProlog(file)
    FILE *file;
{
    /*
     * Output disipatch structure beginning.
     */
    fprintf(file,
	"\n/*\n * Functions to call for handling returned messages.\n */\n");
    fprintf(file, "typedef struct %s {\n"
	"\tvoid\t\t*arg;\t\t/* argument to pass to function */\n"
	"\tint\t\ttimeout;\t/* timeout for RPC return msg_send */\n",
	SubsystemName);
}
#endif	NeXT

static void
WriteEpilog(file)
    FILE *file;
{
    fprintf(file, "\n");
#if	NeXT
    if (GenHandler)
	fprintf(file, "#endif\t/* _%s_handler */\n", SubsystemName);
    else
	fprintf(file, "#endif\t/* _%s%s */\n", syPrefix, SubsystemName);
#else	NeXT
    fprintf(file, "#endif\t/* _%s */\n", SubsystemName);
#endif	NeXT
}

static void
WriteRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");
#if	NeXT
    if (writing_server_header && GenHandler) {
	fprintf(file, "\t/* %s %s */\n",
	    rtRoutineKindToStr(rt->rtKind), rt->rtName);
	fprintf(file, "\t%s (*%s) (\n\t\tvoid *%s",
		ReturnTypeStr(rt),
		rt->rtName,
		FirstArgName(file, rt->rtArgs, akbServerArg));
	WriteListSkipFirst(file, rt->rtArgs, WriteOneVarDecl, akbArg, ",\n\t",
	    ");\n");
	
    } else {
	fprintf(file, "/* %s %s */\n",
	    rtRoutineKindToStr(rt->rtKind), rtName(rt));
	fprintf(file, "mig_external %s %s (\n", ReturnTypeStr(rt), rtName(rt));
	WriteList(file, rt->rtArgs, WriteOneVarDecl, akbArg, ",\n", ");\n");
    }
#else	NeXT
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "mig_external %s %s\n", ReturnTypeStr(rt), rt->rtUserName);
    fprintf(file, "#if\t%s\n", LintLib);
    fprintf(file, "    (");
    WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", " , "");
    fprintf(file, ")\n");
    WriteList(file, rt->rtArgs, WriteVarDecl, akbUserArg, ";\n", ";\n");
    fprintf(file, "{ ");
    if (!rt->rtProcedure)
	fprintf(file, "return ");
    fprintf(file, "%s(", rt->rtUserName);
    WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", ", "");
    fprintf(file, "); }\n");
    fprintf(file, "#else\n");
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "(\n");
    WriteList(file, rt->rtArgs, WriteVarDecl, akbUserArg, ",\n", "\n");
    fprintf(file, ");\n");
    fprintf(file, "#else\n");
    fprintf(file, "    ();\n");
    fprintf(file, "#endif\n");
    fprintf(file, "#endif\n");
#endif	NeXT
}

#if	NeXT
static void
WriteServerDecl(file, MaxRequestSize, MaxReplySize)
    FILE *file;
    unsigned int MaxRequestSize, MaxReplySize;
{
    fprintf(file, "\n#define\t%sMaxRequestSize\t%d\n",
	SubsystemName, MaxRequestSize);
    fprintf(file, "#define\t%sMaxReplySize\t%d\n\n",
	SubsystemName, MaxReplySize);
    fprintf(file, "/* Server %s */\n", ServerProcName);
    fprintf(file, "mig_external boolean_t %s\n", ServerProcName);
    fprintf(file, "\t(msg_header_t *InHeadP, msg_header_t *OutHeadP);\n");
}

static void
WriteHandlerDecl(file, MaxRequestSize, MaxReplySize)
    FILE *file;
    unsigned int MaxRequestSize, MaxReplySize;
{
    fprintf(file, "} %s_t;\n\n", SubsystemName);
    fprintf(file, "\n#define\t%sMaxRequestSize\t%d\n",
	SubsystemName, MaxRequestSize);
    fprintf(file, "#define\t%sMaxReplySize\t%d\n\n",
	SubsystemName, MaxReplySize);
    fprintf(file, "/* Handler %s */\n", ServerProcName);
    fprintf(file, "mig_external kern_return_t %s (\n", ServerProcName);
    fprintf(file, "\tmsg_header_t *InHeadP,\n\t%s_t *%s);\n",
    	SubsystemName, SubsystemName);
}
#endif	NeXT

void
WriteHeader(file, stats, isserver)
    FILE *file;
    statement_t *stats;
    int isserver;
{
    register statement_t *stat;

#if	NeXT
    writing_server_header = isserver;
#endif	NeXT
    WriteProlog(file);

#if	NeXT
    /*
     * Output import statements.
     */
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    break;
	  case skImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skUImport:
	    if (!isserver)
		WriteImport(file, stat->stFileName);
	    break;
	  case skSImport:
	    if (isserver)
		WriteImport(file, stat->stFileName);
	    break;
	  default:
	    fatal("WriteHeader(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}

    if (isserver && GenHandler) {
	/*
	 * Write out handler prolog.
	 */
	WriteHandlerProlog(file);
    }

    /*
     * Write out routine entries.
     */
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine)
	    WriteRoutine(file, stat->stRoutine);

    if (isserver) {
	u_int MaxRequest = 0, MaxReply = 0;
	/*
	 * Write out values to use for the maximum request/reply message sizes.
	 */
	for (stat = stats; stat != stNULL; stat = stat->stNext) {
	    if (stat->stKind != skRoutine)
		continue;
	    if (stat->stRoutine->rtMaxRequestSize > MaxRequest)
		MaxRequest = stat->stRoutine->rtMaxRequestSize;
	    if (stat->stRoutine->rtMaxReplySize > MaxReply)
		MaxReply = stat->stRoutine->rtMaxReplySize;
	}

	/*
	 * Write out the server procedure declaration.
	 */
	if (GenHandler)
	    WriteHandlerDecl(file, MaxRequest, MaxReply);
	else
	    WriteServerDecl(file, MaxRequest, MaxReply);
    }
#else	NeXT
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    WriteRoutine(file, stat->stRoutine);
	    break;
	  case skImport:
	  case skUImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skSImport:
	    break;
	  default:
	    fatal("WriteUser(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
#endif	NeXT

    WriteEpilog(file);
}
