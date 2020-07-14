/*
 * Copyright 1991 NeXT Computer, Inc.
 */

#import <objc/Protocol.h>
#import <objc/Object.h>
#import <appkit/exceptions.h>

@interface NXProtocolChecker : Object {
	id	target;
	Protocol *protocol;
}

- initWithObject:anObject forProtocol:(Protocol *)proto;

// reimplemented Object methods...
- forward:(SEL)sel :(void *)args;
- (struct objc_method_description *) descriptionForMethod:(SEL)sel;
//- (BOOL) conformsTo: (Protocol *)aProtocolObject;
- free;

@end

