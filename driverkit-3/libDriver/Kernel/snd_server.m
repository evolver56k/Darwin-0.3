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
 * snd_server.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *	Field old SoundDSP driver user messages for audio driver.
 *
 * HISTORY
 *	08/10/92/mtm	Original coding.
 */

#import "audioLog.h"
#import "snd_reply.h"
#import "snd_server.h"
#import "audio_server.h"
#import <driverkit/IOAudioPrivate.h>
#import "AudioChannel.h"
#import "AudioStream.h"
#import "audio_kern_server.h"
#import <driverkit/NXSoundParameterTags.h>
#import <bsd/dev/snd_msgs.h>
#import <bsd/dev/audioTypes.h>
#import <mach/mach_types.h>
#import <kernserv/queue.h>
#import	<bsd/sys/time.h>
#import <driverkit/kernelDriver.h>	// for IOVmTaskSelf()

#define SND_DIR_PLAY	0
#define SND_DIR_RECORD	1

/*
 * DO NOT PUT ANY STATIC DATA IN THIS FILE.
 * These routines may be used by multiple AudioDevice instances
 * if more sound boards are added.
 */

static int snd_nx_encoding(int encoding)
{
    switch (encoding) {
      case SND_STREAM_FORMAT_ENCODING_LINEAR_16:
	return NX_SoundStreamDataEncoding_Linear16;
      case SND_STREAM_FORMAT_ENCODING_LINEAR_8:
	return NX_SoundStreamDataEncoding_Linear8;
      case SND_STREAM_FORMAT_ENCODING_MULAW_8:
      default:
	return NX_SoundStreamDataEncoding_Mulaw8;
    }
}
	
static void snd_set_stream_format(int dir, void *stream,
				  int srate, int encoding, int chans,
				  int low_water, int high_water)
{
    int parray[] = { NX_SoundStreamLowWaterMark,
                         NX_SoundStreamHighWaterMark,
                         NX_SoundStreamSamplingRate,
                         NX_SoundStreamDataEncoding,
                         NX_SoundStreamChannelCount };
    int pcount = sizeof(parray)/sizeof(int);
    int pvalues[pcount];
    kern_return_t kerr;
    
    pvalues[0] = low_water;
    pvalues[1] = high_water;
    pvalues[2] = srate;
    pvalues[3] = snd_nx_encoding(encoding);
    pvalues[4] = chans;
    kerr = _NXAudioSetStreamParameters(stream, parray, pcount, pvalues);
    if (kerr != KERN_SUCCESS)
	xpr_audio_user("AU: snd_set_stream_format: setStreamParams returns "
			"%d\n", kerr, 2,3,4,5);
}

/*
 * Handle stream port messages.
 */
static int snd_stream_msg(msg_header_t *in_msg, msg_header_t *out_msg)
{
    kern_return_t kerr;
    int size;
    snd_msg_type_t *msg_type;
    vm_address_t snd_data = 0;
    vm_size_t snd_size = 0;
    int snd_control = 0;
    int options = 0;
    int error = 0;
    int direction = -1;
    int high_water = 0;
    int low_water = 0;
    u_int dma_size, dma_count;
    struct timeval time_now;
    snd_stream_msg_t *stream_msg = (snd_stream_msg_t *)in_msg;
    int data_tag = stream_msg->data_tag;
    port_t reply_port = PORT_NULL;
    u_int bytes_processed, time_stamp;
    int sample_rate = 0, data_encoding = 0, chan_count = 0;
    boolean_t set_format = FALSE;
    AudioStream *audioStream = [AudioChannel
			           streamForUserPort:in_msg->msg_local_port];

    xpr_audio_user("AU: snd_stream_msg: stream 0x%x\n", audioStream, 
    		2,3,4,5);

    if (in_msg->msg_id == SND_MSG_STREAM_NSAMPLES) {
	xpr_audio_user("AU: snd_stream_msg: nsamples\n", 1,2,3,4,5);
	kerr = _NXAudioStreamInfo(audioStream, &bytes_processed, 
				&time_stamp);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_stream_msg: streamInfo returns %d\n", 
	    		kerr, 2,3,4,5);
	audio_snd_reply_ret_samples(out_msg, in_msg->msg_remote_port,
			      bytes_processed, time_stamp);
	return 0;
    }

    /*
     * NOTE: snddriver_client.c sends options and play/record data
     * in the same message.
     */
    size = in_msg->msg_size - sizeof(*stream_msg);
    msg_type = (snd_msg_type_t *)(stream_msg + 1);
    while (size > 0) {
	switch (msg_type->type) {
	  case SND_MT_PLAY_DATA: {
#ifdef DDM_DEBUG
	      snd_stream_play_data_t *stream_play_data =
		  (snd_stream_play_data_t *)msg_type;
#endif DDM_DEBUG
	      
	      *(int *)&msg_type += sizeof(snd_stream_play_data_t);
	      size -= sizeof(snd_stream_play_data_t);
	      
	      if (!error && direction == SND_DIR_RECORD)
		  error = SND_BAD_PARM;
#ifdef DDM_DEBUG
	      options = stream_play_data->options;
	      snd_data = (vm_address_t)stream_play_data->data;
	      snd_size = (vm_size_t)
		  stream_play_data->dataType.msg_type_long_number;
#endif DDM_DEBUG
	      xpr_audio_user("AU: snd_stream_msg: play data=0x%x, size=%d\n", 
	      	snd_data, snd_size, 3,4,5);
	      direction = SND_DIR_PLAY;
	      break;
	  }
	  case SND_MT_RECORD_DATA: {
	      int r;
	      //char *filename;
	      int filename_len;
	      snd_stream_record_data_t *record_data =
		  (snd_stream_record_data_t *)msg_type;

	      filename_len = record_data->filenameType.msg_type_number;
	      r = sizeof(*record_data) + filename_len;
	      *(int *)&msg_type += r;
	      size -= r;
	      xpr_audio_user("AU: snd_stream_msg: record data\n", 1,2,3,4,5);
	      break;
	  }
	  case SND_MT_CONTROL: {
	      snd_stream_control_t *stream_control =
		  (snd_stream_control_t *)msg_type;

	      xpr_audio_user("AU: snd_stream_msg: control\n", 1,2,3,4,5);
	      *(int *)&msg_type += sizeof(snd_stream_control_t);
	      size -= sizeof(snd_stream_control_t);

	      xpr_audio_user("AU: snd_stream_msg: control 0x%x\n",
			     stream_control->snd_control, 2,3,4,5);
	      snd_control |= stream_control->snd_control;
	      break;
	  }
	  case SND_MT_FORMAT: {
	      snd_stream_format_t *stream_format =
		  (snd_stream_format_t *)msg_type;
	      *(int *)&msg_type += sizeof(snd_stream_format_t);
	      size -= sizeof(snd_stream_format_t);
	      
	      sample_rate = stream_format->rate;
	      data_encoding = stream_format->encoding;
	      chan_count = stream_format->chan_count;
	      set_format = TRUE;
	      xpr_audio_user("AU: snd_stream_msg: format sr=%d de=%d cc=%d\n",
			     stream_format->rate,
			     stream_format->encoding,
			     stream_format->chan_count, 4,5);
	      break;
	  }
	  case SND_MT_OPTIONS: {
	      snd_stream_options_t *stream_options =
		  (snd_stream_options_t *)msg_type;
	      xpr_audio_user("AU: snd_stream_msg: options hw=%d lw=%d "
	      		     "size=%d\n",
			     stream_options->high_water,
			     stream_options->low_water,
			     stream_options->dma_size, 4,5);
	      *(int *)&msg_type += sizeof(snd_stream_options_t);
	      size -= sizeof(snd_stream_options_t);
	      /*
	       * Save water marks for later use by play/record stream call.
	       */
	      high_water = stream_options->high_water;
	      low_water = stream_options->low_water;
	      
	      kerr = _NXAudioGetBufferOptions([audioStream channel],
					      &dma_size, &dma_count);
	      if (kerr != KERN_SUCCESS)
		  xpr_audio_user("AU: snd_stream_msg: getBufferOpts returns "
		  		"%d\n", kerr, 2,3,4,5);
	      kerr = _NXAudioSetBufferOptions([audioStream channel], 0,
					      stream_options->dma_size,
					      dma_count);
	      if (kerr != KERN_SUCCESS)
		  xpr_audio_user("AU: snd_stream_msg: setBufferOpts returns "
		  		"%d\n", kerr, 2,3,4,5);
	      break;
	  }
	  case SND_MT_NDMA: {
	      snd_stream_ndma_t *stream_ndma = (snd_stream_ndma_t *)msg_type;
	      xpr_audio_user("AU: snd_stream_msg: ndma %d\n", 
	      		     stream_ndma->ndma, 2,3,4,5);
	      *(int *)&msg_type += sizeof(snd_stream_ndma_t);
	      size -= sizeof(snd_stream_ndma_t);
	      kerr = _NXAudioGetBufferOptions([audioStream channel],
					      &dma_size, &dma_count);
	      if (kerr != KERN_SUCCESS)
		  xpr_audio_user("AU: snd_stream_msg: getBufferOpts returns "
		  		"%d\n",kerr, 2,3,4,5);
	      kerr = _NXAudioSetBufferOptions([audioStream channel], 0,
					      dma_size,
					      stream_ndma->ndma);
	      if (kerr != KERN_SUCCESS)
		  xpr_audio_user("AU: snd_stream_msg: setBufferOpts returns "
		  		"%d\n", kerr, 2,3,4,5);
	      break;
	  }
	  default:
	    error = SND_BAD_MSG;
	    size = 0;
	    break;
	}
    }
       
    /*
     * If we found any errors, cleanup and return an error.
     */
    if (error) {
	size = in_msg->msg_size - sizeof(*stream_msg);
	msg_type = (snd_msg_type_t *)(stream_msg + 1);
	while (size > 0) {
	    switch (msg_type->type) {
	      case SND_MT_PLAY_DATA: {
		  snd_stream_play_data_t *stream_play_data =
		      (snd_stream_play_data_t *)msg_type;
		  
		  *(int *)&msg_type +=
		      sizeof(snd_stream_play_data_t);
		  size -= sizeof(snd_stream_play_data_t);
		  
		  vm_deallocate(IOVmTaskSelf(),
				(vm_address_t)stream_play_data->data,
				stream_play_data->
				dataType.msg_type_long_number);
		  break;
	      }
	      case SND_MT_RECORD_DATA:
		*(int *)&msg_type +=
		    sizeof(snd_stream_record_data_t);
		size -= sizeof(snd_stream_record_data_t);
		break;
	      case SND_MT_CONTROL:
		*(int *)&msg_type +=
		    sizeof(snd_stream_control_t);
		size -= sizeof(snd_stream_control_t);
		break;
	      case SND_MT_FORMAT:
		*(int *)&msg_type +=
		    sizeof(snd_stream_format_t);
		size -= sizeof(snd_stream_format_t);
		break;
	      case SND_MT_OPTIONS:
		*(int *)&msg_type +=
		    sizeof(snd_stream_options_t);
		size -= sizeof(snd_stream_options_t);
		break;
	      case SND_MT_NDMA:
		*(int *)&msg_type +=
		    sizeof(snd_stream_ndma_t);
		size -= sizeof(snd_stream_ndma_t);
		break;
	      default:
		/*
		 * This shouldn't happen.
		 */
		break;
	    }
	}
	return error;
    }
    
    /*
     * Process control requests.
     */
    timerclear(&time_now);

    if (snd_control&SND_DC_ABORT) {
	kerr = _NXAudioStreamControl(audioStream, _NXAUDIO_STREAM_ABORT,
				    time_now);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_stream_msg: streamControl returns %d\n", 
	    		   kerr,2,3,4,5);
    }
    if (snd_control&SND_DC_AWAIT) {
	kerr = _NXAudioStreamControl(audioStream, _NXAUDIO_STREAM_RETURN_DATA,
				    time_now);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_stream_msg: streamControl returns %d\n", 
	    		   kerr,2,3,4,5);
    }
    if (snd_control&SND_DC_PAUSE) {
	kerr = _NXAudioStreamControl(audioStream, _NXAUDIO_STREAM_PAUSE,
				    time_now);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_stream_msg: streamControl returns %d\n", 
	    	           kerr, 2,3,4,5);
    }
    /*
     * Resume control handled below after (possible) data enqueue.
     */

    /*
     * This time for sure.
     */
    size = in_msg->msg_size - sizeof(*stream_msg);
    msg_type = (snd_msg_type_t *)(stream_msg + 1);
    while (size > 0) {
	snd_size = 0;
	switch (msg_type->type) {
	  case SND_MT_PLAY_DATA: {
	      snd_stream_play_data_t *stream_play_data =
		  (snd_stream_play_data_t *)msg_type;
	      
	      *(int *)&msg_type += sizeof(snd_stream_play_data_t);
	      size -= sizeof(snd_stream_play_data_t);
	      
	      options = stream_play_data->options;
	      snd_data = (vm_address_t)stream_play_data->data;
	      snd_size = (vm_size_t)
		  stream_play_data->dataType.msg_type_long_number;
	      reply_port = stream_play_data->reg_port;
	      break;
	  }
	  case SND_MT_RECORD_DATA: {
	      snd_stream_record_data_t *record_data =
		  (snd_stream_record_data_t *)msg_type;

	      *(int *)&msg_type += sizeof(*record_data);
	      size -= sizeof(*record_data);

	      options = record_data->options;
	      snd_size = (vm_size_t)record_data->nbytes;
	      /*
	       * Note: record data is allocated in audio_server.
	       */
	      reply_port = record_data->reg_port;
	      break;
	  }
	  case SND_MT_CONTROL:
	    *(int *)&msg_type += sizeof(snd_stream_control_t);
	    size -= sizeof(snd_stream_control_t);
	    break;
	  case SND_MT_FORMAT:
	    *(int *)&msg_type += sizeof(snd_stream_format_t);
	    size -= sizeof(snd_stream_format_t);
	    break;
	  case SND_MT_OPTIONS:
	    *(int *)&msg_type += sizeof(snd_stream_options_t);
	    size -= sizeof(snd_stream_options_t);
	    break;
	  case SND_MT_NDMA:
	    *(int *)&msg_type += sizeof(snd_stream_ndma_t);
	    size -= sizeof(snd_stream_ndma_t);
	    break;
	  default:
	    /*
	     * This shouldn't happen.
	     */
	    break;
	}

	/*
	 * Enqueue the region.
	 */
	if (snd_size) {
	    u_int messages = 0;

	    if ((options&SND_DM_STARTED_MSG) != 0)
		messages |= _NXAUDIO_MSG_STARTED;
	    /*
	     * Old recording code cannot handle completed msg.
	     */
	    if (direction == SND_DIR_PLAY &&
		(options&SND_DM_COMPLETED_MSG) != 0)
		messages |= _NXAUDIO_MSG_COMPLETED;
	    if ((options&SND_DM_PAUSED_MSG) != 0)
		messages |= _NXAUDIO_MSG_PAUSED;
	    if ((options&SND_DM_RESUMED_MSG) != 0)
		messages |= _NXAUDIO_MSG_RESUMED;
	    if ((options&SND_DM_ABORTED_MSG) != 0)
		messages |= _NXAUDIO_MSG_ABORTED;
	    if ((options&SND_DM_OVERFLOW_MSG) != 0)
		messages |= _NXAUDIO_MSG_UNDERRUN;
	    
	    if (direction == SND_DIR_PLAY) {
		if (set_format) {
		    snd_set_stream_format(direction, audioStream,
					  sample_rate,
					  data_encoding,
					  chan_count,
					  low_water, high_water);
		    kerr = _NXAudioPlayStreamData((OutputStream *)audioStream,
						  snd_data,
						  snd_size, data_tag,
						  reply_port, messages);
		    if (kerr != KERN_SUCCESS) {
			xpr_audio_user("AU: snd_stream_msg: playStreamD "
				       "returns %d\n", kerr, 2,3,4,5);
		    }
		} else {
		    if ([audioStream type] == _NXAUDIO_STREAM_TYPE_SND_22)
			sample_rate = _NXAUDIO_RATE_22050;
		    else
			sample_rate = _NXAUDIO_RATE_44100;
		    kerr = _NXAudioPlayStream((OutputStream *)audioStream,
					      snd_data,
					      snd_size,
					      data_tag, 2, sample_rate,
					      32768, 32768,	/* gains */
					      low_water, high_water,
					      reply_port, messages);
		    if (kerr != KERN_SUCCESS) {
			/* audio_server deallocs data if error */
			xpr_audio_user("AU: snd_stream_msg: playStream returns"
				       " %d\n",kerr, 2,3,4,5);
		    }
		}
		/*
		 * Data has been enqueued. Send a message to the client
		 * telling that it is okay to enqueue more data. 
		 */
	    } else {
		if (set_format) {
		    snd_set_stream_format(direction, audioStream,
					  sample_rate,
					  data_encoding,
					  chan_count,
					  low_water, high_water);
		    kerr = _NXAudioRecordStreamData((InputStream *)audioStream,
						    snd_size, data_tag,
						    reply_port, messages);
		    if (kerr != KERN_SUCCESS) {
			xpr_audio_user("AU: snd_stream_msg: recStreamD returns"
				       " %d\n",kerr, 2,3,4,5);
		    }
		} else {
		    kerr = _NXAudioRecordStream((InputStream *)audioStream,
						snd_size,
						data_tag,
						low_water, high_water,
						reply_port, messages);
		    if (kerr != KERN_SUCCESS)
			xpr_audio_user("AU: snd_stream_msg: recordStream "
					"returns %d\n", kerr, 2,3,4,5);
		}
	    }
	}
    }
       
    if (snd_control&SND_DC_RESUME) {
	kerr = _NXAudioStreamControl(audioStream, _NXAUDIO_STREAM_RESUME,
				    time_now);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_stream_msg: streamControl returns %d\n",
	    		   kerr, 2,3,4,5);
    }

    return (error ? error : SND_NO_ERROR);
}

static void snd_get_stream_formats(void *dev, int *rates,
				   int *low_rate, int *high_rate,
				   int *encodings, int *chans)
{
    int parms[_NXAUDIO_PARAM_MAX];
    int count, continuous, i;
    kern_return_t kerr;

    kerr = _NXAudioGetSamplingRates(dev, &continuous, low_rate,
				    high_rate, parms, &count);
    if (kerr != KERN_SUCCESS)
	xpr_audio_user("AU: snd_get_stream_formats: getSamplingRates returns "
		       "%d\n", kerr, 2,3,4,5);

    *rates = 0;
    if (continuous)
	*rates |= SND_STREAM_FORMAT_RATE_CONTINUOUS;
    for (i = 0; i < count; i++) {
	if (parms[i] >= 8000 && parms[i] <= 8013)
	    *rates |= SND_STREAM_FORMAT_RATE_8000;
        else switch (parms[i]) {
	  case 11025:
	    *rates |= SND_STREAM_FORMAT_RATE_11025;
	    break;
	  case 16000:
	    *rates |= SND_STREAM_FORMAT_RATE_16000;
	    break;
	  case 22050:
	    *rates |= SND_STREAM_FORMAT_RATE_22050;
	    break;
	  case 32000:
	    *rates |= SND_STREAM_FORMAT_RATE_32000;
	    break;
	  case 44100:
	    *rates |= SND_STREAM_FORMAT_RATE_44100;
	    break;
	  case 48000:
	    *rates |= SND_STREAM_FORMAT_RATE_48000;
	    break;
	  default:
	    break;
	}
    }

    kerr = _NXAudioGetDataEncodings(dev, parms, &count);
    if (kerr != KERN_SUCCESS)
	xpr_audio_user("AU: snd_get_stream_formats: getDataEncodings returns "
		       "%d\n", kerr,2,3,4,5);

    *encodings = 0;
    for (i = 0; i < count; i++)
	switch (parms[i]) {
	  case NX_SoundStreamDataEncoding_Mulaw8:
	    *encodings |= SND_STREAM_FORMAT_ENCODING_MULAW_8;
	    break;
	  case NX_SoundStreamDataEncoding_Linear8:
	    *encodings |= SND_STREAM_FORMAT_ENCODING_LINEAR_8;
	    break;
	  case NX_SoundStreamDataEncoding_Linear16:
	    *encodings |= SND_STREAM_FORMAT_ENCODING_LINEAR_16;
	    break;
	  default:
	    break;
	}
    kerr = _NXAudioGetChannelCountLimit(dev, chans);
    if (kerr != KERN_SUCCESS)
	xpr_audio_user("AU: snd_get_stream_formats: getChanCount returns %d\n", 
			kerr, 2,3,4,5);
}

/*
 * Handle device port messages.
 */
static int snd_device_msg(msg_header_t *in_msg, msg_header_t *out_msg)
{
    int ret = SND_NO_ERROR;
    snd_get_stream_t *get_stream;
    snd_set_parms_t *set_parms;
    snd_get_volume_t *get_volume;
    snd_set_volume_t *set_volume;
    snd_set_owner_t *set_owner;
    snd_get_stream_formats_t *get_formats;
    snd_reset_owner_t *reset_owner;
    snd_new_device_t *new_device;
    u_int new_parms = 0, parms = 0;
    u_int vol;
    int lv, rv;
    int rates, low, high, encodings, chans;
    kern_return_t kerr;
    id audioChan;
    port_t new_stream, new_port;
    int stream_type;
    struct timeval time_now;
    AudioStream *audioStream;

    timerclear(&time_now);

    /*
     * The return values mean:
     * SND_NO_ERROR - no error but need a reply msg sent
     * 0 - no error, and no reply needed (we sent a reply)
     * other - error, send a reply message
     */

    switch (in_msg->msg_id) {
      case SND_MSG_GET_STREAM:
	if (in_msg->msg_size != sizeof(snd_get_stream_t))
	    return SND_BAD_PARM;
	get_stream = (snd_get_stream_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: get_stream 0x%x\n", 
			get_stream->stream,2,3,4,5);

	if (get_stream->stream == SND_GD_SIN) {
	    audioChan =
		[IOAudio _inputChannelForSndPort:in_msg->msg_local_port];
	    xpr_audio_user("AU: snd_dev_msg: input chan 0x%x\n", audioChan, 
	    			2,3,4,5);
	    if (!(new_stream =
		  [audioChan streamUserForOwnerPort:get_stream->owner])) {
		kerr = _NXAudioAddStream(audioChan, &new_stream,
					 get_stream->owner, 0, /* id */
					 _NXAUDIO_STREAM_TYPE_SND);
		if (kerr != KERN_SUCCESS)
		    xpr_audio_user("AU: snd_dev_msg: addStream returns %d\n", 
		    			kerr, 2,3,4,5);
	    }
	} else {
	    if (get_stream->stream == SND_GD_SOUT_22)
		stream_type = _NXAUDIO_STREAM_TYPE_SND_22;
	    else
		stream_type = _NXAUDIO_STREAM_TYPE_SND_44;
	    audioChan =
		[IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	    xpr_audio_user("AU: snd_dev_msg: output chan 0x%x\n", audioChan, 
	    			2,3,4,5);
	    if (!(new_stream =
		 [audioChan streamUserForOwnerPort:get_stream->owner])) {
		kerr = _NXAudioAddStream(audioChan, &new_stream,
					 get_stream->owner, 0, /* id */
					 stream_type);
		if (kerr != KERN_SUCCESS)
		    xpr_audio_user("AU: snd_dev_msg: addStream returns %d\n", 
		    			kerr,2,3,4,5);
	    }
	}
	audio_snd_reply_ret_stream(out_msg, in_msg->msg_remote_port, new_stream);
	ret = 0;
	break;
      case SND_MSG_SET_PARMS:
	xpr_audio_user("AU: snd_dev_msg: set_parms\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_set_parms_t))
	    return SND_BAD_PARM;
	set_parms = (snd_set_parms_t *)in_msg;

	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	kerr = _NXAudioGetSndoutOptions(audioChan, &new_parms);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: getSndoutOptions returns %d\n", 
	    			kerr, 2,3,4,5);
	if (set_parms->parms&SND_PARM_ZEROFILL)
	    new_parms |= _NXAUDIO_SNDOUT_ZEROFILL;
	else
	    new_parms &= ~_NXAUDIO_SNDOUT_ZEROFILL;
	if (set_parms->parms&SND_PARM_SPEAKER)
	    new_parms |= _NXAUDIO_SNDOUT_SPEAKER_ON;	
	else
	    new_parms &= ~_NXAUDIO_SNDOUT_SPEAKER_ON;
	if (set_parms->parms&SND_PARM_LOWPASS)
	    new_parms |= _NXAUDIO_SNDOUT_DEEMPHASIS;
	else
	    new_parms &= ~_NXAUDIO_SNDOUT_DEEMPHASIS;

	kerr = _NXAudioSetSndoutOptions(audioChan, 0, new_parms);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: setSndoutOptions returns %d\n",
	    			 kerr, 2,3,4,5);
	break;
      case SND_MSG_GET_PARMS:
	xpr_audio_user("AU: snd_dev_msg: get_parms\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_get_parms_t))
	    return SND_BAD_PARM;

	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	kerr = _NXAudioGetSndoutOptions(audioChan, &new_parms);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: getSndoutOptions returns %d\n", 
	    			kerr,2,3,4,5);
	if (new_parms&_NXAUDIO_SNDOUT_ZEROFILL)
	    parms |= SND_PARM_ZEROFILL;
	if (new_parms&_NXAUDIO_SNDOUT_SPEAKER_ON)
	    parms |= SND_PARM_SPEAKER;
	if (new_parms&_NXAUDIO_SNDOUT_DEEMPHASIS)
	    parms |= SND_PARM_LOWPASS;

	audio_snd_reply_ret_parms(out_msg, in_msg->msg_remote_port, parms);
	ret = 0;
	break;
      case SND_MSG_SET_VOLUME:
	xpr_audio_user("AU: snd_dev_msg: set_vol\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_set_volume_t))
	    return SND_BAD_PARM;
	set_volume = (snd_set_volume_t *)in_msg;

	vol = set_volume->volume;
	lv = ((vol&SND_VOLUME_LCHAN_MASK) >>SND_VOLUME_LCHAN_SHIFT);
	rv = ((vol&SND_VOLUME_RCHAN_MASK) >>SND_VOLUME_RCHAN_SHIFT);
	xpr_audio_user("AU: snd_dev_msg: set_vol: %d %d\n", lv, rv, 3,4,5);
	/*
	 * Convert from range [1,43] to [-84,0].
	 */
	lv = ((lv-1)*2)-84;
	rv = ((rv-1)*2)-84;
	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	kerr = _NXAudioSetSpeaker(audioChan, 0, lv, rv);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: setSpeaker returns %d\n", kerr,
			   2,3,4,5);
	break;
      case SND_MSG_GET_VOLUME:
	xpr_audio_user("AU: snd_dev_msg: get_vol\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_get_volume_t))
	    return SND_BAD_PARM;
	get_volume = (snd_get_volume_t *)in_msg;

	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	kerr = _NXAudioGetSpeaker(audioChan, &lv, &rv);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: getSpeaker returns %d\n", kerr,
			   2,3,4,5);
	/*
	 * Convert from range [-84,0] to [1,43].
	 */
	lv = 43 + (lv/2);
	rv = 43 + (rv/2);
	vol = (lv<<SND_VOLUME_LCHAN_SHIFT) | (rv<<SND_VOLUME_RCHAN_SHIFT);

	xpr_audio_user("AU: snd_dev_msg: get_vol: %d %d\n", lv, rv, 3,4,5);
	audio_snd_reply_ret_volume(out_msg, in_msg->msg_remote_port, vol);
	ret = 0;
	break;
	/* New for 3.1 */
      case SND_MSG_GET_SNDOUT_FORMATS:
	xpr_audio_user("AU: snd_dev_msg: get_sndout_formats\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_get_stream_formats_t))
	    return SND_BAD_PARM;
	get_formats = (snd_get_stream_formats_t *)in_msg;
	  
	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	snd_get_stream_formats(audioChan, &rates, &low, &high,
			       &encodings, &chans);
	audio_snd_reply_ret_formats(out_msg, in_msg->msg_remote_port,
			      rates, low, high,
			      encodings, chans);
	ret = 0;
	break;
      case SND_MSG_GET_SNDIN_FORMATS:
	xpr_audio_user("AU: snd_dev_msg: get_sndin_formats\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_get_stream_formats_t))
	    return SND_BAD_PARM;
	get_formats = (snd_get_stream_formats_t *)in_msg;
	  
	audioChan = [IOAudio _inputChannelForSndPort:in_msg->msg_local_port];
	snd_get_stream_formats(audioChan, &rates, &low, &high,
			       &encodings, &chans);
	audio_snd_reply_ret_formats(out_msg, in_msg->msg_remote_port,
			      rates, low, high,
			      encodings, chans);
	ret = 0;
	break;
      case SND_MSG_SET_RAMP:
	xpr_audio_user("AU: snd_dev_msg: set_ramp\n", 1,2,3,4,5);
	if (in_msg->msg_size != sizeof(snd_set_parms_t))
	    return SND_BAD_PARM;
	set_parms = (snd_set_parms_t *)in_msg;

	audioChan = [IOAudio _outputChannelForSndPort:in_msg->msg_local_port];
	kerr = _NXAudioGetSndoutOptions(audioChan, &new_parms);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: getSndoutOptions returns %d\n", 
	    		kerr, 2,3,4,5);
	if (set_parms->parms&SND_PARM_RAMPUP)
	    new_parms |= _NXAUDIO_SNDOUT_RAMPUP;
	else
	    new_parms &= ~_NXAUDIO_SNDOUT_RAMPUP;
	if (set_parms->parms&SND_PARM_RAMPDOWN)
	    new_parms |= _NXAUDIO_SNDOUT_RAMPDOWN;
	else
	    new_parms &= ~_NXAUDIO_SNDOUT_RAMPDOWN;

	kerr = _NXAudioSetSndoutOptions(audioChan, 0, new_parms);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: setSndoutOptions returns %d\n", 
	    		kerr,2,3,4,5);
	break;
      case SND_MSG_SET_DSPOWNER:
	xpr_audio_user("AU: snd_dev_msg: set_dspowner\n", 1,2,3,4,5);
	/*
	 * DSP is never available.
	 */
	return SND_SEARCH;
	break;
      case SND_MSG_SET_SNDINOWNER:
	if (in_msg->msg_size != sizeof(snd_set_owner_t))
	    return SND_BAD_PARM;
	set_owner = (snd_set_owner_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: set_sndinowner 0x%x\n", 
			set_owner->owner,2,3,4,5);
	/*
	 * Abort (reset) stream if asserting same owner port.
	 */
	if ((audioStream =
	     [AudioChannel streamForOwnerPort:set_owner->owner])) {
	    kerr = _NXAudioStreamControl(audioStream,
					 _NXAUDIO_STREAM_ABORT, time_now);
	    if (kerr != KERN_SUCCESS)
		xpr_audio_user("AU: snd_dev_msg: streamControl returns %d\n", 
				kerr,2,3,4,5);
	}
	break;
      case SND_MSG_SET_SNDOUTOWNER:
	if (in_msg->msg_size != sizeof(snd_set_owner_t))
	    return SND_BAD_PARM;
	set_owner = (snd_set_owner_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: set_sndoutowner 0x%x\n", 
			set_owner->owner, 2,3,4,5);
	/*
	 * Abort (reset) stream if asserting same owner port.
	 */
	if ((audioStream =
	     [AudioChannel streamForOwnerPort:set_owner->owner])) {
	    kerr = _NXAudioStreamControl(audioStream,
					 _NXAUDIO_STREAM_ABORT, time_now);
	    if (kerr != KERN_SUCCESS)
		xpr_audio_user("AU: snd_dev_msg: streamControl returns %d\n", 
				kerr, 2,3,4,5);
	}
	break;
      case SND_MSG_NEW_DEVICE_PORT:
	/*
	 * This is only marginally useful now, since the NXSound device
	 * ports do NOT get reset by this message.
	 */
	if (in_msg->msg_size != sizeof(snd_new_device_t))
	    return SND_BAD_PARM;
	new_device = (snd_new_device_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: new_device_port\n", 1,2,3,4,5);
	/*
	 * Note: input and output chans share the same user msg device port.
	 */
	audioChan = [IOAudio _outputChannelForSndPort: in_msg->msg_local_port];
	new_port = audio_reset_snd_dev_port([audioChan audioDevice],
					    new_device->priv);
	if (new_port == PORT_NULL)
	    return SND_BAD_HOST_PRIV;

	[audioChan removeSndStreams];
	audioChan = [IOAudio _inputChannelForSndPort: in_msg->msg_local_port];
	[audioChan removeSndStreams];

	audio_snd_reply_ret_device(out_msg, in_msg->msg_remote_port, new_port);
	ret = 0;
	break;
      case SND_MSG_RESET_SNDINOWNER:
	if (in_msg->msg_size != sizeof(snd_reset_owner_t))
	    return SND_BAD_PARM;
	reset_owner = (snd_reset_owner_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: reset_sndinowner 0x%x\n",
		       reset_owner->old_owner, 2,3,4,5);
	audioStream = [AudioChannel streamForOwnerPort:reset_owner->old_owner];
	if (!audioStream)
	    return SND_NOT_OWNER;
	kerr = _NXAudioRemoveStream(audioStream);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: removeStream returns %d\n", kerr,
			   2,3,4,5);
	break;
      case SND_MSG_RESET_SNDOUTOWNER:
	if (in_msg->msg_size != sizeof(snd_reset_owner_t))
	    return SND_BAD_PARM;
	reset_owner = (snd_reset_owner_t *)in_msg;
	xpr_audio_user("AU: snd_dev_msg: reset_sndoutowner 0x%x\n",
		       reset_owner->old_owner, 2,3,4,5);
	audioStream = [AudioChannel streamForOwnerPort:reset_owner->old_owner];
	if (!audioStream)
	    return SND_NOT_OWNER;
	kerr = _NXAudioRemoveStream(audioStream);
	if (kerr != KERN_SUCCESS)
	    xpr_audio_user("AU: snd_dev_msg: removeStream returns %d\n", kerr,
			   2,3,4,5);
	break;
      case SND_MSG_DSP_PROTO:
      case SND_MSG_GET_DSP_CMD_PORT:
      case SND_MSG_RESET_DSPOWNER:
	xpr_audio_user("AU: snd_dev_msg: dsp msg\n", 1,2,3,4,5);
	/*
	 * DSP is never available.
	 */
	return SND_SEARCH;
	break;
      default:
	/* impossible */
	break;
    }
    return ret;
}

/*
 * Handle in_msg and setup out_msg for reply.
 */
boolean_t snd_server(msg_header_t *in_msg, msg_header_t *out_msg)
{
    boolean_t ret = TRUE;
    int reply_val = 0;

    /*
     * Standard reply header.
     */
    out_msg->msg_simple = TRUE;
    out_msg->msg_size = sizeof(msg_header_t);
    out_msg->msg_type = MSG_TYPE_NORMAL;
    out_msg->msg_local_port = PORT_NULL;
    out_msg->msg_remote_port = PORT_NULL;
    out_msg->msg_id = 0;

    if (in_msg->msg_id >= SND_MSG_STREAM_BASE &&
	in_msg->msg_id <= SND_MSG_STREAM_NSAMPLES)
	reply_val = snd_stream_msg(in_msg, out_msg);
    else if (in_msg->msg_id >= SND_MSG_DEVICE_BASE &&
	     in_msg->msg_id <= SND_MSG_GET_SNDIN_FORMATS)
	reply_val = snd_device_msg(in_msg, out_msg);
    else if (in_msg->msg_id >= SND_MSG_DSP_BASE &&
	     in_msg->msg_id <= SND_MSG_DSP_REQ_ERR) {
	/*
	 * Should not happen since you can't set a dsp owner port.
	 */
	log_error(("Audio: received dsp cmd port msg!\n"));
	audio_snd_reply_illegal_msg(out_msg, PORT_NULL, in_msg->msg_remote_port,
			      in_msg->msg_id, SND_BAD_MSG);
	ret = FALSE;
    } else {
	audio_snd_reply_illegal_msg(out_msg, PORT_NULL, in_msg->msg_remote_port,
			      in_msg->msg_id, SND_BAD_MSG);
	ret = FALSE;
    }
    /*
     * Send a reply if msg handler routine didn't.
     */
    if (reply_val)
	audio_snd_reply_illegal_msg(out_msg, PORT_NULL, in_msg->msg_remote_port,
			      in_msg->msg_id, reply_val);
    return ret;
}
