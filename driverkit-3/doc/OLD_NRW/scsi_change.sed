# this sed script munges one file in order to bring the file into 
# conformance with the SCSI API changes made in 3/93.

# SCSITypes.h
s/SCSITypes.h/scsiTypes.h/g
s/executeCdb/executeRequest/g
s/dmaDirRead/read/g
s/dmaMax/maxTransfer/g
s/ioTimeout/timeoutLength/g
s/ioStatus/driverStatus/g
s/dmaXfr/bytesTransferred/g
s/scsiReq_t/IOSCSIRequest/g
s/startAlignRead/readStart/g
s/startAlignWrite/writeStart/g
s/SCSIControllerPublic/SCSIControllerExported/g
s/scsiReset/resetSCSIBus/g
s/scToIoReturn/returnFromScStatus/g
s/maxDmaSize/maxTransfer/g
s/sc_statusValues/IOScStatusStrings/g
s/SCSISenseValues/IOSCSISenseStrings/g
s/SCSIOpcodeValues/IOSCSIOpcodeStrings/g
s/SCSI_DMA_ALIGNMENT/IO_SCSI_DMA_ALIGNMENT/g
#
# SCSIController.h
#
s/reserveArray/_reserveArray/g
s/reserveCount/_reserveCount/g
s/reserveLock/_reserveLock/g
s/SCSIInterrupt/interruptOccurred/g
s/SCSITimeout/timeoutOccurred/g
s/IO_ISA_CONTROLLER/IO_IS_A_SCSI_CONTROLLER/g
s/IO_isa_CONTROLLER/IOIsASCSIController/g
s/IO_CONTROLLER_STATISTICS/IO_SCSI_CONTROLLER_STATISTICS/g
s/IO_Controller_Statistics/IOSCSIControllerStatistics/g
s/CONTROLLER_STATS_SIZE/IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE/g
s/CONTROLLER_MAX_QUEUE_LEN/IO_SCSI_CONTROLLER_MAX_QUEUE_LENGTH/g
s/CONTROLLER_QUEUE_SAMPLES/IO_SCSI_CONTROLLER_QUEUE_SAMPLES/g
s/CONTROLLER_QUEUE_TOTAL/IO_SCSI_CONTROLLER_QUEUE_TOTAL/g
