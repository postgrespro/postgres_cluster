CXX=g++
CXXFLAGS=-g -Wall -O0 -pthread

all: dtmbench

dtmbench: dtmbench.cpp
	$(CXX) $(CXXFLAGS) -o dtmbench dtmbench.cpp -lpqxx

clean:
	rm -f dtmbench
