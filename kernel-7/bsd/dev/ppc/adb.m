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
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * Copyright 1996 1995 by Apple Computer, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

/*
 * ADB Driver - used internally by ADB device drivers
 *
 * Currently this is heavily tied to the design  of
 * the CUDA hardware and MACH driver
 */


#include <mach_kdb.h>
#include <mach/mach_types.h>
#include <machdep/ppc/powermac.h>  //for HasPMU() definition
#include <sys/tty.h>
#include <sys/conf.h>
#include "busses.h"
#include "adb.h"
#include "adb_io.h"
#include "drvPMU/pmu.h"

void waitForCallback(void);
void ADBcallback(id, UInt32, UInt32, UInt8 *);
void ADBReadCallback(id, UInt32, UInt32, UInt8 *);
void ADBReadCallback2(id, UInt32, UInt32, UInt8 *);
void inputHandler(id, UInt32, UInt32, UInt32, UInt8*);

id		PMUdriver = NULL;
UInt8		read_buffer[8];
int		read_length;
boolean_t       adb_initted = FALSE;
int             adb_count = 0;

adb_request_t   *adb_queue_root = NULL, *adb_queue_end = NULL;

#define ADB_POOL_COUNT  16

adb_device_t    adb_devices[ADB_DEVICE_COUNT];
adb_request_t   adb_async_requests[ADB_POOL_COUNT], *adb_async_pool;
boolean_t adb_hardware = FALSE;


void adbattach( struct bus_device *device);
int adbprobe(caddr_t addr, void *ui);

struct bus_device   *adb_info[NADB];

void
InitializeADB(void)
{

    if (adb_hardware == FALSE) {
   	[PMUdriver registerForADBAutopoll:inputHandler:(id)NULL];
	adb_hardware = TRUE;
	adbprobe(0, 0);
   }
}


static int
adb_is_dev_present(unsigned short addr)
{
    struct adb_device   *devp = &adb_devices[addr];
    unsigned short value;
    int retval;

//kprintf("adb_is_dev_present: addr = %08x, devp = %08x\n", addr, devp);

    retval = adb_readreg(addr, 3, &value);

//kprintf("adb_is_dev_present: retval = %08x, ADB_RET_TIMEOUT = %08x\n",
//	retval, ADB_RET_TIMEOUT);

#if notdef_next // DEBUG
    if (retval != ADB_RET_TIMEOUT)
        printf("adb_is_dev_present(%2d) returned %d\n", addr, retval);
#endif /* DEBUG */
    if (retval != ADB_RET_TIMEOUT) {

//kprintf("adb_is_dev_present: value = %08x\n", value);

        devp->a_dev_addr = (value >> 8) & 0xf;
        devp->a_dev_orighandler = devp->a_dev_handler = (value & 0xff);
        devp->a_flags |= ADB_FLAGS_PRESENT;
        return TRUE;
    }

    return FALSE;
}

int
adb_set_handler(struct adb_device *devp, int handler)
{
    unsigned long   retval;
    unsigned short value;

    retval = adb_readreg(devp->a_addr, 3, &value);

    if (retval != ADB_RET_OK) 
	return( retval);

    value = (value & 0xF000) | handler;

    retval = adb_writereg(devp->a_addr, 3, value);

    if (retval != ADB_RET_OK)
	return( retval);

    retval = adb_readreg(devp->a_addr, 3, &value);

    if (retval != ADB_RET_OK)
	return( retval);

    if( (value & 0xFF) != handler)
	retval = ADB_RET_UNEXPECTED_RESULT;

    /* Update to new handler/id */
    devp->a_dev_handler = (value & 0xff);


    return( retval);
}

static void
adb_move_dev(unsigned short from, unsigned short to)
{
    int addr;
    adb_writereg(from, 3, ((to << 8) | 0xfe));

    addr = adb_devices[to].a_addr;
    adb_devices[to] = adb_devices[from];
    adb_devices[to].a_addr = addr;
    adb_devices[from].a_flags = 0;
    
    return;
}

static boolean_t 
adb_find_unresolved_dev(unsigned short *devnum)
{
    int i;
    struct adb_device   *devp;

    devp = &adb_devices[1];

    for (i = 1; i < ADB_DEVICE_COUNT; i++, devp++) {
        if (devp->a_flags & ADB_FLAGS_UNRESOLVED) {
            *devnum = i;
            return  TRUE;
        }
    }

    return FALSE;
}

static int
adb_find_freedev(void)
{
    struct adb_device   *devp;
    int i;

    for (i = ADB_DEVICE_COUNT-1; i >= 1; i--) {
        devp = &adb_devices[i];

        if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0) 
            return i;
    }

    panic("ADB: Cannot find a free ADB slot for reassignment!");
    return -1;
}

int
adbprobe(caddr_t addr, void *ui)
{
    unsigned short devnum, freenum, devlist;
    struct adb_device   *devp;
    int i;
    spl_t   s;

//kprintf("adbprobe: Begining\n");

    if (adb_initted)
        return 1;

//kprintf("adbprobe: Not initted\n");

    for (i = 0; i < ADB_POOL_COUNT; i++) {
        adb_async_requests[i].a_next = adb_async_pool;
        adb_async_pool = &adb_async_requests[i];
    }

    s = spltty();

    /* Kill the auto poll until a new dev id's have been setup */

	if ( [PMUdriver ADBPollDisable:(UInt32)current_thread():0:ADBcallback] == kPMUNoError ) {
		waitForCallback();
		}
    /*
     * Send a ADB bus reset - reply is sent after bus has reset,
     * so there is no need to wait for the reset to complete.
     */

	if ( [PMUdriver ADBReset:(UInt32)current_thread():0:ADBcallback] == kPMUNoError ) {
		waitForCallback();
		}
    /*
     * Okay, now attempt reassign the
     * bus 
     */

//kprintf("adbprobe: probing\n");

    /* Skip 0 -- its special! */
    for (i = 1; i < ADB_DEVICE_COUNT; i++) {
        adb_devices[i].a_addr = i;

//kprintf("adbprobe: probing %x\n", i);

        if (adb_is_dev_present(i)) {

//kprintf("adbprobe: adb_is_dev_present\n");

            adb_devices[i].a_dev_type = i;
            adb_devices[i].a_flags |= ADB_FLAGS_UNRESOLVED;
        }
    }

    /* Now attempt to reassign the addresses */
    while (adb_find_unresolved_dev(&devnum)) {
        freenum = adb_find_freedev();
        adb_move_dev(devnum, freenum);

        if (!adb_is_dev_present(freenum)) {
            /* It didn't move.. damn! */
            adb_devices[devnum].a_flags &= ~ADB_FLAGS_UNRESOLVED;
            printf("WARNING : ADB DEVICE %d having problems "
                   "probing!\n", devnum);
            continue;
        }

        if (!adb_is_dev_present(devnum)) {
            /* no more at this address, good !*/
            /* Move it back.. */

            adb_move_dev(freenum, devnum);

            /* Check the device to talk again.. */
            (void) adb_is_dev_present(devnum);
            adb_devices[devnum].a_flags &= ~ADB_FLAGS_UNRESOLVED;
        } else {
            /* Found another device at the address, leave
             * the first device moved to one side and set up
             * newly found device for probing
             */
            /* device_present already called on freenum above */
            adb_devices[freenum].a_flags &= ~ADB_FLAGS_UNRESOLVED;

            /* device_present already called on devnum above */
            adb_devices[devnum].a_dev_type = devnum;
            adb_devices[devnum].a_flags |= ADB_FLAGS_UNRESOLVED;
#if 0
            printf("Found hidden device at %d, previous device "
                   "moved to %d\n", devnum, freenum);
#endif /* DEBUG */
        }
    }

    /*
     * Now build up a dev list bitmap and total device count
     */

    devp = adb_devices;
    devlist = 0;

    for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
        if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
            continue;

        devlist |= (1<<i);
        adb_count++;
    }


    // ADBSetPollList calls the callback before it returns when
    // autopolling is off.
	if (HasPMU())
	{
    	[PMUdriver ADBSetPollList:devlist:0:0:NULL];
		//PG&E doesn't perform this hardware operation immediately
		//   so it's OK not to wait for callback
	}
	else
	if ( [PMUdriver ADBSetPollList:devlist:(UInt32)current_thread():0:ADBcallback] == kPMUNoError)
	{
     	 waitForCallback(); 
	}


    if ( [PMUdriver ADBSetPollRate:11:(UInt32)current_thread():0:ADBcallback] == kPMUNoError ) {
      waitForCallback();
    }

    if ( [PMUdriver ADBPollEnable:(UInt32)current_thread():0:ADBcallback] == kPMUNoError ) {
      waitForCallback();
    }

    if ( [PMUdriver ADBSetFileServerMode:(UInt32)current_thread():0:ADBcallback] == kPMUNoError ) {
      waitForCallback();
    }


    adb_initted = TRUE;
    splx(s);

    return 1;
}

void
adbattach( struct bus_device *device)
{
    int i;
    adb_device_t    *devp = &adb_devices[0];

    printf("\nadb0: %d devices on bus.\n", adb_count);
    for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
        if (devp->a_flags & ADB_FLAGS_PRESENT) {
            printf("    %d: ", i);
            switch (devp->a_dev_type) {
            case    ADB_DEV_PROTECT:
                printf("security device");
                break;
            case    ADB_DEV_KEYBOARD:
                printf("keyboard");
                break;
            case    ADB_DEV_MOUSE:
                printf("mouse");
                break;
            case    ADB_DEV_TABLET:
                printf("tablet");
                break;
            case    ADB_DEV_MODEM:
                printf("modem");
                break;
            case    ADB_DEV_APPL:
                printf("application device");
                break;
            default:
                printf("unknown device id=%d", i);
            }
            printf("\n");
        }
    }
}

/*
 * adbopen
 *
 */

io_return_t
adbopen(dev_t dev, dev_mode_t flag, io_req_t ior)
{
    return  D_SUCCESS;
}

void
adbclose(dev_t dev)
{
    return;
}

io_return_t
adbread(dev_t dev, io_req_t ior)
{
    return  D_INVALID_OPERATION;
}

io_return_t
adbwrite(dev_t dev, io_req_t ior)
{
    return  D_INVALID_OPERATION;
}

boolean_t
adbportdeath(dev_t dev, ipc_port_t port)
{
    return  FALSE;
}

#if 0
io_return_t
adbgetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
    mach_msg_type_number_t *status_count)
{
/* DS1
    struct  adb_info *info = (struct adb_info *) data;
    struct adb_device *devp;
    int i;

    switch (flavor) {
    case    ADB_GET_INFO:
        devp = adb_devices; 
            for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
            if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
                continue;

            info->a_addr = devp->a_addr;
            info->a_type = devp->a_dev_type;
            info->a_handler = devp->a_dev_handler;
            info->a_orighandler = devp->a_dev_orighandler;
            info++;
        }
        *status_count = (sizeof(struct adb_info)*adb_count)/sizeof(int);
        break;

    case    ADB_GET_COUNT:
        *((unsigned int *) data) = adb_count;
        *status_count = 1;
        break;

    case    ADB_READ_DATA: {
        struct adb_regdata *reg = (struct adb_regdata *) data;

        reg->a_count = adb_reg_data.a_reply.a_bcount;
        bcopy((char *) adb_reg_data.a_reply.a_buffer,
            (char *) reg->a_buffer, adb_reg_data.a_reply.a_bcount);
        *status_count = (sizeof(struct adb_regdata))/sizeof(int);
    }
        break;
            
    default:
        return  D_INVALID_OPERATION;
    }
DS1 */
    return  D_SUCCESS;
}

io_return_t
adbsetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
    mach_msg_type_number_t status_count)
{
/* DS1
    struct adb_info *info = (struct adb_info *) data;
    struct adb_device *devp;

    switch (flavor) {
    case    ADB_SET_HANDLER:
        if (info->a_addr < 1 || info->a_addr >= ADB_DEVICE_COUNT)
            return  D_NO_SUCH_DEVICE;

        devp = &adb_devices[info->a_addr];
        adb_set_handler(devp, info->a_handler);
        if (devp->a_dev_handler != info->a_handler)
            return  D_READ_ONLY;
        break;

    case    ADB_READ_REG: {
        struct adb_regdata *reg = (struct adb_regdata *) data;

        adb_init_request(&adb_reg_data);
        ADB_BUILD_CMD2(&adb_reg_data, ADB_PACKET_ADB,
            (ADB_ADBCMD_READ_ADB|(reg->a_addr<<4)|reg->a_reg));
        
        adb_send(&adb_reg_data, TRUE);

        if (adb_reg_data.a_result != ADB_RET_OK) 
            return D_IO_ERROR;

    }
        break;

    case    ADB_WRITE_REG: {
        struct adb_regdata *reg = (struct adb_regdata *) data;

        adb_init_request(&adb_reg_data);
        ADB_BUILD_CMD2_BUFFER(&adb_reg_data, ADB_PACKET_ADB,
            (ADB_ADBCMD_WRITE_ADB|(reg->a_addr<<4)|reg->a_reg),
            reg->a_count, &reg->a_buffer);

        adb_send(&adb_reg_data, TRUE);

        if (adb_reg_data.a_result != ADB_RET_OK) 
            return D_IO_ERROR;
    }
        break;

    default:
        return  D_INVALID_OPERATION;
    }
DS1 */
    return  D_SUCCESS;
}
#endif

/*
 * Register a routine which is to send events from all
 * devices matching a given type.
 */

void
adb_register_handler(int type,
        void (*handler)(int number, unsigned char *packet, int count, void * ssp))
{
    int dev;
    adb_device_t    *devp;

//kprintf("adb_register_handler: type = %08x, handler = %08x\n",
//	type, handler);

    for (dev = 0; dev < ADB_DEVICE_COUNT; dev++) {
        devp = &adb_devices[dev];

//kprintf("adb_register_handler: dev = %08x, devp = %08x, devp->a_dev_type = %08x\n", dev, devp, devp->a_dev_type);

        if (devp->a_dev_type != type)
            continue;

        devp->a_flags |= ADB_FLAGS_REGISTERED;
        devp->a_handler = handler;
    }
}

/* 
 * Register a routine to handle a dev at a specific address
 */

void
adb_register_dev(int devnum, void (*handler)(int number, unsigned char *packet, int count, void * ssp))
{
    adb_device_t    *devp;

    if (devnum < 0 || devnum > ADB_DEVICE_COUNT)
        panic("adb_register: addr is out of range.");

    devp = &adb_devices[devnum];

    devp->a_flags |= ADB_FLAGS_REGISTERED;
    devp->a_handler = handler;
}


static void    (*oldHandlers[ ADB_DEVICE_COUNT ])(int number, unsigned char *buffer, int bytes, void * ssp);

void
borrow_adb( void (*handler)(int number, unsigned char *packet, int count, void * ssp))
{
    int dev;
    adb_device_t    *devp;

    for (dev = 1; dev < ADB_DEVICE_COUNT; dev++) {
        devp = &adb_devices[dev];

	if( devp->a_flags & ADB_FLAGS_REGISTERED)
	    oldHandlers[ dev] = devp->a_handler;
	else
	    oldHandlers[ dev] = 0;

        devp->a_flags |= ADB_FLAGS_REGISTERED;
        devp->a_handler = handler;
    }
}

void
return_adb( void )
{

    int dev;
    adb_device_t    *devp;

    for (dev = 1; dev < ADB_DEVICE_COUNT; dev++) {

        devp = &adb_devices[dev];

	if( oldHandlers[ dev] ) {
	    devp->a_flags |= ADB_FLAGS_REGISTERED;
	    devp->a_handler = oldHandlers[ dev];
	} else
	    devp->a_flags &= ~ADB_FLAGS_REGISTERED;
    }
}


#if 0
/* DS2...
void
adb_done(adb_request_t *req)
{
    adb_packet_t    *pack = &req->a_reply;
DS2 */
    /* Note, adb_busy is not reset because the CUDA chip
     * needs time to settle back into an idle state.
     * adb_busy will be reset in adb_next_request() when
     * the CUDA driver is ready for the next one.
     */
/* DS2
    if (req->a_flags & ADB_IS_ASYNC) {
        req->a_next = adb_async_pool;
        adb_async_pool = req;
        return;
    }

    req->a_flags |= ADB_DONE;

    if (!adb_polling) {
        if (req->a_done)
            req->a_done(req);
        else    thread_wakeup((event_t) req);
    }
}

void
adb_unsolicited_done(adb_packet_t *pack, void * ssp)
{
    adb_device_t    *devp;
    int devnum;

DS2 */
    /* adb_next_request() will reset this when CUDA
     * asks for the next request.. until then the bus
     * has to become idle
     */
/* DS2
    adb_busy = TRUE;

    devnum = pack->a_header[2] >> 4;
    devp = &adb_devices[devnum];

    if (devp->a_flags & ADB_FLAGS_REGISTERED)
        devp->a_handler(devnum, pack->a_buffer, pack->a_bcount, ssp);
}
...DS2 */

#endif

int
adb_readreg(int number, int reg, unsigned short *value)
{
  if ([PMUdriver ADBRead:number :reg :(UInt32)current_thread() :0 :ADBReadCallback] != kPMUNoError) {
    return ADB_RET_UNEXPECTED_RESULT;
  }

  waitForCallback();

  if ( read_length ) {
    *value = (read_buffer[0] << 8) | read_buffer[1];
    return ADB_RET_OK;
  }
  else {
    return ADB_RET_TIMEOUT;
  }
}

int
adb_readreg2(int number, int reg, unsigned char *buffer, int *length)
{
  if ([PMUdriver ADBRead:number :reg :(UInt32)current_thread() :0 :ADBReadCallback2] != kPMUNoError) {
    return ADB_RET_UNEXPECTED_RESULT;
  }
  waitForCallback();

  *length = read_length;
  if ( read_length ) {
    int i;
    for ( i = 0; i < read_length; i++ ) {
      buffer[i] = read_buffer[i];
    }
    if ( reg == 3 ) {				// snoop on reads from reg 3
      adb_devices[number].a_dev_handler = read_buffer[1];
    }
    
    return ADB_RET_OK;
  }
  else {
    return ADB_RET_TIMEOUT;
  }
  
}


int
adb_flush(int number)
{
  if ([PMUdriver ADBFlush: number :(UInt32)current_thread() :0 :ADBReadCallback] != kPMUNoError) {
    return ADB_RET_UNEXPECTED_RESULT;
  }
  
  waitForCallback();
  
  return ADB_RET_OK;
}

int
adb_writereg(int number, int reg, unsigned short value)
{
  UInt8 buffer[2];
  
  buffer[0] = value >> 8;
  buffer[1] = value & 0xff;
  
  if ([PMUdriver ADBWrite:number :reg :2 :buffer :(UInt32)current_thread() :0 :ADBcallback] != kPMUNoError) {
    return ADB_RET_UNEXPECTED_RESULT;
  }

  waitForCallback();
  return ADB_RET_OK;
}

int
adb_writereg2(int number, int reg, unsigned char *buffer, int length)
{
  if ([PMUdriver ADBWrite:number :reg :length :buffer :(UInt32)current_thread() :0 :ADBcallback] != kPMUNoError) {
    return ADB_RET_UNEXPECTED_RESULT;
  }

  waitForCallback();
  return ADB_RET_OK;
}

void
adb_writereg_async(int number, int reg, unsigned short value)
{
  UInt8 buffer[2];
  
  buffer[0] = value >> 8;
  buffer[1] = value & 0xff;
  
  if ([PMUdriver ADBWrite:number :reg :2 :buffer :0 :0:NULL] != kPMUNoError) {
    return;
  }

  return;
}

#if 0
/* DS1
void
adb_send(adb_request_t *adb, boolean_t wait)
{
    spl_t   s;

    if (!adb_polling)
        s  = spltty();

    adb->a_reply.a_bsize = sizeof(adb->a_reply.a_buffer);

    if (wait)
        adb->a_done = NULL;

    adb->a_flags &= ~ADB_DONE;
    adb->a_next = NULL;

    if (adb_busy) {
        if (adb_queue_root == NULL)
            adb_queue_root = adb_queue_end = adb;
        else {
            adb_queue_end->a_next = adb;
            adb_queue_end = adb;
        }
        if (adb_polling) {
            adb_hardware->ao_poll();
            if ((adb->a_flags & ADB_DONE) == 0) 
                panic("adb_send: request did not complete?!?");
        }
    } else {
        adb_busy = TRUE;
        adb_hardware->ao_send(adb);
    }

    if (wait && !adb_polling) {
        simple_lock(&adb->a_lock);
	// simon: was thread_sleep_simple_lock
        thread_sleep((event_t) adb, simple_lock_addr(adb->a_lock), TRUE);
    }

    if (!adb_polling)
        splx(s);
}
adb_request_t *
adb_next_request()
{
    adb_request_t   *adb;

    adb = adb_queue_root;

    if (adb) {
        adb_queue_root = adb_queue_root->a_next;
        if (adb_queue_root == NULL)
            adb_queue_end = NULL;
        adb_busy = TRUE;
    } else  adb_busy = FALSE;


    return adb;
}
void
adb_poll(void)
{
    boolean_t   save_poll = adb_polling;

    adb_polling = TRUE;

    adb_hardware->ao_poll();

    adb_polling = save_poll;
}
DS1 */

#endif

// The PMU driver is finished sending the last command.
// Wake up the thread that is waiting.
void ADBcallback(id unused, UInt32 refnum, UInt32 length, UInt8* buffer)
{
  clear_wait(refnum,0,FALSE);
}


// The PMU driver has received data from the PMU that is the result of our
// previous read.  Copy the data and wake up the thread.
void ADBReadCallback(id unused, UInt32 refnum, UInt32 length, UInt8* buffer)
{
  if (buffer != NULL) {
    read_buffer[0] = *buffer;
    read_buffer[1] = *(buffer+1);
  }
  
  read_length = length;
  
  clear_wait(refnum,0,FALSE);
}


// The PMU driver has received data from the PMU that is the result of our
// previous read.  Copy the data and wake up the thread.
void ADBReadCallback2(id unused, UInt32 refnum, UInt32 length, UInt8* buffer)
{
  int i;
  
  for ( i = 0; i < length; i++ ) {
    read_buffer[i] = buffer[i];
  }
  read_length = length;
  
  clear_wait(refnum,0,FALSE);
}


// We have sent a command to the PMU driver with current_thread as the refnum.
// Sleep until it has sent the command to the PMU.
void waitForCallback(void)
{
  assert_wait(current_thread(), FALSE);
  thread_block();
}

// Called by the keyboard driver.  Interrupts are off.  Call PMU/Cuda
// to let it service any interrupts.
void CheckADBPoll(void)
{
  [PMUdriver poll_device];
}


// Autopoll data has arrived from the PMU.
void inputHandler(id unused, UInt32 refnum, UInt32 devnum, UInt32 length, UInt8* buffer)
{
  adb_device_t	*devp;

//kprintf("inputHandler: refnum: %08x, devnum = %08x, length = %08x, buffer = %08x\n", refnum, devnum, length, buffer);
//kprintf("buffer[0] = %02x, buffer[1] = %02x\n", buffer[0], buffer[1]);

  devp = &adb_devices[devnum];

//kprintf("devp = %08x, devp->a_handler = %08x\n", devp, devp->a_handler);
  
  if (devp->a_flags & ADB_FLAGS_REGISTERED) {
    devp->a_handler(devnum, buffer, length, 0);
  } 
}
