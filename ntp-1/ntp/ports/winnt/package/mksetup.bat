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

open( INPUT1, '<..\..\..\configure' );
open( INPUT2, '<value-shl.in' );
#open( OUTPUT, '>value-test.shl' );
open( OUTPUT, '>install\String Tables\0009-English\value.shl' );


while ($_ = <INPUT1> )
{
   if (/^PACKAGE=/) 
   {
      $NTPPACKAGE = $POSTMATCH;
   }
   if (/^VERSION=/) 
   {
      $NTPVERSION = $POSTMATCH;
   }
}
chomp $NTPPACKAGE;
chomp $NTPVERSION;

print OUTPUT "[Data]\n";
print OUTPUT "PRODUCT_VERSION=${NTPVERSION}\n";

while (<INPUT2>)
{
   print OUTPUT;
}

close( OUTPUT );
close( INPUT2 );
close( INPUT1 );


__END__
:endofperl
