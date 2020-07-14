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
/*****************************************************
 * ABSTRACT:
 *   Code generation utility routines.
 *   Called by header.c, user.c and server.c
 *   Exports routines:
 *	WriteImport, WriteRCSDecl, WriteBogusDefines,
 *	WriteList, WriteNameDecl, WriteVarDecl, WriteTypeDecl,
 *	ReturnTypeStr, WriteStructDecl, WriteStaticDecl,
 *	WriteCopyType, WritePackMsgType.
 *
 *	$Header: /cvs/Darwin/CoreOS/Commands/NeXT/bootstrap_cmds/migcom.tproj/utils.c,v 1.1.1.1 1999/04/15 18:30:38 wsanchez Exp $	
 *
 **********************************************************
 * HISTORY
 * 21-Jul-97  Daniel Wade (danielw) at Apple
 *	Changed _doprnt to call vprintf
 *
 * 03-Jul-97  Daniel Wade (danielw) at Apple
 *      Generated code is now ANSI C compliant
 *
 *  4-Sep-91  Gregg kellogg (gk) at NeXT
 *	Make static type checking ignore unused bits.
 *
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added deallocflag to the WritePackMsg routines.
 *
 * 29-Jul-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed WriteVarDecl to not automatically write
 *	semi-colons between items, so that it can be
 *	used to write C++ argument lists.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 ***************************************************/

#include <mach/message.h>
#include <varargs.h>
#include "write.h"
#include "utils.h"
#include "global.h"
#include <stdarg.h>

#define _doprnt(a, b, c) vfprintf( (c), (a), (b) )

void
WriteImport(file, filename)
    FILE *file;
    string_t filename;
{
    fprintf(file, "#include %s\n", filename);
}

void
WriteRCSDecl(file, name, rcs)
    FILE *file;
    identifier_t name;
    string_t rcs;
{
    fprintf(file, "#ifndef\tlint\n");
    fprintf(file, "#if\tUseExternRCSId\n");
#if	__STDC__
    fprintf(file, "char const %s_rcsid[] = %s;\n", name, rcs);
#else	__STDC__
    fprintf(file, "char %s_rcsid[] = %s;\n", name, rcs);
#endif	__STDC__
    fprintf(file, "#else\t/* UseExternRCSId */\n");
#if	__STDC__
    fprintf(file, "static const char rcsid[] = %s;\n", rcs);
#else	__STDC__
    fprintf(file, "static char rcsid[] = %s;\n", rcs);
#endif	__STDC__
    fprintf(file, "#endif\t/* UseExternRCSId */\n");
    fprintf(file, "#endif\t/* lint */\n");
    fprintf(file, "\n");
}

void
WriteBogusDefines(file)
    FILE *file;
{
    fprintf(file, "#ifndef\tmig_internal\n");
    fprintf(file, "#define\tmig_internal\tstatic\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    if (HeaderFileName == strNULL) {
	/*
	 * No header file to get mig_external from, define it here.
	 */
	fprintf(file, "#ifndef\tmig_external\n");
	fprintf(file, "#define\t mig_external\textern\n");
	fprintf(file, "#endif\n");
	fprintf(file, "\n");
    }

    fprintf(file, "#ifndef\tTypeCheck\n");
    fprintf(file, "#define\tTypeCheck 1\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    fprintf(file, "#ifndef\tUseExternRCSId\n");
    fprintf(file, "#ifdef\thc\n");
    fprintf(file, "#define\tUseExternRCSId\t\t1\n");
    fprintf(file, "#endif\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    /* hc 1.4 can't handle the static msg-type declarations (??) */

    fprintf(file, "#ifndef\tUseStaticMsgType\n");
    fprintf(file, "#if\t!defined(hc) || defined(__STDC__)\n");
    fprintf(file, "#define\tUseStaticMsgType\t1\n");
    fprintf(file, "#endif\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");
}

void
WriteList(file, args, func, mask, between, after)
    FILE *file;
    argument_t *args;
    void (*func)();
    u_int mask;
    char *between, *after;
{
    register argument_t *arg;
    register boolean_t sawone = FALSE;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	{
	    if (sawone)
		fprintf(file, "%s", between);
	    sawone = TRUE;

	    (*func)(file, arg);
	}

    if (sawone)
	fprintf(file, "%s", after);
}

#if	NeXT
/*
 * Same as WriteList(), just don't output the first argument.
 */
void
WriteListSkipFirst(file, args, func, mask, between, after)
    FILE *file;
    argument_t *args;
    void (*func)();
    u_int mask;
    char *between, *after;
{
    register argument_t *arg;
    register boolean_t sawone = FALSE;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	{
	    if (sawone) {
		fprintf(file, "%s", between);
		(*func)(file, arg);
	    } else
		sawone = TRUE;
	}

    if (sawone)
	fprintf(file, "%s", after);
}

const char *
FirstArgName(file, args, mask)
    FILE *file;
    argument_t *args;
    u_int mask;
{
    register argument_t *arg;
    register boolean_t sawone = FALSE;

    for (arg = args; arg != argNULL; arg = arg->argNext)
	if (akCheckAll(arg->argKind, mask))
	{
	    return (const char *)arg->argVarName;
	}

    return NULL;
}

#endif	NeXT

static boolean_t
WriteReverseListPrim(file, arg, func, mask, between)
    FILE *file;
    register argument_t *arg;
    void (*func)();
    u_int mask;
    char *between;
{
    boolean_t sawone = FALSE;

    if (arg != argNULL)
    {
	sawone = WriteReverseListPrim(file, arg->argNext, func, mask, between);

	if (akCheckAll(arg->argKind, mask))
	{
	    if (sawone)
		fprintf(file, "%s", between);
	    sawone = TRUE;

	    (*func)(file, arg);
	}
    }

    return sawone;
}

void
WriteReverseList(file, args, func, mask, between, after)
    FILE *file;
    argument_t *args;
    void (*func)();
    u_int mask;
    char *between, *after;
{
    boolean_t sawone;

    sawone = WriteReverseListPrim(file, args, func, mask, between);

    if (sawone)
	fprintf(file, "%s", after);
}

void
WriteNameDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    fprintf(file, "%s", arg->argVarName);
}

void
WriteVarDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    char *ref = argByReferenceUser(arg) ? "*" : "";

    fprintf(file, "\t%s %s%s", arg->argType->itUserType, ref, arg->argVarName);
}

void
WriteServerVarDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    char *ref = argByReferenceServer(arg) ? "*" : "";

    fprintf(file, "\t%s %s%s", arg->argType->itTransType, ref, arg->argVarName);
}

void
WriteTypeDecl(file, arg)
    FILE *file;
    register argument_t *arg;
{
    WriteStaticDecl(file, arg->argType,
		    arg->argDeallocate, arg->argLongForm,
		    arg->argTTName);
}

void
WriteCheckDecl(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    /* We'll only be called for short-form types.
       Note we use itOutNameStr instead of itInNameStr, because
       this declaration will be used to check received types. */

    fprintf(file, "#if\tUseStaticMsgType\n");
#if	__STDC__
    fprintf(file, "\tstatic const msg_type_t %sCheck = {\n", arg->argVarName);
#else	__STDC__
    fprintf(file, "\tstatic msg_type_t %sCheck = {\n", arg->argVarName);
#endif	__STDC__
    fprintf(file, "\t\t/* msg_type_name = */\t\t%s,\n", it->itOutNameStr);
    fprintf(file, "\t\t/* msg_type_size = */\t\t%d,\n", it->itSize);
    fprintf(file, "\t\t/* msg_type_number = */\t\t%d,\n", it->itNumber);
    fprintf(file, "\t\t/* msg_type_inline = */\t\t%s,\n",
	    strbool(it->itInLine));
    fprintf(file, "\t\t/* msg_type_longform = */\tFALSE,\n");
    fprintf(file, "\t\t/* msg_type_deallocate = */\t%s,\n",
	    strbool(arg->argDeallocate));
    fprintf(file, "\t\t/* msg_type_unused = */\t\t0\n");
    fprintf(file, "\t};\n");
    fprintf(file, "#endif\t/* UseStaticMsgType */\n");
}

char *
ReturnTypeStr(rt)
    routine_t *rt;
{
    if (rt->rtProcedure)
	return "void";
    else
	return rt->rtReturn->argType->itUserType;
}

char *
FetchUserType(it)
    ipc_type_t *it;
{
    return it->itUserType;
}

char *
FetchServerType(it)
    ipc_type_t *it;
{
    return it->itServerType;
}

void
WriteFieldDeclPrim(file, arg, tfunc)
    FILE *file;
    argument_t *arg;
    char *(*tfunc)();
{
    register ipc_type_t *it = arg->argType;

    fprintf(file, "\t\tmsg_type_%st %s;\n",
	    arg->argLongForm ? "long_" : "", arg->argTTName);

    if (it->itInLine && it->itVarArray)
    {
	register ipc_type_t *btype = it->itElement;

	/*
	 *	Build our own declaration for a varying array:
	 *	use the element type and maximum size specified.
	 *	Note arg->argCount->argMultiplier == btype->itNumber.
	 */
	fprintf(file, "\t\t%s %s[%d];",
			(*tfunc)(btype),
			arg->argMsgField,
			it->itNumber/btype->itNumber);
    }
    else
	fprintf(file, "\t\t%s %s;", (*tfunc)(it), arg->argMsgField);

    if (it->itPadSize != 0)
	fprintf(file, "\n\t\tchar %s[%d];", arg->argPadName, it->itPadSize);
}

void
WriteStructDecl(file, args, func, mask, name)
    FILE *file;
    argument_t *args;
    void (*func)();
    u_int mask;
    char *name;
{
    fprintf(file, "\ttypedef struct {\n");
    fprintf(file, "\t\tmsg_header_t Head;\n");
    WriteList(file, args, func, mask, "\n", "\n");
    fprintf(file, "\t} %s;\n", name);
    fprintf(file, "\n");
}

static void
WriteStaticLongDecl(file, it, dealloc, longform, name)
    FILE *file;
    register ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    identifier_t name;
{
#if	__STDC__
    fprintf(file, "\tstatic const msg_type_long_t %s = {\n", name);
#else	__STDC__
    fprintf(file, "\tstatic msg_type_long_t %s = {\n", name);
#endif	__STDC__
    fprintf(file, "\t{\n");
    fprintf(file, "\t\t/* msg_type_name = */\t\t0,\n");
    fprintf(file, "\t\t/* msg_type_size = */\t\t0,\n");
    fprintf(file, "\t\t/* msg_type_number = */\t\t0,\n");
    fprintf(file, "\t\t/* msg_type_inline = */\t\t%s,\n",
	    strbool(it->itInLine));
    fprintf(file, "\t\t/* msg_type_longform = */\t%s,\n",
	    strbool(longform));
    fprintf(file, "\t\t/* msg_type_deallocate = */\t%s,\n",
	    strbool(dealloc));
    fprintf(file, "\t},\n");
    fprintf(file, "\t\t/* msg_type_long_name = */\t%s,\n", it->itInNameStr);
    fprintf(file, "\t\t/* msg_type_long_size = */\t%d,\n", it->itSize);
    fprintf(file, "\t\t/* msg_type_long_number = */\t%d,\n", it->itNumber);
    fprintf(file, "\t};\n");
}

static void
WriteStaticShortDecl(file, it, dealloc, longform, name)
    FILE *file;
    register ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    identifier_t name;
{
#if	__STDC__
    fprintf(file, "\tstatic const msg_type_t %s = {\n", name);
#else	__STDC__
    fprintf(file, "\tstatic msg_type_t %s = {\n", name);
#endif	__STDC__
    fprintf(file, "\t\t/* msg_type_name = */\t\t%s,\n", it->itInNameStr);
    fprintf(file, "\t\t/* msg_type_size = */\t\t%d,\n", it->itSize);
    fprintf(file, "\t\t/* msg_type_number = */\t\t%d,\n", it->itNumber);
    fprintf(file, "\t\t/* msg_type_inline = */\t\t%s,\n",
	    strbool(it->itInLine));
    fprintf(file, "\t\t/* msg_type_longform = */\t%s,\n",
	    strbool(longform));
    fprintf(file, "\t\t/* msg_type_deallocate = */\t%s,\n",
	    strbool(dealloc));
    fprintf(file, "\t\t/* msg_type_unused = */\t\t0,\n");
    fprintf(file, "\t};\n");
}

void
WriteStaticDecl(file, it, dealloc, longform, name)
    FILE *file;
    ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    identifier_t name;
{
    fprintf(file, "#if\tUseStaticMsgType\n");
    if (longform)
	WriteStaticLongDecl(file, it, dealloc, longform, name);
    else
	WriteStaticShortDecl(file, it, dealloc, longform, name);
    fprintf(file, "#endif\t/* UseStaticMsgType */\n");
}

/*ARGSUSED*/
/*VARARGS4*/
void
WriteCopyType(file, it, left, right, va_alist)
    FILE *file;
    ipc_type_t *it;
    char *left, *right;
    va_dcl
{
    va_list pvar;
    va_start(pvar);

    if (it->itStruct)
    {
	fprintf(file, "\t");
	_doprnt(left, pvar, file);
	fprintf(file, " = ");
	_doprnt(right, pvar, file);
	fprintf(file, ";\n");
    }
    else if (it->itString)
    {
#if	NeXT
	fprintf(file, "\t(void) strncpy(");
#else	NeXT
	fprintf(file, "\t(void) mig_strncpy(");
#endif	NeXT
	_doprnt(left, pvar, file);
	fprintf(file, ", ");
	_doprnt(right, pvar, file);
	fprintf(file, ", %d);\n\t", it->itTypeSize);
	_doprnt(left, pvar, file);
	fprintf(file, "[%d] = '\\0';\n", it->itTypeSize - 1);
    }
    else
    {
	fprintf(file, "\t{ typedef struct { char data[%d]; } *sp; * (sp) ",
		it->itTypeSize);
	_doprnt(left, pvar, file);
	fprintf(file, " = * (sp) ");
	_doprnt(right, pvar, file);
	fprintf(file, "; }\n");
    }
    va_end(pvar);
}

static void
WritePackMsgTypeLong(file, it, dealloc, longform, left, pvar)
    FILE *file;
    ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    char *left;
    va_list pvar;
{
    if (it->itInName != MSG_TYPE_POLYMORPHIC)
    {
	/* if polymorphic-in, this field will get filled later */
	fprintf(file, "\t"); _doprnt(left, pvar, file);
	fprintf(file, ".msg_type_long_name = %s;\n", it->itInNameStr);
    }
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_long_size = %d;\n", it->itSize);
    if (!it->itVarArray)
    {
	/* if VarArray, this field will get filled later */
	fprintf(file, "\t"); _doprnt(left, pvar, file);
	fprintf(file, ".msg_type_long_number = %d;\n", it->itNumber);
    }
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_header.msg_type_inline = %s;\n", strbool(it->itInLine));
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_header.msg_type_longform = %s;\n", strbool(longform));
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_header.msg_type_deallocate = %s;\n", strbool(dealloc));
}

static void
WritePackMsgTypeShort(file, it, dealloc, longform, left, pvar)
    FILE *file;
    ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    char *left;
    va_list pvar;
{
    if (it->itInName != MSG_TYPE_POLYMORPHIC)
    {
	/* if polymorphic-in, this field will get filled later */
	fprintf(file, "\t"); _doprnt(left, pvar, file);
	fprintf(file, ".msg_type_name = %s;\n", it->itInNameStr);
    }
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_size = %d;\n", it->itSize);
    if (!it->itVarArray)
    {
	/* if VarArray, this field will get filled later */
	fprintf(file, "\t"); _doprnt(left, pvar, file);
	fprintf(file, ".msg_type_number = %d;\n", it->itNumber);
    }
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_inline = %s;\n", strbool(it->itInLine));
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_longform = %s;\n", strbool(longform));
    fprintf(file, "\t"); _doprnt(left, pvar, file);
    fprintf(file, ".msg_type_deallocate = %s;\n", strbool(dealloc));
}

/*ARGSUSED*/
/*VARARGS4*/
void
WritePackMsgType(file, it, dealloc, longform, left, right, va_alist)
    FILE *file;
    ipc_type_t *it;
    boolean_t dealloc;
    boolean_t longform;
    char *left, *right;
    va_dcl
{
    va_list pvar;
    va_start(pvar);

    fprintf(file, "#if\tUseStaticMsgType\n");
    fprintf(file, "\t");
    _doprnt(left, pvar, file);
    fprintf(file, " = ");
    _doprnt(right, pvar, file);
    fprintf(file, ";\n");
    fprintf(file, "#else\t/* UseStaticMsgType */\n");
    if (longform)
	WritePackMsgTypeLong(file, it, dealloc, longform, left, pvar);
    else
	WritePackMsgTypeShort(file, it, dealloc, longform, left, pvar);
    fprintf(file, "#endif\t/* UseStaticMsgType */\n");

    va_end(pvar);
}
