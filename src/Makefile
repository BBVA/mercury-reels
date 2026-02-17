help:
	@echo ""
	@echo "make [void],"
	@echo "make help     : Show this help."
	@echo ""
	@echo "make reels    : Make the command line Reels executable."
	@echo "make test     : Make the DEBUG&TEST reels executable."
	@echo "make test_cov : Make the DEBUG&TEST reels executable with coverage analysis."
	@echo "make doc_cpp  : Build the reels C++ documentation."
	@echo "make doc_py   : Build the reels Python documentation."
	@echo ""
	@echo "make clean    : Clean up all files not stored in the repo."
	@echo "make package  : Clean, compile all and build the Python reels package."
	@echo "make analyze  : Make, run, analyze the coverage and generate html report."

	@echo ""

RFLAGS := -DNDEBUG -O3
TFLAGS := -DDEBUG -DTEST -g
CFLAGS := -DDEBUG -DTEST -g --coverage -fprofile-update=atomic

ifneq ("$(wildcard mode_release)","")
	CPPFLAGS := $(RFLAGS)
endif

ifneq ("$(wildcard mode_test)","")
	CPPFLAGS := $(TFLAGS)
endif

ifneq ("$(wildcard mode_cov)","")
	CPPFLAGS := $(CFLAGS)
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

mode_cov:
	@echo "Switching to mode DEBUG&TEST with coverage analysis ..."
	@make clean
	@touch mode_cov
	$(eval CPPFLAGS = $(CFLAGS))

reels: mode_release reels.o reels_main.o
	@echo "Making the command line Reels as ./reels_cli ..."
	g++ -o reels_cli reels.o reels_main.o

test: mode_test reels.o reels_test.o
	@echo "Making Reels as reels_test ..."
	g++ -o reels_test reels_test.o reels.o

test_cov: mode_cov reels.o reels_test.o
	@echo "Making Reels as reels_cov ..."
	g++ --coverage -o reels_cov reels_test.o reels.o

.PHONY	: clean
clean:
	@echo "Cleaning up all files not stored in the repo ..."
	@rm -f *.o *.gcno *.gcda reels_cli reels_test reels_cov mode_* errors.log .coverage
	@rm -f reels/*.so reels/*.o
	@rm -f logs/*
	@rm -rf coverage.info coverage_html
	@find .. | grep __pycache__ | xargs rm -rf
	@find .. | grep .ipynb_checkpoints | xargs rm -rf
	@rm -rf ../reference ../build ../dist ../mercury_reels.egg-info mercury_reels.egg-info ../.pytest_cache ../notebooks/.ipynb*

.PHONY	: doc_cpp
doc_cpp:
	@-doxygen 2>errors.log 1>/dev/null
	@cat errors.log

.PHONY	: doc_py
doc_py:
	@-mkdocs build 2>errors.log 1>/dev/null
	@cat errors.log | grep -vE 'No type or annotation for'

.PHONY	: package
package: mode_release
	g++ -c -fpic -O3 -std=c++11 -Ireels -DNDEBUG -o reels.o reels/reels.cpp
	cd reels && swig -python -o py_reels_wrap.cpp py_reels.i && mv py_reels.py __init__.py && cat ../version.py >>__init__.py && cat imports.in >>__init__.py
	g++ -c -fpic -O3 reels/py_reels_wrap.cpp -Dpython -I/usr/include/python3.10 -I/usr/include/python3.11 -I/usr/include/python3.12 -I/usr/include/python3.13
	g++ -shared reels.o py_reels_wrap.o -o reels/_py_reels.so
	@printf "\nPython 3.x package was built locally in the folder './reels'.\n"
	@printf "\nYou can run 'import reels' for here or ./test.sh to test it!\n"

.PHONY	: analyze
analyze: test_cov
	@./reels_cov
	@lcov --capture --directory . --output-file coverage.info --rc geninfo_unexecuted_blocks=1
	@lcov --remove coverage.info '/usr/include/*' '*catch2*' --output-file coverage.info
	@genhtml coverage.info --output-directory coverage_html
	@printf "\nCoverage report generated in the folder './coverage_html'.\n"
