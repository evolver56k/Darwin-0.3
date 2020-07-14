# $Id: procfilter.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, os, posix

class procfilter:

	def __init__ (self, f, cmd):

		self.filter (f, '/bin/sh', ['/bin/sh', '-c', cmd])

	def filter (self, f, cmd, argv, env = None):

		(ri, wi) = os.pipe ()
		(ro, wo) = os.pipe ()
		(re, we) = os.pipe ()

		self.writer = os.fork ()
		if (self.writer == 0):
			sys.argv[0] = sys.argv[0] + "[source]"
			for i in [ri, wi, ro, wo, re, we, 0, 1, 2]:
				if (i not in [wi]):
					try:
						os.close (i)
					except posix.error, val: 
						pass
			nf = os.fdopen (wi, 'w')
			s = f.read ()
			nf.write (s)
			nf.flush ()
			os.close (wi)
			os._exit (0)

		self.cmd = [cmd, argv, env]
		self.filter = os.fork ()
		if (self.filter == 0):
			sys.argv[0] = sys.argv[0] + "[filter]"
			for i in [ri, wi, ro, wo, re, we, 0, 1, 2]:
				if (i not in [ri, wo, we]):
					try:
						os.close (i)
					except posix.error, val: 
						pass
			os.dup2 (ri, 0)
			os.dup2 (wo, 1)
			os.dup2 (we, 2)
			if (env == None):
				os.execv (cmd, argv)
			else:
				os.execve (cmd, argv, env)

		for i in [ri, wi, ro, wo, re, we]:
			if (i not in [ro, re]):
				os.close (i)

		self.o = os.fdopen (ro, 'r')
		self.pos = 0
		self.blocksize = 16 * 1024

		self.e = os.fdopen (re, 'r')
		self.epos = 0

	def isatty (self):
		return 0

	def seek (self, offset, mode = 0):
		#print 'filter: seek %d (cur %d)' % (offset, self.tell ())
		while (self.tell () < offset):
			self.read (min (self.blocksize, offset - self.tell()))
		if (offset < self.tell()):
			raise ValueError, 'unable to seek backwards on stream'

	def tell (self):
		#print 'filter: tell %d' % self.pos
		return self.pos

	def read (self, n = -1):
		r = self.o.read (n)
		#print 'filter: read %d bytes from %d' % (len (r), self.pos)
		self.pos = self.pos + len (r)
		return r

	def readline (self):
		r = self.o.readline ()
		self.pos = self.pos + len (r)
		return r

	def readlines (self):
		rl = self.o.readlines ()
		for r in rl:
			self.pos = self.pos + len (r)
		return rl

	def stderr (self):
		return self.e

	def stdout (self):
		return self.o

	def wait (self):
		self.o.close ()
		self.e.close ()
		writer_pid, writer_status = os.waitpid (self.writer, 0)
		#print writer_pid, writer_status
		if (self.writer != writer_pid):
			raise ValueError, 'unexpected writer process-id'
		filter_pid, filter_status = os.waitpid (self.filter, 0)
		#print filter_pid, filter_status
		if (self.filter != filter_pid):
			raise ValueError, 'unexpected filter process-id'
		return writer_status, filter_status

	def check_status (self, ws, fs):
		if (ws != 0):
			raise ValueError, 'writer sub-process for filter %s returned error exit status %d' % (str (self.cmd), ws)
		if (fs != 0):
			raise ValueError, 'filter sub-process %s returned error exit status %d' % (str (self.cmd), fs)

	def close (self):
		ws, fs = self.wait ()
		self.check_status (ws, fs)
