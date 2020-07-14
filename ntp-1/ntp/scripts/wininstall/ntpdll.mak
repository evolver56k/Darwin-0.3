# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=ntpdll - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ntpdll - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ntpdll - Win32 Release" && "$(CFG)" != "ntpdll - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntpdll.mak" CFG="ntpdll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntpdll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ntpdll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "ntpdll - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "ntpdll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\ntpdll.dll" "$(OUTDIR)\ntpdll.bsc"

CLEAN : 
	-@erase ".\WinRel\ntpdll.bsc"
	-@erase ".\WinRel\ntpdll.sbr"
	-@erase ".\WinRel\ntpdll.dll"
	-@erase ".\WinRel\ntpdll.obj"
	-@erase ".\WinRel\ntpdll.lib"
	-@erase ".\WinRel\ntpdll.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\compat\include" /D "NDEBUG" /D "WIN32" /D "WINNT" /D "_USRDLL" /D "_MBCS" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\compat\include" /D\
 "NDEBUG" /D "WIN32" /D "WINNT" /D "_USRDLL" /D "_MBCS" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/ntpdll.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntpdll.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ntpdll.sbr"

"$(OUTDIR)\ntpdll.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/ntpdll.pdb"\
 /machine:I386 /def:".\ntpdll.def" /out:"$(OUTDIR)/ntpdll.dll"\
 /implib:"$(OUTDIR)/ntpdll.lib" 
DEF_FILE= \
	".\ntpdll.def"
LINK32_OBJS= \
	"$(INTDIR)/ntpdll.obj"

"$(OUTDIR)\ntpdll.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ntpdll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\ntpdll.dll" "$(OUTDIR)\ntpdll.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\WinDebug\ntpdll.bsc"
	-@erase ".\WinDebug\ntpdll.sbr"
	-@erase ".\WinDebug\ntpdll.dll"
	-@erase ".\WinDebug\ntpdll.obj"
	-@erase ".\WinDebug\ntpdll.ilk"
	-@erase ".\WinDebug\ntpdll.lib"
	-@erase ".\WinDebug\ntpdll.exp"
	-@erase ".\WinDebug\ntpdll.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "..\compat\include" /D "_DEBUG" /D "WIN32" /D "WINNT" /D "_USRDLL" /D "_MBCS" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I\
 "..\compat\include" /D "_DEBUG" /D "WIN32" /D "WINNT" /D "_USRDLL" /D "_MBCS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/ntpdll.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntpdll.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ntpdll.sbr"

"$(OUTDIR)\ntpdll.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/ntpdll.pdb" /debug\
 /machine:I386 /def:".\ntpdll.def" /out:"$(OUTDIR)/ntpdll.dll"\
 /implib:"$(OUTDIR)/ntpdll.lib" 
DEF_FILE= \
	".\ntpdll.def"
LINK32_OBJS= \
	"$(INTDIR)/ntpdll.obj"

"$(OUTDIR)\ntpdll.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "ntpdll - Win32 Release"
# Name "ntpdll - Win32 Debug"

!IF  "$(CFG)" == "ntpdll - Win32 Release"

!ELSEIF  "$(CFG)" == "ntpdll - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ntpdll.def

!IF  "$(CFG)" == "ntpdll - Win32 Release"

!ELSEIF  "$(CFG)" == "ntpdll - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ntpdll.c
DEP_CPP_NTPDL=\
	"..\..\libntp\log.h"\
	

"$(INTDIR)\ntpdll.obj" : $(SOURCE) $(DEP_CPP_NTPDL) "$(INTDIR)"

"$(INTDIR)\ntpdll.sbr" : $(SOURCE) $(DEP_CPP_NTPDL) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
