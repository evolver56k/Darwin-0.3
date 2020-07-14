# Microsoft Developer Studio Generated NMAKE File, Based on ntpdc.dsp
!IF "$(CFG)" == ""
CFG=ntpdc - Release
!MESSAGE No configuration specified. Defaulting to ntpdc - Release.
!ENDIF 

!IF "$(CFG)" != "ntpdc - Release" && "$(CFG)" != "ntpdc - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpdc.mak" CFG="ntpdc - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpdc - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntpdc - Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "ntpdc - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpdc.obj"
	-@erase "$(INTDIR)\ntpdc.sbr"
	-@erase "$(INTDIR)\ntpdc_ops.obj"
	-@erase "$(INTDIR)\ntpdc_ops.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpdc.bsc"
	-@erase "$(OUTDIR)\ntpdc.exe"
	-@erase "$(OUTDIR)\ntpdc.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\include" /I "..\include" /D\
 "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D "HAVE_CONFIG_H"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpdc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpdc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdc.sbr" \
	"$(INTDIR)\ntpdc_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpdc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Release\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpdc.pdb" /machine:I386 /out:"$(OUTDIR)\ntpdc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdc.obj" \
	"$(INTDIR)\ntpdc_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpdc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"
   copy ".\Release"\ntpdc.exe  ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ntpdc - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"

!ELSE 

ALL : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ntpdc.obj"
	-@erase "$(INTDIR)\ntpdc.sbr"
	-@erase "$(INTDIR)\ntpdc_ops.obj"
	-@erase "$(INTDIR)\ntpdc_ops.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\ntpdc.bsc"
	-@erase "$(OUTDIR)\ntpdc.exe"
	-@erase "$(OUTDIR)\ntpdc.ilk"
	-@erase "$(OUTDIR)\ntpdc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\include"\
 /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /D "SYS_WINNT" /D\
 "HAVE_CONFIG_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ntpdc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ntpdc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ntpdc.sbr" \
	"$(INTDIR)\ntpdc_ops.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\ntpdc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib winmm.lib ..\libntp\Debug\libntp.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ntpdc.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ntpdc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ntpdc.obj" \
	"$(INTDIR)\ntpdc_ops.obj" \
	"$(INTDIR)\version.obj"

"$(OUTDIR)\ntpdc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ntpdc.exe" "$(OUTDIR)\ntpdc.bsc"
   copy ".\Debug"\ntpdc.exe ..\package\distrib\Debug
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


!IF "$(CFG)" == "ntpdc - Release" || "$(CFG)" == "ntpdc - Debug"
SOURCE=..\..\..\ntpdc\ntpdc.c

!IF  "$(CFG)" == "ntpdc - Release"

DEP_CPP_NTPDC=\
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
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpdc\ntpdc.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPDC=\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntpdc.obj"	"$(INTDIR)\ntpdc.sbr" : $(SOURCE) $(DEP_CPP_NTPDC)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpdc - Debug"

DEP_CPP_NTPDC=\
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
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpdc\ntpdc.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\ntpdc.obj"	"$(INTDIR)\ntpdc.sbr" : $(SOURCE) $(DEP_CPP_NTPDC)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\..\ntpdc\ntpdc_ops.c

!IF  "$(CFG)" == "ntpdc - Release"

DEP_CPP_NTPDC_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpdc\ntpdc.h"\
	"..\include\arpa\inet.h"\
	"..\include\config.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	"..\include\sys\time.h"\
	{$(INCLUDE)}"sys\timeb.h"\
	{$(INCLUDE)}"sys\types.h"\
	
NODEP_CPP_NTPDC_=\
	"..\..\..\include\ioLib.h"\
	"..\..\..\include\taskLib.h"\
	"..\..\..\include\vxWorks.h"\
	

"$(INTDIR)\ntpdc_ops.obj"	"$(INTDIR)\ntpdc_ops.sbr" : $(SOURCE)\
 $(DEP_CPP_NTPDC_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ntpdc - Debug"

DEP_CPP_NTPDC_=\
	"..\..\..\include\l_stdlib.h"\
	"..\..\..\include\ntp.h"\
	"..\..\..\include\ntp_control.h"\
	"..\..\..\include\ntp_fp.h"\
	"..\..\..\include\ntp_machine.h"\
	"..\..\..\include\ntp_malloc.h"\
	"..\..\..\include\ntp_proto.h"\
	"..\..\..\include\ntp_refclock.h"\
	"..\..\..\include\ntp_request.h"\
	"..\..\..\include\ntp_stdlib.h"\
	"..\..\..\include\ntp_string.h"\
	"..\..\..\include\ntp_types.h"\
	"..\..\..\ntpdc\ntpdc.h"\
	"..\include\arpa\inet.h"\
	"..\include\netdb.h"\
	"..\include\netinet\in.h"\
	"..\include\sys\socket.h"\
	

"$(INTDIR)\ntpdc_ops.obj"	"$(INTDIR)\ntpdc_ops.sbr" : $(SOURCE)\
 $(DEP_CPP_NTPDC_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

