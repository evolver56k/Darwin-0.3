FULL_VERSION  = 27.2.0
MAJOR_VERSION = 27

SHLIB      = libg++.so.$(FULL_VERSION)
MSHLINK    = libg++.so.$(MAJOR_VERSION)
BUILD_LIBS = $(ARLIB) $(SHLIB) $(SHLINK) $(MSHLINK)
SHFLAGS    = -Wl,-soname,$(MSHLINK)