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
 * audio_mix.c
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver mixing.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import "audioLog.h"
#import "audio_mix.h"
#import "audio_peak.h"
#import "audio_mulaw.h"
#import <architecture/byte_order.h>
#import <limits.h>
#import <bsd/string.h>

#define MULAW_MAX       128
#define MULAW_MIN       0

/*
 * Macros to clip int to short/byte/mulaw-byte and load into pointer.
 * Increments destination pointer.
 * Increments clip count.
 */
#define clip16(i,p,c) if (i > SHRT_MAX) { \
                        *p++ = SHRT_MAX; \
                        c++; \
                    } else if (i < SHRT_MIN) { \
                        *p++ = SHRT_MIN; \
                        c++; \
                    } else \
                        *p++ = (short)i;

#define clip8(i,p,c) if (i > SCHAR_MAX) { \
                        *p++ = SCHAR_MAX; \
                        c++; \
                    } else if (i < SCHAR_MIN) { \
                        *p++ = SCHAR_MIN; \
                        c++; \
                    } else \
                        *p++ = (char)i;

/* xor the high bit to convert between 2's comp and 1's comp (unary) */
#define unary(i)		(((i) ^ (char)0x80) | ((i) & (char)0x7f))

#define clipUnary8(i,p,c) if (i > SCHAR_MAX) { \
                        *p++ = unary(SCHAR_MAX); \
                        c++; \
                    } else if (i < SCHAR_MIN) { \
                        *p++ = unary(SCHAR_MIN); \
                        c++; \
                    } else \
                        *p++ = unary((char)i);

#define clipMulaw8(i,p,c) if (i > SHRT_MAX) { \
                        *p++ = MULAW_MAX; \
                        c++; \
                    } else if (i < SHRT_MIN) { \
                        *p++ = MULAW_MIN; \
                        c++; \
                    } else \
                        *p++ = audio_shortToMulaw((short)i);


void audio_swapSamples(short *src, short *dest, u_int count)
{
#if	defined(hppa) || defined(sparc)
// I have noticed that src is not aligned to short boundary when two samples with
// different sampling rate are played at the same time. Also this happens at the end of 
// second sample. This results in an alignment trap in hppa leading to system crash.
// Therefore I am providing this workaround of forcing the alignment to even bounary, 
// instead of a complete fix.  This is 
// a late stage for us to make big changes  for FCS (6/6/94). 
// This has to be fixed for 3.3
// TBD: Fix the alignment probs
	unsigned int ptr,ptr1;
	ptr = (unsigned int)src;
	ptr1 = (unsigned int)dest;
	
//	if (!count) return;
	if ((ptr & 1)){
//	 printf("The source is not aligned %x\n",src);
	 ptr &= 0xfffffffe;		//forcing the alignment
	 src = (short *)ptr;
	}
	if ((ptr1 & 1)){
//	 printf("The Dest is not aligned %x \n",dest);
	 ptr1 &= 0xfffffffe;		//forcing the alignment
	 dest = (short *)ptr1;
	}
#endif
    while (count--)
        *dest++ = NXSwapBigShortToHost(*src++);
}

void audio_twosComp8ToUnary(char *src, char *dest, u_int count)
{
    while (count--) {
        /* xor the high bit */
        *dest++ = unary(*src);
        src++;
    }
}

/*
 * Scale samples, return number of times clipped.
 */
u_int audio_scaleSamples(char *src, char *dest, u_int count,
                   IOAudioDataFormat format, u_int chanCount,
                   int leftGain, int rightGain)
{
    short *shortSrc, *shortDest;
    u_int clips = 0;
    int val = 0;
    int leftGain8, rightGain8, monoGain = 0;
    unsigned char *uSrc, *uDest;

    /*
     * Gains MUST be signed values when used in the following
     * calculations.  It is not clear (to me) why.
     */

    switch (format) {
      case IOAudioDataFormatLinear16:
        count /= 2;
        shortSrc = (short *)src;
        shortDest = (short *)dest;
        switch (chanCount) {
          case 1:
            monoGain = (leftGain + rightGain) / 2;
            while (count--) {
                val = (((int)*shortSrc++)*monoGain)>>15;
                clip16(val, shortDest, clips);
            }
            break;
          case 2:
            count /= 2;
            while (count--) {
                val = (((int)*shortSrc++)*leftGain)>>15;
                /*
                 * FIXME: clipping not working.
                 */
                *shortDest++ = (short)val;
                //clip16(val, shortDest, clips);
                val = (((int)*shortSrc++)*rightGain)>>15;
                *shortDest++ = (short)val;
                //clip16(val, shortDest, clips);
            }
            break;
          default:
            break;
        }
        break;
      case IOAudioDataFormatLinear8:
        /* Gains are passed in 16-bit range */
        leftGain8 = leftGain >> 8;
        rightGain8 = rightGain >> 8;
        switch (chanCount) {
          case 1:
            monoGain = (leftGain8 + rightGain8) / 2;
            while (count--) {
                val = (((int)*src++)*monoGain)>>7;
                clip8(val, dest, clips);
            }
            break;
          case 2:
            count /= 2;
            while (count--) {
                val = (((int)*src++)*leftGain8)>>7;
                clip8(val, dest, clips);
                val = (((int)*src++)*rightGain8)>>7;
                clip8(val, dest, clips);
            }
            break;
          default:
            break;
        }
        break;
      case IOAudioDataFormatMulaw8:
        /* Gains are passed in 16-bit range */
        leftGain8 = leftGain >> 8;
        rightGain8 = rightGain >> 8;
	uSrc = (unsigned char *)src;
	uDest = (unsigned char *)dest;
        switch (chanCount) {
          case 1:
            monoGain = (leftGain8 + rightGain8) / 2;
            while (count--) {
		val = (((int)audio_muLaw[(unsigned int)*uSrc++])*monoGain)>>7;
		clipMulaw8(val, uDest, clips);
            }
            break;
          case 2:
            count /= 2;
	    while (count--) {
		val = (((int)audio_muLaw[(unsigned int)*uSrc++])*leftGain8)>>7;
		clipMulaw8(val, uDest, clips);
		val = (((int)audio_muLaw[(unsigned int)*uSrc++])*
		       rightGain8)>>7;
		clipMulaw8(val, uDest, clips);
            }
            break;
          default:
            break;
        }
        break;
      default:
        IOLog("Audio: unrecognized format %d in scaleSamples\n", format);
        break;
    }

    return clips;
}

void audio_resample22To44(char *src, char *dest, u_int count,
                    IOAudioDataFormat format, u_int channelCount)
{
    if ( channelCount == 1 )
    {
        /* Just double samples! */
        audio_convertMonoToStereo(src, dest, count, format);
    }
    else
    {
        short *shortSrc, *shortDest;     
  
        shortSrc  = (short *)src;
        shortDest = (short *)dest;

        count /= 2;
        while (count--)
        {
            *(shortDest+2) = *shortSrc;
            *shortDest++   = *shortSrc++;
            *(shortDest+2) = *shortSrc;
            *shortDest++   = *shortSrc++;
            shortDest     += 2;
        }        
    }
}

void audio_resample44To22(char *src, char *dest, u_int count,
			  IOAudioDataFormat format, u_int *phase)
{
    short *shortSrc, *shortDest;

    /*
     * FIXME: implement phase.
     */

    count /= 2;	/* src is incremented twice each loop */

    switch (format) {
      case IOAudioDataFormatLinear16:
        count /= 2;
        shortSrc = (short *)src;
        shortDest = (short *)dest;
        while (count--) {
            *shortDest++ = *shortSrc++;
            shortSrc++;
        }
        break;
      case IOAudioDataFormatLinear8:
      case IOAudioDataFormatMulaw8:
        while (count--) {
            *dest++ = *src++;
            src++;
        }
        break;
      default:
        IOLog("Audio: unrecognized format %d in resample\n", format);
        break;
    }
}

void audio_convertMonoToStereo(char *src, char *dest, u_int count,
			       IOAudioDataFormat format)
{
    short *shortSrc, *shortDest;

    switch (format) {
      case IOAudioDataFormatLinear16:
        count /= 2;
        shortSrc = (short *)src;
        shortDest = (short *)dest;
        while (count--) {
            *shortDest++ = *shortSrc;
            *shortDest++ = *shortSrc++;
        }
        break;
      case IOAudioDataFormatLinear8:
      case IOAudioDataFormatMulaw8:
        while (count--) {
            *dest++ = *src;
            *dest++ = *src++;
        }
        break;
      default:
        IOLog("Audio: unrecognized format %d in convMono\n", format);
        break;
    }
}

void audio_convertStereoToMono(char *src, char *dest, u_int count,
                         IOAudioDataFormat format, u_int *phase)
{
    short *shortSrc, *shortDest;
    unsigned char *uSrc, *uDest;
    int sample;

    /*
     * FIXME: implement phase.
     */

    count /= 2;	/* src is incremented twice each loop */

    switch (format) {
      case IOAudioDataFormatLinear16:
        count /= 2;
        shortSrc = (short *)src;
        shortDest = (short *)dest;
        while (count--) {
            sample = *shortSrc++;
            sample += *shortSrc++;
            sample += sample & 1; /* round up if odd */
            sample /= 2;
            *shortDest++ = (short)sample;
        }
        break;
      case IOAudioDataFormatLinear8:
        while (count--) {
            sample = *src++;
            sample += *src++;
            sample += sample & 1;
            sample /= 2;
            *dest++ = (char)sample;
        }
        break;
      case IOAudioDataFormatMulaw8:
	uSrc = (unsigned char *)src;
	uDest = (unsigned char *)dest;
        while (count--) {
	    sample = audio_muLaw[(unsigned int)*uSrc++];
	    sample += audio_muLaw[(unsigned int)*uSrc++];
            sample += sample & 1;
            sample /= 2;
            *uDest++ = audio_shortToMulaw((short)sample);
        }
        break;
      default:
        IOLog("Audio: unrecognized format %d in convMono\n", format);
        break;
    }
}

void audio_convertLinear8ToLinear16(char *src, short *dest, u_int count)
{
    while (count--)
	*dest++ = *src++ << 8;
}

void audio_convertLinear8ToMulaw8(char *src, char *dest, u_int count)
{
    while (count--)
	*dest = audio_shortToMulaw((short)(*src++) << 8);
}

void audio_convertLinear16ToLinear8(short *src, char *dest, u_int count,
				    u_int *phase)
{
    while (count--)
	*dest++ = *src++ >> 8;
}

void audio_convertLinear16ToMulaw8(short *src, char *dest, u_int count,
				   u_int *phase)
{
    while (count--)
	*dest = audio_shortToMulaw(*src++);
}

void audio_convertMulaw8ToLinear16(char *src, short *dest, u_int count)
{
    unsigned char *uSrc = (unsigned char *)src;
    unsigned char *uDest = (unsigned char *)dest;
    
    while (count--)
	*uDest++ = audio_muLaw[(unsigned int)*uSrc++];
}

void audio_convertMulaw8ToLinear8(char *src, char *dest, u_int count)
{
    return;
}

/*
 * Mix source into destination.
 * Return number of times mix clipped.
 */
u_int audio_mix(char *src, char *dest, u_int count, IOAudioDataFormat format,
                boolean_t virgin)
{
    int sum;
    u_int clips = 0;
    short *shortSrc, *shortDest;
    unsigned char *uSrc, *uDest;

    if (count == 0)
        return 0;

    if (virgin) {
	if (format == IOAudioDataFormatLinear8)
	    audio_twosComp8ToUnary(src, dest, count);
	else
	    bcopy(src, dest, count);
    } else switch (format) {
      case IOAudioDataFormatLinear16:
        count /= 2;
        shortSrc = (short *)src;
        shortDest = (short *)dest;
        while (count--) {
            sum = (int)*shortDest + (int)*shortSrc++;
            clip16(sum, shortDest, clips);
        }
        break;
      case IOAudioDataFormatLinear8:
        while (count--) {
	    /*
	     * FIXME: only convert to unary if devIsUnary.
	     */
            sum = (int)unary(*dest) + (int)*src++;
            clipUnary8(sum, dest, clips);
        }
        break;
      case IOAudioDataFormatMulaw8:
	uSrc = (unsigned char *)src;
	uDest = (unsigned char *)dest;
        while (count--) {
	    sum = (int)audio_muLaw[(unsigned int)*uDest] +
		(int)audio_muLaw[(unsigned int)*uSrc++];
	    clipMulaw8(sum, uDest, clips);
        }
        break;
      default:
        IOLog("Audio: unrecognized format %d in mix\n", format);
        break;
    }

    return clips;
}
