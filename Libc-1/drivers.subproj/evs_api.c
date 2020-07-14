/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/******************************************************************************
	event_status_driver_api.c
	API for the events status driver.
	This file implements public API.
	mpaque 11Oct91
	
	Copyright 1991 NeXT Computer, Inc.
	
	Modified:
	29 June 1992 Mike Paquette at NeXT
		Implemented API for the new Mach based Event Driver.
	
******************************************************************************/

#ifndef	DRIVER_PRIVATE
#define	DRIVER_PRIVATE

#import <drivers/event_status_driver.h>
#import <bsd/dev/machine/evsio.h>

#ifdef _NeXT_MACH_EVENT_DRIVER_		/* Flag defined in evsio.h */
#import <mach/mach.h>
#import <math.h>
#import <bsd/dev/machine/evio.h>

/* Definitions specific to the Mach based event driver */
#define BRIGHT_MAX 64
#define VOLUME_MAX 43

/* Lazy error checking for bad event handles */
#define CHECKEVS if ( ! handle || handle->var.port == PORT_NULL) return;
#define RCHECKEVS(retval) \
	if ( ! handle || handle->var.port == PORT_NULL) return(retval);

static void secs_to_packed_nsecs(double secs, unsigned int *nsecs)
{
	_NX_packed_time_t data;
	int i;

	if ( secs > (double)INT_MAX )
	{
	    for ( i = 0; i < EVS_PACKED_TIME_SIZE; ++i )
		    nsecs[i] = INT_MAX;
	    return;
	}
	secs *= 1000000000.0;		// Secs to nsecs
	data.tval = (ns_time_t)secs;	// nsecs to ns_time_t
	for ( i = 0; i < EVS_PACKED_TIME_SIZE; ++i )
		nsecs[i] = data.itval[i];
}

static double packed_nsecs_to_secs(unsigned int *nsecs)
{
	double secs;
	_NX_packed_time_t data;
	int i;

	for ( i = 0; i < EVS_PACKED_TIME_SIZE; ++i )
		data.itval[i] = nsecs[i];
	secs = (double) ((long long)data.tval); // Try to preserve sign
	secs /= 1000000000.0;			// nsecs to secs
	return secs;
}

static port_t _NXGetEventPort()
{
	port_t evport;
	extern port_t _event_port_by_tag();
	if ( (evport = _event_port_by_tag(0)) == PORT_NULL )
	{
		if ( (evport = _event_port_by_tag(1)) == PORT_NULL )
			return PORT_NULL;
	}
	return evport;
}

/* Open and Close */
NXEventHandle NXOpenEventStatus(void)
{
	NXEventHandle handle;
	extern port_t _event_port_by_tag();
	
	handle = (NXEventHandle)malloc( sizeof (struct _NXEventHandle_) );
	if ( handle == (NXEventHandle)0 )
		return (NXEventHandle)0;

	/*
	 * Try first for priviledged port, then for non-priviledged port
	 * Only root will be able to get the priviledged port.
	 */
	if ( (handle->var.port = _NXGetEventPort()) == PORT_NULL )
	{
		free((void *)handle);
		return (NXEventHandle)0;
	}
	return handle;
}

void NXCloseEventStatus(NXEventHandle handle)
{
	if ( handle == (NXEventHandle)0 )
		return;
	if ( handle->var.port != PORT_NULL )
		port_deallocate( task_self(), handle->var.port  );
	free((void *)handle);
	return;
}

/* post event */
void _NXPostLLEvent(NXEventHandle handle, _NXLLEvent *event)
{
	unsigned int params[EVIOLLPE_SIZE];
	CHECKEVS;

	params[EVIOLLPE_TYPE] = event->type;
	params[EVIOLLPE_LOC_X] = event->location.x;
	params[EVIOLLPE_LOC_Y] = event->location.y;
	bcopy(	(char *)&event->data,
		(char *)&params[EVIOLLPE_DATA0],
		sizeof (NXEventData) );
    
	NXEvSetParameterInt(handle, EVIOLLPE, params, EVIOLLPE_SIZE);
}

/* Status query */
NXEventSystemInfoType NXEventSystemInfo(NXEventHandle handle,
					char *flavor,
					NXEventSystemInfoType evs_info,
					unsigned int *evs_info_cnt)
{
	int r;
	
	RCHECKEVS((NXEventSystemInfoType)0);
	// Translate the one existing old case to new format
	if ( ((int)flavor) == __OLD_NX_EVS_DEVICE_INFO )
		flavor = NX_EVS_DEVICE_INFO;

	r = NXEvGetParameterInt(handle, flavor, *evs_info_cnt,
				evs_info, evs_info_cnt );
	if ( r != IO_R_SUCCESS )
		return (NXEventSystemInfoType)0;
	return evs_info;
}

/* Keyboard */

void NXResetKeyboard(NXEventHandle handle)
{
	unsigned int params[EVSIORKBD_SIZE];
	CHECKEVS;
    
	NXEvSetParameterInt(	handle, EVSIORKBD, params, EVSIORKBD_SIZE);
}

void NXSetKeyRepeatInterval(NXEventHandle handle, double rate)
{
	unsigned int params[EVSIOSKR_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( rate, params );
	NXEvSetParameterInt(	handle, EVSIOSKR, params, EVSIOSKR_SIZE);
}

double NXKeyRepeatInterval(NXEventHandle handle)
{
	unsigned int params[EVSIOCKR_SIZE];
	int rcnt = EVSIOCKR_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCKR, EVSIOCKR_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( &params[EVSIOCKR_BETWEEN] );
}

void NXSetKeyRepeatThreshold(NXEventHandle handle, double threshold)
{
	unsigned int params[EVSIOSIKR_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( threshold, params );
	NXEvSetParameterInt(	handle, EVSIOSIKR, params, EVSIOSIKR_SIZE);
}

double NXKeyRepeatThreshold(NXEventHandle handle)
{
	unsigned int params[EVSIOCKR_SIZE];
	int rcnt = EVSIOCKR_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCKR, EVSIOCKR_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( &params[EVSIOCKR_INITIAL] );
}

NXKeyMapping * NXSetKeyMapping(NXEventHandle handle, NXKeyMapping *keymap)
{
	int r;
	RCHECKEVS((NXKeyMapping *)0);
	if ( keymap->size > EVSIOSKM_SIZE )
	    	return (NXKeyMapping *)0;
	r = NXEvSetParameterChar(handle, EVSIOSKM,
				keymap->mapping, keymap->size);
	if ( r != IO_R_SUCCESS )
	    return (NXKeyMapping *)0;
	return keymap;
}

int NXKeyMappingLength(NXEventHandle handle)
{
	int r, rcnt;
	unsigned int params[EVSIOCKML_SIZE];
	RCHECKEVS(0);
	r = NXEvGetParameterInt(handle, EVSIOCKML, EVSIOCKML_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0;
	return (int) (params[0]);
}

NXKeyMapping * NXGetKeyMapping(NXEventHandle handle, NXKeyMapping *keymap)
{
	int r;
	RCHECKEVS((NXKeyMapping *)0);
	r = NXEvGetParameterChar(handle, EVSIOCKM, keymap->size,
				keymap->mapping, &keymap->size );
	if ( r != IO_R_SUCCESS )
    		return (NXKeyMapping *)0;
	return keymap;
}

/* Mouse */

void NXResetMouse(NXEventHandle handle)
{
	unsigned int params[EVSIORMS_SIZE];
	CHECKEVS;
    
	NXEvSetParameterInt(handle, EVSIORMS, params, EVSIORMS_SIZE);
}

void NXSetClickTime(NXEventHandle handle, double secs)
{
	unsigned int params[EVSIOSCT_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( secs, params );
	NXEvSetParameterInt(handle, EVSIOSCT, params, EVSIOSCT_SIZE);
}

double NXClickTime(NXEventHandle handle)
{
	unsigned int params[EVSIOCCT_SIZE];
	int rcnt = EVSIOCCT_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCCT, EVSIOCCT_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( params );
}

void NXSetClickSpace(NXEventHandle handle, _NXSize_ *area)
{
	unsigned int params[EVSIOSCS_SIZE];
	CHECKEVS;
	params[EVSIOSCS_X] = (unsigned int)(area->width);
	params[EVSIOSCS_Y] = (unsigned int)(area->height);
	NXEvSetParameterInt(handle, EVSIOSCS, params, EVSIOSCS_SIZE);
}

void NXGetClickSpace(NXEventHandle handle, _NXSize_ *area)
{
	unsigned int params[EVSIOCCS_SIZE];
	int rcnt = EVSIOCCS_SIZE;
	CHECKEVS;
	NXEvGetParameterInt(handle, EVSIOCCS, EVSIOCCS_SIZE,
				params, &rcnt );
	area->width = params[EVSIOCCS_X];
	area->height = params[EVSIOCCS_Y];
}

void NXSetMouseScaling(NXEventHandle handle, NXMouseScaling *scaling)
{
	unsigned int params[EVSIOSMS_SIZE];
	int i;
	unsigned int *dp;
	CHECKEVS;
	params[EVSIOSMS_NSCALINGS] = scaling->numScaleLevels;
	if ( params[EVSIOSMS_NSCALINGS] > NX_MAXMOUSESCALINGS )
		params[EVSIOSMS_NSCALINGS] = NX_MAXMOUSESCALINGS;
	dp = &params[EVSIOSMS_DATA];
	for ( i = 0; i < params[EVSIOSMS_NSCALINGS]; ++i )
	{
		*dp++ = scaling->scaleThresholds[i];
		*dp++ = scaling->scaleFactors[i];
	}
	NXEvSetParameterInt(handle, EVSIOSMS, params, EVSIOSMS_SIZE);
}

void NXGetMouseScaling(NXEventHandle handle, NXMouseScaling *scaling)
{
	int r;
	unsigned int params[EVSIOCMS_SIZE];
	int rcnt = EVSIOCMS_SIZE;
	int i;
	unsigned int *dp;
	CHECKEVS;
	r = NXEvGetParameterInt(handle, EVSIOCMS, EVSIOCMS_SIZE, params,&rcnt);
	if ( r != IO_R_SUCCESS )
	{
		scaling->numScaleLevels = 0;
		return;
	}
	scaling->numScaleLevels = params[EVSIOSMS_NSCALINGS];
	dp = &params[EVSIOSMS_DATA];
	for ( i = 0; i < params[EVSIOSMS_NSCALINGS]; ++i )
	{
		scaling->scaleThresholds[i] = *dp++;
		scaling->scaleFactors[i] = *dp++;
	}
}


void NXEnableMouseButton(NXEventHandle handle, NXMouseButton button)
{
	unsigned int params[EVSIOSMH_SIZE];
	CHECKEVS;
	params[0] = (unsigned int)button;
	NXEvSetParameterInt(handle, EVSIOSMH, params, EVSIOSMH_SIZE);
}

NXMouseButton NXMouseButtonEnabled(NXEventHandle handle)
{
	unsigned int params[EVSIOCMH_SIZE];
	int rcnt = EVSIOCMH_SIZE;
	int r;
	RCHECKEVS(NX_OneButton);
	r = NXEvGetParameterInt(handle, EVSIOCMH, EVSIOCMH_SIZE, params,&rcnt);
	if ( r != IO_R_SUCCESS )
		return NX_OneButton;
	return (NXMouseButton)(params[0]);
}


/* Wait Cursor */

void NXSetWaitCursorThreshold(NXEventHandle handle, double threshold)
{
	unsigned int params[EVSIOSWT_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( threshold, params );
	NXEvSetParameterInt(handle, EVSIOSWT, params, EVSIOSWT_SIZE);
}

double NXWaitCursorThreshold(NXEventHandle handle)
{
	unsigned int params[EVSIOCWINFO_SIZE];
	int rcnt = EVSIOCWINFO_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCWINFO, EVSIOCWINFO_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( &params[EVSIOCWINFO_THRESH] );
}

void NXSetWaitCursorSustain(NXEventHandle handle, double sustain)
{
	unsigned int params[EVSIOSWS_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( sustain, params );
	NXEvSetParameterInt(handle, EVSIOSWS, params, EVSIOSWS_SIZE);
}

double NXWaitCursorSustain(NXEventHandle handle)
{
	unsigned int params[EVSIOCWINFO_SIZE];
	int rcnt = EVSIOCWINFO_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCWINFO, EVSIOCWINFO_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( &params[EVSIOCWINFO_SUSTAIN] );
}

void NXSetWaitCursorFrameInterval(NXEventHandle handle, double interval)
{
	unsigned int params[EVSIOSWFI_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( interval, params );
	NXEvSetParameterInt(handle, EVSIOSWFI, params, EVSIOSWFI_SIZE);
}

double NXWaitCursorFrameInterval(NXEventHandle handle)
{
	unsigned int params[EVSIOCWINFO_SIZE];
	int rcnt = EVSIOCWINFO_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCWINFO, EVSIOCWINFO_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( &params[EVSIOCWINFO_FINTERVAL] );
}

/* Screen Brightness and Auto-dimming */

void NXSetAutoDimThreshold(NXEventHandle handle, double threshold)
{
	unsigned int params[EVSIOSADT_SIZE];
	CHECKEVS;
	secs_to_packed_nsecs( threshold, params );
	NXEvSetParameterInt(handle, EVSIOSADT, params, EVSIOSADT_SIZE);
}

double NXAutoDimThreshold(NXEventHandle handle)
{
	unsigned int params[EVSIOCADT_SIZE];
	int rcnt = EVSIOCADT_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCADT, EVSIOCADT_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( params );
}

double NXAutoDimTime(NXEventHandle handle)
{
	unsigned int params[EVSIOGDADT_SIZE];
	int rcnt = EVSIOGDADT_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOGDADT, EVSIOGDADT_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( params );
}

double NXIdleTime(NXEventHandle handle)
{
	unsigned int params[EVSIOIDLE_SIZE];
	int rcnt = EVSIOIDLE_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOIDLE, EVSIOIDLE_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 0.0;
	return packed_nsecs_to_secs( params );
}

void NXSetAutoDimState(NXEventHandle handle, BOOL dimmed)
{
	unsigned int params[EVSIOSADS_SIZE];
	CHECKEVS;
	params[0] = dimmed;
	NXEvSetParameterInt(handle, EVSIOSADS, params, EVSIOSADS_SIZE);
}

BOOL NXAutoDimState(NXEventHandle handle)
{
	unsigned int params[EVSIOCADS_SIZE];
	int rcnt = EVSIOCADS_SIZE;
	int r;
	RCHECKEVS(0.0);
	r = NXEvGetParameterInt(handle, EVSIOCADS, EVSIOCADS_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return NO;
	return (params[0] ? YES : NO);
}

void NXSetScreenBrightness(NXEventHandle handle, double brightness)
{
	unsigned int params[EVSIOSB_SIZE];
	CHECKEVS;
	params[0] = (int) (brightness * BRIGHT_MAX);
	NXEvSetParameterInt(handle, EVSIOSB, params, EVSIOSB_SIZE);
}

double NXScreenBrightness(NXEventHandle handle)
{
	unsigned int params[EVSIO_DCTLINFO_SIZE];
	int rcnt = EVSIO_DCTLINFO_SIZE;
	int r;
	RCHECKEVS(1.0);
	r = NXEvGetParameterInt(handle, EVSIO_DCTLINFO, EVSIO_DCTLINFO_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 1.0;
	return ((double)params[EVSIO_DCTLINFO_BRIGHT] / (double)BRIGHT_MAX);
}

void NXSetAutoDimBrightness(NXEventHandle handle, double brightness)
{
	unsigned int params[EVSIOSADB_SIZE];
	CHECKEVS;
	params[0] = (int) (brightness * BRIGHT_MAX);
	NXEvSetParameterInt(handle, EVSIOSADB, params, EVSIOSADB_SIZE);
}

double NXAutoDimBrightness(NXEventHandle handle)
{
	unsigned int params[EVSIO_DCTLINFO_SIZE];
	int rcnt = EVSIO_DCTLINFO_SIZE;
	int r;
	RCHECKEVS(1.0);
	r = NXEvGetParameterInt(handle, EVSIO_DCTLINFO, EVSIO_DCTLINFO_SIZE,
				params, &rcnt );
	if ( r != IO_R_SUCCESS )
		return 1.0;
	return ((double)params[EVSIO_DCTLINFO_AUTODIMBRIGHT]
		/ (double)BRIGHT_MAX);
}

#if !defined(__DYNAMIC__) && !defined(ppc)

/* Speaker Volume */
void NXSetCurrentVolume(NXEventHandle handle, double volume)
{
	int vol;
	extern int SNDSetVolume( int, int );
	
	CHECKEVS;
	// Convert from 0..1 to 1..43 range
	vol = (int)(volume * (VOLUME_MAX - 1)) + 1;
	// clamp if needed
	if ( vol < 1 )
		vol = 1;
	else if ( vol > VOLUME_MAX )
		vol = VOLUME_MAX;
	SNDSetVolume( vol, vol );
}

double NXCurrentVolume(NXEventHandle handle)
{
	int l_vol, r_vol;
	double volume;
	extern int SNDGetVolume( int *, int * );

	RCHECKEVS(0.0);
	l_vol = r_vol = 1;
	// Fetch channel volumes from sound driver
	SNDGetVolume( &l_vol, &r_vol );
	--l_vol;	// Convert from 1..43 to 0..42 range
	--r_vol;
	// Average and scale into 0..1 range
	volume = (double)(l_vol + r_vol);
	volume /= (((double)(VOLUME_MAX - 1)) * 2.0);
	return volume;
}

#endif /* !defined __DYNAMIC__ */

/* Generic entry points */
/*
 * FIXME: For some odd reason, apps not running as root get the actual
 *	event port invalidated between appDidInit and the main event loop.
 *	Non-appkit based programs are unaffected.
 *	We try to catch the appDidInit case, and generate a valid port.
 */
int NXEvSetParameterInt(NXEventHandle handle,
			char *parameterName,
			unsigned int *parameterArray,
			unsigned int count)
{
	msg_return_t r;
	r = _NXEvSetParameterInt( handle->var.port, 0,
				parameterName, parameterArray, count);
	if ( r == SEND_INVALID_PORT )
	{
	    if ( (handle->var.port = _NXGetEventPort()) != PORT_NULL )
		r = _NXEvSetParameterInt( handle->var.port, 0,
					parameterName, parameterArray, count);
	}
	return r;
}

int NXEvSetParameterChar(NXEventHandle handle,
			char *parameterName,
			unsigned char *parameterArray,
			unsigned int count)
{
	msg_return_t r;
	r = _NXEvSetParameterChar( handle->var.port, 0,
				parameterName, parameterArray, count);
	if ( r == SEND_INVALID_PORT )
	{
	    if ( (handle->var.port = _NXGetEventPort()) != PORT_NULL )
		r = _NXEvSetParameterChar( handle->var.port, 0,
					parameterName, parameterArray, count);
	}
	return r;
}

int NXEvGetParameterInt(NXEventHandle handle,
			char *parameterName,
			unsigned int maxCount,
			unsigned int *parameterArray,
			unsigned int *returnedCount)
{
	msg_return_t r;
	r = _NXEvGetParameterInt( handle->var.port, 0,
				parameterName,
				maxCount,
				parameterArray,
				returnedCount );
	if ( r == SEND_INVALID_PORT )
	{
	    if ( (handle->var.port = _NXGetEventPort()) != PORT_NULL )
		r = _NXEvGetParameterInt( handle->var.port, 0,
					parameterName,
					maxCount,
					parameterArray,
					returnedCount );
	}
	return r;
}

int NXEvGetParameterChar(NXEventHandle handle,
			char *parameterName,
			unsigned int maxCount,
			unsigned char *parameterArray,
			unsigned int *returnedCount)
{
	msg_return_t r;
	r = _NXEvGetParameterChar( handle->var.port, 0,
				parameterName,
				maxCount,
				parameterArray,
				returnedCount );
	if ( r == SEND_INVALID_PORT )
	{
	    if ( (handle->var.port = _NXGetEventPort()) != PORT_NULL )
		r = _NXEvGetParameterChar( handle->var.port, 0,
					parameterName,
					maxCount,
					parameterArray,
					returnedCount );
	}
	return r;
}

#else /* ! _NeXT_MACH_EVENT_DRIVER_ */

#import <sys/ioctl.h>
/* Definitions specific to the old Unix event driver */
#define BRIGHT_MAX 61
#define VOLUME_MAX 43

/* Lazy error checking for bad event handles */
#define CHECKEVS if ( ! handle || handle->var.fd < 0) return;
#define RCHECKEVS(retval) if ( ! handle || handle->var.fd < 0) return(retval);

#define SECSTOVBL(x) (int)(70.0*(x))
#define VBLTOSECS(x) ((double)(x)/70.0)


/* Open and Close */
NXEventHandle NXOpenEventStatus(void)
{
	NXEventHandle handle;
	
	handle = (NXEventHandle)malloc( sizeof (struct _NXEventHandle_) );
	if ( handle == (NXEventHandle)0 )
		return (NXEventHandle)0;

	if ( (handle->var.fd = open("/dev/evs0", O_RDWR, 0)) == -1 )
	{
		free((void *)handle);
		return (NXEventHandle)0;
	}
	return handle;
}

void NXCloseEventStatus(NXEventHandle handle)
{
	if ( handle == (NXEventHandle)0 )
		return;
	if ( handle->var.fd >= 0 )
		close( handle->var.fd  );
	free((void *)handle);
	return;
}

/* post event */
void _NXPostLLEvent(NXEventHandle handle, _NXLLEvent *event)
{
    CHECKEVS;
    ioctl(handle->var.fd, EVSIOLLPE, event);
}

/*
 * Status query - This was designed before the new DriverKit and Event Driver
 * API was stable.  We assumed int flavors instead of char *.  Fortunately,
 * only one flavor was ever implemented in the ioctl() based Event Status
 * driver.  We check for that flavor here and reject all others.
 * If the ioctl() interface should ever have to be extended, please reconsider.
 * You'll be better off porting the new Event Driver into the kernel.
 */
NXEventSystemInfoType NXEventSystemInfo(NXEventHandle handle,
					char *flavor,
					NXEventSystemInfoType evs_info,
					unsigned int *evs_info_cnt)
{
	struct _NXEventSystemInfo_ioctl param;
	
	RCHECKEVS((NXEventSystemInfoType)0);
	if ( ((int)flavor) != __OLD_NX_EVS_DEVICE_INFO
	    && strcmp(flavor, NX_EVS_DEVICE_INFO) != 0)
		return (NXEventSystemInfoType)0;
		
	param.flavor = __OLD_NX_EVS_DEVICE_INFO;
	param.data = evs_info;
	param.size = *evs_info_cnt;
	if ( ioctl(handle->var.fd, EVSIOINFO, &param ) == -1 )
		return (NXEventSystemInfoType)0;
	*evs_info_cnt = param.size;
	return evs_info;
}

/* Keyboard */

void NXResetKeyboard(NXEventHandle handle)
{
    CHECKEVS;
    ioctl(handle->var.fd, EVSIORKBD);
}

void NXSetKeyRepeatInterval(NXEventHandle handle, double rate)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(rate);
    ioctl(handle->var.fd, EVSIOSKR, &i);
}

double NXKeyRepeatInterval(NXEventHandle handle)
{
    int rate;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCKR, &rate);
    return VBLTOSECS(rate);
}

void NXSetKeyRepeatThreshold(NXEventHandle handle, double threshold)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(threshold);
    ioctl(handle->var.fd, EVSIOSIKR, &i);
}

double NXKeyRepeatThreshold(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCIKR, &i);
    return VBLTOSECS(i);
}

NXKeyMapping * NXSetKeyMapping(NXEventHandle handle, NXKeyMapping *keymap)
{
    RCHECKEVS((NXKeyMapping *)0);
    if ( ioctl(handle->var.fd, EVSIOSKM, keymap) == -1 )
    	return (NXKeyMapping *)0;
    return keymap;
}

int NXKeyMappingLength(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0);
    ioctl(handle->var.fd, EVSIOCKML, &i);
    return i;
}

NXKeyMapping * NXGetKeyMapping(NXEventHandle handle, NXKeyMapping *keymap)
{
    RCHECKEVS((NXKeyMapping *)0);
    if ( ioctl(handle->var.fd, EVSIOCKM, keymap) == -1 )
    	return (NXKeyMapping *)0;
    return keymap;
}

/* Mouse */

void NXResetMouse(NXEventHandle handle)
{
    CHECKEVS;
    ioctl(handle->var.fd, EVSIORMS);
}

void NXSetClickTime(NXEventHandle handle, double time)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(time);
    ioctl(handle->var.fd, EVSIOSCT, &i);
}

double NXClickTime(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCCT, &i);
    return VBLTOSECS(i);
}

void NXSetClickSpace(NXEventHandle handle, _NXSize_ *area)
{
    struct evsioPoint p;
    CHECKEVS;
    p.x = (short)(area->width);
    p.y = (short)(area->height);
    ioctl(handle->var.fd, EVSIOSCS, &p);
}

void NXGetClickSpace(NXEventHandle handle, _NXSize_ *area)
{
    struct evsioPoint p;
    CHECKEVS;
    ioctl(handle->var.fd, EVSIOCCS, &p);
    area->width = p.x;
    area->height = p.y;
}

void NXSetMouseScaling(NXEventHandle handle, NXMouseScaling *scaling)
{
    CHECKEVS;
    ioctl(handle->var.fd, EVSIOSMS, scaling);
}

void NXGetMouseScaling(NXEventHandle handle, NXMouseScaling *scaling)
{
    CHECKEVS;
    ioctl(handle->var.fd, EVSIOCMS, scaling);
}


void NXEnableMouseButton(NXEventHandle handle, NXMouseButton button)
{
    int tied;
    int swap;

    switch( button )
    {
	    case NX_OneButton:
		tied = 1;
		swap = 0;
		break;

	    case NX_LeftButton:	/* Menu on left button */
		tied = 0;
		swap = 1;
		break;

	    case NX_RightButton: /* Menu on right button */	
		tied = 0;
		swap = 0;
		break;
		    
	    default:
	    	return;
    }
    ioctl(handle->var.fd, EVSIOSMBT, &tied);
    ioctl(handle->var.fd, EVSIOSMH, &swap);
}

NXMouseButton NXMouseButtonEnabled(NXEventHandle handle)
{
    int swap;
    int tied;

    RCHECKEVS(NX_OneButton);

    ioctl(handle->var.fd, EVSIOCMBT, &tied);
    if ( tied )
    	return NX_OneButton;

    ioctl(handle->var.fd, EVSIOCMH, &swap);
    return swap ? NX_LeftButton : NX_RightButton;
}


/* Wait Cursor */

void NXSetWaitCursorThreshold(NXEventHandle handle, double threshold)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(threshold);
    ioctl(handle->var.fd, EVSIOSWT, &i);
}

double NXWaitCursorThreshold(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCWT, &i);
    return VBLTOSECS(i);
}

void NXSetWaitCursorSustain(NXEventHandle handle, double sustain)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(sustain);
    ioctl(handle->var.fd, EVSIOSWS, &i);
}

double NXWaitCursorSustain(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCWS, &i);
    return VBLTOSECS(i);
}

void NXSetWaitCursorFrameInterval(NXEventHandle handle, double rate)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(rate);
    ioctl(handle->var.fd, EVSIOSWFR, &i);
}

double NXWaitCursorFrameInterval(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCWFR, &i);
    return VBLTOSECS(i);
}

/* Screen Brightness and Auto-dimming */

void NXSetAutoDimThreshold(NXEventHandle handle, double threshold)
{
    int i;
    CHECKEVS;
    i = SECSTOVBL(threshold);
    ioctl(handle->var.fd, EVSIOSADT, &i);
}

double NXAutoDimThreshold(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCADT, &i);
    return VBLTOSECS(i);
}

double NXAutoDimTime(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOGDADT, &i);
    return VBLTOSECS(i);
}

double NXIdleTime(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    if (ioctl(handle->var.fd, EVSIOIDLE, &i) < 0) {
    	i = 0;
    }
    return VBLTOSECS(i);
}

void NXSetAutoDimState(NXEventHandle handle, BOOL dimmed)
{
    int i;
    CHECKEVS;
    i = dimmed;
    ioctl(handle->var.fd, EVSIOSADS, &i);
    return;
}

BOOL NXAutoDimState(NXEventHandle handle)
{
    int i;
    RCHECKEVS(NO);
    ioctl(handle->var.fd, EVSIOCADS, &i);
    return (i) ? YES : NO;
}

void NXSetScreenBrightness(NXEventHandle handle, double brightness)
{
    int i;
    CHECKEVS;
    i = (int) (brightness * BRIGHT_MAX);
    ioctl(handle->var.fd, EVSIOSB, &i);
}

double NXScreenBrightness(NXEventHandle handle)
{
    int i;
    RCHECKEVS(1.0);
    ioctl(handle->var.fd, EVSIOCB, &i);
    return ((double)i) / BRIGHT_MAX;
}

void NXSetAutoDimBrightness(NXEventHandle handle, double brightness)
{
    int i;
    CHECKEVS;
    i = (int) (brightness * BRIGHT_MAX);
    ioctl(handle->var.fd, EVSIOSADB, &i);
}

double NXAutoDimBrightness(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCADB, &i);
    return ((double)i)/BRIGHT_MAX;
}

#if !defined __DYNAMIC__

/* Speaker Volume */
void NXSetCurrentVolume(NXEventHandle handle, double volume)
{
    int i;
    CHECKEVS;
    i = (1.0 - volume) * VOLUME_MAX;
    /* evs driver will clip integer to 0-43 inclusive */
    ioctl(handle->var.fd, EVSIOSA, &i);	
}

double NXCurrentVolume(NXEventHandle handle)
{
    int i;
    RCHECKEVS(0.0);
    ioctl(handle->var.fd, EVSIOCA, &i);
    return 1.0-(((double)i)/VOLUME_MAX);
}

#endif /* !defined __DYNAMIC__ */

#import <driverkit/return.h>

/*
 * Generic entry points, not supported by old driver.
 * These are here to cover unresolved references, so we don't have to
 * re-tweak and roll libsys as the new driver is ported to existing
 * architectures.  This code is not compiled when the new driver is installed.
 */
int NXEvSetParameterInt(NXEventHandle handle,
			char *parameterName,
			unsigned int *parameterArray,
			unsigned int count)
{	return IO_R_UNSUPPORTED; }

int NXEvSetParameterChar(NXEventHandle handle,
			char *parameterName,
			unsigned char *parameterArray,
			unsigned int count)
{	return IO_R_UNSUPPORTED; }

int NXEvGetParameterInt(NXEventHandle handle,
			char *parameterName,
			unsigned int maxCount,
			unsigned int *parameterArray,
			unsigned int *returnedCount)
{	return IO_R_UNSUPPORTED; }

int NXEvGetParameterChar(NXEventHandle handle,
			char *parameterName,
			unsigned int maxCount,
			unsigned char *parameterArray,
			unsigned int *returnedCount)
{	return IO_R_UNSUPPORTED; }

#endif	/* ! _NeXT_MACH_EVENT_DRIVER_ */
#endif	/* DRIVER_PRIVATE */

