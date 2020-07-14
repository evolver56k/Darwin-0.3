OTHER_SRCS = Makefile
NOTES = CompilerTools.rtf DR2CompilerTools.rtf Private_CompilerTools.rtf \
	FatFiles.rtf
#PPC = PPC_Instructions.rtf
INSTALL_FILES = $(OTHER_SRCS) $(NOTES) $(PPC)

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

NOTESDIR = /usr/local/RelNotes
#PPC_ASSEMBLER_DIR = /NextLibrary/Documentation/NextDev/Reference/DevTools/Assembler/Part2_ArchSpecific

install: ${DSTROOT}${NOTESDIR} ${DSTROOT}${PPC_ASSEMBLER_DIR} ${NOTES} ${PPC}
	install -c -m 444 ${IFLAGS} ${NOTES} ${DSTROOT}${NOTESDIR}
#	install -c -m 444 ${IFLAGS} ${PPC} ${DSTROOT}${PPC_ASSEMBLER_DIR}

installGNUsrc installMWsrc:

installsrc:
	$(MKDIRS) $(SRCROOT)
	chmod 755 $(SRCROOT)
	gnutar cf - $(INSTALL_FILES) | (cd $(SRCROOT); gnutar xf -)
	chmod 444 $(SRCROOT)/*

${DSTROOT}${NOTESDIR}:; $(MKDIRS) $@

${DSTROOT}${PPC_ASSEMBLER_DIR}:; $(MKDIRS) $@

depend:

clean:
shlib_clean:

all:
