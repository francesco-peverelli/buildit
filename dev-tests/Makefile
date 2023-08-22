# Simple Makefile to test buildit changes

CXX = g++
INCLUDES = -I ../include/
BUILDIT_PATH = ../build
LIBS := -L $(BUILDIT_PATH) -lbuildit 

test: fpga-test1.cpp
	$(CXX) $< $(INCLUDES) $(LIBS) -o $@
clean:
	rm -rf test

   