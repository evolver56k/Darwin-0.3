: File: Build-package.bat
:
: Purpose: To create an InstallShield package 
:
: The following main steps are carried out in order. If an error 
: occurs during the compile or build, the distribution diskette is 
: not created: 
: 
: Step 1. Compile the setup script
: Step 2. Build the media. 
: Step 3. Copy the setup files to a diskette in drive A.
: 
: Output from the Compile and Build tools is redirected to the file 
: ISBuild.rpt. If errors occur, view this file for further information.
: 

: Turn off command echo to display.
  @Echo off


: Insert Version Number into Package
perl .\mksetup.bat

: YOU MUST SET THE NEXT VARIABLE TO THE INSTALL DIRECTORY OF InstallShield
set IPRG=n:\apps\InstallShield\InstallShield 5.1 Professional Edition
set IPFW=n:\apps\InstallShield\PackageForTheWeb 2

: Save the current search path
  Set SavePath=%Path%

: Set the search path to find InstallShield's command line tools
  Path %IPRG%\Program;%IPFW%;%PATH%

: Execute the command line compiler
  Compile -I"%IPRG%\Include;.\install\Script Files" "install\Script Files\setup.rul" > ISBuild.rpt

: Test for success. Quit if there was a compiler error.
  If errorlevel 1 goto CompilerErrorOccurred

: Build the media
  ISbuild -p"install" -m"NTP" >> ISBuild.rpt

: Test for success. Quit if there was a build error.
  If errorlevel 1 goto BuildErrorOccurred


pftwwiz "webpack\Projects\NetworkTimeProtocol.pfw" -a

: Prompt the user to place a diskette in drive A.
  Echo Insert a blank formatted diskette in drive A. 
  Pause Then press any key to create the distribution diskette.

: Copy the setup files to the diskette in drive A.
  XCopy install\media\testpa~1\diskim~1\disk1\*.* A:\

: Skip over the error handling and exit
  Goto Done

: Report the compiler error; then exit
  :CompilerErrorOccurred
  Echo Error on compile; media not built. 
  Pause Press any key to view report.
  Type ISBuild.rpt | More
  Goto Done

: Report the build error; then exit
  :BuildErrorOccurred
  Echo Error on build; media not built. 
  Pause Press any key to view report.
  Type ISBuild.rpt | More

  :Done
: Restore the search path
  Path=%SavePath%
  Set SavePath=