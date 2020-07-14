import socket, regex, regex_syntax, regsub, string
import traceback, types, pgp, strfile
import dpkg_packages, dpkg_util, dpkg_certificate

def chop (s):
	if ((s[-1] != '\n') or (s[-2] != '\r')):
		raise ValueError, ('invalid line "%s" received from server (ended with 0x%02x 0x%02x)' % 
						   (s, ord (s[-2]), ord (s[-1])))
	return s[:-2]

def process_client (db, r, w):

	regex.set_syntax (regex_syntax.RE_SYNTAX_EMACS)
	cregex = regex.compile ('^CERT[ \t]+\([-+_:.A-Za-z0-9]+\)$')

	w.write ('220 package verification server ready\r\n')
	w.flush ()
	while 1:
		l = r.readline ()
		if (l == ''):
			break
	   	l = chop (l)
		if (regex.match ('^[ \t]*$', l) >= 0):
			continue
		elif (regex.match ('^LIST[ \t]*$', l) >= 0):
			w.write ('211 certificate list follows\r\n')
			for key in db.keys(): 
				w.write (key + '\r\n')
			w.write ('.\r\n')
			w.flush ()
		elif (cregex.match (l) >= 0):
			key = cregex.group (1)
			if (db.has_key (key)):
				w.write ('210 certificate follows\r\n')
				cert = db.fetch_key (key)
				cert = regsub.gsub ('\n', '\r\n', cert)
				w.write (cert)
				w.write ('.\r\n')
			else:
				w.write ('501 no such certificate\r\n')
			w.flush ()
		elif (regex.match ('^HELP[ \t]*$', l) >= 0):
			w.write ('100 legal commands\r\n  CERT [package-name]\r\n  HELP\r\n  LIST\r\n  QUIT\r\n.\r\n')
			w.flush ()
		elif (regex.match ('^QUIT[ \t]*$', l) >= 0):
			w.write ('221 closing connection\r\n')
			w.flush ()
			break
		else:
			w.fwrite ('500 invalid command\r\n')
			w.flush ()

def do_server (db, port, logger):
	s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
	s.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.bind ('', port)
	s.listen (1)
	while 1:
		c, addr = s.accept()
		logger ('info', 'connection from %s port %d' % addr)
		r, w = c.makefile ('r'), c.makefile ('w')
		try:
			process_client (db, r, w)
		except IOError, v:
			logger ('error', 'client from %s port %d triggered server error' % addr)
			traceback.print_exc ()
		r.close (); w.close (); c.close ()
		logger ('info', 'client from %s port %d disconnected' % addr)

class certserver_connection:

	def __init__ (self, host, port):

		self.s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
		self.s.connect (host, port)
		self.r = self.s.makefile ('r')
		self.w = self.s.makefile ('w')
		s = self.r.readline ()
		s = chop (s)
		if (s[0:3] != '220'):
			raise ValueError, 'invalid response from server "%s"' % s

	def close (self):

		self.s.close ()
		self.r.close ()
		self.w.close ()
		
	def list (self):

		self.w.write ('LIST\r\n')
		self.w.flush ()
		available = []
		s = self.r.readline ()
		if (s == ''):
			raise ValueError, 'unexpected end-of-file parsing response to CERT command'
		s = chop (s)
		if (s[0:3] != '211'):
			raise ValueError, 'invalid response from server "%s"' % s
		while 1:
			s = self.r.readline ()
			if (s == ''):
				raise ValueError, 'unexpected end-of-file parsing response to LIST command'
			s = chop (s)
			if (s == '.'):
				break
			available.append (s)
		return available

	def has_key (self, key):
		try:
			if (self.fetch_key (key) == None): 
				return 0
			return 1
		except ValueError:
			return 0
			

	def fetch_key (self, key):

		self.w.write ('CERT %s\r\n' % key)
		self.w.flush ()
		s = self.r.readline ()
		if (s == ''):
			raise ValueError, 'unexpected end-of-file parsing response to CERT command'
		s = chop (s)
		if (s[0:3] == '501'):
			return None
		if (s[0:3] != '210'):
			raise ValueError, 'invalid response from server "%s"' % s

		body = ''
		signature = ''

		while 1:
			s = self.r.readline ()
			if (s == ''):
				raise ValueError, 'parse error looking for certificate'
			s = chop (s)
			if (s == ''):
				continue
			if (s == '-----BEGIN PGP SIGNED MESSAGE-----'):
				break
		if (self.r.readline () != '\r\n'):
			raise ValueError, 'missing ASCII armor before message body'
		while 1:
			s = self.r.readline ()
			if (s == ''):
				raise ValueError, 'end-of-file while parsing certificate'
			s = chop (s)
			if (s == '-----BEGIN PGP SIGNATURE-----'):
				break
			body = body + s + '\n'
		if (body[-1] != '\n'):
			raise ValueError, 'missing ASCII armor after message body'
		body = body[:-1]
		while 1:
			s = self.r.readline ()
			if (s == ''):
				raise ValueError, 'unexpected end-of-file parsing response to CERT command'
			s = chop (s)
			if (s == '-----END PGP SIGNATURE-----'):
				break
			signature = signature + s + '\n'

		while 1:
			s = self.r.readline ()
			if (s == ''):
				raise ValueError, 'unexpected end-of-file parsing response to CERT command'
			s = chop (s)
			if (s == ''):
				continue
			if (s == '.'):
				break
			raise ValueError, 'unexpected string "%s" after PGP certificate in response to CERT command' % s

		return body, signature


	def fetch_package (self, package, architecture, log):
	
		if (type (package) == types.StringType):
			pname = package
		else:
			pname = package['package'] + '_' + dpkg_packages.package_canon_version (package)
			if (self.has_key (pname + '_' + architecture)):
				pname = pname + '_' + architecture
			elif (self.has_key (pname + '_' + 'all')):
				pname = pname + '_' + 'all'
			else:
				raise KeyError, pname

		body, sig = self.fetch_key (pname)
		pgp.verify (body, sig, dpkg_util.verifier_keyname, dpkg_util.verifier_keyid)
			
		f = strfile.strfile (body)
		c = dpkg_certificate.read_entry (f, log.log)
		f.close ()

		cname = dpkg_packages.package_canon_name (c)
		if (cname != pname):
			raise ValueError, 'corrupt certificate database (certificate for key "%s" is actually "%s")' % (pname, cname)
		
		return c
