# Microsoft Developer Studio Generated NMAKE File, Based on ntpd.dsp
!IF "$(CFG)" == ""
CFG=ntpd - Release
!MESSAGE No configuration specified. Defaulting to ntpd - Release.
!ENDIF 

!IF "$(CFG)" != "ntpd - Release" && "$(CFG)" != "ntpd - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpd.mak" CFG="ntpd - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpd - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntpd - Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ntpd - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\nt_com.obj"
	-@erase "$(INTDIR)\nt_com.sbr"
	-@erase "$(INTDIR)\ntp_config.obj"
	-@erase "$(INTDIR)\ntp_config.sbr"
	-@erase "$(INTDIR)\ntp_control.obj"
	-@erase "$(INTDIR)\ntp_control.sbr"
	-@erase "$(INTDIR)\ntp_filegen.obj"
	-@erase "$(INTDIR)\ntp_filegen.sbr"
	-@erase "$(INTDIR)\ntp_intres.obj"
	-@erase "$(INTDIR)\ntp_intres.sbr"
	-@erase "$(INTDIR)\ntp_io.obj"
	-@erase "$(INTDIR)\ntp_io.sbr"
	-@erase "$(INTDIR)\ntp_loopfilter.obj"
	-@erase "$(INTDIR)\ntp_loopfilter.sbr"
	-@erase "$(INTDIR)\ntp_monitor.obj"
	-@erase "$(INTDIR)\ntp_monitor.sbr"
	-@erase "$(INTDIR)\ntp_peer.obj"
	-@erase "$(INTDIR)\ntp_peer.sbr"
	-@erase "$(INTDIR)\ntp_proto.obj"
	-@erase "$(INTDIR)\ntp_proto.sbr"
	-@erase "$(INTDIR)\ntp_refclock.obj"
	-@erase "$(INTDIR)\ntp_refclock.sbr"
	-@erase "$(INTDIR)\ntp_request.obj"
	-@erase "$(INTDIR)\ntp_request.sbr"
	-@erase "$(INTDIR)\ntp_restrict.obj"
	-@erase "$(INTDIR)\ntp_restrict.sbr"
	-@erase "$(INTDIR)\ntp_timer.obj"
	-@erase "$(INTDIR)\ntp_timer.sbr"
	-@erase "$(INTDIR)\ntp_util.obj"
	-@erase "$(INTDIR)\ntp_util.sbr"
	-@erase "$(INTDIR)\ntpd.obj"
	-@erase "$(INTDIR)\ntpd.sbr"
	-@erase "$(INTDIR)\refclock_acts.obj"
	-@erase "$(INTDIR)\refclock_acts.sbr"
	-@erase "$(INTDIR)\refclock_arbiter.obj"
	-@erase "$(INTDIR)\refclock_arbiter.sbr"
	-@erase "$(INTDIR)\refclock_arc.obj"
	-@erase "$(INTDIR)\refclock_arc.sbr"
	-@erase "$(INTDIR)\refclock_as2201.obj"
	-@erase "$(INTDIR)\refclock_as2201.sbr"
	-@erase "$(INTDIR)\refclock_atom.obj"
	-@erase "$(INTDIR)\refclock_atom.sbr"
	-@erase "$(INTDIR)\refclock_bancomm.obj"
	-@erase "$(INTDIR)\refclock_bancomm.sbr"
	-@erase "$(INTDIR)\refclock_chu.obj"
	-@erase "$(INTDIR)\refclock_chu.sbr"
	-@erase "$(INTDIR)\refclock_conf.obj"
	-@erase "$(INTDIR)\refclock_conf.sbr"
	-@erase "$(INTDIR)\refclock_datum.obj"
	-@erase "$(INTDIR)\refclock_datum.sbr"
	-@erase "$(INTDIR)\refclock_gpsvme.obj"
	-@erase "$(INTDIR)\refclock_gpsvme.sbr"
	-@erase "$(INTDIR)\refclock_heath.obj"
	-@erase "$(INTDIR)\refclock_heath.sbr"
	-@erase "$(INTDIR)\refclock_hpgps.obj"
	-@erase "$(INTDIR)\refclock_hpgps.sbr"
	-@erase "$(INTDIR)\refclock_irig.obj"
	-@erase "$(INTDIR)\refclock_irig.sbr"
	-@erase "$(INTDIR)\refclock_jupiter.obj"
	-@erase "$(INTDIR)\refclock_jupiter.sbr"
	-@erase "$(INTDIR)\refclock_leitch.obj"
	-@erase "$(INTDIR)\refclock_leitch.sbr"
	-@erase "$(INTDIR)\refclock_local.obj"
	-@erase "$(INTDIR)\refclock_local.sbr"
	-@erase "$(INTDIR)\refclock_msfees.obj"
	-@erase "$(INTDIR)\refclock_msfees.sbr"
	-@erase "$(INTDIR)\refclock_mx4200.obj"
	-@erase "$(INTDIR)\refclock_mx4200.sbr"
	-@erase "$(INTDIR)\refclock_nmea.obj"
	-@erase "$(INTDIR)\refclock_nmea.sbr"
	-@erase "$(INTDIR)\refclock_oncore.obj"
	-@erase "$(INTDIR)\refclock_oncore.sbr"
	-@erase "$(INTDIR)\refclock_palisade.obj"
	-@erase "$(INTDIR)\refclock_palisade.sbr"
	-@erase "$(INTDIR)\refclock_parse.obj"
	-@erase "$(INTDIR)\refclock_parse.sbr"
	-@erase "$(INTDIR)\refclock_pst.obj"
	-@erase "$(INTDIR)\refclock_pst.sbr"
	-@erase "$(INTDIR)\refclock_ptbacts.obj"
	-@erase "$(INTDIR)\refclock_ptbacts.sbr"
	-@erase "$(INTDIR)\refclock_shm.obj"
	-@erase "$(INTDIR)\refclock_shm.sbr"
	-@erase "$(INTDIR)\refclock_tpro.obj"
	-@erase "$(INTDIR)\refclock_tpro.sbr"
	-@erase "$(INTDIR)\refclock_trak.obj"
	-@erase "$(INTDIR)\refclock_trak.sbr"
	-@erase "$(INTDIR)\refclock_true.obj"
	-@erase "$(INTDIR)\refclock_true.sbr"
	-@erase "$(INTDIR)\refclock_usno.obj"
	-@erase "$(INTDIR)\refclock_usno.sbr"
	-@erase "$(INTDIR)\refclock_wwvb.obj"
	-@erase "$(INTDIR)\refclock_wwvb.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpd.bsc"
	-@erase "$(OUTDIR)\ntpd.exe"
	-@erase "$(OUTDIR)\ntpd.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\messages.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\nt_com.sbr" \
	"$(INTDIR)\ntp_config.sbr" \
	"$(INTDIR)\ntp_control.sbr" \
	"$(INTDIR)\ntp_filegen.sbr" \
	"$(INTDIR)\ntp_intres.sbr" \
	"$(INTDIR)\ntp_io.sbr" \
	"$(INTDIR)\ntp_loopfilter.sbr" \
	"$(INTDIR)\ntp_monitor.sbr" \
	"$(INTDIR)\ntp_peer.sbr" \
	"$(INTDIR)\ntp_proto.sbr" \
	"$(INTDIR)\ntp_refclock.sbr" \
	"$(INTDIR)\ntp_request.sbr" \
	"$(INTDIR)\ntp_restrict.sbr" \
	"$(INTDIR)\ntp_timer.sbr" \
	"$(INTDIR)\ntp_util.sbr" \
	"$(INTDIR)\ntpd.sbr" \
	"$(INTDIR)\refclock_acts.sbr" \
	"$(INTDIR)\refclock_arbiter.sbr" \
	"$(INTDIR)\refclock_arc.sbr" \
	"$(INTDIR)\refclock_as2201.sbr" \
	"$(INTDIR)\refclock_atom.sbr" \
	"$(INTDIR)\refclock_bancomm.sbr" \
	"$(INTDIR)\refclock_chu.sbr" \
	"$(INTDIR)\refclock_conf.sbr" \
	"$(INTDIR)\refclock_datum.sbr" \
	"$(INTDIR)\refclock_gpsvme.sbr" \
	"$(INTDIR)\refclock_heath.sbr" \
	"$(INTDIR)\refclock_hpgps.sbr" \
	"$(INTDIR)\refclock_irig.sbr" \
	"$(INTDIR)\refclock_jupiter.sbr" \
	"$(INTDIR)\refclock_leitch.sbr" \
	"$(INTDIR)\refclock_local.sbr" \
	"$(INTDIR)\refclock_msfees.sbr" \
	"$(INTDIR)\refclock_mx4200.sbr" \
	"$(INTDIR)\refclock_nmea.sbr" \
	"$(INTDIR)\refclock_oncore.sbr" \
	"$(INTDIR)\refclock_palisade.sbr" \
	"$(INTDIR)\refclock_parse.sbr" \
	"$(INTDIR)\refclock_pst.sbr" \
	"$(INTDIR)\refclock_ptbacts.sbr" \
	"$(INTDIR)\refclock_shm.sbr" \
	"$(INTDIR)\refclock_tpro.sbr" \
	"$(INTDIR)\refclock_trak.sbr" \
	"$(INTDIR)\refclock_true.sbr" \
	"$(INTDIR)\refclock_usno.sbr" \
	"$(INTDIR)\refclock_wwvb.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpd.pdb" /machine:I386 /out:"$(OUTDIR)\ntpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\nt_com.obj" \
	"$(INTDIR)\ntp_config.obj" \
	"$(INTDIR)\ntp_control.obj" \
	"$(INTDIR)\ntp_filegen.obj" \
	"$(INTDIR)\ntp_intres.obj" \
	"$(INTDIR)\ntp_io.obj" \
	"$(INTDIR)\ntp_loopfilter.obj" \
	"$(INTDIR)\ntp_monitor.obj" \
	"$(INTDIR)\ntp_peer.obj" \
	"$(INTDIR)\ntp_proto.obj" \
	"$(INTDIR)\ntp_refclock.obj" \
	"$(INTDIR)\ntp_request.obj" \
	"$(INTDIR)\ntp_restrict.obj" \
	"$(INTDIR)\ntp_timer.obj" \
	"$(INTDIR)\ntp_util.obj" \
	"$(INTDIR)\ntpd.obj" \
	"$(INTDIR)\refclock_acts.obj" \
	"$(INTDIR)\refclock_arbiter.obj" \
	"$(INTDIR)\refclock_arc.obj" \
	"$(INTDIR)\refclock_as2201.obj" \
	"$(INTDIR)\refclock_atom.obj" \
	"$(INTDIR)\refclock_bancomm.obj" \
	"$(INTDIR)\refclock_chu.obj" \
	"$(INTDIR)\refclock_conf.obj" \
	"$(INTDIR)\refclock_datum.obj" \
	"$(INTDIR)\refclock_gpsvme.obj" \
	"$(INTDIR)\refclock_heath.obj" \
	"$(INTDIR)\refclock_hpgps.obj" \
	"$(INTDIR)\refclock_irig.obj" \
	"$(INTDIR)\refclock_jupiter.obj" \
	"$(INTDIR)\refclock_leitch.obj" \
	"$(INTDIR)\refclock_local.obj" \
	"$(INTDIR)\refclock_msfees.obj" \
	"$(INTDIR)\refclock_mx4200.obj" \
	"$(INTDIR)\refclock_nmea.obj" \
	"$(INTDIR)\refclock_oncore.obj" \
	"$(INTDIR)\refclock_palisade.obj" \
	"$(INTDIR)\refclock_parse.obj" \
	"$(INTDIR)\refclock_pst.obj" \
	"$(INTDIR)\refclock_ptbacts.obj" \
	"$(INTDIR)\refclock_shm.obj" \
	"$(INTDIR)\refclock_tpro.obj" \
	"$(INTDIR)\refclock_trak.obj" \
	"$(INTDIR)\refclock_true.obj" \
	"$(INTDIR)\refclock_usno.obj" \
	"$(INTDIR)\refclock_wwvb.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetDir=".\Release"
SOURCE=$(InputPath)
PostBuild_Desc=Copy Executable
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"
   copy ".\Release"\ntpd.exe ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ntpd - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\messages.res"
	-@erase "$(INTDIR)\nt_com.obj"
	-@erase "$(INTDIR)\nt_com.sbr"
	-@erase "$(INTDIR)\ntp_config.obj"
	-@erase "$(INTDIR)\ntp_config.sbr"
	-@erase "$(INTDIR)\ntp_control.obj"
	-@erase "$(INTDIR)\ntp_control.sbr"
	-@erase "$(INTDIR)\ntp_filegen.obj"
	-@erase "$(INTDIR)\ntp_filegen.sbr"
	-@erase "$(INTDIR)\ntp_intres.obj"
	-@erase "$(INTDIR)\ntp_intres.sbr"
	-@erase "$(INTDIR)\ntp_io.obj"
	-@erase "$(INTDIR)\ntp_io.sbr"
	-@erase "$(INTDIR)\ntp_loopfilter.obj"
	-@erase "$(INTDIR)\ntp_loopfilter.sbr"
	-@erase "$(INTDIR)\ntp_monitor.obj"
	-@erase "$(INTDIR)\ntp_monitor.sbr"
	-@erase "$(INTDIR)\ntp_peer.obj"
	-@erase "$(INTDIR)\ntp_peer.sbr"
	-@erase "$(INTDIR)\ntp_proto.obj"
	-@erase "$(INTDIR)\ntp_proto.sbr"
	-@erase "$(INTDIR)\ntp_refclock.obj"
	-@erase "$(INTDIR)\ntp_refclock.sbr"
	-@erase "$(INTDIR)\ntp_request.obj"
	-@erase "$(INTDIR)\ntp_request.sbr"
	-@erase "$(INTDIR)\ntp_restrict.obj"
	-@erase "$(INTDIR)\ntp_restrict.sbr"
	-@erase "$(INTDIR)\ntp_timer.obj"
	-@erase "$(INTDIR)\ntp_timer.sbr"
	-@erase "$(INTDIR)\ntp_util.obj"
	-@erase "$(INTDIR)\ntp_util.sbr"
	-@erase "$(INTDIR)\ntpd.obj"
	-@erase "$(INTDIR)\ntpd.sbr"
	-@erase "$(INTDIR)\refclock_acts.obj"
	-@erase "$(INTDIR)\refclock_acts.sbr"
	-@erase "$(INTDIR)\refclock_arbiter.obj"
	-@erase "$(INTDIR)\refclock_arbiter.sbr"
	-@erase "$(INTDIR)\refclock_arc.obj"
	-@erase "$(INTDIR)\refclock_arc.sbr"
	-@erase "$(INTDIR)\refclock_as2201.obj"
	-@erase "$(INTDIR)\refclock_as2201.sbr"
	-@erase "$(INTDIR)\refclock_atom.obj"
	-@erase "$(INTDIR)\refclock_atom.sbr"
	-@erase "$(INTDIR)\refclock_bancomm.obj"
	-@erase "$(INTDIR)\refclock_bancomm.sbr"
	-@erase "$(INTDIR)\refclock_chu.obj"
	-@erase "$(INTDIR)\refclock_chu.sbr"
	-@erase "$(INTDIR)\refclock_conf.obj"
	-@erase "$(INTDIR)\refclock_conf.sbr"
	-@erase "$(INTDIR)\refclock_datum.obj"
	-@erase "$(INTDIR)\refclock_datum.sbr"
	-@erase "$(INTDIR)\refclock_gpsvme.obj"
	-@erase "$(INTDIR)\refclock_gpsvme.sbr"
	-@erase "$(INTDIR)\refclock_heath.obj"
	-@erase "$(INTDIR)\refclock_heath.sbr"
	-@erase "$(INTDIR)\refclock_hpgps.obj"
	-@erase "$(INTDIR)\refclock_hpgps.sbr"
	-@erase "$(INTDIR)\refclock_irig.obj"
	-@erase "$(INTDIR)\refclock_irig.sbr"
	-@erase "$(INTDIR)\refclock_jupiter.obj"
	-@erase "$(INTDIR)\refclock_jupiter.sbr"
	-@erase "$(INTDIR)\refclock_leitch.obj"
	-@erase "$(INTDIR)\refclock_leitch.sbr"
	-@erase "$(INTDIR)\refclock_local.obj"
	-@erase "$(INTDIR)\refclock_local.sbr"
	-@erase "$(INTDIR)\refclock_msfees.obj"
	-@erase "$(INTDIR)\refclock_msfees.sbr"
	-@erase "$(INTDIR)\refclock_mx4200.obj"
	-@erase "$(INTDIR)\refclock_mx4200.sbr"
	-@erase "$(INTDIR)\refclock_nmea.obj"
	-@erase "$(INTDIR)\refclock_nmea.sbr"
	-@erase "$(INTDIR)\refclock_oncore.obj"
	-@erase "$(INTDIR)\refclock_oncore.sbr"
	-@erase "$(INTDIR)\refclock_palisade.obj"
	-@erase "$(INTDIR)\refclock_palisade.sbr"
	-@erase "$(INTDIR)\refclock_parse.obj"
	-@erase "$(INTDIR)\refclock_parse.sbr"
	-@erase "$(INTDIR)\refclock_pst.obj"
	-@erase "$(INTDIR)\refclock_pst.sbr"
	-@erase "$(INTDIR)\refclock_ptbacts.obj"
	-@erase "$(INTDIR)\refclock_ptbacts.sbr"
	-@erase "$(INTDIR)\refclock_shm.obj"
	-@erase "$(INTDIR)\refclock_shm.sbr"
	-@erase "$(INTDIR)\refclock_tpro.obj"
	-@erase "$(INTDIR)\refclock_tpro.sbr"
	-@erase "$(INTDIR)\refclock_trak.obj"
	-@erase "$(INTDIR)\refclock_trak.sbr"
	-@erase "$(INTDIR)\refclock_true.obj"
	-@erase "$(INTDIR)\refclock_true.sbr"
	-@erase "$(INTDIR)\refclock_usno.obj"
	-@erase "$(INTDIR)\refclock_usno.sbr"
	-@erase "$(INTDIR)\refclock_wwvb.obj"
	-@erase "$(INTDIR)\refclock_wwvb.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpd.bsc"
	-@erase "$(OUTDIR)\ntpd.exe"
	-@erase "$(OUTDIR)\ntpd.ilk"
	-@erase "$(OUTDIR)\ntpd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpd.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\messages.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\nt_com.sbr" \
	"$(INTDIR)\ntp_config.sbr" \
	"$(INTDIR)\ntp_control.sbr" \
	"$(INTDIR)\ntp_filegen.sbr" \
	"$(INTDIR)\ntp_intres.sbr" \
	"$(INTDIR)\ntp_io.sbr" \
	"$(INTDIR)\ntp_loopfilter.sbr" \
	"$(INTDIR)\ntp_monitor.sbr" \
	"$(INTDIR)\ntp_peer.sbr" \
	"$(INTDIR)\ntp_proto.sbr" \
	"$(INTDIR)\ntp_refclock.sbr" \
	"$(INTDIR)\ntp_request.sbr" \
	"$(INTDIR)\ntp_restrict.sbr" \
	"$(INTDIR)\ntp_timer.sbr" \
	"$(INTDIR)\ntp_util.sbr" \
	"$(INTDIR)\ntpd.sbr" \
	"$(INTDIR)\refclock_acts.sbr" \
	"$(INTDIR)\refclock_arbiter.sbr" \
	"$(INTDIR)\refclock_arc.sbr" \
	"$(INTDIR)\refclock_as2201.sbr" \
	"$(INTDIR)\refclock_atom.sbr" \
	"$(INTDIR)\refclock_bancomm.sbr" \
	"$(INTDIR)\refclock_chu.sbr" \
	"$(INTDIR)\refclock_conf.sbr" \
	"$(INTDIR)\refclock_datum.sbr" \
	"$(INTDIR)\refclock_gpsvme.sbr" \
	"$(INTDIR)\refclock_heath.sbr" \
	"$(INTDIR)\refclock_hpgps.sbr" \
	"$(INTDIR)\refclock_irig.sbr" \
	"$(INTDIR)\refclock_jupiter.sbr" \
	"$(INTDIR)\refclock_leitch.sbr" \
	"$(INTDIR)\refclock_local.sbr" \
	"$(INTDIR)\refclock_msfees.sbr" \
	"$(INTDIR)\refclock_mx4200.sbr" \
	"$(INTDIR)\refclock_nmea.sbr" \
	"$(INTDIR)\refclock_oncore.sbr" \
	"$(INTDIR)\refclock_palisade.sbr" \
	"$(INTDIR)\refclock_parse.sbr" \
	"$(INTDIR)\refclock_pst.sbr" \
	"$(INTDIR)\refclock_ptbacts.sbr" \
	"$(INTDIR)\refclock_shm.sbr" \
	"$(INTDIR)\refclock_tpro.sbr" \
	"$(INTDIR)\refclock_trak.sbr" \
	"$(INTDIR)\refclock_true.sbr" \
	"$(INTDIR)\refclock_usno.sbr" \
	"$(INTDIR)\refclock_wwvb.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Debug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpd.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ntpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\messages.res" \
	"$(INTDIR)\nt_com.obj" \
	"$(INTDIR)\ntp_config.obj" \
	"$(INTDIR)\ntp_control.obj" \
	"$(INTDIR)\ntp_filegen.obj" \
	"$(INTDIR)\ntp_intres.obj" \
	"$(INTDIR)\ntp_io.obj" \
	"$(INTDIR)\ntp_loopfilter.obj" \
	"$(INTDIR)\ntp_monitor.obj" \
	"$(INTDIR)\ntp_peer.obj" \
	"$(INTDIR)\ntp_proto.obj" \
	"$(INTDIR)\ntp_refclock.obj" \
	"$(INTDIR)\ntp_request.obj" \
	"$(INTDIR)\ntp_restrict.obj" \
	"$(INTDIR)\ntp_timer.obj" \
	"$(INTDIR)\ntp_util.obj" \
	"$(INTDIR)\ntpd.obj" \
	"$(INTDIR)\refclock_acts.obj" \
	"$(INTDIR)\refclock_arbiter.obj" \
	"$(INTDIR)\refclock_arc.obj" \
	"$(INTDIR)\refclock_as2201.obj" \
	"$(INTDIR)\refclock_atom.obj" \
	"$(INTDIR)\refclock_bancomm.obj" \
	"$(INTDIR)\refclock_chu.obj" \
	"$(INTDIR)\refclock_conf.obj" \
	"$(INTDIR)\refclock_datum.obj" \
	"$(INTDIR)\refclock_gpsvme.obj" \
	"$(INTDIR)\refclock_heath.obj" \
	"$(INTDIR)\refclock_hpgps.obj" \
	"$(INTDIR)\refclock_irig.obj" \
	"$(INTDIR)\refclock_jupiter.obj" \
	"$(INTDIR)\refclock_leitch.obj" \
	"$(INTDIR)\refclock_local.obj" \
	"$(INTDIR)\refclock_msfees.obj" \
	"$(INTDIR)\refclock_mx4200.obj" \
	"$(INTDIR)\refclock_nmea.obj" \
	"$(INTDIR)\refclock_oncore.obj" \
	"$(INTDIR)\refclock_palisade.obj" \
	"$(INTDIR)\refclock_parse.obj" \
	"$(INTDIR)\refclock_pst.obj" \
	"$(INTDIR)\refclock_ptbacts.obj" \
	"$(INTDIR)\refclock_shm.obj" \
	"$(INTDIR)\refclock_tpro.obj" \
	"$(INTDIR)\refclock_trak.obj" \
	"$(INTDIR)\refclock_true.obj" \
	"$(INTDIR)\refclock_usno.obj" \
	"$(INTDIR)\refclock_wwvb.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetDir=".\Debug"
SOURCE=$(InputPath)
PostBuild_Desc=Copy Executable
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpd.exe" "$(OUTDIR)\ntpd.bsc"
   copy ".\Debug"\ntpd.exe ..\package\distrib\Debug
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

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


!IF "$(CFG)" == "ntpd - Release" || "$(CFG)" == "ntpd - Debug"
SOURCE=..\libntp\messages.rc
DEP_RSC_MESSA=\
	"..\libntp\MSG00001.bin"\
	

!IF  "$(CFG)" == "ntpd - Release"


"$(INTDIR)\messages.res" : $(SOURCE) $(DEP_RSC_MESSA) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\messages.res" /i\
 "\ntp-4.0.73a\ports\winnt\libntp" /i "\ntp-4.0.72j\ports\winnt\libntp" /i\
 "\ntp-4.0.70a-export\libntp" /i "N:\ntp-4.0.70a-export\libntp" /d "NDEBUG"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"


"$(INTDIR)\messages.res" : $(SOURCE) $(DEP_RSC_MESSA) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\messages.res" /i\
 "\ntp-4.0.73a\ports\winnt\libntp" /i "\ntp-4.0.72j\ports\winnt\libntp" /i\
 "\ntp-4.0.70a-export\libntp" /i "N:\ntp-4.0.70a-export\libntp" /d "_DEBUG"\
 $(SOURCE)


!ENDIF 

SOURCE=.\nt_com.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NT_CO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\nt_com.obj"	"$(INTDIR)\nt_com.sbr" : $(SOURCE) $(DEP_CPP_NT_CO)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NT_CO=\
	"..\include\config.h"\
	

"$(INTDIR)\nt_com.obj"	"$(INTDIR)\nt_com.sbr" : $(SOURCE) $(DEP_CPP_NT_CO)\
 "$(INTDIR)"


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_config.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_C=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\param.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\sys\wait.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_C=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_config.obj"	"$(INTDIR)\ntp_config.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_C=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\param.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\wait.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_config.obj"	"$(INTDIR)\ntp_config.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_control.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_CO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_CO=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_control.obj"	"$(INTDIR)\ntp_control.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_CO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_control.obj"	"$(INTDIR)\ntp_control.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_filegen.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_F=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_F=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_filegen.obj"	"$(INTDIR)\ntp_filegen.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_F=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_filegen.obj"	"$(INTDIR)\ntp_filegen.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_F) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_intres.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_I=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_I=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_intres.obj"	"$(INTDIR)\ntp_intres.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_I=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_intres.obj"	"$(INTDIR)\ntp_intres.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_io.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_IO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\param.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_IO=\
	"..\..\..\..\sys\sync\queue.h"\
	"..\..\..\..\sys\sync\sema.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_io.obj"	"$(INTDIR)\ntp_io.sbr" : $(SOURCE) $(DEP_CPP_NTP_IO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_IO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_io.obj"	"$(INTDIR)\ntp_io.sbr" : $(SOURCE) $(DEP_CPP_NTP_IO)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_loopfilter.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_L=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_L=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_loopfilter.obj"	"$(INTDIR)\ntp_loopfilter.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_L) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_L=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_loopfilter.obj"	"$(INTDIR)\ntp_loopfilter.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_L) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_monitor.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_M=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_M=\
	"..\..\..\..\sys\sync\queue.h"\
	"..\..\..\..\sys\sync\sema.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_monitor.obj"	"$(INTDIR)\ntp_monitor.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_M=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_monitor.obj"	"$(INTDIR)\ntp_monitor.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_peer.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_P=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_P=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_peer.obj"	"$(INTDIR)\ntp_peer.sbr" : $(SOURCE) $(DEP_CPP_NTP_P)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_P=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_peer.obj"	"$(INTDIR)\ntp_peer.sbr" : $(SOURCE) $(DEP_CPP_NTP_P)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_proto.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_PR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_PR=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_proto.obj"	"$(INTDIR)\ntp_proto.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_PR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_PR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_proto.obj"	"$(INTDIR)\ntp_proto.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_PR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_refclock.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_R=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_R=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_refclock.obj"	"$(INTDIR)\ntp_refclock.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_R) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_R=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_refclock.obj"	"$(INTDIR)\ntp_refclock.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_R) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_request.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_RE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_RE=\
	"..\..\..\..\sys\sync\queue.h"\
	"..\..\..\..\sys\sync\sema.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_request.obj"	"$(INTDIR)\ntp_request.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_RE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_RE=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_request.obj"	"$(INTDIR)\ntp_request.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_RE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_restrict.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_RES=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_RES=\
	"..\..\..\..\sys\sync\queue.h"\
	"..\..\..\..\sys\sync\sema.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_restrict.obj"	"$(INTDIR)\ntp_restrict.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_RES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_RES=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_restrict.obj"	"$(INTDIR)\ntp_restrict.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_RES) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_timer.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_T=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\signal.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_T=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_timer.obj"	"$(INTDIR)\ntp_timer.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_T=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_timer.obj"	"$(INTDIR)\ntp_timer.sbr" : $(SOURCE)\
 $(DEP_CPP_NTP_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntp_util.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTP_U=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\resource.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTP_U=\
	"..\..\..\..\sys\sync\queue.h"\
	"..\..\..\..\sys\sync\sema.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntp_util.obj"	"$(INTDIR)\ntp_util.sbr" : $(SOURCE) $(DEP_CPP_NTP_U)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTP_U=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_filegen.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_if.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\net\if.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntp_util.obj"	"$(INTDIR)\ntp_util.sbr" : $(SOURCE) $(DEP_CPP_NTP_U)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\ntpd.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_NTPD_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\..\..\libntp\log.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\param.h"\
	"..\include\sys\resource.h"\
	"..\include\sys\signal.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\stat.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPD_=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntpd.obj"	"$(INTDIR)\ntpd.sbr" : $(SOURCE) $(DEP_CPP_NTPD_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_NTPD_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\..\..\libntp\log.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntpd.obj"	"$(INTDIR)\ntpd.sbr" : $(SOURCE) $(DEP_CPP_NTPD_)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_acts.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCL=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCL=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_acts.obj"	"$(INTDIR)\refclock_acts.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCL) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCL=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_acts.obj"	"$(INTDIR)\refclock_acts.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCL) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_arbiter.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLO=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_arbiter.obj"	"$(INTDIR)\refclock_arbiter.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLO=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_arbiter.obj"	"$(INTDIR)\refclock_arbiter.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_arc.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOC=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOC=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_arc.obj"	"$(INTDIR)\refclock_arc.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOC=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_arc.obj"	"$(INTDIR)\refclock_arc.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOC) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_as2201.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_as2201.obj"	"$(INTDIR)\refclock_as2201.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_as2201.obj"	"$(INTDIR)\refclock_as2201.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_atom.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_atom.obj"	"$(INTDIR)\refclock_atom.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_atom.obj"	"$(INTDIR)\refclock_atom.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_bancomm.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_B=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_B=\
	"..\..\..\..\etc\conf\h\io.h"\
	"..\..\..\..\etc\conf\machine\vme2.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_bancomm.obj"	"$(INTDIR)\refclock_bancomm.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_B) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_B=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_bancomm.obj"	"$(INTDIR)\refclock_bancomm.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_B) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_chu.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_C=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_C=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_chu.obj"	"$(INTDIR)\refclock_chu.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_C=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_chu.obj"	"$(INTDIR)\refclock_chu.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_conf.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_CO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_CO=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_conf.obj"	"$(INTDIR)\refclock_conf.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_CO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\refclock_conf.obj"	"$(INTDIR)\refclock_conf.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_CO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_datum.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_D=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_D=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_datum.obj"	"$(INTDIR)\refclock_datum.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_D) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_D=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_datum.obj"	"$(INTDIR)\refclock_datum.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_D) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_gpsvme.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_G=\
	"..\..\..\include\gps.h"\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_G=\
	"..\..\..\..\etc\conf\h\io.h"\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_gpsvme.obj"	"$(INTDIR)\refclock_gpsvme.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_G=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_gpsvme.obj"	"$(INTDIR)\refclock_gpsvme.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_G) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_heath.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_H=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_H=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_heath.obj"	"$(INTDIR)\refclock_heath.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_H) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_H=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_heath.obj"	"$(INTDIR)\refclock_heath.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_H) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_hpgps.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_HP=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_HP=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_hpgps.obj"	"$(INTDIR)\refclock_hpgps.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_HP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_HP=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_hpgps.obj"	"$(INTDIR)\refclock_hpgps.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_HP) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_irig.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_I=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_I=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_irig.obj"	"$(INTDIR)\refclock_irig.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_I=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_irig.obj"	"$(INTDIR)\refclock_irig.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_I) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_jupiter.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_J=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\..\..\ntpd\jupiter.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_J=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_jupiter.obj"	"$(INTDIR)\refclock_jupiter.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_J) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_J=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_jupiter.obj"	"$(INTDIR)\refclock_jupiter.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_J) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_leitch.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_L=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_L=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_leitch.obj"	"$(INTDIR)\refclock_leitch.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_L) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_L=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_leitch.obj"	"$(INTDIR)\refclock_leitch.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_L) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_local.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_LO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_LO=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_local.obj"	"$(INTDIR)\refclock_local.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_LO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_LO=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\refclock_local.obj"	"$(INTDIR)\refclock_local.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_LO) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_msfees.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_M=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_M=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_msfees.obj"	"$(INTDIR)\refclock_msfees.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_M=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_msfees.obj"	"$(INTDIR)\refclock_msfees.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_M) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_mx4200.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_MX=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\mx4200.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_MX=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_mx4200.obj"	"$(INTDIR)\refclock_mx4200.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_MX) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_MX=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_mx4200.obj"	"$(INTDIR)\refclock_mx4200.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_MX) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_nmea.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_N=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_N=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_nmea.obj"	"$(INTDIR)\refclock_nmea.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_N) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_N=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_nmea.obj"	"$(INTDIR)\refclock_nmea.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_N) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_oncore.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_O=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_O=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_oncore.obj"	"$(INTDIR)\refclock_oncore.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_O) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_O=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_oncore.obj"	"$(INTDIR)\refclock_oncore.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_O) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_palisade.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_P=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_P=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 

"$(INTDIR)\refclock_palisade.obj"	"$(INTDIR)\refclock_palisade.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_P) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_P=\
	"..\include\config.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I\
 "..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpd.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\refclock_palisade.obj"	"$(INTDIR)\refclock_palisade.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_P) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_parse.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_PA=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\..\..\include\parse.h"\
	"..\..\..\include\parse_conf.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_PA=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_parse.obj"	"$(INTDIR)\refclock_parse.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_PA=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_parse.obj"	"$(INTDIR)\refclock_parse.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PA) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_pst.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_PS=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_PS=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_pst.obj"	"$(INTDIR)\refclock_pst.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_PS=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_pst.obj"	"$(INTDIR)\refclock_pst.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PS) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_ptbacts.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_PT=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\..\..\ntpd\refclock_acts.c"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_PT=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_ptbacts.obj"	"$(INTDIR)\refclock_ptbacts.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PT) "$(INTDIR)" "..\..\..\ntpd\refclock_acts.c"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_PT=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_ptbacts.obj"	"$(INTDIR)\refclock_ptbacts.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_PT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_shm.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_S=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_S=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_shm.obj"	"$(INTDIR)\refclock_shm.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_S=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_shm.obj"	"$(INTDIR)\refclock_shm.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_S) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_tpro.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_T=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_T=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	"..\..\..\ntpd\sys\tpro.h"\
	

"$(INTDIR)\refclock_tpro.obj"	"$(INTDIR)\refclock_tpro.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_T=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_tpro.obj"	"$(INTDIR)\refclock_tpro.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_T) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_trak.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_TR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_TR=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_trak.obj"	"$(INTDIR)\refclock_trak.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_TR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_TR=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_trak.obj"	"$(INTDIR)\refclock_trak.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_TR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_true.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_TRU=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_TRU=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_true.obj"	"$(INTDIR)\refclock_true.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_TRU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_TRU=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_true.obj"	"$(INTDIR)\refclock_true.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_TRU) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_usno.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_U=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_U=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_usno.obj"	"$(INTDIR)\refclock_usno.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_U) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_U=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_usno.obj"	"$(INTDIR)\refclock_usno.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_U) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpd\refclock_wwvb.c

!IF  "$(CFG)" == "ntpd - Release"

DEP_CPP_REFCLOCK_W=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntpd.h"\
	"..\include\config.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_REFCLOCK_W=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\refclock_wwvb.obj"	"$(INTDIR)\refclock_wwvb.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_W) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpd - Debug"

DEP_CPP_REFCLOCK_W=\
	"..\include\config.h"\
	

"$(INTDIR)\refclock_wwvb.obj"	"$(INTDIR)\refclock_wwvb.sbr" : $(SOURCE)\
 $(DEP_CPP_REFCLOCK_W) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

