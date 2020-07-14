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
#ifdef SHLIB
#include "shlib.h"
#endif
/*
    except.c

    This file implements the exception raising scheme.
	It is thread safe, although Alt nodes may not behave
	as expected.

    Copyright (c) 1988, 1991 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#ifdef KERNEL
#import <mach/mach_types.h>
#import <kernserv/prototypes.h>
#else /* KERNEL */
#import <mach/cthreads.h>
#import <stdio.h>
#endif /* KERNEL */

#import "error.h"
#ifndef	KERNEL
#import <stdlib.h>
#endif

extern void _NXLogError (const char *format, ...);

#ifdef KERNEL

#import <mach/machine/simple_lock.h>

#ifdef hppa
#define SIMPLE_LOCK_INITIALIZER { 1, 1, 1, 1 }
#else
#define SIMPLE_LOCK_INITIALIZER { 1 }
#endif

#define LOCK_T			simple_lock_data_t
#define LOCK_INITIALIZER SIMPLE_LOCK_INITIALIZER
#define LOCK(x)			simple_lock(&(x))
#define UNLOCK(x)		simple_unlock(&(x))

#define cthread_t	thread_t
#define cthread_self()	current_thread()

#else  /* KERNEL */

#define	LOCK_T				struct mutex
#define LOCK_INITIALIZER	MUTEX_INITIALIZER
#define LOCK(x)				mutex_lock(&x)
#define UNLOCK(x) 			mutex_unlock(&x)

#endif /* KERNEL */

#ifdef KERNEL
//int _setjmp(jmp_buf env) { return setjmp(env); }
//void _longjmp(jmp_buf env, int val) { longjmp(env, val); }
#define _setjmp(env) { return setjmp(env); }
#define _longjmp(env, val) { longjmp(env, val); }

NXUncaughtExceptionHandler *_NXUncaughtExceptionHandler;

#endif /* KERNEL */

typedef void AltProc(void *context, int code, const void *data1, const void *data2);

/*  These nodes represent handlers that are called as normal procedures
    instead of longjmp'ed to.  When these procs return, the next handler
    in the chain is processed.
 */
typedef struct {		/* an alternative node in the handler chain */
    struct _NXHandler *next;		/* ptr to next handler */
    AltProc *proc;			/* proc to call */
    void *context;			/* blind data for client */
} AltHandler;

static int ErrorBufferSize = 0;
static NXExceptionRaiser *ExceptionRaiser = &NXDefaultExceptionRaiser;

 /* Multiple thread support.
	Basic strategy is to collect globals into a struct; singly link them
	(probably not worth it put them in a hashtable) using the cthread id
	as a key.  Upon use, look up proper stack of handlers...
 */

typedef struct xxx {
	NXHandler *handlerStack;	/* start of handler chain */
	AltHandler *altHandlers;
	int altHandlersAlloced;
	int altHandlersUsed;
	cthread_t	thread;			/* key */
	struct xxx	*next;			/* link */
} ExceptionHandlerStack;

/* Allocate the handler stack for the first thread statically so that
   we don't keep an extra page in the heap hot (the libsys data is
   already hot).  Also statically allocate a small number of altHandlers.
   This should be the maximum typical depth of lockFocus'es. */

static AltHandler BaseAltHandlers[16] = { 0 };

static ExceptionHandlerStack Base =
{
  NULL,
  BaseAltHandlers,
  sizeof (BaseAltHandlers) / sizeof (BaseAltHandlers[0]),
  0,
  0,
  NULL
};

static LOCK_T Lock = LOCK_INITIALIZER;

#ifndef	KERNEL
static void _NXClearExceptionStack (void);
extern void _set_cthread_free_callout (void (*) (void));
#endif	KERNEL

static ExceptionHandlerStack *addme (cthread_t self)
{
  ExceptionHandlerStack *stack;
  
  LOCK (Lock);			// lookup is thread safe; addition isn't
  
#ifdef	KERNEL
  for (stack = &Base; stack; stack = stack->next)
    if (stack->thread == 0) {
      stack->thread = self;
      UNLOCK (Lock);
      return stack;
    }
#else	KERNEL
  if (Base.thread == 0)		// try statically allocated stack
    {
      Base.thread = self;
      UNLOCK (Lock);
      return &Base;
    }
  
  /* Pass exception handler cleanup routine to cthread package.  */
  _set_cthread_free_callout (&_NXClearExceptionStack);
#endif	KERNEL
  
  stack = calloc (sizeof (ExceptionHandlerStack), 1);
  stack->thread = self;
  stack->next = Base.next;
  Base.next = stack;		// insert atomically
  
  UNLOCK (Lock);
  
  return stack;
}


/*
	Get callers thread & find | allocate a stack.  Allocation is only
	proper for some usages, but we don't check for them...
	Also, these structs are not recovered upon thread death. XXX
*/
static inline ExceptionHandlerStack *findme (void)
{
  ExceptionHandlerStack *stack;
  cthread_t self = cthread_self ();
  
  for (stack = &Base; stack; stack = stack->next)
    if (stack->thread == self)
      return stack;
  
  return addme (self);
}

#ifdef	KERNEL
/*
 * Call back from thread_deallocate()
 * to indicate that a thread is being
 * destroyed.  Kernel thread ids aren't
 * reused the way that cthread ids are,
 * so we have to nuke the value in the
 * structure.
 *
 * XXX Should free data structures here.
 */
void
_threadFreeExceptionStack(
    thread_t		thread
)
{
  ExceptionHandlerStack *stack;

  // Don't allocate a stack for this thread if it doesn't already have one.
  for (stack = &Base; stack; stack = stack->next)
    if (stack->thread == thread)
      break;

  if (stack)
    {
      stack->handlerStack = 0;    	 // reset the handler chain
      stack->altHandlersUsed = 0; 	 // reset the alt handler count
      stack->thread = 0;
    }
}
#else	KERNEL		
/* cthreads will reuse a stack and a cthread_id.
 * We provide this call-in to clean up matters
 */
static void _NXClearExceptionStack (void)
{
  ExceptionHandlerStack *stack;
  cthread_t self = cthread_self ();
  
  // Don't allocate a stack for this thread if it doesn't already have one.
  for (stack = &Base; stack; stack = stack->next)
    if (stack->thread == self)
      break;

  if (stack)
    {
      stack->handlerStack = 0;    	 // reset the handler chain
      stack->altHandlersUsed = 0; 	 // reset the alt handler count
    }
}
#endif	KERNEL

#define IS_ALT(ptr)			((int)(ptr) % 2)
#define NEXT_HANDLER(ptr)		\
	(IS_ALT(ptr) ? ALT_CODE_TO_PTR(ptr)->next : ((NXHandler *)ptr)->next)
#define ALT_PTR_TO_CODE(ptr)		(((ptr) - me->altHandlers) * 2 + 1)
#define ALT_CODE_TO_PTR(code)		(me->altHandlers + ((int)(code) - 1) / 2)


/* if the node passed in isnt on top of the stack, something's fishy */

static void trickyRemoveHandler (NXHandler *handler, int removingAlt)
{
  AltHandler *altNode;
  NXHandler *node;
  NXHandler **nodePtr;
  ExceptionHandlerStack *me = findme ();
  
  /* try to find the node anywhere on the stack */
  node = me->handlerStack;
  while (node != handler && node)
    {
      /* Watch for attempts to remove handlers which are outside the
	 active portion of the stack.  This happens when you return
	 from an NX_DURING context without removing the handler.
	 This code assumes the stack grows downward. */
#if hppa
	/* stack grows upward */
      if (IS_ALT (node) || (void *) node < (void *) &altNode)
	node = NEXT_HANDLER (node);
      else
#else
      if (IS_ALT (node) || (void *) node > (void *) &altNode)
	node = NEXT_HANDLER (node);
      else
#endif
	{
	  _NXLogError ("Exception handlers were not properly removed.");
	  abort ();
	}
    }
  
  if (node)
    {
      /* 
      * Clean off the stack up to the out of place node.  If we are trying
      * to remove an non-alt handler, we pop eveything off the stack
      * including that handler, calling any alt procs along the way.  If
      * we are removing an alt handler, we leave all the non-alt handlers
      * alone as we clean off the stack, but pop off all the alt handlers
      * we find, including the node we were asked to remove.
      */
      if (!removingAlt)
	_NXLogError ("Exception handlers were not properly removed.");
      
      nodePtr = &me->handlerStack;
      do
	{
	  node = *nodePtr;
	  if (IS_ALT(node))
	    {
	      altNode = ALT_CODE_TO_PTR(node);
	      if (removingAlt)
	        {
		  if (node == handler)
		    *nodePtr = altNode->next;	/* del matching node */
		  else
		    nodePtr = &altNode->next;	/* skip node */
	        }
	      else
	        {
		  if (node != handler)
		    (*altNode->proc)(altNode->context, 1, 0, 0);
		  me->altHandlersUsed = altNode - me->altHandlers;
		  *nodePtr = altNode->next;	/* del any alt node */
	        }
	    }
	  else
	    {
	      if (removingAlt)
		nodePtr = &node->next;		/* skip node */
	      else
		*nodePtr = node->next;		/* nuke non-alt node */
	    }
	}
      while (node != handler);
    }
  else
    {
#ifdef KERNEL
      ;
#else /* KERNEL */
      _NXLogError ("Attempt to remove unrecognized exception handler.");
#endif /* KERNEL */
    }
}


NXHandler *_NXAddAltHandler (AltProc *proc, void *context)
{
  AltHandler *new;
  ExceptionHandlerStack *me = findme ();

  if (me->altHandlersUsed == me->altHandlersAlloced)
    {
      if (me->altHandlers == BaseAltHandlers)
	{
	  me->altHandlers = malloc (++me->altHandlersAlloced *
				    sizeof(AltHandler));
	  bcopy (BaseAltHandlers, me->altHandlers, sizeof (BaseAltHandlers));
	}
      else
	me->altHandlers = realloc (me->altHandlers, ++me->altHandlersAlloced *
						    sizeof(AltHandler));
    }
  
  new = me->altHandlers + me->altHandlersUsed++;
  new->next = me->handlerStack;
  me->handlerStack = (NXHandler *) ALT_PTR_TO_CODE (new);
  new->proc = proc;
  new->context = context;
  return me->handlerStack;
}


void _NXRemoveAltHandler (NXHandler *handler)
{
  ExceptionHandlerStack *me;
  
  for (me = &Base; me; me = me->next)
    if (me->handlerStack == handler)
      {
	AltHandler *altNode = ALT_CODE_TO_PTR (handler);
	
	me->altHandlersUsed = altNode - me->altHandlers;
	me->handlerStack = altNode->next;
	return;
      }
  
  trickyRemoveHandler (handler, TRUE);
}


void _NXAddHandler (NXHandler *handler)
{
  ExceptionHandlerStack *me = findme ();
  
  handler->next = me->handlerStack;
  me->handlerStack = handler;
  handler->code = 0;
}


void _NXRemoveHandler (NXHandler *handler)
{
  ExceptionHandlerStack *me;
  
  for (me = &Base; me; me = me->next)
    if (me->handlerStack == handler)
      {
        me->handlerStack = me->handlerStack->next;
	return;
      }
  
  trickyRemoveHandler (handler, FALSE);
}


void NXSetExceptionRaiser(NXExceptionRaiser *proc)
{
    ExceptionRaiser = proc;
}


NXExceptionRaiser *NXGetExceptionRaiser (void)
{
    return ExceptionRaiser;
}

volatile void _NXRaiseError(int code, const void *data1, const void *data2)
{
    (*ExceptionRaiser)(code, data1, data2);
    abort ();	/* we should never get here! */
}


/* forwards the error to the next handler */
volatile void NXDefaultExceptionRaiser(int code, const void *data1, const void *data2)
{
    NXHandler *destination;
    AltHandler *altDest;
	ExceptionHandlerStack *me = findme ();

    while (1) {
		destination = me->handlerStack;
		if (!destination) {
			if (_NXUncaughtExceptionHandler)
				(*_NXUncaughtExceptionHandler)(code, data1, data2);
			else {
#ifndef KERNEL
					_NXLogError("Uncaught exception #%d\n", code);
#endif /* not KERNEL */
			}
#ifdef KERNEL
				panic("Uncaught exception");
#else /* KERNEL */
				exit(-1);
#endif /* KERNEL */
		} else if (IS_ALT(destination)) {
			altDest = ALT_CODE_TO_PTR(destination);
			me->handlerStack = altDest->next;
			me->altHandlersUsed = altDest - me->altHandlers;
			(*altDest->proc)(altDest->context, code, data1, data2);
		} else {
			destination->code = code;
			destination->data1 = data1;
			destination->data2 = data2;
			me->handlerStack = destination->next;
			_longjmp(destination->jumpState, 1);
		}
    }
}


static char *ErrorBuffer = NULL;
static int ErrorBufferOffset = 0;

/* stack allocates some space from the error buffer */
void NXAllocErrorData(int size, void **data)
{
    int goalSize;
	static LOCK_T mylock = LOCK_INITIALIZER;
	
	LOCK (mylock);
    goalSize = (ErrorBufferOffset + size + 7) & ~7;
    if (goalSize > ErrorBufferSize) {
		ErrorBuffer = realloc(ErrorBuffer, (unsigned)goalSize);
		ErrorBufferSize = goalSize;
    }
    *data = ErrorBuffer + ErrorBufferOffset;
    ErrorBufferOffset = goalSize;
	UNLOCK (mylock);
}


void NXResetErrorData(void)
{
    ErrorBufferOffset = 0;		// multiple threads users must cooperate XXX
}
