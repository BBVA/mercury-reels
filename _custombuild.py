# _custom_build.py

import setuptools

from .setup import reels_ext


class build_py(setuptools.command.build_py):

    def run(self):
        self.run_command('build_ext')

        return super().run()


    def initialize_options(self):
        super().initialize_options()

        if self.distribution.ext_modules == None:
            self.distribution.ext_modules = []

        self.distribution.ext_modules.append(reels_ext)
