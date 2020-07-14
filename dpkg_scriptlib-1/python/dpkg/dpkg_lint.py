# $Id: dpkg_lint.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import traceback, marshal
import dpkg_index, dpkg_version, dpkg_certdb, dpkg_packages, dpkg_filedb, dpkg_certificate

def check_distribution (tag, packages, allowed, all, logger):

	details = []

	for p in packages:

		try:
			dpkg_version.parse_version (p['version'])
		except ValueError, v:
			print 'error parsing version for package "%s": %s' % (p['package'], v)
			continue

		olist = [ 'xbaseR6', 'xdevel', 'xlib', 'xlibraries', 'X11R6', 'xR6shlib' ]
		vlist = [ 'elf-x11r6lib', 'aout-x11r6lib', 'xserver',
				  'mail-transport-agent', 'mail-reader', 'news-transport-system', 'news-reader',
				  'inews', 'pgp', 'imap-client', 'imap-server',
				  'java-base-libs', 'java-awt-libs', 'java-compiler', 'java-virtual-machine', 'java-appletviewer',
				  'libc.so.4', 'info-browser', 'kernel-source', 'kernel-headers', 'kernel-image',
				  'httpd', 'postscript-viewer', 'postscript-preview', 'www-browser', 'awk', 'c-shell',
				  'pdf-viewer', 'pdf-preview',
				  'compress', 'emacs', 'sgmls', 'picons' ]

		for key in ['pre-depends', 'depends', 'recommends']:
			if (p.has_key (key)):
				ret1, details1 = dpkg_index.check_deps (p['package'], p[key], allowed)
				ret2, details2 = dpkg_index.check_deps (p['package'], p[key], all)
				for i in xrange (len (details1)):
					if (ret1 != 0):
						if (details1[i][1][0:10] == 'unresolved'):
							details1[i] = ('info', details1[i][1] + ' (resolved by alternative)')
					elif (ret2 != 0):
						if (details1[i][1][0:10] == 'unresolved'):
							details1[i] = ('warning', details1[i][1] + ' (resolved in inaccessible distribution)')
					details1[i] = (details1[i][0], p['package'], '%s for package %s in %s: ' % (key, p['package'], tag) + details1[i][1])
				details = details + details1

		if (p.has_key ('suggests')):
			ret2, details2 = dpkg_index.check_deps (p['package'], p['suggests'], all)
			for i in xrange (len (details2)):
				if (details2[i][1][0:10] == 'unresolved'):
					details2[i] = ('info', details2[i][1] + ' (suggestion only)')
				details2[i] = (details2[i][0], p['package'], 'suggests for package %s in %s: ' % (p['package'], tag) + details2[i][1])
			details = details + details2

	return 0, details

authoritative_list = ['base-files', 'libc5-dev', 'xbase', 'man-db', 'emacs', 'menu']

def is_authoritative (p):
	if (p[0:13] == 'ucbmpeg_play_'):
		return 0
	return (dpkg_packages.parse_package_name (p)['package'] in authoritative_list)

def most_authoritative (l):

	for a in authoritative_list:
		cur = None
		for p in l:
			if (p[0:13] == 'ucbmpeg_play_'):
				continue
			if (dpkg_packages.parse_package_name (p)['package'] == a):
				if (cur == None): 
					cur = p
				else:
					pcur = dpkg_packages.parse_package_name (cur)
					pp = dpkg_packages.parse_package_name (p)
					if (dpkg_version.compare_versions (pp['version'], pcur['version']) > 0):
						cur = p
		if (cur != None):
			return cur

	return None

def has_authoritative (l):
	for p in l:
		if (p[0:13] == 'ucbmpeg_play_'):
			continue
		if (dpkg_packages.parse_package_name (p)['package'] in authoritative_list):
			return 1
	return 0

def check_certificate (cdb, fdb, pdb, certificate, cfunc):

	package = dpkg_packages.package_canon_name (certificate)
	for file in certificate['data-files']:

		if (file['name'][-1] == '/'):
			filename = file['name'][0:-1]
		else:
			filename = file['name']
		filemap = dpkg_filedb.parse_files (fdb.fetch_file (filename))

		def ncfunc (pkg1, pkg2, field, msg, pdb = pdb, filename = filename, filemap = filemap, cfunc = cfunc):
			if (ignore_conflict (filemap, pkg1, pkg2, pdb, field)):
				return
			cfunc (filename, filemap, pkg1, pkg2, field, msg)
			
		check_filemap (filemap, package, ncfunc)

def check_file (cdb, fdb, pdb, filename, cfunc):

	filemap = dpkg_filedb.parse_files (fdb.fetch_file (filename))

	def ncfunc (pkg1, pkg2, field, msg, pdb = pdb, filename = filename, filemap = filemap, cfunc = cfunc):
		if (ignore_conflict (filemap, pkg1, pkg2, pdb, field)):
			return
		cfunc (filename, filemap, pkg1, pkg2, field, msg)
			
	a = most_authoritative (filemap.keys())
	if (a == None):
		a = filemap.keys()[0]

	check_filemap (filemap, a, ncfunc)

def check_files (cdb, fdb, pdb, cfunc):

	for filename in fdb.keys ():
		if (filename[0] == '.'): continue
		check_file (cdb, fdb, pdb, filename, cfunc)

def ignore_conflict (filemap, pkg1, pkg2, pdb, field):

	if ((not pdb.has_key (pkg1)) or (not pdb.has_key (pkg2))):
		return 1
	if (dpkg_packages.parse_package_name (pkg1)['package'] == dpkg_packages.parse_package_name (pkg2)['package']):
		return 1
	if (not dpkg_packages.packages_compatible (marshal.loads (pdb[pkg1]), marshal.loads (pdb[pkg2]))):
		return 1
	if ((filemap[pkg1]['type'] == 'd') and (field in ['mode', 'uname', 'gname'])):
		if (has_authoritative (filemap.keys ()) and not (is_authoritative (pkg1) or is_authoritative (pkg2))):
			return 1
	return 0

def check_filemap (filemap, pkg1, cfunc):
	
	f1 = filemap[pkg1]

	for pkg2 in filemap.keys ():

		f2 = filemap[pkg2]

		if (f1['type'] != f2['type']):
			cfunc (pkg1, pkg2, 'type', 'file types differ')
			continue

		if (f1['type'] == 'd'):
			if (f1['mode'] != f2['mode']): cfunc (pkg1, pkg2, 'mode', 'directory modes differ')
			if (f1['uname'] != f2['uname']): cfunc (pkg1, pkg2, 'uname', 'directory user ownerships differ')
			if (f1['gname'] != f2['gname']): cfunc (pkg1, pkg2, 'gname', 'directory group ownerships differ')

		elif (f1['type'] == 'f'):
			if (f1['mode'] != f2['mode']): cfunc (pkg1, pkg2, 'mode', 'file modes differ')
			if (f1['uname'] != f2['uname']): cfunc (pkg1, pkg2, 'uname', 'file user ownerships differ')
			if (f1['gname'] != f2['gname']): cfunc (pkg1, pkg2, 'gname', 'file group ownerships differ')
			if (f1['md5'] != f2['md5']): cfunc (pkg1, pkg2, 'md5', 'file MD5 checsums differ')
		elif (f1['type'] == 'l'):
			if (f1['mode'] != f2['mode']): cfunc (pkg1, pkg2, 'mode', 'hard link modes differ')
			if (f1['uname'] != f2['uname']): cfunc (pkg1, pkg2, 'uname', 'hard link user ownerships differ')
			if (f1['gname'] != f2['gname']): cfunc (pkg1, pkg2, 'gname', 'hard link group ownerships differ')
			if (f1['link'] != f2['link']): cfunc (pkg1, pkg2, 'link', 'hard link destinations differ')
		elif (f1['type'] == 's'):
			if (f1['link'] != f2['link']): cfunc (pkg1, pkg2, 'link', 'symbolic link destinations differ')
		elif (f1['type'] == 'b'):
			if (f1['mode'] != f2['mode']): cfunc (pkg1, pkg2, 'mode', 'block device modes differ')
			if (f1['uname'] != f2['uname']): cfunc (pkg1, pkg2, 'uname', 'block device user ownerships differ')
			if (f1['gname'] != f2['gname']): cfunc (pkg1, pkg2, 'gname', 'block device group ownerships differ')
			if (f1['dmaj'] != f2['dmaj']): cfunc (pkg1, pkg2, 'dmaj', 'block device major numbers differ')
			if (f1['dmin'] != f2['dmin']): cfunc (pkg1, pkg2, 'dmin', 'block device minor numbers differ')
		elif (f1['type'] == 'c'):
			if (f1['mode'] != f2['mode']): cfunc (pkg1, pkg2, 'mode', 'character device modes differ')
			if (f1['uname'] != f2['uname']): cfunc (pkg1, pkg2, 'uname', 'character device user ownerships differ')
			if (f1['gname'] != f2['gname']): cfunc (pkg1, pkg2, 'gname', 'character device group ownerships differ')
			if (f1['dmaj'] != f2['dmaj']): cfunc (pkg1, pkg2, 'dmaj', 'character device major numbers differ')
			if (f1['dmin'] != f2['dmin']): cfunc (pkg1, pkg2, 'dmin', 'character device minor numbers differ')
		else:
			raise ValueError, "unknown type '%s'" % f1['type']
