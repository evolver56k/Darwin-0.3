# $Id: subfile.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

class subfile:

	def __init__ (self, f, offset, length = -1):
		self.f = f
		self.f.seek (offset)
		self.offset = offset
		self.pos = 0
		self.length = length
		self.blocksize = 16 * 1024

	def isatty (self):
		return 0

	def tell (self):
		#print 'sfile: tell is %d' % self.pos
		return self.pos

	def seek (self, offset, mode = 0):
		#print 'sfile: seeking from %d to %d' % (self.pos, offset)
		if (mode == 0):
			if (self.length < 0):
				self.pos = offset
				self.f.seek (self.pos + self.offset)
			else:
				if (offset > self.length):
					raise IOError, 'seek past end of file'
				self.pos = offset
				self.f.seek (self.pos + self.offset)

		if (mode == 1):
			self.pos = self.pos + offset
			self.f.seek (self.pos + self.offset)

		elif (mode == 2):
			if (length < 0):
				self.f.seek (0, 2)
				self.pos = self.f.tell () - self.offset
			elif (length > self.length):
				raise IOError, 'seek past end of file'
			else:
				self.seek (self.pos + self.length - self.offset)

	def read (self, n = -1):
		self.f.seek (self.offset + self.pos)
		#print 'sfile: n: %d' % n
		if ((n < 0) and (self.length >= 0)):
			n = self.length - self.pos
		if (self.length >= 0):
			n = min (n, self.length - self.pos)
		#print 'sfile: n: %d' % n
		r = ''
		while (n != 0):
			op = self.f.tell ()
			s = self.f.read (n)
			#print 'sfile: read %d bytes from %d (%d in original)' % (len (s), self.pos, op)
			self.pos = self.pos + len (s)
			n = n - len (s)
			if (s == ''):
				break
			r = r + s
		#print 'returning: "%s"' % r
		return r

	def readline (self):
		r = ''
		while 1:
			s = self.read (1)
			if (s == ''):
				return r
			r = r + s
			if (s == '\n'):
				return r

	def readlines (self):
		r = []
		while 1:
			l = self.readline ()
			if (l == ''):
				return r
			r.append (l)
