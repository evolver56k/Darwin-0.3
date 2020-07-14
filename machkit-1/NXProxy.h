/*  NXProxy.h
    Copyright 1992, NeXT, Inc.
*/

#import <stdlib.h>
#import <stdarg.h>
#import <objc/HashTable.h>
#import <objc/Protocol.h>
#import <appkit/NXConnection.h>
#import <appkit/transport.h>
#import <appkit/reference.h>




/*****************  Distributed Objects     **************************/

typedef enum  {
    NX_REMOTE_EXCEPTION_BASE = 11000,
    NX_couldntSendException = 11001,
    NX_couldntReceiveException = 11002,
    NX_couldntDecodeArgumentsException = 11003,
    NX_unknownMethodException = 11004,
    NX_objectInaccessibleException = 11005,
    NX_objectNotAvailableException = 11007,
    NX_remoteInternalException = 11008,
    NX_multithreadedRecursionDeadlockException = 11009,
    NX_destinationInvalid = 11010,
    NX_originatorInvalid = 11011,
    NX_sendTimedOut = 11012,
    NX_receiveTimedOut = 11013,
    NX_REMOTE_LAST_EXCEPTION = 11999
} NXRemoteException;


/* we keep a NXProxy for each Object, whether remote or local,
 * that has been communicated over the wire.
 */

@interface NXProxy <NXTransport,NXReference> {
@private
    Class           isa;
    unsigned        name;           /* object name */
    unsigned        wire;           /* is this a stub for a local object? */
    NXConnection    *conn;          /* what conn are we registered on? */
    Protocol        *proto;         /* what protocol do we serve? */
    unsigned        refcount;       /* how many references have been made? */
    void            *knownSelectors;    /* cache */
}


- setProtocolForProxy:(Protocol *)proto;

- (BOOL) isProxy;   // always returns YES

- connectionForProxy;
- (unsigned) nameForProxy;

- freeProxy;
@end

@interface Object (IsProxy)
- (BOOL) isProxy;   // always returns NO
@end


