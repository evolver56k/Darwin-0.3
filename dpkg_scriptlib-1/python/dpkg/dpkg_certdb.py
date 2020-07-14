import gdbm, time, pgp, strfile, types
import dpkg_packages, dpkg_util, dpkg_certificate

class dpkg_certdb:
	
	def __init__ (self, fn):
		self.fn = fn

	def store (self, key, val, logger):
		while 1:
			try:
				db = gdbm.open (self.fn, 'w')
				break
			except gdbm.error, v:
				logger ('info', "error %d opening gdbm file ('%s'), retrying" % v)
				time.sleep (5)
		db[key] = val
		db.close ()

	def keys (self):
		db = gdbm.open (self.fn, 'r')
		r = db.keys (); db.close (); return r

	def create (self):
		db = gdbm.open (self.fn, 'c')
		db.close ()

	def clear (self):
		db = gdbm.open (self.fn, 'n')
		db.close ()

	def fetch_key (self, key):
		db = gdbm.open (self.fn, 'r')
		r = db[key]; db.close (); return r

	def fetch_package (self, package, architecture, log):
	
		db = gdbm.open (self.fn, 'r')

		if (type (package) == types.StringType):
			pname = package
		else:
			pname = package['package'] + '_' + dpkg_packages.package_canon_version (package)
			if (db.has_key (pname + '_' + architecture)):
				pname = pname + '_' + architecture
			elif (db.has_key (pname + '_' + 'all')):
				pname = pname + '_' + 'all'
			else:
				raise KeyError, pname

		signed = db[pname]
		db.close ()

		body, sig = pgp.parse_message (signed)

		pgp.verify (body, sig, dpkg_util.verifier_keyname, dpkg_util.verifier_keyid)
			
		f = strfile.strfile (body)
		c = dpkg_certificate.read_entry (f, log.log)
		f.close ()

		cname = dpkg_packages.package_canon_name (c)
		if (cname != pname):
			raise ValueError, 'corrupt certificate database (certificate for key "%s" is actually "%s")' % (pname, cname)
		
		return c

	def iterate (self, f):
		db = gdbm.open (self.fn, 'r')
		for key in db.keys():
			f (key, db)

	def has_key (self, key):
		db = gdbm.open (self.fn, 'r')
		r = db.has_key (key); db.close (); return r

	def close (self):
		pass
