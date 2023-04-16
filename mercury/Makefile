help:
	@echo ""
	@echo "make [void],"
	@echo "make help    : Show this help."
	@echo ""
	@echo "make reels   : Make the command line Reels executable."
	@echo "make test    : Make the DEBUG&TEST reels executable."
	@echo "make doc_cpp : Build the reels C++ documentation."
	@echo "make doc_py  : Build the reels Python documentation."
	@echo ""
	@echo "make clean   : Clean up all files not stored in the repo."
	@echo "make package : Clean, compile all and build the mercury-dynamics package."

	@echo ""

RFLAGS := -DNDEBUG -O3
TFLAGS := -DDEBUG -DTEST -g

ifneq ("$(wildcard mode_release)","")
	CPPFLAGS := $(RFLAGS)
endif

ifneq ("$(wildcard mode_test)","")
	CPPFLAGS := $(TFLAGS)
endif

CXXFLAGS := -std=c++11 -Ireels -Icatch2

VPATH = reels catch2

reels.o: reels.h reels.cpp
reels_main.o: reels.o reels_main.cpp
reels_test.o: reels.o reels_test.cpp

mode_release:
	@echo "Switching to mode RELEASE ..."
	@make clean
	@touch mode_release
	$(eval CPPFLAGS = $(RFLAGS))

mode_test:
	@echo "Switching to mode DEBUG&TEST ..."
	@make clean
	@touch mode_test
	$(eval CPPFLAGS = $(TFLAGS))

reels: mode_release reels.o reels_main.o
	@echo "Making the command line Reels as ./reels_cli ..."
	g++ -o reels_cli reels.o reels_main.o

test: mode_test reels.o reels_test.o
	@echo "Making Reels as reels_test ..."
	g++ -o reels_test reels_test.o reels.o

.PHONY	: clean
clean:
	@echo "Cleaning up all files not stored in the repo ..."
	@rm -f *.o reels_cli reels_test mode_* errors.log .coverage
	@rm -f reels/*.so reels/*.o
	@rm -f logs/*
	@find . | grep __pycache__ | xargs rm -rf
	@find . | grep .ipynb_checkpoints | xargs rm -rf
	@rm -rf ../reference

.PHONY	: doc_cpp
doc_cpp:
	@doxygen 2>errors.log 1>/dev/null
	@cat errors.log

.PHONY	: package
package: mode_release
	g++ -c -fpic -O3 -std=c++11 -Ireels -DNDEBUG -o reels.o reels/reels.cpp
	cd reels && swig -python py_reels.i && mv py_reels.py __init__.py && cat imports.py >>__init__.py
	g++ -c -fpic -O3 reels/py_reels_wrap.c -Dpython -I/usr/include/python3.8 -I/usr/include/python3.10
	g++ -shared reels.o py_reels_wrap.o -o reels/_py_reels.so
