import os, shutil


# Move notebooks inside src/reels before packaging
if os.path.exists('notebooks'):
	shutil.move('notebooks', 'src/reels/tutorials')


from setuptools import setup, find_packages, Extension


reels_ext = Extension(name					= 'reels._py_reels',
					  sources				= ['src/reels/reels.cpp', 'src/reels/py_reels_wrap.cpp'],
					  include_dirs			= ['src/reels'],
					  extra_compile_args	= ['-std=c++11', '-c', '-fpic', '-O3'])

setup_args = dict(
	packages			 = find_packages(where = 'src'),
	package_dir			 = {'' : 'src'},
	ext_modules			 = [reels_ext],
	scripts				 = ['src/test_all.py'],
	include_package_data = True,
	package_data		 = {'mypackage': ['tutorials/*', 'tutorials/data/*']}
)

setup(**setup_args)
