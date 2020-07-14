# Microsoft Developer Studio Generated NMAKE File, Based on libntp.dsp
!IF "$(CFG)" == ""
CFG=libntp - Release
!MESSAGE No configuration specified. Defaulting to libntp - Release.
!ENDIF 

!IF "$(CFG)" != "libntp - Release" && "$(CFG)" != "libntp - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libntp.mak" CFG="libntp - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libntp - Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libntp - Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe

!IF  "$(CFG)" == "libntp - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libntp.lib" "$(OUTDIR)\libntp.bsc"

!ELSE 

ALL : "$(OUTDIR)\libntp.lib" "$(OUTDIR)\libntp.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\a_md5encrypt.obj"
	-@erase "$(INTDIR)\a_md5encrypt.sbr"
	-@erase "$(INTDIR)\atoint.obj"
	-@erase "$(INTDIR)\atoint.sbr"
	-@erase "$(INTDIR)\atolfp.obj"
	-@erase "$(INTDIR)\atolfp.sbr"
	-@erase "$(INTDIR)\atouint.obj"
	-@erase "$(INTDIR)\atouint.sbr"
	-@erase "$(INTDIR)\authencrypt.obj"
	-@erase "$(INTDIR)\authencrypt.sbr"
	-@erase "$(INTDIR)\authkeys.obj"
	-@erase "$(INTDIR)\authkeys.sbr"
	-@erase "$(INTDIR)\authparity.obj"
	-@erase "$(INTDIR)\authparity.sbr"
	-@erase "$(INTDIR)\authreadkeys.obj"
	-@erase "$(INTDIR)\authreadkeys.sbr"
	-@erase "$(INTDIR)\authusekey.obj"
	-@erase "$(INTDIR)\authusekey.sbr"
	-@erase "$(INTDIR)\buftvtots.obj"
	-@erase "$(INTDIR)\buftvtots.sbr"
	-@erase "$(INTDIR)\caljulian.obj"
	-@erase "$(INTDIR)\caljulian.sbr"
	-@erase "$(INTDIR)\calleapwhen.obj"
	-@erase "$(INTDIR)\calleapwhen.sbr"
	-@erase "$(INTDIR)\caltontp.obj"
	-@erase "$(INTDIR)\caltontp.sbr"
	-@erase "$(INTDIR)\calyearstart.obj"
	-@erase "$(INTDIR)\calyearstart.sbr"
	-@erase "$(INTDIR)\clocktime.obj"
	-@erase "$(INTDIR)\clocktime.sbr"
	-@erase "$(INTDIR)\clocktypes.obj"
	-@erase "$(INTDIR)\clocktypes.sbr"
	-@erase "$(INTDIR)\decodenetnum.obj"
	-@erase "$(INTDIR)\decodenetnum.sbr"
	-@erase "$(INTDIR)\dofptoa.obj"
	-@erase "$(INTDIR)\dofptoa.sbr"
	-@erase "$(INTDIR)\dolfptoa.obj"
	-@erase "$(INTDIR)\dolfptoa.sbr"
	-@erase "$(INTDIR)\emalloc.obj"
	-@erase "$(INTDIR)\emalloc.sbr"
	-@erase "$(INTDIR)\findconfig.obj"
	-@erase "$(INTDIR)\findconfig.sbr"
	-@erase "$(INTDIR)\fptoa.obj"
	-@erase "$(INTDIR)\fptoa.sbr"
	-@erase "$(INTDIR)\fptoms.obj"
	-@erase "$(INTDIR)\fptoms.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\hextoint.obj"
	-@erase "$(INTDIR)\hextoint.sbr"
	-@erase "$(INTDIR)\hextolfp.obj"
	-@erase "$(INTDIR)\hextolfp.sbr"
	-@erase "$(INTDIR)\humandate.obj"
	-@erase "$(INTDIR)\humandate.sbr"
	-@erase "$(INTDIR)\inttoa.obj"
	-@erase "$(INTDIR)\inttoa.sbr"
	-@erase "$(INTDIR)\lib_strbuf.obj"
	-@erase "$(INTDIR)\lib_strbuf.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\machines.obj"
	-@erase "$(INTDIR)\machines.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\mexit.obj"
	-@erase "$(INTDIR)\mexit.sbr"
	-@erase "$(INTDIR)\mfptoa.obj"
	-@erase "$(INTDIR)\mfptoa.sbr"
	-@erase "$(INTDIR)\mfptoms.obj"
	-@erase "$(INTDIR)\mfptoms.sbr"
	-@erase "$(INTDIR)\modetoa.obj"
	-@erase "$(INTDIR)\modetoa.sbr"
	-@erase "$(INTDIR)\mstolfp.obj"
	-@erase "$(INTDIR)\mstolfp.sbr"
	-@erase "$(INTDIR)\msutotsf.obj"
	-@erase "$(INTDIR)\msutotsf.sbr"
	-@erase "$(INTDIR)\msyslog.obj"
	-@erase "$(INTDIR)\msyslog.sbr"
	-@erase "$(INTDIR)\netof.obj"
	-@erase "$(INTDIR)\netof.sbr"
	-@erase "$(INTDIR)\numtoa.obj"
	-@erase "$(INTDIR)\numtoa.sbr"
	-@erase "$(INTDIR)\numtohost.obj"
	-@erase "$(INTDIR)\numtohost.sbr"
	-@erase "$(INTDIR)\octtoint.obj"
	-@erase "$(INTDIR)\octtoint.sbr"
	-@erase "$(INTDIR)\prettydate.obj"
	-@erase "$(INTDIR)\prettydate.sbr"
	-@erase "$(INTDIR)\ranny.obj"
	-@erase "$(INTDIR)\ranny.sbr"
	-@erase "$(INTDIR)\refnumtoa.obj"
	-@erase "$(INTDIR)\refnumtoa.sbr"
	-@erase "$(INTDIR)\statestr.obj"
	-@erase "$(INTDIR)\statestr.sbr"
	-@erase "$(INTDIR)\syssignal.obj"
	-@erase "$(INTDIR)\syssignal.sbr"
	-@erase "$(INTDIR)\systime.obj"
	-@erase "$(INTDIR)\systime.sbr"
	-@erase "$(INTDIR)\tsftomsu.obj"
	-@erase "$(INTDIR)\tsftomsu.sbr"
	-@erase "$(INTDIR)\tstotv.obj"
	-@erase "$(INTDIR)\tstotv.sbr"
	-@erase "$(INTDIR)\tvtoa.obj"
	-@erase "$(INTDIR)\tvtoa.sbr"
	-@erase "$(INTDIR)\tvtots.obj"
	-@erase "$(INTDIR)\tvtots.sbr"
	-@erase "$(INTDIR)\uglydate.obj"
	-@erase "$(INTDIR)\uglydate.sbr"
	-@erase "$(INTDIR)\uinttoa.obj"
	-@erase "$(INTDIR)\uinttoa.sbr"
	-@erase "$(INTDIR)\utvtoa.obj"
	-@erase "$(INTDIR)\utvtoa.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\libntp.bsc"
	-@erase "$(OUTDIR)\libntp.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "SYS_WINNT" /D "__STDC__" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libntp.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libntp.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\a_md5encrypt.sbr" \
	"$(INTDIR)\atoint.sbr" \
	"$(INTDIR)\atolfp.sbr" \
	"$(INTDIR)\atouint.sbr" \
	"$(INTDIR)\authencrypt.sbr" \
	"$(INTDIR)\authkeys.sbr" \
	"$(INTDIR)\authparity.sbr" \
	"$(INTDIR)\authreadkeys.sbr" \
	"$(INTDIR)\authusekey.sbr" \
	"$(INTDIR)\buftvtots.sbr" \
	"$(INTDIR)\caljulian.sbr" \
	"$(INTDIR)\calleapwhen.sbr" \
	"$(INTDIR)\caltontp.sbr" \
	"$(INTDIR)\calyearstart.sbr" \
	"$(INTDIR)\clocktime.sbr" \
	"$(INTDIR)\clocktypes.sbr" \
	"$(INTDIR)\decodenetnum.sbr" \
	"$(INTDIR)\dofptoa.sbr" \
	"$(INTDIR)\dolfptoa.sbr" \
	"$(INTDIR)\emalloc.sbr" \
	"$(INTDIR)\findconfig.sbr" \
	"$(INTDIR)\fptoa.sbr" \
	"$(INTDIR)\fptoms.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\hextoint.sbr" \
	"$(INTDIR)\hextolfp.sbr" \
	"$(INTDIR)\humandate.sbr" \
	"$(INTDIR)\inttoa.sbr" \
	"$(INTDIR)\lib_strbuf.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\machines.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\mexit.sbr" \
	"$(INTDIR)\mfptoa.sbr" \
	"$(INTDIR)\mfptoms.sbr" \
	"$(INTDIR)\modetoa.sbr" \
	"$(INTDIR)\mstolfp.sbr" \
	"$(INTDIR)\msutotsf.sbr" \
	"$(INTDIR)\msyslog.sbr" \
	"$(INTDIR)\netof.sbr" \
	"$(INTDIR)\numtoa.sbr" \
	"$(INTDIR)\numtohost.sbr" \
	"$(INTDIR)\octtoint.sbr" \
	"$(INTDIR)\prettydate.sbr" \
	"$(INTDIR)\ranny.sbr" \
	"$(INTDIR)\refnumtoa.sbr" \
	"$(INTDIR)\statestr.sbr" \
	"$(INTDIR)\syssignal.sbr" \
	"$(INTDIR)\systime.sbr" \
	"$(INTDIR)\tsftomsu.sbr" \
	"$(INTDIR)\tstotv.sbr" \
	"$(INTDIR)\tvtoa.sbr" \
	"$(INTDIR)\tvtots.sbr" \
	"$(INTDIR)\uglydate.sbr" \
	"$(INTDIR)\uinttoa.sbr" \
	"$(INTDIR)\utvtoa.sbr"

"$(OUTDIR)\libntp.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libntp.lib" 
LIB32_OBJS= \
	"$(INTDIR)\a_md5encrypt.obj" \
	"$(INTDIR)\atoint.obj" \
	"$(INTDIR)\atolfp.obj" \
	"$(INTDIR)\atouint.obj" \
	"$(INTDIR)\authencrypt.obj" \
	"$(INTDIR)\authkeys.obj" \
	"$(INTDIR)\authparity.obj" \
	"$(INTDIR)\authreadkeys.obj" \
	"$(INTDIR)\authusekey.obj" \
	"$(INTDIR)\buftvtots.obj" \
	"$(INTDIR)\caljulian.obj" \
	"$(INTDIR)\calleapwhen.obj" \
	"$(INTDIR)\caltontp.obj" \
	"$(INTDIR)\calyearstart.obj" \
	"$(INTDIR)\clocktime.obj" \
	"$(INTDIR)\clocktypes.obj" \
	"$(INTDIR)\decodenetnum.obj" \
	"$(INTDIR)\dofptoa.obj" \
	"$(INTDIR)\dolfptoa.obj" \
	"$(INTDIR)\emalloc.obj" \
	"$(INTDIR)\findconfig.obj" \
	"$(INTDIR)\fptoa.obj" \
	"$(INTDIR)\fptoms.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\hextoint.obj" \
	"$(INTDIR)\hextolfp.obj" \
	"$(INTDIR)\humandate.obj" \
	"$(INTDIR)\inttoa.obj" \
	"$(INTDIR)\lib_strbuf.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\machines.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\mexit.obj" \
	"$(INTDIR)\mfptoa.obj" \
	"$(INTDIR)\mfptoms.obj" \
	"$(INTDIR)\modetoa.obj" \
	"$(INTDIR)\mstolfp.obj" \
	"$(INTDIR)\msutotsf.obj" \
	"$(INTDIR)\msyslog.obj" \
	"$(INTDIR)\netof.obj" \
	"$(INTDIR)\numtoa.obj" \
	"$(INTDIR)\numtohost.obj" \
	"$(INTDIR)\octtoint.obj" \
	"$(INTDIR)\prettydate.obj" \
	"$(INTDIR)\ranny.obj" \
	"$(INTDIR)\refnumtoa.obj" \
	"$(INTDIR)\statestr.obj" \
	"$(INTDIR)\syssignal.obj" \
	"$(INTDIR)\systime.obj" \
	"$(INTDIR)\tsftomsu.obj" \
	"$(INTDIR)\tstotv.obj" \
	"$(INTDIR)\tvtoa.obj" \
	"$(INTDIR)\tvtots.obj" \
	"$(INTDIR)\uglydate.obj" \
	"$(INTDIR)\uinttoa.obj" \
	"$(INTDIR)\utvtoa.obj"

"$(OUTDIR)\libntp.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libntp - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\libntp.lib" "$(OUTDIR)\libntp.bsc"

!ELSE 

ALL : "$(OUTDIR)\libntp.lib" "$(OUTDIR)\libntp.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\a_md5encrypt.obj"
	-@erase "$(INTDIR)\a_md5encrypt.sbr"
	-@erase "$(INTDIR)\atoint.obj"
	-@erase "$(INTDIR)\atoint.sbr"
	-@erase "$(INTDIR)\atolfp.obj"
	-@erase "$(INTDIR)\atolfp.sbr"
	-@erase "$(INTDIR)\atouint.obj"
	-@erase "$(INTDIR)\atouint.sbr"
	-@erase "$(INTDIR)\authencrypt.obj"
	-@erase "$(INTDIR)\authencrypt.sbr"
	-@erase "$(INTDIR)\authkeys.obj"
	-@erase "$(INTDIR)\authkeys.sbr"
	-@erase "$(INTDIR)\authparity.obj"
	-@erase "$(INTDIR)\authparity.sbr"
	-@erase "$(INTDIR)\authreadkeys.obj"
	-@erase "$(INTDIR)\authreadkeys.sbr"
	-@erase "$(INTDIR)\authusekey.obj"
	-@erase "$(INTDIR)\authusekey.sbr"
	-@erase "$(INTDIR)\buftvtots.obj"
	-@erase "$(INTDIR)\buftvtots.sbr"
	-@erase "$(INTDIR)\caljulian.obj"
	-@erase "$(INTDIR)\caljulian.sbr"
	-@erase "$(INTDIR)\calleapwhen.obj"
	-@erase "$(INTDIR)\calleapwhen.sbr"
	-@erase "$(INTDIR)\caltontp.obj"
	-@erase "$(INTDIR)\caltontp.sbr"
	-@erase "$(INTDIR)\calyearstart.obj"
	-@erase "$(INTDIR)\calyearstart.sbr"
	-@erase "$(INTDIR)\clocktime.obj"
	-@erase "$(INTDIR)\clocktime.sbr"
	-@erase "$(INTDIR)\clocktypes.obj"
	-@erase "$(INTDIR)\clocktypes.sbr"
	-@erase "$(INTDIR)\decodenetnum.obj"
	-@erase "$(INTDIR)\decodenetnum.sbr"
	-@erase "$(INTDIR)\dofptoa.obj"
	-@erase "$(INTDIR)\dofptoa.sbr"
	-@erase "$(INTDIR)\dolfptoa.obj"
	-@erase "$(INTDIR)\dolfptoa.sbr"
	-@erase "$(INTDIR)\emalloc.obj"
	-@erase "$(INTDIR)\emalloc.sbr"
	-@erase "$(INTDIR)\findconfig.obj"
	-@erase "$(INTDIR)\findconfig.sbr"
	-@erase "$(INTDIR)\fptoa.obj"
	-@erase "$(INTDIR)\fptoa.sbr"
	-@erase "$(INTDIR)\fptoms.obj"
	-@erase "$(INTDIR)\fptoms.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\hextoint.obj"
	-@erase "$(INTDIR)\hextoint.sbr"
	-@erase "$(INTDIR)\hextolfp.obj"
	-@erase "$(INTDIR)\hextolfp.sbr"
	-@erase "$(INTDIR)\humandate.obj"
	-@erase "$(INTDIR)\humandate.sbr"
	-@erase "$(INTDIR)\inttoa.obj"
	-@erase "$(INTDIR)\inttoa.sbr"
	-@erase "$(INTDIR)\lib_strbuf.obj"
	-@erase "$(INTDIR)\lib_strbuf.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\machines.obj"
	-@erase "$(INTDIR)\machines.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\mexit.obj"
	-@erase "$(INTDIR)\mexit.sbr"
	-@erase "$(INTDIR)\mfptoa.obj"
	-@erase "$(INTDIR)\mfptoa.sbr"
	-@erase "$(INTDIR)\mfptoms.obj"
	-@erase "$(INTDIR)\mfptoms.sbr"
	-@erase "$(INTDIR)\modetoa.obj"
	-@erase "$(INTDIR)\modetoa.sbr"
	-@erase "$(INTDIR)\mstolfp.obj"
	-@erase "$(INTDIR)\mstolfp.sbr"
	-@erase "$(INTDIR)\msutotsf.obj"
	-@erase "$(INTDIR)\msutotsf.sbr"
	-@erase "$(INTDIR)\msyslog.obj"
	-@erase "$(INTDIR)\msyslog.sbr"
	-@erase "$(INTDIR)\netof.obj"
	-@erase "$(INTDIR)\netof.sbr"
	-@erase "$(INTDIR)\numtoa.obj"
	-@erase "$(INTDIR)\numtoa.sbr"
	-@erase "$(INTDIR)\numtohost.obj"
	-@erase "$(INTDIR)\numtohost.sbr"
	-@erase "$(INTDIR)\octtoint.obj"
	-@erase "$(INTDIR)\octtoint.sbr"
	-@erase "$(INTDIR)\prettydate.obj"
	-@erase "$(INTDIR)\prettydate.sbr"
	-@erase "$(INTDIR)\ranny.obj"
	-@erase "$(INTDIR)\ranny.sbr"
	-@erase "$(INTDIR)\refnumtoa.obj"
	-@erase "$(INTDIR)\refnumtoa.sbr"
	-@erase "$(INTDIR)\statestr.obj"
	-@erase "$(INTDIR)\statestr.sbr"
	-@erase "$(INTDIR)\syssignal.obj"
	-@erase "$(INTDIR)\syssignal.sbr"
	-@erase "$(INTDIR)\systime.obj"
	-@erase "$(INTDIR)\systime.sbr"
	-@erase "$(INTDIR)\tsftomsu.obj"
	-@erase "$(INTDIR)\tsftomsu.sbr"
	-@erase "$(INTDIR)\tstotv.obj"
	-@erase "$(INTDIR)\tstotv.sbr"
	-@erase "$(INTDIR)\tvtoa.obj"
	-@erase "$(INTDIR)\tvtoa.sbr"
	-@erase "$(INTDIR)\tvtots.obj"
	-@erase "$(INTDIR)\tvtots.sbr"
	-@erase "$(INTDIR)\uglydate.obj"
	-@erase "$(INTDIR)\uglydate.sbr"
	-@erase "$(INTDIR)\uinttoa.obj"
	-@erase "$(INTDIR)\uinttoa.sbr"
	-@erase "$(INTDIR)\utvtoa.obj"
	-@erase "$(INTDIR)\utvtoa.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\libntp.bsc"
	-@erase "$(OUTDIR)\libntp.lib"
	-@erase ".\messages.rc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_WINDOWS" /D "SYS_WINNT" /D "__STDC__" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libntp.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libntp.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\a_md5encrypt.sbr" \
	"$(INTDIR)\atoint.sbr" \
	"$(INTDIR)\atolfp.sbr" \
	"$(INTDIR)\atouint.sbr" \
	"$(INTDIR)\authencrypt.sbr" \
	"$(INTDIR)\authkeys.sbr" \
	"$(INTDIR)\authparity.sbr" \
	"$(INTDIR)\authreadkeys.sbr" \
	"$(INTDIR)\authusekey.sbr" \
	"$(INTDIR)\buftvtots.sbr" \
	"$(INTDIR)\caljulian.sbr" \
	"$(INTDIR)\calleapwhen.sbr" \
	"$(INTDIR)\caltontp.sbr" \
	"$(INTDIR)\calyearstart.sbr" \
	"$(INTDIR)\clocktime.sbr" \
	"$(INTDIR)\clocktypes.sbr" \
	"$(INTDIR)\decodenetnum.sbr" \
	"$(INTDIR)\dofptoa.sbr" \
	"$(INTDIR)\dolfptoa.sbr" \
	"$(INTDIR)\emalloc.sbr" \
	"$(INTDIR)\findconfig.sbr" \
	"$(INTDIR)\fptoa.sbr" \
	"$(INTDIR)\fptoms.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\hextoint.sbr" \
	"$(INTDIR)\hextolfp.sbr" \
	"$(INTDIR)\humandate.sbr" \
	"$(INTDIR)\inttoa.sbr" \
	"$(INTDIR)\lib_strbuf.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\machines.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\mexit.sbr" \
	"$(INTDIR)\mfptoa.sbr" \
	"$(INTDIR)\mfptoms.sbr" \
	"$(INTDIR)\modetoa.sbr" \
	"$(INTDIR)\mstolfp.sbr" \
	"$(INTDIR)\msutotsf.sbr" \
	"$(INTDIR)\msyslog.sbr" \
	"$(INTDIR)\netof.sbr" \
	"$(INTDIR)\numtoa.sbr" \
	"$(INTDIR)\numtohost.sbr" \
	"$(INTDIR)\octtoint.sbr" \
	"$(INTDIR)\prettydate.sbr" \
	"$(INTDIR)\ranny.sbr" \
	"$(INTDIR)\refnumtoa.sbr" \
	"$(INTDIR)\statestr.sbr" \
	"$(INTDIR)\syssignal.sbr" \
	"$(INTDIR)\systime.sbr" \
	"$(INTDIR)\tsftomsu.sbr" \
	"$(INTDIR)\tstotv.sbr" \
	"$(INTDIR)\tvtoa.sbr" \
	"$(INTDIR)\tvtots.sbr" \
	"$(INTDIR)\uglydate.sbr" \
	"$(INTDIR)\uinttoa.sbr" \
	"$(INTDIR)\utvtoa.sbr"

"$(OUTDIR)\libntp.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libntp.lib" 
LIB32_OBJS= \
	"$(INTDIR)\a_md5encrypt.obj" \
	"$(INTDIR)\atoint.obj" \
	"$(INTDIR)\atolfp.obj" \
	"$(INTDIR)\atouint.obj" \
	"$(INTDIR)\authencrypt.obj" \
	"$(INTDIR)\authkeys.obj" \
	"$(INTDIR)\authparity.obj" \
	"$(INTDIR)\authreadkeys.obj" \
	"$(INTDIR)\authusekey.obj" \
	"$(INTDIR)\buftvtots.obj" \
	"$(INTDIR)\caljulian.obj" \
	"$(INTDIR)\calleapwhen.obj" \
	"$(INTDIR)\caltontp.obj" \
	"$(INTDIR)\calyearstart.obj" \
	"$(INTDIR)\clocktime.obj" \
	"$(INTDIR)\clocktypes.obj" \
	"$(INTDIR)\decodenetnum.obj" \
	"$(INTDIR)\dofptoa.obj" \
	"$(INTDIR)\dolfptoa.obj" \
	"$(INTDIR)\emalloc.obj" \
	"$(INTDIR)\findconfig.obj" \
	"$(INTDIR)\fptoa.obj" \
	"$(INTDIR)\fptoms.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\hextoint.obj" \
	"$(INTDIR)\hextolfp.obj" \
	"$(INTDIR)\humandate.obj" \
	"$(INTDIR)\inttoa.obj" \
	"$(INTDIR)\lib_strbuf.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\machines.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\mexit.obj" \
	"$(INTDIR)\mfptoa.obj" \
	"$(INTDIR)\mfptoms.obj" \
	"$(INTDIR)\modetoa.obj" \
	"$(INTDIR)\mstolfp.obj" \
	"$(INTDIR)\msutotsf.obj" \
	"$(INTDIR)\msyslog.obj" \
	"$(INTDIR)\netof.obj" \
	"$(INTDIR)\numtoa.obj" \
	"$(INTDIR)\numtohost.obj" \
	"$(INTDIR)\octtoint.obj" \
	"$(INTDIR)\prettydate.obj" \
	"$(INTDIR)\ranny.obj" \
	"$(INTDIR)\refnumtoa.obj" \
	"$(INTDIR)\statestr.obj" \
	"$(INTDIR)\syssignal.obj" \
	"$(INTDIR)\systime.obj" \
	"$(INTDIR)\tsftomsu.obj" \
	"$(INTDIR)\tstotv.obj" \
	"$(INTDIR)\tvtoa.obj" \
	"$(INTDIR)\tvtots.obj" \
	"$(INTDIR)\uglydate.obj" \
	"$(INTDIR)\uinttoa.obj" \
	"$(INTDIR)\utvtoa.obj"

"$(OUTDIR)\libntp.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(CFG)" == "libntp - Release" || "$(CFG)" == "libntp - Debug"
SOURCE=..\..\..\libntp\a_md5encrypt.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_A_MD5=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\include\config.h"\
	

"$(INTDIR)\a_md5encrypt.obj"	"$(INTDIR)\a_md5encrypt.sbr" : $(SOURCE)\
 $(DEP_CPP_A_MD5) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_A_MD5=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\include\config.h"\
	

"$(INTDIR)\a_md5encrypt.obj"	"$(INTDIR)\a_md5encrypt.sbr" : $(SOURCE)\
 $(DEP_CPP_A_MD5) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\atoint.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_ATOIN=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\atoint.obj"	"$(INTDIR)\atoint.sbr" : $(SOURCE) $(DEP_CPP_ATOIN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_ATOIN=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\atoint.obj"	"$(INTDIR)\atoint.sbr" : $(SOURCE) $(DEP_CPP_ATOIN)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\atolfp.c
DEP_CPP_ATOLF=\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\atolfp.obj"	"$(INTDIR)\atolfp.sbr" : $(SOURCE) $(DEP_CPP_ATOLF)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\atouint.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_ATOUI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\atouint.obj"	"$(INTDIR)\atouint.sbr" : $(SOURCE) $(DEP_CPP_ATOUI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_ATOUI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\atouint.obj"	"$(INTDIR)\atouint.sbr" : $(SOURCE) $(DEP_CPP_ATOUI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\authencrypt.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_AUTHE=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\include\config.h"\
	

"$(INTDIR)\authencrypt.obj"	"$(INTDIR)\authencrypt.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_AUTHE=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\include\config.h"\
	

"$(INTDIR)\authencrypt.obj"	"$(INTDIR)\authencrypt.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\authkeys.c
DEP_CPP_AUTHK=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\authkeys.obj"	"$(INTDIR)\authkeys.sbr" : $(SOURCE) $(DEP_CPP_AUTHK)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\authparity.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_AUTHP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\authparity.obj"	"$(INTDIR)\authparity.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_AUTHP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\authparity.obj"	"$(INTDIR)\authparity.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\authreadkeys.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_AUTHR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\authreadkeys.obj"	"$(INTDIR)\authreadkeys.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_AUTHR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\authreadkeys.obj"	"$(INTDIR)\authreadkeys.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\authusekey.c
DEP_CPP_AUTHU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\authusekey.obj"	"$(INTDIR)\authusekey.sbr" : $(SOURCE)\
 $(DEP_CPP_AUTHU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\buftvtots.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_BUFTV=\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\buftvtots.obj"	"$(INTDIR)\buftvtots.sbr" : $(SOURCE)\
 $(DEP_CPP_BUFTV) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_BUFTV=\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\buftvtots.obj"	"$(INTDIR)\buftvtots.sbr" : $(SOURCE)\
 $(DEP_CPP_BUFTV) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\caljulian.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CALJU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\caljulian.obj"	"$(INTDIR)\caljulian.sbr" : $(SOURCE)\
 $(DEP_CPP_CALJU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CALJU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\caljulian.obj"	"$(INTDIR)\caljulian.sbr" : $(SOURCE)\
 $(DEP_CPP_CALJU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\calleapwhen.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CALLE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\calleapwhen.obj"	"$(INTDIR)\calleapwhen.sbr" : $(SOURCE)\
 $(DEP_CPP_CALLE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CALLE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\calleapwhen.obj"	"$(INTDIR)\calleapwhen.sbr" : $(SOURCE)\
 $(DEP_CPP_CALLE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\caltontp.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CALTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\caltontp.obj"	"$(INTDIR)\caltontp.sbr" : $(SOURCE) $(DEP_CPP_CALTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CALTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\caltontp.obj"	"$(INTDIR)\caltontp.sbr" : $(SOURCE) $(DEP_CPP_CALTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\calyearstart.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CALYE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\calyearstart.obj"	"$(INTDIR)\calyearstart.sbr" : $(SOURCE)\
 $(DEP_CPP_CALYE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CALYE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\calyearstart.obj"	"$(INTDIR)\calyearstart.sbr" : $(SOURCE)\
 $(DEP_CPP_CALYE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\clocktime.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CLOCK=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\clocktime.obj"	"$(INTDIR)\clocktime.sbr" : $(SOURCE)\
 $(DEP_CPP_CLOCK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CLOCK=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\clocktime.obj"	"$(INTDIR)\clocktime.sbr" : $(SOURCE)\
 $(DEP_CPP_CLOCK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\clocktypes.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_CLOCKT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\clocktypes.obj"	"$(INTDIR)\clocktypes.sbr" : $(SOURCE)\
 $(DEP_CPP_CLOCKT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_CLOCKT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\clocktypes.obj"	"$(INTDIR)\clocktypes.sbr" : $(SOURCE)\
 $(DEP_CPP_CLOCKT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\decodenetnum.c
DEP_CPP_DECOD=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\decodenetnum.obj"	"$(INTDIR)\decodenetnum.sbr" : $(SOURCE)\
 $(DEP_CPP_DECOD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\dofptoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_DOFPT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\dofptoa.obj"	"$(INTDIR)\dofptoa.sbr" : $(SOURCE) $(DEP_CPP_DOFPT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_DOFPT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\dofptoa.obj"	"$(INTDIR)\dofptoa.sbr" : $(SOURCE) $(DEP_CPP_DOFPT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\dolfptoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_DOLFP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\dolfptoa.obj"	"$(INTDIR)\dolfptoa.sbr" : $(SOURCE) $(DEP_CPP_DOLFP)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_DOLFP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\dolfptoa.obj"	"$(INTDIR)\dolfptoa.sbr" : $(SOURCE) $(DEP_CPP_DOLFP)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\emalloc.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_EMALL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\emalloc.obj"	"$(INTDIR)\emalloc.sbr" : $(SOURCE) $(DEP_CPP_EMALL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_EMALL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\emalloc.obj"	"$(INTDIR)\emalloc.sbr" : $(SOURCE) $(DEP_CPP_EMALL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\findconfig.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_FINDC=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\findconfig.obj"	"$(INTDIR)\findconfig.sbr" : $(SOURCE)\
 $(DEP_CPP_FINDC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_FINDC=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\findconfig.obj"	"$(INTDIR)\findconfig.sbr" : $(SOURCE)\
 $(DEP_CPP_FINDC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\fptoa.c
DEP_CPP_FPTOA=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\fptoa.obj"	"$(INTDIR)\fptoa.sbr" : $(SOURCE) $(DEP_CPP_FPTOA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\fptoms.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_FPTOM=\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\fptoms.obj"	"$(INTDIR)\fptoms.sbr" : $(SOURCE) $(DEP_CPP_FPTOM)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_FPTOM=\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\fptoms.obj"	"$(INTDIR)\fptoms.sbr" : $(SOURCE) $(DEP_CPP_FPTOM)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\getopt.c
DEP_CPP_GETOP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\getopt.obj"	"$(INTDIR)\getopt.sbr" : $(SOURCE) $(DEP_CPP_GETOP)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\hextoint.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_HEXTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\hextoint.obj"	"$(INTDIR)\hextoint.sbr" : $(SOURCE) $(DEP_CPP_HEXTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_HEXTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\hextoint.obj"	"$(INTDIR)\hextoint.sbr" : $(SOURCE) $(DEP_CPP_HEXTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\hextolfp.c
DEP_CPP_HEXTOL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\hextolfp.obj"	"$(INTDIR)\hextolfp.sbr" : $(SOURCE) $(DEP_CPP_HEXTOL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\humandate.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_HUMAN=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\humandate.obj"	"$(INTDIR)\humandate.sbr" : $(SOURCE)\
 $(DEP_CPP_HUMAN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_HUMAN=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\humandate.obj"	"$(INTDIR)\humandate.sbr" : $(SOURCE)\
 $(DEP_CPP_HUMAN) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\inttoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_INTTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\inttoa.obj"	"$(INTDIR)\inttoa.sbr" : $(SOURCE) $(DEP_CPP_INTTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_INTTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\inttoa.obj"	"$(INTDIR)\inttoa.sbr" : $(SOURCE) $(DEP_CPP_INTTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\lib_strbuf.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_LIB_S=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\lib_strbuf.obj"	"$(INTDIR)\lib_strbuf.sbr" : $(SOURCE)\
 $(DEP_CPP_LIB_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_LIB_S=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\lib_strbuf.obj"	"$(INTDIR)\lib_strbuf.sbr" : $(SOURCE)\
 $(DEP_CPP_LIB_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\log.c
DEP_CPP_LOG_C=\
	".\log.h"\
	".\messages.h"\
	

"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) $(DEP_CPP_LOG_C)\
 "$(INTDIR)"


SOURCE=..\..\..\libntp\machines.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_MACHI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\machines.obj"	"$(INTDIR)\machines.sbr" : $(SOURCE) $(DEP_CPP_MACHI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_MACHI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\machines.obj"	"$(INTDIR)\machines.sbr" : $(SOURCE) $(DEP_CPP_MACHI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\md5c.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_MD5C_=\
	"..\..\..\include\global.h"\
	"..\..\..\include\md5.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\md5c.obj"	"$(INTDIR)\md5c.sbr" : $(SOURCE) $(DEP_CPP_MD5C_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_MD5C_=\
	"..\..\..\include\global.h"\
	"..\..\..\include\md5.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\md5c.obj"	"$(INTDIR)\md5c.sbr" : $(SOURCE) $(DEP_CPP_MD5C_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\messages.rc

!IF  "$(CFG)" == "libntp - Release"

!ELSEIF  "$(CFG)" == "libntp - Debug"

InputPath=.\messages.rc

"messages.rc"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc messages.mc

!ENDIF 

SOURCE=..\..\..\libntp\mexit.c

"$(INTDIR)\mexit.obj"	"$(INTDIR)\mexit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\mfptoa.c
DEP_CPP_MFPTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\mfptoa.obj"	"$(INTDIR)\mfptoa.sbr" : $(SOURCE) $(DEP_CPP_MFPTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\mfptoms.c
DEP_CPP_MFPTOM=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\mfptoms.obj"	"$(INTDIR)\mfptoms.sbr" : $(SOURCE) $(DEP_CPP_MFPTOM)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\modetoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_MODET=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\modetoa.obj"	"$(INTDIR)\modetoa.sbr" : $(SOURCE) $(DEP_CPP_MODET)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_MODET=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\modetoa.obj"	"$(INTDIR)\modetoa.sbr" : $(SOURCE) $(DEP_CPP_MODET)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\mstolfp.c
DEP_CPP_MSTOL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\mstolfp.obj"	"$(INTDIR)\mstolfp.sbr" : $(SOURCE) $(DEP_CPP_MSTOL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\msutotsf.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_MSUTO=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\msutotsf.obj"	"$(INTDIR)\msutotsf.sbr" : $(SOURCE) $(DEP_CPP_MSUTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_MSUTO=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\msutotsf.obj"	"$(INTDIR)\msutotsf.sbr" : $(SOURCE) $(DEP_CPP_MSUTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\msyslog.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_MSYSL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	".\log.h"\
	".\messages.h"\
	

"$(INTDIR)\msyslog.obj"	"$(INTDIR)\msyslog.sbr" : $(SOURCE) $(DEP_CPP_MSYSL)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_MSYSL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	".\log.h"\
	".\messages.h"\
	

"$(INTDIR)\msyslog.obj"	"$(INTDIR)\msyslog.sbr" : $(SOURCE) $(DEP_CPP_MSYSL)\
 "$(INTDIR)" ".\messages.h"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\netof.c
DEP_CPP_NETOF=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\netof.obj"	"$(INTDIR)\netof.sbr" : $(SOURCE) $(DEP_CPP_NETOF)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\numtoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_NUMTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\numtoa.obj"	"$(INTDIR)\numtoa.sbr" : $(SOURCE) $(DEP_CPP_NUMTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_NUMTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\numtoa.obj"	"$(INTDIR)\numtoa.sbr" : $(SOURCE) $(DEP_CPP_NUMTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\numtohost.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_NUMTOH=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\numtohost.obj"	"$(INTDIR)\numtohost.sbr" : $(SOURCE)\
 $(DEP_CPP_NUMTOH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_NUMTOH=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\numtohost.obj"	"$(INTDIR)\numtohost.sbr" : $(SOURCE)\
 $(DEP_CPP_NUMTOH) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\octtoint.c
DEP_CPP_OCTTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\octtoint.obj"	"$(INTDIR)\octtoint.sbr" : $(SOURCE) $(DEP_CPP_OCTTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\prettydate.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_PRETT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\prettydate.obj"	"$(INTDIR)\prettydate.sbr" : $(SOURCE)\
 $(DEP_CPP_PRETT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_PRETT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\prettydate.obj"	"$(INTDIR)\prettydate.sbr" : $(SOURCE)\
 $(DEP_CPP_PRETT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\ranny.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_RANNY=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\ranny.obj"	"$(INTDIR)\ranny.sbr" : $(SOURCE) $(DEP_CPP_RANNY)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_RANNY=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\ranny.obj"	"$(INTDIR)\ranny.sbr" : $(SOURCE) $(DEP_CPP_RANNY)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\refnumtoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_REFNU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\refnumtoa.obj"	"$(INTDIR)\refnumtoa.sbr" : $(SOURCE)\
 $(DEP_CPP_REFNU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_REFNU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\refnumtoa.obj"	"$(INTDIR)\refnumtoa.sbr" : $(SOURCE)\
 $(DEP_CPP_REFNU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\statestr.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_STATE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\statestr.obj"	"$(INTDIR)\statestr.sbr" : $(SOURCE) $(DEP_CPP_STATE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_STATE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\statestr.obj"	"$(INTDIR)\statestr.sbr" : $(SOURCE) $(DEP_CPP_STATE)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\syssignal.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_SYSSI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\syssignal.obj"	"$(INTDIR)\syssignal.sbr" : $(SOURCE)\
 $(DEP_CPP_SYSSI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_SYSSI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\syssignal.obj"	"$(INTDIR)\syssignal.sbr" : $(SOURCE)\
 $(DEP_CPP_SYSSI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\systime.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_SYSTI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\systime.obj"	"$(INTDIR)\systime.sbr" : $(SOURCE) $(DEP_CPP_SYSTI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_SYSTI=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\systime.obj"	"$(INTDIR)\systime.sbr" : $(SOURCE) $(DEP_CPP_SYSTI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\tsftomsu.c
DEP_CPP_TSFTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\tsftomsu.obj"	"$(INTDIR)\tsftomsu.sbr" : $(SOURCE) $(DEP_CPP_TSFTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\..\libntp\tstotv.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_TSTOT=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tstotv.obj"	"$(INTDIR)\tstotv.sbr" : $(SOURCE) $(DEP_CPP_TSTOT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_TSTOT=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tstotv.obj"	"$(INTDIR)\tstotv.sbr" : $(SOURCE) $(DEP_CPP_TSTOT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\tvtoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_TVTOA=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tvtoa.obj"	"$(INTDIR)\tvtoa.sbr" : $(SOURCE) $(DEP_CPP_TVTOA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_TVTOA=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tvtoa.obj"	"$(INTDIR)\tvtoa.sbr" : $(SOURCE) $(DEP_CPP_TVTOA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\tvtots.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_TVTOT=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tvtots.obj"	"$(INTDIR)\tvtots.sbr" : $(SOURCE) $(DEP_CPP_TVTOT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_TVTOT=\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_types.h"\
	"..\include\config.h"\
	

"$(INTDIR)\tvtots.obj"	"$(INTDIR)\tvtots.sbr" : $(SOURCE) $(DEP_CPP_TVTOT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\uglydate.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_UGLYD=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\uglydate.obj"	"$(INTDIR)\uglydate.sbr" : $(SOURCE) $(DEP_CPP_UGLYD)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_UGLYD=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\uglydate.obj"	"$(INTDIR)\uglydate.sbr" : $(SOURCE) $(DEP_CPP_UGLYD)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\uinttoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_UINTT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\uinttoa.obj"	"$(INTDIR)\uinttoa.sbr" : $(SOURCE) $(DEP_CPP_UINTT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_UINTT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\uinttoa.obj"	"$(INTDIR)\uinttoa.sbr" : $(SOURCE) $(DEP_CPP_UINTT)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\libntp\utvtoa.c

!IF  "$(CFG)" == "libntp - Release"

DEP_CPP_UTVTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\utvtoa.obj"	"$(INTDIR)\utvtoa.sbr" : $(SOURCE) $(DEP_CPP_UTVTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libntp - Debug"

DEP_CPP_UTVTO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\libntp\lib_strbuf.h"\
	"..\include\config.h"\
	

"$(INTDIR)\utvtoa.obj"	"$(INTDIR)\utvtoa.sbr" : $(SOURCE) $(DEP_CPP_UTVTO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\messages.h
USERDEP__MESSA="messages.rc"	

!IF  "$(CFG)" == "libntp - Release"

!ELSEIF  "$(CFG)" == "libntp - Debug"

InputPath=.\messages.h

"messages.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__MESSA)
	mc messages.mc

!ENDIF 


!ENDIF 

