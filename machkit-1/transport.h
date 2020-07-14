/* transport.h
 * encoding/decoding protocols for DistributedObjects
 * Copyright 1992, NeXT Computer, Inc.
 */

#import <objc/Object.h>
#import <mach/port.h>

/* these are the general encoding/decoding protocols implemented by all objective-C message buffering/delivery classes */

/* The general scheme is that Proxys will encode a method and its arguments onto a portal, and/or decode requests or replies from a portal.  The encoding/decoding protocol allows for potentially different implementations of portals, such as an in memory queue or a TCP channel or a System V STREAMS version... or whatever your favorite connection paradigm is.  Note that the treatment of MACH memory and MACH ports would need to be emulated very closely in any underlying implementation. */

@protocol NXEncoding
// encode an objc (parameter) type
- encodeData:(void *)data ofType:(const char *)type;

// encoding methods for transcribing custom objects
- encodeBytes:(const void *)bytes count:(int)count;
- encodeVM:(const void *)bytes count:(int)count;
- encodeMachPort:(port_t)port;
- encodeObject:anObject;    // send a ref to the object across
- encodeObjectBycopy:anObject;  // copy the object across

@end

@protocol NXDecoding
// decode an objc (parameter) type
- decodeData:(void *)d ofType:(const char *)t;

// decoding methods for transcribing custom objects
- decodeBytes:(void *)bytes count:(int)count;
- decodeVM:(void **)bytes count:(int *)count;
- decodeMachPort:(port_t *)pp;
- (id) decodeObject;                    // returns decoded object
@end

/* Objects that encode themselves "on the wire" need to implement the following methods */

@class NXConnection;

@protocol NXTransport
// This method is called for every object before encoding.
// It should return self if the object always transcribes itself.
// If the object wants to conditionally transcribe itself depending
// on whether the bycopy keyword is present in the methods protocol
// description, it should return self if isBycopy is true, and
// return [super encodeRemotelyFor...] if false.
// If the object wants another object to be sent, it should return that
// object.
// Setting *flagp to true will cause the returned object to be freed after encoding.
- encodeRemotelyFor:(NXConnection *)connection freeAfterEncoding:(BOOL *)flagp isBycopy:(BOOL)isBycopy;

// The object should encode itself on the portal
- encodeUsing:(id <NXEncoding>)portal;
// The object should initialize itself from the portal
- decodeUsing:(id <NXDecoding>)portal;
@end


/* Object provides an implementation that creates NXProxies on the other side */

@interface Object (Object_MakeRemote)
- encodeRemotelyFor:(NXConnection *)connection freeAfterEncoding:(BOOL *)flagp isBycopy:(BOOL)isBycopy;
@end



