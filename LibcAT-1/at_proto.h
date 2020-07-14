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
/*
 *      Copyright (c) 1997 Apple Computer, Inc.
 *
 *      The information contained herein is subject to change without
 *      notice and  should not be  construed as a commitment by Apple
 *      Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *      for any errors that may appear.
 *
 *      Confidential and Proprietary to Apple Computer, Inc.
 *
 */

/* at_proto.h --  Prototype Definitions for the AppleTalk API

   See the "AppleTalk Programming Interface" document for a full
   explanation of these functions and their parameters.

   Required header files are as follows:

   #include <netat/at/appletalk.h>
   #include <netat/at/atp.h>
   #include <netat/at/nbp.h>
   #include <netat/at/asp_errno.h>
*/

#ifndef _AT_PROTO_H_
#define _AT_PROTO_H_

/* Appletalk Stack status Function. */
enum {
	  RUNNING
	, NOTLOADED
	, LOADED
	, OTHERERROR
};

int checkATStack();

int at_send_to_dev(int fd, int cmd, char *dp, int *length);


/* Datagram Delivery Protocol (DDP) Functions */

int ddp_open (at_socket *socket);
int ddp_close(int fd);
int atpproto_open(at_socket *socket);
int adspproto_open(at_socket *socket);

/* Routing Table Maintenance Protocol (RTMP) Function */

int rtmp_netinfo(int fd, at_inet_t *addr, at_inet_t *bridge);
     
/* AppleTalk Transaction Protocol (ATP) Functions */

int atp_open(at_socket *socket);
int atp_close(int fd);
int atp_sendreq(int fd,
		at_inet_t *dest,
		char *buf,
		int len, 
		int userdata, 
		int xo, 
		int xo_relt,
		u_short *tid,
		at_resp_t *resp,
		at_retry_t *retry,
		int nowait);
int atp_getreq(int fd,
	       at_inet_t *src,
	       char *buf,
	       int *len, 
	       int *userdata, 
	       int *xo,
	       u_short *tid,
	       u_char *bitmap,
	       int nowait);
int atp_sendrsp(int fd,
		at_inet_t *dest,
		int xo,
		u_short tid,
		at_resp_t *resp);
int atp_getresp(int fd,
		u_short *tid,
		at_resp_t *resp);
int atp_look(int fd);
int atp_abort(int fd,
	      at_inet_t *dest,
	      u_short tid);

/* Name Binding Protocol (NBP) Functions */

int nbp_parse_entity(at_entity_t *entity,
		     char *str);
int nbp_make_entity(at_entity_t *entity, 
		    char *obj, 
		    char *type, 
		    char *zone);
int nbp_confirm(at_entity_t *entity,
		at_inet_t *dest,
		at_retry_t *retry);
int nbp_lookup(at_entity_t *entity,
	       at_nbptuple_t *buf,
	       int max,
	       at_retry_t *retry);
int nbp_register(at_entity_t *entity, 
		 int fd, 
		 at_retry_t *retry);
int nbp_remove(at_entity_t *entity, 
	       int fd);	     

/* Printer Access Protocol (PAP) Functions */

int pap_open(at_nbptuple_t *tuple);
int pap_read(int fd,
	     u_char *data,
	     int len);
int pap_read_ignore(int fd);
char *pap_status(at_nbptuple_t *tuple);
int pap_write(int fd,
	      char *data,
	      int len,
	      int eof,
	      int flush);
int pap_close(int fd);

/* AppleTalk Data Stream Protocol (ADSP) Functions: */

int ADSPaccept(int fd, 
	       void *name, 
	       int *namelen);
int ADSPbind(int fd, 
	     void *name, 
	     int namelen);
int ADSPclose(int fd);
int ADSPconnect(int fd, 
		void *name, 
		int namelen);
int ADSPfwdreset(int fd);
int ADSPgetpeername(int fd, 
		    void *name, 
		    int *namelen);
int ADSPgetsockname(int fd, 
		    void *name, 
		    int *namelen);
int ADSPgetsockopt(int fd, 
		   int level, 
		   int optname, 
		   char *optval, 
		   int *optlen);
int ADSPlisten(int fd, 
	       int backlog);
int ADSPrecv(int fd, 
	     char *buf, 
	     int len, 
	     int flags);
int ADSPsend(int fd,
	     char *buf,
	     int len,
	     int flags);
int ADSPsetsockopt(int fd,
		   int level,
		   int optname,
		   char *optval,
		   int optlen);
int ADSPsocket(int fd,
	       int type,
	       int protocol);
int ASYNCread(int fd,
	      char *buf,
	      int len);
int ASYNCread_complete(int fd,
		       char *buf,
		       int len);

/* AppleTalk Session Protocol (ASP) Functions */

int SPAttention(int SessRefNum,
		unsigned short AttentionCode,
		OSErr *SPError,
		int NoWait);
int SPCloseSession(int SessRefNum,
		   OSErr *SPError);
int SPCmdReply(int SessRefNum,
	       unsigned short ReqRefNum,
	       int CmdResult,
	       char *CmdReplyData,
	       int CmdReplyDataSize,
	       OSErr *SPError);
int SPCommand(int SessRefNum,
	      char *CmdBlock,
	      int CmdBlockSize,
	      char *ReplyBuffer,
	      int ReplyBufferSize,
	      int *CmdResult,
	      int *ActRcvdReplyLen,
	      OSErr *SPError,
	      int NoWait);
void SPConfigure(unsigned short TickleInterval,
		 unsigned short SessionTimer,
		 at_retry_t *Retry);
int SPEnableSelect(int SessRefNum,
		   OSErr *SPError);
void SPGetParms(int *MaxCmdSize,
		int *QuantumSize,
		int SessRefNum);
int SPGetProtoFamily(int SessRefNum,
		      int *ProtoFamily,
		      OSErr *SPError);
int SPGetRemEntity(int SessRefNum,
		   void *SessRemEntityIdentifier,
		   OSErr *SPError);
int SPGetReply(int SessRefNum,
	       char *ReplyBuffer,
	       int ReplyBufferSize,
	       int *CmdResult,
	       int *ActRcvdReplyLen,
	       OSErr *SPError);
int SPGetRequest(int SessRefNum,
		 char *ReqBuffer,
		 int ReqBufferSize,
		 unsigned short *ReqRefNum,
		 int *ReqType,
		 int *ActRcvdReqLen,
		 OSErr *SPError);
int SPGetSession(int SLSRefNum,
		 int *SessRefNum,
		 OSErr *SPError);
int SPGetStatus(at_inet_t *SLSEntityIdentifier,
		char *StatusBuffer,
		int StatusBufferSize,
		int *ActRcvdStatusLen,
		OSErr *SPError,
		int SLSFamily,
		char *SLSName);
int SPInit(at_inet_t *SLSEntityIdentifier,
	   char *ServiceStatusBlock,
	   int ServiceStatusBlockSize,
	   int *SLSRefNum,
	   OSErr *SPError);
int SPLook(int SessRefNum,
	   OSErr *SPError);
int SPNewStatus(int SLSRefNum,
		char *ServiceStatusBlock,
		int ServiceStatusBlockSize,
		OSErr *SPError);
int SPOpenSession(at_inet_t *SLSEntityIdentifier,
		  void (*AttentionCode)(),
		  int *SessRefNum,
		  OSErr *SPError,
		  int SLSFamily,
		  char *SLSName);
int SPRegister(at_entity_t *SLSEntity,
	       at_retry_t *Retry,
	       at_inet_t *SLSEntityIdentifier,
	       OSErr *SPError);
/*
 * the following API is added to fix bug 2285307;  It replaces SPRegister which now only behaves as asp
 * over appletalk.
*/
int SPRegisterWithTCPPossiblity(at_entity_t *SLSEntity,
           at_retry_t *Retry,
           at_inet_t *SLSEntityIdentifier,
           OSErr *SPError);
int SPRemove(at_entity_t *SLSEntity,
	     at_inet_t *SLSEntityIdentifier,
	     OSErr *SPError);
int SPSetPid(int SessRefNum,
	     int SessPid,
	     OSErr *SPError);
int SPWrite(int SessRefNum,
	    char *CmdBlock,
	    int CmdBlockSize,
	    char *WriteData,
	    int WriteDataSize,
	    char *ReplyBuffer,
	    int ReplyBufferSize,
	    int *CmdResult,
	    int *ActLenWritten,
	    int *ActRcvdReplyLen,
	    OSErr *SPError,
	    int NoWait);
int SPWrtContinue(int SessRefNum,
		  unsigned short ReqRefNum,
		  char *Buff,
		  int BuffSize,
		  int *ActLenRcvd,
		  OSErr *SPError,
		  int NoWait);
int SPWrtReply(int SessRefNum,
	       unsigned short ReqRefNum,
	       int CmdResult,
	       char *CmdReplyData,
	       int CmdReplyDataSize,
	       OSErr *SPError);

/* Zone Information Protocol (ZIP) Functions */

int zip_getmyzone(at_nvestr_t *zone);
int zip_getzonelist(int start,
		    at_nvestr_t *buf[]);
int zip_getlocalzones(int start,
		      at_nvestr_t *buf[]);
     
#endif _AT_PROTO_H_
