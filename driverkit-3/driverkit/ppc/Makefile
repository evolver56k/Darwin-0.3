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

PUBLIC_HFILES= 	driverTypes.h directDevice.h \
		IOPPCDeviceDescription.h
PRIVATE_HFILES=	driverServer.h \
		driverTypesPrivate.h \
		IOPPCDeviceDescriptionPriv.h \
		IOMacOSTypes.h \
		IOPropertyTable.h \
		IOTreeDevice.h \
		IOSmartDisplay.h \
		IODeviceTreeBus.h \
		IOFramebuffer.h \
		IOPCIDevice.h \
		IODBDMA.h 

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

PUBLIC_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/Headers/driverkit/ppc
LOCAL_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/PrivateHeaders/driverkit/ppc

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
