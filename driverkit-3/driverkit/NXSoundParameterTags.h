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
 * NXSoundParameterTags.h
 *
 * Copyright (c) 1993, NeXT Computer, Inc.  All rights reserved. 
 */

#define NX_SoundDeviceParameterKeyBase		0
#define NX_SoundDeviceParameterValueBase	200
#define NX_SoundStreamParameterKeyBase		400
#define NX_SoundStreamParameterValueBase	600
#define NX_SoundParameterTagMax			799

typedef enum _NXSoundParameterTag {
    NX_SoundDeviceBufferSize = NX_SoundDeviceParameterKeyBase,
    NX_SoundDeviceBufferCount,
    NX_SoundDeviceDetectPeaks,
    NX_SoundDeviceRampUp,
    NX_SoundDeviceRampDown,
    NX_SoundDeviceInsertZeros,
    NX_SoundDeviceDeemphasize,
    NX_SoundDeviceMuteSpeaker,
    NX_SoundDeviceMuteHeadphone,
    NX_SoundDeviceMuteLineOut,
    NX_SoundDeviceOutputLoudness,
    NX_SoundDeviceOutputAttenuationStereo,
    NX_SoundDeviceOutputAttenuationLeft,
    NX_SoundDeviceOutputAttenuationRight,
    NX_SoundDeviceAnalogInputSource,
    NX_SoundDeviceMonitorAttenuation,
    NX_SoundDeviceInputGainStereo,
    NX_SoundDeviceInputGainLeft,
    NX_SoundDeviceInputGainRight,

    NX_SoundDeviceAnalogInputSource_Microphone
	= NX_SoundDeviceParameterValueBase,
    NX_SoundDeviceAnalogInputSource_LineIn,

    NX_SoundStreamDataEncoding = NX_SoundStreamParameterKeyBase,
    NX_SoundStreamSamplingRate,
    NX_SoundStreamChannelCount,
    NX_SoundStreamHighWaterMark,
    NX_SoundStreamLowWaterMark,
    NX_SoundStreamSource,
    NX_SoundStreamSink,
    NX_SoundStreamDetectPeaks,
    NX_SoundStreamGainStereo,
    NX_SoundStreamGainLeft,
    NX_SoundStreamGainRight,

    NX_SoundStreamDataEncoding_Linear16 = NX_SoundStreamParameterValueBase,
    NX_SoundStreamDataEncoding_Linear8,
    NX_SoundStreamDataEncoding_Mulaw8,
    NX_SoundStreamDataEncoding_Alaw8,
    NX_SoundStreamDataEncoding_AES,
    NX_SoundStreamSource_Analog,
    NX_SoundStreamSource_AES,
    NX_SoundStreamSink_Analog,
    NX_SoundStreamSink_AES
} NXSoundParameterTag;
