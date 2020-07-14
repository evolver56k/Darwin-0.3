/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * ioThread.m - I/O thread example. 
 *
 * This example consists of a simple driver class with one I/O thread. 
 * Exported methods are invoked interactively via stdin. These methods 
 * pass commands to the I/O thread, which performs various trivial tasks
 * and then notifies the exported methods of command complete.
 */

#include "ioThread.h"
#include "ioThreadPrivate.h"
 
static void MyDeviceThread(id deviceId);

/*
 * A regular old Unix-y program to exercise the MyDevice class.
 */
int main(int argc, char **argv)
{
	id devId;
	char in_string[40];
	int aNumber;
	
	/*
	 * Create and initialize one instance of MyDevice. Forget devPort
	 * for this example.
	 */
	devId = [MyDevice alloc];
	[devId myDeviceInit];
	
	/*
	 * Main I/O loop. We'll invoke the exported methods on myDevice 
	 * per user input.
	 */
	 
	while(1) {
		printf("Main Loop:\n");
		printf("  p  print number\n");
		printf("  g  get number\n");
		printf("  q  quit program\n\n");
		printf("Enter Selection: ");
		gets(in_string);
		switch(in_string[0]) {
		   case 'p':
		   	/*
			 * Get a number, pass it throught to MyDevice's
			 * I/O thread for display.
			 */
		   	printf("\nEnter number to pass to I/O thread: ");
			gets(in_string);
			aNumber = atoi(in_string);
			[devId printNumber:aNumber];
			break;
			
		   case 'g':
		   	/*
			 * Get a number via MyDevice's I/O thread.
			 */
			aNumber = [devId getNumber];
			printf("getNumber returned %d\n", aNumber);
			break;
						
		   case 'q':
		   	/*
			 * Shut down.
			 */
		   	[devId free];
			exit(0);
			
		   default:
		   	printf("**Illegal Selection\n");
			break;
		}
	}
	
	/* NOT REACHED */
}

/*
 * Implementation of simple device.
 */
@implementation MyDevice

/*
 * Initialization.
 */
- myDeviceInit
{
	/*
	 * Initialize instance variables.
	 */
	ioQueueLock = [NXConditionLock new];
	queue_init(&ioQueue);
	
	/*
	 * Start up the I/O thread.
	 */
	ioThread = IOForkThread((IOThreadFunc)MyDeviceThread, self);

	/*
	 * Register with IODevice-level code.
	 */
	[self setName:"myDevice"];
	[super init];
	[self registerDevice];
	return self;
}

/*
 * Run-time methods.
 */
 
/*
 * Have I/O thread get a number from user.
 */
- (int)getNumber
{
	cmdBuf_t *cmdBuf;
	int result;
	
	/*
	 * Set up a cmdBuf.
	 */
	cmdBuf = [self cmdBufAlloc];
	cmdBuf->cmd = IOC_GETNUM;
	
	/*
	 * Pass the cmdBuf to the I/O thread. On return, the desired
	 * number is in cmdBuf.aNumber.
	 */
	[self cmdBufExec:cmdBuf];
	result = cmdBuf->aNumber;
	[self cmdBufFree:cmdBuf];
	return result;
}

/*
 * Have I/O thread print a number.
 */
- (void)printNumber : (int)aNumber
{
	cmdBuf_t *cmdBuf;
	
	/*
	 * Set up a cmdBuf.
	 */
	cmdBuf = [self cmdBufAlloc];
	cmdBuf->cmd = IOC_PRINTNUM;
	cmdBuf->aNumber = aNumber;
	
	/*
	 * Pass the cmdBuf to the I/O thread.
	 */
	[self cmdBufExec:cmdBuf];
	[self cmdBufFree:cmdBuf];
	return;
}

/*
 * Have I/O thread shut down cleanly.
 */
- free
{
	cmdBuf_t *cmdBuf;
	
	/*
	 * Set up a cmdBuf.
	 */
	cmdBuf = [self cmdBufAlloc];
	cmdBuf->cmd = IOC_QUIT;
	
	/*
	 * Pass the cmdBuf to the I/O thread.
	 */
	[self cmdBufExec:cmdBuf];
	[self cmdBufFree:cmdBuf];

	/*
	 * Free instance variables.
	 */	
	[ioQueueLock free];
	
	/*
	 * Have superclass take care of the rest.
	 */
	return [super free];
}


/*
 * Methods invoked by ioThread.
 */
 
/*
 * Wait for a work to appear in ioQueue.
 */
- (cmdBuf_t *)waitForCmdBuf
{
	cmdBuf_t *cmdBuf;
	
 	[ioQueueLock lockWhen:QUEUE_FULL];

	/*
	 * At this point, we still hold ioQueueLock'. Remove 
	 * the first element in ioQueue.
	 */
	cmdBuf = (cmdBuf_t *)queue_first(&ioQueue);
	queue_remove(&ioQueue,
		cmdBuf,
		cmdBuf_t *,
		link);
		
	/*
	 * Release ioQueueLock, updating its condition variable as 
	 * appropriate, and return the new cmdBuf to the ioThread.
	 */
	if(queue_empty(&ioQueue))
		[ioQueueLock unlockWith:QUEUE_EMPTY];
	else
		[ioQueueLock unlockWith:QUEUE_FULL];
	return cmdBuf;
}

/*
 * Notify client of I/O complete.
 */
- (void)cmdBufComplete : (cmdBuf_t *)cmdBuf
{
	[cmdBuf->cmdLock lock];
	[cmdBuf->cmdLock unlockWith:CMD_COMPLETE];
}

@end



/*
 * Private methods.
 */
 
@implementation MyDevice ( MyDevicePrivate )

/*
 * Create and initialize a new cmdBuf_t.
 */
- (cmdBuf_t *)cmdBufAlloc
{
	cmdBuf_t *cmdBuf;
	
	cmdBuf = IOMalloc(sizeof(cmdBuf_t));
	cmdBuf->cmdLock = [NXConditionLock new];
	[cmdBuf->cmdLock lock];
	[cmdBuf->cmdLock unlockWith:CMD_BUSY];
	return cmdBuf;
}

/*
 * Free a cmdBuf_t.
 */
- (void)cmdBufFree : (cmdBuf_t *)cmdBuf
{
	[cmdBuf->cmdLock free];
	IOFree(cmdBuf, sizeof(cmdBuf_t));
}

/*
 * Pass a cmdBuf to the I/O thread and wait for completion.
 */
- (void)cmdBufExec : (cmdBuf_t *)cmdBuf
{
	/*
	 * Add the cmdBuf to the ioQueue and let the ioThread know it has 
	 * work to do.
	 */
	[ioQueueLock lock];
	queue_enter(&ioQueue,
		cmdBuf,
		cmdBuf_t *,
		link);
	[ioQueueLock unlockWith:QUEUE_FULL];
	
	/*
	 * Wait for I/O complete.
	 */
	[cmdBuf->cmdLock lockWhen:CMD_COMPLETE];
	[cmdBuf->cmdLock unlock];
	
	return;
}

@end


/*
 * The I/O thread. This thread sits around waiting for work to do on
 * the ioQueue, then behaves accordingly.
 */
static void MyDeviceThread(id deviceId)
{
	cmdBuf_t *cmdBuf;
	char in_string[40];
	
	while(1) {
		
		/*
		 * Wait for something to do.
		 */
		cmdBuf = [deviceId waitForCmdBuf];
		
		/*
		 * OK, What's up?
		 */
		switch(cmdBuf->cmd) {
		    case IOC_GETNUM:
		    	/*
			 * Get a number from user, pass back to client.
			 */
		    	printf("MyDeviceThread: Enter number: ");
			gets(in_string);
			cmdBuf->aNumber = atoi(in_string);
			break;
			
		    case IOC_PRINTNUM:
		    	/*
			 * Just display client's number.
			 */
		    	printf("MyDeviceThread: cmdBuf->aNumber = %d\n",
				cmdBuf->aNumber);
			break;
			
		    case IOC_QUIT:
		    	/*
			 * Time to die. First notify client that we got 
			 * the command, then terminate.
			 */
			[deviceId cmdBufComplete:cmdBuf];
			IOExitThread();
			 
		}
		
		/*
		 * Notify client of I/O complete.
		 */
		[deviceId cmdBufComplete:cmdBuf];
	}
	
	/* NOT REACHED */
}
