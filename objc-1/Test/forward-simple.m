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

#import <objc/Object.h>
#import <objc/HashTable.h>
#import <float.h>
#import <stdarg.h>

struct foo 
{
	int i;
	char c;
	float f;
	char *s;
};

static void foo_display(struct foo *s, int i)
{
    printf("arg[%d]->i = %d\n", i, s->i);
    printf("arg[%d]->c = '%c'\n", i, s->c);
    printf("arg[%d]->f = %f\n", i, s->f);
    printf("arg[%d]->s = %s\n", i, s->s);
};

struct large
{
	int a[50];
};

@implementation Tool : Object

- large:(struct large)l small:(int)i
{
	return self;
}

- varargs:(int)n, ... 
{
	va_list ap;
	int i;

	va_start(ap, n); 
	for (i = 0; i < n; i++)
		printf("va_arg[%d] = %d ", i, va_arg(ap,int));
	va_end(ap);
	printf("\n");
}

- double:(double)d char:(char)c float:(float)f double:(double)dd
{
	return self;
}
- incrementMe:(int)i
{
	printf("i = %d\n",i);
	return self;
}

- pointerToStructFoo:(struct foo *)s
{
	printf("i = %d c = '%c' f = %f s = %s\n",s->i,s->c,s->f,s->s);
}

- structFoo:(struct foo)s
{
	printf("i = %d c = '%c' f = %f s = %s\n",s.i,s.c,s.f,s.s);
}

- long:(long)l short:(short)a char:(char)c
{
	return self;
}

- array:(int [7])ary 
{
	return self;
}

@end

@implementation Forward : Object
{
	id destination;
}

- setDestination:dest
{
	destination = dest;
}

static int getTagLen(char *tag)
{
	int len = 0;

	while (*tag++ != _C_STRUCT_E) len++;

	return len;
}

/*
   Purpose: 	Provide support required when forwarding messages 
		to remote objects. -performv::, -methodArgSize:,
		method_getNumberOfArguments(), method_getSizeOfArguments(),
		and method_getArgumentInfo() provide additional support
		for understanding the format of the stack. This level of
		support is currently used exclusively by Workspace and
		is part of the Unpublished API for Remote Objects.

   Semantics:	This function is responsible for two things:
		(1) obtaining the object that does respond to the selector.
		(2) sending the object the message via "performv::"	
   Features:	(1) chance to analyze/transform arguments on the stack 
		    before sending the message.
		(2) chance to perform a post operation after sending 
		    the message.
   Limitations:	(1) cannot forward methods that take a variable number 
		    of arguments.
		(2) forwarding methods that return values greater than
		    `sizeof(id)' are unreliable.
*/

- forward:(SEL)asel :(marg_list)args
{
	void *retval;
	Method method;
	int i, nargs;

	// analyze argument types of an anonymous (un-known) selector...
	// ...necessary for doing remote objects.

 	method = class_getInstanceMethod([destination class], asel);
 	nargs = method_getNumberOfArguments(method);

	for (i = 0; i < nargs; i++) {
		char *type;
		int offset;

		method_getArgumentInfo(method, i, &type, &offset);

		printf ("arg[%d] (offset = %d, type = '%c')",
			i, offset, type[0]);

		switch (type[0]) {
		   case _C_ID:
			{
			id obj = marg_getValue(args, offset, id);
			printf(" = %s\n", [obj name]);
			break;
			}
		   case _C_SEL:
			{
			SEL sel = marg_getValue(args, offset, SEL);
			printf(" = %s\n", sel_getName(sel));
			break;
			}
		   case _C_CHARPTR:
			{
			char *str = marg_getValue(args, offset, char *);
			printf(" = %s\n", str);
			break;
			}
		   case _C_LNG: case _C_INT:
			{
			long longVal = marg_getValue(args, offset, long);
			printf(" = %d\n", longVal);

			if (asel == @selector(incrementMe:))
				marg_setValue(args, offset, long, ++longVal);
			break;
			}
		   case _C_SHT:
			{
			short shortVal = marg_getValue(args, offset, short);
			printf(" = %d\n", shortVal); 
			break;
			}
		   case _C_CHR:
			{
			char charVal = marg_getValue(args, offset, char);
			printf(" = '%c'\n", charVal);
			break;
			}
		   case _C_FLT:
			{
			float floatVal = marg_getValue(args, offset, float);
			printf(" = %f\n", floatVal);
			break;
			}
		   case _C_DBL:
			{
			double doubleVal = marg_getValue(args, offset, double);
			printf(" = %f\n", doubleVal);
			break;
			}
		   case _C_PTR:
			{
			if (type[1] == _C_STRUCT_B) 
			  {
			  if (strncmp(&type[2], "foo", getTagLen(&type[2])) == 0) 
			    {
			    struct foo *s = marg_getValue(args, offset, struct foo *);
			    foo_display(s, i);
			    }
			  }
			break;
			}
		   case _C_STRUCT_B:
			{
			if (strncmp(&type[1], "foo", getTagLen(&type[1])) == 0) 
			  {
			  struct foo s = marg_getValue(args, offset, struct foo);
			  foo_display(marg_getRef(args, offset, struct foo), i);

			  s.i++, s.c++, s.f++, s.s = "transformed string";
			  marg_setValue(args, offset, struct foo, s);
			  }
			break;
			}
		   case _C_ARY_B:
			{
			printf("array type not recognized ('%s')\n", type);
			}
		   default:
			printf("type not recognized ('%c')\n", type[0]);
			break;
		}
	}
	retval = [destination performv:asel :args];

	return retval;
}

@end


main()
{
	id forward = [Forward new];
	id tool = [Tool new];

	struct foo afoo = {33, 'y', 9.9, "great"};
	struct large alarge;
	int ary[7];

	[tool varargs:3, 1,2,3];
	[tool long:77 short:7 char:'z'];
	[tool incrementMe:100];
	[tool pointerToStructFoo:&afoo];
	[tool structFoo:afoo];
	[tool double:4.4 char:'5' float:3.4 double:8.8];
	[tool large:alarge small:7007];
	[tool array:ary];

 	[forward setDestination:tool];

	[forward varargs:3, 1,2,3];
	[forward long:77 short:7 char:'z'];
	[forward incrementMe:100];
	[forward pointerToStructFoo:&afoo];
	[forward structFoo:afoo];
	[forward double:4.4 char:'5' float:3.4 double:8.8];
	[forward large:alarge small:7007];
	[forward array:ary];
	printf("\n");
}
