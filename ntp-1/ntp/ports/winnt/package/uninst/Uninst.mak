# Microsoft Developer Studio Generated NMAKE File, Based on Uninst.dsp
!IF "$(CFG)" == ""
CFG=Uninst - Release
!MESSAGE No configuration specified. Defaulting to Uninst - Release.
!ENDIF 

!IF "$(CFG)" != "Uninst - Release" && "$(CFG)" != "Uninst - Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Uninst.mak" CFG="Uninst - Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Uninst - Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Uninst - Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Uninst - Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Uninst.dll"

!ELSE 

ALL : "$(OUTDIR)\Uninst.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\uninst.obj"
	-@erase "$(INTDIR)\Uninst.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\Uninst.dll"
	-@erase "$(OUTDIR)\Uninst.exp"
	-@erase "$(OUTDIR)\Uninst.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\Uninst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Uninst.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Uninst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\Uninst.pdb" /machine:I386 /def:".\uninst.def"\
 /out:"$(OUTDIR)\Uninst.dll" /implib:"$(OUTDIR)\Uninst.lib" 
DEF_FILE= \
	".\uninst.def"
LINK32_OBJS= \
	"$(INTDIR)\uninst.obj" \
	"$(INTDIR)\Uninst.res"

"$(OUTDIR)\Uninst.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetDir=".\Release"
SOURCE=$(InputPath)
PostBuild_Desc=Copy DLL
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\Uninst.dll"
   copy ".\Release"\uninst.dll ..\distrib\Release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "Uninst - Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Uninst.dll"

!ELSE 

ALL : "$(OUTDIR)\Uninst.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\uninst.obj"
	-@erase "$(INTDIR)\Uninst.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Uninst.dll"
	-@erase "$(OUTDIR)\Uninst.exp"
	-@erase "$(OUTDIR)\Uninst.ilk"
	-@erase "$(OUTDIR)\Uninst.lib"
	-@erase "$(OUTDIR)\Uninst.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\Uninst.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Uninst.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Uninst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\Uninst.pdb" /debug /machine:I386 /def:".\uninst.def"\
 /out:"$(OUTDIR)\Uninst.dll" /implib:"$(OUTDIR)\Uninst.lib" 
DEF_FILE= \
	".\uninst.def"
LINK32_OBJS= \
	"$(INTDIR)\uninst.obj" \
	"$(INTDIR)\Uninst.res"

"$(OUTDIR)\Uninst.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetDir=".\Debug"
SOURCE=$(InputPath)
PostBuild_Desc=Copy DLL
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\Uninst.dll"
   copy ".\Debug"\uninst.dll  ..\distrib\Debug
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


!IF "$(CFG)" == "Uninst - Release" || "$(CFG)" == "Uninst - Debug"
SOURCE=.\uninst.cpp

"$(INTDIR)\uninst.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Uninst.rc

"$(INTDIR)\Uninst.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

