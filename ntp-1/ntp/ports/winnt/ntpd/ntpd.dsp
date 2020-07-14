# Microsoft Developer Studio Project File - Name="ntpd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ntpd - Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ntpd.mak".
!MESSAGE 
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

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ntpd - Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /machine:I386
# Begin Special Build Tool
TargetDir=".\Release"
SOURCE=$(InputPath)
PostBuild_Desc=Copy Executable
PostBuild_Cmds=copy $(TargetDir)\ntpd.exe ..\package\distrib\Release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ntpd - Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# ADD BASE CPP /nologo /ML /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib winmm.lib ..\libntp\Debug\libntp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
# Begin Special Build Tool
TargetDir=".\Debug"
SOURCE=$(InputPath)
PostBuild_Desc=Copy Executable
PostBuild_Cmds=copy $(TargetDir)\ntpd.exe ..\package\distrib\Debug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ntpd - Release"
# Name "ntpd - Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\libntp\messages.rc
# ADD BASE RSC /l 0x409 /i "\ntp-4.0.73a\ports\winnt\libntp" /i "\ntp-4.0.72j\ports\winnt\libntp" /i "\ntp-4.0.70a-export\libntp"
# ADD RSC /l 0x409 /i "\ntp-4.0.73a\ports\winnt\libntp" /i "\ntp-4.0.72j\ports\winnt\libntp" /i "\ntp-4.0.70a-export\libntp" /i "N:\ntp-4.0.70a-export\libntp"
# End Source File
# Begin Source File

SOURCE=.\nt_com.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_config.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_control.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_filegen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_intres.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_io.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_loopfilter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_monitor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_peer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_proto.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_refclock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_request.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_restrict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_timer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntp_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\ntpd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_acts.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_arbiter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_arc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_as2201.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_atom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_bancomm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_chu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_conf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_datum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_gpsvme.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_heath.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_hpgps.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_irig.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_jupiter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_leitch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_local.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_msfees.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_mx4200.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_nmea.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_oncore.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_palisade.c
# ADD CPP /D "HAVE_CONFIG_H"
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_parse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_pst.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_ptbacts.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_shm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_tpro.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_trak.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_true.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_usno.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ntpd\refclock_wwvb.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
