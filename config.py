#!/usr/bin/python

# Update version in pyproject.toml

from src.version import __version__

print('Reading version %s from src/version.py' % __version__)

def replace(fn, start, new):
	with open(fn, 'r') as f:
		txt = [s.rstrip() for s in f]

	for i, s in enumerate(txt):
		if s.startswith(start):
			txt[i] = new

	print('Writing version to %s ' % fn, end = '...')

	with open(fn, 'w') as f:
		f.write('%s\n' % '\n'.join(txt))

	print(' Ok.')


replace('pyproject.toml', 'version', "version\t\t\t= '%s'" % __version__)
replace('src/reels/__init__.py', '__version__', "__version__ = '%s'" % __version__)

print('\nDone.')
