# Microsoft Developer Studio Generated NMAKE File, Based on ntpq.dsp
!IF "$(CFG)" == ""
CFG=ntpq - Release
!MESSAGE No configuration specified. Defaulting to ntpq - Release.
!ENDIF 

!IF "$(CFG)" != "ntpq - Release" && "$(CFG)" != "ntpq - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpq.mak" CFG="ntpq - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpq - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntpq - Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "ntpq - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpq.obj"
	-@erase "$(INTDIR)\ntpq.sbr"
	-@erase "$(INTDIR)\ntpq_ops.obj"
	-@erase "$(INTDIR)\ntpq_ops.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpq.bsc"
	-@erase "$(OUTDIR)\ntpq.exe"
	-@erase "$(OUTDIR)\ntpq.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\include\winnt" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpq.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpq.sbr" \
	"$(INTDIR)\ntpq_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpq.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpq.pdb" /machine:I386 /out:"$(OUTDIR)\ntpq.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpq.obj" \
	"$(INTDIR)\ntpq_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpq.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"
   copy ".\Release"\ntpq.exe ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ntpq - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpq.obj"
	-@erase "$(INTDIR)\ntpq.sbr"
	-@erase "$(INTDIR)\ntpq_ops.obj"
	-@erase "$(INTDIR)\ntpq_ops.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpq.bsc"
	-@erase "$(OUTDIR)\ntpq.exe"
	-@erase "$(OUTDIR)\ntpq.ilk"
	-@erase "$(OUTDIR)\ntpq.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\include\winnt"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpq.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpq.sbr" \
	"$(INTDIR)\ntpq_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpq.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Debug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpq.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ntpq.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpq.obj" \
	"$(INTDIR)\ntpq_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpq.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpq.exe" "$(OUTDIR)\ntpq.bsc"
   copy ".\Debug"\ntpq.exe ..\package\distrib\Debug
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


!IF "$(CFG)" == "ntpq - Release" || "$(CFG)" == "ntpq - Debug"
SOURCE=..\..\..\ntpq\ntpq.c

!IF  "$(CFG)" == "ntpq - Release"

DEP_CPP_NTPQ_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntpq\ntpq.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPQ_=\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 

"$(INTDIR)\ntpq.obj"	"$(INTDIR)\ntpq.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ntpq - Debug"

DEP_CPP_NTPQ_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_calendar.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_io.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_select.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\include\ntp_unixtime.h"\
	"..\..\..\ntpq\ntpq.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I\
 "..\..\..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "SYS_WINNT" /D "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ntpq.obj"	"$(INTDIR)\ntpq.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\..\..\ntpq\ntpq_ops.c

!IF  "$(CFG)" == "ntpq - Release"

DEP_CPP_NTPQ_O=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpq\ntpq.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPQ_O=\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 

"$(INTDIR)\ntpq_ops.obj"	"$(INTDIR)\ntpq_ops.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_O)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ntpq - Debug"

DEP_CPP_NTPQ_O=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpq\ntpq.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I\
 "..\..\..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "SYS_WINNT" /D "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ntpq_ops.obj"	"$(INTDIR)\ntpq_ops.sbr" : $(SOURCE) $(DEP_CPP_NTPQ_O)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\version.c

!IF  "$(CFG)" == "ntpq - Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ntpq - Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I\
 "..\..\..\include" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "SYS_WINNT" /D "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpq.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

