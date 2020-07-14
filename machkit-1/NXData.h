/*
 * Copyright 1991 NeXT Computer, Inc.
 */
 
#import <objc/Object.h>
#import <appkit/transport.h>


@interface NXData : Object <NXTransport>  {
@private
    void            *data;
    unsigned int    size;
    BOOL            dealloc;
}

// allocate some memory; will be deallocated on free
- initWithSize: (unsigned int) size;

// create a wrapper for existing allocated memory;
// We will deallocate this on free if asked
- initWithData:(void *)data size:(unsigned) size dealloc:(BOOL) flag;

- (void *) data;
- (unsigned) size;

// object methods reimplemented
- copyFromZone:(NXZone *)zone;
- free;
@end


