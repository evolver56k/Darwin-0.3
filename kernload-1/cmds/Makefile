SUBDIR=	kl_log kl_util kl_ld

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

all tags clean:
	@for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} \
			$${SYMROOT+SYMROOT=$$SYMROOT} \
			$${SYMDIR+SYMDIR=$$SYMDIR/$$i} \
			$${OBJDIR+OBJDIR=$$OBJDIR/$$i} \
			$@); \
	done

install: DSTROOT
	@for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} DSTROOT=$(DSTROOT) \
			$${SYMROOT+SYMROOT=$$SYMROOT} \
			$${SYMDIR+SYMDIR=$$SYMDIR/$$i} \
			$${OBJDIR+OBJDIR=$$OBJDIR/$$i} \
			$@); \
	done

installsrc: SRCDIR ${SRCDIR}
	$(CP) -p Makefile ${SRCDIR}
	@for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} SRCDIR=$(SRCDIR)/$$i $@) \
	done

SRCDIR DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		$(ECHO) Must define $@; \
		exit 1; \
	fi

$(SRCDIR):
	$(MKDIRS) $@
