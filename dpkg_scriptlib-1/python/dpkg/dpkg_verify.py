# $Id: dpkg_verify.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import sys, string, regex, os, pwd, grp, posix, stat
import dpkg_debsums, dpkg_certificate, dpkg_util

def parse_stat (s):
	r = {}
	r['mode'], r['ino'], r['dev'], r['nlink'], r['uid'], r['gid'], r['size'], r['atime'], r['mtime'], r['ctime'] = s
	return r

def parse_passwd (s):
	r = {}
	try:
		p = pwd.getpwuid (s['uid'])
		r['name'], r['passwd'], r['uid'], r['gid'], r['gecos'], r['dir'], r['shell'] = p
	except KeyError:
		r['uid'] = s['uid']
		r['name'] = str (s['uid'])
	return r

def parse_group (s):
	r = {}
	try:
		g = grp.getgrgid (s['gid'])
		r['name'], r['passwd'], r['gid'], r['mem'] = g
	except KeyError:
		r['gid'] = s['gid']
		r['name'] = str (s['gid'])
	return r

VerificationError = 'verification-error'

def gen_errstr (r, s):
	return 'file "%s" %s (should be %s)' % (r['name'], s, dpkg_certificate.verification_entry (r))

def verify (r):

	if (r['type'] not in ['f', 'l', 'c', 'b', 'd', 's']):
		raise ValueError, "unknown type code '%s'" % r['type']

	if (r['type'] in ['f', 'l', 'c', 'b', 'd']):
		try:
			s = parse_stat (os.lstat (r['name']))
			p = parse_passwd (s)
			g = parse_group (s)
		except posix.error, v:
			if (v[0] == 2):
				raise VerificationError, gen_errstr (r, 'does not exist')
			elif (v[0] == 13):
				raise VerificationError, gen_errstr (r, 'permission denied on stat')
			else:
				raise sys.exc_type, sys.exc_value, sys.exc_traceback

	if (r['type'] in ['f', 'l', 'c', 'b', 'd']):
		if (stat.S_IMODE (r['mode']) != stat.S_IMODE (s['mode'])):
			raise VerificationError, gen_errstr (r, 'has permissions %04o' % stat.S_IMODE (s['mode']))
		if (r['uname'] != p['name']):
			raise VerificationError, gen_errstr (r, 'has wrong owner %s' % (p['name']))
		if (r['gname'] != g['name']):
			raise VerificationError, gen_errstr (r, 'has wrong group %s' % (g['name']))

	if (r['type'] in ['f']):

		try:
			fd = open (r['name'], 'r')
			md5 = dpkg_util.md5_file (fd)
		except IOError, v:
			if (v[0] == 13):
				raise VerificationError, gen_errstr (r, 'unable to compute MD5 (permission denied)')
			else:
				raise sys.exc_type, sys.exc_value, sys.exc_traceback
		
		if (r['md5'] != md5):
			raise VerificationError, gen_errstr (r, 'has bad MD5 %s' % md5)

	if (r['type'] in ['s']):
		try:
			link = posix.readlink (r['name'])
		except posix.error, v:
			if (v[0] == 2):
				raise VerificationError, gen_errstr (r, 'does not exist')
			elif (v[0] == 22):
				raise VerificationError, gen_errstr (r, 'is not a symbolic link')
			else:
				raise sys.exc_type, sys.exc_value, sys.exc_traceback
		if (link != r['link']):
			raise VerificationError, gen_errstr (r, 'has incorrect destination "%s")' % link)

	if (r['type'] == 'l'):
		try:
			s_to = parse_stat (os.stat (r['link']))
		except posix.error, v:
			if (v[0] == 2):
				raise VerificationError,  'destination for hard link "%s" does not exist (should be -> "%s")' % (r['name'], r['link'])
			else:
				raise sys.exc_type, sys.exc_value, sys.exc_traceback
		if (s['ino'] != s_to['ino']):
			raise VerificationError, 'hard link "%s" should point to "%s" (inode %d)' % (r['name'], r['link'], s_to['ino'])

	return
		
def verify_certificate (entry, logger):
	for r in entry['data-files']:
		if (entry.has_key ('configuration-files')):
			try:
				verify (r)
			except VerificationError, s:
				if (r['name'] in entry['configuration-files']):
					logger ('warning', 'configuration file ' + s)
				else:
					logger ('error', s)
