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


/**
 * Copyright (c) 1994-1996 NeXT Software, Inc.  All rights reserved.
 * Copyright © 1997 Apple Computer Inc. All Rights Reserved.
 * @author   Martin Minow   mailto:minow@apple.com
 * @revision    1997.02.13  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Edit History
 * 1997.02.13   MM      Initial conversion from AMDPCSCSIDriver sources.
 */


#define AUTO_SENSE_ENABLE    1

    /* These are accessible by SCSIInspector:   */

#define SYNC_ENABLE         "Synchronous"
#define FAST_ENABLE         "Fast SCSI"
#define CMD_QUEUE_ENABLE    "Cmd Queueing"

    /* These are only accessible by Configure's Expert mode:    */

#define EXTENDED_TIMING     "Extended Timing"
#define SCSI_CLOCK_RATE     "SCSI Clock Rate"   /* in MHz */


#define USE_ELG FALSE
#define CustomMiniMon FALSE
//#define CustomMiniMon TRUE
//#define USE_ELG   TRUE
#define kEvLogSize  (4096*16)   // 16 pages = 64K = 4096 events

#if USE_ELG /* (( */
#define ELG(A,B,ASCI,STRING)    EvLog( (UInt32)(A), (UInt32)(B), (UInt32)(ASCI), STRING )
#define PAUSE(A,B,ASCI,STRING)  Pause( (UInt32)(A), (UInt32)(B), (UInt32)(ASCI), STRING )
#else /* ) not USE_ELG: (   */
#define ELG(A,B,ASCI,S)
#define PAUSE(A,B,ASCI,S)
#define CKSTOP(A,E,ASCI,S)
#endif /* USE_ELG )) */

#define TIMESTAMP_AT_IOCOMPLETE     0

#ifndef TIMESTAMP_AT_IOCOMPLETE
#define TIMESTAMP_AT_IOCOMPLETE     0
#endif


#ifndef TIMESTAMP
#define TIMESTAMP  0    // mlj - linking problems with Curio
#endif

    /* These types will ultimately be moved to an implementation-wide header file*/
#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1
    typedef unsigned char   UInt8;              /* An unsigned 8-bit value  */
    typedef unsigned int    UInt32;             /* An unsigned integer      */
    typedef signed int      SInt32;             /* An explicitly signed int */
    typedef void            *LogicalAddress;    /* A virtual address        */
    typedef UInt32          PhysicalAddress;    /* A hardware address       */
    typedef UInt32          ByteCount;          /* A transfer length count  */
    typedef UInt32          ItemCount;          /* An index or counter      */
    typedef UInt32          Boolean;            /* A true/false value       */
#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif
#endif /* __APPLE_TYPES_DEFINED__ */
    typedef signed char     SInt8;
    typedef UInt32          OSType;

#ifndef SynchronizeIO
#define SynchronizeIO()     eieio()     /* TEMP */
#endif /* SynchronizeIO */


    enum
    {   DBDMA_ReadStartAlignment    = 8,    // mlj ???
        DBDMA_WriteStartAlignment   = 8
    };

        /* Operation flags and options: */

    typedef enum BusPhase   /* These are the real SCSI bus phases (from busStatus0):   */
    {
        kBusPhaseDATO   = 0,
        kBusPhaseDATI,
        kBusPhaseCMD,
        kBusPhaseSTS,
        kBusPhaseReserved1,
        kBusPhaseReserved2,
        kBusPhaseMSGO,
        kBusPhaseMSGI,
        kBusPhaseBusFree
    } BusPhase;

        /* Command to be executed by IO thread.                     */
        /* These are ultimately derived from ioctl control values.  */

    typedef enum
    {   kCommandExecute,            /* Execute IOSCSIRequest    */
        kCommandResetBus,           /* Reset bus                */
        kCommandAbortRequest        /* Abort IO thread          */
    } CommandOperation;

        /* We read target messages using a simple state machine.    */
        /* On entrance to MSGI phase, gMsgInState = kMsgInInit.     */
        /* Continue reading messages until either                   */
        /* gMsgInState == kMsgInReady or the target changes phase   */
        /* (which is an error).                                     */
    typedef enum MsgInState
    {
        kMsgInInit = 0,     /*  0 Not reading a message (must be zero)      */
        kMsgInReading,      /*  1 MSG input state: reading counted data     */
        kMsgInCounting,     /*  2 MSG input state: reading count byte       */
        kMsgInReady         /*  3 MSG input state: a msg is now available   */
    } MsgInState;


        /* This is the maximum number of bytes to be transferred            */
        /* in an autosense request. It is, by inspection, less than 256.    */
    enum { kMaxAutosenseByteCount = sizeof( esense_reply_t ) };

        /* These values are stored in gCurrentTarget and gCurrentLUN    */
        /* when there is no active request.                             */
    enum
    {   kInvalidTarget          = 0xFFFF,
        kInvalidLUN             = 0xFFFF
    };

        /* The default initiator bus ID (needs to be fetched from NVRAM).    */
    enum { kInitiatorIDDefault = 7 };
#define APPLE_SCSI_RESET_DELAY  250 /* Msec */

    /* Command struct passed to IO thread.                  */
    /* One of these are created for each active IO request. */

typedef struct CommandBuffer
{
        /* Fields valid when commandBuf is passed to IO thread. */
    CommandOperation    op;         /* kCommandExecute, etc.                */

        /* The following 3 fields are only valid if op == kCommandExecute.  */
        /* They are passed into the SCSI driver by executeRequest.          */

    IOSCSIRequest       *scsiReq;   /* -> The SCSI command parameter block  */
    IOMemoryDescriptor  *mem;       /* -> Memory to transfer, if any        */
    vm_task_t           client;     /* == The client task (for vm mapping)  */

        /* These fields are used by the IO thread to manage the IO request: */
        /*  cmdLock         Wait for the command to complete                */
        /*  link            Queue link for the command, disconnect, and     */
        /*                  pending queues.                                 */
        /*  timeoutPort     Port for timeout messages                       */
        /*  queueTag        SCSI tagged request if not QUEUE_TAG_NONTAGGED  */

    NXConditionLock *cmdLock;       /* client waits on this             */
    queue_chain_t   link;           /* for enqueueing on commandQ       */
//  port_t          timeoutPort;    /* for timeout messages             */
    UInt8           queueTag;       /* QUEUE_TAG_NONTAGGED or queue tag */
    UInt8           cdbLength;      /* Actual length of this command    */

        /* SCSI bus state variables. Note that currentDataIndex can exceed    */
        /* scsiReq->maxTransfer if the device sends (receives) more data than */
        /* we can receive (send). These values are NOT used for autosense.    */
        /* The first byte to transfer is at logical address                   */
        /*  gActiveCommand->buffer + currentDataIndex.                        */

    UInt32     currentDataIndex;    /* Where we are in the DATA transfer    */
    UInt32     savedDataIndex;      /* For SaveDataPointers                 */

    IOMemoryDescriptorState savedDataState;/* saved index for IOMemoryDescriptor*/

        /* Request management flags                                         */
        /*  flagActive                                                      */
        /*      Set if we're in the active array and active count           */
        /*      reflects our existance. Managed by [ self activateCmd ]     */
        /*      and [ self deactivateCmd : cmdBuf ].                        */
        /*      and that IOScheduleFunc() has been called.                  */
        /*  flagRequestSelectOK                                             */
        /*      Arbitration/selection succeeded for this request.           */
        /*  flagIsAutosense                                                 */
        /*      Set if we are executing an internally-generated             */
        /*      Request Sense command. If this is an autosense,             */
        /*      the operation is modified as follows:                       */
        /*          Arb/Select: Disable disconnects. Re-establish           */
        /*                      synchronous and fast for this target,       */
        /*                      use the current tag, if any.                */
        /*          Command:    Use an internally-generated Request Sense.  */
        /*          Data:       Read into our wired-down sense buffer.      */
        /*                      Do not touch the data index and transfer    */
        /*                      count variables. On completion, copy        */
        /*                      from our wired-down buffer to the caller's  */
        /*                      sense array.                                */
        /*          Completion: Good status, return SR_IOST_CHKSV to client.*/
        /*                      Bad status: never set isAutosense. Driver   */
        /*                      return SR_IOST_CHKSNV.                      */

    UInt32      flagActive:1,           /* We're in activeArray and activeCount */
                flagRequestSelectOK:1,  /* Did arbitration/selection succeed?   */
                flagIsAutosense:1,      /* Set if THIS is an autosense command  */
                pad:29;
        /* This is set by autosense Status phase.  */
    UInt8       autosenseStatus;    /* Did autosense complete ok?           */
        /* Statistics support.  */
    ns_time_t   startTime;          /* time cmd started                     */
    ns_time_t   disconnectTime;     /* time of last disconnect              */

} CommandBuffer;

        /* Condition variable states for commandBuf.cmdLock.    */

    enum { CMD_PENDING = 0, CMD_COMPLETE };

    /* Value of queueTag for nontagged commands.                    */
    /* This value is never used for the tag for tagged commands.    */

    enum { QUEUE_TAG_NONTAGGED  = 0 };

        /* Per-target info.                                                             */
        /*                                                                              */
        /* maxQueue is set to a non-zero value when we reach a target's queue size      */
        /* limit, detected by a STAT_QUEUE_FULL status. A value of zero means we        */
        /* have not reached the target's limit and we are free to queue additional      */
        /* commands (if allowed by the overall cmdQueueEnable flag).                    */
        /*                                                                              */
        /* syncXferPeriod and syncXferOffset are set to non-zero during sync            */
        /* transfer negotiation. Units of syncXferPeriod is NANOSECONDS, which          */
        /* differs from both the chip's register format (dependent on clock             */
        /* frequency and fast SCSI/fast clock enables) and the SCSI bus's format        */
        /* (which is 4 ns per unit).                                                    */
        /*                                                                              */
        /* cmdQueueDisable and syncDisable have a default (initial) value of            */
        /* zero regardless of the driver's overall cmdQueueEnable and syncModeEnable    */
        /* flags. They are set to one when a target explicitly tells us that the        */
        /* indicated feature is unsupported.                                            */
        /*                                                                              */
        /* negotiateSDTR        has one of the following values (defined in             */
        /*                      AppleMeshDefinitions.h):                                */
        /*          kSyncParmsAsync     Async with min period                           */
        /*          kSyncParmsFast      Offset = 15, period = Fast (10 MB/s)            */
        /* syncParms            Shadow of MESH syncParms register.                      */

    typedef struct
    {
        UInt8   maxQueue;           /* Max queue depth for this target  */
        UInt8   negotiateSDTR;      /* Synchronous negotiation control  */
        UInt8   syncParms;          /* Synchronous period and offset    */
        UInt8   inquiry_7;          /* 7th byte peeked fm Inquiry data  */
        UInt8   syncDisable;        /* No synchronous for this target   */
    } PerTargetData;

    typedef struct MeshRegister     /* Mesh registers:  */
    {
        volatile UInt8      transferCount0;     UInt8   pad00[ 0x0F ];
        volatile UInt8      transferCount1;     UInt8   pad01[ 0x0F ];
        volatile UInt8      xFIFO;              UInt8   pad02[ 0x0F ];
        volatile UInt8      sequence;           UInt8   pad03[ 0x0F ];
        volatile UInt8      busStatus0;         UInt8   pad04[ 0x0F ];
        volatile UInt8      busStatus1;         UInt8   pad05[ 0x0F ];
        volatile UInt8      FIFOCount;          UInt8   pad06[ 0x0F ];
        volatile UInt8      exception;          UInt8   pad07[ 0x0F ];
        volatile UInt8      error;              UInt8   pad08[ 0x0F ];
        volatile UInt8      interruptMask;      UInt8   pad09[ 0x0F ];
        volatile UInt8      interrupt;          UInt8   pad10[ 0x0F ];
        volatile UInt8      sourceID;           UInt8   pad11[ 0x0F ];
        volatile UInt8      destinationID;      UInt8   pad12[ 0x0F ];
        volatile UInt8      syncParms;          UInt8   pad13[ 0x0F ];
        volatile UInt8      MESHID;             UInt8   pad14[ 0x0F ];
        volatile UInt8      selectionTimeOut;
    } MeshRegister;

        /* The following structure shadows the MESH chip registers: */

    typedef union MESHShadow
    {   UInt32      longWord[ 3 ];      /* for debugging ease.                  */
        struct
        {   UInt8   interrupt;          /* Interrupt                            */
            UInt8   error;              /* Error register                       */
            UInt8   exception;          /* Exception register                   */
            UInt8   FIFOCount;          /* FIFO count                           */

            UInt8   busStatus0;         /* Bus phase + REQ, ACK, & ATN signals  */
            UInt8   busStatus1;         /* RST, BSY, SEL                        */
            UInt8   interruptMask;      /* Interrupt mask for debugging         */
            UInt8   transferCount0;     /* low  order byte of transfer count    */

            UInt8   transferCount1;     /* high order byte of transfer count    */
            UInt8   sequence;           /* Sequence register                    */
            UInt8   syncParms;          /* syncParms for debugging              */
            UInt8   destinationID;      /* Target ID                            */
        } mesh;
    } MESHShadow;

        /* Mesh Register set offsets    */

    enum
    {
        kMeshTransferCount0 =   0x00,
        kMeshTransferCount1 =   0x10,
        kMeshFIFO           =   0x20,
        kMeshSequence       =   0x30,
        kMeshBusStatus0     =   0x40,
        kMeshBusStatus1     =   0x50,
        kMeshFIFOCount      =   0x60,
        kMeshException      =   0x70,
        kMeshError          =   0x80,
        kMeshInterruptMask  =   0x90,
        kMeshInterrupt      =   0xA0,
        kMeshSourceID       =   0xB0,
        kMeshDestinationID  =   0xC0,
        kMeshSyncParms      =   0xD0,
        kMeshMESHID         =   0xE0,
        kMeshSelTimeOut     =   0xF0
    };

    enum { kMeshMESHID_Value    =   0x02 };     /* Read value of kMESHID lowest 5 bits only */


        /* MESH commands & modifiers for sequence register: */

    typedef enum
    {
        kMeshNoOpCmd            =   0x00,

        kMeshArbitrateCmd       =   0x01,
        kMeshSelectCmd          =   0x02,
        kMeshCommandCmd         =   0x03,
        kMeshStatusCmd          =   0x04,
        kMeshDataOutCmd         =   0x05,
        kMeshDataInCmd          =   0x06,
        kMeshMessageOutCmd      =   0x07,
        kMeshMessageInCmd       =   0x08,
        kMeshBusFreeCmd         =   0x09,
            /* non interrupting:    */
        kMeshEnableParity       =   0x0A,
        kMeshDisableParity      =   0x0B,
        kMeshEnableReselect     =   0x0C,
        kMeshDisableReselect    =   0x0D,
        kMeshResetMESH          =   0x0E,
        kMeshFlushFIFO          =   0x0F,
            /* Sequence command modifier bits:  */
        kMeshSeqDMA     =   0x80,   /* Data Xfer for this command will use DMA  */
        kMeshSeqTMode   =   0x40,   /* Target mode - unused                     */
        kMeshSeqAtn     =   0x20    /* ATN is to be asserted after command      */
    } MeshCommand;

        /* The bus Status Registers 0 & 1 have the actual   */
        /* bus signals WHEN READ.                           */

    enum                        /* bus Status Register 0 bits:  */
    {
        kMeshIO     =   0x01,   /* phase bit    */
        kMeshCD     =   0x02,   /* phase bit    */
        kMeshMsg    =   0x04,   /* phase bit    */
        kMeshAtn    =   0x08,   /* Attention signal */
        kMeshAck    =   0x10,   /* Ack signal       */
        kMeshReq    =   0x20,   /* Request signal   */
        kMeshAck32  =   0x40,   /* unused - 32 bit bus  */
        kMeshReq32  =   0x80    /* unused - 32 bit bus  */
    };

    enum { kMeshPhaseMask   =   (kMeshMsg + kMeshCD + kMeshIO)   };

    enum                        /* bus Status Register 1 bits:  */
    {
        kMeshSel    =   0x20,   /* Select signal    */
        kMeshBsy    =   0x40,   /* Busy signal      */
        kMeshRst    =   0x80    /* Reset signal     */
    };

    enum                                 /* Exception Register bits:   */
    {
        kMeshExcSelTO           =   0x01,   /* Selection timeout    */
        kMeshExcPhaseMM         =   0x02,   /* Phase mismatch       */
        kMeshExcArbLost         =   0x04,   /* lost arbitration     */
        kMeshExcResel           =   0x08,   /* reselection occurred */
        kMeshExcSelected        =   0x10,
        kMeshExcSelectedWAtn    =   0x20
    };

    enum                                    /* Error Register bits:     */
    {
        kMeshErrParity0         =   0x01,   /* parity error             */
        kMeshErrParity1         =   0x02,   /* unused - 32 bit bus      */
        kMeshErrParity2         =   0x04,   /* unused - 32 bit bus      */
        kMeshErrParity3         =   0x08,   /* unused - 32 bit bus      */
        kMeshErrSequence        =   0x10,   /* Sequence error           */
        kMeshErrSCSIRst         =   0x20,   /* Reset signal asserted    */
        kMeshErrDisconnected    =   0x40    /* unexpected disconnect    */
    };

    enum                                    /* Interrupt Register bits: */
    {
        kMeshIntrCmdDone    =   0x01,       /* command done             */
        kMeshIntrException  =   0x02,       /* exception occurred       */
        kMeshIntrError      =   0x04,       /* error     occurred       */
        kMeshIntrMask       =   (kMeshIntrCmdDone | kMeshIntrException | kMeshIntrError)
    };


    enum        /* Values for SyncParms MESH register:      */
    {           /* 1st nibble is offset, 2nd is period.     */
                /* Zero offset means async.                 */
        kSyncParmsAsync = 0x02, /* Async with min period = 2            */
        kSyncParmsFast  = 0xF0  /* offset = 15, period = Fast (10 MB/s) */
    };

        /* The following are specific to the MESH CCL               */
        /* Stage Names. (These were originally 'xxxx' identifiers,  */
        /* which is convenient for debugging, but results in many   */
        /* warning messages from the NeXT compiler.                 */
    enum
    {
        kcclStageIdle   = 0,        /*  0 - Idle    */
        kcclStageInit,              /*  1 - 'Init'  */
        kcclStageCCLx,              /*  2 - 'CCL~'  */
        kcclStageArb,               /*  3 - ' Arb'  */
        kcclStageSelA,              /*  4 - 'SelA'  */
        kcclStageMsgO,              /*  5 - 'MsgO'  */
        kcclStageCmdO,              /*  6 - 'CmdO'  */
        kcclStageXfer,              /*  7 - 'Xfer'  */
        kcclStageBucket,            /*  8 - 'Buck'  */
        kcclStageSyncHack,          /*  9 - 'Hack'  */
        kcclStageStat,              /*  A - ' Sta'  */
        kcclStageMsgI,              /*  B - 'MsgI'  */
        kcclStageFree,              /*  C - 'Free'  */
        kcclStageGood,              /*  D - 'Good'  */
        kcclStageStop,              /*  E - '++++'  */
        kcclTerminatorWithoutComma
    };

    /* offsets into the Channel Command List page:  */

#define kcclProblem     0x00    // Interrupt & Stop channel commands for anomalies
#define kcclCMDOdata    0x20    // reserve for 6, 10, 12 byte commands
#define kcclMSGOdata    0x30    // reserve for Identify, Tag stuff
#define kcclMSGOLast    0x3F    // reserve for last or only msg0ut byte
#define kcclMSGIdata    0x40    // reserve for Message In data
#define kcclBucket      0x48    // Bit Bucket
#define kcclStatusData  0x4F    // reserve for Status byte
#define kcclSenseCDB    0x50    // CDB for (auto) Sense
#define kcclBatchSize   0x60    // Current MESH batch size
#define kcclStageLabel  0x6C    // storage for label of last stage entered.
#define kcclSense       0x70    // Channel Commands for (Auto)Sense
#define kcclPrototype   0xC0    // Prototype MESH 4-command Transfer sequence
#define kcclStart       0x120   // Channel Program starts here with Arbitrate
#define kcclBrProblem   0x140   // channel command to wait for cmdDone & Br if problem
#define kcclMsgoStage   0x190   // Branch to single byte Message-Out
#define kcclMsgoBranch  0x1B0   // Branch to single byte Message-Out
#define kcclMsgoMTC     0x1D8   // MESH Transfer Count for MSGO (low order only)
#define kcclMsgoDTC     0x1F0   // DMA  Transfer Count for MSGO (low order only)
#define kcclLastMsgo    0x210   // Channel commands to put last/only byte of Message-Out
#define kcclCmdoStage   0x290   // Start of Command phase
#define kcclCmdoMTC     0x2C8   // MESH Transfer Count for CMDO (low order only)
#define kcclCmdoDTC     0x2E0   // DMA  Transfer Count for CMDO (low order only)
#define kcclReselect    0x2F0   // Reselect enters CCL here - Branch to xfer data
#define kcclOverrun     0x320   // data overrun - dump the excess in the bit bucket
#define kcclOverrunMESH 0x370   // data overrun - patch the MESH Seq Reg I/O
#define kcclOverrunDBDMA 0x380  // data overrun - patch the DBDMA I/O
#define kcclSyncCleanUp 0x3B0   // clean up at end of Sync xfer
#define kcclGetStatus   0x3D0   // Finish up with Status, Message In, and Bus Free
#define kcclMESHintr    0x4D0   // transaction done or going well
#define kcclSenseBuffer 0x500   // Buffer for Autosense data
#define kcclDataXfer    0x600   // INPUT or OUTPUT channel commands for data
#define kcclSenseResult 0x63C   // Result field in Sense INPUT channel command


    /* generic relocation types:    */

#define kRelNone    0x00    /* default - no relocation                      */
#define kRelMESH    0x01    /* Relocate to MESH register area               */
#define kRelCP      0x02    /* Relocate to Channel Program area             */
#define kRelCPdata  0x03    /* Relocate to Channel Program data structure   */
#define kRelPhys    0x04    /* Relocate to user Physical address space      */
#define kRelNoSwap  0x05    /* don't relocate or swap (Label)               */

    /* Relocatable ADDRESS types:   */

#define kRelAddress         0xFF        <<8 /* relocatable address mask         */
#define kRelAddressMESH     kRelMESH    <<8 /* MESH physical address            */
#define kRelAddressCP       kRelCP      <<8 /* Channel Program Physical address */
#define kRelAddressPhys     kRelPhys    <<8 /* User data Physical address       */

    /* Relocatable COMMAND-DEPENDENT types: */

#define kRelCmdDep      0xFF        /* relocatable command-dependent mask           */
#define kRelCmdDepCP    kRelCP      /* Channel Program command-dependent (branch)   */
#define kRelCmdDepLabel kRelNoSwap  /* Channel Program label - don't swap           */


    /* Channel Program macros:  */

#define STAGE(v)        STORE_QUAD  | KEY_SYSTEM    | 4, kcclStageLabel, v, kRelAddressCP | kRelCmdDepLabel
#define CLEAR_CMD_DONE  STORE_QUAD  | KEY_SYSTEM    | 1, kMeshInterrupt, kMeshIntrCmdDone, kRelAddressMESH
#define CLEAR_INT_REG   STORE_QUAD  | KEY_SYSTEM    | 1, kMeshInterrupt, kMeshIntrMask, kRelAddressMESH
#define CLR_PHASEMM     STORE_QUAD  | KEY_SYSTEM    | 1, kMeshInterrupt, kMeshIntrCmdDone | kMeshIntrException, kRelAddressMESH
#define MOVE_1(a,v,r)   STORE_QUAD  | KEY_SYSTEM    | 1, a, v, r
#define MOVE_4(a,v,r)   STORE_QUAD  | KEY_SYSTEM    | 4, a, v, r
#define MESH_REG(a,v)   STORE_QUAD  | KEY_SYSTEM    | 1, a, v, kRelAddressMESH
#define MESH_REG_WAIT(a,v)  STORE_QUAD  | KEY_SYSTEM | kWaitIfTrue | 1, a, v, kRelAddressMESH

#define SENSE(c)        INPUT_LAST  | kBranchIfFalse | kWaitIfTrue | c, kcclSenseBuffer,  kcclProblem, kRelAddressCP   | kRelCmdDepCP
#define MSGO(a,c)       OUTPUT_LAST | kBranchIfFalse | kWaitIfTrue | c, a,                kcclProblem, kRelAddressCP   | kRelCmdDepCP
#define CMDO(c)         OUTPUT_LAST | kBranchIfFalse | kWaitIfTrue | c, kcclCMDOdata,     kcclProblem, kRelAddressCP   | kRelCmdDepCP
#define MSGI(c)         INPUT_LAST  | kBranchIfFalse | kWaitIfTrue | c, kcclMSGIdata,     kcclProblem, kRelAddressCP   | kRelCmdDepCP
#define STATUS_IN       INPUT_LAST  | kBranchIfFalse | kWaitIfTrue | 1, kcclStatusData,   kcclProblem, kRelAddressCP   | kRelCmdDepCP
#define BUCKET          INPUT_LAST  | kBranchIfFalse               | 8, kcclBucket,       kcclProblem, kRelAddressCP   | kRelCmdDepCP

#define BRANCH(a)        NOP_CMD | kBranchAlways,                0, a, kRelCmdDepCP
#define BR_IF_PROBLEM    NOP_CMD | kBranchIfFalse | kWaitIfTrue, 0, kcclProblem, kRelCmdDepCP
#define BR_NO_PROBLEM(a) NOP_CMD | kBranchIfTrue               , 0, a, kRelCmdDepCP
#define STOP(L)          STOP_CMD,                               0, L, kRelCmdDepLabel
#define INTERRUPT(a)     NOP_CMD | kIntAlways, 0, a, 0
#define RESERVE          0xCEFECEFE, 0xCEFECEFE, 0xCEFECEFE, 0xCEFECEFE
#define WAIT_4_CMDDONE   NOP_CMD | kWaitIfTrue, 0, 0, 0

//#define SWAP(x) (UInt32)EndianSwap32Bit( (UInt32)( x ) )
#define SWAP(x) (UInt32)EndianSwap32( (UInt32)( x ) )


    /* Return values from hardwareStart.    */

    typedef enum
    {   kHardwareStartOK,           /* command started successfully         */
        kHardwareStartRejected,     /* command rejected, try another        */
        kHardwareStartBusy          /* hardware not ready for command       */
    } HardwareStartResult;

         /* This structure defines the DBDMA Channel Command descriptor.    */
         /***** WARNING:    Endian-ness issues must be considered when  *****/
         /***** performing load/store!                                  *****/
         /***** DBDMA specifies memory organization as quadlets so it   *****/
         /***** is not correct to think of either the operation or      *****/
         /***** result field as two 16-bit fields. This would have      *****/
         /***** undesirable effects on the byte ordering within their   *****/
         /***** respective quadlets. Use the accessor macros provided   *****/
         /***** below.                                                  *****/

    struct DBDMADescriptor
    {
        UInt32  operation;       /* cmd || key || i || b || w || reqCount   */
        UInt32  address;
        UInt32  cmdDep;
        UInt32  result;         /* xferStatus || resCount   */
    };
    typedef struct DBDMADescriptor  DBDMADescriptor;

    typedef DBDMADescriptor *DBDMADescriptorPtr;

        /* These constants define the DBDMA channel command operations and modifiers.*/

    enum        /* Command.cmd operations   */
    {
        OUTPUT_MORE     = 0x00000000,
        OUTPUT_LAST     = 0x10000000,
        INPUT_MORE      = 0x20000000,
        INPUT_LAST      = 0x30000000,
        STORE_QUAD      = 0x40000000,
        LOAD_QUAD       = 0x50000000,
        NOP_CMD         = 0x60000000,
        STOP_CMD        = 0x70000000,
        kdbdmaCmdMask   = 0xF0000000
    };


    enum
    {       /* Command.key modifiers                            */
            /* (choose one for INPUT, OUTPUT, LOAD, and STORE)  */
        KEY_STREAM0         = 0x00000000,   /* default modifier*/
        KEY_STREAM1         = 0x01000000,
        KEY_STREAM2         = 0x02000000,
        KEY_STREAM3         = 0x03000000,
        KEY_REGS            = 0x05000000,
        KEY_SYSTEM          = 0x06000000,
        KEY_DEVICE          = 0x07000000,
        kdbdmaKeyMask       = 0x07000000,   /* Command.i modifiers (choose one for INPUT, OUTPUT, LOAD, STORE, and NOP)*/
        kIntNever           = 0x00000000,   /* default modifier     */
        kIntIfTrue          = 0x00100000,
        kIntIfFalse         = 0x00200000,
        kIntAlways          = 0x00300000,
        kdbdmaIMask         = 0x00300000,   /* Command.b modifiers (choose one for INPUT, OUTPUT, and NOP)*/
        kBranchNever        = 0x00000000,   /* default modifier     */
        kBranchIfTrue       = 0x00040000,
        kBranchIfFalse      = 0x00080000,
        kBranchAlways       = 0x000C0000,
        kdbdmaBMask         = 0x000C0000,   /* Command.w modifiers (choose one for INPUT, OUTPUT, LOAD, STORE, and NOP)*/
        kWaitNever          = 0x00000000,   /* default modifier     */
        kWaitIfTrue         = 0x00010000,
        kWaitIfFalse        = 0x00020000,
        kWaitAlways         = 0x00030000,
        kdbdmaWMask         = 0x00030000,    /* operation masks     */
    };


        /* This is a temporary implementation of EndianSwap32Bit    */
        /* until the correct library/method is made available.      */
        /*  @param  value       The value to change                 */
        /*  @result The value endian-swapped.                       */

#ifdef CRAP
    static inline unsigned EndianSwap32Bit( unsigned value )
    {
        register unsigned   temp;

        temp  = ((value & 0xFF000000) >> 24);
        temp |= ((value & 0x00FF0000) >>  8);
        temp |= ((value & 0x0000FF00) <<  8);
        temp |= ((value & 0x000000FF) << 24);
        return temp;
    }
#else
    static __inline__ UInt32     EndianSwap32( UInt32 y )
    {
        UInt32		     result;
        volatile UInt32   x;

        x = y;
        __asm__ volatile("lwbrx %0, 0, %1" : "=r" (result) : "r" (&x) : "r0");
        return result;
    }
#endif /* CRAP */


    typedef struct globals      /* Globals for this module (not per instance)   */
    {
        UInt32       evLogFlag; // debugging only
        UInt8       *evLogBuf;
        UInt8       *evLogBufe;
        UInt8       *evLogBufp;
        UInt8       intLevel;

        MESHShadow  shadow; // move to per instance??? /* Last MESH register state      */

        UInt32      cclLogAddr,     cclPhysAddr;    // for debugging/miniMon ease
        UInt32      meshAddr;                       // for debugging/miniMon ease
    } globals;


    /* "Global" data for each instance of the MESH Host Bus Adapter:   */

@interface AppleMesh_SCSI : IOSCSIController < IOPower >
{
        /* These globals locate the hardware interfaces in  */
        /* logical and physical address spaces.             */

    MeshRegister    *meshAddr;              /* -> Mesh registers (logical)      */
    PhysicalAddress gMESHPhysAddr;          /* -> Mesh registers (physical)     */

    dbdma_regmap_t  *dbdmaAddr;             /* -> DBDMA registers (logical)     */
    PhysicalAddress dbdmaAddrPhys;          /* -> DBDMA registers (physical)    */

    PhysicalAddress cclPhysAddr;            /* -> DBDMA channel area (physical) */
    DBDMADescriptor *cclLogAddr;            /* -> DBDMA channel area (logical)  */
    UInt32          cclLogAddrSize;         /* == DBDMA channel allocated size  */
    UInt32          gDBDMADescriptorMax;    /* Number of DATA descriptors       */


        /* There are 3 queues: incomingCmdQ, pendingCmdQ, disconnectedCmdQ.     */
        /* Commands are passed from exported methods to the IO thread via       */
        /* incomingCmdQ, which is protected by incomingCmdLock.                 */
        /* Commands which are disconnected but not complete are kept in         */
        /* disconnectedCmdQ.                                                    */
        /* Commands which have been dequeued from incomingCmdQ by the IO thread */
        /* but which have not been started because a command is currently active*/
        /* on the bus are kept in pendingCmdQ. This queue also holds commands   */
        /* pushed back when we lose arbitration.                                */

        /* The currently active command, if any, is kept in gActiveCommand.     */
        /* Only commandBufs with op == kCommandExecute are ever placed in       */
        /* gActiveCommand.                                                      */
    id              incomingCmdLock;           /* NXLock for incomingCmdQ       */
    queue_head_t    incomingCmdQ;
    queue_head_t    pendingCmdQ;
    queue_head_t    disconnectedCmdQ;
    queue_head_t    abortCmdQ;

        /* This is the command we're currently execution. If NULL, the Mac      */
        /* is idle (or all commands are disconnected). Normally, gCurrentTarget */
        /* and gCurrentLUN track the values in the active command's associated  */
        /* SCSI request. They are set to kInvalidTarget and kInvalidLUN at      */
        /* initialization, command deactivation, command complete, and command  */
        /* disconnect. They are set to valid values (with no active command)    */
        /* during reselection. This is tricky, so look carefully at the code.   */
    CommandBuffer   *gActiveCommand;        /* -> The currently executing command   */
    UInt32          gCurrentTarget;         /* == The current target bus ID         */
    UInt32          gCurrentLUN;            /* == The current target LUN            */

        /* Global option flags, accessible via instance table or setIntValues. Note: some of
         * these are intended for debugging. However, users may need to disable command
         * queuing, synchronous, or fast to handle device or bus limitations. The
         * architecture-specific initialization "looks" at the device to determine whether
         * specific features (such as synchronous) are supported.
         *  gOptionAutoSenseEnable      Debug only, normally set
         *  gOptionCmdQueueEnable       Enable tagged queuing.
         *  gOptionSyncModeEnable       Enable synchronous transfers (clear if problems)
         *  gOptionFastModeEnable       Enable fast transfers (clear if problems)
         *  gOptionExtendTiming         Extended selection timing (debug, unused)
         *  gFlagIOThreadRunning        Set when IO thread is initialized. Needed
         *                              for shutdown.
         *  gFlagIncompleteDBDMA        Set in the data transfer setup if there was
         *                              so much data that the entire transfer could not
         *                              be stored in the CCL area. If so, the interrupt
         *                              service routine ("good completion") will restart
         *                              the data transfer operation.
         */
    UInt32      gOptionAutoSenseEnable      : 1,
                gOptionCmdQueueEnable       : 1,
                gOptionSyncModeEnable       : 1,
                gOptionFastModeEnable       : 1,
                gOptionExtendTiming         : 1,
                gFlagIOThreadRunning        : 1,    /* Set at init              */
                gFlagIncompleteDBDMA        : 1,    /* Need more DMA            */
                gFlagReselecting            : 1,    /* Reselection in progress  */
                pad                         : 24;

        /* Array of active IO counters, one counter per LUN per target.         */
        /* If command queueing is disabled, the max value of each counter is 1. */
        /* gActiveCount is the sum of all elements in activeArray.              */

    UInt8       gActiveArray[ SCSI_NTARGETS ][ SCSI_NLUNS ];
    UInt32      gActiveCount;

        /* These variables change during SCSI IO operation.                 */
        /*  msgOutPtr       Points to the next free byte in the MSGO buffer */
        /*                  in the shared CCL area.                         */

    UInt8       *msgOutPtr;             /* ptr to message-out data  */

        /* These variables manage Message-In bus phase. Because the */
        /* Message-In handler uses programmed IO, gMsgInCount and   */
        /* gMsgInState are actually local variables to the message  */
        /* reader, and are here for debugging convenience.          */

    UInt8       gMsgInBuffer[ 16 ];
    SInt8       gMsgInCount;            /* Message bytes still to read  */
    MsgInState  gMsgInState;            /* How are we handling messages */

#define kFlagMsgIn_Reject       0x01
#define kFlagMsgIn_Disconnect   0x02
    UInt8       gMsgInFlag;

#define kFlagMsgOut_SDTR        0x01
#define kFlagMsgOut_Queuing     0x02
    UInt8       gMsgOutFlag;

        /* These variables are used during reselection to select the correct   */
        /* (tagged) command. msgInTagType is the last Tagged Queue message     */
        /* received from a target during reselection. msgInTag is the          */
        /* tag value. Currently, we should only see a Simple Queue Tag.        */
    UInt8       msgInTagType;           /* Last tag type                */
    UInt8       msgInTag;               /* Last tag value               */

        /* Hardware related variables:  */

    UInt8       gInitiatorID;           /* Our SCSI ID                  */
    UInt8       gInitiatorIDMask;       /* BusID bitmask for selection  */
    UInt8       gSelectionTimeout;      /* In MESH 10 msec units        */

        /* commandBuf->queueTag for next IO. This is never zero;    */
        /* for all requests involving a T/L/Q nexus, a queue tag    */
        /* of zero indicates a nontagged command.                   */
    UInt8       gNextQueueTag;

    PerTargetData   gPerTargetData[ SCSI_NTARGETS ];

        /* Statistics support:  */

    UInt32      gMaxQueueLen;
    UInt32      gQueueLenTotal;
    UInt32      gTotalCommands;
}

    /* Public methods (called by higher-level driver functions) */

+ (Boolean) probe  : deviceDescription;     /* Initialize the SCSI driver.  */
- free;                                     /* Shutdown the driver.         */


- (sc_status_t) executeRequest                  /* Execute a SCSI IO request    */
                                : (IOSCSIRequest*)  scsiReq
                        buffer  : (void*)           buffer
                        client  : (vm_task_t)       client;

    /* Execute a SCSI request using an IOMemoryDescriptor.                      */
    /* This allows callers to provide (kernel-resident) logical scatter-gather  */
    /* lists. For compatibility with existing implementations, the low-level    */
    /* SCSI device driver must first ensure that                                */
    /* executeRequest:ioMemoryDescriptor is supported by executing:             */
    /*  [controller respondsToSelector : executeRequest:ioMemoryDescriptor]     */
    /* @param scsiReq   The SCSI request command record, including the          */
    /* target device and LUN, the command to execute, and various control flags.*/
    /* @param ioMemoryDescriptor The data buffer(s), if any. This may be NULL   */
    /* if no data phase is expected.                                            */
    /* @param client   The client task that "owns" the memory buffer.           */
    /* @return      Return a bus adaptor specific error status.                 */
- (sc_status_t) executeRequest  : (IOSCSIRequest*)scsiReq 
            ioMemoryDescriptor  : (IOMemoryDescriptor*)ioMemoryDescriptor;

- (sc_status_t) resetSCSIBus;                   /* Reset the SCSI bus           */
- (void)        resetStatistics;                /* Reset statistics buffers     */
- (unsigned)    numQueueSamples;
- (unsigned)    sumQueueLengths;
- (unsigned)    maxQueueLength;

    /* interruptOccurred is a public method called by the       */
    /* IO thread in IODirectDevice when an interrupt occurs.    */
- (void)        interruptOccurred;

    /* timeoutOccurred is a public method called by the */
    /* IO thread in IODirectDevice when it receives a   */
    /* timeout message.                                 */

- (void)       timeoutOccurred;

#if APPLE_MESH_ENABLE_GET_SET

- (IOReturn)    setIntValues    : (unsigned*)       parameterArray
                forParameter    : (IOParameterName) parameterName
                count           : (unsigned)        count;

- (IOReturn)    getIntValues    : (unsigned*)       parameterArray
                forParameter    : (IOParameterName) parameterName
                count           : (unsigned*)       count;          /* in/out */

    /* get/setIntValues parameters: */

#define APPLE_MESH_AUTOSENSE                "AutoSense"
#define APPLE_MESH_CMD_QUEUE                "CmdQueue"
#define APPLE_MESH_SYNC                     "Synchronous"
#define APPLE_MESH_FAST_SCSI                "FastSCSI"
#define APPLE_MESH_RESET_TARGETS            "ResetTargets"
#define APPLE_MESH_RESET_TIMESTAMP          "ResetTimestamp"
#define APPLE_MESH_ENABLE_TIMESTAMP         "EnableTimestamp"
#define APPLE_MESH_DISABLE_TIMESTAMP        "DisableTimestamp"
#define APPLE_MESH_PRESERVE_FIRST_TIMESTAMP "PreserveFirstTimestamp"
#define APPLE_MESH_PRESERVE_LAST_TIMESTAMP  "PreserveLastTimestamp"
#define APPLE_MESH_READ_TIMESTAMP           "ReadTimestamp"
#define APPLE_MESH_STORE_TIMESTAMP          "StoreTimestamp"

    /*
     * Recording and setting timestamps may be done using getIntValues (this permits
     * access from non-privileged tasks.
     *      ResetTimestamp          Clear the timestamp vector - do this before starting
     *                              a sequence (no parameters)
     *      EnableTimestamp         Start recording (no parameters) (default)
     *      DisableTimestamp        Stop recording (no parameters)
     *      PreserveFirstTimestamp  Stop recording when the buffer fills (until it is emptied)
     *      PreserveLastTimestamp   Discard old values when new arrive (default)
     *      ReadTimestamp           Read a vector of timestamps (see sample below)
     *      StoreTimestamp          Store a timestamp (from user mode) (see sample below)
     * ReadTimestamp copies timestamps from the internal database to user-specified vector.
     * Because getIntValues parameters are defined in int units, the code is slighthly
     * non-obvious:
     *      TimestampDataRecord     myTimestamps[ 123 ];
     *      unsigned                count;
     *      count = sizeof (myTimestamps) / sizeof (unsigned);
     *      [scsiDevice getIntValues
     *                          : (unsigned int *) myTimestamps
     *          forParameter    : APPLE_MESH_READ_TIMESTAMP
     *          count           : &count
     *      ];
     *      count = (count * sizeof (unsigned)) / sizeof (TimestampDataRecord);
     *      for (i = 0; i < count; i++) {
     *          Process(myTimestamps[i]);
     *      }
     * Applications can store timestamps using one of three parameter formats:
     *      unsigned               paramVector[4];
     * Tag only -- the library will supply the event time
     *      paramVector[0] = kMyTagValue;
     *      [scsiDevice getIntValues
     *                          : paramVector
     *          forParameter    : "StoreTimestamp"
     *          count           : 1
     *      ];
     * Tag plus value:
     *      paramVector[0] = kMyTagValue;
     *      paramVector[1] = 123456;
     *      [scsiDevice getIntValues
     *                          : paramVector
     *          forParameter    : "StoreTimestamp"
     *          count           : 2
     *      ];
     * Tag plus value + time:
     *      paramVector[0] = kMyTagValue;
     *      paramVector[1] = 123456;
     *      IOGetTimestamp( (ns_time_t*)&paramVector[2] );
     *      [scsiDevice getIntValues
     *                          : paramVector
     *          forParameter    : "StoreTimestamp"
     *          count           : 4
     *      ];
     * Note that you can combine tag only with tag plus value plus time to measure
     * user->device latency.
     */

#endif APPLE_MESH_ENABLE_GET_SET

@end



@interface AppleMesh_SCSI( Hardware )

-                       InitializeHardware : deviceDescription;
- (IOReturn)            ResetHardware      : (Boolean)resetSCSIBus;
- (HardwareStartResult) hardwareStart      : (CommandBuffer*)cmdBuf;

@end


    /* These macros are used to access words (32 bit) and bytes (8 bit) in  */
    /* the channel command area. They may be used as source or destination. */
    /* CCLDescriptor is aligned to a descriptor start, CCLAddress is just   */
    /* an address pointer.                                                  */
#define CCLAddress(offset)      (((UInt8*)cclLogAddr) + (offset))
#define CCLDescriptor(offset)   ((DBDMADescriptor*)CCLAddress(offset))
#define CCLWord(offset)         (*((UInt32*)CCLAddress(offset)))
#define CCLByte(offset)         (*((UInt8*)CCLAddress(offset)))

@interface AppleMesh_SCSI ( HardwarePrivate )

- (IOReturn) AllocHdwAndChanMem : deviceDescription;
- (void) InitAutosenseCCL;
- (void) UpdateCP : (Boolean) reselecting;
- (void) StartBucket;
- (void) SetupMsgO;
- (void) ClearCPResults;
- (void) InitCP;

@end

@interface AppleMesh_SCSI( MeshInterrupt )

- (void) DoHardwareInterrupt;       /* Respond to an Interrupt Service message. */
- (void) ProcessInterrupt;
- (void)    DoInterruptStageArb;
- (void)    DoInterruptStageSelA;
- (void)    DoInterruptStageMsgO;
- (void)    DoInterruptStageCmdO;
- (void)    DoInterruptStageXfer;
- (void)    DoInterruptStageXferAutosense;
- (void)    DoInterruptStageGood;
- (IOReturn)    DoMessageInPhase;           /* Handle MSGI phase.   */
- (void)        ProcessMSGI;
- (void)        HandleReselectionInterrupt; /* Process a reselection interrupt. */
- (Boolean)     getReselectionTargetID;

@end


    /* SCSI command status (from status phase)  */

#define kScsiStatusGood             0x00    /* Normal completion        */
#define kScsiStatusCheckCondition   0x02    /* Need GetExtendedStatus   */
#define kScsiStatusConditionMet     0x04
#define kScsiStatusBusy             0x08    /* Device busy (self-test?) */
#define kScsiStatusIntermediate     0x10    /* Intermediate status      */
#define kScsiStatusIntermediateMet  0x14    /* Intermediate cond. met   */
#define kScsiStatusResConflict      0x18    /* Reservation conflict     */
#define kScsiStatusTerminated       0x22    /* Command Terminated       */
#define kScsiStatusQueueFull        0x28    /* Target can't do command  */
#define kScsiStatusReservedMask     0x3E    /* Vendor specific?         */

    /* SCSI command codes. Commands defined as ...6, ...10, ...12, are          */
    /* six-byte, ten-byte, and twelve-byte variants of the indicated command.   */

    /* These commands are supported for all devices.   */

#define kScsiCmdChangeDefinition    0x40
#define kScsiCmdCompare             0x39
#define kScsiCmdCopy                0x18
#define kScsiCmdCopyAndVerify       0x3A
#define kScsiCmdInquiry             0x12
#define kScsiCmdLogSelect           0x4C
#define kScsiCmdLogSense            0x4D
#define kScsiCmdModeSelect12        0x55
#define kScsiCmdModeSelect6         0x15
#define kScsiCmdModeSense12         0x5A
#define kScsiCmdModeSense6          0x1A
#define kScsiCmdReadBuffer          0x3C
#define kScsiCmdRecvDiagResult      0x1C
#define kScsiCmdRequestSense        0x03
#define kScsiCmdSendDiagnostic      0x1D
#define kScsiCmdTestUnitReady       0x00
#define kScsiCmdWriteBuffer         0x3B

    /* These commands are supported by direct-access devices only:   */

#define kScsiCmdFormatUnit          0x04
#define kSCSICmdCopy                0x18
#define kSCSICmdCopyAndVerify       0x3A
#define kScsiCmdLockUnlockCache     0x36
#define kScsiCmdPrefetch            0x34
#define kScsiCmdPreventAllowRemoval 0x1E
#define kScsiCmdRead6               0x08
#define kScsiCmdRead10              0x28
#define kScsiCmdReadCapacity        0x25
#define kScsiCmdReadDefectData      0x37
#define kScsiCmdReadLong            0x3E
#define kScsiCmdReassignBlocks      0x07
#define kScsiCmdRelease             0x17
#define kScsiCmdReserve             0x16
#define kScsiCmdRezeroUnit          0x01
#define kScsiCmdSearchDataEql       0x31
#define kScsiCmdSearchDataHigh      0x30
#define kScsiCmdSearchDataLow       0x32
#define kScsiCmdSeek6               0x0B
#define kScsiCmdSeek10              0x2B
#define kScsiCmdSetLimits           0x33
#define kScsiCmdStartStopUnit       0x1B
#define kScsiCmdSynchronizeCache    0x35
#define kScsiCmdVerify              0x2F
#define kScsiCmdWrite6              0x0A
#define kScsiCmdWrite10             0x2A
#define kScsiCmdWriteAndVerify      0x2E
#define kScsiCmdWriteLong           0x3F
#define kScsiCmdWriteSame           0x41

    /* These commands are supported by sequential devices:  */

#define kScsiCmdRewind              0x01
#define kScsiCmdWriteFilemarks      0x10
#define kScsiCmdSpace               0x11
#define kScsiCmdLoadUnload          0x1B

    /* ANSI SCSI-II for CD-ROM devices. */
#define kScsiCmdReadCDTableOfContents   0x43

    /* Message codes (for Msg In and Msg Out phases).   */

#define kScsiMsgAbort                   0x06
#define kScsiMsgAbortTag                0x0D
#define kScsiMsgBusDeviceReset          0x0C
#define kScsiMsgClearQueue              0x0E
#define kScsiMsgCmdComplete             0x00
#define kScsiMsgDisconnect              0x04
#define kScsiMsgIdentify                0x80
#define kScsiMsgIdentifyLUNMask         0x07    /* LUN bits in Identify message */
#define kScsiMsgIgnoreWideResdue        0x23
#define kScsiMsgInitiateRecovery        0x0F
#define kScsiMsgInitiatorDetectedErr    0x05
#define kScsiMsgLinkedCmdComplete       0x0A
#define kScsiMsgLinkedCmdCompleteFlag   0x0B
#define kScsiMsgParityErr               0x09
#define kScsiMsgRejectMsg               0x07
#define kScsiMsgModifyDataPtr           0x00    /* Extended msg     */
#define kScsiMsgNop                     0x08
#define kScsiMsgHeadOfQueueTag          0x21    /* Two byte msg     */
#define kScsiMsgOrderedQueueTag         0x22    /* Two byte msg     */
#define kScsiMsgSimpleQueueTag          0x20    /* Two byte msg     */
#define kScsiMsgReleaseRecovery         0x10
#define kScsiMsgRestorePointers         0x03
#define kScsiMsgSaveDataPointers        0x02
#define kScsiMsgSyncXferReq             0x01    /* Extended msg     */
#define kScsiMsgWideDataXferReq         0x03    /* Extended msg     */
#define kScsiMsgTerminateIOP            0x11
#define kScsiMsgExtended                0x01
#define kScsiMsgEnableDisconnectMask    0x40

#define kScsiMsgOneByteMin          0x02
#define kScsiMsgOneByteMax          0x1F
#define kScsiMsgTwoByteMin          0x20
#define kScsiMsgTwoByteMax          0x2F



    /* These methods bang on the MESH chip.         */
    /* Many should be redone as inline functions.   */

@interface AppleMesh_SCSI( Mesh )

- (IOReturn) ResetMESH  : (Boolean) resetSCSIBus;
- (IOReturn) DoHBASelfTest;
- (IOReturn) WaitForMesh : (Boolean) clearInterrupts;
- (IOReturn) WaitForReq;
- (void) SetSeqReg : (MeshCommand) meshCommand;
- (void) RunDBDMA : (UInt32) offset     stageLabel  : (UInt32) stageLabel;
- (void) GetHBARegsAndClear : (Boolean) clearInts;
- (void) SetIntMask : (UInt8) interruptMask;
- (void) AbortActiveCommand;
- (void) AbortDisconnectedCommand;

- (void) logTimestamp   : (const char*) reason;

@end


@interface AppleMesh_SCSI( Private )

    /* Send a command to the controller thread, and wait for its completion.    */
    /* Only invoked by publicly exported methods in SCSIController.m.           */

- (IOReturn) executeCmdBuf : (CommandBuffer*) cmdBuf;

    /* Abort all active and disconnected commands with specified status.    */
    /* No hardware action. Used by threadResetBus and during processing     */
    /* of a kCommandAbortRequest command.                                   */
- (void) abortAllCommands : (sc_status_t) status;

    /* IO thread version of resetSCSIBus and executeRequest.   */
- (void) threadResetBus : (const char*) reason;


- (void) threadExecuteRequest : (CommandBuffer*) cmdBuf;

    /* Methods called by other modules in this driver:  */

    /* Called when a transaction associated with cmdBuf is complete. Notify
     * waiting thread. If cmdBuf->scsiReq exists (i.e., this is not a reset
     * or an abort), scsiReq->driverStatus must be valid. If cmdBuf is activeCmd,
     * caller must remove from activeCmd.
     */
- (void) ioComplete : (CommandBuffer*) cmdBuf;

    /* A target reported a full queue. Push this command back on the pending
     * queue and try it again, later. (Return SR_IOST_GOOD if successful,
     * SR_IOST_BADST on failure.
     */
- (sc_status_t) pushbackFullTargetQueue : (CommandBuffer*) cmdBuf;

    /* A command couldn't be issued (because a target is trying to reselect
     * us or we lost arbitration for some other reason). Push this request
     * onto the front of the pending request queue.
     */
- (void) pushbackCurrentRequest : (CommandBuffer*) cmdBuf;

    /* A command can't be continued. Perhaps there is no target.    */
- (void) killCurrentRequest;

    /* IO associated with activeCmd has disconnected. Place it  */
    /* on disconnectQ and enable another transaction.           */
- (void) disconnect;

    /* Specified target, lun, and queueTag is trying to reselect. If we have
     * a CommandBuffer for this TLQ nexus on disconnectQ, remove it, make it the
     * current activeCmd, and return YES. Else return NO.
     * A value of zero for queueTag indicates a nontagged command (zero is never
     * used as the queue tag value for a tagged command).
     */
- (IOReturn) reselectNexus  : (UInt8) target
                lun         : (UInt8) lun
                queueTag    : (UInt8) queueTag;
- (Boolean) commandCanBeStarted : (CommandBuffer*) cmdBuf;
- (void) selectNextRequest; /* Choose the next request that can be started. */
- (void) killActiveCommand : (sc_status_t) status;  // mlj added
- (void) activateCommand : (CommandBuffer*) cmdBuf;
- (void) deactivateCmd : (CommandBuffer*) cmdBuf;

    /* Kill everything in the indicated queue. Called after bus reset.  */
- (void) killQueue : (queue_head_t*)queuePtr  finalStatus : (sc_status_t)scsiStatus;
- (void) UpdateCurrentIndex;

@end

#if USE_ELG && CustomMiniMon
            /* for debugging:   */
    extern void     EvLog( UInt32 a, UInt32 b, UInt32 ascii, char* str );
    extern void     Pause( UInt32 a, UInt32 b, UInt32 ascii, char* str );
    extern void     AllocateEventLog( UInt32 ); // defined in miniMon
#endif /* NotMiniMon */
    extern void     call_kdp();     // for debugging


    /* Usage:
     *  1.  In the makefile (or elsewhere), define TIMESTAMP non-zero. If zero, this
     *      code will be stubbed out.
     *  2.  In your initialization routine, call MakeTimestampRecord() to create a
     *      timestamp record. This will be stored in a static, private, variable.
     *  3.  When you want to time something, call StoreTimestamp() as follows:
     *          {
     *              ns_time_t       eventTime;
     *              IOGetTimestamp(&eventTime);
     *              StoreTimestamp(timestampTag, timestampValue, eventTime);
     *          }
     *      Where timestampTag and timestampValue are 32-bit unsigned integers
     *      that are not otherwise interpreted by the Timestamp library. By
     *      convention, timestampTag contains a 4-byte character (Macintosh OSType)
     *      that distinguishes timing events. The OSType and OSTag macros
     *      can be used to construct tag values. OSTag is useful for recording
     *      elapsed time:
     *          StoreTimestamp(OSTag('+', "foo"), 0, startTime);
     *          ...
     *          StoreTimestamp(OSTag('-', "foo"), 0, endTime);
     */


//#ifndef TIMESTAMP
#define TIMESTAMP   0   // mlj - resolve dup symbols with Curio     /* TEMP TEMP TEMP */
//#endif

    /* Construct an OSType from four characters.     */
#define OSType(c0, c1, c2, c3) ( \
        (   ((c0) << 24)        \
         |  ((c1) << 16)        \
         |  ((c2) <<  8)        \
         |  ((c3) <<  0) ))
    /* Construct an OSType from a single character and the  */
    /* first three characters from a given string.          */
#define OSTag(where, what)  (OSType((where), (what)[0], (what)[1], (what)[2]))

        /*  ._______________________________________________________________________________.
            | Each timestamp entry contains the following information:                      |
            |   timestampTag    A user-specified OSType that identifies this timestamp      |
            |   timestampValue  A user-specified additional value                           |
            |   eventTime       The system UpTime value at the time the data was collected. |
            ._______________________________________________________________________________.
        */
    struct TimestampDataRecord
    {
        OSType          timestampTag;       /* Caller's tag parameter       */
        UInt32          timestampValue;     /* Caller's value parameter     */
        ns_time_t       eventTime;          /* UpTime() at Timestamp call   */
    };
    typedef struct TimestampDataRecord  TimestampDataRecord, *TimestampDataPtr;

#if TIMESTAMP  /* (( */
    void    MakeTimestampRecord( UInt32 nEntries );
    void    StoreTimestamp( OSType      timestampTag,
                            UInt32      timestampValue,
                            ns_time_t   timestampEvent );
        /**
         * Returns the next timestamp, if any, in resultData.
         * @param   resultData      Where to store the data
         * @return  TRUE            Valid data returned
         *          FALSE           No data is available.
         */
    Boolean  ReadTimestamp( TimestampDataPtr resultData );
        /**
         * Return a vector of timestamps.
         * @param   resultVector    Where to store the data
         * @param   count           On entrance, this has the maximum number of elements
         *                          to return. On exit, this will have the actual number
         *                          of elements that were returned.
         * Note that, if the semaphore is blocked, ReadTimestampVector will not return any
         * data. Data cannot be collected while ReadTimestampVector is copying data
         * to the user's buffer. Note that, since the user's buffer will typically be
         * in pageable memory, pageing I/O that might otherwise be timestamped will
         * be lost.
         */
    void    ReadTimestampVector( TimestampDataPtr resultVector, UInt32 *count );        /* -> Max count, <-actual   */
    Boolean EnableTimestamp( Boolean enableTimestamp );
    Boolean PreserveTimestamp( Boolean preserveFirst );
    void    ResetTimestampIndex(void);
    UInt32  GetTimestampSemaphoreLostCounter( void );
#else  /* )( not TIMESTAMP: */
#define MakeTimestampRecord( nEntries ) /* Nothing */
#define StoreTimestamp( timestampTag, timestampValue, timestampEvent )  /* Nothing  */
#define ReadTimestamp(resultData)   (0)                                 /* Fails    */
#define ReadTimestampVector(resultVector, count)    \
    do { if ( count ) { *(count) = 0; }  } while (0)
#define EnableTimestamp( enableTimestamp )  (enableTimestamp)
#define PreserveTimestamp( preserveFirst )  (preserveFirst)
#define ResetTimestampIndex()                                           /* Nothing  */
#define GetTimestampSemaphoreLostCount()    (0)
#endif /* )) */
