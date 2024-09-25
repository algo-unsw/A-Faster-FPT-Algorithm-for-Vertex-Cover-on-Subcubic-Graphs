 
# Makefile for randomizedMC

SUBDIRS := nauty alglib-cpp
.PHONY: all $(SUBDIRS) clean

# Compiler
CC = clang++

# Compiler flags
CFLAGS = -w -g -O3 -std=c++2b -march=native -fno-exceptions -flto -funroll-loops -pthread #-fsanitize=address

# Linker flags
LDFLAGS = -I./nauty
LDFLAGS += nauty/nauty.o nauty/nautil.o nauty/naugraph.o nauty/schreier.o nauty/naurng.o nauty/gtools.o nauty/gtnauty.o nauty/naututil.o nauty/nautinv.o nauty/gutil2.o nauty/traces.o nauty/nausparse.o -I./alglib-cpp ./alglib-cpp/*.o

#  database.cpp

EXECS = main

# Header files
HDRS = labelg.h graph.hpp lpsolver.hpp utils.hpp helperalgos.hpp branchSelector.hpp

# Source files
SRCS = graph.cpp helperalgos.cpp

# Object files
OBJS = graph.o helperalgos.o

# Default target
all: $(SUBDIRS) $(EXECS)

$(SUBDIRS):
	$(MAKE) -C $@

# graph.o: graph.cpp $(HDRS) 
# 	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)
# helperalgos.o: helperalgos.cpp $(HDRS) 
# 	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)
# Compile source files
main: main.cpp $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(SRCS) -D MAXDEGREE=3

# Clean target
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f $(EXECS) *.o




