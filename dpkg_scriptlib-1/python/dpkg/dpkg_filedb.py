import gdbm, time, pgp, strfile, pickle, string
import dpkg_packages, dpkg_util, dpkg_certificate

def parse_files (s):
	lines = string.split (s, '\n')[:-1]
	m = {}
	for l in lines:
		p = dpkg_certificate.parse_fline (l)	
		m[p['package']] = p
	return m

class dpkg_filedb:
	
	def __init__ (self, fn):
		self.fn = fn
		self.p = None

	def store_certificate (self, c, log):

		while 1:
			try:
				db = gdbm.open (self.fn, 'w')
				break
			except gdbm.error, v:
				log.log ('info', "error %d opening gdbm file ('%s'), retrying" % v)
				time.sleep (5)

		if (self.p == None):
			self.p = pickle.loads (db['.packages'])

		pname = dpkg_packages.package_canon_name (c)

		for f in c['data-files']:
			n = f['name']
			if (n[-1] == '/'):
				n = n[:-1]
			if (db.has_key (n)):
				data = db[n]
			else:
				data = ''
			data = data + pname + ' ' + dpkg_certificate.verification_data (f) + '\n'
			db[n] = data

		self.p[pname] = None
		db['.packages'] = pickle.dumps (self.p)

		db.close ()

	def keys (self):
		db = gdbm.open (self.fn, 'r')
		r = db.keys (); db.close (); return r

	def packages (self):
		if (self.p == None):
			db = gdbm.open (self.fn, 'r')
			self.p = pickle.loads (db['.packages']) 
			db.close ();
		return self.p.keys ()

	def create (self):
		db = gdbm.open (self.fn, 'c')
		if (not db.has_key ('.packages')):
			db['.packages'] = pickle.dumps ({})
		db.close ()

	def clear (self):
		db = gdbm.open (self.fn, 'n')
		db.close ()

	def fetch_file (self, key):
		db = gdbm.open (self.fn, 'r')
		r = db[key]; db.close (); return r

	def iterate (self, f):
		db = gdbm.open (self.fn, 'r')
		for key in db.keys():
			f (key, db)

	def has_key (self, key):
		db = gdbm.open (self.fn, 'r')
		r = db.has_key (key); db.close (); return r

	def close (self):
		pass
