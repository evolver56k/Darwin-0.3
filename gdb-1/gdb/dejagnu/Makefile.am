## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus dejagnu

# Install all the associated data files
SUBDIRS = baseboards config lib doc

# Install the bourne shell script driver in the bindir
bin_SCRIPTS = runtest

# Install the main DejaGnu expect file in the datadir
pkgdata_DATA = runtest.exp

# Install config.guess too, so we can use it to figure out
# what type of machine we're running on.
libexecdir = $(pkgdatadir)/libexec
libexec_SCRIPTS = config.guess



