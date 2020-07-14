# $Id: pgp.py,v 1.1.1.1 1999/08/05 00:43:25 wsanchez Exp $
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

import subfile, string, os, sys, strfile, procfilter, outstr, regex, regex_syntax, strfile

pgpargs = '+tmp=/tmp +language=en +myname="" +batchmode=on +clearsig=on +textmode=on +charset=noconv +armor=on +armorlines=0 +compress=off +completes_needed=1 +marginals_needed=0 +cert_depth=1 +pubring=/usr/lib/dpkgcert/pubring.pgp +secring=/usr/lib/dpkgcert/secring.pgp +verbose=1 +interactive=off +nomanual=on'

def sign (body, uid, passphrase = ''):
	f = strfile.strfile (passphrase + '\n' + body)
	nf = procfilter.procfilter (f, "PGPPASSFD=0 pgp -fs %s -u '%s'" % (pgpargs, uid))
	s = nf.read ()
	bstr = '-----BEGIN PGP SIGNED MESSAGE-----\n'
	estr = '-----END PGP SIGNATURE-----\n'
	errtxt = nf.stderr().read()
	if ((s[:len (bstr)] != bstr) or (s[len(s)-len(estr):] != estr)):
		raise ValueError, 'error signing message: ' + string.translate (errtxt, string.maketrans ('\n', ' '))
	nf.close ()
	return s

regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
verexp = regex.compile ('^Good signature from user "\([^"]+\)"\.$')
sigexp = regex.compile ('^Signature made \(.*\) using \([0-9]+\)-bit key, key ID \([A-Z0-9]+\)$')
brexp = regex.compile ('[Bb]ad signature')

def parse_err (l):

	if (brexp.search (l[7][:-1]) >= 0):
		raise ValueError, 'bad signature'
	
	if (verexp.match (l[7][:-1]) < 0):
		raise ValueError, 'unable to parse PGP output "%s"' % l[7][:-1]
	if (sigexp.match (l[8][:-1]) < 0):
		raise ValueError, 'unable to parse PGP output "%s"' % l[8][:-1]

	r = {}
	r['keyid'] = sigexp.group (3)
	r['bits'] = string.atoi (sigexp.group (2))
	r['date'] = sigexp.group (1)
	r['keyname'] = verexp.group (1)

	return r

def verify (body, signature, keyname, keyid):
	r = checksig (body, signature)
	if (string.lower (r['keyid']) != string.lower (keyid)):
		raise ValueError, 'invalid keyid %s (should be %s)' % (r['keyid'], keyid)
	if (r['keyname'] != keyname):
		raise ValueError, 'invalid key name "%s" (should be "%s")' % (r['keyname'], keyname)

def checksig (body, signature):
	
	of = outstr.outstr ()
	write_message (of, body, signature)
	f = strfile.strfile (of.getstr ())
	of.close ()
	nf = procfilter.procfilter (f, 'pgp -f %s > /dev/null' % pgpargs)
	errlines = nf.stderr().readlines()
	f.close ()
	nf.close ()
	r = parse_err (errlines)
	return r

def write_message (f, body, sig):

	f.write ('-----BEGIN PGP SIGNED MESSAGE-----\n\n')
	f.write (body)
	f.write ('\n-----BEGIN PGP SIGNATURE-----\n')
	f.write (sig)
	f.write ('-----END PGP SIGNATURE-----\n')

def parse_message (s):
	f = strfile.strfile (s)
	r = read_message (f)
	f.close ()
	return r

def read_message (f):

	body = ''
	signature = ''

	while 1:
		s = f.readline ()
		if (s == ''):
			return None
		if (s == '\n'):
			continue
		if (s == '-----BEGIN PGP SIGNED MESSAGE-----\n'):
			break
		raise ValueError, 'parse error looking for certificate'
	if (f.readline () != '\n'):
		raise ValueError, 'missing ASCII armor before message body'
	while 1:
		s = f.readline ()
		if (s == ''):
			raise ValueError, 'end-of-file while reading certificate'
		if (s == '-----BEGIN PGP SIGNATURE-----\n'):
			break
		body = body + s
	if (body[-1] != '\n'):
		raise ValueError, 'missing ASCII armor after message body'
	body = body[:-1]
	while 1:
		s = f.readline ()
		if (s == ''):
			raise ValueError, 'end-of-file while reading certificate'
		if (s == '-----END PGP SIGNATURE-----\n'):
			break
		signature = signature + s

	return body, signature

