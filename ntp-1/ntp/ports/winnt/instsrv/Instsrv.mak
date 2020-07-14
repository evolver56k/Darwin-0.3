# Microsoft Developer Studio Generated NMAKE File, Based on Instsrv.dsp
!IF "$(CFG)" == ""
CFG=instsrv - Release
!MESSAGE No configuration specified. Defaulting to instsrv - Release.
!ENDIF 

!IF "$(CFG)" != "instsrv - Release" && "$(CFG)" != "instsrv - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Instsrv.mak" CFG="instsrv - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "instsrv - Release" (based on "Win32 (x86) Console Application")
!MESSAGE "instsrv - Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "instsrv - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"

!ELSE 

ALL : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\instsrv.obj"
	-@erase "$(INTDIR)\instsrv.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\Instsrv.bsc"
	-@erase "$(OUTDIR)\Instsrv.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Instsrv.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Instsrv.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\instsrv.sbr"

"$(OUTDIR)\Instsrv.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /incremental:no /pdb:"$(OUTDIR)\Instsrv.pdb" /machine:I386\
 /out:"$(OUTDIR)\Instsrv.exe" 
LINK32_OBJS= \
	"$(INTDIR)\instsrv.obj"

"$(OUTDIR)\Instsrv.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"
   copy ".\Release"\instsrv.exe ..\package\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "instsrv - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"

!ELSE 

ALL : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\instsrv.obj"
	-@erase "$(INTDIR)\instsrv.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Instsrv.bsc"
	-@erase "$(OUTDIR)\Instsrv.exe"
	-@erase "$(OUTDIR)\Instsrv.ilk"
	-@erase "$(OUTDIR)\Instsrv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Instsrv.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Instsrv.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\instsrv.sbr"

"$(OUTDIR)\Instsrv.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\Instsrv.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)\Instsrv.exe" 
LINK32_OBJS= \
	"$(INTDIR)\instsrv.obj"

"$(OUTDIR)\Instsrv.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\Instsrv.exe" "$(OUTDIR)\Instsrv.bsc"
   copy ".\Debug"\instsrv.exe ..\package\distrib\Debug
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(CFG)" == "instsrv - Release" || "$(CFG)" == "instsrv - Debug"
SOURCE=.\instsrv.c

"$(INTDIR)\instsrv.obj"	"$(INTDIR)\instsrv.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

