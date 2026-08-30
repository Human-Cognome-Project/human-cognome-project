CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
INC       = -I/usr/include/postgresql
LIBS      = -lpq

gloss-kernel: main.cpp GlossKernel.cpp GlossKernel.h md5.h
	$(CXX) $(CXXFLAGS) $(INC) -o $@ main.cpp GlossKernel.cpp $(LIBS)

clean:
	rm -f gloss-kernel

.PHONY: clean
