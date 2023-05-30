# Mercury Reels

![](https://img.shields.io/badge/-c++-black?logo=c%2B%2B&style=social)
![](https://img.shields.io/pypi/v/mercury-reels?label=latest%20pypi%20build)
[![Python 3.8](https://img.shields.io/badge/python-3.8-blue.svg)](https://www.python.org/downloads/release/python-3816/)
[![Python 3.9](https://img.shields.io/badge/python-3.9-blue.svg)](https://www.python.org/downloads/release/python-3916/)
[![Python 3.10](https://img.shields.io/badge/python-3.10-blue.svg)](https://www.python.org/downloads/release/python-31011/)
[![Python 3.11](https://img.shields.io/badge/python-3.11-blue.svg)](https://www.python.org/downloads/release/python-3113/)
[![Apache 2 license](https://shields.io/badge/license-Apache%202-blue)](http://www.apache.org/licenses/LICENSE-2.0)
[![Ask Me Anything !](https://img.shields.io/badge/Ask%20me-anything-1abc9c.svg)](https://github.com/BBVA/mercury-reels/issues)

## What is this?

### TLDR: Reels helps identify patterns in event data and can predict target events.

**Reels** is a library to analyze sequences of events extracted from transactional data. These events can be automatically discovered
or manually defined. **Reels** identifies events by assigning them **event codes** and creates **clips**, which are sequences of
**(code, time of occurrence)** tuples for each **client**. Using these clips, a model can be generated to predict the time at which
**target events** may occur in the future.

### What problems is REELS good for?

Reels was born to analyze web navigation transactional data. It has natural applications in cybersecurity and everywhere where predicting
**events** or scoring **risk of events** based on past events makes sense. The definition of **relevant event** may be discovered from
transactional data or is established as business domain knowledge. It can also be semi-automated using Reels event optimizer to iterate
and learn how events predict a target.

### What data size can reels tackle?

Reels is a C++ implementation with a Python interface. It is single threaded and can seamlessly operate over millions of clients and
billions of records with hundreds of thousands of events. To further parallelize, it can partition the data by dividing the set of
clients into smaller subsets and operate on each subset independently.

### I already have time series and sequence prediction tools, why would I need another one?

  * Reels is oriented towards **events** (as opposed to continuous variables like a price).
  * Even when your problem is better seen as time series, Reels predictions can be used as features to boost another model.
  * Supports manual, full automatic or assisted definition of what relevant (predictive) events are.
  * Predicts a target event within or outside the transactional dataset.
  * Highly efficient C++ implementation.
  * 100% pythonic interface: objects are serializable, use iterators, interfaces with pandas and pyspark.

## The Python API

Reels is implemented in four classes.

### The Events class
<img src="https://raw.githubusercontent.com/BBVA/mercury-reels/master/notebooks/images/events.png" width = "640" />

  * [Python API](https://bbva.github.io/mercury-reels/reference/python/reference/events/)
  * [C++ API](https://bbva.github.io/mercury-reels/reference/html/classreels_1_1Events.html)

### The Clients class
<img src="https://raw.githubusercontent.com/BBVA/mercury-reels/master/notebooks/images/clients.png" width = "640" />

  * [Python API](https://bbva.github.io/mercury-reels/reference/python/reference/clients/)
  * [C++ API](https://bbva.github.io/mercury-reels/reference/html/classreels_1_1Clients.html)

### The Clips class
<img src="https://raw.githubusercontent.com/BBVA/mercury-reels/master/notebooks/images/clips.png" width = "640" />

  * [Python API](https://bbva.github.io/mercury-reels/reference/python/reference/clips/)
  * [C++ API](https://bbva.github.io/mercury-reels/reference/html/classreels_1_1Clips.html)

### The Targets class
<img src="https://raw.githubusercontent.com/BBVA/mercury-reels/master/notebooks/images/targets.png" width = "640" />

  * [Python API](https://bbva.github.io/mercury-reels/reference/python/reference/targets/)
  * [C++ API](https://bbva.github.io/mercury-reels/reference/html/classreels_1_1Targets.html)

## Try it without any installation on Google Colab

  * **Introductory**: A walk through Reels [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/BBVA/mercury-reels/blob/master/notebooks/reels_walkthrough_colab.ipynb)
  * **Advanced**: Event optimization -- How to do assisted event discovery [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/BBVA/mercury-reels/blob/master/notebooks/reels_event_optimization_colab.ipynb)

## Install

```bash
pip install mercury-reels
```

## Clone and set up a development environment to work with it

To work with Reels command line or develop Reels, you can set up an environment with git, gcc, make and the following tools:

  * catch2 (Already included in source code)
  * doxygen 1.9.5 or better (to render C++ documentation)
  * mkdocs 1.4.2 or better (to render Python documentation)
  * swig 4.0.2
  * python 3.x with appropriate paths to python.h (see Makefile)

```bash
git clone https://github.com/BBVA/mercury-reels.git
cd mercury-reels/src

make
```

Make without arguments gives help. Try all the options. Everything should work assuming the tools are installed.

## Documentation

  * [Python API](https://bbva.github.io/mercury-reels/reference/python/index.html)
  * [C++ API](https://bbva.github.io/mercury-reels/reference/html/index.html)

## License

```text
                         Apache License
                   Version 2.0, January 2004
                http://www.apache.org/licenses/

     Copyright 2022-23, Banco de Bilbao Vizcaya Argentaria, S.A.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0
```