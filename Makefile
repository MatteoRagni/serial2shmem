CC = gcc
CXX = g++
LIBS = -lm -lrt
CFLAGS = -g -Wall -Iinclude -std=c++11

.PHONY: default all clean

default: exe

libserial:
	$(CXX) $(CFLAGS) -c src/libserial.cc -o build/libserial.o

main:
	$(CXX) $(CFLAGS) -c main.cpp -o build/main.o

exe: libserial main
	$(CXX) build/libserial.o build/main.o -o test $(LIBS)

clean:
	-rm -f build/*.o
	-rm -f test
