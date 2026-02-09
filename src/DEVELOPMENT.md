## How to setup the IDE and dependencies

**NOTE**: This is how the main developer feels comfortable working, if you have ideas for other IDEs, etc. feel free to share.

### Prerequisites:

  * gcc
  * make
  * catch2 (Already included in source code)
  * doxygen 1.9.5 or better (to render C++ documentation)
  * mkdocs 1.4.2 or better (to render Python documentation)
  * swig 4.2.0 or better
  * lcov and genhtml (for C++ coverage reports)
  * python 3.x with appropriate paths to python.h (see Makefile)

### Setting the IDE:

Clone and set up a development environment to work with it.

```bash
git clone https://github.com/BBVA/mercury-reels.git
cd mercury-reels/src

make
```

Make without arguments gives help. Try all the options. Everything should work assuming the tools are installed.

> [!IMPORTANT]
> You have to build the package with `make package` in order to run the tests or try new features without installing the library.
> In that case, `from reels import reels` works because `reels` is a folder in your current folder.

We recommend (and the json shared assumes) to work inside the `mercury-reels/src` folder, not the project's root folder.

#### Configuring both Python and c++ debuggers to work on any file

You can use this `launch.json` in `mercury-reels/src/.vscode`

```json
{
	"version": "0.2.0",
	"configurations": [
		{
			"name": "Python Debugger: Current File",
			"type": "debugpy",
			"request": "launch",
			"program": "${file}",
			"console": "integratedTerminal"
		},
		{
			"name": "(gdb) test",
			"type": "cppdbg",
			"request": "launch",
			"program": "${workspaceFolder}/reels_test",
			"args": [],
			"stopAtEntry": false,
			"cwd": "${workspaceFolder}",
			"environment": [],
			"MIMode": "gdb",
			"setupCommands": [
				{
					"description": "Enable pretty-printing for gdb",
					"text": "-enable-pretty-printing",
					"ignoreFailures": true
				}
			]
		}
	]
}
```

### Contributing:

If you can complete a new feature on your own (new feature, doc, tests, version bump, changelog), you can directly create a Pull
request to the master branch. Of course, you will get help via the PR. There is no proper documentation on those things so you will
have to do some guesswork.

An easier way to contribute is to create a new issue. If the idea is accepted, we will create a branch for you and start working on
how to implement it.
