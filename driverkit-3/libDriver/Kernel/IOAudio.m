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
 * IOAudio.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 */
 
#import <mach/mach_user_internal.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/IODeviceDescription.h>
#if defined (ppc)
#import "kernserv/ppc/spl.h"
#elif defined (i386)
#import "kernserv/i386/spl.h"
#else
#error architecture not supported
#endif


#if defined(hppa)
#import <machdep/machine/pmap.h>
#endif

#import <driverkit/IOAudio.h>
#import <driverkit/IOAudioPrivate.h>
#import <kernserv/prototypes.h>		// for msg_send_from_kernel
#import "AudioChannel.h"

#import "InputStream.h"
#import "OutputStream.h"

#import "AudioCommand.h"

#import "portFuncs.h"
#import "audio_types.h"
#import "audio_kern_server.h"
#import "audio_mulaw.h"
#import "audioLog.h"			// For XPRs

#ifdef hppa
#warning interrupt hack for hppa on
id my_audioid = 0;
// The below map is used in files AudioChannel.m, HPAdvancedAudio.m apart from here
pmap_t audio_pmap;
extern aspitab();
#endif

static void ioThread(IOAudio *audioDevice);
static void keyThread(IOAudio *audioDevice);

/*
 * FIXME: These tags should be incorporated in NXSoundParameterTags. 
 */
typedef enum {
    NX_SoundDeviceLineOut = NX_SoundDeviceParameterKeyBase + 25,
    NX_SoundDeviceSpeakerOut,
    NX_SoundDeviceCDOut,
    NX_SoundDeviceAux1Out,
    NX_SoundDeviceAux2Out,
        
    NX_SoundDeviceMicIn,
    NX_SoundDeviceLineIn,
    NX_SoundDeviceCDIn,
    NX_SoundDeviceAux1In,
    NX_SoundDeviceAux2In,
} NXSoundParameterTagExtra;

/*
 * NeXTTime relies on +_instance, so
 * these can be factory variables.
 */
static BOOL runningExclusive = NO;
static unsigned long exclusiveProgress = 0;
static unsigned int exclusiveIncrement = 0;
static IOAudioInterruptClearFunc exclusiveInterruptClearFunc = 0;

/*
 * Driver makefiles generate source with this interface
 * that goes into each reloc.
 */
@interface Object (AudioDriverPrivate)
+ (kern_server_t *)kernelServerInstance;
@end

/*
 * Since EventDriver.h cannot be imported, the objc_lookUpClass interface is
 * used to locate the EventDriver class. This definition eliminates warning
 * messages from the compiler.
 */
@interface Object (EventDriver)
+ instance;
			
- (IOReturn) setSpecialKeyPort: (port_t)dev_port
			          keyFlavor: (int)special_key
				      keyPort: (port_t)key_port;

- (port_t)ev_port;
@end

/*
 * imports for EventDriver
 */
#import <bsd/dev/ev_keymap.h>
#import <bsd/dev/evio.h>
#import <objc/objc-runtime.h>	// objc_lookUpClass

/*
 * Currently we store the timestamp for only the last interrupt. In future we
 * might store them for last n interrupts. This will allow us to know if any
 * interrupts have been dropped. 
 */

static ns_time_t	_lastInterruptTime;

#if defined(hppa) || defined(sparc)
interruptHandler(void *identity, void *state)
#else	hppa
static void
interruptHandler(void *identity, void *state, unsigned int arg)
#endif	hppa
{
    if (runningExclusive) {
	exclusiveProgress += exclusiveIncrement;
	if (exclusiveInterruptClearFunc) {
	    (*exclusiveInterruptClearFunc)();
	    return;
	}
    }
    
    IOGetTimestamp(&_lastInterruptTime);
    
    xpr_audio_device("AD: interruptHandler\n", 1,2,3,4,5);
    
    /* Forward this to the I/O thread for further handling. */
    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
}

@implementation IOAudio (Private)

/* Factory variables */
static id _channelList = nil;
static id _deviceInstance = nil;


/*
 * Add a channel.
 * Note: channels are added at driver init time (main.m)
 * and never removed.  Therefore channel list locking is not necessary.
 */
+ _addChannel:channel
{
    if (!_channelList)
	_channelList = [[List alloc] init];
    [_channelList addObject:channel];
    return self;
}

/*
 * Search for a channel given a user port.
 */
+ _channelForUserPort:(port_t)port
{
    int i;
    id channel;

    for (i = 0; i < [_channelList count]; i++) {
	channel = [_channelList objectAt:i];
	if ([channel userChannelPort] == port)
	    return channel;
    }
    return nil;
}

/*
 * Search for a channel given an exclusive owner port.
 */
+ _channelForExclusivePort:(port_t)port
{
    int i;
    id channel;

    for (i = 0; i < [_channelList count]; i++) {
	channel = [_channelList objectAt:i];
	if ([channel exclusiveUser] == port)
	    return channel;
    }
    return nil;
}

/*
 * Search for a input channel given an snd user port.
 */
+ _inputChannelForSndPort:(port_t)port
{
    int i;
    id channel;

    for (i = 0; i < [_channelList count]; i++) {
	channel = [_channelList objectAt:i];
	if (channel == [[channel audioDevice] _inputChannel] && [channel userSndPort] == port)
	    return channel;
    }
    return nil;
}

/*
 * Search for a output channel given an snd user port.
 */
+ _outputChannelForSndPort:(port_t)port
{
    int i;
    id channel;

    for (i = 0; i < [_channelList count]; i++) {
	channel = [_channelList objectAt:i];
	if (channel == [[channel audioDevice] _outputChannel] && [channel userSndPort] == port)
	    return channel;
    }
    return nil;
}

+ (void) _setInstance:anObject
{
    _deviceInstance = anObject;
}

+ _instance
{
    return _deviceInstance;
}

- _inputChannel
{
    return _inputChannel;
}
- _outputChannel
{
    return _outputChannel;
}

/*
 * Subclass can override to detect when a stream is added to a channel.
 * Returning NO prevents the stream from being added.
 */
- (BOOL) _channelWillAddStream
{
    return YES;
}

/*
 * The audioCommand allows channels to send synchronous commands to the
 * device.
 */
- _audioCommand
{
    return _audioCommand;
}

/*
 * 
 */
- (void) _setSampleRate: (unsigned int) rate
{
    _sampleRate = rate;
}

- (void) _setDataEncoding:(unsigned int)encoding
{
    _dataEncoding = encoding;
}

/*
 *
 */
- (void) _setChannelCount:(unsigned int)count
{
    _channelCount = count;
}

- (void) _dataPendingForChannel: (id)channel
{
    kern_return_t krtn;

    xpr_audio_device("AD: _dataPendingForChannel\n",1,2,3,4,5);
    
   if (!_dataPendingMessage) {
	_dataPendingMessage = (msg_header_t *)IOMalloc(sizeof(msg_header_t));
	_dataPendingMessage->msg_simple = TRUE;
	_dataPendingMessage->msg_size = sizeof(msg_header_t);
	_dataPendingMessage->msg_type = MSG_TYPE_NORMAL;
	_dataPendingMessage->msg_local_port = PORT_NULL;
	_dataPendingMessage->msg_remote_port = _commandPort;
    }

	if ([channel isRead])
	    _dataPendingMessage->msg_id = AD_CMD_MSG_INPUT_PENDING;
	else
	    _dataPendingMessage->msg_id = AD_CMD_MSG_OUTPUT_PENDING;

   krtn = msg_send_from_kernel(_dataPendingMessage, SEND_TIMEOUT, 1000);
    if (krtn != KERN_SUCCESS && krtn != SEND_TIMED_OUT)
	IOLog("Audio: data pending msg_send error: %d\n", krtn);
}


/*
 * The ioThread executes this method when a pending message arrives on the
 * command port. The IOAudio subclass will ask each channel to see if
 * stream requests are pending. When requests are pending, the device asks
 * the channel to make a dma buffer. If this operation is successful, the
 * device initiates DMA for the channel. The implementation of this method
 * is dependent on the number of DMA channels supported by the device. For
 * example, when one channel is supported, DMA occurs in one direction until
 * it is complete, then DMA is started in the other direction.
 *
 * See <newClasses/IOAudio.m> for a version which will support simultaneous
 * transfer when 2 independent DMA channels are available.
 */

- (void) _dataPendingOccurred: (id) channel
{
    if ([self isInputActive] || [self isOutputActive])
        return;
	    
    (void) [self _attemptToStartDMAForChannel: channel channelStatus: FALSE];
}

/*
 * The ioThread executes this method when an interrupt message arrives on
 * the interrupt port.
 */
- (void) _interruptOccurred
{
    BOOL	serviceInputChannel = FALSE;
    BOOL	serviceOutputChannel = FALSE;
    
    if (!runningExclusive ||
	(runningExclusive && !exclusiveInterruptClearFunc))
	[self interruptOccurredForInput: &serviceInputChannel
                              forOutput: &serviceOutputChannel];
    
    /*
     * Private, exclusive dma mode.
     */
    if (runningExclusive)
	return;

    xpr_audio_device("AD: _interruptOccurred\n",1,2,3,4,5);
    
    /*
     * spurious interrupt
     */
     if (!serviceInputChannel && !serviceOutputChannel)
         return;

    [self _setLastInterruptTimeStamp:_lastInterruptTime];
        
    if (serviceInputChannel)
        (void)[self _attemptToStopDMAForChannel: [self _inputChannel]];
	
    if (serviceOutputChannel)
        (void)[self _attemptToStopDMAForChannel: [self _outputChannel]];
	
    xpr_audio_device("AD: interrupt serviced\n",1,2,3,4,5);
}

/*
 * The ioThread executes this method when a command message arrives on the
 * command port.
 */
- (void) _commandOccurred
{
    int		command = [[self _audioCommand] command];
    int		result = 1;
    
    switch (command) {
 
	case setDeviceInputGainLeft:
	    [self updateInputGainLeft];
	    [[self _audioCommand] done: result];
	    break;
	      
	case setDeviceInputGainRight:
	    [self updateInputGainRight];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputMute:
	    [self updateOutputMute];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputAttenuationLeft:
	    [self updateOutputAttenuationLeft];
	    [[self _audioCommand] done: result];
	    break;

	case setDeviceOutputAttenuationRight:
	    [self updateOutputAttenuationRight];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceLoudness:
	    [self updateLoudnessEnhanced];
	    [[self _audioCommand] done: result];
	    break;
	    
	case abortInputChannel:
	    if ([self isInputActive])
		[self _stopDMAForChannel: [self _inputChannel]];
	    [[self _audioCommand] done: result];
	    break;
	    
	case abortOutputChannel:
	    if ([self isOutputActive])
		[self _stopDMAForChannel: [self _outputChannel]];
	    [[self _audioCommand] done: result];
	    break;

	case setDeviceInputMicEnable:
	    [self setInput:NX_SoundDeviceMicIn enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputMicDisable:
	    [self setInput:NX_SoundDeviceMicIn enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputLineEnable:
	    [self setInput:NX_SoundDeviceLineIn enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputLineDisable:
	    [self setInput:NX_SoundDeviceLineIn enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputCDEnable:
	    [self setInput:NX_SoundDeviceCDIn enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputCDDisable:
	    [self setInput:NX_SoundDeviceCDIn enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputAux1Enable:
	    [self setInput:NX_SoundDeviceAux1In enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputAux1Disable:
	    [self setInput:NX_SoundDeviceAux1In enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputAux2Enable:
	    [self setInput:NX_SoundDeviceAux2In enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceInputAux2Disable:
	    [self setInput:NX_SoundDeviceAux2In enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputLineEnable:
	    [self setOutput:NX_SoundDeviceLineOut enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputLineDisable:
	    [self setOutput:NX_SoundDeviceLineOut enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputSpeakerEnable:
	    [self setOutput:NX_SoundDeviceSpeakerOut enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputSpeakerDisable:
	    [self setOutput:NX_SoundDeviceSpeakerOut enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputCDEnable:
	    [self setOutput:NX_SoundDeviceCDOut enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputCDDisable:
	    [self setOutput:NX_SoundDeviceCDOut enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputAux1Enable:
	    [self setOutput:NX_SoundDeviceAux1Out enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputAux1Disable:
	    [self setOutput:NX_SoundDeviceAux1Out enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputAux2Enable:
	    [self setOutput:NX_SoundDeviceAux2Out enable: YES];
	    [[self _audioCommand] done: result];
	    break;
	    
	case setDeviceOutputAux2Disable:
	    [self setOutput:NX_SoundDeviceAux2Out enable: NO];
	    [[self _audioCommand] done: result];
	    break;
	    
	default:
	    // error message?
	    [[self _audioCommand] done: result];
	    break;
    }	
}

/*
 *
 */
- (void) _setTimeout: (unsigned int)milliseconds
{
    _timeout = milliseconds;
}

/*
 *
 */
- (unsigned int) _timeout
{
    return _timeout;
}

- (port_set_name_t) _devicePortSet
{
    return _devicePortSet;
}

- (void) _setLastInterruptTimeStamp: (ns_time_t)startTime
{
    ((_audioPrivateData *) _audioPrivate)->_timestampBuffer =
	_lastInterruptTime;
}

- (ns_time_t) _lastInterruptTimeStamp
{
    return ((_audioPrivateData *)_audioPrivate)->_timestampBuffer;
}

- (void) _setOutputStartTime: (ns_time_t)startTime
{
    ((_audioPrivateData *)_audioPrivate)->_outputStartTime = startTime;
}

- (ns_time_t) _outputStartTime
{
    return  ((_audioPrivateData *)_audioPrivate)->_outputStartTime;
}

#if	hppa
static int audio_first =0;
#endif	hppa

- (BOOL) _attemptToStartDMAForChannel: (id)channel channelStatus:(BOOL) isChannelActive
{
    int			i;
    /*
     * When isChannelActive is NO, the streams are allowed to change the
     * sample rate, data format, and channel count 
     */
     int		rate = 0;
     IOAudioDataFormat	format = IOAudioDataFormatUnset;
     unsigned int	count = 0;
     BOOL		didStart = NO;
     ns_time_t		startTime;
#if defined(hppa) || defined(sparc)
    vm_offset_t		addr;
#endif

     if ( [self _is22KRateSupported] == NO )
     {
         rate = 44100;
     } 

     if (isChannelActive) {
         rate	= [self sampleRate];
	 //format	= [self dataFormat];
	 format = _dataEncoding;			// encoding FIXME
	 count	= [self channelCount];
    }

    /*
     * Enqueue only half of the needed descriptors. 
     */
    for (i = 0; i < [channel dmaCount] / 2; i++) {
	if (![channel enqueueDescriptor:&rate dataFormat:&format
	      channelCount:&count])
	    break;
    }

#if	hppa
    audio_first = 1;
#endif	hppa
        
    if ([channel enqueueCount]) {
        [self _setSampleRate: rate];
	[self _setDataEncoding: format];
	[self _setChannelCount: count];
	
#if defined(hppa) || defined(sparc)
	addr = (vm_offset_t)[channel channelBuffer];
#if hppa
	pmap_flush_range(kernel_pmap,addr, [channel enqueueCount] * 4096 );
#endif	
	didStart = [self startDMAForChannel: [channel localChannel]
		read: [channel isRead]
		buffer: (void *)addr	// beacuse kernel is 1-1 mapped
		bufferSizeForInterrupts: [channel descriptorSize]];
#else	hppa
	didStart = [self startDMAForChannel: [channel localChannel]
		read: [channel isRead]
		buffer: [channel channelBuffer]
		bufferSizeForInterrupts: [channel descriptorSize]];
#endif	hppa

        if (didStart) {
	    IOGetTimestamp(&startTime);
	    [self _setOutputStartTime: startTime];

	    if ([channel isRead])
		[self _setInputActive: YES];
	    else
		[self _setOutputActive: YES];
		    
	    if ([self _timeout] == IOAUDIO_MAXIMUM_TIMEOUT)
		// Assume worst case for interrupt timeouts (1K mono data)
		[self _setTimeout:[channel descriptorSize]];
	} else {
	    while ([channel enqueueCount])
		[channel dequeueDescriptor];
	    
	    // FIXME: The channel never ran.
	    [self stopDMAForChannel:[channel localChannel]
			    read:[channel isRead]];
	    [channel freeDescriptors];
	    [self _setTimeout: IOAUDIO_MAXIMUM_TIMEOUT];
        }
    }
	     
    return didStart;
}

- (BOOL) _attemptToStopDMAForChannel: (id)channel
{
    unsigned int rate		= [self sampleRate];
    IOAudioDataFormat format	= _dataEncoding;	// FIXME : [self dataEncoding]
    unsigned int count		= [self channelCount];
    
    xpr_audio_device("AD: _attemptToStopDMAForChannel\n",1,2,3,4,5);
    
#if	hppa
    if (audio_first) {
	    audio_first= 0;
	    return;
    }
#endif	hppa

    /*
     * We should not try to dequeue if nothing is enqueued. 
     */
    if ([channel enqueueCount])	{
	[channel dequeueDescriptor];
    } else {
	xpr_audio_device("AD: interrupt received but no descriptors enqueued!\n",1,2,3,4,5);
    }
    
    xpr_audio_device("AD: descriptors dequeued\n",1,2,3,4,5);
    
    /*
     * Checks to see if more data is available from the channel.
     */
    if (! [channel enqueueDescriptor: &rate dataFormat: &format channelCount: &count]) {
	if (! [channel enqueueCount]) {
	    [self _stopDMAForChannel: (id)channel];
	    [self _setOutputStartTime: 0];
	    [self _setLastInterruptTimeStamp:0];
		
	    xpr_audio_device("AD: no more descriptors\n",1,2,3,4,5);
	    return TRUE;
	}
    }
    xpr_audio_device("AD: new descriptors enqueued\n",1,2,3,4,5);
 
    return FALSE;
    
 }
 

- (void) _stopDMAForChannel: (id)channel
{  
    [self stopDMAForChannel: [channel localChannel] read: [channel isRead]];
    
    [channel freeDescriptors];
    
    if ([channel isRead])
	[self _setInputActive: NO];
    else
	[self _setOutputActive: NO];
	
    if (! [self isInputActive] && ! [self isOutputActive])
	[self _setTimeout: IOAUDIO_MAXIMUM_TIMEOUT];
}


- (BOOL) _is22KRateSupported
{
    int		lowRate, highRate;

    if ( [self acceptsContinuousSamplingRates] == NO )
    {
        [self getSamplingRatesLow: &lowRate high: &highRate];
        return ((lowRate > 22050) ? NO : YES);
    }
    return YES;
}

/*
 * Respond to sound key events.
 */
- (void) _keyOccurred:(int)key event:(int)direction flags:(int)flags
{
    BOOL up = NO, down = NO;
    BOOL mute = NO;
    int left, right;

    if (direction != NX_KEYDOWN)
	return;

    if (key == NX_KEYTYPE_SOUND_UP) {
	if (!(flags & NX_COMMANDMASK))
	    up = YES;
    } else if (key == NX_KEYTYPE_SOUND_DOWN) {
	if (flags & NX_COMMANDMASK)
	    mute = YES;
	else
	    down = YES;
    }
    
    if (!(flags & NX_SHIFTMASK))	{
    
	if (mute) {
	    [self _setOutputMute: ! [self isOutputMuted]];
	    return;
	}
	
	left = [self outputAttenuationLeft];
	right = [self outputAttenuationRight];
	if (up) {
	    left += 1;
	    if (left > 0)
		left = 0;
	    right += 1;
	    if (right > 0)
		right = 0;
	} else if (down) {
	    left -= 1;
	    if (left < -84)
		left = -84;
	    right -= 1;
	    if (right < -84)
		right = -84;
	}
	[self _setOutputAttenuationLeft: left];
	[self _setOutputAttenuationRight: right];
    } else {
	left = [self inputGainLeft];
	right = [self inputGainRight];
	if (up) {
	    left += 1638; 		// same as 32768/20
	    if (left >= 32768)
		left = 32768;
	    right += 1638;
	    if (right >= 32768)
		right = 32768;
	} else if (down) {
	    left -= 1638;
	    if (left <= 0)
		left = 0;
	    right -= 1638;
	    if (right <= 0)
		right = 0;
	}
	[self _setInputGainLeft: left];
	[self _setInputGainRight: right];
     }
}

- (void) _setInputGainLeft:(unsigned int)gain
{
    _inputGainLeft = gain;
    [[self _audioCommand] send: setDeviceInputGainLeft];
}

- (void) _setInputGainRight:(unsigned int)gain
{
    _inputGainRight = gain;
    [[self _audioCommand] send: setDeviceInputGainRight];
}

- (void) _setOutputMute:(BOOL)flag
{
    _isOutputMuted = flag;
    [[self _audioCommand] send: setDeviceOutputMute];
}

- (void) _setLoudnessEnhanced:(BOOL)flag
{
    _isLoudnessEnhanced = flag;
    [[self _audioCommand] send: setDeviceLoudness];
}

- (void) _setOutputAttenuationLeft:(int)attenuation
{
    _outputAttenuationLeft = attenuation;
    [[self _audioCommand] send: setDeviceOutputAttenuationLeft];
}

- (void) _setOutputAttenuationRight:(int)attenuation
{
    _outputAttenuationRight = attenuation;
    [[self _audioCommand] send: setDeviceOutputAttenuationRight];
}

- (void) _setInputActive: (BOOL) isActive
{
    _isInputActive = isActive;
}

- (void) _setOutputActive: (BOOL) isActive
{
    _isOutputActive = isActive;
}

- (void) _setInputFor:(NXSoundParameterTag)ptag to:(BOOL)enable
{
    switch (ptag) {
      case NX_SoundDeviceMicIn:
	((_audioPrivateData *) _audioPrivate)->_enableMicIn = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceInputMicEnable : setDeviceInputMicDisable];
	break;
      case NX_SoundDeviceLineIn:
	((_audioPrivateData *) _audioPrivate)->_enableLineIn = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceInputLineEnable : setDeviceInputLineDisable];
	break;
      case NX_SoundDeviceCDIn:
	((_audioPrivateData *) _audioPrivate)->_enableCDIn = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceInputCDEnable : setDeviceInputCDDisable];
	break;
      case NX_SoundDeviceAux1In:
	((_audioPrivateData *) _audioPrivate)->_enableAux1In = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceInputAux1Enable : setDeviceInputAux1Disable];
	break;
      case NX_SoundDeviceAux2In:
	((_audioPrivateData *) _audioPrivate)->_enableAux2In = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceInputAux2Enable : setDeviceInputAux2Disable];
	break;
      default:
        IOLog("Audio: unknown input source: %d\n", (int)ptag);
	break;
    }
}

- (void) _setOutputFor:(NXSoundParameterTag)ptag to:(BOOL)enable
{
    switch (ptag) {
      case NX_SoundDeviceSpeakerOut:
	((_audioPrivateData *) _audioPrivate)->_enableSpeakerOut = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceOutputSpeakerEnable : setDeviceOutputSpeakerDisable];
	break;
      case NX_SoundDeviceLineOut:
	((_audioPrivateData *) _audioPrivate)->_enableLineOut = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceOutputLineEnable : setDeviceOutputLineDisable];
	break;
      case NX_SoundDeviceCDOut:
	((_audioPrivateData *) _audioPrivate)->_enableCDOut = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceOutputCDEnable : setDeviceOutputCDDisable];
	break;
      case NX_SoundDeviceAux1Out:
	((_audioPrivateData *) _audioPrivate)->_enableAux1Out = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceOutputAux1Enable : setDeviceOutputAux1Disable];
	break;
      case NX_SoundDeviceAux2Out:
	((_audioPrivateData *) _audioPrivate)->_enableAux2Out = enable;
	[[self _audioCommand] send: (enable) ?
	    setDeviceOutputAux2Enable : setDeviceOutputAux2Disable];
	break;
      default:
        IOLog("Audio: unknown output source: %d\n", (int)ptag);
	break;
    }
}


- (NXSoundParameterTag)_analogInputSource
{
    if (((_audioPrivateData *) _audioPrivate)->_enableMicIn)
	return NX_SoundDeviceMicIn;
    else if (((_audioPrivateData *) _audioPrivate)->_enableLineIn)
	return NX_SoundDeviceAux1In;
    else if (((_audioPrivateData *) _audioPrivate)->_enableCDIn)
	return NX_SoundDeviceAux1In;
    else if (((_audioPrivateData *) _audioPrivate)->_enableAux1In)
	return NX_SoundDeviceAux1In;
    else if (((_audioPrivateData *) _audioPrivate)->_enableAux2In)
	return NX_SoundDeviceAux2In;
    else
    	return 0;
}

- (void)_setAnalogInputSource:(NXSoundParameterTag)ptag
{
    [self _setInputFor:NX_SoundDeviceMicIn to:NO];
    [self _setInputFor:NX_SoundDeviceLineIn to:NO];
    [self _setInputFor:NX_SoundDeviceCDIn to:NO];
    [self _setInputFor:NX_SoundDeviceAux1In to:NO];
    [self _setInputFor:NX_SoundDeviceAux2In to:NO];
    [self _setInputFor:ptag to:YES];
}


/*
 * FIXME: really device-dependent
 */
- (int)_intValueForParameter: (NXSoundParameterTag)ptag
                                forObject: anObject
{
    int val = 0;

    if ([anObject isEqual: [self _inputChannel]]) {	
        switch (ptag) {
	  case NX_SoundDeviceBufferSize:
	    val = [anObject descriptorSize];
	    break;
	  case NX_SoundDeviceBufferCount:
	    val = [anObject dmaCount];
	    break;
	  case NX_SoundDeviceDetectPeaks:
	    val = (int)[anObject isDetectingPeaks];
	    break;
	  case NX_SoundDeviceAnalogInputSource:
	    val = (int)[self _analogInputSource];
	    break;
	  case NX_SoundDeviceInputGainStereo:
	    val = (int)(([self inputGainLeft] + [self inputGainRight]) / 2);
	    break;
	  case NX_SoundDeviceInputGainLeft:
	    val = (int)[self inputGainLeft];
	    break;
	  case NX_SoundDeviceInputGainRight:
	    val = (int)[self inputGainRight];
	    break;
	  case NX_SoundDeviceMicIn:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableMicIn;
	    break;
	  case NX_SoundDeviceLineIn:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableLineIn;
	    break;
	  case NX_SoundDeviceCDIn:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableCDIn;
	    break;
	  case NX_SoundDeviceAux1In:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableAux1In;
	    break;
	  case NX_SoundDeviceAux2In:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableAux2In;
	    break;
	  default:
	    break;
	}
    } else if ([anObject isEqual: [self _outputChannel]]) {
	switch (ptag) {
	  case NX_SoundDeviceBufferSize:
	    val = [anObject descriptorSize];
	    break;
	  case NX_SoundDeviceBufferCount:
	    val = [anObject dmaCount];
	    break;
	  case NX_SoundDeviceDetectPeaks:
	    val = (int)[anObject isDetectingPeaks];
	    break;
	  case NX_SoundDeviceRampUp:
	    val = NO;			// FIXME (not implemented)
	    break;
	  case NX_SoundDeviceRampDown:
	    val = NO;			// FIXME (not implemented)
	    break;
	  case NX_SoundDeviceInsertZeros:
	    val = NO;			// FIXME (not implemented)
	    break;
	  case NX_SoundDeviceDeemphasize:
	    val = NO;			// FIXME (not implemented)
	    break;
	  case NX_SoundDeviceMuteSpeaker:
	  case NX_SoundDeviceMuteHeadphone:
	  case NX_SoundDeviceMuteLineOut:
	    val = (int)[self isOutputMuted];
	    break;
	  case NX_SoundDeviceOutputLoudness:
	    val = (int)[self isLoudnessEnhanced];
	    break;
	  case NX_SoundDeviceOutputAttenuationStereo:
	    val = ([self outputAttenuationLeft] +
		   [self outputAttenuationRight]) / 2;
	    break;
	  case NX_SoundDeviceOutputAttenuationLeft:
	    val = [self outputAttenuationLeft];
	    break;
	  case NX_SoundDeviceOutputAttenuationRight:
	    val = [self outputAttenuationRight];
	    break;
	  case NX_SoundDeviceSpeakerOut:
	    val= (int)((_audioPrivateData *) _audioPrivate)->_enableSpeakerOut;
	    break;
	  case NX_SoundDeviceLineOut:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableLineOut;
	    break;
	  case NX_SoundDeviceCDOut:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableCDOut;
	    break;
	  case NX_SoundDeviceAux1Out:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableAux1Out;
	    break;
	  case NX_SoundDeviceAux2Out:
	    val = (int) ((_audioPrivateData *) _audioPrivate)->_enableAux2Out;
	    break;
	  default:
	    break;
	}
    } else if ([anObject isKindOf:[InputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    val = (int)[anObject dataEncoding];
	    break;
	  case NX_SoundStreamSamplingRate:
	    val = [anObject samplingRate];
	    break;
	  case NX_SoundStreamChannelCount:
	    val = [anObject channelCount];
	    break;
	  case NX_SoundStreamHighWaterMark:
	    val = [anObject highWaterMark];
	    break;
	  case NX_SoundStreamLowWaterMark:
	    val = [anObject lowWaterMark];
	    break;
	  case NX_SoundStreamSource:
	    val = NX_SoundStreamSource_Analog;
	    break;
	  default:
	    break;
	}
    } else if ([anObject isKindOf:[OutputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    val = (int)[anObject dataEncoding];
	    break;
	  case NX_SoundStreamSamplingRate:
	    val = [anObject samplingRate];
	    break;
	  case NX_SoundStreamChannelCount:
	    val = [anObject channelCount];
	    break;
	  case NX_SoundStreamHighWaterMark:
	    val = [anObject highWaterMark];
	    break;
	  case NX_SoundStreamLowWaterMark:
	    val = [anObject lowWaterMark];
	    break;
	  case NX_SoundStreamSink:
	    val = NX_SoundStreamSink_Analog;
	    break;
	  case NX_SoundStreamDetectPeaks:
	    val = (int)[anObject isDetectingPeaks];
	    break;
	  case NX_SoundStreamGainStereo:
	    val = ([anObject gainLeft] + [anObject gainRight]) / 2;
	    break;
	  case NX_SoundStreamGainLeft:
	    val = [anObject gainLeft];
	    break;
	  case NX_SoundStreamGainRight:
	    val = [anObject gainRight];
	    break;
	  default:
	    break;
	}
    } else {
	IOLog("Audio: unknown parameter object\n");
	return 0;
    }
    return val;
}

- (BOOL) _setParameter: (NXSoundParameterTag)ptag
                               toInt: (int)value
                       forObject: anObject
{
    BOOL ret = YES;

    if ([anObject isEqual: [self _inputChannel]]) {
	switch (ptag) {
	  case NX_SoundDeviceBufferSize:
	    /*
	     * FIXME: currently not settable
	     * [anObject setDescriptorSize:value];
	     */
	    break;
	  case NX_SoundDeviceBufferCount:
	    /*
	     * FIXME: currently not settable
	     * [anObject setDmaCount:value];
	     */
	    break;
	  case NX_SoundDeviceDetectPeaks:
	    [anObject setDetectPeaks:(BOOL)value];
	    break;
	  case NX_SoundDeviceInputGainStereo:
	    [self _setInputGainLeft:(unsigned int)value];
	    [self _setInputGainRight:(unsigned int)value];
	    break;
	  case NX_SoundDeviceInputGainLeft:
	    [self _setInputGainLeft:(unsigned int)value];
	    break;
	  case NX_SoundDeviceAnalogInputSource:
	    [self _setAnalogInputSource:(NXSoundParameterTag)value];
	    break;
	  // Enable and disable individual input sources
	  case NX_SoundDeviceMicIn:
	  case NX_SoundDeviceLineIn:
	  case NX_SoundDeviceCDIn:
	  case NX_SoundDeviceAux1In:
	  case NX_SoundDeviceAux2In:
	    [self _setInputFor:ptag to:(BOOL)value];
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else if ([anObject isEqual: [self _outputChannel]]) {
	switch (ptag) {
	  case NX_SoundDeviceBufferSize:
	    /*
	     * FIXME: currently not settable
	     * [anObject setDescriptorSize:value];
	     */
	    break;
	  case NX_SoundDeviceBufferCount:
	    /*
	     * FIXME: currently not settable
	     * [anObject setDmaCount:value];
	     */
	    break;
	  case NX_SoundDeviceDetectPeaks:
	    [anObject setDetectPeaks:(BOOL)value];
	    break;
	  case NX_SoundDeviceRampUp:
	    //[self setRampsUp:(BOOL)value];		// FIXME
	    break;
	  case NX_SoundDeviceRampDown:
	    //[self setRampsDown:(BOOL)value];		// FIXME
	    break;
	  case NX_SoundDeviceInsertZeros:
	    //[self _setInsertsZeros:(BOOL)value];	// FIXME
	    break;
	  case NX_SoundDeviceDeemphasize:
	    //[self _setDeemphasis:(BOOL)value];	// FIXME
	    break;
	  case NX_SoundDeviceMuteSpeaker:
	  case NX_SoundDeviceMuteHeadphone:
	  case NX_SoundDeviceMuteLineOut:
	    [self _setOutputMute:(BOOL)value];
	    break;
	  case NX_SoundDeviceOutputLoudness:
	    [self _setLoudnessEnhanced:(BOOL)value];
	    break;
	  case NX_SoundDeviceOutputAttenuationStereo:
	    [self _setOutputAttenuationLeft:value];
	    [self _setOutputAttenuationRight:value];
	    break;
	  case NX_SoundDeviceOutputAttenuationLeft:
	    [self _setOutputAttenuationLeft:value];
	    break;
	  case NX_SoundDeviceOutputAttenuationRight:
	    [self _setOutputAttenuationRight:value];
	    break;
	  // Enable and disable individual output sources
	  case NX_SoundDeviceSpeakerOut:
	  case NX_SoundDeviceLineOut:
	  case NX_SoundDeviceCDOut:
	  case NX_SoundDeviceAux1Out:
	  case NX_SoundDeviceAux2Out:
	    [self _setOutputFor:ptag to:(BOOL)value];
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else if ([anObject isKindOf:[InputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    [anObject setDataEncoding:(NXSoundParameterTag)value];
	    break;
	  case NX_SoundStreamSamplingRate:
	    [anObject setSamplingRate:value];
	    break;
	  case NX_SoundStreamChannelCount:
	    [anObject setChannelCount:value];
	    break;
	  case NX_SoundStreamHighWaterMark:
	    [anObject setHighWaterMark:value];
	    break;
	  case NX_SoundStreamLowWaterMark:
	    [anObject setLowWaterMark:value];
	    break;
	  case NX_SoundStreamSource:
	    if (value != NX_SoundStreamSource_Analog)
		ret = NO;
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else if ([anObject isKindOf:[OutputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    [anObject setDataEncoding:(NXSoundParameterTag)value];
	    break;
	  case NX_SoundStreamSamplingRate:
	    [anObject setSamplingRate:value];
	    break;
	  case NX_SoundStreamChannelCount:
	    [anObject setChannelCount:value];
	    break;
	  case NX_SoundStreamHighWaterMark:
	    [anObject setHighWaterMark:value];
	    break;
	  case NX_SoundStreamLowWaterMark:
	    [anObject setLowWaterMark:value];
	    break;
	  case NX_SoundStreamSink:
	    if (value != NX_SoundStreamSink_Analog)
		ret = NO;
	    break;
	  case NX_SoundStreamDetectPeaks:
	    [anObject setDetectPeaks:(BOOL)value];
	    break;
	  case NX_SoundStreamGainStereo:
	    [anObject setGainLeft:value];
	    [anObject setGainRight:value];
	    break;
	  case NX_SoundStreamGainLeft:
	    [anObject setGainLeft:value];
	    break;
	  case NX_SoundStreamGainRight:
	    [anObject setGainRight:value];
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else {
	IOLog("Audio: unknown parameter object\n");
	ret = NO;
    }

    return ret;
}


- (BOOL) _setParameters: (const NXSoundParameterTag *)plist
                          toValues: (const unsigned int *)vlist
                               count: (unsigned int)numParameters
                         forObject: anObject
{
    int i;
    BOOL ret = YES;

    for (i = 0; i < numParameters; i++)
	if (![self _setParameter:plist[i] toInt:vlist[i] forObject:anObject])
	    ret = NO;
    return ret;
}

- (void) _getParameters: (const NXSoundParameterTag *)plist
                           values: (unsigned int *)vlist
                            count: (unsigned int)numParameters
                      forObject: anObject
{
    int i;

    for (i = 0; i < numParameters; i++)
	vlist[i] = [self _intValueForParameter:plist[i] forObject:anObject];
}

- (void) _getSupportedParameters: (NXSoundParameterTag *)list
                                   count: (unsigned int *)numParameters
                                   forObject: anObject
{
    static const NXSoundParameterTag chan_out_params[] = {
	NX_SoundDeviceBufferSize,
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
	NX_SoundDeviceOutputAttenuationRight
	};
    static const NXSoundParameterTag chan_in_params[] = {
	NX_SoundDeviceBufferSize,
	NX_SoundDeviceBufferCount,
	NX_SoundDeviceDetectPeaks,
	NX_SoundDeviceAnalogInputSource,
	NX_SoundDeviceInputGainStereo,
	NX_SoundDeviceInputGainLeft,
	NX_SoundDeviceInputGainRight
	};
    static const NXSoundParameterTag stream_out_params[] = {
	NX_SoundStreamDataEncoding,
	NX_SoundStreamSamplingRate,
	NX_SoundStreamChannelCount,
	NX_SoundStreamHighWaterMark,
	NX_SoundStreamLowWaterMark,
	NX_SoundStreamSink,
	NX_SoundStreamDetectPeaks,
	NX_SoundStreamGainStereo,
	NX_SoundStreamGainLeft,
	NX_SoundStreamGainRight
	};
    static const NXSoundParameterTag stream_in_params[] = {
	NX_SoundStreamDataEncoding,
	NX_SoundStreamSamplingRate,
	NX_SoundStreamChannelCount,
	NX_SoundStreamHighWaterMark,
	NX_SoundStreamLowWaterMark,
	NX_SoundStreamSource
	};
    int i;
    const NXSoundParameterTag *plist = 0;

    *numParameters = 0;
    if ([anObject isEqual: [self _inputChannel]]) {
	plist = chan_in_params;
	*numParameters = sizeof(chan_in_params) / sizeof(NXSoundParameterTag);
    } else if ([anObject isEqual: [self _outputChannel]]) {
	plist = chan_out_params;
	*numParameters = sizeof(chan_out_params) / sizeof(NXSoundParameterTag);
    } else if ([anObject isKindOf:[InputStream class]]) {
	plist = stream_in_params;
	*numParameters = sizeof(stream_in_params) /
	    sizeof(NXSoundParameterTag);
    } else if ([anObject isKindOf:[OutputStream class]]) {
	plist = stream_out_params;
	*numParameters = sizeof(stream_out_params) /
	    sizeof(NXSoundParameterTag);
    } else {
	IOLog("Audio: unknown parameter object\n");
    }

    for (i = 0; i < *numParameters; i++)
	list[i] = plist[i];
}

- (BOOL) _getValues: (NXSoundParameterTag *)list
                        count: (unsigned int *)numValues 
            forParameter: (NXSoundParameterTag)ptag
                  forObject: anObject
{
    BOOL ret = YES;

    *numValues = 0;

    if ([anObject isEqual: [self _inputChannel]]) {
	switch (ptag) {
	  case NX_SoundDeviceAnalogInputSource:
	    *numValues = 2;
	    list[0] = NX_SoundDeviceAnalogInputSource_Microphone;
	    list[1] = NX_SoundDeviceAnalogInputSource_LineIn;
	    break;
	  default:
	    ret = _NXAUDIO_ERR_PARAMETER;
	    break;
	}
    } else if ([anObject isEqual: [self _outputChannel]]) {
	ret = NO;
    } else if ([anObject isKindOf:[InputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    /*
	     * FIXME: should match device-dependent
	     * getStreamDataEncodings.
	     */
	    *numValues = 4;
	    list[0] = NX_SoundStreamDataEncoding_Linear16;
	    list[1] = NX_SoundStreamDataEncoding_Linear8;
	    list[2] = NX_SoundStreamDataEncoding_Mulaw8;
	    list[3] = NX_SoundStreamDataEncoding_Alaw8;
	    break;
	  case NX_SoundStreamSource:
	    *numValues = 1;
	    list[0] = NX_SoundStreamSource_Analog;
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else if ([anObject isKindOf:[OutputStream class]]) {
	switch (ptag) {
	  case NX_SoundStreamDataEncoding:
	    /*
	     * FIXME: should match device-dependent
	     * getStreamDataEncodings.
	     */
	    *numValues = 4;
	    list[0] = NX_SoundStreamDataEncoding_Linear16;
	    list[1] = NX_SoundStreamDataEncoding_Linear8;
	    list[2] = NX_SoundStreamDataEncoding_Mulaw8;
	    list[3] = NX_SoundStreamDataEncoding_Alaw8;
	    break;
	  case NX_SoundStreamSink:
	    *numValues = 1;
	    list[0] = NX_SoundStreamSink_Analog;
	    break;
	  default:
	    ret = NO;
	    break;
	}
    } else {
	IOLog("Audio: unknown parameter object\n");
	ret = NO;
    }

    return ret;
}

/*
 * Initialize audio hardware with default values for gain, attenuation etc. 
 */
- (void) _initAudioHardwareSettings
{
    [self _setInputGainRight:32768/2];
    [self _setInputGainRight:32768/2];
    
    [self _setOutputAttenuationLeft:-42];
    [self _setOutputAttenuationLeft:-42];
    
    // add more generic ones here
}


// Private hooks for NeXTTime.

// The DMA buffer does not exist until the first stream is activated, and
// it goes away when the last stream is deactivated.  The app should do a
// setExclusiveUse:YES, then activate a stream and not use it.

- (void) _getOutputChannelBuffer:(vm_address_t *)addr
                            size:(unsigned int *)byteCount
{
    *addr = [_outputChannel channelBufferAddress];
    *byteCount = [_outputChannel descriptorSize] * [_outputChannel dmaCount];
}

// YES starts output DMA, NO stops it.

- (void) _getInputChannelBuffer:(vm_address_t *)addr
                            size:(unsigned int *)byteCount
{
    *addr = [_inputChannel channelBufferAddress];
    *byteCount = [_inputChannel descriptorSize] * [_inputChannel dmaCount];
}

// YES starts output DMA, NO stops it.

- (void) _runExclusiveOutputDMA:(BOOL)flag
{
#if defined(hppa) || defined(sparc)
    vm_offset_t		addr;
#endif
    if (flag == runningExclusive)
	return;

    if (flag) {
	exclusiveProgress = 0;
	exclusiveIncrement = [_outputChannel descriptorSize];
	exclusiveInterruptClearFunc = [self interruptClearFunc];
#if  defined(hppa) || defined(sparc)
        addr = (vm_offset_t)[_outputChannel channelBuffer];
	[self startDMAForChannel: [_outputChannel localChannel]
		    read: NO
		    buffer: (void *)addr
	    bufferSizeForInterrupts: exclusiveIncrement];
#else	hppa
	[self startDMAForChannel: [_outputChannel localChannel]
                            read: NO
                          buffer: [_outputChannel channelBuffer]
	 bufferSizeForInterrupts: exclusiveIncrement];
#endif	hppa
    } else
	[self stopDMAForChannel: [_outputChannel localChannel] read: NO];

    runningExclusive = flag;
}

// XXXX The following duplicates the above.   Condense into 1 call,
// with compat hook for NEXTIME if need be - pcd 6/95
- (void) _runExclusiveInputDMA:(BOOL)flag
{
#if defined(hppa) || defined(sparc)
    vm_offset_t		addr;
#endif
    if (flag == runningExclusive)
	return;

    if (flag) {
	exclusiveProgress = 0;
	exclusiveIncrement = [_inputChannel descriptorSize];
	exclusiveInterruptClearFunc = [self interruptClearFunc];
#if defined(hppa) || defined(sparc)
        addr = (vm_offset_t)[_inputChannel channelBuffer];
	[self startDMAForChannel: [_inputChannel localChannel]
		    read: YES
		    buffer: (void *)addr
	    bufferSizeForInterrupts: exclusiveIncrement];
#else	hppa
	[self startDMAForChannel: [_inputChannel localChannel]
                            read: YES
                          buffer: [_inputChannel channelBuffer]
	 bufferSizeForInterrupts: exclusiveIncrement];
#endif	hppa
    } else
	[self stopDMAForChannel: [_inputChannel localChannel] read: YES];

    runningExclusive = flag;
}
// XXX End duplicated code


// Progress is in bytes.  It gets reset to 0 when _runExclusiveDMA:YES
// is called.

- (unsigned long) _exclusiveProgress
{
    return exclusiveProgress;
}

@end


@implementation IOAudio

- (IOAudioInterruptClearFunc) interruptClearFunc
{
    return 0;
}

- initFromDeviceDescription:_description
{
    int	*channelList;
    IOConfigTable *configTable;
    const char *serverName;
#define MAX_CLASS_NAME_LEN	256 /* FIXME */
    char instClassName[MAX_CLASS_NAME_LEN+1];
    const char *serverPostfix = "KernelServerInstance";
    id instClass;
    kern_server_t *kernServInst;
    IOThread thread;
#if i386
    IOEISADeviceDescription *description = _description;	// FIXME
#elif hppa
    IOHPPADeviceDescription *description = _description;	// FIXME
#else
    id description = _description;
#endif
    
#ifdef hppa
    audio_pmap = (kernel_pmap);
#endif

    if ([super initFromDeviceDescription:description] == nil)
    	return nil;
	
    if ([self attachInterruptPort] != IO_R_SUCCESS) {
	return nil;
    }

    port_set_backlog(task_self(), [self interruptPort], PORT_BACKLOG_MAX);

    if ([self reset] == NO)
        return nil;

    [self _initAudioHardwareSettings];
    
    /*
     * Get the kernel server instance for the just-loaded reloc.
     */
    configTable = [description configTable];
    if (configTable == nil) {
	IOLog("Audio: no configTable\n");
	return nil;
    }
    serverName = [configTable valueForStringKey:"Server Name"];
    strncpy(instClassName, serverName,
	    MAX_CLASS_NAME_LEN-strlen(serverPostfix));
    strcat(instClassName, serverPostfix);
    instClass = objc_lookUpClass(instClassName);
    if (instClass == nil) {
	IOLog("Audio: no kernel server instance class '%s'\n", instClassName);
	return nil;
    }
    kernServInst = [instClass kernelServerInstance];
    if (!kernServInst) {
	IOLog("Audio: no kernel server instance\n");
	return nil;
    }
    audioKernServInit(kernServInst);
    audio_makeIMuLawTab();
    
   _audioPrivate = (_audioPrivateData *) IOMalloc(sizeof(_audioPrivateData));

#ifdef	DDM_DEBUG
    IOInitDDM(AUDIO_NUM_XPR_BUFS);
#endif	DDM_DEBUG

    if ([IOAudio _instance] != nil)
	IOLog("Audio: replacing previously registered driver\n");
    [IOAudio _setInstance: self];
    
    /*
     * Initialize channel instances.
     */
    _inputChannel = [[AudioChannel alloc] initOnDevice: self read: TRUE];
    [IOAudio _addChannel: _inputChannel];

    _outputChannel = [[AudioChannel alloc] initOnDevice: self read: FALSE];
    [IOAudio _addChannel: _outputChannel];

    [_inputChannel setLocalChannel: 0];

#ifdef	i386
    if ([description numChannels] == 1) {		
	IOLog("%s at dma channel %d irq %d\n",
	    [self name],
	    [description channel],
	    [description interrupt]);
	    
	[_outputChannel setLocalChannel:0];
    } else if ([description numChannels] == 2) {
        channelList = [description channelList];
        IOLog("%s at dma channels %d and %d irq %d\n",
	    [self name],
	    channelList[0], channelList[1],
	    [description interrupt]);
	    
    	[_outputChannel setLocalChannel:1];
    }
#else
    [_outputChannel setLocalChannel:0];
#endif	i386

    /*
     * The ioThread uses this port set to receive interrupt, soundkey, and
     * command messages.
     */
    _devicePortSet = allocatePortSet();
    addPort(_devicePortSet, [self interruptPort]);

    /*
     * A separate port is used to receive pending messages and synchronous 
     * command messages from the channel instances. By ensuring that only
     * interrupt messages arrive at the interrupt port, the ioThread could
     * use port_status to see if additional interrupts are pending.
     */
    _commandPort = allocatePort();
    addPort(_devicePortSet, _commandPort);
    _commandPort = IOConvertPort(_commandPort, IO_KernelIOTask, IO_Kernel);

    /*
     * To Do: define reasonable default values for instance variables
     */
    _audioCommand = [[AudioCommand alloc] initPort: _commandPort];

    [self _setTimeout: IOAUDIO_MAXIMUM_TIMEOUT];
    thread = IOForkThread((IOThreadFunc)ioThread, (void *)self);
    (void) IOSetThreadPolicy(thread, POLICY_FIXEDPRI);
    (void) IOSetThreadPriority(thread, 30);	/* XXX */
    (void)IOForkThread((IOThreadFunc)keyThread, (void *)self);
    

    /*
     * This should be after the ioThread fork to guarantee that
     * we are ready to receive device messages.
     */
#ifdef hppa
    my_audioid = self;
#endif
    [self registerDevice];

    return self;
}

/*
 * An IOAudio subclass will need to override this method to reset hardware
 * based on configuration information in the device description.
 */
- (BOOL)reset
{
    [self subclassResponsibility:_cmd];
    return NO;
}

/*
 * FIXME: free channels and kill threads
 */
- free
{
    [IOAudio _setInstance: nil];
    // FIXME: audio_freeIMuLawTab();
    IOFree(_audioPrivate, sizeof(_audioPrivateData));
    return [super free];
}

/*
 * returns the current sample rate
 */
- (unsigned int) sampleRate
{
    return _sampleRate;
}

/*
 * FIXME
 *
 * Why do we "remap" formats/encodings? Wouldn't it be simpler to use one set
 * of enums rather than performing translations? (bkr)
 *
 * It's just historical (see below).  We can change everything over to
 * NXSoundParameterTags now if we want. (mminnick)
 *
 * 1. The orignial 68k sound/DSP driver used defines in the form SND_something.
 *
 * 2. The audio driver split from the DSP driver and started using _NXAUDIO.
 *  The soundkit also used _NXAUDIO (non-public, of course).  Since the DSP 
 * driver had to communicate with the audio driver, it started converting SND 
 * to _NXAUDIO.
 *
 * 3.  The kit needed a public parameter interface, and thus was born 
 * NXSoundParameterTags.  The kit now uses an unfortunate combination of ptags
 * and _NXAUDIO.
 *
 * I think we should should flush IOAudio defines (at least where they overlap   
 * with NXSoundParameterTags) from the exported interface and documentation.  
 * We can change all of our internal code as well, although that is less 
 * important.
 */
- (NXSoundParameterTag) dataEncoding
{
    switch (_dataEncoding) {
      case IOAudioDataFormatLinear16:
	return NX_SoundStreamDataEncoding_Linear16;
      case IOAudioDataFormatLinear8:
	return NX_SoundStreamDataEncoding_Linear8;
      case IOAudioDataFormatMulaw8:
	return NX_SoundStreamDataEncoding_Mulaw8;
      case IOAudioDataFormatAlaw8:
	return NX_SoundStreamDataEncoding_Alaw8;
    }
}

/*
 * returns the current channel count
 */
- (unsigned int) channelCount
{
    return _channelCount;
}

- (BOOL) isInputActive
{
    return _isInputActive;
}

- (BOOL) isOutputActive
{
    return _isOutputActive;
}

- (void) interruptOccurredForInput: (BOOL *) serviceInput
                         forOutput: (BOOL *) serviceOutput
{
    [self subclassResponsibility:_cmd];
}

/*
 * The ioThread executes this method when its msg_receive operation times out.
 * The IOAudio subclass might reset the hardware if a DMA operation is in
 * progress, but no interrupts are being received in the ioThread.
 */
- (void) timeoutOccurred
{
    xpr_audio_device("AD: time out occurred\n", 1,2,3,4,5);
}

- (BOOL) startDMAForChannel: (unsigned int) localChannel
	read: (BOOL) isRead
	buffer: (void *) buffer
	bufferSizeForInterrupts: (unsigned int) bufferSize
{
    [self subclassResponsibility:_cmd];
    return NO;
}


- (void) stopDMAForChannel: (unsigned int) localChannel read: (BOOL) isRead
{
    [self subclassResponsibility:_cmd];
}

- (void)getInputChannelBuffer: (void *)addr
                          size: (unsigned int *)byteCount
{
    * (vm_address_t *) addr = [_inputChannel channelBufferAddress];
    *byteCount = [_inputChannel descriptorSize] * [_inputChannel dmaCount];
}

- (void)getOutputChannelBuffer: (void *) addr
                          size: (unsigned int *)byteCount
{
    * (vm_address_t *) addr = [_outputChannel channelBufferAddress];
    *byteCount = [_outputChannel descriptorSize] * [_outputChannel dmaCount];
}

- (unsigned int)inputGainLeft
{
    return _inputGainLeft;
}

- (unsigned int)inputGainRight
{
    return _inputGainRight;
}

- (int)outputAttenuationLeft
{
    return _outputAttenuationLeft;
}

- (int)outputAttenuationRight
{
    return _outputAttenuationRight;
}

- (BOOL)isOutputMuted
{
    return _isOutputMuted;
}

- (BOOL)isLoudnessEnhanced
{
    return _isLoudnessEnhanced;
}

- (void) updateLoudnessEnhanced
{

}

- (void) updateInputGainLeft
{

}

- (void) updateInputGainRight
{

}

- (void) updateOutputMute
{

}

- (void) updateOutputAttenuationLeft
{

}

- (void) updateOutputAttenuationRight
{

}

- (void) setInput:(NXSoundParameterTag)ptag enable:(BOOL)enable
{
}

- (void) setOutput:(NXSoundParameterTag)ptag enable:(BOOL)enable
{
}


/*
 * Parameter access.
 */

- (BOOL) acceptsContinuousSamplingRates
{
    return NO;
}

- (void) getSamplingRatesLow: (int *)lowRate
                        high: (int *)highRate
{
    *lowRate = *highRate = 0;
} 

- (void)getSamplingRates: (int *)rates
                   count: (unsigned int *)numRates
{
    *numRates = 0;
}
- (void)getDataEncodings: (NXSoundParameterTag *)encodings
                   count: (unsigned int *)numEncodings
{
    *numEncodings = 0;
}

- (unsigned int) channelCountLimit
{
    return 0;
}

#if	hppa
- (BOOL) getHandler: (IOHPPAInterruptHandler *)handler
              level: (unsigned int *) ipl
           argument: (unsigned int *) arg
       forInterrupt: (unsigned int) localInterrupt
{
    return NO;
}
#elif sparc
- (BOOL) getHandler: (IOSPARCInterruptHandler *)handler
              level: (unsigned int *) ipl
           argument: (unsigned int *) arg
       forInterrupt: (unsigned int) localInterrupt
{
    *handler = interruptHandler;
    *ipl = IPLDEVICE;
    *arg = (unsigned int)self;
    return YES;
}
#elif	i386
- (BOOL) getHandler: (IOEISAInterruptHandler *)handler
              level: (unsigned int *) ipl
           argument: (unsigned int *) arg
       forInterrupt: (unsigned int) localInterrupt
{
    *handler = interruptHandler;
    *ipl = IPLDEVICE;
    *arg = (unsigned int)self;
    return YES;
}
#endif
@end


static void ioThread(IOAudio *audioDevice)
{
    msg_return_t result;
    msg_header_t Msg, *msg = &Msg;
	
    while (TRUE) {

	msg->msg_size = sizeof(Msg);
	msg->msg_local_port = [audioDevice _devicePortSet];

	result = msg_receive(msg, RCV_TIMEOUT, [audioDevice _timeout]);

	switch (result) {

	  case RCV_SUCCESS:

	    xpr_audio_device("AD: message received in ioThread\n",
			     1, 2, 3, 4, 5);
	    if (msg->msg_id == IO_DEVICE_INTERRUPT_MSG)
		[audioDevice _interruptOccurred];
	    else if (msg->msg_id == AD_CMD_MSG_INPUT_PENDING)
		[audioDevice _dataPendingOccurred:
			[audioDevice _inputChannel]];
	    else if (msg->msg_id == AD_CMD_MSG_OUTPUT_PENDING)
		[audioDevice _dataPendingOccurred:
			[audioDevice _outputChannel]];
	    else if (msg->msg_id == AD_CMD_MSG_SYNCHRONOUS)
		[audioDevice _commandOccurred];
	    else
		IOLog("Audio: unknown message id %d\n", msg->msg_id);

	    break;

	  case RCV_TIMED_OUT:

	    if ([audioDevice isInputActive] || [audioDevice isOutputActive])
		[audioDevice timeoutOccurred];
	    break;

	  default:

	    IOLog("%s: %s thread: msg_receive returns %d\n",
		  [audioDevice name], [audioDevice deviceKind], result);
	    IOExitThread();
	}
    }
}

static void keyThread(IOAudio *audioDevice)
{
    id eventClass;
    id eventInstance;
    port_t keyPort;
    port_t eventPort;
    IOReturn ioReturn;
    struct evioSpecialKeyMsg msg;
    msg_return_t msgReturn;
    
    keyPort = allocatePort();
    
    /*
     * Locate EventDriver class
     */
    eventClass = objc_lookUpClass("EventDriver");
    if (eventClass == nil) {
        IOLog("Audio: objc_lookUpClass failure\n");
        IOExitThread();
    }
    
    /*
     * Locate EventDriver instance
     */
    eventInstance = [eventClass instance];

    eventPort = (port_t)[eventInstance ev_port];
    ioReturn = [eventInstance setSpecialKeyPort: eventPort
		    keyFlavor: NX_KEYTYPE_SOUND_UP
		    keyPort: keyPort];

   if (ioReturn != IO_R_SUCCESS) {
       IOLog("Audio: SetSpecialKeyPort error %d\n", ioReturn);
       IOExitThread();
   }

    ioReturn = [eventInstance setSpecialKeyPort: eventPort
		    keyFlavor: NX_KEYTYPE_SOUND_DOWN
		    keyPort: keyPort];
		
    if (ioReturn != IO_R_SUCCESS) {
       IOLog("Audio: SetSpecialKeyPort error %d\n", ioReturn);
       IOExitThread();
   }
   
    while (TRUE) {
	msg.Head.msg_local_port = keyPort;
	msg.Head.msg_size = sizeof(msg);
	msgReturn = msg_receive(&msg.Head, MSG_OPTION_NONE, 0);
	
	if (msgReturn != RCV_SUCCESS) {
	    IOLog("Audio: keyThread msg_receive error: %d\n", msgReturn);
	    IOExitThread();
	}
	
	if (msg.Head.msg_id != EV_SPECIAL_KEY_MSG_ID) {
	    IOLog("Audio: unknown msg id %d in keyThread\n", msg.Head.msg_id);
	    IOExitThread();
	}
	
	[audioDevice _keyOccurred:msg.key event: msg.direction flags: msg.flags];
    }

    IOExitThread();
}


/*
 * HISTORY 
 *
 * 4/1/94/rkd: Added input gain, buffer address/size methods. 
 *
 * 11/8/95/rkd: Added support for input/output source selection.
 */ 
