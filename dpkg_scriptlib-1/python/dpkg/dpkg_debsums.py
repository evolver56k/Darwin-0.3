# $Id: dpkg_debsums.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import tar, sys, ar, time, procfilter, outstr, stat
import dpkg_newformat, dpkg_oldformat, dpkg_packages, dpkg_util

def convert_entry (e, prefix = ''):

	r = {}

	t_conv = { '0' : 'f', '1' : 'l', '2' : 's', '3' : 'c', '4' : 'b', '5' : 'd' }
	if (e['type'] in t_conv.keys()):
		r['type'] = t_conv[e['type']]
	else:
		raise ValueError, 'unknown type %s' % e['type']
	
	r['name'] = prefix + e['name']

	if (r['type'] in ['f', 'l', 'c', 'b', 'd']):

		if (e['uname'] != ''):
			r['uname'] = e['uname']
		else:
			r['uname'] = str (int (e['uid']))
		if (e['gname'] != ''):
			r['gname'] = e['gname']
		else:
			r['gname'] = str (int (e['gid']))

		r['mode'] = stat.S_IMODE (e['mode'])

	if (r['type'] in ['b', 'c']):
		r['dmaj'] = e['dmaj']
		r['dmin'] = e['dmin']

	if (r['type'] in ['l']):
		r['link'] = prefix + e['link']

	if (r['type'] in ['s']):
		r['link'] = e['link']

	if (r['type'] in ['f']):
		r['md5'] = dpkg_util.md5_file (e['file'])

	return r

def do_data (certificate, tar_entry, logger, prefix = ''):

	if (tar_entry['name'] == './'):
		return
	certificate['data-files'].append (convert_entry (tar_entry, prefix))

def do_control (certificate, tar_entry, logger):

	if (tar_entry['name'] == './'):
		return
	if (tar_entry['name'] == 'control'):
		p = dpkg_packages.read (tar_entry['file'], logger)
		if (len (p) != 1):
			raise ValueError, 'error parsing control file'
		certificate['control'] = p[0]
	if (tar_entry['name'] == 'conffiles'):
		l = tar_entry['file'].readlines ()
		for i in range (len (l)):
			l[i] = l[i][:-1]
		certificate['configuration-files'] = l
	certificate['control-files'].append (convert_entry (tar_entry))

def parse_changes (f, logger):
	pass

def parse_deb (f, logger):

	try:
		d = dpkg_newformat.parse (f)
	except ValueError, s:
		e = 'file is not a new-format debian archive'
		if (s[0:len(e)] != e):
			raise sys.exc_type, sys.exc_value, sys.exc_traceback
		d = dpkg_oldformat.parse (f)
	
	r = { }

	r['certificate-type'] = 'binary-only'

	r['control-files'] = []
	zf = procfilter.procfilter (d['control'], 'gzip -cd')
	tar.parse_file (zf, lambda f, t, r = r, logger = logger: do_control (r, t, logger))
	zf.close ()

	r['data-files'] = []
	zf = procfilter.procfilter (d['data'], 'gzip -cd')
	tar.parse_file (zf, lambda f, t, r = r, logger = logger: do_data (r, t, logger, '/'))
	zf.close ()

	r['verification-date'] = time.strftime ('%a %b %d %H:%M:%S -000', time.localtime (time.time ()))

	c = r['control']
	
	r['package'] = c['package']
	r['maintainer'] = c['maintainer']
	if (c.has_key ('architecture')):
		r['architecture'] = c['architecture']
	else:
		r['architecture'] = 'i386'
	r['version'] = dpkg_packages.package_canon_version (c)
	return r
