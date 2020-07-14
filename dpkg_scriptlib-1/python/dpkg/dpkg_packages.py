# $Id: dpkg_packages.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import os, sys, string, regex, regex_syntax, regsub, types, posix, posixpath
import dpkg_message, dpkg_version

def parse_package_name (s):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	pregex = regex.compile ('^\\([-.+A-Za-z0-9]+\\)\\(_\\([^_]+\\)\\)?\\(_\\([^_]+\\)\\)?$')
	r = {}
	if (pregex.search (s) < 0):
		raise ValueError, 'invalid package name "%s"' % s
	r['package'] = pregex.group (1)
	if (pregex.group (3) != None):
		r['version'] = pregex.group (3)
	if (pregex.group (5) != None):
		r['architecture'] = pregex.group (5)
	return r

def package_canon_version (p):

	r = p['version']
	if (p.has_key ('revision') and p.has_key ('package_revision')):
		raise ValueError ('package has both revision and pacakge_revision entries')
	if (p.has_key ('package_revision')):
		r = r + '-' + p['package_revision']
	if (p.has_key ('revision')):
		r = r + '-' + p['revision']
	return r

def package_canon_name (p):
	return p['package'] + '_' + package_canon_version (p) + '_' + p['architecture']

def parse_provides (s):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	depstr = regex.compile ('^[ \t]*\\([-.+A-Za-z0-9]+\\)[ \t]*\\((.*)\\)?[ \t]*$')
	whitespace = regex.compile ('^[ \t]*$')

	strs = regsub.split (s, '[ \t]*[,][ \t]*')
	ret = []
	for s in strs:
		if (whitespace.search (s) >= 0):
			pass
		else:
			ret.append (s)
	return ret

def parse_depversion (s):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	depstr = regex.compile ('^[ \t]*\\([-.+A-Za-z0-9]+\\)[ \t]*\\((.*)\\)?[ \t]*$')
	version = regex.compile ('^(\\([<>=]+\\)[ \t]*\\(.*\\))$')
	whitespace = regex.compile ('^[ \t]*$')

	if (depstr.search (s) >= 0):
		dep = depstr.group (1)
		ver = depstr.group (2)
		if ((ver == None) or (whitespace.search (ver) >= 0)):
			return dep
		elif (version.search (ver) >= 0):
			relation = version.group (1)
			value = version.group (2)
			return [relation, dep, value]
		else:
			raise ValueError, 'syntax error parsing dependency version "%s"' % ver
	else:
		raise ValueError, 'syntax error parsing dependency "%s"' % s

def parse_depsub (s, defc):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	whitespace = regex.compile ('^[ \t]*$')

	vstrs = regsub.split (s, '[,]')
	tret = []
	for vs in vstrs:
		ostrs = regsub.split (vs, '[|]')
		ret = []
		for s in ostrs:
			if (whitespace.search (s) >= 0):
				pass
			else:
				ret.append (parse_depversion (s))
		if (len (ret) == 0):
			pass
		elif (len (ret) == 1):
			tret.append (ret[0])
		else:
			tret.append (['|'] + ret)

	if (len (tret) == 0):
		return None
	elif (len (tret) == 1):
		return tret[0]
	else:
		return [ defc ] + tret

def parse_depends (s):
	return parse_depsub (s, '&')

def parse_conflicts (s):
	return parse_depsub (s, '|')

def parse_field (package, field, logger):

	key, lineno, val, detail = string.lower (field[0]), field[1], field[2], field[3]

	if (package.has_key (key)):
		logger ('warning', 'duplicate key "%s" on line %d: ignoring' % (key, lineno))
		return

	if (key == 'description'):
		long = ''
		for line in detail:
			line = line[1]
			if (line == ' .'):
				# blank line
				pass
			elif (line[0:2] == ' .'):
				# reserved for expansion
				logger ('warning', 'unknown expansion command "%s" on line %d' % (line[2:], lineno))
			elif ((len (line) >= 2)
				  and ((line[0] == ' ') or (line[0] == '\t'))
				  and ((line[1] == ' ') or (line[1] == '\t'))):
				# verbatim body
				long = long + line
			else:
				# paragraph continuation
				long = long + line
		package[key] = [val, long]
		return

	if (key == 'conffiles'):
		package[key] = map (lambda e: e[1], detail)
		return

	if (len (detail) != 0):
		logger ('error', 'unexpected extended data in field "%s" on line %d: ignoring' % (key, lineno))
		
	if ((key == 'depends') or (key == 'pre-depends') or (key == 'recommends') or (key == 'suggests') or (key == 'conflicts')):
		try:
			if (key == 'conflicts'):
				package[key] = parse_conflicts (val)
			else:
				package[key] = parse_depends (val)
		except ValueError, val:
			logger ('error', 'error parsing dependency information "%s" on line %d: ignoring' % (val, lineno))
		return

	if (key == 'package'):
		regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
		pstr = regex.compile ('^[-.+A-Za-z0-9]+$')
		if (pstr.search (val) < 0):
			logger ('error', 'invalid package name "%s" on line %d' % (val, lineno))
		package[key] = val
		return

	if (key == 'provides'):
		package[key] = parse_provides (val)
		return

	package[key] = val

def parse_entry (entry, logger):
	package = {}
	for field in entry:
		parse_field (package, field, logger)
	return package

def read_entry (file, logger):

	entry = dpkg_message.read_entry (file)
	return parse_entry (entry, logger)

def read (file, logger):

	ret = dpkg_message.read (file)
	plist = []
	for entry in ret:
		plist.append (parse_entry (entry, logger))
	return plist

def package_satisfies (p, clause):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	depstr = regex.compile ('^<<\\|<=\\|=\\|>=\\|>>\\|<\\|>')
	if (type (clause) == types.StringType):
		if (p['package'] == clause): return 1
		if (p.has_key ('provides') and (clause in p['provides'])): return 1
		return 0
	elif (depstr.search (clause[0]) >= 0):
		constraint = clause[0]
		package = clause[1]
		version = clause[2]
		if (p['package'] == package): return 1
		return dpkg_version.check_version (package_canon_version (p), constraint, version)
	else:
		raise ValueError, 'invalid dependency clause "%s"' % str (clause)

def packages_compatible (p1, p2):

	if (p1['package'] == p2['package']):
		return 0
	if (p1.has_key ('conflicts') and (check_deps (p1['conflicts'], lambda c, p2 = p2: package_satisfies (p2, c)))):
		return 0
	if (p2.has_key ('conflicts') and (check_deps (p2['conflicts'], lambda c, p1 = p1: package_satisfies (p1, c)))):
		return 0

	return 1

def check_deps (clause, pfunc):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	depstr = regex.compile ('^<<\\|<=\\|=\\|>=\\|>>\\|<\\|>')

	if (clause == None):
		return 1

	if (len (clause) == 0):
		return 1
	
	if (clause[0] == '&'):
		ret = 1
		for sclause in clause[1:]:
			cret = check_deps (sclause, pfunc)
			if (cret == 0):
				ret = 0
		return ret

	if (clause[0] == '|'):
		ret = 0
		for sclause in clause[1:]:
			cret = check_deps (sclause, pfunc)
			if (cret != 0):
				ret = 1
		return ret

	if (type (clause) == types.StringType):
		return pfunc (clause)
	
	if (depstr.search (clause[0]) >= 0):
		return pfunc (clause)

	raise ValueError, 'invalid dependency clause "%s"' % str (clause)

def locate_package (base, path):

	while 1:
		if (os.path.isfile (os.path.join (base, path))):
			return os.path.join (base, path)
		nbase = (os.path.split (base))[0]
		if (nbase == base):
			return None
		base = nbase

def find_package (p, path):

	ret = [ None ]
	def visit (arg, dirname, names):
		for name in names:
			if (name == package_canon_name (p)):
				ret[0] = dirname + '/'
