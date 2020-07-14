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
 * audio_server.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *	Field user messages for audio driver.
 *
 * HISTORY
 *	08/10/92/mtm	Original coding from m68k version.
 */

#import "audioLog.h"
#import <driverkit/IOAudioPrivate.h>
#import "InputStream.h"
#import "OutputStream.h"
#import "audio_server.h"
#import <bsd/dev/audioTypes.h>
#import "audioReply.h"
#import <driverkit/kernelDriver.h>	// for IOVmTaskSelf()
#import <mach/vm_param.h>		// PAGE_SIZE
#import <kernserv/prototypes.h>

/*
 * DO NOT PUT ANY STATIC DATA IN THIS FILE.
 * These routines may be used by multiple IOAudio instances
 * if more sound boards are added.
 */

/*
 * Mig calls this to convert the port a message is
 * received on into each routine's first argument.
 */
audio_device_t audio_port_to_device(port_t port)
{
    AudioChannel *chan = nil;

    if (chan = [IOAudio _channelForUserPort:port])
	return (audio_device_t)chan;
    else {
	log_error(("Audio: server can't translate port to channel\n"));
	return (audio_device_t)0;
    }
}

/*
 * Mig calls this to convert the port a message is
 * received on into each routine's first argument.
 */
audio_stream_t audio_port_to_stream(port_t port)
{
    AudioStream *stream = nil;

    if (stream = [AudioChannel streamForUserPort:port])
	return (audio_stream_t)stream;
    else {
	log_error(("Audio: server can't translate port to stream\n"));
	return (audio_stream_t)0;
    }
}

/**************************
 * Generic device routines.
 **************************/

/*
 * Get current exclusive stream owner, may be PORT_NULL.
 */
kern_return_t _NXAudioGetExclusiveUser(AudioChannel *chan,
				       port_t *stream_owner)
{
    xpr_audio_user("AU: GetExclUser: chan=0x%x, owner=0x%x\n", chan, stream_owner, 3,4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *stream_owner = [chan exclusiveUser];
    return _NXAUDIO_SUCCESS;
}

/*
 * Establish exclusive use of streams.
 * If stream_owner is PORT_NULL, exclusive use is canceled.
 */
kern_return_t _NXAudioSetExclusiveUser(AudioChannel *chan, port_t stream_owner)
{
    xpr_audio_user("AU: SetExclUser: chan=0x%x, owner=%d\n", chan, stream_owner,3,4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    [chan setExclusiveUser:stream_owner];
    return _NXAUDIO_SUCCESS;
}

/*
 * Get dma buffer size and count.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioGetBufferOptions(AudioChannel *chan, u_int *size,
				       u_int *count)
{
    xpr_audio_user("AU: GetBufOpts: chan=0x%x, size=0x%x, count=0x%x\n",
		   chan, size, count, 4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *size = [[chan audioDevice] _intValueForParameter:NX_SoundDeviceBufferSize
	        forObject:chan];
    *count = [[chan audioDevice] _intValueForParameter:NX_SoundDeviceBufferCount
	         forObject:chan];
    return _NXAUDIO_SUCCESS;
}

/*
 * Set dma buffer size and count.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioSetBufferOptions(AudioChannel *chan,
				       port_t stream_owner,
				       u_int size, u_int count)
{
    xpr_audio_user("AU: SetBufOpts: chan=0x%x, owner=%d, size=%d, count=%d\n",
		   chan, stream_owner, size, count, 5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    (void)[[chan audioDevice] _setParameter:NX_SoundDeviceBufferSize toInt:size
	   forObject:chan];
    (void)[[chan audioDevice] _setParameter:NX_SoundDeviceBufferCount toInt:count
	   forObject:chan];
    return _NXAUDIO_SUCCESS;
}

/*
 * Stream control of owned streams.
 */
kern_return_t _NXAudioControlStreams(AudioChannel *chan, port_t stream_owner,
				     int action)
{
    xpr_audio_user("AU: ControlStreams: chan=0x%x, owner=%d, action=%d\n",
		   chan, stream_owner, action, 4,5);
    if (!chan || !stream_owner)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    if (!(action == _NXAUDIO_STREAM_PAUSE ||
	  action == _NXAUDIO_STREAM_RESUME ||
	  action == _NXAUDIO_STREAM_ABORT ||
	  action == _NXAUDIO_STREAM_RETURN_DATA))
	return _NXAUDIO_ERR_CONTROL;

    [chan controlStreams:action];
    return _NXAUDIO_SUCCESS;
}

/*
 * Add a stream.
 */
kern_return_t _NXAudioAddStream(AudioChannel *chan, port_t *stream_port,
				port_t stream_owner, int stream_id,
				u_int stream_type)
    
{
    xpr_audio_user("AU: AddStream: chan=0x%x, stream=0x%x, owner=%d, id=%d, "
		   "type=%d\n", chan, stream_port, stream_owner, stream_id,
		   stream_type);
    if (!chan || !stream_owner)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;

    return ([chan addStreamTag:stream_id user:stream_port
	        owner:stream_owner type:stream_type] ?
	    _NXAUDIO_SUCCESS : KERN_FAILURE);
}

/*
 * Get device peak options.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioGetDevicePeakOptions(AudioChannel *chan, u_int *enabled,
					   u_int *history)
{
    xpr_audio_user("AU: GetDevPeakOpts: chan=0x%x, enabled=0x%x, "
    		"history=0x%x\n", chan, enabled, history, 4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *enabled = [[chan audioDevice]
	        _intValueForParameter:NX_SoundDeviceDetectPeaks
		forObject:chan];
    *history = 1;	/* peak history no longer supported */
    return _NXAUDIO_SUCCESS;
}

/*
 * Set device peak options.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioSetDevicePeakOptions(AudioChannel *chan,
					   port_t stream_owner,
					   u_int enabled,
					   u_int history)
{
    xpr_audio_user("AU: SetDevPeakOpts: chan=0x%x, owner=%d, enabled=%d, "
		   "history=%\n", chan, stream_owner, enabled, history, 5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    (void)[[chan audioDevice] _setParameter:NX_SoundDeviceDetectPeaks
	      toInt:(enabled ? YES : NO) forObject:chan];
    /* peak history no longer supported */
    return _NXAUDIO_SUCCESS;
}

/*
 * Get device peak.
 */
kern_return_t _NXAudioGetDevicePeak(AudioChannel *chan, u_int *peak_left,
				    u_int *peak_right)
{
    xpr_audio_user("AU: GetDevPeak: chan=0x%x, peak_l=0x%x, peak_r=0x%x\n",
		   chan, peak_left, peak_right, 4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![[chan audioDevice] _intValueForParameter:NX_SoundDeviceDetectPeaks
	forObject:chan])
	return _NXAUDIO_ERR_PEAK;
    [chan getPeakLeft:peak_left right:peak_right];
    return _NXAUDIO_SUCCESS;
}

/*
 * Get clip count.
 */
kern_return_t _NXAudioGetClipCount(AudioChannel *chan, u_int *count)
{
    xpr_audio_user("AU: GetClipCount: chan=0x%x, count=0x%x\n", chan, count,
		   3,4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *count = [chan clipCount];
    return _NXAUDIO_SUCCESS;
}

/********************
 * Soundout routines.
 ********************/

/*
 * Get soundout options.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioGetSndoutOptions(AudioChannel *chan, u_int *bits)
{
    IOAudio *dev;

    xpr_audio_user("AU: GetSndoutOpts: chan=0x%x, bits=0x%x\n", chan, bits, 3,4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *bits = 0;
    dev = [chan audioDevice];

    if ([dev _intValueForParameter:NX_SoundDeviceInsertZeros
        forObject:chan])
	*bits |= _NXAUDIO_SNDOUT_ZEROFILL;
    if ([dev _intValueForParameter:NX_SoundDeviceRampUp
        forObject:chan])
	*bits |= _NXAUDIO_SNDOUT_RAMPUP;
    if ([dev _intValueForParameter:NX_SoundDeviceRampDown
        forObject:chan])
	*bits |= _NXAUDIO_SNDOUT_RAMPDOWN;
    if ([dev _intValueForParameter:NX_SoundDeviceDeemphasize
	forObject:chan])
	*bits |= _NXAUDIO_SNDOUT_DEEMPHASIS;
    if (![dev _intValueForParameter:NX_SoundDeviceMuteSpeaker
	forObject:chan])
        *bits |= _NXAUDIO_SNDOUT_SPEAKER_ON;

    return _NXAUDIO_SUCCESS;
}

/*
 * Set soundout options.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioSetSndoutOptions(AudioChannel *chan,
				       port_t stream_owner,
				       u_int bits)
{
    IOAudio *dev;

    xpr_audio_user("AU: SetSndoutOpts: chan=0x%x, owner=%d, bits=0x%x\n", chan,
		   stream_owner, bits, 4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    dev = [chan audioDevice];

    if (bits & _NXAUDIO_SNDOUT_ZEROFILL)
       (void)[dev _setParameter:NX_SoundDeviceInsertZeros toInt:YES
	     forObject:chan];
    else
       (void)[dev _setParameter:NX_SoundDeviceInsertZeros toInt:NO
	     forObject:chan];
    if (bits & _NXAUDIO_SNDOUT_RAMPUP)
       (void)[dev _setParameter:NX_SoundDeviceRampUp toInt:YES
             forObject:chan];
    else
	(void)[dev _setParameter:NX_SoundDeviceRampUp toInt:NO
              forObject:chan];
    if (bits & _NXAUDIO_SNDOUT_RAMPDOWN)
	(void)[dev _setParameter:NX_SoundDeviceRampDown toInt:YES
              forObject:chan];
    else
	(void)[dev _setParameter:NX_SoundDeviceRampDown toInt:NO
              forObject:chan];
    if (bits & _NXAUDIO_SNDOUT_DEEMPHASIS)
	(void)[dev _setParameter:NX_SoundDeviceDeemphasize toInt:YES
              forObject:chan];
    else
	(void)[dev _setParameter:NX_SoundDeviceDeemphasize toInt:NO
              forObject:chan];
    if (bits & _NXAUDIO_SNDOUT_SPEAKER_ON)
	(void)[dev _setParameter:NX_SoundDeviceMuteSpeaker toInt:NO
              forObject:chan];
    else
	(void)[dev _setParameter:NX_SoundDeviceMuteSpeaker toInt:YES
              forObject:chan];

    return _NXAUDIO_SUCCESS;
}

/*
 * Get speaker attenuation.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioGetSpeaker(AudioChannel *chan, int *left, int *right)
{
    xpr_audio_user("AU: GetSpeaker: chan=0x%x, left=0x%x, right=0x%x\n", chan,
		   left, right, 4,5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *left = [[chan audioDevice]
            _intValueForParameter:NX_SoundDeviceOutputAttenuationLeft
            forObject:chan];
    *right = [[chan audioDevice]
             _intValueForParameter:NX_SoundDeviceOutputAttenuationRight
             forObject:chan];
    return _NXAUDIO_SUCCESS;
}

/*
 * Set speaker attenuation.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioSetSpeaker(AudioChannel *chan,
				 port_t stream_owner,
				 int left, int right)
{
    xpr_audio_user("AU: SetSpeaker: chan=0x%x, owner=%d, left=%d, right=%d\n",
		   chan, stream_owner, left, right, 5);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    (void)[[chan audioDevice]
          _setParameter:NX_SoundDeviceOutputAttenuationLeft toInt:left
	  forObject:chan];
    (void)[[chan audioDevice]
          _setParameter:NX_SoundDeviceOutputAttenuationRight toInt:right
          forObject:chan];
    return _NXAUDIO_SUCCESS;
}

/**************************
 * Generic stream routines.
 **************************/

/*
 * Set stream gain.
 */
kern_return_t _NXAudioSetStreamGain(OutputStream *stream, int left, int right)
{
    xpr_audio_user("AU: SetStreamGain: stream=0x%x, left=%d, right=%d\n",
		   stream, left, right, 4,5);
    [[[stream channel] audioDevice] _setParameter:NX_SoundStreamGainLeft
        toInt:left forObject:stream];
    [[[stream channel] audioDevice] _setParameter:NX_SoundStreamGainRight
        toInt:right forObject:stream];
    return _NXAUDIO_SUCCESS;
}

/*
 * Change stream owner of an existing stream.
 */
kern_return_t _NXAudioChangeStreamOwner(AudioStream *stream,
					port_t stream_owner)
{
    xpr_audio_user("AU: ChangeStreamOwner: stream=0x%x, owner=%d\n",
		   stream, stream_owner, 3,4,5);
    if (!stream || !stream_owner)
	return _NXAUDIO_ERR_PORT;
    [stream setOwner:stream_owner];
    return _NXAUDIO_SUCCESS;
}

/*
 * Stream control.
 */
kern_return_t _NXAudioStreamControl(AudioStream *stream, int action,
				    audio_time_t time)
{
    xpr_audio_user("AU: StreamControl: stream=0x%x, action=%d, time=%d:%d\n",
		   stream, action, time.tv_sec, time.tv_usec, 5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    if (![[stream channel] checkOwner:[stream ownerPort]])
	return _NXAUDIO_ERR_NOT_OWNER;

    switch (action) {
      case _NXAUDIO_STREAM_ABORT:
	[stream control:AC_ControlAbort atTime:time];
	break;
      case _NXAUDIO_STREAM_PAUSE:
	[stream control:AC_ControlPause atTime:time];
	break;
      case _NXAUDIO_STREAM_RESUME:
	[stream control:AC_ControlResume atTime:time];
	break;
      case _NXAUDIO_STREAM_RETURN_DATA:
	[stream returnRecordedData];
	break;
      default:
	return _NXAUDIO_ERR_CONTROL;
	break;
    }
    return _NXAUDIO_SUCCESS;
}

/*
 * Stream info.
 */
kern_return_t _NXAudioStreamInfo(AudioStream *stream, u_int *bytes_processed,
				u_int *time_stamp)
{
    xpr_audio_user("AU: StreamInfo: stream=0x%x, bytes=0x%x\n",
		   stream, bytes_processed, 3,4,5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    (void) [stream bytesProcessed: bytes_processed atTime: time_stamp];
    return _NXAUDIO_SUCCESS;
}

/*
 * Remove a stream.
 */
kern_return_t _NXAudioRemoveStream(AudioStream *stream)
{
    xpr_audio_user("AU: RemoveStream: stream=0x%x\n", stream, 2,3,4,5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    [[stream channel] removeStream:stream];
    return _NXAUDIO_SUCCESS;
}

/***************************
 * Playback stream routines.
 ***************************/

/*
 * Enqueue play request to stream.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioPlayStream(OutputStream *stream, pointer_t data,
				 u_int count, int tag, int channel_count,
				 int sample_rate,
				 u_int left_gain, u_int right_gain,
				 u_int low_water, u_int high_water,
				 port_t reply_port, u_int messages)
{
    kern_return_t ret, krtn;
    audio_array_t params;
    audio_array_t values;

    xpr_audio_user("AU: PlayStream: stream=0x%x %d %d %d %d\n", 
    		stream, data, count, tag, channel_count);
    
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    if (![[stream channel] checkOwner:[stream ownerPort]]) {
	ret = _NXAUDIO_ERR_NOT_OWNER;
	goto error_return;
    }
    if (!count) {
	ret = _NXAUDIO_ERR_SIZE;
	goto error_return;
    }

    /*
     * left and right gains no longer supported.
     */
    params[0] = NX_SoundStreamChannelCount;
    values[0] = channel_count;
    params[1] = NX_SoundStreamSamplingRate;
    /*
     * In this interface, sample rate is an enum constant!
     */
    values[1] = (sample_rate == _NXAUDIO_RATE_22050 ? 22050 : 44100);
    params[2] = NX_SoundStreamLowWaterMark;
    values[2] = low_water;
    params[3] = NX_SoundStreamHighWaterMark;
    values[3] = high_water;
    (void)[[[stream channel] audioDevice]
	 _setParameters:(const NXSoundParameterTag *)params
	 toValues:values count:4 forObject:stream];

    if ([stream playBuffer:(void *)data size:count
                                     tag:tag
                                 replyTo:reply_port
                               replyMsgs:messages])
	return _NXAUDIO_SUCCESS;
    else
	ret = _NXAUDIO_ERR_SIZE;

  error_return:
    krtn = vm_deallocate(task_self(), data, count);
    if (krtn != KERN_SUCCESS)
	log_error(("Audio: audio server vm_deallocate error: %s (%d)\n",
		   mach_error_string(krtn), krtn));
    xpr_audio_user("AU: playStream returns err %d\n", krtn, 2,3,4,5);
    return ret;
}

/*
 * Set stream peak options.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioSetStreamPeakOptions(OutputStream *stream, u_int enabled,
					   u_int history)
{
    xpr_audio_user("AU: SetStreamPeakOpts: stream=0x%x, enabled=%d, "
    		"history=%d\n", stream, enabled, history, 4,5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    (void)[[[stream channel] audioDevice]
	      _setParameter:NX_SoundStreamDetectPeaks
	      toInt:(enabled ? YES : NO) forObject:stream];
    /* peak history no longer supported */
    return _NXAUDIO_SUCCESS;
}

/*
 * Get stream peak.
 */
kern_return_t _NXAudioGetStreamPeak(OutputStream *stream,
				    u_int *peak_left, u_int *peak_right)
{
    xpr_audio_user("AU: GetStreamPeak: stream=0x%x, peak_l=0x%x, "
    			"peak_r=0x%x\n", stream, peak_left, peak_right, 4,5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    if (![[[stream channel] audioDevice]
	_intValueForParameter:NX_SoundStreamDetectPeaks forObject:stream])
	return _NXAUDIO_ERR_PEAK;
    [stream getPeakLeft:peak_left right:peak_right];
    return _NXAUDIO_SUCCESS;
}

/****************************
 * Recording stream routines.
 ****************************/

/*
 * Enqueue record request to stream.
 * (OBSOLETE but must still be supported.)
 */
kern_return_t _NXAudioRecordStream(InputStream *stream, u_int count,
				   int tag,
				   u_int low_water, u_int high_water,
				   port_t reply_port, u_int messages)
{
    audio_array_t params;
    audio_array_t values;

    xpr_audio_user("AU: RecordStream: stream=0x%x %d %d\n", 
    		stream, count, tag, 4, 5);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    if (![[stream channel] checkOwner:[stream ownerPort]])
	return _NXAUDIO_ERR_NOT_OWNER;
    if (!count)
	return _NXAUDIO_ERR_SIZE;

    params[0] = NX_SoundStreamLowWaterMark;
    values[0] = low_water;
    params[1] = NX_SoundStreamHighWaterMark;
    values[1] = high_water;
    (void)[[[stream channel] audioDevice]
	 _setParameters:(const NXSoundParameterTag *)params
	 toValues:values count:2 forObject:stream];

    if ([stream recordSize:count tag:tag
                         replyTo:reply_port
                       replyMsgs:messages])
	return _NXAUDIO_SUCCESS;
    else
	return _NXAUDIO_ERR_SIZE;
}

/*
 * Deal with death of a port.
 */
void audio_port_gone(port_t port)
{
    AudioStream *stream = nil;
    AudioChannel *chan = nil;
    boolean_t more = TRUE;

    if (port == PORT_NULL)
	return;

    while (more) {
	more = FALSE;
	if (chan = [IOAudio _channelForExclusivePort:port]) {
	    xpr_audio_user("AU: audio_port_gone: chan 0x%x excl user %d\n",
			   chan, port, 3,4,5);
	    [chan setExclusiveUser:PORT_NULL];
	    more = TRUE;
	} else if (stream = [AudioChannel streamForOwnerPort:port]) {
	    xpr_audio_user("AU: audio_port_gone: stream 0x%x owner %d\n",
			   stream, port, 3,4,5);
	    _NXAudioRemoveStream(stream);
	    more = TRUE;
	}
    }
}

/****************************
 * New for 3.1.
 ****************************/

/*
 * Play and record with current parameters.
 */

kern_return_t _NXAudioPlayStreamData(OutputStream *stream, pointer_t data,
				     u_int count, int tag,
				     port_t reply_port, u_int messages)
{
    xpr_audio_user("AU: playStreamData\n", 0,0,0,0,0);
    if (!stream)
	return _NXAUDIO_ERR_PORT;

    if ([stream playBuffer:(void *)data size:count tag:tag
        replyTo:reply_port replyMsgs:messages])
	return _NXAUDIO_SUCCESS;
    else
	return _NXAUDIO_ERR_SIZE;
}

kern_return_t _NXAudioRecordStreamData(InputStream *stream, u_int count,
					int tag,
					port_t reply_port, u_int messages)
{
    xpr_audio_user("AU: recordStreamData\n", 0,0,0,0,0);
    if (!stream)
	return _NXAUDIO_ERR_PORT;

    if ([stream recordSize:count tag:tag
        replyTo:reply_port replyMsgs:messages])
	return _NXAUDIO_SUCCESS;
    else
	return _NXAUDIO_ERR_SIZE;
}

/*
 * Generic parameter api.
 */

kern_return_t _NXAudioGetDeviceName(AudioChannel *chan, audio_name_t name,
				    u_int *count)
{
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    strncpy(name, [[IOAudio _instance] name], _NXAUDIO_PARAM_MAX-1);
    name[_NXAUDIO_PARAM_MAX-1] = '\0';
    *count = strlen(name);
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioSetDeviceParameters(AudioChannel *chan,
					  port_t stream_owner,
					  audio_array_t params, u_int count,
					  audio_array_t values)
{
    xpr_audio_user("AU: setDevParams\n", 0,0,0,0,0);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    if (![chan checkOwner:stream_owner])
	return _NXAUDIO_ERR_NOT_OWNER;
    return ([[chan audioDevice] 
    		_setParameters:(const NXSoundParameterTag *)params
	        toValues:values count:count forObject:chan] ?
	    _NXAUDIO_SUCCESS : _NXAUDIO_ERR_PARAMETER);
}

kern_return_t _NXAudioGetDeviceParameters(AudioChannel *chan,
					  audio_array_t params, u_int count,
					  audio_array_t values)
{
    xpr_audio_user("AU: getDevParams\n", 0,0,0,0,0);
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    [[chan audioDevice] _getParameters:(const NXSoundParameterTag *)params
        values:values count:count forObject:chan];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetDeviceSupportedParameters(AudioChannel *chan,
						   audio_array_t params,
						   u_int *count)
{
    xpr_audio_user("AU: getDevSupParams\n", 0,0,0,0,0);
    *count = 0;
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    [[chan audioDevice] _getSupportedParameters:
        (NXSoundParameterTag *)params
        count:count forObject:chan];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetDeviceParameterValues(AudioChannel *chan, int param,
					       audio_array_t values,
					       u_int *count)
{
    xpr_audio_user("AU: getDevParamValues\n", 0,0,0,0,0);
    *count = 0;
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    return ([[chan audioDevice] _getValues:(NXSoundParameterTag *)values
	        count:count forParameter:param forObject:chan] ?
	    _NXAUDIO_SUCCESS : _NXAUDIO_ERR_PARAMETER);
}

kern_return_t _NXAudioGetSamplingRates(AudioChannel *chan,
				       int *continuous,
				       int *low, int *high,
				       audio_array_t rates,
				       u_int *count)
{
    xpr_audio_user("AU: getRates\n", 0,0,0,0,0);
    *count = 0;
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *continuous = [[chan audioDevice] acceptsContinuousSamplingRates];
    [[chan audioDevice] getSamplingRatesLow:low high:high];
    [[chan audioDevice] getSamplingRates:rates count:count];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetDataEncodings(AudioChannel *chan,
				       audio_array_t encodings,
				       u_int *count)
{
    xpr_audio_user("AU: getEncodings\n", 0,0,0,0,0);
    
    *count = 0;
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    [[chan audioDevice]
        getDataEncodings:(NXSoundParameterTag *)encodings
        count:count];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetChannelCountLimit(AudioChannel *chan, u_int *count)
{
    xpr_audio_user("AU: getChans\n", 0,0,0,0,0);
    *count = 0;
    if (!chan)
	return _NXAUDIO_ERR_PORT;
    *count = [[chan audioDevice] channelCountLimit];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioSetStreamParameters(AudioStream *stream,
					  audio_array_t params, u_int count,
					  audio_array_t values)
{
    xpr_audio_user("AU: setStreamParams\n", 0,0,0,0,0);
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    return ([[[stream channel] audioDevice]
                _setParameters:(const NXSoundParameterTag *)params
	        toValues:values count:count forObject:stream] ?
	    _NXAUDIO_SUCCESS : _NXAUDIO_ERR_PARAMETER);
}

kern_return_t _NXAudioGetStreamParameters(AudioStream *stream,
					  audio_array_t params, u_int count,
					  audio_array_t values)
{
    xpr_audio_user("AU: getStreamParams\n", 0,0,0,0,0);
    if (!stream)
	return _NXAUDIO_ERR_PORT;

    [[[stream channel] audioDevice]
        _getParameters:(const NXSoundParameterTag *)params
	values:values count:count forObject:stream];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetStreamSupportedParameters(AudioStream *stream,
						   audio_array_t params,
						   u_int *count)
{
    xpr_audio_user("AU: getStreamSupParams\n", 0,0,0,0,0);
    *count = 0;
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    [[[stream channel] audioDevice]
        _getSupportedParameters:(NXSoundParameterTag *)params
        count:count forObject:stream];
    return _NXAUDIO_SUCCESS;
}

kern_return_t _NXAudioGetStreamParameterValues(AudioStream *stream, int param,
					       audio_array_t values,
					       u_int *count)
{
    xpr_audio_user("AU: getStreamParamValues\n", 0,0,0,0,0);
    *count = 0;
    if (!stream)
	return _NXAUDIO_ERR_PORT;
    return ([[[stream channel] audioDevice]
                _getValues:(NXSoundParameterTag *)values
	        count:count forParameter:param forObject:stream] ?
	    _NXAUDIO_SUCCESS : _NXAUDIO_ERR_PARAMETER);
}
