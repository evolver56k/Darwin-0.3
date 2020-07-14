@echo off
REM
REM File: cleanup.bat
REM Purpose: To cleanup the NTP source tree for Win NT after a build.
REM Date: 1998/06/27 (yyyy/mm/dd)
REM


del /Q ntp.opt ntp.ncb ntp.plg

@echo -----------------------------------------
@echo Cleanup event logging Resources
@echo -----------------------------------------
@cd libntp
del /Q MSG00001.bin
del /Q messages.h
del /Q messages.rc

@echo -----------------------------------------
@echo Cleanup support library
@echo -----------------------------------------
del /Q Debug\*.*
del /Q Release\*.*
del /Q libntp.opt libntp.ncb libntp.plg


@echo -----------------------------------------
@echo Clenaup ntpq program
@echo -----------------------------------------
@cd ..\ntpq
del /Q Debug\*.*
del /Q Release\*.*
del /Q ntpq.opt ntpq.ncb ntpq.plg
del /Q version.c


@echo -----------------------------------------
@echo Cleanup ntpdate program
@echo -----------------------------------------
@cd ..\ntpdate
del /Q Debug\*.*
del /Q Release\*.*
del /Q ntpdate.opt ntpdate.ncb ntpdate.plg
del /Q version.c



@echo -----------------------------------------
@echo Cleanup ntptrace program
@echo -----------------------------------------
@cd ..\ntptrace
del /Q Debug\*.*
del /Q Release\*.*
del /Q ntptrace.opt ntptrace.ncb ntptrace.plg
del /Q version.c


@echo -----------------------------------------
@echo Cleanup the Network Time Protocol server
@echo -----------------------------------------
@cd ..\ntpd
del /Q Debug\*.*
del /Q Release\*.*
del /Q ntpd.opt ntpd.ncb ntpd.plg
del /Q version.c

@echo -----------------------------------------
@echo Cleanup the NTP server control program
@echo -----------------------------------------
@cd ..\ntpdc
del /Q Debug\*.*
del /Q Release\*.*
del /Q ntpdc.opt ntpdc.ncb ntpdc.plg
del /Q version.c

@echo -----------------------------------------
@echo Cleanup the service installer
@echo -----------------------------------------
@cd ..\instsrv
del /Q Debug\*.*
del /Q Release\*.*
del /Q instsrv.opt instsrv.ncb instsrv.plg

@echo -----------------------------------------
@echo Cleanup the InstallShield support DLL
@echo -----------------------------------------
@cd ..\package\uninst
del /Q Debug\*.*
del /Q Release\*.*
del /Q uninst.opt uninst.ncb uninst.plg
@cd ..

@echo -----------------------------------------
@echo Cleanup the InstallShield
@echo -----------------------------------------
@cd install
del /Q "Media\NTP\Report Files\*.rpt"
del /Q "Media\NTP\Log Files\*.log"
del /Q "Media\NTP\Disk Images\disk1\*.cab"
@cd ..


@echo -----------------------------------------
@echo Cleanup the Package For The Web
@echo -----------------------------------------
@cd webpack
del /Q Output\*.*
del /Q Projects\*.log
@cd ..


REM
REM  Cleanup EXEs 
REM
cd distrib
del /Q Debug\*.*
del /Q Release\*.*
cd ..
del /Q ISBuild.rpt
cd ..

REM @echo -----------------------------------------
REM @echo Cleanup InstallShield Package
REM @echo -----------------------------------------


@echo
@echo -----------------------------------------
@echo Cleanup Complete 
@echo -----------------------------------------
