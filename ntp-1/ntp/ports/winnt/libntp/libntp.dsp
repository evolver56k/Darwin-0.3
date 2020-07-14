# Microsoft Developer Studio Project File - Name="libntp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libntp - Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libntp.mak".
!MESSAGE 
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

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "libntp - Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "SYS_WINNT" /D "__STDC__" /D "HAVE_CONFIG_H" /FR /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libntp - Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# ADD BASE CPP /nologo /ML /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_WINDOWS" /D "SYS_WINNT" /D "__STDC__" /D "HAVE_CONFIG_H" /FR /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libntp - Release"
# Name "libntp - Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\..\..\libntp\a_md5encrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\atoint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\atolfp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\atouint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\authencrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\authkeys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\authparity.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\authreadkeys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\authusekey.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\buftvtots.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\caljulian.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\calleapwhen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\caltontp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\calyearstart.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\clocktime.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\clocktypes.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\decodenetnum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\dofptoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\dolfptoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\emalloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\findconfig.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\fptoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\fptoms.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\getopt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\hextoint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\hextolfp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\humandate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\inttoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\lib_strbuf.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\machines.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\md5c.c
# End Source File
# Begin Source File

SOURCE=.\messages.rc

!IF  "$(CFG)" == "libntp - Release"

!ELSEIF  "$(CFG)" == "libntp - Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Make MC
InputPath=.\messages.rc

"messages.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc messages.mc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\mexit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\mfptoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\mfptoms.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\modetoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\mstolfp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\msutotsf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\msyslog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\netof.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\numtoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\numtohost.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\octtoint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\prettydate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\ranny.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\refnumtoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\statestr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\syssignal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\systime.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\tsftomsu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\tstotv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\tvtoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\tvtots.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\uglydate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\uinttoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\utvtoa.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\..\..\libntp\lib_strbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\libntp\log.h
# End Source File
# Begin Source File

SOURCE=.\messages.h
USERDEP__MESSA="messages.rc"	

!IF  "$(CFG)" == "libntp - Release"

!ELSEIF  "$(CFG)" == "libntp - Debug"

# Begin Custom Build - Make MC
InputPath=.\messages.h

"messages.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc messages.mc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
