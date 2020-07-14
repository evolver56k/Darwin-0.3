#
# Machine-dependent Makefile template. This is only used for installhdrs and
# installsrc.
#
# Exported and Private header files are enumerated separately for installation
# purposes.
#
# .c and .m files are enumerated in the common Makefile for build purposes.
#

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

PUBLIC_HFILES= 	ioPorts.h driverTypes.h directDevice.h \
		IOEISADeviceDescription.h kernelDriver.h \
		displayRegisters.h \
		IOPCIDirectDevice.h IOPCMCIADirectDevice.h \
		IOPCIDeviceDescription.h IOPCMCIADeviceDescription.h \
		IOPCMCIATuple.h
PRIVATE_HFILES=	driverServer.h driverTypesPrivate.h \
	IOEISADeviceDescriptionPrivate.h vpCode.h IOVPCodeDisplay.h \
	IOEISADMATransferBuffer.h \
	IOPCMCIADeviceDescriptionPrivate.h IOPCMCIATuplePrivate.h \
	IOPCIDeviceDescriptionPrivate.h

PUBLIC_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/Headers/driverkit/i386
LOCAL_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/PrivateHeaders/driverkit/i386

SOURCEFILES=	$(PUBLIC_HFILES) $(PRIVATE_HFILES)

INSTFLAGS=-c -m 444

install installhdrs install_only: DSTROOT $(DSTROOT)$(PUBLIC_DSTDIR)  \
		$(DSTROOT)$(LOCAL_DSTDIR)
	install ${INSTFLAGS} $(PUBLIC_HFILES) $(DSTROOT)$(PUBLIC_DSTDIR)
	install ${INSTFLAGS} *.h $(DSTROOT)$(LOCAL_DSTDIR)
	
installsrc:	SRCROOT $(SRCROOT)
	gnutar cf - $(SOURCEFILES) *.m *.c Makefile | (cd $(SRCROOT); gnutar xpf -)

clean:
	-rm -rf *.d Makedep *~

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
