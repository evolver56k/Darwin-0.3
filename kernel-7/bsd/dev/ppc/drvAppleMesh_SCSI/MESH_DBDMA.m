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
     * Copyright 1993-1995 by Apple Computer, Inc., all rights reserved.
     * Copyright 1997-1998 Apple Computer Inc. All Rights Reserved.
     * @author    Martin Minow  mailto:minow@apple.com
     * @revision  1997.02.13    Initial conversion from Copland sources.
     *
     * Set tabs every 4 characters.
     *
     * Edit History
     * 1997.02.25   MM      Initial conversion from Copland sources.
     */

#import <sys/systm.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/align.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/scsiTypes.h>
#import <driverkit/debugging.h>
#import <driverkit/IODirectDevice.h>
#import <driverkit/IOMemoryDescriptor.h>
#import <driverkit/IOSimpleMemoryDescriptor.h>
#import <driverkit/IOSCSIController.h>
#import <driverkit/IOPower.h>
#import <driverkit/return.h>
#import <bsd/dev/scsireg.h>
#import <mach/kern_return.h>
#import <mach/mach_interface.h>
#import <mach/message.h>
#import <machkit/NXLock.h>
#import <machdep/ppc/proc_reg.h>
#import <machdep/ppc/powermac.h>
#import <machdep/ppc/interrupts.h>
#import <machdep/ppc/dbdma.h>
#import <kernserv/prototypes.h>
#import <objc/objc.h>

    extern void             flush_cache_v( vm_offset_t pa, unsigned length );       /* Should be available from kernel headers! */
//  extern void             invalidate_cache_v( vm_offset_t pa, unsigned length );
    extern void             kprintf( const char *, ... );
    extern kern_return_t    msg_send_from_kernel( msg_header_t*, int, int );


#import "MESH_DBDMA.h"

#undef  ASSERT
#define ASSERT(x)

#if CustomMiniMon
    extern globals      g;  /**** Use custom MiniMon's globals   ****/
    extern UInt32       gMESH_DBDMA, gMESH_DBDMA_Phys;
#else
    globals             g;      /**** Instantiate the globals ****/
#endif /* CustomMiniMon */



    /* Channel Program. Note that this script must match the offsets    */
    /* specified in AppleMeshDefinitions.h. This script is copied into  */
    /* the channel command area (with appropriate entries byte-swapped  */
    /* so it ends up with the correct endian-ness).                     */
    /* Lines beginning with "slash, star, star, slash" are modified     */
    /* by the driver before it starts the Channel Program.              */

const DBDMADescriptor   gDescriptorList[] =
{
            /* 0x00 kcclProblem - Branch here for anomalies */

    {   MESH_REG( kMeshInterruptMask, kMeshIntrMask )   },  // Enable MESH interrupt
    {   STOP( kcclStageCCLx )                           },  // anomaly

            /* 0x20 through 0x60 - Data for information phases: */

    {   RESERVE                                         },  // kcclCMDOdata - CDB ( 6,10,12,16 bytes)
    {   RESERVE                                         },  // kcclMSGOdata - MSGO data (last byte @3F)
    {   RESERVE                                         },  // kcclMSGIdata - MSGI data & STATUS
    {   RESERVE                                         },  // kcclSenseCDB - CDB for (auto) Sense
    {   RESERVE                                         },  // kcclBatchSize, kcclStageLabel

            /* 0x70 - kcclSense - AutoSense input:  */

    {   MESH_REG( kMeshTransferCount1,  0x00 )          },  // set MESH xfer count to 255
    {   MESH_REG( kMeshTransferCount0,  kMaxAutosenseByteCount & 0xFF )},
    {   MESH_REG( kMeshSequence, kMeshDataInCmd | kMeshSeqDMA )},// Data-In to Seq register
    {   SENSE( kMaxAutosenseByteCount )                 },  // Sense INPUT
    {   BRANCH( kcclGetStatus )                         },  // do finish sequence

            /* 0xC0 - kcclPrototype - Prototype MESH 4-command Transfer sequence:   */

    {   MOVE_4( kcclBatchSize, 0, kRelAddressCP )       },  // MESH batch size
    {   MESH_REG( kMeshTransferCount1, 0 )              },  // Set high order Transfer Count
    {   MESH_REG( kMeshTransferCount0, 0 )              },  // Set low order Transfer Count
    {   MESH_REG( kMeshSequence, kMeshDataInCmd | kMeshSeqDMA )},  // Assume Data-In

    {   RESERVE                                         },  // spare
    {   RESERVE                                         },  // spare

            /* 0x120 kcclStart - Arbitrate (START CHANNEL PROGRAM HERE):    */
            /* 0x140 kcclBrProblem                                          */

    {   STAGE( kcclStageArb )                           },
    {   MESH_REG( kMeshSequence, kMeshArbitrateCmd )    },  // issue Arbitrate
    {   BR_IF_PROBLEM                                   },  // branch if exception or error

            /* 0x150 - Select with Attention:  */

    {   STAGE( kcclStageSelA )                          },
    {   CLEAR_CMD_DONE                                  },
    {   MESH_REG( kMeshSequence, kMeshSelectCmd | kMeshSeqAtn ) },  // select with attention
    {   BR_IF_PROBLEM                                   },  // branch if failed

            /* 0x190 kcclMsgoStage- Message-Out:    */

    {   STAGE( kcclStageMsgO )                          },
    {   CLEAR_CMD_DONE                                  },

            /* 0x1B0 kcclMsgoBranch - modify this BRANCH to fall through for multibyte messages:    */

/**/{   BRANCH( kcclLastMsgo )                          },  // kcclMsgoBranch - go do only byte of Msg

            /* 0x1C0 - do all but last byte of multibyte message:  */

    {   MESH_REG( kMeshTransferCount1,  0x00 )          },  // count does include last byte
/**/{   MESH_REG( kMeshTransferCount0,  0xFF )          },  // kcclMsgoMTC - modify MESH xfer count here
    {   MESH_REG( kMeshSequence, kMeshMessageOutCmd | kMeshSeqAtn | kMeshSeqDMA )   },  // DMA MsgO with ATN
/**/{   MSGO( kcclMSGOdata, 255 )                       },  // kcclMsgoDTC - output all but last byte
    {   CLEAR_CMD_DONE                                  },

            /* 0x210 kcclLastMsgo - wait for REQ signal before dropping ATN:    */

    {   MESH_REG( kMeshInterruptMask, 0 )               },  // inhibit MESH interrupt
    {   MESH_REG_WAIT( kMeshSequence, kMeshStatusCmd | kMeshSeqAtn ) }, // gen PhaseMM
    {   CLEAR_INT_REG                                   },  // clear PhaseMM & CmdDone
    {   MESH_REG( kMeshInterruptMask, kMeshIntrException | kMeshIntrError ) },  // re-enable ERR/EXC Ints

            /* 0x250 - put out the last or only byte of Message-Out phase:  */

    {   MESH_REG( kMeshTransferCount1,  0x00 )          },
    {   MESH_REG( kMeshTransferCount0,  0x01 )          },
    {   MESH_REG( kMeshSequence, kMeshMessageOutCmd | kMeshSeqDMA ) },// no more ATN
    {   MSGO( kcclMSGOLast, 1 )                         },

            /* 0x290 kcclCmdoStage - Command Out:  */

    {   STAGE( kcclStageCmdO )                          },
    {   CLEAR_CMD_DONE                                  },
    {   MESH_REG( kMeshTransferCount1,  0x00 )          },
/**/{   MESH_REG( kMeshTransferCount0,  0x06 )          },  // kcclCmdoMTC - Set MESH xfer count to 6
    {   MESH_REG( kMeshSequence, kMeshCommandCmd | kMeshSeqDMA )},  // Command phase with DMA on
/**/{   CMDO( 6 )                                       },  // kcclCmdoDTC - output the CDB

            /* 0x2F0 - DATA XFER - branch to the built CCL @ 0x05D0:    */
            /* also, kcclReselect - reselect code enters here:          */

    {   CLEAR_CMD_DONE                                  },
    {   STAGE( kcclStageXfer )                          },
    {   BRANCH( kcclDataXfer )                          },  // go do Xfer CCL

            /* 0x320 kcclOverrun - dump excess data in the bit bucket:  */
            /* Exc and Err are still disabled.                          */

    {   STAGE( kcclStageBucket )                        },
    {   MESH_REG( kMeshTransferCount1,  0x00 )          },  // set MESH Transfer Count to max
    {   MESH_REG( kMeshTransferCount0,  0x00 )          },
    {   CLR_PHASEMM                                     },
    {   MESH_REG( kMeshInterruptMask, kMeshIntrException | kMeshIntrError ) },  // re-enable ERR/EXC Ints
/**/{   MESH_REG( kMeshSequence, kMeshDataInCmd | kMeshSeqDMA ) }, // set Seq Reg
/**/{   BUCKET                                          },  // OUT/INPUT_LAST the bits
    {   BR_NO_PROBLEM( kcclOverrunDBDMA )               },  // loop til PhaseMismatch
    {   BR_IF_PROBLEM                                   },  // take the interrupt now

            /* 0x3B0 kcclSyncCleanUp - clean up after Sync xfer:  */
    {   CLEAR_INT_REG                                   },  // clear PhaseMM & CmdDone (& Err?)
    {   MESH_REG( kMeshInterruptMask, kMeshIntrException | kMeshIntrError ) },  // re-enable ERR/EXC Ints

            /* 0x3D0 kcclGetStatus - setup CCL for status, command complete and bus free:   */

    {   STAGE( kcclStageStat )                          },
    {   MESH_REG( kMeshTransferCount1,  0x00 )          },
    {   MESH_REG( kMeshTransferCount0,  0x01 )          },  // set MESH xfer count to 1
    {   MESH_REG( kMeshSequence, kMeshStatusCmd | kMeshSeqDMA )},// Status-in phase with DMA on
    {   STATUS_IN                                       },  // input the status byte

            /* 0x420 - Message In:  */

    {   STAGE( kcclStageMsgI )                          },
    {   CLEAR_CMD_DONE                                  },
    {   MESH_REG( kMeshTransferCount1, 0x00 )           },
    {   MESH_REG( kMeshTransferCount0, 0x01 )           },  // set MESH xfer count to 1
    {   MESH_REG( kMeshSequence, kMeshMessageInCmd | kMeshSeqDMA )},   // Status-in phase with DMA on
    {   MSGI( 1 )                                       },  // get the Message-In byte

            /* 0x480 - Bus Free:   */

    {   STAGE( kcclStageFree )                          },
    {   CLEAR_CMD_DONE                                  },
    {   MESH_REG( kMeshSequence, kMeshEnableReselect )  },  // Enable Reselect
    {   MESH_REG( kMeshSequence, kMeshBusFreeCmd )      },  // Bus Free phase
    {   BR_IF_PROBLEM                                   },  // branch if failed

            /* 0x4D0 kcclMESHintr - Good completion:    */

    {   STAGE( kcclStageGood )                          },
    {   MESH_REG( kMeshInterruptMask, kMeshIntrMask )   },  // latch MESH interrupt
    {   STOP( kcclStageStop )                           },  // Stop

        /* The rest of the Channel Program area is used for autosense   */
        /* and data transfer channel commands:                          */
        /*  kcclSenseBuffer Autosense area                              */
        /*  kcclDataXfer    Start of data transfer channel commands     */
        /*  kcclSenseResult Autosense result stored here                */

}; /* end gDescriptorList structure */

    const UInt32 gDescriptorListSize = sizeof( gDescriptorList );

    enum                       /* values for g.intLevel:                */
    {
        kLevelISR       = 0x80, /* In Interrupt Service Routine         */
        kLevelLocked    = 0x40, /* MESH interrupts locked out           */
        kLevelSIH       = 0x20, /* In Secondary Interrupt Handler       */
        kLevelLatched   = 0x10  /* Interrupt latched                    */
    };


//  IONamedValue    scsiChipRegisterStrings[] = { { 0, NULL, } };

    static int          getConfigParam( id configTable, const char *paramName );
    static unsigned int GetSCSICommandLength( const cdb_t *cdbPtr, unsigned int defaultLength );


        /* MAX_DMA_XFER is set so that we don't have to worry about the     */
        /* ambiguous "zero" value in the MESH and DBDMA transfer registers  */
        /* that can mean either 65536 bytes or zero bytes.                  */

#define MAX_DMA_XFER    0x0000F000

#define ONE_SECOND      1    /* for IOScheduleFunc and serviceTimeoutInterrupt */

        /*  Template for command message sent to the IO thread: */

    static const msg_header_t     cmdMessageTemplate =
    {
        0,                          /* msg_unused                   */
        1,                          /* msg_simple                   */
        sizeof( msg_header_t ),     /* msg_size                     */
        MSG_TYPE_NORMAL,            /* msg_type                     */
        PORT_NULL,                  /* msg_local_port               */
        PORT_NULL,                  /* msg_remote_port - filled in  */
        IO_COMMAND_MSG              /* msg_id                       */
    };
    
        /* Template for timeout message.    */
    
    static const msg_header_t     gTimeoutMsgTemplate =
    {
        0,                          /* msg_unused                   */
        1,                          /* msg_simple                   */
        sizeof( msg_header_t ),     /* msg_size                     */
        MSG_TYPE_NORMAL,            /* msg_type                     */
        PORT_NULL,                  /* msg_local_port               */
        PORT_NULL,                  /* msg_remote_port - filled in  */
        IO_TIMEOUT_MSG              /* msg_id                       */
    };

    static port_t           gKernelInterruptPort;   /* for int/timeout msgs */

    static void             serviceTimeoutInterrupt( void *arg );
    static AppleMesh_SCSI   *gInstance;


#if USE_ELG && !CustomMiniMon
void AllocateEventLog( UInt32 size )
{
    if ( !g.evLogBuf )  g.evLogBuf = (UInt8*)kalloc( size );
    if ( !g.evLogBuf )
        kprintf( "probe - MESH evLog allocation failed " );

    g.evLogFlag = 0;            /* assume insufficient memory   */
    g.evLogBufp = g.evLogBuf;

    if ( g.evLogBuf )
    {
        g.evLogBufe  = g.evLogBufp + kEvLogSize - 0x20; // ??? overran buffer?
        g.evLogFlag  = 0xFEEDBEEF;
    //  g.evLogFlag  = 0x0333;
    }
    return;
}/* end AllocateEventLog */


void EvLog( UInt32 a, UInt32 b, UInt32 ascii, char* str )
{
    register UInt32     *lp;           /* Long pointer      */
    ns_time_t           time;

    if ( g.evLogFlag == 0 )
        return;

    IOGetTimestamp( &time );

    lp = (UInt32*)g.evLogBufp;
    g.evLogBufp += 0x10;

    if ( g.evLogBufp >= g.evLogBufe )       /* handle buffer wrap around if any */
    {    g.evLogBufp  = g.evLogBuf;
        if ( g.evLogFlag != 0xFEEDBEEF )    // make 0xFEEDBEEF a symbolic ???
            g.evLogFlag = 0;                /* stop tracing if wrap undesired   */
    }

        /* compose interrupt level with 3 byte time stamp:  */

    *lp++ = (g.intLevel << 24) | ((time >> 10) & 0x003FFFFF);   // ~ 1 microsec resolution
    *lp++ = a;
    *lp++ = b;
    *lp   = ascii;

    if( g.evLogFlag == 'step' )
        kprintf( str );

    return;
}/* end EvLog */


void Pause( UInt32 a, UInt32 b, UInt32 ascii, char* str )
{
    char        work [256 ];
    char        name[] = "AppleMeshSCSI:";
    char        *bp = work;
    UInt8       x;
    int         i;


    EvLog( a, b, ascii, str );
    EvLog( '****', '** P', 'ause', "*** Pause" );

    bcopy( name, bp, sizeof( name ) );
    bp += sizeof( name ) - 1;

    *bp++ = '{';                               // prepend p1 in hex:
    for ( i = 7; i >= 0; --i )
    {
        x = a & 0x0F;
        if ( x < 10 )
             x += '0';
        else x += 'A' - 10;
        bp[ i ] = x;
        a >>= 4;
    }
    bp += 8;

    *bp++ = ' ';                               // prepend p2 in hex:

    for ( i = 7; i >= 0; --i )
    {
        x = b & 0x0F;
        if ( x < 10 )
             x += '0';
        else x += 'A' - 10;
        bp[ i ] = x;
        b >>= 4;
    }
    bp += 8;
    *bp++ = '}';

    *bp++ = ' ';

    for ( i = sizeof( work ) - (int)(bp - work); i && (*bp++ = *str++); --i )   ;

    kprintf( work );
//  call_kdp();         // ??? use kdp=3 in boot parameters
    return;
}/* end Pause */
#endif /* not CustomMiniMon */


    /* serviceTimeoutInterrupt - Handle timeouts.                         */
    /* This  function is invoked in kernel context on a DriverKit thread. */
    /* Just send a timeout message to the IO thread to wake it up.        */

static void serviceTimeoutInterrupt( void *arg )
{
    msg_header_t    msg = gTimeoutMsgTemplate;


    ELG( 0, 0, 'Tick', "serviceTimeoutInterrupt\n" );

         /* roll me another one: */
    IOScheduleFunc( serviceTimeoutInterrupt, (void*)0x333, ONE_SECOND );

        /* Tell the IO thread: */
    msg.msg_remote_port = gKernelInterruptPort;
    msg_send_from_kernel( &msg, MSG_OPTION_NONE, 0 );
    return;
}/* end serviceTimeoutInterrupt */


    /* Used in timeoutOccurred to determine if specified cmdBuf has timed out.  */
    /* Returns YES if timeout, else NO.                                         */

static Boolean isCmdTimedOut( CommandBuffer *cmdBuf )
{
    IOSCSIRequest   *scsiReq = cmdBuf->scsiReq;
    ns_time_t       now, expire;
    Boolean         result;


    IOGetTimestamp( &now );
    expire  = cmdBuf->startTime +
                (1000000000ULL * (unsigned long long)scsiReq->timeoutLength);
    result = (now > expire);
    if ( result ) ELG( cmdBuf, cmdBuf->scsiReq->timeoutLength, 'Tim-', "isCmdTimedOut" );
    return  result;
}/* end isCmdTimedOut */


    /* Implement the public methods for the MESH controller. */

@implementation AppleMesh_SCSI

    /* Create and initialize one instance of AppleMesh_SCSI.        */
    /* The work is done by architecture- and chip-specific modules. */

+ (Boolean) probe : deviceDescription
{
    Boolean     result;


    gInstance   = [ self alloc ];   /* Instantiate yourself   */
    g.intLevel  = 0;

    MakeTimestampRecord( 512 );     /* conditionally compiled */

#if USE_ELG
    AllocateEventLog( kEvLogSize );
    ELG( g.evLogBufp, &g.evLogFlag, 'Prob', "probe - event logging set up.\n" );
#endif /* USE_ELG */

        /* Perform device-specific initialization. */
        /* Free the instance on failure.           */

    if ( [ gInstance InitializeHardware : deviceDescription ] == nil )
         result = NO;
    else result = YES;

    return result;
}/* end probe */


    /* The driver is shutting down. Kill everything worth killing.  */

- free
{
    CommandBuffer  cmdBuf;


        /* First kill the IO thread if running. */

    if ( gFlagIOThreadRunning )
    {
        cmdBuf.op       = kCommandAbortRequest;
        cmdBuf.scsiReq  = NULL;
        [ self executeCmdBuf : &cmdBuf ];
    }

    if (  incomingCmdLock )
        [ incomingCmdLock free ];

    dbdma_stop( DBDMA_MESH_SCSI );

    if ( cclLogAddr )
    {
        IOFree( cclLogAddr, cclLogAddrSize );
        cclLogAddr = NULL;
    }
        /* ??? Unmap physical address mapping to registers. */

    return [ super free ];
}/* end free */


    /* Return required DMA alignment for current architecture.              */
    /* We specify 8-byte alignment to avoid a bug in the Grand Central chip:*/
    /*  if (Reading                                                         */
    /*   && (kdbdmaSetFlush || kdbdmaClrRun)                                */
    /*   && no bytes transferred yet                                        */
    /*   && buffer not 8-byte aligned)                                      */
    /*  {                                                                   */
    /*      THEN memory in front of buffer will be trashed.                 */
    /*  }                                                                   */

- (void) getDMAAlignment : (IODMAAlignment*)alignment
{
    alignment->readStart   = DBDMA_ReadStartAlignment;
    alignment->writeStart  = DBDMA_WriteStartAlignment;
    alignment->readLength  = 0;
    alignment->writeLength = 0;
    return;
}/* end getDMAAlignment */


    /* Statistics support.  */

- (unsigned int) numQueueSamples
{
    return gTotalCommands;
}/* end numQueueSamples */


- (unsigned int) sumQueueLengths
{
    return gQueueLenTotal;
}/* end sumQueueLengths */


- (unsigned int) maxQueueLength
{
    return gMaxQueueLen;
}/* end maxQueueLength */


- (void) resetStatistics
{
    gMaxQueueLen    = 0;
    gQueueLenTotal  = 0;
    gMaxQueueLen    = 0;
    return;
}/* resetStatistics */


    /* Do a SCSI command, as specified by an IOSCSIRequest.                 */
    /* All the work is done by the IO thread.                               */
    /* @param   scsiReq     The request to execute                          */
    /* @param   buffer      The data buffer to transfer to/from, if any     */
    /* @param   client      The data buffer owner task (for VM munging)     */
    /*                                                                      */
    /* This method is called from IOSCSIDevice                              */
- (sc_status_t) executeRequest  : (IOSCSIRequest*)scsiReq
                        buffer  : (void*)buffer 
                        client  : (vm_task_t)client
{
    IOMemoryDescriptor  *mem        = NULL;
    sc_status_t         scsiStatus  = SR_IOST_GOOD;     /* Fool compiler */


    ELG( scsiReq->lun<<16 | scsiReq->target, scsiReq, 'sReq', "executeRequest (buffer)" );
    ELG( buffer, scsiReq->maxTransfer, 'Buff', "executeRequest" );

        /* Create a simple IO memory descriptor for this client,    */
        /* then toss it to the common method.                       */

    if ( buffer )
    {
        if ( scsiReq->read && ((UInt32)buffer & (DBDMA_ReadStartAlignment - 1)) )
        {
            ELG( scsiReq->maxTransfer, buffer, 'Aln-', "executeRequest/simple buffer - unaligned read buffer." );
            return SR_IOST_ALIGN;
        }
        mem = [ [ IOSimpleMemoryDescriptor alloc ]
                                    initWithAddress : (void*)buffer
                                    length          : scsiReq->maxTransfer ];
        [ mem setClient : client ];
    }

    scsiStatus = [ self executeRequest : scsiReq    ioMemoryDescriptor : mem ];

    if (  mem )
        [ mem release ];

    return scsiStatus;
}/* end executeRequest buffer */


    /* Execute a SCSI request using an IOMemoryDescriptor.      */
    /* This allows callers to provide (kernel-resident) logical */
    /* scatter-gather lists. For compatibility with existing    */
    /* implementations, the low-level SCSI device driver must   */
    /* first ensure that executeRequestWithIOMemoryDescriptor   */
    /* is supported by executing:                               */
    /*  [ controller respondsToSelector : executeRequestWithIOMemoryDescriptor ]    */

- (sc_status_t) executeRequest  : (IOSCSIRequest*)scsiReq
            ioMemoryDescriptor  : (IOMemoryDescriptor*)ioMemoryDescriptor
{
    CommandBuffer    commandBuffer;


    ELG( scsiReq->lun<<16 | scsiReq->target, scsiReq, 'dReq', "executeRequest (IOMemoryDescriptor)" );
    ELG( 0, ioMemoryDescriptor, 'iomd', "executeRequest" );

    scsiReq->driverStatus = SR_IOST_INVALID;   /* "In progress" */
    if ( ioMemoryDescriptor )
    {
        [ ioMemoryDescriptor setMaxSegmentCount : MAX_DMA_XFER ];
        [ ioMemoryDescriptor state : &commandBuffer.savedDataState ];
    }
    bzero( &commandBuffer, sizeof( CommandBuffer ) );
    commandBuffer.op            = kCommandExecute;
    commandBuffer.scsiReq       = scsiReq;
    commandBuffer.mem           = ioMemoryDescriptor;

    [ self executeCmdBuf : &commandBuffer ];

#if TIMESTAMP_AT_IOCOMPLETE
    [ self logTimestamp : "IO complete" ];      /* After RESULT macro   */
#endif

    return commandBuffer.scsiReq->driverStatus;
}/* end executeRequest IOMemoryDescriptor */


    /* Reset the SCSI bus. All the work is done by the IO thread.   */

- (sc_status_t) resetSCSIBus
{
    CommandBuffer   commandBuffer;


    commandBuffer.op        = kCommandResetBus;
    commandBuffer.scsiReq   = NULL;

    [ self executeCmdBuf : &commandBuffer ];
    return  SR_IOST_GOOD;           /* can not fail */
}/* end resetSCSIBus */


    /* The following 6 methods,                                             */
    /*     interruptOccurred, interruptOccurredAt, otherOccurred,           */
    /*     receiveMsg, timeoutOccurred, commandRequestOccurred,             */
    /* are all called from the IO thread in IODirectDevice.                 */

    /* Called from the IO thread when it receives an interrupt message.     */
    /* Currently all work is done by chip-specific module; maybe we should  */
    /* put this method there....                                            */

- (void) interruptOccurred
{
    g.intLevel |=  kLevelISR;                               /* set ISR flag     */
    g.intLevel &= ~kLevelLatched;                           /* clear latched    */
    ELG( dbdmaAddr->d_status, dbdmaAddr->d_cmdptrlo, 'Int+', "interruptOccurred." );
//  ELG( *(UInt32*)0xF3000024, *(UInt32*)0xF300002C, 'Int ', "interruptOccurred." );

    [ self DoHardwareInterrupt ];      /**** HANDLE THE INTERRUPT  ****/

//  ELG( gActiveCommand, *(UInt32*)0xF300002C, 'Intx', "interruptOccurred." );

    g.intLevel &= ~kLevelISR;                  /* clear ISR flag    */
    return;
}/* end interruptOccurred */


    /* These three should not occur; they are here as error traps.  */
    /* All three are called out from the IO thread upon receipt of  */
    /* messages which it should not be seeing.                      */

- (void) interruptOccurredAt : (int)localNum
{
    PAUSE( 0, localNum, 'int@', "interruptOccurredAt.\n" );
    return;
}/* end interruptOccurredAt */


- (void) otherOccurred : (int)id
{
    PAUSE( 0, id, 'Othr', "otherOccurred.\n" );
    return;
}/* end otherOccurred */


- (void) receiveMsg
{
    PAUSE( 0, 0, 'RcvM', "receiveMsg.\n" );

        /* We have to let IODirectDevice take care of this (i.e.,   */
        /* dequeue the bogus message).                              */

    [ super receiveMsg ];
    return;
}/* end receiveMsg */


    /* This method is invoked by DriverKit when it receives a message       */
    /* generated by the function serviceTimeoutInterrupt() which was called */
    /* by the kernel on some DriverKit thread.                              */

- (void) timeoutOccurred
{
    CommandBuffer       *cmdBuf = gActiveCommand;
    CommandBuffer       *nextCmdBuf;


    if ( g.intLevel & kLevelLatched )
    {
        ELG( cmdBuf, 0, 'TocL', "timeoutOccurred - interrupt already latched; do nothing" );
        return;
    }

    g.intLevel |= kLevelISR;                /* set IOthread-running-flag */
    ELG( CCLWord( kcclStageLabel ), dbdmaAddr->d_cmdptrlo, 'Tock', "timeoutOccurred - tick.\n" );
    [ self GetHBARegsAndClear : FALSE ];    /* get the MESH registers    */

        /* If gActiveCommand timed out: */

    if ( cmdBuf )
    {   if ( isCmdTimedOut( cmdBuf )
          && ((CCLWord( kcclStageLabel ) != kcclStageFree)
            ||  (CCLWord( kcclStageLabel ) != kcclStageGood) ) )
        {
            dbdma_flush( DBDMA_MESH_SCSI );         /* DBDMA may be hung in */
            dbdma_stop(  DBDMA_MESH_SCSI );         /* middle of transfer.  */
        //  invalidate_cache_v( (vm_offset_t)cclLogAddr, cclLogAddrSize );

            cmdBuf->scsiReq->driverStatus = SR_IOST_IOTO;
            [ self ioComplete : cmdBuf ];

            [ self AbortActiveCommand ];

            g.intLevel &= ~kLevelISR;       /* clear IOthread-running-flag */
            return;
        }
    }
    else    /* Move any/all timed-out disconnected commands to abortCmdQ: */
    {
        cmdBuf = (CommandBuffer*)queue_first( &disconnectedCmdQ );
        while ( !queue_end( &disconnectedCmdQ, (queue_entry_t)cmdBuf ) )
        {
            nextCmdBuf = (CommandBuffer*)queue_next( &cmdBuf->link );
            if ( isCmdTimedOut( cmdBuf ) )
            {       /* Move cmdBuf from disconnectQ to abortQ:  */
                queue_remove( &disconnectedCmdQ, cmdBuf, CommandBuffer*, link );
                queue_enter( &abortCmdQ, cmdBuf, CommandBuffer*, link );
                cmdBuf->scsiReq->driverStatus = SR_IOST_IOTO;
            }
            cmdBuf = nextCmdBuf;
        }/* end WHILE scanning commands in the disconnected queue */
        [ self AbortDisconnectedCommand ];
    }
    g.intLevel &= ~kLevelISR;                /* clear IOthread-running-flag */
    return;
}/* end timeoutOccurred */


    /* Process all commands in incomingCmdQ. At most one of these          */
    /* will become gActiveCommand. The remainder of kCommandExecute commands*/
    /* go to pendingCmdQ. Other types of commands (such as bus reset)      */
    /* are executed immediately.                                            */
    /* This method is called from IODirectDevice.                           */
    /*                                                                      */
    /* Note that we don't have a concept of frozen queue.                   */

- (void) commandRequestOccurred
{
    CommandBuffer      *cmdBuf, *pendCmd;


    [ incomingCmdLock lock ];

    while ( !queue_empty( &incomingCmdQ ) )
    {
        cmdBuf = (CommandBuffer*)queue_first( &incomingCmdQ );
        queue_remove( &incomingCmdQ, cmdBuf, CommandBuffer*, link );
        [ incomingCmdLock unlock ];
        ELG( gActiveCommand, cmdBuf, 'CRO+', "commandRequestOccurred" );

        switch ( cmdBuf->op )
        {
        case kCommandResetBus:
                /* Note all active and disconnected commands will be terminated.*/
            [ self threadResetBus : "Reset Command Received" ];
            [ self ioComplete : cmdBuf ];
            break;

        case kCommandAbortRequest:
                /* 1. Abort all active, pending, and disconnected commands. */
                /* 2. Notify caller of completion.                          */
                /* 3. Self-terminate.                                       */

            [ self abortAllCommands : SR_IOST_INT ];
            pendCmd = (CommandBuffer*)queue_first( &pendingCmdQ );

            while ( !queue_end( &pendingCmdQ, (queue_entry_t)pendCmd ) )
            {
                pendCmd->scsiReq->driverStatus = SR_IOST_INT;
                [ self ioComplete : pendCmd ];
                pendCmd = (CommandBuffer*)queue_next( &pendCmd->link );
            }

            [ cmdBuf->cmdLock lock ];
            [ cmdBuf->cmdLock unlockWith : CMD_COMPLETE ];
            IOExitThread();
            /***** not reached *****/

        case kCommandExecute:
            [ self threadExecuteRequest : cmdBuf ];
            break;
        }/* end SWITCH */

        [ incomingCmdLock lock ];
    }/* end WHILE queue not empty */

    [ incomingCmdLock unlock ];
    return;
}/* end commandRequestOccurred */


    /* Power management methods. All we care about is power off, when   */
    /* we must reset the SCSI bus due to the Compaq BIOS's lack of a    */
    /* SCSI reset, which causes a hang if we have set up targets for    */
    /* sync data transfer mode.                                         */

- (IOReturn) getPowerState : (PMPowerState*)state_p
{
    return IO_R_UNSUPPORTED;
}/* end getPowerState */


- (IOReturn) setPowerState : (PMPowerState) state
{
    ELG( 0, state, 'sPwr', "setPowerState.\n" );

    if ( state == PM_OFF )
    {
    //  [ self scsiReset ];
            // ** ** ** TBS: [ self powerDown ];
        return IO_R_SUCCESS;
    }
    return IO_R_UNSUPPORTED;
}/* end setPowerState */


- (IOReturn) getPowerManagement : (PMPowerManagementState*)state_p
{
    return IO_R_UNSUPPORTED;
}/* end getPowerManagement */


- (IOReturn) setPowerManagement : (PMPowerManagementState)state
{
    return IO_R_UNSUPPORTED;
}/* end setPowerManagement */


#if APPLE_MESH_ENABLE_GET_SET

- (IOReturn) setIntValues       : (unsigned*)       parameterArray
                forParameter    : (IOParameterName) parameterName
                count           : (unsigned int)    count
{
    int             target;
    PerTargetData   *perTargetPtr;
    IOReturn        ioReturn = IO_R_INVALID_ARG;


    if ( strcmp( parameterName, APPLE_MESH_AUTOSENSE ) == 0 )
    {
        if ( count == 1 )
        {
            autoSenseEnable = parameterArray[0] ? 1 : 0;
            ELG( 0, autoSenseEnable, 'sVas', "setIntValues - autoSense\n" );
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_CMD_QUEUE ) == 0 )
    {
        if ( count == 1 )
        {
            cmdQueueEnable = parameterArray[0] ? 1 : 0;
            ELG( 0, cmdQueueEnable, 'sVqe', "setIntValues - cmdQueueEnable\n" );
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_SYNC ) == 0 )
    {
        if ( count == 1 )
        {
            syncModeEnable = parameterArray[0] ? 1 : 0;
            ELG( 0, syncModeEnable, 'sVse', "setIntValues - syncModeEnable\n" );
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_FAST_SCSI ) == 0 )
    {
        if ( count == 1 )
        {
            fastModeEnable = parameterArray[0] ? 1 : 0;
            ELG( 0, fastModeEnable, 'sVfe', "setIntValues - fastModeEnable\n" );
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_RESET_TARGETS ) == 0 )
    {
        if ( count == 0 )
        {
                /* Re-enable sync and command queuing.      */
                /* The disable bits persist after a reset.  */
            for ( target = 0; target < SCSI_NTARGETS; target++ )
            {
                perTargetPtr = &gPerTargetData[ target ];
                perTargetPtr->syncDisable       = FALSE;
                perTargetPtr->maxQueue          = 0;
                perTargetPtr->inquiry_7         = 0;
            }
            ELG( 0, 0, 'sVrt', "setIntValues - reset targets\n" );
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_RESET_TIMESTAMP ) == 0 )
    {
        ResetTimestampIndex();
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_ENABLE_TIMESTAMP ) == 0 )
    {
        EnableTimestamp( TRUE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_DISABLE_TIMESTAMP ) == 0 )
    {
        EnableTimestamp( FALSE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_PRESERVE_FIRST_TIMESTAMP ) == 0 )
    {
        PreserveTimestamp( TRUE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_PRESERVE_LAST_TIMESTAMP ) == 0 )
    {
        PreserveTimestamp( FALSE );
        ioReturn = IO_R_SUCCESS;
    }
    else
    {
        ioReturn [ super setIntValues   : parameterArray
                        forParameter    : parameterName
                        count           : count ];
    }
    return ioReturn;
}/* end setIntValues */


- (IOReturn) getIntValues   : (unsigned*)       parameterArray
        forParameter        : (IOParameterName) parameterName
        count               : (unsigned*)       count      /* in/out  */
{
    IOReturn        ioReturn = IO_R_INVALID_ARG;


    if ( strcmp( parameterName, APPLE_MESH_AUTOSENSE) == 0 )
    {
        if ( *count == 1 )
        {
            parameterArray[0] = autoSenseEnable;
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_CMD_QUEUE ) == 0 )
    {
        if ( *count == 1 )
        {
            parameterArray[0] = cmdQueueEnable;
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_SYNC ) == 0 )
    {
        if ( *count == 1 )
        {
            parameterArray[0] = syncModeEnable;
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_FAST_SCSI ) == 0 )
    {
        if ( *count == 1 )
        {
            parameterArray[0] = fastModeEnable;
            ioReturn = IO_R_SUCCESS;
        }
    }
    else if ( strcmp( parameterName, APPLE_MESH_RESET_TIMESTAMP ) == 0 )
    {
        ResetTimestampIndex();
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_ENABLE_TIMESTAMP ) == 0 )
    {
        EnableTimestamp( TRUE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_DISABLE_TIMESTAMP ) == 0 )
    {
        EnableTimestamp( FALSE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_PRESERVE_FIRST_TIMESTAMP ) == 0 )
    {
        PreserveTimestamp( TRUE );
        ioReturn = IO_R_SUCCESS;
    }
    else if ( strcmp( parameterName, APPLE_MESH_PRESERVE_LAST_TIMESTAMP ) == 0 )
    {
        PreserveTimestamp( FALSE );
        ioReturn = IO_R_SUCCESS;
    }
    else
    {
        ioReturn = [ super  getIntValues    : parameterArray
                            forParameter    : parameterName
                            count           : count ];
    }
    return  ioReturn;
}/* end getIntValues */

#endif APPLE_MESH_ENABLE_GET_SET

@end /* AppleMesh_SCSI */



@implementation AppleMesh_SCSI( Hardware )

    /* Perform MESH-specific initialization.                      */
    /* Fetch the device's bus address and interrupt port number.  */
    /* Also, allocate one page of memory for the channel program. */

- InitializeHardware : deviceDescription
{
    IOReturn        ioReturn    = IO_R_SUCCESS;
    id              result      = self;
    kern_return_t   kernelReturn;
    UInt8           target, lun;
    id              configTable;
    const char      *configValue;
    UInt8           deviceNumber;
    UInt8           functionNumber;
    UInt8           busNumber;


    configTable = [ deviceDescription configTable ];
    ASSERT( configTable );
    configValue = [ configTable valueForStringKey: "Bus Type" ];

    if ( configValue == NULL || strcmp( configValue, "PPC" ) )
    {
        PAUSE( 0, 'init', 'Hdw-', "InitializeHardware - bus type NG.\n" );
        ioReturn = IO_R_NO_DEVICE;
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
#if 0 // ** ** ** Need correct definition ** ** **
        ioReturn = [ deviceDescription getPCIDevice
                                    : &deviceNumber
                        function    : &functionNumber
                        bus         : &busNumber ];
#else
        deviceNumber    = 0;
        functionNumber  = 0;
        busNumber       = 0;
        kernelReturn    = 0;
#endif
        if ( ioReturn != IO_R_SUCCESS )
            PAUSE( 0, ioReturn, 'iHd-', "InitializeHardware - Can't get PCI device information.\n" );
    }

    if ( configValue )
    {
        [ configTable freeString : configValue ];
        configValue = NULL;
    }

    if ( ioReturn == IO_R_SUCCESS )
        ioReturn = [ self AllocHdwAndChanMem : deviceDescription ];

    if ( ioReturn == IO_R_SUCCESS )
    {
        for ( target = 0; target < SCSI_NTARGETS; target++ )
        {
            gPerTargetData[ target ].syncParms      = kSyncParmsAsync;
            gPerTargetData[ target ].negotiateSDTR  = kSyncParmsFast;   // negotiate Fast
            gPerTargetData[ target ].inquiry_7      = 0;
        }

            /* All of the addresses are established.           */
            /* Check that the hardware is present and working. */
        ioReturn = [ self DoHBASelfTest ];
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
            /* Tell the superclass to initialize our IO thread.        */
            /* After this, we should be able to execute SCSI requests. */

        if ( [ super initFromDeviceDescription : deviceDescription ] == NULL )
        {
            PAUSE( 0, 0, 'i h-', "InitializeHardware - Host Adaptor was not initialized. Fatal.\n" );
            ioReturn = IO_R_NO_DEVICE;
        }
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
        gFlagIOThreadRunning = 1;

            /* Initialize local variables. Note that activeArray and    */
            /* perTarget arrays are zeroed by objc runtime.             */

        queue_init( &disconnectedCmdQ );
        queue_init( &incomingCmdQ );
        queue_init( &pendingCmdQ );
        queue_init( &abortCmdQ );
        incomingCmdLock = [ [ NXLock alloc ] init ];
        gActiveCommand = NULL;
        [ self resetStatistics ];
        gNextQueueTag       = QUEUE_TAG_NONTAGGED + 1;
        gInitiatorID        = kInitiatorIDDefault;
        gInitiatorIDMask    = 1 << gInitiatorID;    /* BusID bitmask for selection. */
        gFlagReselecting    = FALSE;

            /* Reserve the initiator ID for all LUNs:   */

        for ( lun = 0; lun < SCSI_NLUNS; lun++ )
            [ self reserveTarget : gInitiatorID     lun : lun   forOwner : self ];

            /* Get tagged command queueing, sync mode,  */
            /* fast mode enables from configTable.      */

        gOptionCmdQueueEnable   = getConfigParam( configTable, CMD_QUEUE_ENABLE );
        gOptionSyncModeEnable   = getConfigParam( configTable, SYNC_ENABLE );
        gOptionFastModeEnable   = getConfigParam( configTable, FAST_ENABLE );
        gOptionExtendTiming     = getConfigParam( configTable, EXTENDED_TIMING );
        gOptionAutoSenseEnable  = AUTO_SENSE_ENABLE;       // from bringup.h

        gOptionCmdQueueEnable   = 1;    /* Temp for testing???  */

            /* Get internal version of interruptPort;           */
            /* set the port queue length to the maximum size.   */
            /* It is not clear if we want to do this.           */

        gKernelInterruptPort = IOConvertPort(   [ self interruptPort ],
                                                IO_KernelIOTask,
                                                IO_Kernel );
#if 0 /***** Need correct header file   *****/
        kernelReturn = port_set_backlog(    task_self(),
                                            [ self interruptPort ],
                                            PORT_BACKLOG_MAX );
        if ( kernelReturn != KERN_SUCCESS )
            PAUSE( 0, kernelReturn, 'i H-', "InitializeHardware - warning, port_set_backlog error.\n" );
#endif

            /* Initialize the chip and reset the bus:   */

        ioReturn = [ self ResetHardware : TRUE ];
        meshAddr->sourceID  = gInitiatorID; // mlj ??? fix this
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
            /* OK, we're ready to roll. */

        [ self enableInterrupt : 0 ];
        [ self registerDevice ];

        IOScheduleFunc( serviceTimeoutInterrupt, (void*)0x333, ONE_SECOND );
    }
    else
    {       /* Do we need to free the locks and similar?    */
        [ self free ];
        result = NULL;
    }

    return  result;
}/* end InitializeHardware */


    /* This includes a SCSI reset.                                  */
    /* Handling of ioComplete of active and disconnected commands   */
    /* must be done elsewhere. Returns IO_R_SUCCESS if successful.  */
    /* This is called from a Task thread. It will disable and       */
    /* re-enable interrupts. Reason is for error logging.           */

- (IOReturn) ResetHardware : (Boolean)resetSCSIBus
{
    ELG( 0, resetSCSIBus, 'RstH', "ResetHardware - Bus Reset.\n" );

    [ self abortAllCommands : SR_IOST_RESET ];
    [ self ResetMESH        : resetSCSIBus  ];

    return  IO_R_SUCCESS;
}/* end ResetHardware */


    /* Start a SCSI transaction for the specified command.                  */
    /* ActiveCmd must be NULL. A return of kHardwareStartRejected           */
    /* indicates that caller may try again with another command;            */
    /* kHardwareStartBusy indicates a condition other than                  */
    /* (activeCmd != NULL) which prevents the processing of the command.    */

- (HardwareStartResult) hardwareStart : (CommandBuffer*) cmdBuf
{
    IOSCSIRequest       *scsiReq;
    HardwareStartResult result          = kHardwareStartOK;
    cdb_t               *cdbp;
    Boolean             okToDisconnect  = TRUE;
    Boolean             okToQueue       = gOptionCmdQueueEnable;
    UInt8               msgByte;


    ASSERT( cmdBuf && cmdBuf->scsiReq );

    scsiReq         = cmdBuf->scsiReq;
    gCurrentTarget  = scsiReq->target;
    gCurrentLUN     = scsiReq->lun;
    cdbp            = &scsiReq->cdb;
    gMsgOutFlag     = 0;

    cmdBuf->cdbLength = GetSCSICommandLength( cdbp, scsiReq->cdbLength );
    if ( cmdBuf->cdbLength == 0 )
    {
            /* Failure: we can't determine the length of this command.  */

        scsiReq->driverStatus = SR_IOST_CMDREJ;
        [ self ioComplete : cmdBuf ];
        result = kHardwareStartRejected;
    }
    {   UInt8   *bp = (UInt8*)cdbp;
        ELG(    ( bp[0]<<24) | (bp[1]<<16) | (bp[2]<<8) | bp[3],
                ( bp[4]<<24) | (bp[5]<<16) | (bp[6]<<8) | bp[7],
                '=CDB', "hardwareStart - CDB" );
    }

    if ( result == kHardwareStartOK )
    {
            /* Peek at the control byte (the last byte in the command). */

        msgByte = ((UInt8*)cdbp)[ cmdBuf->cdbLength - 1 ];
        if ( (msgByte & CTRL_LINKFLAG) != CTRL_NOLINK )
        {
                /* Failure: we don't support linked commands.   */

            scsiReq->driverStatus = SR_IOST_CMDREJ;
            [ self ioComplete : cmdBuf ];
            result = kHardwareStartRejected;
        }
    }

    if ( result == kHardwareStartOK )
    {
            /* Autosense always renegotiates synchronous transfer mode. */
            /* This is necessary as the target might have been reset    */
            /* or hit with a power-cycle. Autosense is never issued     */
            /* with a queue tag.                                        */

        cmdBuf->queueTag = QUEUE_TAG_NONTAGGED;     /* No tag just yet  */
        if ( cmdBuf->flagIsAutosense )
        {
            okToDisconnect = FALSE;
            gPerTargetData[ gCurrentTarget ].negotiateSDTR  = gPerTargetData[ gCurrentTarget ].syncParms;
        }
        else
        {
                /* This is a real command. Setup the user data pointers */
                /* and counters and build a SCSI request CCL.           */
                /* First, peek at the command for some special cases.   */

            switch ( cdbp->cdb_opcode )
            {
            case kScsiCmdInquiry:

                    /* The first command SCSIDisk sends us is an Inquiry.   */
                    /* This never gets retried, so avoid a possible         */
                    /* reject of a command queue tag. Avoid this hack if    */
                    /* there are any other commands outstanding for this    */
                    /* Target/LUN.                                          */

                if ( gActiveArray[ scsiReq->target ][ scsiReq->lun ] == 0 )
                    scsiReq->cmdQueueDisable = TRUE;

                okToDisconnect   = FALSE;  /* no disconnect, no queuing */
                break;

            case kScsiCmdRequestSense:
                    /* Always force sync renegotiation on any Request Sense */
                    /* to catch independent target power cycles.            */
                    /* (Sync renegotiation needed should be set after all   */
                    /* target-detected errors -- fix needed in MessageIn).  */
                    /* Sense is always issued with disconnect disabled to   */
                    /* maintain T/L/Q nexus.                                */
                    /* Watch it: request sense from a client is incompatible*/
                    /* with tagged queuing.                                 */

                gPerTargetData[ gCurrentTarget ].negotiateSDTR
                                    = gPerTargetData[ gCurrentTarget ].syncParms;
                okToDisconnect  = FALSE;
                break;

            case kScsiCmdTestUnitReady:
            case kScsiCmdReadCapacity:
                okToDisconnect  = FALSE;
                break;
            }/* end SWITCH on opcode */
        }/* end ELSE not auto sense */
    }/* end IF kHardwareStartOK */

    okToDisconnect &= scsiReq->disconnect;

    okToQueue   &= okToDisconnect
                && (scsiReq->cmdQueueDisable == FALSE)
                && (gPerTargetData[ scsiReq->target ].inquiry_7 & 0x02);

    cmdBuf->flagActive = 0;     /* Initialize flags for this command.   */

        /* Make sure that the HBA is stable before we  */
        /* try to start a request.                     */

    if ( result == kHardwareStartOK )
    {
        if ( gActiveCommand )
        {
                /* This should never happen. It ensures that there are  */
                /* no race conditions that reselect us between the time */
                /* threadExecuteRequest looked at gActiveCommand and    */
                /* the time we disabled interrupts.                     */

            queue_enter( &pendingCmdQ, cmdBuf, CommandBuffer*, link );
            result = kHardwareStartBusy;
        }
    }

    if ( result == kHardwareStartOK )
    {
            /* Activate this command - if we fail later, we'll de-activate it.  */

        ASSERT( gActiveCommand == NULL );
        [ self activateCommand : cmdBuf ];
        ASSERT( scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN );
        [ self ClearCPResults ];

            /* Reset the message-out buffer pointer for the */
            /* Identify, SDTR, and queue tag messages.      */

        msgOutPtr = (UInt8*)CCLAddress( kcclMSGOdata );
        msgByte   = kScsiMsgIdentify | scsiReq->lun;

        if ( okToDisconnect )
            msgByte |= kScsiMsgEnableDisconnectMask;

        *msgOutPtr++ = msgByte;

            /* According to the SCSI Spec, the tag command              */
            /* immediately follows the selection.                       */
            /* Note that autosense is never tagged..                    */
            /* The command was initialized with QUEUE_TAG_NONTAGGED.    */
            /***** Driver Kit only supports simple queue tags.      *****/

        if ( okToQueue )
        {
                /* Avoid using tag QUEUE_TAG_NONTAGGED (zero).  */

            cmdBuf->queueTag = gNextQueueTag;
            if ( ++gNextQueueTag == QUEUE_TAG_NONTAGGED )
                gNextQueueTag++;
            *msgOutPtr++   = kScsiMsgSimpleQueueTag;
            *msgOutPtr++   = cmdBuf->queueTag;
            gMsgOutFlag    |= kFlagMsgOut_Queuing;
        }

            /* Do we need to negotiate SDTR for this target?    */

        msgByte = gPerTargetData[ scsiReq->target ].negotiateSDTR;
        ELG(    scsiReq->target << 16   | gPerTargetData[ scsiReq->target ].inquiry_7,
                gPerTargetData[ scsiReq->target ].syncParms << 16 | msgByte,
                'SYN?', "Sync" );
        if ( !(gPerTargetData[ scsiReq->target ].inquiry_7 & 0x10) )
        {
            msgByte = 0;        /* if Inquiry data doesn't permit Synchronous   */
        }
        if ( msgByte )
        {
        //  gPerTargetData[ scsiReq->target ].negotiateSDTR = 0;
            *msgOutPtr++   = kScsiMsgExtended; /* Extended Message */
            *msgOutPtr++   = 0x03;             /* Message Length   */
            *msgOutPtr++   = kScsiMsgSyncXferReq;
            if ( msgByte == kSyncParmsAsync )
            {
                *msgOutPtr++   = 200 >> 4;     /* Period? used?    */
                *msgOutPtr++   = 0;            /* Offset (async)   */
            }
            else
            {
                *msgOutPtr++   = 100 >> 2;     /* 100 nSec period  */
                *msgOutPtr++   = msgByte >> 4; /* FIFO size        */
            }
            gMsgOutFlag |= kFlagMsgOut_SDTR;
        }/* end IF need to negotiate (a)sync */

        if ( cmdBuf->flagIsAutosense )
        {
            [ self InitAutosenseCCL ];
        }
        else
        {
            cmdBuf->currentDataIndex    = 0;
            cmdBuf->savedDataIndex      = 0;
            if ( cmdBuf->mem )
            {
                [ cmdBuf->mem setPosition : 0 ];
                [ cmdBuf->mem state : &cmdBuf->savedDataState ];
            }
            scsiReq->driverStatus       = SR_IOST_INVALID;
            scsiReq->totalTime          = 0;
            scsiReq->latentTime         = 0;
            [ self UpdateCP : FALSE ];   /* Update the DBDMA Channel Program    */
        }
    
            /***** Can a caller override the default timeout?   *****/
    
        meshAddr->selectionTimeOut  = gSelectionTimeout;
        meshAddr->destinationID     = scsiReq->target;
        meshAddr->syncParms         = gPerTargetData[ scsiReq->target ].syncParms;
        SynchronizeIO();
        [ self RunDBDMA : kcclStart  stageLabel : kcclStageInit ];
        IOGetTimestamp( &cmdBuf->startTime );
    }
    return result;
}/* end hardwareStart */


@end /* AppleMesh_SCSI( Hardware ) */


    /* Obtain a YES/NO type parameter from the config table.    */
    /* @param   configTable The table to examine.               */
    /* @param   paramName  The parameter to look for.           */
    /* @result  Zero if missing from the table or the table     */
    /* value is not YES. One if present in the table and the    */
    /* table value is YES.                                      */

static int getConfigParam( id configTable, const char *paramName )
{
    const char  *value;
    int         rtn = 0;    // default if not present in table


    value = [ configTable valueForStringKey : paramName ];
    if ( value )
    {
        if ( strcmp( value, "YES" ) == 0 )
            rtn = 1;
        [ configTable freeString : value ];
    }
    return rtn;
}/* end getConfigParam */


static unsigned int GetSCSICommandLength( const cdb_t *cdbPtr, unsigned int defaultLength )
{
    unsigned int    result;

        /* Warning: don't use sizeof here - the compiler rounds */
        /* the value up to the next word boundary.              */

    switch ( ((UInt8*)cdbPtr)[0] & 0xE0 )
    {
    case (0 << 5):  result = 6;                                         break;
    case (1 << 5):
    case (2 << 5):  result = 10;                                        break;
    case (5 << 5):  result = 12;                                        break;
    case (6 << 5):  result = (defaultLength != 0) ? defaultLength : 6;  break;
    case (7 << 5):  result = (defaultLength != 0) ? defaultLength : 10; break;
    default:        result = 0;                                         break;
    }
    return result;
}/* end GetSCSICommandLength */


    /* These are the hardware-specific methods that are not */
    /* explicitly tied to Mesh and DBDMA.                   */

@implementation AppleMesh_SCSI( HardwarePrivate )

    /* Fetch the device's bus address and allocate one page of memory   */
    /* for the channel command. (Strictly speaking, we don't need an    */
    /* entire page, but we can use the rest of the page for a permanent */
    /* status log).                                                     */
    /* @param   deviceDescription   Specify the device to initialize.   */
    /* @return  IO_R_SUCCESS if successful, else an error status.       */

- (IOReturn) AllocHdwAndChanMem : deviceDescription
{
    IOReturn    ioReturn = IO_R_SUCCESS;
    enum
    {   kMESHRegisterBase   = 0,
        kDBDMARegisterBase  = 1,
        kNumberRegisters    = 2
    };


    meshAddr    = (MeshRegister*)gMESHPhysAddr      =   0;
    dbdmaAddr   = (dbdma_regmap_t*)dbdmaAddrPhys    =   0;

        /* Set the default selection timeout to the MESH value (10 msec units). */

    gSelectionTimeout = 250 / 10;   // ??? symbolic

        /* Allocate a page of wired-down memory in the kernel. Although */
        /* Driver Kit provides a memory allocator, IOMalloc, it does    */
        /* not guarantee page alignment. Thus, we call the Mach kernel  */
        /* routine. According to the description of kalloc(), 8192 is   */
        /* the smallest amount of memory we can allocate. The channel   */
        /* command area will fit into the start of this area.           */

    cclLogAddrSize  = page_size;
    cclLogAddr      = (DBDMADescriptor*)kalloc( cclLogAddrSize );
    if ( !cclLogAddr )
    {   PAUSE( 0, cclLogAddrSize, 'CCA-', "AllocHdwAndChanMem - can't allocate channel command area.\n" );
        ioReturn = IO_R_NO_MEMORY;
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
        if ( IOIsAligned( cclLogAddr, page_size ) == 0 )
        {
            PAUSE( 0, cclLogAddr, 'cca-', "AllocHdwAndChanMem - not page-aligned.\n" );
            ioReturn = IO_R_NO_MEMORY;
        }
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
            /* Remember the number of DBDMA descriptors that    */
            /* can be used for data transfer channel commands.  */

        gDBDMADescriptorMax = (cclLogAddrSize - kcclDataXfer)
                            / sizeof( DBDMADescriptor );
#if 0
            /* Fetch the logical and physical addresses */
            /* to access the MESH and DBDMA hardware.   */

        memoryRangeList     = [ deviceDescription memoryRangeList ];
        numMemoryRanges     = [ deviceDescription numMemoryRanges ];
        for ( i = 0; i < numMemoryRanges; i++ )
            ELG( memoryRangeList[ i ].start, memoryRangeList[ i ].size, 'Rang', "AllocHdwAndChanMem - range start & size.\n" );
        if ( numMemoryRanges != kNumberRegisters )
        {   PAUSE( memoryRangeList[ i ].start, memoryRangeList[ i ].size, 'Rng-', "AllocHdwAndChanMem - numMemoryRanges != kNumberRegisters.\n" );
            ioReturn = IO_R_INVALID;   /* This "can't happen"   */
        }
#endif
    }

#if 0
    if ( ioReturn == IO_R_SUCCESS )
    {
            /* We know that the first range describes the MESH chip,    */
            /* and the second range describes the DBDMA chip.           */

        gMESHPhysAddr   = (PhysicalAddress)memoryRangeList[ kMESHRegisterBase  ].start;
        dbdmaAddrPhys   = (PhysicalAddress)memoryRangeList[ kDBDMARegisterBase ].start;

            /* Weave together the logical and physical addresses.           */
            /* First, map the MESH and DBDMA chips into our address space.  */

        ioReturn = IOMapPhysicalIntoIOTask( (UInt32)gMESHPhysAddr,
                                            sizeof( MeshRegister ),
                                            (vm_address_t*)&meshAddr );
        if ( ioReturn != IO_R_SUCCESS )
            PAUSE( 0, ioReturn, 'map-', "AllocHdwAndChanMem - MESH mapping err.\n" );
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
        ioReturn = IOMapPhysicalIntoIOTask( (UInt32)dbdmaAddrPhys,
                                            sizeof( dbdma_regmap_t ),
                                            (vm_address_t*)&dbdmaAddr );
        if ( ioReturn != IO_R_SUCCESS )
            PAUSE( 0, ioReturn, 'Map-', "AllocHdwAndChanMem - DBDMA mapping err.\n" );
    }
#else
    if ( ioReturn == IO_R_SUCCESS )
    {
        meshAddr        = (MeshRegister*)gMESHPhysAddr = (PhysicalAddress)PCI_MESH_BASE_PHYS;

    //  dbdmaAddr       = (dbdma_regmap_t*)PCI_MESH_DMA_BASE_PHYS;
        dbdmaAddr       = (dbdma_regmap_t*)DBDMA_REGMAP( DBDMA_MESH_SCSI );
    //  dbdmaAddrPhys   = (PhysicalAddress)KVTOPHYS( (vm_offset_t)dbdmaAddr );
        dbdmaAddrPhys   = (PhysicalAddress)dbdmaAddr;

        ELG( dbdmaAddrPhys, dbdmaAddr, 'DBDM',
                "AllocHdwAndChanMem - DBDMA phys/logical addresses." );
        g.meshAddr = (UInt32)meshAddr;      // for debugging, miniMon ...
#if CustomMiniMon
        gMESH_DBDMA      = (UInt32)dbdmaAddr;
        gMESH_DBDMA_Phys = (UInt32)dbdmaAddrPhys;
#endif /* CustomMiniMon */
    }

#endif

    if ( ioReturn == IO_R_SUCCESS )
    {
            /* Ensure that the addresses are valid: */

        ASSERT( probe_rb( meshAddr  ) == 0 );
        ASSERT( probe_rb( dbdmaAddr ) == 0 );

            /* Get the physical address corresponding the DBDMA channel area:   */

        ioReturn = IOPhysicalFromVirtual(   IOVmTaskSelf(),
                                            (UInt32)cclLogAddr,
                                            (vm_offset_t*)&cclPhysAddr );
        g.cclPhysAddr   = (UInt32)cclPhysAddr;  // for debugging ease
        g.cclLogAddr    = (UInt32)cclLogAddr;
        if ( ioReturn != IO_R_SUCCESS )
            PAUSE( 0, ioReturn, 'MAP-', "AllocHdwAndChanMem - DBDMA mapping err.\n" );
    }

    if ( ioReturn == IO_R_SUCCESS)
    {
        ELG( cclPhysAddr, cclLogAddr, '=CCL',
                "AllocHdwAndChanMem - CCL phys/logical addresses." );
        [ self InitCP ];
    }
        /* What do we do on failure? Should we try to deallocate    */
        /* the stuff we created, or will the system do this for us? */

    return  ioReturn;
}/* end AllocHdwAndChanMem */


    /* Perform one-time-only channel command program initialization.    */

- (void) InitCP
{
    register DBDMADescriptor        *dst = cclLogAddr;
    register const DBDMADescriptor  *src = gDescriptorList;
    UInt32                          i;
    UInt8                           *bp;



        /* Set the interrupt, branch, and wait DBDMA registers.         */
        /* Caution: the following MESH interrupt register bits are      */
        /* EndianSwapped, reverse polarity and in a different position. */
        /* The pattern is: 0xvv00mm00, where mm is a mask byte          */
        /* and vv is a value byte to match. (After EndianSwapping).     */
        /*  0x80    means NO errors         (kMeshIntrError)            */
        /*  0x40    means NO exceptions     (kMeshIntrException)        */
        /*  0x20    means NO command done   (kMeshIntrCmdDone)          */
        /*  Branch Select is used with BRANCH_FALSE                     */

//  DBDMASetInterruptSelect( 0x00000000 );  /* Never let DBDMA interrupt    */
//  DBDMASetWaitSelect( 0x00200020 );       /* Wait until command done      */
//  DBDMASetBranchSelect( 0x00C000C0 );     /* Branch if exception or error */

    *(volatile UInt32*)&dbdmaAddr->d_intselect  = 0x00000000;   /* Never let DBDMA interrupt    */
    *(volatile UInt32*)&dbdmaAddr->d_wait       = 0x20002000;   /* Wait until command done      */
    *(volatile UInt32*)&dbdmaAddr->d_branch     = 0xC000C000;   /* Br if Exc or Err             */
    SynchronizeIO();

        /* Relocate and EndianSwap the global channel command list   */
        /* into the page that is shared with the DBDMA device.       */

    for ( i = 0; i < gDescriptorListSize; i += sizeof( DBDMADescriptor )  )
    {
        dst->operation  = SWAP( src->operation );    /* copy command with count  */

        switch ( src->result & kRelAddress )
        {
        case kRelAddressMESH:
            dst->address = SWAP( src->address + (UInt32)gMESHPhysAddr );
            break;
        case kRelAddressCP:
            dst->address = SWAP( src->address + (UInt32)cclPhysAddr );
            break;
        case kRelAddressPhys:
            dst->address = SWAP( src->address );
            break;
        default:
            dst->address = SWAP( src->address );
            break;
        }

        switch ( src->result & kRelCmdDep )
        {
        case kRelCmdDepCP:
            dst->cmdDep = SWAP( src->cmdDep + (UInt32)cclPhysAddr );
            break;
        case kRelCmdDepLabel:
            dst->cmdDep = src->cmdDep;
            break;
        default:
            dst->cmdDep = SWAP( src->cmdDep );
            break;
        }

        dst->result = 0;
        src++;
        dst++;
    } /* FOR all elements in the descriptor list */

        /* Build a SCSI CDB for the autosense Request Sense command.    */

    bp = (UInt8*)CCLAddress( kcclSenseCDB );
    *bp++ = kScsiCmdRequestSense;   /* Command                              */
    *bp++ = 0;                      /* LUN to be filled in                  */
    *bp++ = 0;                      /* reserved                             */
    *bp++ = 0;                      /* reserved                             */
    *bp++ = kMaxAutosenseByteCount; /* Allocation length - to be filled in  */
    *bp++ = 0;                      /* Control (flag)                       */
    return;
}/* end InitCP */


    /* Initialize the data transfer channel command list for a normal SCSI  */
    /* command. The channel command list has a complex structure of         */
    /* transfer groups and items, where:                                    */
    /*  transfer group      The number of bytes transferred by a single     */
    /*                      MESH operation. This will be from 1 to          */
    /*                      kMaxDMATransferLength (65536 - 4096).           */
    /*  transfer item       The number of bytes transferred by a single     */
    /*                      DBDMA operation. These bytes are guaranteed     */
    /*                      to be physically-contiguous.                    */
    /* Thus, the data transfer CCL looks like the following:                */
    /*      Prolog 1:       Load MESH with the first group count.           */
    /*      Item 1.1:       Load DBDMA with the first physical address and  */
    /*                      item count.                                     */
    /*      Item 1.2 etc:   Load DBDMA with the next physical address and   */
    /*                      item count.                                     */
    /*      Prolog 2, etc.  Load MESH with the next group count.            */
    /*      Item 2.1, etc.  Load DBDMA with the next group of physical      */
    /*                      addresses.                                      */
    /*      Stop/Branch     If all of the data transfer commands fit in the */
    /*                      channel command list, branch to the Status phase*/
    /*                      channel command. Otherwise, stop transfer       */
    /*                      (which stops in Data phase) and re-build the    */
    /*                      command list for the next set of data.          */
    /* Note that the last DBDMA command must be INPUT_LAST or OUTPUT_LAST   */
    /* to handle synchronous transfer odd-byte disconnect.                  */

- (void) UpdateCP : (Boolean) reselecting
{
    CommandBuffer       *cmdBuf;
    IOSCSIRequest       *scsiReq;
    DBDMADescriptor     *descProto = CCLDescriptor( kcclPrototype );
    IOReturn            ioReturn   = IO_R_SUCCESS;
    DBDMADescriptor     *descriptorPtr;     /* current data descriptor          */
    DBDMADescriptor     *descriptorMax;     /* beyond the last data descriptor  */
    DBDMADescriptor     *preamblePtr;       /* current prolog descriptor        */
    UInt32              dbdmaOpProto;       /* prototype Opcode for DBDMA       */
    UInt32              dbdmaOp;            /* Opcode for DBDMA                 */
    UInt32              meshSeq;            /* Opcode for MESH request          */
    SInt32              transferLength;     /* Number of bytes left to transfer */
    UInt32              totalXferLen   = 0; /* Total length of this transfer    */
    UInt32              groupLength;        /* Number of bytes in this group    */
    UInt8               syncParms;          /* Fast synchronous param value     */
    ByteCount           bc;
    PhysicalRange       range;
    ItemCount           rangeByteCount;
    DBDMADescriptor     *dp;


    ASSERT( gActiveCommand && gActiveCommand->scsiReq );
    cmdBuf  = gActiveCommand;
    scsiReq = cmdBuf->scsiReq;
    ASSERT( scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN );

        /* How many descriptors can we store (need some slop for the    */
        /* terminator commands). Get a pointer to the first free        */
        /* descriptor and the total number of bytes left to transfer in */
        /* this IO request.                                             */

    descriptorPtr   = CCLDescriptor( kcclDataXfer );
    descriptorMax   = &descriptorPtr[ gDBDMADescriptorMax - 16 ];
    transferLength  = scsiReq->maxTransfer - cmdBuf->currentDataIndex;
    ELG( cmdBuf, transferLength, 'UpCP', "UpdateCP" );

    if ( reselecting == FALSE )
    {
        [ self SetupMsgO ];     /* Setup for Message Out phase. */

                                /* Setup for Command phase:     */
        CCLByte( kcclCmdoMTC )  = cmdBuf->cdbLength;    /* MESH transfer count  */
        CCLByte( kcclCmdoDTC )  = cmdBuf->cdbLength;    /* DBDMA count          */
        bcopy( &scsiReq->cdb, CCLAddress( kcclCMDOdata ), cmdBuf->cdbLength );
    }

        /* Generate MESH "sequence" & DBDMA "operation" for Input or Output:    */

    if ( scsiReq->read )
    {   dbdmaOpProto    = INPUT_MORE | kBranchIfFalse;
        meshSeq         = kMeshDataInCmd | kMeshSeqDMA;
    }
    else
    {   dbdmaOpProto    = OUTPUT_MORE | kBranchIfFalse;
        meshSeq         = kMeshDataOutCmd | kMeshSeqDMA;
    }

    CCLWord( kcclBatchSize ) = 0;

    while ( ioReturn == IO_R_SUCCESS
            && transferLength > 0
            && descriptorPtr < descriptorMax )
    {
            /* Do one group, ie, enough CCs to fill a MESH transfer count.  */
            /* There are more data to be transferred, and CCL space to store*/
            /* another group of data. First, leave space for the preamble.  */

        preamblePtr      = descriptorPtr;
        groupLength      = 0;
        descriptorPtr   += 4;               /* Preamble takes 4 descriptors */

        while ( transferLength  > 0                     /* more to xfer     */
             && descriptorPtr   < descriptorMax )       /* room in CCL      */
        {
                /* Do one physically contiguous segment:   */

            bc = MAX_DMA_XFER - groupLength; /* calc room left in group  */
            if ( bc < page_size )
                break;
            rangeByteCount = [ cmdBuf->mem getPhysicalRanges : (ItemCount) 1
                                maxByteCount    : bc
                                newPosition     : NULL
                                actualRanges    : NULL
                                physicalRanges  : &range ];

            if ( rangeByteCount == 0 )
                break;

            ASSERT( range.length > 0 );
            groupLength     += range.length;
            transferLength  -= range.length;
            dbdmaOp          = dbdmaOpProto | range.length;
            if ( transferLength <= 0 )
                 dbdmaOp |= (OUTPUT_MORE ^ OUTPUT_LAST);    /* add LAST to cmd  */
            descriptorPtr->operation    = SWAP( dbdmaOp );
            descriptorPtr->address      = SWAP( (UInt32)range.address );
            descriptorPtr->cmdDep       = SWAP( (UInt32)cclPhysAddr + kcclProblem );
            descriptorPtr->result       = 0;    // for debugging
            descriptorPtr++;
        }/* end inner WHILE */

        if ( groupLength == 0 )
        {
                /* Nothing was built - we apparently failed to get              */
                /* a physical address. Note: there is a potential problem with  */
                /* the following sequence as the *previous* DBDMA command, if   */
                /* any, should be changed to set xxPUT_LAST.                    */

            ELG( 0, 0, 'Grp-', "UpdateCP - groupLength is 0" );
            preamblePtr->operation  = SWAP(
                                        NOP_CMD | kBranchIfFalse | kWaitIfTrue );
            preamblePtr->address    = 0;
            preamblePtr->cmdDep     = SWAP( (UInt32)cclPhysAddr + kcclProblem );
            preamblePtr->result     = 0;
            descriptorPtr           = preamblePtr + 1;
            ioReturn = IO_R_INVALID;    /* Exit the outer loop      */
        }
        else
        {
            totalXferLen += groupLength;

                /* This group is complete. Fill in the preamble.        */
                /* The preamble consists of the following commands:     */
                /*  [0] Move <totalXferLen> to kcclBatchSize            */
                /*  [1] Store group length high-byte in MESH            */
                /*      transfer count 1 register                       */
                /*  [2] Store group length low-byte in MESH             */
                /*      transfer count 1 register                       */
                /*  [3] Store the input/output command in the MESH      */
                /*      sequence register.                              */
                /* If the command finishes prematurely (perhaps the     */
                /* device wants to disconnect), the interrupt service   */
                /* routine will use totalXferLen - the residual byte    */
                /* count to determine the number of bytes xferred.      */

            descProto[0].cmdDep = totalXferLen; // update batch size
            descProto[1].cmdDep = SWAP( groupLength >> 8 );
            descProto[2].cmdDep = SWAP( groupLength & 0xFF );
            descProto[3].cmdDep = SWAP( meshSeq );
            bcopy( descProto, preamblePtr, sizeof( DBDMADescriptor ) * 4 );
            ELG( preamblePtr, totalXferLen, '=Tot', "UpdateCP - set preamble" );

                /* If there is another group, wait for */
                /* cmdDone and clear it:               */
            if ( transferLength > 0 )
            {       /* Wait for CmdDone: */
                bcopy( CCLDescriptor( kcclBrProblem ), descriptorPtr, sizeof( DBDMADescriptor ) );
                ++descriptorPtr;
                    /* Clear CmdDone:    */
                    /* HACK - if we reached the end of the CCL page,       */
                    /* we don't want to clear cmdDone because we will lose */
                    /* an interrupt. So, this instruction may be deleted   */
                    /* down below. (Radar 2298440)                         */
                descriptorPtr->operation = SWAP( STORE_QUAD | KEY_SYSTEM | 1 );
                descriptorPtr->address   = SWAP( (UInt32)gMESHPhysAddr + kMeshInterrupt );
                descriptorPtr->cmdDep    = SWAP( kMeshIntrCmdDone );
                descriptorPtr->result    = 0;
                ++descriptorPtr;
            }/* end IF not last group */
        }/* end if/ELSE a group was built */
    }/* end outer WHILE */

        /* All of the data have been transferred (or we ran off the end */
        /* of the CCL). Update the transfer start index to reflect on   */
        /* what we *think* we will transfer in this DATA operation. If  */
        /* we completed DATA phase, branch to the Status Phase CCL;     */
        /* if not, stop the channel command so we can reload the CCL    */
        /* with the next big chunk.                                     */
        /* When the transfer completes, the last prolog will have stored*/
        /* the total number of bytes transferred in a known location in */
        /* the CCL area.                                                */
        /* Now, append the data transfer postamble to handle            */
        /* synchronous odd-byte disconnect and jump to status phase     */
        /* (or just stop if there's more DMA)                           */

#define kMaxPostamble           9
#define kDBDMADescriptorEnd     (CCLDescriptor(kcclDataXfer) + gDBDMADescriptorMax)

     ASSERT( descriptorPtr + kMaxPostamble < kDBDMADescriptorEnd );

         /* Do some synchronous data transfer cleanup: */

    syncParms = gPerTargetData[ scsiReq->target ].syncParms;
    meshAddr->syncParms = syncParms;
    SynchronizeIO();
    ELG( gMsgOutFlag, syncParms, 'SynP', "UpdateCP - sync parms" );

    if ( ((syncParms & 0xF0) || (gMsgOutFlag & kFlagMsgOut_SDTR))  // Sync?
     && (totalXferLen > 0)         // any data moving?
     && (transferLength == 0) )    // end of xfer?
    {
        gFlagIncompleteDBDMA = FALSE;               /* indicate complete xfer  */

            /* MESH has a problem at the end of Synchronous transfers.          */
            /* If the target is fast enough, it can move from data phase to     */
            /* Status phase while MESH still has ACKed bytes in its FIFO and    */
            /* the DBDMA is still running. MESH raises PhaseMismatch Exception  */
            /* causing an interrupt in which we must empty the FIFO and move    */
            /* the bytes to the user's buffer by programmed IO.                 */
            /* If the target is not fast enough, we can save the interrupt and  */
            /* bypass the mess.                                                 */
            /* So, we do the following:                                         */
            /* 1)  Enable only MESH Err interrupts; disable Exc and CmdDone.    */
            /* 2)  Don't Wait; Branch if an interrupt may have already occurred.*/
            /* 3)  Wait for cmdDone at least for TC = FIFO count = 0 and        */
            /*     maybe including PhaseMismatch. Branch to SyncCleanup if PMM. */
            /* 4)  Assume an interphase condition as opposed to an              */
            /*     overrun condition and Branch Always to get Status.           */

            /* If the Channel Program gets this far, the OUTPUT_LAST        */
            /* has finished writing its data to the FIFO and MESH may still */
            /* be putting bytes on the bus OR the INPUT_LAST has read all   */
            /* its data from the FIFO and MESH has already ACKed them.      */
            /* There may be or not some time before REQ appears again,      */
            /* either for data overrun or the next phase.                   */

            /* Disable Exc and CmdDone (leave Err enabled): */

        descriptorPtr->operation    = SWAP( STORE_QUAD | KEY_SYSTEM | 1 );
        descriptorPtr->address      = SWAP( (UInt32)gMESHPhysAddr + kMeshInterruptMask );
        descriptorPtr->cmdDep       = SWAP( kMeshIntrError );
        descriptorPtr->result       = 0;
        ++descriptorPtr;

            /* Take the interrupt if PhaseMismatch not definitely caught.        */
            /* Branch (don't wait for cmdDone) if Exc may have already occurred: */

        descriptorPtr->operation    = SWAP( NOP_CMD | kBranchIfFalse );
        descriptorPtr->address      = 0;
        descriptorPtr->cmdDep       = SWAP( (UInt32)cclPhysAddr + kcclProblem );
        descriptorPtr->result       = 0;
            /* Radar 2281306 ( and 2272931 ):                                 */
            /* Output may completely fit in the FIFO and not make it out      */
            /* to the SCSI bus if the target disconnects after the command.   */
            /* If that's possible, wait here for cmdDone and                  */
            /* take the PhaseMismatch interrupt. This situation occurred on a */
            /* Mode Select with an output of 12 bytes. Do this to prevent     */
            /* the Stage from advancing from kcclStageXfer so that proper     */
            /* cleanup can take place.                                        */
        if ( (totalXferLen < 16) && !scsiReq->read )
            descriptorPtr->operation= SWAP( NOP_CMD | kWaitIfTrue | kBranchIfFalse );
        ++descriptorPtr;

            /* Possible PhaseMisMatch caught after FIFO emptied. */
            /* Wait for cmdDone. If Exc, branch to SyncCleanUp:  */

        descriptorPtr->operation    = SWAP( NOP_CMD | kWaitIfTrue | kBranchIfFalse );
        descriptorPtr->address      = 0;
        descriptorPtr->cmdDep       = SWAP( (UInt32)cclPhysAddr + kcclSyncCleanUp );
        descriptorPtr->result       = 0;
        descriptorPtr++;

            /* Interphase condition or possible overrun.  */
            /* 29sep98 PhaseMismatch occurred even after  */
            /* CmdDone was set.                           */


            /* Branch Always to assume we will bit bucket some data: */

        descriptorPtr->operation    = SWAP( NOP_CMD | kBranchAlways );
        descriptorPtr->address      = 0;
        descriptorPtr->cmdDep       = SWAP( (UInt32)cclPhysAddr + kcclOverrun );
        descriptorPtr->result       = 0;
        descriptorPtr++;

            /* Fix up the DataOverrun code just in case: */

        dp = CCLDescriptor( kcclOverrunMESH );
        if ( scsiReq->read )
        {   dp->cmdDep = SWAP( kMeshDataInCmd | kMeshSeqDMA );
            dp = CCLDescriptor( kcclOverrunDBDMA ); 
            dp->operation = SWAP( INPUT_LAST | kBranchIfFalse | 8 );
        }
        else
        {   dp->cmdDep = SWAP( kMeshDataOutCmd | kMeshSeqDMA );
            dp = CCLDescriptor( kcclOverrunDBDMA ); 
            dp->operation = SWAP( OUTPUT_LAST | kBranchIfFalse | 8 );
        }
    }/* end IF last of Synchronous transfer */
    else
    {
            /* Async or incomplete Sync. Append Branches to finish this process: */

            /* If this is a partial transfer, set 'incomplete' flag.  */

        if ( transferLength > 0 )
             gFlagIncompleteDBDMA = TRUE;    /* set incomplete        */
        else gFlagIncompleteDBDMA = FALSE;   /* assume complete xfer  */


        if ( gFlagIncompleteDBDMA )
        {                        /* Delete the ccl to clear cmdDone:  */
             --descriptorPtr;    /* see HACK note above.              */
        }
        else if ( totalXferLen > 0 )
        {       /* If something moved AND (Radar 2298440) xfer completed,  */
                /*  Wait & Branch if problem:                              */
                /* Radar 2272931 - If entire output fits in FIFO, then     */
                /* the OUTPUT_LAST completes OK without a PhaseMismatch if */
                /* the target disconnects right after the command phase.   */
            bcopy( CCLDescriptor( kcclBrProblem ), descriptorPtr, sizeof( DBDMADescriptor ) );
            descriptorPtr++;
        }
            /* Assume all's well - Branch to get status: */
        descriptorPtr->operation    = SWAP( NOP_CMD | kBranchAlways );
        descriptorPtr->address      = 0;
        descriptorPtr->cmdDep       = SWAP( (UInt32)cclPhysAddr + kcclGetStatus );
        descriptorPtr->result       = 0;

            /* If this is a partial transfer, set 'incomplete' flag and */
            /* change the Branch from GetStatus to Good:                */

        if ( gFlagIncompleteDBDMA )
        {       /* change last Branch from Status to Good: */
            descriptorPtr->cmdDep = SWAP( (UInt32)cclPhysAddr + kcclMESHintr );
            ELG( descriptorPtr, transferLength, 'Part', "UpdateCP - built partial CCL." );
        }
        descriptorPtr++;
    }/* end if/ELSE Async or partial xfer */

    ASSERT( descriptorPtr < kDBDMADescriptorEnd );
    return;
}/* end UpdateCP */


    /* StartBucket -  Start the channel commands to run the bit bucket. */

- (void) StartBucket
{
    CommandBuffer       *cmdBuf;
    IOSCSIRequest       *scsiReq;
    DBDMADescriptor     *dp;      /* current data descriptor          */
    UInt32              dbdmaOp;  /* Opcode for DBDMA                 */
    UInt32              meshSeq;  /* Opcode for MESH request          */


    cmdBuf  = gActiveCommand;
    scsiReq = cmdBuf->scsiReq;
    ELG( cmdBuf, scsiReq, 'Bkt-', "StartBucket" );

        /* Generate MESH "sequence" & DBDMA "operation" for Input or Output:   */

    if ( scsiReq->read )
    {   dbdmaOp = INPUT_MORE | kBranchIfFalse | 8;
        meshSeq = kMeshDataInCmd | kMeshSeqDMA;
    }
    else
    {   dbdmaOp = OUTPUT_MORE | kBranchIfFalse | 8;
        meshSeq = kMeshDataOutCmd | kMeshSeqDMA;
    }

    dp = CCLDescriptor( kcclOverrunMESH );   dp->cmdDep    = meshSeq;
    dp = CCLDescriptor( kcclOverrunDBDMA );  dp->operation = dbdmaOp;

    [ self RunDBDMA : kcclDataXfer  stageLabel : kcclStageBucket ];
    return;
}/* end StartBucket */


    /* Set up the channel commands for MsgO phase.  */

- (void) SetupMsgO
{
    UInt8       msgoSize;


    msgOutPtr--;       /* treat the last or only byte special (drop ATN)   */
    msgoSize = msgOutPtr - CCLAddress( kcclMSGOdata );
    if( msgoSize == 0 )
    {       /* Identify byte only:  */
        CCLWord( kcclMsgoBranch ) = SWAP( NOP_CMD | kBranchAlways );
    }
    else    /* multibyte message - set counts for all but last byte:    */
    {   CCLByte( kcclMsgoMTC )  = msgoSize;
        CCLByte( kcclMsgoDTC )  = msgoSize;
            /* NOP the BRANCH:  */
        CCLWord( kcclMsgoBranch ) = SWAP( NOP_CMD );
    }
    CCLByte( kcclMSGOLast )= *msgOutPtr;           /* position last byte   */
    return;
}/* end SetupMsgO */


    /* Initialize the autosense area and build the autosense channel command.   */

- (void) InitAutosenseCCL
{
    ELG( 0, 0, 'Auto', "InitAutosenseCCL" );
        /* Make sure we've allocated enough space in the CCL area.  */

    ASSERT( kcclSenseBuffer + kMaxAutosenseByteCount < kcclSenseResult );
    bzero( CCLAddress( kcclSenseBuffer ), kMaxAutosenseByteCount );

        /* Copy the Sense CDB to the CDB area and   */
        /* copy the Sense CCL to the Xfer area      */

        /* Copy the Sense CDB & Sense CCL */
    bcopy( CCLAddress( kcclSenseCDB ), CCLAddress( kcclCMDOdata ), 6 );
    bcopy( CCLAddress( kcclSense ), CCLAddress( kcclDataXfer ), 5 * sizeof( DBDMADescriptor ) );

        /* Set the MESH and DBDMA transfer counts for the command.  */
    CCLByte( kcclCmdoMTC ) = 6;
    CCLByte( kcclCmdoDTC ) = 6;

        /* Set the data transfer count (use a hard-wired value).   */

    CCLWord( kcclBatchSize ) = kMaxAutosenseByteCount;

    [ self SetupMsgO ];

    return;
}/* end InitAutosenseCCL */


- (void) ClearCPResults
{
    register DBDMADescriptor    *dp = CCLDescriptor( kcclStart );
    register int                i;


        /*  Don't clear the reserved areas or prototypes    */

    for ( i = (gDescriptorListSize - kcclStart) / sizeof ( DBDMADescriptor ); i; --i )
    {
        dp->result = 0;
        dp++;
    }

    return;
}/* end ClearCPResults */


@end /* AppleMesh_SCSI(HardwarePrivate) */




@implementation AppleMesh_SCSI( MeshInterrupt )


    /* DoHardwareInterrupt - Handle an Interrupt Service message    */

- (void) DoHardwareInterrupt    /* called from interruptOccurred    */
{
    [ self GetHBARegsAndClear : TRUE ];     /* get the MESH registers   */
    [ self SetIntMask : 0 ];                /* Disable MESH interrupts  */

    gFlagReselecting = FALSE;

    if ( g.shadow.mesh.interrupt == 0 )
    {       /* Interrupts can occur with no bits set in the         */
            /* interrupt register one way:                          */
            /*  -   Eating interrupts in the driver (the ASIC       */
            /*          latches the interrupt even though the       */
            /*          driver or Channel Program clears the MESH   */
            /*          interrupt register).                        */
        PAUSE(  dbdmaAddr->d_cmdptrlo,
                (g.shadow.mesh.busStatus0 << 8) | g.shadow.mesh.busStatus1,
                'ISR?',
                "DoHardwareInterrupt - spurious interrupt" );

        if ( !gActiveCommand )
        {
            [ self selectNextRequest ];
            if ( !gActiveCommand && queue_empty( &abortCmdQ ) )
                    /* if neither new request nor aborting:   */
                [ self SetIntMask : kMeshIntrMask ];/* Enable interrupts    */
        }
        return;
    }/* end IF no bit set in interrupt register */

    dbdma_flush( DBDMA_MESH_SCSI );         /* DBDMA may be hung in */
    dbdma_stop(  DBDMA_MESH_SCSI );         /* middle of transfer.  */
//  invalidate_cache_v( (vm_offset_t)cclLogAddr, cclLogAddrSize );

        /* If the DBDMA was running a channel command, handle this  */
        /* (this could be done at a lower priority level).          */

    if ( CCLWord( kcclStageLabel ) )
    {
        [ self ProcessInterrupt ];
        return;
    }

        /* This was not a DBDMA completion.         */
        /* See if the last MESH operation completed */
        /* without errors or exceptions.            */

    if ( g.shadow.mesh.interrupt == kMeshIntrCmdDone )
    {
            /* This was presumably a Programmed IO completion.  */

        if ( gActiveCommand )
        {       /* The command has not completed yet.                   */
                /* We need to wait for a phase stabilizing interrupt.   */
    
            PAUSE( 0, 0, 'dhi-', "DoHardwareInterrupt - MESH interrupt problem: need phase stabilizing wait.\n" );
            return;
        }
        else
        {       /* There is no active command.                  */
                /* This is presumably a bus-free completion.    */

            [ self selectNextRequest ]; /* Try to start another request.*/

            if ( !gActiveCommand && queue_empty( &abortCmdQ ) )
                    /* If still nothing to do:  */
                [ self SetIntMask : kMeshIntrMask ];/* Re-enable ints   */
            return;
        }
    }/* end IF CmdDone without Err or Exc */

        /* None of the above "completion" states occurred.      */
        /* Either a command completed unsuccessfully, or we     */
        /* were reselected. First, check for phase mismatch.    */
    if ( g.shadow.mesh.interrupt == (kMeshIntrCmdDone | kMeshIntrException)
     &&  g.shadow.mesh.exception == kMeshExcPhaseMM )
    {
            PAUSE( 0, 0, 'DHI-', "DoHardwareInterrupt - MESH interrupt problem: phase mismatch interrupt.\n" );
    }
    else
    {       /* Handle reselection and all other problems separately.    */
            /* (This can be done at a lower priority.)                  */
        [ self ProcessInterrupt ];
    }
    return;
}/* end DoHardwareInterrupt */


    /* Respond to a DBDMA channel command completion interrupt  */
    /* or some error or exception condition.                    */

- (void) ProcessInterrupt
{
    register CommandBuffer  *cmdBuf;
    register IOSCSIRequest  *scsiReq;
    UInt32                  stage;          /* Stage in the Channel Program */
    UInt32                  cclIndex;       /* Index of CCL descriptor      */
    UInt32                  count;          /* transfer count               */
    UInt8                   phase;          /* Current bus phase            */
    IOReturn                rc;


    if ( gActiveCommand == NULL )
    {
        if ( g.shadow.mesh.exception & kMeshExcResel )
        {
            [ self HandleReselectionInterrupt ];
        }
        else
        {       /* There is no active request and we are not reselecting.   */
                /* Can get here if Reject/Abort occurs or after a BusFree   */
                /* command is put in the Sequence register and we exit the  */
                /* interrupt.                                               */
            if ( !queue_empty( &abortCmdQ ) )
            {
                [ self SetSeqReg : kMeshFlushFIFO ];     /* flush the FIFO  */
                queue_remove_first( &abortCmdQ, cmdBuf, CommandBuffer*, link );
                [ self ioComplete : cmdBuf ];
            }/* end IF Aborting disconnected commands */
            else
            {       /* This should be a Bus Free interrupt: */
                ELG( 0, 0, 'Int0', "Process interrupt with no active request\n" );
            }

            [ self selectNextRequest ];
            if ( !gActiveCommand && queue_empty( &abortCmdQ ) )
                    /* Re-enable ints for reselect  */
                [ self SetIntMask : (kMeshIntrException | kMeshIntrError) ];
        }
        return;
    }/* end IF had no active command */

        /* There is an active request:                  */
        /* get the stage of the CCL and Switch on it.   */
    stage       = CCLWord( kcclStageLabel );
    cmdBuf      = gActiveCommand;
    scsiReq     = cmdBuf->scsiReq;
    cclIndex    = SWAP( dbdmaAddr->d_cmdptrlo )
                - (UInt32) cclPhysAddr;
    CCLWord( kcclStageLabel ) = 0;
    ASSERT( scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN );

        /* Analyse where the DBDMA ended up:    */

    switch ( stage )
    {
    case kcclStageGood:                         /* Normal completion        */
        [ self DoInterruptStageGood ];
        break;

    case kcclStageInit:                         /* Value before DBDMA runs  */
    case kcclStageArb:                          /* Arbitration anomaly      */
        [ self DoInterruptStageArb ];
        break;

    case kcclStageSelA:                         /* Selection anomaly       */
        [ self DoInterruptStageSelA ];
        break;

    case kcclStageMsgO:                         /* Message Out              */
        [ self DoInterruptStageMsgO ];
        break;

    case kcclStageCmdO:                         /* Command stage anomaly   */
        [ self DoInterruptStageCmdO ];
        break;

    case kcclStageXfer:
        if ( cmdBuf->flagIsAutosense )
             [ self DoInterruptStageXferAutosense ];
        else [ self DoInterruptStageXfer ];     /* DMA transfer complete    */
        break;
    
    case kcclStageStat:             /* Synchronous, odd transfer, data-out  */
                                    /* OR no data, disconnect               */

            /* Don't use UpdateCurrentIndex here because */
            /* kcclStageStat destroys TC with a 1.       */
        count  = CCLWord( kcclBatchSize );          /* Our transfer count   */
        cmdBuf->currentDataIndex += count;          /* Increment data index */
        if (  cmdBuf->mem )
            [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];

        ELG( count, cmdBuf->currentDataIndex, 'Uidx', "ProcessInterrupt" );
        CCLWord( kcclBatchSize ) = 0;              /* Clear our count       */

            /* Analyze the current bus signals: */

        if ( !(g.shadow.mesh.busStatus0 & kMeshReq) )
        {       /* Get here if Sync Read or Write is too short as   */
                /* in reading 512 bytes from a 2K block of CD-ROM.  */
            [ self StartBucket ];
            return;
        }/* end IF no REQ signal */

        phase = g.shadow.mesh.busStatus0 & kMeshPhaseMask;
        switch ( phase )
        {
        case kBusPhaseMSGI:
            rc = [ self DoMessageInPhase ];
        //  if ( rc == IO_R_SUCCESS && gActiveCommand )
        //      break;              /* msg processed ok & not disconnect    */
                    /* Enable Exc (for Reselect) and Err interrupts (not CmdDone)*/
            break;

        case kBusPhaseDATO:
        case kBusPhaseDATI:
                /* Get here if Async Read or Write is too short as  */
                /* in reading 512 bytes from a 2K block of CD-ROM   */
            [ self StartBucket ];
            break;

        default:
            PAUSE( scsiReq->target, phase, 'pmm-',
                            "ProcessInterrupt - expected Status phase.\n" );
            break;
        }/* end SWITCH on phase */
        break;

    case kcclStageBucket:
        count  = CCLWord( kcclBatchSize );          /* Our transfer count   */
        cmdBuf->currentDataIndex += count;          /* Increment data index */
        if (  cmdBuf->mem )
            [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];
        CCLWord( kcclBatchSize ) = 0;              /* Clear our count       */
 
        ELG( count, cmdBuf->currentDataIndex, 'Buck', "ProcessInterrupt - bit bucket done.\n" );

    //  scsiReq->driverStatus = SR_IOST_DMAOR;     /* set DMA OverRun error */

        [ self SetSeqReg : kMeshFlushFIFO ];       /* flush the FIFO        */
        [ self RunDBDMA : kcclGetStatus stageLabel : kcclStageStat ];
        break;

    case kcclStageMsgI:         /* Message-in:     */
    case kcclStageFree:         /* Bus free:       */
    default:                    /* Can't happen?   */
        PAUSE( cclIndex, stage, 'P i-', "ProcessInterrupt - strange or unknown interrupt for device.\n" );
        break;
    }/* end SWITCH on Channel Program stage */
    return;
}/* end ProcessInterrupt */


    /* The channel command (and, hence, the IO request) ran to  */
    /* completion without problems. Complete this IO request    */
    /* and try to start another.                                */

- (void) DoInterruptStageGood
{
    register CommandBuffer      *cmdBuf;
    register IOSCSIRequest      *scsiReq;
    UInt32                      totalXferLen;
    IOMemoryDescriptorState     state;
    UInt8                       byte;


    ASSERT( gActiveCmd && gActiveCmd->scsiReq );

//  [ self SetSeqReg : kMeshEnableReselect ];   // done by CCL

    cmdBuf  = gActiveCommand;
    scsiReq = cmdBuf->scsiReq;

            /* Retrieve the total number of bytes transferred   */
            /* in the last data phase.                          */

    cmdBuf->flagRequestSelectOK = FALSE;
    totalXferLen = CCLWord( kcclBatchSize );
    CCLWord( kcclBatchSize ) = 0;

    ELG( scsiReq, totalXferLen, 'Good', "DoInterruptStageGood" );

    if ( cmdBuf->flagIsAutosense )
    {
            /* We are completing an autosense command.      */
            /* Copy the status byte (which had better be    */
            /* "good" ) from the CCL to the autosense status*/
            /* and complete the IO request. The autosense   */
            /* data itself was copied into the user buffer  */
            /* by a previous 'Xfer' interrupt.              */

        cmdBuf->autosenseStatus = CCLByte( kcclStatusData );
        [ self deactivateCmd : cmdBuf ];
        [ self ioComplete    : cmdBuf ];
    }
    else
    {       /* We are completing a normal command.                  */
            /* Update the transfer count and current data pointer.  */

        cmdBuf->currentDataIndex += totalXferLen;
        if (  cmdBuf->mem )
            [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];

        if ( gFlagIncompleteDBDMA == FALSE )
        {
                /* Yes, the IO is really complete:  */

            scsiReq->scsiStatus     = CCLByte( kcclStatusData );
            scsiReq->driverStatus   = SR_IOST_GOOD;

                /* If this was an Inquiry, peek at the data */
                /* for Synchronous and Queuing support:     */

            if ( (*(UInt8*)&scsiReq->cdb == kScsiCmdInquiry)
             &&  (cmdBuf->currentDataIndex > 7) )
            {
                [ cmdBuf->mem state : &state ];     /* save context */
                [ cmdBuf->mem setPosition : 7 ];
                if ( [ cmdBuf->mem readFromClient : &byte count : 1 ] == 1 )
                {
                    gPerTargetData[ scsiReq->target ].inquiry_7 = byte;
                    ELG( scsiReq->target, byte, 'Inq+',
                        "DoInterruptStageGood - peek at Inquiry data" );
                }
                [ cmdBuf->mem setState : &state ];  /* restore context  */
            }
            [ self deactivateCmd    : cmdBuf ];
            [ self ioComplete       : cmdBuf ];
        }
        else
        {       /* The CCL ended, but the caller expected more data.    */
                /* Restart the CCL.                                     */
                /* Don't regenerate arbitration or command stuff.       */

            [ self UpdateCP : TRUE ];
            [ self RunDBDMA : kcclDataXfer  stageLabel : kcclStageXfer ];
            return;
        }/* end ELSE need to continue Channel Program */
    }/* end ELSE not AutoSense */

        /* Since IO completed (otherwise, we would have exited in the   */
        /* "return" above), check whether a reselection attempt         */
        /* is piggy-backed on top of the good DBDMA completion.         */

    if ( g.shadow.mesh.exception & kMeshExcResel )
        [ self HandleReselectionInterrupt ];
    else           /* Nothing happening. Try to start another request.  */
    {
        [ self selectNextRequest ];
        if ( !gActiveCommand && queue_empty( &abortCmdQ ) )
                   /* Re-enable ints for reselect  */
            [ self SetIntMask : (kMeshIntrException | kMeshIntrError) ];
    }

    return;
}/* end DoInterruptStageGood */


    /* Process the autosense data transfer phase. IO is not complete.   */
    /* There are several reasons why we might get here:                 */
    /*  -- autosense completion (which could be a separate stage)       */
    /*  -- DMA completion with more DMA to do                           */
    /*  -- Bus phase mismatch (short transfer or disconnect)            */

- (void) DoInterruptStageXferAutosense
{
    register CommandBuffer  *cmdBuf     = gActiveCommand;
    IOSCSIRequest           *scsiReq    = cmdBuf->scsiReq;
    UInt32                  residual;
    UInt32                  count;


    cmdBuf->flagRequestSelectOK = FALSE;

        /* An autosense Data In transfer is complete. Copy the  */
        /* autosense data from our private buffer to the        */
        /* caller's sense_data area and restart IO to get the   */
        /* status and command-complete message byte.            */

    residual = SWAP( CCLWord( kcclSenseResult ) ) & 0xFF;
    count = kMaxAutosenseByteCount - residual;
    ASSERT( count <= sizeof( esense_reply_t ) );
    bcopy( CCLAddress( kcclSenseBuffer ), &scsiReq->senseData, count );

        /* Driver Kit does not return "sense valid" or  */
        /* the actual sense transfer count.             */
        /* Restart the channel command to fetch the     */
        /* status byte and Command Completion byte.     */

    [ self RunDBDMA : kcclGetStatus stageLabel : kcclStageStat ];
    return;
}/* end DoInterruptStageXferAutosense */


    /* Process a normal data phase interrupt. IO is not complete.   */
    /* There are several reasons why we might get here:             */
    /*  -- autosense completion (which could be a separate stage)   */
    /*  -- DMA completion with more DMA to do                       */
    /*  -- Bus phase mismatch (short transfer or disconnect, MsgIn) */
    /* Note that we know that we are not in autosense.              */

- (void) DoInterruptStageXfer
{
    register CommandBuffer  *cmdBuf     = gActiveCommand;
    UInt32                  count;          /* DMA transfer count   */
    UInt8                   phase;          /* Current bus phase    */
    IOReturn                rc;
    int                     goAround;


    cmdBuf->flagRequestSelectOK = FALSE;
    count = cmdBuf->currentDataIndex;

    [ self UpdateCurrentIndex ];

    do
    {   goAround = FALSE;       /* assume loop not repeated */
        [ self SetSeqReg : kMeshFlushFIFO ];

            /* We've cleaned up the mess from the previous data transfer.   */
            /* Look at the current bus phase. The channel command waited    */
            /* for REQ to be set before interrupting the processor.         */

        phase = g.shadow.mesh.busStatus0 & kMeshPhaseMask;
        ASSERT( g.shadow.mesh.busStatus0 & kMeshReq );  /* REQ is set, right?   */

        switch ( phase )
        {
        case kBusPhaseSTS:
            gFlagIncompleteDBDMA = FALSE;       /* indicate no-more-data    */
            [ self RunDBDMA : kcclGetStatus     stageLabel : kcclStageStat ];
            break;

        case kBusPhaseMSGI:
            rc = [ self DoMessageInPhase ];
            if ( rc == IO_R_SUCCESS && gActiveCommand )
                goAround = TRUE;                /* msg ok & not disconnect  */
            break;

        case kBusPhaseDATO:
        case kBusPhaseDATI:
            if ( count != cmdBuf->currentDataIndex )
            {       /* Data phase had already started:  */
                PAUSE( 0, phase, 'dat-', "DoInterruptStageXfer - unexpected Data phase.\n" );
            }
            else
            {       /* try starting data phase again    */
                [ self RunDBDMA : kcclDataXfer  stageLabel : kcclStageXfer ];
            }
            break;

        default:
            PAUSE( cmdBuf->scsiReq->target, phase, 'Phs-', "DoInterruptStageXfer - bogus phase.\n" );
            break;
        }/* end SWITCH on phase */
    } while ( goAround );
    return;
}/* end DoInterruptStageXfer */


    /* DoInterruptStageArb - Process an anomaly during arbitration.                 */

- (void) DoInterruptStageArb
{
    ASSERT( gActiveCommand );
    ELG( 0, 0, 'Arb-', "DoInterruptStageArb - Lost arbitration.\n" );

    [ self pushbackCurrentRequest : gActiveCommand ];
    ASSERT( gActiveCommand == NULL );

    if ( g.shadow.mesh.exception & kMeshExcResel )
    {   if ( g.shadow.mesh.error & kMeshErrDisconnected )
        {
                /* 18sep98 - Sometimes MESH gets real confused when its        */
                /* arbitration loses to a target's reselect arbitration.       */
                /* The registers show Exc:ArbLost, Resel and Err:UnExpDisc.    */
                /* The FIFO count is 1 (should be SCSI ID bits) while the      */
                /* BusStatus0,1 registers show IO and Sel both of which are    */
                /* set by the reselecting Target.                              */
                /* The SCSI bus anaylzer shows the following events occcurring */
                /* within a few microseconds of BSY being set by the target:   */
                /*      bus free for at least hundreds of microseconds         */
                /*      Target raises BSY along with its ID bit                */
                /*      Target raises SEL                                      */
                /*      Target raises IO to indicate reselection               */
                /*      Target adds MESH's ID bit                              */
                /*      Target drops BSY                                       */
                /*      MESH raises BSY to accept reselection                  */
                /* **** MESH drops BSY **** here is where MESH is confused     */
                /*      Target stays on bus for 250 milliseconds.              */
                /* To solve this problem, whack MESH with a RstMESH.           */

            ELG( ' Rst', 'MESH', 'UEP-', "DoInterruptStageArb - Resel/Unexpected Disconnect.\n" );
            [ self SetSeqReg            : kMeshResetMESH ]; /* completes quickly */
            [ self GetHBARegsAndClear   : TRUE ];           /* clear cmdDone     */
            [ self SetSeqReg            : kMeshEnableReselect ];
            [ self SetIntMask           : kMeshIntrMask ];  /* Enable Interrupts */
            return;                      /* now wait for another reselect interrupt */
        }
        [ self HandleReselectionInterrupt ];
    }
    else
    {       /* 22sep97 - lost arbitration without reselection.      */
            /* Probably lost the reselect condition processing an   */
            /* error or something.                                  */
        ELG( 0, 0, 'ARB-', "DoInterruptStageArb - Lost arbitration without reselect.\n" );
    }
    return;
}/* end DoInterruptStageArb */


    /* Process an anomaly during target selection.  */

- (void) DoInterruptStageSelA
{
    ASSERT( gActiveCommand );
    [ self SetSeqReg : kMeshEnableReselect ];
    [ self SetSeqReg : kMeshBusFreeCmd ];   /* clear ATN signal MESH left on    */
    [ self killCurrentRequest ];
    [ self GetHBARegsAndClear : FALSE ];            /* check MESH registers */
    [ self SetIntMask : kMeshIntrMask ];            /* Enable Interrupts    */
    return;
}/* end DoInterruptStageSelA */


    /* Process an anomaly during Message-Out phase. */
    /* Target probably doing Message Reject (0x07). */ 

- (void) DoInterruptStageMsgO
{
    UInt8       phase;
    IOReturn    rc;


    ASSERT( gActiveCommand );

    phase = g.shadow.mesh.busStatus0 & kMeshPhaseMask;      /* phase me */
    PAUSE( gActiveCommand, phase, 'Mgo-',
                "DoInterruptStageMsgO - error during msg-out phase.\n" );

    switch ( phase )
    {
        case kBusPhaseMSGI:
            rc = [ self DoMessageInPhase ];
            if ( rc != IO_R_SUCCESS )
            {
                PAUSE( 0, rc, ' MI-',
                            "DoInterruptStageMsgO - MsgIn during MsgOut phase.\n" );
            //  ??? need to get to bus-free from here
            //  ??? need to blow off the IO
            }
            else
            {   ELG( 0, gMsgInFlag, 'rej?', "DoInterruptStageMsgO - got MsgIn.\n" );
                if ( gMsgInFlag & kFlagMsgIn_Reject )
                    [ self AbortActiveCommand ];
            }
            break;

        default:
            PAUSE( gMsgInFlag, phase, 'mgo-',
                        "DoInterruptStageMsgO - unknown phase during MsgOut phase.\n" );
            break;
    }
    return;
}/* end DoInterruptStageMsgO */


    /* DoInterruptStageCmdO - Process an anomaly during command stage.  */

- (void) DoInterruptStageCmdO
{
    register CommandBuffer  *cmdBuf;
    UInt8                   phase;
    IOReturn                rc;


        /* See if this is part of the normal AbortTag/BusDeviceReset process: */


    if ( !queue_empty( &abortCmdQ ) )
    {
        [ self SetSeqReg : kMeshFlushFIFO ];                /* flush the FIFO  */
        cmdBuf = (CommandBuffer*)queue_first( &abortCmdQ );
        ELG( cmdBuf, 0, 'Abo-', "DoInterruptStageCmdO - Aborting." );
        queue_remove( &abortCmdQ, cmdBuf, CommandBuffer*, link );
        [ self ioComplete : cmdBuf ];
        [ self AbortDisconnectedCommand ];    /* do the next, if any */
        return;
    }

        /* Not aborting - something bad happened: */

    ASSERT( gActiveCommand );
    cmdBuf = gActiveCommand;
    cmdBuf->flagRequestSelectOK = FALSE;
    phase = g.shadow.mesh.busStatus0 & kMeshPhaseMask;      /* phase me */
    ELG( cmdBuf, phase, 'CMD?', "DoInterruptStageCmdO - anomaly during Cmd phase.\n" );

    if ( phase == kBusPhaseMSGI )
    {       /* We are probably negotiating SDTR or          */
            /* getting rejected on a nonzero LUN.           */
        rc = [ self DoMessageInPhase ];
        if ( rc != IO_R_SUCCESS )
        {
            PAUSE( 0, rc, ' mi-',
                        "DoInterruptStageCmdO - MsgIn during Cmd phase.\n" );
        }
        else
        {       /* Message processed - where do we go from here?    */

            if ( !gActiveCommand )                  /* if Rejected, */
                return;                             /* return       */

            phase = g.shadow.mesh.busStatus0 & kMeshPhaseMask;
            switch ( phase )
            {
            case kBusPhaseSTS:
                [ self RunDBDMA : kcclCmdoStage     stageLabel : kcclStageInit ];
                break;

            case kBusPhaseMSGO:
                msgOutPtr = (UInt8*)CCLAddress( kcclMSGOdata );
                [ self SetupMsgO ];
                [ self RunDBDMA : kcclMsgoStage     stageLabel : kcclStageInit ];
                break;

            case kBusPhaseCMD:
                [ self RunDBDMA : kcclCmdoStage     stageLabel : kcclStageInit ];
                break;
            }
        }
    }
    else if ( phase == kBusPhaseSTS )           /* Probably Check Condition    */
    {                                           /* Perhaps block # invalid     */
        gFlagIncompleteDBDMA = FALSE;           /* indicate no-more-data       */
        [ self RunDBDMA : kcclGetStatus         stageLabel : kcclStageStat ];
    }
    else
    {
        PAUSE( 0, phase, 'Phs?', "DoInterruptStageCmdO - error during Command phase.\n" );
    }
    return;
}/* end DoInterruptStageCmdO */


    /* We are in MSGI phase. Read the bytes. Return TRUE if an entire       */
    /* message was read (we may still be in MSGI phase). Note that this     */
    /* is done by programmed IO, which will fail (logging the error) if     */
    /* the target sets MSGI but does not send us a message quickly enough.  */
    /* This method is called from the normal data transfer interrupt when   */
    /* the target enters message in phase, and from the reselection         */
    /* interrupt handler when we read a valid reselection target ID.        */
    /* Note that MESH interrupts are disabled on exit.                      */

- (IOReturn) DoMessageInPhase
{
    register UInt8      messageByte;
    UInt32              index = 0;
    IOReturn            ioReturn = IO_R_SUCCESS;


        /* We do not necessarily have a valid command in this method.   */
        /* While we're processing Message-In bytes, we don't want any   */
        /* MESH hardware interrupts.                                    */

    [ self SetIntMask   : 0 ];          /* no MESH interrupt latching   */
    [ self SetSeqReg    : kMeshFlushFIFO ];         /* Flush the FIFO   */

    gMsgInCount = 0;
    gMsgInState = kMsgInInit;

    while ( gMsgInState != kMsgInReady  /* Disconnect makes gActiveCommand  */
         && ioReturn == IO_R_SUCCESS )  /* go away                          */
    {
        meshAddr->transferCount1    = 0;
        meshAddr->transferCount0    = 1;                /* get single byte  */
        [ self SetSeqReg : kMeshMessageInCmd ];         /* issue MsgIn      */

        ioReturn = [ self WaitForMesh : TRUE ];         /* wait for cmdDone */
        if ( ioReturn != IO_R_SUCCESS )
        {
            PAUSE( gCurrentTarget, ioReturn, 'Mgi-', "DoMessageInPhase - Target hung: message in timeout.\n" );
            break;           /* Bus reset here? */
        }

        if ( (g.shadow.mesh.exception  & kMeshExcPhaseMM)
         ||  (g.shadow.mesh.busStatus0 & kMeshPhaseMask) != kBusPhaseMSGI )
        {
            break;                  /* exit loop if no longer in Msg-In phase   */
        }

        if ( g.shadow.mesh.FIFOCount == 0 )
        {
            PAUSE( gCurrentTarget, 0, 'mgi-', "DoMessageInPhase - no message byte.\n" );
            break;
        }

        messageByte = meshAddr->xFIFO;

        ASSERT( index < 256 );
        gMsgInBuffer[ index++ ] = messageByte;

        switch ( gMsgInState )
        {
        case kMsgInInit:
                /* This is the first message byte. Check for 1-byte codes.  */
            if ( messageByte == kScsiMsgCmdComplete
             || (messageByte >= kScsiMsgOneByteMin && messageByte <= kScsiMsgOneByteMax)
             ||  messageByte >= kScsiMsgIdentify )
            {
                gMsgInState = kMsgInReady;
            }
            else if ( messageByte >= kScsiMsgTwoByteMin
                  &&  messageByte <= kScsiMsgTwoByteMax )
            {
                    /* This is a two-byte message.              */
                    /* Set the count and read the next byte.    */

                gMsgInState = kMsgInReading;       /* Need one more */
                gMsgInCount = 1;
            }
            else
            {       /* This is an extended message. */
                    /* The next byte has the count. */
                gMsgInState = kMsgInCounting;
            }
            break;

        case kMsgInCounting:        /* Count byte of multi-byte message:   */
            gMsgInCount = messageByte;
            gMsgInState = kMsgInReading;
            break;

        case kMsgInReading:                 /* Body of multi-byte message:  */
            if ( --gMsgInCount <= 0 )
                gMsgInState = kMsgInReady;
            break;

        default:
            ASSERT( gMsgInState );                  /* Bogus state  */
            PAUSE( 0, 0, 'Msg-', "DoMessageInPhase  - Bogus MSGI state!\n" );
            gMsgInState = kMsgInReady;
            break;
        }/* end SWITCH on MSGI state */

        if ( gMsgInState == kMsgInReady )
        {
            [ self ProcessMSGI ];
            gMsgInState = kMsgInInit;
            index = 0;
            if ( gMsgInBuffer[0] == kScsiMsgDisconnect )
                ioReturn = IO_R_IO;     /* break out of WHILE loop  */

            if ( gMsgInFlag & kFlagMsgIn_Reject )
            {
                [ self AbortActiveCommand ];
                break;
            }

            if ( gFlagReselecting )
                break;  /* Take Identify only - leave +ACK  */
        }/* end IF have a complete message-in to process */
    }/* end WHILE there are more message bytes */

        /***** If the target switches out of MSGI phase without *****/
        /***** sending a complete message, we should do some    *****/
        /***** sort of error recovery.                          *****/

    if ( gMsgInState != kMsgInInit )
    {
        PAUSE( gCurrentTarget, gMsgInState, 'MGI-', "DoMessageInPhase - incomplete message.\n" );
        if ( ioReturn == IO_R_SUCCESS )
             ioReturn  = IO_R_IO;       /* General IO error     */
    }

    return  ioReturn;
}/* end DoMessageInPhase */


    /* ProcessMSGI - DoMessageInPhase has read a complete message.  */
    /* Process it (this will probably change our internal state).   */

- (void) ProcessMSGI
{
        /* Note that, during reselection, we may not have           */
        /* a current target or LUN, nor possibly a valid command    */

    register CommandBuffer  *cmdBuf;
    register IOSCSIRequest  *scsiReq;

    UInt8       sdtr;
    UInt8       currentTarget, currentLUN;
    UInt8       period, offset;
    UInt8       targetResponse;     /* responding or requesting?   */


    cmdBuf  = gActiveCommand;                       /* May be NULL      */
    scsiReq = (cmdBuf == NULL) ? NULL : cmdBuf->scsiReq;
    if ( scsiReq )
    {   currentTarget   = scsiReq->target;
        currentLUN      = scsiReq->lun;
        ASSERT( currentTarget == gCurrentTarget && currentLUN == gCurrentLUN );
    }
    else
    {   currentTarget   = gCurrentTarget;
        currentLUN      = gCurrentLUN;
    }

    ELG( 0, *(UInt32*)gMsgInBuffer, '<Msg', "ProcessMSGI" );

    switch ( gMsgInBuffer[0] )
    {
    case kScsiMsgCmdComplete:
        if ( cmdBuf )
        {
                /* This command is complete. Clear interrupts and   */
                /* allow subsequent MESH interrupts. Then tell the  */
                /* MESH to wait for the target to release the bus.  */

            [ self SetSeqReg : kMeshEnableReselect ];
            [ self SetSeqReg : kMeshBusFreeCmd ];    /* cause Int   */

            if ( cmdBuf->flagIsAutosense == FALSE )
            {
                if ( scsiReq )
                    scsiReq->scsiStatus = CCLByte( kcclStatusData );

                    /* Driver Kit does not return the command-complete byte.    */

            }
            [ self ioComplete : cmdBuf ];
        }
        goto exit; /* Don't exit through the SWITCH end */

    case kScsiMsgLinkedCmdComplete:
    case kScsiMsgLinkedCmdCompleteFlag:
        PAUSE( gCurrentTarget, 0, 'pmi-', "ProcessMSGI - linked command complete not supported.\n" );
        [ self AbortActiveCommand ];
        break;

    case kScsiMsgNop:
        break;

    case kScsiMsgRestorePointers:
        if ( cmdBuf )
        {
            cmdBuf->currentDataIndex = cmdBuf->savedDataIndex;
            if (  cmdBuf->mem )
                [ cmdBuf->mem setState : &cmdBuf->savedDataState ];
        }
        break;

    case kScsiMsgSaveDataPointers:
        if ( cmdBuf )
        {
            cmdBuf->savedDataIndex = cmdBuf->currentDataIndex;
            if (  cmdBuf->mem )
                [ cmdBuf->mem state : &cmdBuf->savedDataState ];
        }
        break;

    case kScsiMsgDisconnect:
            /* Driver Kit does not support automatic Save Data Pointers on      */
            /* Disconnect.                                                      */
            /* Move this request to the disconnect queue, enable reselection,   */
            /* re-enable MESH interrupts, and wait (here) for bus free, but     */
            /* don't eat the interrupt.                                         */

        gMsgInFlag |= kFlagMsgIn_Disconnect;
        [ self disconnect ];                            /* requeue active   */
        [ self SetSeqReg    : kMeshEnableReselect ];    /* enable reselect  */
        [ self SetIntMask   : kMeshIntrMask ];          /* Enable Ints      */
        [ self SetSeqReg    : kMeshBusFreeCmd ];        /* issue BusFree    */

            /* wait for Bus Free command to complete:    */

        [ self WaitForMesh : FALSE ];       /* don't clear possible reselect    */

            /* Interrupt for bus-free now latched. Prevent a double interrupt,  */
            /* 1 from bus-free + 1 from reselect from occurring.                */
            /* This fixes the following BADNESS:                                */
            /*      Issue bus-free for disconnect.                              */
            /*      Interrupt occurs in microseconds - even before exiting      */
            /*      "interruptOccurred" routine.                                */
            /*      Mach queues message to driverKit.                           */
            /*      Exit "interruptOccurred" routine.                           */
            /*      DriverKit dequeues and starts handling 1st Mach message.    */
            /*      Interrupt occurs for reselect while driverKit running.      */
            /*      Mach queues 2nd message to driverKit.                       */
            /*      DriverKit invokes MESH driver for 1st msg.                  */
            /*      MESH driver sees cmdDone fm bus-free AND reselect exception.*/
            /*      MESH driver handles reselect by setting up and running      */
            /*      DBDMA. MESH driver exits.                                   */
            /*      DriverKit invokes MESH driver with 2nd Mach message.        */
            /*      MESH driver handles this as a DBDMA completion and royally  */
            /*      screws up.                                                  */

        g.intLevel |= kLevelLatched;            /* set latched-interrupt flag   */
        [ self SetIntMask : 0 ];                /* prevent multiple MESH ints   */
        break;

    case kScsiMsgRejectMsg:
        ELG( currentTarget, gMsgOutFlag, 'Rej-', "ProcessMSGI - Reject." );
        gMsgInFlag |= kFlagMsgIn_Reject;
        break;

    case kScsiMsgSimpleQueueTag:
        msgInTagType   = gMsgInBuffer[0];
        msgInTag       = gMsgInBuffer[1];
        ELG( 0, msgInTag, '=Tag', "Simple Queue Tag" );
        break;

    case kScsiMsgExtended:

            /* Multi-byte message, presumably Synchronous Negotiation:  */

        switch ( gMsgInBuffer[ 2 ] )    /* switch on the msg code byte  */
        {
        case kScsiMsgSyncXferReq:       /* handle sync negotiation:     */
            if ( scsiReq == NULL )      // ??? can this happen?
            {
                PAUSE( currentTarget, 0, 'pMI-', "ProcessMSGI - attempted to negotiate SDTR without a nexus.\n" );
                [ self AbortActiveCommand ];
            }
            else
            {       /* Get period in  nanoseconds  */
                period = gMsgInBuffer[ 3 ] * 4; /* SCSI uses 4ns granularity    */

                    /* determine target responding or initiating?   */
                targetResponse = gPerTargetData[ scsiReq->target ].negotiateSDTR;
                gPerTargetData[ scsiReq->target ].negotiateSDTR = 0;
                if ( targetResponse )
                {   
                    if ( gMsgInBuffer[ 4 ] == 0 )   /* check offset                 */
                    {
                        sdtr = kSyncParmsAsync;     /* Offset == 0 implies async    */
                    }
                    else                            /* synchronous:                 */
                    {
                        if ( period == 100 )        /* special-case 100=FAST    */
                        {
                            sdtr = kSyncParmsFast & 0x0F;
                        }
                        else    /* Older CD-ROMs get here.                      */
                        {       /* The MESH manual says:                        */
                                /* period = 4 * clk + 2 * clk * P               */
                                /* where:                                       */
                                /*      period is the target nanoseconds        */
                                /*      clk is the MESH clock rate which is     */
                                /*          20 nanoseconds for a 50 MHz clock   */
                                /*      P is the 1-nibble period code we stuff  */
                                /*          in the syncParms register           */
                                /* So:                                          */
                                /*      period = 4 * 20 + 2 * 20 * P            */
                                /*      period = 80 + 40 * P                    */
                                /*      P = (period - 80) / 40                  */
                                /* Since P must round up for safety:            */
                                /*      P = ((period - 80) + 39) / 40           */
                                /*      P = (period - 41) / 40                  */
                                /* A value of P == 3 results in 5 MB/s          */
                            sdtr = (UInt8)((period - 41) / 40);
                        }
#ifdef CRAP
                                /* If period is longer than 200 ns resulting    */
                                /* in less than 5 MB/s, renegotiate async later.*/
                        if ( period >= 200 )
                            gPerTargetData[ scsiReq->target ].negotiateSDTR = kSyncParmsAsync;
#endif /* CRAP */
                    }/* end ELSE have offset ergo Synchronous */

                        /*  OR in the offset.   */
                    sdtr |= (gMsgInBuffer[ 4 ] << 4);
                }/* end IF Target is responding to negotiation */

                else                    /* target is initiating negotiation:    */
                {
                    msgOutPtr = (UInt8*)CCLAddress( kcclMSGOdata );
                    *msgOutPtr++   = kScsiMsgExtended;     /* 0x01 Ext Msg     */
                    *msgOutPtr++   = 0x03;                 /* 0x03 Message Len */
                    *msgOutPtr++   = kScsiMsgSyncXferReq;  /* 0x01 SDTR code   */
                    offset = gMsgInBuffer[ 4 ];
                    if ( offset == 0 )              /* Offset == 0 means async: */
                    {
                        *msgOutPtr++ = 0;          /* clear period byte        */
                        *msgOutPtr++ = 0;          /* offset byte = 0 for async*/
                        sdtr = kSyncParmsAsync;     /* set value for MESH reg   */
                    }
                    else                            /* have offset ergo sync:   */
                    {
                        if ( offset > 15 )
                             offset = 15;           /* MESH can only handle 15  */
                        
                        if ( period <= 100 )        /* special-case 100=FAST    */
                             period  = 100;
                        else
                        {       /* round up to MESH's 40 ns granularity */
                            period = ((period + 39) / 40) * 40;
                        }
                        *msgOutPtr++ = period / 4; /* SCSI 4ns granularity     */
                        *msgOutPtr++ = offset;
                        sdtr = (offset << 8) | (UInt8)((period - 41) / 40);
                    }/* end target is negotiating Sync */
                        /* respond to target:  */
                    [ self RunDBDMA : kcclMsgoStage stageLabel : kcclStageInit ];
                    gPerTargetData[ scsiReq->target ].negotiateSDTR = 0;
                }/* end ELSE target is initiating negotiation */

                meshAddr->syncParms = sdtr;
                SynchronizeIO();
                gPerTargetData[ scsiReq->target ].syncParms = sdtr;
                ELG( *(UInt32*)&gMsgInBuffer[0], gMsgInBuffer[4]<<24 | sdtr, 'SDTR', "ProcessMSGI - SDTR" );
            } /* end ELSE have a nexus */
            break;

        default:
            PAUSE( currentTarget, gMsgInBuffer[0], 'PMi-', "ProcessMSGI - unsupported extended message.\n" );
            [ self AbortActiveCommand ];
            break;
        }/* end SWITCH on extended message code */
        break;

    default:
        if ( gMsgInBuffer[0] >= kScsiMsgIdentify )
        {
            ASSERT( gCurrentTarget != kInvalidTarget );
            ASSERT( gCurrentLUN == kInvalidLUN );
            gCurrentLUN = gMsgInBuffer[0] & kScsiMsgIdentifyLUNMask;
            currentLUN  = gCurrentLUN;
        }
        else
        {
            PAUSE( currentTarget, gMsgInBuffer[0], 'mi -', "ProcessMSGI - unsupported message: rejected.\n" );
            [ self AbortActiveCommand ];
        }
    }/* end SWITCH on message selection */

exit:
    return;
}/* end ProcessMSGI */


    /* Process a reselection interrupt. */

- (void) HandleReselectionInterrupt
{
    IOReturn    ioReturn;


    ASSERT( gActiveCommand == NULL );

    gFlagReselecting = TRUE;

        /* Sometimes MESH gives a bogus Disconnected error during Reselection.  */
        /* 31mar98 - Issuing an Abort message, causes "unexpected disconnect".  */
        /* When Err:UnexpDisc and Exc:Resel are simultaneously set, the         */
        /* busStatus0,1 registers may not be current.                           */
    if ( g.shadow.mesh.error & kMeshErrDisconnected )
    {
        [ self SetSeqReg    : kMeshBusFreeCmd ];
        [ self WaitForMesh  : TRUE ];    // now maybe busStatus0,1 are live
        PAUSE( 0, 0, 'Dsc-',
                "HandleReselectionInterrupt: Caught disconnected glitch\n" );
    }/* End IF bus disconnect error */

        /* Read the target ID (which should be our initiator ID OR'd with the       */
        /* Target and the Identify byte with the reselecting LUN. Store this        */
        /* in gTargetID and gTargetLUN. Note that, during reselection, we will      */
        /* have a NULL gCurrentCommand and a valid gCurrentTarget and gCurrentLUN.  */
        /* If we get a valid reselection target, call the message in phase          */
        /* directly to read the LUN byte.                                           */
        /* @return TRUE if successful.                                              */

    msgInTag = 0;
    if ( meshAddr->FIFOCount == 0 )
    {
        PAUSE( 0, 0, 'HRI-', "HandleReselectionInterrupt - Empty FIFO in reselection.\n" );
        return;
    }
    else    /* get the Target ID bit from the bus out of the FIFO   */
    {       /* then, get the msg-in Identify byte for the LUN.      */
        if ( [ self getReselectionTargetID ] )
        {
            if ( [ self DoMessageInPhase ] != IO_R_SUCCESS )   /* get Identify  */
            {
                PAUSE( 0, 0, 'Id -', "HandleReselectionInterrupt - Expected Identify byte after reselection.\n" );
            }
        }
        else return;
    }

        /* Try to find an untagged command for this Target/LUN: */

    ioReturn = [ self reselectNexus : gCurrentTarget
                        lun         : gCurrentLUN
                        queueTag    : 0 ];

    if ( ioReturn != IO_R_SUCCESS )
    {       /* No untagged command, try to get a Tag. Hope that */
            /* you're still in Message-In phase at this point.  */
        if ( [ self DoMessageInPhase ] != IO_R_SUCCESS )   /* get Tag msg  */
        {
            PAUSE( 0, 0, 'tag-', "HandleReselectionInterrupt - Expected tag message.\n" );
        }
        ioReturn = [ self reselectNexus : gCurrentTarget
                            lun         : gCurrentLUN
                            queueTag    : msgInTag ];
    }

    if ( ioReturn == IO_R_SUCCESS )
    {
        ELG(    gActiveCommand,
                (msgInTag<<16) | (gCurrentLUN<<8) | gCurrentTarget,
                'Resl',     "HandleReselectionInterrupt" );
            /* If reselectNexus succeeded, gActiveCommand is set to the command.*/
            /* Clear out the channel command results and build the channel      */
            /* command to continue operation. The TRUE flag prevents            */
            /* constructing an arbitrate/select/command sequence.               */

        [ self ClearCPResults ];
        [ self UpdateCP : TRUE ];
        [ self RunDBDMA : kcclReselect  stageLabel : kcclStageInit ];
    }
    else
    {       /* There is no associated command.              */
            /* Reject the reselection attempt.              */
            /* This should cycle back to selectNextRequest. */
        PAUSE( gCurrentTarget, msgInTag, 'Rsl-',
            "HandleReselectionInterrupt - No command for reselection attempt.\n" );
        [ self AbortActiveCommand ];
    }
    return;
}/* end HandleReselectionInterrupt */


    /* Validate the target's reselection byte (put on the bus before    */
    /* reselecting us). Erase the initiator ID and convert the other    */
    /* bit into an index. The algorithm should be faster than a         */
    /* sequential search, but it probably doesn't matter much.          */
    /* @return  TRUE if successful (gCurrentTarget is now valid).       */
    /*          This function does not check whether there actually     */
    /*          is a command pending for this target.                   */

- (Boolean) getReselectionTargetID
{
    Boolean             success     = FALSE;
    register UInt8      targetID    = 0;
    register UInt8      bitValue    = 0;        /* Suppress warning         */
    register UInt8      targetBits;


    targetBits  = meshAddr->xFIFO;              /***** Read the FIFO    *****/
    targetBits &= ~gInitiatorIDMask;            /* Remove our bit           */
    if ( targetBits )
    {                       /* Is there another bit?    */
        bitValue        = targetBits;
        if ( bitValue > 0x0F )
        {
            targetID    += 4;
            bitValue    >>= 4;
        }
        if ( bitValue > 0x03 )
        {
            targetID    += 2;
            bitValue    >>= 2;
        }
        if ( bitValue > 0x01 )
        {
            targetID    += 1;
        }
        targetBits      &= ~(1 << targetID);    /* Remove the target mask   */
        if ( targetBits == 0 )
        {                                       /* Was exactly one set?     */
            success = TRUE;                     /* Yes: success!            */
            gCurrentTarget = targetID;          /* Save the current target  */
        }
    }

    if ( !success )
        PAUSE( targetID, targetBits, 'rsl-', "getReselectionTargetID - Expected Identify byte after reselection.\n" );

    return  success;
}/* end getReselectionTargetID */


@end /* AppleMesh_SCSI(MeshInterrupt) */


@implementation AppleMesh_SCSI( Mesh )

    /* Reusable hardware initializer function. if resetSCSIBus is TRUE,     */
    /* this includes a SCSI reset. Handling of ioComplete of active and     */
    /* disconnected commands must be done elsewhere. Returns IO_R_SUCCESS.  */

- (IOReturn) ResetMESH : (Boolean)resetSCSIBus
{
    IOReturn    ioReturn = IO_R_SUCCESS;
    UInt8       defaultSelectionTimeout = 25;   // mlj ??? fix this value
    UInt8       target;


        /* Reset interrupts, the MESH Hardware Bus Adapter, and the DMA engine. */

//  [ self SetIntMask           : 0 ];          /* ResetMESH clrs interruptMask */
    [ self SetSeqReg            : kMeshResetMESH ]; /* completes quickly        */
    [ self GetHBARegsAndClear   : TRUE ];       /* clear cmdDone                */

    dbdma_reset( DBDMA_MESH_SCSI );

        /* Init state variables:    */

    gFlagIncompleteDBDMA    = FALSE;
//  gBusState               = SCS_DISCONNECTED;

        /* Smash all active command state (just in case):   */

    gActiveCommand  = NULL;
    gCurrentTarget  = kInvalidTarget;
    gCurrentLUN     = kInvalidLUN;
    gMsgInState     = kMsgInInit;
    msgOutPtr       = (UInt8*)CCLAddress( kcclMSGOdata );

    if ( resetSCSIBus )
    {
        ASSERT( gInterruptNestingLevel > 0 );
        meshAddr->busStatus1 = kMeshRst; /***** ASSERT RESET SIGNAL *****/
        SynchronizeIO();
        IODelay( 25 );                   /* leave asserted for 25 mikes */
        meshAddr->busStatus1 = 0;        /***** CLEAR  RESET SIGNAL *****/
        SynchronizeIO();

            /* Delay for 250 msec after resetting the bus.          */
            /* This serves two purposes: it gives the MESH time to  */
            /* stabilize (about 10 msec is sufficient) and gives    */
            /* some devices time to re-initialize themselves.       */

        IOSleep( APPLE_SCSI_RESET_DELAY );      /* Give Targets time to clean up */
        [ self SetSeqReg : kMeshResetMESH ];    /* clear Err condition  */
        [ self GetHBARegsAndClear : TRUE ];     /* check regs           */

        for ( target = 0; target < SCSI_NTARGETS; target++ )
        {
            gPerTargetData[ target ].syncParms      = kSyncParmsAsync;
            gPerTargetData[ target ].negotiateSDTR  = kSyncParmsFast;   // negotiate Fast
        }
    }/* end IF resetSCSIBus */

    meshAddr->selectionTimeOut = defaultSelectionTimeout;
    SynchronizeIO();

    return  ioReturn;
}/* end ResetMESH */


    /* Wait for an immediate (non-interrupting) command to complete.    */
    /* Note that it spins while waiting. It is timed to prevent a buggy */
    /* chip or target from hanging the system.                          */

- (IOReturn) WaitForMesh : (Boolean) clearInterrupts
{
    ns_time_t   startTime, endTime;
    IOReturn    ioReturn = IO_R_SUCCESS;
#if USE_ELG
    UInt8      *logp = g.evLogBufp;
#endif /* USE_ELG */
//#define WAIT_TIME    (1000000000ULL)
//#define WAIT_TIME    (3000000ULL)    // mlj - make it 3 milliseconds
//#define WAIT_TIME    19000000        // mlj - make it 19 milliseconds for ZIP
#define WAIT_TIME      250000000       // mlj - make it 250 milliseconds for SONY CD-ROM


    IOGetTimestamp( &startTime );

    for ( g.shadow.mesh.interrupt = 0; g.shadow.mesh.interrupt == 0; )
    {
#if USE_ELG
        g.evLogBufp = logp;    /* set back the log pointer */
#endif /* USE_ELG */
        [ self GetHBARegsAndClear : clearInterrupts ];

        IOGetTimestamp( &endTime );
        if ( (endTime - startTime) >= WAIT_TIME )
        {       /* It took too long! We're dead.    */
            PAUSE( 0, 0, 'WFM-', "WaitForMesh - MESH chip does not respond to command.\n" );
            ioReturn = IO_R_INTERNAL;
            break;
        }
    }/* end FOR */

    return  ioReturn;
}/* end WaitForMesh */


    /* WaitForReq - spins while waiting. It is timed to prevent a buggy */
    /* chip or target from hanging the system.                          */

- (IOReturn) WaitForReq             /* This method is currently unused. */
{
    ns_time_t   startTime, endTime;
    IOReturn    ioReturn = IO_R_SUCCESS;


    IOGetTimestamp( &startTime );

    g.shadow.mesh.busStatus0 = 0;
    while ( (g.shadow.mesh.busStatus0 & kMeshReq) == 0 )
    {
        [ self GetHBARegsAndClear : FALSE ];

        IOGetTimestamp( &endTime );
        if ( (endTime - startTime) >= 1000000000L )
        {       /* It took too long!    */
            PAUSE( endTime, startTime, 'WFR-', "WaitForReq - Target not in valid phase.\n" );
            ioReturn = IO_R_INTERNAL;
            break;
        }
        if ( (endTime - startTime) >= 1000000L
         &&  (g.shadow.mesh.busStatus0 & kMeshReq) == 0  )
        {
            IODelay( 1000 );           /* After 1 ms, start yielding time   */
        }
    }/* end WHILE REQ not set */

    return  ioReturn;
}/* end WaitForReq */


    /* Send a command to the MESH chip. This may cause an interrupt.    */

- (void) SetSeqReg : (MeshCommand) meshCommand
{
    ELG( (meshAddr->interruptMask<<16) | meshAddr->interrupt, meshCommand, '=Seq', "SetSeqReg" );

    if ( meshAddr->interruptMask & kMeshIntrCmdDone
      && meshCommand <= kMeshBusFreeCmd )
        ELG( meshAddr->interrupt, meshAddr->interruptMask, 'Trig',
                    "SetSeqReg - may trigger interrupt.\n" );

    meshAddr->sequence = (UInt8)meshCommand;    /***** DO IT    *****/
    SynchronizeIO();
    IODelay( 1 );                               /* G3 is too fast   */

    return;
}/* end SetSeqReg */


    /* MESH chip self-test. (Minimal: it could be extended.)    */
    /* @return  IO_R_SUCCESS if successful.                     */

- (IOReturn) DoHBASelfTest
{
    IOReturn    ioReturn = IO_R_SUCCESS;
    UInt8       tempByte;


    ELG( gMESHPhysAddr, meshAddr, 'MESH', "DoHBASelfTest" );
#ifdef CRAP
    if ( probe_rb( ((void*)((UInt32)gMESHPhysAddr) + kMeshMESHID) ) == 0 )
    {
        PAUSE( 0, gMESHPhysAddr, 'HBA-', "DoHBASelfTest - Invalid MESH physical address.\n" );
        ioReturn = IO_R_NO_DEVICE;
    }
#else
     ASSERT( probe_rb( ((void*)((UInt32)gMESHPhysAddr) + kMeshMESHID) ) == 0 );
#endif /* CRAP */

    if ( ioReturn == IO_R_SUCCESS )
    {
        tempByte = meshAddr->MESHID & 0x1f;
        if ( tempByte < kMeshMESHID_Value )
        {
            PAUSE( 0, tempByte, 'hba-', "DoHBASelfTest - Invalid MESH chip ID .\n" );
            ioReturn = IO_R_NO_DEVICE;
        }
    }
    return  ioReturn;
}/* end DoHBASelfTest */


    /* Start a Channel Program at the given offset  */
    /* with the specified stage label.              */

- (void) RunDBDMA : (UInt32) offset       stageLabel : (UInt32) stageLabel
{
    register UInt8      intReg;
    ns_time_t           arbEndTime, curTime;


    gMsgInFlag = 0;                                 /* clear message-in flags.  */

    CCLWord( kcclStageLabel ) = stageLabel;         /* set the stage            */

        /* Let MESH interrupt only for errors or exceptions, but not cmdDone    */
    [ self SetIntMask : (kMeshIntrException | kMeshIntrError) ];

    intReg = meshAddr->interrupt;
    switch ( intReg )
    {
    case kMeshIntrCmdDone:
        if ( !gFlagReselecting )    // ??? Don't drop ACK fm MSG-IN or Sync data flows
                /* clear any pending command interrupts (but not reselect et al)    */
            meshAddr->interrupt = kMeshIntrCmdDone;     SynchronizeIO();
        /***** fall through *****/
    case 0:
            /* This is a Go:                                            */
            /* Flush any CCL and related data to the CCL physical page  */
            /* that may still be sitting in cache:                      */
        flush_cache_v( (vm_offset_t)cclLogAddr, cclLogAddrSize );
    //ELG( *(UInt32*)0xF3000020, 0, 'G C+', "RunDBDMA." );
    //ELG( 0, *(UInt32*)0xF300002C, 'G C ', "RunDBDMA." );

        if ( offset == kcclStart )
        {
            gFlagReselecting = FALSE;
            [ self SetSeqReg : kMeshArbitrateCmd ];     /* ARBITRATE                */
    
                /* wait 50 mikes or cmdDone, whichever comes first: */
    
            IOGetTimestamp( &arbEndTime );
            arbEndTime += 50000;
            do
            {
                [ self GetHBARegsAndClear : FALSE ];        /* get regs without hosing  */
                IOGetTimestamp( &curTime );
            }while ( !(g.shadow.mesh.interrupt & kMeshIntrCmdDone) && curTime < arbEndTime );
    
            if ( g.shadow.mesh.interrupt == kMeshIntrCmdDone )
            {       /* No err, no exc: Arbitration won: */
                meshAddr->interrupt = kMeshIntrCmdDone;
                SynchronizeIO();
                [ self SetSeqReg : kMeshDisableReselect ];  /* disable reselect */
                offset = 0x150;             // ??? fix this. Point to Select/Atn
                CCLWord( kcclStageLabel ) = kcclStageArb;   /* set stage to Arbitrate  */
            }/* end IF won Arbitration */
            else    /* Arbitration not won - CAUTION - HACK AHEAD:              */
            {       /* Sometimes, MESH does not return ArbLost as it says in    */
                    /* the documentation. Instead, it waits for the winner to   */
                    /* get off the bus (usually after the 250 ms timeout) and   */
                    /* then MESH continues its arbitration. This wastes 250 ms  */
                    /* of valuable bus time. Further, IOmega's Zip drive has a  */
                    /* nasty bug whereby if its reselection is snubbed and it   */
                    /* times out, it leaves the I/O signal asserted on the bus  */
                    /* even as other activity on the bus unrelated to the Zip   */
                    /* is ongoing.                                              */
                    /* We don't need to hack if ArbLost is indicated correctly  */
                    /* or Reselect is indicated. If either is true, don't bother*/
                    /* starting the DBDMA; rather, let the interrupt already    */
                    /* latched handle the situation.                            */

                if ( !(g.shadow.mesh.exception & (kMeshExcArbLost | kMeshExcResel)) )
                {
                    ELG( '****', '****', 'HACK', "RunDBDMA - Arbitrate HACK." );
                    [ self SetSeqReg : kMeshResetMESH ];        /* hack it: whack it*/
                    [ self GetHBARegsAndClear : TRUE ];         /* get regs/preserve*/
                    [ self SetSeqReg : kMeshEnableReselect ];   /* Let reselect again*/
                    [ self GetHBARegsAndClear : FALSE ];        /* get regs/preserve*/
                    if ( g.shadow.mesh.interrupt == 0 )
                        PAUSE( 0, 0, 'Arb*', "RunDBDMA - Arbitrate/Reselect problem." );
                }
                if ( g.shadow.mesh.interrupt )      /* If Err or Exc set:           */
                {   g.intLevel |= kLevelLatched;    /* set latched-interrupt flag.  */
                    return;                         /* let pending Int clean up.    */
                }
            }/* end ELSE lost Arbitration */
        }/* end IF DBDMA to start at Arbitrate */

        [ self GetHBARegsAndClear : FALSE ];    // ??? debug: see if ACK still set
        ELG( 0, offset<<16 | stageLabel, 'DMA+', "RunDBDMA" );
        dbdma_start( DBDMA_MESH_SCSI, (dbdma_command_t*)((UInt32)cclPhysAddr + offset) );
        break;

    default:                            /* Err or Exc or both are set   */
        ELG( 'Err ', 'Exc ', 'Pnd-', "RunDBDMA - interrupt probably pending (reselect?)." );
        [ self GetHBARegsAndClear : FALSE ];    // display without hosing
    }/* end SWITCH on interrupt register */
    return;
}/* end RunDBDMA */


    /* Retrieve the MESH volatile register contents,        */
    /* storing them in the global register shadow.          */
    /* @param   clearInts   YES to clear MESH interrupts.   */

- (void) GetHBARegsAndClear : (Boolean) clearInts
{
    register MeshRegister   *mesh = meshAddr;


    g.shadow.mesh.interrupt         = mesh->interrupt;
    g.shadow.mesh.error             = mesh->error;
    g.shadow.mesh.exception         = mesh->exception;
    g.shadow.mesh.FIFOCount         = mesh->FIFOCount;

    g.shadow.mesh.busStatus0        = mesh->busStatus0;
    g.shadow.mesh.busStatus1        = mesh->busStatus1;
    g.shadow.mesh.transferCount1    = mesh->transferCount1;
    g.shadow.mesh.transferCount0    = mesh->transferCount0;

    g.shadow.mesh.sequence          = mesh->sequence;           // debugging
    g.shadow.mesh.interruptMask     = mesh->interruptMask;      // debugging
    g.shadow.mesh.syncParms         = mesh->syncParms;          // debugging
    g.shadow.mesh.destinationID     = mesh->destinationID;      // debugging

    ELG( g.shadow.longWord[ 0 ], g.shadow.longWord[ 1 ], clearInts ? 'Regs' : 'regs', "GetHBARegsAndClear." );

    if ( g.shadow.mesh.error )  // this occurs when dbdma -> Seq while reselect
        ELG( g.shadow.mesh.interruptMask, g.shadow.mesh.sequence, 'Err-',
                                "GetHBARegsAndClear - MESH error detected" );

        /* It is possible to have the Reselected bit set in the Exception   */
        /* register without an Exception bit in the interrupt register.     */
        /* This may be caused by timing window where we clear the interrupt */
        /* register with the interrupt register instead of 0x07.            */
        /* Handle this by faking an exception.                              */
        /* 04may98 - it is also possible to have PhaseMisMatch set in the   */
        /* Exception register without Exception indicated in the Interrupt  */
        /* register. This happened when a Synchronous output finished and   */
        /* the target went to Message-In phase with Save-Data-Pointer.      */


    if ( g.shadow.mesh.exception )
         g.shadow.mesh.interrupt |= kMeshIntrException;

    if ( clearInts && g.shadow.mesh.interrupt )
    {
        mesh->interrupt = g.shadow.mesh.interrupt;
        SynchronizeIO();
    }
    return;
}/* end GetHBARegsAndClear */


- (void) SetIntMask : (UInt8) mask
{
    ELG( (meshAddr->interrupt<<16) | meshAddr->interruptMask, mask, 'Mask', "SetIntMask" );
    meshAddr->interruptMask = mask;         /* enable whatever  */
    SynchronizeIO();
    return;
}/* end SetIntMask */


- (void) AbortActiveCommand
{
    IOReturn        ioReturn;


    ELG( gActiveCommand, 0, '-AB*', "AbortActiveCommand" );
    [ self GetHBARegsAndClear : TRUE ];   /* clear possible cmdDone et al  */
    [ self SetIntMask : 0 ];              /* Disable MESH interrupts       */

    gMsgInFlag = 0;                       /* clear kFlagMsgIn_Reject et al */

    meshAddr->busStatus0 = kMeshAtn;      /***** Raise ATN signal      *****/
    SynchronizeIO();

    [ self SetSeqReg : kMeshBusFreeCmd ]; /* clear ACK                     */
    [ self WaitForMesh : TRUE ];          /* wait for PhaseMM              */

    if ( (g.shadow.mesh.busStatus0 & (kMeshPhaseMask | kMeshReq))
                                   == (kBusPhaseMSGO | kMeshReq) )
    {           /* this is what we want:    */
            [ self SetSeqReg : kMeshFlushFIFO ];    /* Flush the FIFO           */
            meshAddr->transferCount0    = 1;        /* set TC low = 1           */
            meshAddr->transferCount1    = 0;
            meshAddr->busStatus0        = 0;        /***** clear ATN signal *****/
            SynchronizeIO();

                /* Issue the Message Out sending the Abort on its way.  */
                /* Note that this will cause an Unexpected-Disconnect.  */
            [ self SetSeqReg : kMeshMessageOutCmd ]; /* drop ATN signal         */
            meshAddr->xFIFO = kScsiMsgAbort;         /* put out the Abort byte  */
            ioReturn = [ self WaitForMesh : TRUE ];  /* wait for cmdDone        */
            if ( ioReturn == IO_R_SUCCESS )
            {
                [ self SetSeqReg : kMeshEnableReselect ];/* bus about to go free    */
                [ self SetIntMask : kMeshIntrMask ];     /* Enable interrupts       */
                [ self SetSeqReg : kMeshBusFreeCmd ];    /* Clr ACK & go Bus-Free   */
                g.intLevel |= kLevelLatched;             /* set latched-int flag    */
                return;
            }
    }/* end IF MSGO phase and REQ is set */
 
        /***** USE THE HAMMER - NUKE THE BUS: *****/

    ELG( 0, 0, '-AB-', "AbortActiveCommand - target refused to enter MSGO phase" );
    [ self ResetHardware : TRUE ];
    return;
}/* end AbortActiveCommand */


- (void) AbortDisconnectedCommand
{
    CommandBuffer       *cmdBuf;
    IOSCSIRequest       *scsiReq;
    UInt8               msgByte;


    if ( !queue_empty( &abortCmdQ ) )
    {
        cmdBuf    = (CommandBuffer*)queue_first( &abortCmdQ );
        scsiReq   = cmdBuf->scsiReq;
        meshAddr->destinationID = scsiReq->target;
        msgOutPtr = (UInt8*)CCLAddress( kcclMSGOdata );
        msgByte   = kScsiMsgIdentify | scsiReq->lun;
        *msgOutPtr++ = msgByte;
        if ( cmdBuf->queueTag )
        {       /* Tagged command: */
            *msgOutPtr++ = kScsiMsgSimpleQueueTag;
            *msgOutPtr++ = cmdBuf->queueTag;
            *msgOutPtr++ = kScsiMsgAbortTag;
            ELG( cmdBuf, cmdBuf->queueTag, 'AbT-', "AbortDisconnectedCommand - Tag" );
        }
        else
        {       /* Untagged command: */
            *msgOutPtr++ = kScsiMsgAbort;
            ELG( cmdBuf, 0, 'AbU-', "AbortDisconnectedCommand - Abort (untagged)" );
        }
        [ self SetupMsgO ];     /* Setup for Message Out phase. */

        [ self RunDBDMA : kcclStart  stageLabel : kcclStageInit ];
    }
    return;
}/* end AbortDisconnectedCommand */


- (void) logTimestamp : (const char*) reason
{
#if DEBUG
    /* kMaxTimestamp should be greater than twice the expected method depth     */
    /* since, if we dump the timestamp after it has wrapped around, we expect   */
    /* to lose earlier entries and, hence, the shallower method starts.         */
#ifndef kMaxTimestampStack
#define kMaxTimestampStack  64
#endif

    TimestampDataRecord stack[ kMaxTimestampStack + 1 ];    /* Allocate one extra */
    UInt32              index       = 0;
    int                 start;
    UInt32              count       = 0;
    UInt32              maxDepth    = 0;
    Boolean             wasEnabled, unused;
    char                work[ 8 ];
    struct timeval      tv;
    ns_time_t           lastEventTime;
    UInt32              elapsed;
    UInt32              sinceMethodStart;


    if ( reason )
    {
        IOLog( "%s: *** Log timestamp: %s\n",    [ self name ], reason );
    }

        /* In case something we call causes timestamping,   */
        /* we want to avoid getting into an infinite loop.  */

    wasEnabled = EnableTimestamp( FALSE );
    lastEventTime = 0;
    while ( ReadTimestamp( &stack[ index ] ) )
    {
        ++count;
        work[0] = stack[ index ].timestampTag >> 24 & 0xFF;
        work[1] = stack[ index ].timestampTag >> 16 & 0xFF;
        work[2] = stack[ index ].timestampTag >>  8 & 0xFF;
        work[3] = stack[ index ].timestampTag >>  0 & 0xFF;
        work[4] = '\0';

        elapsed         = (unsigned)stack[ index ].eventTime - lastEventTime;
        lastEventTime   = stack[ index ].eventTime;
        ns_time_to_timeval( stack[ index ].eventTime, &tv );

        switch ( work[0] )
        {
        case '+':       /* Entering a method */
            IOLog( "%s: '%s' %u.%06u %u.%03u 0.0 %d\n",
                    [ self name ],  work,
                    tv.tv_sec,      tv.tv_usec,
                    elapsed / 1000, elapsed - ((elapsed / 1000) * 1000),
                    stack[ index ].timestampValue );
            if ( index < kMaxTimestampStack )
            {   if ( ++index > maxDepth )
                    maxDepth = index;
            }
            break;

        case '=':           /* Intermediate tag: find the method start  */
        case '-':           /* End of method: find the method start     */
            sinceMethodStart = 0;
            for ( start = index - 1; start >= 0; --start )
            {
                if ( (stack[ start ].timestampTag & 0x00FFFFFF)
                        == (stack[ index ].timestampTag & 0x00FFFFFF) )
                {
                    sinceMethodStart    = (unsigned)stack[ index ].eventTime
                                        - stack[ start ].eventTime;
                    break;
                }
            }
            IOLog( "%s: '%s' %u.%06u %u.%03u %u.%03u %d\n",
                    [ self name ],  work,
                    tv.tv_sec,      tv.tv_usec,
                    elapsed / 1000, elapsed - ((elapsed / 1000) * 1000),
                    sinceMethodStart / 1000,
                    sinceMethodStart - ((sinceMethodStart / 1000) * 1000),
                    stack[ index ].timestampValue );
            if ( start >= 0 && work[0] == '-' )
                index = start;      /* Pop the stack */
            break;

        default:
            IOLog( "%s: '%s' %u.%06u %u.%03u 0.0 %d _NoNestMark_\n",
                [ self name ],
                work,
                tv.tv_sec,
                tv.tv_usec,
                elapsed / 1000,
                elapsed - ((elapsed / 1000) * 1000),
                stack[ index ].timestampValue );
            break;
        }
    }/* end WHILE */

    IOLog( "%s: *** %d timestamps, %d max method depth\n",
            [ self name ], count, maxDepth );
    unused = EnableTimestamp( wasEnabled );
#endif /* DEBUG */
    return;
}/* end logTimestamp */

@end /* AppleMesh_SCSI(Mesh) */


@implementation AppleMesh_SCSI( Private )

    /* Private chip- and architecture-independent methods.  */

    /* Pass one CommandBuffer to the IO thread; wait for completion.    */
    /* (We are called on the client's execution thread.)                */
    /* Normal completion status is in cmdBuf->scsiReq->driverStatus;    */
    /* a non-zero return from this function indicates a Mach IPC error. */
    /* This method allocates and frees cmdBuf->cmdLock.                 */

- (IOReturn) executeCmdBuf : (CommandBuffer*) cmdBuf
{
    msg_header_t    msg             = cmdMessageTemplate;
    kern_return_t   kernelReturn;
    IOReturn        ioReturn        = IO_R_SUCCESS;


    cmdBuf->flagActive  = 0;
    cmdBuf->cmdLock     = [ [ NXConditionLock alloc ] initWith : CMD_PENDING ];
    [ incomingCmdLock lock ];
    queue_enter( &incomingCmdQ, cmdBuf, CommandBuffer*, link );
    [ incomingCmdLock unlock ];
    ELG( cmdBuf, *(UInt32*)&incomingCmdQ, 'ExeC', "executeCmdBuf" );

        /* Create a Mach message and send it in order to wake up the IO thread: */

    msg.msg_remote_port = gKernelInterruptPort;
    kernelReturn        = msg_send_from_kernel( &msg, MSG_OPTION_NONE, 0 );
    if ( kernelReturn != KERN_SUCCESS )
    {
        PAUSE( 0, kernelReturn, 'exe-', "executeCmdBuf - msg_send_from_kernel() error status .\n" );
        ioReturn = IO_R_IPC_FAILURE;
    }
    else    /* Wait for IO complete:    */
    {
        [ cmdBuf->cmdLock lockWhen : CMD_COMPLETE ];
    }

    [ cmdBuf->cmdLock free ];
    return ioReturn;
}/* end executeCmdBuf */


    /* Abort all active and disconnected commands with specified status.    */
    /* No hardware action. Currently used by threadResetBus and during      */
    /* processing of a kCommandAbortRequest command.                        */

- (void) abortAllCommands : (sc_status_t)status
{
    ELG( 0, status, 'AbAl', "abortAllCommands" );
    [ incomingCmdLock lock ];

    [ self killActiveCommand : status ];

    [ self killQueue  : &abortCmdQ         finalStatus : status ];
    [ self killQueue  : &disconnectedCmdQ  finalStatus : status ];
    [ self killQueue  : &pendingCmdQ       finalStatus : status ];
    [ self killQueue  : &incomingCmdQ      finalStatus : status ];

    [ incomingCmdLock unlock ];
    return;
}/* end abortAllCommands */


    /* Abort all active and disconnected commands with status SR_IOST_RESET. */
    /* Reset hardware and SCSI bus.                                          */
    /* If there is a command in pendingCmdQ, start it up.                    */

- (void) threadResetBus : (const char*) reason
{
    [ self abortAllCommands : SR_IOST_RESET ];
    [ self ResetHardware : TRUE ];      /* Reset SCSI and chip               */
    [ self selectNextRequest ];         /* This restarts processing commands */
    return;
}/* end threadResetBus */


    /* Commence processing of the specified command. This is called by      */
    /* commandRequestOccurred when it receives a kCommandExecute message    */
    /* from IODirectDevice. There is a new SCSI request. Either start it    */
    /* now, or add it to the end of our pending request queue.              */

- (void) threadExecuteRequest : (CommandBuffer*) cmdBuf
{
    HardwareStartResult rc;


    if ( gActiveCommand || (g.intLevel & kLevelLatched) )
    {
            /* We are currently executing a request.    */

        ELG( gActiveCommand, cmdBuf, 'Busy', "threadExecuteRequest - bus busy so queue this cmd" );
        queue_enter( &pendingCmdQ, cmdBuf, CommandBuffer*, link );
    }
    else if ( [ self commandCanBeStarted : cmdBuf ] == FALSE )
    {
            /* This request can't be started right now (perhaps the */
            /* target's tagged command limit has been reached).     */

        ELG( cmdBuf, 0, 'qFul', "threadExecuteRequest - can't start cmd so queue this cmd" );
        queue_enter( &pendingCmdQ, cmdBuf, CommandBuffer*, link );
    }
    else
    {       /* Apparently, we can start this request. Call the hardware layer.  */

        rc = [ self hardwareStart : cmdBuf ];
        switch ( rc )
        {
        case kHardwareStartOK:          /* Command started correctly        */
        case kHardwareStartBusy:        /* Hardware can't start now         */
            break;
        case kHardwareStartRejected:    /* Command rejected, try another    */
            [ self selectNextRequest ]; /* Try another command              */
        }
    }
    return;
}/* end threadExecuteRequest */


    /* Called when a transaction associated with cmdBuf is complete.    */
    /* Notify waiting thread. If cmdBuf->scsiReq exists (i.e., this     */
    /* is not a reset or an abort), scsiReq->driverStatus must be valid.*/
    /* If cmdBuf is active, caller must remove from gActiveCommand.     */
    /* We decrement activeArray[][] counter if appropriate.             */

- (void) ioComplete : (CommandBuffer*) cmdBuf
{
    ns_time_t           currentTime;
    IOSCSIRequest       *scsiReq;


    ASSERT( cmdBuf );
    ELG( cmdBuf->scsiReq, cmdBuf->scsiReq->driverStatus,  ' IOC', "ioComplete" );

    if ( cmdBuf == gActiveCommand )
        [ self deactivateCmd : cmdBuf ];

    scsiReq = cmdBuf->scsiReq;
    if ( scsiReq )
    {
        IOGetTimestamp( &currentTime );
        scsiReq->totalTime          = currentTime - cmdBuf->startTime;
        scsiReq->bytesTransferred   = cmdBuf->currentDataIndex;

            /* Catch bad SCSI status now.   */

        if ( scsiReq->driverStatus == SR_IOST_GOOD )
        {
            if ( cmdBuf->flagIsAutosense )
            {
                    /* We are completing an autosense command. Don't touch      */
                    /* the request status (it should still be Check Condition). */
                    /* Queue full is a real problem.                            */

                ASSERT( scsiReq->scsiStatus == kScsiStatusCheckCondition );
                switch ( cmdBuf->autosenseStatus )
                {
                case kScsiStatusGood:
                    scsiReq->driverStatus = SR_IOST_CHKSV;
                    break;

                case kScsiStatusQueueFull:
                    if ( [ self pushbackFullTargetQueue : cmdBuf ] == SR_IOST_GOOD)
                    {
                        return;     /* We'll try this one again */
                    }
                    /* Fall through to failure */

                default:
                    scsiReq->driverStatus = SR_IOST_CHKSNV;
                    break;
                }
            }
            else    /* not AutoSense:   */
            {
                switch ( scsiReq->scsiStatus )
                {
                case kScsiStatusGood:
                    break;

                case kScsiStatusCheckCondition:

                        /***** The 386 hardware suppresses autosense for    ****/
                        /***** Test Unit Ready to avoid request sense       ****/
                        /***** when polling for removable devices. This     ****/
                        /***** should be the caller's decision.             ****/

                    ELG( 0, 0, 'Chek', "ioComplete - Check Condition" );
                    if ( gOptionAutoSenseEnable
                     && (scsiReq->ignoreChkcond == FALSE) )
                    {
                        cmdBuf->flagIsAutosense = 1;/* We're doing autosense    */
                        queue_enter_first( &pendingCmdQ, cmdBuf, CommandBuffer*, link );
                        return;
                    }
                    else
                    {       /* This command failed and we aren't doing autosense.   */
                        scsiReq->driverStatus = SR_IOST_CHKSNV;
                    }
                    break;

                case kScsiStatusQueueFull:
                    if ( [ self pushbackFullTargetQueue : cmdBuf ] == SR_IOST_GOOD)
                    {
                        return;
                    }
                    /* Huh? we weren't doing tagged queuing, fall through */

                default:
                    scsiReq->driverStatus = SR_IOST_BADST;
                    break;
                }/* end SWITCH on SCSI status */

            }/* end IF driverStatus is SR_IOST_GOOD */
        }/* end IF not autosense */
    }/* end IF have scsiReq */

    if ( cmdBuf->flagActive )
    {       /* Note that the active flag is false for non-kCommandExecute   */
            /* commands and commands aborted from pendingCmdQ.     */

        ASSERT( cmdBuf == gActiveCommand );
        [ self deactivateCmd : cmdBuf ];
    }

    [ cmdBuf->cmdLock lock ];
    [ cmdBuf->cmdLock unlockWith : YES ];

    return;
}/* end ioComplete */


    /* A target reported a full queue. Push this command back       */
    /* on the pending queue and try it again, later.                */
    /* Return SR_IOST_GOOD if successful, SR_IOST_BADST on failure. */

- (sc_status_t) pushbackFullTargetQueue : (CommandBuffer*) cmdBuf
{
    IOSCSIRequest   *scsiReq;
    int             target, lun;
    IOReturn        ioReturn;


    ASSERT( cmdBuf && cmdBuf->scsiReq );
        /* Avoid notifying client of this condition; update           */
        /* perTarget.maxQueue and place this request on pendingCmdQ. */
        /* We'll try this again when we ioComplete at least one       */
        /* command in this target's queue.                            */
        /* Note that this can execute commands out of order.          */
        /* This can be disastrous for directory commands.             */
        /* In the long run, the client (disk/tape/whatever)           */
        /* needs to tell us how to execute the command                */
        /* (in-order, out-of-order, etc.) For example,                */
        /* virtual-memory page faults can be executed                 */
        /* out of order, but directory and volume bitmap              */
        /* updates must be executed in-order to preserve              */
        /* volume integrity.                                          */
    if ( cmdBuf->queueTag == QUEUE_TAG_NONTAGGED )
    {
            /* Huh? We're not doing command queueing... */
        ioReturn = SR_IOST_BADST;
    }
    else
    {
        scsiReq     = cmdBuf->scsiReq;
        target      = scsiReq->target;
        lun         = scsiReq->lun;
        gPerTargetData[ target ].maxQueue = gActiveArray[ target ][ lun ];
        [ self pushbackCurrentRequest : cmdBuf ];
        ioReturn = SR_IOST_GOOD;
    }
    return ioReturn;
}/* end pushbackFullTargetQueue */


    /* Push this request back on the pending queue. */

- (void) pushbackCurrentRequest : (CommandBuffer*) cmdBuf
{
    ASSERT( cmdBuf );
    if ( cmdBuf->flagActive )
    {
        ASSERT( cmdBuf == gActiveCommand );
        [ self deactivateCmd : cmdBuf ];
    }
    queue_enter_first( &pendingCmdQ, cmdBuf, CommandBuffer*, link );
    return;
}/* end pushbackCurrentRequest */


    /* Kill a request that can't be continued.  */

- (void) killCurrentRequest
{
    CommandBuffer   *cmdBuf;
    IOSCSIRequest   *scsiReq;


    if ( gActiveCommand )
    {
        cmdBuf = gActiveCommand;
        ASSERT( cmdBuf->scsiReq );
        scsiReq = cmdBuf->scsiReq;

        if ( cmdBuf->flagRequestSelectOK == FALSE )
             scsiReq->driverStatus = SR_IOST_SELTO;     /* No such device   */
        else scsiReq->driverStatus = SR_IOST_HW;        /* Target went away */

        [ self deactivateCmd    : cmdBuf ];
        [ self ioComplete       : cmdBuf ];
    }
    return;
}/* end killCurrentRequest */


    /* IO associated with gActiveCommand has disconnected.  */
    /* Place it on the disconnected command queue and       */
    /* enable another transaction.                          */

- (void) disconnect
{
    ASSERT( gActiveCommand );
    queue_enter( &disconnectedCmdQ, gActiveCommand, CommandBuffer*, link );


        /* Record this time so that gActiveCommand can be billed    */
        /* for disconnect latency at reselect time.                 */

    IOGetTimestamp( &gActiveCommand->disconnectTime );
    gActiveCommand  = NULL;
    gCurrentTarget  = kInvalidTarget;
    gCurrentLUN     = kInvalidLUN;

        /* Since there is no active command, the caller */
        /* must configure the bus interface to wait for */
        /* bus free, then allow reselection.            */

    return;
}/* end disconnect */


    /* The specified target, LUN, and queueTag is trying to reselect.   */
    /* If we have a CommandBuffer for this TLQ nexus on disconnectQ,    */
    /* remove it, make it the current gActiveCommand, and return YES.   */
    /* Else return NO. A value of zero for queueTag indicates a         */
    /* nontagged command (zero is never used as the queue tag value for */
    /* a tagged command).                                               */

- (IOReturn) reselectNexus  : (UInt8) target
                lun         : (UInt8) lun
                queueTag    : (UInt8) queueTag
{
    CommandBuffer   *cmdBuf;
    IOSCSIRequest   *scsiReq;
    ns_time_t       currentTime;
    IOReturn        ioReturn = SR_IOST_BV;  /* Presume failure      */


        /* Scan the disconnected queue looking for  */
        /* a command for this nexus.                */

    ASSERT( gActiveCommand == NULL );

    cmdBuf = (CommandBuffer*)queue_first( &disconnectedCmdQ );

    while ( !queue_end( &disconnectedCmdQ, (queue_t)cmdBuf ) )
    {
        scsiReq = cmdBuf->scsiReq;
        if (scsiReq->target     == target
         && scsiReq->lun        == lun
         && cmdBuf->queueTag    == queueTag )
        {
                /* We found the correct command.    */

            queue_remove( &disconnectedCmdQ, cmdBuf, CommandBuffer*, link );
            gActiveCommand = cmdBuf;
            ASSERT( scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN );

                /* Bill this operation for latency time:    */

            IOGetTimestamp( &currentTime );
            scsiReq->latentTime += (currentTime - gActiveCommand->disconnectTime);
            ioReturn = IO_R_SUCCESS;
            break;
        }
            /* Try next element in queue.   */

        cmdBuf = (CommandBuffer*)cmdBuf->link.next;
    }/* end WHILE */

    return  ioReturn;
}/* end reselectNexus */


    /* Determine if gActiveArray[][], maxQueue, cmdQueueEnable, and a cmd's */
    /* Target and LUN show that it's OK to start processing cmdBuf.         */
    /* Returns YES if this command can be started.                          */
    /***** Here's where we can test for a frozen LUN queue.             *****/

- (Boolean) commandCanBeStarted : (CommandBuffer*) cmdBuf
{
    IOSCSIRequest   *scsiReq;
    unsigned        target;
    unsigned        lun;
    UInt8           active;
    UInt8           maxQ;
    Boolean         result;


    ASSERT( cmdBuf && cmdBuf->scsiReq );

    scsiReq     = cmdBuf->scsiReq;
    target      = scsiReq->target;
    lun         = scsiReq->lun;
    active = gActiveArray[ target ][ lun ];
    if ( active == 0 )
    {       /* No commands are active for this target, always ok.   */
        result = TRUE;
    }
    else if ( gOptionCmdQueueEnable == FALSE
          || ((gPerTargetData[ target ].inquiry_7 & 0x02) == 0) )
    {
        ELG( active, gOptionCmdQueueEnable, 'CQE-', "commandCanBeStarted - cmd q'n disabled" );
        result = FALSE; /*  q'ing is disabled for target (or disabled globally) */
    }
    else
    {
        maxQ = gPerTargetData[ target ].maxQueue;
        if ( maxQ == 0 || active < maxQ )
        {       /* If maxQ is zero, we haven't reached the target's limit.  */
                /* Otherwise, we're under the limit.                        */
                /* In both cases, we can (presumably) start this command.   */

            result = TRUE;
        }
        else
        {
            ELG( maxQ, active, 'QLm-', "commandCanBeStarted - queue limit reached." );
            result = FALSE; /* We're over the target limit. Wait on this one.   */
        }
    }
    return  result;
}/* end commandCanBeStarted */


    /* The bus has gone free. Start up a command from pendingCmdQ,         */
    /* if any, and if allowed by cmdQueueEnable and gActiveArray[][].       */
    /* This is called from the interrupt routine when it is about to exit   */
    /* (and the bus is free and there is no active command). It may also    */
    /* be called from threadExecuteRequest when the selected command        */
    /* couldn't be started.                                                 */

- (void) selectNextRequest
{
    CommandBuffer   *cmdBuf         = NULL;
    Boolean         foundRequest    = FALSE;


    if ( !queue_empty( &abortCmdQ ) )
    {
        [ self AbortDisconnectedCommand ];
        return;
    }

    if ( !queue_empty( &pendingCmdQ ) )
    {
            /* Attempt to find a CommandBuffer in pendingCmdQ  */
            /* which we are in a position to process:           */

        cmdBuf = (CommandBuffer*)queue_first( &pendingCmdQ );
        while ( !queue_end( &pendingCmdQ, (queue_entry_t)cmdBuf ) )
        {
            if ( [ self commandCanBeStarted : cmdBuf ] )
            {
                queue_remove( &pendingCmdQ, cmdBuf, CommandBuffer*, link);
                ELG( cmdBuf, cmdBuf->scsiReq->lun<<16 | cmdBuf->scsiReq->target, 'De Q', "selectNextRequest - dequeued one." );
                foundRequest = TRUE;
                break;

                    /* Note that threadExecuteRequest may call selectNextRequest    */
                    /* if the command was rejected. If so, the rejected */
                    /* command will have been returned (with an error   */
                    /* status) to its client, so there is no chance of  */
                    /* an infinite loop here.                           */
            }
            else
            {
                cmdBuf = (CommandBuffer*)queue_next( &cmdBuf->link );
            }
        }/* end WHILE */
    }/* end IF queue not empty */

    if ( foundRequest )
        [ self threadExecuteRequest : cmdBuf ];

    return;
}/* end selectNextRequest */


- (void) killActiveCommand : (sc_status_t)status
{
    ELG( gActiveCommand, status, 'KilA', "killActiveCommand" );
    if ( gActiveCommand )
    {
        gActiveCommand->scsiReq->driverStatus = status;
        [ self ioComplete : gActiveCommand ];
        gActiveCommand  = NULL;
        gCurrentTarget  = kInvalidTarget;
        gCurrentLUN     = kInvalidLUN;
    }
    return;
}/* end killActiveCommand */


    /* Called by chip level to indicate that a command  */
    /* has gone out to the hardware.                    */

- (void) activateCommand : (CommandBuffer*)cmdBuf
{
    ASSERT( gActiveCommand == NULL );

        /* This is the only place where a gActiveArray[][] counter      */
        /* is incremented (and, hence, the only place where             */
        /* cmdBuf->active is set). The only other place gActiveCommand  */
        /* is set to non-NULL is in reselectNexus:target:lun:queueTag   */
        /* (but that doesn't increment the active command counter)      */

    gActiveCommand      = cmdBuf;
    gCurrentTarget      = cmdBuf->scsiReq->target;
    gCurrentLUN         = cmdBuf->scsiReq->lun;
    gActiveArray[ gCurrentTarget ][ gCurrentLUN ]++;
    gActiveCount++;

    cmdBuf->flagActive  = TRUE;

        /* Accumulate statistics.   */

    gMaxQueueLen = MAX( gMaxQueueLen, gActiveCount );
    gQueueLenTotal += gActiveCount;
    gTotalCommands++;
    return;
}/* end activateCommand */


    /* Remove specified cmdBuf from "active" status.                    */
    /* Update activeArray, activeCount, and unschedule pending timer.   */

- (void) deactivateCmd : (CommandBuffer*)cmdBuf
{
    IOSCSIRequest   *scsiReq;
    unsigned        target, lun;


    ASSERT( cmdBuf  && cmdBuf->scsiReq );
    ASSERT( cmdBuf == gActiveCommand );     /* ??  */
    gActiveCommand      = NULL;
    gCurrentTarget      = kInvalidTarget;
    gCurrentLUN         = kInvalidLUN;
    scsiReq             = cmdBuf->scsiReq;
    target              = scsiReq->target;
    lun                 = scsiReq->lun;

    ASSERT( gActiveArray[ target ][ lun ] );
    gActiveArray[ target ][ lun ]--;
    ASSERT( gActiveCount );
    gActiveCount--;

        /* Cancel pending timeout request.                                      */
        /* Commands which timed out don't have a timer request pending anymore. */

    if ( scsiReq->driverStatus != SR_IOST_IOTO )
    {
        IOUnscheduleFunc( serviceTimeoutInterrupt, cmdBuf );
    }
    cmdBuf->flagActive = FALSE;
    return;
}/* end deactivateCmd */


    /* Kill everything in the indicated queue. Called after bus reset.  */

- (void) killQueue : (queue_head_t*)queuePtr   finalStatus : (sc_status_t)scsiStatus
{
    CommandBuffer   *cmdBuf;


    ELG( 0, queuePtr, 'KilQ', "killQueue" );
    while ( !queue_empty( queuePtr ) )
    {
        cmdBuf = (CommandBuffer*)queue_first( queuePtr );
        queue_remove( queuePtr, cmdBuf, CommandBuffer*, link );
        cmdBuf->scsiReq->driverStatus = scsiStatus;
        [ self ioComplete : cmdBuf ];
    }
    return;
}/* end killQueue */


- (void) UpdateCurrentIndex
{
    CommandBuffer   *cmdBuf     = gActiveCommand;
    IOSCSIRequest   *scsiReq    = cmdBuf->scsiReq;
    UInt32          count;                           /* DMA transfer count */
    UInt32          length = g.shadow.mesh.FIFOCount;
    UInt8           buffer[ 16 ];
    UInt32          i;

        /* Calculate the number of bytes xferred by this channel command.   */
        /* We don't trust the DBDMA residual count.                         */

    count  = CCLWord( kcclBatchSize );              /* Our transfer count   */
    if ( count == 0 )                               /* If batch is empty,   */
        return;                                     /* look at nothing else.*/
    count -= g.shadow.mesh.transferCount1 << 8;     /* MESH residual high   */
    count -= g.shadow.mesh.transferCount0;          /* MESH residual low    */
    cmdBuf->currentDataIndex += count;              /* Increment data index */
    if ( cmdBuf->mem )
        [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];
    CCLWord( kcclBatchSize ) = 0;                   /* Clear our count      */

        /* Check the FIFO, if empty, increment the current data pointer.    */
        /* If there is stuff in it, we have more work to do.                */

    if ( g.shadow.mesh.FIFOCount )                /* If data in FIFO:       */
    {
        if ( scsiReq->read == FALSE )             /* If Writing:            */
        {
            [ self SetSeqReg : kMeshFlushFIFO ];
                /* We didn't write these bytes in the FIFO - adjust index   */
            cmdBuf->currentDataIndex -= g.shadow.mesh.FIFOCount;
            if (  cmdBuf->mem )
                [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];

        }
        else    /* Must be Reading:                                     */
        {       /* On a Read with data left in the FIFO, we must copy   */
                /* the FIFO directly into the user's data buffer:       */

            ELG( cmdBuf->currentDataIndex, g.shadow.mesh.FIFOCount, 'FIFO',
                                "UpdateCurrentIndex - copy FIFO to user buffer." );
            [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];
            count = scsiReq->maxTransfer - cmdBuf->currentDataIndex;
            if ( count > length )
                 count = length;

                /* FYI - emptying the FIFO causes cmdDone to get set.   */

            for ( i = 0; i < count; i++ )
                buffer[ i ] = meshAddr->xFIFO;

            [ cmdBuf->mem writeToClient : buffer  count : count ];
            cmdBuf->currentDataIndex += count;
            [ cmdBuf->mem setPosition : cmdBuf->currentDataIndex ];
        }/* end if/ELSE must be Reading */
    }/* end IF FIFO was not empty */
    ELG( 0, cmdBuf->currentDataIndex, 'UpIx', "UpdateCurrentIndex" );
    return;
}/* end UpdateCurrentIndex */


@end   /* AppleMesh_SCSI( Private ) */
