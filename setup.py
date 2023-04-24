import setuptools

from setuptools.command.build_py import build_py as _build_py

reels_ext = setuptools.Extension(name				= 'src.reels._py_reels',
								 sources			= ['src/reels/reels.cpp', 'src/reels/py_reels_wrap.cpp'],
								 extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])


class custom_build_py(_build_py):

	def run(self):
		self.run_command('build_ext')

		ret = super().run()

# TODO: Find the paths and names in a better way

		self.move_file('build/lib.linux-x86_64-3.10/src/reels/_py_reels.cpython-310-x86_64-linux-gnu.so',
					   'build/lib.linux-x86_64-3.10/reels/_py_reels.cpython-310-x86_64-linux-gnu.so')

		return ret


	# def initialize_options(self):
	# 	super().initialize_options()

	# 	if self.distribution.ext_modules == None:
	# 		self.distribution.ext_modules = []

	# 	self.distribution.ext_modules.append(reels_ext)


	# g++ -c -fpic -O3 -std=c++11 -Ireels -DNDEBUG -o reels.o reels/reels.cpp
	# cd reels && swig -python py_reels.i && mv py_reels.py __init__.py && cat imports.py >>__init__.py
	# g++ -c -fpic -O3 reels/py_reels_wrap.c -Dpython -I/usr/include/python3.8 -I/usr/include/python3.10
	# g++ -shared reels.o py_reels_wrap.o -o reels/_py_reels.so

# https://setuptools.pypa.io/en/latest/deprecated/distutils/apiref.html#distutils.core.Extension
# https://packaging.python.org/en/latest/guides/distributing-packages-using-setuptools/
# https://ianhopkinson.org.uk/2022/02/understanding-setup-py-setup-cfg-and-pyproject-toml-in-python/

setuptools.setup(
    name			 = 'mercury-reels',
    version			 = '1.4.1',
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
