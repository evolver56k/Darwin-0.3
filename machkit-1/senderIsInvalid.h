/*
 * NXSenderIsInvalid Protocols
 * Copyright 1991, NeXT Computer, Inc.
 */

#import <objc/Object.h>

// This protocol is implemented by objects that wish to be
// informed of the invalidation of other objects

@protocol NXSenderIsInvalid
- senderIsInvalid:sender;
@end

