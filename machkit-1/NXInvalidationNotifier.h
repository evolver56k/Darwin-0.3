/*
 * NXInvalidationNotifier
 * allow objects to register themselves for a senderIsInvalid message
 * Notification is triggered by sending "invalidate" to this object
 * Copyright 1991, NeXT Computer, Inc.
 */

#import <objc/List.h>
#import <appkit/reference.h>
#import <appkit/senderIsInvalid.h>

@interface NXInvalidationNotifier : Object <NXReference> {
@protected
	unsigned	refcount;
	BOOL		isValid;
	id		listGate;
	List		*funeralList;
}

- init;
- deallocate;

- invalidate;
- (BOOL) isValid;

// The target of this message will arrange to send "senderIsInvalid"
// when, indeed, the target is about to die (is invalidated).

- registerForInvalidationNotification:(id <NXSenderIsInvalid>)anObject;

// cancel the request for notification
- unregisterForInvalidationNotification:(id <NXSenderIsInvalid>)anObject;

@end
