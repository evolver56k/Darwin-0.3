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

PUBLIC_HFILES= 	IOPCIDirectDevice.h IOPCIDeviceDescription.h
PRIVATE_HFILES=	IOPCIDeviceDescriptionPrivate.h

PUBLIC_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/Headers/driverkit/pci
LOCAL_DSTDIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/Versions/B/PrivateHeaders/driverkit/pci

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
