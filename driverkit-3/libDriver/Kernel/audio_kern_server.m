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
 * audio_kern_server.m
 *
 * Copyright (c) 1993, NeXT Computer, Inc.  All rights reserved.
 *
 */

#import <mach/mach_types.h>
#import <kern/lock.h>
#import <kernserv/kern_server_types.h>
#import <kernserv/prototypes.h>
#import <kernserv/kern_server_reply.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/generalFuncs.h>
#import "audio_server.h"
#import "snd_server.h"
#import "audio_kern_server.h"

/*
 * definitions for audioMessages
 */
#define SERVER_SEND_OPTIONS	SEND_TIMEOUT
#define SERVER_SEND_TIMEOUT	1000

/*
 * for IOAudio instance
 */
#import <driverkit/IOAudioPrivate.h>

/*
 * imported for handleControl
 */
#import <audio/audio_msgs.h>
#import "portFuncs.h"

/*
 * imported for snd_server
 */
#import <bsd/dev/snd_msgs.h>
#import "snd_reply.h"

#import <mach/mig_errors.h>			// death_pill_t

/*
 * declaration for createAudioPorts()
 */
void audioMessages(msg_header_t *in_msg, msg_header_t *out_msg);

/*
 * these are initialized at load time but not used until after
 * the device is probed.
 */
port_t outPort = PORT_NULL, inPort = PORT_NULL, sndPort = PORT_NULL;

static kern_server_t	*instance;	/* kernserv instance */

/*
 * Allocate user message listen ports.
 */
static void createAudioPorts()
{
    kern_return_t	krtn;
    
    /*
     * Kern_server passes the last argument of kern_serv_port_serv()
     * as the first argument to the MIG-generated interface
     * functions (in audio_server.m).  In this case, it is the
     * port the message was received on.
     */
    
    outPort = allocatePort();
    krtn = kern_serv_port_serv(instance, outPort,
			       (port_map_proc_t) audioMessages, outPort);
    if (krtn != KERN_SUCCESS) 
    	IOLog("Audio: createAudioPorts error %d\n", krtn);
    
    inPort = allocatePort();
    krtn = kern_serv_port_serv(instance, inPort,
			       (port_map_proc_t) audioMessages, inPort);
    if (krtn != KERN_SUCCESS) 
    	IOLog("Audio: createAudioPorts error %d\n", krtn);
    
    sndPort = allocatePort();
    krtn = kern_serv_port_serv(instance, sndPort,
			       (port_map_proc_t) audioMessages, sndPort);
    if (krtn != KERN_SUCCESS) 
    	IOLog("Audio: createAudioPorts error %d\n", krtn);
}

/*
 * Called when an audio driver instance is initialized.
 */
void audioKernServInit(kern_server_t *instance_var)
{
#ifdef DEBUG
    IOLog("Audio server loaded, inst=0x%x\n", (unsigned int)instance_var);
#endif DEBUG

    instance = instance_var;
}


/*
 * Register stream port - exported for Audio.
 */
boolean_t audio_enroll_stream_port(port_t stream_port, boolean_t enroll)
{
    kern_return_t	krtn;
    
    if (enroll) {
	krtn = kern_serv_port_serv(instance, stream_port,
	    (port_map_proc_t) audioMessages, stream_port);
	if (krtn != KERN_SUCCESS)
	    IOLog("Audio: kern_serv_port_serv returns %d\n", krtn);
    } else {
        kern_serv_port_gone(instance, stream_port);
	krtn = KERN_SUCCESS;
    }

    return (krtn == KERN_SUCCESS ? TRUE : FALSE);
}

/*
 * Realloc just the snd device port - exported for snd_server.m.
 */
port_t audio_reset_snd_dev_port(IOAudio *dev, port_t priv_port)
{
    port_t kern_port, sndPort;

    if ((kern_port = IOHostPrivSelf()) == PORT_NULL) {
	IOLog("Audio: cannot get kernel port (must run as root)\n");
	IOLog("reset_snd_dev_port");
    }
    if (priv_port != kern_port)
	return PORT_NULL;

    /* snd input and output share the same user port */
    deallocatePort([[dev _outputChannel] userSndPort]);
    sndPort = allocatePort();
    [[dev _inputChannel] setUserSndPort:sndPort];
    [[dev _outputChannel] setUserSndPort:sndPort];

    return sndPort;
}

/*
 * Handle privileged control message.
 */
static boolean_t handleControl(msg_header_t *in_msg,
				msg_header_t *out_msg)
{
    audio_get_ports_t *get_ports = (audio_get_ports_t *)in_msg;
    audio_ret_ports_t *ret_ports = (audio_ret_ports_t *)out_msg;
    port_t kern_port;
    
    IOAudio		*dev = [IOAudio _instance];
    static boolean_t	initialized = FALSE;

    /*
     * Standard reply header.
     */
    out_msg->msg_simple = TRUE;
    out_msg->msg_size = sizeof(msg_header_t);
    out_msg->msg_type = MSG_TYPE_NORMAL;
    out_msg->msg_local_port = PORT_NULL;
    out_msg->msg_remote_port = in_msg->msg_remote_port;
    out_msg->msg_id = 0;

    if (in_msg->msg_id != AUDIO_MSG_GET_PORTS)
	return FALSE;

    /*
     * If the IOAudio instance fails to load, the control ports are
     * never returned. The kernel server instance variable is guaranteed to
     * be set when an audio device is successfully instantiated.
     */
    if (dev == nil)
        return TRUE;

    if (! initialized) {
	kern_serv_port_death_proc(instance,
	    (port_death_proc_t)audio_port_gone);
	createAudioPorts();
	initialized = TRUE;
    }
	
    out_msg->msg_simple = FALSE;
    out_msg->msg_size = sizeof(audio_ret_ports_t);
    out_msg->msg_id = AUDIO_MSG_RET_PORTS;
    ret_ports->portsType.msg_type_name = MSG_TYPE_PORT;
    ret_ports->portsType.msg_type_size = 32;
    ret_ports->portsType.msg_type_number = 3;
    ret_ports->portsType.msg_type_inline = TRUE;
    ret_ports->portsType.msg_type_longform = FALSE;
    ret_ports->portsType.msg_type_deallocate = FALSE;
    ret_ports->portsType.msg_type_unused = 0;

    if ((kern_port = IOHostPrivSelf()) == PORT_NULL) {
	IOLog("Audio: cannot get kernel port (must run as root)\n");
	return TRUE;
    }
    
    if (get_ports->priv != kern_port) {
    
#ifdef DEBUG
        IOLog("handleControl: returning NULL ports\n");
#endif DEBUG

	ret_ports->inputPort = PORT_NULL;
	ret_ports->outputPort = PORT_NULL;
	ret_ports->sndPort = PORT_NULL;
    } else {
	if (get_ports->reset) {
	
#ifdef DEBUG
	    IOLog("handleControl: resetting ports\n");
#endif DEBUG

	    if (inPort != PORT_NULL) {
		kern_serv_port_gone(instance, inPort);
		deallocatePort(inPort);
	    }
	    if (outPort != PORT_NULL) {
		kern_serv_port_gone(instance, outPort);
		deallocatePort(outPort);
	    }
	    /* snd input and output share the same user port */
	    if (sndPort != PORT_NULL) {
		kern_serv_port_gone(instance, sndPort);
		deallocatePort(sndPort);
	    }
	    createAudioPorts();
	}
	
	ret_ports->inputPort = inPort;
	ret_ports->outputPort = outPort;
	ret_ports->sndPort = sndPort;
	
#ifdef DEBUG
	IOLog("handleControl: returning ports %d %d %d\n",
	      ret_ports->inputPort, ret_ports->outputPort,
	      ret_ports->sndPort);
#endif DEBUG
    }
    
    [[dev _outputChannel] setUserChannelPort:outPort];
    [[dev _outputChannel] setUserSndPort:sndPort];
    [[dev _inputChannel] setUserChannelPort:inPort];
    [[dev _inputChannel] setUserSndPort:sndPort];

    return TRUE;
}

/*
 * Called when messages arrive on registered ports
 */
void audioMessages(msg_header_t *in_msg, msg_header_t *out_msg)
{
    kern_return_t krtn;
    
#ifdef DEBUG
//    IOLog("Audio: message received id: %d\n", in_msg->msg_id);
#endif DEBUG

    if (in_msg->msg_local_port == 0) {
	if (! (handleControl(in_msg, out_msg)))
	    IOLog("Audio: unrecognized control message %d\n",
		  in_msg->msg_id);
    } else if (in_msg->msg_id < AUDIO_USER_MSG_BASE) {
	/*
	 * Handle old style SoundDSP driver message.
	 */
	if (!snd_server(in_msg, out_msg))
	    IOLog("Audio: unrecognized snd user message %d\n",
			in_msg->msg_id);
    } else {
	/*
	 * Handle new style NXSoundDevice message.
	 */
	if (!audio_server(in_msg, out_msg))
	    IOLog("Audio: unrecognized audio user message %d\n",
			in_msg->msg_id);
    }
    /*
     * Send a reply message, even if error.
     */
    krtn = msg_send(out_msg, SERVER_SEND_OPTIONS, SERVER_SEND_TIMEOUT);
    if (krtn != KERN_SUCCESS) {
	IOLog("msg_send failed %d\n", krtn);
    }
    
    /*
     * Under normal circumstances, kernserv sends the reply message. In this 
     * case, both the control and snd interfaces are not generated by MiG. We
     * must send the response and then set MIG_NO_REPLY to prevent kernserv
     * from sending a response on our behalf.
     */
    ((death_pill_t *)out_msg)->RetCode = MIG_NO_REPLY;
}
