CC = gcc
CXX = g++
LIBS = -lm -lrt -lpthread
CFLAGS = -g -pthread -Wall -Wno-comment -Iinclude -std=c++11

.PHONY: default all clean

default: exe

builddir:
	-mkdir -p build

libserial: builddir
	$(CXX) $(CFLAGS) -c src/libserial.cc -o build/libserial.o

main: builddir
	$(CXX) $(CFLAGS) -c main.cpp -o build/main.o

exe: libserial main
	$(CXX) build/libserial.o build/main.o -o serial2shmem $(LIBS)

clean:
	-rm -f build/*.o
	-rm -f test
