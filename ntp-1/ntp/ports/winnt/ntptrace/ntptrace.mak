# Microsoft Developer Studio Generated NMAKE File, Based on ntptrace.dsp
!IF "$(CFG)" == ""
CFG=ntptrace - Release
!MESSAGE No configuration specified. Defaulting to ntptrace - Release.
!ENDIF 

!IF "$(CFG)" != "ntptrace - Release" && "$(CFG)" != "ntptrace - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntptrace.mak" CFG="ntptrace - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntptrace - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntptrace - Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "ntptrace - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntptrace.obj"
	-@erase "$(INTDIR)\ntptrace.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntptrace.bsc"
	-@erase "$(OUTDIR)\ntptrace.exe"
	-@erase "$(OUTDIR)\ntptrace.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntptrace.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntptrace.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntptrace.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntptrace.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntptrace.pdb" /machine:I386 /out:"$(OUTDIR)\ntptrace.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntptrace.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntptrace.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"
   copy ".\Release"\ntptrace.exe  ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ntptrace - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntptrace.obj"
	-@erase "$(INTDIR)\ntptrace.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntptrace.bsc"
	-@erase "$(OUTDIR)\ntptrace.exe"
	-@erase "$(OUTDIR)\ntptrace.ilk"
	-@erase "$(OUTDIR)\ntptrace.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntptrace.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntptrace.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntptrace.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntptrace.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Debug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntptrace.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\ntptrace.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntptrace.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntptrace.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntptrace.exe" "$(OUTDIR)\ntptrace.bsc"
   copy ".\Debug"\ntptrace.exe  ..\package\distrib\Debug
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


!IF "$(CFG)" == "ntptrace - Release" || "$(CFG)" == "ntptrace - Debug"
SOURCE=..\..\..\ntptrace\ntptrace.c

!IF  "$(CFG)" == "ntptrace - Release"

DEP_CPP_NTPTR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntptrace\ntptrace.h"\
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
	
NODEP_CPP_NTPTR=\
	"..\..\..\include\gizmo_syslog.h"\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntptrace.obj"	"$(INTDIR)\ntptrace.sbr" : $(SOURCE) $(DEP_CPP_NTPTR)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntptrace - Debug"

DEP_CPP_NTPTR=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_syslog.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntptrace\ntptrace.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\ioctl.h"\
	"..\include\sys\resource.h"\
	"..\include\sys\socket.h"\
	"..\include\syslog.h"\
	

"$(INTDIR)\ntptrace.obj"	"$(INTDIR)\ntptrace.sbr" : $(SOURCE) $(DEP_CPP_NTPTR)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

