# $Id: dpkg_util.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import md5, string, os, sys, posix, posixmodule

blocksize = 16 * 1024

def stripabs (l):
	if (l[0] == ''):
		l = l[1:]
	return l

def dircanon (l):
	return string.join (l, '/')

def dirparse (s):
	if (s == ''):
		raise ValueError, 'invalid directory ""'
	p = string.split (s, '/')
	if ((len (p) > 1) and (p[-1] == '')):
		p = p[:-1]
	if (len (p) == 1):
		return p
	return filter (lambda s: s != '.', p)

def file_exists (f):
	try:
		posix.stat (f)
		return 1
	except posix.error, (a, b):
		if (a != 2):
			raise sys.exc_type, sys.exc_value, sys.exc_traceback
		return 0

def dir_exists (f):
	try:
		posix.stat (f)
		return 1
	except posix.error, (a, b):
		if (a != 2):
			raise sys.exc_type, sys.exc_value, sys.exc_traceback
		return 0

def docmd (cmd, argv, dry_run, verbose, prefix = ''):
	if (verbose):
		print prefix + string.join (argv, ' ')
	if (dry_run):
		return
	pid = os.fork ()
	if (pid == 0):
		os.execv (cmd, argv)
	pid, status = os.waitpid (pid, 0)
	if (status != 0):
		raise ValueError, 'command %s returned error status %s' % (string.join (argv, ' '), str (ret))

def doenv (cmd, argv, env, dry_run, verbose, prefix = ''):
	if (verbose):
		print prefix + string.join (argv, ' ')
	if (dry_run):
		return
	pid = os.fork ()
	if (pid == 0):
		os.execv (cmd, argv)
	pid, status = os.waitpid (pid, 0)
	if (status != 0):
		raise ValueError, 'command %s returned error status %s' % (string.join (argv, ' '), str (ret))

def doenvuser (cmd, argv, env, uid, root, dry_run, verbose, prefix = ''):
	if (verbose):
		print prefix + string.join (argv, ' ')
	if (dry_run):
		return
	pid = os.fork ()
	if (pid == 0):
		posix.chdir (root)
		posixmodule.chroot (root)
		posix.setuid (uid)
		os.execve (prog, argv, env)
	pid, status = os.waitpid (pid, 0)
	if (status != 0):
		raise ValueError, 'command %s returned error status %s' % (string.join (argv, ' '), str (ret))

def user_apply (uid, root, f, args):

	pid = os.fork ()
	if (pid == 0):
		posix.chdir (root)
		posixmodule.chroot (root)
		posix.setuid (uid)
		apply (f, args)
		os._exit (0)
	pid, status = os.waitpid (pid, 0)
	if (status != 0):
		raise ValueError, 'command %s (%s) returned error status %s' % (f, str (args), str (ret))

def to_hex (s):
	r = ''
	for i in range (len (s)):
		r = r + '%02x' % ord (s[i])
	return r

def md5_file (f):
	m = md5.new ()
	while 1:
		s = f.read (blocksize)
		if (s == ''): 
			break
		m.update (s)
	return to_hex (m.digest ())

def disclaimer ():
	return '''
-----------------------------------------------------------------------
THIS IS EXPERIMENTAL SOFTWARE, NOT AN OFFICIAL PRODUCT OF THE DEBIAN
PROJECT.  USE AT YOUR OWN RISK.

Although this program is intended to be used to verify the integrity
of an installed Debian GNU/Linux system, it is currently suited for
experimental use only.  It is likely to contain bugs and exploitable
security flaws, and should not be used in any production capacity.
-----------------------------------------------------------------------


'''

verifier_keyname = 'Debian Package Verifier [INSECURE] [FOR EXPERIMENTAL USE ONLY] <klee@debian.org>'
verifier_keyid = 'f4edcc1d'
verifier_fingerprint = 'fee423408b6c92af19929ff3436a513b'
