@echo off
REM
REM File: bldrel.bat
REM Purpose: To compile the NTP source tree for Win NT.
REM Date: 05/03/1996
REM

mkdir package\distrib\Release
@echo on

@echo -----------------------------------------
@echo Building event logging Resources
@echo -----------------------------------------
@cd libntp
call makemc
@echo -----------------------------------------
@echo Compiling the support library
@echo -----------------------------------------
nmake  /f libntp.mak CFG="libntp - Release"

@echo -----------------------------------------
@echo Compiling ntpq program
@echo -----------------------------------------
@cd ..\ntpq
perl ..\scripts\mkver.bat -P ntpq
nmake  /f ntpq.mak CFG="ntpq - Release"

@echo -----------------------------------------
@echo Compiling ntpdate program
@echo -----------------------------------------
@cd ..\ntpdate
perl ..\scripts\mkver.bat -P ntpdate
nmake  /f ntpdate.mak CFG="ntpdate - Release"

@echo -----------------------------------------
@echo Compiling ntptrace program
@echo -----------------------------------------
@cd ..\ntptrace
perl ..\scripts\mkver.bat -P ntptrace
nmake  /f ntptrace.mak CFG="ntptrace - Release"

@echo -----------------------------------------
@echo Compiling the Network Time Protocol server
@echo -----------------------------------------
@cd ..\ntpd
perl ..\scripts\mkver.bat -P ntpd
nmake  /f ntpd.mak CFG="ntpd - Release"

@echo -----------------------------------------
@echo Compiling the NTP server control program
@echo -----------------------------------------
@cd ..\ntpdc
perl ..\scripts\mkver.bat -P ntpdc
nmake  /f ntpdc.mak CFG="ntpdc - Release"

@echo -----------------------------------------
@echo Compiling the service installer
@echo -----------------------------------------
@cd ..\instsrv
nmake  /f instsrv.mak CFG="instsrv - Release"

@echo -----------------------------------------
@echo Compiling the InstallShield support DLL
@echo -----------------------------------------
@cd ..\package\uninst
nmake  /f uninst.mak CFG="uninst - Release"
@cd ..

@echo off
REM @echo -----------------------------------------
REM @echo Building InstallShield Package
REM @echo -----------------------------------------
REM build-package.bat

@echo
@echo -----------------------------------------
@echo Build Complete 
@echo Run install.bat in package\distrib 
@echo to complete installation.
@echo -----------------------------------------
