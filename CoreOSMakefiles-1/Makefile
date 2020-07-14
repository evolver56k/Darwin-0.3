Project=CoreOSMakefiles

CoreOSMakefiles = .

include $(CoreOSMakefiles)/ReleaseControl/Common.make

Destination = /System/Developer/Makefiles/CoreOS

install_headers::
	@$(MAKE) install_source SRCROOT=$(DSTROOT)$(Destination)
	$(_v) $(RM) -f $(DSTROOT)$(Destination)/Makefile
