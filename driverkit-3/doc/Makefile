ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

all debug install install_only installhdrs kern:

clean:
	rm -f *~
	
installsrc: $(SRCROOT)
	cp -r -p * $(SRCROOT)

$(SRCROOT):
	$(MKDIRS) $@

