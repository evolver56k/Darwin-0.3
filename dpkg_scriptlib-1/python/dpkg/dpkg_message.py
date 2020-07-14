# $Id: dpkg_message.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, string, regex, regex_syntax, regsub

regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
header = regex.compile ('^\\([-_A-Za-z0-9]+\\):[ \t]*\\(.*\\)$')
body = regex.compile ('^[ \t].*$')

def parse_lines (lines):

	entry = None
	ret = []

	for lineno in xrange (len (lines)):

		line = lines[lineno]

		if (header.search (line) >= 0):
			key = header.group (1)
			val = header.group (2)
			if (entry == None):
				entry = []
				ret.append (entry)
			entry.append ([key, lineno, val, []])

		elif (body.search (line) >= 0):
			if (entry == None):
				raise ValueError, 'body text occurs outside of a package field on line %d' % lineno
			entry[-1][3].append ((lineno, line))

		elif (line == ''):
			entry = None
			
		else:
			raise ValueError, 'unable to parse line %d: "%s"' % (lineno + 1, line)

	return ret

def read_entry (f):

	lines = []
	started = 0

	while 1:
		s = f.readline ()
		if (s == ''):
			break
		s = s[:-1]
		if (s != ''):
			started = 1
			lines.append (s)
		if ((s == '') and (started == 1)):
			break

	if (len (lines) == 0):
		return None
	return parse_lines (lines)[0]
		
def read (f):

	data = f.read ()
	lines = regsub.split (data, '[\r\n]')
	return parse_lines (lines)
