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
 * AudioStream.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver channel object.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import "AudioStream.h"
#import <driverkit/IOAudioPrivate.h>
#import "AudioChannel.h"
#import "audioLog.h"
#import "snd_reply.h"
#import <kernserv/prototypes.h>
#import <mach/mach_types.h>
#import "audioReply.h"
#import	<bsd/sys/time.h>
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>
#import <kernserv/kern_server_types.h>
#import <driverkit/kernelDriver.h>
#import <kernserv/i386/us_timer.h>	// for microtime
#import <bsd/machine/limits.h>		// ULONG_LONG_MAX

/* Factory variables */

static void *threadArg = 0;
static NXLock *threadArgLock = nil;

@implementation AudioStream

/*
 * Initialize.
 */
- initChannel:chan tag:(int)aTag user:(port_t *)user
        owner:(port_t)owner type:(u_int)aType
{
    kern_return_t krtn;

    xpr_audio_stream("AS: initChannel chan=0x%x tag=%d, user=0x%x owner=%d, "
		     "type=%d\n", chan, aTag, user, owner, aType);
    [super init];
    channel = chan;
    device = [chan audioDevice];
    tag = aTag;
    samplingRate = 22050;
    dataFormat = IOAudioDataFormatLinear16;
    channelCount = 2;
    queue_init(&regionQueue);
    regionQueueLock = [[NXLock alloc] init];
    krtn = port_allocate(task_self(), user);
    if (krtn) {
	xpr_audio_stream("AS: initChannel: stream port_allocate error %d\n",
		   krtn,2,3,4,5);
	log_error(("Audio: initChannel: stream port_allocate: %s\n",
		   mach_error_string(krtn)));
	[self free];
	return nil;
    }
    userPort = *user;
    kernUserPort = IOConvertPort(userPort, IO_CurrentTask, IO_Kernel);
    ownerPort = owner;
    type = aType;
    
    return self;
}

/*
 * Return instance variables.
 */
- (port_t)userPort
{
    return userPort;
}
- (port_t)ownerPort
{
    return ownerPort;
}
- channel
{
    return channel;
}
- (ASType)type
{
    return type;
}

/*
 * Init snd reply msg.
 */
- createSndReplyMsg
{
    if (!sndReplyMsg)
	sndReplyMsg = (msg_header_t *)IOMalloc(MSG_SIZE_MAX);
    sndReplyMsg->msg_simple = TRUE;
    sndReplyMsg->msg_size = sizeof(msg_header_t);
    sndReplyMsg->msg_type = MSG_TYPE_NORMAL;
    sndReplyMsg->msg_local_port = PORT_NULL;
    sndReplyMsg->msg_remote_port = PORT_NULL;
    sndReplyMsg->msg_id = 0;
    return self;
}

/*
 * Send user stream control reply message.
 */
- sendControlMessage:(ASReplyMsg)status mask:(ASMsgRequest)statusMask
{
    kern_return_t krtn;
    region_t *region;
    port_t ourReplyPort, ourUserPort;

    xpr_audio_stream("AS: sendControlMsg status=0x%x, mask=0x%x\n", status,
		     statusMask, 3,4,5);

    /*
     * If new interface, send one message and return.
     */
    if (type == AS_TypeUser) {
	if (userReplyMessages & statusMask) {
	    ourReplyPort = IOConvertPort(userReplyPort, IO_Kernel,
					 IO_KernelIOTask);
	    ourUserPort = IOConvertPort(kernUserPort, IO_Kernel,
					IO_KernelIOTask);
	    krtn = _NXAudioReplyStreamStatus(ourReplyPort, ourUserPort,
					     ourReplyPort, tag,
					     0, status);
	    if (krtn != KERN_SUCCESS) {
		xpr_audio_stream("AS: sendControlMsg: replyStreamStatus "
				"error %s (%d)\n",
				mach_error_string(krtn), krtn, 3,4,5);
	    }
	}
	return self;
    }

    /*
     * Old interface sends one per request, and may send none.
     */
    xpr_audio_stream("AS: sendControlMsg: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    if (queue_empty(&regionQueue)) {
	[regionQueueLock unlock];
	xpr_audio_stream("AS: sendControlMsg: unlocked regionQueue\n", 
			1,2,3,4,5);
	return self;
    }
    [self createSndReplyMsg];

    region = (region_t *)queue_first(&regionQueue);
    while (!queue_end(&regionQueue, (queue_entry_t)region)) {
	if (region->messages & statusMask) {
	    ourReplyPort = IOConvertPort(region->reply_port, IO_Kernel,
					 IO_KernelIOTask);
	    if (status == AS_ReplyPaused)
		audio_snd_reply_paused(sndReplyMsg, ourReplyPort, region->tag);
	    else if (status == AS_ReplyResumed)
		audio_snd_reply_resumed(sndReplyMsg, ourReplyPort, region->tag);
	    else if (status == AS_ReplyAborted)
		audio_snd_reply_aborted(sndReplyMsg, ourReplyPort, region->tag);
	    krtn = msg_send(sndReplyMsg, STREAM_SEND_OPTIONS,
			    STREAM_SEND_TIMEOUT);
	    if (krtn != KERN_SUCCESS) {
		xpr_audio_stream("AS: sendControlMsg msg_send error %s"
				 " (%d)\n", mach_error_string(krtn),
				 krtn, 3,4,5);
	    }
	}
	region = (region_t *)queue_next(&region->link);
    }

    [regionQueueLock unlock];
    xpr_audio_stream("AS: sendControlMsg: unlocked regionQueue\n", 1,2,3,4,5);
    return self;
}

/*
 * Send user stream status message.
 */
- sendStatusMessage:(ASReplyMsg)status forRegion:(region_t *)region
{
    kern_return_t krtn;
    port_t ourReplyPort, ourUserPort;

    xpr_audio_stream("AS: sendStatusMsg %d for region 0x%x\n", status,
		     region, 3,4,5);

    ourReplyPort = IOConvertPort(region->reply_port, IO_Kernel,
				 IO_KernelIOTask);
    ourUserPort = IOConvertPort(kernUserPort, IO_Kernel, IO_KernelIOTask);

    if (type == AS_TypeUser) {
	if (status == AS_ReplyAborted && region->flags.excluded)
	    status = AS_ReplyExcluded;
	krtn = _NXAudioReplyStreamStatus(ourReplyPort, ourUserPort,
					 ourReplyPort, tag,
					 region->tag, status);
	if (krtn != KERN_SUCCESS) {
	    IOLog("AS: replyStreamStatus returns %d\n", krtn);
	    xpr_audio_stream("AS: replyStreamStatus error %s (%d)\n",
			     mach_error_string(krtn), krtn, 3,4,5);
	}
    } else {
	[self createSndReplyMsg];
	if (status == _NXAUDIO_STATUS_STARTED)
	    audio_snd_reply_started(sndReplyMsg, ourReplyPort, region->tag);
        else if (status == _NXAUDIO_STATUS_COMPLETED)
	    audio_snd_reply_completed(sndReplyMsg, ourReplyPort, region->tag);
        else if (status == _NXAUDIO_STATUS_UNDERRUN)
	    audio_snd_reply_overflow(sndReplyMsg, ourReplyPort, region->tag);
	krtn = msg_send(sndReplyMsg, STREAM_SEND_OPTIONS, STREAM_SEND_TIMEOUT);
	if (krtn != KERN_SUCCESS) {
	    xpr_audio_stream("AS: sendStatusMsg msg_send error %s (%d)\n",
			     mach_error_string(krtn), krtn, 3,4,5);
	}
    }
    return self;
}

/*
 * Free a region.
 * Output stream subclass overrides to free region data.
 */
- freeRegion:(region_t *)region
{
    IOFree((void *)region, sizeof(region_t));
    return self;
}

/*
 * Subclass can implement.
 */
- completeRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            size:(u_int)xfer used:(u_int *)used
{
    return self;
}

/*
 * Handle dma descriptor complete.
 * Input streams overrides to check for recorded data to return.
 */
- dmaCompleteDescriptor:(dma_desc_t *)ddp transfered:(u_int)count
{
    region_t *region;
    queue_chain_t *region_link;
    u_int used = 0;
    BOOL sentAbort = NO;

    xpr_audio_stream("AS: dmaComp desc=0x%x, size=%d, xfer=%d\n",
		     ddp, ddp->size, count, 4,5);

    xpr_audio_stream("AS: dmaComp: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    if (queue_empty(&regionQueue)) {
	[regionQueueLock unlock];
	xpr_audio_stream("AS: dmaComp: unlocked regionQueue\n", 1,2,3,4,5);
	return self;
    }

    region = (region_t *)queue_first(&regionQueue);
    while (!queue_end(&regionQueue, (queue_entry_t)region)) {
	xpr_audio_stream("AS: dmaComplete region 0x%x\n", region, 2,3,4,5);

	if (region->start_ddp == ddp) {
	    xpr_audio_stream("AS: dmaComplete started region\n", 1,2,3,4,5);
	    if (region->messages & AS_RequestStarted)
		[self sendStatusMessage:AS_ReplyStarted forRegion:region];
	    region->start_ddp = 0;
	}

	[self completeRegion:region descriptor:ddp size:count used:&used];
	
	if ((region->end_ddp == ddp) || region->flags.aborted
	    || region->flags.split) {
	    xpr_audio_stream("AS: dmaComplete completed region\n", 1,2,3,4,5);
	    /*
	     * Old style (snd) streams get an abort message
	     * for every region.
	     */
	    if (region->flags.aborted &&
		(region->messages & AS_RequestAborted) &&
		(!sentAbort || type == AS_TypeSnd)) {
		[self sendStatusMessage:AS_ReplyAborted forRegion:region];
		sentAbort = YES;
	    }
	    /*
	     * Note snd_server interface does not allow completed messsage
	     * bit to be set if recording (for compatibility with old driver).
	     */
	    if (!region->flags.aborted && !region->flags.split &&
		(region->messages & AS_RequestCompleted))
		[self sendStatusMessage:AS_ReplyCompleted forRegion:region];
	    /*
	     * Free and remove this completed region.
	     */
	    region_link = &region->link;
	    queue_remove(&regionQueue, region, region_t *, link);
	    [self freeRegion:region];
	    region = (region_t *)queue_next(region_link);
	    continue;
	}
	if (used >= count)
	    break;
	
        region = (region_t *)queue_next(&region->link);
    }
    
    [regionQueueLock unlock];
    xpr_audio_stream("AS: dmaComp: unlocked regionQueue\n", 1,2,3,4,5);
    return self;
}

/*
 * Subclass must implement.
 */
- (u_int)mixRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            buffer:(vm_address_t)data maxCount:(u_int)max
            virgin:(BOOL)isVirgin
	    rate:(u_int)srate format:(IOAudioDataFormat)format
	    channelCount:(u_int)chans

{
    log_error(("Audio: subclass does not implement mixRegion\n"));
    return 0;
}

/*
 * Playback stream overrides this.
 */
- clearForMix:(char *)buf size:(u_int)count format:(IOAudioDataFormat)format
{
    return self;
}

/*
 * Determine if data can be converted to current device parameters.
 * Subclass overrides if it can do any conversions.
 */
- (BOOL)canConvertRegion:(region_t *)region
	     rate:(u_int)srate format:(IOAudioDataFormat)format
	     channelCount:(u_int)chans

{
    if (srate == samplingRate &&
	format == dataFormat &&
	chans == channelCount)
	return YES;
    else
	return NO;
}

/*
 * Mix into a dma buffer.
 */
- (u_int)mixBuffer:(vm_address_t)data maxCount:(u_int)max
              rate:(u_int *)srate format:(IOAudioDataFormat *)format
      channelCount:(u_int *)chans descriptor:(dma_desc_t *)ddp
            virgin:(BOOL)isVirgin streamCount:(u_int)streams
{
    region_t *region;
    u_int count = 0;
    u_int mixCount;
 	u_int temp_align_buffer;
   
    if (isPaused) {
	xpr_audio_stream("AS: mixBuffer: stream is paused\n", 1,2,3,4,5);
	return 0;
    }

    xpr_audio_stream("AS: mixBuffer: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    if (queue_empty(&regionQueue)) {
	[regionQueueLock unlock];
	xpr_audio_stream("AS: mixBuffer: unlocked regionQueue\n", 1,2,3,4,5);
	return 0;
    }

    region = (region_t *)queue_first(&regionQueue);
    while (!queue_end(&regionQueue, (queue_entry_t)region)) {
	xpr_audio_stream("AS: mixBuffer: mix region 0x%x\n", region, 2,3,4,5);
	/*
	 * Skip completed or aborted region.
	 */
	if ((region->enq_ptr >= region->end) || region->flags.aborted) {
	    region = (region_t *)queue_next(&region->link);
	    continue;
	}
	
	if (*srate == 0)
	    *srate = samplingRate;
	if (*format == IOAudioDataFormatUnset)
	    *format = dataFormat;
	if (*chans == 0)
	    *chans = channelCount;

	/*
	 * FIXME: abort/deq if can't convert?
	 */
	if (![self canConvertRegion:region rate:*srate
	    format:*format channelCount: *chans]) {
	    xpr_audio_stream("AS: mixBuffer: mix region can not convert to "
	    		     "srate=%d, "
			     "fmt=%d chans=%d\n", *srate, *format, *chans,
			     4,5);
	    region = (region_t *)queue_next(&region->link);
	    continue;
	}

	temp_align_buffer = (data + count);
	
#if defined(sparc) || defined(hppa)
	if ((dataFormat == IOAudioDataFormatLinear16)  && (temp_align_buffer & 1))
			temp_align_buffer &= 0xfffffffe;

#endif

	mixCount = [self mixRegion:region descriptor:ddp
                          buffer:(temp_align_buffer)
		          maxCount:(max - count) virgin:isVirgin
			  rate:*srate format:*format
			  channelCount:*chans];

	if (!region->flags.started) {
	    region->start_ddp = ddp;
	    region->flags.started = TRUE;
	}
	if (region->enq_ptr >= region->end && !region->flags.ended) {
	    region->end_ddp = ddp;
	    region->flags.ended = TRUE;
	}

	xpr_audio_stream("AS: mixBuffer: enq_ptr=0x%x\n", region->enq_ptr, 
				2,3,4,5);
	count += mixCount;
	if (count >= max)
	    break;
        region = (region_t *)queue_next(&region->link);
    }

    [regionQueueLock unlock];
    xpr_audio_stream("AS: mixBuffer: unlocked regionQueue\n", 1,2,3,4,5);
    /*
     * First playback mix needs to clear buffer to end.
     */
    if (isVirgin && (count < max))
	[self clearForMix:(char *)(data + count) size:(max - count)
	           format: *format];

    return count;
}

/*
 * Mark current regions as aborted.
 * Return YES if any region marked.
 */
- (BOOL)markAbortionsExclude:(BOOL)excluded
{
    region_t *region;
    BOOL ret = NO;

    xpr_audio_stream("AS: markAbortions: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    if (queue_empty(&regionQueue)) {
	[regionQueueLock unlock];
	xpr_audio_stream("AS: markAbortions: unlocked regionQueue\n", 
			1,2,3,4,5);
	return ret;
    }

    region = (region_t *)queue_first(&regionQueue);
    while (!queue_end(&regionQueue, (queue_entry_t)region)) {
	region->flags.aborted = TRUE;
	region->flags.excluded = excluded;
	ret = YES;
	xpr_audio_stream("AS: markAbortions: region=0x%x, excl=%d\n",
			 region, excluded, 3,4,5);
        region = (region_t *)queue_next(&region->link);
    }

    [regionQueueLock unlock];
    xpr_audio_stream("AS: markAbortions: unlocked regionQueue\n", 1,2,3,4,5);
    return ret;
}

/*
 * Subtract timevals.
 */
static void subtract_times(struct timeval *t1, struct timeval *t2)
{
    t1->tv_sec -= t2->tv_sec;
    t1->tv_usec -= t2->tv_usec;
    if (t1->tv_usec < 0) {
	t1->tv_sec--;
	t1->tv_usec += 1000000;
    }
}

/*
 * Thread to send timed controls.
 * One each launched for pause, resume, and abort.
 */
static void controlThread(void *arg)
{
    port_t listenPort;
    kern_return_t krtn;
    control_msg_t *controlMsg =
	(control_msg_t *)IOMalloc(sizeof(control_msg_t));
    int timeout = 0;
    AudioStream *stream = nil;
    ACStreamControl action = 0;
    struct timeval now, when;
#ifndef	KERNEL
    struct timezone tz;
#endif	KERNEL

#ifdef	KERNEL
    listenPort = (port_t)threadArg;
    [threadArgLock unlock];
#else	KERNEL
    listenPort = (port_t)arg;
#endif	KERNEL

    xpr_audio_stream("AS: controlThread: port=0x%x\n", listenPort, 2,3,4,5);

    while (1) {
	controlMsg->header.msg_local_port = listenPort;
	controlMsg->header.msg_size = sizeof(control_msg_t);
	if (timeout > 0)
	    krtn = msg_receive(&controlMsg->header, RCV_TIMEOUT, timeout);
	else
	    krtn = msg_receive(&controlMsg->header, MSG_OPTION_NONE, 0);
	/*
	 * Timeout elapsed, perform stream control action.
	 */
	if (krtn == RCV_TIMED_OUT) {
	    [stream control:action];
	    timeout = 0;
	    continue;
	}
	/*
	 * Exit when port goes stale.
	 */
	if (krtn != KERN_SUCCESS) {
	    xpr_audio_stream("AS: controlThread msg_rcv error %d\n", krtn,
			     2,3,4,5);
	    break;
	}
	/*
	 * Request for action, calculate timeout.
	 */
	if (controlMsg->header.msg_id != AC_CONTROL_THREAD_MSG) {
	    xpr_audio_stream("AS: controlThread bad msg id=%d\n",
			     controlMsg->header.msg_id, 2,3,4,5);
	    break;
	}
	stream = (AudioStream *)controlMsg->obj;
	action = controlMsg->action;
	when = *controlMsg->when;
#ifdef	KERNEL
	microtime(&now);
#else	KERNEL
	gettimeofday(&now, &tz);
#endif	KERNEL
	subtract_times(&when, &now);
	timeout = when.tv_sec * 1000;
	timeout += when.tv_usec / 1000;
	xpr_audio_stream("AS: controlThread timeout = %d\n", timeout,
			 2,3,4,5);
	if (timeout <= 0) {
	    [stream control:action];
	    timeout = 0;
	}
    }
    xpr_audio_stream("AS: controlThread: port=0x%x exits\n", listenPort,
		     2,3,4,5);
    IOExitThread();
}

/*
 * Timed stream control.
 */
static void timedControl(port_t *threadPort, id self, ACStreamControl action,
			 struct timeval *when)
{
    static const control_msg_t msg = {
      {
	  0,				/* msg_unused */
	  TRUE,				/* msg_simple */
	  sizeof(control_msg_t),	/* msg_size */
	  MSG_TYPE_NORMAL,		/* msg_type */
	  PORT_NULL,			/* msg_local_port */
	  PORT_NULL,			/* msg_remote_port */
	  AC_CONTROL_THREAD_MSG		/* msg_id */
      },
      {
	  MSG_TYPE_INTEGER_32,		/* msg_type_name */
	  32,				/* msg_type_size */
	  3,				/* msg_type_number */
	  FALSE,			/* msg_type_longform */
	  FALSE				/* msg_type_deallocate */
      },
      0,
      0,
      0
    };
    control_msg_t *controlMsg =
	(control_msg_t *)IOMalloc(sizeof(control_msg_t));
    kern_return_t krtn;

    if (!(*threadPort)) {
	krtn = port_allocate(task_self(), threadPort);
	if (krtn) {
	    log_error(("Audio: stream control thread port_allocate: %s\n",
		       mach_error_string(krtn)));
	    audio_panic();
	}
#ifdef	KERNEL
	if (threadArgLock == nil)
	    threadArgLock = [[NXLock alloc] init];
	[threadArgLock lock];
	threadArg = (void *)*threadPort;
#ifdef	DEBUG
	IOLog("threadArg = 0x%x\n", threadArg);
#endif	DEBUG
	kernel_thread(current_task_EXTERNAL(), (void *)controlThread);
#else	KERNEL
	(void)IOForkThread((IOThreadFunc)controlThread, (void *)*threadPort);
#endif
    }
    *controlMsg = msg;
    controlMsg->header.msg_remote_port = *threadPort;
    controlMsg->obj = self;
    controlMsg->action = action;
    controlMsg->when = when;

    krtn = msg_send(&controlMsg->header, SEND_TIMEOUT, 1000);
    if (krtn != KERN_SUCCESS) {
	xpr_audio_stream("AS: timedControl msg_send error %s (%d)\n",
			 mach_error_string(krtn), krtn, 3,4,5);
    }
}

/*
 * Stream control.
 */
- control:(ACStreamControl)action atTime:(struct timeval)when
{
    switch (action) {
      case AC_ControlPause:
	if (timerisset(&when)) {
	    pauseTime = when;
	    timedControl(&pausePort, self, action, &pauseTime);
	} else {
	    isPaused = YES;
	    [self sendControlMessage:AS_ReplyPaused mask:AS_RequestPaused];
	}
	break;
      case AC_ControlResume:
	if (timerisset(&when)) {
	    resumeTime = when;
	    timedControl(&resumePort, self, action, &resumeTime);
	} else {
	    isPaused = NO;
	    [self sendControlMessage:AS_ReplyResumed mask:AS_RequestResumed];
	    [device _dataPendingForChannel: [self channel]];
	}
	break;
      case AC_ControlAbort:
	if (timerisset(&when)) {
	    abortTime = when;
	    timedControl(&abortPort, self, action, &abortTime);
	} else if (![self markAbortionsExclude:NO]) {
	    /*
	     * Send reply messages now if no regions to abort.
	     */
	    [self sendControlMessage:AS_ReplyAborted mask:AS_RequestAborted];
	}
	break;
      case AC_ControlExclude:
	[self markAbortionsExclude:YES];
	break;
      default:
	log_error(("audio: unrecognized stream control %d\n", action));
	break;
    }
    return self;
}

/*
 * Stream control.
 */
- control:(ACStreamControl)action
{
    struct timeval now;

    timerclear(&now);
    return [self control:action atTime:now];
}

/*
 * InputStream overrides.
 */
- returnRecordedData
{
    return self;
}

/*
 * Free regions and region queue.
 */
- freeRegions
{
    region_t *region;

    xpr_audio_stream("AS: free regions\n", 1,2,3,4,5);
    while (!queue_empty(&regionQueue)) {
	queue_remove_first(&regionQueue, region, region_t *, link);
	[self freeRegion:region];
    }
    return self;
}

/*
 * Allocate and init a region.
 */
- (region_t *)newRegion
{
    region_t *region = (region_t *)IOMalloc(sizeof(region_t));

    bzero((char *)region, sizeof(region_t));
    return region;
}

/*
 * Free resources and object.
 */
- free
{
    kern_return_t krtn;

    if (pausePort)
	port_deallocate(task_self(), pausePort);
    if (resumePort)
	port_deallocate(task_self(), resumePort);
    if (abortPort)
	port_deallocate(task_self(), abortPort);
    [self freeRegions];
    krtn = port_deallocate(task_self(), userPort);
    if (krtn)
	log_error(("Audio: stream port_deallocate: %s\n",
		   mach_error_string(krtn)));
    [regionQueueLock free];
    if (sndReplyMsg)
	IOFree(sndReplyMsg, MSG_SIZE_MAX);
    return [super free];
}

/*
 * Set stream owner port.
 */
- setOwner:(port_t)newOwner
{
    ownerPort = newOwner;
    return self;
}

/*
 * This is dependent upon the machine representation of ns_time_t.
 */

- (BOOL)bytesProcessed:(unsigned int *)num atTime:(unsigned int *)ts
{
    ns_time_t interruptTime, startTime;
    
    startTime = [device _outputStartTime];
    interruptTime = [device _lastInterruptTimeStamp];
    
    /* Almost certainly the DMA hasn't started yet. */
    if (interruptTime == 0)	{
    	*num = *ts = 0;
	return NO;
    }

    interruptTime /= 1000;	// microseconds
    startTime /= 1000;
        
    xpr_audio_stream("AS: byteCount %d at %d\n", 
    	bytesProcessed, interruptTime,3,4,5);
    
    if (ts != NULL)
	*ts = (unsigned int) interruptTime;
    *num = bytesProcessed;
	
    return YES;
}

- (NXSoundParameterTag)dataEncoding
{
    /* FIXME: change dataFormat to dataEncoding */

    switch (dataFormat) {
      case IOAudioDataFormatLinear16:
	return NX_SoundStreamDataEncoding_Linear16;
      case IOAudioDataFormatLinear8:
	return NX_SoundStreamDataEncoding_Linear8;
      case IOAudioDataFormatMulaw8:
	return NX_SoundStreamDataEncoding_Mulaw8;
      case IOAudioDataFormatAlaw8:
	return NX_SoundStreamDataEncoding_Alaw8;
      default:
	IOLog("Audio: unrecognized data format: %d\n", dataFormat);
	return -1;
    }
}
- (void)setDataEncoding:(NXSoundParameterTag)encoding
{
    /* FIXME: change dataFormat to dataEncoding */

    switch (encoding) {
      case NX_SoundStreamDataEncoding_Linear16:
	dataFormat = IOAudioDataFormatLinear16;
	break;
      case NX_SoundStreamDataEncoding_Linear8:
	dataFormat = IOAudioDataFormatLinear8;
	break;
      case NX_SoundStreamDataEncoding_Mulaw8:
	dataFormat = IOAudioDataFormatMulaw8;
	break;
      case NX_SoundStreamDataEncoding_Alaw8:
	dataFormat = IOAudioDataFormatAlaw8;
	break;
      case NX_SoundStreamDataEncoding_AES:
	dataFormat = IOAudioDataFormatAES;
	break;
      default:
	IOLog("Audio: supported encoding: %d\n", encoding);
	break;
    }
}
- (u_int)samplingRate
{
    return samplingRate;
}
- (void)setSamplingRate:(u_int)rate
{
    samplingRate = rate;
}
- (u_int)channelCount
{
    return channelCount;
}
- (void)setChannelCount:(u_int)count
{
    channelCount = count;
}
- (u_int)lowWaterMark
{
    return 0;
}
- (void)setLowWaterMark:(u_int)count
{
    return;
}
- (u_int)highWaterMark
{
    return 0;
}
- (void)setHighWaterMark:(u_int)count
{
    return;
}

@end
