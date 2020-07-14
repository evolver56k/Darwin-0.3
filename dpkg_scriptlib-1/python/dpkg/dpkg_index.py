# $Id: dpkg_index.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import string, regex, regex_syntax, types
import dpkg_version, dpkg_packages

def create_index ():
	return {}

def augment_index (index, packages, source):

	for p in packages:
		names = [p['package']]
		if (p.has_key ('provides')):
			names = names + p['provides']
		for name in names:
			if (not index.has_key (name)):
				index[name] = []
			index[name].append (p)

def check_deps (pname, clause, index):

	details = []

	def check_dependency (clause, index = index, details = details):

		regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
		depstr = regex.compile ('^<<\\|<=\\|=\\|>=\\|>>\\|<\\|>')

		if (type (clause) == types.StringType):
			constraint, package, version = None, clause, None
		elif (depstr.search (clause[0]) >= 0):
			constraint = clause[0]
			package = clause[1]
			version = clause[2]
		else:
			raise ValueError, 'invalid dependency clause "%s"' % str (clause)
	
		if (index.has_key (package)):
			plist = index[package]
		else:
			plist = []

		found = 0
		for t in plist:
			pver = dpkg_packages.package_canon_version (t)
			if ((constraint == None) or (dpkg_version.check_version (pver, constraint, version))):
				details.append (('debug', 'resolved dependency on "%s" with "%s" version "%s"'
								 % (str(clause), t['package'], pver)))
				found = 1
			else:
				details.append (('debug', 'failed to resolve dependency on "%s" with "%s" version "%s"'
								 % (str(clause), t['package'], pver)))

		if (not found):
			details.append (('error', 'unresolved dependency on "%s"' % str (clause)))
			return 0
		else:
			return 1

	rval = dpkg_packages.check_deps (clause, check_dependency)
	return rval, details
