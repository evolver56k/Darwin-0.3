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
#import <appkit/screens.h>
#import <appkit/graphics.h>
#import <appkit/color.h>
#import <objc/objc-class.h>
#import <mach/mach_types.h>
#import <netinfo/ni.h>
#import <mach-o/ldsyms.h>
#import <objc/Protocol.h>

@protocol kitmethods

- beginPageSetupRect:(const NXRect *)aRect placement:(const NXPoint *)location;
- placePrintRect:(const NXRect *)aRect offset:(NXPoint *)location;
- moveTopLeftTo:(NXCoord)x :(NXCoord)y;
- setBackgroundColor:(NXColor)color;
- copyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream;
- windowResized:(NXEvent *)theEvent;

@end

@protocol testMethodQualifiers <kitmethods>

- in:(in int *)meaningless out:(out unsigned int *)y;
- inout:(inout int *)weeb bycopy:(bycopy id)x;
- structReference:(const NXRect *)s :(const NXRect **)y;
- structValue:(NXPoint)s;
- (oneway)asyncronousMessage;
+ someClassMethod;

@end

@protocol GetValues
- (int)intValue;
- (float)floatValue;
- (double)doubleValue;
- (const char *)stringValue;
@end

@protocol SetValues
- setIntValue:(int)anInt;
- setFloatValue:(float)aFloat;
- setDoubleValue:(double)aDouble;
- setStringValue:(const char *)aString;
@end

@protocol Values <SetValues, GetValues>
@end

@interface Example1 : Object <SetValues, GetValues>
@end

@implementation Example1
@end

static void test_conformsTo()
{
	Object <SetValues, GetValues> *ex = [Example1 new];
	
	/* test protocol lookup */
	
	Protocol *pobj2 = [Protocol getProtocol:"kitmethods"], 
	         *pobj3 = [Protocol getProtocol:"testMethodQualifiers"];

	/* test protocol conformance for objects */
	
	if ([ex conformsTo:[Protocol getProtocol:"SetValues"]])
	  printf("ex DOES conform to SetValues\n");
	if (![ex conformsTo:[Protocol getProtocol:"Values"]])
	  printf("ex DOES NOT conform to Values\n");
	  
	if ([ex conformsToGivenName:"SetValues"])
	  printf("ex DOES conform to SetValues\n");
	if (![ex conformsToGivenName:"Values"])
	  printf("ex DOES NOT conform to Values\n");

	/* test protocol conformance for classes */
	
	if ([[Example1 class] conformsTo:[Protocol getProtocol:"SetValues"]])
	  printf("Example1 DOES conform to SetValues\n");
	if (![[Example1 class] conformsTo:[Protocol getProtocol:"Values"]])
	  printf("Example1 DOES NOT conform to Values\n");

	/* test protocol conformance for protocols */
	  
	if (![pobj2 conformsTo:pobj3])
	  printf("kitmethods DO NOT conform to testMethodQualifiers\n");
	if (![pobj2 conformsToGivenName:[pobj3 protocolName]])
	  printf("kitmethods DO NOT conform to testMethodQualifiers\n");
	  
	if ([pobj3 conformsTo:pobj2])
	  printf("testMethodQualifiers DO conform to kitmethods\n");
	if ([pobj3 conformsToGivenName:[pobj2 protocolName]])
	  printf("testMethodQualifiers DO conform to kitmethods\n");
}

static void test_methodDescFor()
{
	Protocol *valuesProtocol = [Protocol getProtocol:"Values"];
	struct objc_method_desc *m;
		
	m = [valuesProtocol instanceMethodDescFor:@selector(setIntValue:)];
	if (m)
	  printf("name = %s types = %s\n",sel_getName(m->name),m->types);
	  
	m = [valuesProtocol instanceMethodDescFor:@selector(intValue)];
	if (m)
	  printf("name = %s types = %s\n",sel_getName(m->name),m->types);
}

main()
{
	test_conformsTo();
	test_methodDescFor();
}
