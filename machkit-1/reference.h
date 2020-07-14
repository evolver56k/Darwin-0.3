/*
 * Reference.h
 * Copyright 1991, NeXT Computer, Inc.
 */

#import <appkit/exceptions.h>

@protocol NXReference

// how many references are there outstanding
- (unsigned) references;

// add a reference; return the object.
- addReference;

// remove a reference; return self if references remain
- free;
@end

