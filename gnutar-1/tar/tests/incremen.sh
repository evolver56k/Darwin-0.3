#! /bin/sh
# A directory older than the listed entry was skipped completely.

. ./preset
. $srcdir/before

set -e
mkdir structure
touch structure/file
# FIXME: The sleep is necessary for the second tar to work.  Exactly why?
sleep 1
tar cf archive --listed=list structure
tar cfv archive --listed=list structure
echo -----
touch structure/file
tar cfv archive --listed=list structure

out="\
structure/
-----
structure/
structure/file
"

. $srcdir/after
