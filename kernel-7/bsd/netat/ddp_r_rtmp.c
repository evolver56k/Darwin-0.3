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

/*--------------------------------------------------------------------------
 * Router RTMP protocol functions: 
 *
 * This file contains Routing specifics to handle RTMP packets and
 * the maintenance of the routing table through....
 *
 * The entry point for the rtmp input in ddp is valid only when we're
 * running in router mode. 
 *
 *
 *-------------------------------------------------------------------------
 *
 *      Copyright (c) 1994, 1996-1998 Apple Computer, Inc.
 *
 *      The information contained herein is subject to change without
 *      notice and  should not be  construed as a commitment by Apple
 *      Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *      for any errors that may appear.
 *
 *      Confidential and Proprietary to Apple Computer, Inc.
 */

#include <sysglue.h>
#include <sys/malloc.h>
#include <at/appletalk.h>
#include <lap.h>
#include <llap.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <rtmp.h>
#include <at/at_lap.h>
#include <at_elap.h>
#include <at_ddp.h>
#include <at_zip.h>
#include <routing_tables.h>
#include <at_aurp.h>
extern void (*ddp_AURPsendx)();
extern at_if_t *aurp_ifID;

/*DEBUG ONLY */
static int dump_counter =0;
/*DEBUG ONLY */

static kern_err_t	ke;
gbuf_t *rtmp_prep_new_packet();

void rtmp_timeout();
void rtmp_send_port();
void rtmp_dropper();
extern RT_entry *rt_blookup ();

extern at_if_t *ifID_table[];
extern char errstr[]; /* formatting of Router Errors */
extern at_state_t	*at_statep;

extern pktsIn,pktsOut,pktsDropped, pktsHome;
extern gref_t *at_delay_gref;
extern gbuf_t *at_delay_m;
extern int (*at_delay_func)();
extern int at_delay_errno;
extern char at_delay_flag;
extern void *atalk_timeout();
extern void atalk_untimeout();
static void *dropper_tmo = 0;

extern short ErrorRTMPoverflow;
extern short ErrorZIPoverflow;
extern atlock_t ddpinp_lock;

static void rtmp_update();
static void rtmp_request();

/* ddprunning_flag was used to make sure that shutdown did not occur
   while a packet was being processed.  However looping while waiting
   for ddprunning_flag to be cleared has resulted in the system hanging.
   Therefore this variable has been temporarily removed, until such time
   as we can verify that there is no condition under which it will cause
   trouble.
*/
#ifdef CHECK_DDPR_FLAG
int ddprunning_flag;  /* temporarily removed using CHECK_DDPR_FLAG */
#endif

/*
 * rtmp_router_input: function called by DDP (in router mode) to handle
 *                    all incoming RTMP packets. Listen to the RTMP socket
 *                    for all the connected ports.
 *					  Switch to the relevant rtmp functions.
 */

void
rtmp_router_input (mp, ifID)
register gbuf_t  *mp;
register at_if_t        *ifID;
{
        register at_ddp_t       *ddp = (at_ddp_t *)gbuf_rptr(mp);
                                /* NOTE: there is an assumption here that the
                                 * DATA follows the header. */

		register at_net_al OurNet;
		register at_node OurNode;
		register at_net_al DstNet;
		register at_node DstNode;
		short tuples;
		RT_entry *Entry;

    	if (!ifID || ! IFID_VALID(ifID) || (ifID->ifRoutingState < PORT_ACTIVATING)) {
                gbuf_freem(mp);
                return;
        }


		OurNet = NET_VALUE(ifID->ifThisNode.atalk_net);
 		OurNode =ifID->ifThisNode.atalk_node;


        if (gbuf_type(mp) != MSG_DATA) {

                /* If this is a M_ERROR message, DDP is shutting down,
                 * nothing to do here...If it's something else, we don't
                 * understand what it is
                 */
		dPrintf(D_M_RTMP, D_L_WARNING, ("rtmp_router_input: Not an M_DATA type\n"));
                gbuf_freem(mp);
                return;
        }

		DstNet = NET_VALUE(ddp->dst_net);
		DstNode = ddp->dst_node;

		/* check the kind of RTMP packet we received */

		switch (ddp->type) {

			case RTMP_DDP_TYPE:
				
				tuples = gbuf_len(mp) - DDP_X_HDR_SIZE - RTMP_IDLENGTH;
			/*
			 * we need to make sure that the size of 'tuples' is
			 * not less than or equal to 0 due to a bad packet
			 */
				if (tuples <= 0) {
					gbuf_freem(mp);
					break;
				}

				if (tuples % 3)	{/* not a valid RTMP data packet */
						gbuf_freem(mp);
						dPrintf(D_M_RTMP, D_L_WARNING,
							("rtmp_input: bad number of tuple in RTMP packet\n"));
						return;
				}

				tuples = tuples / 3;

				rtmp_update(ifID, (at_rtmp *)ddp->data, tuples);
				gbuf_freem(mp);
					
				break;

			case RTMP_REQ:

				/* we should treat requests a bit differently.
				 * - if the request if not for the port, route it and also respond
				 *   for this port if not locally connected.
				 * - if the request for this port, then just respond to it.
				 */

				if (!ROUTING_MODE) {
					gbuf_freem(mp);
					return;
				}
				if (DstNode == 255) {
					if (((DstNet >= CableStart) && (DstNet <= CableStop)) ||
						DstNet == 0) {
							rtmp_request(ifID, ddp);
							gbuf_freem(mp);
							return;
					}
					else {
							/* check if directly connected port */

							if ((Entry = rt_blookup(DstNet)) &&  (Entry->NetDist == 0)) {
									dPrintf(D_M_RTMP, D_L_WARNING, 
									("rtmp_router_input: request for %d.%d, port %d\n",
										DstNet, DstNode, Entry->NetPort));
									rtmp_request(ifID_table[Entry->NetPort], ddp);
									gbuf_freem(mp);
									return;
							}
							else {
								dPrintf(D_M_RTMP, D_L_WARNING,
					 			("rtmp_router_input: RTMP packet received for %d.%d, also forward\n",
					 			NET_VALUE(ddp->dst_net),ddp->dst_node));
								routing_needed(mp, ifID, TRUE);
								return;
							}
					}
				}
				else {

					if ((DstNode == OurNode) && (DstNet == OurNet)) {
							rtmp_request(ifID, ddp);
							gbuf_freem(mp);
							return;
					}
					else  {
					dPrintf(D_M_RTMP, D_L_WARNING,
					 ("rtmp_router_input: RTMP packet received for %d.%d, forward\n",
					 NET_VALUE(ddp->dst_net), ddp->dst_node));
							routing_needed(mp, ifID, TRUE);
					}
				}

				break;

			default:

				dPrintf(D_M_RTMP, D_L_WARNING,
					("rtmp_input: RTMP packet type=%d, route it\n", ddp->type));
				routing_needed(mp, ifID, TRUE);
				break;

		}	

				
}

/*
 * rtmp_update:
 *
 */

static
void rtmp_update(ifID, rtmp, tuple_nb)
register at_if_t 	*ifID;
register at_rtmp 	*rtmp;
register short	tuple_nb;
{
		register int PortFlags = ifID->ifFlags;
		register at_rtmp_tuple *FirstTuple =  (at_rtmp_tuple *)&rtmp->at_rtmp_id[1];
		register at_rtmp_tuple *SecondTuple = (at_rtmp_tuple *)&rtmp->at_rtmp_id[4];
		RT_entry NewRoute, *CurrentRoute;

		register u_char SenderNodeID = rtmp->at_rtmp_id[0];
		char *TuplePtr;
		short state;


		/* Make sure this an AppleTalk node sending us the RTMP packet */

		if (rtmp->at_rtmp_id_length  != 8) {
			dPrintf(D_M_RTMP, D_L_WARNING,
				("rtmp_update : RTMP ID not as expected Net=%d L=x%x\n", 
				NET_VALUE(rtmp->at_rtmp_this_net), rtmp->at_rtmp_id_length));
			return;
		}

		/*
		 * If the port is activating, only take the Network range from the
         * the RTMP packet received.
		 * Check if there is a conflict with our seed infos.
         */

		if (ifID->ifRoutingState == PORT_ACTIVATING) {
			if (PortFlags & RTR_XNET_PORT) {
				if ((PortFlags & RTR_SEED_PORT) &&
					((CableStart != TUPLENET(FirstTuple)) ||
					(CableStop != TUPLENET(SecondTuple)))) {
						ifID->ifRoutingState = PORT_ERR_SEED;
						ke.errno 	= KE_CONF_SEED_RNG;
						ke.port1 	= ifID->ifPort;
						strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
						ke.net 		=  NET_VALUE(rtmp->at_rtmp_this_net);
						ke.node     = SenderNodeID;
						ke.netr1b 	= TUPLENET(FirstTuple);
						ke.netr1e 	= TUPLENET(SecondTuple);
						ke.netr2b 	= CableStart;
						ke.netr2e	= CableStop;
						RouterError(ifID->ifPort, ERTR_SEED_CONFLICT);
						return;
				}
				CableStart = TUPLENET(FirstTuple);
				CableStop  = TUPLENET(SecondTuple); 
/*
				dPrintf(D_M_RTMP, D_L_INFO,
					("rtmp_update: Port #%d activating, set Cable %d-%d\n",
					ifID->ifPort, CableStart, CableStop));
*/
			}
			else { /* non extended cable */
				if ((PortFlags & RTR_SEED_PORT) &&
				(ifID->ifThisCableEnd != *(at_net_al*)&rtmp->at_rtmp_this_net)) {
						ke.errno 	= KE_CONF_SEED1;
						ke.port1 	= ifID->ifPort;
						strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
						ke.net 		=  NET_VALUE(rtmp->at_rtmp_this_net);
						ke.node     = SenderNodeID;
						ke.netr1e 	= ifID->ifThisCableEnd;
						ifID->ifRoutingState = PORT_ERR_SEED;
						RouterError(ifID->ifPort, ERTR_SEED_CONFLICT);
						return;
				}
				CableStop =  *(at_net_al *)&rtmp->at_rtmp_this_net;
				CableStart = 0;
				dPrintf(D_M_RTMP, D_L_INFO,
					("rtmp_update: Port #%d NONX activating, set Cable %d-%d\n",
					ifID->ifPort, CableStart, CableStop));
			}

		}

		/*
		 * Perform a few sanity checks on the received RTMP data packet
         */

		if ((PortFlags & RTR_XNET_PORT) && (tuple_nb >= 2)) {

			/* The first tuple must be extended */

			if (! TUPLERANGE(FirstTuple)) {
				dPrintf(D_M_RTMP, D_L_WARNING,
				("rtmp_update: bad range value in 1st tuple =%d\n",
					 TUPLERANGE(FirstTuple)));
				return;
			}

			if (PortFlags & RTR_SEED_PORT)
				if ((TUPLENET(FirstTuple) != CableStart) ||
					(TUPLENET(SecondTuple) != CableStop)) {
					dPrintf(D_M_RTMP, D_L_WARNING, ("rtmp_update: conflict on Seed Port\n"));
					ifID->ifRoutingState = PORT_ERR_CABLER;
					ke.errno 	= KE_CONF_SEED_NODE;
					ke.port1 	= ifID->ifPort;
					strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
					ke.net 		=  NET_VALUE(rtmp->at_rtmp_this_net);
					ke.node     = SenderNodeID;
					ke.netr1b 	= TUPLENET(FirstTuple);
					ke.netr1e 	= TUPLENET(SecondTuple);
					ke.netr2b 	= CableStart;
					ke.netr2e	= CableStop;
					RouterError(ifID->ifPort, ERTR_CABLE_CONFLICT);	
					return;
				}

			/* check that the tuple matches the range */

			if ((TUPLENET(SecondTuple) < TUPLENET(FirstTuple)) ||
				(TUPLENET(FirstTuple) == 0) ||
				(TUPLENET(FirstTuple) >= DDP_STARTUP_LOW) ||
				(TUPLENET(SecondTuple) == 0) ||
				(TUPLENET(SecondTuple) >= DDP_STARTUP_LOW)) {
					
					/*
					 * IS THIS NON-FATAL?????
					 */
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_update: STARTUP RANGE!!! 1st %d-%d\n",
						TUPLENET(FirstTuple), TUPLENET(SecondTuple)));
					ifID->ifRoutingState = PORT_ERR_STARTUP;
					ke.errno 	= KE_SEED_STARTUP;
					ke.port1 	= ifID->ifPort;
					strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
					ke.net 		=  NET_VALUE(rtmp->at_rtmp_this_net);
					ke.node     = SenderNodeID;
					RouterError(ifID->ifPort, ERTR_CABLE_STARTUP);
					return;
			}

			if (TUPLEDIST(FirstTuple) != 0) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						 ("rtmp_update: Invalid distance in 1st tuple\n"));
					return;
			}

			if (rtmp->at_rtmp_id[6] != RTMP_VERSION_NUMBER) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_update: Invalid RTMP version = x%x\n",
						rtmp->at_rtmp_id[6]));
					return;
			}

			
		}
		else {	/* non extended interface or problem in tuple*/
		
			if (PortFlags & RTR_XNET_PORT) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_update: invalid number of tuple for X-net\n"));
					return;
			}

			if (TUPLENET(FirstTuple) == 0) { /* non extended RTMP data */

				if (rtmp->at_rtmp_id[3] > RTMP_VERSION_NUMBER) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_update: Invalid non extended RTMP version\n"));
					return;
				}

			}
			else {
				dPrintf(D_M_RTMP, D_L_WARNING,
					("rtmp_update: version 1.0 non Xtended net not supported\n"));
				ifID->ifRoutingState = PORT_ERR_BADRTMP;
				ke.errno 	= KE_BAD_VER;
				ke.rtmp_id = rtmp->at_rtmp_id[6];
				ke.net 		=  NET_VALUE(rtmp->at_rtmp_this_net);
				ke.node     = SenderNodeID;
				RouterError(ifID->ifPort, ERTR_RTMP_BAD_VERSION);
				return;
			}			
		}

				
		NewRoute.NextIRNet  = NET_VALUE(rtmp->at_rtmp_this_net);
		NewRoute.NextIRNode = SenderNodeID;
		NewRoute.NetPort	= ifID->ifPort;

		/* 
		 * Process the case where a non-seed port needs to acquire the right
		 * information.
         */

		if (!(PortFlags & RTR_SEED_PORT) && (ifID->ifRoutingState == PORT_ACTIVATING)) {
			dPrintf(D_M_RTMP_LOW, D_L_INFO,
				("rtmp_update: Port# %d, set non seed cable %d-%d\n",
				ifID->ifPort, TUPLENET(FirstTuple), TUPLENET(SecondTuple)));

			if (PortFlags & RTR_XNET_PORT) {
				NewRoute.NetStart = TUPLENET(FirstTuple);
				NewRoute.NetStop = TUPLENET(SecondTuple);
				ifID->ifThisCableStart = TUPLENET(FirstTuple);
				ifID->ifThisCableEnd  = TUPLENET(SecondTuple);
				
				
			}
			else {

				NewRoute.NetStart = 0;
				NewRoute.NetStop  = NET_VALUE(rtmp->at_rtmp_this_net);
				ifID->ifThisCableStart = NET_VALUE(rtmp->at_rtmp_this_net);
				ifID->ifThisCableEnd  = NET_VALUE(rtmp->at_rtmp_this_net);
			}
			/*
			 * Now, check if we already know this route, or we need to add it
			 * (or modify it in the table accordingly)
			 */

			if ((CurrentRoute = RT_LOOKUP(NewRoute)) &&
					(CurrentRoute->NetStop  == NewRoute.NetStop) &&
					(CurrentRoute->NetStart == NewRoute.NetStart)) {
/*LD 7/31/95 tempo########*/
				if (NewRoute.NetPort != CurrentRoute->NetPort) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_update: port# %d, not the port we waited for %d\n",
							ifID->ifPort, CurrentRoute->NetPort));
					/* propose to age the entry we know... */
					
					state = CurrentRoute->EntryState & 0x0F;
					/* if entry has been updated recently, just clear the UPDATED 
					   bit. if bit not set, then we can age the entry */
					if (state)
						if (CurrentRoute->EntryState & RTE_STATE_UPDATED) {
							CurrentRoute->EntryState &= ~RTE_STATE_UPDATED; 
						}
						else {
							state  = state >> 1 ;	/* decrement state */
						}

					CurrentRoute->EntryState = (CurrentRoute->EntryState & 0xF0) | state;
				}
			}

			else { /* add the new route */

				dPrintf(D_M_RTMP, D_L_INFO,
					("rtmp_update: P# %d, 1st tuple route not known, add %d-%d\n",
					ifID->ifPort, NewRoute.NetStart, NewRoute.NetStop));

				NewRoute.EntryState = RTE_STATE_GOOD|RTE_STATE_UPDATED;
				NewRoute.NetDist	= 0;

				if (rt_insert(NewRoute.NetStop, NewRoute.NetStart, 0, 
				 		  0, NewRoute.NetDist, NewRoute.NetPort,
						  NewRoute.EntryState) == (RT_entry *)NULL)
 
					ErrorRTMPoverflow = 1;
			}	

		}

		if (ifID->ifRoutingState == PORT_ACTIVATING) {
			dPrintf(D_M_RTMP, D_L_INFO,
	  		("rtmp_update: port activating, ignoring remaining tuples\n"));
			return;
		}

		/*
		 * Process all the tuples against our routing table
		 */

		TuplePtr = (char *)FirstTuple;


		while (tuple_nb-- > 0) {


			if (TUPLEDIST(TuplePtr) == NOTIFY_N_DIST) {
				dPrintf(D_M_RTMP, D_L_INFO,
					("rtmp_update: Port# %d, Tuple with Notify Neighbour\n",
					ifID->ifPort));
				NewRoute.NetDist = NOTIFY_N_DIST;
				NewRoute.EntryState = RTE_STATE_BAD;
			}
			else {
				NewRoute.NetDist = TUPLEDIST(TuplePtr) + 1;
				NewRoute.EntryState = RTE_STATE_GOOD;
				NewRoute.EntryState = RTE_STATE_GOOD|RTE_STATE_UPDATED;
			}


			if (TUPLERANGE(TuplePtr)) {	/* Extended Tuple */


				NewRoute.NetStart = TUPLENET(TuplePtr);
				TuplePtr += 3;
				NewRoute.NetStop  = TUPLENET((TuplePtr));
				TuplePtr += 3;
				tuple_nb--;

				if ((NewRoute.NetDist  == 0) ||
					(NewRoute.NetStart == 0) ||
					(NewRoute.NetStop  == 0) ||
					(NewRoute.NetStop  < NewRoute.NetStart) ||
					(NewRoute.NetStart >= DDP_STARTUP_LOW) ||
					(NewRoute.NetStop  >= DDP_STARTUP_LOW)) {
					
						dPrintf(D_M_RTMP, D_L_WARNING,
							("rtmp_update: P# %d, non valid xtuple received [%d-%d]\n",
							ifID->ifPort, NewRoute.NetStart, NewRoute.NetStop));

						continue;
				}
	
			}
			else {		/* Non Extended Tuple */

				NewRoute.NetStart = 0;
				NewRoute.NetStop  = TUPLENET(TuplePtr);
				
				TuplePtr += 3;
			
				if ((NewRoute.NetDist  == 0) ||
					(NewRoute.NetStop  == 0) ||
					(NewRoute.NetStop  >= DDP_STARTUP_LOW)) {

						dPrintf(D_M_RTMP, D_L_WARNING,
							("rtmp_update: P# %d, non valid tuple received [%d]\n",
							ifID->ifPort, NewRoute.NetStop));

						continue;
				}
			}

			if (CurrentRoute = RT_LOOKUP(NewRoute)) { /* found something... */

				if (NewRoute.NetDist < 16 || NewRoute.NetDist == NOTIFY_N_DIST ) {

					/*
					 * Check if the definition of the route changed
					 */

					if (NewRoute.NetStop != CurrentRoute->NetStop ||
						NewRoute.NetStart != CurrentRoute->NetStart) {
						
							if (NewRoute.NetStop == CurrentRoute->NetStop &&
								NewRoute.NetStop == CurrentRoute->NetStart &&
								NewRoute.NetStart == 0)
				
								NewRoute.NetStart = NewRoute.NetStop;

							else if (NewRoute.NetStop == CurrentRoute->NetStop &&
									 NewRoute.NetStart == NewRoute.NetStop &&
									 CurrentRoute->NetStart == 0) {
								dPrintf(D_M_RTMP, D_L_WARNING,
								("rtmp_update: Range %d-%d has changed to %d-%d Dist=%d\n",
								CurrentRoute->NetStart, CurrentRoute->NetStop,
								NewRoute.NetStart, NewRoute.NetStop, NewRoute.NetDist));
									NewRoute.NetStart = 0;
								}

								else {
									dPrintf(D_M_RTMP, D_L_WARNING,
										("rtmp_update: Net Conflict Cur=%d, New=%d\n", 
										CurrentRoute->NetStop, NewRoute.NetStop));
									CurrentRoute->EntryState = 
									(CurrentRoute->EntryState & 0xF0) | RTE_STATE_BAD; 
									continue;

								}
					}

					/*
					 * If we don't know the associated zones
					 */

					if (!RT_ALL_ZONES_KNOWN(CurrentRoute)) {

						dPrintf(D_M_RTMP_LOW, D_L_INFO,
							("rtmp_update: Zone unknown for %d-%d state=0x%x\n",
								CurrentRoute->NetStart, CurrentRoute->NetStop,
								CurrentRoute->EntryState));

						/* set the flag in the ifID structure telling
					     * that a scheduling of Zip Query is needed.
						 */

						ifID->ifZipNeedQueries = 1;
						continue;
					}

					if (((CurrentRoute->EntryState & 0x0F) <= RTE_STATE_SUSPECT) &&
							NewRoute.NetDist != NOTIFY_N_DIST) {

						dPrintf(D_M_RTMP, D_L_INFO,
							("rtmp_update: update suspect entry %d-%d State=%d\n",
							NewRoute.NetStart, NewRoute.NetStop,
							(CurrentRoute->EntryState & 0x0F)));

						if (NewRoute.NetDist <= CurrentRoute->NetDist) {
							CurrentRoute->NetDist 	 = NewRoute.NetDist;
							CurrentRoute->NetPort 	 = NewRoute.NetPort;
							CurrentRoute->NextIRNode = NewRoute.NextIRNode;
							CurrentRoute->NextIRNet  = NewRoute.NextIRNet;
							CurrentRoute->EntryState = 
								(CurrentRoute->EntryState & 0xF0) | 
								(RTE_STATE_GOOD|RTE_STATE_UPDATED); 
						}
						continue;
					}
					else {

						if (NewRoute.NetDist == NOTIFY_N_DIST) {
	
							CurrentRoute->EntryState = 
								(CurrentRoute->EntryState & 0xF0) | RTE_STATE_SUSPECT; 
							CurrentRoute->NetDist = NOTIFY_N_DIST;
							continue;
						}
					}

				}


				if ((NewRoute.NetDist <= CurrentRoute->NetDist) && (NewRoute.NetDist <16)) { 

					 /* Found a shorter or more recent Route,
					  * Replace with the New entryi
					  */

						CurrentRoute->NetDist    = NewRoute.NetDist;
						CurrentRoute->NetPort    = NewRoute.NetPort;
						CurrentRoute->NextIRNode = NewRoute.NextIRNode;
						CurrentRoute->NextIRNet  = NewRoute.NextIRNet;
						CurrentRoute->EntryState |= RTE_STATE_UPDATED; 

					/* Can we consider now that the entry is updated? */	
					dPrintf(D_M_RTMP_LOW, D_L_INFO,
						("rtmp_update: Shorter route found %d-%d, update\n",
						NewRoute.NetStart, NewRoute.NetStop));

			if (ddp_AURPsendx && (aurp_ifID->ifFlags & AT_IFF_AURP))
				ddp_AURPsendx(AURPCODE_RTUPDATE,
					(void *)&NewRoute, AURPEV_NetDistChange);
				}
			}
			else { /* no entry found */

				if (NewRoute.NetDist < 16 && NewRoute.NetDist != NOTIFY_N_DIST &&
					NewRoute.NextIRNet >= ifID->ifThisCableStart &&
					NewRoute.NextIRNet <= ifID->ifThisCableEnd) {
				
					NewRoute.EntryState = (RTE_STATE_GOOD|RTE_STATE_UPDATED);

					dPrintf(D_M_RTMP_LOW, D_L_INFO,
						("rtmp_update: NewRoute %d-%d Tuple #%d\n",
						NewRoute.NetStart, NewRoute.NetStop, tuple_nb));

					ifID->ifZipNeedQueries = 1;

					if (rt_insert(NewRoute.NetStop, NewRoute.NetStart, NewRoute.NextIRNet, 
				 		  NewRoute.NextIRNode, NewRoute.NetDist, NewRoute.NetPort,
						  NewRoute.EntryState) == (RT_entry *)NULL)
						ErrorRTMPoverflow = 1;

			else if (ddp_AURPsendx && (aurp_ifID->ifFlags & AT_IFF_AURP))
				ddp_AURPsendx(AURPCODE_RTUPDATE,
					(void *)&NewRoute, AURPEV_NetAdded);
				}		
			}

						

	} /* end of main while */			
	ifID->ifRouterState = ROUTER_UPDATED;
	if (ifID->ifZipNeedQueries) 
		zip_send_queries(ifID, 0, 0xFF);

/*
	ifID->tmo_1 = atalk_timeout(rtmp_timeout, (caddr_t) ifID, 20*SYS_HZ);
*/

}
/* The RTMP validity timer expired, we need to update the
 * state of each routing entry in the table
 * because there is only one validity timer and it is always running,
 * we can't just age all the entries automatically, as we might be
 * aging entries that were just updated. So, when an entry is updated,
 * the RTE_STATE_UPDATED bit is set and when the aging routine is called
 * it just resets this bit if it is set, only if it is not set will the
 * route actually be aged.
 * Note there are 4 states for an entry, the state is decremented until
 * it reaches the bad state. At this point, the entry is removed
 *
 *      RTE_STATE_GOOD   :  The entry was valid (will be SUSPECT)
 *      RTE_STATE_SUSPECT:  The entry was suspect (can still be used for routing)
 *      RTE_STATE_BAD    : 	The entry was bad and is now deleted 
 *      RTE_STATE_UNUSED :  Unused or removed entry in the table
 */

void rtmp_timeout(ifID)
register at_if_t        *ifID;
{
		register u_char state;
		register unsigned int s;
		short i;
		RT_entry *en = &RT_table[0];

		if (ifID->ifRoutingState < PORT_ONLINE) 
			return;

		/* for multihoming mode, we use ifRouterState to tell if there
           is a router out there, so we know when to use cable multicast */
		if (ifID->ifRouterState > NO_ROUTER)
			ifID->ifRouterState--;

		ATDISABLE(s, ddpinp_lock);
		for (i = 0 ; i < RT_MAXENTRY; i++,en++) {

			/* we want to age "learned" nets, not directly connected ones */
			state  = en->EntryState & 0x0F;


			if (state > RTE_STATE_UNUSED && 
			   !(en->EntryState & RTE_STATE_PERMANENT) && en->NetStop && 
			   en->NetDist && en->NetPort == ifID->ifPort) {

					/* if entry has been updated recently, just clear the UPDATED 
					   bit. if bit not set, then we can age the entry */
				if (en->EntryState & RTE_STATE_UPDATED) {
					en->EntryState &= ~RTE_STATE_UPDATED;
					continue;
				}
				else
					state  = state >> 1 ;	/* decrement state */

				if (state == RTE_STATE_UNUSED)	{/* was BAD, needs to delete */
					dPrintf(D_M_RTMP, D_L_INFO,
						("rtmp_timeout: Bad State for %d-%d (e#%d): remove\n",
							en->NetStart, en->NetStop, i));

				if (ddp_AURPsendx && (aurp_ifID->ifFlags & AT_IFF_AURP))
					ddp_AURPsendx(AURPCODE_RTUPDATE,
						(void *)en, AURPEV_NetDeleted);
	
					/* then clear the bit in the table concerning this entry.
					If the zone Count reaches zero, remove the entry */

					zt_remove_zones(en->ZoneBitMap);		
					
					RT_DELETE(en->NetStop, en->NetStart);
				}
				else {
					en->EntryState = (en->EntryState & 0xF0) | state;
					dPrintf(D_M_RTMP, D_L_INFO, ("Change State for %d-%d to %d (e#%d)\n",
							en->NetStart, en->NetStop, state, i));
				}
			}
		}
		ATENABLE(s, ddpinp_lock);
		ifID->tmo_1 = atalk_timeout(rtmp_timeout, (caddr_t) ifID, 20*SYS_HZ);
		
}
			 
/*
 * rtmp_prep_new_packet: allocate a ddp packet for RTMP use (reply to a RTMP request or
 *                  Route Data Request, or generation of RTMP data packets.
 *				    The ddp header is filled with relevant information, as well as
 *                  the beginning of the rtmp packet with the following info:
 *						Router's net number  (2bytes)
 *						ID Length = 8		 (1byte)
 *						Router's node ID	 (1byte)
 *						Extended Range Start (2bytes)
 *						Range + dist (0x80)  (1byte)
 *						Extended Range End   (2bytes)
 *						Rtmp version (0x82)  (1byte)
 *
 */				
		
gbuf_t *rtmp_prep_new_packet (ifID, DstNet, DstNode, socket)
register at_if_t        *ifID;
register at_net DstNet;
register uchar DstNode;
register char socket;

{
	gbuf_t		*m;
	register at_ddp_t	*ddp;
	register char * rtmp_data;
	
	if ((m = gbuf_alloc(AT_WR_OFFSET+1024, PRI_HI)) == NULL) {
		dPrintf(D_M_RTMP, D_L_WARNING, ("rtmp_new_packet: Can't allocate mblock\n"));
		return ((gbuf_t *)NULL);
	}

	gbuf_rinc(m,AT_WR_OFFSET); 
	gbuf_wset(m,DDP_X_HDR_SIZE + 10); 
	ddp = (at_ddp_t *)(gbuf_rptr(m));

	/*
	 * Prepare the DDP header of the new packet 
	 */


	ddp->unused = ddp->hopcount = 0;

	UAS_ASSIGN(ddp->checksum, 0);

	NET_NET(ddp->dst_net, DstNet);
	ddp->dst_node =  DstNode;
	ddp->dst_socket = socket;

	NET_NET(ddp->src_net, ifID->ifThisNode.atalk_net);
	ddp->src_node = ifID->ifThisNode.atalk_node;
	ddp->src_socket = RTMP_SOCKET;
	ddp->type = RTMP_DDP_TYPE;

	/*
     * Prepare the RTMP header (Router Net, ID, Node and Net Tuple
	 * (this works only if we are on an extended net)
	 */

	rtmp_data = ddp->data;
	
	*rtmp_data++ = (NET_VALUE(ifID->ifThisNode.atalk_net) & 0xff00) >> 8;
	*rtmp_data++ = NET_VALUE(ifID->ifThisNode.atalk_net) & 0x00ff ;
	*rtmp_data++ = 8;	
	*rtmp_data++ = (uchar)ifID->ifThisNode.atalk_node;
	*rtmp_data++ = (CableStart & 0xff00) >> 8;
	*rtmp_data++ = CableStart & 0x00ff ;
	*rtmp_data++ = 0x80;	/* first tuple, so distance is always zero */
	*rtmp_data++ = (CableStop & 0xff00) >> 8;
	*rtmp_data++ = CableStop & 0x00ff ;
	*rtmp_data++ = RTMP_VERSION_NUMBER;

	return (m);


}
int rtmp_r_find_bridge(ifID, orig_ddp)
register at_if_t    *ifID;
register at_ddp_t 	*orig_ddp;

{
	gbuf_t		*m;
	register int		size, status;
	register at_ddp_t	*ddp;
	register char * rtmp_data;
	RT_entry *Entry;


	/* find the bridge for the querried net */

	Entry = rt_blookup(NET_VALUE(orig_ddp->dst_net));

	if (Entry == NULL) {
		dPrintf(D_M_RTMP, D_L_WARNING, ("rtmp_r_find_bridge: no info for net %d\n",
			 NET_VALUE(orig_ddp->dst_net)));
		return (1);
	}

	
	size = DDP_X_HDR_SIZE + 10 ;
	if ((m = gbuf_alloc(AT_WR_OFFSET+size, PRI_HI)) == NULL) {
		dPrintf(D_M_RTMP, D_L_WARNING, ("rtmp_r_find_bridge: Can't allocate mblock\n"));
		return (ENOBUFS);
	}

	gbuf_rinc(m,AT_WR_OFFSET);
	gbuf_wset(m,size);
	ddp = (at_ddp_t *)(gbuf_rptr(m));

	/*
	 * Prepare the DDP header of the new packet 
	 */

	ddp->unused = ddp->hopcount = 0;

	DDPLEN_ASSIGN(ddp, size);
	UAS_ASSIGN(ddp->checksum, 0);

	NET_NET(ddp->dst_net, orig_ddp->src_net);
	ddp->dst_node =  orig_ddp->src_node;
	ddp->dst_socket = orig_ddp->src_socket;

	NET_ASSIGN(ddp->src_net, Entry->NextIRNet);
	ddp->src_node = Entry->NextIRNode;
	ddp->src_socket = RTMP_SOCKET;
	ddp->type = RTMP_DDP_TYPE;

	/*
     * Prepare the RTMP header (Router Net, ID, Node and Net Tuple
	 * (this works only if we are on an extended net)
	 */

	rtmp_data = ddp->data;
	
	*rtmp_data++ = (Entry->NextIRNet & 0xff00) >> 8;
	*rtmp_data++ = Entry->NextIRNet & 0x00ff ;
	*rtmp_data++ = 8;	
	*rtmp_data++ = (uchar)Entry->NextIRNode;
	*rtmp_data++ = (Entry->NetStart & 0xff00) >> 8;
	*rtmp_data++ = Entry->NetStart & 0x00ff ;
	*rtmp_data++ = 0x80;	/* first tuple, so distance is always zero */
	*rtmp_data++ = (Entry->NetStop & 0xff00) >> 8;
	*rtmp_data++ = Entry->NetStop & 0x00ff ;
	*rtmp_data++ = RTMP_VERSION_NUMBER;


	dPrintf(D_M_RTMP, D_L_INFO, ("rtmp_r_find_bridge: for net %d send back router %d.%d\n",
				NET_VALUE(orig_ddp->dst_net), Entry->NextIRNet, Entry->NextIRNode));
	if (status = ddp_router_output(m, ifID, AT_ADDR, NET_VALUE(orig_ddp->src_net),
			orig_ddp->src_node, 0)){
		dPrintf(D_M_RTMP, D_L_WARNING,
			("rtmp_r_find_bridge: ddp_router_output failed status=%d\n", status));
				return (status);
	}
	return (0);
}

/*
 * rtmp_send_table: 
 *					Send the routing table entries in RTMP data packets.
 *					Use split horizon if specified. The Data packets are sent
 *				    as full DDP packets, if the last packet is full an empty
 *					packet is sent to tell the recipients that this is the end of
 *					the table...
 *
 */
static
int rtmp_send_table(ifID, DestNet, DestNode, split_hz, socket, n_neighbors)
register at_if_t   *ifID;			/* interface/port params */
register at_net 	DestNet;		/* net where to send the table */
register uchar 		DestNode;		/* node where to send to table */
short 				split_hz;		/* use split horizon */
char				socket;			/* the destination socket to send to */
short 				n_neighbors;	/* used to send packets telling we are going down */
{

		RT_entry *Entry;
		char *Buff_ptr;
		uchar NewDist;
		gbuf_t *m;
		short size,status ;
		register at_ddp_t	*ddp;
		register short EntNb = 0, sent_tuple = 0;
		register unsigned int s;

		if (ifID->ifRoutingState < PORT_ONLINE) {
			dPrintf(D_M_RTMP, D_L_INFO,
				("rtmp_send_table: port %d activating, we don't send anything!\n",
				ifID->ifPort));
			return (0);
		}

		/* prerare tuples and packets for DDP*/
		/* if split horizon, do not send tuples we can reach on the port we
		 * want to send too
		 */

		Entry = &RT_table[0];
		size = 0;
		if (!(m = rtmp_prep_new_packet(ifID, DestNet, DestNode, socket))) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_send_table: rtmp_prep_new_packet failed\n"));
				return(ENOBUFS);
		}

		ddp = (at_ddp_t *)(gbuf_rptr(m));
		Buff_ptr = (char *)((char *)ddp + DDP_X_HDR_SIZE + 10); 

		ATDISABLE(s, ddpinp_lock);
		while (EntNb < RT_MAXENTRY) {

			if (Entry->NetStop && ((Entry->EntryState & 0x0F) >= RTE_STATE_SUSPECT)) {
				if (!(split_hz && ifID->ifPort == Entry->NetPort)) {
					sent_tuple++;

					if (((Entry->EntryState & 0x0F) < RTE_STATE_SUSPECT) || n_neighbors)
						NewDist = NOTIFY_N_DIST;
					else
						NewDist = Entry->NetDist & 0x1F;

					if (Entry->NetStart) {	/* Extended */
						*Buff_ptr++ = (Entry->NetStart & 0xFF00) >> 8;
						*Buff_ptr++ = (Entry->NetStart & 0x00FF);
						*Buff_ptr++ = 0x80 | NewDist;
						*Buff_ptr++ = (Entry->NetStop & 0xFF00) >> 8;
						*Buff_ptr++ = (Entry->NetStop & 0x00FF);
						*Buff_ptr++ = RTMP_VERSION_NUMBER;
						size += 6;
					}
					else {	/* non extended tuple */
						*Buff_ptr++ = (Entry->NetStop & 0xFF00) >> 8;
						*Buff_ptr++ = (Entry->NetStop & 0x00FF);
						*Buff_ptr++ = NewDist;
						size += 3;
					}
				}
			}

			if (size > (DDP_MAX_DATA-20)) {
				DDPLEN_ASSIGN(ddp, size + DDP_X_HDR_SIZE + 10);
				gbuf_winc(m,size);
				ATENABLE(s, ddpinp_lock);
				if (status = ddp_router_output(m, ifID, AT_ADDR,
					NET_VALUE(DestNet),DestNode, 0)){
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_send_table: ddp_router_output failed status=%d\n",
						 status));
					return (status);
				}
				if ((m = rtmp_prep_new_packet (ifID, DestNet, DestNode, socket)) == NULL){
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_send_table: rtmp_prep_new_poacket failed status=%d\n",
						status));
					return (ENOBUFS);
				}
				ddp = (at_ddp_t *)(gbuf_rptr(m));
				Buff_ptr = (char *)((char *)ddp + DDP_X_HDR_SIZE + 10); 

				dPrintf(D_M_RTMP_LOW, D_L_OUTPUT,
					("rtmp_s_tble: Send %d tuples on port %d\n",
					sent_tuple, ifID->ifPort));
				sent_tuple = 0;
				size = 0;
				ATDISABLE(s, ddpinp_lock);
			}

			Entry++;
			EntNb++;
		}
		ATENABLE(s, ddpinp_lock);

		/*
		 * If we have some remaining entries to send, send them now.
         * otherwise, the last packet we sent was full, we need to send an empty one
         */

	DDPLEN_ASSIGN(ddp, size + DDP_X_HDR_SIZE + 10);
	gbuf_winc(m,size);
	if (status = ddp_router_output(m, ifID, AT_ADDR, NET_VALUE(DestNet),DestNode, 0)){
		dPrintf(D_M_RTMP, D_L_WARNING,
		("rtmp_send_table: ddp_router_output failed status=%d\n", status));
		return (status);
	}
	dPrintf(D_M_RTMP_LOW, D_L_OUTPUT,
		("rtmp_s_tble: LAST Packet split=%d with %d tuples sent on port %d\n",
		split_hz, sent_tuple, ifID->ifPort));
			
	return (0);
}

/*
 * rtmp_request: respond to the 3 types of RTMP requests RTMP may receive
 *      RTMP func =1 : respond with an RTMP Reponse Packet
 *		RTMP func =2 : respond with the routing table RTMP packet with split horizon
 *		RTMP func =3 : respond with the routing table RTMP packet no split horizon	
 *
 * see Inside AppleTalk around page 5-18 for "details"
 */

static
void rtmp_request(ifID, ddp)
register at_if_t 	*ifID;
register at_ddp_t 	*ddp;
{

	short split_horizon = FALSE;
	short code;
	short error;

	/* We ignore the request if we're activating on that port */

	if (ifID->ifRoutingState <  PORT_ONLINE) 
			return;

	/* check RTMP function code */

	code = ddp->data[0];

	switch (code) {

		case RTMP_REQ_FUNC1:	/* RTMP Find Bridge */

				/* RTMP Request Packet: we send a response with the next IRrange */
				dPrintf(D_M_RTMP, D_L_INPUT,
					( "rtmp_request: find bridge for net %d port %d node %d.%d\n",
						 NET_VALUE(ddp->dst_net), ifID->ifPort,
						 NET_VALUE(ddp->src_net), ddp->src_node));

				if (error = rtmp_r_find_bridge (ifID, ddp)) {
					dPrintf(D_M_RTMP, D_L_WARNING,
						("rtmp_request: Code 1 ddp_r_output failed error=%d\n",
						error));
					return;
				}

				break;

		case RTMP_REQ_FUNC2:

				split_horizon = TRUE;	

		case RTMP_REQ_FUNC3:

				/* RTMP Route Request Packet */

				dPrintf(D_M_RTMP, D_L_INPUT,
					("rtmp_request:  received code=%d from %d.%d for %d.%d\n", 
						code, NET_VALUE(ddp->src_net), ddp->src_node,
						NET_VALUE(ddp->dst_net), ddp->dst_node));

						rtmp_send_table(ifID, ddp->src_net, ddp->src_node,
							 split_horizon, ddp->src_socket, 0);

				break;

		default:

				/* unknown type of request */
				dPrintf(D_M_RTMP, D_L_WARNING,
					("rtmp_request : invalid type of request =%d\n",
					code));
				break;
	}			

}

/*
 * rtmp_send_all_ports : send the routing table on all connected ports
 *                       check for the port status and if ok, send the
 *                       rtmp tuples to the broadcast address for the port
 *                       usually called on timeout every 10 seconds.
 */

void rtmp_send_port(ifID)
register at_if_t 	*ifID;
{
	at_net 	DestNet;	

	NET_ASSIGN(DestNet, 0);

	if (ifID && IFID_VALID(ifID) && ifID->ifRoutingState == PORT_ONLINE) {
		dPrintf(D_M_RTMP_LOW, D_L_OUTPUT,
			("rtmp_send_port: do stuff for port=%d\n",
			 ifID->ifPort));
		if (ifID->ifZipNeedQueries) 
			zip_send_queries(ifID, 0, 0xFF);
		if (MULTIHOME_MODE)
			return(0);
		rtmp_send_table(ifID, DestNet, 0xFF, 1, RTMP_SOCKET, 0);
	}

	if (ifID == ifID_table[0])
		dPrintf(D_M_RTMP_LOW, D_L_VERBOSE,
	("I:%5d O:%5d H:%5d dropped:%d\n",pktsIn,pktsOut, pktsHome ,pktsDropped));

	dPrintf(D_M_RTMP_LOW, D_L_TRACE,
		("rtmp_send_port: func=0x%x, ifID=0x%x\n", (u_int) rtmp_send_port, (u_int) ifID));
	ifID->tmo_2 = atalk_timeout (rtmp_send_port, (caddr_t)ifID, 10 * SYS_HZ);
}

/* rtmp_dropper: check the number of packet received every x secondes.
 *               the actual packet dropping is done in ddp_input
 */

void rtmp_dropper()
{
	pktsIn = pktsOut = pktsHome = pktsDropped = 0;
	dropper_tmo = atalk_timeout(rtmp_dropper, NULL, 2*SYS_HZ);
}
	
/*
 * rtmp_router_start: perform the sanity checks before declaring the router up
 *	 and running. This function looks for discrepency between the net infos
 *	 for the different ports and seed problems.
 *	 If everything is fine, the state of each port is brought to PORT_ONLINE.\
 *   ### LD 01/09/95 Changed to correct Zone problem on non seed ports.
 */
     
int rtmp_router_start(flag, m, gref)
int flag;
gbuf_t *m;
gref_t *gref;
{
	static short next_i = 0, next_router_starting_timer = 0;
	int err;
	register at_if_t 	*ifID, *ifID2;
	register short i,j,k, Index, router_starting_timer = 0;
	register RT_entry *Entry;
	register at_net_al	netStart, netStop;

	switch (flag) {
	case 1:
		if ((m = at_delay_m) != NULL) {
			at_delay_m = NULL;
			at_delay_func = rtmp_router_start;
			at_delay_flag = 2;
			ioc_ack(ENOTREADY, m, at_delay_gref);
		}
		return (0);
	case 2:
		goto startZoneInfo;
	case 3:
		if ((m = at_delay_m) != NULL) {
			at_delay_m = NULL;
			at_delay_func = rtmp_router_start;
			at_delay_flag = 4;
			ioc_ack(ENOTREADY, m, at_delay_gref);
		}
		return (0);
	case 4:
		goto startZoneInfoCont;
	case 9:
		goto elap_onlineCont;
	default:
		break;
	}

	for (i= 0; i < IF_TOTAL_MAX; i++) {
		ifID = ifID_table[i];

		if (!ifID || !IFID_VALID(ifID))
			break;

		/* if non seed, need to acquire the right node address */

		if ((ifID->ifFlags & RTR_SEED_PORT) == 0)  {
			if ((ifID->ifThisCableStart == 0 && ifID->ifThisCableEnd == 0) ||
				(ifID->ifThisCableStart >= DDP_STARTUP_LOW && 
				ifID->ifThisCableEnd <= DDP_STARTUP_HIGH))  {

				if (ifID->ifThisCableEnd == 0)  {
					ke.errno 	= KE_NO_SEED;
					ke.port1 	= ifID->ifPort;
					strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
				}
				else {
					ke.errno 	= KE_INVAL_RANGE;
					ke.port1 	= ifID->ifPort;
					strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
					ke.netr1b 	= ifID->ifThisCableStart;
					ke.netr1e 	= ifID->ifThisCableEnd;
				}
				ifID->ifRoutingState = PORT_ERR_STARTUP;
				RouterError(ifID->ifPort, ERTR_CABLE_STARTUP);

				goto error;
			}
			
			/* we are non seed, so try to acquire the zones for that guy */
			ifID->ifZipNeedQueries = 1;

			dPrintf(D_M_RTMP, D_L_STARTUP,
				("rtmp_router_start: call elap_online for Non Seed port #%d cable =%d-%d\n",
					ifID->ifPort, CableStart, CableStop));
			at_delay_func = rtmp_router_start;
			at_delay_flag = 9;
			at_delay_errno = ENOTREADY;
			err = elap_online(gref, ifID->ifName, NULL, m);
			if (err != ENOTREADY) {
				at_delay_m = m;
				at_delay_gref = gref;
				goto error;
			}
			next_i = i;
			return (ENOTREADY);
elap_onlineCont:
			i = next_i;
		}
	}
	next_i = 0;


	/* Check if we have a problem with the routing table size */

	if (ErrorRTMPoverflow) {
		ke.errno = KE_RTMP_OVERFLOW;	
		goto error;
	}


	/* Now, check that we don't have a conflict in between our interfaces */

	for (i= 0; i < IF_TOTAL_MAX; i++) {
		ifID = ifID_table[i];

		if (!ifID || !IFID_VALID(ifID))
			break;

			/* check if the RoutingState != PORT_ONERROR */

		if (ifID->ifRoutingState < PORT_ACTIVATING) {
			goto error;
		}

		if ((ifID->ifThisCableStart == 0 && ifID->ifThisCableEnd == 0) ||
			(ifID->ifThisCableStart >= DDP_STARTUP_LOW && 
			ifID->ifThisCableEnd <= DDP_STARTUP_HIGH))  {

			if (ifID->ifThisCableEnd == 0)  {
				ke.errno 	= KE_NO_SEED;
				ke.port1 	= ifID->ifPort;
				strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
			}
			else {
				ke.errno 	= KE_INVAL_RANGE;
				ke.port1 	= ifID->ifPort;
				strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
				ke.netr1b 	= ifID->ifThisCableStart;
				ke.netr1e 	= ifID->ifThisCableEnd;
			}
			
			ifID->ifRoutingState = PORT_ERR_STARTUP;
			RouterError(ifID->ifPort, ERTR_CABLE_STARTUP);

			goto error;
		}

			/* check the interface address against all other ifs */

		netStart = ifID->ifThisCableStart;
		netStop = ifID->ifThisCableEnd;

		for (j = i+1 ; j < IF_TOTAL_MAX ; j++) {
			ifID2 = ifID_table[j];
			if (!ifID2 || !IFID_VALID(ifID2))
					break;

			if (((netStart >= ifID2->ifThisCableStart) && 
				(netStart <= ifID2->ifThisCableEnd)) ||
			    ((netStop >= ifID2->ifThisCableStart) && 
				(netStop <= ifID2->ifThisCableEnd)) ||
				((ifID2->ifThisCableStart >= netStart) &&
				(ifID2->ifThisCableStart <= netStop)) ||
				((ifID2->ifThisCableEnd >= netStart) &&
				(ifID2->ifThisCableEnd <= netStop)) ) {

					ke.errno 	= KE_CONF_RANGE;
					ke.port1 	= ifID->ifPort;
					strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
					ke.port2 	= ifID2->ifPort;
					strncpy(ke.name2, ifID2->ifName,IF_NAME_LEN);
					ke.netr1b 	= ifID->ifThisCableStart;
					ke.netr1e 	= ifID->ifThisCableEnd;
					ifID->ifRoutingState = PORT_ERR_CABLER;
					RouterError(ifID->ifPort, ERTR_CABLE_CONFLICT);
					goto error;
			}

		}

		/* ### LD 01/04/94: We need to fill in the next IR info in the routing table */
		Entry = rt_blookup(ifID->ifThisCableEnd);

		if (Entry == NULL) {
			dPrintf(D_M_RTMP, D_L_ERROR,
				("rtmp_router_start: we don't know our cable range port=%d\n",
			ifID->ifPort));

			goto error;
		}

			/*
			 * Note: At this point, non seed ports may not be aware of their Default zone
			 */

		if (!(ifID->ifFlags & RTR_SEED_PORT)) {
			ifID->ifDefZone = 0;
			Entry->EntryState |= (RTE_STATE_GOOD|RTE_STATE_UPDATED);
		}
		
			
		ifID->ifRoutingState = PORT_ONLINE;
		ifID->ifState = LAP_ONLINE;

		/* set the right net and node for each port */

		Entry->NextIRNet = NET_VALUE(ifID->ifThisNode.atalk_net);
		Entry->NextIRNode= ifID->ifThisNode.atalk_node;

		dPrintf(D_M_RTMP, D_L_STARTUP,
		 ("rtmp_router_start: bring port=%d [%d.%d]... on line\n",
		 ifID->ifPort, NET_VALUE(ifID->ifThisNode.atalk_net),ifID->ifThisNode.atalk_node));

	}

	/*
	 * Everything is fine, we can begin to babble on the net...
	 */

	for (k= 0; k < IF_TOTAL_MAX ; k++) {
		ifID = ifID_table[k];
		if (ifID && IFID_VALID(ifID) && ifID->ifRoutingState == PORT_ONLINE)  {
			rtmp_send_port(ifID);
			ifID->tmo_1 = atalk_timeout (rtmp_timeout, (caddr_t)ifID, (50+k) * SYS_HZ);
			if (ifID->ifRoutingState  < PORT_ACTIVATING) {
				goto error;
			}
		}
	}

	/* Check if we have a problem with the routing or zip table size */

	if (ErrorRTMPoverflow) {
		ke.errno = KE_RTMP_OVERFLOW;	
		goto error;
	}
	if (ErrorZIPoverflow) {
		ke.errno = KE_ZIP_OVERFLOW;	
		goto error;
	}

	at_delay_m = m; at_delay_gref = gref;
	atalk_timeout(rtmp_router_start, 1, 10 * SYS_HZ);
	return (ENOTREADY);

startZoneInfo:
	for (i= 0; i < IF_TOTAL_MAX; i++) {
		ifID = ifID_table[i];

		if (!ifID || !IFID_VALID(ifID))
			break;

		if (ifID->ifRoutingState < PORT_ACTIVATING) {
			goto error;
		}

		if ((ifID->ifZipNeedQueries) 
		 && (ifID->ifFlags & RTR_SEED_PORT) == 0)  {
			dPrintf(D_M_RTMP, D_L_STARTUP,
				("rtmp_router_start: send Zip Queries for Port %d\n",
					ifID->ifPort));
			zip_send_queries(ifID, 0, 0xFF);

			if (router_starting_timer >= 15) {
				dPrintf(D_M_RTMP, D_L_WARNING,
					("rtmp_router_start: no received response to ZipNeedQueries\n"));
				ke.errno 	= KE_NO_ZONES_FOUND;
				ke.port1 	= ifID->ifPort;
				strncpy(ke.name1, ifID->ifName,IF_NAME_LEN);
				ke.netr1b 	= ifID->ifThisCableStart;
				ke.netr1e 	= ifID->ifThisCableEnd;
				ifID->ifRoutingState = PORT_ERR_CABLER;
				RouterError(ifID->ifPort, ERTR_CABLE_CONFLICT);
				goto error;
			}

			dPrintf(D_M_RTMP, D_L_STARTUP,
				("rtmp_router_start: waiting for zone info to complete\n"));
			at_delay_m = m; at_delay_gref = gref;
			atalk_timeout(rtmp_router_start, 3, 10 * SYS_HZ);
			next_router_starting_timer = router_starting_timer+1;
			return (ENOTREADY);
startZoneInfoCont:
			router_starting_timer = next_router_starting_timer;
			i=0;
			goto startZoneInfo;
		}

	}
	next_router_starting_timer = 0;

	/* At This Point, check if we know the default zones for non seed port */

	for (i= 0; i < IF_TOTAL_MAX; i++) {
		ifID = ifID_table[i];

		if (!ifID || !IFID_VALID(ifID))
			break;

		if (ifID->ifRoutingState  < PORT_ACTIVATING){
				goto error;
		}


		if (!(ifID->ifFlags & RTR_SEED_PORT)) { 
			Entry = rt_blookup(ifID->ifThisCableEnd);

			if (Entry == NULL) {
				dPrintf(D_M_RTMP, D_L_ERROR,
					("rtmp_router_start: (2)we don't know our cable range port=%d\n",
					ifID->ifPort));
				goto error;
			}
		if (!(ifID->ifFlags & RTR_SEED_PORT)) {
			
			dPrintf(D_M_RTMP, D_L_STARTUP,
				("rtmp_router_start: if %s set to permanent\n", 
					ifID->ifName));
			Entry->NetDist = 0; 	/* added 4-29-96 jjs, prevent direct
									   nets from showing non-zero distance */
			Entry->EntryState |= RTE_STATE_PERMANENT; /* upgrade the non seed ports. */
		}

			Index = zt_ent_zindex(Entry->ZoneBitMap);
			if (Index <= 0) {
				dPrintf(D_M_RTMP, D_L_ERROR,
					 ("rtmp_router_start: still don't know default zone for port %d\n",
					ifID->ifPort));
			}
			else  if (ROUTING_MODE) 
				ifID->ifDefZone = Index;
			

		}
		
	}

	/* Check if we have a problem with the routing or zip table size */

	if (ErrorRTMPoverflow) {
		ke.errno = KE_RTMP_OVERFLOW;	
		goto error;
	}
	if (ErrorZIPoverflow) {
		ke.errno = KE_ZIP_OVERFLOW;	
		goto error;
	}

	/*
	 * Handle the Home Port specifics
	 */

	ifID = ifID_table[IFID_HOME];

		/* set the router address as being us no matter what*/

	NET_NET(ifID->ifARouter.atalk_net , ifID->ifThisNode.atalk_net);
	ifID->ifARouter.atalk_node = ifID->ifThisNode.atalk_node;
	ifID->ifRouterState = ROUTER_UPDATED;

	/* if we are non seed, take the first zone of the list as our home zone */	

	if ( 0 && !(ifID->ifFlags & RTR_SEED_PORT)) {
		Entry = rt_blookup(ifID->ifThisCableEnd);
		if (Entry != NULL) {
			Index = zt_ent_zindex(Entry->ZoneBitMap);
	
			ifID->ifDefZone = Index;
			dPrintf(D_M_RTMP, D_L_ERROR,
				("rtmp_router_start: Index of default zone is: %d\n",
				 Index));

			ifID->ifZoneName.len = ZT_table[Index-1].Zone.len;

			dPrintf(D_M_RTMP, D_L_STARTUP,
				("rtmp_router_start: Zone Name '%s' (#%d) set for non seed home port\n",
			ZT_table[Index-1].Zone.str, Index-1));

			bcopy(&ZT_table[Index-1].Zone.str, &ifID->ifZoneName.str, 
				ZT_table[Index-1].Zone.len);

		}
	}
	if (MULTIHOME_MODE)
		at_state.flags &= ~AT_ST_ROUTER; 

		/* set the zone and stuffs for NBP */

	zip_notify_nbp((int)ifID);

	if ((m = at_delay_m) != NULL) {
		at_delay_m = NULL;
		ioc_ack(0, m, at_delay_gref);
	}

	/* prepare the packet dropper timer */

	dropper_tmo = atalk_timeout (rtmp_dropper, NULL, 1*SYS_HZ);
	return(0);

error:
	dPrintf(D_M_RTMP,D_L_ERROR, 
		("rtmp_router_start: error type=%d occured on port %d\n",
		ifID->ifRoutingState, ifID->ifPort));
	rtmp_shutdown();
	if (m) {
		at_delay_m = m;
		at_delay_gref = gref;
	}
	if ((m = at_delay_m) != NULL) {
		at_delay_m = NULL;
		if ((gbuf_cont(m) = gbuf_alloc(sizeof(ke), PRI_HI)) == NULL)
			ioc_ack(ENOBUFS, m, gref);
		else {
			bcopy(&ke, gbuf_rptr(gbuf_cont(m)), sizeof(ke));
			gbuf_wset(gbuf_cont(m),sizeof(ke));
			((ioc_t *)gbuf_rptr(m))->ioc_count = sizeof(ke);
			ioc_ack(0, m, at_delay_gref);  
		}	
	}
	next_i = 0;
	return(ENOTREADY);
}


void rtmp_shutdown()
{
	register at_if_t 	*ifID;
	register short i;
	at_net 	DestNet;	

#ifdef CHECK_DDPR_FLAG
	while (ddprunning_flag)
		;
#endif
	NET_ASSIGN(DestNet, 0);

	dPrintf(D_M_RTMP, D_L_SHUTDN,
		("rtmp_shutdown:stop sending to all ports\n"));

	untimeout(rtmp_dropper, (caddr_t)0);
	untimeout(rtmp_router_start, 1); /* added for 2225395 */
	untimeout(rtmp_router_start, 3); /* added for 2225395 */
	
	for (i= 0; i < IF_TOTAL_MAX; i++) {
		ifID = ifID_table[i];
		if (ifID && IFID_VALID(ifID) && ifID->ifRoutingState > PORT_OFFLINE ) {
			if (ifID->ifRoutingState == PORT_ONLINE)  {
				untimeout(rtmp_send_port, (caddr_t)ifID);
				untimeout(rtmp_timeout, (caddr_t) ifID); 
			}
			/* 
			 * it's better to notify the neighbour routers that we are going down
			 */
			if (!MULTIHOME_MODE)
				rtmp_send_table(ifID, DestNet, 0xFF, TRUE, RTMP_SOCKET, TRUE);

			rtmp_send_table(ifID, DestNet, 0xFF, TRUE, RTMP_SOCKET, TRUE);

			ifID->ifRoutingState = PORT_OFFLINE;

			dPrintf(D_M_RTMP, D_L_SHUTDN,
				("rtmp_shutdown: routing on port=%d... off line\nStats:\n",
				 ifID->ifPort));
			dPrintf(D_M_RTMP, D_L_SHUTDN,
			 ("fwdBytes     : %ld\nfwdPackets   : %ld\ndroppedBytes : %ld\ndroppedPkts  : %ld\n",
			ifID->ifStatistics.fwdBytes, ifID->ifStatistics.fwdPkts,
			ifID->ifStatistics.droppedBytes, ifID->ifStatistics.droppedPkts));
 
		}
	}

}

/*
 * Remove all entries associated with the specified port.
 */
void rtmp_purge(ifID)
	at_if_t *ifID;
{
	u_char state;
	int i, s;
	RT_entry *en = &RT_table[0];

	ATDISABLE(s, ddpinp_lock);
	for (i=0; i < RT_MAXENTRY; i++) {
		state = en->EntryState & 0x0F;
		if ((state > RTE_STATE_UNUSED) && (state != RTE_STATE_PERMANENT)
			&& en->NetStop && en->NetDist && (en->NetPort == ifID->ifPort)) {
			zt_remove_zones(en->ZoneBitMap);		
			RT_DELETE(en->NetStop, en->NetStart);
		}
		en++;
	}
	ATENABLE(s, ddpinp_lock);
}
