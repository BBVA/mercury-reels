import setuptools

	# g++ -c -fpic -O3 -std=c++11 -Ireels -DNDEBUG -o reels.o reels/reels.cpp
	# cd reels && swig -python py_reels.i && mv py_reels.py __init__.py && cat imports.py >>__init__.py
	# g++ -c -fpic -O3 reels/py_reels_wrap.c -Dpython -I/usr/include/python3.8 -I/usr/include/python3.10
	# g++ -shared reels.o py_reels_wrap.o -o reels/_py_reels.so

# https://setuptools.pypa.io/en/latest/deprecated/distutils/apiref.html#distutils.core.Extension
# https://packaging.python.org/en/latest/guides/distributing-packages-using-setuptools/
# https://ianhopkinson.org.uk/2022/02/understanding-setup-py-setup-cfg-and-pyproject-toml-in-python/

reels_ext = setuptools.Extension(name				= 'mercury.reels._py_reels',
								 sources			= ['mercury/reels/reels.cpp', 'mercury/reels/py_reels_wrap.cpp'],
								 extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])

setuptools.setup(
    name		= 'mercury-reels',
    version		= '1.4.1',
    description	= 'Reels helps identify patterns in event data and can predict target events.',
    packages	= ['mercury/reels'],
    ext_modules	= [reels_ext]
)
