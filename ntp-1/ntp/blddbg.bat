@echo off
REM
REM File: blddbg.bat
REM Purpose: To compile the NTP source tree for Win NT.
REM Date: 05/03/1996
REM
@echo on

@echo -----------------------------------------
@echo Building event logging Resources
@echo -----------------------------------------
@cd libntp
call makemc
@echo -----------------------------------------
@echo Compiling the support library
@echo -----------------------------------------
nmake  /f libntp.mak CFG="libntp - Win32 Debug"

@echo -----------------------------------------
@echo Compiling ntpq program
@echo -----------------------------------------
@cd ..\ntpq
perl ..\scripts\mkver.bat -P ntpq
nmake  /f ntpq.mak CFG="ntpq - Win32 Debug"

@echo -----------------------------------------
@echo Compiling ntpdate program
@echo -----------------------------------------
@cd ..\ntpdate
perl ..\scripts\mkver.bat -P ntpdate
nmake  /f ntpdate.mak CFG="ntpdate - Win32 Debug"

@echo -----------------------------------------
@echo Compiling ntptrace program
@echo -----------------------------------------
@cd ..\ntptrace
perl ..\scripts\mkver.bat -P ntptrace
nmake  /f ntptrace.mak CFG="ntptrace - Win32 Debug"

@echo -----------------------------------------
@echo Compiling the Network Time Protocol server
@echo -----------------------------------------
@cd ..\ntpd
perl ..\scripts\mkver.bat -P ntpd
nmake  /f ntpd.mak CFG="ntpd - Win32 Debug"

@echo -----------------------------------------
@echo Compiling the NTP server control program
@echo -----------------------------------------
@cd ..\ntpdc
perl ..\scripts\mkver.bat -P ntpdc
nmake  /f ntpdc.mak CFG="ntpdc - Win32 Debug"

@echo -----------------------------------------
@echo Compiling the service installer
@echo -----------------------------------------
@cd ..\scripts\wininstall\instsrv
nmake  /f instsrv.mak CFG="instsrv - Win32 Debug"

@echo -----------------------------------------
@echo Compiling the InstallShield support DLL
@echo -----------------------------------------
@cd ..
nmake  /f ntpdll.mak CFG="ntpdll - Win32 Debug"

@echo -----------------------------------------
@echo Now copying all executables to the scripts\wininstall\distrib directory
@echo -----------------------------------------
@cd ..\..
copy ntpd\windebug\*.exe    scripts\wininstall\distrib
copy ntpdc\windebug\*.exe   scripts\wininstall\distrib
copy ntpq\windebug\*.exe     scripts\wininstall\distrib
copy ntpdate\windebug\*.exe  scripts\wininstall\distrib
copy ntptrace\windebug\*.exe scripts\wininstall\distrib
copy scripts\wininstall\instsrv\windebug\*.exe  scripts\wininstall\distrib
copy scripts\wininstall\windebug\*.dll  scripts\wininstall
@echo -----------------------------------------
@echo -----------------------------------------
@echo

@echo -----------------------------------------
@echo Building InstallShield Package
@echo -----------------------------------------
@cd scripts\wininstall
build.bat

@echo
@echo -----------------------------------------
@echo Build Complete 
@echo Run install.bat in scripts\wininstall\distrib 
@echo to complete installation.
@echo -----------------------------------------
