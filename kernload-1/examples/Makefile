SUBDIR=	log_test simple simple_server thread panic
EXDIR=$(SYSTEM_DEVELOPER)/Examples/KernelServer

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

all:

all tags clean:
	@for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} \
			$${SYMROOT+SYMROOT=$$SYMROOT} \
			$${SYMROOT+SYMDIR=$$SYMROOT/$$i} \
			$${OBJROOT+OBJDIR=$$OBJROOT/$$i} \
			$@); \
	done

install:

installsrc:  $(SRCDIR)
	$(CP) -p Makefile ${SRCDIR}
	@for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} SRCDIR=$(SRCDIR)/$$i $@) || exit $?; \
	done

$(SRCDIR) $(DSTROOT)$(EXDIR):
	$(MKDIRS) $@
