import string, regex, regex_syntax, copy

class backupfile:
	def __init__ (self, f):
		self.f = f
		self.cur = None
		self.dobackup = 0
	def rl (self):
		if (self.dobackup):
			self.dobackup = 0
			return self.cur
		else:
			self.cur = self.f.readline ()
			if (self.cur == ''):
				return self.cur
			if (self.cur[-1] != '\n'):
				raise ValueError, 'diff file does not end with newline'
			return self.cur
	def peekrl (self):
		l = self.rl ()
		self.backup ()
		return l
	def checkrl (self):
		l = self.rl ()
		if (l == ''):
			raise ValueError, 'premature end-of-file in diff file'
		return l
	def backup (self):
		self.dobackup = 1;

def quote_char (c):
	if (c == '\t'):
		return '\\t'
	elif (c == '\n'):
		return '\\n'
	elif (ord (c) < 128):
		return c
	else:
		return str (ord (c))
	
def parse_context_diff (df, template):

	regex.set_syntax (regex_syntax.RE_NO_BK_PARENS | regex_syntax.RE_NO_BK_VBAR)

	fromr = regex.compile ('^\*\*\* ([^\t ]*)[\n\t ]')
	tor = regex.compile ('^--- ([^\t ]*)[\n\t ]')
	dstartr = regex.compile ('^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$')
	fromstartr = regex.compile ('^\*\*\* ([0-9]+),([0-9]+) \*\*\*\*$')
	tostartr = regex.compile ('^--- ([0-9]+),([0-9]+) ----$')

	diff = {}
	diff['type'] = 'context'

	l = df.checkrl ()
	if (fromr.match (l) < 0):
		raise ValueError, ('unable to determine name of original file from diff line "%s"' % l)
	diff['from'] = fromr.group (1);

	l = df.checkrl ()
	if (tor.match (l) < 0):
		raise ValueError, ('unable determine to name of patched file from diff line "%s"' % l)
	diff['to'] = tor.group (1);

	while (1):

		l = df.rl ()
		if (l == ''): return diffs

		if (dstartr.match (l) < 0):
			raise ValueError, ('file did not have diff start block in appropriate location: %s' % l)

		l = df.checkrl ()

		if (fromstartr.match (l) < 0):
			raise ValueError, ('file did not have diff from block in appropriate location: %s' % l)
		fromskip = string.atoi (fromstartr.group (2))
		for i in range (fromskip):
			l = df.checkrl ()
			if (l[0] not in [' ', '+', '-', '!']):
				raise ValueError, ('diff line contains invalid character: %s' % l)

		l = df.checkrl ()
		if (l[0:12] != '\ No newline'):
			df.backup ()

		l = df.checkrl ()

		if (tostartr.match (l) < 0):
			raise ValueError, ('file did not have diff to block in appropriate location: %s' % l)
		toskip = string.atoi (tostartr.group (2))
		for i in range (toskip):
			l = df.checkrl ()
			if (l[0] not in [' ', '+', '-', '!']):
				raise ValueError, ('diff line contains invalid character: %s' % l)

		l = df.rl ()
		if (l == ''):
			break
		if (l[0:12] != '\ No newline'):
			df.backup ()

		l = df.peekrl ()
		if (l == ''):
			break
		if (l[0:5] == 'diff '):
			break

	return diff

def parse_unified_diff (df, template):

	regex.set_syntax (regex_syntax.RE_NO_BK_PARENS | regex_syntax.RE_NO_BK_VBAR)

	fromr = regex.compile ('^--- ([^\t ]*)[\n\t ]')
	tor = regex.compile ('^\+\+\+ ([^\t ]*)[\n\t ]')
	startr = regex.compile ('^@@ -(([0-9]+),)?([0-9]+) \+(([0-9]+),)?([0-9]+) @@$')

	l = df.checkrl ()
	if (fromr.match (l) < 0):
		raise ValueError, ('unable to determine name of original file from diff line "%s"' % l)
	difffrom = fromr.group (1);

	l = df.checkrl ()
	if (tor.match (l) < 0):
		raise ValueError, ('unable determine to name of patched file from diff line "%s"' % l)
	diffto = tor.group (1);

	diffs = []

	while (1):

		l = df.rl ()
		if (l == ''):
			return diffs

		if (startr.match (l) < 0):
			raise ValueError, ("file did not have diff start block in appropriate location: '%s'" % l)

		diff = copy.deepcopy (template)

		diff['type'] = 'unified'
		diff['linespec'] = l[0:-1]
		diff['diff-from'] = difffrom
		diff['diff-to'] = diffto
		diff['lines'] = []

		while (1):
			l = df.rl ()
			if (l == ''): 
				break
			if (l[0:2] == '@@'):
				df.backup ()
				break
			if (l[0:5] == 'diff '):
				df.backup ()
				break
			if (l[0:4] == '--- '):
				df.backup ()
				break
			if (l[0:4] == '+++ '):
				df.backup ()
				break
			if (l[0:4] == '*** '):
				df.backup ()
				break
			if (l[0:12] == '\ No newline'):
				diff['lines'].append (l)
				continue
			if (l[0] not in [' ', '+', '-', '!']):
				raise ValueError, ("diff line starts with invalid character '%s': '%s'" % (quote_char (l[0]), l))
			diff['lines'].append (l)
			continue

		diffs.append (diff)

		l = df.peekrl ()
		if (l == ''):
			break
		if (l[0:5] == 'diff '):
			break
		if (l[0:4] == '--- '):
			break
		if (l[0:4] == '+++ '):
			break
		if (l[0:4] == '*** '):
			break

	return diffs

def parse_diff (f):

	diffs = []
	df = backupfile (f)

	template = {}

	while (1):

		l = df.rl ()
		if (l == ''):
			return diffs

		if (l[-1] != '\n'):
			raise ValueError, ('diff line does not end in newline: %s' % l)
		l = l[0:-1]

		if (l[0:4] == '*** '):
			df.backup ()
			ndiffs = parse_context_diff (df, template)
			diffs.append (ndiffs)
			continue
		elif (l[0:4] == '--- '):
			df.backup ()
			ndiffs = parse_unified_diff (df, template)
			diffs.append (ndiffs)
			continue

		v = string.split (l, ' ')
		if (v[0] != 'diff'):
			raise ValueError, ('diff line does not start with \'diff\': %s' % l)
		if (len (v) < 3):
			raise ValueError, ('diff line does not contain two filenames: %s' % l)

		template['file-from'] = v[-1]
		template['file-to'] = v[-2]
		template['file-args'] = v[1:-2]

		continue
	
	return diffs

def parse_output_header (df):

	header = {}
	hlines = []

	l = df.rl ()
	if (l != '--------------------------\n'):
		raise ValueError, ('header starts with unexpected line: %s' % l)

	while (1):
		l = df.rl ()
		if (l == ''):
			raise ValueError, ('end-of-file while parsing header')
		if (l == '--------------------------\n'):
			break
		hlines.append (l)

	l = hlines[0]
	if (l[0:6] == '|diff '):
		if (l[-1:] != '\n'):
			raise ValueError, ('header contains line not ending in newline: %s' % l)
		l = l[:-1]
		v = string.split (l, ' ')
		if (len (v) < 3):
			raise ValueError, ('diff line does not contain two filenames: %s' % l)

		header['file-from'] = v[-1]
		header['file-to'] = v[-2]
		header['file-args'] = v[1:-2]

		hlines = hlines[1:]

	if (len (hlines) != 2):
		raise ValueError, ('invalid header: %s' % str (hlines))

	v = string.split (hlines[0])
	header['diff-from'] = v[1]

	v = string.split (hlines[1])
	header['diff-to'] = v[1]

	return header

def parse_output (f):

	diffs = []
	df = backupfile (f)

	while (1):

		while (1):
			l = df.rl ()
			if (l == ''):
				break
			if (l == '--------------------------\n'):
				df.backup ()
				break
		if (l == ''):
			break

		header = parse_output_header (df)
		header['hunks'] = []

		regex.set_syntax (regex_syntax.RE_NO_BK_PARENS | regex_syntax.RE_NO_BK_VBAR)
		hunkr = regex.compile ('^Hunk \#([0-9]+) ([a-zA-Z]+) at ([0-9]+)( \(offset (\-?[0-9]+) lines?\))?\.$')

		i = 1
		while (1):
			l = df.rl ()
			if (l == ''):
				break
			if (l[0:5] == 'Hunk '):
				if (hunkr.match (l) < 0):
					raise ValueError, ('unable to parse hunk result \"%s\"' % l)
				hunkno = string.atoi (hunkr.group (1))
				status = hunkr.group (2)
				lineno = string.atoi (hunkr.group (3))
				if (hunkr.group (5)):
					offset = string.atoi (hunkr.group (5))
				else:
					offset = 0
				if (hunkno != i):
					raise ValueError, ('invalid hunk number %d' % hunkno)
				i = i + 1
				header['hunks'].append ([status, lineno, offset])
			if (l == '--------------------------\n'):
				df.backup ()
				break

		diffs.append (header)

		if (l == ''):
			break

	return diffs
