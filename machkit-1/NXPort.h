/*
 * Copyright 1991 NeXT Computer, Inc.
 */
 
#import <objc/List.h>
#import <appkit/NXInvalidationNotifier.h>
#import <appkit/exceptions.h>
#import <appkit/transport.h>
#import <mach/port.h>

@interface NXPort : NXInvalidationNotifier <NXTransport> {
@public
    port_t  machPort;
@private
    BOOL    deallocate;
    int     _enableCount;
    void    *_enableProc;
	void	*_enablePriority;
    void *  _expansion;
}

+ new;                  // allocate a new port; dealloc on free
+ newFromMachPort: (port_t) p;  // wrap an existing port; don't dealloc on free
+ newFromMachPort: (port_t) p dealloc: (BOOL) flag;

+ worryAboutPortInvalidation;   // fork a thread to listen for port deaths

- (port_t) machPort;    // get port name

- (unsigned) hash;
- free;
@end
