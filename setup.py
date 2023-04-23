import setuptools

	# g++ -c -fpic -O3 -std=c++11 -Ireels -DNDEBUG -o reels.o reels/reels.cpp
	# cd reels && swig -python py_reels.i && mv py_reels.py __init__.py && cat imports.py >>__init__.py
	# g++ -c -fpic -O3 reels/py_reels_wrap.c -Dpython -I/usr/include/python3.8 -I/usr/include/python3.10
	# g++ -shared reels.o py_reels_wrap.o -o reels/_py_reels.so

# https://setuptools.pypa.io/en/latest/deprecated/distutils/apiref.html#distutils.core.Extension
# https://packaging.python.org/en/latest/guides/distributing-packages-using-setuptools/
# https://ianhopkinson.org.uk/2022/02/understanding-setup-py-setup-cfg-and-pyproject-toml-in-python/

reels_ext = setuptools.Extension(name				= 'src.reels._py_reels',
								 sources			= ['src/reels/reels.cpp', 'src/reels/py_reels_wrap.cpp'],
								 extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])

setuptools.setup(
    name			 = 'mercury-reels',
    version			 = '1.4.1',
    description		 = 'Reels helps identify patterns in event data and can predict target events.',
	long_description = """
Reels is a library to analyze sequences of events extracted from transactional data. These events can be automatically discovered
or manually defined. Reels identifies events by assigning them event codes and creates clips, which are sequences of
(code, time of occurrence) tuples for each client. Using these clips, a model can be generated to predict the time at which
target events may occur in the future.
""",
	url				 = 'https://github.com/BBVA/mercury-reels',
	license			 = '',
	classifiers		 = ['Programming Language :: Python :: 3',
		'License :: OSI Approved :: Apache Software License',
		'Operating System :: OS Independent']
	keywords		 = ['event detection', 'event prediction', 'time series']
    packages		 = ['reels'],
	py_modules		 = ['_custom_build'],
	python_requires	 = '>=3.8',
    ext_modules		 = [reels_ext]
)
