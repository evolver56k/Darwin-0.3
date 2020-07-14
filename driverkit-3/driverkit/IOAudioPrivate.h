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
#import <bsd/machine/limits.h>		// UINT_MAX for timeout
#import <driverkit/IOAudio.h>
#import <kernserv/clock_timer.h>

#define	IOAUDIO_MAXIMUM_TIMEOUT		UINT_MAX

typedef struct {
    ns_time_t 	_outputStartTime;
    ns_time_t 	_timestampBuffer;
    /* input and output settings */
    BOOL 	_enableMicIn;
    BOOL 	_enableLineIn;
    BOOL 	_enableCDIn;
    BOOL 	_enableAux1In;
    BOOL 	_enableAux2In;
    BOOL 	_enableSpeakerOut;
    BOOL 	_enableLineOut;
    BOOL 	_enableCDOut;
    BOOL 	_enableAux1Out;
    BOOL 	_enableAux2Out;
}       _audioPrivateData;

/*
 * IO thread command message id's.
 */
#define	AD_CMD_MSG_BASE			900
#define AD_CMD_MSG_OUTPUT_PENDING	(AD_CMD_MSG_BASE+0)
#define AD_CMD_MSG_INPUT_PENDING	(AD_CMD_MSG_BASE+1)
#define AD_CMD_MSG_SYNCHRONOUS		(AD_CMD_MSG_BASE+2)

@interface IOAudio (Private)

+ _addChannel:channel;
+ _channelForUserPort: (port_t) port;
+ _channelForExclusivePort: (port_t) port;
+ _inputChannelForSndPort: (port_t) port;
+ _outputChannelForSndPort: (port_t) port;

+ (void) _setInstance: anObject;	
+ _instance;

- _inputChannel;
- _outputChannel;
- (BOOL) _channelWillAddStream;
- _audioCommand;
- (port_set_name_t) _devicePortSet;

- (void) _setSampleRate: (unsigned int) rate;

- (void) _setChannelCount: (unsigned int) count;
- (void) _setDataEncoding: (unsigned int) encoding;
- (void) _interruptOccurred;

- (void) _dataPendingForChannel: (id) channel;
- (void) _dataPendingOccurred: (id) channel;

- (void) _setTimeout: (unsigned int) milliseconds;
- (unsigned int) _timeout;

- (BOOL) _attemptToStartDMAForChannel: (id) channel
                        channelStatus: (BOOL) isChannelActive;

- (BOOL) _is22KRateSupported;

- (void) _stopDMAForChannel: (id) channel;
					       
- (BOOL) _attemptToStopDMAForChannel: (id) channel;

- (void) _setInputGainLeft: (unsigned int) gain;
- (void) _setInputGainRight: (unsigned int) gain;
- (void) _setOutputAttenuationLeft: (int) attenuation;
- (void) _setOutputAttenuationRight: (int) attenuation;

- (void)_setLoudnessEnhanced: (BOOL) flag;
- (void)_setOutputMute: (BOOL) flag;

- (void) _initAudioHardwareSettings;

- (void) _commandOccurred;

- (void) _keyOccurred: (int) key
                           event: (int) direction
			    flags: (int) flags;

- (void) _setInputActive: (BOOL) isActive;
- (void) _setOutputActive: (BOOL) isActive;


- (int) _intValueForParameter: (NXSoundParameterTag) ptag
                                 forObject: anObject;

- (BOOL) _setParameter: (NXSoundParameterTag) ptag
                               toInt: (int)value
                        forObject: anObject;

- (BOOL) _setParameters: (const NXSoundParameterTag *) plist
                          toValues: (const unsigned int *) vlist
                               count: (unsigned int) numParameters
                         forObject: anObject;
    
- (void) _getParameters: (const NXSoundParameterTag *) plist
                           values: (unsigned int *) vlist
                            count: (unsigned int) numParameters
                      forObject: anObject;
    
- (void) _getSupportedParameters: (NXSoundParameterTag *) list
                           count: (unsigned int *) numParameters
                       forObject: anObject;
    
- (BOOL) _getValues: (NXSoundParameterTag *) list
                        count: (unsigned int *) numValues
            forParameter: (NXSoundParameterTag) ptag
                  forObject: anObject;

// Timing information

- (void)_setLastInterruptTimeStamp:(ns_time_t)startTime;
- (ns_time_t)_lastInterruptTimeStamp;

- (void)_setOutputStartTime:(ns_time_t)startTime;
- (ns_time_t)_outputStartTime;

- (void) _setInputFor:(NXSoundParameterTag)ptag to:(BOOL)enable;
- (void) _setOutputFor:(NXSoundParameterTag)ptag to:(BOOL)enable;

// convenience methods
- (NXSoundParameterTag)_analogInputSource;
- (void)_setAnalogInputSource:(NXSoundParameterTag)ptag;


// Private hooks for NeXTTime.

// The DMA buffer does not exist until the first stream is activated, and
// it goes away when the last stream is deactivated.  The app should do a
// setExclusiveUse:YES, then activate a stream and not use it.

- (void) _getOutputChannelBuffer:(vm_address_t *)addr
                            size:(unsigned int *)byteCount;

// YES starts output DMA, NO stops it.  XXXX This should be one method.

- (void) _getInputChannelBuffer:(vm_address_t *)addr
                            size:(unsigned int *)byteCount;

// YES starts output DMA, NO stops it.  XXXX This should be one method.

- (void) _runExclusiveOutputDMA:(BOOL)flag;

// YES starts input DMA, NO stops it.

- (void) _runExclusiveInputDMA:(BOOL)flag;

// Progress is in bytes.  It gets reset to 0 when _runExclusiveDMA:YES
// is called.

- (unsigned long) _exclusiveProgress;

@end
