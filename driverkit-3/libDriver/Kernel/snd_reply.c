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
 * audio_snd_reply.c
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Snd reply message routines.
 *
 * HISTORY
 *      08/07/92/mtm    Original coding from m68k snd_reply.c.
 */

#import "snd_reply.h"
#import <bsd/dev/snd_msgs.h>

/*
 * Interface routines for composing messages to send in response to
 * sound facility services.
 */

static msg_type_long_t snd_type_ool_template = {
  {
      0,	/* msg_type_name */
      0,	/* msg_type_size */
      0,	/* msg_type_number */
      FALSE, 	/* msg_type_inline */
      TRUE,     /* msg_type_longform */
      TRUE	/* msg_type_deallocate */
  },
  MSG_TYPE_INTEGER_8,	/* msg_type_long_name */
  8,			/* msg_type_long_size */
  0,			/* msg_type_long_number */
};

static msg_type_t snd_type_int_template = {
    MSG_TYPE_INTEGER_32,	/* msg_type_name */
    32,				/* msg_type_size */
    1,				/* msg_type_number */
    TRUE,			/* msg_type_inline */
    FALSE,			/* msg_type_longform */
    FALSE			/* msg_type_deallocate */
};

/*
 * Generic replies
 */
/*
 * Message to return the device port.
 */
void audio_snd_reply_ret_device(msg_header_t *msg,
			  port_name_t remote_port,
			  port_name_t device_port)
{
    msg->msg_id = SND_MSG_RET_DEVICE;
    msg->msg_local_port = device_port;
    msg->msg_remote_port = remote_port;
}

/*
 * Message to return a stream port.
 */
void audio_snd_reply_ret_stream(msg_header_t *msg,
			  port_name_t remote_port,
			  port_name_t stream_port)
{
    msg->msg_id = SND_MSG_RET_STREAM;
    msg->msg_local_port = stream_port;
    msg->msg_remote_port = remote_port;
}

static void snd_reply_with_tag(msg_header_t *msg, port_name_t port,
			       int tag, int msg_id)
{
    snd_taged_reply_t *smsg = (snd_taged_reply_t *)msg;

    msg->msg_id = msg_id;
    msg->msg_size = sizeof(snd_taged_reply_t);
    msg->msg_remote_port = port;
    smsg->data_tagType = snd_type_int_template;
    smsg->data_tag = tag;
}

/*
 * Message return on illegal message.
 */
void audio_snd_reply_illegal_msg(msg_header_t *msg,
			   port_name_t local_port,
			   port_name_t remote_port,
			   int msg_id,
			   int error)
{
    snd_illegal_msg_t *smsg = (snd_illegal_msg_t *)msg;
    
    msg->msg_id = SND_MSG_ILLEGAL_MSG;
    msg->msg_size = sizeof(snd_illegal_msg_t);
    msg->msg_local_port = local_port;
    msg->msg_remote_port = remote_port;
    
    smsg->dataType = snd_type_int_template;
    smsg->dataType.msg_type_number = 2;
    smsg->ill_msgid = msg_id;
    smsg->ill_error = error;
}

/*
 * Replies specific to streaming soundout/soundin
 */
/*
 * Message returning recorded data.
 */
void audio_snd_reply_recorded_data(msg_header_t *msg,
			     port_name_t remote_port,
			     int data_tag,
			     pointer_t data,
			     int nbytes)
{
    snd_recorded_data_t *smsg = (snd_recorded_data_t *)msg;
    
    msg->msg_simple = FALSE;
    msg->msg_size = sizeof(snd_recorded_data_t);
    msg->msg_type = MSG_TYPE_NORMAL;
    msg->msg_remote_port = remote_port;
    msg->msg_local_port = PORT_NULL;
    msg->msg_id = SND_MSG_RECORDED_DATA;
    
    smsg->data_tagType = snd_type_int_template;
    smsg->data_tag = data_tag;
    smsg->dataType = snd_type_ool_template;
    smsg->dataType.msg_type_long_number = nbytes;
    smsg->recorded_data = data;
}

/*
 * Message to return a timeout indication.
 */
void audio_snd_reply_timed_out(msg_header_t *msg,
			 port_name_t remote_port,
			 int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_TIMED_OUT);
}

/*
 * Message to return an integer count of number samples played/recorded.
 */
void audio_snd_reply_ret_samples(msg_header_t *msg,
			   port_name_t remote_port,
			   int nsamples, int timeStamp)
{
    snd_ret_samples_t *smsg = (snd_ret_samples_t *)msg;

    msg->msg_size = sizeof(snd_ret_samples_t);
    msg->msg_id = SND_MSG_RET_SAMPLES;
    msg->msg_remote_port = remote_port;
    
    smsg->dataType = snd_type_int_template;
    smsg->nsamples = nsamples;
    smsg->timeStamp = timeStamp;
}

/*
 * Message to return an overflow indication.
 */
void audio_snd_reply_overflow(msg_header_t *msg,
			port_name_t remote_port,
			int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_OVERFLOW);
}

/*
 * Message to return a region started indication.
 */
void audio_snd_reply_started(msg_header_t *msg,
		       port_name_t remote_port,
		       int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_STARTED);
}

/*
 * Message to return region completed indication.
 */
void audio_snd_reply_completed(msg_header_t *msg,
			 port_name_t remote_port,
			 int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_COMPLETED);
}

/*
 * Message to return region aborted indication.
 */
void audio_snd_reply_aborted(msg_header_t *msg,
		       port_name_t remote_port,
		       int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_ABORTED);
}

/*
 * Message to return region paused indication.
 */
void audio_snd_reply_paused(msg_header_t *msg,
		      port_name_t remote_port,
		      int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_PAUSED);
}

/*
 * Message to return region resumed indication.
 */
void audio_snd_reply_resumed(msg_header_t *msg,
		       port_name_t remote_port,
		       int data_tag)
{
    return snd_reply_with_tag(msg, remote_port, data_tag,
			      SND_MSG_RESUMED);
}

/*
 * Replies specific to the sound device.
 */
/*
 * Message to return the set of parameters.
 */
void audio_snd_reply_ret_parms(msg_header_t *msg,
			 port_name_t remote_port,
			 u_int parms)
{
    snd_ret_parms_t *smsg = (snd_ret_parms_t *)msg;
	
    msg->msg_id = SND_MSG_RET_PARMS;
    msg->msg_size = sizeof(snd_ret_parms_t);
    msg->msg_remote_port = remote_port;

    smsg->dataType = snd_type_int_template;
    smsg->parms = parms;
}

/*
 * Message to return the volume.
 */
void audio_snd_reply_ret_volume(msg_header_t *msg,
			  port_name_t remote_port,
			  u_int volume)
{
    snd_ret_volume_t *smsg = (snd_ret_volume_t *)msg;
	
    msg->msg_id = SND_MSG_RET_VOLUME;
    msg->msg_size = sizeof(snd_ret_volume_t);
    msg->msg_remote_port = remote_port;

    smsg->dataType = snd_type_int_template;
    smsg->volume = volume;
}

/*
 * Message to return supported data formats.
 */
void audio_snd_reply_ret_formats(msg_header_t *msg,
			   port_name_t remote_port,
			   u_int rates, u_int low, u_int high,
			   u_int encodings, u_int chans)
{
    snd_ret_stream_formats_t *smsg = (snd_ret_stream_formats_t *)msg;
	
    msg->msg_id = SND_MSG_RET_STREAM_FORMATS;
    msg->msg_size = sizeof(snd_ret_stream_formats_t);
    msg->msg_remote_port = remote_port;

    smsg->Type = snd_type_int_template;
    smsg->Type.msg_type_number = 5;
    smsg->rate = rates;
    smsg->low_rate = low;
    smsg->high_rate = high;
    smsg->encoding = encodings;
    smsg->chan_count = chans;
}
