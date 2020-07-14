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
#ifndef	_bootstrap
#define	_bootstrap

/* Module bootstrap */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#ifndef	mig_external
#define mig_external extern
#endif

#include <mach/std_types.h>
#include <servers/bootstrap_defs.h>

/* Routine bootstrap_check_in */
mig_external kern_return_t bootstrap_check_in (
	port_t bootstrap_port,
	name_t service_name,
	port_all_t *service_port);

/* Routine bootstrap_register */
mig_external kern_return_t bootstrap_register (
	port_t server_port,
	name_t service_name,
	port_t service_port);

/* Routine bootstrap_look_up */
mig_external kern_return_t bootstrap_look_up (
	port_t bootstrap_port,
	name_t service_name,
	port_t *service_port);

/* Routine bootstrap_look_up_array */
mig_external kern_return_t bootstrap_look_up_array (
	port_t bootstrap_port,
	name_array_t service_names,
	unsigned int service_namesCnt,
	port_array_t *service_ports,
	unsigned int *service_portsCnt,
	boolean_t *all_services_known);

/* Routine bootstrap_status */
mig_external kern_return_t bootstrap_status (
	port_t bootstrap_port,
	name_t service_name,
	boolean_t *service_active);

/* Routine bootstrap_info */
mig_external kern_return_t bootstrap_info (
	port_t bootstrap_port,
	name_array_t *service_names,
	unsigned int *service_namesCnt,
	name_array_t *server_names,
	unsigned int *server_namesCnt,
	bool_array_t *service_active,
	unsigned int *service_activeCnt);

/* Routine bootstrap_subset */
mig_external kern_return_t bootstrap_subset (
	port_t bootstrap_port,
	port_t requestor_port,
	port_t *subset_port);

/* Routine bootstrap_create_service */
mig_external kern_return_t bootstrap_create_service (
	port_t bootstrap_port,
	name_t service_name,
	port_t *service_port);

#endif	_bootstrap
