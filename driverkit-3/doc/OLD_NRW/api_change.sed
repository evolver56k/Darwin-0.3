# this sed script munges one file in order to bring the file into 
# conformance with the driverkit API changes made in 1/92.
# Must be used in conjunction with disk_changes.sed.
#
# user_driver.h
s/user_driver.h/driverServer.h/g
s/IOGetDeviceTypeFromPort/_IOLookupByDevicePort/g
s/IOGetDeviceType/_IOLookupByDeviceNumber/g
s/IOPhysDevicePage/_IOGetPhysicalAddressOfDevicePage/g
s/IOMapSlot/_IOMapSlotSpace/g
s/IOUnmapSlot/_IOUnmapSlotSpace/g
s/IOMapBoard/_IOMapBoardSpace/g
s/IOUnmapBoard/_IOUnmapBoardSpace/g
s/IO_CC_INTR_ENABLE/IO_CC_ENABLE_INTERRUPTS/g
s/IO_CC_INTR_DISABLE/IO_CC_DISABLE_INTERRUPTS/g
s/IO_CC_LOOP_FRAME/IO_CC_CONNECT_FRAME_LOOP/g
s/IO_CC_UNLOOP_FRAME/IO_CC_DISCONNECT_FRAME_LOOP/g
s/IO_CC_DEV_INTR_ENABLE/IO_CC_DISABLE_DEVICE_INTERRUPTS/g
s/IO_CC_DEV_INTR_DISABLE/IO_CC_DISABLE_DEVICE_INTERRUPTS/g
s/IOEnqueueDma/_IOEnqueueDMA/g
s/IO_CEO_EOR/IO_CEO_END_OF_RECORD/g
s/IO_CEO_DESC_INTR/IO_CEO_DESCRIPTOR_INTERRUPT/g
s/IO_CEO_ENABLE_INTR/IO_CEO_ENABLE_INTERRUPTS/g
s/IO_CEO_ENABLE_CHAN/IO_CEO_ENABLE_CHANNEL/g
s/IO_CEO_DESC_CMD/IO_CEO_DESCRIPTOR_COMMAND/g
s/IODequeueDma/_IODequeueDMA/g
s/IO_CDO_ENABLE_INTR/IO_CDO_ENABLE_INTERRUPTS/g
s/IO_CDO_EI_IF_MT/IO_CDO_ENABLE_INTERRUPTS_IF_EMPTY/g
s/DMA_ID_NULL/IO_NULL_DMA_ID/g
s/IOInquire/_IOLookupByObjectNumber/g
s/IOLookup(/_IOLookupByDeviceName(/g
s/IOGetParameterInt/_IOGetIntValues/g
s/IOGetParameterChar/_IOGetCharValues/g
s/IOSetParameterInt/_IOSetIntValues/g
s/IOSetParameterChar/_IOSetCharValues/g
s/IO_CHAN_NONE/IO_NO_CHANNEL/g
s/IOCreateDevicePort/_IOCreateDevicePort/g
s/IODestroyDevicePort/_IODestroyDevicePort/g
s/IOAttachInterrupt/_IOAttachInterrupt/g
s/IODetachInterrupt/_IODetachInterrupt/g
s/IOAttachChannel/_IOAttachChannel/g
s/IODetachChannel/_IODetachChannel/g
s/IOMapDevicePage/_IOMapDevicePage/g
s/IOUnmapDevicePage/_IOUnmapDevicePage/g
s/IOSendChannelCommand/_IOSendChannelCommand/g
#
# user_driver_types.h
#
s/user_driver_types.h/driverTypes.h/g
s/IODeviceReturn/IOReturn/g
s/IO_DR_SUCCESS/IO_R_SUCCESS/g
s/IO_DR_ACCESS/IO_R_PRIVILEGE/g
s/IO_DR_NOTATTACHED/IO_R_NOT_ATTACHED/g
s/IO_DR_INVALID/IO_R_INVALIDARG/g
s/IO_DR_MEMORY/IO_R_PRIVILEGE/g
s/IO_DR_NOCHANNEL/IO_R_NO_CHANNELS/g
s/IO_DR_NOSPACE/IO_R_NO_SPACE/g
s/IO_DR_NODEVICES/IO_R_NO_DEVICES/g
s/IO_DR_NOPORT/IO_R_RESOURCE/g
s/IO_DR_MAXCHANNELS/IO_R_INVALIDARG/g
s/IO_DR_EXISTS/IO_R_PORT_EXISTS/g
s/IO_DR_NOINTR/IO_R_RESOURCE/g
s/IO_DR_BUSY/IO_R_BUSY/g
s/IO_DR_UNSUPPORTED/IO_R_UNSUPPORTED/g
s/IO_DR_ALIGNMENT/IO_R_ALIGN/g
s/IO_DR_CANTWIRE/IO_R_CANT_WIRE/g
s/IO_DR_NOINTERRUPT/IO_R_NO_INTERRUPT/g
s/IO_DR_RESOURCE/IO_R_RESOURCE/g
s/IO_DR_NOFRAMES/IO_R_NO_FRAMES/g
s/IO_DR_INTERNAL/IO_R_INTERNAL/g
s/IOUnitNumber/IOObjectNumber/g
s/IO_DS_NONE/IO_None/g
s/IO_DS_COMPLETE/IO_Complete/g
s/IO_DS_RUNNING/IO_Running/g
s/IO_DS_UNDERRUN/IO_Underrun/g
s/IO_DS_BUSERR/IO_BusError/g
s/IO_DS_BUFERR/IO_BufferError/g
s/IO_DMA_DIR_READ/IO_DMARead/g
s/IO_DMA_DIR_WRITE/IO_DMAWrite/g
s/DEV_CACHE_OFF/IO_CacheOff/g
s/DEV_CACHE_WRITE_THRU/IO_WriteThrough/g
s/DEV_CACHE_COPY_BACK/IO_CopyBack/g
s/DEV_TASK_NULL/IO_NULL_VM_TASK/g
s/NATIVE_SLOT_ID/IO_NATIVE_SLOT_ID/g
s/DEV_TYPE_SLOT/IO_SLOT_DEVICE_TYPE/g
s/SLOT_ID_NULL/IO_NULL_SLOT_ID/g
s/DEV_TYPE_NULL/IO_NULL_DEVICE_TYPE/g
s/DEV_INDEX_NULL/IO_NULL_DEVICE_INDEX/g
s/BOARD_SIZE_MAX/IO_MAX_BOARD_SIZE/g
s/SLOT_SIZE_NRW_MAX/IO_MAX_NRW_SLOT_SIZE/g
s/SLOT_SIZE_MAX/IO_MAX_SLOT_SIZE/g
s/IO_PARAMETER_NAME_SIZE/IO_MAX_PARAMETER_NAME_LENGTH/g
s/IO_MAX_PARAMETER_ARRAY/IO_MAX_PARAMETER_ARRAY_LENGTH/g
s/dmaStatValues/IODMAStatusStrings/g
s/rvValue/value/g
s/rvName/name/g
s/regValues/IONamedValue/g
s/IOUnitType/IOString/g
s/IOUnitName/IOString/g
s/IOCacheSpec/IOCache/g
s/UNITTYPE_LENGTH/IO_STRING_LENGTH/g
s/UNITNAME_LENGTH/IO_STRING_LENGTH/g
s/IODmaDirection/IODMADirection/g
s/IODmaStatus/IODMAStatus/g
s/IODevicePort/port_t/g
#
# i386/user_driver_types.h
#
s/EISA_io_mapping_t/IOEISAPortRange/g
s/EISA_CONF_NIOMAP/IO_NUM_EISA_PORT_RANGES/g
s/EISA_io_conf_t/IOEISAPortMap/g
s/EISA_phys_mapping_t/IOEISAMemoryRange/g
s/EISA_CONF_NPHYSMAP/IO_NUM_EISA_MEMORY_RANGES/g
s/EISA_phys_conf_t/IOEISAMemoryMap/g
s/EISA_intr_req_t/IOEISAInterrupt/g
s/EISA_CONF_NIRQ/IO_NUM_EISA_INTERRUPTS/g
s/EISA_intr_conf_t/IOEISAInterruptList/g
s/EISA_dma_req_t/IOEISADMAChannel/g
s/EISA_CONF_NDMA/IO_NUM_EISA_DMA_CHANNELS/g
s/EISA_dma_conf_t/IOEISADMAChannelList/g
#
# deviceCommon.h
#
s/deviceCommon.h/return.h/g
s/IO_R_BADMSGID/IO_R_BAD_MSG_ID/g
s/IO_R_EXCL_ACCESS/IO_R_EXCLUSIVE_ACCESS/g
s/IO_R_INVALIDARG/IO_R_INVALID_ARG/g
s/IO_R_IPCFAIL/IO_R_IPC_FAILURE/g
s/IO_R_NODEVICE/IO_R_NO_DEVICE/g
s/IO_R_NOMEM/IO_R_NO_MEMORY/g
s/IO_R_NOTOPEN/IO_R_NOT_OPEN/g
s/IO_R_NOTREADABLE/IO_R_NOT_READABLE/g
s/IO_R_NOTREADY/IO_R_NOT_READY/g
s/IO_R_NOTWRITEABLE/IO_R_NOT_WRITABLE/g
s/IO_R_VMEMFAIL/IO_R_VM_FAILURE/g
s/IO_R_NOTATTACHED/IO_R_NOT_ATTACHED/g
#
# uxpr.h
#
s/uxpr.h/debugging.h/g
s/XPR_DEBUG/DDM_DEBUG/g
s/NUM_XPR_MASKS/IO_NUM_DDM_MASKS/g
s/xprMask/IODDMMasks/g
s/xprInit(/IOInitDDM(/g
s/xprAdd(/IOAddDDMEntry(/g
s/xprClear(/IOClearDDM(/g
s/xprGetBitmask(/IOGetDDMMask(/g
s/xprSetBitmask(/IOSetDDMMask(/g
s/xprGetEntry(/IOGetDDMEntry(/g
s/xpr_string(/IOCopyString(/g
s/uxpr(/IODEBUG(/g
#
# uxprServer.h
#
s/uxprServer.h/debuggingMsg.h/g
s/SIZEOF_EXPORTED_STRING/IO_DDM_STRING_LENGTH/g
s/arg_type/argType/g
s/mask_value/maskValue/g
s/timestamp_hi/timestampHighInt/g
s/timestamp_low/timestampLowInt/g
s/cpu_num/cpuNumber/g
s/string_type/stringType/g
s/uxprMessage_t/IODDMMsg/g
s/UXPR_MSG_BASE/IO_DDM_MSG_BASE/g
s/UXPR_MSG_LOCK/IO_LOCK_DDM_MSG/g
s/UXPR_MSG_UNLOCK/IO_UNLOCK_DDM_MSG/g
s/UXPR_MSG_GETSTRING/IO_GET_DDM_ENTRY_MSG/g
s/UXPR_MSG_SETMASK/IO_SET_DDM_MASK_MSG/g
s/UXPR_MSG_CLEAR/IO_CLEAR_DDM_MSG/g
s/XPR_STATUS_GOOD/IO_DDM_SUCCESS/g
s/XPR_STATUS_NOBUF/IO_NO_DDM_BUFFER/g
s/XPR_STATUS_BADINDEX/IO_BAD_DDM_INDEX/g
s/uxpr_ns_time/IONsTimeFromDDMMsg/g
#
# libIO.h
#
s/IOlibIOInit(/IOInitGeneralFuncs(/g
s/libIO.h/generalFuncs.h/g
s/IOThreadFcn/IOThreadFunc/g
s/IOTimeout/IOScheduleFunc/g
s/IOUntimeout/IOUnscheduleFunc/g
s/IOTimeStamp/IOGetTimestamp/g
s/IOIntToString/IOFindNameForValue/g
s/IOStringToInt/IOFindValueForName/g
s/IOlibIOInit/IOInitGeneralFunc/g
s/IOTaskSelf/task_self/g
#
# interrupt_msg.h
#
s/interrupt_msg.h/interruptMsg.h/g
s/interruptMsg_t/IOInterruptMsg/g
s/INT_MSG_ID_BASE/IO_INTERRUPT_MSG_ID_BASE/g
s/INT_MSG_TIMEOUT/IO_TIMEOUT_MSG/g
s/INT_MSG_COMMAND/ IO_COMMAND_MSG/g
s/INT_MSG_DEVICE/IO_DEVICE_INTERRUPT_MSG/g
s/INT_MSG_DMA/IO_DMA_INTERRUPT_MSG/g
#
# IOAlign.h 
#
s/IOAlign.h/align.h/
s/DMA_DO_ALIGN/IOAlign/g
s/DMA_ISALIGNED/IOIsAligned/g
#
# IODevice.h
#
s/IO_Class_Name/IOClassName/g
s/IO_Device_Name/IODeviceName/g
s/IO_Device_Type/IODeviceType/g
s/IOReturnToString/stringFromReturn/g
s/IOReturnToErrno/errnoFromReturn/g
s/setDeviceType/setDeviceKind/g
s/setDeviceName/setName/g
s/ deviceName\]/ name]/g
s/ deviceType\]/ deviceKind]/g
#
# kern_driver.h
#
s/kern_driver.h/kernelDriver.h/g
s/IOKernDeviceLookup/IOGetObjectForDeviceName/g
s/IO_CC_DISABLE_UXPR/IO_CC_DISABLE_DDM/g
s/IO_CC_ENABLE_UXPR/IO_CC_ENABLE_DDM/g
s/IOEnqueueDmaInt/_IOEnqueueDMAInt/g
#
# DeviceUxpr.h
#
s/DeviceUxpr.h/Device_ddm.h/g
#
# uxprPrivate.h
#
s/uxprPrivate.h/ddmPrivate.h/g
#
# i386/user_driver.h
s/IOMapEISADevicePorts/_IOMapEISADevicePorts/g
s/IOUnMapEISADevicePorts/_IOUnMapEISADevicePorts/g
s/IOMapEISADeviceMemory/_IOMapEISADeviceMemory/g
s/IOEnableEISAInterrupt/_IOEnableEISAInterrupt/g
s/IODisableEISAInterrupt/_IODisableEISAInterrupt/g
s/IOGetEISADeviceConfig/_IOGetEISADeviceConfig/g
#
# configPublic.h
s/configPublic.h/userConfigServer.h/g
#
# architecture/i386/io.h
s/io_addr_t/IOEISAPortAddress/g
s/io_len_t/IONumEISAPorts/g



