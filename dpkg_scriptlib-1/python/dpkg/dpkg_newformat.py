# $Id: dpkg_newformat.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, string, subfile, ar

def do_ar (f, r, s):

	if (r['name'] == 'data.tar.gz'):
		s['data'] = r['file']
	elif (r['name'] == 'control.tar.gz'):
		s['control'] = r['file']
	elif (r['name'] == 'debian-binary'):
		s['version'] = r['file'].read ()[:-1]
	else:
		raise ValueError, 'unknown archive member "%s"' % r['name']

def parse (f):

	s = {}
	try:
		ar.parse_file (f, lambda f, r, s = s: do_ar (f, r, s))
	except ValueError, s:
		e = 'archive file is missing magic number'
		if (s[0:len(e)] != e):
			raise sys.exc_type, sys.exc_value, sys.exc_traceback
		raise ValueError, 'file is not a new-format debian archive'
	return s
