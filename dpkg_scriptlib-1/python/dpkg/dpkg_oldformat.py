# $Id: dpkg_oldformat.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
#
# Copyright (C) 1997  Klee Dienes <klee@mit.edu>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

import sys, string, subfile

def parse (f):

	l1 = f.readline ()
	l2 = f.readline ()
	
	r = {}
	r['version'] = l1[:-1]
	r['control'] = subfile.subfile (f, f.tell(), string.atoi (l2))
	r['data'] = subfile.subfile (f, f.tell() + string.atoi (l2), -1)

	return r
