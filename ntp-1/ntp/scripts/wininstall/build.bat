@echo off
rem ----------------------------------------------------------------------
rem    BUILD.BAT:  the install-shield release-build batch file, for NTP
rem    Author:  Paul Wren, Software.Com
rem    modified: l. kahn for freeware version
rem    modified: Greg Schueman for freeware NTP 
rem    Date:    05/07/96
rem
rem    Adapted to MIPS/Intel 12/19/95.  Switch by passing 'mips' on
rem        command line for MIPS machine.
rem
rem    NOTE:  Will default to Intel.
rem 
rem ----------------------------------------------------------------------

rem
rem MODIFY THE NEXT line.
rem Its' value MUST be set to the location of the InstallShield installation.
rem
set IPRG=c:\msdev\ishield\program


if (%1)==() goto :intel
set VERSION=MIPS_Rel
set INSTDIR=mips
set ZIP=zip
echo Building MIPS Version...

if (%1)==(mips) goto :skip_intel

:intel
set VERSION=WinRel
set INSTDIR=intel
set ZIP=zip
echo Building INTEL version...
:skip_intel

set INST=%INSTDIR%\data

rem 
rem Setup directory structure
rem
mkdir %INSTDIR%\data
mkdir %INSTDIR%\disk1
mkdir %INSTDIR%\setup

rem
rem Next command creates the setup.rul file from setup.rul.in
rem
perl mksetup.bat
echo %INST% 
echo ...Build components...
%IPRG%\compile setup.rul -g
copy setup.ins %INSTDIR%\disk1

echo ...Copy NTP stuff to data directory...

COPY distrib\n*.exe     %INST%
COPY distrib\x*.exe     %INST%
COPY distrib\readme.nt  %INST%
COPY distrib\ntpog.wri  %INST%

COPY ..\..\COPYRIGHT    %INST%

cd %INSTDIR%
	echo ...Compress release data...
	del data\data.z
	%IPRG%\icomp -i data\*.* data\data.z
	copy data\data.z disk1
	del data\data.z

	echo ...Make setup packing list...
	cd setup
	copy ..\..\setup.lst .
	%IPRG%\packlist setup.lst
	copy setup.pkg ..\disk1
	copy setup.dbg ..\disk1
	cd ..

	echo ...Finish up disk contents with support files
rem	copy ..\setup.bmp       disk1
	copy %IPRG%\_setup.lib  disk1
	copy %IPRG%\_setup.dll  disk1
	copy %IPRG%\setup.exe   disk1

	echo ...Add helper dll libraries
	%IPRG%\icomp %IPRG%\ctl3d32.dll          disk1\_setup.lib
	%IPRG%\icomp ..\ntpdll.dll               disk1\_setup.lib
	%IPRG%\icomp ..\distrib\readme.nt        disk1\_setup.lib
rem	%IPRG%\icomp ..\ishield.bmp              disk1\_setup.lib

	rem Zip in separately so they're available BEFORE install runs
	copy ..\distrib\readme.nt  disk1
	copy %IPRG%\_INST32I.EX_ disk1 
rem copy stupid program needed for nt
rem that wasn't even installed by the
rem freeware install shield had to get it
rem off the vc++4.0 cd
	copy data\ntpog.wri disk1

	rem Zip the whole enchilada into one file
	rem erase ntp35frel.zip
	rem %ZIP% ntp35frel.zip disk1\*.*
cd ..

set NTP=
set INST=
set IPRG=

echo .
echo .
echo Completed.

