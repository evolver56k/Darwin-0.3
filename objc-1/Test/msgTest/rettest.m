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
// Test all possible Objective-C method return types
// This test exercises both _msg and _msgSuper to make sure that
// all possible return types and handled correctly.  There are
// two classes used: MsgTest and SuperTest.
//
// Class MsgTest defines one method for each possible return type.
// Each method takes a single argument which is a pointer to a
// location to put a copy of the return value.  Each method stores
// its return value in that location, then returns it.  The caller
// verifies that the return value and the stored copy match.
//
// Class SuperTest does the same, and simply passes the message and
// argument on to the superclass via a message to "super".
//
// Usage:
//	rettest [ msgFlag ]
//
// If invoked with an argument, the message trace flag is enabled.
//
// Diagnostics:
//	ERROR: return value mismatch while testing '%s' in class %s
//
// A return value does not match what was expected.
//
// The first '%s' says which type is currently under test; the second
// names either "MsgTest" or "SuperTest" to indicate which message
// routine may be faulty.  Note that because SuperTest simply passes
// the message on, if there is a problem in the _msg function, it will
// show up on both tests.
//
// This diagnostic will also be accompanied by "%V:%V", where "%V"
// indicates some representation of a value.  The first is the value
// actually returned; the second is what was expected.
//
// Also printed is the filename and line number containing the test
// which failed.
//
// For example, if a method returning a double didn't match the expected
// value, you would see an error like:
//
//		return value mismatch while testing 'double' in class MsgTest
//			line 179 in file "rettest.m":
//			3.1416:2.9999935e+231
//
//	ERROR: unexpected exit while processing test '%s' using %s.
//
// An unexpected exit was called during testing, most likely indicating
// a program fault.  If so, the fault will also be diagnosed with a stack
// backtrace.
//
// As before, the first '%s' is the type currently under test; the
// second is either "MsgTest" or "SuperTest".


#include <objc/objc.h>
#include "rettest.h"
#include <objc/objc-class.h>
#include <stdio.h>

int currentLine = 0;
char *currentTest = "", *currentFile = "";
id currentReceiver = nil;
int nErrors = 0;


#import "ret1.h"
#import "ret2.h"


// exit handler
// This prints a diagnostic if an exit is called in the middle of processing,
// for example if a segmentation violation resulted from one of the test
// methods.
myCleanup(estat,arg)
int estat;
char* arg;
{
	if (estat != 0) {
		fprintf (stderr,
			 "ERROR: unexpected exit while processing test '%s' using %s\n\t",
			 currentTest, [currentReceiver str]);
		fprintf (stderr, "line %d in file \"%s\"\n", currentLine, currentFile);
	}
}

//extern id (*_objc_msgPreop)();
//extern void _objc_msgCollectStats();
//extern void _objc_msgPrintStats();

main(argc, argv)
int	argc;
char	**argv;
{
	id foo = 0;

	if (argc > 1)
	  {
//	  _objc_msgPreop = _objc_msgCollectStats;
	  }

	currentReceiver = [MsgTest new];

	doTests(currentReceiver);
	// print hit/miss info.
//	if (_objc_msgPreop)
//	  _objc_msgPrintStats();

	doTests(currentReceiver);
	// print hit/miss info.
//	if (_objc_msgPreop)
//	  _objc_msgPrintStats();

	if (nErrors == 0)
		fprintf (stderr, "All return tests to `MsgTest' succeeded\n");
	else
		fprintf (stderr, "%d errors noted\n", nErrors);

	[foo bar];
#if 1
	currentReceiver = [SuperTest new];
	doTests(currentReceiver);

	// print hit/miss info.
//	if (_objc_msgPreop)
//	  _objc_msgPrintStats();

	if (nErrors == 0)
		fprintf (stderr, "All return tests to `SuperTest' succeeded\n");
	else
		fprintf (stderr, "%d errors noted\n", nErrors);

#endif
	exit (0);	// OK exit
}

// Function to call all the test methods for the argument
// instance.
doTests(retInstance)
id retInstance;
{
	// scalars
	RETDECL(char);
	RETDECL(uchar_t);
	RETDECL(short);
	RETDECL(ushort_t);
	RETDECL(int);
	RETDECL(unsigned);
	RETDECL(long);
	RETDECL(ulong_t);
	RETDECL(float);
	RETDECL(double);
	// pointers
	RETDECL(id);
	RETDECL(STR);
	// structures
	RETDECL(S_BITS_16_t);
	RETDECL(S_BITS_32_t);
	RETDECL(S_BITS_64_t);
	RETDECL(S_BITS_BIG_t);
	// unions
	RETDECL(U_BITS_16_t);
	RETDECL(U_BITS_32_t);
	RETDECL(U_BITS_64_t);
	RETDECL(U_BITS_BIG_t);
	// enums
	RETDECL(E_BITS_8_t);
	RETDECL(E_BITS_16_t);
#ifdef INT_32
	RETDECL(E_BITS_32_t);
#endif


	NEXTTEST("char");
	if (RETTEST(retInstance,char)) {
		RETERR("%d:%d",char_value, char_refValue);
	}

	NEXTTEST("unsigned char");
	if (RETTEST(retInstance,uchar_t)) {
		RETERR("%u:%u",uchar_t_value, uchar_t_refValue);
	}

	NEXTTEST("short");
	if (RETTEST(retInstance,short)) {
		RETERR("%d:%d",short_value, short_refValue);
	}

	NEXTTEST("unsigned short");
	if (RETTEST(retInstance,ushort_t)) {
		RETERR("%u:%u",ushort_t_value, ushort_t_refValue);
	}

	NEXTTEST("int");
	if (RETTEST(retInstance,int)) {
		RETERR("%d:%d",int_value, int_refValue);
	}
	
	NEXTTEST("unsigned int");
	if (RETTEST(retInstance,unsigned)) {
		RETERR("%u:%u",unsigned_value, unsigned_refValue);
	}

	NEXTTEST("long");
	if (RETTEST(retInstance,long)) {
		RETERR("%ld:%ld",long_value, long_refValue);
	}

	NEXTTEST("unsigned long");
	if (RETTEST(retInstance,ulong_t)) {
		RETERR("%lu:%lu",ulong_t_value, ulong_t_refValue);
	}

	NEXTTEST("float");
	if (RETTEST(retInstance,float)) {
		RETERR("%.8g:%.8g",float_value, float_refValue);
	}

	NEXTTEST("double");
	if (RETTEST(retInstance,double)) {
		RETERR("%.17g:%.17g",double_value, double_refValue);
	}

	// pointer types
	NEXTTEST("id");
	if (RETTEST(retInstance,id)) {
		RETERR("%lx:%lx",id_value, id_refValue);
	}

	NEXTTEST("char*");
	if (RETTEST(retInstance,STR)) {
		RETERR4("%lx=%s:%lx=%s",STR_value, STR_value,
			STR_refValue, STR_refValue);
	}

	// Now test the aggregate types
	NEXTTEST("struct S_BITS_16");
	if (SRETTEST(retInstance,S_BITS_16_t)) {
		RETERR("*%lx:*%lx",&S_BITS_16_t_value, &S_BITS_16_t_refValue);
	}

	NEXTTEST("struct S_BITS_32");
	if (SRETTEST(retInstance,S_BITS_32_t)) {
		RETERR("*%lx:*%lx",&S_BITS_32_t_value, &S_BITS_32_t_refValue);
	}

	NEXTTEST("struct S_BITS_64");
	if (SRETTEST(retInstance,S_BITS_64_t)) {
		RETERR("*%lx:*%lx",&S_BITS_64_t_value, &S_BITS_64_t_refValue);
	}

	NEXTTEST("struct S_BITS_BIG");
	if (SRETTEST(retInstance,S_BITS_BIG_t)) {
		RETERR("*%lx:*%lx",&S_BITS_BIG_t_value, &S_BITS_BIG_t_refValue);
	}

	// unions
	NEXTTEST("union U_BITS_16");
	if (SRETTEST(retInstance,U_BITS_16_t)) {
		RETERR("*%lx:*%lx",&U_BITS_16_t_value, &U_BITS_16_t_refValue);
	}

	NEXTTEST("union U_BITS_32");
	if (SRETTEST(retInstance,U_BITS_32_t)) {
		RETERR("*%lx:*%lx",&U_BITS_32_t_value, &U_BITS_32_t_refValue);
	}

	NEXTTEST("union U_BITS_64");
	if (SRETTEST(retInstance,U_BITS_64_t)) {
		RETERR("*%lx:*%lx",&U_BITS_64_t_value, &U_BITS_64_t_refValue);
	}

	NEXTTEST("union U_BITS_BIG");
	if (SRETTEST(retInstance,U_BITS_BIG_t)) {
		RETERR("*%lx:*%lx",&U_BITS_BIG_t_value, &U_BITS_BIG_t_refValue);
	}

	NEXTTEST("enum E_BITS_8");
	if (RETTEST(retInstance,E_BITS_8_t)) {
		RETERR("*%d:*%d",E_BITS_8_t_value, E_BITS_8_t_refValue);
	}

	NEXTTEST("enum E_BITS_16");
	if (RETTEST(retInstance,E_BITS_16_t)) {
		RETERR("*%d:*%d",E_BITS_16_t_value, E_BITS_16_t_refValue);
	}

#if 0
#ifdef INT_32
	NEXTTEST("enum E_BITS_32");
	if (RETTEST(retInstance,E_BITS_32_t)) {
		RETERR("*%d:*%d",E_BITS_32_t_value, E_BITS_32_t_refValue);
	}
#endif
#endif

}

// function to handle errors coming from return value
// mismatches.  Print a standard tag and the received
// and expected values.
#import <stdarg.h>

retError(char *fmt, char *file, int line, ...)
{
	va_list vp;

	nErrors += 1;
	fprintf (stderr, "ERROR: return value mismatch while testing '%s' in class %s\n\t",
		 currentTest, [currentReceiver str]);
	fprintf (stderr, "line %d in file \"%s\":\n\t", line, file);
	va_start(vp, line);
	vfprintf(stderr,fmt,vp);
	va_end(vp);
	fputc('\n',stderr);
}

doArgCheck(class,method,self,_cmd)
Class class;
SEL method;
id self;
SEL _cmd;
{
	if (self->isa != class && self->isa->super_class != class)
		error("bad 'self' argument in '%s.%s': 0x%lx",
			class->name, SELNAME(method), self);
	if (method != _cmd)
		error("bad '_cmd' argument in '%s.%s': 0x%lx",
			class->name, method, SELNAME(_cmd));
}

// General error discovered in some class
error(char *fmt, ...)
{
	va_list vp;

	nErrors += 1;
	fprintf (stderr, "ERROR: ");
	va_start(vp, fmt);
	vfprintf(stderr,fmt,vp);
	va_end(vp);
	fputc('\n',stderr);
}

// Compare two arbitrary sized objects one byte at a time.
retCompare(a1,a2,size)
char *a1, *a2;
int size;
{
	while (size-- > 0)
		if (*a1++ != *a2++)
			return 1;
	return 0;
}

