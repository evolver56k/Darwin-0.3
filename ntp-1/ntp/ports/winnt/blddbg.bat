@echo off
REM
REM File: blddbg.bat
REM Purpose: To compile the NTP source tree for Win NT.
REM Date: 05/03/1996
REM

mkdir package\distrib\Debug
@echo on

@echo -----------------------------------------
@echo Building event logging Resources
@echo -----------------------------------------
@cd libntp
call makemc
@echo -----------------------------------------
@echo Compiling the support library
@echo -----------------------------------------
nmake  /f libntp.mak CFG="libntp - Debug"

@echo -----------------------------------------
@echo Compiling ntpq program
@echo -----------------------------------------
@cd ..\ntpq
perl ..\scripts\mkver.bat -P ntpq
nmake  /f ntpq.mak CFG="ntpq - Debug"

@echo -----------------------------------------
@echo Compiling ntpdate program
@echo -----------------------------------------
@cd ..\ntpdate
perl ..\scripts\mkver.bat -P ntpdate
nmake  /f ntpdate.mak CFG="ntpdate - Debug"

@echo -----------------------------------------
@echo Compiling ntptrace program
@echo -----------------------------------------
@cd ..\ntptrace
perl ..\scripts\mkver.bat -P ntptrace
nmake  /f ntptrace.mak CFG="ntptrace - Debug"

@echo -----------------------------------------
@echo Compiling the Network Time Protocol server
@echo -----------------------------------------
@cd ..\ntpd
perl ..\scripts\mkver.bat -P ntpd
nmake  /f ntpd.mak CFG="ntpd - Debug"

@echo -----------------------------------------
@echo Compiling the NTP server control program
@echo -----------------------------------------
@cd ..\ntpdc
perl ..\scripts\mkver.bat -P ntpdc
nmake  /f ntpdc.mak CFG="ntpdc - Debug"

@echo -----------------------------------------
@echo Compiling the service installer
@echo -----------------------------------------
@cd ..\instsrv
nmake  /f instsrv.mak CFG="instsrv - Debug"

@echo -----------------------------------------
@echo Compiling the InstallShield support DLL
@echo -----------------------------------------
@cd ..\package\uninst
nmake  /f uninst.mak CFG="Uninst - Debug"
@cd ..


@echo
@echo -----------------------------------------
@echo Build Complete 
@echo Run install.bat in package\distrib 
@echo to complete installation.
@echo -----------------------------------------
