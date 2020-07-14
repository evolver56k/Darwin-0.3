# 
# driverkit test files
#

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

OBJROOT= .
SYMROOT= .

CFILES=buflib.c ux.c netbuf.c smsg.c
#
# We'll keep around some of the driver-specific source files (like 
# scsidisk.m, floppy.m, etc.) event though we no longer compile them.
# Drivers removed from driverkit project 17-Feb-92 dmitch.)
#
MFILES=query.m read.m write.m locktest.m acctest.m wrclp.m readlp.m \
	writelp.m readq.m writeq.m eject.m glabel.m slabel.m gparms.m \
	dx.m malloctest.c testdriver.c scsidisk.m sctest.m \
	open.m fx.m floppy.m fd_lib.m ether.m netif.m NXLockTest.m \
	adbevent.m timertest.m keyboard.m klibtest.m dma.m dev_serv_test.m \
	configTable.m

SECTFILES= NXLockTestLoad.sect NXLockTestUnload.sect \
	   klibtestLoad.sect klibtestUnload.sect

HFILES=		defaults.h buflib.h fd_lib.h
BUFLIB=		buflib.o
DRIVER_LIB=	$(SYMROOT)/../libDriver/syms/libDriver_g.a
DRIVER_LIB_KERN=
DMA_STUB=	dma.o

VPATH= $(OBJROOT):$(SYMROOT)
SOURCEFILES= Makefile *.[cmh] *.sect
OFILES= $(CFILES:.c=.o) $(MFILES:.m=.o)
EXECUTABLES= \
	$(OBJROOT)/read \
	$(OBJROOT)/write \
	$(OBJROOT)/readlp \
	$(OBJROOT)/writelp \
	$(OBJROOT)/eject \
	$(OBJROOT)/glabel \
	$(OBJROOT)/slabel \
	$(OBJROOT)/gparms \
	$(OBJROOT)/ux \
	$(OBJROOT)/testdriver \
	$(OBJROOT)/smsg \
	$(OBJROOT)/timertest \
	$(OBJROOT)/dev_serv_test \
	$(OBJROOT)/configTable

#
# other deleted executables (to avoid bogus project dependencies or 
# obsolete functionality)
#
#	$(OBJROOT)/query
#	$(OBJROOT)/NXLockTest_reloc
#	$(OBJROOT)/klibtest_reloc
#	$(OBJROOT)/locktest
#	$(OBJROOT)/acctest
#	$(OBJROOT)/wrclp 
#	$(OBJROOT)/readq
#	$(OBJROOT)/writeq
#	$(OBJROOT)/dx

CFLAGS= -g -DDEBUG -DXPR_DEBUG -MD -Wall -Wnoformat $(RC_CFLAGS) -DNRW_BUILD  -I..

KERNEL_CFLAGS= -g -DDEBUG -DXPR_DEBUG -MD -Wall -Wnoformat $(RC_CFLAGS) -I.. -I/TeflonRoot$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework/PrivateHeaders -DKERNEL -DMACH_USER_API

all debug: $(OBJROOT) $(SYMROOT) $(EXECUTABLES)

install: all

installhdrs install_only tags kern:
	
installsrc: $(SRCROOT)
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -)

clean:
	-rm -f *.o *.d $(EXECUTABLES) Makedep *~

$(SRCROOT) $(OBJROOT) $(SYMROOT):
	$(MKDIRS) $@

#
# Note: this Makefile is so ugly (with a zillion $(OBJROOT) prefixes) because
# I didn't want to have a separate build directory for the executable
# tests. To avoid having VPATH find binaries in this directory when we're
# building somewhere else, we have to fully specify the path of each 
# target.
#
$(OBJROOT)/read: read.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/write:	write.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/query:	query.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/locktest: locktest.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/acctest:	acctest.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/wrclp:	wrclp.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/readlp:	readlp.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/writelp:	writelp.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/writeq:	writeq.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/readq:	readq.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/eject:	eject.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/glabel:	glabel.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/slabel:	slabel.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/gparms:	gparms.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/dx:	dx.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB} -lcs

$(OBJROOT)/ux:	ux.o
	${CC} ${CFLAGS} -o $@ $@.o

$(OBJROOT)/smsg:	smsg.o
	$(CC) $(CFLAGS) -o $@ $@.o
	
$(OBJROOT)/malloctest: malloctest.o
	${CC} ${CFLAGS} -o $@ $@.o ../Libraries/88kLib/libIO_g.a ${BUFLIB} -lcs

$(OBJROOT)/devr_0999_0000: devr_0999_0000.o $(DRIVER_LIB) 
	${CC} ${CFLAGS} -o $@ $@.o $(DRIVER_LIB) -lcs
	cp -p $(OBJROOT)/$@ /usr/Devices
	
$(OBJROOT)/testdriver: testdriver.o $(DRIVER_LIB) 
	${CC} ${CFLAGS} -o $@ $@.o $(DRIVER_LIB) -lcs
	
$(OBJROOT)/testdriverInst: testdriver
#
#	two versions of file, with two devices per driver
#
	cp -p testdriver /usr/Devices/devr_0999_0000_test
	cp -p testdriver /usr/Devices/devr_0999_0010_test
	rm -f /usr/Devices/devr_0001_0010_test
	ln /usr/Devices/devr_0999_0010_test /usr/Devices/devr_0001_0010_test

$(OBJROOT)/open:	open.o  ${DRIVER_LIB} ${BUFLIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB} ${BUFLIB}

$(OBJROOT)/fx:		fx.o ${DMA_STUB} ${DRIVER_LIB} ${BUFLIB} $(FLOPPY_LIBS)
	${CC} ${CFLAGS} -o $@ $@.o ${DMA_STUB} $(FLOPPY_LIBS) ${DRIVER_LIB} \
			${BUFLIB} -lcs

$(OBJROOT)/timertest:	timertest.o  ${DRIVER_LIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB}

$(OBJROOT)/dev_serv_test: dev_serv_test.o ${DRIVER_LIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB}

$(OBJROOT)/configTable: configTable.o ${DRIVER_LIB}
	${CC} ${CFLAGS} -o $@ $@.o ${DRIVER_LIB}

#
# Special build for kernel .o files...
#
$(OBJROOT)/NXLockTest.o:	NXLockTest.m
	${CC} ${KERNEL_CFLAGS} -c -o $@ $(*F).m
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
	
$(OBJROOT)/NXLockTest_reloc: $(OBJROOT)/NXLockTest.o NXLockTestLoad.sect \
	NXLockTestUnload.sect ${DRIVER_LIB_KERN}
	kl_ld -n NXLockTest -l NXLockTestLoad.sect -u \
		NXLockTestUnload.sect -i NXLock_ks_var  \
		-d NXLockTest_loadable -o $@ $(OBJROOT)/NXLockTest.o \
		${DRIVER_LIB_KERN}

$(OBJROOT)/klibtest.o:	klibtest.m
	${CC} ${KERNEL_CFLAGS} -c -o $@ $(*F).m
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
	
$(OBJROOT)/klibtest_reloc: $(OBJROOT)/klibtest.o klibtestLoad.sect \
	klibtestUnload.sect ${DRIVER_LIB_KERN}
	kl_ld -n klibtest -l klibtestLoad.sect -u \
		klibtestUnload.sect -i klibtest_ks_var  \
		-d klibtest_loadable -o $@ $(OBJROOT)/klibtest.o \
		${DRIVER_LIB_KERN}

.c.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).c
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
.m.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).m
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
-include Makedep
