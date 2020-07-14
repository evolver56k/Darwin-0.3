# $Id: tar.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, string, subfile, os, stat

type_file = '0'
type_hardlink = '1'
type_symlink = '2'
type_chardev = '3'
type_blockdev = '4'
type_directory = '5'

def parse_string (s):
	e = len (s) - 1
	while ((e != -1) and (s[e] == '\0')):
		e = e - 1
	return s[0:e+1]

def parse_num (s):
	b = 0
	e = len (s) - 1
	while ((e != -1) and ((s[e] == '\0') or (s[e] == ' '))):
		e = e - 1
	while ((b <= e) and (s[b] == ' ')):
		b = b + 1
	s = s[b:e+1]
	if (s == ''):
		return -1L
	return string.atol (s, 8)

def parse_header (s):
	r = {}
	r['name'] = parse_string (s[0:100])
  	r['mode'] = parse_num (s[100:108])
	r['uid'] = parse_num (s[108:116])
	r['gid'] = parse_num (s[116:124])
	r['size'] = parse_num (s[124:136])
	r['time'] = parse_num (s[136:148])
	r['checksum'] = parse_num (s[148:156])
	r['type'] = s[156:157]
	r['link'] = parse_string (s[157:257])
	r['magic'] = s[257:265]
	r['uname'] = parse_string (s[265:297])
	r['gname'] = parse_string (s[297:329])
	r['dmaj'] = parse_num (s[329:337])
	r['dmin'] = parse_num (s[337:345])
	return r

def fseek (f, i):
	try:
		f.seek (i, 0)
		return
	except:
		pass
	while (f.tell () < i):
		f.read (min (4096, i - f.tell()))
	if (i < f.tell()):
		raise ValueError, 'unable to seek backwards on stream'

def parse_file (f, p):
	i = 0
	while 1:
		fseek (f, i)
		s = f.read (512)
		#print 'tar.parse: read %d bytes at %d' % (len (s), i)
		last = 0
		if (len (s) != 512): 
			last = 1
		r = parse_header (s)
		nblocks = (r['size'] + 511) / 512
		r['offset'] = i + 512
		i = i + ((nblocks + 1) * 512)
		if (r['name'] != ''):
			r['file'] = subfile.subfile (f, r['offset'], r['size'])
			p (f, r)
		if (last):
			return
