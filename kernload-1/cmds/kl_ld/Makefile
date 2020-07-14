PROJECT=kl_ld

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

DSTDIR=/usr/bin
SOURCEFILES= Makefile $(PROJECT)

all:

install: $(DSTROOT)$(DSTDIR) $(PROJECT)
	install -m 755 -c $(PROJECT) $(DSTROOT)$(DSTDIR)

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

clean:

$(DSTROOT)$(DSTDIR) $(SRCDIR):
	$(MKDIRS) $@
