import os, warnings, setuptools

from setuptools.command.build_py import build_py as _build_py
from src.version import __version__


def warn(*args, **kwargs):
	pass

warnings.warn = warn


reels_ext = setuptools.Extension(name				= 'src.reels._py_reels',
								 sources			= ['src/reels/reels.cpp', 'src/reels/py_reels_wrap.cpp'],
								 extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])


class custom_build_py(_build_py):

	def run(self):
		self.run_command('build_ext')

		dis = self.distribution if type(self.distribution) != list else self.distribution[0]
		cob = dis.command_obj['build_ext']

		fn = cob.build_lib
		assert 'src' in os.listdir(fn)

		fn += '/src'
		assert 'reels' in os.listdir(fn)

		fn += '/reels'

		fnl = os.listdir(fn)
		assert len(fnl) == 1

		fn += '/' + fnl[0]

		ret = super().run()

		self.move_file(fn, fn.replace('src/reels', 'reels'))

		return ret

# Update version in pyproject.toml

with open('pyproject.toml', 'r') as f:
	txt = [s.rstrip() for s in f]

for i, s in enumerate(txt):
	if s.startswith('version'):
		txt[i] = "version\t\t\t= '%s'" % __version__

with open('pyproject.toml', 'w') as f:
	f.write('%s\n' % '\n'.join(txt))


setuptools.setup(
	name			 = 'mercury-reels',
	version			 = __version__,
	description		 = 'Reels helps identify patterns in event data and can predict target events.',
	long_description = """    Reels is a library to analyze sequences of events extracted from transactional data. These events can be
		automatically discovered or manually defined. Reels identifies events by assigning them event codes and creates clips, which are
		sequences of (code, time of occurrence) tuples for each client. Using these clips, a model can be generated to predict the time at
		which target events may occur in the future.""",
	url				 = 'https://github.com/BBVA/mercury-reels',
	license			 = 'Apache 2',
	platforms		 = ['Linux', 'MacOS', 'Windows'],
	classifiers		 = ['Programming Language :: Python :: 3',
		'License :: OSI Approved :: Apache Software License',
		'Operating System :: OS Independent'],
	keywords		 = ['event detection', 'event prediction', 'time series'],
	packages		 = ['reels'],
	package_dir		 = {'' : 'src'},
	python_requires	 = '>=3.8',
	cmdclass		 = {'build_py' : custom_build_py},
	ext_modules		 = [reels_ext]
)
