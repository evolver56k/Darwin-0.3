@ECHO OFF
IF EXIST %SystemRoot%\system32\tcpsvcs.exe goto TCPOK

ECHO TCP Service not installed.
ECHO .
ECHO You must install TCP/IP before installing the NTP service.
ECHO .

goto LAST

:TCPOK


REM
REM Setup NTP \var\ntp directory
REM

IF EXIST %SystemRoot%\..\var\ntp goto Skip1

MKDIR %SystemRoot%\..\var
MKDIR %SystemRoot%\..\var\ntp

:Skip1

REM
REM Copy configuration files.
REM

ECHO Installing Configuration Files

IF EXIST %SystemRoot%\ntp.conf goto Skip2

COPY ntp.conf %SystemRoot%

:Skip2


IF EXIST %SystemRoot%\ntp.drift goto Skip3

COPY ntp.drift  %SystemRoot%\ntp.drift

:Skip3


COPY ntpog.wri %SystemRoot%\..\var\ntp\ntpog.wri


REM 
REM Copy executables 
REM 

ECHO Installing Executables

COPY ntpd.exe     %SystemRoot%\system32
COPY ntpdc.exe    %SystemRoot%\system32
COPY ntpq.exe      %SystemRoot%\system32
COPY ntpdate.exe   %SystemRoot%\system32
COPY ntptrace.exe  %SystemRoot%\system32


REM
REM Install NTP server as an NT Service.
REM

.\instsrv.exe %SystemRoot%\system32\ntpd.exe

     
ECHO .
ECHO The NTP Service is now installed.
ECHO Please modify the NTP.CONF file in %SystemRoot% appropriately.
ECHO .
ECHO .
ECHO See readme.txt for more information.
ECHO .
ECHO After modifying the configuration file, use the services control panel
ECHO to make NTP autostart and either reboot or manually start it.
ECHO When the system restarts, the NTP service will be running.
ECHO For more information on NTP Operations please see the NTPOG.Wri
ECHO (NTP Operations Guide) in the %SystemRoot%\..\var\ntp directory...
ECHO .

:LAST
