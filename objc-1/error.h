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
    error.h

    This file defines the interface to the exception raising scheme.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#ifndef _OBJC_ERROR_H_
#define _OBJC_ERROR_H_

#include <setjmp.h>

typedef struct _NXHandler {	/* a node in the handler chain */
    jmp_buf jumpState;			/* place to longjmp to */
    struct _NXHandler *next;		/* ptr to next handler */
    int code;				/* error code of exception */
    const void *data1, *data2;		/* blind data for describing error */
} NXHandler;


/* Handles RAISE's with nowhere to longjmp to */
typedef void NXUncaughtExceptionHandler(int code, const void *data1,
						const void *data2);
extern NXUncaughtExceptionHandler *_NXUncaughtExceptionHandler;
#define NXGetUncaughtExceptionHandler() _NXUncaughtExceptionHandler
#define NXSetUncaughtExceptionHandler(proc) \
			(_NXUncaughtExceptionHandler = (proc))

/* NX_DURING, NX_HANDLER and NX_ENDHANDLER are always used like:

	NX_DURING
	    some code which might raise an error
	NX_HANDLER
	    code that will be jumped to if an error occurs
	NX_ENDHANDLER

   If any error is raised within the first block of code, the second block
   of code will be jumped to.  Typically, this code will clean up any
   resources allocated in the routine, possibly case on the error code
   and perform special processing, and default to RERAISE the error to
   the next handler.  Within the scope of the handler, a local variable
   called NXLocalHandler of type NXHandler holds information about the
   error raised.

   It is illegal to exit the first block of code by any other means than
   NX_VALRETURN, NX_VOIDRETURN, or just falling out the bottom.
 */

/* private support routines.  Do not call directly. */
extern void _NXAddHandler( NXHandler *handler );
extern void _NXRemoveHandler( NXHandler *handler );
extern _setjmp(jmp_buf env);

#define NX_DURING { NXHandler NXLocalHandler;			\
		    _NXAddHandler(&NXLocalHandler);		\
		    if( !_setjmp(NXLocalHandler.jumpState) ) {

#define NX_HANDLER _NXRemoveHandler(&NXLocalHandler); } else {

#define NX_ENDHANDLER }}

#define NX_VALRETURN(val)  do { typeof(val) temp = (val);	\
			_NXRemoveHandler(&NXLocalHandler);	\
			return(temp); } while (0)

#define NX_VOIDRETURN	do { _NXRemoveHandler(&NXLocalHandler);	\
			return; } while (0)

/* RAISE and RERAISE are called to indicate an error condition.  They
   initiate the process of jumping up the chain of handlers.
 */

extern 
#ifdef __GNUC__
#ifndef __STRICT_ANSI__
__volatile	/* never returns */
#endif /* not __STRICT_ANSI__ */
#endif /* __GNUC__ */
void _NXRaiseError(int code, const void *data1, const void *data2)
#ifdef __GNUC__
  __attribute__ ((noreturn))
#endif
;

#define NX_RAISE( code, data1, data2 )	\
		_NXRaiseError( (code), (data1), (data2) )

#define NX_RERAISE() 	_NXRaiseError( NXLocalHandler.code,	\
				NXLocalHandler.data1, NXLocalHandler.data2 )

/* These routines set and return the procedure which is called when
   exceptions are raised.  This procedure must NEVER return.  It will
   usually either longjmp, or call the uncaught exception handler.
   The default exception raiser is also declared
 */
typedef volatile void NXExceptionRaiser(int code, const void *data1, const void *data2);
extern void NXSetExceptionRaiser(NXExceptionRaiser *proc);
extern NXExceptionRaiser *NXGetExceptionRaiser(void);
extern NXExceptionRaiser NXDefaultExceptionRaiser;


/* The error buffer is used to allocate data which is passed up to other
   handlers.  Clients should clear the error buffer in their top level
   handler.  The Application Kit does this.
 */
extern void NXAllocErrorData(int size, void **data);
extern void NXResetErrorData(void);

#endif /* _OBJC_ERROR_H_ */
