
# Packages maker makefile

include package.make

# Packages targets

packages: yves_packages

yves_packages: appextkit_package

appextkit_package:
	$(MAKE) package PACKAGENAME=AppExtKit PACKAGECOMPONENTS="$(SYSTEM_LIBRARY_DIR)/PrivateFrameworks/AppExtKit.framework $(SYSTEM_DEVELOPER_DIR)/PrivatePalettes/AppExtKitPalette.palette"

yvesgrailstuff_package:
	$(MAKE) package PACKAGENAME=YvesGrailStuff BUILDROOT="$(YVESDIR)/GrailRoots" PACKAGECOMPONENTS="/"

