/*  NXPortPortal.h
    Sending Objective-C messages in Mach msgs
    Copyright 1989,1991 NeXT Computer, Inc.
*/

#import <objc/Object.h>
#import <mach/message.h>
#import <appkit/NXPort.h>
#import <objc/Protocol.h>
#import <appkit/reference.h>
#import <appkit/NXInvalidationNotifier.h>
#import <appkit/transport.h>
#import <appkit/NXConnection.h>

typedef void (*receive_enable_proc_t)(port_t port, BOOL shouldEnable);
 
@protocol NXPortal <NXEncoding, NXDecoding>
// obtain the next message
- startDecodingWithTimeout:(int) t;
- finishDecoding;

// begin preparation of a reply or a response
- startEncodingWithConnection:conn andSequenceNumber:(unsigned int) seq;
- setRequestMsg:(BOOL)flag;         // default is otherwise a reply
- finishEncoding;

- finishEncodingAndStartDecoding:(BOOL) doRPC;	// rpc

- (BOOL) isRequestMsg;
- (unsigned int) sequenceNumber;
- (unsigned) goodMsgSize;
- setMsg:(void *)buffer;

- connection;

- setPort:(NXPort *)aPort;
- (NXPort *)port;

// The following method allow an external agent to dispatch MACH messages.  
+ (void) dispatchMsg:(msg_header_t *)msg;

// other things we do to portals...
- free;
- move;		// copy msg to new buffer & reinitialize
- (NXZone *) zone;
+ newPort;
+ newFromMachPort:(port_t)machport dealloc:(BOOL) flag;
+ lookUpPortWithName:(const char *)n onHost:(const char *)h;
+ checkInPort:(NXPort *)newport withName:(const char *)n;

// something that should go into the Encoding protocol...
- encodeVM:(const void *)bytes count:(int)count dealloc:(BOOL)d;

@end

// NXPort really does our work for us
// The following methods allow an external agent to acquire MACH messages.
// ltoggle is a routine supplied by the
// agent that will be called to turn on and off the external acquisition
// of msgs.  This is necessary because DO may need to receive directly
// on the port in question. (en/dis)ableExternal is the routine called by the
// DO facility which, in turn, calls the ltoggle routine.
@interface NXPort (NXPortPortal)
- setEnableProc:(receive_enable_proc_t)lToggle data:(void *)priority;
- (receive_enable_proc_t)enableProc;
// these methods are called  to toggle any
// "external" message reception (such as the AppKit (DPS))
// these routines count their invocations
- enableExternal;
- disableExternal;
@end

/* this particular class defines buffering for an objective-C message to be sent or received across a mach port */


typedef struct _remote_message_t {
    msg_header_t    header;
    msg_type_t      sequenceType;
    int             sequence;
    /* following is chars type and chars, int types and ints, etc ... */
} remote_message_t;


typedef struct _oold {  // a private data structure
    void        *data;
    unsigned    len;
    BOOL	dflag;	// deallocate on xfer
    
} oold;                 /* out-of-line data */

@interface NXPortPortal : Object <NXPortal> {
    msg_header_t    *msg;
    BOOL    freeMsgFlag;
    BOOL    write;      /* writing vs reading */    //REMOVE AFTER DEBUG!
    id      connection;     // table of ids exported/imported
    NXPort  *port;
    port_t  machPort;
    char    *chars; int     nchars;     int     maxchars;
    int     *ints;  int     nints;      int     maxints;
    port_t  *ports; int     nports;     int     maxports;
    oold    *oolds; int     noolds;     int     maxoolds;
	unsigned	sequence;
	unsigned	msgid;
}


- initWithMachPort: (port_t) machport;
- initWithPort: (NXPort *) port;

- (NXPort *) port;

@end




