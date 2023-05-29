#!/usr/bin/python

# Update version in pyproject.toml

from src.version import __version__

print('Reading version %s from src/version.py' % __version__)

with open('pyproject.toml', 'r') as f:
	txt = [s.rstrip() for s in f]

for i, s in enumerate(txt):
	if s.startswith('version'):
		txt[i] = "version\t\t\t= '%s'" % __version__

print('Writing version to pyproject.toml.')

with open('pyproject.toml', 'w') as f:
	f.write('%s\n' % '\n'.join(txt))

print('\nDone.')
