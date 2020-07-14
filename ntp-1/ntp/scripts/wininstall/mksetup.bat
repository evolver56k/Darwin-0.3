@rem = '--*-Perl-*--';
@rem = '
@echo off
perl -S %0.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
@rem ';
######################################################################
#
# Revision: mksetup.bat
# Author:   Greg Schueman
# Date:     05/03/1996 
# Purpose:  Provide a perl script for Windows NT to create the files
#		SETUP.RUL and SETUP.LST used by InstallShield.
#		This script puts in the correct version numbers and
#		directory paths.
#           
#
#
# Subroutines:
#     print_help
#     
#
#
######################################################################

use English;
use Getopt::Long;

#********************************************************************* 
#  Program Dependency Requirements
#*********************************************************************

#********************************************************************* 
#  Set Environment
#*********************************************************************
$PROGRAM = $0;
$USAGE   = "Usage: ${PROGRAM} [ -H ]\n";


#********************************************************************* 
#  Subroutine Print Help 
#*********************************************************************

sub print_help 
{
   print STDERR $USAGE;
   print STDERR " -H --Help         Help on options\n";
   print STDERR "\n";
} # print_help end



#********************************************************************* 
#  Main program
#*********************************************************************

#
# Process runtime options
#
$result = GetOptions('help|H'); 

if ($opt_help == 1)
{ 
   print_help(); 
   exit();
};



#
# Program logic
#

$DATE =  localtime;
chomp $DATE;
$RUN = "0"; # Not working yet

# PROCESS SETUP.RUL now

open( INPUT1, '<..\..\configure' );
open( INPUT2, '<setup.rul.in' );
open( OUTPUT, '>setup.rul' );

while ($_ = <INPUT1> )
{
   if (/^PACKAGE=/) 
   {
      $XNTPPACKAGE = $POSTMATCH;
   }
   if (/^VERSION=/) 
   {
      $XNTPVERSION = $POSTMATCH;
   }
}
chomp $XNTPPACKAGE;
chomp $XNTPVERSION;

print OUTPUT "declare \n";
print OUTPUT "\#define APP_NAME2               \"$XNTPPACKAGE $XNTPVERSION Freeware\"\n";
print OUTPUT "\#define PRODUCT_VERSION         \"${XNTPVERSION}Rel\"\n";
print OUTPUT "\#define DEINSTALL_KEY           \"NTP$XNTPVERSION\"\n";

while (<INPUT2>)
{
   print OUTPUT;
}

close( OUTPUT );
close( INPUT2 );
close( INPUT1 );

# PROCESS SETUP.LST now

$Pwd = Win32::GetCwd;

open( INPUT1, '<setup.lst.in' );
open( OUTPUT, '>setup.lst' );

print OUTPUT "1;\n";

while ($_ = <INPUT1> )
{
   if (/^CURDIR/) 
   {
      print OUTPUT "$Pwd$POSTMATCH";
   }
}
close( OUTPUT );
close( INPUT1 );


__END__
:endofperl
