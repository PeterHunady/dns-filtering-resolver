# file: Makefile

# Author: Peter Huňady
# Login: xhunadp00

CXX = g++
CXXFLAGS = -Wall -pedantic -std=c++17 -pthread
TARGET = dns
SOURCES = dns.cpp src/sockets.cpp src/filter.cpp src/dnsPacket.cpp src/parameters.cpp

.PHONY: all run clean test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)

test: all
	sh tests/tests.sh

