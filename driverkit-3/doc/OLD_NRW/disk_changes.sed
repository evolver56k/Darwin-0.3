# this sed script munges one file in order to bring the file into 
# conformance with the driverkit API changes made in 1/92.

#
# IODiskDevice.h
#
s/IODiskDevice.h/IODisk.h/g
s/IODiskDevice/IODisk/g
s/IODevToIdMap/IODevAndIdInfo/g
s/IO_RS_READY/IO_Ready/g
s/IO_RS_NOTREADY/IO_NotReady/g
s/IO_RS_NODISK/IO_NoDisk/g
s/IO_RS_EJECTING/IO_Ejecting/g
s/_deviceSize/_diskSize/g
s/setDeviceSize/setDiskSize/g
s/deviceSize/diskSize/g
s/setIsPhysDevice/setIsPhysical/g
s/isPhysDevice/isPhysical/g
s/ejectDisk/eject/g
s/setFormattedInt/setFormattedInternal/g
s/ formatted\]/ isFormatted]/g
s/ removable\]/ isRemovable]/g
s/readStats/addToBytesRead/g
s/writeStats/addToBytesWritten/g
s/didReadRetry/incrementReadRetries/g
s/gotReadError/incrementReadErrors/g
s/didWriteRetry/incrementWriteRetries/g
s/gotWriteError/incrementWriteErrors/g
s/didOtherRetry/incrementOtherRetries/g
s/gotOtherError/incrementOtherErrors/g
s/IO_DISK_STATISTICS/IO_DISK_STATS/g
s/IO_ISA_DISK/IO_IS_A_DISK/g
s/IO_Disk_Statistics/IODiskStats/g
s/IO_isa_Disk/IOIsADisk/g
s/IO_ISA_PHYSDEVICE/IO_IS_A_PHYSICAL_DISK/g
s/IO_isa_physDevice/IOIsAPhysicalDisk/g
s/DISK_STATS_READ_OPS/IO_Reads/g
s/DISK_STATS_BYTES_READ/IO_BytesRead/g
s/DISK_STATS_READ_TOTAL_TIME/IO_TotalReadTime/g
s/DISK_STATS_READ_LATENT_TIME/IO_LatentReadTime/g
s/DISK_STATS_READ_RETRIES/IO_ReadRetries/g
s/DISK_STATS_READ_ERRORS/IO_ReadErrors/g
s/DISK_STATS_WRITE_OPS/IO_Writes/g
s/DISK_STATS_BYTES_WRITTEN/IO_BytesWritten/g
s/DISK_STATS_WRITE_TOTAL_TIME/IO_TotalWriteTime/g
s/DISK_STATS_WRITE_LATENT_TIME/IO_LatentWriteTime/g
s/DISK_STATS_WRITE_RETRIES/IO_WriteRetries/g
s/DISK_STATS_WRITE_ERRORS/IO_WriteErrors/g
s/DISK_STATS_OTHER_RETRIES/IO_OtherRetries/g
s/DISK_STATS_OTHER_ERRORS/IO_OtherErrors/g
s/registerLogicalDisk/setLogicalDisk/g
s/unlockLogical/unlockLogicalDisks/g
s/lockLogical/lockLogicalDisks/g
s/DiskDeviceSubclass/IOPhysicalDiskMethods/g
s/getPhysParams/updatePhysicalParameters/g
s/isDiskPresent/isDiskReady/g
s/devEjectDisk/ejectPhysical/g
s/checkReady/updateReadyState/g
s/DiskDeviceRw/IODiskReadingAndWriting/g
s/diskPresent/diskBecameReady/g
s/IO_R_NOLABEL/IO_R_NO_LABEL/g
s/IO_R_NODISK/IO_R_NO_DISK/g
s/IO_R_BLK0/IO_R_NO_BLOCK_ZERO/g
s/DiskReadyState/IODiskReadyState/g
s/diskStatsIndices/IODiskStatIndices/g
s/DISK_STATS_ARRAY_SIZE/IO_DISK_STAT_ARRAY_SIZE/g
s/partId/partitionId/g
#
# DiskDeviceKern.h
#
s/DiskDeviceKern.h/kernelDiskMethods.h/g
s/DISK_UNIT/IO_DISK_UNIT/g
s/DISK_PART/IO_DISK_PART/g
s/getIdMap/devAndIdInfo/g
s/setIdMap/setDevAndIdInfo/g
s/diskBlockDev/blockDev/g
s/diskRawDev/rawDev/g
#
# IOLogicalDisk
#
s/connectToPhysDev/connectToPhysicalDisk/g
s/anyOtherOpen/isAnyOtherOpen/g
s/setDiskIsOpen/setInstanceOpen/g
s/ diskIsOpen\]/ isInstanceOpen]/g
#
# NXDisk
#
s/NXDisk.h/IODiskPartition.h/g
s/NXDisk/IODiskPartition/g
s/NXDiskPublic/IODiskPartitionExported/g
s/getLabel/readLabel/g
s/_blockDevOpen/_blockDeviceOpen/g
s/_rawDevOpen/_rawDeviceOpen/g
s/blockDevOpen /isBlockDeviceOpen/g
s/setBlockDevOpen/setBlockDeviceOpen/g
s/rawDevOpen/isRawDeviceOpen/g
s/setRawDevOpen/setRawDeviceOpen/g




