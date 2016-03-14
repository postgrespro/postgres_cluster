CXX=g++
CXXFLAGS=-g -Wall -O2 -pthread

all: dtmbench

dtmbench: dtmbench.cpp
	$(CXX) $(CXXFLAGS) -o dtmbench dtmbench.cpp -lpqxx

clean:
	rm -f dtmbench