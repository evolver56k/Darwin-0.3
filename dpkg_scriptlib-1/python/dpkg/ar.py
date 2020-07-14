# $Id: ar.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

def parse_string (s):
	e = len (s) - 1
	while ((e != -1) and (s[e] == ' ')):
		e = e - 1
	return s[0:e+1]

def parse_num (s):
	return string.atoi (s)

def parse_oct (s):
	return string.atoi (s, 8)

def parse_header (s):
	r = {}
	r['name'] = parse_string (s[0:16])
	r['date'] = parse_num (s[16:28])
	r['uid'] = parse_oct (s[28:34])
	r['gid'] = parse_oct (s[34:40])
	r['mode'] = parse_oct (s[40:48])
	r['size'] = parse_num (s[48:58])
	r['fmag'] = s[58:60]
	return r

def fseek (f, i):
	try:
		f.seek (i, 0)
		return
	except IOError, (errno, errstr):
		if (errno != 29):
			raise IOError, (errno, errstr)
	while (f.tell () < i):
		f.read (min (4096, i - f.tell()))
	if (i < f.tell()):
		raise ValueError, 'unable to seek backwards on stream'

def parse_file (f, p):
	s = f.read (8)
	if (s != '!<arch>\n'):
		raise ValueError, 'archive file is missing magic number (read "%s")' % s
	i = 8
	while 1:
		fseek (f, i)
		s = f.read (60)
		if (s == ''):
			return
		if (len (s) != 60):
			raise ValueError, 'bad read'
		r = parse_header (s)
		r['offset'] = i + 60
		i = i + 60 + (((r['size'] + 1) / 2) * 2)
		r['file'] = subfile.subfile (f, r['offset'], r['size'])
		p (f, r)
