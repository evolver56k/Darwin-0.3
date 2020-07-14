# $Id: dpkg_log.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys

class logger:

	def __init__ (self):
		self.minsev = 'debug'
		self.allsevs = ['debug', 'info', 'warning', 'error']
		self.psevs = ['debug', 'info', 'warning', 'error']

	def set_min_severity (self, minsev):
		self.minsev = minsev
		if (minsev == 'debug'): self.psevs = ['debug', 'info', 'warning', 'error']
		elif (minsev == 'info'): self.psevs = ['info', 'warning', 'error']
		elif (minsev == 'warning'): self.psevs = ['warning', 'error']
		elif (minsev == 'error'): self.psevs = ['error']
		else: raise ValueError, 'invalid severity specification "%s"' % minsev

	def log (self, sev, *msgs):
		msg = ''
		for s in msgs:
			msg = msg + str (s)
		if sev in self.psevs:
			sys.stderr.write ('%s: %s\n' % (sev, str (msg)))
			sys.stderr.flush ()
		if (sev not in self.allsevs):
			raise ValueError, 'invalid severity specification "%s"' % sev
