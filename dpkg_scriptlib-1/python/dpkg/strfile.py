# $Id: strfile.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import string

class strfile:

	def __init__ (self, s):
		self.s = s
		self.pos = 0

	def isatty (self):
		return 0

	def tell (self):
		return self.pos

	def seek (self, offset, mode = 0):

		if (mode == 0):
			npos = offset
		elif (mode == 1):
			npos = self.pos + offset
		elif (mode == 2):
			npos = len (s) - offset
		else:
			raise ValueError, 'invalid seek mode %d' % mode
		
		if (npos < 0):
			raise IOError, 'seek before beginning of string'
		if (npos > len (s)):
			raise IOError, 'seek past end of string'

		self.pos = npos

	def read (self, n = -1):
		if (n < 0):
			return self.s[self.pos:]
		else:
			self.pos = self.pos + n
			return self.s[self.pos - n:self.pos]

	def readline (self):
		i = string.find (self.s, '\n', self.pos)
		if (i < 0):
			r = self.s[self.pos:]
			self.pos = len (self.s)
			return r
		else:
			r = self.s[self.pos:i + 1]
			self.pos = i + 1
			return r

	def readlines (self):
		r = []
		while 1:
			l = self.readline ()
			if (l == ''):
				return r
			r.append (l)

	def close (self):
		return
