# $Id: dpkg_version.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

def parse_version (v):
	
	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	ereg = regex.compile ('^[0-9]+$')
	ureg = regex.compile ('^[-:.+A-Za-z0-9]+$')
	dreg = regex.compile ('^[.+A-Za-z0-9]+$')

	s = v
	r = string.find (s, ':')
	if (r >= 0):
		epoch = s[0:r]
		if (ereg.match (epoch) < 0):
			raise ValueError, 'epoch ("%s") has invalid format for version string "%s"' % (epoch, v)
		s = s[r+1:]
	else:
		epoch = None

	r = string.rfind (s, '-')
	if (r > 0):
		debian = s[r+1:]
		if (dreg.match (debian) < 0):
			raise ValueError, 'debian-revision ("%s") has invalid format for version string "%s"' % (debian, v)
		s = s[:r]
	else:
		debian = None

	upstream = s[0:]
	if (ureg.match (upstream) < 0):
			raise ValueError, 'upstream-version ("%s") has invalid format for version string "%s"' % (upstream, v)

	return (epoch, upstream, debian)

def _strip_nondigit (s):
	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	dstr = regex.compile ('^\\([^0-9]*\\)\\(.*\\)$')
	if (dstr.match (s) < 0):
		raise ValueError, 'internal error'
	prefix = s[dstr.regs[1][0]:dstr.regs[1][1]]
	rest = s[dstr.regs[2][0]:dstr.regs[2][1]]
	return prefix, rest

def _strip_digit (s):
	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	dstr = regex.compile ('^\\([0-9]*\\)\\(.*\\)$')
	if (dstr.match (s) < 0):
		raise ValueError, 'internal error'
	prefix = s[dstr.regs[1][0]:dstr.regs[1][1]]
	rest = s[dstr.regs[2][0]:dstr.regs[2][1]]
	if (prefix == ''): prefix = '0'
	return string.atol (prefix), rest

def _compare_nondigit (s1, s2):
	for i in (xrange (max (len (s1), len (s2)))):
		if (i >= len (s1)): return -1
		if (i >= len (s2)): return 1
		if ((s1[i] in string.letters) and (s2[i] not in string.letters)): return -1
		if ((s2[i] in string.letters) and (s1[i] not in string.letters)): return 1
		if (s1[i] < s2[i]): return -1
		if (s2[i] < s1[i]): return 1
	return 0

def compare_subversion (v1, v2):

	nv1 = v1
	nv2 = v2

	while 1:

		# print 'nondigit: nv1: "%s", nv2: "%s"' % (nv1, nv2)
		sv1, nv1 = _strip_nondigit (nv1)
		sv2, nv2 = _strip_nondigit (nv2)
		# print 'nondigit: sv1: "%s", sv2: "%s"' % (sv1, sv2)

		r = _compare_nondigit (sv1, sv2)
		if (r != 0):
			return r
		if ((nv1 == '') and (nv2 == '')):
			return 0

		# print 'digit: nv1: "%s", nv2: "%s"' % (nv1, nv2)
		sv1, nv1 = _strip_digit (nv1)
		sv2, nv2 = _strip_digit (nv2)
		# print 'digit: sv1: "%s", sv2: "%s"' % (sv1, sv2)

		if (sv1 < sv2):
			return -1
		if (sv1 > sv2):
			return 1
		if ((nv1 == '') and (nv2 == '')):
			return 0

def compare_versions (v1, v2):

	e1, u1, d1 = parse_version (v1)
	e2, u2, d2 = parse_version (v2)

	if (e1 == None): e1 = ''
	if (d1 == None): d1 = ''
	if (e2 == None): e2 = ''
	if (d2 == None): d2 = ''

	r = compare_subversion (e1, e2)
	if (r != 0): return r
	r = compare_subversion (u1, u2)
	if (r != 0): return r
	r = compare_subversion (d1, d2)
	if (r != 0): return r

	return 0

def check_version (v1, constraint, v2):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	eqreg = regex.compile ('^=\\|<=\\|>=\\|<\\|>$')
	ltreg = regex.compile ('^<=\\|<<\\|<$')
	gtreg = regex.compile ('^>=\\|>>\\|>$')

	r = compare_versions (v1, v2)
	if (r == 0):
		return (eqreg.match (constraint) > 0)
	elif (r > 0): 
		return (gtreg.match (constraint) > 0)
	else: # (r < 0): 
		return (ltreg.match (constraint) > 0)
