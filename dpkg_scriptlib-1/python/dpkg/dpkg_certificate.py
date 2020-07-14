# $Id: dpkg_certificate.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, string, regex, regex_syntax, regsub, dpkg_message, dpkg_packages, outstr, stat

regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
nregex = regex.compile ('^[0-9]+$')

def parse_ugid (s):
	if (nregex.match (s) >= 0):
		return string.atoi (s)
	else:
		return s

regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
dvregex = regex.compile ('^\([^ ]+\) d \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\)')
fvregex = regex.compile ('^\([^ ]+\) f \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9a-f]+\)')
lvregex = regex.compile ('^\([^ ]+\) l \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) "\([^"]+\)"')
svregex = regex.compile ('^\([^ ]+\) s "\([^"]+\)"')
bvregex = regex.compile ('^\([^ ]+\) b \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9]+\),\([0-9]+\)')
cvregex = regex.compile ('^\([^ ]+\) c \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9]+\),\([0-9]+\)')

def parse_fline (s):

	r = {}

	if (dvregex.match (s) >= 0):
		r['type'] = 'd'
		r['package'] = dvregex.group (1)
		r['mode'] = string.atoi (dvregex.group (2), 8)
		r['uname'] = parse_ugid (dvregex.group (3))
		r['gname'] = parse_ugid (dvregex.group (4))
	elif (fvregex.match (s) >= 0):
		r['type'] = 'f'
		r['package'] = fvregex.group (1)
		r['mode'] = string.atoi (fvregex.group (2), 8)
		r['uname'] = parse_ugid (fvregex.group (3))
		r['gname'] = parse_ugid (fvregex.group (4))
		r['md5'] = fvregex.group (5)
	elif (lvregex.match (s) >= 0):
		r['type'] = 'l'
		r['package'] = lvregex.group (1)
		r['mode'] = string.atoi (lvregex.group (2), 8)
		r['uname'] = parse_ugid (lvregex.group (3))
		r['gname'] = parse_ugid (lvregex.group (4))
		r['link'] = lvregex.group (5)
	elif (svregex.match (s) >= 0):
		r['type'] = 's'
		r['package'] = svregex.group (1)
		r['link'] = svregex.group (2)
	elif (bvregex.match (s) >= 0):
		r['type'] = 'b'
		r['package'] = bvregex.group (1)
		r['mode'] = string.atoi (bvregex.group (2), 8)
		r['uname'] = parse_ugid (bvregex.group (3))
		r['gname'] = parse_ugid (bvregex.group (4))
		r['dmaj'] = string.atoi (bvregex.group (5), 10)
		r['dmin'] = string.atoi (bvregex.group (6), 10)
	elif (cvregex.match (s) >= 0):
		r['type'] = 'c'
		r['package'] = cvregex.group (1)
		r['mode'] = string.atoi (cvregex.group (2), 8)
		r['uname'] = parse_ugid (cvregex.group (3))
		r['gname'] = parse_ugid (cvregex.group (4))
		r['dmaj'] = string.atoi (cvregex.group (5), 10)
		r['dmin'] = string.atoi (cvregex.group (6), 10)
	else:
		raise ValueError, "unable to parse line '%s'" % s

	return r

regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
dregex = regex.compile ('^d \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) "\([^"]+\)"')
fregex = regex.compile ('^f \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9a-f]+\) "\([^"]+\)"')
lregex = regex.compile ('^l \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) "\([^"]+\)" "\([^"]+\)"')
sregex = regex.compile ('^s "\([^"]+\)" "\([^"]+\)"')
bregex = regex.compile ('^b \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9]+\),\([0-9]+\) "\([^"]+\)"')
cregex = regex.compile ('^c \([0-7]+\) \([-a-zA-Z0-9]+\)/\([-a-zA-Z0-9]+\) \([0-9]+\),\([0-9]+\) "\([^"]+\)"')

def parse_line (s):

	r = {}

	if (dregex.match (s) >= 0):
		r['type'] = 'd'
		r['mode'] = string.atoi (dregex.group (1), 8)
		r['uname'] = parse_ugid (dregex.group (2))
		r['gname'] = parse_ugid (dregex.group (3))
		r['name'] = dregex.group (4)
	elif (fregex.match (s) >= 0):
		r['type'] = 'f'
		r['mode'] = string.atoi (fregex.group (1), 8)
		r['uname'] = parse_ugid (fregex.group (2))
		r['gname'] = parse_ugid (fregex.group (3))
		r['md5'] = fregex.group (4)
		r['name'] = fregex.group (5)
	elif (lregex.match (s) >= 0):
		r['type'] = 'l'
		r['mode'] = string.atoi (lregex.group (1), 8)
		r['uname'] = parse_ugid (lregex.group (2))
		r['gname'] = parse_ugid (lregex.group (3))
		r['name'] = lregex.group (4)
		r['link'] = lregex.group (5)
	elif (sregex.match (s) >= 0):
		r['type'] = 's'
		r['name'] = sregex.group (1)
		r['link'] = sregex.group (2)
	elif (bregex.match (s) >= 0):
		r['type'] = 'b'
		r['mode'] = string.atoi (bregex.group (1), 8)
		r['uname'] = parse_ugid (bregex.group (2))
		r['gname'] = parse_ugid (bregex.group (3))
		r['dmaj'] = string.atoi (bregex.group (4), 10)
		r['dmin'] = string.atoi (bregex.group (5), 10)
		r['name'] = bregex.group (6)
	elif (cregex.match (s) >= 0):
		r['type'] = 'c'
		r['mode'] = string.atoi (cregex.group (1), 8)
		r['uname'] = parse_ugid (cregex.group (2))
		r['gname'] = parse_ugid (cregex.group (3))
		r['dmaj'] = string.atoi (cregex.group (4), 10)
		r['dmin'] = string.atoi (cregex.group (5), 10)
		r['name'] = cregex.group (6)
	else:
		raise ValueError, "unable to parse line '%s'" % s

	return r

def verification_data (e):

	if (e['type'] == 's'):
		return 's "%s"' % (e['link'])

	if (e['type'] == 'f'):
		return 'f %04o %s/%s %s' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['md5'])

	if (e['type'] == 'l'):
		return 'l %04o %s/%s "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['link'])

	if (e['type'] == 'c'):
		return 'c %04o %s/%s %d,%d' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['dmaj'], e['dmin'])

	if (e['type'] == 'b'):
		return 'b %04o %s/%s %d,%d' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['dmaj'], e['dmin'])

	if (e['type'] == 'd'):
		return 'd %04o %s/%s' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'])

	raise ValueError, "unknown type " + str (e['type'])

def verification_entry (e):

	if (e['type'] == 's'):
		return 's "%s" "%s"' % (e['name'], e['link'])

	if (e['type'] == 'f'):
		return 'f %04o %s/%s %s "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['md5'], e['name'])

	if (e['type'] == 'l'):
		return 'l %04o %s/%s "%s" "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['name'], e['link'])

	if (e['type'] == 'c'):
		return 'c %04o %s/%s %d,%d "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['dmaj'], e['dmin'], e['name'])

	if (e['type'] == 'b'):
		return 'b %04o %s/%s %d,%d "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['dmaj'], e['dmin'], e['name'])

	if (e['type'] == 'd'):
		return 'd %04o %s/%s "%s"' % (stat.S_IMODE (e['mode']), e['uname'], e['gname'], e['name'])

	raise ValueError, "unknown type " + str (e['type'])

def certificate_string (s):
	f = outstr.outstr ()
	write_certificate (s, f)
	r = f.getstr ()
	f.close ()
	return r

def write_certificate (s, f):

	f.write ('Package: %s\n' % s['package'])
	f.write ('Maintainer: %s\n' % s['maintainer'])
	f.write ('Version: %s\n' % s['version'])
	f.write ('Architecture: %s\n' % s['architecture'])
	f.write ('Verification-Date: %s\n' % s['verification-date'])
	f.write ('Certificate-Type: %s\n' % s['certificate-type'])
	f.write ('Certificate-Version: %s\n' % '0.1')
	f.write ('Configuration-Files:\n')
	if (s.has_key ('configuration-files')):
		for l in s['configuration-files']:
			f.write (' "%s"\n' % l)
	f.write ('Control-Files:\n')
	for l in s['control-files']:
		f.write (' %s\n' % verification_entry (l))
	f.write ('Data-Files:\n')
	for l in s['data-files']:
		f.write (' %s\n' % verification_entry (l))

def parse_field (package, field, logger):

	key, lineno, val, detail = string.lower (field[0]), field[1], field[2], field[3]

	if (package.has_key (key)):
		logger ('warning', 'duplicate key "%s" on line %d' % (key, lineno))
		
	if (key == 'configuration-files'):
		package[key] = map (lambda e: e[1][2:-1], detail)
		return

	if (key in ['control-files', 'data-files']):
		package[key] = map (lambda e: parse_line (e[1][1:]), detail)
		return

	return dpkg_packages.parse_field (package, field, logger)

def parse_entry (entry, logger):
	package = {}
	for field in entry:
		parse_field (package, field, logger)
	return package

def read_entry (file, logger):
	entry = dpkg_message.read_entry (file)
	if (entry == None):
		return None
	return parse_entry (entry, logger)

def read (file, logger):
	ret = dpkg_message.read (file)
	plist = []
	for entry in ret:
		plist.append (parse_entry (entry, logger))
	return plist
