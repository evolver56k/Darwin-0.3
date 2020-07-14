/*
 * Copyright 1991 NeXT Computer, Inc.
 */
 
#import <appkit/NXPort.h>


@interface NXNetNameServer : Object
+ checkInPort:(NXPort *)nxport withName:(const char *)aName;
+ checkOutPortWithName:(const char *)name;

+ (NXPort *) lookUpPortWithName:(const char *)name;
+ (NXPort *) lookUpPortWithName:(const char *)name onHost:(const char *)hostname;
@end
