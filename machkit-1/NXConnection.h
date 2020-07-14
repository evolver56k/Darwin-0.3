/*  NXConnection.h
    Copyright 1992, NeXT, Inc.
*/

#import <stdlib.h>
#import <stdarg.h>
#import <objc/hashtable.h>
#import <objc/Protocol.h>
#import <objc/List.h>
#import <appkit/transport.h>
#import <appkit/NXInvalidationNotifier.h>

@class NXProxy;
@class NXPort;

/*****************  NXConnection    **************************/

/* A connection object is a bookkeeper for objects vended or received over a particular portal (channel) resource.  It may be shared by multiple threads. */

@interface NXConnection: NXInvalidationNotifier <NXSenderIsInvalid> {
    id  delegate;
@private
    id  inPortal;   // portal that we decode from
    void *inMachPort;
    id  outPortal;  // portal that we encode to
    void *outMachPort;
    id                  rootObject; // the "default" object for this connection
    unsigned            msgcount;   // how many objc messages sent
    void                *localProxies;
    void                *remoteProxies;
    int                 inTimeout;
    int                 outTimeout;
    NXZone              *zone;
    id                  bufferClass;
}

+ (NXProxy *) connectToName:(const char *)n;
+ (NXProxy *) connectToName:(const char *)n onHost:(const char *)h;
+ (NXProxy *) connectToPort:(NXPort *)p;    // allocate a inPort (reply port)
+ (NXProxy *) connectToPort:(NXPort *)aPort withInPort:(NXPort *)inPort;

+ (NXProxy *) connectToName:(const char *)n fromZone:(NXZone *) z;
+ (NXProxy *) connectToName:(const char *)n onHost:(const char *)h fromZone:(NXZone *) z;
+ (NXProxy *) connectToPort:(NXPort *)p fromZone:(NXZone *) z;
+ (NXProxy *) connectToPort:(NXPort *)aPort withInPort:(NXPort *)inPort fromZone:(NXZone *) z;

+ registerRoot:anObject;    // returns an NXConnection
+ registerRoot:anObject withName:(const char *)n;   // returns an NXConnection

+ registerRoot:anObject fromZone:(NXZone *) z;
+ registerRoot:anObject withName:(const char *)n fromZone:(NXZone *) z;

+ removeObject:anObject;
+ unregisterForInvalidationNotification:anObject;

+ (int)messagesReceived;

+ connections:(List *) l;   // append existing connections to list

+ setDefaultTimeout:(int) t;
+ (int) defaultTimeout;

+ setDefaultZone: (NXZone *) zone;
+ (NXZone *) defaultZone;

// a delegate, if present, will be asked:
//  - connection: (NXConnection *)oldConn didConnect:(NXConnection *)newConn;
- setDelegate:anObject;
- delegate;

- setRoot:anObject;	// useful for connections without roots

- setInTimeout:(int) t;
- setOutTimeout:(int) t;
- (int) inTimeout;
- (int) outTimeout;

- (NXPort *) inPort;
- (NXPort *) outPort;

- rootObject;

- newRemote:(unsigned)name withProtocol:(Protocol *)p;

- (List *)remoteObjects;    // the remote objects imported
- (List *)localObjects;     // the local objects exported

- getLocal:anId;        // return the Proxy for a local id

- run;                  // blocks
- runWithTimeout:(int) t;
- runInNewThread;       // spawns thread, thread will "run"; nonblocking

- free;
@end
 

// By special request, a convenient interface for AppKit programmers
@interface NXConnection (NXAppKitServer) 
- runFromAppKit;    // nonblocking
@end

// definitions

// timeout is specified in milliseconds
#define NX_CONNECTION_DEFAULT_TIMEOUT   15000
