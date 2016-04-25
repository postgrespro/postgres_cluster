CXX=g++
CXXFLAGS=-g -Wall -O0 -pthread 

all: dtmbench dtmacid

dtmbench: dtmbench.cpp
	$(CXX) $(CXXFLAGS) -o dtmbench dtmbench.cpp -lpqxx -lpq

dtmacid: dtmacid.cpp
	$(CXX) $(CXXFLAGS) -o dtmacid dtmacid.cpp -lpqxx -lpq

clean:
	rm -f dtmbench dtmacid
