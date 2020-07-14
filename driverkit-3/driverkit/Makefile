#
# makefile for common header file directory, driverkit project.
#

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

PUBLIC_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/Headers/driverkit
LOCAL_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/PrivateHeaders/driverkit
INSTFLAGS=-c -m 644
MACHINE_SUBDIRS=ppc i386 machine

PUBLIC_HFILES=return.h interruptMsg.h driverTypes.h \
	      IOConfigTable.h IODisk.h IOLogicalDisk.h IODiskPartition.h \
	      IODevice.h kernelDiskMethods.h IONetwork.h IONetbufQueue.h \
	      generalFuncs.h debugging.h debuggingMsg.h \
	      align.h IODeviceDescription.h IODirectDevice.h IODeviceMaster.h \
	      IOEthernet.h IOTokenRing.h IODisplay.h IOFrameBufferDisplay.h \
	      IOMemoryContainer.h IOMemoryDescriptor.h \
	      IOSimpleMemoryDescriptor.h \
	      displayDefs.h IOSVGADisplay.h \
	      IOSCSIController.h scsiTypes.h\
	      eventProtocols.h IOEventSource.h \
	      devsw.h kernelDriver.h IOAudio.h scsiRequest.h \
	      NXSoundParameterTags.h 

PRIVATE_HFILES=IODiskRwDistributed.h ddmPrivate.h IODeviceParams.h \
	driverServer.h Device_ddm.h volCheck.h kernelDriverPrivate.h \
	disk_label.h diskstruct.h IODeviceKernPrivate.h IOEthernetPrivate.h \
	IODeviceDescriptionPrivate.h IODirectDevicePrivate.h \
	IOBufDevice.h kernelDiskMethodsPrivate.h \
	EventDriver.h EventInput.h KeyMap.h \
	SCSIDiskThread.h SCSIDiskPrivate.h SCSIDiskTypes.h \
	SCSIStructInlines.h SCSIDiskKern.h SCSIGenericPrivate.h \
	driverTypesPrivate.h IONetworkPrivate.h configTablePrivate.h \
	SCSIDisk.h SCSIGeneric.h \
	IODisplayPrivate.h IOFrameBufferShared.h IOVGAShared.h \
	IOVGADisplay.h IOVGADisplayPrivate.h \
	configTableKern.h userConfigServer.h IOAudioPrivate.h memcpy.h \
	IOBuffer.h IOPower.h IOProperties.h

HFILES= $(PUBLIC_HFILES) $(PRIVATE_HFILES)
SOURCEFILES= Makefile $(HFILES)

all debug kern:

install installhdrs install_only: DSTROOT $(DSTROOT)$(PUBLIC_DSTDIR)  \
		$(DSTROOT)$(LOCAL_DSTDIR)
	install ${INSTFLAGS} $(PUBLIC_HFILES) $(DSTROOT)$(PUBLIC_DSTDIR)
	install ${INSTFLAGS} *.h $(DSTROOT)$(LOCAL_DSTDIR)
#
# Now machine-dependent source.	Install all machine-specific directories.
#
#
	SUBDIRS="$(MACHINE_SUBDIRS)";		\
	for DIR in `echo $$SUBDIRS`; do		\
	    if [ -d "$$DIR" ]; then		\
		echo ================= make $@ for $$DIR =================;   \
		(cd $$DIR; ${MAKE} DSTROOT=$(DSTROOT) $@) \
			|| exit $?; 		\
	    fi;					\
	done

clean:
	rm -f *~
	
installsrc: SRCROOT $(SRCROOT)
	cp -p $(SOURCEFILES) $(SRCROOT)
	@for i in $(MACHINE_SUBDIRS);	\
	    do				\
		    echo ================= make $@ for $$i =================; \
		    (cd $$i; ${MAKE} SRCROOT=$(SRCROOT)/$$i $@) || exit $?; \
	    done	    	

$(SRCROOT) $(DSTROOT)$(PUBLIC_DSTDIR) $(DSTROOT)$(LOCAL_DSTDIR):
	$(MKDIRS) $@

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; 			\
	then 					\
		exit 0; 			\
	else 					\
		echo Must define $@; 		\
		exit 1; 			\
	fi
