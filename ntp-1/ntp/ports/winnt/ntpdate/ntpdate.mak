# Microsoft Developer Studio Generated NMAKE File, Based on ntpdate.dsp
!IF "$(CFG)" == ""
CFG=ntpdate - Release
!MESSAGE No configuration specified. Defaulting to ntpdate - Release.
!ENDIF 

!IF "$(CFG)" != "ntpdate - Release" && "$(CFG)" != "ntpdate - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpdate.mak" CFG="ntpdate - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpdate - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntpdate - Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "ntpdate - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpdate.obj"
	-@erase "$(INTDIR)\ntpdate.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpdate.bsc"
	-@erase "$(OUTDIR)\ntpdate.exe"
	-@erase "$(OUTDIR)\ntpdate.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "__STDC__" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpdate.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpdate.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdate.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpdate.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpdate.pdb" /machine:I386 /out:"$(OUTDIR)\ntpdate.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdate.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpdate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"
   copy ".\Release"\ntpdate.exe  ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ntpdate - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpdate.obj"
	-@erase "$(INTDIR)\ntpdate.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpdate.bsc"
	-@erase "$(OUTDIR)\ntpdate.exe"
	-@erase "$(OUTDIR)\ntpdate.ilk"
	-@erase "$(OUTDIR)\ntpdate.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "__STDC__" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpdate.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpdate.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdate.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpdate.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib wsock32.lib ..\libntp\Debug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpdate.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ntpdate.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdate.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpdate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpdate.exe" "$(OUTDIR)\ntpdate.bsc"
   copy ".\Debug"\ntpdate.exe  ..\package\distrib\Debug
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


!IF "$(CFG)" == "ntpdate - Release" || "$(CFG)" == "ntpdate - Debug"
SOURCE=..\..\..\ntpdate\ntpdate.c

!IF  "$(CFG)" == "ntpdate - Release"

DEP_CPP_NTPDA=\
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
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntpdate\ntpdate.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\resource.h"\
	"..\include\sys\signal.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	"..\include\syslog.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPDA=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	"..\..\..\ntpdate\ioLib.h"\
	"..\..\..\ntpdate\sockLib.h"\
	"..\..\..\ntpdate\timers.h"\
	

"$(INTDIR)\ntpdate.obj"	"$(INTDIR)\ntpdate.sbr" : $(SOURCE) $(DEP_CPP_NTPDA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpdate - Debug"

DEP_CPP_NTPDA=\
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
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntpdate\ntpdate.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntpdate.obj"	"$(INTDIR)\ntpdate.sbr" : $(SOURCE) $(DEP_CPP_NTPDA)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

