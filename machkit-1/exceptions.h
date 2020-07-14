/*
 * machkit exceptions.h
 * Copyright 1991, NeXT Computer, Inc.
 */

typedef enum  {
	NX_MACH_KIT_EXCEPTION_BASE = 10000,
	NX_portInvalidException = 10001,
	NX_restrictionEnforcedException = 10010,
	NX_referenceAlreadyFreeException = 10020,
	NX_MACH_KIT_LAST_EXCEPTION = 10999
} NXMachKitException;

