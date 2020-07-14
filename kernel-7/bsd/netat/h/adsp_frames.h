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
 * LAP Frame Information
 */

typedef struct {
  u_char     lap_dest;
  u_char     lap_src;
  u_char     lap_type;
  u_char     lap_data[1];
} LAP_FRAME;

#define LAP_FRAME_LEN     3

#define MAX_FRAME_SIZE    603

#define LAP_DDP           0x01
#define LAP_DDPX          0x02

typedef struct {
  ua_short   ddp_length;             /* length of ddp fields        */
  u_char     ddp_dest;               /* destination socket          */
  u_char     ddp_source;             /* source socket               */
  u_char     ddp_type;               /* protocol type               */
  u_char     ddp_data[1];            /* data field                  */
} DDP_FRAME;

#define DDPS_FRAME_LEN     5

typedef struct {
  ua_short   ddpx_length;            /* length and hop count        */
  ua_short   ddpx_cksm;              /* checksum                    */
  at_net     ddpx_dnet;              /* destination network number  */
  at_net     ddpx_snet;              /* source network number       */
  u_char     ddpx_dnode;             /* destination node            */
  u_char     ddpx_snode;             /* source node                 */
  u_char     ddpx_dest;              /* destination socket          */
  u_char     ddpx_source;            /* source socket               */
  u_char     ddpx_type;              /* protocol type               */
  u_char     ddpx_data[1];           /* data field                  */
} DDPX_FRAME;

#define DDPL_FRAME_LEN     13

#define DDP_RTMP          0x01
#define DDP_NBP           0x02
#define DDP_ATP           0x03
#define DDP_ECHO          0x04
#define DDP_RTMP_REQ      0x05
#define DDP_ZIP           0x06
#define DDP_ADSP          0x07

#define RESPONDER_SOCKET  254

#define ECHO_REQUEST      1
#define ECHO_RESPONSE     2

typedef struct {
  u_char     nbp_ctrl_cnt;           /* control and tuple count     */
  u_char     nbp_id;                 /* enquiry/reply id            */
  u_char     nbp_data[1];            /* tuple space                 */
} NBP_FRAME;

#define NBP_TYPE_MASK     0xf0     /* mask of ctrl_cnt field      */
#define NBP_CNT_MASK      0x0f     /* mask for number of tuples   */
#define NBP_BROADCAST     0x10     /* internet lookup             */
#define NBP_LOOKUP        0x20     /* lookup request              */
#define NBP_REPLY         0x30     /* response to lookup          */

typedef struct {
  u_char     atp_control;            /* control field               */
  u_char     atp_map;                /* bitmap for acknowlegement   */
  ua_short   atp_tid;                /* transaction id              */
  union
  {
      u_char     b[4];               /* user u_chars                  */
      ua_long    dw;
  } atp_ub;
  u_char     atp_data[1];            /* data field                  */
} ATP_FRAME;

#define ATP_FRAME_LEN      8

#define ATP_TREQ          0x40     /* transaction request         */
#define ATP_TRESP         0x80     /* response packet             */
#define ATP_TREL          0xc0     /* transaction release packet  */
#define ATP_XO            0x20     /* exactly once flag           */
#define ATP_EOM           0x10     /* end of message flag         */
#define ATP_STS           0x08     /* send transaction status     */

#define ATP_TYPE(x)       ((x)->atp_control & 0xc0)

typedef struct {
  at_net     net1;
  u_char     zonename[33];
} ZIP_1;

typedef struct {
  at_net     net1;
  at_net     net2;
  u_char     zonename[33];
} ZIP_2;

typedef struct {
  u_char     zip_command;             /* zip command number          */
  u_char     flags;                   /* Bit-mapped                  */
  union
  {
     ZIP_1 o;                       /* Packet has one net number   */
     ZIP_2 r;                       /* Packet has cable range      */
  } u;
} ZIP_FRAME;

/* Flags in the ZIP GetNetInfo & NetInfoReply buffer  */

#define ZIPF_BROADCAST     0x80
#define ZIPF_ZONE_INVALID  0x80
#define ZIPF_USE_BROADCAST 0x40
#define ZIPF_ONE_ZONE      0x20

#define ZIP_QUERY          1        /* ZIP Commands in zip frames  */
#define ZIP_REPLY          2
#define ZIP_TAKEDOWN       3
#define ZIP_BRINGUP        4
#define ZIP_GETNETINFO     5
#define ZIP_NETINFOREPLY   6
#define ZIP_NOTIFY         7

#define ZIP_GETMYZONE      7        /* ZIP commands in atp user u_chars[0]  */
#define ZIP_GETZONELIST    8
#define ZIP_GETLOCALZONES  9
#define ZIP_GETYOURZONE    10       

/*
 * Response to Reponder Request type #1.
 *
 * The first 4 u_chars are actually the 4 ATP user u_chars
 * Following this structure are 4 PASCAL strings:
 *    System Version String. (max 127)
 *    Finder Version String. (max 127)
 *    LaserWriter Version String. (max 127)
 *    AppleShare Version String. (max 24)
 */
typedef struct
{
   u_char  UserU_Chars[2];
   ua_short  ResponderVersion;
   ua_short  AtalkVersion;
   u_char  ROMVersion;
   u_char  SystemType;
   u_char  SystemClass;
   u_char  HdwrConfig;
   ua_short  ROM85Version;
   u_char  ResponderLevel;
   u_char  ResponderLink;
   u_char  data[1];
} RESPONDER_FRAME;


/*
 * ADSP Frame
 */
typedef struct {
   ua_short CID;
   ua_long pktFirstByteSeq;
   ua_long pktNextRecvSeq;
   ua_short  pktRecvWdw;
   u_char descriptor;		/* Bit-Mapped */
   u_char data[1];
} ADSP_FRAME, *ADSP_FRAMEPtr;

#define ADSP_FRAME_LEN     13

#define ADSP_CONTROL_BIT   0x80
#define ADSP_ACK_REQ_BIT   0x40
#define ADSP_EOM_BIT       0x20
#define ADSP_ATTENTION_BIT 0x10
#define ADSP_CONTROL_MASK  0x0F

#define ADSP_CTL_PROBE        0x00 /* Probe or acknowledgement */
#define ADSP_CTL_OREQ         0x01 /* Open Connection Request */
#define ADSP_CTL_OACK         0x02 /* Open Request acknowledgment */
#define ADSP_CTL_OREQACK      0x03 /* Open Request and acknowledgement */
#define ADSP_CTL_ODENY        0x04 /* Open Request denial */
#define ADSP_CTL_CLOSE        0x05 /* Close connection advice */
#define ADSP_CTL_FRESET       0x06 /* Forward Reset */
#define ADSP_CTL_FRESET_ACK   0x07 /* Forward Reset Acknowledgement */
#define ADSP_CTL_RETRANSMIT   0x08 /* Retransmit advice	*/

typedef struct {
   ua_short  version;		/* Must be in network byte order */
   ua_short  dstCID;		/* */
   ua_long pktAttnRecvSeq;		/* Must be in network byte order */
} ADSP_OPEN_DATA, *ADSP_OPEN_DATAPtr;

#define ADSP_OPEN_FRAME_LEN   8

#define ADSP_MAX_DATA_LEN		572

/* END FRAMES.H  */
