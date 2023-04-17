from setuptools import setup

from _custom_build import reels_ext


setup(
    name		= 'reels',
    version		= '1.4.1',
    description	= 'Reels helps identify patterns in event data and can predict target events.',
    packages	= ['mercury-reels'],
    ext_modules	= [reels_ext]
)
